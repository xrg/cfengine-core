/* 

        Copyright (C) 1995-
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


/*******************************************************************/
/*                                                                 */
/* File Image copying                                              */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/
/* Level 1                                                           */
/*********************************************************************/

void RecursiveImage(ip,from,to,maxrecurse)

struct Image *ip;
char *from, *to;
int maxrecurse;

{ struct stat statbuf, deststatbuf;
  char newfrom[bufsize];
  char newto[bufsize];
  int save_uid, save_gid, succeed;
  struct Item *namecache = NULL;
  struct Item *ptr, *ptr1;
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

strncpy(newto,to,bufsize-2);
AddSlash(newto);

if (! MakeDirectoriesFor(newto,ip->forcedirs))
   {
   snprintf(OUTPUT,bufsize*2,"Unable to make directory for %s in copy: %s to %s\n",newto,ip->path,ip->destination);
   CfLog(cferror,OUTPUT,"");
   return;
   }

if ((dirh = cfopendir(from,ip)) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"copy can't open directory [%s]\n",from);
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

   strncpy(newfrom,from,bufsize-2);                             /* Assemble pathname */
   AddSlash(newfrom);
   strncpy(newto,to,bufsize-2);
   AddSlash(newto);

   if (BufferOverflow(newfrom,dirp->d_name))
      {
      printf(" culprit: RecursiveImage\n");
      cfclosedir(dirh);
      return;
      }

   strncat(newfrom,dirp->d_name,bufsize-2);

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
      for (ptr = VEXCLUDECACHE; ptr != NULL; ptr=ptr->next)
          {
          if ((strncmp(ptr->name,newto,strlen(newto)+1) == 0) && (strncmp(ptr->classes,ip->classes,strlen(ip->classes)+1) == 0))
             {
             succeed = 1;
             }
          }

      if (succeed)
         {
         snprintf(OUTPUT,bufsize*2,"Skipping excluded file %s class %s\n",newto,ip->classes);
         CfLog(cfverbose,OUTPUT,"");
         continue;
         }
      else
         {
         Debug2("file %s class %s was not excluded\n",newto,ip->classes);
         }
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      if (TRAVLINKS || ip->linktype == 'n')
	 {
	 CfLog(cfverbose,"Traversing directory links during copy is too dangerous, pruned","");
	 continue;
	 }
      
      bzero(&deststatbuf,sizeof(struct stat));
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
	 }

      CheckCopiedFile(newto,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&statbuf,NULL,ip->acl_aliases);

      (ip->uid)->uid = save_uid;
      (ip->gid)->gid = save_gid;

      Verbose("Opening %s/%s\n",newfrom,newto);
      RecursiveImage(ip,newfrom,newto,maxrecurse-1);
      }
   else
      {
      CheckImage(newfrom,newto,ip);
      }
   }

if (ip->purge == 'y')
   {
   PurgeFiles(namecache,to,ip->inclusions); /* inclusions not exclusions, since exclude
					       from purge means include */
   DeleteItemList(namecache);
   }
 
DeleteCompressedArray(ip->inode_cache);

ip->inode_cache = NULL;

cfclosedir(dirh);
}

/*********************************************************************/

void CheckHomeImages(ip)

struct Image *ip;

