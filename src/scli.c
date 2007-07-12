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

/****************************************************************************/
/*                                                                          */
/* File: scli.c                                                             */
/*                                                                          */
/****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*

This is a special case of shell commands for the SCLI interpreter.

*/

void SCLIScript()

{ struct ShellComm *ptr;
 char line[CF_BUFSIZE],eventname[CF_BUFSIZE],dummy[1];
  char execstr[CF_EXPANDSIZE],tmpfile[CF_BUFSIZE];
  char chdir_buf[CF_EXPANDSIZE];
  char chroot_buf[CF_EXPANDSIZE];
  char *defines = dummy,*elsedef = dummy;
  time_t start,end;
  int error,i,ifelapsed = -1,expireafter = -1;
  mode_t maskval = 0;
  FILE *pp,*fout;
  int have_chdir = false, have_chroot = false;
  int uid = -1, gid = -1, timeout = -1;

if (VSCLI == NULL)
   {
   return;
   }

Banner("SCLI / SNMP script phase");

dummy[0] = '\0';

snprintf(tmpfile,CF_BUFSIZE,"%s/scli_monologue_pipe",CFWORKDIR);
unlink(tmpfile);

if ((fout = fopen(tmpfile,"w")) == NULL)
   {
   CfLog(cferror,"Unable to open scli monologue pipe","fopen");
   return;
   }

for (ptr = VSCLI; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   if (ptr->done == 'y')
      {
      continue;
      }

   ResetOutputRoute(ptr->log,ptr->inform);
   ExpandVarstring(ptr->name,execstr,NULL);
   
   snprintf(OUTPUT,CF_BUFSIZE*2,"\nConstructing scli monologue %s...(timeout=%d,uid=%d,gid=%d)\n",execstr,ptr->timeout,ptr->uid,ptr->gid);
   CfLog(cfinform,OUTPUT,"");
   
   if (ptr->fork == 'y')
      {
      Verbose("Cannot background SCLI scripts %s\n",execstr);
      }

   if (have_chdir)
      {
      CfLog(cferror,"Multiple declaration of chdir option in scli - does not make sense","");
      }
   else
      {
      if (strlen(ptr->chdir) != 0)
         {
         have_chdir = true;
         ExpandVarstring(ptr->chdir,chdir_buf,"");
         }
      }

   /* This is fragile to silly coding in the config file - a lot of work to fix this.
      Not worth it unless this all proves useful */
   
   uid = ptr->uid;
   gid = ptr->gid;
   defines = ptr->defines;
   elsedef = ptr->elsedef;
   ifelapsed = ptr->ifelapsed;
   expireafter = ptr->expireafter;
   
   if (have_chroot)
      {
      CfLog(cferror,"Multiple declaration of chroot option in scli - does not make sense","");
      }
   else
      {
      if (strlen(ptr->chroot) != 0)
         {
         have_chroot = true;
         ExpandVarstring(ptr->chdir,chdir_buf,"");
         }
      }

   if (timeout > 0)
      {
      if (timeout != ptr->timeout)
         {
         CfLog(cferror,"Multiple declaration of timeout option in scli - does not make sense","");
         }
      }
   else
      {
      timeout = ptr->timeout;
      }
      
   ExpandVarstring(ptr->name,execstr,NULL);

   fprintf(fout,"%s\n",execstr);
   Verbose("to-scli: %s\n",execstr);
   }

fclose(fout);

if (DONTDO)
   {
   return;
   }

if (!GetLock(ASUniqueName("scli"),CF_SCLI_COMM,ifelapsed,expireafter,VUQNAME,CFSTARTTIME))
   {
   return;
   }

if (timeout > 0)
   {
   signal(SIGALRM,(void *)TimeOut);
   alarm(timeout);
   }

start = time(NULL);   

snprintf(VBUFF,CF_BUFSIZE,"%s < %s",CF_SCLI_COMM,tmpfile);

pp = cfpopen_shsetuid(VBUFF,"r",uid,gid,chdir_buf,chroot_buf);

if (pp == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",execstr);
   CfLog(cferror,OUTPUT,"popen");
   ResetOutputRoute('d','d');
   ReleaseCurrentLock();
   return;
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
   
   for (i = 0; CF_SCLICODES[i][0] != NULL; i++)
      {
      if (strncmp(CF_SCLICODES[cfscli_noconnect][0],line,3) == 0
          || strncmp(CF_SCLICODES[cfscli_snmp_return][0],line,3) == 0 )
         {
         error = true;
         snprintf(OUTPUT,CF_BUFSIZE,"Unable to enter snmp dialogue, %s\n",line+strlen(CF_SCLICODES[0][0])+1);
         CfLog(cfinform,OUTPUT,"");
         break;
         }
      }

   snprintf(OUTPUT,CF_BUFSIZE,"%s\n",line+strlen(CF_SCLICODES[0][0])+1);
   CfLog(cfinform,OUTPUT,"");
      
   if (error)
      {
      break;
      }
   }

cfpclose_def(pp,defines,elsedef);

if (timeout != 0)
   {
   alarm(0);
   signal(SIGALRM,SIG_DFL);
   }
   
snprintf(OUTPUT,CF_BUFSIZE*2,"End scli script\n");
CfLog(cfinform,OUTPUT,"");

end = time(NULL);
snprintf(eventname,CF_BUFSIZE-1,"SCLIdialogue(%s)",execstr);
RecordPerformance(eventname,start,(double)(end-start));

ResetOutputRoute('d','d');
ReleaseCurrentLock();
unlink(tmpfile);
}
