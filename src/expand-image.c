/* cfengine for GNU
 
        Copyright (C) 1995
        Free Software Foundation, Inc.
 
   This file is part of GNU cfengine - written and maintained 
   by Mark Burgess, Dept of Computing and Engineering, Oslo College,
   Dept. of Theoretical physics, University of Oslo
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/
 

#include "cf.defs.h"
#include "cf.extern.h"

/*****************************************************************************/
/*                                                                           */
/* File: expand-image.c                                                      */
/*                                                                           */
/*****************************************************************************/

void RecursiveImage(struct Image *ip,char *from,char *to,int maxrecurse)

{ struct stat statbuf, deststatbuf;
  char newfrom[CF_BUFSIZE];
  char newto[CF_BUFSIZE];
  int save_uid, save_gid, succeed;
  struct Item *namecache = NULL;
  struct Item *ptr;
  struct cfdirent *dirp;
  CFDIR *dirh;

if (maxrecurse == 0)  /* reached depth limit */
   {
   Debug2("MAXRECURSE ran out, quitting at level %s with endlist = %d\n",from,ip->next);
   return;
   }

Debug2("RecursiveImage(%s,lev=%d,next=%d)\n",from,maxrecurse,ip->next);

if (IgnoreFile(from,"",ip->ignores))
   {
   Verbose("Ignoring directory %s\n",from);
   return;
   }

if (strlen(from) == 0)     /* Check for root dir */
   {
   from = "/";
   }

  /* Check that dest dir exists before starting */

strncpy(newto,to,CF_BUFSIZE-2);
AddSlash(newto);

if (strcmp(ip->action,"warn") != 0)
   {
   if (! MakeDirectoriesFor(newto,ip->forcedirs))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to make directory for %s in copy: %s to %s\n",newto,ip->path,ip->destination);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   }

if ((dirh = cfopendir(from,ip)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"copy can't open directory [%s]\n",from);
   CfLog(cfverbose,OUTPUT,"");
   return;
   }

for (dirp = cfreaddir(dirh,ip); dirp != NULL; dirp = cfreaddir(dirh,ip))
   {
   if (!SensibleFile(dirp->d_name,from,ip))
      {
      continue;
      }

   if (ip->purge == 'y') /* Do not purge this file */
      {
      AppendItem(&namecache,dirp->d_name,NULL);
      }

   if (IgnoreFile(from,dirp->d_name,ip->ignores))
      {
      continue;
      }

   strncpy(newfrom,from,CF_BUFSIZE-2);                             /* Assemble pathname */
   AddSlash(newfrom);
   strncpy(newto,to,CF_BUFSIZE-2);
   AddSlash(newto);

   if (BufferOverflow(newfrom,dirp->d_name))
      {
      printf(" culprit: RecursiveImage\n");
      cfclosedir(dirh);
      return;
      }

   strncat(newfrom,dirp->d_name,CF_BUFSIZE-2);

   if (BufferOverflow(newto,dirp->d_name))
      {
      printf(" culprit: RecursiveImage\n");
      cfclosedir(dirh);
      return;
      }

   strcat(newto,dirp->d_name);

   if (TRAVLINKS || ip->linktype == 'n')
      {
      /* No point in checking if there are untrusted symlinks here,
         since this is from a trusted source, by defintion */
      
      if (cfstat(newfrom,&statbuf,ip) == -1)
         {
         Verbose("%s: (Can't stat %s)\n",VPREFIX,newfrom);
         continue;
         }
      }
   else
      {
      if (cflstat(newfrom,&statbuf,ip) == -1)
         {
         Verbose("%s: (Can't stat %s)\n",VPREFIX,newfrom);
         continue;
         }
      }

   if ((ip->xdev =='y') && DeviceChanged(statbuf.st_dev))
      {
      Verbose("Skipping %s on different device\n",newfrom);
      continue;
      }
   
   if (!FileObjectFilter(newfrom,&statbuf,ip->filters,image))
      {
      continue;
      }

   if (!S_ISDIR(statbuf.st_mode) && IgnoredOrExcluded(image,dirp->d_name,ip->inclusions,ip->exclusions))
      {
      continue;
      }

   if (!S_ISDIR(statbuf.st_mode))
      {
      succeed = 0;

      if (IsItemIn(VEXCLUDECACHE,newto))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Skipping single-copied file %s class %s\n",newto,ip->classes);
         CfLog(cfverbose,OUTPUT,"");
         continue;
         }
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      if (TRAVLINKS)
         {
         CfLog(cfverbose,"Traversing directory links during copy is too dangerous, pruned","");
         continue;
         }
      
      memset(&deststatbuf,0,sizeof(struct stat));
      save_uid = (ip->uid)->uid;
      save_gid = (ip->gid)->gid;
      
      if ((ip->uid)->uid == (uid_t)-1)          /* Preserve uid and gid  */
         {
         (ip->uid)->uid = statbuf.st_uid;
         }
      
      if ((ip->gid)->gid == (gid_t)-1)
         {
         (ip->gid)->gid = statbuf.st_gid;
         }
      
      if (stat(newto,&deststatbuf) == -1)
         {
         mkdir(newto,statbuf.st_mode);
         if (stat(newto,&deststatbuf) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n",newto);
            CfLog(cferror,OUTPUT,"stat");
            continue;
            }
         }
      
      CheckCopiedFile(ip->cf_findertype,newto,&deststatbuf,&statbuf,ip);
      
      (ip->uid)->uid = save_uid;
      (ip->gid)->gid = save_gid;
      
      Verbose("Opening %s->%s\n",newfrom,newto);
      RecursiveImage(ip,newfrom,newto,maxrecurse-1);
      }
   else
      {
      CheckImage(newfrom,newto,ip);
      }
   }

if (ip->purge == 'y')
   {
   /* inclusions not exclusions, since exclude from purge means include */
   PurgeFiles(namecache,to,ip->inclusions); 
   DeleteItemList(namecache);
   }
 
DeleteCompressedArray(ip->inode_cache);

ip->inode_cache = NULL;

cfclosedir(dirh);
}