{ DIR *dirh, *dirh2;
  struct dirent *dirp, *dirp2;
  char username[maxvarsize];
  char homedir[bufsize],dest[bufsize];
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
   snprintf(OUTPUT,bufsize*2,"Master file %s doesn't exist for copying\n",ip->path);
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
      snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",itp->name);
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
	 snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",homedir);
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
	       snprintf(OUTPUT,bufsize*2,"Error in home/copy to %s",ip->destination);
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

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void CheckImage(source,destination,ip)

char *source;
char *destination;
struct Image *ip;

{ CFDIR *dirh;
  char sourcefile[bufsize];
  char sourcedir[bufsize];
  char destdir[bufsize];
  char destfile[bufsize];
  struct stat sourcestatbuf, deststatbuf;
  struct cfdirent *dirp;
  int save_uid, save_gid, found;
  
Debug2("CheckImage (source=%s destination=%s)\n",source,destination);

if (ip->linktype == 'n')
   {
   found = cfstat(source,&sourcestatbuf,ip);
   }
else
   {
   found = cflstat(source,&sourcestatbuf,ip);
   }
 
if (found == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",source);
   CfLog(cferror,OUTPUT,"");
   FlushClientCache(ip);
   return;
   }

if (sourcestatbuf.st_nlink > 1)    /* Preserve hard link structure when copying */
   {
   RegisterHardLink(sourcestatbuf.st_ino,destination,ip);
   }

save_uid = (ip->uid)->uid;
save_gid = (ip->gid)->gid;

if ((ip->uid)->uid == (uid_t)-1)          /* Preserve uid and gid  */
   {
   (ip->uid)->uid = sourcestatbuf.st_uid;
   }

if ((ip->gid)->gid == (gid_t)-1)
   {
   (ip->gid)->gid = sourcestatbuf.st_gid;
   }

if (S_ISDIR(sourcestatbuf.st_mode))
   {
   strcpy(sourcedir,source);
   AddSlash(sourcedir);
   strcpy(destdir,destination);
   AddSlash(destdir);

   if ((dirh = cfopendir(sourcedir,ip)) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",sourcedir);
      CfLog(cfverbose,OUTPUT,"opendir");
      FlushClientCache(ip);
      (ip->uid)->uid = save_uid;
      (ip->gid)->gid = save_gid;
      return;
      }

   /* Now check any overrides */

   CheckCopiedFile(destdir,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);
   
   for (dirp = cfreaddir(dirh,ip); dirp != NULL; dirp = cfreaddir(dirh,ip))
      {
      if (!SensibleFile(dirp->d_name,sourcedir,ip))
         {
         continue;
         }

      strcpy(sourcefile, sourcedir);
      
      if (BufferOverflow(sourcefile,dirp->d_name))
	 {
	 FatalError("Culprit: CheckImage");
	 }
  
      strcat(sourcefile, dirp->d_name);
      strcpy(destfile, destdir);
      
      if (BufferOverflow(destfile,dirp->d_name))
	 {
	 FatalError("Culprit: CheckImage");
	 }
      
      strcat(destfile, dirp->d_name);

      if (cflstat(sourcefile,&sourcestatbuf,ip) == -1)
         {
         printf("%s: Can't stat %s\n",VPREFIX,sourcefile);
	 FlushClientCache(ip);       
	 (ip->uid)->uid = save_uid;
	 (ip->gid)->gid = save_gid;
         return;
	 }

      ImageCopy(sourcefile,destfile,sourcestatbuf,ip);
      }

   cfclosedir(dirh);
   FlushClientCache(ip);
   (ip->uid)->uid = save_uid;
   (ip->gid)->gid = save_gid;
   return;
   }

strcpy(sourcefile,source);
strcpy(destfile,destination);

ImageCopy(sourcefile,destfile,sourcestatbuf,ip);
(ip->uid)->uid = save_uid;
(ip->gid)->gid = save_gid;
FlushClientCache(ip);
}

/*********************************************************************/

void PurgeFiles(filelist,directory,inclusions)

struct Item *filelist;
char *directory;
struct Item *inclusions;

{ DIR *dirh;
  struct stat statbuf; 
  struct dirent *dirp;
  char filename[bufsize];

Debug("PurgeFiles(%s)\n",directory);

 /* If we purge with no authentication we wipe out EVERYTHING */ 

 if (strlen(directory) < 2)
    {
    snprintf(OUTPUT,bufsize,"Purge of %s denied -- too dangerous!",directory);
    CfLog(cferror,OUTPUT,"");
    return;
    }
 
 if (!AUTHENTICATED)
    {
    Verbose("Not purging %s - no verified contact with server\n",directory);
    return;
    }

 if ((dirh = opendir(directory)) == NULL)
    {
    snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",directory);
    CfLog(cfverbose,OUTPUT,"cfopendir");
    return;
    }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,directory,NULL))
      {
      continue;
      }
   
   if (! IsItemIn(filelist,dirp->d_name))
      {
      strncpy(filename,directory,bufsize-2);
      AddSlash(filename);
      strncat(filename,dirp->d_name,bufsize-2);
      
      Debug("Checking purge %s..\n",filename);
      
      if (DONTDO)
	 {
	 printf("Need to purge %s from copy dest directory\n",filename);
	 }
      else
	 {
	 snprintf(OUTPUT,bufsize*2,"Purging %s in copy dest directory\n",filename);
	 CfLog(cfinform,OUTPUT,"");
	 
	 if (lstat(filename,&statbuf) == -1)
	    {
	    snprintf(OUTPUT,bufsize*2,"Couldn't stat %s while purging\n",filename);
	    CfLog(cfverbose,OUTPUT,"stat");
	    }
	 
	 if (S_ISDIR(statbuf.st_mode))
	    {
	    struct Tidy tp;
	    struct TidyPattern tpat;
	    
	    tp.maxrecurse = 2;
	    tp.done = 'n';
	    tp.tidylist = &tpat;
	    tp.next = NULL;
	    tp.path = filename;
	    tp.exclusions = inclusions; /* exclude means don't purge, i.e. include here */
	    tp.ignores = NULL;

	    tpat.filters = NULL;	    
	    tpat.recurse = INFINITERECURSE;
	    tpat.age = 0;
	    tpat.size = 0;
	    tpat.pattern = strdup("*");
	    tpat.classes = strdup("any");
	    tpat.defines = NULL;
	    tpat.elsedef = NULL;
	    tpat.dirlinks = 'y';
	    tpat.travlinks = 'n';
	    tpat.rmdirs = 'y';
	    tpat.searchtype = 'a';
	    tpat.log = 'd';
	    tpat.inform = 'd';
	    tpat.next = NULL;
	    RecursiveTidySpecialArea(filename,&tp,INFINITERECURSE,&statbuf);
	    free(tpat.pattern);
	    free(tpat.classes);

	    chdir("..");
	    
	    if (rmdir(filename) == -1)
	       {
	       snprintf(OUTPUT,bufsize*2,"Couldn't remove directory %s while purging\n",filename);
	       CfLog(cfverbose,OUTPUT,"rmdir");
	       }
	    
	    continue;
	    }
	 else if (unlink(filename) == -1)
	    {
	    snprintf(OUTPUT,bufsize*2,"Couldn't unlink %s while purging\n",filename);
	    CfLog(cfverbose,OUTPUT,"");
	    }
	 }
      }
   }
 
