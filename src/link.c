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
/* TOOLKIT : links                                                   */
/*********************************************************************/

int LinkChildFiles(from,to,type,inclusions,exclusions,copy,nofile,ptr)

char *from, *to;
char type;
struct Item *inclusions, *exclusions, *copy;
short nofile;
struct Link *ptr;

{ DIR *dirh;
  struct dirent *dirp;
  char pcwdto[bufsize],pcwdfrom[bufsize];
  struct stat statbuf;
  int (*linkfiles) ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));

Debug("LinkChildFiles(%s,%s)\n",from,to);
  
if (stat(to,&statbuf) == -1)
   {
   return(false);  /* no error warning, since the higher level routine uses this */
   }

if ((dirh = opendir(to)) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",to);
   CfLog(cferror,OUTPUT,"opendir");
   return false;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,to,NULL))
      {
      continue;
      }

   strcpy(pcwdto,to);                               /* Assemble pathnames */
   AddSlash(pcwdto);

   if (BufferOverflow(pcwdto,dirp->d_name))
      {
      FatalError("Can't build filename in LinkChildFiles");
      }
   strcat(pcwdto,dirp->d_name);

   strcpy(pcwdfrom,from);
   AddSlash(pcwdfrom);

  if (BufferOverflow(pcwdfrom,dirp->d_name))
      {
      FatalError("Can't build filename in LinkChildFiles");
      }
   strcat(pcwdfrom,dirp->d_name);
   
   switch (type)
      {
      case 's':
                linkfiles = LinkFiles;
                break;
      case 'r':
	        linkfiles = RelativeLink;
		break;
      case 'a':
	        linkfiles = AbsoluteLink;
                break;
      case 'h':
                linkfiles = HardLinkFiles;
                break;
      default:
                printf("Internal error, link type was [%c]\n",type);
                continue;
      }
   
   (*linkfiles)(pcwdfrom,pcwdto,inclusions,exclusions,copy,nofile,ptr);
   }

closedir(dirh);
return true;
}

/*********************************************************************/

void LinkChildren(path,type,rootstat,uid,gid,inclusions,exclusions,copy,nofile,ptr)


/* --------------------------------------------------------------------
   Here we try to break up the path into a part which will match the
   last element of a mounted filesytem mountpoint and the remainder
   after that. We parse the path backwards to get a math e.g.

   /fys/lib/emacs ->  lib /emacs
                      fys /lib/emacs
                          /fys/lib/emacs

   we try to match lib and fys to the binserver list e.g. /mn/anyon/fys
   and hope for the best. If it doesn't match, tough! 
   --------------------------------------------------------------------- */

char *path, type;
struct stat *rootstat;
uid_t uid;
gid_t gid;
struct Item *inclusions, *exclusions, *copy;
short nofile;
struct Link *ptr;

{ char *sp;
  char lastlink[bufsize],server[bufsize],from[bufsize],to[bufsize],relpath[bufsize];
  char odir[bufsize];
  DIR *dirh;
  struct dirent *dirp;
  struct stat statbuf;
  int matched = false;
  int (*linkfiles) ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));

Debug("LinkChildren(%s)\n",path);
  
if (! S_ISDIR(rootstat->st_mode))
   {
   snprintf(OUTPUT,bufsize*2,"File %s is not a directory: it has no children to link!\n",path);
   CfLog(cferror,OUTPUT,"");
   return;
   }

Verbose("Linking the children of %s\n",path);
 
