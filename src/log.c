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

extern char CFLOCK[bufsize];

/*****************************************************************************/

void CfLog(level,string,errstr)

enum cfoutputlevel level;
char *string, *errstr;

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
   if (*sp == '%')
      {
      *sp = '?';
      }
   }

switch(level)
   {
   case cfsilent:    if (! SILENT || VERBOSE || DEBUG || D2)
                        {
			ShowAction();
			printf("%s: %s",VPREFIX,buffer);
			endl = true;
                        }
                     break;

   case cfinform:    if (SILENT)
                        {
			return;
                        }
   
                     if (INFORM || VERBOSE || DEBUG || D2)
                        {
			ShowAction();
			printf("%s: %s",VPREFIX,buffer);
			endl = true;
                        }
		     
		     if (LOGGING && IsPrivileged())
			{
			syslog(LOG_NOTICE, "%s", buffer); 

			if (strlen(errstr) != 0)
			   {
			   syslog(LOG_ERR,"%s: %s",errstr,strerror(errno));  
			   }
			}
                     break;
			
   case cfverbose:   if (VERBOSE || DEBUG || D2)
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

   case cfeditverbose: if (EDITVERBOSE || DEBUG)
                          {
			  ShowAction();
		   	  printf("%s: %s",VPREFIX,buffer);
			  endl = true;
                          }
                     break;

   case cflogonly:
                     if (LOGGING && IsPrivileged())
			{
			syslog(LOG_ERR," %s",buffer);    
			
			if ((errstr == NULL) || (strlen(errstr) > 0))
			   {
			   syslog(LOG_ERR," %s",errstr);  
			   }
			}
		     
                     break;

   case cferror:
                     printf("%s: %s",VPREFIX,buffer);

		     if (LOGGING && IsPrivileged())
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
			
			if (LOGGING && IsPrivileged())
			   {
			   syslog(LOG_ERR, " %s: %s",errstr,strerror(errno));   
			   }
                        }
		     return;
   }

 
if (endl && (buffer[strlen(buffer)-1] != '\n'))
   {
   printf("\n");
   }
}

/*****************************************************************************/

void ResetOutputRoute (log,inform)

char log, inform;

  /* t = true, f = false, d = default */

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
