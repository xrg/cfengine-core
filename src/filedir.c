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


/*********************************************************************/

int IsHomeDir(name)

      /* This assumes that the dir lies under mountpattern */

char *name;

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

Debug("IsHomeDir(false)\n");
return(false);
}


/*********************************************************************/

int EmptyDir(path)

char *path;

{ DIR *dirh;
  struct dirent *dirp;
  int count = 0;
  
Debug2("cfengine: EmptyDir(%s)\n",path);

if ((dirh = opendir(path)) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",path);
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

int RecursiveCheck(name,plus,minus,action,uidlist,gidlist,recurse,rlevel,ptr,sb)

char *name;
mode_t plus,minus;
struct UidList *uidlist;
struct GidList *gidlist;
enum fileactions action;
int recurse;
int rlevel;
struct File *ptr;
struct stat *sb;

{ DIR *dirh;
  int goback; 
  struct dirent *dirp;
  char pcwd[bufsize];
  struct stat statbuf;
  
if (recurse == -1)
   {
   return false;
   }

if (rlevel > recursion_limit)
   {
   snprintf(OUTPUT,bufsize*2,"WARNING: Very deep nesting of directories (>%d deep): %s (Aborting files)",rlevel,name);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
 
bzero(pcwd,bufsize); 

Debug("RecursiveCheck(%s,+%o,-%o)\n",name,plus,minus);

if (!DirPush(name,sb))
   {
   return false;
   }
 
if ((dirh = opendir(".")) == NULL)
   {
   if (lstat(name,&statbuf) != -1)
      {
      CheckExistingFile(name,plus,minus,action,uidlist,gidlist,&statbuf,ptr,ptr->acl_aliases);
      }
   return true;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,name,NULL))
      {
      continue;
      }

   if (IgnoreFile(name,dirp->d_name,ptr->ignores))
      {
      continue;
      }
   
   strcpy(pcwd,name);                                   /* Assemble pathname */
   AddSlash(pcwd);

   if (BufferOverflow(pcwd,dirp->d_name))
      {
      closedir(dirh);
      return true;
      }

   strcat(pcwd,dirp->d_name);

   if (lstat(dirp->d_name,&statbuf) == -1)
      {
      snprintf(OUTPUT,bufsize*2,"RecursiveCheck was looking at %s when this happened:\n",pcwd);
      CfLog(cferror,OUTPUT,"lstat");
      continue;
      }
   
   if (TRAVLINKS)
      {
      if (lstat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",pcwd);
	 CfLog(cferror,OUTPUT,"stat");
         continue;
         }

      if (S_ISLNK(statbuf.st_mode) && (statbuf.st_mode != getuid()))	  
	 {
	 snprintf(OUTPUT,bufsize,"File %s is an untrusted link. cfagent will not follow it with a destructive operation (tidy)",pcwd);
	 continue;
	 }

      if (stat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"RecursiveCheck was working on %s when this happened:\n",pcwd);
	 CfLog(cferror,OUTPUT,"stat");
         continue;
         }
      }


   if (S_ISLNK(statbuf.st_mode))            /* should we ignore links? */
      {
      CheckExistingFile(pcwd,plus,minus,action,uidlist,gidlist,&statbuf,ptr,ptr->acl_aliases);
      continue;
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      if (IsMountedFileSystem(&statbuf,pcwd,rlevel))
         {
         continue;
         }
      else
         {
         if ((ptr->recurse > 1) || (ptr->recurse == INFINITERECURSE))
            {
            CheckExistingFile(pcwd,plus,minus,action,uidlist,gidlist,&statbuf,ptr,ptr->acl_aliases);
            goback = RecursiveCheck(pcwd,plus,minus,action,uidlist,gidlist,recurse-1,rlevel+1,ptr,&statbuf);
	    DirPop(goback,name,sb);
            }
         else
            {
            CheckExistingFile(pcwd,plus,minus,action,uidlist,gidlist,&statbuf,ptr,ptr->acl_aliases);
            }
         }
      }
   else
      {
      CheckExistingFile(pcwd,plus,minus,action,uidlist,gidlist,&statbuf,ptr,ptr->acl_aliases);
      }
   }

closedir(dirh);
return true; 
}


/*********************************************************************/

void CheckExistingFile(file,plus,minus,action,uidlist,gidlist,dstat,ptr,acl_aliases)

char *file;
mode_t plus,minus;
struct UidList *uidlist;
struct GidList *gidlist;
enum fileactions action;
struct stat *dstat;
struct File *ptr;
struct Item *acl_aliases;
 
