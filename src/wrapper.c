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

void TidyWrapper(startpath,vp)

char *startpath;
void *vp;

{ struct Tidy *tp;

tp = (struct Tidy *) vp;

Debug2("TidyWrapper(%s)\n",startpath);

if (tp->done == 'y')
   {
   return;
   }

tp->done = 'y'; 

if (!GetLock(ASUniqueName("tidy"),CanonifyName(startpath),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }
 
RecursiveTidySpecialArea(startpath,tp,tp->maxrecurse);

ReleaseCurrentLock(); 
}

/*******************************************************************/

void RecHomeTidyWrapper(startpath,vp)

char *startpath;
void *vp;

{
Verbose("Tidying Home partition %s...\n",startpath);

if (!GetLock(ASUniqueName("tidy"),CanonifyName(startpath),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }

RecursiveHomeTidy(startpath,1);
 
ReleaseCurrentLock();
}

/*******************************************************************/

void CheckFileWrapper(startpath,vp)

char *startpath;
void *vp;

{  struct stat statbuf;
   mode_t filemode;
   char *lastnode, lock[maxvarsize];
   int fd;
   struct File *ptr;

ptr = (struct File *)vp;

if (ptr->uid != NULL)
   {
   snprintf(lock,maxvarsize-1,"%s_%o_%o_%d",startpath,ptr->plus,ptr->minus,ptr->uid->uid);
   }
else
   {
   snprintf(lock,maxvarsize-1,"%s_%o_%o",startpath,ptr->plus,ptr->minus);
   }
 
 
if (!GetLock(ASUniqueName("files"),CanonifyName(lock),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
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
    
if (stat(startpath,&statbuf) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Cannot access file/directory %s\n",ptr->path);
   CfLog(cfinform,OUTPUT,"stat");

   if (TouchDirectory(ptr))                /* files ending in /. */
      {
      MakeDirectoriesFor(startpath,'n');
      ptr->action = fixall;
      *(startpath+strlen(ptr->path)-2) = '\0';       /* trunc /. */
      CheckExistingFile(startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,&statbuf,ptr,ptr->acl_aliases);
      ReleaseCurrentLock();
      return;
      }

   filemode = DEFAULTMODE;      /* Decide the mode for filecreation */
   filemode |=   ptr->plus;
   filemode &= ~(ptr->minus);

   switch (ptr->action)
      {
      case create:
      case touch:   if (! DONTDO)
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

		       CheckExistingFile(startpath,ptr->plus,ptr->minus,fixall,ptr->uid,ptr->gid,&statbuf,ptr,ptr->acl_aliases);
                       }

                    snprintf(OUTPUT,bufsize*2,"Creating file %s, mode = %o\n",ptr->path,filemode);
		    CfLog(cfinform,OUTPUT,"");
                    break;
      case alert:
      case linkchildren: 
      case warnall:
      case warnplain:
      case warndirs:
      case fixplain:
      case fixdirs:
      case fixall:  snprintf(OUTPUT,bufsize*2,"File/Dir %s did not exist and was marked (%s)\n",ptr->path,FILEACTIONTEXT[ptr->action]);
	            CfLog(cfinform,OUTPUT,"");
                    break;
      case compress:
	            break;

      default:      FatalError("cfengine: Internal sofware error: Checkfiles(), bad action\n");
      }
   }
else
   {
/*
   if (TouchDirectory(ptr)) Don't check, just touch..
      {
      ReleaseCurrentLock();
      return;
      }
*/
   if (ptr->action == create)
      {
      ReleaseCurrentLock();
      return;
      }

   if (ptr->action == linkchildren)
      {
      LinkChildren(ptr->path,
		   's',
		   &statbuf,
		   ptr->uid->uid,ptr->gid->gid,
		   ptr->inclusions,
		   ptr->exclusions,
		   NULL,
		   0,
		   NULL);
      ReleaseCurrentLock();
      return;
      }

   if (S_ISDIR(statbuf.st_mode) && (ptr->recurse != 0))
      {
      if (!IgnoreFile(startpath,ReadLastNode(startpath),ptr->ignores))
	 {
	 Verbose("%s: Skipping ignored directory %s\n",VPREFIX,startpath);
	 CheckExistingFile(startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,&statbuf,ptr,ptr->acl_aliases);
	 }

          
      RecursiveCheck(startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,ptr->recurse,0,ptr);
      }
   else
      {
      CheckExistingFile(startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,&statbuf,ptr,ptr->acl_aliases);
      }
   }

ReleaseCurrentLock();
}

/*******************************************************************/

void DirectoriesWrapper(dir,vp)

char *dir;
void *vp;

{ struct stat statbuf;
  char directory[bufsize];
  struct File *ptr;

ptr=(struct File *)vp;
 
bzero(directory,bufsize);
ExpandVarstring(dir,directory,"");

AddSlash(directory);
strcat(directory,".");
 
MakeDirectoriesFor(directory,'n');
 
if (stat(directory,&statbuf) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Cannot stat %s\n",directory);
   CfLog(cfinform,OUTPUT,"stat");
   return;
   }

if (!GetLock(ASUniqueName("directories"),CanonifyName(directory),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }

CheckExistingFile(dir,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,&statbuf,ptr,ptr->acl_aliases);
ReleaseCurrentLock(); 
}