for (sp = path+strlen(path); sp != path-1; sp--)
   {
   if (*(sp-1) == '/')
      {
      relpath[0] = '\0';
      sscanf(sp,"%[^/]%s", lastlink,relpath);

      if (MatchAFileSystem(server,lastlink))
         {
         strcpy(odir,server);

	 if (BufferOverflow(odir,relpath))
	    {
	    FatalError("culprit: LinkChildren()");
	    }
         strcat(odir,relpath);

         if ((dirh = opendir(odir)) == NULL)
            {
            snprintf(OUTPUT,bufsize*2,"Can't open directory %s\n",path);
	    CfLog(cferror,OUTPUT,"opendir");
            return;
            }

         for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
            {
            if (!SensibleFile(dirp->d_name,odir,NULL))
               {
               continue;
               } 

            strcpy(from,path);
	    AddSlash(from);
	    
	    if (BufferOverflow(from,dirp->d_name))
	       {
	       FatalError("culprit: LinkChildren()");
	       }
	    
            strcat(from,dirp->d_name);
	    
            strcpy(to,odir);
            AddSlash(to);
	    
	    if (BufferOverflow(to,dirp->d_name))
	       {
	       FatalError("culprit: LinkChildren()");
	       }
	    
            strcat(to,dirp->d_name);

            Debug2("LinkChild from = %s to = %s\n",from,to);

            if (stat(to,&statbuf) == -1)
               {
               continue;
               }
            else
               {
	       switch (type)
                  {
                  case 's':
                            linkfiles = LinkFiles;
                            break;
                  case 'r':
	                    linkfiles = RelativeLink;
		            break;
                  case 'a':
	                    linkfiles = AbsoluteLink;
                            break;
                  case 'h':
                            linkfiles = HardLinkFiles;
                            break;
                  default:
                            snprintf(OUTPUT,bufsize*2,"Internal error, link type was [%c]\n",type);
			    CfLog(cferror,OUTPUT,"");
                            continue;
                  }

	       matched = (*linkfiles)(from,to,inclusions,exclusions,copy,nofile,ptr);

               if (matched && !DONTDO)
		 {
                 chown(from,uid,gid);
                 }
               }
            }

         if (matched) return;
         }
      }
   }

snprintf(OUTPUT,bufsize*2,"Couldn't link the children of %s to anything because no\n",path);
CfLog(cferror,OUTPUT,""); 
snprintf(OUTPUT,bufsize*2,"file system was found to mirror it in the defined binservers list.\n");
CfLog(cferror,OUTPUT,""); 
}

/*********************************************************************/

int RecursiveLink(lp,from,to,maxrecurse)

struct Link *lp;
char *from, *to;
int maxrecurse;

{ struct stat statbuf;
  DIR *dirh;
  struct dirent *dirp;
  char newfrom[bufsize];
  char newto[bufsize];
  void *bug_check;
  int (*linkfiles) ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));
 
if (maxrecurse == 0)  /* reached depth limit */
   {
   Debug2("MAXRECURSE ran out, quitting at level %s with endlist = %d\n",to,lp->next);
   return false;
   }

if (IgnoreFile(to,"",lp->ignores))
   {
   Verbose("%s: Ignoring directory %s\n",VPREFIX,from);
   return false;
   }

if (strlen(to) == 0)     /* Check for root dir */
   {
   to = "/";
   }

bug_check = lp->next;

if ((dirh = opendir(to)) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Can't open directory [%s]\n",to);
   CfLog(cferror,OUTPUT,"opendir");
   return false;
   }

if (lp->next != bug_check)
   {
   printf("%s: solaris BSD compat bug: opendir wrecked the heap memory!!",VPREFIX);
   printf("%s: in copy to %s, using workaround...\n",VPREFIX,from);
   lp->next = bug_check;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,to,NULL))
      {
      continue;
      }

   if (IgnoreFile(to,dirp->d_name,lp->ignores))
      {
      continue;
      }

   strcpy(newfrom,from);                                   /* Assemble pathname */
   AddSlash(newfrom);
   strcpy(newto,to);
   AddSlash(newto);

   if (BufferOverflow(newfrom,dirp->d_name))
      {
      closedir(dirh);
      return true;
      }

   strcat(newfrom,dirp->d_name);

   if (BufferOverflow(newto,dirp->d_name))
      {
      closedir(dirh);
      return true;
      }

   strcat(newto,dirp->d_name);

   if (TRAVLINKS)
      {
      if (stat(newto,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",newto);
	 CfLog(cfverbose,OUTPUT,"");
         continue;
         }
      }
   else
      {
      if (lstat(newto,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't stat %s\n",newto);
	 CfLog(cfverbose,OUTPUT,"");
	 
	 bzero(VBUFF,bufsize);
         if (readlink(newto,VBUFF,bufsize-1) != -1)
            {
            Verbose("File is link to -> %s\n",VBUFF);
            }
         continue;
         }
      }

   if (!FileObjectFilter(newto,&statbuf,lp->filters,links))
      {
      Debug("Skipping filtered file %s\n",newto);
      continue;
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      RecursiveLink(lp,newfrom,newto,maxrecurse-1);
      }
   else
      {
      switch (lp->type)
	 {
	 case 's':
                   linkfiles = LinkFiles;
                   break;
         case 'r':
	           linkfiles = RelativeLink;
		   break;
         case 'a':
	           linkfiles = AbsoluteLink;
                   break;
         case 'h':
                   linkfiles = HardLinkFiles;
                   break;
         default:
                   printf("cfengine: internal error, link type was [%c]\n",lp->type);
                   continue;
	 }

      (*linkfiles)(newfrom,newto,lp->inclusions,lp->exclusions,lp->copy,lp->nofile,lp);
      }
   }

closedir(dirh);
return true;
}