closedir(dirh);
}


/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void ImageCopy(sourcefile,destfile,sourcestatbuf,ip)

char *sourcefile;
char *destfile;
struct stat sourcestatbuf;
struct Image *ip;

{ char linkbuf[bufsize], *lastnode;
  struct stat deststatbuf;
  struct Link empty;
  struct Item *ptr, *ptr1;
  int succeed, silent = false, enforcelinks;
  mode_t srcmode = sourcestatbuf.st_mode;
  int ok_to_copy = false, found;

Debug2("ImageCopy(%s,%s,+%o,-%o)\n",sourcefile,destfile,ip->plus,ip->minus);

if ((strcmp(sourcefile,destfile) == 0) && (strcmp(ip->server,"localhost") == 0))
   {
   snprintf(OUTPUT,bufsize*2,"Image loop: file/dir %s copies to itself",sourcefile);
   CfLog(cfinform,OUTPUT,"");
   return;
   }
empty.defines = NULL;
empty.elsedef = NULL;
if (IgnoredOrExcluded(image,sourcefile,ip->inclusions,ip->exclusions))
   {
   return;
   }

succeed = 0;
for (ptr = VEXCLUDECACHE; ptr != NULL; ptr=ptr->next)
    {
    if ((strncmp(ptr->name,destfile,strlen(destfile)+1) == 0) && (strncmp(ptr->classes,ip->classes,strlen(ip->classes)+1) == 0))
       {
       succeed = 1;
       }
    }

if (succeed)
   {
   snprintf(OUTPUT,bufsize*2,"Skipping excluded file %s class %s\n",destfile,ip->classes);
   CfLog(cfverbose,OUTPUT,"");
   return;
   }
else
   {
   Debug2("file %s class %s was not excluded\n",destfile,ip->classes);
   }


if (ip->linktype != 'n')
   {
   lastnode=ReadLastNode(sourcefile);
   if (IsWildItemIn(VLINKCOPIES,lastnode) || IsWildItemIn(ip->symlink,lastnode))
      {
      Verbose("cfengine: copy item %s marked for linking instead\n",sourcefile);
      enforcelinks = ENFORCELINKS;
      ENFORCELINKS = true;
      
      switch (ip->linktype)
	 {
	 case 's':
	     succeed = LinkFiles(destfile,sourcefile,NULL,NULL,NULL,true,&empty);
	     break;
	 case 'r':
	     succeed = RelativeLink(destfile,sourcefile,NULL,NULL,NULL,true,&empty);
	     break;
	 case 'a':
	     succeed = AbsoluteLink(destfile,sourcefile,NULL,NULL,NULL,true,&empty);
	     break;
	 default:
	     printf("%s: internal error, link type was [%c] in ImageCopy\n",VPREFIX,ip->linktype);
	     return;
	 }

      if (succeed)
	 {
	 ENFORCELINKS = enforcelinks;
	 lstat(destfile,&deststatbuf);
	 CheckCopiedFile(destfile,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);
	 }
      
      return;
      }
   }

if (strcmp(ip->action,"silent") == 0)
   {
   silent = true;
   }

bzero(linkbuf,bufsize);

found = lstat(destfile,&deststatbuf);

if (found != -1)
   {
   if ((S_ISLNK(deststatbuf.st_mode) && (ip->linktype == 'n')) || (S_ISLNK(deststatbuf.st_mode)  && ! S_ISLNK(sourcestatbuf.st_mode)))
      {
      if (!S_ISLNK(sourcestatbuf.st_mode) && (ip->typecheck == 'y'))
	 {
	 printf("%s: image exists but destination type is silly (file/dir/link doesn't match)\n",VPREFIX);
 	 printf("%s: source=%s, dest=%s\n",VPREFIX,sourcefile,destfile);
 	 return;
	 }
      if (DONTDO)
	 {
	 Verbose("Need to remove old symbolic link %s to make way for copy\n",destfile);
	 }
      else 
	 {
	 if (unlink(destfile) == -1)
	    {
	    snprintf(OUTPUT,bufsize*2,"Couldn't remove link %s",destfile);
	    CfLog(cferror,OUTPUT,"unlink");
	    return;
	    }
	 Verbose("Removing old symbolic link %s to make way for copy\n",destfile);
	 found = -1;
	 }
      } 
   }

if (ip->size != cfnosize)
   {
   switch (ip->comp)
      {
      case '<':  if (sourcestatbuf.st_size > ip->size)
	 {
	 snprintf(OUTPUT,bufsize*2,"Source file %s is > %d bytes in copy (omitting)\n",sourcefile,ip->size);
	 CfLog(cfinform,OUTPUT,"");
	 return;
	 }
      break;
      
      case '=':
	  if (sourcestatbuf.st_size != ip->size)
	     {
	     snprintf(OUTPUT,bufsize*2,"Source file %s is not %d bytes in copy (omitting)\n",sourcefile,ip->size);
	     CfLog(cfinform,OUTPUT,"");
	     return;
	     }
      break;
      
      default:
	  if (sourcestatbuf.st_size < ip->size)
	     {
	     Silent(OUTPUT,"Source file %s is < %d bytes in copy (omitting)\n",sourcefile,ip->size);
	     CfLog(cfinform,OUTPUT,"");
	     return;
	     }
	  break;;
      }
   }
 
 
if (found == -1)
   {
   if (strcmp(ip->action,"warn") == 0)
      {
      snprintf(OUTPUT,bufsize*2,"Image file %s is non-existent\n",destfile);
      CfLog(cfinform,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"(should be copy of %s)\n",sourcefile);
      CfLog(cfinform,OUTPUT,"");
      return;
      }

   if (S_ISREG(srcmode))
      {
      snprintf(OUTPUT,bufsize*2,"%s wasn't at destination (copying)",destfile);
      if (DONTDO)
	 {
	 printf("Need this: %s\n",OUTPUT);
	 return;
	 }

      CfLog(cfverbose,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"Copying from %s:%s\n",ip->server,sourcefile);
      CfLog(cfinform,OUTPUT,"");

      if (CopyReg(sourcefile,destfile,sourcestatbuf,deststatbuf,ip))
         {
	 stat(destfile,&deststatbuf);
	 CheckCopiedFile(destfile,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);
	 AddMultipleClasses(ip->defines);

	 for (ptr = VAUTODEFINE; ptr != NULL; ptr=ptr->next)
	     {
	     if (strncmp(ptr->name,destfile,strlen(destfile)+1) == 0) 
		{
		snprintf(OUTPUT,bufsize*2,"cfengine: image %s was set to autodefine %s\n",ptr->name,ptr->classes);
		CfLog(cfinform,OUTPUT,"");
		AddMultipleClasses(ptr->classes);
		}
	     }
	 
	 if (VSINGLECOPY != NULL)
	    {
	    succeed = 1;
	    }
	 else
	    {
	    succeed = 0;
	    }

	 for (ptr = VSINGLECOPY; ptr != NULL; ptr=ptr->next)
	    {
	    if ((strcmp(ptr->name,"on") != 0) && (strcmp(ptr->name,"true") != 0))
	       {
	       continue;
	       }
	    
	    if (strncmp(ptr->classes,ip->classes,strlen(ip->classes)+1) == 0)
	       {
	       for (ptr1 = VEXCLUDECACHE; ptr1 != NULL; ptr1=ptr1->next)
		  {
		  if ((strncmp(ptr1->name,destfile,strlen(destfile)+1) == 0) && (strncmp(ptr1->classes,ip->classes,strlen(ip->classes)+1) == 0))
		     {
		     succeed = 0;		
		     }
		  }
	       }
	    }
	 
         if (succeed)
            {
            Debug("Appending image %s class %s to singlecopy list\n",destfile,ip->classes);
            AppendItem(&VEXCLUDECACHE,destfile,ip->classes);
            }
         }
      else
	 {
	 AddMultipleClasses(ip->failover);
	 }

      Debug2("Leaving ImageCopy\n");
      return;
      }

   if (S_ISFIFO (srcmode))
      {
#ifdef HAVE_MKFIFO
      if (DONTDO)
         {
         Silent("%s: Make FIFO %s\n",VPREFIX,destfile);
         }
      else if (mkfifo (destfile,srcmode))
         {
         snprintf(OUTPUT,bufsize*2,"Cannot create fifo `%s'", destfile);
	 CfLog(cferror,OUTPUT,"mkfifo");
         return;
         }
      
      AddMultipleClasses(ip->defines);
#endif
      }
   else
      {
      if (S_ISBLK (srcmode) || S_ISCHR (srcmode) || S_ISSOCK (srcmode))
         {
         if (DONTDO)
            {
            Silent("%s: Make BLK/CHR/SOCK %s\n",VPREFIX,destfile);
            }
         else if (mknod (destfile, srcmode, sourcestatbuf.st_rdev))
            {
            snprintf(OUTPUT,bufsize*2,"Cannot create special file `%s'",destfile);
	    CfLog(cferror,OUTPUT,"mknod");
            return;
            }
	 
	 AddMultipleClasses(ip->defines);
         }
      }

   if (S_ISLNK(srcmode))
      {
      if (cfreadlink(sourcefile,linkbuf,bufsize,ip) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't readlink %s\n",sourcefile);
	 CfLog(cferror,OUTPUT,"");
	 Debug2("Leaving ImageCopy\n");
         return;
         }

      snprintf(OUTPUT,bufsize*2,"Checking link from %s to %s\n",destfile,linkbuf);
      CfLog(cfverbose,OUTPUT,"");

      if (ip->linktype == 'a' && linkbuf[0] != '/')      /* Not absolute path - must fix */
         {
         strcpy(VBUFF,sourcefile);
         ChopLastNode(VBUFF);
         AddSlash(VBUFF);
         strncat(VBUFF,linkbuf,bufsize-1);
         strncpy(linkbuf,VBUFF,bufsize-1);
         }
      
      switch (ip->linktype)
         {
         case 's':
	           if (*linkbuf == '.')
		      {
		      succeed = RelativeLink(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
		      }
		   else
		      {
                      succeed = LinkFiles(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
		      }
                   break;
         case 'r':
                   succeed = RelativeLink(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
	           break;
         case 'a':
                   succeed = AbsoluteLink(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
                   break;
         default:
                   printf("cfengine: internal error, link type was [%c] in ImageCopy\n",ip->linktype);
                   return;
	 }

      if (succeed)
	 {
	 lstat(destfile,&deststatbuf);
	 CheckCopiedFile(destfile,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);
	 AddMultipleClasses(ip->defines);
	 }
      }
   }
else
   {
   Debug("Destination file %s exists\n",destfile);
   
   if (ip->force == 'n')
      {
      switch (ip->type)
         {
         case 'c': if (S_ISREG(deststatbuf.st_mode) && S_ISREG(srcmode))
		      {
	              ok_to_copy = CompareCheckSums(sourcefile,destfile,ip,&sourcestatbuf,&deststatbuf);
		      }	     
	           else
	              {
		      CfLog(cfverbose,"Checksum comparison replaced by ctime: files not regular\n","");
		      snprintf(OUTPUT,bufsize*2,"%s -> %s\n",sourcefile,destfile);
		      CfLog(cfinform,OUTPUT,"");
		      ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime);
	              }

                   if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                      { 
                      snprintf(OUTPUT,bufsize*2,"Image file %s has a wrong MD5 checksum (should be copy of %s)\n",destfile,sourcefile);
		      CfLog(cferror,OUTPUT,"");
                      return;
                      }
	           break;

	 case 'b': if (S_ISREG(deststatbuf.st_mode) && S_ISREG(srcmode))
		      {
	              ok_to_copy = CompareBinarySums(sourcefile,destfile,ip,&sourcestatbuf,&deststatbuf);
		      }	     
	           else
	              {
		      CfLog(cfinform,"Byte comparison replaced by ctime: files not regular\n","");
		      snprintf(OUTPUT,bufsize*2,"%s -> %s\n",sourcefile,destfile);
		      CfLog(cfverbose,OUTPUT,"");
		      ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime);
	              }

                   if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                      { 
                      snprintf(OUTPUT,bufsize*2,"Image file %s has a wrong binary checksum (should be copy of %s)\n",destfile,sourcefile);
		      CfLog(cferror,OUTPUT,"");
                      return;
                      }
	           break;

	 case 'm': ok_to_copy = (deststatbuf.st_mtime < sourcestatbuf.st_mtime);
	     
                   if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                      { 
                      snprintf(OUTPUT,bufsize*2,"Image file %s out of date (should be copy of %s)\n",destfile,sourcefile);
                      CfLog(cferror,OUTPUT,"");
                      return;
                      }
	           break;
		   
         default:  ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime);
	     
                   if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                      { 
                      snprintf(OUTPUT,bufsize*2,"Image file %s out of date (should be copy of %s)\n",destfile,sourcefile);
                      CfLog(cferror,OUTPUT,"");
                      return;
                      }
	           break;
         }
      }


   if (ip->typecheck == 'y')
      {
      if ((S_ISDIR(deststatbuf.st_mode)  && ! S_ISDIR(sourcestatbuf.st_mode))  ||
	  (S_ISREG(deststatbuf.st_mode)  && ! S_ISREG(sourcestatbuf.st_mode))  ||
	  (S_ISBLK(deststatbuf.st_mode)  && ! S_ISBLK(sourcestatbuf.st_mode))  ||
	  (S_ISCHR(deststatbuf.st_mode)  && ! S_ISCHR(sourcestatbuf.st_mode))  ||
	  (S_ISSOCK(deststatbuf.st_mode) && ! S_ISSOCK(sourcestatbuf.st_mode)) ||
	  (S_ISFIFO(deststatbuf.st_mode) && ! S_ISFIFO(sourcestatbuf.st_mode)) ||
	  (S_ISLNK(deststatbuf.st_mode)  && ! S_ISLNK(sourcestatbuf.st_mode)))
	  
	 {
	 printf("%s: image exists but destination type is silly (file/dir/link doesn't match)\n",VPREFIX);
	 printf("%s: source=%s, dest=%s\n",VPREFIX,sourcefile,destfile);
	 return;
	 }
      }

   if ((ip->force == 'y') || ok_to_copy || S_ISLNK(sourcestatbuf.st_mode))  /* Always check links */
      {
      if (S_ISREG(srcmode))
         {
         snprintf(OUTPUT,bufsize*2,"Update of image %s from master %s on %s",destfile,sourcefile,ip->server);

	 if (DONTDO)
	    {
	    return;
	    }
	 
	 CfLog(cfinform,OUTPUT,"");

	 AddMultipleClasses(ip->defines);

	 for (ptr = VAUTODEFINE; ptr != NULL; ptr=ptr->next)
	     {
	     if (strncmp(ptr->name,destfile,strlen(destfile)+1) == 0)
		{
		snprintf(OUTPUT,bufsize*2,"cfengine: image %s was set to autodefine %s\n",ptr->name,ptr->classes);
		CfLog(cfinform,OUTPUT,"");
		AddMultipleClasses(ptr->classes);
		}
	     }	

         if (CopyReg(sourcefile,destfile,sourcestatbuf,deststatbuf,ip))
            {
            stat(destfile,&deststatbuf);
	    CheckCopiedFile(destfile,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);

	    if (VSINGLECOPY != NULL)
	       {
	       succeed = 1;
	       }
	    else
	       {
	       succeed = 0;
	       }
	    
            for (ptr = VSINGLECOPY; ptr != NULL; ptr=ptr->next)
                {
                if (strncmp(ptr->classes,ip->classes,strlen(ip->classes)+1) == 0)
                   {
                   for (ptr1 = VEXCLUDECACHE; ptr1 != NULL; ptr1=ptr1->next)
                       {
                       if ((strncmp(ptr1->name,destfile,strlen(destfile)+1) == 0) && (strncmp(ptr1->classes,ip->classes,strlen(ip->classes)+1) == 0))
                          {
                          succeed = 0;
                          }
                       }
                    }
                 }

            if (succeed)
               {
               AppendItem(&VEXCLUDECACHE,destfile,ip->classes);
               }
	   }	
	 else
	    {
	    AddMultipleClasses(ip->failover);
	    }
	 
         return;
         }

      if (S_ISLNK(sourcestatbuf.st_mode))
         {
         if (cfreadlink(sourcefile,linkbuf,bufsize,ip) == -1)
            {
            snprintf(OUTPUT,bufsize*2,"Can't readlink %s\n",sourcefile);
	    CfLog(cferror,OUTPUT,"");
            return;
            }

         snprintf(OUTPUT,bufsize*2,"Checking link from %s to %s\n",destfile,linkbuf);
	 CfLog(cfverbose,OUTPUT,"");

	 enforcelinks = ENFORCELINKS;
	 ENFORCELINKS = true;

	 switch (ip->linktype)
	    {
	    case 's':
		if (*linkbuf == '.')
		   {
		   succeed = RelativeLink(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
		   }
		else
		   {
		   succeed = LinkFiles(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
		   }
		break;
	    case 'r':
		succeed = RelativeLink(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
		break;
	    case 'a':
		succeed = AbsoluteLink(destfile,linkbuf,NULL,NULL,NULL,true,&empty);
		break;
	    default:
		printf("cfengine: internal error, link type was [%c] in ImageCopy\n",ip->linktype);
		return;
	    }

	 if (succeed)
	    {
	    CheckCopiedFile(destfile,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);
	    }
	 ENFORCELINKS = enforcelinks;
         }
      }
   else
      {
      CheckCopiedFile(destfile,ip->plus,ip->minus,fixall,ip->uid,ip->gid,&deststatbuf,&sourcestatbuf,NULL,ip->acl_aliases);

      
      if (VSINGLECOPY != NULL)
	 {
	 succeed = 1;
	 }
      else
	 {
	 succeed = 0;
	 }
      
      for (ptr = VSINGLECOPY; ptr != NULL; ptr=ptr->next)
          {
          if (strncmp(ptr->classes,ip->classes,strlen(ip->classes)+1) == 0)
             {
             for (ptr1 = VEXCLUDECACHE; ptr1 != NULL; ptr1=ptr1->next)
                 {
                 if ((strncmp(ptr1->name,destfile,strlen(destfile)+1) == 0) && (strncmp(ptr1->classes,ip->classes,strlen(ip->classes)+1) == 0))
                    {
                    succeed = 0;
                    }
                 }
              }
           }

      if (succeed)
         {
         Debug("Appending image %s class %s to singlecopy list\n",destfile,ip->classes);
         AppendItem(&VEXCLUDECACHE,destfile,ip->classes);
         }

      Debug("Image file is up to date: %s\n",destfile);
      AddMultipleClasses(ip->elsedef);
      }
   }
}

/*********************************************************************/

int cfstat(file,buf,ip)

 /* wrapper for network access */

char *file;
struct stat *buf;
struct Image *ip;

{ int res;

if (ip == NULL)
   {
   return stat(file,buf);
   }

if (strcmp(ip->server,"localhost") == 0)
   {
   res = stat(file,buf);
   CheckForHoles(buf,ip);
   return res;
   }
else
   {
   return cf_rstat(file,buf,ip,"file");
   }
}

/*********************************************************************/

int cflstat(file,buf,ip)

char *file;
struct stat *buf;
struct Image *ip;

 /* wrapper for network access */

{ int res;

if (ip == NULL)
   {
   return lstat(file,buf);
   }

if (strcmp(ip->server,"localhost") == 0)
   {
   res = lstat(file,buf);
   CheckForHoles(buf,ip);
   return res;
   }
else
   {
   /* read cache if possible */
   return cf_rstat(file,buf,ip,"link");
   }
}

/*********************************************************************/

int cfreadlink(sourcefile,linkbuf,buffsize,ip)

char *sourcefile, *linkbuf;
int buffsize;
struct Image *ip;

 /* wrapper for network access */

{ struct cfstat *sp;

bzero(linkbuf,buffsize);
 
if (strcmp(ip->server,"localhost") == 0)
   {
   return readlink(sourcefile,linkbuf,buffsize-1);
   }

for (sp = ip->cache; sp != NULL; sp=sp->next)
   {
   if ((strcmp(ip->server,sp->cf_server) == 0) && (strcmp(sourcefile,sp->cf_filename) == 0))
      {
      if (sp->cf_readlink != NULL)
	 {
	 if (strlen(sp->cf_readlink)+1 > buffsize)
	    {
	    printf("%s: readlink value is too large in cfreadlink\n",VPREFIX);
	    printf("%s: [%s]]n",VPREFIX,sp->cf_readlink);
	    return -1;
	    }
	 else
	    {
	    bzero(linkbuf,buffsize);
	    strcpy(linkbuf,sp->cf_readlink);
	    return 0;
	    }
	 }
      }
   }

return -1;
}

/*********************************************************************/

CFDIR *cfopendir(name,ip)

char *name;
struct Image *ip;

{ CFDIR *cf_ropendir(),*returnval;

if (strcmp(ip->server,"localhost") == 0)
   {
   if ((returnval = (CFDIR *)malloc(sizeof(CFDIR))) == NULL)
      {
      FatalError("Can't allocate memory in cfopendir()\n");
      }
   
   returnval->cf_list = NULL;
   returnval->cf_listpos = NULL;
   returnval->cf_dirh = opendir(name);

   if (returnval->cf_dirh != NULL)
      {
      return returnval;
      }
   else
      {
      free ((char *)returnval);
      return NULL;
      }
   }
else
   {
   return cf_ropendir(name,ip);
   }
}

/*********************************************************************/

struct cfdirent *cfreaddir(cfdirh,ip)

CFDIR *cfdirh;
struct Image *ip;

  /* We need this cfdirent type to handle the weird hack */
  /* used in SVR4/solaris dirent structures              */

{ static struct cfdirent dir;
  struct dirent *dirp;

bzero(dir.d_name,bufsize);

if (strcmp(ip->server,"localhost") == 0)
   {
   dirp = readdir(cfdirh->cf_dirh);

   if (dirp == NULL)
      {
      return NULL;
      }

   strncpy(dir.d_name,dirp->d_name,bufsize-1);
   return &dir;
   }
else
   {
   if (cfdirh->cf_listpos != NULL)
      {
      strncpy(dir.d_name,(cfdirh->cf_listpos)->name,bufsize);
      cfdirh->cf_listpos = cfdirh->cf_listpos->next;
      return &dir;
      }
   else
      {
      return NULL;
      }
   }
}
 
/*********************************************************************/

void cfclosedir(dirh)

CFDIR *dirh;

{
if ((dirh != NULL) && (dirh->cf_dirh != NULL))
   {
   closedir(dirh->cf_dirh);
   }

Debug("cfclosedir()\n");
DeleteItemList(dirh->cf_list);
free((char *)dirh); 
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

int CopyReg(source,dest,sstat,dstat,ip)

char *source, *dest;
struct stat sstat, dstat;
struct Image *ip;

{ char backup[bufsize];
  char new[bufsize], *linkable;
  int remote = false, silent, backupisdir=false;
  struct stat s;
#ifdef HAVE_UTIME_H
  struct utimbuf timebuf;
#endif  

Debug2("CopyReg(%s,%s)\n",source,dest);

if (DONTDO)
   {
   printf("%s: copy from %s to %s\n",VPREFIX,source,dest);
   return false;
   }

 /* Make an assoc array of inodes used to preserve hard links */

linkable = CompressedArrayValue(ip->inode_cache,sstat.st_ino);

if (sstat.st_nlink > 1)  /* Preserve hard links, if possible */
   {
   if (CompressedArrayElementExists(ip->inode_cache,sstat.st_ino) && (strcmp(dest,linkable) != 0))
      {
      unlink(dest);

      silent = SILENT;
      SILENT = true;
      
      DoHardLink(dest,linkable,NULL);
      
      SILENT = silent;
      return true;
      }
   }

if (strcmp(ip->server,"localhost") != 0)
   {
   Debug("This is a remote copy from server: %s\n",ip->server);
   remote = true;
   }

strcpy(new,dest);
strcat(new,CF_NEW);

if (remote)
   {
   if (CONN->error)
      {
      return false;
      }
   
   if (!CopyRegNet(source,new,ip,sstat.st_size))
      {
      return false;
      }
   }
else
   {
   if (!CopyRegDisk(source,new,ip))
      {
      return false;
      }

   if (ip->stealth == 'y')
      {
#ifdef HAVE_UTIME_H
      timebuf.actime = sstat.st_atime;
      timebuf.modtime = sstat.st_mtime;
      utime(source,&timebuf);
#endif      
      }
   }

Debug("CopyReg succeeded in copying to %s to %s\n",source,new);

if (IMAGEBACKUP != 'n')
   {
   char stamp[bufsize];
   time_t STAMPNOW;
   STAMPNOW = time((time_t *)NULL);
   
   sprintf(stamp, "_%d_%s", CFSTARTTIME, CanonifyName(ctime(&STAMPNOW)));

   strcpy(backup,dest);

   if (IMAGEBACKUP == 's')
      {
      strcat(backup,stamp);
      }

   strcat(backup,CF_SAVED);

   if (IsItemIn(VREPOSLIST,backup))
      {
      return true;
      }

   /* Mainly important if there is a dir in the way */

   if (lstat(backup,&s) != -1)
      {
      if (S_ISDIR(s.st_mode))
	 {
	 backupisdir = true;
	 PurgeFiles(NULL,backup,NULL);
	 rmdir(backup);
	 }

      unlink(backup);
      }
   
   if (rename(dest,backup) == -1)
      {
      /* ignore */
      }
   }
else
   {
   /* Mainly important if there is a dir in the way */

   if (stat(dest,&s) != -1)
      {
      if (S_ISDIR(s.st_mode))
	 {
	 PurgeFiles(NULL,dest,NULL);
	 rmdir(dest);
	 }
      }
   }

stat(new,&dstat);

if (dstat.st_size != sstat.st_size)
   {
   snprintf(OUTPUT,bufsize*2,"WARNING: new file %s seems to have been corrupted in transit (sizes %d and %d), aborting!\n",new, (int) dstat.st_size, (int) sstat.st_size);
   CfLog(cfverbose,OUTPUT,"");
   if (rename(backup,dest) == -1)
      {
      /* ignore */
      }
   return false;
   }

if (ip->verify == 'y')
   {
   Verbose("Final verification of transmission.\n");
   if (CompareCheckSums(source,new,ip,&sstat,&dstat))
      {
      snprintf(OUTPUT,bufsize*2,"WARNING: new file %s seems to have been corrupted in transit, aborting!\n",new);
      CfLog(cfverbose,OUTPUT,"");
      if (rename(backup,dest) == -1)
	 {
	 }
      return false;
      }
   }

if (rename(new,dest) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Problem: could not install copy file as %s, directory in the way?\n",dest);
   CfLog(cferror,OUTPUT,"rename");
   rename(backup,dest);
   return false;
   }

if ((IMAGEBACKUP != 'n') && backupisdir)
   {
   snprintf(OUTPUT,bufsize,"Cannot move a directory to repository, leaving at %s",backup);
   CfLog(cfinform,OUTPUT,"");
   }
else if ((IMAGEBACKUP != 'n') && Repository(backup,ip->repository))
   {
   unlink(backup);
   }

if (ip->preservetimes == 'y')
   {
#ifdef HAVE_UTIME_H
   timebuf.actime = sstat.st_atime;
   timebuf.modtime = sstat.st_mtime;
   utime(dest,&timebuf);
#endif
   }

return true;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void RegisterHardLink(i,value,ip)

int i;
char *value;
struct Image *ip;

{
if (!FixCompressedArrayValue(i,value,&(ip->inode_cache)))
   {
    /* Not root hard link, remove to preserve consistency */
   if (DONTDO)
      {
      Verbose("Need to remove old hard link %s to preserve structure..\n",value);
      }
   else
      {
      Verbose("Removing old hard link %s to preserve structure..\n",value);
      unlink(value);
      }
   }
}
