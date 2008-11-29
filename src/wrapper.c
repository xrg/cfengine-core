/* 

        Copyright (C) 1995-2000
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
/*****************************************************************************/
/*                                                                           */
/* File: wrapper.c                                                           */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

/* These functions are wrappers for the real functions so that
    we can abstract the task of parsing general wildcard paths
    like /a*b/bla?/xyz from a single function ExpandWildCardsAndDo() */

/*******************************************************************/

void TidyWrapper(char *startpath,void *vp)

{ struct Tidy *tp;
  struct stat sb; 

tp = (struct Tidy *) vp;

Debug2("TidyWrapper(%s)\n",startpath);

if (tp->done == 'y')
   {
   return;
   }

tp->done = 'y'; 

if (!GetLock(ASUniqueName("tidy"),CanonifyName(startpath),tp->ifelapsed,tp->expireafter,VUQNAME,CFSTARTTIME))
   {
   return;
   }

if (stat(startpath,&sb) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Tidy directory %s cannot be accessed",startpath);
   CfLog(cfinform,OUTPUT,"stat");
   ReleaseCurrentLock(); 
   return;
   }

RegisterRecursionRootDevice(sb.st_dev);
RecursiveTidySpecialArea(startpath,tp,tp->maxrecurse,&sb);
tp->done = 'y';

ReleaseCurrentLock(); 
}

/*******************************************************************/

void RecHomeTidyWrapper(char *startpath,void *vp)

