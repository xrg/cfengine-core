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
   MERCHANTABILITY or FITNESS FOR A PARTICULARmandar PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/
 

/*********************************************************************/
/*                                                                   */
/*  TOOLKITS: "object" library                                       */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include "../pub/global.h"
#include <db.h>

/*********************************************************************/
/* TOOLKIT : files/directories                                       */
/*********************************************************************/

int DirPush(char *name,struct stat *sb)          /* Enter dir and check for race exploits */

{
if (chdir(name) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not change to directory %s, mode %o in tidy",name,sb->st_mode & 07777);
   CfLog(cfinform,OUTPUT,"chdir");
   return false;
   }
else
   {
   Debug("Changed directory to %s\n",name);
   }

CheckLinkSecurity(sb,name);
return true; 
}

/**********************************************************************/

void DirPop(int goback,char * name,struct stat *sb)      /* Exit dir and check for race exploits */

{
if (goback && TRAVLINKS)
   {
   if (chdir(name) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Error in backing out of recursive descent securely to %s",name);
      CfLog(cferror,OUTPUT,"chdir");
      HandleSignal(SIGTERM);
      }
   
   CheckLinkSecurity(sb,name); 
   }
else if (goback)
   {
   if (chdir("..") == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Error in backing out of recursive descent securely to %s",name);
      CfLog(cferror,OUTPUT,"chdir");
      HandleSignal(SIGTERM);
      }
   }
}

/**********************************************************************/

void CheckLinkSecurity(struct stat *sb,char *name)

{ struct stat security;

Debug("Checking the inode and device to make sure we are where we think we are...\n"); 

if (stat(".",&security) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not stat directory %s after entering!",name);
   CfLog(cferror,OUTPUT,"stat");
   return;
   }

if ((sb->st_dev != security.st_dev) || (sb->st_ino != security.st_ino))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"SERIOUS SECURITY ALERT: path race exploited in recursion to/from %s. Not safe for agent to continue - aborting",name);
   CfLog(cferror,OUTPUT,"");
   HandleSignal(SIGTERM);
   /* Exits */
   }
}

/**********************************************************************/

void TruncateFile(char *name)

{ struct stat statbuf;
  int fd;

if (stat(name,&statbuf) == -1)
   {
   Debug2("cfengine: didn't find %s to truncate\n",name);
   return;
   }
else
   {
   if ((fd = creat(name,000)) == -1)      /* dummy mode ignored */
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"creat(%s) failed\n",name);
      CfLog(cferror,OUTPUT,"creat");
      }
   else
      {
      close(fd);
      }
   }
}

/*************************************************************************/

int FileSecure (char *name)

{ struct stat statbuf;

if (PARSEONLY || !CFPARANOID)
   {
   return true;
   }

if (stat(name,&statbuf) == -1)
   {
   return false;
   }

if (statbuf.st_uid != getuid())
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is not owned by uid %d (security exception)",name,getuid());
   CfLog(cferror,OUTPUT,"");
   }
 
/* Is the file writable by ANYONE except the owner ? */
 
if (statbuf.st_mode & (S_IWGRP | S_IWOTH))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s (owner %d) is writable by others (security exception)",name,getuid());
   CfLog(cferror,OUTPUT,"");
   return false;
   }

return true; 
}


/*************************************************************************/

int IgnoredOrExcluded(enum actions action,char *file,struct Item *inclusions,struct Item *exclusions)

{ char *lastnode;

Debug("IgnoredOrExcluded(%s)\n",file);

if (strstr(file,"/"))
   {
   for (lastnode = file+strlen(file); *lastnode != '/'; lastnode--)
      {
      }

   lastnode++;
   }
else
   {
   lastnode = file;
   }

if ((inclusions != NULL) && !IsWildItemIn(inclusions,lastnode))
   {
   Debug("cfengine: skipping non-included pattern %s\n",file);
   return true;
   }

switch(action)
   {
   case image:
              if (IsWildItemIn(VEXCLUDECOPY,lastnode) || IsWildItemIn(exclusions,lastnode))
                 {
                 Debug("Skipping excluded pattern %s\n",file);
                 return true;
                 }
              break;
              
   case links:
              if (IsWildItemIn(VEXCLUDELINK,lastnode) || IsWildItemIn(exclusions,lastnode))
                 {
                 Debug("Skipping excluded pattern %s\n",file);
                 return true;
                 }
              
              break;
   default:
              if (IsWildItemIn(exclusions,lastnode))
                 {
                 Debug("Skipping excluded pattern %s\n",file);
                 return true;
                 }       
   }

return false;
}