{ mode_t newperm = dstat->st_mode, maskvalue;
  int amroot = true, fixmode = true, docompress=false;
  unsigned char digest[EVP_MAX_MD_SIZE+1];

#if defined HAVE_CHFLAGS
  u_long newflags;
#endif

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
   Debug("CheckExistingFile(+%o,-%o,+%o,-%o)\n",plus,minus,ptr->plus_flags,ptr->minus_flags);
   }
else
   {
   Debug("CheckExistingFile(+%o,-%o)\n",plus,minus);
   }
#else
Debug("CheckExistingFile(+%o,-%o)\n",plus,minus);
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
      snprintf(OUTPUT,bufsize*2,"Alert specified on file %s (m=%o,o=%d,g=%d)",file,(dstat->st_mode & 07777),dstat->st_uid,dstat->st_gid);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   }

if (!IsPrivileged())                            
   {
   amroot = false;
   }

 /* directories must have x set if r set, regardless  */


newperm = (dstat->st_mode & 07777);
newperm |= plus;
newperm &= ~minus;

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

if (dstat->st_uid == 0 && (dstat->st_mode & S_ISUID))
   {
   if (newperm & S_ISUID)
      {
      if (! IsItemIn(VSETUIDLIST,file))
         {
         if (amroot)
	    {
	    snprintf(OUTPUT,bufsize*2,"NEW SETUID root PROGRAM %s\n",file);
	    CfLog(cfinform,OUTPUT,"");
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
         case fixplain:  snprintf(OUTPUT,bufsize*2,"Removing setuid (root) flag from %s...\n\n",file);
	                 CfLog(cfinform,OUTPUT,"");
			 
	                 if (ptr != NULL)
                            {
                            AddMultipleClasses(ptr->defines);
                            }
                         break;
         case warnall:
         case warndirs:
         case warnplain: if (amroot)
	                    {
      	                    snprintf(OUTPUT,bufsize*2,"WARNING setuid (root) flag on %s...\n\n",file);
			    CfLog(cferror,OUTPUT,"");
	                    }
                         break;

         }
      }
   }

if (dstat->st_uid == 0 && (dstat->st_mode & S_ISGID))
   {
   if (newperm & S_ISGID)
      {
      if (! IsItemIn(VSETUIDLIST,file))
         {
         if (S_ISDIR(dstat->st_mode))
            {
            /* setgid directory */
            }
         else
            {
            if (amroot)
	       {
	       snprintf(OUTPUT,bufsize*2,"NEW SETGID root PROGRAM %s\n",file);
	       CfLog(cferror,OUTPUT,"");
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
         case fixplain: snprintf(OUTPUT,bufsize*2,"Removing setgid (root) flag from %s...\n\n",file);
	                CfLog(cfinform,OUTPUT,"");
	                if (ptr != NULL)
			   {
                           AddMultipleClasses(ptr->defines);
			   }
	                break;
         case warnall:
         case warndirs:
         case warnplain: snprintf(OUTPUT,bufsize*2,"WARNING setgid (root) flag on %s...\n\n",file);
	                 CfLog(cferror,OUTPUT,"");
                         break;

         default:        break;
         }
      }
   }

if (CheckOwner(file,action,uidlist,gidlist,dstat))
   {
   if (ptr != NULL)
      {
      AddMultipleClasses(ptr->defines);
      }
   }

if ((ptr != NULL) && S_ISREG(dstat->st_mode) && (ptr->checksum != 'n'))
    {
    Debug("Checking checksum integrity of %s\n",file);

    bzero(digest,EVP_MAX_MD_SIZE+1);
    ChecksumFile(file,digest,ptr->checksum);

    if (!DONTDO)
       {
       if (ChecksumChanged(file,digest,cferror,false,ptr->checksum))
	  {
	  }
       }
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
   snprintf(OUTPUT,bufsize*2,"Can't stat file %s while looking at permissions (was not copied?)\n",file);
   CfLog(cfverbose,OUTPUT,"stat");
   umask(maskvalue);
   return;
   }

if (CheckACLs(file,action,acl_aliases))
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
if ((newperm & 07777) == (dstat->st_mode & 07777))    /* file okay */
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
	     snprintf(OUTPUT,bufsize*2,"%s has permission %o\n",file,dstat->st_mode & 07777);
	     CfLog(cferror,OUTPUT,"");
	     snprintf(OUTPUT,bufsize*2,"[should be %o]\n",newperm & 07777);
	     CfLog(cferror,OUTPUT,"");
	     }
	  break;
      case warndirs:
	  if (S_ISDIR(dstat->st_mode))
	     {
	     snprintf(OUTPUT,bufsize*2,"%s has permission %o\n",file,dstat->st_mode & 07777);
	     CfLog(cferror,OUTPUT,"");
	     snprintf(OUTPUT,bufsize*2,"[should be %o]\n",newperm & 07777);
	     CfLog(cferror,OUTPUT,"");
	     }
	  break;
      case warnall:   
	  snprintf(OUTPUT,bufsize*2,"%s has permission %o\n",file,dstat->st_mode & 07777);
	  CfLog(cferror,OUTPUT,"");
	  snprintf(OUTPUT,bufsize*2,"[should be %o]\n",newperm & 07777);
	  CfLog(cferror,OUTPUT,"");
	  break;
	  
      case fixplain:
	  if (S_ISREG(dstat->st_mode))
	     {
	     if (! DONTDO)
		{
		if (chmod (file,newperm & 07777) == -1)
		   {
		   snprintf(OUTPUT,bufsize*2,"chmod failed on %s\n",file);
		   CfLog(cferror,OUTPUT,"chmod");
		   break;
		   }
		}
	     
	     snprintf(OUTPUT,bufsize*2,"%s had permission %o, changed it to %o\n",
		     file,dstat->st_mode & 07777,newperm & 07777);
	     CfLog(cfinform,OUTPUT,"");
	     
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
		   snprintf(OUTPUT,bufsize*2,"chmod failed on %s\n",file);
		   CfLog(cferror,OUTPUT,"chmod");
		   break;
		   }
		}
	     
	     snprintf(OUTPUT,bufsize*2,"%s had permission %o, changed it to %o\n",
		     file,dstat->st_mode & 07777,newperm & 07777);
	     CfLog(cfinform,OUTPUT,"");
	     
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
		snprintf(OUTPUT,bufsize*2,"chmod failed on %s\n",file);
		CfLog(cferror,OUTPUT,"chmod");
		break;
		}
	     }
	  
	  snprintf(OUTPUT,bufsize*2,"%s had permission %o, changed it to %o\n",
		  file,dstat->st_mode & 07777,newperm & 07777);
	  CfLog(cfinform,OUTPUT,"");
	  
	  if (ptr != NULL)
	     {
	     AddMultipleClasses(ptr->defines);
	     }
	  break;
	  
      case touch:
	  if (! DONTDO)
	     {
	     if (chmod (file,newperm & 07777) == -1)
		{
		snprintf(OUTPUT,bufsize*2,"chmod failed on %s\n",file);
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
		snprintf(OUTPUT,bufsize*2,"%s has flags %o\n",file,dstat->st_flags & CHFLAGS_MASK);
		CfLog(cferror,OUTPUT,"");
		snprintf(OUTPUT,bufsize*2,"[should be %o]\n",newflags & CHFLAGS_MASK);
		CfLog(cferror,OUTPUT,"");
		}
	     break;
	 case warndirs:
	     if (S_ISDIR(dstat->st_mode))
		{
		snprintf(OUTPUT,bufsize*2,"%s has flags %o\n",file,dstat->st_mode & CHFLAGS_MASK);
		CfLog(cferror,OUTPUT,"");
		snprintf(OUTPUT,bufsize*2,"[should be %o]\n",newflags & CHFLAGS_MASK);
		CfLog(cferror,OUTPUT,"");
		}
	     break;
	 case warnall:
	     snprintf(OUTPUT,bufsize*2,"%s has flags %o\n",file,dstat->st_mode & CHFLAGS_MASK);
	     CfLog(cferror,OUTPUT,"");
	     snprintf(OUTPUT,bufsize*2,"[should be %o]\n",newflags & CHFLAGS_MASK);
	     CfLog(cferror,OUTPUT,"");
	     break;
	     
	 case fixplain:
	     
	     if (S_ISREG(dstat->st_mode))
		{
		if (! DONTDO)
		   {
		   if (chflags (file,newflags & CHFLAGS_MASK) == -1)
		      {
		      snprintf(OUTPUT,bufsize*2,"chflags failed on %s\n",file);
		      CfLog(cferror,OUTPUT,"chflags");
		      break;
		      }
		   }
		
		snprintf(OUTPUT,bufsize*2,"%s had flags %o, changed it to %o\n",
			file,dstat->st_flags & CHFLAGS_MASK,newflags & CHFLAGS_MASK);
		CfLog(cfinform,OUTPUT,"");
		
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
		      snprintf(OUTPUT,bufsize*2,"chflags failed on %s\n",file);
		      CfLog(cferror,OUTPUT,"chflags");
		      break;
		      }
		   }
		
		snprintf(OUTPUT,bufsize*2,"%s had flags %o, changed it to %o\n",
			file,dstat->st_flags & CHFLAGS_MASK,newflags & CHFLAGS_MASK);
		CfLog(cfinform,OUTPUT,"");
		
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
		snprintf(OUTPUT,bufsize*2,"chflags failed on %s\n",file);
		CfLog(cferror,OUTPUT,"chflags");
		break;
		}
	     }
	  
	  snprintf(OUTPUT,bufsize*2,"%s had flags %o, changed it to %o\n",
		  file,dstat->st_flags & CHFLAGS_MASK,newflags & CHFLAGS_MASK);
	  CfLog(cfinform,OUTPUT,"");
	  
	  if (ptr != NULL)
	     {
	     AddMultipleClasses(ptr->defines);
	     }
	  break;
	  
      case touch:
	  if (! DONTDO)
	     {
	     if (chflags (file,newflags & CHFLAGS_MASK) == -1)
		{
		snprintf(OUTPUT,bufsize*2,"chflags failed on %s\n",file);
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

void CheckCopiedFile(file,plus,minus,action,uidlist,gidlist,dstat,sstat,ptr,acl_aliases)

char *file;
mode_t plus,minus;
struct UidList *uidlist;
struct GidList *gidlist;
enum fileactions action;
struct stat *dstat;
struct stat *sstat;
struct File *ptr;
struct Item *acl_aliases;

{ mode_t newplus,newminus;

 /* plus/minus must be relative to source file, not to
    perms of newly created file! */

Debug("CheckCopiedFile(%s)\n",file); 

if ((plus == 0) && (minus == 0))
    {
    newplus = sstat->st_mode & 07777 | plus;
    newminus = ~(sstat->st_mode & 07777 & ~minus) & 07777;
    CheckExistingFile(file,newplus,newminus,fixall,uidlist,gidlist,dstat,NULL,acl_aliases);
    }
 else
    {
    CheckExistingFile(file,plus,minus,fixall,uidlist,gidlist,dstat,NULL,acl_aliases);
    }
}

/*********************************************************************/

int CheckOwner(file,action,uidlist,gidlist,statbuf)

char *file;
enum fileactions action;
struct UidList *uidlist;
struct GidList *gidlist;
struct stat *statbuf;

{ struct passwd *pw;
  struct group *gp;
  struct UidList *ulp, *unknownulp;
  struct GidList *glp, *unknownglp;
  short uidmatch = false, gidmatch = false;
  uid_t uid = sameowner; 
  gid_t gid = samegroup;

Debug("CheckOwner: %d\n",statbuf->st_uid);
  
for (ulp = uidlist; ulp != NULL; ulp=ulp->next)
   {
   Debug(" uid %d\n",ulp->uid);
   
   if (ulp->uid == unknown_owner) /* means not found while parsing */
      {
      unknownulp = MakeUidList (ulp->uidname);	/* Will only match one */
      if (unknownulp != NULL && statbuf->st_uid == unknownulp->uid)
	 {
	 uid = unknownulp->uid;
	 uidmatch = true;
	 break;
	 }
      }
   
   if (ulp->uid == sameowner || statbuf->st_uid == ulp->uid)   /* "same" matches anything */
      {
      uid = ulp->uid;
      uidmatch = true;
      break;
      }
   }

for (glp = gidlist; glp != NULL; glp=glp->next)
   {
   if (glp->gid == unknown_group) /* means not found while parsing */
      {
      unknownglp = MakeGidList (glp->gidname);	/* Will only match one */
      if (unknownglp != NULL && statbuf->st_gid == unknownglp->gid)
	 {
	 gid = unknownglp->gid;
	 gidmatch = true;
	 break;
	 }
      }
   if (glp->gid == samegroup || statbuf->st_gid == glp->gid)  /* "same" matches anything */
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
      for (ulp = uidlist; ulp != NULL; ulp=ulp->next)
	 {
	 if (uidlist->uid != unknown_owner)
	    {
	    uid = uidlist->uid;    /* default is first (not unknown) item in list */
	    break;
	    }
	 }
      }
   
   if (! gidmatch)
      {
      for (glp = gidlist; glp != NULL; glp=glp->next)
	 {
	 if (gidlist->gid != unknown_group)
	    {
	    gid = gidlist->gid;    /* default is first (not unknown) item in list */
	    break;
	    }
	 }
      }

   if (S_ISLNK(statbuf->st_mode) && (action == fixdirs || action == fixplain))
      {
      Debug2("File %s incorrect type (link), skipping...\n",file);
      return false;
      }

   if ((S_ISREG(statbuf->st_mode) && action == fixdirs) || (S_ISDIR(statbuf->st_mode) && action == fixplain))
      {
      Debug2("File %s incorrect type, skipping...\n",file);
      return false;
      }

   switch (action)
      {
      case fixplain:
      case fixdirs:
      case fixall: 
      case touch:
          	  if (VERBOSE || DEBUG || D2)
		     {
		     if (uid == sameowner && gid == samegroup)
			{
			printf("%s:   touching %s\n",VPREFIX,file);
			}
		     else
                        {
                        if (uid != sameowner)
                           {
                           Debug("(Change owner to uid %d if possible)\n",uid);
                           }

                        if (gid != samegroup)
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
                        snprintf(OUTPUT,bufsize*2,"Cannot set ownership on link %s!\n",file);
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
                     if (chown(file,uid,gid) == -1)
                        {
                        snprintf(OUTPUT,bufsize*2,"Cannot set ownership on file %s!\n",file);
                        CfLog(cflogonly,OUTPUT,"chown");
                        }
		     else
			{
			return true;
			}
                     }
                  break;

      case linkchildren:
      case warnall: 
      case warndirs:
      case warnplain:
                  if ((pw = getpwuid(statbuf->st_uid)) == NULL)
                     {
                     snprintf(OUTPUT,bufsize*2,"File %s is not owned by anybody in the passwd database\n",file);
		     CfLog(cferror,OUTPUT,"");
                     snprintf(OUTPUT,bufsize*2,"(uid = %d,gid = %d)\n",statbuf->st_uid,statbuf->st_gid);
		     CfLog(cferror,OUTPUT,"");
                     break;
                     }

                  if ((gp = getgrgid(statbuf->st_gid)) == NULL)
                     {
                     snprintf(OUTPUT,bufsize*2,"File %s is not owned by any group in group database\n",file);
		     CfLog(cferror,OUTPUT,"");
                     break;
                     }

                  snprintf(OUTPUT,bufsize*2,"File %s is owned by [%s], group [%s]\n",file,pw->pw_name,gp->gr_name);
		  CfLog(cferror,OUTPUT,"");
                  break;
      }
   }

return false; 
}


/*********************************************************************/

int CheckHomeSubDir(testpath,tidypath,recurse)

char *testpath, *tidypath;
int recurse;

{ char *subdirstart, *sp1, *sp2;
  char buffer[bufsize];
  int homelen;

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

   Debug2("CheckHomeSubDir(%s,%s)\n",testpath,tidypath);

   subdirstart = tidypath + 4;                                   /* Ptr to start of subdir */

   strcpy(buffer,testpath);

   ChopLastNode(buffer);                                         /* Filename only */

   for (sp1 = buffer + homelen +1; *sp1 != '/' && *sp1 != '\0'; sp1++) /* skip user name dir */
      {
      }

   sp2 = subdirstart;

   if (strncmp(sp1,sp2,strlen(sp2)) != 0)
      {
      return false;
      } 

   Debug2("CheckHomeSubDir(true)\n");
   return(true);
   }

return true;
}

/**************************************************************/

int FileIsNewer(file1,file2)

/* True if file2 is newer than file 1 */

char *file1, *file2;

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
/* Used by files and tidy modules                                    */
/*********************************************************************/

int IgnoreFile (pathto,name,ignores)

char *pathto, *name;
struct Item *ignores;

{ struct Item *ip;

Debug("IgnoreFile(%s)\n",name);
 
if (name == NULL || strlen(name) == 0)
   {
   return false;
   }
 
strcpy(VBUFF,pathto);
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

void CompressFile(file)

char *file;

{ char comm[bufsize];
  FILE *pp;
 
if (strlen(COMPRESSCOMMAND) == 0)
   {
   CfLog(cferror,"CompressCommand variable is not defined","");
   return;
   }
 
snprintf(comm,bufsize,"%s %s",COMPRESSCOMMAND,file); 

snprintf(OUTPUT,bufsize*2,"Compressing file %s",file); 
CfLog(cfinform,OUTPUT,"");
 
if ((pp=cfpopen(comm,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Compression command failed on %s",file);
   CfLog(cfverbose,OUTPUT,"");
   }

while (!feof(pp))
   {
   ReadLine(VBUFF,bufsize,pp);
   CfLog(cfinform,VBUFF,"");
   }

cfpclose(pp); 
}

