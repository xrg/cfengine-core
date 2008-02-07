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

void GetRemoteMethods()

/* Schedule files matching pattern to be collected on next
   actionsequence -- this function should only be called during
   parsing since it uses covert variables that belong to the parser */

{ struct Item*ip;
  char client[CF_BUFSIZE]; 
 
Banner("Looking for remote method collaborations");

NewParser();

for (ip = VRPCPEERLIST; ip != NULL; ip = ip->next)
   {
   strncpy(client,ip->name,MAXHOSTNAMELEN);
   
   if (strstr(ip->name,".")||strstr(ip->name,":"))
      {
      }
   else
      {
      strcat(client,".");
      strcat(client,VDOMAIN);
      }
   
   if ((strcmp(client,VFQNAME) == 0) || (strcmp(client,VUQNAME) == 0))
      {
      /* Do not need to do this to ourselves ..  */
      continue;
      }
   
   Verbose(" Hailing remote peer %s for messages for us\n",client);
   
   InitializeAction();
   snprintf(DESTINATION,CF_BUFSIZE,"%s/rpc_in",CFWORKDIR);
   snprintf(CURRENTOBJECT,CF_BUFSIZE,"%s/rpc_out",CFWORKDIR);
   snprintf(FINDERTYPE,1,"*");
   PLUSMASK  = 0400;
   MINUSMASK = 0377;
   IMAGEBACKUP = 'n';
   ENCRYPT = 'y';
   strcpy(IMAGEACTION,"fix");
   strcpy(CLASSBUFF,"any");
   snprintf(VUIDNAME,CF_MAXVARSIZE,"%d",getuid());
   snprintf(VGIDNAME,CF_MAXVARSIZE,"%d",getgid());
   IMGCOMP = '>';
   VRECURSE = 1;
   PIFELAPSED = 0;
   PEXPIREAFTER = 0;
   COPYTYPE = DEFAULTCOPYTYPE;
   
   InstallImageItem(FINDERTYPE,CURRENTOBJECT,PLUSMASK,MINUSMASK,DESTINATION,
                    IMAGEACTION,VUIDNAME,VGIDNAME,IMGSIZE,IMGCOMP,
                    VRECURSE,COPYTYPE,LINKTYPE,client);
   }

DeleteParser();

Verbose("\nFinished with RPC\n\n");

}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void CheckImage(char *source,char *destination,struct Image *ip)

