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
/**********************************************************************/

void TruncateFile(name)

char *name;

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
      snprintf(OUTPUT,bufsize*2,"creat(%s) failed\n",name);
      CfLog(cferror,OUTPUT,"creat");
      }
   else
      {
      close(fd);
      }
   }
}

/*************************************************************************/

int FileSecure (name)

char *name;

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
   snprintf(OUTPUT,bufsize*2,"File %s is not owned by uid %d (security exception)",name,getuid());
   CfLog(cferror,OUTPUT,"");
   }
 
/* Is the file writable by ANYONE except the owner ? */
 
if (statbuf.st_mode & (S_IWGRP | S_IWOTH))
   {
   snprintf(OUTPUT,bufsize*2,"File %s (owner %d) is writable by others (security exception)",name,getuid());
   CfLog(cferror,OUTPUT,"");
   return false;
   }

return true; 
}

/***************************************************************/

int ChecksumChanged(filename,digest,warnlevel,refresh,type)

    /* Returns false if filename never seen before, and adds a checksum
       to the database. Returns true if checksums do not match and also
       updates database to the new value */
    
char *filename, type;
unsigned char digest[EVP_MAX_MD_SIZE+1];
int warnlevel,refresh;

{ struct stat stat1, stat2;
  int i,needupdate = false, size = 21;
  unsigned char dbvalue[EVP_MAX_MD_SIZE+1],dbdigest[EVP_MAX_MD_SIZE+1];
  DBT key,value;
  DB *dbp;
  DB_ENV *dbenv = NULL;

Debug("ChecksumChanged: key %s with data %s\n",filename,ChecksumPrint(type,digest));
 
switch(type)  /* Size of message digests */
   {
   case 'm': size = CF_MD5_LEN;
       break;
   case 's': size = CF_SHA_LEN;
       break;
   }
 
if (CHECKSUMDB == NULL)
   {
   if (ISCFENGINE)
      {
      CfLog(cferror,"ChecksumDatabase = (<undefined>) in control:","");
      return false;
      }
   else
      {
      Debug("Direct comparison (no db)\n");
      ChecksumFile(filename,dbdigest,type);
      
      for (i = 0; i < size; i++)
	 {
	 if (digest[i] != dbdigest[i])
	    {
	    return true;
	    }
	 }
      return false;
      }
   }

if (refresh)
   {
   /* Check whether database is current wrt local file */

   if (stat(filename,&stat1) == -1)
      {
      snprintf(OUTPUT,bufsize*2,"Couldn't stat %s\n",filename);
      CfLog(cferror,OUTPUT,"stat");
      return false;
      }
   
   if (stat(CHECKSUMDB,&stat2) != -1)
      {
      if (stat1.st_mtime > stat2.st_mtime)
	 {
	 Debug("Checksum database is older than %s...refresh needed\n",filename);
	 needupdate = true;
	 }
      else
	 {
	 Debug("Checksum up to date..\n");
	 }
      }
   else
      {
      needupdate = true;
      }      
   }
 
if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
   CfLog(cferror,OUTPUT,"db_open");
   return false;
   }
 
