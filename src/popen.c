
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
/* File: popen.c                                                             */
/*                                                                           */
/* Created: Tue Dec  2 02:14:16 1997                                         */
/*                                                                           */
/* popen replacement for POSIX 2 avoiding shell piggyback                    */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h" 

pid_t *CHILD;
int    MAXFD = 20; /* Max number of simultaneous pipes */

/*****************************************************************************/

FILE *cfpopen(command,type)
    
char *command, *type;

 { static char arg[maxshellargs][bufsize];
   int i, argc, pd[2];
   char **argv;
   pid_t pid;
   FILE *pp = NULL;

 Debug("cfpopen(%s)\n",command);

 if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
    {
    errno = EINVAL;
    return NULL;
    }
 
 if (CHILD == NULL)   /* first time */
    {
    if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
       {
       return NULL;
       }
    }

 if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
    {
    return NULL;
    }

 if ((pid = fork()) == -1)
    {
    return NULL;
    }
 
 if (pid == 0)
    {
    switch (*type)
       {
       case 'r':

	   close(pd[0]);        /* Don't need output from parent */

	   if (pd[1] != 1)
	      {
	      dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
	      dup2(pd[1],2);    /* Merge stdout/stderr */
	      close(pd[1]);
	      }

	   break;

       case 'w':

	   close(pd[1]);

	   if (pd[0] != 0)
	      {
	      dup2(pd[0],0);
	      close(pd[0]);
	      }
       }

    for (i = 0; i < MAXFD; i++)
       {
       if (CHILD[i] > 0)
	  {
	  close(i);
	  }
       }

    argc = SplitCommand(command,arg);
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
       snprintf(OUTPUT,bufsize,"Couldn't run %s",arg[0]);
       CfLog(cferror,OUTPUT,"execv");
       }

    free((char *)argv);
    _exit(1);
    }
 else
    {
    switch (*type)
       {
       case 'r':

	   close(pd[1]);
	   
	   if ((pp = fdopen(pd[0],type)) == NULL)
	      {
	      return NULL;
	      }
	   break;
	   
       case 'w':

	   close(pd[0]);
	   
	   if ((pp = fdopen(pd[1],type)) == NULL)
	      {
	      return NULL;
	      }
       }

    CHILD[fileno(pp)] = pid;
    return pp;
    }

 return NULL; /* Cannot reach here */
 }

/*****************************************************************************/

FILE *cfpopensetuid(command,type,uid,gid,chdirv,chrootv)
    
char *command, *type, *chdirv, *chrootv;
uid_t uid;
gid_t gid;

 { static char arg[maxshellargs][bufsize];
   int i, argc, pd[2];
   char **argv;
   pid_t pid;
   FILE *pp = NULL;

 Debug("cfpopen(%s)\n",command);

 if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
    {
    errno = EINVAL;
    return NULL;
    }
 
 if (CHILD == NULL)   /* first time */
    {
    if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
       {
       return NULL;
       }
    }

 if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
    {
    return NULL;
    }

 if ((pid = fork()) == -1)
    {
    return NULL;
    }

 if (pid == 0)
    {
    switch (*type)
       {
       case 'r':

	   close(pd[0]);        /* Don't need output from parent */

	   if (pd[1] != 1)
	      {
	      dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
	      dup2(pd[1],2);    /* Merge stdout/stderr */
	      close(pd[1]);
	      }

	   break;

       case 'w':

	   close(pd[1]);

	   if (pd[0] != 0)
	      {
	      dup2(pd[0],0);
	      close(pd[0]);
	      }
       }

    for (i = 0; i < MAXFD; i++)
       {
       if (CHILD[i] > 0)
	  {
	  close(i);
	  }
       }

    argc = SplitCommand(command,arg);
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
    
    if (strlen(chrootv) != 0)
       {
       if (chroot(chrootv) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't chroot to %s\n",chrootv);
	  CfLog(cferror,OUTPUT,"chroot");
	  return NULL;
	  }
       }
    
    if (strlen(chdirv) != 0)
       {
       if (chdir(chdirv) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't chdir to %s\n",chdirv);
	  CfLog(cferror,OUTPUT,"chdir");
	  return NULL;
	  }
       }
    
    if (gid != (gid_t) -1)
       {
       Verbose("Changing gid to %d\n",gid);      
       
       if (setgid(gid) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't set gid to %d\n",gid);
	  CfLog(cferror,OUTPUT,"setgid");
	  return NULL;
	  }
       }
    
    if (uid != (uid_t) -1)
       {
       Verbose("Changing uid to %d\n",uid);
       
       if (setuid(uid) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't effective uid to %d\n",uid);
	  CfLog(cferror,OUTPUT,"setuid");
	  return NULL;
	  }
       }
    
    if (execv(arg[0],argv) == -1)
       {
       snprintf(OUTPUT,bufsize,"Couldn't run %s",arg[0]);
       CfLog(cferror,OUTPUT,"execv");
       }

    free((char *)argv);
    _exit(1);
    }
 else
    {
    switch (*type)
       {
       case 'r':

	   close(pd[1]);
	   
	   if ((pp = fdopen(pd[0],type)) == NULL)
	      {
	      return NULL;
	      }
	   break;
	   
       case 'w':

	   close(pd[0]);
	   
	   if ((pp = fdopen(pd[1],type)) == NULL)
	      {
	      return NULL;
	      }
       }

    CHILD[fileno(pp)] = pid;
    return pp;
    }
 return NULL; /* cannot reach here */
 }


