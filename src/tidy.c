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
/*                                                                   */
/* Tidy object                                                       */
/*                                                                   */
/*********************************************************************/

int RecursiveHomeTidy(name,level,sb)

char *name;
int level;
struct stat *sb;

{ struct stat statbuf;
  DIR *dirh;
  struct dirent *dirp;
  char pcwd[bufsize];
  time_t ticks;
  int done = false,goback;

Debug("\n RecursiveHomeTidy(%s,%d)\n\n",name,level);

if (!DirPush(name,sb))
   {
   return false;
   }
 
if (strlen(name) == 0)
   {
   name = "/";
   }

if (level > recursion_limit)
   {
   snprintf(OUTPUT,bufsize*2,"WARNING: Very deep nesting of directories (> %d deep): %s (Aborting tidy)",level,name);
   CfLog(cferror,OUTPUT,"");
   return true;
   }

Debug2("HomeTidy: Opening %s as .\n",name);

if ((dirh = opendir(".")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",name);
   CfLog(cfverbose,OUTPUT,"opendir");
   return true;
   }

if (level == 2)
   {
   if (LOGTIDYHOMEFILES)
      {
      int tempfd;
      strcpy(VLOGFILE,name);
      strcat(VLOGFILE,"/.cfengine.rm");
      
      /* Unlink here to avoid an exploit which could be used to overwrite a system
	 file with root privileges. */
      
      if (unlink(VLOGFILE) == -1)
	 {
	 Debug("Pre-existing object %s could not be removed\n",VLOGFILE);
	 }

      if ((tempfd = open(VLOGFILE, O_CREAT|O_EXCL|O_WRONLY,0600)) < 0)
 	 {
 	 snprintf(OUTPUT,bufsize,"Couldn't open a file %s\n",VLOGFILE);	 
	 CfLog(cferror,OUTPUT,"open");
	 VLOGFP = stderr;
	 }
      else if ((VLOGFP = fdopen(tempfd,"w")) == NULL)   
	 {
	 sprintf(OUTPUT,"Couldn't open a file %s\n",VLOGFILE);
	 CfLog(cferror,OUTPUT,"fdopen");
	 VLOGFP = stderr;
	 }
      else
	 {
	 ticks = time((time_t *)NULL);
	 fprintf(VLOGFP,"This file is generated by cfengine %s\n",VERSION);
	 fprintf(VLOGFP,"It contains a log of the files which have been tidied.\n");
	 fprintf(VLOGFP,"The time of writing is %s\n",ctime(&ticks));
	 fprintf(VLOGFP,"If you have any questions about this, send them to %s.\n",VSYSADM);
	 fprintf(VLOGFP,"-(Start transcript)---------------\n");
	 }
      }
   }
 
for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,name,NULL))
      {
      continue;
      }

   if (IgnoreFile(name,dirp->d_name,NULL))
      {
      continue;
      }

   strcpy(pcwd,name);                                 /* Assemble pathname */
   AddSlash(pcwd);

   if (BufferOverflow(pcwd,dirp->d_name))
      {
      return true;
      }

   strcat(pcwd,dirp->d_name);

   if (TRAVLINKS)
      {
      Verbose("Warning: you are using travlinks=true. It is a potential security hazard if there are untrusted users\n");
      if (lstat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",pcwd);
	 CfLog(cferror,OUTPUT,"stat");
         continue;
         }

      if (S_ISLNK(statbuf.st_mode) && (statbuf.st_mode != getuid()))	  
	 {
	 snprintf(OUTPUT,bufsize,"File %s is an untrusted link. cfagent will not follow it with a destructive operation (tidy)",pcwd);
	 CfLog(cfinform,OUTPUT,"");
	 continue;
	 }
      
      if (stat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",pcwd);
	 CfLog(cferror,OUTPUT,"stat");
         continue;
         }
      }
   else
      {
      if (lstat(pcwd,&statbuf) == -1)
         {
         if (DEBUG || D2 || VERBOSE)
            {
            snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",pcwd);
	    CfLog(cferror,OUTPUT,"lstat");
	    bzero(VBUFF,bufsize);
            if (readlink(pcwd,VBUFF,bufsize-1) != -1)
               {
               snprintf(OUTPUT,bufsize*2,"File is link to -> %s\n",VBUFF);
	       CfLog(cferror,OUTPUT,"");
               }
            }
         continue;
         }
      }


   if (S_ISDIR(statbuf.st_mode))
      {
      if (IsMountedFileSystem(&statbuf,pcwd,1))
         {
         continue;
         }
      else
         {
	 if (!done)
	    {
	    /* Note, here we pass on the full path name, not relative name to retain
	    state, but we have statted the right file above with opendir("."), so 
	    the race test is still secure for the next recursion level */

	    goback = RecursiveHomeTidy(pcwd,level+1,&statbuf);
	    DirPop(goback,name,sb);	    
	    }
         }
      }
   else
      {
      if (!TidyHomeFile(pcwd,dirp->d_name,&statbuf,level))
	 {
	 done = true;
	 }
      }
   }

