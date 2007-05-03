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
/* File: scli.c                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*

This is a special case of shell commands for the SCLI interpreter.

*/

void SCLIScript()

{ struct ShellComm *ptr;
  char line[CF_BUFSIZE],eventname[CF_BUFSIZE];
  char comm[20], *sp;
  char execstr[CF_EXPANDSIZE];
  char chdir_buf[CF_EXPANDSIZE];
  char chroot_buf[CF_EXPANDSIZE];
  time_t start,end;
  int print, outsourced;
  mode_t maskval = 0;
  FILE *pp;
  int preview = false;

if (VSCLI == NULL)
   {
   return;
   }


if (!GetLock(ASUniqueName("scli"),CF_SCLI_COMM,ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
   {
   ptr->done = 'y';
   return;
   }

for (ptr = VSCLI; ptr != NULL; ptr=ptr->next)
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
   
   snprintf(OUTPUT,CF_BUFSIZE*2,"\nExecuting scli dialogue %s...(timeout=%d,uid=%d,gid=%d)\n",execstr,ptr->timeout,ptr->uid,ptr->gid);
   CfLog(cfinform,OUTPUT,"");

   start = time(NULL);

   if (DONTDO && preview != 'y')
      {
      printf("%s: scli %s\n",VPREFIX,execstr);
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
         Verbose("Cannot background SCLI scripts %s\n",execstr);
         }

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
      
      pp = cfpopen_shsetuid(CF_SCLI_COMM,"rw",ptr->uid,ptr->gid,chdir_buf,chroot_buf);
      
      if (pp == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",execstr);
         CfLog(cferror,OUTPUT,"popen");
         ResetOutputRoute('d','d');
         ReleaseCurrentLock();
         break;
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
               snprintf(OUTPUT,CF_BUFSIZE,"%s: %s\n",VPREFIX,comm,line);
               CfLog(cfinform,OUTPUT,"");
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
   
   snprintf(OUTPUT,CF_BUFSIZE*2,"End scli %s\n",execstr);
   CfLog(cfinform,OUTPUT,"");
   
   end = time(NULL);
   snprintf(eventname,CF_BUFSIZE-1,"SCLIdialogue(%s)",execstr);
   RecordPerformance(eventname,start,(double)(end-start));
   }

ResetOutputRoute('d','d');
ReleaseCurrentLock();
}
