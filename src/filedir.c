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

#ifdef DARWIN
#include <sys/attr.h>
#endif

/*********************************************************************/

int IsHomeDir(char *name)

      /* This assumes that the dir lies under mountpattern */

{ char *sp;
  struct Item *ip;
  int slashes;

if (name == NULL || strlen(name) == 0)
   {
   return false;
   }
  
if (VMOUNTLIST == NULL)
   {
   return (false);
   } 

for (ip = VHOMEPATLIST; ip != NULL; ip=ip->next)
   {
   slashes = 0;

   for (sp = ip->name; *sp != '\0'; sp++)
      {
      if (*sp == '/')
         {
         slashes++;
         }
      }
   
   for (sp = name+strlen(name); (*(sp-1) != '/') && (sp >= name) && (slashes >= 0); sp--)
      {
      if (*sp == '/')
         {
         slashes--;
         }
      }
   
   /* Full comparison */
   
   if (WildMatch(ip->name,sp))
      {
      Debug("IsHomeDir(true)\n");
      return(true);
      }
   }

Debug("IsHomeDir(%s,false)\n",name);
return(false);
}


/*********************************************************************/

int EmptyDir(char *path)

{ DIR *dirh;
  struct dirent *dirp;
  int count = 0;
  
Debug2("cfengine: EmptyDir(%s)\n",path);

if ((dirh = opendir(path)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",path);
   CfLog(cfverbose,OUTPUT,"opendir");
   return true;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,path,NULL))
      {
      continue;
      }

   count++;
   }

closedir(dirh);

return (!count);
}


/*********************************************************************/

void CheckExistingFile(char *cf_findertype,char *file,struct stat *dstat,struct File *ptr)

{ mode_t newperm = dstat->st_mode, maskvalue;
 int amroot = true, fixmode = true, docompress=false, changed = false;
  unsigned char digest1[EVP_MAX_MD_SIZE+1];
  unsigned char digest2[EVP_MAX_MD_SIZE+1];
  enum fileactions action = ptr->action;

#if defined HAVE_CHFLAGS
  u_long newflags;
#endif

Debug("CheckExistingFile(%s,%s)\n",file,FILEACTIONTEXT[ptr->action]);
  
maskvalue = umask(0);                 /* This makes the DEFAULT modes absolute */
 
if (action == compress)
   {
   docompress = true;
   
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   
   action = fixall;    /* Fix any permissions which are set */
   }
  
Debug("%s: Checking fs-object %s\n",VPREFIX,file);
   
#if defined HAVE_CHFLAGS
if (ptr != NULL)
   {
   Debug("CheckExistingFile(+%o,-%o,+%o,-%o)\n",ptr->plus,ptr->minus,ptr->plus_flags,ptr->minus_flags);
   }
else
   {
   Debug("CheckExistingFile(+%o,-%o)\n",ptr->plus,ptr->minus);
   }
#else
Debug("CheckExistingFile(+%o,-%o)\n",ptr->plus,ptr->minus);
#endif
 
if (ptr != NULL)
   {
   if (IgnoredOrExcluded(files,file,ptr->inclusions,ptr->exclusions))
      {
      Debug("Skipping excluded file %s\n",file);
      umask(maskvalue);
      return;
      }

   if (!FileObjectFilter(file,dstat,ptr->filters,files))
      {
      Debug("Skipping filtered file %s\n",file);
      umask(maskvalue);
      return;
      }

   if (action == alert)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Alert specified on file %s (m=%o,o=%d,g=%d)",file,(dstat->st_mode & 07777),dstat->st_uid,dstat->st_gid);
      CfLog(cferror,OUTPUT,"");
      AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
      return;
      }
   }

if (!IsPrivileged())                            
   {
   amroot = false;
   }

 /* directories must have x set if r set, regardless  */

newperm = (dstat->st_mode & 07777);
newperm |= ptr->plus;
newperm &= ~(ptr->minus);

if (S_ISREG(dstat->st_mode) && (action == fixdirs || action == warndirs)) 
   {
   Debug("Regular file, returning...\n");
   umask(maskvalue);
   return;
   }