if (level == 2)
   {
   if (LOGTIDYHOMEFILES)
      {
      fclose(VLOGFP);
      chmod(VLOGFILE,DEFAULTMODE);
      }
   }

closedir(dirh);
return true;
}


/*********************************************************************/

int TidyHomeFile(path,name,statbuf,level)

char *path;
char *name;
struct stat *statbuf;
int level;

  /* Tidy a file if it's past its sell-by date in kB, and if
     it is greater than the specified size. Don't need an OR,
     since size age can just be set to zero.                 */

{ struct Tidy *tp;
  struct TidyPattern *tlp;
  short savetravlinks = TRAVLINKS, savekilloldlinks = KILLOLDLINKS;

/* Note that we have to go through the whole tidy list here, even non-home,
   so be careful to pick out the rules the affect us! The info
   about home dissappears here, since we have expanded the home/
   directive into an actual path, so we make sure that no non-home
   rules can be applied to home directories */
  
for (tp = VTIDY; tp != NULL; tp=tp->next)
   {
   if ((strncmp(tp->path,"home/",5) != 0) && (strcmp(tp->path,"home") != 0))
      {
      continue;
      }

   if (tp->tidylist == NULL || tp->done == 'y')
      {
      continue;
      }

   Debug("  Check rule %s ...\n",tp->path);
   
   if ((tp->maxrecurse != INFINITERECURSE) && (level > tp->maxrecurse+1))
      {
      Debug("Recursion maxed out at level %d/%d\n",level,tp->maxrecurse+1);
      /*return false;*/
      continue;
      }

   for (tlp = tp->tidylist; tlp != NULL; tlp=tlp->next)
      {
      if (IsExcluded(tlp->classes))
	 {
	 continue;
	 }

      savetravlinks = TRAVLINKS;
      savekilloldlinks = KILLOLDLINKS;

      ResetOutputRoute(tlp->log,tlp->inform);
      
      if (tlp->travlinks == 'T')
         {
         TRAVLINKS = true;
         }
      else if (tlp->travlinks == 'F')
         {
         TRAVLINKS = false;
	 }
      else if (tlp->travlinks == 'K')
         {
         KILLOLDLINKS = true;
         }

      
      if (!FileObjectFilter(path,statbuf,tlp->filters,tidy))
	 {
         continue;
	 }
      
      if (IgnoredOrExcluded(tidy,path,NULL,tp->exclusions))
	 {
         Debug("Skipping ignored/excluded file %s\n",path);
	 continue;
	 }
      
      if (WildMatch(tlp->pattern,name) && CheckHomeSubDir(path,tp->path,tp->maxrecurse))
         {
	 if ((tlp->recurse != INFINITERECURSE) && (level > tlp->recurse+1))
	    {
            Debug("Not tidying %s - level %d > limit %d\n",path,level,tlp->recurse+1);
	    continue;
	    }
	 
         DoTidyFile(path,name,tlp,statbuf,CF_USELOGFILE,false);
	 }
      
      ResetOutputRoute('d','d');
      }
   }

TRAVLINKS = savetravlinks;
KILLOLDLINKS = savekilloldlinks;
return true; 
}

/*********************************************************************/

int RecursiveTidySpecialArea(name,tp,maxrecurse,sb)

char *name;
struct Tidy *tp;
int maxrecurse;
struct stat *sb;