/*****************************************************************************/
/* Shell versions of commands - not recommended for security reasons         */
/*****************************************************************************/

FILE *cfpopen_sh(command,type)
    
char *command, *type;

 { int i,pd[2];
   pid_t pid;
   FILE *pp = NULL;

 Debug("cfpopen(%s)\n",command);

 if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
    {
    errno = EINVAL;
    return NULL;
    }
 
 if (CHILD == NULL)   /* first time */
    {
    if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
       {
       return NULL;
       }
    }

 if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
    {
    return NULL;
    }

 if ((pid = fork()) == -1)
    {
    return NULL;
    }
 
 if (pid == 0)
    {
    switch (*type)
       {
       case 'r':

	   close(pd[0]);        /* Don't need output from parent */

	   if (pd[1] != 1)
	      {
	      dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
	      dup2(pd[1],2);    /* Merge stdout/stderr */
	      close(pd[1]);
	      }

	   break;

       case 'w':

	   close(pd[1]);

	   if (pd[0] != 0)
	      {
	      dup2(pd[0],0);
	      close(pd[0]);
	      }
       }

    for (i = 0; i < MAXFD; i++)
       {
       if (CHILD[i] > 0)
	  {
	  close(i);
	  }
       }

    execl("/bin/sh","sh","-c",command,NULL);
    _exit(1);
    }
 else
    {
    switch (*type)
       {
       case 'r':
	   
	   close(pd[1]);
	   
	   if ((pp = fdopen(pd[0],type)) == NULL)
	      {
	      return NULL;
	      }
	   break;
	   
       case 'w':
	   
	   close(pd[0]);
	   
	   if ((pp = fdopen(pd[1],type)) == NULL)
	      {
	      return NULL;
	      }
       }

    CHILD[fileno(pp)] = pid;
    return pp;
    }

return NULL;
}

/******************************************************************************/

FILE *cfpopen_shsetuid(command,type,uid,gid,chdirv,chrootv)
    