if (S_ISDIR(dstat->st_mode))  
   {
   if (action == fixplain || action == warnplain)
      {
      umask(maskvalue);
      return;
      }

   if (ptr->rxdirs != 'n')
      {
      Debug("Directory...fixing x bits\n");
      
      if (newperm & S_IRUSR)
         {
         newperm  |= S_IXUSR;
         }
      
      if (newperm & S_IRGRP)
         {
         newperm |= S_IXGRP;
         }
      
      if (newperm & S_IROTH)
         {
         newperm |= S_IXOTH;
         }
      }
   else
      {
      Verbose("NB: rxdirs is set to false - x for r bits not checked\n");
      }
   }

if (dstat->st_uid == 0 && (dstat->st_mode & S_ISUID))
   {
   if (newperm & S_ISUID)
      {
      if (! IsItemIn(VSETUIDLIST,file))
         {
         if (amroot)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"NEW SETUID root PROGRAM %s\n",file);
            CfLog(cfinform,OUTPUT,"");
            AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
            }
         PrependItem(&VSETUIDLIST,file,NULL);
         }
      }
   else
      {
      switch (action)
         {
         case fixall:
         case fixdirs:
         case fixplain:
             snprintf(OUTPUT,CF_BUFSIZE*2,"Removing setuid (root) flag from %s...\n\n",file);
             CfLog(cfinform,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
             
             if (ptr != NULL)
                {
                AddMultipleClasses(ptr->defines);
                }
             break;
         case warnall:
         case warndirs:
         case warnplain:
             if (amroot)
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING setuid (root) flag on %s...\n\n",file);
                CfLog(cferror,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
                }
             break;             
         }
      }
   }
 
if (dstat->st_uid == 0 && (dstat->st_mode & S_ISGID))
   {
   if (newperm & S_ISGID)
      {
      if (!IsItemIn(VSETUIDLIST,file))
         {
         if (S_ISDIR(dstat->st_mode))
            {
            /* setgid directory */
            }
         else
            {
            if (amroot)
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"NEW SETGID root PROGRAM %s\n",file);
               CfLog(cferror,OUTPUT,"");
               AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
               }

            PrependItem(&VSETUIDLIST,file,NULL);
            }
         }
      }
   else
      {
      switch (action)
         {
         case fixall:
         case fixdirs:
         case fixplain:
             snprintf(OUTPUT,CF_BUFSIZE*2,"Removing setgid (root) flag from %s...\n\n",file);
             CfLog(cfinform,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);

             if (ptr != NULL)
                {
                AddMultipleClasses(ptr->defines);
                }
             break;
         case warnall:
         case warndirs:
         case warnplain:
             snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING setgid (root) flag on %s...\n\n",file);
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
             CfLog(cferror,OUTPUT,"");
             break;
             
         default:
             break;
         }
      }
   }

#ifdef DARWIN
if (CheckFinderType(file,action, cf_findertype, dstat))
   {
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   }
#endif

if (CheckOwner(file,ptr,dstat))
   {
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   }

if ((ptr != NULL) && S_ISREG(dstat->st_mode) && (ptr->checksum != 'n'))
   {
   Debug("Checking checksum integrity of %s\n",file);
   
   memset(digest1,0,EVP_MAX_MD_SIZE+1);
   memset(digest2,0,EVP_MAX_MD_SIZE+1);

   if (ptr->checksum == 'b')
      {
      ChecksumFile(file,digest1,'m');
      ChecksumFile(file,digest2,'s');
   
      if (!DONTDO)
         {
         if (ChecksumChanged(file,digest1,cferror,false,'m') || ChecksumChanged(file,digest2,cferror,false,'s'))
            {
            changed = true;
            }
         }
      }
   else
      {
      ChecksumFile(file,digest1,ptr->checksum);
   
      if (!DONTDO)
         {
         if (ChecksumChanged(file,digest1,cferror,false,ptr->checksum))
            {
            changed = true;
            }
         }
      }
   }

if (changed)
   {
   snprintf(VBUFF,CF_BUFSIZE,"checksum_alert");
   AddPersistentClass(VBUFF,30,cfpreserve);
   AddClassToHeap(VBUFF);
   LogChecksumChange(file);
   }

if (S_ISLNK(dstat->st_mode))             /* No point in checking permission on a link */
   {
   if (ptr != NULL)
      {
      KillOldLink(file,ptr->defines);
      }
   else
      {
      KillOldLink(file,NULL);
      }

   umask(maskvalue);
   return;
   }