/*********************************************************************/

int LinkFiles(from,to_tmp,inclusions,exclusions,copy,nofile,ptr)

/* should return true if 'to' found */

char *from, *to_tmp;
struct Item *inclusions, *exclusions, *copy;
short nofile;
struct Link *ptr;

{ struct stat buf,savebuf;
  char to[bufsize],linkbuf[bufsize],saved[bufsize],absto[bufsize],*lastnode;
  struct UidList fakeuid;
  struct Image ip;

bzero(to,bufsize);
  
if ((*to_tmp != '/') && (*to_tmp != '.'))  /* links without a directory reference */
   {
   strcpy(to,"./");
   }

if (strlen(to_tmp)+3 > bufsize)
   {
   printf("%s: bufsize boundaries exceeded in LinkFiles(%s->%s)\n",VPREFIX,from,to_tmp);
   return false;
   }

strcat(to,to_tmp);
  
Debug2("Linkfiles(%s,%s)\n",from,to);

for (lastnode = from+strlen(from); *lastnode != '/'; lastnode--)
   {
   }

lastnode++;

if (IgnoredOrExcluded(links,lastnode,inclusions,exclusions))
   {
   Verbose("%s: Skipping non-included pattern %s\n",VPREFIX,from);
   return true;
   }

if (IsWildItemIn(VCOPYLINKS,lastnode) || IsWildItemIn(copy,lastnode))
   {
   fakeuid.uid = sameowner;
   fakeuid.next = NULL;
   ip.plus = samemode;
   ip.minus = samemode;
   ip.uid = &fakeuid;
   ip.gid = (struct GidList *) &fakeuid;
   ip.action = "do";
   ip.recurse = 0;
   ip.type = 't';
   ip.defines = ptr->defines;
   ip.elsedef = ptr->elsedef;
   ip.backup = true;
   ip.exclusions = NULL;
   ip.inclusions = NULL;
   ip.symlink = NULL;
   ip.classes = NULL;
   ip.plus_flags = 0;
   ip.minus_flags = 0;
   ip.server = strdup("localhost");
   Verbose("%s: Link item %s marked for copying instead\n",VPREFIX,from);
   MakeDirectoriesFor(to,'n');
   CheckImage(to,from,&ip);
   free(ip.server);
   return true;
   }

if (*to != '/')         /* relative path, must still check if exists */
   {
   Debug("Relative link destination detected: %s\n",to);
   strcpy(absto,AbsLinkPath(from,to));
   Debug("Absolute path to relative link = %s, from %s\n",absto,from);
   }
else
   {
   strcpy(absto,to);
   }

if (!nofile)
  {
  if (stat(absto,&buf) == -1)
     {
     return(false);  /* no error warning, since the higher level routine uses this */
     }
  }
else
   {
   Verbose("Not checking whether link pointed object exists\n");
   }
 
Debug2("Trying to link %s -> %s (%s)\n",from,to,absto);

if (lstat(from,&buf) == 0)
   {
   if (! S_ISLNK(buf.st_mode) && ! ENFORCELINKS)
      {
      snprintf(OUTPUT,bufsize*2,"Error linking %s -> %s\n",from,to);
      CfLog(cfsilent,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"Cannot make link: %s exists and is not a link! (uid %d)\n",from,buf.st_uid);
      CfLog(cfsilent,OUTPUT,"");
      return(true);
      }

   if (S_ISREG(buf.st_mode) && ENFORCELINKS)
      {
      snprintf(OUTPUT,bufsize*2,"Moving %s to %s%s\n",from,from,CF_SAVED);
      CfLog(cfsilent,OUTPUT,"");

      if (DONTDO)
         {
         return true;
         }

      saved[0] = '\0';
      strcpy(saved,from);
      strcat(saved,CF_SAVED);

      if (rename(from,saved) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't rename %s to %s\n",from,saved);
	 CfLog(cferror,OUTPUT,"rename");
         return(true);
         }

      if (Repository(saved,VREPOSITORY))
	 {
	 unlink(saved);
	 }
      }

   if (S_ISDIR(buf.st_mode) && ENFORCELINKS)
      {
      snprintf(OUTPUT,bufsize*2,"Moving directory %s to %s%s.dir\n",from,from,CF_SAVED);
      CfLog(cfsilent,OUTPUT,"");

      if (DONTDO)
         {
         return true;
         }

      saved[0] = '\0';
      strcpy(saved,from);
      strcat(saved,CF_SAVED);
      strcat(saved,".dir");

      if (stat(saved,&savebuf) != -1)
	 {
	 snprintf(OUTPUT,bufsize*2,"Couldn't save directory %s, since %s exists already\n",from,saved);
	 CfLog(cferror,OUTPUT,"");
	 snprintf(OUTPUT,bufsize*2,"Unable to force link to existing directory %s\n",from);
	 CfLog(cferror,OUTPUT,"");
	 return true;
	 }

      if (rename(from,saved) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't rename %s to %s\n",from,saved);
	 CfLog(cferror,OUTPUT,"rename");
         return(true);
         }
      }
   }

bzero(linkbuf,bufsize);

if (readlink(from,linkbuf,bufsize-1) == -1)
   {
   if (! MakeDirectoriesFor(from,'n'))                  /* link doesn't exist */
      {
      snprintf(OUTPUT,bufsize*2,"Couldn't build directory tree up to %s!\n",from);
      CfLog(cfsilent,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"One element was a plain file, not a directory!\n");
      CfLog(cfsilent,OUTPUT,"");      
      return(true);
      }
   }
else
   { int off1 = 0, off2 = 0;

   DeleteSlash(linkbuf);
   
   if (strncmp(linkbuf,"./",2) == 0)   /* Ignore ./ at beginning */
      {
      off1 = 2;
      }

   if (strncmp(to,"./",2) == 0)
      {
      off2 = 2;
      }
   
   if (strcmp(linkbuf+off1,to+off2) != 0)
      {
      if (ENFORCELINKS)
         {
         snprintf(OUTPUT,bufsize*2,"Removing link %s\n",from);
	 CfLog(cfinform,OUTPUT,"");

         if (!DONTDO)
            {
            if (unlink(from) == -1)
               {
               perror("unlink");
               return true;
               }

            return DoLink(from,to,ptr->defines);
            }
         }
      else
         {
         snprintf(OUTPUT,bufsize*2,"Old link %s points somewhere else. Doing nothing!\n",from);
	 CfLog(cfsilent,OUTPUT,"");
         snprintf(OUTPUT,bufsize*2,"(Link points to %s not %s)\n\n",linkbuf,to);
	 CfLog(cfsilent,OUTPUT,"");	 
         return(true);
         }
      }
   else
      {
      snprintf(OUTPUT,bufsize*2,"Link (%s->%s) exists.\n",from,to_tmp);
      CfLog(cfverbose,OUTPUT,"");

      if (!nofile)
	 {
	 KillOldLink(from,ptr->defines);      /* Check whether link points somewhere */
	 return true;
	 }

      AddMultipleClasses(ptr->elsedef);
      return(true);
      }
   }

return DoLink(from,to,ptr->defines);
}

