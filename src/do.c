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
  char path[CF_BUFSIZE],mountitem[CF_BUFSIZE];

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
      snprintf(OUTPUT,CF_BUFSIZE*2,"INFO: Host %s seems to have no (additional) local disks except the OS\n",VDEFAULTBINSERVER.name);
      CfLog(cfverbose,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"      mounted under %s\n\n",ip->name);
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"Host defines a home directory %s\n",VBUFF);
         CfLog(cfverbose,OUTPUT,"");
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Host defines a potential mount point %s\n",VBUFF);
         CfLog(cfverbose,OUTPUT,"");
         }

      snprintf(path,CF_BUFSIZE,"%s%s",ip->name,dirp->d_name);
      snprintf(mountitem,CF_BUFSIZE,"%s:%s",VDEFAULTBINSERVER.name,path);

      if (! IsItemIn(VMOUNTED,mountitem))
         {
         if ( MOUNTCHECK && ! RequiredFileSystemOkay(path) && VERBOSE)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Found a mountpoint %s but there was\n",path);
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
  char buf1[CF_BUFSIZE],buf2[CF_BUFSIZE],buf3[CF_BUFSIZE];
  char host[CF_MAXVARSIZE], mounton[CF_BUFSIZE];
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
   snprintf(OUTPUT,CF_BUFSIZE*2,"%s: Can't open %s\n",VPREFIX,buf1);
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
   
   ReadLine(VBUFF,CF_BUFSIZE,pp);

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
         snprintf(OUTPUT,CF_BUFSIZE*2,"%s\n",VBUFF);
  CfLog(cfinform,OUTPUT,"");
         }

      GOTMOUNTINFO = false;
      ReleaseCurrentLock();
      cfpclose(pp);
      return;
      }

   switch (VSYSTEMHARDCLASS)
      {
      case darwin:
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
      case qnx:
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
  char pathbuff[CF_BUFSIZE],basename[CF_EXPANDSIZE];
  
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal use of home in directories: %s\n",ptr->path);
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
  char to[CF_EXPANDSIZE],from[CF_EXPANDSIZE],path[CF_EXPANDSIZE];
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

   snprintf(VBUFF,CF_BUFSIZE,"%.50s.%.50s",lp->from,lp->to); /* Unique ID for copy locking */

   if (!GetLock(ASUniqueName("link"),CanonifyName(VBUFF),lp->ifelapsed,lp->expireafter,VUQNAME,CFSTARTTIME))
      {
      lp->done = 'y';
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Makechildlinks() can't stat %s\n",from);
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"Error while trying to childlink %s -> %s\n",from,path);
  CfLog(cferror,OUTPUT,"");
         snprintf(OUTPUT,CF_BUFSIZE*2,"The directory %s does not exist. Can't link.\n",path);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"ChildLink didn't find any server to match %s -> %s\n",from,to);
      CfLog(cferror,OUTPUT,"");
      }

   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void MakeLinks()     /* <binserver> should expand to a best fit filesys */