if (stat(file,dstat) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat file %s while looking at permissions (was not copied?)\n",file);
   CfLog(cfverbose,OUTPUT,"stat");
   umask(maskvalue);
   return;
   }

if (CheckACLs(file,action,ptr->acl_aliases))
   {
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   }

#ifndef HAVE_CHFLAGS
if (((newperm & 07777) == (dstat->st_mode & 07777)) && (action != touch))    /* file okay */
   {
   Debug("File okay, newperm = %o, stat = %o\n",(newperm & 07777),(dstat->st_mode & 07777));
   if (docompress)
      {
      CompressFile(file);
      }

   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->elsedef);
      }

   umask(maskvalue);
   return;
   }
#else
if (((newperm & 07777) == (dstat->st_mode & 07777)) && (action != touch))    /* file okay */
   {
   Debug("File okay, newperm = %o, stat = %o\n",(newperm & 07777),(dstat->st_mode & 07777));
   fixmode = false;
   }
#endif 

if (fixmode)
   {
   Debug("Trying to fix mode...\n"); 
   
   switch (action)
      {
      case linkchildren:

      case warnplain:
          if (S_ISREG(dstat->st_mode))
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"%s has permission %o\n",file,dstat->st_mode & 07777);
             CfLog(cferror,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
             snprintf(OUTPUT,CF_BUFSIZE*2,"[should be %o]\n",newperm & 07777);
             CfLog(cferror,OUTPUT,"");
             }
          break;
      case warndirs:
          if (S_ISDIR(dstat->st_mode))
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"%s has permission %o\n",file,dstat->st_mode & 07777);
             CfLog(cferror,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
             snprintf(OUTPUT,CF_BUFSIZE*2,"[should be %o]\n",newperm & 07777);
             CfLog(cferror,OUTPUT,"");
             }
          break;
      case warnall:   
          snprintf(OUTPUT,CF_BUFSIZE*2,"%s has permission %o\n",file,dstat->st_mode & 07777);
          CfLog(cferror,OUTPUT,"");
          AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
          snprintf(OUTPUT,CF_BUFSIZE*2,"[should be %o]\n",newperm & 07777);
          CfLog(cferror,OUTPUT,"");
          break;
          
      case fixplain:
          if (S_ISREG(dstat->st_mode))
             {
             if (! DONTDO)
                {
                if (chmod (file,newperm & 07777) == -1)
                   {
                   snprintf(OUTPUT,CF_BUFSIZE*2,"chmod failed on %s\n",file);
                   CfLog(cferror,OUTPUT,"chmod");
                   break;
                   }
                }
             
             snprintf(OUTPUT,CF_BUFSIZE*2,"Plain file %s had permission %o, changed it to %o\n",
                      file,dstat->st_mode & 07777,newperm & 07777);
             CfLog(cfinform,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
             
             if (ptr != NULL)
                {
                AddMultipleClasses(ptr->defines);
                }
             }
          break;
          
      case fixdirs:
          if (S_ISDIR(dstat->st_mode))
             {
             if (! DONTDO)
                {
                if (chmod (file,newperm & 07777) == -1)
                   {
                   snprintf(OUTPUT,CF_BUFSIZE*2,"chmod failed on %s\n",file);
                   CfLog(cferror,OUTPUT,"chmod");
                   break;
                   }
                }
             
             snprintf(OUTPUT,CF_BUFSIZE*2,"Directory %s had permission %o, changed it to %o\n",
                      file,dstat->st_mode & 07777,newperm & 07777);
             CfLog(cfinform,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
             
             if (ptr != NULL)
                {
                AddMultipleClasses(ptr->defines);
                }
             }
          break;
          
      case fixall:
          if (! DONTDO)
             {
             if (chmod (file,newperm & 07777) == -1)
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"chmod failed on %s\n",file);
                CfLog(cferror,OUTPUT,"chmod");
                break;
                }
             }
          
          snprintf(OUTPUT,CF_BUFSIZE*2,"Object %s had permission %o, changed it to %o\n",
                   file,dstat->st_mode & 07777,newperm & 07777);
          CfLog(cfinform,OUTPUT,"");
          AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
          
          if (ptr != NULL)
             {
             AddMultipleClasses(ptr->defines);
             }
          break;
          
      case touch:
      case create:
          if (! DONTDO)
             {
             if (chmod (file,newperm & 07777) == -1)
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"chmod failed on %s\n",file);
                CfLog(cferror,OUTPUT,"chmod");
                break;
                }
             utime (file,NULL);
             }
          if (ptr != NULL)
             {
             AddMultipleClasses(ptr->defines);
             }
          break;
          
      default:     FatalError("cfengine: internal error CheckExistingFile(): illegal file action\n");
      }
   }
 