{ CFDIR *dirh;
  char sourcefile[CF_BUFSIZE];
  char sourcedir[CF_BUFSIZE];
  char destdir[CF_BUFSIZE];
  char destfile[CF_BUFSIZE];
  struct stat sourcestatbuf, deststatbuf;
  struct cfdirent *dirp;
  int save_uid, save_gid, found;
  
Debug2("CheckImage (source=%s destination=%s)\n",source,destination);

if (ip->linktype == 'n')
   {
   Debug("Treating links as files for %s\n",source);
   found = cfstat(source,&sourcestatbuf,ip);
   }
else
   {
   found = cflstat(source,&sourcestatbuf,ip);
   }

if (found == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n",source);
   CfLog(cferror,OUTPUT,"");
   FlushClientCache(ip);
   AddMultipleClasses(ip->elsedef);
   AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_FAIL);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",sourcedir);
      CfLog(cfverbose,OUTPUT,"opendir");
      FlushClientCache(ip);
      (ip->uid)->uid = save_uid;
      (ip->gid)->gid = save_gid;
      return;
      }

   /* Now check any overrides */
  
    if (stat(destdir,&deststatbuf) == -1)
       {
       snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat directory %s\n",destdir);
       CfLog(cferror,OUTPUT,"stat");
       }
    else
       {
       CheckCopiedFile(ip->cf_findertype,destdir,&deststatbuf,&sourcestatbuf,ip);
       }

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

      if (ip->linktype == 'n')
         {
         if (cfstat(sourcefile,&sourcestatbuf,ip) == -1)
            {
            printf("%s: Can't stat %s\n",VPREFIX,sourcefile);
            FlushClientCache(ip);       
            (ip->uid)->uid = save_uid;
            (ip->gid)->gid = save_gid;
            return;
            }
         }
      else
         {
         if (cflstat(sourcefile,&sourcestatbuf,ip) == -1)
            {
            printf("%s: Can't stat %s\n",VPREFIX,sourcefile);
            FlushClientCache(ip);       
            (ip->uid)->uid = save_uid;
            (ip->gid)->gid = save_gid;
            return;
            }
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

void PurgeFiles(struct Item *filelist,char *directory,struct Item *inclusions)

{ DIR *dirh;
  struct stat statbuf; 
  struct dirent *dirp;
  char filename[CF_BUFSIZE];

Debug("PurgeFiles(%s)\n",directory);

 /* If we purge with no authentication we wipe out EVERYTHING */ 

 if (strlen(directory) < 2)
    {
    snprintf(OUTPUT,CF_BUFSIZE,"Purge of %s denied -- too dangerous!",directory);
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
    snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",directory);
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
      strncpy(filename,directory,CF_BUFSIZE-2);
      AddSlash(filename);
      strncat(filename,dirp->d_name,CF_BUFSIZE-2);
      
      Debug("Checking purge %s..\n",filename);
      
      if (DONTDO)
         {
         printf("Need to purge %s from copy dest directory\n",filename);
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Purging %s in copy dest directory\n",filename);
         CfLog(cfinform,OUTPUT,"");
         
         if (lstat(filename,&statbuf) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't stat %s while purging\n",filename);
            CfLog(cfverbose,OUTPUT,"stat");
            }
         else if (S_ISDIR(statbuf.st_mode))
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
            tpat.recurse = CF_INF_RECURSE;
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
            tpat.compress = 'n';
            tpat.log = 'd';
            tpat.inform = 'd';
            tpat.next = NULL;
            RecursiveTidySpecialArea(filename,&tp,CF_INF_RECURSE,&statbuf);
            free(tpat.pattern);
            free(tpat.classes);
            
            chdir("..");
            
            if (rmdir(filename) == -1)
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't remove directory %s while purging\n",filename);
               CfLog(cfverbose,OUTPUT,"rmdir");
               }
            
            continue;
            }
         else if (unlink(filename) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't unlink %s while purging\n",filename);
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

void ImageCopy(char *sourcefile,char *destfile,struct stat sourcestatbuf,struct Image *ip)

{ char linkbuf[CF_BUFSIZE],server[CF_EXPANDSIZE],*lastnode;
  struct stat deststatbuf;
  struct Link empty;
  struct Item *ptr, *ptr1;
  int succeed = false,silent = false, enforcelinks;
  mode_t srcmode = sourcestatbuf.st_mode;
  int ok_to_copy = false, found;

Debug2("ImageCopy(%s,%s,+%o,-%o)\n",sourcefile,destfile,ip->plus,ip->minus);

ExpandVarstring(ip->server,server,NULL);
      
if ((strcmp(sourcefile,destfile) == 0) && (strcmp(ip->server,"localhost") == 0))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Image loop: file/dir %s copies to itself",sourcefile);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

memset(&empty,0,sizeof(struct Link));
empty.nofile = true;

if (IgnoredOrExcluded(image,sourcefile,ip->inclusions,ip->exclusions))
   {
   return;
   }

if (IsItemIn(VEXCLUDECACHE,destfile))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Skipping single-copied file %s class %s\n",destfile,ip->classes);
   CfLog(cfverbose,OUTPUT,"");
   return;
   }

if (ip->linktype != 'n')
   {
   lastnode=ReadLastNode(sourcefile);

   if (IsWildItemIn(VLINKCOPIES,lastnode) || IsWildItemIn(ip->symlink,lastnode))
      {
      Verbose("cfengine: copy item %s marked for linking instead\n",sourcefile);
      enforcelinks = ENFORCELINKS;
      ENFORCELINKS = true;
      //ip->returnstatus = CF_CHG;
      
      switch (ip->linktype)
         {
         case 's':
             succeed = LinkFiles(destfile,sourcefile,&empty);
             break;
         case 'r':
             succeed = RelativeLink(destfile,sourcefile,&empty);
             break;
         case 'a':
             succeed = AbsoluteLink(destfile,sourcefile,&empty);
             break;
         default:
             printf("%s: internal error, link type was [%c] in ImageCopy\n",VPREFIX,ip->linktype);
             return;
         }
      
      if (succeed)
         {
         ENFORCELINKS = enforcelinks;
         
         if (lstat(destfile,&deststatbuf) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Can't lstat %s\n",destfile);
            CfLog(cferror,OUTPUT,"lstat");
            }
         else
            {
            CheckCopiedFile(ip->cf_findertype,destfile,&deststatbuf,&sourcestatbuf,ip);
            }     
         }      
      return;
      }
   }
 
if (strcmp(ip->action,"silent") == 0)
   {
   silent = true;
   }

memset(linkbuf,0,CF_BUFSIZE);

found = lstat(destfile,&deststatbuf);

if (found != -1)
   {
   if ((S_ISLNK(deststatbuf.st_mode) && (ip->linktype == 'n')) || (S_ISLNK(deststatbuf.st_mode) && ! S_ISLNK(sourcestatbuf.st_mode)))
      {
      if ((!S_ISLNK(sourcestatbuf.st_mode) && (ip->typecheck == 'y')) && (ip->linktype != 'n'))
         {
         printf("%s: image exists but destination type is silly (file/dir/link doesn't match) - %c\n",VPREFIX,ip->linktype);
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't remove link %s",destfile);
            CfLog(cferror,OUTPUT,"unlink");
            return;
            }
         Verbose("Removing old symbolic link %s to make way for copy\n",destfile);
         //ip->returnstatus = CF_CHG;
         found = -1;
         }
      } 
   }

if (ip->size != CF_NOSIZE)
   {
   switch (ip->comp)
      {
      case '<':  if (sourcestatbuf.st_size > ip->size)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Source file %s is > %d bytes in copy (omitting)\n",sourcefile,ip->size);
         CfLog(cfinform,OUTPUT,"");
         return;
         }
          break;
          
      case '=':
          if (sourcestatbuf.st_size != ip->size)
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"Source file %s is not %d bytes in copy (omitting)\n",sourcefile,ip->size);
             CfLog(cfinform,OUTPUT,"");
             return;
             }
          break;
          
      default:
          if (sourcestatbuf.st_size < ip->size)
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"Source file %s is < %d bytes in copy (omitting)\n",sourcefile,ip->size);
             CfLog(cfinform,OUTPUT,"");
             return;
             }
          break;
      }
   }