{ struct Link *lp;
  char from[CF_EXPANDSIZE],to[CF_EXPANDSIZE],path[CF_EXPANDSIZE];
  struct Item *ip;
  int matched,varstring;
  short saveenforce;
  short savesilent;
  int (*linkfiles)(char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr);

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

   snprintf(VBUFF,CF_BUFSIZE,"%.50s.%.50s",lp->from,lp->to); /* Unique ID for copy locking */
   
   if (!GetLock(ASUniqueName("link"),CanonifyName(VBUFF),lp->ifelapsed,lp->expireafter,VUQNAME,CFSTARTTIME))
      {
      lp->done = 'y';
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

   for(ip = VBINSERVERS; ip != NULL && (!matched); ip = ip->next)
      {
      path[0] = '\0';

      varstring = ExpandVarbinserv(to,path,ip->name);

      if ((*linkfiles)(from,path,lp->inclusions,lp->exclusions,lp->copy,lp->nofile,lp))
         {
         matched = true;
         }
      else if (! varstring)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Error while trying to link %s -> %s\n",from,path);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Links didn't find any file to match %s -> %s\n",from,to);
      CfLog(cferror,OUTPUT,"");
      }
   
   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void MailCheck()

{ char mailserver[CF_BUFSIZE];
  char mailhost[CF_MAXVARSIZE];
  char rmailpath[CF_BUFSIZE];
  char lmailpath[CF_BUFSIZE];


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

snprintf(lmailpath,CF_BUFSIZE,"%s:%s",mailhost,VMAILDIR[VSYSTEMHARDCLASS]);


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

void ExpiredUserCheck(char *spooldir,int always)

{
Verbose("%s: Checking for expired users in %s\n",VPREFIX,spooldir); 

if (always || (strncmp(VMAILSERVER,VFQNAME,strlen(VMAILSERVER)) != 0))
   { DIR *dirh;
     struct dirent *dirp;
     struct stat statbuf;

   if ((dirh = opendir(spooldir)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open spool directory %s",spooldir);
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
               snprintf(OUTPUT,CF_BUFSIZE*2,"File %s in spool dir %s is not owned by any user",dirp->d_name,spooldir);
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
                  snprintf(OUTPUT,CF_BUFSIZE*2,"Deleting file %s in spool dir %s not owned by any user",dirp->d_name,spooldir);
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"File %s in spool dir %s is not the name of a user",dirp->d_name,spooldir);
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
               snprintf(OUTPUT,CF_BUFSIZE*2,"Deleting file %s in spool dir %s (not a username)",dirp->d_name,spooldir);
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


if (!GetLock(ASUniqueName("domount"),"",VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
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
    snprintf(OUTPUT,CF_BUFSIZE*2,"Failed to open pipe from %s\n",VMOUNTCOMM[VSYSTEMHARDCLASS]);
    CfLog(cferror,OUTPUT,"popen");
    ReleaseCurrentLock();
    return;
    }
 
while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      CfLog(cfinform,"Error mounting filesystems\n","ferror");
      break;
      }
   
   ReadLine(VBUFF,CF_BUFSIZE,pp);

   if (ferror(pp))  /* abortable */
      {
      CfLog(cfinform,"Error mounting filesystems\n","ferror");
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
      CfLog(cfinform,"There was a mount error, trying to mount one of the filesystems on this host.\n","");
      snprintf(OUTPUT,CF_BUFSIZE*2,"%s\n",VBUFF);
      CfLog(cfinform,OUTPUT,"");
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
ReleaseCurrentLock();
}

/*******************************************************************/

void CheckRequired()

{ struct Disk *rp;
  struct Item *ip;
  int matched=0,varstring=0,missing = 0;
  char path[CF_EXPANDSIZE],expand[CF_EXPANDSIZE];

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
   
   if (!GetLock(ASUniqueName("disks"),rp->name,rp->ifelapsed,rp->expireafter,VUQNAME,CFSTARTTIME))
      {
      rp->done = 'y';
      continue;
      }
 
   ResetOutputRoute(rp->log,rp->inform);
   matched = varstring = false;

   for(ip = VBINSERVERS; ip != NULL && (!matched); ip = ip->next)
      {
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"The file %s does not exist or is suspicious.\n\n",path);
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
         snprintf(OUTPUT,CF_BUFSIZE,"Free space below %d, defining %s\n",rp->freespace, rp->define);
         CfLog(cfinform,OUTPUT,"");
         AddMultipleClasses(rp->define);
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Free space above %d, defining %s\n",rp->freespace, rp->elsedef);
         CfLog(cfverbose,OUTPUT,"");
         AddMultipleClasses(rp->elsedef);
         }
      }
   
   if (matched == false && ip == NULL)
      {
      printf(" didn't find any file to match the required filesystem %s\n",rp->name);
      missing++;
      }

   ReleaseCurrentLock();
   ResetOutputRoute('d','d');
   }

if (missing)
   {
   time_t tloc;

   if ((tloc = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n");
      }
   snprintf(OUTPUT,CF_BUFSIZE*2,"MESSAGE at %s\n\n",ctime(&tloc));
   CfLog(cferror,OUTPUT,"");
   snprintf(OUTPUT,CF_BUFSIZE*2,"There are %d required file(system)s missing on host <%s>\n",missing,VDEFAULTBINSERVER.name);
   CfLog(cferror,OUTPUT,"");   
   CfLog(cferror,"even after all mounts have been attempted.\n","");
   CfLog(cferror,"This may be caused by a failure to mount a network filesystem (check exports)\n","");
   snprintf(OUTPUT,CF_BUFSIZE*2,"or because no valid server was specified in the program %s\n\n",VINPUTFILE);
   CfLog(cferror,OUTPUT,"");
   }

/* Look for any statistical gathering to be scheduled ... */ 

if (IGNORELOCK)  /* This is too heavy to allow without locks */
   {
   return;
   }

Verbose("Checking for filesystem scans...\n"); 
 
for (rp = VREQUIRED; rp != NULL; rp = rp->next)
   {
   if (IsExcluded(rp->classes))
      {
      continue;
      }

   if (rp->scanarrivals != 'y')
      {
      continue;
      }
   
   ResetOutputRoute(rp->log,rp->inform);

   for(ip = VBINSERVERS; ip != NULL && (!matched); ip = ip->next)
      {
      struct stat statbuf;
      DB *dbp = NULL;
      DB_ENV *dbenv = NULL;
      char database[CF_MAXVARSIZE],canon[CF_MAXVARSIZE];
      int ifelapsed = rp->ifelapsed;

      if (ifelapsed < CF_WEEK)
         {
         Verbose("IfElapsed time is too short for these data - changes only slowly\n");
         ifelapsed = CF_WEEK;
         }
      
      ExpandVarstring(rp->name,expand,NULL);
      varstring = ExpandVarbinserv(expand,path,ip->name);

      if (lstat(path,&statbuf) == -1)
         {
         continue;
         }
      
      if (!GetLock(ASUniqueName("diskscan"),CanonifyName(rp->name),ifelapsed,rp->expireafter,VUQNAME,CFSTARTTIME))
         {
         continue;
         }
      
      snprintf(canon,CF_MAXVARSIZE-1,"%s",CanonifyName(path));      
      snprintf(database,CF_MAXVARSIZE-1,"%s/scan:%s.db",VLOCKDIR,canon);
      
      Verbose("Scanning filesystem %s for arrival processes...to %s\n",path,database);
      
      unlink(database);
      
      if ((errno = db_create(&dbp,dbenv,0)) != 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
         CfLog(cferror,OUTPUT,"db_open");
         return;
         }
      
#ifdef CF_OLD_DB
      if ((errno = dbp->open(dbp,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
      if ((errno = dbp->open(dbp,NULL,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open database %s\n",database);
         CfLog(cferror,OUTPUT,"db_open");
         dbp->close(dbp,0);
         continue;
         }      

      chmod(database,0644); 
      RegisterRecursionRootDevice(statbuf.st_dev);
      ScanFileSystemArrivals(path,0,&statbuf,dbp);
      
      dbp->close(dbp,0);
      ReleaseCurrentLock();      
      }
   
   ResetOutputRoute('d','d');
   }
}

/*******************************************************************/

void TidyFiles()

   /* Here we start by scanning for any absolute path wildcards */
   /* After that's finished, we go snooping around the homedirs */

{ char basename[CF_EXPANDSIZE],pathbuff[CF_BUFSIZE];
  struct TidyPattern *tlp;
  struct Tidy *tp;
  struct Item *ip1,*ip2;
  int homesearch = false, pathsearch = false;

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
   
   pathsearch = false;
   
   for (tlp = tp->tidylist; tlp != NULL; tlp=tlp->next)
      {
      if (IsExcluded(tlp->classes))
         {
         continue;
         }
      pathsearch = true;
      }
   
   if (pathsearch && (tp->done == 'n'))
      {
      Debug("\nTidying from base directory %s\n",tp->path);
      basename[0] = '\0';
      ExpandWildCardsAndDo(tp->path,basename,TidyWrapper,tp);
      tp->done = 'y';
      if (tp->tidylist->next != NULL) 
         {
         tp->done = 'n';
         }
      else 
         {
         tp->done = 'y';
         }
      }
   else
      {
      Debug("\nNo patterns active in base directory %s\n",tp->path);
      }
   }
 
 
Debug2("End PATHTIDY:\n");


Banner("Tidying home directories");
 
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
  char line[CF_BUFSIZE],eventname[CF_BUFSIZE];
  char comm[20], *sp;
  char execstr[CF_EXPANDSIZE];
  char chdir_buf[CF_EXPANDSIZE];
  char chroot_buf[CF_EXPANDSIZE];
  struct timespec start,stop;
  int measured_ok = true;
  double dt = 0;
  int print, outsourced;
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
   ExpandVarstring(ptr->name,execstr,NULL);

   if (!GetLock(ASUniqueName("shellcommand"),execstr,ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
      {
      ptr->done = 'y';
      continue;
      }
   
   snprintf(OUTPUT,CF_BUFSIZE*2,"\nExecuting script %s...(timeout=%d,uid=%d,gid=%d)\n",execstr,ptr->timeout,ptr->uid,ptr->gid);
   CfLog(cfinform,OUTPUT,"");

   if (clock_gettime(CLOCK_REALTIME, &start) == -1)
      {
      CfLog(cfverbose,"Clock gettime failure","clock_gettime");
      measured_ok = false;
      }

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
      
      memset(comm,0,20);
      strncpy(comm,sp,15);

      if (ptr->fork == 'y')
         {
         Verbose("Backgrounding job %s\n",execstr);
         outsourced = fork();
         }
      else
         {
         outsourced = false;
         }

      if (outsourced || ptr->fork != 'y')
         {
         if (ptr->timeout != 0)
            {
            signal(SIGALRM,(void *)TimeOut);
            alarm(ptr->timeout);
            }
         
         Verbose("(Setting umask to %o)\n",ptr->umask);
         maskval = umask(ptr->umask);
         
         if (ptr->umask == 0)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Programming %s running with umask 0! Use umask= to set\n",execstr);
            CfLog(cfsilent,OUTPUT,"");
            }
         
         ExpandVarstring(ptr->chdir,chdir_buf,"");
         ExpandVarstring(ptr->chroot,chroot_buf,"");
         
         switch (ptr->useshell)
            {
            case 'y':  pp = cfpopen_shsetuid(execstr,"r",ptr->uid,ptr->gid,chdir_buf,chroot_buf);
                break;
            default:   pp = cfpopensetuid(execstr,"r",ptr->uid,ptr->gid,chdir_buf,chroot_buf);
                break;      
            }
         
         if (pp == NULL)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",execstr);
            CfLog(cferror,OUTPUT,"popen");
            ResetOutputRoute('d','d');
            ReleaseCurrentLock();
            continue;
            } 
         
         while (!feof(pp))
            {
            if (ferror(pp))  /* abortable */
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",execstr);
               CfLog(cferror,OUTPUT,"ferror");
               break;
               }
            
            ReadLine(line,CF_BUFSIZE-1,pp);
            
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
                  
                  for (i=0; i < precount; i++)
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
               
               snprintf(OUTPUT,CF_BUFSIZE,"%s (preview of %s)\n",message,comm);
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
      
      snprintf(OUTPUT,CF_BUFSIZE*2,"Finished script %s\n",execstr);
      CfLog(cfinform,OUTPUT,"");
      
      ResetOutputRoute('d','d');
      ReleaseCurrentLock();
      
      if (clock_gettime(CLOCK_REALTIME, &stop) == -1)
         {
         CfLog(cfverbose,"Clock gettime failure","clock_gettime");
         measured_ok = false;
         }
      
      dt = (double)(stop.tv_sec - start.tv_sec)+(double)(stop.tv_nsec-start.tv_nsec)/(double)CF_BILLION;
      
      snprintf(eventname,CF_BUFSIZE-1,"Exec(%s)",execstr);
      
      if (measured_ok)
         {
         RecordPerformance(eventname,start.tv_sec,dt);
         }

      if (ptr->fork == 'y' && outsourced)
         {
         Verbose("Backgrounded shell command (%s) exiting\n",execstr);
         exit(0);
         }
      }
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
      ReadLine(VBUFF,CF_BUFSIZE,fp);

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
  char ebuff[CF_EXPANDSIZE];
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

   ebuff[0] = '\0';

   ExpandWildCardsAndDo(ptr->path,ebuff,CheckFileWrapper,ptr);

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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open %s for writing\n",VSETUIDLOG);
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
  char workname[CF_EXPANDSIZE],path[CF_BUFSIZE];
  
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

   if (!GetLock(ASUniqueName("disable"),CanonifyName(dp->name),dp->ifelapsed,dp->expireafter,VUQNAME,CFSTARTTIME))
      {
      continue;
      }

   dp->done = 'y';

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
      if ((strcmp(dp->type,"file") == 0) || (strcmp(dp->type,"link") == 0))
         {
         Verbose("Filetype %s, %s is not there - ok\n",dp->type,workname);
         ResetOutputRoute('d','d');
         ReleaseCurrentLock();
         continue;
         }      
      }

   Verbose("Disable/rename checking %s\n",workname);

   if (S_ISLNK(statbuf.st_mode))
      {
      if (strcmp(dp->type,"file") == 0)
         {
         Verbose("%s: %s is a link, not disabling\n",VPREFIX,workname);
         ResetOutputRoute('d','d');
         ReleaseCurrentLock();
         continue;
         }
      
      memset(VBUFF,0,CF_BUFSIZE);
      
      if (readlink(workname,VBUFF,CF_BUFSIZE-1) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"DisableFiles() can't read link %s\n",workname);
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
               snprintf(OUTPUT,CF_BUFSIZE*2,"Error while unlinking %s\n",workname);
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
         snprintf(OUTPUT,CF_BUFSIZE,"Warning - file %s exists\n",workname);
         CfLog(cferror,OUTPUT,"");
         }
      }
   else
      {
      if (!S_ISREG(statbuf.st_mode) && (strlen(dp->destination) == 0))
         {
         Verbose("%s: %s is not a plain file - won't rename/disable without specific destination\n",VPREFIX,workname);
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
      
      if (dp->size != CF_NOSIZE)
         {
         switch (dp->comp)
            {
            case '<':
                if (statbuf.st_size < dp->size)
                   {
                   Verbose("cfengine %s is smaller than %d bytes\n",workname,dp->size);
                   break;
                   }
                Verbose("Size is okay\n");
                ResetOutputRoute('d','d');
                ReleaseCurrentLock();
                continue;
                
            case '=':
                if (statbuf.st_size == dp->size)
                   {
                   Verbose("cfengine %s is equal to %d bytes\n",workname,dp->size);
                   break;
                   }
                Verbose("Size is okay\n");
                ResetOutputRoute('d','d');
                ReleaseCurrentLock();
                continue;
                
            default:
                if (statbuf.st_size > dp->size)
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
         if (strlen(dp->destination) > 0)
            {
            if (IsFileSep(dp->destination[0]))
               {
               strncpy(path,dp->destination,CF_BUFSIZE-1);
               }
            else
               {
               strcpy(path,workname);
               ChopLastNode(path);
               AddSlash(path);
               if (BufferOverflow(path,dp->destination))
                  {
                  snprintf(OUTPUT,CF_BUFSIZE*2,"Buffer overflow occurred while renaming %s\n",workname);
                  CfLog(cferror,OUTPUT,"");
                  ResetOutputRoute('d','d');
                  ReleaseCurrentLock();
                  continue;
                  }
               strcat(path,dp->destination);
               }
            }
         else
            {
            strcpy(path,workname);
            strcat(path,".cfdisabled");
            }

         snprintf(OUTPUT,CF_BUFSIZE*2,"Disabling/renaming file %s to %s (pending repository move)\n",workname,path);
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
                     snprintf(OUTPUT,CF_BUFSIZE*2,"Error occurred while renaming %s\n",workname);
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
                  snprintf(OUTPUT,CF_BUFSIZE,"Warning - file %s exists (need to disable)",workname);
                  CfLog(cferror,OUTPUT,"");
                  }        
               }
            }
         }
      else if (dp->rotate == CF_TRUNCATE)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Truncating (emptying) %s\n",workname);
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
            snprintf(OUTPUT,CF_BUFSIZE,"File %s needs emptying",workname);
            CfLog(cferror,OUTPUT,"");
            }
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Rotating files %s by %d\n",workname,dp->rotate);
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
               snprintf(OUTPUT,CF_BUFSIZE,"File %s needs rotating/emptying",workname);
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
  char host[CF_MAXVARSIZE];
  char mountdir[CF_BUFSIZE];
  char maketo[CF_BUFSIZE];
  struct Item *ip;

  /*
   * HvB: Bas van der Vlies
  */
  char mountmode[CF_BUFSIZE];

if (! GOTMOUNTINFO)
   {
   CfLog(cfinform,"Incomplete mount info due to RPC failure.\n","");
   snprintf(OUTPUT,CF_BUFSIZE*2,"%s will not be modified on this pass!\n\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if (!IsPrivileged())                            
   {
   CfLog(cferror,"Only root can mount filesystems.\n","");
   return;
   }

Banner("Checking home and binservers");

Debug("BINSERVER = %s\n",VDEFAULTBINSERVER.name);

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

   Debug("I am [%s], you are [%s]\n",host,VDEFAULTBINSERVER.name);

   if (strcmp(host,VDEFAULTBINSERVER.name) == 0) /* A host never mounts itself nfs */
      {
      Debug("Skipping host %s\n",host);
      continue;
      }

   /* HvB: Bas van der Vlies */
   if (mp->readonly) 
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
  char host[CF_MAXVARSIZE];
  char mountdir[CF_BUFSIZE];
  char maketo[CF_BUFSIZE];
  char mtpt[CF_BUFSIZE];

if (! GOTMOUNTINFO)
   {
   CfLog(cfinform,"Incomplete mount info due to RPC failure.\n","");
   snprintf(OUTPUT,CF_BUFSIZE*2,"%s will not be modified on this pass!\n\n",VFSTAB[VSYSTEMHARDCLASS]);
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

   snprintf(mtpt,CF_BUFSIZE,"%s:%s",host,mp->onto);
   
   if (!IsItemIn(VMOUNTED,mtpt))
      {
      MakeDirectoriesFor(maketo,'n');
      AddToFstab(host,mountdir,mp->onto,mp->mode,mp->options,false);
      }
   else
      {
      AddToFstab(host,mountdir,mp->onto,mp->mode,mp->options,true);
      }
   }
}

/*********************************************************************/

void Unmount()

{ struct UnMount *ptr;
  char comm[CF_BUFSIZE];
  char fs[CF_BUFSIZE];
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
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open %s!\n",VFSTAB[VSYSTEMHARDCLASS]);
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

   if (!GetLock(ASUniqueName("unmount"),CanonifyName(ptr->name),ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
      {
      ptr->done = 'y';
      continue;
      }

   fs[0] = '\0';

   sscanf(ptr->name,"%*[^:]:%s",fs);

   if (strlen(fs) == 0)
      {
      ReleaseCurrentLock();
      continue;
      }
   
   snprintf(OUTPUT,CF_BUFSIZE*2,"Unmount filesystem %s on %s\n",fs,ptr->name);
   CfLog(cfverbose,OUTPUT,"");

   if (strcmp(fs,"/") == 0 || strcmp(fs,"/usr") == 0)
      {
      CfLog(cfinform,"Request to unmount / or /usr is refused!\n","");
      ReleaseCurrentLock();     
      continue;
      }

   if (IsItemIn(VMOUNTED,ptr->name) && (! DONTDO))
      {
      snprintf(comm,CF_BUFSIZE,"%s %s",VUNMOUNTCOMM[VSYSTEMHARDCLASS],fs);

      if ((pp = cfpopen(comm,"r")) == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Failed to open pipe from %s\n",VUNMOUNTCOMM[VSYSTEMHARDCLASS]);
         CfLog(cferror,OUTPUT,"");
         ReleaseCurrentLock();     
         return;
         }
      
      ReadLine(VBUFF,CF_BUFSIZE,pp);
      
      if (strstr(VBUFF,"busy") || strstr(VBUFF,"Busy"))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"umount warned that the device under %s\n",ptr->name);
         CfLog(cfinform,OUTPUT,"");
         CfLog(cfinform,"was busy. Cannot unmount that device.\n","");
         /* don't delete the mount dir when unmount's failed */
         ptr->deletedir = 'n';
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Unmounting %s\n",ptr->name);
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Warning! %s was not a directory.\n",fs);
            CfLog(cfinform,OUTPUT,"");
            CfLog(cfinform,"(Unmount) will not delete this!\n","");
            KillOldLink(fs,NULL);
            }
         else if (! DONTDO)
            {
            if (rmdir(fs) == -1)
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to remove the directory %s\n",fs);
               CfLog(cferror,OUTPUT,"rmdir");
               } 
            else
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"Removing directory %s\n",ptr->name);
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Bad format in %s\n",VFSTAB[aix]);
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
                   snprintf(VBUFF,CF_BUFSIZE,"[^:]+:[^ \t]+[ \t]+[^ \t]+[ \t]+%s[ \t]",fs);
                   break;
               default:
                   /* find fs in proper context ("<host>:<remotepath> <fs> ") */
                   snprintf(VBUFF,CF_BUFSIZE,"[^:]+:[^ \t]+[ \t]+%s[ \t]",fs);
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

Debug("Editfiles()\n");
  
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
  
if (strcmp(VDOMAIN,"") == 0)
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
   snprintf(OUTPUT,CF_BUFSIZE*2,"Trying to create %s\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
   CfLog(cfinform,OUTPUT,"");
   existed = false;
   
   if ((fd = creat(VRESOLVCONF[VSYSTEMHARDCLASS],0644)) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to create file %s\n",VRESOLVCONF[VSYSTEMHARDCLASS]);
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

/* This code seems to solve an ancient problem that should no longer exist
for (ip = filebase; ip != NULL; ip=ip->next)
   {
   if (strlen(ip->name) == 0)
      {
      continue;
      }

   ch = *(ip->name+strlen(ip->name)-2);

   if (isspace((int)ch))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Deleted line %s ended with a space in %s.\n",ip->name,VRESOLVCONF[VSYSTEMHARDCLASS]);
      CfLog(cfinform,OUTPUT,"");
      CfLog(cfinform,"The resolver doesn't understand this.\n","");
      DeleteItem(&filebase,ip);
      }
   else if (isspace((int)*(ip->name)))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Deleted line %s started with a space in %s.\n",ip->name,VRESOLVCONF[VSYSTEMHARDCLASS]);
      CfLog(cfinform,OUTPUT,"");
      CfLog(cfinform,"The resolver doesn't understand this.\n","");
      DeleteItem(&filebase,ip);
      }
   }
*/

DeleteItemStarting(&filebase,"domain");

while(DeleteItemStarting(&filebase,"search"))
   {
   }

if (OptionIs(CONTEXTID,"EmptyResolvConf", true))
   {
   DeleteItemList(filebase);
   filebase = NULL;
   }
 
EditItemsInResolvConf(VRESOLVE,&filebase); 

snprintf(VBUFF,CF_BUFSIZE,"domain %s",ToLowerStr(VDOMAIN));
PrependItem(&filebase,VBUFF,NULL);
 
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
  char path[CF_EXPANDSIZE],destination[CF_EXPANDSIZE],vbuff[CF_BUFSIZE];
  char server[CF_EXPANDSIZE],listserver[CF_EXPANDSIZE];
  
for (svp = VSERVERLIST; svp != NULL; svp=svp->next) /* order servers */
   {
   CONN = NewAgentConn();  /* Global input/output channel */

   if (CONN == NULL)
      {
      return;
      }
   
   ExpandVarstring(svp->name,listserver,NULL);

   for (ip = VIMAGE; ip != NULL; ip=ip->next)
      {
      struct timespec start,stop;
      double dt = 0;
      int measured_ok = true;
      char eventname[CF_BUFSIZE];

      if (clock_gettime(CLOCK_REALTIME, &start) == -1)
         {
         CfLog(cfverbose,"Clock gettime failure","clock_gettime");
         measured_ok = false;
         }
      
      ExpandVarstring(ip->server,server,NULL);            
      AddMacroValue(CONTEXTID,"this",server);
      ExpandVarstring(ip->path,path,NULL);
      ExpandVarstring(ip->destination,destination,NULL);
      DeleteMacro(CONTEXTID,"this");

      if (strcmp(server,"none") == 0)
         {
         Verbose("Server none is a no-op\n");
         continue;
         }
      
      if (strcmp(listserver,server) != 0)  /* group together similar hosts so */
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to establish connection with %s (failover)\n",listserver);
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
      
      if (cfstat(path,&statbuf,ip) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s in copy\n",path);
         CfLog(cfverbose,OUTPUT,"");
         SILENT = savesilent;
         ResetOutputRoute('d','d');
         continue;
         }
      
      snprintf(vbuff,CF_BUFSIZE,"%.255s.%.50s_%.50s",path,destination,server); /* Unique ID for copy locking */
      
      if (!GetLock(ASUniqueName("copy"),CanonifyName(vbuff),ip->ifelapsed,ip->expireafter,VUQNAME,CFSTARTTIME))
         {
         SILENT = savesilent;
         ResetOutputRoute('d','d');
         ip->done = 'y';
         continue;
         }
      
      IMAGEBACKUP = ip->backup;
      
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
            RegisterRecursionRootDevice(statbuf.st_dev);
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

      if (clock_gettime(CLOCK_REALTIME, &stop) == -1)
         {
         CfLog(cfverbose,"Clock gettime failure","clock_gettime");
         measured_ok = false;
         }

      dt = (double)(stop.tv_sec - start.tv_sec)+(double)(stop.tv_nsec-start.tv_nsec)/(double)CF_BILLION;
      
      snprintf(eventname,CF_BUFSIZE-1,"Copy(%s:%s > %s)",server,path,destination);

      if (measured_ok)
         {
         RecordPerformance(eventname,start.tv_sec,dt);
         }
      }
   
   CloseServerConnection();
   DeleteAgentConn(CONN); 
   }
}

/*******************************************************************/

void ConfigureInterfaces()

{ struct Interface *ifp;

Banner("Network interface configuration");

if (GetLock("netconfig",VIFDEV[VSYSTEMHARDCLASS],VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   if (strlen(VNETMASK) != 0)
      {
      IfConf(VIFDEV[VSYSTEMHARDCLASS],VIPADDRESS,VNETMASK,VBROADCAST);
      }
   
   SetDefaultRoute();
   ReleaseCurrentLock();   
   }
    
for (ifp = VIFLIST; ifp != NULL; ifp=ifp->next)
   {
   if (!GetLock("netconfig",ifp->ifdev,VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
      {
      ifp->done = 'y';
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
   
   IfConf(ifp->ifdev,ifp->ipaddress,ifp->netmask,ifp->broadcast);
   SetDefaultRoute();
   ReleaseCurrentLock();
   }
}

/*******************************************************************/

void CheckTimeZone()

{ struct Item *ip;
  char tz[CF_MAXVARSIZE];
  
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

snprintf(OUTPUT,CF_BUFSIZE*2,"Time zone was %s which is not in the list of acceptable values",tz);
CfLog(cferror,OUTPUT,""); 
}

/*******************************************************************/

void CheckProcesses()

{ struct Process *pp;
  struct Item *procdata = NULL;
  char *psopts = VPSOPTS[VSYSTEMHARDCLASS];
  

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

   if (pp->done == 'y' || strcmp(pp->scope,CONTEXTID) != 0)
      {
      continue;
      }
   else
      {
      pp->done = 'y';
      }

   snprintf(VBUFF,CF_BUFSIZE-1,"proc-%s-%s",pp->expr,pp->restart);
   
   if (!GetLock(ASUniqueName("processes"),CanonifyName(VBUFF),pp->ifelapsed,pp->expireafter,VUQNAME,CFSTARTTIME))
      {
      pp->done = 'y';
      continue;
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
   ReleaseCurrentLock(); 
   }
}

/*******************************************************************/

void CheckPackages()

{ struct Package *ptr;
  int match = 0;
  char lock[CF_BUFSIZE];
  struct Item *pending_pkgs = NULL;
  enum pkgmgrs prev_pkgmgr = pkgmgr_none;
  enum pkgactions prev_action = pkgaction_none;

for (ptr = VPKG; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }
   
   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID) != 0)
      {
      continue;
      }
   
   snprintf(lock,CF_BUFSIZE-1,"%s_%d_%d",ptr->name,ptr->cmp,ptr->action);
   
   if (!GetLock(ASUniqueName("packages"),CanonifyName(lock),ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
      {
      ptr->done = 'y';
      continue;
      }
   
   match = PackageCheck(ptr->name, ptr->pkgmgr, ptr->ver, ptr->cmp);
   
   /* Check for a problem executing the command */
   
   if ((match != 1) && (match != 0))
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Error: Package manager query failed, skipping %s\n", ptr->name);
      CfLog(cferror,OUTPUT,"");
      ptr->done = 'y';
      continue;
      }
   
   /* Process any queued actions (install/remove). */

   if ((pending_pkgs != NULL) && ((ptr->action != prev_action) || (ptr->pkgmgr != prev_pkgmgr)))
      {
      ProcessPendingPackages(prev_pkgmgr, prev_action, &pending_pkgs);
      DeleteItemList( pending_pkgs );
      pending_pkgs = NULL; 
      }
   
/* Handle install/remove logic now. */
   if (match)
      {
      AddMultipleClasses(ptr->defines);

      if (ptr->action == pkgaction_remove) 
         {
         PackageList(ptr->name, ptr->pkgmgr, ptr->ver, ptr->cmp, &pending_pkgs);
         }
      else if (ptr->action == pkgaction_upgrade)
         {
         UpgradePackage( ptr->name, ptr->pkgmgr, ptr->ver, ptr->cmp );
         }
      }
   else
      {
      AddMultipleClasses(ptr->elsedef);

      if (ptr->action == pkgaction_install)
         {
         AppendItem(&pending_pkgs, ptr->name, NULL);
         
         /* Some package managers operate best doing things one at a time. */
         
         if ((ptr->pkgmgr == pkgmgr_freebsd) || (ptr->pkgmgr == pkgmgr_sun))
            {
            InstallPackage( ptr->pkgmgr, &pending_pkgs );
            DeleteItemList( pending_pkgs );
            pending_pkgs = NULL; 
            }
         }
      }
   
   ptr->done = 'y';
   ReleaseCurrentLock();
   
   if (ptr->action != pkgaction_none)
      {
      prev_action = ptr->action;
      prev_pkgmgr = ptr->pkgmgr;
      }
   }

if (pending_pkgs != NULL)
   {
   ProcessPendingPackages(prev_pkgmgr, prev_action, &pending_pkgs);
   DeleteItemList( pending_pkgs );
   pending_pkgs = NULL; 
   }
}

/*******************************************************************/

void DoMethods()

{ struct Method *ptr;
  struct Item *ip, *uniqueid;
  char label[CF_BUFSIZE];
  unsigned char digest[EVP_MAX_MD_SIZE+1];
    
Banner("Dispatching new methods");

for (ptr = VMETHODS; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   if (ptr->done == 'y' || strcmp(ptr->scope,CONTEXTID) != 0)
      {
      continue;
      }
   else
      {
      ptr->done = 'y';
      }

   uniqueid = NULL;
   
   CopyList(&uniqueid,ptr->send_args);
   /* Append server to make this unique */

   for (ip = ptr->servers; ip != NULL; ip=ip->next)
      {
      PrependItem(&uniqueid,ip->name,NULL);
      }

   ChecksumList(ptr->send_args,digest,'m');
   DeleteItemList(uniqueid);
   
   snprintf(label,CF_BUFSIZE-1,"%s/rpc_in/localhost+localhost+%s+%s",VLOCKDIR,ptr->name,ChecksumPrint('m',digest));

   if (!GetLock(ASUniqueName("methods-dispatch"),CanonifyName(label),ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
      {
      ptr->done = 'y';
      continue;
      }
   
   DispatchNewMethod(ptr);

   ptr->done = 'y';
   ReleaseCurrentLock();
   }

Banner("Evaluating incoming methods that policy accepts...");

for (ip = GetPendingMethods(CF_METHODEXEC); ip != NULL; ip=ip->next)
   {
   /* Call child process to execute method*/
   EvaluatePendingMethod(ip->name);
   }

DeleteItemList(ip);
 
Banner("Fetching replies to finished methods");
 
for (ip = GetPendingMethods(CF_METHODREPLY); ip != NULL; ip=ip->next)
   { 
   if (ParentLoadReplyPackage(ip->name))
      {
      }
   }

DeleteItemList(ip); 
}

/*******************************************************************/
/* Level 2                                                         */
/*******************************************************************/

int RequiredFileSystemOkay (char *name)

{ struct stat statbuf, localstat;
  DIR *dirh;
  struct dirent *dirp;
  long sizeinbytes = 0, filecount = 0;
  char buff[CF_BUFSIZE];

Debug("Checking required filesystem %s\n",name);

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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s which checking required/disk\n",name);
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

         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s in required/disk\n",buff);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"File system %s is suspiciously small! (%d bytes)\n",name,sizeinbytes);
      CfLog(cferror,OUTPUT,"");
      return(false);
      }

   if (filecount < SENSIBLEFILECOUNT)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Filesystem %s has only %d files/directories.\n",name,filecount);
      CfLog(cferror,OUTPUT,"");
      return(false);
      }
   }

return(true);
}


/*******************************************************************/

int ScanFileSystemArrivals(char *name,int rlevel,struct stat *sb,DB *dbp)

{ DIR *dirh;
  int goback; 
  struct dirent *dirp;
  char pcwd[CF_BUFSIZE];
  struct stat statbuf;
  
if (rlevel > CF_RECURSION_LIMIT)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING: Very deep nesting of directories (>%d deep): %s (Aborting files)",rlevel,name);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
 
memset(pcwd,0,CF_BUFSIZE); 

Debug("ScanFileSystemArrivals(%s)\n",name);

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
      snprintf(OUTPUT,CF_BUFSIZE*2,"RecursiveCheck was looking at %s when this happened:\n",pcwd);
      CfLog(cferror,OUTPUT,"lstat");
      continue;
      }
   
   if (DeviceChanged(statbuf.st_dev))
      {
      Verbose("Skipping %s on different device\n",pcwd);
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
         RecordFileSystemArrivals(dbp,sb->st_mtime);
         goback = ScanFileSystemArrivals(pcwd,rlevel+1,&statbuf,dbp);
         DirPop(goback,name,sb);
         }
      }
   else
      {
      RecordFileSystemArrivals(dbp,sb->st_mtime);
      }
   }

closedir(dirh);
return true; 
}

/*******************************************************************/

void InstallMountedItem(char *host,char *mountdir)

{ char buf[CF_BUFSIZE];
 
strcpy (buf,host);
strcat (buf,":");
strcat (buf,mountdir);
 
if (IsItemIn(VMOUNTED,buf))
   {
   if (! SILENT || !WARNINGS)
      {
      if (!strstr(buf,"swap"))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING mount item %s\n",buf);
         CfLog(cferror,OUTPUT,"");
         CfLog(cferror,"is mounted multiple times!\n","");
         }
      }
   }

 AppendItem(&VMOUNTED,buf,NULL);
}

/*******************************************************************/

void AddToFstab(char *host,char *rmountpt,char *mountpt,char *mode,char *options,int ismounted)

{ char fstab[CF_BUFSIZE];
  char *opts;
  FILE *fp;
  char aix_lsnfsmnt[CF_BUFSIZE];

Debug("AddToFstab(%s)\n",mountpt);

if (mode == NULL)
   {
   mode = "rw";
   }

if ((options != NULL) && (strlen(options) > 0))
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
   case qnx:
   case sun4:    snprintf(fstab,CF_BUFSIZE,"%s:%s \t %s %s\t%s,%s 0 0",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case crayos:
                 snprintf(fstab,CF_BUFSIZE,"%s:%s \t %s %s\t%s,%s",host,rmountpt,mountpt,ToUpperStr(VNFSTYPE),mode,opts);
                 break;
   case ultrx:   snprintf(fstab,CF_BUFSIZE,"%s@%s:%s:%s:0:0:%s:%s",rmountpt,host,mountpt,mode,VNFSTYPE,opts);
                 break;
   case hp:      snprintf(fstab,CF_BUFSIZE,"%s:%s %s \t %s \t %s,%s 0 0",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;
   case aix:     snprintf(fstab,CF_BUFSIZE,"%s:\n\tdev\t= %s\n\ttype\t= %s\n\tvfs\t= %s\n\tnodename\t= %s\n\tmount\t= true\n\toptions\t= %s,%s\n\taccount\t= false\n",mountpt,rmountpt,VNFSTYPE,VNFSTYPE,host,mode,opts);
		 snprintf(aix_lsnfsmnt, CF_BUFSIZE, 
				 "%s:%s:%s:%s:%s",
				 mountpt, rmountpt, host, VNFSTYPE, mode
			 );
                 break;
   case GnU:
   case linuxx:  snprintf(fstab,CF_BUFSIZE,"%s:%s \t %s \t %s \t %s,%s",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case netbsd:
   case openbsd:
   case bsd_i:
   case freebsd: snprintf(fstab,CF_BUFSIZE,"%s:%s \t %s \t %s \t %s,%s 0 0",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case unix_sv:
   case solarisx86:
   case solaris: snprintf(fstab,CF_BUFSIZE,"%s:%s - %s %s - yes %s,%s",host,rmountpt,mountpt,VNFSTYPE,mode,opts);
                 break;

   case cfnt:    snprintf(fstab,CF_BUFSIZE,"/bin/mount %s:%s %s",host,rmountpt,mountpt);
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
   if (VSYSTEMHARDCLASS == aix) /* This AIX code by Graham Bevan */
      {
      FILE *pp;
      int fs_found = 0;
      int fs_changed = 0;
      char comm[CF_BUFSIZE];

      if ((pp = cfpopen("/usr/sbin/lsnfsmnt -c", "r")) == NULL)
         {
         CfLog(cferror,"Failed to open pipe to /usr/sbin/lsnfsmnt command.", "");
         return;
         }
      
      while(!feof(pp))
         {
         ReadLine(VBUFF, CF_BUFSIZE, pp);
         if (VBUFF[0] == '#')
            {
            continue;
            }

         if (strstr(VBUFF,mountpt))
            {
            fs_found++;
            if (!strstr(VBUFF,aix_lsnfsmnt))
               {
               fs_changed = 1;
               }
            }
         }
      fclose(pp);
      
      if (fs_found == 1 && !fs_changed)
         {
         return;
         }
      else
         {
         /* if entry not found, duplicates found or entry is different from lookup string
          * then remove for re-add */
         int failed = 0;
         snprintf(OUTPUT,CF_BUFSIZE*2,"Removing \"%s\" entry from %s to allow update (fs_found=%d):\n",
                  mountpt,
                  VFSTAB[VSYSTEMHARDCLASS],
                  fs_found
                  );

         CfLog(cfinform,OUTPUT,"");
         CfLog(cfinform,"---------------------------------------------------","");

         snprintf(comm, CF_BUFSIZE, "/usr/sbin/rmnfsmnt -f %s", mountpt);

         if ((pp = cfpopen(comm,"r")) == NULL)
            {
            CfLog(cferror,"Failed to open pipe to /usr/sbin/rmnfsmnt command.", "");
            return;
            }
         
         while(!feof(pp))
            {
            ReadLine(VBUFF, CF_BUFSIZE, pp);
            if (VBUFF[0] == '#')
               {
               continue;
               }
            if (strstr(VBUFF,"busy"))
               {
               snprintf(OUTPUT,CF_BUFSIZE*2,"umount warned that the device under %s\n",mountpt);
               CfLog(cfinform,OUTPUT,"");
               CfLog(cfinform,"was busy. Cannot unmount (rmnfsmnt) that device.\n","");
               failed = 1;
               }
            }
         
         if (fclose(pp) != 0)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"rmnfsmnt failed on fclose() for %s: %s\n",mountpt, strerror(errno));
            CfLog(cferror,OUTPUT,"");
            return;
            }
         
         if (failed)
            {
            return;
            }
         }
      
      } /* if aix */
   else
      { 
      /* if the fstab entry has changed, remove the old entry and update */
      if (!MatchStringInFstab(fstab))
         { struct UnMount *saved_VUNMOUNT = VUNMOUNT;
           char mountspec[MAXPATHLEN];
           struct Item *mntentry = NULL;
           struct UnMount cleaner;
         
         snprintf(OUTPUT,CF_BUFSIZE*2,"Removing \"%s\" entry from %s to allow update:\n",mountpt,VFSTAB[VSYSTEMHARDCLASS]);
         CfLog(cfinform,OUTPUT,"");
         CfLog(cfinform,"---------------------------------------------------","");
         
         /* delete current fstab entry and unmount if necessary */
         snprintf(mountspec,CF_BUFSIZE,".+:%s",mountpt);
         mntentry = LocateItemContainingRegExp(VMOUNTED,mountspec);
         if (mntentry)
            {
            sscanf(mntentry->name,"%[^:]:",mountspec);  /* extract current host */
            strcat(mountspec,":");
            strcat(mountspec,mountpt);
            }
         else  /* mountpt isn't mounted, so Unmount can use dummy host name */
             snprintf(mountspec,CF_BUFSIZE,"host:%s",mountpt);
         
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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Warning the file system %s seems to be in %s\n",mountpt,VFSTAB[VSYSTEMHARDCLASS]);
            CfLog(cfinform,OUTPUT,"");
            snprintf(OUTPUT,CF_BUFSIZE*2,"already, but I was not able to mount it.\n");
            CfLog(cfinform,OUTPUT,"");
            snprintf(OUTPUT,CF_BUFSIZE*2,"Check the exports file on host %s? Check for file with same name as dir?\n",host);
            CfLog(cfinform,OUTPUT,"");
            }
         
         return;
         }
      }
   } /* if aix */
 
 if (DONTDO)
    {
    printf("%s: add filesystem to %s\n",VPREFIX,VFSTAB[VSYSTEMHARDCLASS]);
    printf("%s: %s\n",VPREFIX,fstab);
    }
 else
    {
    struct Item *filelist = NULL;

    if (! LoadItemList(&filelist,VFSTAB[VSYSTEMHARDCLASS]))
       {
       snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open %s!\n",VFSTAB[VSYSTEMHARDCLASS]);
       CfLog(cferror,OUTPUT,"");
       return;
       }
    
    NUMBEROFEDITS = 0;
    
    snprintf(OUTPUT,CF_BUFSIZE*2,"Adding filesystem to %s\n",VFSTAB[VSYSTEMHARDCLASS]);
    CfLog(cfinform,OUTPUT,"");
    snprintf(OUTPUT,CF_BUFSIZE*2,"%s\n",fstab);
    CfLog(cfinform,OUTPUT,"");

    if (!IsItemIn(filelist,fstab))
       {
       AppendItem(&filelist,fstab,NULL);
       }

    SaveItemList(filelist,VFSTAB[VSYSTEMHARDCLASS],VREPOSITORY);
    
    chmod(VFSTAB[VSYSTEMHARDCLASS],DEFAULTSYSTEMMODE);
    }
}


/*******************************************************************/

int CheckFreeSpace (char *file,struct Disk *disk_ptr)

{ struct stat statbuf;
  int free;
  int kilobytes;

if (stat(file,&statbuf) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't stat %s checking diskspace\n",file);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Free disk space is under %d%% for partition\n",kilobytes);
      CfLog(cfinform,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"containing %s (%d%% free)\n",file,free);
      CfLog(cfinform,OUTPUT,"");
      return false;
      }
   }
else
   {
   free = GetDiskUsage(file, cfabs);

   if (free < kilobytes)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Disk space under %d kB for partition\n",kilobytes);
      CfLog(cfinform,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"containing %s (%d kB free)\n",file,free);
      CfLog(cfinform,OUTPUT,"");
      return false;
      }
   }

return true;
}

/*******************************************************************/

void CheckHome(struct File *ptr)                      /* iterate check over homedirs */

{ struct Item *ip1, *ip2;
  char basename[CF_EXPANDSIZE],pathbuff[CF_BUFSIZE];

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
            snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal use of home in files: %s\n",ptr->path);
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

void EditItemsInResolvConf(struct Item *from,struct Item **list)

{ char buf[CF_MAXVARSIZE],work[CF_EXPANDSIZE];
 struct Item *ip;

 for (ip = from; ip != NULL; ip=ip->next)
    {
    if (IsExcluded(ip->classes))
       {
       continue;
       }
    
    ExpandVarstring(ip->name,work,"");
    
    if (isdigit((int)*(work)))
       {
       snprintf(buf,CF_MAXVARSIZE,"nameserver %s",work);
       }
    else
       {
       strncpy(buf,work,CF_MAXVARSIZE-1);
       }
    
    DeleteItemMatching(list,buf); /* del+prep = move to head of list */
    PrependItem(list,buf,NULL);
    }
}


/*******************************************************************/

int TZCheck(char *tzsys,char *tzlist)

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

void ExpandWildCardsAndDo(char *wildpath,char *buffer,void (*function)(char *path, void *ptr),void *argptr)
 
 /* This function recursively expands a path containing wildcards */
 /* and executes the function pointed to by function for each     */
 /* matching file or directory                                    */

 
{ char *rest, extract[CF_BUFSIZE], construct[CF_BUFSIZE],varstring[CF_EXPANDSIZE],cleaned[CF_BUFSIZE], *work;
  struct stat statbuf;
  DIR *dirh;
  struct dirent *dp;
  int count, isdir = false,i,j;

varstring[0] = '\0';

memset(cleaned,0,CF_BUFSIZE); 
 
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
       snprintf(OUTPUT,CF_BUFSIZE*2,"Culprit %s\n",extract);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open dir: %s\n",buffer);
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
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n\n",buffer);
         CfLog(cferror,OUTPUT,"stat");
         continue;
         }
 
      if ((S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) && WildMatch(extract,dp->d_name))
         {
         ExpandWildCardsAndDo(rest,buffer,function,argptr);
         } 
      }
 
   if (count == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"No directories matching %s in %s\n",extract,buffer);
      CfLog(cfinform,OUTPUT,"");
      return;
      }
   closedir(dirh);
   }
}