if (CheckOwner(file,ptr,dstat)) /* Again */
   {
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   }             

#if defined HAVE_CHFLAGS  /* BSD special flags */

if (ptr != NULL)
   {
   newflags = (dstat->st_flags & CHFLAGS_MASK) ;
   newflags |= ptr->plus_flags;
   newflags &= ~(ptr->minus_flags);
   
   if ((newflags & CHFLAGS_MASK) == (dstat->st_flags & CHFLAGS_MASK))    /* file okay */
      {
      Debug("File okay, flags = %o, current = %o\n",(newflags & CHFLAGS_MASK),(dstat->st_flags & CHFLAGS_MASK));
      }
   else
      {
      Debug("Fixing %s, newflags = %o, flags = %o\n",file,(newflags & CHFLAGS_MASK),(dstat->st_flags & CHFLAGS_MASK));
      
      switch (action)
         {
         case linkchildren:
             
         case warnplain:
             if (S_ISREG(dstat->st_mode))
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"%s has flags %o\n",file,dstat->st_flags & CHFLAGS_MASK);
                CfLog(cferror,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
                snprintf(OUTPUT,CF_BUFSIZE*2,"[should be %o]\n",newflags & CHFLAGS_MASK);
                CfLog(cferror,OUTPUT,"");
                }
             break;
         case warndirs:
             if (S_ISDIR(dstat->st_mode))
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"%s has flags %o\n",file,dstat->st_mode & CHFLAGS_MASK);
                CfLog(cferror,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
                snprintf(OUTPUT,CF_BUFSIZE*2,"[should be %o]\n",newflags & CHFLAGS_MASK);
                CfLog(cferror,OUTPUT,"");
                }
             break;
         case warnall:
             snprintf(OUTPUT,CF_BUFSIZE*2,"%s has flags %o\n",file,dstat->st_mode & CHFLAGS_MASK);
             CfLog(cferror,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
             snprintf(OUTPUT,CF_BUFSIZE*2,"[should be %o]\n",newflags & CHFLAGS_MASK);
             CfLog(cferror,OUTPUT,"");
             break;
             
         case fixplain:
             
             if (S_ISREG(dstat->st_mode))
                {
                if (! DONTDO)
                   {
                   if (chflags (file,newflags & CHFLAGS_MASK) == -1)
                      {
                      snprintf(OUTPUT,CF_BUFSIZE*2,"chflags failed on %s\n",file);
                      CfLog(cferror,OUTPUT,"chflags");
                      break;
                      }
                   }
                
                snprintf(OUTPUT,CF_BUFSIZE*2,"%s had flags %o, changed it to %o\n",
                         file,dstat->st_flags & CHFLAGS_MASK,newflags & CHFLAGS_MASK);
                CfLog(cfinform,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
                
                if (ptr != NULL)
                   {
                   AddMultipleClasses(ptr->defines);
                   }
                }
             break;
             
         case fixdirs:
             if (S_ISDIR(dstat->st_mode))
                {
                if (! DONTDO)
                   {
                   if (chflags (file,newflags & CHFLAGS_MASK) == -1)
                      {
                      snprintf(OUTPUT,CF_BUFSIZE*2,"chflags failed on %s\n",file);
                      CfLog(cferror,OUTPUT,"chflags");
                      break;
                      }
                   }
                
                snprintf(OUTPUT,CF_BUFSIZE*2,"%s had flags %o, changed it to %o\n",
                         file,dstat->st_flags & CHFLAGS_MASK,newflags & CHFLAGS_MASK);
                CfLog(cfinform,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
                
                if (ptr != NULL)
                   {
                   AddMultipleClasses(ptr->defines);
                   }
                }
             break;
             
         case fixall:
             if (! DONTDO)
                {
                if (chflags (file,newflags & CHFLAGS_MASK) == -1)
                   {
                   snprintf(OUTPUT,CF_BUFSIZE*2,"chflags failed on %s\n",file);
                   CfLog(cferror,OUTPUT,"chflags");
                   break;
                   }
                }
             
             snprintf(OUTPUT,CF_BUFSIZE*2,"%s had flags %o, changed it to %o\n",
                      file,dstat->st_flags & CHFLAGS_MASK,newflags & CHFLAGS_MASK);
             CfLog(cfinform,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
             
             if (ptr != NULL)
                {
                AddMultipleClasses(ptr->defines);
                }
             break;
             
         case touch:
         case create:
             if (! DONTDO)
                {
                if (chflags (file,newflags & CHFLAGS_MASK) == -1)
                   {
                   snprintf(OUTPUT,CF_BUFSIZE*2,"chflags failed on %s\n",file);
                   CfLog(cferror,OUTPUT,"chflags");
                   break;
                   }
                utime (file,NULL);
                }
             if (ptr != NULL)
                {
                AddMultipleClasses(ptr->defines);
                }
             break;
             
         default:     FatalError("cfengine: internal error CheckExistingFile(): illegal file action\n");
         }
      }
   }
#endif
 
if (docompress)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Compressing %s",file);
   AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);

   printf("COMPRESS %s\n",file);
   CompressFile(file);
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   }

