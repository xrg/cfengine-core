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
 

/*********************************************************/
/* patches.c                                             */
/*                                                       */
/* Contains any fixes which need to be made because of   */
/* lack of OS support on a given platform                */
/* These are conditionally compiled, pending extensions  */
/* or developments in the OS concerned.                  */
/*********************************************************/


#include "cf.defs.h"
#include "cf.extern.h"

#ifndef HAVE_GETNETGRENT

#ifndef const
# define const
#endif

/*********************************************************/

void setnetgrent(netgroup)

const char *netgroup;

{
}

/**********************************************************/

int getnetgrent(a,b,c)

char **a, **b, **c;

{
*a=NULL;
*b=NULL;
*c=NULL;
return 0;
}

/***********************************************************/

void endnetgrent()

{
}

#endif

#ifndef HAVE_UNAME

#ifndef const
# define const
#endif

/***********************************************************/
/* UNAME is missing on some weird OSes                     */
/***********************************************************/

int uname (sys)

struct utsname *sys;

{ char buffer[bufsize], *sp;

if (gethostname(buffer,bufsize) == -1)
   {
   perror("gethostname");
   exit(1);
   }

strcpy(sys->nodename,buffer);

if (strcmp(buffer,AUTOCONF_HOSTNAME) != 0)
   {
   Verbose("This binary was complied on a different host (%s).\n",AUTOCONF_HOSTNAME);
   Verbose("This host does not have uname, so I can't tell if it is the exact same OS\n");
   }

strcpy(sys->sysname,AUTOCONF_SYSNAME);
strcpy(sys->release,"cfengine-had-to-guess");
strcpy(sys->machine,"missing-uname(2)");
strcpy(sys->version,"unknown");


  /* Extract a version number if possible */

for (sp = sys->sysname; *sp != '\0'; sp++)
   {
   if (isdigit(*sp))
      {
      strcpy(sys->release,sp);
      strcpy(sys->version,sp);
      *sp = '\0';
      break;
      }
   }

return (0);
}

#endif

/***********************************************************/
/* strstr() missing on old BSD systems                     */
/***********************************************************/

#ifndef HAVE_STRSTR


#ifndef const
# define const
#endif

char *strstr(s1,s2)

char *s1, *s2;

{ char *sp;

for (sp = s1; *sp != '\0'; sp++)
   {
   if (*sp != *s2)
      {
      continue;
      }

   if (strncmp(sp,s2,strlen(s2))== 0)
      {
      return sp;
      }
   }

return NULL;
}

#endif

/***********************************************************/
/* strdup() missing on old BSD systems                     */
/***********************************************************/

#ifndef HAVE_STRDUP

char *strdup(str)

char *str;

{ char *sp;
 
if (str == NULL)
   {
   return NULL;
   }

if ((sp = malloc(strlen(str)+1)) == NULL)
   {
   perror("malloc");
   return NULL;
   }

strcpy(sp,str);
return sp; 
}

#endif



/***********************************************************/
/* strrchr() missing on old BSD systems                     */
/***********************************************************/

#ifndef HAVE_STRRCHR

char *strrchr(str,ch)

char *str;
char ch;

{ char *sp;
 
if (str == NULL)
   {
   return NULL;
   }

if (strlen(str) == 0)
   {
   return NULL;
   }

for (sp = str+strlen(str)-1; sp > str; sp--)
   {
   if (*sp == ch)
      {
      return *sp;
      }
   }

return NULL; 
}

#endif


/***********************************************************/
/* strerror() missing on systems                           */
/***********************************************************/

#ifndef HAVE_STRERROR

char *strerror(err)

int err;

{ static char buffer[20];

sprintf(buffer,"Error number %d\n",err);
return buffer; 
}

#endif

/***********************************************************/
/* putenv() missing on old BSD systems                     */
/***********************************************************/

#ifndef HAVE_PUTENV


#ifndef const
# define const
#endif

int putenv (s)

char *s;

{
Verbose("(This system does not have putenv: cannot update CFALLCLASSES\n");
return 0;
}


#endif


/***********************************************************/
/* seteuid/gid() missing on some on posix systems          */
/***********************************************************/

#ifndef HAVE_SETEUID


#ifndef const
# define const
#endif

int seteuid (uid)

uid_t uid;

{
#ifdef HAVE_SETREUID
return setreuid(-1,uid);
#else
Verbose("(This system does not have setreuid (patches.c)\n");
return -1; 
#endif 
}

#endif

/***********************************************************/

#ifndef HAVE_SETEGID

#ifndef const
# define const
#endif

int setegid (gid)

gid_t gid;

{
#ifdef HAVE_SETREGID
return setregid(-1,gid);
#else
Verbose("(This system does not have setregid (patches.c)\n");
return -1; 
#endif 
}

#endif

/*******************************************************************/

#ifndef HAVE_DRAND48  

double drand48(void)

{
return (double)random();
}

#endif

#ifndef HAVE_DRAND48  

void srand48(seed)

long seed;

{
srandom((unsigned int)seed);
}

#endif


/*******************************************************************/

int IsPrivileged()

{
#ifdef NT
return true;
#else
return (getuid() == 0);
#endif 
}