if (found == -1)
   {
   if (strcmp(ip->action,"warn") == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Image file %s is non-existent\n",destfile);
      CfLog(cferror,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"(should be copy of %s)\n",sourcefile);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   
   if (S_ISREG(srcmode) || (S_ISLNK(srcmode) && ip->linktype == 'n'))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"%s wasn't at destination (copying)",destfile);
      if (DONTDO)
         {
         printf("Need this: %s\n",OUTPUT);
         return;
         }
      
      CfLog(cfverbose,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"Copying from %s:%s\n",server,sourcefile);
      CfLog(cfinform,OUTPUT,"");
      
      if (CopyReg(sourcefile,destfile,sourcestatbuf,deststatbuf,ip))
         {
         if (stat(destfile,&deststatbuf) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n",destfile);
            CfLog(cferror,OUTPUT,"stat");
            }
         else
            {
            CheckCopiedFile(ip->cf_findertype,destfile,&deststatbuf,&sourcestatbuf,ip);
            }

         snprintf(OUTPUT,CF_BUFSIZE*2,"Copied from %s:%s\n",server,sourcefile);
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_CHG);
         AddMultipleClasses(ip->defines);
         
         if (ALL_SINGLECOPY || IsWildItemIn(VSINGLECOPY,destfile))
            {
            if (!IsItemIn(VEXCLUDECACHE,destfile))
               {
               Debug("Appending image %s class %s to singlecopy list\n",destfile,ip->classes);
               PrependItem(&VEXCLUDECACHE,destfile,NULL);
               }
            }         

         for (ptr = VAUTODEFINE; ptr != NULL; ptr=ptr->next)
            {
            if (WildMatch(ptr->name,destfile))
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Image %s was set to autodefine class %s\n",ptr->name,ptr->classes);
               CfLog(cfinform,OUTPUT,"");
               AddMultipleClasses(ptr->classes);
               }
            } 
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Copy from %s:%s\n",server,sourcefile);
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_FAIL);
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot create fifo `%s'", destfile);
         CfLog(cferror,OUTPUT,"mkfifo");
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_FAIL);
         return;
         }

      snprintf(OUTPUT,CF_BUFSIZE*2,"Created fifo %s", destfile);
      AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_CHG);
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot create special file `%s'",destfile);
            CfLog(cferror,OUTPUT,"mknod");
            return;
            }

         snprintf(OUTPUT,CF_BUFSIZE*2,"Created socket/device %s", destfile);
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_CHG);
         AddMultipleClasses(ip->defines);
         }
      }
   
   if (S_ISLNK(srcmode) && ip->linktype != 'n')
      {
      if (cfreadlink(sourcefile,linkbuf,CF_BUFSIZE,ip) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't readlink %s\n",sourcefile);
         CfLog(cferror,OUTPUT,"");
         Debug2("Leaving ImageCopy\n");
         return;
         }
      
      snprintf(OUTPUT,CF_BUFSIZE*2,"Checking link from %s to %s\n",destfile,linkbuf);
      CfLog(cfverbose,OUTPUT,"");
      
      if (ip->linktype == 'a' && linkbuf[0] != '/')      /* Not absolute path - must fix */
         {
         strcpy(VBUFF,sourcefile);
         ChopLastNode(VBUFF);
         AddSlash(VBUFF);
         strncat(VBUFF,linkbuf,CF_BUFSIZE-1);
         strncpy(linkbuf,VBUFF,CF_BUFSIZE-1);
         }
      
      switch (ip->linktype)
         {
         case 's':
             if (*linkbuf == '.')
                {
                succeed = RelativeLink(destfile,linkbuf,&empty);
                }
             else
                {
                succeed = LinkFiles(destfile,linkbuf,&empty);
                }
             break;
         case 'r':
             succeed = RelativeLink(destfile,linkbuf,&empty);
             break;
         case 'a':
             succeed = AbsoluteLink(destfile,linkbuf,&empty);
             break;

         case 'n': /* Link copied instead */
             break;
             
         default:
             printf("cfengine: internal error, link type was [%c] in ImageCopy 2\n",ip->linktype);
             return;
         }
      
      if (succeed)
         {
         if (lstat(destfile,&deststatbuf) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Can't lstat %s\n",destfile);
            CfLog(cferror,OUTPUT,"lstat");
            }
         else
            {
            CheckCopiedFile(ip->cf_findertype,destfile,&deststatbuf,&sourcestatbuf,ip);
            }

         snprintf(OUTPUT,CF_BUFSIZE*2,"Created link %s", destfile);
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_CHG);
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
         case 'c':
             if (S_ISREG(deststatbuf.st_mode) && S_ISREG(srcmode))
                {
                ok_to_copy = CompareCheckSums(sourcefile,destfile,ip,&sourcestatbuf,&deststatbuf);
                }      
             else
                {
                CfLog(cfinform,"Checksum comparison replaced by ctime: files not regular\n","");
                snprintf(OUTPUT,CF_BUFSIZE*2,"%s -> %s\n",sourcefile,destfile);
                CfLog(cfinform,OUTPUT,"");
                ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime);
                }
             
             if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                { 
                snprintf(OUTPUT,CF_BUFSIZE*2,"Image file %s has a wrong MD5 checksum (should be copy of %s)\n",destfile,sourcefile);
                CfLog(cfinform,OUTPUT,"");
                return;
                }
             break;
             
         case 'b':
             if (S_ISREG(deststatbuf.st_mode) && S_ISREG(srcmode))
                {
                ok_to_copy = CompareBinarySums(sourcefile,destfile,ip,&sourcestatbuf,&deststatbuf);
                }      
             else
                {
                CfLog(cfinform,"Byte comparison replaced by ctime: files not regular\n","");
                snprintf(OUTPUT,CF_BUFSIZE*2,"%s -> %s\n",sourcefile,destfile);
                CfLog(cfinform,OUTPUT,"");
                ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime);
                }
             
             if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                { 
                snprintf(OUTPUT,CF_BUFSIZE*2,"Image file %s has a wrong binary checksum (should be copy of %s)\n",destfile,sourcefile);
                CfLog(cferror,OUTPUT,"");
                return;
                }
             break;
             
         case 'm':
             ok_to_copy = (deststatbuf.st_mtime < sourcestatbuf.st_mtime);
             
             if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                { 
                snprintf(OUTPUT,CF_BUFSIZE*2,"Image file %s out of date (should be copy of %s)\n",destfile,sourcefile);
                CfLog(cferror,OUTPUT,"");
                return;
                }
             break;
             
         case 'a':
             
             ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime)||CompareBinarySums(sourcefile,destfile,ip,&sourcestatbuf,&deststatbuf);
             
             if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                { 
                snprintf(OUTPUT,CF_BUFSIZE*2,"Image file %s is updated somehow (should be copy of %s)\n",destfile,sourcefile);
                CfLog(cferror,OUTPUT,"");
                return;
                }
             break;
              
         default:
             ok_to_copy = (deststatbuf.st_ctime < sourcestatbuf.st_ctime)||(deststatbuf.st_mtime < sourcestatbuf.st_mtime);
             
             if (ok_to_copy && strcmp(ip->action,"warn") == 0)
                { 
                snprintf(OUTPUT,CF_BUFSIZE*2,"Image file %s out of date (should be copy of %s)\n",destfile,sourcefile);
                CfLog(cferror,OUTPUT,"");
                return;
                }
             break;
         }
      }
   
   
   if (ip->typecheck == 'y' && ip->linktype != 'n')
      {
      if ((S_ISDIR(deststatbuf.st_mode)  && ! S_ISDIR(sourcestatbuf.st_mode))  ||
          (S_ISREG(deststatbuf.st_mode)  && ! S_ISREG(sourcestatbuf.st_mode))  ||
          (S_ISBLK(deststatbuf.st_mode)  && ! S_ISBLK(sourcestatbuf.st_mode))  ||
          (S_ISCHR(deststatbuf.st_mode)  && ! S_ISCHR(sourcestatbuf.st_mode))  ||
          (S_ISSOCK(deststatbuf.st_mode) && ! S_ISSOCK(sourcestatbuf.st_mode)) ||
          (S_ISFIFO(deststatbuf.st_mode) && ! S_ISFIFO(sourcestatbuf.st_mode)) ||
          (S_ISLNK(deststatbuf.st_mode)  && ! S_ISLNK(sourcestatbuf.st_mode)))
          
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Image exists but src/dest type mismatch source=%s, dest=%s\n",sourcefile,destfile);
         CfLog(cfinform,OUTPUT,"");
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_FAIL);
         return;
         }
      }
   
   if ((ip->force == 'y') || ok_to_copy || S_ISLNK(sourcestatbuf.st_mode))  /* Always check links */
      {
      if (S_ISREG(srcmode) || ip->linktype == 'n')
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Updated image %s from master %s on %s",destfile,sourcefile,server);
         
         if (DONTDO)
            {
            printf("Need: %s\n",OUTPUT);
            return;
            }
         
         CfLog(cfinform,OUTPUT,"");
         AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_CHG);
         AddMultipleClasses(ip->defines);
         
         for (ptr = VAUTODEFINE; ptr != NULL; ptr=ptr->next)
            {
            if (WildMatch(ptr->name,destfile))
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Image %s was set to autodefine class %s\n",ptr->name,ptr->classes);
               CfLog(cfinform,OUTPUT,"");
               AddMultipleClasses(ptr->classes);
               }
            } 
         
         if (CopyReg(sourcefile,destfile,sourcestatbuf,deststatbuf,ip))
            {
            if (stat(destfile,&deststatbuf) == -1)
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n",destfile);
               CfLog(cferror,OUTPUT,"stat");
               AuditLog(ip->logaudit,ip->audit,ip->lineno,OUTPUT,CF_FAIL);
               }
            else
               {
               CheckCopiedFile(ip->cf_findertype,destfile,&deststatbuf,&sourcestatbuf,ip);
               }
            
            if (ALL_SINGLECOPY || IsWildItemIn(VSINGLECOPY,destfile))
               {
               if (!IsItemIn(VEXCLUDECACHE,destfile))
                  {
                  Debug("Appending image %s class %s to singlecopy list\n",destfile,ip->classes);
                  PrependItem(&VEXCLUDECACHE,destfile,NULL);
                  }
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
         if (cfreadlink(sourcefile,linkbuf,CF_BUFSIZE,ip) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Can't readlink %s\n",sourcefile);
            CfLog(cferror,OUTPUT,"");
            return;
            }
         
         snprintf(OUTPUT,CF_BUFSIZE*2,"Checking link from %s to %s\n",destfile,linkbuf);
         CfLog(cfverbose,OUTPUT,"");
         
         enforcelinks = ENFORCELINKS;
         ENFORCELINKS = true;
         
         switch (ip->linktype)
            {
            case 's':
                if (*linkbuf == '.')
                   {
                   succeed = RelativeLink(destfile,linkbuf,&empty);
                   }
                else
                   {
                   succeed = LinkFiles(destfile,linkbuf,&empty);
                   }
                break;
            case 'r':
                succeed = RelativeLink(destfile,linkbuf,&empty);
                break;
            case 'a':
                succeed = AbsoluteLink(destfile,linkbuf,&empty);
                break;
            default:
                snprintf(OUTPUT,CF_BUFSIZE,"Internal error, link type was [%c] in ImageCopy\n",ip->linktype);
                CfLog(cferror,OUTPUT,"");
                return;
            }
         
         if (succeed)
            {
            CheckCopiedFile(ip->cf_findertype,destfile,&deststatbuf,&sourcestatbuf,ip);
            }
         ENFORCELINKS = enforcelinks;
         }
      }
   else
      {
      /*  This check should go into CheckCopiedFile  ip->returnstatus = CF_FAIL; */
      CheckCopiedFile(ip->cf_findertype,destfile,&deststatbuf,&sourcestatbuf,ip);
      
      /* Now we have to check for single copy, even though nothing was copied
         otherwise we can get oscillations between multipe versions if type is based on a checksum */
      
      if (ALL_SINGLECOPY || IsWildItemIn(VSINGLECOPY,destfile))
         {
         if (!IsItemIn(VEXCLUDECACHE,destfile))
            {
            Debug("Appending image %s class %s to singlecopy list\n",destfile,ip->classes);
            PrependItem(&VEXCLUDECACHE,destfile,NULL);
            }
          }
      
      Debug("Image file is up to date: %s\n",destfile);
      AddMultipleClasses(ip->elsedef);
      }
   }
}