umask(maskvalue);  
Debug("CheckExistingFile(Done)\n"); 
}

/*********************************************************************/

void CheckCopiedFile(char *cf_findertype,char *file,struct stat *dstat,struct stat *sstat,struct Image *ptr)

{ mode_t newplus,newminus;
  enum fileactions convert = fixall;
  struct File tmp;

 /* plus/minus must be relative to source file, not to
    perms of newly created file! */

Debug("CheckCopiedFile(%s,+%o,-%o)\n",file,ptr->plus,ptr->minus); 

newplus = (sstat->st_mode & 07777) | ptr->plus;
newminus = ~(newplus & ~(ptr->minus)) & 07777;

if (strcmp(ptr->action,"fix") == 0 || strcmp(ptr->action,"silent") == 0)
   {
   convert = fixall;
   }
else if (strcmp(ptr->action,"warn") == 0)
   {
   convert = warnall;
   }

memset(&tmp,0,sizeof(struct File));

tmp.plus = newplus;
tmp.minus = newminus;
tmp.action = convert;
tmp.classes = ptr->classes;
tmp.defines = ptr->defines;
tmp.elsedef = ptr->elsedef;
tmp.uid = ptr->uid;
tmp.gid = ptr->gid;
tmp.xdev = ptr->xdev;
tmp.checksum = 'n';

CheckExistingFile(cf_findertype,file,dstat,&tmp);
}

#ifdef DARWIN
/*********************************************************************/

int CheckFinderType(char *file,enum fileactions action,char *cf_findertype, struct stat *statbuf)

