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
 

/********************************************************************/
/*                                                                  */
/* EDITING of simple textfiles (Toolkit)                            */
/*                                                                  */
/********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/********************************************************************/
/* EDIT Data structure routines                                     */
/********************************************************************/

int DoRecursiveEditFiles(name,level,ptr,sb)

char *name;
int level;
struct Edit *ptr;
struct stat *sb;

{ DIR *dirh;
  struct dirent *dirp;
  char pcwd[bufsize];
  struct stat statbuf;
  int goback;

if (level == -1)
   {
   return false;
   }

Debug("RecursiveEditFiles(%s)\n",name);

if (!DirPush(name,sb))
   {
   return false;
   }
 
if ((dirh = opendir(".")) == NULL)
   {
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
      return true;
      }

   strcat(pcwd,dirp->d_name);

   if (!FileObjectFilter(pcwd,&statbuf,ptr->filters,editfiles))
      {
      Verbose("Skipping filtered file %s\n",pcwd);
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
   else
      {
      if (lstat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"RecursiveCheck was working in %s when this happened:\n",pcwd);
         CfLog(cferror,OUTPUT,"lstat");
         continue;
         }
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      if (IsMountedFileSystem(&statbuf,pcwd,level))
         {
         continue;
         }
      else
         {
         if ((ptr->recurse > 1) || (ptr->recurse == INFINITERECURSE))
            {
            goback = DoRecursiveEditFiles(pcwd,level-1,ptr,&statbuf);
	    DirPop(goback,name,sb);
            }
         else
            {
            WrapDoEditFile(ptr,pcwd);
            }
         }
      }
   else
      {
      WrapDoEditFile(ptr,pcwd);
      }
   }

closedir(dirh);
return true; 
}

/********************************************************************/

void DoEditHomeFiles(ptr)

struct Edit *ptr;

{ DIR *dirh, *dirh2;
  struct dirent *dirp, *dirp2;
  char *sp,homedir[bufsize],dest[bufsize];
  struct passwd *pw;
  struct stat statbuf;
  struct Item *ip;
  uid_t uid;
  
if (!MountPathDefined())
   {
   CfLog(cfinform,"Mountpattern is undefined\n","");
   return;
   }

for (ip = VMOUNTLIST; ip != NULL; ip=ip->next)
   {
   if (IsExcluded(ip->classes))
      {
      continue;
      }
   
   if ((dirh = opendir(ip->name)) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",ip->name);
      CfLog(cferror,OUTPUT,"opendir");
      return;
      }

   for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,ip->name,NULL))
         {
         continue;
         }

      strcpy(homedir,ip->name);
      AddSlash(homedir);
      strcat(homedir,dirp->d_name);

      if (! IsHomeDir(homedir))
         {
         continue;
         }

      if ((dirh2 = opendir(homedir)) == NULL)
         {
         snprintf(OUTPUT,bufsize*2,"Can't open directory%s\n",homedir);
         CfLog(cferror,OUTPUT,"opendir");
         return;
         }

      for (dirp2 = readdir(dirh2); dirp2 != NULL; dirp2 = readdir(dirh2))
         {
         if (!SensibleFile(dirp2->d_name,homedir,NULL))
	    {
	    continue;
	    }

         strcpy(dest,homedir);
         AddSlash(dest);
         strcat(dest,dirp2->d_name);
         AddSlash(dest);
         sp = ptr->fname + strlen("home/");
         strcat(dest,sp);

         if (stat(dest,&statbuf))
            {
            EditVerbose("File %s doesn't exist for editing, skipping\n",dest);
            continue;
            }
      
         if ((pw = getpwnam(dirp2->d_name)) == NULL)
            {
            Debug2("cfengine: directory corresponds to no user %s - ignoring\n",dirp2->d_name);
            continue;
            }
         else
            {
            Debug2("(Setting user id to %s)\n",dirp2->d_name);
            }

         uid = statbuf.st_uid;

         WrapDoEditFile(ptr,dest);
      
         chown(dest,uid,sameowner);
         }
      closedir(dirh2);
      }
   closedir(dirh);
   }
}

/********************************************************************/

void WrapDoEditFile(ptr,filename)

struct Edit *ptr;
char *filename;

{ struct stat statbuf,statbuf2;
  char linkname[bufsize];
  char realname[bufsize];

Debug("WrapDoEditFile(%s,%s)\n",ptr->fname,filename);
  
if (lstat(filename,&statbuf) != -1)
   {
   if (S_ISLNK(statbuf.st_mode))
      {
      EditVerbose("File %s is a link, editing real file instead\n",filename);
      
      bzero(linkname,bufsize);
      bzero(realname,bufsize);
      
      if (readlink(filename,linkname,bufsize-1) == -1)
	 {
	 snprintf(OUTPUT,bufsize*2,"Cannot read link %s\n",filename);
	 CfLog(cferror,OUTPUT,"readlink");
	 return;
	 }
      
      if (linkname[0] != '/')
	 {
	 strcpy(realname,filename);
	 ChopLastNode(realname);
	 AddSlash(realname);
	 }
      
      if (BufferOverflow(realname,linkname))
	 {
	 snprintf(OUTPUT,bufsize*2,"(culprit %s in editfiles)\n",filename);
	 CfLog(cferror,OUTPUT,"");
	 return;
	 }
      
      if (stat(filename,&statbuf2) != -1)
	 {
	 if (statbuf2.st_uid != statbuf.st_uid)
	    {
	    /* Link to /etc/passwd? ouch! */
	    snprintf(OUTPUT,bufsize*2,"Forbidden to edit a link to another user's file with privilege (%s)",filename);
	    CfLog(cfinform,OUTPUT,"");
	    return;
	    }
	 }
      
      strcat(realname,linkname);

      if (!FileObjectFilter(realname,&statbuf2,ptr->filters,editfiles))
	 {
	 Debug("Skipping filtered editfile %s\n",filename);
	 return;
	 }
      DoEditFile(ptr,realname);
      return;
      }
   else
      {
      if (!FileObjectFilter(filename,&statbuf,ptr->filters,editfiles))
	 {
	 Debug("Skipping filtered editfile %s\n",filename);
	 return;
	 }
      DoEditFile(ptr,filename);
      return;
      }
   }
else
   {
   if (!FileObjectFilter(filename,&statbuf,ptr->filters,editfiles))
      {
      Debug("Skipping filtered editfile %s\n",filename);
      return;
      }
   DoEditFile(ptr,filename);
   }
}

