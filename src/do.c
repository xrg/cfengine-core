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
/* File: do.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

void GetHomeInfo()

{ DIR *dirh;
  struct dirent *dirp;
  struct Item *ip;
  char path[bufsize],mountitem[bufsize];

if (!IsPrivileged())                            
   {
   Debug("Not root, so skipping GetHomeInfo()\n");
   return;
   }

if (!MountPathDefined())
   {
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
      snprintf(OUTPUT,bufsize*2,"INFO: Host %s seems to have no (additional) local disks except the OS\n",VDEFAULTBINSERVER.name);
      CfLog(cfverbose,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"      mounted under %s\n\n",ip->name);
      CfLog(cfverbose,OUTPUT,"");
      return;
      }

   for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,ip->name,NULL))
         {
         continue;
         }

      strcpy(VBUFF,ip->name);
      AddSlash(VBUFF);
      strcat(VBUFF,dirp->d_name);

      if (IsHomeDir(VBUFF))
         {
         snprintf(OUTPUT,bufsize*2,"Host defines a home directory %s\n",VBUFF);
	 CfLog(cfverbose,OUTPUT,"");
         }
      else
         {
         snprintf(OUTPUT,bufsize*2,"Host defines a potential mount point %s\n",VBUFF);
	 CfLog(cfverbose,OUTPUT,"");
         }

      snprintf(path,bufsize,"%s%s",ip->name,dirp->d_name);
      snprintf(mountitem,bufsize,"%s:%s",VDEFAULTBINSERVER.name,path);

      if (! IsItemIn(VMOUNTED,mountitem))
         {
         if ( MOUNTCHECK && ! RequiredFileSystemOkay(path) && VERBOSE)
            {
            snprintf(OUTPUT,bufsize*2,"Found a mountpoint %s but there was\n",path);
	    CfLog(cfinform,OUTPUT,"");
            CfLog(cfinform,"nothing mounted on it.\n\n","");
            }
         }
      }
   closedir(dirh);
   }
}

/*******************************************************************/

