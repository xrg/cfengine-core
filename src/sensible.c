/*

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

/*****************************************************************************/
/*                                                                           */
/* File: sensible.c                                                          */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/
/* Files to be ignored when parsing directories                      */
/*********************************************************************/

char *VSKIPFILES[] =
   {
   ".",
   "..",
   "lost+found",
   ".cfengine.rm",
   NULL
   };

/*********************************************************************/

int SensibleFile(nodename,path,ip)

char *nodename, *path;
struct Image *ip;

{ int i, suspicious = true;
  struct stat statbuf; 
  unsigned char *sp, newname[bufsize],vbuff[bufsize];

if (strlen(nodename) < 1)
   {
   snprintf(OUTPUT,bufsize,"Empty (null) filename detected in %s\n",path);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

if (IsItemIn(SUSPICIOUSLIST,nodename))
   {
   snprintf(OUTPUT,bufsize,"Suspicious file %s found in %s\n",nodename,path);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

if ((strcmp(nodename,"...") == 0) && (strcmp(path,"/") == 0))
   {
   Verbose("DFS cell node detected in /...\n");
   return true;
   }
  
for (i = 0; VSKIPFILES[i] != NULL; i++)
   {
   if (strcmp(nodename,VSKIPFILES[i]) == 0)
      {
      Debug("Filename %s/%s is classified as ignorable\n",path,nodename);
      return false;
      }
   }

if ((strcmp("[",nodename) == 0) && (strcmp("/usr/bin",path) == 0))
   {
   if (VSYSTEMHARDCLASS == linuxx)
      {
      return true;
      }
   }

suspicious = true;

for (sp = nodename; *sp != '\0'; sp++)
   {
   if ((*sp > 31) && (*sp < 127))
      {
      suspicious = false;
      break;
      }
   }

strcpy(vbuff,path);
AddSlash(vbuff);
strcat(vbuff,nodename); 

if (suspicious && NONALPHAFILES)
   {
   snprintf(OUTPUT,bufsize,"Suspicious filename %s in %s has no alphanumeric content (security)",CanonifyName(nodename),path);
   CfLog(cfsilent,OUTPUT,"");
   strcpy(newname,vbuff);

   for (sp = newname+strlen(path); *sp != '\0'; sp++)
      {
      if ((*sp > 126) || (*sp < 33))
	 {
	 *sp = 50 + (*sp / 50);  /* Create a visible ASCII interpretation */
	 }
      }

   strcat(newname,".cf-nonalpha");
   
   snprintf(OUTPUT,bufsize,"Renaming file %s to %s",vbuff,newname);
   CfLog(cfsilent,OUTPUT,"");
   
   if (rename(vbuff,newname) == -1)
      {
      CfLog(cfverbose,"Rename failed - foreign filesystem?\n","rename");
      }
   if (chmod(newname,0644) == -1)
      {
      CfLog(cfverbose,"Mode change failed - foreign filesystem?\n","chmod");
      }
   return false;
   }

if (strstr(nodename,".") && (EXTENSIONLIST != NULL))
   {
   if (cflstat(vbuff,&statbuf,ip) == -1)
      {
      snprintf(OUTPUT,bufsize,"Couldn't examine %s - foreign filesystem?\n",vbuff);
      CfLog(cfverbose,OUTPUT,"lstat");
      return true;
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      if (strcmp(nodename,"...") == 0)
	 {
	 Verbose("Hidden directory ... found in %s\n",path);
	 return true;
	 }

      for (sp = nodename+strlen(nodename)-1; *sp != '.'; sp--)
	 {
	 }

      if ((char *)sp != nodename) /* Don't get .dir */
	 {
	 sp++; /* Find file extension, look for known plain files  */

	 if ((strlen(sp) > 0) && IsItemIn(EXTENSIONLIST,sp))
	    {
	    snprintf(OUTPUT,bufsize,"Suspicious directory %s in %s looks like plain file with extension .%s",nodename,path,sp);
	    CfLog(cfsilent,OUTPUT,"");
	    return false;
	    }
	 }
      }
   }

for (sp = nodename; *sp != '\0'; sp++) /* Check for files like ".. ." */
   {
   if ((*sp != '.') && ! isspace(*sp))
      {
      suspicious = false;
      return true;
      }
   }

snprintf(OUTPUT,bufsize,"Suspicous looking file object \"%s\" masquerading as hidden file in %s\n",nodename,path);
CfLog(cfsilent,OUTPUT,"");
Debug("Filename looks suspicious\n"); 

/* removed if (EXTENSIONLIST==NULL) mb */ 

if (cflstat(vbuff,&statbuf,ip) == -1)
   {
   snprintf(OUTPUT,bufsize,"Couldn't stat %s",vbuff);
   CfLog(cfverbose,OUTPUT,"lstat");
   return true;
   }

if (S_ISLNK(statbuf.st_mode))
   {
   snprintf(OUTPUT,bufsize,"   %s is a symbolic link\n",nodename);
   CfLog(cfsilent,OUTPUT,"");
   }
else if (S_ISDIR(statbuf.st_mode))
   {
   snprintf(OUTPUT,bufsize,"   %s is a directory\n",nodename);
   CfLog(cfsilent,OUTPUT,"");
   }

snprintf(OUTPUT,bufsize,"[%s] has size %ld and full mode %o\n",nodename,(unsigned long)(statbuf.st_size),(unsigned int)(statbuf.st_mode));
CfLog(cfsilent,OUTPUT,"");
 
return true;
}