{ struct stat sb;
  struct Tidy *tp = (struct Tidy *) vp;
 
Verbose("Tidying Home partition %s...\n",startpath);

if (tp != NULL)
   {
   if (!GetLock(ASUniqueName("tidy"),CanonifyName(startpath),tp->ifelapsed,tp->expireafter,VUQNAME,CFSTARTTIME))
      {
      return;
      }
   }
else
   {
   if (!GetLock(ASUniqueName("tidy"),CanonifyName(startpath),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      return;
      }
   }

if (stat(startpath,&sb) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Tidy directory %s cannot be accessed",startpath);
   CfLog(cfinform,OUTPUT,"stat");
   ReleaseCurrentLock();
   return;
   }

RegisterRecursionRootDevice(sb.st_dev); 
RecursiveHomeTidy(startpath,1,&sb);

ReleaseCurrentLock();
}

/*******************************************************************/

void CheckFileWrapper(char *startpath,void *vp)

{  struct stat statbuf;
   mode_t filemode;
   char *lastnode, lock[CF_MAXVARSIZE];
   int fd;
   struct File *ptr;

ptr = (struct File *)vp;

if (ptr->uid != NULL)
   {
   snprintf(lock,CF_MAXVARSIZE-1,"%s_%o_%o_%d",startpath,ptr->plus,ptr->minus,ptr->uid->uid);
   }
else
   {
   snprintf(lock,CF_MAXVARSIZE-1,"%s_%o_%o",startpath,ptr->plus,ptr->minus);
   }
 
 
if (!GetLock(ASUniqueName("files"),CanonifyName(lock),ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
   {
   return;
   }

if ((strlen(startpath) == 0) || (startpath == NULL))
   {
   return;
   }
 
if (ptr->action == touch && IsWildCard(ptr->path))
   {
   printf("%s: Can't touch a wildcard! (%s)\n",VPREFIX,ptr->path);
   return;
   }

for (lastnode = startpath+strlen(startpath)-1; *lastnode != '/'; lastnode--)
   {
   }

lastnode++;
 
if (ptr->inclusions != NULL && !IsWildItemIn(ptr->inclusions,lastnode))
   {
   Debug2("cfengine: skipping non-included pattern %s\n",lastnode);

   if (stat(startpath,&statbuf) != -1)
      {
      if (!S_ISDIR(statbuf.st_mode))
         {
         return;  /* assure that we recurse into directories */
         }
      }
   }

if (IsWildItemIn(ptr->exclusions,lastnode))
   {
   Debug2("Skipping excluded pattern file %s\n",lastnode);
   ReleaseCurrentLock();
   return;
   }

Debug("Checking wrapped file object %s\n",ptr->path);

if (stat(startpath,&statbuf) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot access file/directory %s\n",ptr->path);
   CfLog(cfinform,OUTPUT,"stat");

   if (TouchDirectory(ptr))                /* files ending in /. */
      {
      MakeDirectoriesFor(startpath,'n');
      ptr->action = fixall;
      *(startpath+strlen(ptr->path)-2) = '\0';       /* trunc /. */
      CheckExistingFile("*",startpath,&statbuf,ptr);
      ReleaseCurrentLock();
      return;
      }

   if (ptr->plus == 0 && ptr->minus == 0)
      {
      filemode = 0600;
      }
   else
      {
      filemode = DEFAULTMODE;      /* Decide the mode for filecreation */
      filemode |=   ptr->plus;
      filemode &= ~(ptr->minus);
      }

   switch (ptr->action)
      {
      case create:
      case touch:
          if (! DONTDO)
             {
             MakeDirectoriesFor(startpath,'n');
             
             if ((fd = creat(ptr->path,filemode)) == -1)
                { 
                perror("creat");
                AddMultipleClasses(ptr->elsedef);
                return;
                }
             else
                {
                AddMultipleClasses(ptr->defines);
                close(fd);
                }
             
             CheckExistingFile("*",startpath,&statbuf,ptr);
             }
          
          snprintf(OUTPUT,CF_BUFSIZE*2,"Creating file %s, mode = %o\n",ptr->path,filemode);
          CfLog(cfinform,OUTPUT,"");
          break;
          
      case alert:
      case linkchildren: 
      case warnall:
      case warnplain:
      case warndirs:
      case fixplain:
      case fixdirs:
      case fixall:
          snprintf(OUTPUT,CF_BUFSIZE*2,"File/Dir %s did not exist and was marked (%s)\n",ptr->path,FILEACTIONTEXT[ptr->action]);
          CfLog(cfinform,OUTPUT,"");
          break;
      case compress:
          break;
          
      default:      FatalError("cfengine: Internal sofware error: Checkfiles(), bad action\n");
      }
   }
else
   { struct Link empty;

   empty.inclusions = ptr->inclusions;
   empty.exclusions = ptr->exclusions;
   empty.defines = NULL;
   empty.elsedef = NULL;
   empty.nofile = false;

   if (ptr->action == create)
      {
      struct File tmp;
      memcpy(&tmp,ptr,sizeof(struct File));
      tmp.action = fixall;

      if (tmp.plus == 0 && tmp.minus == 0)
         {
         tmp.plus = 0600;
         }
      
      CheckExistingFile("*",startpath,&statbuf,&tmp);
      ReleaseCurrentLock();
      return;
      }
   
   if (ptr->action == linkchildren)
      {
      LinkChildren(ptr->path,'s',&statbuf,ptr->uid->uid,ptr->gid->gid,&empty);
      ReleaseCurrentLock();
      return;
      }
   
   if (S_ISDIR(statbuf.st_mode) && (ptr->recurse != 0))
      {
      if (IgnoreFile(startpath,ReadLastNode(startpath),ptr->ignores))
         {
         Verbose("%s: (Skipping ignored directory %s)\n",VPREFIX,startpath);
         return;
         }
      
      CheckExistingFile("*",startpath,&statbuf,ptr);
      RegisterRecursionRootDevice(statbuf.st_dev);
      RecursiveCheck(startpath,ptr->recurse,0,ptr,&statbuf);
      }
   else
      {
      if (lstat(startpath,&statbuf) == -1)
         {
         CfLog(cferror,"Unable to stat already statted object!","lstat");
         return;
         }
      CheckExistingFile("*",startpath,&statbuf,ptr);
      }
   }
 
ReleaseCurrentLock();
}

/*******************************************************************/

void DirectoriesWrapper(char *dir,void *vp)

{ struct stat statbuf;
  char directory[CF_EXPANDSIZE];
  struct File *ptr;
  int succeeded = false;
  int ok = false;

memset(directory,0,CF_BUFSIZE);
ExpandVarstring(dir,directory,"");
ptr = (struct File *)vp;

if (stat(directory,&statbuf) == 0)
   {
   if (S_ISDIR(statbuf.st_mode))
      {
      Verbose("Directory %s okay\n",directory);
      ok = true;
      }
   else
      {
      snprintf(OUTPUT,CF_BUFSIZE,"A non-directory object prevents directory %s from being created\n",directory);
      CfLog(cferror,OUTPUT,"");
      AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_FAIL);
      succeeded = false;
      AddMultipleClasses(ptr->elsedef);
      return;
      }
   }

if (!ok)
   {
   ExpandVarstring(dir,directory,"");
   AddSlash(directory);
   strcat(directory,".");
   MakeDirectoriesFor(directory,'n');

   if (stat(directory,&statbuf) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot stat %s after creating it",directory);
      CfLog(cfinform,OUTPUT,"stat");
      }
   else
      {
      succeeded = true;
      }
   }

if (!GetLock(ASUniqueName("directories"),CanonifyName(directory),ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
   {
   return;
   }

CheckExistingFile("*",dir,&statbuf,ptr);

if (ptr != NULL)
   {
   if (succeeded)
      {
      AddMultipleClasses(ptr->defines);
      }
   else
      {
      AddMultipleClasses(ptr->elsedef);
      }
   }

ReleaseCurrentLock(); 
}

/*******************************************************************/

int TouchDirectory(struct File *ptr)                     /* True if file path in /. */

{ char *sp;

if (ptr->action == touch||ptr->action == create)
   {
   sp = ptr->path+strlen(ptr->path)-2;

   if (strcmp(sp,"/.") == 0)
      {
      return(true);
      }
   else
      {
      return false;
      }
   }
else
   {
   return false;
   }
}