/*********************************************************************/

int RelativeLink(from,to,inclusions,exclusions,copy,nofile,ptr)

char *from, *to;
struct Item *inclusions, *exclusions,*copy;
short nofile;
struct Link *ptr;
/* global char LINKTO[] */

{ char *sp, *commonto, *commonfrom;
  char buff[bufsize];
  int levels=0;
  
Debug2("RelativeLink(%s,%s)\n",from,to);

if (*to == '.')
   {
   return LinkFiles(from,to,inclusions,exclusions,copy,nofile,ptr);
   }

if (!CompressPath(LINKTO,to))
   {
   snprintf(OUTPUT,bufsize*2,"Failed to link %s to %s\n",from,to);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

commonto = LINKTO;
commonfrom = from;

if (strcmp(commonto,commonfrom) == 0)
   {
   CfLog(cferror,"Can't link file to itself!\n","");
   snprintf(OUTPUT,bufsize*2,"(%s -> %s)\n",from,to);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

while (*commonto == *commonfrom)
   {
   commonto++;
   commonfrom++;
   }

while (!((*commonto == '/') && (*commonfrom == '/')))
   {
   commonto--;
   commonfrom--;
   }

commonto++; 

Debug("Commonto = %s, common from = %s\n",commonto,commonfrom); 

for (sp = commonfrom; *sp != '\0'; sp++)
   {
   if (*sp == '/')
       {
       levels++;
       }
   }

Debug("LEVELS = %d\n",levels);
 
bzero(buff,bufsize);

strcat(buff,"./");

while(--levels > 0)
   {
   if (BufferOverflow(buff,"../"))
      {
      return false;
      }
   
   strcat(buff,"../");
   }

if (BufferOverflow(buff,commonto))
   {
   return false;
   }

strcat(buff,commonto);
 
return LinkFiles(from,buff,inclusions,exclusions,copy,nofile,ptr);
}

/*********************************************************************/

int AbsoluteLink(from,to,inclusions,exclusions,copy,nofile,ptr)

char *from, *to;
struct Item *inclusions,*exclusions,*copy;
short nofile;
struct Link *ptr;
/* global LINKTO */

{ char absto[bufsize];
  char expand[bufsize];
  
Debug2("AbsoluteLink(%s,%s)\n",from,to);

if (*to == '.')
   {
   strcpy(LINKTO,from);
   ChopLastNode(LINKTO);
   AddSlash(LINKTO);
   strcat(LINKTO,to);
   }
else
   {
   strcpy(LINKTO,to);
   }

CompressPath(absto,LINKTO);

expand[0] = '\0';

if (!nofile)
   {  
   if (!ExpandLinks(expand,absto,0))  /* begin at level 1 and beam out at 15 */
      {
      CfLog(cferror,"Failed to make absolute link in\n","");
      snprintf(OUTPUT,bufsize*2,"%s -> %s\n",from,to);
      CfLog(cferror,OUTPUT,"");
      return false;
      }
   else
      {
      Debug2("ExpandLinks returned %s\n",expand);
      }
   }
else
   {
   strcpy(expand,absto);
   }

CompressPath(LINKTO,expand);

return LinkFiles(from,LINKTO,inclusions,exclusions,copy,nofile,ptr);
}

/*********************************************************************/

int DoLink (from,to,defines)

char *from, *to, *defines;

{
if (DONTDO)
   {
   printf("cfengine: Need to link files %s -> %s\n",from,to);
   }
else
   {
   snprintf(OUTPUT,bufsize*2,"Linking files %s -> %s\n",from,to);
   CfLog(cfinform,OUTPUT,"");

   if (symlink(to,from) == -1)
      {
      snprintf(OUTPUT,bufsize*2,"Couldn't link %s to %s\n",to,from);
      CfLog(cferror,OUTPUT,"symlink");
      return false;
      }
   else
      {
      AddMultipleClasses(defines);
      return true;
      }
   }
 return true;
}

/*********************************************************************/

void KillOldLink(name,defs)

char *name,*defs;

{ char linkbuf[bufsize];
  char linkpath[bufsize],*sp;
  struct stat statbuf;

Debug("KillOldLink(%s)\n",name);
bzero(linkbuf,bufsize);
bzero(linkpath,bufsize); 

if (readlink(name,linkbuf,bufsize-1) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"(Can't read link %s while checking for deadlinks)\n",name);
   CfLog(cfverbose,OUTPUT,"");
   return;
   }

if (linkbuf[0] != '/')
   {
   strcpy(linkpath,name);    /* Get path to link */

   for (sp = linkpath+strlen(linkpath); (*sp != '/') && (sp >= linkpath); sp-- )
     {
     *sp = '\0';
     }
   }

strcat(linkpath,linkbuf);
CompressPath(VBUFF,linkpath); 
 
if (stat(VBUFF,&statbuf) == -1)               /* link points nowhere */
   {
   if (KILLOLDLINKS || DEBUG || D2)
      {
      snprintf(OUTPUT,bufsize*2,"%s is a link which points to %s, but that file doesn't seem to exist\n",name,VBUFF);
      CfLog(cfsilent,OUTPUT,"");
      }

   if (KILLOLDLINKS)
      {
      snprintf(OUTPUT,bufsize*2,"Removing dead link %s\n",name);
      CfLog(cfinform,OUTPUT,"");

      if (! DONTDO)
         {
         unlink(name);  /* May not work on a client-mounted system ! */
	 AddMultipleClasses(defs);
         }
      }
   }
}

/*********************************************************************/

int HardLinkFiles(from,to,inclusions,exclusions,copy,nofile,ptr)  /* should return true if 'to' found */

char *from, *to;
struct Item *inclusions,*exclusions,*copy;
short nofile;
struct Link *ptr;

{ struct stat frombuf,tobuf;
  char saved[bufsize], *lastnode;
  struct UidList fakeuid;
  struct Image ip;
  
for (lastnode = from+strlen(from); *lastnode != '/'; lastnode--)
   {
   }

lastnode++;

if (inclusions != NULL && !IsWildItemIn(inclusions,lastnode))
   {
   Verbose("%s: Skipping non-included pattern %s\n",VPREFIX,from);
   return true;
   }

if (IsWildItemIn(VEXCLUDELINK,lastnode) || IsWildItemIn(exclusions,lastnode))
   {
   Verbose("%s: Skipping excluded pattern %s\n",VPREFIX,from);
   return true;
   }

if (IsWildItemIn(VCOPYLINKS,lastnode) || IsWildItemIn(copy,lastnode))
   {
   fakeuid.uid = sameowner;
   fakeuid.next = NULL;
   ip.plus = samemode;
   ip.minus = samemode;
   ip.uid = &fakeuid;
   ip.gid = (struct GidList *) &fakeuid;
   ip.action = "do";
   ip.recurse = 0;
   ip.type = 't';
   ip.backup = true;
   ip.plus_flags = 0;
   ip.minus_flags = 0;
   ip.exclusions = NULL;
   ip.symlink = NULL;
   Verbose("%s: Link item %s marked for copying instead\n",VPREFIX,from);
   CheckImage(to,from,&ip);
   return true;
   }

if (stat(to,&tobuf) == -1)
   {
   return(false);  /* no error warning, since the higher level routine uses this */
   }

if (! S_ISREG(tobuf.st_mode))
   {
   snprintf(OUTPUT,bufsize*2,"%s: will only hard link regular files and %s is not regular\n",VPREFIX,to);
   CfLog(cfsilent,OUTPUT,"");
   return true;
   }

Debug2("Trying to (hard) link %s -> %s\n",from,to);

if (stat(from,&frombuf) == -1)
   {
   DoHardLink(from,to,ptr->defines);
   return true;
   }

    /* both files exist, but are they the same file? POSIX says  */
    /* the files could be on different devices, but unix doesn't */
    /* allow this behaviour so the tests below are theoretical...*/

if (frombuf.st_ino != tobuf.st_ino && frombuf.st_dev != frombuf.st_dev)
   {
   Verbose("If this is POSIX, unable to determine if %s is hard link is correct\n",from);
   Verbose("since it points to a different filesystem!\n");

   if (frombuf.st_mode == tobuf.st_mode && frombuf.st_size == tobuf.st_size)
      {
      snprintf(OUTPUT,bufsize*2,"Hard link (%s->%s) on different device APPEARS okay\n",from,to);
      CfLog(cfverbose,OUTPUT,"");
      AddMultipleClasses(ptr->elsedef);
      return true;
      }
   }

if (frombuf.st_ino == tobuf.st_ino && frombuf.st_dev == frombuf.st_dev)
   {
   snprintf(OUTPUT,bufsize*2,"Hard link (%s->%s) exists and is okay.\n",from,to);
   CfLog(cfverbose,OUTPUT,"");
   AddMultipleClasses(ptr->elsedef);
   return true;
   }

snprintf(OUTPUT,bufsize*2,"%s does not appear to be a hard link to %s\n",from,to);
CfLog(cfinform,OUTPUT,""); 

if (ENFORCELINKS)
   {
   snprintf(OUTPUT,bufsize*2,"Moving %s to %s.%s\n",from,from,CF_SAVED);
   CfLog(cfinform,OUTPUT,"");

   if (DONTDO)
      {
      return true;
      }

   saved[0] = '\0';
   strcpy(saved,from);
   strcat(saved,CF_SAVED);

   if (rename(from,saved) == -1)
      {
      perror("rename");
      return(true);
      }

   DoHardLink(from,to,ptr->defines);
   }

return(true);
}

/*********************************************************************/

void DoHardLink (from,to,defines)

char *from, *to, *defines;

{
if (DONTDO)
   {
   printf("Hardlink files %s -> %s\n\n",from,to);
   }
else
   {
   snprintf(OUTPUT,bufsize*2,"Hardlinking files %s -> %s\n",from,to);
   CfLog(cfinform,OUTPUT,"");

   if (link(to,from) == -1)
      {
      CfLog(cferror,"","link");
      }
   else
      {
      AddMultipleClasses(defines);
      }
   }
}

/*********************************************************************/

int ExpandLinks(dest,from,level)                            /* recursive */

  /* Expand a path contaning symbolic links, up to 4 levels  */
  /* of symbolic links and then beam out in a hurry !        */

char *dest, *from;
int level;

{ char *sp, buff[bufsize];
  char node[maxlinksize];
  struct stat statbuf;
  int lastnode = false;

bzero(dest,bufsize);

Debug2("ExpandLinks(%s,%d)\n",from,level);

if (level >= maxlinklevel)
   {
   CfLog(cferror,"Too many levels of symbolic links to evaluate absolute path\n","");
   return false;
   }

for (sp = from; *sp != '\0'; sp++)
   {
   if (*sp == '/')
      {
      continue;
      }
   
   sscanf(sp,"%[^/]",node);
   sp += strlen(node);

   if (*sp == '\0')
      {
      lastnode = true;
      }
   
   if (strcmp(node,".") == 0)
      {
      continue;
      }

   if (strcmp(node,"..") == 0)
      {
      if (! ChopLastNode(LINKTO))
	 {
	 Debug("cfengine: used .. beyond top of filesystem!\n");
	 return false;
	 }
      continue;
      }
   else
      {
      strcat(dest,"/");
      }
   
   strcat(dest,node);

   if (lstat(dest,&statbuf) == -1)  /* File doesn't exist so we can stop here */
      {
      snprintf(OUTPUT,bufsize*2,"Can't stat %s in ExpandLinks\n",dest);
      CfLog(cferror,OUTPUT,"stat");
      return false;
      }

   if (S_ISLNK(statbuf.st_mode))
      {
      bzero(buff,bufsize);
      
      if (readlink(dest,buff,bufsize-1) == -1)
	 {
	 snprintf(OUTPUT,bufsize*2,"Expand links can't stat %s\n",dest);
	 CfLog(cferror,OUTPUT,"readlink");
	 return false;
	 }
      else
         {
         if (buff[0] == '.')
	    {
            ChopLastNode(dest);
	    AddSlash(dest);
	    if (BufferOverflow(dest,buff))
	       {
	       return false;
	       }
	    strcat(dest,buff);
	    }
         else if (buff[0] == '/')
	    {
  	    strcpy(dest,buff);
	    DeleteSlash(dest);

	    if (strcmp(dest,from) == 0)
	       {
	       Debug2("No links to be expanded\n");
	       return true;
	       }
	    
	    if (!lastnode && !ExpandLinks(buff,dest,level+1))
	       {
	       return false;
	       }
	    }
	 else
	    {
	    ChopLastNode(dest);
	    AddSlash(dest);
	    strcat(dest,buff);
	    DeleteSlash(dest);

	    if (strcmp(dest,from) == 0)
	       {
	       Debug2("No links to be expanded\n");
	       return true;
	       }
	    
	    bzero(buff,bufsize);

	    if (!lastnode && !ExpandLinks(buff,dest,level+1))
	       {
	       return false;
	       }	    
	    }
         }
      }
   }
return true;
}

/*********************************************************************/

char *AbsLinkPath (from,relto)

char *from, *relto;

/* Take an abolute source and a relative destination object
   and find the absolute name of the to object */

{ char *sp;
  int pop = 1;
 
if (*relto == '/')
   {
   printf("Cfengine internal error: call to AbsLInkPath with absolute pathname\n");
   FatalError("");
   }

strcpy(DESTINATION,from);  /* reuse to save stack space */
 
for (sp = relto; *sp != '\0'; sp++)
   {
   if (strncmp(sp,"../",3) == 0)
      {
      pop++;
      sp += 2;
      continue;
      }

   if (strncmp(sp,"./",2) == 0)
      {
      sp += 1;
      continue;
      }

   break; /* real link */
   }

while (pop > 0)
    {
    ChopLastNode(DESTINATION);
    pop--;
    }

if (strlen(DESTINATION) == 0)
   {
   strcpy(DESTINATION,"/");
   }
else
   {
   AddSlash(DESTINATION);
   }
 
strcat(DESTINATION,sp);
Debug("Reconstructed absolute linkname = %s\n",DESTINATION);
return DESTINATION; 
}