/*********************************************************************/

void Banner(char *string)

{
Verbose("---------------------------------------------------------------------\n");
Verbose("%s\n",string);
Verbose("---------------------------------------------------------------------\n\n");
}

/*******************************************************************/

int ShellCommandReturnsZero(char *comm,int useshell)

{ int status, i, argc = 0;
  pid_t pid;
  char arg[CF_MAXSHELLARGS][CF_BUFSIZE];
  char **argv;

if (!useshell)
   {
   /* Build argument array */

   for (i = 0; i < CF_MAXSHELLARGS; i++)
      {
      memset (arg[i],0,CF_BUFSIZE);
      }

   argc = SplitCommand(comm,arg);

   if (argc == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Too many arguments in %s\n",comm);
      CfLog(cferror,OUTPUT,"");
      return false;
      }
   }
    
if ((pid = fork()) < 0)
   {
   FatalError("Failed to fork new process");
   }
else if (pid == 0)                     /* child */
   {
   if (useshell)
      {
      if (execl("/bin/sh","sh","-c",comm,NULL) == -1)
         {
         yyerror("script failed");
         perror("execl");
         exit(1);
         }
      }
   else
      {
      argv = (char **) malloc((argc+1)*sizeof(char *));

      if (argv == NULL)
         {
         FatalError("Out of memory");
         }

      for (i = 0; i < argc; i++)
         {
         argv[i] = arg[i];
         }

      argv[i] = (char *) NULL;

      if (execv(arg[0],argv) == -1)
         {
         yyerror("script failed");
         perror("execvp");
         exit(1);
         }

      free((char *)argv);
      }
   }
else                                    /* parent */
   {
   pid_t wait_result;
   
   while ((wait_result = wait(&status)) != pid)
      {
      if (wait_result <= 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Wait for child failed\n");
         CfLog(cfinform,OUTPUT,"wait");
         return false;
         }
      }

   if (WIFSIGNALED(status))
      {
      Debug("Script %s returned: %d\n",comm,WTERMSIG(status));
      return false;
      }
   
   if (! WIFEXITED(status))
      {
      return false;
      }
   
   if (WEXITSTATUS(status) == 0)
      {
      Debug("Shell command returned 0\n");
      return true;
      }
   else
      {
      Debug("Shell command was non-zero: %d\n",WEXITSTATUS(status));
      return false;
      }
   }

return false;
}


/*********************************************************************/

void SetClassesOnScript(char *execstr,char *classes,char *elseclasses,int useshell)

{ FILE *pp;
  int print;
  char line[CF_BUFSIZE],*sp;
 
switch (useshell)
   {
   case 'y':  pp = cfpopen_sh(execstr,"r");
              break;
   default:   pp = cfpopen(execstr,"r");
              break;      
   }
 
if (pp == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",execstr);
   CfLog(cferror,OUTPUT,"popen");
   return;
   } 
 
while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",execstr);
      CfLog(cferror,OUTPUT,"ferror");
      break;
      }
   
   ReadLine(line,CF_BUFSIZE,pp);
   
   if (strstr(line,"cfengine-die"))
      {
      break;
      }
   
   if (ferror(pp))  /* abortable */
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",execstr);
      CfLog(cferror,OUTPUT,"ferror");
      break;
      }
   
   /*
    * Dumb script - echo non-empty lines to standard output.
    */
   
   print = false;
   
   for (sp = line; *sp != '\0'; sp++)
      {
      if (! isspace((int)*sp))
         {
         print = true;
         break;
         }
      }
   
   if (print)
      {
      Verbose("%s:%s: %s\n",VPREFIX,execstr,line);
      }
   }
 
cfpclose_def(pp,classes,elseclasses);
}

/*********************************************************************/

char *UnQuote(char *s)

{
 if (s[strlen(s)-1] == '\"')
    {
    s[strlen(s)-1] = '\0';
    }

 if (s[0] == '\"')
    {
    return s+1;
    }
 else
    {
    return s;
    }
}

/*********************************************************************/

void AddListSeparator(char *s)