{ /* Code modeled after hfstar's extract.c */
 typedef struct t_fndrinfo
    {
    long fdType;
    long fdCreator;
    short fdFlags;
    short fdLocationV;
    short fdLocationH;
    short fdFldr;
    short fdIconID;
    short fdUnused[3];
    char fdScript;
    char fdXFlags;
    short fdComment;
    long fdPutAway;
    }
 FInfo;
 
 struct attrlist attrs;
 struct
    {
    long ssize;
    struct timespec created;
    struct timespec modified;
    struct timespec changed;
    struct timespec backup;
    FInfo fi;
    }
 fndrInfo;
 
 int retval;
 
 Debug("CheckFinderType of %s for %s\n", file, cf_findertype);
 
 if (strncmp(cf_findertype, "*", CF_BUFSIZE) == 0 || strncmp(cf_findertype, "", CF_BUFSIZE) == 0)
    { /* No checking - no action */
    Debug("CheckFinderType not needed\n");
    return 0;
    }
 
 attrs.bitmapcount = ATTR_BIT_MAP_COUNT;
 attrs.reserved = 0;
 attrs.commonattr = ATTR_CMN_CRTIME | ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME | ATTR_CMN_BKUPTIME | ATTR_CMN_FNDRINFO;
 attrs.volattr = 0;
 attrs.dirattr = 0;
 attrs.fileattr = 0;
 attrs.forkattr = 0;
 
 memset(&fndrInfo, 0, sizeof(fndrInfo));
 
 getattrlist(file, &attrs, &fndrInfo, sizeof(fndrInfo),0);
 
 if (fndrInfo.fi.fdType != *(long *)cf_findertype)
    { /* Need to update Finder Type field */
    fndrInfo.fi.fdType = *(long *)cf_findertype;
    
    /* Determine action to take */
    
    if (S_ISLNK(statbuf->st_mode) || S_ISDIR(statbuf->st_mode))
       {
       printf("CheckFinderType: Wrong file type for %s -- skipping\n", file);
       return 0;
       }
    
    if (S_ISREG(statbuf->st_mode) && action == fixdirs)
       {
       printf("CheckFinderType: Wrong file type for %s -- skipping\n", file);
       return 0;
       }
    
    switch (action)
       {       
       /* Fix it */
       case fixplain:
       case fixdirs:
       case fixall: 
       case touch:
           
           
           if (DONTDO)
              { /* well, unless we shouldn't fix it */
              printf("CheckFinderType: Dry run. No action taken.\n");
              return 0;
              }
           
           /* setattrlist does not take back in the long ssize */        
           retval = setattrlist(file, &attrs, &fndrInfo.created, 4*sizeof(struct timespec) + sizeof(FInfo), 0);
           
           Debug("CheckFinderType setattrlist returned %d\n", retval);
           
           if (retval >= 0)
              {
              printf("Setting Finder Type code of %s to %s\n", file, cf_findertype);
              }
           else
              {
              printf("Setting Finder Type code of %s to %s failed!!\n", file, cf_findertype);
              }
           
           return retval;
           
            /* Just warn */
       case linkchildren:
       case warnall: 
       case warndirs:
       case warnplain:
           printf("Warning: FinderType does not match -- not fixing.\n");
           return 0;
           
       default:
           printf("Should never get here. Aroo?\n");
           return 0;
       }
    }
 
 else
    {
    Debug("CheckFinderType matched\n");
    return 0;
    }
}

#endif


/*********************************************************************/

int CheckOwner(char *file,struct File *ptr,struct stat *statbuf)