void GetMountInfo ()  /* This is, in fact, the most portable way to read the mount info! */
                      /* Depressing, isn't it? */
{ FILE *pp;
  char buf1[bufsize],buf2[bufsize],buf3[bufsize];
  char host[maxvarsize], mounton[bufsize];
  int i;

if (!GetLock(ASUniqueName("mountinfo"),"",1,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }

  /* sscanf(VMOUNTCOMM[VSYSTEMHARDCLASS],"%s",buf1); */
  /* Old BSD scanf crashes here! Why!? workaround: */

for (i=0; VMOUNTCOMM[VSYSTEMHARDCLASS][i] != ' '; i++)
   {
   buf1[i] =  VMOUNTCOMM[VSYSTEMHARDCLASS][i];
   }

buf1[i] = '\0';

signal(SIGALRM,(void *)TimeOut);
alarm(RPCTIMEOUT);

if ((pp = cfpopen(buf1,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"%s: Can't open %s\n",VPREFIX,buf1);
   CfLog(cferror,OUTPUT,"popen");
   return;
   }

do
   {
   VBUFF[0] = buf1[0] = buf2[0] = buf3[0] = '\0';

   if (ferror(pp))  /* abortable */
      {
      GOTMOUNTINFO = false;
      CfLog(cferror,"Error getting mount info\n","ferror");
      break;
      }
   
   ReadLine(VBUFF,bufsize,pp);

   if (ferror(pp))  /* abortable */
      {
      GOTMOUNTINFO = false;
      CfLog(cferror,"Error getting mount info\n","ferror");
      break;
      }
   
   sscanf(VBUFF,"%s%s%s",buf1,buf2,buf3);

   if (VBUFF[0] == '\n')
      {
      break;
      }

   if (strstr(VBUFF,"not responding"))
      {
      printf("%s: %s\n",VPREFIX,VBUFF);
      }

   if (strstr(VBUFF,"be root"))
      {
      CfLog(cferror,"Mount access is denied. You must be root.\n","");
      CfLog(cferror,"Use the -n option to run safely.","");
      }

   if (strstr(VBUFF,"retrying") || strstr(VBUFF,"denied") || strstr(VBUFF,"backgrounding"))
      {
      continue;
      }

   if (strstr(VBUFF,"exceeded") || strstr(VBUFF,"busy"))
      {
      continue;
      }

   if (strstr(VBUFF,"RPC"))
      {
      if (! SILENT)
         {
         CfLog(cfinform,"There was an RPC timeout. Aborting mount operations.\n","");
         CfLog(cfinform,"Session failed while trying to talk to remote host\n","");
         snprintf(OUTPUT,bufsize*2,"%s\n",VBUFF);
	 CfLog(cfinform,OUTPUT,"");
         }

      GOTMOUNTINFO = false;
      ReleaseCurrentLock();
      cfpclose(pp);
      return;
      }

   switch (VSYSTEMHARDCLASS)
      {
      case sun4:
      case sun3:
      case ultrx: 
      case irix:
      case irix4:
      case irix64:
      case linuxx:
      case GnU:
      case unix_sv:
      case freebsd:
      case netbsd:
      case openbsd:
      case bsd_i:
      case nextstep:
      case bsd4_3:
      case newsos:
      case aos:
      case osf:
      case crayos:
                    if (buf1[0] == '/')
                       {
                       strcpy(host,VDEFAULTBINSERVER.name);
                       strcpy(mounton,buf3);
                       }
                    else
                       {
                       sscanf(buf1,"%[^:]",host);
                       strcpy(mounton,buf3);
                       }

                    break;
      case solaris:
      case solarisx86:

      case hp:      
                    if (buf3[0] == '/')
                       {
                       strcpy(host,VDEFAULTBINSERVER.name);
                       strcpy(mounton,buf1);
                       }
                    else
                       {
                       sscanf(buf3,"%[^:]",host);
                       strcpy(mounton,buf1);
                       }

                    break;
      case aix:
                   /* skip header */

                    if (buf1[0] == '/')
                       {
                       strcpy(host,VDEFAULTBINSERVER.name);
                       strcpy(mounton,buf2);
                       }
                    else
                       {
                       strcpy(host,buf1);
                       strcpy(mounton,buf3);
                       }
                    break;

      case cfnt:    strcpy(mounton,buf2);
	            strcpy(host,buf1);
                    break;
      case unused1:
      case unused2:
      case unused3:
                    break;

      case cfsco: CfLog(cferror,"Don't understand SCO mount format, no data","");

      default:
                    printf("cfengine software error: case %d = %s\n",VSYSTEMHARDCLASS,CLASSTEXT[VSYSTEMHARDCLASS]);
                    FatalError("System error in GetMountInfo - no such class!");
      }

   InstallMountedItem(host,mounton);
   }

while (!feof(pp));

alarm(0);
signal(SIGALRM,SIG_DFL);
ReleaseCurrentLock();
cfpclose(pp);
}

/*******************************************************************/

void MakePaths()

{ struct File *ptr;
  struct Item *ip1,*ip2;
  char pathbuff[bufsize],basename[bufsize];
  
for (ptr = VMAKEPATH; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      ptr->done = 'y';
      }

   ResetOutputRoute(ptr->log,ptr->inform);
      
   if (strncmp(ptr->path,"home/",5) == 0) /* home/subdir */
      {
      if (*(ptr->path+4) != '/')
	 {
	 snprintf(OUTPUT,bufsize*2,"Illegal use of home in directories: %s\n",ptr->path);
	 CfLog(cferror,OUTPUT,"");
	 continue;
	 }
      
      for (ip1 = VHOMEPATLIST; ip1 != NULL; ip1=ip1->next)
	 {
	 for (ip2 = VMOUNTLIST; ip2 != NULL; ip2=ip2->next)
	    {
	    if (IsExcluded(ip2->classes))
	       {
	       continue;
	       }
	    
	    pathbuff[0]='\0';
	    basename[0]='\0';	    
	    
	    strcpy(pathbuff,ip2->name);
	    AddSlash(pathbuff);
	    strcat(pathbuff,ip1->name);
	    AddSlash(pathbuff);
	    strcat(pathbuff,"*/");   
	    strcat(pathbuff,ptr->path+5);
	    
	    ExpandWildCardsAndDo(pathbuff,basename,DirectoriesWrapper,ptr);	 
	    }
	 }
      }
   else
      {
      Verbose("MakePath(%s)\n",ptr->path);
      pathbuff[0]='\0';
      basename[0]='\0';	        

      ExpandWildCardsAndDo(ptr->path,basename,DirectoriesWrapper,ptr);
      }
   ResetOutputRoute('d','d');
   }
}

/*******************************************************************/

void MakeChildLinks()     /* <binserver> should expand to a best fit filesys */

{ struct Link *lp;
  struct Item *ip;
  int matched,varstring;
  char to[bufsize],from[bufsize],path[bufsize];
  struct stat statbuf;
  short saveenforce;
  short savesilent;

if (NOLINKS)
   {
   return;
   }

ACTION = links; 

for (lp = VCHLINK; lp != NULL; lp = lp->next)
   {
   if (IsExcluded(lp->classes))
      {
      continue;
      }

   if (lp->done == 'y' || strcmp(lp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      lp->done = 'y';
      }

   snprintf(VBUFF,bufsize,"%.50s.%.50s",lp->from,lp->to); /* Unique ID for copy locking */

   if (!GetLock(ASUniqueName("link"),CanonifyName(VBUFF),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   ExpandVarstring(lp->from,from,NULL); 
   ExpandVarstring(lp->to,to,NULL); 

   saveenforce = ENFORCELINKS;
   ENFORCELINKS = ENFORCELINKS || (lp->force == 'y');

   savesilent = SILENT;
   SILENT = SILENT || lp->silent;

   ResetOutputRoute(lp->log,lp->inform);
   
   matched = varstring = false;

   for(ip = VBINSERVERS; ip != NULL && (!matched); ip = ip->next)
      {
      path[0] = '\0';

      if (strcmp(to,"linkchildren") == 0)           /* linkchildren */
         {
         if (stat(from,&statbuf) == -1)
            {
            snprintf(OUTPUT,bufsize*2,"Makechildlinks() can't stat %s\n",from);
	    CfLog(cferror,OUTPUT,"stat");
	    ResetOutputRoute('d','d');
            continue;
            }
         LinkChildren(from,lp->type,&statbuf,0,0,lp->inclusions,lp->exclusions,lp->copy,lp->nofile,lp);
         break;
         }

      varstring = ExpandVarbinserv(to,path,ip->name);

      if (lp->recurse != 0)
	 {
	 matched = RecursiveLink(lp,from,path,lp->recurse);
         }
      else if (LinkChildFiles(from,path,lp->type,lp->inclusions,lp->exclusions,lp->copy,lp->nofile,lp))
         {
         matched = true;
         }
      else if (! varstring)
         {
         snprintf(OUTPUT,bufsize*2,"Error while trying to childlink %s -> %s\n",from,path);
	 CfLog(cferror,OUTPUT,"");
         snprintf(OUTPUT,bufsize*2,"The directory %s does not exist. Can't link.\n",path);
	 CfLog(cferror,OUTPUT,"");	 
         }

      if (! varstring)                       /* don't iterate over binservers if not var */
         {
	 ReleaseCurrentLock();
         break;
         }
      }

   ENFORCELINKS = saveenforce;
   SILENT = savesilent;
   ResetOutputRoute('d','d');

   if (matched == false && ip == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"ChildLink didn't find any server to match %s -> %s\n",from,to);
      CfLog(cferror,OUTPUT,"");
      }

   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void MakeLinks()     /* <binserver> should expand to a best fit filesys */

{ struct Link *lp;
  char from[bufsize],to[bufsize],path[bufsize];
  struct Item *ip;
  int matched,varstring;
  short saveenforce;
  short savesilent;
  int (*linkfiles) ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));

if (NOLINKS)
   {
   return;
   }

ACTION = links; 

for (lp = VLINK; lp != NULL; lp = lp->next)
   {
   if (IsExcluded(lp->classes))
      {
      continue;
      }

   if (lp->done == 'y' || strcmp(lp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      lp->done = 'y';
      }

   snprintf(VBUFF,bufsize,"%.50s.%.50s",lp->from,lp->to); /* Unique ID for copy locking */
   
   if (!GetLock(ASUniqueName("link"),CanonifyName(VBUFF),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   ExpandVarstring(lp->from,from,NULL);
   ExpandVarstring(lp->to,to,NULL); 

   ResetOutputRoute(lp->log,lp->inform);
   
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
                printf("%s: internal error, link type was [%c]\n",VPREFIX,lp->type);
		ReleaseCurrentLock();
                continue;
      }

   saveenforce = ENFORCELINKS;
   ENFORCELINKS = ENFORCELINKS || (lp->force == 'y');

   savesilent = SILENT;
   SILENT = SILENT || lp->silent;

   matched = varstring = false;

   for( ip = VBINSERVERS; ip != NULL && (!matched); ip = ip->next)
      {
      path[0] = '\0';

      varstring = ExpandVarbinserv(to,path,ip->name);

      if ((*linkfiles)(from,path,lp->inclusions,lp->exclusions,lp->copy,lp->nofile,lp))
         {
         matched = true;
         }
      else if (! varstring)
         {
         snprintf(OUTPUT,bufsize*2,"Error while trying to link %s -> %s\n",from,path);
	 CfLog(cfinform,OUTPUT,"");
         }

      if (! varstring)                       /* don't iterate over binservers if not var */
         {
         break;
         }
      }

   ENFORCELINKS = saveenforce;
   SILENT = savesilent;

   ResetOutputRoute('d','d');
   
   if (matched == false && ip == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Links didn't find any file to match %s -> %s\n",from,to);
      CfLog(cferror,OUTPUT,"");
      }
   
   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void MailCheck()

{ char mailserver[bufsize];
  char mailhost[maxvarsize];
  char rmailpath[maxvarsize];
  char lmailpath[maxvarsize];


if (!GetLock("Mailcheck",CanonifyName(VFSTAB[VSYSTEMHARDCLASS]),0,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }
  
if (VMAILSERVER[0] == '\0')
   {
   FatalError("Program does not define a mailserver for this host");
   }

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can alter the mail configuration.\n","");
   ReleaseCurrentLock();
   return;
   }

sscanf (VMAILSERVER,"%[^:]:%s",mailhost,rmailpath);

if (VMAILSERVER[0] == '\0')
   {
   CfLog(cfinform,"\n%s: Host has no defined mailserver!\n","");
   ReleaseCurrentLock();
   return;
   }

if (strcmp(VDEFAULTBINSERVER.name,mailhost) == 0) /* Is this the mailserver ?*/
   {
   ExpiredUserCheck(rmailpath,false);
   ReleaseCurrentLock();
   return;
   }

snprintf(lmailpath,bufsize,"%s:%s",mailhost,VMAILDIR[VSYSTEMHARDCLASS]);


if (IsItemIn(VMOUNTED,lmailpath))                             /* Remote file system mounted on */
   {                                                          /* local mail dir - correct      */
   Verbose("%s: Mail spool area looks ok\n",VPREFIX);
   ReleaseCurrentLock();
   return;
   }

strcpy(mailserver,VMAILDIR[VSYSTEMHARDCLASS]);
AddSlash(mailserver);
strcat(mailserver,".");

MakeDirectoriesFor(mailserver,'n');                                  /* Check directory is in place */

if (IsItemIn(VMOUNTED,VMAILSERVER))
   {
   if (!SILENT)
      {
      Verbose("%s: Warning - the mail directory seems to be mounted as on\n",VPREFIX);
      Verbose("%s: the remote mailserver and not on the correct local directory\n",VPREFIX);
      Verbose("%s: Should strictly mount on %s\n",VPREFIX,VMAILDIR[VSYSTEMHARDCLASS]);
      }
   ReleaseCurrentLock();
   return;
   }

if (MatchStringInFstab("mail"))
   {
   if (!SILENT)
      {
      Verbose("%s: Warning - the mail directory seems to be mounted\n",VPREFIX);
      Verbose("%s: in a funny way. I can find the string <mail> in %s\n",VPREFIX,VFSTAB[VSYSTEMHARDCLASS]);
      Verbose("%s: but nothing is mounted on %s\n\n",VPREFIX,VMAILDIR[VSYSTEMHARDCLASS]);
      }
   ReleaseCurrentLock();
   return;
   }

printf("\n%s: Trying to mount %s\n",VPREFIX,VMAILSERVER);

if (! DONTDO)
   {
   AddToFstab(mailhost,rmailpath,VMAILDIR[VSYSTEMHARDCLASS],"rw",NULL,false);
   }
else
   {
   printf("%s: Need to mount %s:%s on %s\n",VPREFIX,mailhost,rmailpath,mailserver);
   }

ReleaseCurrentLock(); 
}

/*******************************************************************/

void ExpiredUserCheck(spooldir,always)

char *spooldir;
int always;

{
Verbose("%s: Checking for expired users in %s\n",VPREFIX,spooldir); 

if (always || (strncmp(VMAILSERVER,VFQNAME,strlen(VMAILSERVER)) != 0))
   { DIR *dirh;
     struct dirent *dirp;
     struct stat statbuf;

   if ((dirh = opendir(spooldir)) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open spool directory %s",spooldir);
      CfLog(cfverbose,OUTPUT,"opendir");
      return;
      }

   for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,spooldir,NULL))
         {
         continue;
         }

      strcpy(VBUFF,spooldir);
      AddSlash(VBUFF);
      strcat(VBUFF,dirp->d_name);

      if (stat(VBUFF,&statbuf) != -1)
	 {
	 if (getpwuid(statbuf.st_uid) == NULL)
	    {
	    if (TrueVar("WarnNonOwnerMail")||TrueVar("WarnNonOwnerFiles"))
	       {
	       snprintf(OUTPUT,bufsize*2,"File %s in spool dir %s is not owned by any user",dirp->d_name,spooldir);
	       CfLog(cferror,OUTPUT,"");
	       }
	    
	    if (TrueVar("DeleteNonOwnerMail")||TrueVar("DeleteNonOwnerFiles"))
	       {
	       if (DONTDO)
		  {
		  printf("%s: Delete file %s\n",VPREFIX,VBUFF);
		  }
	       else
		  {
		  snprintf(OUTPUT,bufsize*2,"Deleting file %s in spool dir %s not owned by any user",dirp->d_name,spooldir);
		  CfLog(cferror,OUTPUT,"");

		  if (unlink(VBUFF) == -1)
		     {
		     CfLog(cferror,"","unlink");
		     }
		  }
	       }
	    }
	 }

      if (strstr(dirp->d_name,"lock") || strstr(dirp->d_name,".tmp"))
	 {
	 Verbose("Ignoring non-user file %s\n",dirp->d_name);
	 continue;
	 }

      if (getpwnam(dirp->d_name) == NULL)
	 {
	 if (TrueVar("WarnNonUserMail")||TrueVar("WarnNonUserFiles"))
	    {
	    snprintf(OUTPUT,bufsize*2,"File %s in spool dir %s is not the name of a user",dirp->d_name,spooldir);
	    CfLog(cferror,OUTPUT,"");
	    }
	 
	 if (TrueVar("DeleteNonUserMail")||TrueVar("DeleteNonUserFiles"))
	    {
	    if (DONTDO)
	       {
	       printf("%s: Delete file %s\n",VPREFIX,VBUFF);
	       }
	    else
	       {
	       snprintf(OUTPUT,bufsize*2,"Deleting file %s in spool dir %s (not a username)",dirp->d_name,spooldir);
	       CfLog(cferror,OUTPUT,"");
	       
	       if (unlink(VBUFF) == -1)
		  {
		  CfLog(cferror,"","unlink");
		  }	       
	       }
	    }	
	 }
      }
   closedir(dirh);
   Verbose("%s: Done checking spool directory %s\n",VPREFIX,spooldir);
   } 
}

/*******************************************************************/

void MountFileSystems()

{ FILE *pp;
  int fd;
  struct stat statbuf;

if (! GOTMOUNTINFO || DONTDO)
   {
   return;
   }

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can mount filesystems.\n","");
   return;
   }

if (VSYSTEMHARDCLASS == cfnt)
   {
   /* This is a shell script. Make sure it hasn't been compromised. */
   if (stat("/etc/fstab",&statbuf) == -1)
      {
      if ((fd = creat("/etc/fstab",0755)) > 0)
	 {
	 write(fd,"#!/bin/sh\n\n",10);
	 close(fd);
	 }
      else
	 {
	 if (statbuf.st_mode & (S_IWOTH | S_IWGRP))
	    {
	    CfLog(cferror,"File /etc/fstab was insecure. Cannot mount filesystems.\n","");
	    GOTMOUNTINFO = false;
	    return;
	    }
	 }
      }
   }

signal(SIGALRM,(void *)TimeOut);
alarm(RPCTIMEOUT);
 
if ((pp = cfpopen(VMOUNTCOMM[VSYSTEMHARDCLASS],"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Failed to open pipe from %s\n",VMOUNTCOMM[VSYSTEMHARDCLASS]);
   CfLog(cferror,OUTPUT,"popen");
   return;
   }

while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error mounting filesystems\n","ferror");
      break;
      }
   
   ReadLine(VBUFF,bufsize,pp);

   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error mounting filesystems\n","ferror");
      break;
      }

   if (strstr(VBUFF,"already mounted") || strstr(VBUFF,"exceeded") || strstr(VBUFF,"determined"))
      {
      continue;
      }

   if (strstr(VBUFF,"not supported"))
      {
      continue;
      }

   if (strstr(VBUFF,"denied") || strstr(VBUFF,"RPC"))
      {
      CfLog(cferror,"There was a mount error, trying to mount one of the filesystems on this host.\n","");
      snprintf(OUTPUT,bufsize*2,"%s\n",VBUFF);
      CfLog(cferror,OUTPUT,"");
      GOTMOUNTINFO = false;
      break;
      }

   if (strstr(VBUFF,"trying") && !strstr(VBUFF,"NFS version 2"))
      {
      CfLog(cferror,"Aborted because MountFileSystems() went into a retry loop.\n","");
      GOTMOUNTINFO = false;
      break;
      }
   }

alarm(0);
signal(SIGALRM,SIG_DFL);
cfpclose(pp);
}

/*******************************************************************/

void CheckRequired()

{ struct Disk *rp;
  struct Item *ip;
  int matched,varstring,missing = 0;
  char path[bufsize],expand[bufsize];

ACTION=required;

for (rp = VREQUIRED; rp != NULL; rp = rp->next)
   {
   if (IsExcluded(rp->classes))
      {
      continue;
      }

   if (rp->done == 'y' || strcmp(rp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      rp->done = 'y';
      }
   
   if (!GetLock(ASUniqueName("disks"),rp->name,VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }
 
   ResetOutputRoute(rp->log,rp->inform);
   matched = varstring = false;

   for(ip = VBINSERVERS; ip != NULL && (!matched); ip = ip->next)
      {
      path[0] = expand[0] = '\0';

      ExpandVarstring(rp->name,expand,NULL);
      varstring = ExpandVarbinserv(expand,path,ip->name);

      if (RequiredFileSystemOkay(path))  /* simple or reduced item */
         {
         Verbose("Filesystem %s looks sensible\n",path);
         matched = true;
	 
	 if (rp->freespace == -1)
	    {
	    AddMultipleClasses(rp->elsedef);
	    }
         }
      else if (! varstring)
         {
         snprintf(OUTPUT,bufsize*2,"The file %s does not exist or is suspicious.\n\n",path);
	 CfLog(cferror,OUTPUT,"");
	 
         /* Define the class if there was no freespace option. */
         if (rp->freespace == -1)
	    {
	    AddMultipleClasses(rp->define);
	    }
         }

      if (! varstring)                       /* don't iterate over binservers if not var */
         {
         break;
         }
      }

   if ((rp->freespace != -1))
     {
     /* HvB : Bas van der Vlies */
     if (!CheckFreeSpace(path,rp))
	{
	Verbose("Free space below %d, defining %s\n",rp->freespace, rp->define);
	AddMultipleClasses(rp->define);
	}
     else
	{
	Verbose("Free space above %d, defining %s\n",rp->freespace, rp->elsedef);
	AddMultipleClasses(rp->elsedef);
	}
     }

   if (matched == false && ip == NULL)
      {
      printf(" didn't find any file to match the required filesystem %s\n",rp->name);
      missing++;
      }

   ReleaseCurrentLock();
   }

if (missing)

   { time_t tloc;;

   if ((tloc = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n");
      }
   snprintf(OUTPUT,bufsize*2,"MESSAGE at %s\n\n",ctime(&tloc));
   CfLog(cferror,OUTPUT,"");
   snprintf(OUTPUT,bufsize*2,"There are %d required file(system)s missing on host <%s>\n",missing,VDEFAULTBINSERVER.name);
   CfLog(cferror,OUTPUT,"");   
   CfLog(cferror,"even after all mounts have been attempted.\n","");
   CfLog(cferror,"This may be caused by a failure to mount a network filesystem (check exports)\n","");
   snprintf(OUTPUT,bufsize*2,"or because no valid server was specified in the program %s\n\n",VINPUTFILE);
   CfLog(cferror,OUTPUT,"");

   ResetOutputRoute('d','d');
   }
}

/*******************************************************************/

void TidyFiles()

   /* Here we start by scanning for any absolute path wildcards */
   /* After that's finished, we go snooping around the homedirs */

{ char basename[bufsize],pathbuff[bufsize];
  struct TidyPattern *tlp;
  struct Tidy *tp;
  struct Item *ip1,*ip2;
  struct stat statbuf;
  int homesearch = 0;

Banner("Tidying Spool Directories");

for (ip1 = SPOOLDIRLIST; ip1 != NULL; ip1=ip1->next)
   {
   if (!IsExcluded(ip1->classes))
      {
      ExpiredUserCheck(ip1->name,true);
      }
   }
  
Banner("Tidying by directory");

for (tp = VTIDY; tp != NULL; tp=tp->next)
   {
   if (strncmp(tp->path,"home",4)==0)
      {
      for (tlp = tp->tidylist; tlp != NULL; tlp=tlp->next)
	 {
	 if (!IsExcluded(tlp->classes))
	    {
	    homesearch = 1;
	    break;
	    }
	 }
      continue;
      }

   for (tlp = tp->tidylist; tlp != NULL; tlp=tlp->next)
      {
      if (IsExcluded(tlp->classes))
         {
         continue;
         }

      Verbose("Directory %s\n",tp->path);
      strcpy(VBUFF,tp->path);
      AddSlash(VBUFF);
      strcat(VBUFF,tlp->pattern);

      if (stat(VBUFF,&statbuf) != -1)   /* not lstat - could confuse user */
         {
         if (S_ISDIR(statbuf.st_mode))
            {
	    if (! tlp->rmdirs)          /* Are we allowed to rm empty dirs? */
	       {
               Verbose("%s: will not delete directories (matching %s)!\n",VPREFIX,tlp->pattern);
	       Verbose("%s: Applies to %s\n",VPREFIX,VBUFF);
               DeleteTidyList(tp->tidylist);
	       tp->tidylist = NULL;
               continue;
	       }
            }
         }
      }

   basename[0] = '\0';

   ExpandWildCardsAndDo(tp->path,basename,TidyWrapper,tp);
   
   DeleteTidyList(tp->tidylist);
   tp->tidylist = NULL;
   }


Debug2("End PATHTIDY:\n");

if (!homesearch)                           /* If there are "home" wildcards */
   {                                 /* Don't go rummaging around the disks */
   Verbose("No home patterns to search\n");
   return;
   }

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can delete others' files.\n","");
   return;
   }


Banner("Tidying home directories");

if (!MountPathDefined())
   {
   return;
   }

for (ip1 = VHOMEPATLIST; ip1 != NULL; ip1=ip1->next)
   {
   for (ip2 = VMOUNTLIST; ip2 != NULL; ip2=ip2->next)
      {
      if (IsExcluded(ip2->classes))
	 {
	 continue;
	 }
      pathbuff[0]='\0';
      basename[0]='\0';
      strcpy(pathbuff,ip2->name);
      AddSlash(pathbuff);
      strcat(pathbuff,ip1->name);

      ExpandWildCardsAndDo(pathbuff,basename,RecHomeTidyWrapper,NULL);
      }
   }

Verbose("Done with home directories\n");
}

/*******************************************************************/

void Scripts()

{ struct ShellComm *ptr;
  char line[bufsize];
  char comm[20], *sp;
  char execstr[bufsize];
  int print;
  mode_t maskval = 0;
  FILE *pp;
  int preview = false;

for (ptr = VSCRIPT; ptr != NULL; ptr=ptr->next)
   {
   preview = (ptr->preview == 'y');
   
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      ptr->done = 'y';
      }

   ResetOutputRoute(ptr->log,ptr->inform);
   
   if (!GetLock(ASUniqueName("shellcommand"),CanonifyName(ptr->name),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   bzero(execstr,bufsize);
   ExpandVarstring(ptr->name,execstr,NULL);
   
   snprintf(OUTPUT,bufsize*2,"Executing script %s...(timeout=%d,uid=%d,gid=%d)\n",execstr,ptr->timeout,ptr->uid,ptr->gid);
   CfLog(cfinform,OUTPUT,"");
   
   if (DONTDO && preview != 'y')
      {
      printf("%s: execute script %s\n",VPREFIX,execstr);
      }
   else
      {
      for (sp = execstr; *sp != ' ' && *sp != '\0'; sp++)
	 {
	 }

      if (sp - 10 >= execstr)
	 {
	 sp -= 10;   /* copy 15 most relevant characters of command */
	 }
      else
	 {
	 sp = execstr;
	 }

      bzero(comm,20);
      strncpy(comm,sp,15);

      if (ptr->timeout != 0)
	 {
         signal(SIGALRM,(void *)TimeOut);
         alarm(ptr->timeout);
         }

      Verbose("(Setting umask to %o)\n",ptr->umask);
      maskval = umask(ptr->umask);

      if (ptr->umask == 0)
	 {
	 snprintf(OUTPUT,bufsize*2,"Programming %s running with umask 0! Use umask= to set\n",execstr);
	 CfLog(cfsilent,OUTPUT,"");
	 }
      
      switch (ptr->useshell)
	 {
	 case 'y':  pp = cfpopen_shsetuid(execstr,"r",ptr->uid,ptr->gid,ptr->chdir,ptr->chroot);
	            break;
	 default:   pp = cfpopensetuid(execstr,"r",ptr->uid,ptr->gid,ptr->chdir,ptr->chroot);
	            break;	     
	 }

      if (pp == NULL)
	 {
	 snprintf(OUTPUT,bufsize*2,"Couldn't open pipe to command %s\n",execstr);
	 CfLog(cferror,OUTPUT,"popen");
	 ResetOutputRoute('d','d');
	 ReleaseCurrentLock();
	 continue;
	 } 
      
      while (!feof(pp))
	 {
	 if (ferror(pp))  /* abortable */
	    {
	    snprintf(OUTPUT,bufsize*2,"Shell command pipe %s\n",execstr);
	    CfLog(cferror,OUTPUT,"ferror");
	    break;
	    }
	 
	 ReadLine(line,bufsize,pp);
	 
	 if (strstr(line,"cfengine-die"))
	    {
	    break;
	    }
	 
	 if (ferror(pp))  /* abortable */
	    {
	    snprintf(OUTPUT,bufsize*2,"Shell command pipe %s\n",execstr);
	    CfLog(cferror,OUTPUT,"ferror");
	    break;
	    }
	 
	 if (preview == 'y')
	    {
	    /*
	     * Preview script - try to parse line as log message. If line does
	     * not parse, then log as error.
	     */
	    
	    int i;
	    int level = cferror;
	    char *message = line;
	    
	    /*
	     * Table matching cfoutputlevel enums to log prefixes.
	     */
	    
	    char *prefixes[] =
		{
		    ":silent:",
		    ":inform:",
		    ":verbose:",
		    ":editverbose:",
		    ":error:",
		    ":logonly:",
		};
	    int precount = sizeof(prefixes)/sizeof(char *);
	    
	    if (line[0] == ':')
	       {
	       /*
	        * Line begins with colon - see if it matches a log prefix.
		*/
	       
	       for (i=0; i<precount; i++)
		  {
		  int prelen = 0;
		  prelen = strlen(prefixes[i]);
		  if (strncmp(line, prefixes[i], prelen) == 0)
		     {
		     /*
		      * Found log prefix - set logging level, and remove the
		      * prefix from the log message.
		      */
		     level = i;
		     message += prelen;
		     break;
		     }
		  }
	       }

	    snprintf(OUTPUT,bufsize,"%s (preview of %s)\n",message,comm);
	    CfLog(level,OUTPUT,"");
	    }
	 else 
	    {
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
	       printf("%s:%s: %s\n",VPREFIX,comm,line);
	       }
	    }
	 }
      
      cfpclose_def(pp,ptr->defines,ptr->elsedef);
      }

   if (ptr->timeout != 0)
      {
      alarm(0);
      signal(SIGALRM,SIG_DFL);
      }

   umask(maskval);
   
   snprintf(OUTPUT,bufsize*2,"Finished script %s\n",execstr);
   CfLog(cfinform,OUTPUT,"");

   ResetOutputRoute('d','d');
   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void GetSetuidLog()

{ struct Item *filetop = NULL;
  struct Item *ip;
  FILE *fp;
  char *sp;

if (!IsPrivileged())                     /* Ignore this if not root */
   {
   return;
   }

if ((fp = fopen(VSETUIDLOG,"r")) == NULL)
   {
   }
else
   {
   while (!feof(fp))
      {
      ReadLine(VBUFF,bufsize,fp);

      if (strlen(VBUFF) == 0)
         {
         continue;
         }

      if ((ip = (struct Item *)malloc (sizeof(struct Item))) == NULL)
         {
         perror("malloc");
         FatalError("GetSetuidList() couldn't allocate memory #1");
         }

      if ((sp = malloc(strlen(VBUFF)+2)) == NULL)
         {
         perror("malloc");
         FatalError("GetSetuidList() couldn't allocate memory #2");
         }

      if (filetop == NULL)
         {
         VSETUIDLIST = filetop = ip;
         }
      else
         {
         filetop->next = ip;
         }

      Debug2("SETUID-LOG: %s\n",VBUFF);

      strcpy(sp,VBUFF);
      ip->name = sp;
      ip->next = NULL;
      filetop = ip;
      }

   fclose(fp);
   }

}

/*******************************************************************/

void CheckFiles()                         /* Check through file systems */

{ struct File *ptr;
  char buffer[bufsize];
  short savetravlinks = TRAVLINKS;
  short savekilloldlinks = KILLOLDLINKS;

if (TRAVLINKS && (VERBOSE || DEBUG || D2))
   {
   printf("(Default in switched to purge stale links...)\n");
   }

for (ptr = VFILE; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      ptr->done = 'y';
      }
   
   TRAVLINKS = savetravlinks;

   if (ptr->travlinks == 'T')
      {
      TRAVLINKS = true;
      }
   else if (ptr->travlinks == 'F')
      {
      TRAVLINKS = false;
      }
   else if (ptr->travlinks == 'K')
      {
      KILLOLDLINKS = true;
      }

   ResetOutputRoute(ptr->log,ptr->inform);

   if (strncmp(ptr->path,"home",4) == 0)
      {
      CheckHome(ptr);
      continue;
      }

   Verbose("Checking file(s) in %s\n",ptr->path);

   buffer[0] = '\0';

   ExpandWildCardsAndDo(ptr->path,buffer,CheckFileWrapper,ptr);

   ResetOutputRoute('d','d');
   TRAVLINKS = savetravlinks;
   KILLOLDLINKS = savekilloldlinks;
   }
}

/*******************************************************************/

void SaveSetuidLog()

{ FILE *fp;
  struct Item *ip;


if (!IsPrivileged())                     /* Ignore this if not root */
   {
   return;
   }

if (! DONTDO)
   {
   if ((fp = fopen(VSETUIDLOG,"w")) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open %s for writing\n",VSETUIDLOG);
      CfLog(cferror,OUTPUT,"fopen");
      return;
      }

   Verbose("Saving the setuid log in %s\n",VSETUIDLOG);

   for (ip = VSETUIDLIST; ip != NULL; ip=ip->next)
      {
      if (!isspace((int)*(ip->name)) && strlen(ip->name) != 0)
         {                         
         fprintf(fp,"%s\n",ip->name);
         Debug2("SAVE-SETUID-LOG: %s\n",ip->name);
         }
      }

   fclose(fp);
   chmod(VSETUIDLOG,0600);
   }
}

/*******************************************************************/

void DisableFiles()

{ struct Disable *dp;
  struct stat statbuf;
  char workname[bufsize],path[bufsize];
  
for (dp = VDISABLELIST; dp != NULL; dp=dp->next)
   {
   if (IsExcluded(dp->classes))
      {
      continue;
      }

   if (dp->done == 'y' || strcmp(dp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      dp->done = 'y';
      }

   if (!GetLock(ASUniqueName("disable"),CanonifyName(dp->name),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   ExpandVarstring(dp->name,workname,NULL);
   
   ResetOutputRoute(dp->log,dp->inform);
   
   if (lstat(workname,&statbuf) == -1)
      {
      Verbose("Filetype %s, %s is not there - ok\n",dp->type,workname);
      AddMultipleClasses(dp->elsedef);
      ReleaseCurrentLock();
      continue;
      }

   if (S_ISDIR(statbuf.st_mode))
      {
      if ((strcmp(dp->type,"file") != 0) && (strcmp(dp->type,"link") != 0))
         {
         snprintf(OUTPUT,bufsize*2,"Warning %s is a directory.\n",workname);
         CfLog(cferror,OUTPUT,"");
         CfLog(cferror,"I refuse to rename/delete a directory!\n\n","");
         }
      else
         {
         Verbose("Filetype %s, %s is not there - ok\n",dp->type,workname);
         }
      ResetOutputRoute('d','d');
      ReleaseCurrentLock();
      continue;
      }

   Verbose("Disable checking %s\n",workname);

   if (S_ISLNK(statbuf.st_mode))
      {
      if (strcmp(dp->type,"file") == 0)
         {
         Verbose("%s: %s is a link, not disabling\n",VPREFIX,workname);
	 ResetOutputRoute('d','d');
	 ReleaseCurrentLock();
         continue;
         }

      bzero(VBUFF,bufsize);
      
      if (readlink(workname,VBUFF,bufsize-1) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"DisableFiles() can't read link %s\n",workname);
	 CfLog(cferror,OUTPUT,"readlink");
	 ResetOutputRoute('d','d');
	 ReleaseCurrentLock();
         continue;
         }

      if (dp->action == 'd')
	 {
	 printf("%s: Deleting link %s -> %s\n",VPREFIX,workname,VBUFF);
	 
	 if (! DONTDO)
	    {
	    if (unlink(workname) == -1)
	       {
	       snprintf(OUTPUT,bufsize*2,"Error while unlinking %s\n",workname);
	       CfLog(cferror,OUTPUT,"unlink");
	       ResetOutputRoute('d','d');
	       ReleaseCurrentLock();
	       continue;
	       }
	    
	    AddMultipleClasses(dp->defines);
	    }
	 }
      else
	 {
	 snprintf(OUTPUT,bufsize,"Warning - file %s exists\n",workname);
	 CfLog(cferror,OUTPUT,"");
	 }
      }
   else
      {
      if (! S_ISREG(statbuf.st_mode))
         {
         Verbose("%s: %s is not a plain file - won't disable\n",VPREFIX,workname);
	 ResetOutputRoute('d','d');
	 ReleaseCurrentLock();
         continue;
         }

      if (strcmp(dp->type,"link") == 0)
         {
         Verbose("%s: %s is a file, not disabling\n",VPREFIX,workname);
	 ResetOutputRoute('d','d');
	 ReleaseCurrentLock();
         continue;
         }

      if (stat(workname,&statbuf) == -1)
         {
         CfLog(cferror,"Internal; error in Disable\n","");
	 ResetOutputRoute('d','d');
	 ReleaseCurrentLock();
	 return;
         }

      if (dp->size != cfnosize)
	 {
         switch (dp->comp)
	    {
            case '<':  if (statbuf.st_size < dp->size)
	                  {
		          Verbose("cfengine %s is smaller than %d bytes\n",workname,dp->size);
		          break;
	                  }
	               Verbose("Size is okay\n");
		       ResetOutputRoute('d','d');
		       ReleaseCurrentLock();
	               continue;
		    
   	    case '=':  if (statbuf.st_size == dp->size)
	                  {
		          Verbose("cfengine %s is equal to %d bytes\n",workname,dp->size);
		          break;
	                  }
	                Verbose("Size is okay\n");
			ResetOutputRoute('d','d');
			ReleaseCurrentLock();
			continue;
		       
	    default: if (statbuf.st_size > dp->size)
	                  {
		          Verbose("cfengine %s is larger than %d bytes\n",workname,dp->size);
		          break;
	                  }
	               Verbose("Size is okay\n");
		       ResetOutputRoute('d','d');
		       ReleaseCurrentLock();
	               continue;
	    }
	 }

      if (dp->rotate == 0)
         {
         strcpy(path,workname);
         strcat(path,".cfdisabled");

         snprintf(OUTPUT,bufsize*2,"Disabling file %s\n",workname);
         CfLog(cfinform,OUTPUT,"");
	 
         if (! DONTDO)
            {
	    chmod(workname, (mode_t)0600);

	    if (! IsItemIn(VREPOSLIST,path))
	       {
	       if (dp->action == 'd')
		  {
		  if (rename(workname,path) == -1)
		     {
		     snprintf(OUTPUT,bufsize*2,"Error occurred while renaming %s\n",workname);
		     CfLog(cferror,OUTPUT,"rename");
		     ResetOutputRoute('d','d');
		     ReleaseCurrentLock();
		     continue;
		     }

		  if (Repository(path,dp->repository))
		     {
		     unlink(path);
		     }
		  
		  AddMultipleClasses(dp->defines);
		  }
	       else
		  {
		  snprintf(OUTPUT,bufsize,"Warning - file %s exists (need to disable)",workname);
		  CfLog(cferror,OUTPUT,"");
		  }	       
	       }
            }
         }
      else if (dp->rotate == CF_TRUNCATE)
	 {
         snprintf(OUTPUT,bufsize*2,"Truncating (emptying) %s\n",workname);
	 CfLog(cfinform,OUTPUT,"");

	 if (dp->action == 'd')
	    {
	    if (! DONTDO)
	       {
	       TruncateFile(workname);
	       AddMultipleClasses(dp->defines);
	       }
	    }
	 else
	    {
	    snprintf(OUTPUT,bufsize,"File %s needs emptying",workname);
	    CfLog(cferror,OUTPUT,"");
	    }
	 }
      else
	 {
	 snprintf(OUTPUT,bufsize*2,"Rotating files %s by %d\n",workname,dp->rotate);
	 CfLog(cfinform,OUTPUT,"");
	 
	 if (dp->action == 'd')
	    {
	    if (!DONTDO)
	       {
	       RotateFiles(workname,dp->rotate);
	       AddMultipleClasses(dp->defines);
	       }
	    else	       
	       {
	       snprintf(OUTPUT,bufsize,"File %s needs rotating/emptying",workname);
	       CfLog(cferror,OUTPUT,"");
	       }
	    }
	 }
      }
   ResetOutputRoute('d','d');
   ReleaseCurrentLock();
   }
}


/*******************************************************************/

void MountHomeBinServers()

{ struct Mountables *mp;
  char host[maxvarsize];
  char mountdir[bufsize];
  char maketo[bufsize];

  /*
   * HvB: Bas van der Vlies
  */
  char mountmode[bufsize];

if (! GOTMOUNTINFO)
   {
   CfLog(cfinform,"Incomplete mount info due to RPC failure.\n","");
   snprintf(OUTPUT,bufsize*2,"%s will not be modified on this pass!\n\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can mount filesystems.\n","");
   return;
   }

Banner("Checking home and binservers");

for (mp = VMOUNTABLES; mp != NULL; mp=mp->next)
   {
   sscanf(mp->filesystem,"%[^:]:%s",host,mountdir);

   if (mp->done == 'y' || strcmp(mp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      mp->done = 'y';
      }
   
   Debug("Mount: checking %s\n",mp->filesystem);

   strcpy(maketo,mountdir);

   if (maketo[strlen(maketo)-1] == '/')
      {
      strcat(maketo,".");
      }
   else
      {
      strcat(maketo,"/.");
      }

   if (strcmp(host,VDEFAULTBINSERVER.name) == 0) /* A host never mounts itself nfs */
      {
      Debug("Skipping host %s\n",host);
      continue;
      }

   /* HvB: Bas van der Vlies */
   if ( mp->readonly ) 
      {
      strcpy(mountmode, "ro");
      }
   else
      {
      strcpy(mountmode, "rw");
      }

   if (IsHomeDir(mountdir))
      {
      if (!IsItemIn(VMOUNTED,mp->filesystem) && IsClassedItemIn(VHOMESERVERS,host))
         {
         MakeDirectoriesFor(maketo,'n');
         AddToFstab(host,mountdir,mountdir,mountmode,mp->mountopts,false);
         }
      else if (IsClassedItemIn(VHOMESERVERS,host))
	 {
	 AddToFstab(host,mountdir,mountdir,mountmode,mp->mountopts,true);
	 }
      }
   else
      {
      if (!IsItemIn(VMOUNTED,mp->filesystem) && IsClassedItemIn(VBINSERVERS,host))
         {
         MakeDirectoriesFor(maketo,'n');
         AddToFstab(host,mountdir,mountdir,mountmode,mp->mountopts,false);
         }
      else if (IsClassedItemIn(VBINSERVERS,host))
         {
	 AddToFstab(host,mountdir,mountdir,mountmode,mp->mountopts,true);
         }
      }
   }
}


/*********************************************************************/

void MountMisc()

{ struct MiscMount *mp;
  char host[maxvarsize];
  char mountdir[bufsize];
  char maketo[bufsize];
  char mtpt[bufsize];

if (! GOTMOUNTINFO)
   {
   CfLog(cfinform,"Incomplete mount info due to RPC failure.\n","");
   snprintf(OUTPUT,bufsize*2,"%s will not be modified on this pass!\n\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cfinform,OUTPUT,"");
   return;
   }
 
if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can mount filesystems.\n","");
   return;
   }

Banner("Checking miscellaneous mountables:");

for (mp = VMISCMOUNT; mp != NULL; mp=mp->next)
   {
   sscanf(mp->from,"%[^:]:%s",host,mountdir);

   if (mp->done == 'y' || strcmp(mp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      mp->done = 'y';
      }
   
   strcpy(maketo,mp->onto);

   if (maketo[strlen(maketo)-1] == '/')
      {
      strcat(maketo,".");
      }
   else
      {
      strcat(maketo,"/.");
      }

   if (strcmp(host,VDEFAULTBINSERVER.name) == 0) /* A host never mounts itself nfs */
      {
      continue;
      }

   snprintf(mtpt,bufsize,"%s:%s",host,mp->onto);
   
   if (!IsItemIn(VMOUNTED,mtpt))
      {
      MakeDirectoriesFor(maketo,'n');
      AddToFstab(host,mountdir,mp->onto,mp->options,NULL,false);
      }
   else
      {
      AddToFstab(host,mountdir,mp->onto,mp->options,NULL,true);
      }
   }
}

/*********************************************************************/

void Unmount()

{ struct UnMount *ptr;
  char comm[bufsize];
  char fs[bufsize];
  struct Item *filelist, *item;
  struct stat statbuf;
  FILE *pp;

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can unmount filesystems.\n","");
   return;
   }

filelist = NULL;

if (! LoadItemList(&filelist,VFSTAB[VSYSTEMHARDCLASS]))
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open %s!\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cferror,OUTPUT,"");
   return;
   }

NUMBEROFEDITS = 0;

for (ptr=VUNMOUNT; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }
   
   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      ptr->done = 'y';
      }

   if (!GetLock(ASUniqueName("unmount"),CanonifyName(ptr->name),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   fs[0] = '\0';

   sscanf(ptr->name,"%*[^:]:%s",fs);

   if (strlen(fs) == 0)
      {
      ReleaseCurrentLock();
      continue;
      }
   
   snprintf(OUTPUT,bufsize*2,"Unmount filesystem %s on %s\n",fs,ptr->name);
   CfLog(cfverbose,OUTPUT,"");

   if (strcmp(fs,"/") == 0 || strcmp(fs,"/usr") == 0)
      {
      CfLog(cfinform,"Request to unmount / or /usr is refused!\n","");
      ReleaseCurrentLock();     
      continue;
      }

   if (IsItemIn(VMOUNTED,ptr->name) && (! DONTDO))
      {
      snprintf(comm,bufsize,"%s %s",VUNMOUNTCOMM[VSYSTEMHARDCLASS],fs);

      if ((pp = cfpopen(comm,"r")) == NULL)
         {
         snprintf(OUTPUT,bufsize*2,"Failed to open pipe from %s\n",VUNMOUNTCOMM[VSYSTEMHARDCLASS]);
	 CfLog(cferror,OUTPUT,"");
	 ReleaseCurrentLock();     
         return;
         }

      ReadLine(VBUFF,bufsize,pp);

      if (strstr(VBUFF,"busy") || strstr(VBUFF,"Busy"))
         {
         snprintf(OUTPUT,bufsize*2,"umount warned that the device under %s\n",ptr->name);
	 CfLog(cfinform,OUTPUT,"");
         CfLog(cfinform,"was busy. Cannot unmount that device.\n","");
	 /* don't delete the mount dir when unmount's failed */
	 ptr->deletedir = 'n';
	 }
      else
	 {
	 snprintf(OUTPUT,bufsize*2,"Unmounting %s\n",ptr->name);
	 CfLog(cfinform,OUTPUT,"");
	 DeleteItemStarting(&VMOUNTED,ptr->name);  /* update mount record */
         }

      cfpclose(pp);
      }

   if (ptr->deletedir == 'y')
      {
      if (stat(fs,&statbuf) != -1)
	 {
	 if ( ! S_ISDIR(statbuf.st_mode))
	    {
	    snprintf(OUTPUT,bufsize*2,"Warning! %s was not a directory.\n",fs);
	    CfLog(cfinform,OUTPUT,"");
	    CfLog(cfinform,"(Unmount) will not delete this!\n","");
	    KillOldLink(fs,NULL);
	    }
	 else if (! DONTDO)
	    {
	    if (rmdir(fs) == -1)
	       {
	       snprintf(OUTPUT,bufsize*2,"Unable to remove the directory %s\n",fs);
	       CfLog(cferror,OUTPUT,"rmdir");
	       } 
	    else
	       {
	       snprintf(OUTPUT,bufsize*2,"Removing directory %s\n",ptr->name);
	       CfLog(cfinform,OUTPUT,"");
	       }
	    }
	 }
      }
      
   if (ptr->deletefstab == 'y')
      {
      if (VSYSTEMHARDCLASS == aix)
	 {
	 strcpy (VBUFF,fs);
	 strcat (VBUFF,":");
	 
	 item = LocateNextItemContaining(filelist,VBUFF);

	 if (item == NULL || item->next == NULL)
	    {
	    snprintf(OUTPUT,bufsize*2,"Bad format in %s\n",VFSTAB[aix]);
	    CfLog(cferror,OUTPUT,"");
	    continue;
	    }
	 
	 DeleteItem(&filelist,item->next);
	 
	 while (strstr(item->next->name,"="))
	    {
	    DeleteItem(&filelist,item->next);    /* DeleteItem(NULL) is harmless */
	    }
	 }
      else
	 {
	 Debug("Trying to delete filesystem %s from list\n",ptr->name);
	 	 
	 if (VSYSTEMHARDCLASS == ultrx)   /* ensure name is not just a substring */
	    {
	    strcpy (VBUFF,ptr->name);
	    strcat (VBUFF,":");
	    DeleteItemContaining(&filelist,VBUFF);
	    }
	 else
	    {
         switch (VSYSTEMHARDCLASS)
           {
           case unix_sv:
           case solarisx86:
           case solaris:
             /* find fs in proper context ("<host>:<remotepath> <-> <fs> ") */
             snprintf(VBUFF,bufsize,"[^:]+:[^ \t]+[ \t]+[^ \t]+[ \t]+%s[ \t]",fs);
             break;
           default:
             /* find fs in proper context ("<host>:<remotepath> <fs> ") */
             snprintf(VBUFF,bufsize,"[^:]+:[^ \t]+[ \t]+%s[ \t]",fs);
             break;
           }
	    item = LocateItemContainingRegExp(filelist,VBUFF);
	    DeleteItem(&filelist,item);
	    }
	 }
      }

   ReleaseCurrentLock();     
   }

if ((! DONTDO) && (NUMBEROFEDITS > 0))
   {
   SaveItemList(filelist,VFSTAB[VSYSTEMHARDCLASS],VREPOSITORY);
   }

DeleteItemList(filelist);
}

/*********************************************************************/

void EditFiles()

{ struct Edit *ptr;
  struct stat statbuf;

Debug("Editfile()\n");
  
for (ptr=VEDITLIST; ptr!=NULL; ptr=ptr->next)
   {
   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID))
      {
      continue;
      }

   if (strncmp(ptr->fname,"home",4) == 0)
      {
      DoEditHomeFiles(ptr);
      }
   else
      {
      if (lstat(ptr->fname,&statbuf) != -1)
	 {
	 if (S_ISDIR(statbuf.st_mode))
	    {
	    DoRecursiveEditFiles(ptr->fname,ptr->recurse,ptr,&statbuf);
	    }
	 else
	    {
	    WrapDoEditFile(ptr,ptr->fname);	 	    
	    }
	 }
      else
	 {
	 DoEditFile(ptr,ptr->fname);	 
	 }
      }
   }

EDITVERBOSE = false;
}

/*******************************************************************/

void CheckResolv()

{ struct Item *filebase = NULL, *referencefile = NULL;
  struct Item *ip;
  char ch;
  int fd, existed = true;

Verbose("Checking config in %s\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
  
if (VDOMAIN == NULL)
   {
   CfLog(cferror,"Domain name not specified. Can't configure resolver\n","");
   return;
   }

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can configure the resolver.\n","");
   return;
   }

if (! LoadItemList(&referencefile,VRESOLVCONF[VSYSTEMHARDCLASS]))
   {
   snprintf(OUTPUT,bufsize*2,"Trying to create %s\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
   CfLog(cfinform,OUTPUT,"");
   existed = false;
   
   if ((fd = creat(VRESOLVCONF[VSYSTEMHARDCLASS],0644)) == -1)
      {
      snprintf(OUTPUT,bufsize*2,"Unable to create file %s\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
      CfLog(cferror,OUTPUT,"creat");
      return;
      }
   else
      {
      close(fd);
      }
   }

if (existed)
   {
   LoadItemList(&filebase,VRESOLVCONF[VSYSTEMHARDCLASS]);
   }

for (ip = filebase; ip != NULL; ip=ip->next)
   {
   if (strlen(ip->name) == 0)
      {
      continue;
      }

   ch = *(ip->name+strlen(ip->name)-2);

   if (isspace((int)ch))
      {
      snprintf(OUTPUT,bufsize*2,"Deleted line %s ended with a space in %s.\n",ip->name,VRESOLVCONF[VSYSTEMHARDCLASS]);
      CfLog(cfinform,OUTPUT,"");
      CfLog(cfinform,"The resolver doesn't understand this.\n","");
      DeleteItem(&filebase,ip);
      }
   else if (isspace((int)*(ip->name)))
      {
      snprintf(OUTPUT,bufsize*2,"Deleted line %s started with a space in %s.\n",ip->name,VRESOLVCONF[VSYSTEMHARDCLASS]);
      CfLog(cfinform,OUTPUT,"");
      CfLog(cfinform,"The resolver doesn't understand this.\n","");
      DeleteItem(&filebase,ip);
      }
   }

while(DeleteItemStarting(&filebase,"search"))
   {
   }
 
snprintf(VBUFF,bufsize,"search %s",ToLowerStr(VDOMAIN));

/* if (IsItemIn(filebase,VBUFF))
   {
   DeleteItemStarting(&filebase,VBUFF);
   } */

if (LocateNextItemStarting(filebase,VBUFF) != NULL)
   {
   Verbose("File %s has already a line starting %s\n",VRESOLVCONF[VSYSTEMHARDCLASS],VBUFF);
   }
 else
    {
    PrependItem(&filebase,VBUFF,NULL);
    }


DeleteItemStarting(&filebase,"domain");
snprintf(VBUFF,bufsize,"domain %s",ToLowerStr(VDOMAIN));
PrependItem(&filebase,VBUFF,NULL);

if (GetMacroValue(CONTEXTID,"EmptyResolvConf") && (strcmp(GetMacroValue(CONTEXTID,"EmptyResolvConf"),"true") == 0))
   {
   while (DeleteItemStarting(&filebase,"nameserver "))
      {
      }
   }
 
EditItemsInResolvConf(VRESOLVE,&filebase); 
 
if (DONTDO)
   {
   printf("Check %s for editing\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
   }
else if (!ItemListsEqual(filebase,referencefile))
   {
   SaveItemList(filebase,VRESOLVCONF[VSYSTEMHARDCLASS],VREPOSITORY);
   chmod(VRESOLVCONF[VSYSTEMHARDCLASS],DEFAULTSYSTEMMODE);
   }
else
   {
   Verbose("cfengine: %s is okay\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
   }

DeleteItemList(filebase);
DeleteItemList(referencefile);
}


/*******************************************************************/

void MakeImages()

{ struct Image *ip;
  struct Item *svp;
  struct stat statbuf;
  struct servent *serverent;
  int savesilent;
  char path[bufsize];
  char destination[bufsize];
  char server[bufsize];
 
if ((serverent = getservbyname(CFENGINE_SERVICE,"tcp")) == NULL)
   {
   CfLog(cfverbose,"Remember to register cfengine in /etc/services: cfengine 5308/tcp\n","getservbyname");
   PORTNUMBER = htons((unsigned short)5308);
   }
else
   {
   PORTNUMBER = (unsigned short)(serverent->s_port); /* already in network order */
   }
 
for (svp = VSERVERLIST; svp != NULL; svp=svp->next) /* order servers */
   {
   CONN = NewAgentConn();  /* Global input/output channel */

   if (CONN == NULL)
      {
      return;
      }

   for (ip = VIMAGE; ip != NULL; ip=ip->next)
      {
      ExpandVarstring(ip->server,server,NULL);
      ExpandVarstring(ip->path,path,NULL);
      ExpandVarstring(ip->destination,destination,NULL);
      
      if (strcmp(svp->name,server) != 0)  /* group together similar hosts so */
	 {                                /* can can do multiple transactions */
	 continue;                        /* on one connection */
	 }
      
      if (IsExcluded(ip->classes))
	 {
	 continue;
	 }

      if (ip->done == 'y' || strcmp(ip->scope,CONTEXTID))
	 {
	 continue;
	 }
      else
	 {
	 ip->done = 'y';
	 }

      Verbose("Checking copy from %s:%s to %s\n",server,path,destination);
      
      if (!OpenServerConnection(ip))
	 {
	 snprintf(OUTPUT,bufsize*2,"Unable to establish connection with %s\n",svp->name);
	 CfLog(cfinform,OUTPUT,"");
	 AddMultipleClasses(ip->failover);
	 continue;
	 }

      if (AUTHENTICATED)
	 {
	 Debug("Authentic connection verified\n");
	 }
   
      IMAGEBACKUP = true;

      savesilent = SILENT;
      
      if (strcmp(ip->action,"silent") == 0)
	 {
	 SILENT = true;
	 }
      
      ResetOutputRoute(ip->log,ip->inform);
      
      snprintf(VBUFF,bufsize,"%.50s.%.50s",path,destination); /* Unique ID for copy locking */

      if (!GetLock(ASUniqueName("copy"),CanonifyName(VBUFF),VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
	 {
	 SILENT = savesilent;
	 ResetOutputRoute('d','d');
	 continue;
	 }
      
      IMAGEBACKUP = ip->backup;
      
      if (cfstat(path,&statbuf,ip) == -1)
	 {
	 snprintf(OUTPUT,bufsize*2,"Can't stat %s in copy\n",path);
	 CfLog(cfinform,OUTPUT,"");
	 ReleaseCurrentLock();
	 SILENT = savesilent;
	 ResetOutputRoute('d','d');
	 continue;
	 }
      
      if (strncmp(destination,"home",4) == 0)
	 {
	 HOMECOPY = true;          /* Don't send home backups to repository */
	 CheckHomeImages(ip);
	 HOMECOPY = false;
	 }
      else
	 {
	 if (S_ISDIR(statbuf.st_mode))
	    {
	    if (ip->purge == 'y')
	       {
	       Verbose("%s: (Destination purging enabled)\n",VPREFIX);
	       }
	    RecursiveImage(ip,path,destination,ip->recurse);
	    }
	 else
	    {
	    if (! MakeDirectoriesFor(destination,'n'))
	       {
	       ReleaseCurrentLock();
	       SILENT = savesilent;
	       ResetOutputRoute('d','d');
	       continue;
	       }
	    
	    CheckImage(path,destination,ip);
	    }
	 }
      
      ReleaseCurrentLock();
      SILENT = savesilent;
      ResetOutputRoute('d','d');
      }

   CloseServerConnection();
   DeleteAgentConn(CONN); 
   }
}

/*******************************************************************/

void ConfigureInterfaces()

{ struct Interface *ifp;

if (GetLock("netconfig",VIFDEV[VSYSTEMHARDCLASS],VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   if (strlen(VNETMASK) != 0)
      {
      IfConf(VIFDEV[VSYSTEMHARDCLASS],VNETMASK,VBROADCAST);
      }
   
   SetDefaultRoute();
   ReleaseCurrentLock();   
   }
    
for (ifp = VIFLIST; ifp != NULL; ifp=ifp->next)
   {
   if (!GetLock("netconfig",ifp->ifdev,VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   if (ifp->done == 'y' || strcmp(ifp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      ifp->done = 'y';
      }
   
   IfConf(ifp->ifdev,ifp->netmask,ifp->broadcast);
   SetDefaultRoute();
   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void CheckTimeZone()

{ struct Item *ip;
  char tz[maxvarsize];
  
if (VTIMEZONE == NULL)
   {
   CfLog(cferror,"Program does not define a timezone","");
   return;
   }

for (ip = VTIMEZONE; ip != NULL; ip=ip->next)
   {
#ifdef NT
   
   tzset();
   strcpy(tz,timezone());
   
#else
#ifndef AOS
#ifndef SUN4

   tzset();
   strcpy(tz,tzname[0]);

#else

   if ((tloc = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n\n");
      }
   strcpy(tz,localtime(&tloc)->tm_zone);
      
#endif /* SUN4 */
#endif /* AOS  */
#endif /* NT */

   if (TZCheck(tz,ip->name))
      {
      return;
      }
   }

snprintf(OUTPUT,bufsize*2,"Time zone was %s which is not in the list of acceptable values",tz);
CfLog(cferror,OUTPUT,""); 
}

/*******************************************************************/

void CheckProcesses()

{ struct Process *pp;
  struct Item *procdata = NULL;
  char *psopts = VPSOPTS[VSYSTEMHARDCLASS];

if (!GetLock(ASUniqueName("processes"),"allprocs",VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }
 
if (!LoadProcessTable(&procdata,psopts))
   {
   CfLog(cferror,"Unable to read the process table\n","");
   return;
   }

for (pp = VPROCLIST; pp != NULL; pp=pp->next)
   {
   if (IsExcluded(pp->classes))
      {
      continue;
      }

   if (pp->done == 'y' || strcmp(pp->scope,CONTEXTID))
      {
      continue;
      }
   else
      {
      pp->done = 'y';
      }

   ResetOutputRoute(pp->log,pp->inform);
   
   if (strcmp(pp->expr,"SetOptionString") == 0)
      {
      psopts = pp->restart;
      DeleteItemList(procdata);
      procdata = NULL;
      if (!LoadProcessTable(&procdata,psopts))
	 {
	 CfLog(cferror,"Unable to read the process table\n","");
	 }
      }
   else
      {
      DoProcessCheck(pp,procdata);
      }
   
   ResetOutputRoute('d','d');
   }

ReleaseCurrentLock(); 
}

/*******************************************************************/

int RequiredFileSystemOkay (name)

char *name;

{ struct stat statbuf, localstat;
  DIR *dirh;
  struct dirent *dirp;
  long sizeinbytes = 0, filecount = 0;
  char buff[bufsize];

Debug2("Checking required filesystem %s\n",name);

if (stat(name,&statbuf) == -1)
   {
   return(false);
   }

if (S_ISLNK(statbuf.st_mode))
   {
   KillOldLink(name,NULL);
   return(true);
   }

if (S_ISDIR(statbuf.st_mode))
   {
   if ((dirh = opendir(name)) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open directory %s which checking required/disk\n",name);
      CfLog(cferror,OUTPUT,"opendir");
      return false;
      }

   for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,name,NULL))
         {
         continue;
         }

      filecount++;

      strcpy(buff,name);

      if (buff[strlen(buff)] != '/')
         {
         strcat(buff,"/");
         }

      strcat(buff,dirp->d_name);

      if (lstat(buff,&localstat) == -1)
         {
         if (S_ISLNK(localstat.st_mode))
            {
            KillOldLink(buff,NULL);
            continue;
            }

         snprintf(OUTPUT,bufsize*2,"Can't stat %s in required/disk\n",buff);
	 CfLog(cferror,OUTPUT,"lstat");
         continue;
         }

      sizeinbytes += localstat.st_size;
      }

   closedir(dirh);

   if (sizeinbytes < 0)
      {
      Verbose("Internal error: count of byte size was less than zero!\n");
      return true;
      }

   if (sizeinbytes < SENSIBLEFSSIZE)
      {
      snprintf(OUTPUT,bufsize*2,"File system %s is suspiciously small! (%d bytes)\n",name,sizeinbytes);
      CfLog(cferror,OUTPUT,"");
      return(false);
      }

   if (filecount < SENSIBLEFILECOUNT)
      {
      snprintf(OUTPUT,bufsize*2,"Filesystem %s has only %d files/directories.\n",name,filecount);
      CfLog(cferror,OUTPUT,"");
      return(false);
      }
   }

return(true);
}


/*******************************************************************/

void InstallMountedItem(host,mountdir)

char *host, *mountdir;

{ char buf[bufsize];
 
strcpy (buf,host);
strcat (buf,":");
strcat (buf,mountdir);

if (IsItemIn(VMOUNTED,buf))
   {
   if (! SILENT || !WARNINGS)
      {
      if (!strstr(buf,"swap"))
	 {
	 snprintf(OUTPUT,bufsize*2,"WARNING mount item %s\n",buf);
	 CfLog(cferror,OUTPUT,"");
	 CfLog(cferror,"is mounted multiple times!\n","");
	 }
      }
   }

AppendItem(&VMOUNTED,buf,NULL);
}

/*******************************************************************/


void AddToFstab(host,rmountpt,mountpt,mode,options,ismounted)

char *host, *mountpt, *rmountpt, *mode, *options;
int ismounted;

{ char fstab[bufsize];
  char *opts;
  FILE *fp;

Debug("AddToFstab(%s)\n",mountpt);


if ( options != NULL)
   {
   opts = options;
   }
else
   {
   opts = VMOUNTOPTS[VSYSTEMHARDCLASS];
   }

switch (VSYSTEMHARDCLASS)
   {
   case osf:
   case bsd4_3:
   case irix:
   case irix4:
   case irix64:
   case sun3:
   case aos:
   case nextstep:
   case newsos:
   case sun4:    snprintf(fstab,bufsize,"%s:%s \t %s %s\t%s,%s 0 0",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case crayos:
                 snprintf(fstab,bufsize,"%s:%s \t %s %s\t%s,%s",host,rmountpt,mountpt,ToUpperStr(VNFSTYPE),mode,opts);
                 break;
   case ultrx:   snprintf(fstab,bufsize,"%s@%s:%s:%s:0:0:%s:%s",rmountpt,host,mountpt,mode,VNFSTYPE,opts);
                 break;
   case hp:      snprintf(fstab,bufsize,"%s:%s %s \t %s \t %s,%s 0 0",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;
   case aix:     snprintf(fstab,bufsize,"%s:\n\tdev\t= %s\n\ttype\t= %s\n\tvfs\t= %s\n\tnodename\t= %s\n\tmount\t= true\n\toptions\t= %s,%s\n\taccount\t= false\n\n",mountpt,rmountpt,VNFSTYPE,VNFSTYPE,host,mode,opts);
                 break;
   case GnU:
   case linuxx:  snprintf(fstab,bufsize,"%s:%s \t %s \t %s \t %s,%s",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case netbsd:
   case openbsd:
   case bsd_i:
   case freebsd: snprintf(fstab,bufsize,"%s:%s \t %s \t %s \t %s,%s 0 0",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case unix_sv:
   case solarisx86:
   case solaris: snprintf(fstab,bufsize,"%s:%s - %s %s - yes %s,%s",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case cfnt:    snprintf(fstab,bufsize,"/bin/mount %s:%s %s",host,rmountpt,mountpt);
                 break;
   case cfsco:   CfLog(cferror,"Don't understand filesystem format on SCO, no data","");
                 break;
   case unused1:
   case unused2:
   case unused3:
   default:      FatalError("AddToFstab(): unknown hard class detected!\n");
                 break;
   }

if (MatchStringInFstab(mountpt))
   {
   /* if the fstab entry has changed, remove the old entry and update */
   if (!MatchStringInFstab(fstab))
      { struct UnMount *saved_VUNMOUNT = VUNMOUNT;
	char mountspec[MAXPATHLEN];
        struct Item *mntentry = NULL;
	struct UnMount cleaner;

      snprintf(OUTPUT,bufsize*2,"Removing \"%s\" entry from %s to allow update:\n",
              mountpt,VFSTAB[VSYSTEMHARDCLASS]);
      CfLog(cfinform,OUTPUT,"");
      CfLog(cfinform,"---------------------------------------------------","");

      /* delete current fstab entry and unmount if necessary */
      snprintf(mountspec,bufsize,".+:%s",mountpt);
      mntentry = LocateItemContainingRegExp(VMOUNTED,mountspec);
      if (mntentry)
	 {
	 sscanf(mntentry->name,"%[^:]:",mountspec);  /* extract current host */
	 strcat(mountspec,":");
	 strcat(mountspec,mountpt);
	 }
      else  /* mountpt isn't mounted, so Unmount can use dummy host name */
	 snprintf(mountspec,bufsize,"host:%s",mountpt);

      /* delete current fstab entry and unmount if necessary (don't rmdir) */
      cleaner.name        = mountspec;
      cleaner.classes     = NULL;
      cleaner.deletedir   = 'n';
      cleaner.deletefstab = 'y';
      cleaner.force       = 'n';
      cleaner.done        = 'n';
      cleaner.scope       = CONTEXTID;
      cleaner.next        = NULL;

      VUNMOUNT = &cleaner;
      Unmount();
      VUNMOUNT = saved_VUNMOUNT;
      CfLog(cfinform,"---------------------------------------------------","");
      }
   else /* no need to update fstab - this mount entry is already there */
      {
      /* warn if entry's already in the fstab but hasn't been mounted */
      if (!ismounted && !SILENT && !strstr(mountpt,"cdrom"))
	 {
	 snprintf(OUTPUT,bufsize*2,"Warning the file system %s seems to be in %s\n",mountpt,VFSTAB[VSYSTEMHARDCLASS]);
	 CfLog(cfinform,OUTPUT,"");
	 snprintf(OUTPUT,bufsize*2,"already, but I was not able to mount it.\n");
	 CfLog(cfinform,OUTPUT,"");
	 snprintf(OUTPUT,bufsize*2,"Check the exports file on host %s? Check for file with same name as dir?\n",host);
	 CfLog(cfinform,OUTPUT,"");
	 }
      
      return;
      }
   }

if (DONTDO)
   {
   printf("%s: add filesystem to %s\n",VPREFIX,VFSTAB[VSYSTEMHARDCLASS]);
   printf("%s: %s\n",VPREFIX,fstab);
   }
else
   {
   if ((fp = fopen(VFSTAB[VSYSTEMHARDCLASS],"a")) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open %s for appending\n",VFSTAB[VSYSTEMHARDCLASS]);
      CfLog(cferror,OUTPUT,"fopen");
      ReleaseCurrentLock();
      return;
      }

   snprintf(OUTPUT,bufsize*2,"Adding filesystem to %s\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cfinform,OUTPUT,"");
   snprintf(OUTPUT,bufsize*2,"%s\n",fstab);
   CfLog(cfinform,OUTPUT,"");
   fprintf(fp,"%s\n",fstab);
   fclose(fp);
   
   chmod(VFSTAB[VSYSTEMHARDCLASS],DEFAULTSYSTEMMODE);
   }
}


/*******************************************************************/

int CheckFreeSpace (file,disk_ptr)

char *file;
struct Disk *disk_ptr;

{ struct stat statbuf;
  int free;
  int kilobytes;

if (stat(file,&statbuf) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't stat %s checking diskspace\n",file);
   CfLog(cferror,OUTPUT,"");
   return true;
   }

/* HvB : Bas van der Vlies 
	 if force is specified then skip this check if this 
	 is on the file server.
*/
if ( disk_ptr->force != 'y' )
   {
   if (IsMountedFileSystem(&statbuf,file,1))
      {
      return true;
      }
   }

kilobytes = disk_ptr->freespace;
if (kilobytes < 0)  /* percentage */
   {
   free = GetDiskUsage(file,cfpercent);
   kilobytes = -1 * kilobytes;
   if (free < kilobytes)
      {
      snprintf(OUTPUT,bufsize*2,"Free disk space is under %d%% for partition\n",kilobytes);
      CfLog(cfinform,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"containing %s (%d%% free)\n",file,free);
      CfLog(cfinform,OUTPUT,"");
      return false;
      }
   }
else
   {
   free = GetDiskUsage(file, cfabs);

   if (free < kilobytes)
      {
      snprintf(OUTPUT,bufsize*2,"Disk space under %d kB for partition\n",kilobytes);
      CfLog(cfinform,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"containing %s (%d kB free)\n",file,free);
      CfLog(cfinform,OUTPUT,"");
      return false;
      }
   }

return true;
}

/*******************************************************************/

void CheckHome(ptr)                      /* iterate check over homedirs */

struct File *ptr;

{ struct Item *ip1, *ip2;
  char basename[bufsize],pathbuff[bufsize];

Debug("CheckHome(%s)\n",ptr->path);

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can check others' files.\n","");
   return;
   }

if (!MountPathDefined())
   {
   return;
   }

for (ip1 = VHOMEPATLIST; ip1 != NULL; ip1=ip1->next)
   {
   for (ip2 = VMOUNTLIST; ip2 != NULL; ip2=ip2->next)
      {
      if (IsExcluded(ip2->classes))
	 {
	 continue;
	 }
      pathbuff[0]='\0';
      basename[0]='\0';
      strcpy(pathbuff,ip2->name);
      AddSlash(pathbuff);
      strcat(pathbuff,ip1->name);
      AddSlash(pathbuff);

      if (strncmp(ptr->path,"home/",5) == 0) /* home/subdir */
	 {
	 strcat(pathbuff,"*");
	 AddSlash(pathbuff);

         if (*(ptr->path+4) != '/')
            {
            snprintf(OUTPUT,bufsize*2,"Illegal use of home in files: %s\n",ptr->path);
	    CfLog(cferror,OUTPUT,"");
            return;
            }
         else
            {
            strcat(pathbuff,ptr->path+5);
            }

	 ExpandWildCardsAndDo(pathbuff,basename,RecFileCheck,ptr);
	 }
      else
	 {
	 ExpandWildCardsAndDo(pathbuff,basename,RecFileCheck,ptr);
	 }
      }
   }
}

/*******************************************************************/

void EditItemsInResolvConf(from,list)

struct Item *from, **list;

{ char buf[maxvarsize],work[bufsize];

if ((from != NULL) && !IsDefinedClass(from->classes))
   {
   return;
   }
  
if (from == NULL)
   {
   return;
   }
else
   {
   ExpandVarstring(from->name,work,"");
   EditItemsInResolvConf(from->next,list);
   if (isdigit((int)*(work)))
      {
      snprintf(buf,bufsize,"nameserver %s",work);
      }
   else
      {
      strcpy(buf,work);
      }
   
   DeleteItemMatching(list,buf); /* del+prep = move to head of list */
   PrependItem(list,buf,NULL);
   return;
   }
}


/*******************************************************************/

int TZCheck(tzsys,tzlist)

char *tzsys,*tzlist;

{
if (strncmp(tzsys,"GMT",3) == 0)
   {
   return (strncmp(tzsys,tzlist,5) == 0); /* e.g. GMT+1 */
   }
else
   {
   return (strncmp(tzsys,tzlist,3) == 0); /* e.g. MET or CET */
   }
}

/*******************************************************************/

void ExpandWildCardsAndDo(wildpath,buffer,function,argptr)
 
char *wildpath, *buffer;
void (*function) ARGLIST((char *path, void *ptr));
void *argptr;

 /* This function recursively expands a path containing wildcards */
 /* and executes the function pointed to by function for each     */
 /* matching file or directory                                    */

 
{ char *rest, extract[bufsize], construct[bufsize],varstring[bufsize],cleaned[bufsize], *work;
  struct stat statbuf;
  DIR *dirh;
  struct dirent *dp;
  int count, isdir = false,i,j;

varstring[0] = '\0';

bzero(cleaned,bufsize); 
 
for (i = j = 0; wildpath[i] != '\0'; i++,j++)
   {
   if ((i > 0) && (wildpath[i] == '/') && (wildpath[i-1] == '/'))
      {
      j--;
      }
   cleaned[j] = wildpath[i];
   }

ExpandVarstring(cleaned,varstring,NULL);
work = varstring;

Debug2("ExpandWildCardsAndDo(%s=%s)\n",cleaned,work);
 
extract[0] = '\0';

if (*work == '/')
   {
   work++;
   isdir = true;
   }

sscanf(work,"%[^/]",extract);
rest = work + strlen(extract);
 
if (strlen(extract) == 0)
   {
   if (isdir)
      {
      strcat(buffer,"/");
      }
   (*function)(buffer,argptr);
   return;
   }
 
if (! IsWildCard(extract))
   {
   strcat(buffer,"/");
   if (BufferOverflow(buffer,extract))
       {
       snprintf(OUTPUT,bufsize*2,"Culprit %s\n",extract);
       CfLog(cferror,OUTPUT,"");
       exit(0);
       }
   strcat(buffer,extract);
   ExpandWildCardsAndDo(rest,buffer,function,argptr);
   return;
   }
else
   { 
   strcat(buffer,"/");
   
   if ((dirh=opendir(buffer)) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Can't open dir: %s\n",buffer);
      CfLog(cferror,OUTPUT,"opendir");
      return;
      }

   count = 0;
   strcpy(construct,buffer);                 /* save relative path */
 
   for (dp = readdir(dirh); dp != 0; dp = readdir(dirh))
      {
      if (!SensibleFile(dp->d_name,buffer,NULL))
         {
         continue;
         }

      count++;
      strcpy(buffer,construct);
      strcat(buffer,dp->d_name);

      if (stat(buffer,&statbuf) == -1)
         {
         snprintf(OUTPUT,bufsize*2,"Can't stat %s\n\n",buffer);
         CfLog(cferror,OUTPUT,"stat");
         continue;
         }
 
      if (S_ISDIR(statbuf.st_mode) && WildMatch(extract,dp->d_name))
         {
         ExpandWildCardsAndDo(rest,buffer,function,argptr);
         } 
      }
 
   if (count == 0)
      {
      snprintf(OUTPUT,bufsize*2,"No directories matching %s in %s\n",extract,buffer);
      CfLog(cfinform,OUTPUT,"");
      return;
      }
   closedir(dirh);
   }
}
 

/*******************************************************************/

int TouchDirectory(ptr)                     /* True if file path in /. */

struct File *ptr;

{ char *sp;

if (ptr->action == touch)
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


/*******************************************************************/

void RecFileCheck(startpath,vp)

char *startpath;
void *vp;

{ struct File *ptr;
  struct stat sb; 

ptr = (struct File *)vp;

Verbose("%s: Checking files in %s...\n",VPREFIX,startpath);

if (!GetLock(ASUniqueName("files"),startpath,VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   return;
   }
 
if (stat(startpath,&sb) == -1)
   {
   snprintf(OUTPUT,bufsize,"Directory %s cannot be accessed in files",startpath);
   CfLog(cfinform,OUTPUT,"stat");
   return;
   }
 
RecursiveCheck(startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,ptr->recurse,0,ptr,&sb);
 
ReleaseCurrentLock(); 
}

/*******************************************************************/
/* Toolkit fstab                                                   */
/*******************************************************************/

int MatchStringInFstab(str)

char *str;

{ FILE *fp;

if ((fp = fopen(VFSTAB[VSYSTEMHARDCLASS],"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Can't open %s for reading\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cferror,OUTPUT,"fopen");
   return true; /* write nothing */
   }

while (!feof(fp))
   {
   ReadLine(VBUFF,bufsize,fp);

   if (VBUFF[0] == '#')
      {
      continue;
      }

   if (strstr(VBUFF,str))
      {
      fclose(fp);
      return true;
      }
   }

fclose(fp);
return(false);
}