/********************************************************************/

void DoEditFile(ptr,filename)

struct Edit *ptr;
char *filename;


   /* Many of the functions called here are defined in the */
   /* item.c toolkit since they operate on linked lists    */

{ struct Edlist *ep, *loopstart, *loopend, *ThrowAbort();
  struct Item *filestart = NULL, *newlineptr = NULL;
  char currenteditscript[bufsize], searchstr[bufsize], expdata[bufsize];
  char *sp, currentitem[maxvarsize];
  struct stat tmpstat;
  char spliton = ':';
  mode_t maskval;
  int todo = 0, potentially_outstanding = false;
  FILE *loop_fp = NULL;

Debug("DoEditFile(%s)\n",filename);
filestart = NULL;
currenteditscript[0] = '\0';
searchstr[0] = '\0';
bzero(EDITBUFF,bufsize);
AUTOCREATED = false;
IMAGEBACKUP = 's';

if (IgnoredOrExcluded(editfiles,filename,ptr->inclusions,ptr->exclusions))
   {
   Debug("Skipping excluded file %s\n",filename);
   return;
   }

for (ep = ptr->actions; ep != NULL; ep=ep->next)
   {
   if (!IsExcluded(ep->classes))
      {
      todo++;
      }
   }

if (todo == 0)   /* Because classes are stored per edit, not per file */
   {
   return;
   }

if (!GetLock(ASUniqueName("editfile"),CanonifyName(filename),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }
 
if (ptr->binary == 'y')
   {
   BinaryEditFile(ptr,filename);
   ReleaseCurrentLock();
   return;
   }

CheckEditSwitches(filename,ptr);

if (! LoadItemList(&filestart,filename))
   {
   CfLog(cfverbose,"File was marked for editing\n","");
   ReleaseCurrentLock();
   return;
   }

NUMBEROFEDITS = 0;
EDITVERBOSE = VERBOSE;
CURRENTLINENUMBER = 1;
CURRENTLINEPTR = filestart;
strcpy(COMMENTSTART,"# ");
strcpy(COMMENTEND,"");
EDITGROUPLEVEL = 0;
SEARCHREPLACELEVEL = 0;
FOREACHLEVEL = 0;
loopstart = NULL;

Verbose("Begin editing %s\n",filename);
ep = ptr->actions;

while (ep != NULL)
   {
   if (IsExcluded(ep->classes))
      {
      ep = ep->next;
      potentially_outstanding = true;
      continue;
      }
   
   ExpandVarstring(ep->data,expdata,NULL);
   
   Debug2("Edit action: %s\n",VEDITNAMES[ep->code]);

   switch(ep->code)
      {
      case NoEdit:
      case EditInform:
      case EditBackup:
      case EditUmask:
      case AutoCreate:
      case EditInclude:
      case EditExclude:
      case EditFilter:
      case DefineClasses:
      case ElseDefineClasses:
	       break;

      case EditUseShell:
	       if (strcmp(expdata,"false") == 0)
		  {
		  ptr->useshell = 'n';
		  }
	       break;

      case DefineInGroup:
                  for (sp = expdata; *sp != '\0'; sp++)
                      {
                      currentitem[0] = '\0';
                      sscanf(sp,"%[^,:.]",currentitem);
                      sp += strlen(currentitem);
                      AddClassToHeap(currentitem);
                      }
                  break;

      case CatchAbort:
	          EditVerbose("Caught Exception\n");
	          break;

      case SplitOn:
	          spliton = *(expdata);
		  EditVerbose("Split lines by %c\n",spliton);
		  break;

      case DeleteLinesStarting:
               while (DeleteItemStarting(&filestart,expdata))
                  {
                  }
               break;

      case DeleteLinesContaining:
               while (DeleteItemContaining(&filestart,expdata))
                  {
                  }
               break;

      case DeleteLinesAfterThisMatching:

               if ((filestart == NULL) || (CURRENTLINEPTR == NULL))
		  {
		  break;
		  }
	       else if (CURRENTLINEPTR->next != NULL)
		  {
                  while (DeleteItemMatching(&(CURRENTLINEPTR->next),expdata))
                    {
	   	    }
		  }
               break;	       

      case DeleteLinesMatching:
               while (DeleteItemMatching(&filestart,expdata))
                 {
		 }
               break;
	       
      case Append:
              AppendItem(&filestart,expdata,NULL);
              break;

      case AppendIfNoSuchLine:
               if (! IsItemIn(filestart,expdata))
                  {
                  AppendItem(&filestart,expdata,NULL);
                  }
               break;

      case SetLine:
               strcpy(EDITBUFF,expdata);
               EditVerbose("Set current line to %s\n",EDITBUFF);
               break;

      case AppendIfNoLineMatching:


    	       Debug("AppendIfNoLineMatching : %s\n",EDITBUFF);

               if (strcmp(EDITBUFF,"") == 0)
                  {
                  snprintf(OUTPUT,bufsize*2,"SetLine not set when calling AppendIfNoLineMatching %s\n",expdata);
		  CfLog(cferror,OUTPUT,"");
                  continue;
                  }

	       if (strcmp(expdata,"ThisLine") == 0)
		  {
		  if (LocateNextItemMatching(filestart,EDITBUFF) == NULL)
		     {
		     AppendItem(&filestart,EDITBUFF,NULL);
		     }		 

		  break;
		  }
	       
               if (LocateNextItemMatching(filestart,expdata) == NULL)
	          {
                  AppendItem(&filestart,EDITBUFF,NULL);
                  }
               break;

      case Prepend:
               PrependItem(&filestart,expdata,NULL);
               break;

      case PrependIfNoSuchLine:
               if (! IsItemIn(filestart,expdata))
                  {
                  PrependItem(&filestart,expdata,NULL);
                  }
               break;

      case PrependIfNoLineMatching:

               if (strcmp(EDITBUFF,"") == 0)
                  {
                  snprintf(OUTPUT,bufsize,"SetLine not set when calling PrependIfNoLineMatching %s\n",expdata);
		  CfLog(cferror,OUTPUT,"");
                  continue;
                  }

               if (LocateNextItemMatching(filestart,expdata) == NULL)
                  {
                  PrependItem(&filestart,EDITBUFF,NULL);
                  }
               break;

      case WarnIfNoSuchLine:
               if (LocateNextItemMatching(filestart,expdata) == NULL)
                  {
                  printf("Warning, file %s has no line matching %s\n",filename,expdata);
                  }
               break;

      case WarnIfLineMatching:
               if (LocateNextItemMatching(filestart,expdata) != NULL)
                  {
                  printf("Warning, file %s has a line matching %s\n",filename,expdata);
                  }
               break;

      case WarnIfNoLineMatching:
               if (LocateNextItemMatching(filestart,expdata) == NULL)
                  {
                  printf("Warning, file %s has a no line matching %s\n",filename,expdata);
                  }
               break;

      case WarnIfLineStarting:
               if (LocateNextItemStarting(filestart,expdata) != NULL)
                  {
                  printf("Warning, file %s has a line starting %s\n",filename,expdata);
                  }
               break;

      case WarnIfNoLineStarting:
               if (LocateNextItemStarting(filestart,expdata) == NULL)
                  {
                  printf("Warning, file %s has no line starting %s\n",filename,expdata);
                  }
               break;

      case WarnIfLineContaining:
               if (LocateNextItemContaining(filestart,expdata) != NULL)
                  {
                  printf("Warning, file %s has a line containing %s\n",filename,expdata);
                  }
               break;

      case WarnIfNoLineContaining:
               if (LocateNextItemContaining(filestart,expdata) == NULL)
                  {
                  printf("Warning, file %s has no line containing %s\n",filename,expdata);
                  }
               break;

      case SetCommentStart:
               strncpy(COMMENTSTART,expdata,maxvarsize);
               COMMENTSTART[maxvarsize-1] = '\0';
               break;

      case SetCommentEnd:
               strncpy(COMMENTEND,expdata,maxvarsize);
               COMMENTEND[maxvarsize-1] = '\0';
               break;

      case CommentLinesMatching:
               while (CommentItemMatching(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  }
               break;

      case CommentLinesStarting:
               while (CommentItemStarting(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  }
               break;

      case CommentLinesContaining:
               while (CommentItemContaining(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  }
               break;

      case HashCommentLinesContaining:
               while (CommentItemContaining(&filestart,expdata,"# ",""))
                  {
                  }
               break;

      case HashCommentLinesStarting:
               while (CommentItemStarting(&filestart,expdata,"# ",""))
                  {
                  }
               break;

      case HashCommentLinesMatching:
               while (CommentItemMatching(&filestart,expdata,"# ",""))
                  {
                  }
               break;

      case SlashCommentLinesContaining:
               while (CommentItemContaining(&filestart,expdata,"//",""))
                  {
                  }
               break;

      case SlashCommentLinesStarting:
               while (CommentItemStarting(&filestart,expdata,"//",""))
                  {
                  }
               break;

      case SlashCommentLinesMatching:
               while (CommentItemMatching(&filestart,expdata,"//",""))
                  {
                  }
               break;

      case PercentCommentLinesContaining:
               while (CommentItemContaining(&filestart,expdata,"%",""))
                  {
                  }
               break;

      case PercentCommentLinesStarting:
               while (CommentItemStarting(&filestart,expdata,"%",""))
                  {
                  }
               break;

      case PercentCommentLinesMatching:
               while (CommentItemMatching(&filestart,expdata,"%",""))
                  {
                  }
               break;

      case ResetSearch:
               if (!ResetEditSearch(expdata,filestart))
                  {
                  printf("ResetSearch Failed in %s, aborting editing\n",filename);
                  goto abort;
                  }
               break;

      case LocateLineMatching:

               if (CURRENTLINEPTR == NULL)
                  {
                  newlineptr == NULL;
                  }
               else
                  {
                  newlineptr = LocateItemMatchingRegExp(CURRENTLINEPTR,expdata);
                  }
	       
               if (newlineptr == NULL)
                  {
                  EditVerbose("LocateLineMatchingRegexp failed in %s, aborting editing\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case InsertLine:
	          if (filestart == NULL)
		     {
		     AppendItem(&filestart,expdata,NULL);
		     }
		  else
		     {
		     InsertItemAfter(&filestart,CURRENTLINEPTR,expdata);
		     }
                  break;

      case InsertFile:
	          InsertFileAfter(&filestart,CURRENTLINEPTR,expdata);
		  break;

      case IncrementPointer:
               if (! IncrementEditPointer(expdata,filestart))     /* edittools */
                  {
                  printf ("IncrementPointer failed in %s, aborting editing\n",filename);
  	          ep = ThrowAbort(ep);
                  }

               break;
	       
     case ReplaceLineWith:
               if (!ReplaceEditLineWith(expdata))
                  {
                  printf("Aborting edit of file %s\n",filename);
                  continue;
                  }
               break;

      case DeleteToLineMatching:
               if (! DeleteToRegExp(&filestart,expdata))
                  {
                  EditVerbose("Nothing matched DeleteToLineMatching regular expression\n");
                  EditVerbose("Aborting file editing of %s.\n" ,filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case DeleteNLines:
               if (! DeleteSeveralLines(&filestart,expdata))
                  {
                  EditVerbose("Could not delete %s lines from file\n",expdata);
                  EditVerbose("Aborting file editing of %s.\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case HashCommentToLineMatching:
               if (! CommentToRegExp(&filestart,expdata,"#",""))
                  {
                  EditVerbose("Nothing matched HashCommentToLineMatching regular expression\n");
                  EditVerbose("Aborting file editing of %s.\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case PercentCommentToLineMatching:
               if (! CommentToRegExp(&filestart,expdata,"%",""))
                  {
                  EditVerbose("Nothing matched PercentCommentToLineMatching regular expression\n");
                  EditVerbose("Aborting file editing of %s.\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case CommentToLineMatching:
               if (! CommentToRegExp(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  EditVerbose("Nothing matched CommentToLineMatching regular expression\n");
                  EditVerbose("Aborting file editing of %s.\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case CommentNLines:
               if (! CommentSeveralLines(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  EditVerbose("Could not comment %s lines from file\n",expdata);
                  EditVerbose("Aborting file editing of %s.\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;
	       
      case UnCommentNLines:
               if (! UnCommentSeveralLines(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  EditVerbose("Could not comment %s lines from file\n",expdata);
                  EditVerbose("Aborting file editing of %s.\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case UnCommentLinesContaining:
               while (UnCommentItemContaining(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  }
               break;

      case UnCommentLinesMatching:
               while (UnCommentItemMatching(&filestart,expdata,COMMENTSTART,COMMENTEND))
                  {
                  }
               break;
	       
      case SetScript:
                strncpy(currenteditscript, expdata, bufsize);
                currenteditscript[bufsize-1] = '\0';
                break;

      case RunScript:
               if (! RunEditScript(expdata,filename,&filestart,ptr))
                  {
                  printf("Aborting further edits to %s\n",filename);
                  ep = ThrowAbort(ep);
                  }
               break;

      case RunScriptIfNoLineMatching:
               if (! LocateNextItemMatching(filestart,expdata))
                  {
                  if (! RunEditScript(currenteditscript,filename,&filestart,ptr))
                     {
                     printf("Aborting further edits to %s\n",filename);
                     ep = ThrowAbort(ep);
                     }
                  }
               break;

      case RunScriptIfLineMatching:
               if (LocateNextItemMatching(filestart,expdata))
                  {
                  if (! RunEditScript(currenteditscript,filename,&filestart,ptr))
                     {
                     printf("Aborting further edits to %s\n",filename);
                     ep = ThrowAbort(ep);
                     }
                  }
               break;

      case EmptyEntireFilePlease:
               EditVerbose("Emptying entire file\n");
               DeleteItemList(filestart);
	       filestart = NULL;
	       CURRENTLINEPTR = NULL;
	       CURRENTLINENUMBER=0;
               NUMBEROFEDITS++;
               break;

      case GotoLastLine:
               GotoLastItem(filestart);
               break;

      case BreakIfLineMatches:
	       if (CURRENTLINEPTR == NULL || CURRENTLINEPTR->name == NULL )
		  {
		  EditVerbose("(BreakIfLIneMatches - no match for %s - file empty)\n",expdata);
		  break;
		  }
	  
               if (LineMatches(CURRENTLINEPTR->name,expdata))
                  {
                  EditVerbose("Break! %s\n",expdata);
                  goto abort;
                  }
               break;

      case BeginGroupIfNoMatch:
	       if (CURRENTLINEPTR == NULL || CURRENTLINEPTR->name == NULL )
		  {
		  EditVerbose("(Begin Group - no match for %s - file empty)\n",expdata);
		  break;
		  }
	       
               if (LineMatches(CURRENTLINEPTR->name,expdata))
                  {
                  EditVerbose("(Begin Group - skipping %s)\n",expdata);
		  ep = SkipToEndGroup(ep,filename);
                  }
               else
                  {
                  EditVerbose("(Begin Group - no match for %s)\n",expdata);
                  }
               break;

     case BeginGroupIfNoLineMatching:
               if (LocateItemMatchingRegExp(filestart,expdata) != 0)
                  {
                  EditVerbose("(Begin Group - skipping %s)\n",expdata);
		  ep = SkipToEndGroup(ep,filename);
                  }
               else
                  {
                  EditVerbose("(Begin Group - no line matching %s)\n",expdata);
                  }
               break;

      case BeginGroupIfNoLineContaining:
               if (LocateNextItemContaining(filestart,expdata) != 0)
                  {
                  EditVerbose("(Begin Group - skipping, string matched)\n");
                  ep = SkipToEndGroup(ep,filename);
                  }
               else
                  {
                  EditVerbose("(Begin Group - no line containing %s)\n",expdata);
                  }
               break;

      case BeginGroupIfNoSuchLine:
               if (IsItemIn(filestart,expdata))
                  {
                  EditVerbose("(Begin Group - skipping, line exists)\n");
		  ep = SkipToEndGroup(ep,filename);
                  }
               else
                  {
                  EditVerbose("(Begin Group - no line %s)\n",expdata);
                  }
               break;


     case BeginGroupIfFileIsNewer:
               if ((!AUTOCREATED) && (!FileIsNewer(filename,expdata)))
                  {
                  EditVerbose("(Begin Group - skipping, file is older)\n");
                  while(ep->code != EndGroup)
                     {
                     ep=ep->next;
                     }
                  }
               else
                  {
                  EditVerbose("(Begin Group - new file %s)\n",expdata);
                  }
               break;


      case BeginGroupIfDefined:
               if (!IsExcluded(expdata))
                 {
                 EditVerbose("(Begin Group - class %s defined)\n", expdata);
                 }
               else
                 {
                 EditVerbose("(Begin Group - class %s not defined - skipping)\n", expdata);
                 ep = SkipToEndGroup(ep,filename);
                 }
               break;

      case BeginGroupIfNotDefined:
               if (IsExcluded(expdata))
                 {
                 EditVerbose("(Begin Group - class %s not defined)\n", expdata);
                 }
               else
                 {
                 EditVerbose("(Begin Group - class %s defined - skipping)\n", expdata);
                 ep = SkipToEndGroup(ep,filename);
                 }
               break;
	       
     case BeginGroupIfFileExists:
               if (stat(expdata,&tmpstat) == -1)
                  {
                  EditVerbose("(Begin Group - file unreadable/no such file - skipping)\n");
		  ep = SkipToEndGroup(ep,filename);
                  }
               else
                  {
                  EditVerbose("(Begin Group - found file %s)\n",expdata);
                  }
               break;
      case EndGroup:
               EditVerbose("(End Group)\n");
               break;

      case ReplaceAll:
               strncpy(searchstr,expdata,bufsize);
               break;

      case With:
               if (!GlobalReplace(&filestart,searchstr,expdata))
		  {
		  snprintf(OUTPUT,bufsize*2,"Error editing file %s",filename);
		  CfLog(cferror,OUTPUT,"");
		  }
               break;

      case FixEndOfLine:
               DoFixEndOfLine(filestart,expdata);
	       break;

      case AbortAtLineMatching:
               EDABORTMODE = true;
	       strcpy(VEDITABORT,expdata);
	       break;

      case UnsetAbort:
               EDABORTMODE = false;
	       break;

      case AutoMountDirectResources:
	       HandleAutomountResources(&filestart,expdata);
	       break;

      case ForEachLineIn:
	       if (loopstart == NULL)
		  {
	          loopstart = ep;

		  if ((loop_fp = fopen(expdata,"r")) == NULL)
		     {
		     EditVerbose("Couldn't open %s\n",expdata);
		     while(ep->code != EndLoop) /* skip over loop */
		        {
		        ep = ep->next;
		        }
		     break;
		     }
		  
		  EditVerbose("Starting ForEach loop with %s\n",expdata);
		  continue;
		  }
	       else
		  {
	          if (!feof(loop_fp))
		     {
		     bzero(EDITBUFF,bufsize);

		     while (ReadLine(EDITBUFF,bufsize,loop_fp)) /* Like SetLine */
			{
			if (strlen(EDITBUFF) == 0)
			   {
			   EditVerbose("ForEachLineIn skipping blank line");
			   continue;
			   }
			break;
			}
		     if (strlen(EDITBUFF) == 0)
			{
			EditVerbose("EndForEachLineIn\n");
			fclose(loop_fp);
			loopstart = NULL;
			while(ep->code != EndLoop)
			   {
			   ep = ep->next;
			   }
			EditVerbose("EndForEachLineIn, set current line to: %s\n",EDITBUFF);
			}

		     Debug("ForeachLine: %s\n",EDITBUFF);
		     }
		  else
		     {
		     EditVerbose("EndForEachLineIn");
		     
		     fclose(loop_fp);
		     loopstart = NULL;
		     
		     while(ep->code != EndLoop)
		        {
		        ep = ep->next;
		        }
		     }
		  }
	       
	       break;

      case EndLoop:
	       loopend = ep;
   	       ep = loopstart;
	       continue;

      case ReplaceLinesMatchingField:
	       ReplaceWithFieldMatch(&filestart,expdata,EDITBUFF,spliton,filename);
	       break;

      case AppendToLineIfNotContains:
	       AppendToLine(CURRENTLINEPTR,expdata,filename);
	       break;

      default: snprintf(OUTPUT,bufsize*2,"Unknown action in editing of file %s\n",filename);
	       CfLog(cferror,OUTPUT,"");
               break;
      }

   ep = ep->next;
   }

abort :  

EditVerbose("End editing %s\n",filename);
EditVerbose(".....................................................................\n");

EDITVERBOSE = false;
EDABORTMODE = false;

if (DONTDO || CompareToFile(filestart,filename))
   {
   EditVerbose("Unchanged file: %s\n",filename);
   NUMBEROFEDITS = 0;
   }
 
if ((! DONTDO) && (NUMBEROFEDITS > 0))
   {
   SaveItemList(filestart,filename,ptr->repository);
   AddEditfileClasses(ptr,true);
   }
else
   {
   AddEditfileClasses(ptr,false);
   }

ResetOutputRoute('d','d');
ReleaseCurrentLock();

DeleteItemList(filestart);

if (!potentially_outstanding)
   {
   ptr->done = 'y';
   }
}

/********************************************************************/

int IncrementEditPointer(str,liststart)

char *str;
struct Item *liststart;

{ int i,n = 0;
  struct Item *ip;

sscanf(str,"%d", &n);

if (n == 0)
   {
   printf("Illegal increment value: %s\n",str);
   return false;
   }

Debug("IncrementEditPointer(%d)\n",n);

if (CURRENTLINEPTR == NULL)  /* is prev undefined, set to line 1 */
   {
   if (liststart == NULL)
      {
      EditVerbose("cannot increment line pointer in empty file\n");
      return true;
      }
   else
      {
      CURRENTLINEPTR=liststart;
      CURRENTLINENUMBER=1;
      }
   }


if (n < 0)
   {
   if (CURRENTLINENUMBER + n < 1)
      {
      EditVerbose("pointer decrements to before start of file!\n");
      EditVerbose("pointer stopped at start of file!\n");
      CURRENTLINEPTR=liststart;
      CURRENTLINENUMBER=1;      
      return true;
      }

   i = 1;

   for (ip = liststart; ip != CURRENTLINEPTR; ip=ip->next, i++)
      {
      if (i == CURRENTLINENUMBER + n)
         {
         CURRENTLINENUMBER += n;
         CURRENTLINEPTR = ip;
	 Debug2("Current line (%d) starts: %20.20s ...\n",CURRENTLINENUMBER,CURRENTLINEPTR->name);

         return true;
         }
      }
   }

for (i = 0; i < n; i++)
   {
   if (CURRENTLINEPTR->next != NULL)
      {
      CURRENTLINEPTR = CURRENTLINEPTR->next;
      CURRENTLINENUMBER++;

      EditVerbose("incrementing line pointer to line %d\n",CURRENTLINENUMBER);
      }
   else
      {
      EditVerbose("inc pointer failed, still at %d\n",CURRENTLINENUMBER);
      }
   }

Debug2("Current line starts: %20s ...\n",CURRENTLINEPTR->name);

return true;
}

/********************************************************************/

int ResetEditSearch (str,list)

char *str;
struct Item *list;

{ int i = 1 ,n = -1;
  struct Item *ip;

sscanf(str,"%d", &n);

if (n < 1)
   {
   printf("Illegal reset value: %s\n",str);
   return false;
   }

for (ip = list; (i < n) && (ip != NULL); ip=ip->next, i++)
   {
   }

if (i < n || ip == NULL)
   {
   printf("Search for (%s) begins after end of file!!\n",str);
   return false;
   }

EditVerbose("resetting pointers to line %d\n",n);

CURRENTLINENUMBER = n;
CURRENTLINEPTR = ip;

return true;
}

/********************************************************************/

int ReplaceEditLineWith (string)

char *string;

{ char *sp;

if (strcmp(string,CURRENTLINEPTR->name) == 0)
   {
   EditVerbose("ReplaceLineWith - line does not need correction.\n");
   return true;
   }

if ((sp = malloc(strlen(string)+1)) == NULL)
   {
   printf("Memory allocation failed in ReplaceEditLineWith, aborting edit.\n");
   return false;
   }

EditVerbose("Replacing line %d with %10s...\n",CURRENTLINENUMBER,string);
strcpy(sp,string);
free (CURRENTLINEPTR->name);
CURRENTLINEPTR->name = sp;
NUMBEROFEDITS++;
return true;
}

/********************************************************************/

int RunEditScript (script,fname,filestart,ptr)

char *script, *fname;
struct Item **filestart;
struct Edit *ptr;

{ FILE *pp;
  char buffer[bufsize];

if (script == NULL)
   {
   printf("No script defined for with SetScript\n");
   return false;
   }

if (DONTDO)
   {
   return true;
   }

if (NUMBEROFEDITS > 0)
   {
   SaveItemList(*filestart,fname,ptr->repository);
   AddEditfileClasses(ptr,true);
   }
else
   {
   AddEditfileClasses(ptr,false);
   }

DeleteItemList(*filestart);

snprintf(buffer,bufsize,"%s %s %s  2>&1",script,fname,CLASSTEXT[VSYSTEMHARDCLASS]);

EditVerbose("Running command: %s\n",buffer);

      
switch (ptr->useshell)
   {
   case 'y':  pp = cfpopen_sh(buffer,"r");
              break;
   default:   pp = cfpopen(buffer,"r");
              break;	     
   }
  
if (pp == NULL)
   {
   printf("Edit script %s failed to open.\n",fname);
   return false;
   }

while (!feof(pp))   
   {
   ReadLine(CURRENTITEM,bufsize,pp);

   if (!feof(pp))
      {
      EditVerbose("%s\n",CURRENTITEM);
      }
   }

cfpclose(pp);

*filestart = 0;

if (! LoadItemList(filestart,fname))
   {
   EditVerbose("File was marked for editing\n");
   return false;
   }

NUMBEROFEDITS = 0;
CURRENTLINENUMBER = 1;
CURRENTLINEPTR = *filestart;
return true;
}

/************************************************************/

void DoFixEndOfLine(list,type)  /* fix end of line char format */

  /* Assumes that extra_space macro allows enough space */
  /* in the allocated strings to add a few characters */

struct Item *list;
char *type;

{ struct Item *ip;
  char *sp;
  int gotCR;

EditVerbose("Checking end of line conventions: type = %s\n",type);

if (strcmp("unix",type) == 0 || strcmp("UNIX",type) == 0)
   {
   for (ip = list; ip != NULL; ip=ip->next)
      {
      for (sp = ip->name; *sp != '\0'; sp++)
	 {
	 if (*sp == (char)13)
	    {
	    *sp = '\0';
	    NUMBEROFEDITS++;
	    }
	 }
      }
   return;
   }

if (strcmp("dos",type) == 0 || strcmp("DOS",type) == 0)
   {
   for (ip = list; ip != NULL; ip = ip->next)
      {
      gotCR = false;
      
      for (sp = ip->name; *sp !='\0'; sp++)
	 {
	 if (*sp == (char)13)
	    {
	    gotCR = true;
	    }
	 }

      if (!gotCR)
	 {
	 *sp = (char)13;
	 *(sp+1) = '\0';
	 NUMBEROFEDITS++;
	 }
      }
   return;
   }

printf("Unknown file format: %s\n",type);
}

/**************************************************************/

void HandleAutomountResources(filestart,opts)

struct Item **filestart;
char *opts;

{ struct Mountables *mp;
  char buffer[bufsize];
  char *sp;

for (mp = VMOUNTABLES; mp != NULL; mp=mp->next)
   {
   for (sp = mp->filesystem; *sp != ':'; sp++)
      {
      }

   sp++;
   snprintf(buffer,bufsize,"%s\t%s\t%s",sp,opts,mp->filesystem);

   if (LocateNextItemContaining(*filestart,sp) == NULL)
      {
      AppendItem(filestart,buffer,"");
      NUMBEROFEDITS++;
      }
   else
      {
      EditVerbose("have a server for %s\n",sp);
      }
   }
}

/**************************************************************/

void CheckEditSwitches(filename,ptr)

char *filename;
struct Edit *ptr;

{ struct stat statbuf;
  struct Edlist *ep;
  char inform = 'd', log = 'd';
  char expdata[bufsize];
  int fd;
struct Edlist *actions = ptr->actions;
  
PARSING = true;
 
for (ep = actions; ep != NULL; ep=ep->next)
   {
   if (IsExcluded(ep->classes))
      {
      continue;
      }

   ExpandVarstring(ep->data,expdata,NULL);

   switch(ep->code)
      {
      case AutoCreate: if (!DONTDO)
	                  { mode_t mask;

			  if (stat(filename,&statbuf) == -1)
			     {
			     Debug("Setting umask to %o\n",ptr->umask);
			     mask=umask(ptr->umask);
			     
			     if ((fd = creat(filename,0644)) == -1)
				{
				snprintf(OUTPUT,bufsize*2,"Unable to create file %s\n",filename);
				CfLog(cfinform,OUTPUT,"creat");
				}
			     else
				{
				AUTOCREATED = true;
				close(fd);
				}
			     snprintf(OUTPUT,bufsize*2,"Creating file %s, mode %o\n",filename,(0644 & ~ptr->umask));
			     CfLog(cfinform,OUTPUT,"");
			     umask(mask);
			     return;
			     }			 
			  }
                       
                       break;
      
      case EditBackup: if (strcmp("false",ToLowerStr(expdata)) == 0 || strcmp("off",ToLowerStr(expdata)) == 0)
	                  {
			  IMAGEBACKUP = 'n';
	                  }

  	               if (strcmp("single",ToLowerStr(expdata)) == 0 || strcmp("one",ToLowerStr(expdata)) == 0)
	                  {
			  IMAGEBACKUP = 'n';
	                  }

		       if (strcmp("timestamp",ToLowerStr(expdata)) == 0 || strcmp("stamp",ToLowerStr(expdata)) == 0)
	                  {
			  IMAGEBACKUP = 's';
	                  }

                       break;
		       
      case EditLog:    if (strcmp(ToLowerStr(expdata),"true") == 0 || strcmp(ToLowerStr(expdata),"on") == 0)
	                  {
			  log = 'y';
			  break;
			  }

                       if (strcmp(ToLowerStr(expdata),"false") == 0 || strcmp(ToLowerStr(expdata),"off") == 0)
                          {
			  log = 'n';
			  break;
			  }

      case EditInform: if (strcmp(ToLowerStr(expdata),"true") == 0 || strcmp(ToLowerStr(expdata),"on") == 0)
	                  {
			  inform = 'y';
			  break;
			  }

                       if (strcmp(ToLowerStr(expdata),"false") == 0 || strcmp(ToLowerStr(expdata),"off") == 0)
                          {
			  inform = 'n';
			  break;
			  }

	  
      }
   }

PARSING = false; 
ResetOutputRoute(log,inform);
}


/**************************************************************/
/* Level 3                                                    */
/**************************************************************/

void AddEditfileClasses (list,editsdone)

struct Edit *list;
int editsdone;

{ char *sp, currentitem[maxvarsize];
  struct Edlist *ep;

if (editsdone)
   {
   for (ep = list->actions; ep != NULL; ep=ep->next)
      {
      if (IsExcluded(ep->classes))
	 {
	 continue;
	 }

      if (ep->code == DefineClasses)
	 {
	 Debug("AddEditfileClasses(%s)\n",ep->data);
	 
	 for (sp = ep->data; *sp != '\0'; sp++)
	    {
	    currentitem[0] = '\0';
	    
	    sscanf(sp,"%[^,:.]",currentitem);
	    
	    sp += strlen(currentitem);
	    
	    AddClassToHeap(currentitem);
	    }
	 }
      }
   }
else
   {
   for (ep = list->actions; ep != NULL; ep=ep->next)
      {
      if (IsExcluded(ep->classes))
	 {
	 continue;
	 }
      
      if (ep->code == ElseDefineClasses)
	 {
	 Debug("Entering AddEditfileClasses(%s)\n",ep->data);
	 
	 for (sp = ep->data; *sp != '\0'; sp++)
	    {
	    currentitem[0] = '\0';
	    
	    sscanf(sp,"%[^,:.]",currentitem);
	    
	    sp += strlen(currentitem);
	    
	    AddClassToHeap(currentitem);
	    }
	 }
      }
   }

if (ep == NULL)
   {
   return;
   }

}

/**************************************************************/

struct Edlist *ThrowAbort(from)

struct Edlist *from;

{ struct Edlist *ep, *last = NULL;
 
for (ep = from; ep != NULL; ep=ep->next)
   {
   if (ep->code == CatchAbort)
      {
      return ep;
      }
   
   last = ep;
   }

return last; 
}

/**************************************************************/

struct Edlist *SkipToEndGroup(ep,filename)

struct Edlist *ep;
char *filename;

{ int level = -1;
 
while(ep != NULL)
   {
   switch (ep->code)
      {
      case BeginGroupIfNoMatch:
      case BeginGroupIfNoLineMatching:
      case BeginGroupIfNoSuchLine:
      case BeginGroupIfFileIsNewer:
      case BeginGroupIfFileExists:
      case BeginGroupIfNoLineContaining:
      case BeginGroupIfDefined:
      case BeginGroupIfNotDefined:
	  level ++;
      }

   Debug("   skip: %s (%d)\n",VEDITNAMES[ep->code],level);

   if (ep->code == EndGroup)
       {
       if (level == 0)
	  {
	  return ep;
	  }
       level--;
       }
   
   if (ep->next == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Missing EndGroup in %s",filename);
      CfLog(cferror,OUTPUT,"");
      break;
      }

   ep=ep->next;
   }

return ep;
}

/**************************************************************/

int BinaryEditFile(ptr,filename)

struct Edit *ptr;
char *filename;

{ char expdata[bufsize],search[bufsize];
  struct Edlist *ep;
  struct stat statbuf;
  void *memseg;

EditVerbose("Begin (binary) editing %s\n",filename);

NUMBEROFEDITS = 0;
search[0] = '\0'; 
 
if (stat(filename,&statbuf) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't stat %s\n",filename);
   CfLog(cfverbose,OUTPUT,"stat");
   return false;
   }

if ((EDITBINFILESIZE != 0) &&(statbuf.st_size > EDITBINFILESIZE))
   {
   snprintf(OUTPUT,bufsize*2,"File %s is bigger than the limit <editbinaryfilesize>\n",filename);
   CfLog(cfinform,OUTPUT,"");
   return(false);
   }

if (! S_ISREG(statbuf.st_mode))
   {
   snprintf(OUTPUT,bufsize*2,"%s is not a plain file\n",filename);
   CfLog(cfinform,OUTPUT,"");
   return false;
   }

if ((memseg = malloc(statbuf.st_size+buffer_margin)) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Unable to load file %s into memory",filename);
   CfLog(cferror,OUTPUT,"malloc");
   return false;
   }
 
LoadBinaryFile(filename,statbuf.st_size,memseg);

ep = ptr->actions;

while (ep != NULL)
   {
   if (IsExcluded(ep->classes))
      {
      ep = ep->next;
      continue;
      }

   ExpandVarstring(ep->data,expdata,NULL);
   
   Debug2("Edit action: %s\n",VEDITNAMES[ep->code]);

   switch(ep->code)
      {
      case WarnIfContainsString:
	  WarnIfContainsRegex(memseg,statbuf.st_size,expdata,filename);
	  break;
	  
      case WarnIfContainsFile:
	  WarnIfContainsFilePattern(memseg,statbuf.st_size,expdata,filename);
	  break;

      case EditMode:
	  break;

      case ReplaceAll:
	  Debug("Replace %s\n",expdata);
	  strcpy(search,expdata);
	  break;
	  
      case With:
	  if (strcmp(expdata,search) == 0)
	     {
	     Verbose("Search and replace patterns are identical in binary edit %s\n",filename);
	     continue;
	     }
	  
	  if (BinaryReplaceRegex(memseg,statbuf.st_size,search,expdata,filename))
	     {
	     NUMBEROFEDITS++;
	     }
	  break;

      default:
	  snprintf(OUTPUT,bufsize*2,"Cannot use %s in a binary edit (%s)",VEDITNAMES[ep->code],filename);
	  CfLog(cferror,OUTPUT,"");
      }

   ep=ep->next;
   }

if ((! DONTDO) && (NUMBEROFEDITS > 0))
   {
   SaveBinaryFile(filename,statbuf.st_size,memseg,ptr->repository);
   AddEditfileClasses(ptr,true);
   }
else
   {
   AddEditfileClasses(ptr,false);
   }

free(memseg);
EditVerbose("End editing %s\n",filename);
EditVerbose(".....................................................................\n"); 
return true;
}

/**************************************************************/
/* Level 4                                                    */
/**************************************************************/

int LoadBinaryFile(source,size,memseg)

char *source;
off_t size;
void *memseg;

{ int sd,n_read;
  char buf[bufsize];
  off_t n_read_total = 0;
  char *ptr;

Debug("LoadBinaryFile(%s,%d)\n",source,size);
  
if ((sd = open(source,O_RDONLY)) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Can't copy %s!\n",source);
   CfLog(cfinform,OUTPUT,"open");
   return false;
   }

ptr = memseg;
 
while (true)
   {
   if ((n_read = read (sd,buf,bufsize)) == -1)
      {
      if (errno == EINTR) 
         {
         continue;
         }

      close(sd);
      return false;
      }

   bcopy(buf,ptr,n_read);
   ptr += n_read;

   if (n_read < bufsize)
      {
      break;
      }

   n_read_total += n_read;
   }

close(sd);
return true;
}

/**************************************************************/

int SaveBinaryFile(file,size,memseg,repository)

char *file, *repository;
off_t size;
void *memseg;

 /* If we do this, we screw up checksums anyway, so no need to
    preserve unix holes here ...*/

{ int dd;
  char new[bufsize],backup[bufsize];

Debug("SaveBinaryFile(%s,%d)\n",file,size);
Verbose("Saving %s\n",file);
 
strcpy(new,file);
strcat(new,CF_NEW);
strcpy(backup,file);
strcat(backup,CF_EDITED);
 
unlink(new);  /* To avoid link attacks */
 
if ((dd = open(new,O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0600)) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Copy %s security - failed attempt to exploit a race? (Not copied)\n",file);
   CfLog(cfinform,OUTPUT,"open");
   return false;
   }
 
cf_full_write (dd,(char *)memseg,size);

close(dd);

if (! IsItemIn(VREPOSLIST,new))
   {
   if (rename(file,backup) == -1)
      {
      snprintf(OUTPUT,bufsize*2,"Error while renaming backup %s\n",file);
      CfLog(cferror,OUTPUT,"rename ");
      unlink(new);
      return false;
      }
   else if(Repository(backup,repository))
      {
      if (rename(new,file) == -1)
	 {
	 snprintf(OUTPUT,bufsize*2,"Error while renaming %s\n",file);
	 CfLog(cferror,OUTPUT,"rename");
	 return false;
	 }       
      unlink(backup);
      return true;
      }
   }

if (rename(new,file) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Error while renaming %s\n",file);
   CfLog(cferror,OUTPUT,"rename");
   return false;
   }       

return true;
}

/**************************************************************/

void WarnIfContainsRegex(memseg,size,data,filename)

void *memseg;
off_t size;
char *data,*filename;

{ off_t sp;
  regex_t rx,rxcache;
  regmatch_t pmatch;

Debug("WarnIfContainsRegex(%s)\n",data); 

for (sp = 0; sp < (off_t)(size-strlen(data)); sp++)
   {
   if (bcmp((char *) memseg+sp,data,strlen(data)) == 0)
      {
      snprintf(OUTPUT,bufsize*2,"WARNING! File %s contains literal string %.255s",filename,data);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   }

if (CfRegcomp(&rxcache,data,REG_EXTENDED) != 0)
   {
   return;
   }

for (sp = 0; sp < (off_t)(size-strlen(data)); sp++)
   {
   bcopy(&rxcache,&rx,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,(char *)memseg+sp,1,&pmatch,0) == 0)
      {
      snprintf(OUTPUT,bufsize*2,"WARNING! File %s contains regular expression %.255s",filename,data);
      CfLog(cferror,OUTPUT,"");
      regfree(&rx);
      return;
      }
   }
}

/**************************************************************/

void WarnIfContainsFilePattern(memseg,size,data,filename)

void *memseg;
off_t size;
char *data,*filename;

{ off_t sp;
  struct stat statbuf; 
  char *pattern;
  
Debug("WarnIfContainsFile(%s)\n",data); 

if (stat(data,&statbuf) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"File %s cannot be opened",data);
   CfLog(cferror,OUTPUT,"stat");
   return;
   }

if (! S_ISREG(statbuf.st_mode))
   {
   snprintf(OUTPUT,bufsize*2,"File %s cannot be used as a binary pattern",data);
   CfLog(cferror,OUTPUT,"");
   return;
   }

if (statbuf.st_size > size)
   {
   snprintf(OUTPUT,bufsize*2,"File %s is larger than the search file, ignoring",data);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if ((pattern = malloc(statbuf.st_size+buffer_margin)) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"File %s cannot be loaded",data);
   CfLog(cferror,OUTPUT,"");
   return;
   }

if (!LoadBinaryFile(data,statbuf.st_size,pattern))
   {
   snprintf(OUTPUT,bufsize*2,"File %s cannot be opened",data);
   CfLog(cferror,OUTPUT,"stat");
   return;
   }
 
for (sp = 0; sp < (off_t)(size-statbuf.st_size); sp++)
   {
   if (bcmp((char *) memseg+sp,pattern,statbuf.st_size-1) == 0)
      {
      snprintf(OUTPUT,bufsize*2,"WARNING! File %s contains the contents of reference file %s",filename,data);
      CfLog(cferror,OUTPUT,"");
      free(pattern);
      return;
      }
   }
 
free(pattern);
}


/**************************************************************/

int BinaryReplaceRegex(memseg,size,search,replace,filename)

void *memseg;
off_t size;
char *search,*filename,*replace;

{ off_t sp,spr;
  regex_t rx,rxcache;
  regmatch_t pmatch;
  int match = false;
  
Debug("BinaryReplaceRegex(%s,%s,%s)\n",search,replace,filename); 

if (CfRegcomp(&rxcache,search,REG_EXTENDED) != 0)
   {
   return false;
   }

for (sp = 0; sp < (off_t)(size-strlen(replace)); sp++)
   {
   bcopy(&rxcache,&rx,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,(char *)(sp+(char *)memseg),1,&pmatch,0) == 0)
      {
      if (pmatch.rm_eo-pmatch.rm_so < strlen(replace))
	 {
	 snprintf(OUTPUT,bufsize*2,"Cannot perform binary replacement: string doesn't fit in %s",filename);
	 CfLog(cfverbose,OUTPUT,"");
	 }
      else
	 {
	 Verbose("Replacing [%s] with [%s] at %d\n",search,replace,sp);
	 match = true;
	 
	 strncpy((char *)memseg+sp+(off_t)pmatch.rm_so,replace,strlen(replace));

	 Verbose("Padding character is %c\n",PADCHAR);
	 
	 for (spr = (pmatch.rm_so+strlen(replace)); spr < pmatch.rm_eo; spr++)
	    {
	    *((char *)memseg+spr+sp) = PADCHAR; /* default space */
	    }

	 sp += pmatch.rm_eo - pmatch.rm_so - 1;
	 }
      }
   else
      {
      sp += strlen(replace);
      }
   }
 
regfree(&rx);
return match; 
}
