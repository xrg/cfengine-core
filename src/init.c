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
 
/*****************************************************************************/
/*                                                                           */
/* File: init.c                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include "../pub/global.h"

/*********************************************************************/

void CheckWorkDirectories()

{ struct stat statbuf;
  char *sp;

Debug("CheckWorkDirectories()\n");  
snprintf(LOGFILE,bufsize,"%s/cfagent.%s.log",VLOGDIR,VUQNAME);
VSETUIDLOG = strdup(LOGFILE); 
 
if (!IsPrivileged())
   {
   Verbose("\n(Non privileged user...)\n\n");
   
   if ((sp = getenv("HOME")) == NULL)
      {
      FatalError("You do not have a HOME variable pointing to your home directory");
      }

   snprintf(VLOGDIR,bufsize,"%s/.cfengine",sp);
   snprintf(VLOCKDIR,bufsize,"%s/.cfengine",sp);
   snprintf(VBUFF,bufsize,"%s/.cfagent/test",sp);
   MakeDirectoriesFor(VBUFF,'y');
   snprintf(CFPRIVKEYFILE,bufsize,"%s/.cfengine/ppkeys/localhost.priv",sp);
   snprintf(CFPUBKEYFILE,bufsize,"%s/.cfengine/ppkeys/localhost.pub",sp);
   }
else
   {
   snprintf(VBUFF,bufsize,"%s/test",VLOCKDIR);
   snprintf(VBUFF,bufsize,"%s/test",VLOGDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(CFPRIVKEYFILE,bufsize,"%s/ppkeys/localhost.priv",WORKDIR);
   snprintf(CFPUBKEYFILE,bufsize,"%s/ppkeys/localhost.pub",WORKDIR);
   }

snprintf(VBUFF,bufsize,"%s/ppkeys/test",VLOCKDIR);
MakeDirectoriesFor(VBUFF,'n');
snprintf(VBUFF,bufsize,"%s/ppkeys",VLOCKDIR); 
chmod(VBUFF,(mode_t)0700); /* Locks must be immutable to others */ 

if (stat(VBUFF,&statbuf) == -1)
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,bufsize*2,"Lock/Log directory %s was not private! Mode %o - setting to 700\n",VLOCKDIR,statbuf.st_mode & 0777);
      CfLog(cferror,OUTPUT,"");
      }
   }

chown(VLOCKDIR,0,0);
chmod(VLOCKDIR,(mode_t)0755); /* Locks must be immutable to others */
}

/**********************************************************************/

void RandomSeed()

{ static unsigned char digest[EVP_MAX_MD_SIZE+1];
  struct stat statbuf;
  
/* Use the system database as the entropy source for random numbers */

Debug("RandomSeed() work directory is %s\n",VLOGDIR);

snprintf(VBUFF,bufsize,"%s/randseed",VLOGDIR); 

 if (stat(VBUFF,&statbuf) == -1)
    {
    snprintf(AVDB,bufsize,"%s/%s",WORKDIR,AVDB_FILE);
    }
 else
    {
    strcpy(AVDB,VBUFF);
    }

Verbose("Looking for a source of entropy in %s\n",AVDB);

if (!RAND_load_file(AVDB,-1))
   {
   snprintf(OUTPUT,bufsize,"Could not read sufficient randomness from %s\n",AVDB);
   CfLog(cfverbose,OUTPUT,"");
   }

while (!RAND_status())
   {
   MD5Random(digest);
   RAND_seed((void *)digest,16);
   }
}

/**********************************************************************/

void SetSignals()


{ int i;

 SIGNALS[SIGHUP] = strdup("SIGHUP");
 SIGNALS[SIGINT] = strdup("SIGINT");
 SIGNALS[SIGTRAP] = strdup("SIGTRAP");
 SIGNALS[SIGKILL] = strdup("SIGKILL");
 SIGNALS[SIGPIPE] = strdup("SIGPIPE");
 SIGNALS[SIGCONT] = strdup("SIGCONT");
 SIGNALS[SIGINT] = strdup("SIGINT");
 SIGNALS[SIGABRT] = strdup("SIGABRT");
 SIGNALS[SIGSTOP] = strdup("SIGSTOP");
 SIGNALS[SIGQUIT] = strdup("SIGQUIT");
 SIGNALS[SIGTERM] = strdup("SIGTERM");
 SIGNALS[SIGCHLD] = strdup("SIGCHLD");
 SIGNALS[SIGUSR1] = strdup("SIGUSR1");
 SIGNALS[SIGUSR2] = strdup("SIGUSR2");
 SIGNALS[SIGBUS] = strdup("SIGBUS");
 SIGNALS[SIGSEGV] = strdup("SIGSEGV");

 for (i = 0; i < highest_signal; i++)
    {
    if (SIGNALS[i] == NULL)
       {
       SIGNALS[i] = strdup("NOSIG");
       }
    }
}