/*******************************************************************/

void RecFileCheck(char *startpath,void *vp)


{ struct File *ptr;
  struct stat sb; 

ptr = (struct File *)vp;

Verbose("%s: Checking files in %s...\n",VPREFIX,startpath);

if (!GetLock(ASUniqueName("files"),startpath,ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
   {
   return;
   }
 
if (stat(startpath,&sb) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Directory %s cannot be accessed in files",startpath);
   CfLog(cfinform,OUTPUT,"stat");
   ReleaseCurrentLock(); 
   return;
   }

CheckExistingFile("*",startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,&sb,ptr,ptr->acl_aliases);
RecursiveCheck(startpath,ptr->plus,ptr->minus,ptr->action,ptr->uid,ptr->gid,ptr->recurse,0,ptr,&sb);
 
ReleaseCurrentLock(); 
}

/*******************************************************************/

void ProcessPendingPackages (enum pkgmgrs pkgmgr, enum pkgactions action, struct Item **pending_pkgs)
{
switch(action)
   {
   case pkgaction_remove:
       RemovePackage(pkgmgr, pending_pkgs);
       break;
   case pkgaction_install:
       InstallPackage(pkgmgr, pending_pkgs);
       break;
   default:
       snprintf(OUTPUT,CF_BUFSIZE,"Internal error!  Tried to process package with an unknown action: %d.  This should never happen!\n", action);
       CfLog(cferror,OUTPUT,"");
       break; 
   }
}


/*******************************************************************/
/* Level 3                                                         */
/*******************************************************************/

void RecordFileSystemArrivals(DB *dbp,time_t mtime)

{ DBT key,value;
  char *keyval = strdup(ConvTimeKey(ctime(&mtime)));
  double new = 0,old = 0;

Debug("Record fs hit at %s\n",keyval);  
memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
      
key.data = keyval; 
key.size = strlen(keyval)+1;

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp,errno,NULL);
      free(keyval);
      return;
      }

   old = 0.0;
   }
else
   {
   memcpy(&old,value.data,sizeof(double));
   }

new = old + 0.5; /* Arbitrary counting scale (x+(x+1))/2
       becomes like principal value / weighted av */ 

key.data = keyval;
key.size = strlen(keyval)+1;

value.data = (void *) &new;
value.size = sizeof(double);

if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
   {
   CfLog(cferror,"db->put failed","db->put");
   }
 
free(keyval); 
}



/*******************************************************************/
/* Toolkit fstab                                                   */
/*******************************************************************/

int MatchStringInFstab(char *str)

{ FILE *fp;

if ((fp = fopen(VFSTAB[VSYSTEMHARDCLASS],"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open %s for reading\n",VFSTAB[VSYSTEMHARDCLASS]);
   CfLog(cferror,OUTPUT,"fopen");
   return true; /* write nothing */
   }

while (!feof(fp))
   {
   ReadLine(VBUFF,CF_BUFSIZE,fp);

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

/* vim:expandtab:smarttab:sw=3 */
