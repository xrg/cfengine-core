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

 if (uname(&VSYSNAME) == -1)
   {
   perror("uname ");
   FatalError("Uname couldn't get kernel name info!!\n");
   }
 
snprintf(LOGFILE,bufsize,"%s/cfagent.%s.log",VLOGDIR,VSYSNAME.nodename);
VSETUIDLOG = strdup(LOGFILE); 
 
if (!IsPrivileged())
   {
   Verbose("\n(Non privileged user...)\n\n");
   
   if ((sp = getenv("HOME")) == NULL)
      {
      FatalError("You do not have a HOME variable pointing to your home directory");
      }

   snprintf(VLOGDIR,bufsize,"%s/.cfagent",sp);
   snprintf(VLOCKDIR,bufsize,"%s/.cfagent",sp);
   snprintf(VBUFF,bufsize,"%s/.cfagent/test",sp);
   MakeDirectoriesFor(VBUFF,'y');
   snprintf(VBUFF,bufsize,"%s/.cfagent/state/test",sp);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(CFPRIVKEYFILE,bufsize,"%s/.cfagent/ppkeys/localhost.priv",sp);
   snprintf(CFPUBKEYFILE,bufsize,"%s/.cfagent/ppkeys/localhost.pub",sp);
   }
else
   {
   snprintf(VBUFF,bufsize,"%s/test",VLOCKDIR);
   snprintf(VBUFF,bufsize,"%s/test",VLOGDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,bufsize,"%s/state/test",VLOGDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(CFPRIVKEYFILE,bufsize,"%s/ppkeys/localhost.priv",WORKDIR);
   snprintf(CFPUBKEYFILE,bufsize,"%s/ppkeys/localhost.pub",WORKDIR);
   }

Verbose("Checking integrity of the state database\n");
snprintf(VBUFF,bufsize,"%s/state",VLOCKDIR);
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,bufsize,"%s/state",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   chown(VBUFF,getuid(),getgid());
   chmod(VBUFF,(mode_t)0755);
   }
else 
   {
   if (statbuf.st_mode & 022)
      {
      snprintf(OUTPUT,bufsize*2,"UNTRUSTED: State directory %s was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      CfLog(cferror,OUTPUT,"");
      }
   }

Verbose("Checking integrity of the module directory\n"); 

snprintf(VBUFF,bufsize,"%s/modules",VLOCKDIR);
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,bufsize,"%s/modules/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,bufsize,"%s/modules",VLOCKDIR);
   chown(VBUFF,getuid(),getgid());
   chmod(VBUFF,(mode_t)0700);
   }
else 
   {
   if (statbuf.st_mode & 022)
      {
      snprintf(OUTPUT,bufsize*2,"UNTRUSTED: Module directory %s was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      CfLog(cferror,OUTPUT,"");
      }
   }

Verbose("Checking integrity of the input data for RPC\n"); 

snprintf(VBUFF,bufsize,"%s/rpc_in",VLOCKDIR);

if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,bufsize,"%s/rpc_in/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,bufsize,"%s/rpc_in",VLOCKDIR);
   chown(VBUFF,getuid(),getgid());
   chmod(VBUFF,(mode_t)0700);
   }
else 
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,bufsize*2,"UNTRUSTED: RPC input directory %s was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      FatalError(OUTPUT);
      }
   }

Verbose("Checking integrity of the output data for RPC\n"); 

snprintf(VBUFF,bufsize,"%s/rpc_out",VLOCKDIR);
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,bufsize,"%s/rpc_out/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,bufsize,"%s/rpc_out",VLOCKDIR);
   chown(VBUFF,getuid(),getgid());
   chmod(VBUFF,(mode_t)0700);   
   }
else
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,bufsize*2,"UNTRUSTED: RPC output directory %s was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      FatalError(OUTPUT);
      }
   }
 
Verbose("Checking integrity of the PKI directory\n");
snprintf(VBUFF,bufsize,"%s/ppkeys",VLOCKDIR);
    
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,bufsize,"%s/ppkeys/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,bufsize,"%s/ppkeys",VLOCKDIR); 
   chmod(VBUFF,(mode_t)0700); /* Locks must be immutable to others */    
   }
else
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,bufsize*2,"UNTRUSTED: Private key directory %s was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      FatalError(OUTPUT);
      }
   }

Verbose("Making sure that locks are private...\n"); 
chown(VLOCKDIR,getuid(),getgid());
chmod(VLOCKDIR,(mode_t)0755); /* Locks must be immutable to others */
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

/**********************************************************************/

void ActAsDaemon(int preserve)
{
   int fd, maxfd;
#ifdef HAVE_SETSID
   setsid();
#endif 
   closelog();
   fflush(NULL);
   fd = open("/dev/null", O_RDWR, 0);
   if (fd != -1)
      {
      dup2(fd,STDIN_FILENO);
      dup2(fd,STDOUT_FILENO);
      dup2(fd,STDERR_FILENO);
      if (fd > STDERR_FILENO) close(fd);
      }

#ifdef HAVE_SYSCONF
   maxfd = sysconf(_SC_OPEN_MAX);
#else
# ifdef _POXIX_OPEN_MAX
   maxfd = _POSIX_OPEN_MAX;
# else
   maxfd = 1024;
# endif
#endif

for (fd=STDERR_FILENO+1; fd<maxfd; ++fd)
   {
   if (fd != preserve) close(fd);
   }
}