/*********************************************************************/

void CheckHomeImages(struct Image *ip)

{ DIR *dirh, *dirh2;
  struct dirent *dirp, *dirp2;
  char username[CF_MAXVARSIZE];
  char homedir[CF_BUFSIZE],dest[CF_BUFSIZE];
  struct passwd *pw;
  struct group *gr;
  struct stat statbuf;
  struct Item *itp;
  int request_uid = ip->uid->uid;  /* save if -1 */
  int request_gid = ip->gid->gid;  /* save if -1 */

if (!MountPathDefined())
   {
   printf("%s:  mountpattern is undefined\n",VPREFIX);
   return;
   }

if (cfstat(ip->path,&statbuf,ip) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Master file %s doesn't exist for copying\n",ip->path);
   CfLog(cferror,OUTPUT,"");
   return;
   }

for (itp = VMOUNTLIST; itp != NULL; itp=itp->next)
   {
   if (IsExcluded(itp->classes))
      {
      continue;
      }
   
   if ((dirh = opendir(itp->name)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",itp->name);
      CfLog(cfverbose,OUTPUT,"opendir");
      return;
      }

   for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,itp->name,NULL))
         {
         continue;
         }

      strcpy(homedir,itp->name);
      AddSlash(homedir);
      strcat(homedir,dirp->d_name);

      if (! IsHomeDir(homedir))
         {
         continue;
         }

      if ((dirh2 = opendir(homedir)) == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",homedir);
         CfLog(cfverbose,OUTPUT,"opendir");
         return;
         }
      
      for (dirp2 = readdir(dirh2); dirp2 != NULL; dirp2 = readdir(dirh2))
         {
         if (!SensibleFile(dirp2->d_name,homedir,NULL))
            {
            continue;
            }

         strcpy(username,dirp2->d_name);
         strcpy(dest,homedir);
         AddSlash(dest);
         strcat(dest,dirp2->d_name);
         
         if (strlen(ip->destination) > 4)
            {
            AddSlash(dest);
            if (strlen(ip->destination) < 6)
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Error in home/copy to %s",ip->destination);
               CfLog(cferror,OUTPUT,"");
               return;
               }
            else
               {
               strcat(dest,(ip->destination)+strlen("home/"));
               }
            }
         
         if (request_uid == -1)
            {
            if ((pw = getpwnam(username)) == NULL)
               {
               Debug2("cfengine: directory corresponds to no user %s - ignoring\n",username);
               continue;
               }
            else
               {
               Debug2("(Setting user id to %s)\n",pw->pw_name);
               }
            
            ip->uid->uid = pw->pw_uid;
            }
         
         if (request_gid == -1)
            {
            if ((pw = getpwnam(username)) == NULL)
               {
               Debug2("cfengine: directory corresponds to no user %s - ignoring\n",username);
               continue;
               }
            
            if ((gr = getgrgid(pw->pw_gid)) == NULL)
               {
               Debug2("cfengine: no group defined for group id %d - ignoring\n",pw->pw_gid);
               continue;
               }
            else
               {
               Debug2("(Setting group id to %s)\n",gr->gr_name);
               }

            ip->gid->gid = gr->gr_gid;
            }

         CheckImage(ip->path,dest,ip);
         }
      closedir(dirh2);
      }
   closedir(dirh);
   }
}