if ((errno = dbp->open(dbp,CHECKSUMDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return false;
   }

bzero(&value,sizeof(value)); 
bzero(&key,sizeof(key));       
      
key.data = filename;
key.size = strlen(filename)+1;
value.data = dbvalue;
value.size = size; 

if (needupdate)
   {
   bzero(dbdigest,EVP_MAX_MD_SIZE+1);
   ChecksumFile(filename,dbdigest,type);
   
   key.data = filename;
   key.size = strlen(filename)+1;
   value.data = (void *) dbdigest;
   value.size = size;
   
   Debug("Updating checksum for %s to %s\n",filename,ChecksumPrint(type,value.data));
   
   if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
      {
      CfLog(cferror,"","db_store");
      }
  
   key.data = filename;
   key.size = strlen(filename)+1;
   
   if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
      {
      CfLog(cferror,"put failed","db->put");
      }      
   }

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) == 0)
   {
   /* The filename key was found in the db */
   Debug("Comparing %s (sent)\n",ChecksumPrint(type,digest));
   Debug("     with %s (db)\n",ChecksumPrint(type,value.data));

   bzero(dbdigest,EVP_MAX_MD_SIZE+1);
   bcopy(value.data,dbdigest,size);
   
   for (i = 0; i < size; i++)
      {
      if (digest[i] != dbdigest[i])
	 {
	 Debug("Found checksum for %s in database but it didn't match\n",filename);

	 if (EXCLAIM)
	    {
	    CfLog(warnlevel,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!","");
	    }

	 snprintf(OUTPUT,bufsize*2,"SECURITY ALERT: Checksum for %s changed!",filename);
	 CfLog(warnlevel,OUTPUT,"");

	 if (EXCLAIM)
	    {
	    CfLog(warnlevel,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!","");
	    }

	 if (CHECKSUMUPDATES)
	    {
	    bzero(dbdigest,EVP_MAX_MD_SIZE+1);
	    ChecksumFile(filename,dbdigest,type);
	    
	    key.data = filename;
	    key.size = strlen(filename)+1;
	    value.data = (void *) dbdigest;
	    value.size = size;
	    
	    Verbose("Updating checksum for %s to %s\n",filename,ChecksumPrint(type,value.data));
	    
	    if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
	       {
	       CfLog(cferror,"","db->del");
	       }
	    
	    key.data = filename;
	    key.size = strlen(filename)+1;
	    
	    if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
	       {
	       CfLog(cferror,"put failed","db->put");
	       }
	    }

	 dbp->close(dbp,0);
	 return true;                        /* Checksum updated but was changed */
	 }
      }
   
   Debug("Found checksum for %s in database and it matched\n",filename);
   dbp->close(dbp,0);
   return false;
   }
else
   {
   /* Key was not found, so install it */

   if (ISCFENGINE)
      {
      snprintf(OUTPUT,bufsize,"File %s was not in md5 database - new file found",filename);
      CfLog(cfsilent,OUTPUT,"");
      }
   
   bzero(dbdigest,EVP_MAX_MD_SIZE+1);
   ChecksumFile(filename,dbdigest,type);

   key.data = filename;
   key.size = strlen(filename)+1;
   value.data = (void *) dbdigest;
   value.size = size;

   Debug("Storing checksum for %s in database %s\n",filename,ChecksumPrint(type,dbdigest));

   if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
      {
      CfLog(cferror,"put failed","db->put");
      }
   
   dbp->close(dbp,0);

   if (ISCFENGINE)
      {
      return false;      /* No need to warn when first installed */
      }
   else
      {
      return true;
      }
   }
}

/*************************************************************************/

int IgnoredOrExcluded(action,file,inclusions,exclusions)

enum actions action;
char *file;
struct Item *inclusions, *exclusions;

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

if (inclusions != NULL && !IsWildItemIn(inclusions,lastnode))
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
   case links:
              if (IsWildItemIn(VEXCLUDELINK,lastnode) || IsWildItemIn(exclusions,lastnode))
                 {
                 Debug("Skipping excluded pattern %s\n",file);
                 return true;
                 }
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

void Banner(string)

char *string;

{
Verbose("---------------------------------------------------------------------\n");
Verbose("%s\n",string);
Verbose("---------------------------------------------------------------------\n\n");
}

/*********************************************************************/

void DebugBinOut(buffer,len)

char *buffer;
int len;

{ char *sp;

Debug("BinaryBuffer[");
 
for (sp = buffer; (*sp != '\0') && (sp < buffer+len); sp++)
   {
   Debug("%x",*sp);
   }
 
Debug("]\n"); 
}

/*********************************************************************/

char *ChecksumPrint(type,digest)

char type;
unsigned char digest[EVP_MAX_MD_SIZE+1];

{ unsigned int i;
  static char buffer[EVP_MAX_MD_SIZE*4];
  int len = 16;

switch(type)
   {
   case 's': sprintf(buffer,"SHA=  ");
       len = 20;
       break;
   case 'm': sprintf(buffer,"MD5=  ");
       len = 16;
       break;
   }
  
for (i = 0; i < len; i++)
   {
   sprintf((char *)(buffer+4+2*i),"%02x", digest[i]);
   }

return buffer; 
}    

/*******************************************************************/

int ShellCommandReturnsZero(comm)

char *comm;

{ int status, i, argc;
  pid_t pid;
  char arg[maxshellargs][bufsize];
  char **argv;

/* Build argument array */

for (i = 0; i < maxshellargs; i++)
   {
   bzero (arg[i],bufsize);
   }

argc = SplitCommand(comm,arg);

if (argc == -1)
   {
   snprintf(OUTPUT,bufsize,"Too many arguments in %s\n",comm);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
    
if ((pid = fork()) < 0)
   {
   FatalError("Failed to fork new process");
   }
else if (pid == 0)                     /* child */
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
   }
else                                    /* parent */
   {
   if (wait(&status) != pid)
      {
      snprintf(OUTPUT,bufsize,"Wait for child failed\n");
      CfLog(cfinform,OUTPUT,"wait");
      return false;
      }
   else
      {
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
	 Debug("Shell command was non-zero\n");
         return false;
         }
      }
   }

return false;
}

/*********************************************************************/

void SetClassesOnScript(execstr,classes,elseclasses,useshell)

char *execstr, *classes, *elseclasses;
int useshell;

{ FILE *pp;
  int print;
  char line[bufsize],*sp;
 
switch (useshell)
   {
   case 'y':  pp = cfpopen_sh(execstr,"r");
              break;
   default:   pp = cfpopen(execstr,"r");
              break;	     
   }
 
if (pp == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open pipe to command %s\n",execstr);
   CfLog(cferror,OUTPUT,"popen");
   return;
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
