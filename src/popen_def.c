
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
/* File: popen_def.c                                                         */
/*                                                                           */
/* Created: Fri Jul 20 15:13:38 2001                                         */
/*                                                                           */
/* Author:                                           >                       */
/*                                                                           */
/* Revision: $Id$                                                            */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h" 

extern pid_t *CHILD;
extern int    MAXFD; /* Max number of simultaneous pipes */

/******************************************************************************/

int cfpclose_def(pp,defines,elsedef)

FILE *pp;
char *defines,*elsedef;

{ int fd, status;
  pid_t pid;

Debug("cfpclose_def(pp)\n");

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

Debug("cfpopen_def - Waiting for process %d\n",pid); 

#ifdef HAVE_WAITPID

while(waitpid(pid,&status,0) < 0)
   {
   if (errno != EINTR)
      {
      return -1;
      }
   }

if (status == 0)
   {
   AddMultipleClasses(defines);
   }
else
   {
   Debug("Child returned status %d\n",status);
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

    if (WEXITSTATUS(status) == 0)
       {
       AddMultipleClasses(defines);
       }
    else
       {
       AddMultipleClasses(elsedef);
       }

    return (WEXITSTATUS(status));
    }
#endif
}