{ struct stat statbuf,topstatbuf;
  DIR *dirh;
  struct dirent *dirp;
  char pcwd[bufsize];
  int is_dir,level,goback;
  
Debug("RecursiveTidySpecialArea(%s)\n",name);
bzero(&statbuf,sizeof(statbuf));

if (!DirPush(name,sb))
   {
   return false;
   }
 
if (maxrecurse == -1)
   {
   Debug2("MAXRECURSE ran out, quitting at %s\n",name);
   return true;
   }

if (IgnoredOrExcluded(tidy,name,NULL,tp->exclusions))
  {
  Debug("Skipping ignored/excluded file %s\n",name);
  return true;
  }

if (IgnoreFile(name,"",NULL))
   {
   Debug2("cfengine: Ignoring directory %s\n",name);
   return true;
   }

if (strlen(name) == 0)     /* Check for root dir */
   {
   name = (char *) malloc(2);
   name[0] = '/';
   name[1] = '\0';
   }

if (maxrecurse == tp->maxrecurse)
   {
   if (lstat(name,&topstatbuf) == -1)
      {
      if (DEBUG || D2 || VERBOSE)
	 {
	 snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",name);
	 CfLog(cferror,OUTPUT,"");
	 bzero(VBUFF,bufsize);
	 if (readlink(name,VBUFF,bufsize-1) != -1)
	    {
	    snprintf(OUTPUT,bufsize*2,"File is link to -> %s\n",VBUFF);
	    CfLog(cferror,OUTPUT,"");
	    }
         }
      return true;
      }
   }

if ((dirh = opendir(".")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Can't open directory [%s]\n",name);
   CfLog(cfverbose,OUTPUT,"opendir");
   return true;
   }

Debug("Tidy: opening dir %s\n",name);

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,name,NULL))
      {
      continue;
      }

   if (IgnoreFile(name,dirp->d_name,NULL))
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

   if (lstat(dirp->d_name,&statbuf) == -1)          /* Check for links first */
      {
      Verbose("Can't stat %s (%s)\n",dirp->d_name,pcwd);
      continue;
      }
   else
      {
      if (S_ISLNK(statbuf.st_mode) && (statbuf.st_uid != getuid()))	  
	 {
	 snprintf(OUTPUT,bufsize,"File %s is an untrusted link. cfagent will not follow it with a destructive operation (tidy)",pcwd);
	 continue;
	 }
      }

   if (TRAVLINKS && (stat(dirp->d_name,&statbuf) == -1))
      {
      Verbose("Can't stat %s (%s)\n",dirp->d_name,pcwd);
      continue;
      }
   
   if (S_ISDIR(statbuf.st_mode))
      {
      is_dir =  true;
      }
   else
      {
      is_dir = false;
      }
   
   level = tp->maxrecurse - maxrecurse;

   if (S_ISDIR(statbuf.st_mode))              /* note lstat above! */
      {
      if (IsMountedFileSystem(&statbuf,pcwd,1))
         {
         continue;
         }
      else
         {
	 /* Note, here we pass on the full path name, not relative name to retain
	    state, but we have statted the right file above with opendir("."), so 
	    the race test is still secure for the next recursion level */
	 
         goback = RecursiveTidySpecialArea(pcwd,tp,maxrecurse-1,&statbuf);
	 DirPop(goback,name,sb);
         }	 

      TidyParticularFile(pcwd,dirp->d_name,tp,&statbuf,is_dir,level);
      }
   else
      {
      TidyParticularFile(pcwd,dirp->d_name,tp,&statbuf,is_dir,level);
      }
   }

closedir(dirh);

if (maxrecurse == tp->maxrecurse)
   {
   Debug("Checking tidy topmost directory %s\n",name);
   chdir("/");

   TidyParticularFile(name,ReadLastNode(name),tp,&topstatbuf,true,tp->maxrecurse);
   }

return true; 
}

/*********************************************************************/

void TidyParticularFile(path,name,tp,statbuf,is_dir,level)

char *path, *name;
struct Tidy *tp;
struct stat *statbuf;
int level,is_dir;