/*********************************************************************/

int cfstat(char *file,struct stat *buf,struct Image *ip)

 /* wrapper for network access */

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

int cflstat(char *file,struct stat *buf,struct Image *ip)

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

int cfreadlink(char *sourcefile,char *linkbuf,int buffsize,struct Image *ip)

 /* wrapper for network access */

{ struct cfstat *sp;

memset(linkbuf,0,buffsize);
 
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
     memset(linkbuf,0,buffsize);
     strcpy(linkbuf,sp->cf_readlink);
     return 0;
     }
  }
      }
   }

return -1;
}

/*********************************************************************/

CFDIR *cfopendir(char *name,struct Image *ip)

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

struct cfdirent *cfreaddir(CFDIR *cfdirh,struct Image *ip)

  /* We need this cfdirent type to handle the weird hack */
  /* used in SVR4/solaris dirent structures              */

{ static struct cfdirent dir;
  struct dirent *dirp;

memset(dir.d_name,0,CF_BUFSIZE);

if (strcmp(ip->server,"localhost") == 0)
   {
   dirp = readdir(cfdirh->cf_dirh);

   if (dirp == NULL)
      {
      return NULL;
      }

   strncpy(dir.d_name,dirp->d_name,CF_BUFSIZE-1);
   return &dir;
   }
else
   {
   if (cfdirh->cf_listpos != NULL)
      {
      strncpy(dir.d_name,(cfdirh->cf_listpos)->name,CF_BUFSIZE);
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

void cfclosedir(CFDIR *dirh)

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
/* Level 4                                                           */
/*********************************************************************/

int CopyReg(char *source,char *dest,struct stat sstat,struct stat dstat,struct Image *ip)

{ char backup[CF_BUFSIZE];
  char new[CF_BUFSIZE], *linkable;
  int remote = false, silent, backupisdir=false, backupok=false;
  struct stat s;
#ifdef HAVE_UTIME_H
  struct utimbuf timebuf;
#endif  

#ifdef DARWIN
/* For later copy from new to dest */
char *rsrcbuf;
int rsrcbytesr; /* read */
int rsrcbytesw; /* written */
int rsrcbytesl; /* to read */
int rsrcrd;
int rsrcwd;

/* Keep track of if a resrouce fork */
char * tmpstr;
char * forkpointer;

int rsrcfork;
rsrcfork=0;
#endif

#ifdef WITH_SELINUX
int selinux_enabled=0;
/* need to keep track of security context of destination file (if any) */
security_context_t scontext=NULL;
struct stat cur_dest;
int dest_exists;
selinux_enabled = (is_selinux_enabled()>0);
#endif

Debug2("CopyReg(%s,%s)\n",source,dest);

if (DONTDO)
   {
   printf("%s: copy from %s to %s\n",VPREFIX,source,dest);
   return false;
   }

#ifdef WITH_SELINUX
if(selinux_enabled)
    {
    dest_exists = stat(dest,&cur_dest);
    if(dest_exists == 0)
        {
        /* get current security context of destination file */
        getfilecon(dest,&scontext);
        }
    else
        {
        /* use default security context when creating destination file */
        matchpathcon(dest,0,&scontext);
        setfscreatecon(scontext);
        }
    }
#endif

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

#ifdef DARWIN
if (strstr(dest, _PATH_RSRCFORKSPEC))
   { /* Need to munge the "new" name */
   rsrcfork = 1;
   
   tmpstr = malloc(CF_BUFSIZE);
   
   /* Drop _PATH_RSRCFORKSPEC */
   strncpy(tmpstr, dest, CF_BUFSIZE);
   forkpointer = strstr(tmpstr, _PATH_RSRCFORKSPEC);
   *forkpointer = '\0';
   
   strncpy(new, tmpstr, CF_BUFSIZE);
   
   free(tmpstr);
   }
else
   {
#endif

if (BufferOverflow(dest,CF_NEW))
   {
   printf(" culprit: CopyReg\n");
   return false;
   }
strcpy(new,dest);

#ifdef DARWIN
   }
#endif

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
   char stamp[CF_BUFSIZE];
   time_t STAMPNOW;


   Debug("Backup file %s\n",source);

   STAMPNOW = time((time_t *)NULL);
   
   sprintf(stamp, "_%d_%s", CFSTARTTIME, CanonifyName(ctime(&STAMPNOW)));

   if (BufferOverflow(dest,stamp))
      {
      printf(" culprit: CopyReg\n");
      return false;
      }

   strcpy(backup,dest);

   if (IMAGEBACKUP == 's')
      {
      strcat(backup,stamp);
      }

   /* rely on prior BufferOverflow() and on strlen(CF_SAVED) < CF_BUFFERMARGIN */

   strcat(backup,CF_SAVED);

   /* Now in case of multiple copies of same object, try to avoid overwriting original backup */
   
   if (lstat(backup,&s) != -1)
      {
      if (S_ISDIR(s.st_mode))      /* if there is a dir in the way */
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
   
   backupok = (lstat(backup,&s) != -1); /* Did the rename() succeed? NFS-safe */
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

if (stat(new,&dstat) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat new file %s\n",new);
   CfLog(cferror,OUTPUT,"stat");
   return false;
   }

if (dstat.st_size != sstat.st_size)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING: new file %s seems to have been corrupted in transit (sizes %d and %d), aborting!\n",new, (int) dstat.st_size, (int) sstat.st_size);
   CfLog(cfverbose,OUTPUT,"");
   if (backupok)
      {
      rename(backup,dest); /* ignore failure */
      }
   return false;
   }

if (ip->verify == 'y')
   {
   Verbose("Final verification of transmission.\n");
   if (CompareCheckSums(source,new,ip,&sstat,&dstat))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING: new file %s seems to have been corrupted in transit, aborting!\n",new);
      CfLog(cfverbose,OUTPUT,"");
      if (backupok)
         {
         rename(backup,dest); /* ignore failure */
         }
      return false;
      }
   }
 
#ifdef DARWIN
if (rsrcfork)
   { /* Can't just "mv" the resource fork, unfortunately */   
   rsrcrd = open(new, O_RDONLY|O_BINARY);
   rsrcwd = open(dest, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, 0600);
   
   if (rsrcrd == -1 || rsrcwd == -1)
      {
      snprintf(OUTPUT, CF_BUFSIZE, "Open of rsrcrd/rsrcwd failed\n");
      CfLog(cfinform,OUTPUT,"open");
      close(rsrcrd);
      close(rsrcwd);
      return(false);
      }
   
   rsrcbuf = malloc(CF_BUFSIZE);
   
   rsrcbytesr = 0;
   
   while(1)
      {
      rsrcbytesr = read(rsrcrd, rsrcbuf, CF_BUFSIZE);
      
      if (rsrcbytesr == -1)
         { /* Ck error */
         if (errno == EINTR)
            {
            continue;
            }
         else
            {
            snprintf(OUTPUT, CF_BUFSIZE, "Read of rsrcrd failed\n");
            CfLog(cfinform,OUTPUT,"read");
            close(rsrcrd);
            close(rsrcwd);
            free(rsrcbuf);
            return(false);
            }
         }
      
      else if (rsrcbytesr == 0)
         { /* Reached EOF */
         close(rsrcrd);
         close(rsrcwd);
         free(rsrcbuf);
         
         unlink(new); /* Go ahead and unlink .cfnew */
         
         break;
         }
      
      rsrcbytesl = rsrcbytesr;
      rsrcbytesw = 0;
      
      while (rsrcbytesl > 0)
         {
         
         rsrcbytesw += write(rsrcwd, rsrcbuf, rsrcbytesl);
         
         if (rsrcbytesw == -1)
            { /* Ck error */
            if (errno == EINTR)
                {
                continue;
                }
            else
               {
               snprintf(OUTPUT, CF_BUFSIZE, "Write of rsrcwd failed\n");
               CfLog(cfinform,OUTPUT,"write");
               
               close(rsrcrd);
               close(rsrcwd);
               free(rsrcbuf);
               return(false);
               }
            }  
         rsrcbytesl = rsrcbytesr - rsrcbytesw;  
         }
      }
   
   }
 
 else
    {
#endif
    
    
    if (rename(new,dest) == -1)
       {
       snprintf(OUTPUT,CF_BUFSIZE*2,"Problem: could not install copy file as %s, directory in the way?\n",dest);
       CfLog(cferror,OUTPUT,"rename");
       if (backupok)
          {
          rename(backup,dest); /* ignore failure */
          }
       return false;
       }
    
#ifdef DARWIN
    }
 #endif
 
 if ((IMAGEBACKUP != 'n') && backupisdir)
    {
    snprintf(OUTPUT,CF_BUFSIZE,"Cannot move a directory to repository, leaving at %s",backup);
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

#ifdef WITH_SELINUX
if(selinux_enabled)
    {
    if(dest_exists == 0)
        {
        /* set dest context to whatever it was before copy */
        setfilecon(dest,scontext);
        }
    else
        {
        /* set create context back to default */
        setfscreatecon(NULL);
        }
    freecon(scontext);
    }
#endif

 return true;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void RegisterHardLink(int i,char *value,struct Image *ip)

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
      if (strcmp(ip->action,"warn") == 0)
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
}