char *command, *type, *chdirv, *chrootv;
uid_t uid;
gid_t gid;


 { int i,pd[2];
   pid_t pid;
   FILE *pp = NULL;

 Debug("cfpopen(%s)\n",command);

 if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
    {
    errno = EINVAL;
    return NULL;
    }
 
 if (CHILD == NULL)   /* first time */
    {
    if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
       {
       return NULL;
       }
    }

 if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
    {
    return NULL;
    }

 if ((pid = fork()) == -1)
    {
    return NULL;
    }
 
 
 if (pid == 0)
    {
    switch (*type)
       {
       case 'r':

	   close(pd[0]);        /* Don't need output from parent */

	   if (pd[1] != 1)
	      {
	      dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
	      dup2(pd[1],2);    /* Merge stdout/stderr */
	      close(pd[1]);
	      }

	   break;

       case 'w':

	   close(pd[1]);

	   if (pd[0] != 0)
	      {
	      dup2(pd[0],0);
	      close(pd[0]);
	      }
       }

    for (i = 0; i < MAXFD; i++)
       {
       if (CHILD[i] > 0)
	  {
	  close(i);
	  }
       }

    if (strlen(chrootv) != 0)
       {
       if (chroot(chrootv) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't chroot to %s\n",chrootv);
	  CfLog(cferror,OUTPUT,"chroot");
	  return NULL;
	  }
       }

    if (strlen(chdirv) != 0)
       {
       if (chdir(chdirv) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't chdir to %s\n",chdirv);
	  CfLog(cferror,OUTPUT,"chroot");
	  return NULL;
	  }
       }
    
    if (gid != (gid_t) -1)
       {
       Verbose("Changing gid to %d\n",gid);      
       
       if (setgid(gid) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't set gid to %d\n",gid);
	  CfLog(cferror,OUTPUT,"setgid");
	  return NULL;
	  }
       }
    
    if (uid != (uid_t) -1)
       {
       Verbose("Changing uid to %d\n",uid);
       
       if (setuid(uid) == -1)
	  {
	  snprintf(OUTPUT,bufsize,"Couldn't set uid to %d\n",uid);
	  CfLog(cferror,OUTPUT,"setuid");
	  return NULL;
	  }
       }
    
    execl("/bin/sh","sh","-c",command,NULL);
    _exit(1);
    }
 else
    {
    switch (*type)
       {
       case 'r':
	   
	   close(pd[1]);
	   
	   if ((pp = fdopen(pd[0],type)) == NULL)
	      {
	      return NULL;
	      }
	   break;
	   
       case 'w':
	   
	   close(pd[0]);
	   
	   if ((pp = fdopen(pd[1],type)) == NULL)
	      {
	      return NULL;
	      }
       }

    CHILD[fileno(pp)] = pid;
    return pp;
    }

return NULL;
}


/******************************************************************************/
/* Close commands                                                             */
/******************************************************************************/

int cfpclose(pp)

FILE *pp;

{ int fd, status;
  pid_t pid;

Debug("cfpclose(pp)\n");

if (CHILD == NULL)  /* popen hasn't been called */
   {
   return -1;
   }

fd = fileno(pp);

if ((pid = CHILD[fd]) == 0)
   {
   return -1;
   }

CHILD[fd] = 0;

if (fclose(pp) == EOF)
   {
   return -1;
   }

Debug("cfpopen - Waiting for process %d\n",pid); 

#ifdef HAVE_WAITPID

while(waitpid(pid,&status,0) < 0)
   {
   if (errno != EINTR)
      {
      return -1;
      }
   }

 return status; 
 
#else
      
 if (wait(&status) != pid)
    {
    return -1;
    }
 else
    {
    if (WIFSIGNALED(status))
       {
       return -1;
       }
    
    if (! WIFEXITED(status))
       {
       return -1;
       }

    return (WEXITSTATUS(status));
    }
#endif
}


/*******************************************************************/
/* Command exec aids                                               */
/*******************************************************************/

int SplitCommand(comm,arg)

char *comm, arg[maxshellargs][bufsize];

{ char *sp;
  int i = 0;

for (sp = comm; sp < comm+strlen(comm); sp++)
   {
   if (i >= maxshellargs-1)
      {
      CfLog(cferror,"Too many arguments in embedded script","");
      FatalError("Use a wrapper");
      }
   
   while (*sp == ' ' || *sp == '\t')
      {
      sp++;
      }
   
   switch (*sp)
      {
      case '\0': return(i-1);
	  
      case '\"': sscanf (++sp,"%[^\"]",arg[i]);
	  break;
      case '\'': sscanf (++sp,"%[^\']",arg[i]);
	  break;
      case '`':  sscanf (++sp,"%[^`]",arg[i]);
	  break;
      default:   sscanf (sp,"%s",arg[i]);
	  break;
      }
   
   sp += strlen(arg[i]);
   i++;
   }
 
 return (i);
}



