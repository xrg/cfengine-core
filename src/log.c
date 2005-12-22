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
/* File: log.c                                                               */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

extern char CFLOCK[CF_BUFSIZE];

/*****************************************************************************/

void CfOpenLog()

{ char value[CF_BUFSIZE];
  int facility = LOG_USER; 
  static int lastsyslog = 0;
 
if (GetMacroValue(CONTEXTID,"SyslogFacility"))
   {
   strncpy(value,GetMacroValue(CONTEXTID,"SyslogFacility"),32);
   
   if (strcmp(value,"LOG_USER") == 0)
      {
      facility = LOG_USER;
      }
   if (strcmp(value,"LOG_DAEMON") == 0)
      {
      facility = LOG_DAEMON;
      }
   if (strcmp(value,"LOG_LOCAL0") == 0)
      {
      facility = LOG_LOCAL0;
      }
   if (strcmp(value,"LOG_LOCAL1") == 0)
      {
      facility = LOG_LOCAL1;
      }
   if (strcmp(value,"LOG_LOCAL2") == 0)
      {
      facility = LOG_LOCAL2;
      }
   if (strcmp(value,"LOG_LOCAL3") == 0)
      {
      facility = LOG_LOCAL3;
      }
   if (strcmp(value,"LOG_LOCAL4") == 0)
      {
      facility = LOG_LOCAL4;
      }
   if (strcmp(value,"LOG_LOCAL5") == 0)
      {
      facility = LOG_LOCAL5;
      }
   if (strcmp(value,"LOG_LOCAL6") == 0)
      {
      facility = LOG_LOCAL6;
      }   
   if (strcmp(value,"LOG_LOCAL7") == 0)
      {
      facility = LOG_LOCAL7;
      }
   
   if (lastsyslog != 1)
      {
      if (lastsyslog)
         {
         closelog();
         }
      }
   lastsyslog=1;
   openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,facility);
   }
else if (ISCFENGINE)
   {
   if (lastsyslog != 2)
      {
      if( lastsyslog )
         {
         closelog();
         }
      }
   lastsyslog=2;
   openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_USER);
   }
else
   {
   if (lastsyslog != 3)
      {
      if( lastsyslog )
         {
         closelog();
         }
      }
   lastsyslog=3;
   openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);
   }
}

/*****************************************************************************/

void CfLog(enum cfoutputlevel level,char *string,char *errstr)

{ int endl = false;
  char *sp, buffer[1024];

if ((string == NULL) || (strlen(string) == 0))
   {
   return;
   }

strncpy(buffer,string,1022);
buffer[1023] = '\0'; 

/* Check for %s %m which someone might be able to insert into
   an error message in order to get a syslog buffer overflow...
   bug reported by Pekka Savola */
 
for (sp = buffer; *sp != '\0'; sp++)
   {
   if ((*sp == '%') && (*(sp+1) >= 'a'))
      {
      *sp = '?';
      }
   }


#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (!SILENT && (pthread_mutex_lock(&MUTEX_SYSCALL) != 0))
   {
   /* If we can't lock this could be dangerous to proceed with threaded file descriptors */
   return;
   /* CfLog(cferror,"pthread_mutex_lock failed","lock"); would lead to sick recursion */
   }
#endif
 
switch(level)
   {
   case cfsilent:
       if (! SILENT || VERBOSE || DEBUG || D2)
          {
          ShowAction();
          printf("%s: %s",VPREFIX,buffer);
          endl = true;
          }
       break;
       
   case cfinform:
       if (SILENT)
          {
          return;
          }
       
       if (INFORM || VERBOSE || DEBUG || D2)
          {
          ShowAction();
          printf("%s: %s",VPREFIX,buffer);
          endl = true;
          }
       
       if (LOGGING && IsPrivileged() && !DONTDO)
          {
          syslog(LOG_NOTICE, "%s", buffer); 
          
          if ((errstr != NULL) && (strlen(errstr) != 0))
             {
             syslog(LOG_ERR,"%s: %s",errstr,strerror(errno));  
             }
          }
       break;
   
   case cfverbose:
       if (VERBOSE || DEBUG || D2)
          {
          if ((errstr == NULL) || (strlen(errstr) > 0))
             {
             ShowAction();
             printf("%s: %s\n",VPREFIX,buffer);
             printf("%s: %s",VPREFIX,errstr);
             endl = true;
             }
          else
             {
             ShowAction();
             printf("%s: %s",VPREFIX,buffer);
             endl = true;
             }
          }
       break;

   case cfeditverbose:
       if (EDITVERBOSE || DEBUG)
          {
          ShowAction();
          printf("%s: %s",VPREFIX,buffer);
          endl = true;
          }
       break;
       
   case cflogonly: 

       if (LOGGING && IsPrivileged() && !DONTDO)
          {
          syslog(LOG_ERR," %s",buffer);    
          
          if ((errstr != NULL) && (strlen(errstr) > 0))
             {
             syslog(LOG_ERR," %s",errstr);  
             }
          }
       
       break;

   case cferror:
       printf("%s: %s",VPREFIX,buffer);
       
       if (LOGGING && IsPrivileged() && !DONTDO)
          {   
          syslog(LOG_ERR," %s",buffer);    
          }
       
       if (buffer[strlen(buffer)-1] != '\n')
          {
          printf("\n");
          }
       
       if ((errstr != NULL) && (strlen(errstr) > 0))
          {
          ShowAction();
          printf("%s: %s: %s\n",VPREFIX,errstr,strerror(errno));
          endl = true;
          
          if (LOGGING && IsPrivileged() && !DONTDO)
             {
             syslog(LOG_ERR, " %s: %s",errstr,strerror(errno));   
             }
          }
   }

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
    {
    /* CfLog(cferror,"pthread_mutex_unlock failed","lock");*/
    }
#endif 
 
 
 if (endl && (buffer[strlen(buffer)-1] != '\n'))
    {
    printf("\n");
    }
}

/*****************************************************************************/

void ResetOutputRoute (char log,char inform)

{
if ((log == 'y') || (log == 'n') || (inform == 'y') || (inform == 'n'))
   {
   INFORM_save = INFORM;
   LOGGING_save = LOGGING;
   
   switch (log)
      {
      case 'y': LOGGING = true;
         break;
      case 'n': LOGGING = false;
         break;
      }

   switch (inform)
      {
      case 'y': INFORM = true;
         break;
      case 'n': INFORM = false;
         break;
      }
   }
else
   {
   INFORM = INFORM_save;
   LOGGING = LOGGING_save;
   }
}

/*****************************************************************************/

void ShowAction()

{
if (SHOWACTIONS)
   {
   printf("%s:",CFLOCK);
   }
}