{
if (s[strlen(s)-1] != LISTSEPARATOR)
   {
   s[strlen(s)+1] = '\0';
   s[strlen(s)] = LISTSEPARATOR;
   }
}

/*********************************************************************/

void ChopListSeparator(char *s)

{
if (s[strlen(s)-1] == LISTSEPARATOR)
   {
   s[strlen(s)-1] = '\0';
   } 
}

/*******************************************************************/

void IDClasses()

{ struct stat statbuf;
  char *sp;
  int i = 0;

AddClassToHeap("any");      /* This is a reserved word / wildcard */
 
snprintf(VBUFF,CF_BUFSIZE,"cfengine_%s",CanonifyName(VERSION));
AddClassToHeap(VBUFF);
 
for (sp = VBUFF+strlen(VBUFF); i < 2; sp--)
   {
   if (*sp == '_')
      {
      i++;
      *sp = '\0';
      AddClassToHeap(VBUFF);
      }
   }

if (stat("/etc/mandrake-release",&statbuf) != -1)
   {
   Verbose("This appears to be a mandrake system.\n");
   AddClassToHeap("Mandrake");
   linux_mandrake_version();
   }
/* Mandrake has a symlink at /etc/redhat-release pointing to
 * /etc/mandrake-release, so we else-if around that
 */
else if (stat("/etc/redhat-release",&statbuf) != -1)
   {
   Verbose("This appears to be a redhat system.\n");
   AddClassToHeap("redhat");
   linux_redhat_version();
   }

if (stat("/etc/SuSE-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a SuSE system.\n");
   AddClassToHeap("SuSE");
   linux_suse_version();
   }

if (stat("/etc/slackware-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a slackware system.\n");
   AddClassToHeap("slackware");
   }
 
if (stat("/etc/debian_version",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a debian system.\n");
   AddClassToHeap("debian");
   debian_version();
   }

if (stat("/etc/UnitedLinux-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a UnitedLinux system.\n");
   AddClassToHeap("UnitedLinux");
   }

lsb_version();

if (stat("/etc/vmware",&statbuf) != -1)
   {
   if (S_ISDIR(statbuf.st_mode))
      {
      Verbose("\nThis appears to be a VMWare xSX system.\n");
      AddClassToHeap("VMware");
      VM_version();
      }
   }
 
}

/*********************************************************************************/

int linux_fedora_version(void)
{
#define FEDORA_ID "Fedora Core"

#define RELEASE_FLAG "release "

/* We are looking for one of the following strings...
 *
 * Fedora Core release 1 (Yarrow)
 */

#define FEDORA_REL_FILENAME "/etc/fedora-release"

FILE *fp;

/* The full string read in from fedora-release */
char relstring[CF_MAXVARSIZE];
char classbuf[CF_MAXVARSIZE];

/* Fedora */
char *vendor="";
/* Where the numerical release will be found */
char *release=NULL;

int major = -1;
char strmajor[CF_MAXVARSIZE];

/* Grab the first line from the file and then close it. */
 if ((fp = fopen(FEDORA_REL_FILENAME,"r")) == NULL)
    {
    return 1;
    }
 fgets(relstring, sizeof(relstring), fp);
 fclose(fp);
 
 Verbose("Looking for fedora core linux info...\n");
 
 /* First, try to grok the vendor */
 if(!strncmp(relstring, FEDORA_ID, strlen(FEDORA_ID)))
    {
    vendor = "fedora";
    }
 else
    {
    Verbose("Could not identify OS distro from %s\n", FEDORA_REL_FILENAME);
    return 2;
    }
 
 /* Now, grok the release.  We assume that all the strings will
  * have the word 'release' before the numerical release.
  */
 release = strstr(relstring, RELEASE_FLAG);
 if(release == NULL)
    {
    Verbose("Could not find a numeric OS release in %s\n",
     FEDORA_REL_FILENAME);
    return 2;
    }
 else
    {
    release += strlen(RELEASE_FLAG);
    if (sscanf(release, "%d", &major) == 1)
       {
       sprintf(strmajor, "%d", major);
       }
    }
 
 if (major != -1 && (strcmp(vendor,"") != 0))
    {
    classbuf[0] = '\0';
    strcat(classbuf, vendor);
    AddClassToHeap(classbuf);
    strcat(classbuf, "_");
    strcat(classbuf, strmajor);
    AddClassToHeap(classbuf);
    }
 
 return 0;
}

/*********************************************************************************/

int linux_redhat_version(void)
{
#define REDHAT_ID "Red Hat Linux"
#define REDHAT_AS_ID "Red Hat Enterprise Linux AS"
#define REDHAT_AS21_ID "Red Hat Linux Advanced Server"
#define REDHAT_ES_ID "Red Hat Enterprise Linux ES"
#define REDHAT_WS_ID "Red Hat Enterprise Linux WS"
#define MANDRAKE_ID "Linux Mandrake"
#define MANDRAKE_10_1_ID "Mandrakelinux"
#define FEDORA_ID "Fedora Core"
#define WHITEBOX_ID "White Box Enterprise Linux"
#define CENTOS_ID "CentOS"
#define SCIENTIFIC_SL_ID "Scientific Linux SL"
#define SCIENTIFIC_CERN_ID "Scientific Linux CERN"
#define RELEASE_FLAG "release "

/* We are looking for one of the following strings...
 *
 * Red Hat Linux release 6.2 (Zoot)
 * Red Hat Linux Advanced Server release 2.1AS (Pensacola)
 * Red Hat Enterprise Linux AS release 3 (Taroon)
 * Red Hat Enterprise Linux WS release 3 (Taroon)
 * Linux Mandrake release 7.1 (helium)
 * Red Hat Enterprise Linux ES release 2.1 (Panama)
 * Fedora Core release 1 (Yarrow)
 * White Box Enterprise linux release 3.0 (Liberation)
 * Scientific Linux SL Release 4.0 (Beryllium)
 * CentOS release 4.0 (Final)
 */

#define RH_REL_FILENAME "/etc/redhat-release"

FILE *fp;

/* The full string read in from redhat-release */
char relstring[CF_MAXVARSIZE];
char classbuf[CF_MAXVARSIZE];

/* Red Hat, Mandrake */
char *vendor="";
/* as (Advanced Server, Enterprise) */
char *edition="";
/* Where the numerical release will be found */
char *release=NULL;
int i;
int major = -1;
char strmajor[CF_MAXVARSIZE];
int minor = -1;
char strminor[CF_MAXVARSIZE];

/* Grab the first line from the file and then close it. */
 if ((fp = fopen(RH_REL_FILENAME,"r")) == NULL)
    {
    return 1;
    }
 fgets(relstring, sizeof(relstring), fp);
 fclose(fp);
 
Verbose("Looking for redhat linux info in \"%s\"\n",relstring);
 
 /* First, try to grok the vendor and the edition (if any) */
 if(!strncmp(relstring, REDHAT_ES_ID, strlen(REDHAT_ES_ID)))
    {
    vendor = "redhat";
    edition = "es";
    }
 else if(!strncmp(relstring, REDHAT_WS_ID, strlen(REDHAT_WS_ID)))
    {
    vendor = "redhat";
    edition = "ws";
    }
 else if(!strncmp(relstring, REDHAT_WS_ID, strlen(REDHAT_WS_ID)))
    {
    vendor = "redhat";
    edition = "ws";
    }
 else if(!strncmp(relstring, REDHAT_AS_ID, strlen(REDHAT_AS_ID)) ||
  !strncmp(relstring, REDHAT_AS21_ID, strlen(REDHAT_AS21_ID)))
    {
    vendor = "redhat";
    edition = "as";
    }
 else if(!strncmp(relstring, REDHAT_ID, strlen(REDHAT_ID)))
    {
    vendor = "redhat";
    }
 else if(!strncmp(relstring, MANDRAKE_ID, strlen(MANDRAKE_ID)))
    {
    vendor = "mandrake";
    }
 else if(!strncmp(relstring, MANDRAKE_10_1_ID, strlen(MANDRAKE_10_1_ID)))
    {
    vendor = "mandrake";
    }
 else if(!strncmp(relstring, FEDORA_ID, strlen(FEDORA_ID)))
    {
    vendor = "fedora";
    }
 else if(!strncmp(relstring, WHITEBOX_ID, strlen(WHITEBOX_ID)))
    {
    vendor = "whitebox";
    }
 else if(!strncmp(relstring, SCIENTIFIC_SL_ID, strlen(SCIENTIFIC_SL_ID)))
    {
    vendor = "scientific";
    edition = "sl";
    }
 else if(!strncmp(relstring, SCIENTIFIC_CERN_ID, strlen(SCIENTIFIC_CERN_ID)))
    {
    vendor = "scientific";
    edition = "cern";
    }
 else if(!strncmp(relstring, CENTOS_ID, strlen(CENTOS_ID)))
    {
    vendor = "centos";
    }
 else
    {
    Verbose("Could not identify OS distro from %s\n", RH_REL_FILENAME);
    return 2;
    }
 
 /* Now, grok the release.  For AS, we neglect the AS at the end of the
  * numerical release because we already figured out that it *is* AS
  * from the infomation above.  We assume that all the strings will
  * have the word 'release' before the numerical release.
  */

  /* Convert relstring to lowercase so that vendors like
     Scientific Linux don't fall through the cracks.
   */

 for (i = 0; i < strlen(relstring); i++)
    {
    relstring[i] = tolower(relstring[i]);
    }
 
 release = strstr(relstring, RELEASE_FLAG);
 if(release == NULL)
    {
    Verbose("Could not find a numeric OS release in %s\n",
     RH_REL_FILENAME);
    return 2;
    }
 else
    {
    release += strlen(RELEASE_FLAG);
    if (sscanf(release, "%d.%d", &major, &minor) == 2)
       {
       sprintf(strmajor, "%d", major);
       sprintf(strminor, "%d", minor);
       }
    /* red hat 9 is *not* red hat 9.0. 
     * and same thing with RHEL AS 3
     */
    else if (sscanf(release, "%d", &major) == 1)
       {
       sprintf(strmajor, "%d", major);
       minor = -2;
       }
    }
 
 if (major != -1 && minor != -1 && (strcmp(vendor,"") != 0))
    {
    classbuf[0] = '\0';
    strcat(classbuf, vendor);
    AddClassToHeap(classbuf);
    strcat(classbuf, "_");
    
    if (strcmp(edition,"") != 0)
       {
       strcat(classbuf, edition);
       AddClassToHeap(classbuf);
       strcat(classbuf, "_");
       }
    
    strcat(classbuf, strmajor);
    AddClassToHeap(classbuf);
    if (minor != -2)
       {
       strcat(classbuf, "_");
       strcat(classbuf, strminor);
       AddClassToHeap(classbuf);
       }
    }
 return 0;
}

/******************************************************************/

int linux_suse_version(void)
{
#define SUSE_REL_FILENAME "/etc/SuSE-release"
/* Check if it's a SuSE Enterprise version (all in lowercase) */
#define SUSE_SLES8_ID "suse sles-8"
#define SUSE_SLES_ID  "suse linux enterprise server"
#define SUSE_RELEASE_FLAG "linux "

/* The full string read in from SuSE-release */
char relstring[CF_MAXVARSIZE];
char classbuf[CF_MAXVARSIZE];
char vbuf[CF_BUFSIZE];

/* Where the numerical release will be found */
char *release=NULL;

int i,version;
int major = -1;
char strmajor[CF_MAXVARSIZE];
int minor = -1;
char strminor[CF_MAXVARSIZE];

FILE *fp;

 /* Grab the first line from the file and then close it. */

if ((fp = fopen(SUSE_REL_FILENAME,"r")) == NULL)
   {
   return 1;
   }

fgets(relstring, sizeof(relstring), fp);
fclose(fp);
  
   /* Check if it's a SuSE Enterprise version  */

Verbose("Looking for SuSE enterprise info in \"%s\"\n",relstring);
 
 /* Convert relstring to lowercase to handle rename of SuSE to 
  * SUSE with SUSE 10.0. 
  */

for (i = 0; i < strlen(relstring); i++)
   {
   relstring[i] = tolower(relstring[i]);
   }

   /* Check if it's a SuSE Enterprise version (all in lowercase) */

if (!strncmp(relstring, SUSE_SLES8_ID, strlen(SUSE_SLES8_ID)))
   {
   classbuf[0] = '\0';
   strcat(classbuf, "SLES8");
   AddClassToHeap(classbuf);
   }
else
   {
   for (version = 9; version < 13; version++)
      {
      snprintf(vbuf,CF_BUFSIZE,"%s %d ",SUSE_SLES_ID,version);
      Debug("Checking for suse [%s]\n",vbuf);
      
      if (!strncmp(relstring, vbuf, strlen(vbuf)))
         {
         snprintf(classbuf,CF_MAXVARSIZE,"SLES%d",version);
         AddClassToHeap(classbuf);
         }
      }
   }
    
 /* Determine release version. We assume that the version follows
  * the string "SuSE Linux" or "SUSE LINUX".
  */

release = strstr(relstring, SUSE_RELEASE_FLAG);

if (release == NULL)
   {
   Verbose("Could not find a numeric OS release in %s\n",SUSE_REL_FILENAME);
   return 2;
   }
else
   {
   release += strlen(SUSE_RELEASE_FLAG);
   sscanf(release, "%d.%d", &major, &minor);
   sprintf(strmajor, "%d", major);
   sprintf(strminor, "%d", minor);
   }

if(major != -1 && minor != -1)
   {
   classbuf[0] = '\0';
   strcat(classbuf, "SuSE");
   AddClassToHeap(classbuf);
   strcat(classbuf, "_");
   strcat(classbuf, strmajor);
   AddClassToHeap(classbuf);
   strcat(classbuf, "_");
   strcat(classbuf, strminor);
   AddClassToHeap(classbuf);
   }

return 0;
}

/******************************************************************/

int debian_version(void) /* Andrew Stribblehill */
{
#define DEBIAN_VERSION_FILENAME "/etc/debian_version"
int major = -1; 
int release = -1;
char classname[CF_MAXVARSIZE] = "";
FILE *fp;

if ((fp = fopen(DEBIAN_VERSION_FILENAME,"r")) == NULL)
   {
   return 1;
   }

Verbose("Looking for Debian version...\n");
switch (fscanf(fp, "%d.%d", &major, &release))
    {
    case 2:
        Verbose("This appears to be a Debian %u.%u system.", major, release);
        snprintf(classname, CF_MAXVARSIZE, "debian_%u_%u", major, release);
        AddClassToHeap(classname);
        /* Fall-through */
    case 1:
        Verbose("This appears to be a Debian %u system.", major);
        snprintf(classname, CF_MAXVARSIZE, "debian_%u", major);
        AddClassToHeap(classname);
        break;
    case 0:
        Verbose("No Debian version number found.\n");
        fclose(fp);
        return 2;
    }
fclose(fp);
return 0;
}

/******************************************************************/

int linux_mandrake_version(void)
{

/* We are looking for one of the following strings... */
#define MANDRAKE_ID "Linux Mandrake"
#define MANDRAKE_REV_ID "Mandrake Linux"
#define MANDRAKE_10_1_ID "Mandrakelinux"

#define RELEASE_FLAG "release "
#define MANDRAKE_REL_FILENAME "/etc/mandrake-release"

FILE *fp;

/* The full string read in from mandrake-release */
char relstring[CF_MAXVARSIZE];
char classbuf[CF_MAXVARSIZE];

/* I've never seen Mandrake-Move or the other 'editions', so
   I'm not going to try and support them here.  Contributions welcome. */

/* Where the numerical release will be found */
char *release=NULL;
char *vendor=NULL;
int major = -1;
char strmajor[CF_MAXVARSIZE];
int minor = -1;
char strminor[CF_MAXVARSIZE];

/* Grab the first line from the file and then close it. */
 if ((fp = fopen(MANDRAKE_REL_FILENAME,"r")) == NULL)
    {
    return 1;
    }
 fgets(relstring, sizeof(relstring), fp);
 fclose(fp);

 Verbose("Looking for Mandrake linux info in \"%s\"\n",relstring);

  /* Older Mandrakes had the 'Mandrake Linux' string in reverse order */
 if(!strncmp(relstring, MANDRAKE_ID, strlen(MANDRAKE_ID)))
    {
    vendor = "mandrake";
    }
 else if(!strncmp(relstring, MANDRAKE_REV_ID, strlen(MANDRAKE_REV_ID)))
    {
    vendor = "mandrake";
    }

 else if(!strncmp(relstring, MANDRAKE_10_1_ID, strlen(MANDRAKE_10_1_ID)))
    {
    vendor = "mandrake";
    }
 else
    {
    Verbose("Could not identify OS distro from %s\n", MANDRAKE_REL_FILENAME);
    return 2;
    }

 /* Now, grok the release. We assume that all the strings will
  * have the word 'release' before the numerical release.
  */
 release = strstr(relstring, RELEASE_FLAG);
 if(release == NULL)
    {
    Verbose("Could not find a numeric OS release in %s\n",
     MANDRAKE_REL_FILENAME);
    return 2;
    }
 else
    {
    release += strlen(RELEASE_FLAG);
    if (sscanf(release, "%d.%d", &major, &minor) == 2)
       {
       sprintf(strmajor, "%d", major);
       sprintf(strminor, "%d", minor);
       }
    else
       {
       Verbose("Could not break down release version numbers in %s\n",
       MANDRAKE_REL_FILENAME);
       }
    }

 if (major != -1 && minor != -1 && strcmp(vendor, ""))
    {
    classbuf[0] = '\0';
    strcat(classbuf, vendor);
    AddClassToHeap(classbuf);
    strcat(classbuf, "_");
    strcat(classbuf, strmajor);
    AddClassToHeap(classbuf);
    if (minor != -2)
       {
       strcat(classbuf, "_");
       strcat(classbuf, strminor);
       AddClassToHeap(classbuf);
       }
    }
 return 0;
}

/******************************************************************/

static void * lsb_release(const char *command, const char *key)

{ char * info = NULL;
 FILE * fp;

snprintf(VBUFF, CF_BUFSIZE, "%s %s", command, key);
if ((fp = cfpopen(VBUFF, "r")) == NULL)
   {
   return NULL;
   }

if (ReadLine(VBUFF, CF_BUFSIZE, fp))
   {
   char * buffer = VBUFF;
   strsep(&buffer, ":");
   
   while((*buffer != '\0') && isspace(*buffer))
      {
      buffer++;
      }
   
   info = buffer;
   while((*buffer != '\0') && !isspace(*buffer))
      {
      *buffer = tolower(*buffer++);
      }
   
   *buffer = '\0';
   info = strdup(info);
   }

cfpclose(fp);
return info;
}


/******************************************************************/

int lsb_version(void)
{
#define LSB_RELEASE_COMMAND "lsb_release"

char classname[CF_MAXVARSIZE];
char *distrib  = NULL;
char *release  = NULL;
char *codename = NULL;
int major = 0;
int minor = 0;

char *path, *dir, *rest;
struct stat statbuf;

path = rest = strdup(getenv("PATH"));
for (; dir = strsep(&rest, ":") ;)
    {
    snprintf(VBUFF, CF_BUFSIZE, "%s/" LSB_RELEASE_COMMAND, dir);
    if (stat(VBUFF,&statbuf) != -1)
        {
        free(path);
        path = strdup(VBUFF);

        Verbose("\nThis appears to be a LSB compliant system.\n");
        AddClassToHeap("lsb_compliant");
        break;
        }
    }

if (!dir)
    {
    free(path);
    return 1;
    }

if ((distrib  = lsb_release(path, "--id")) != NULL)
    {
    snprintf(classname, CF_MAXVARSIZE, "%s", distrib);
    AddClassToHeap(classname);

    if ((codename = lsb_release(path, "--codename")) != NULL)
        {
        snprintf(classname, CF_MAXVARSIZE, "%s_%s", distrib, codename);
        AddClassToHeap(classname);
        }

    if ((release  = lsb_release(path, "--release")) != NULL)
        {
        switch (sscanf(release, "%d.%d\n", &major, &minor))
            {
            case 2:
                snprintf(classname, CF_MAXVARSIZE, "%s_%u_%u", distrib, major, minor);
                AddClassToHeap(classname);
            case 1:
                snprintf(classname, CF_MAXVARSIZE, "%s_%u", distrib, major);
                AddClassToHeap(classname);
            }
        }

    free(path);
    return 0;
    }
else
    {
    free(path);
    return 2;
    }
}

/******************************************************************/

int VM_version(void)

{ FILE *fp;
  char *sp,buffer[CF_BUFSIZE];
  struct stat statbuf;
  int len = 0;

if ((fp = fopen("/etc/issue","r")) == NULL)
   {
   return 1;   
   }

do
   {
   fgets(buffer,sizeof(buffer), fp);
   Chop(buffer);
   len = strlen(buffer);
   }
while (len == 0);

AddClassToHeap(CanonifyName(buffer));

for (sp = buffer+strlen(buffer)-1; sp > buffer; sp--)
   {
   if (*sp == ' ')
      {
      *sp = '\0';
      AddClassToHeap(CanonifyName(buffer));
      }
   }

fclose(fp);
return 0;
}