{ struct passwd *pw;
  struct group *gp;
  struct UidList *ulp, *unknownulp;
  struct GidList *glp, *unknownglp;
  short uidmatch = false, gidmatch = false;
  uid_t uid = CF_SAME_OWNER; 
  gid_t gid = CF_SAME_GROUP;

Debug("CheckOwner: %d\n",statbuf->st_uid);
  
for (ulp = ptr->uid; ulp != NULL; ulp=ulp->next)
   {
   Debug(" uid %d\n",ulp->uid);
   
   if (ulp->uid == CF_UNKNOWN_OWNER) /* means not found while parsing */
      {
      unknownulp = MakeUidList(ulp->uidname); /* Will only match one */
      if (unknownulp != NULL && statbuf->st_uid == unknownulp->uid)
         {
         uid = unknownulp->uid;
         uidmatch = true;
         break;
         }
      }
   
   if (ulp->uid == CF_SAME_OWNER || statbuf->st_uid == ulp->uid)   /* "same" matches anything */
      {
      uid = ulp->uid;
      uidmatch = true;
      break;
      }
   }
 
for (glp = ptr->gid; glp != NULL; glp=glp->next)
   {
   if (glp->gid == CF_UNKNOWN_GROUP) /* means not found while parsing */
      {
      unknownglp = MakeGidList(glp->gidname); /* Will only match one */
      if (unknownglp != NULL && statbuf->st_gid == unknownglp->gid)
         {
         gid = unknownglp->gid;
         gidmatch = true;
         break;
         }
      }

   if (glp->gid == CF_SAME_GROUP || statbuf->st_gid == glp->gid)  /* "same" matches anything */
      {
      gid = glp->gid;
      gidmatch = true;
      break;
      }
   }
 
if (uidmatch && gidmatch)
   {
   return false;
   }
else
   {
   if (! uidmatch)
      {
      for (ulp = ptr->uid; ulp != NULL; ulp=ulp->next)
         {
         if (ptr->uid->uid != CF_UNKNOWN_OWNER)
            {
            Verbose("Setting owner to %d\n",ptr->uid->uid);
            uid = ptr->uid->uid;    /* default is first (not unknown) item in list */
            break;
            }
         }
      }
   
   if (! gidmatch)
      {
      for (glp = ptr->gid; glp != NULL; glp=glp->next)
         {
         if (ptr->gid->gid != CF_UNKNOWN_GROUP)
            {
            gid = ptr->gid->gid;    /* default is first (not unknown) item in list */
            break;
            }
         }
      }
   
   if (S_ISLNK(statbuf->st_mode) && (ptr->action == fixdirs || ptr->action == fixplain))
      {
      Debug2("File %s incorrect type (link), skipping...\n",file);
      return false;
      }
   
   if ((S_ISREG(statbuf->st_mode) && ptr->action == fixdirs) || (S_ISDIR(statbuf->st_mode) && ptr->action == fixplain))
      {
      Debug2("File %s incorrect type, skipping...\n",file);
      return false;
      }
   
   switch (ptr->action)
      {
      case fixplain:
      case fixdirs:
      case fixall:
      case create:
      case touch:
          if (VERBOSE || DEBUG || D2)
             {
             if (uid == CF_SAME_OWNER && gid == CF_SAME_GROUP)
                {
                printf("%s:   touching %s\n",VPREFIX,file);
                }
             else
                {
                if (uid != CF_SAME_OWNER)
                   {
                   Debug("(Change owner to uid %d if possible)\n",uid);
                   }
                
                if (gid != CF_SAME_GROUP)
                   {
                   Debug("Change group to gid %d if possible)\n",gid);
                   }
                }
             }
          
          if (! DONTDO && S_ISLNK(statbuf->st_mode))
             {
#ifdef HAVE_LCHOWN
             Debug("Using LCHOWN function\n");
             if (lchown(file,uid,gid) == -1)
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot set ownership on link %s!\n",file);
                CfLog(cflogonly,OUTPUT,"lchown");
                }
             else
                {
                return true;
                }
#endif
             }
          else if (! DONTDO)
             {
             if (!uidmatch)
                {
                snprintf(OUTPUT,CF_BUFSIZE,"Owner of %s was %d, setting to %d",file,statbuf->st_uid,uid);
                CfLog(cfinform,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
                }
             
             if (!gidmatch)
                {
                snprintf(OUTPUT,CF_BUFSIZE,"Group of %s was %d, setting to %d",file,statbuf->st_gid,gid);
                CfLog(cfinform,OUTPUT,"");
                AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
                }
             
             if (!S_ISLNK(statbuf->st_mode))
                {
                if (chown(file,uid,gid) == -1)
                   {
                   snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot set ownership on file %s!\n",file);
                   CfLog(cflogonly,OUTPUT,"chown");
                   }
                else
                   {
                   return true;
                   }
                }
             }
          break;
          
      case linkchildren:
      case warnall: 
      case warndirs:
      case warnplain:
          if ((pw = getpwuid(statbuf->st_uid)) == NULL)
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is not owned by anybody in the passwd database\n",file);
             CfLog(cferror,OUTPUT,"");
             snprintf(OUTPUT,CF_BUFSIZE*2,"(uid = %d,gid = %d)\n",statbuf->st_uid,statbuf->st_gid);
             CfLog(cferror,OUTPUT,"");
             break;
             }
          
          if ((gp = getgrgid(statbuf->st_gid)) == NULL)
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is not owned by any group in group database\n",file);
             CfLog(cferror,OUTPUT,"");
             AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
             break;
             }
          
          snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is owned by [%s], group [%s]\n",file,pw->pw_name,gp->gr_name);
          CfLog(cferror,OUTPUT,"");
          AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_WARN);
          break;
      }
   }

return false; 
}


/*********************************************************************/

int CheckHomeSubDir(char *testpath,char *tidypath,int recurse)