{ struct TidyPattern *tlp;
  short savekilloldlinks = KILLOLDLINKS;

Debug2("TidyParticularFile(%s,%s)\n",path,name);

if (tp->tidylist == NULL)
   {
   return;
   }

for (tlp = tp->tidylist; tlp != NULL; tlp=tlp->next)
   {
   if (IsExcluded(tlp->classes))
      {
      continue;
      }
   
   ResetOutputRoute(tlp->log,tlp->inform);

   if (S_ISLNK(statbuf->st_mode) && is_dir && (tlp->dirlinks == 'k') && (tlp->rmdirs == 'n'))  /* Keep links to directories */
      {
      ResetOutputRoute('d','d');
      continue;
      }

   savekilloldlinks = KILLOLDLINKS;

   if (tlp->travlinks == 'K')
      {
      KILLOLDLINKS = true;
      }

   if (S_ISLNK(statbuf->st_mode))             /* No point in checking permission on a link */
      {
      Debug("Checking for dead links\n");
      if (tlp != NULL)
	 {
	 KillOldLink(path,tlp->defines);
	 }
      else
	 {
	 KillOldLink(path,NULL);
	 }
      continue;
      }

   KILLOLDLINKS = savekilloldlinks;
   
   if (is_dir && tlp->rmdirs == 'n')               /* not allowed to rmdir */
      {
      ResetOutputRoute('d','d');
      continue;
      }

   if ((level == tp->maxrecurse) && tlp->rmdirs == 's') /* rmdir subdirs only */
      {
      ResetOutputRoute('d','d');
      continue;
      }
   
   if (level > tlp->recurse && tlp->recurse != INFINITERECURSE)
      {
      Debug2("[PATTERN %s RECURSE ENDED at %d(%d) BEFORE MAXVAL %d]\n",tlp->pattern,
		level,tlp->recurse,tp->maxrecurse);
      ResetOutputRoute('d','d');
      continue;
      }
   
   if (IsExcluded(tlp->classes))
      {
      ResetOutputRoute('d','d');
      continue;
      }

   if (! WildMatch(tlp->pattern,name))
      {
      Debug("Pattern did not match (first filter %s) %s\n",tlp->pattern,path);
      ResetOutputRoute('d','d');
      continue;
      }

   if (!FileObjectFilter(path,statbuf,tlp->filters,tidy))
      {
      Debug("Skipping filtered file %s\n",path);
      continue;
      }

   if (IgnoredOrExcluded(tidy,path,NULL,tp->exclusions))
      {
      Debug("Skipping ignored/excluded file %s\n",path);
      continue;
      }
      
   if (S_ISLNK(statbuf->st_mode) && is_dir && (tlp->dirlinks == 'y'))
      {
      Debug("Link to directory, dirlinks= says delete these\n");
      }
   else if (is_dir && !EmptyDir(path))
      {
      Debug("Non-empty directory %s, skipping..\n",path);
      ResetOutputRoute('d','d');
      continue;
      }
   
   Debug2("Matched %s to %s in %s\n",name,tlp->pattern,path);
   DoTidyFile(path,name,tlp,statbuf,CF_NOLOGFILE,is_dir);
   ResetOutputRoute('d','d');
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void DoTidyFile(path,name,tlp,statbuf,logging_this,isreallydir)

char *path, *name;
struct TidyPattern *tlp;
struct stat *statbuf;
short logging_this;
int isreallydir;

{ time_t nowticks, fileticks = 0;
  int size_match = false, age_match = false;

Debug2("DoTidyFile(%s,%s)\n",path,name);

/* Here we can assume that we are in the right directory with chdir()! */
 
nowticks = time((time_t *)NULL);             /* cmp time in days */

switch (tlp->searchtype)
   {
   case 'a': fileticks = statbuf->st_atime;
             break;
   case 'm': fileticks = statbuf->st_mtime;
	     break;
   case 'c': fileticks = statbuf->st_ctime;
	     break;
   default:  printf("cfengine: Internal error in DoTidyFile()\n");
             break;
   }

if (isreallydir)
   {
   /* Directory age comparison by mtime, since examining will always alter atime */
   fileticks = statbuf->st_mtime;
   }
 
if (nowticks-fileticks < 0)
   {
   snprintf(OUTPUT,bufsize*2,"ALERT: atime for %s is in the future. Check system clock!\n",path);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if (tlp->size == CF_EMPTYFILE)
   {
   if (statbuf->st_size == 0)
      {
      size_match = true;
      }
   else
      {
      size_match = false;
      }
   }
else
   {
   size_match = (tlp->size <= statbuf->st_size);
   }

age_match = tlp->age*ticksperday <= (nowticks-fileticks);

if (age_match && size_match)
   {
   if (logging_this)
      {
      if (VLOGFP != NULL)
	 {
	 fprintf(VLOGFP,"cf: rm %s\n",path);
	 }
      }

   if (! DONTDO)
      {
      if (S_ISDIR(statbuf->st_mode))
	 {
	 snprintf(OUTPUT,bufsize*2,"Deleting directory %s\n",path);
	 CfLog(cfinform,OUTPUT,"");

	 if (rmdir(name) == -1)
	    {
	    CfLog(cferror,"","unlink");
	    }
	 else
	    {
	    AddMultipleClasses(tlp->defines);
	    }
	 }
      else
	 {
	 int ret=false;
	 
	 if (tlp->compress == 'y')
	    {
	    CompressFile(name);
	    }
         else if ((ret = unlink(name)) == -1)
	    {
	    snprintf(OUTPUT,bufsize*2,"Couldn't unlink %s tidying\n",path);
            CfLog(cfverbose,OUTPUT,"unlink");
	    }

	 snprintf(OUTPUT,bufsize*2,"Deleting file %s\n",path);
	 CfLog(cfinform,OUTPUT,"");
	 snprintf(OUTPUT,bufsize*2,"Size=%d bytes, %c-age=%d days\n",
		 statbuf->st_size,tlp->searchtype,(nowticks-fileticks)/ticksperday);
	 CfLog(cfverbose,OUTPUT,"");

	 if (ret != -1)
	    {
	    AddMultipleClasses(tlp->defines);
	    }
	 }
      }
   else
      {
      if (tlp->compress == 'y')
	 {
	 printf("%s: want to compress %s\n",VPREFIX,path);
	 }
      else
	 {
	 printf("%s: want to delete %s\n",VPREFIX,path);
	 }
      }
   }
else
   {
   Debug2("(No age match)\n");
   }
}


/*********************************************************************/

void DeleteTidyList(list)

struct TidyPattern *list;

{
if (list != NULL)
   {
   DeleteTidyList(list->next);
   list->next = NULL;

   if (list->classes != NULL)
      {
      free (list->classes);
      }

   free((char *)list);
   }
}