{ char *subdirstart, *sp1, *sp2;
  char buffer[CF_BUFSIZE];
  int homelen;

Debug("CheckHomeSubDir(%s,%s)\n",testpath,tidypath);
   
if (strncmp(tidypath,"home/",5) == 0)
   {
   strcpy(buffer,testpath);

   for (ChopLastNode(buffer); strlen(buffer) != 0; ChopLastNode(buffer))
     {
     if (IsHomeDir(buffer))
        {
        break;
        }
     }

   homelen = strlen(buffer);

   if (homelen == 0)   /* No homedir */
      {
      return false;
      }

   Debug("....CheckHomeSubDir(%s,%s)\n",testpath,tidypath);

   subdirstart = tidypath + strlen("home");                         /* Ptr to start of subdir */

   strcpy(buffer,testpath);

   /* ChopLastNode(buffer);  Filename only */

   for (sp1 = buffer + homelen + 1; *sp1 != '/' && *sp1 != '\0'; sp1++) /* skip user name dir */
      {
      }
      
   sp2 = subdirstart;

   if (strncmp(sp1,sp2,strlen(sp2)) != 0)
      {
      return false;
      } 

   Debug("CheckHomeSubDir(true)\n");
   return(true);
   }

return false;
}

/**************************************************************/

int FileIsNewer(char *file1,char *file2)

/* True if file2 is newer than file 1 */

{ struct stat statbuf1, statbuf2;

if (stat(file2,&statbuf2) == -1)
   {
   CfLog(cferror,"","stat");
   return false;
   }

if (stat(file1,&statbuf1) == -1)
   {
   CfLog(cferror,"","stat");
   return true;
   }

if (statbuf1.st_mtime < statbuf2.st_mtime)
   {
   return true;
   }
else
   {
   return false;
   }
}

/*********************************************************************/

void LogChecksumChange(char *file)

{ FILE *fp;
 char fname[CF_BUFSIZE],timebuf[CF_MAXVARSIZE];
  time_t now = time(NULL);

/* This is inefficient but we don't want to lose any data */
  
snprintf(fname,CF_BUFSIZE,"%s/state/file_hash_event_history",CFWORKDIR);

if ((fp = fopen(fname,"a")) == NULL)
   {
   CfLog(cferror,"Could not write to the change log","");
   return;
   }

snprintf(timebuf,CF_MAXVARSIZE-1,"%s",ctime(&now));
Chop(timebuf);
fprintf(fp,"%s,%s\n",timebuf,file);

fclose(fp);
}

/*********************************************************************/
/* Used by files and tidy modules                                    */
/*********************************************************************/

int IgnoreFile (char *pathto,char *name,struct Item *ignores)

{ struct Item *ip;

Debug("IgnoreFile(%s,%s)\n",pathto,name);
 
if (name == NULL || strlen(name) == 0)
   {
   return false;
   }
 
strncpy(VBUFF,pathto,CF_BUFSIZE-1);
AddSlash(VBUFF);
strcat(VBUFF,name);

if (ignores != NULL)
   {
   if (IsWildItemIn(ignores,VBUFF))
      {
      Debug("cfengine: Ignoring private abs path [%s][%s]\n",pathto,name);
      return true;
      }

   if (IsWildItemIn(ignores,name))
      {
      Debug("cfengine: Ignoring private pattern [%s][%s]\n",pathto,name);
      return true;
      }
   }

for (ip = VIGNORE; ip != NULL; ip=ip->next)
   {
   if (IsExcluded(ip->classes))
      {
      continue;
      }

   if (*(ip->name) == '/')
      {
      if (strcmp(VBUFF,ip->name) == 0)
         {
         Debug("cfengine: Ignoring global abs path [%s][%s]\n",pathto,name);
         return true;
         }
      }
   else
      {
      if (WildMatch(ip->name,name))
         {
         Debug("cfengine: Ignoring global pattern [%s][%s]\n",pathto,name);
         return true;
         }
      }
   }

return false;
}

/*********************************************************************/

void CompressFile(char *file)

{ char comm[CF_BUFSIZE];
  FILE *pp;
 
if ((COMPRESSCOMMAND == NULL) || strlen(COMPRESSCOMMAND) == 0)
   {
   CfLog(cferror,"CompressCommand variable is not defined","");
   return;
   }
 
snprintf(comm,CF_BUFSIZE,"%s %s",COMPRESSCOMMAND,file); 

snprintf(OUTPUT,CF_BUFSIZE*2,"Compressing file %s",file); 
CfLog(cfinform,OUTPUT,"");
 
if ((pp=cfpopen(comm,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Compression command failed on %s",file);
   CfLog(cfverbose,OUTPUT,"");
   }

while (!feof(pp))
   {
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   CfLog(cfinform,VBUFF,"");
   }

cfpclose(pp); 
}

