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


/*******************************************************************/
/*                                                                 */
/* Process handling                                                */
/*                                                                 */
/*******************************************************************/


#include "cf.defs.h"
#include "cf.extern.h"

int DEADLOCK = false;
FILE *PIPE;

/*******************************************************************/

int LoadProcessTable(procdata,psopts)

struct Item **procdata;
char *psopts;

{ FILE *pp;
  char pscomm[maxlinksize], imgbackup;

snprintf(pscomm,maxlinksize,"%s %s",VPSCOMM[VSYSTEMHARDCLASS],psopts);

Verbose("%s: Running process command %s\n",VPREFIX,pscomm); 
  
if ((pp = cfpopen(pscomm,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open the process list with command %s\n",pscomm);
   CfLog(cferror,OUTPUT,"popen");
   return false;
   }

while (!feof(pp))
   {
   bzero(VBUFF,bufsize);
   ReadLine(VBUFF,bufsize,pp);
   AppendItem(procdata,VBUFF,"");
   }

cfpclose(pp);

snprintf(VBUFF,maxvarsize,"%s/cf_procs",WORKDIR);

imgbackup = IMAGEBACKUP;
IMAGEBACKUP = 'n'; 
SaveItemList(*procdata,VBUFF,"none");
IMAGEBACKUP = imgbackup; 
return true;
}

/*******************************************************************/

void DoProcessCheck(pp,procdata)

struct Process *pp;
struct Item *procdata;

{ char line[bufsize];
  int matches=0,dosignals=true;
  mode_t maskval;
  struct stat statbuf;
  struct Item *killlist = NULL;
  
matches = FindMatches(pp,procdata,&killlist); 

if (matches > 0)
   {
   Verbose("Defining classes %s\n",pp->defines);
   AddMultipleClasses(pp->defines);
   }
else
   {
   Verbose("Defining classes %s\n",pp->elsedef);
   AddMultipleClasses(pp->elsedef);
   }

if (pp->matches >= 0)
   {
   if (pp->action == 'm')
      {
      dosignals = false;
      }
   
   switch (pp->comp)
      {
      case '=': if (matches != (int)pp->matches)
                   {
	    	   snprintf(OUTPUT,bufsize*2,"%d processes matched %s (should be %d)\n",matches,pp->expr,pp->matches);
		   CfLog(cferror,OUTPUT,"");
		   if (pp->action == 'm')
		      {
		      dosignals = true;
		      }
		   }
                break;
		
      case '>': if (matches < (int)pp->matches)
	           {
	    	   snprintf(OUTPUT,bufsize*2,"%d processes matched %s (should be >=%d)\n",matches,pp->expr,pp->matches);
		   CfLog(cferror,OUTPUT,"");
		   if (pp->action == 'm')
		      {
		      dosignals = true;
		      }
	           } 
                break;
		
      case '<': if (matches > (int)pp->matches)
	           {
	    	   snprintf(OUTPUT,bufsize*2,"%d processes matched %s (should be <=%d)\n",matches,pp->expr,pp->matches);
		   CfLog(cferror,OUTPUT,"");
		   if (pp->action == 'm')
		      {
		      dosignals = true;
		      }
	           } 
      }
   }

if (dosignals)
   {
   DoSignals(pp,killlist);
   }
 
DeleteItemList(killlist);

if ((pp->action == 'm') && !dosignals && (matches != 0))
   {
   Verbose("%s: Matches found for %s - no restart sequence\n",VPREFIX,pp->expr);
   return;
   }
 
if (strlen(pp->restart) != 0)
   {
   char argz[256];
   
   if ((matches != 0) && (pp->signal != cfkill) && (pp->signal != cfterm))
      {
      Verbose("%s: Matches found for %s - no restart sequence\n",VPREFIX,pp->expr);
      return;
      }

   sscanf(pp->restart,"%255s",argz);

   if ((stat(argz,&statbuf) != -1) && (statbuf.st_mode & 0111 == 0))
      {
      snprintf(OUTPUT,bufsize,"Restart sequence %s could not be executed (mode=%o), while searching (%s)",pp->restart,statbuf.st_mode &7777,pp->expr);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   
   snprintf(OUTPUT,bufsize*2,"Executing shell command: %s\n",pp->restart);
   CfLog(cfinform,OUTPUT,"");

   if (DONTDO)
      {
      return;
      }

   Verbose ("(Setting umask to %o)\n",pp->umask);
   maskval = umask(pp->umask);
   
   if (pp->umask == 0)
      {
      snprintf(OUTPUT,bufsize*2,"Programming %s running with umask 0! Use umask= to set\n",pp->restart);
      CfLog(cfsilent,OUTPUT,"");
      }
   
   if (pp->useshell == 'y')
      {
      if ((PIPE = cfpopen_shsetuid(pp->restart,"r",pp->uid,pp->gid,pp->chdir,pp->chroot)) == NULL)
	 {
	 snprintf(OUTPUT,bufsize*2,"Process restart execution failed on %s\n",pp->restart);
	 CfLog(cferror,OUTPUT,"popen");
	 return;
	 }
      }
   else
      {
      if ((PIPE = cfpopensetuid(pp->restart,"r",pp->uid,pp->gid,pp->chdir,pp->chroot)) == NULL)
	 {
	 snprintf(OUTPUT,bufsize*2,"Process restart execution failed on %s\n",pp->restart);
	 CfLog(cferror,OUTPUT,"popen");
	 return;
	 }
      }

   DEADLOCK = false;   
   
   while (!feof(PIPE))
      {
      if (pp->useshell == 'd') /* dumb shell */
	 {
	 fgets(line,1,PIPE);
         break;
	 }
      
      ReadLine(line,bufsize,PIPE);

      if (feof(PIPE) || ferror(PIPE))
	 {
	 break;
	 }
      
      if (strstr(line,"cfengine-die"))
	 {
	 break;
	 }

      /* patch for ERESTARTSYSTEM bug in popen */

      if (strstr(line,"cfd: start") || DEADLOCK)
	 {
	 break;
	 }
      
      snprintf(OUTPUT,bufsize*2,"Restart: %s",line);
      CfLog(cfinform,OUTPUT,"");
      }

   if (pp->useshell == 'y')
      {
      pclose(PIPE);
      }
   else
      {
      cfpclose(PIPE);
      }
   
   snprintf(OUTPUT,bufsize*2,"(Done with %s)\n",pp->restart);
   CfLog(cfinform,OUTPUT,"");
   umask(maskval);
   }
}

/**********************************************************************************/

int FindMatches(pp,procdata,killlist)

struct Process *pp;
struct Item *procdata,**killlist;

{ struct Item *ip, *ip2;
  char *sp,saveuid[16];
  int pid=-1, ret, matches=0, got, i;
  regex_t rx,rxcache;
  regmatch_t pmatch;
  pid_t cfengine_pid = getpid();
  char *names[noproccols];      /* ps headers */
  int start[noproccols];
  int end[noproccols];
  
Debug2("Looking for process %s\n",pp->expr);

if (CfRegcomp(&rxcache,pp->expr,REG_EXTENDED) != 0)
   {
   return 0;
   }

GetProcessColumns(procdata->name,(char **)names,start,end); 

for (ip = procdata; ip != NULL; ip=ip->next)
   {
   bcopy(&rxcache,&rx,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      pid = -1;
      got = true;
      Debug("Regex %s matched %s\n",ip->name,pp->expr);
      
      for (ip2 = pp->inclusions; ip2 != NULL; ip2 = ip2->next)
	 {
	 got = false;
	 
	 if (strstr(ip->name,ip2->name) || WildMatch(ip2->name,ip->name))
	    {
	    got = true;
	    break;
	    }
	 }
      
      if (!got)
	 {
	 continue;
	 }
      
      got = false;
      
      for (ip2 = pp->exclusions; ip2 != NULL; ip2 = ip2->next)
	 {
	 if (strstr(ip->name,ip2->name) || WildMatch(ip2->name,ip->name))
	    {
	    got = true;
	    break;
	    }
	 }

      if (!ProcessFilter(ip->name,pp->filters,names,start,end))
	 {
	 Debug("%s Filtered away\n",ip->name);
	 continue;
	 }
      
      if (got)
	 {
	 continue;
	 }

      Debug("Matched proc[%s]\n",ip->name);
      
      for (sp = ip->name; *sp != '\0'; sp++) /* if first field contains alpha, skip */
	 {
	 while (true)
	    {
	    while (!isdigit((int)*sp) && (*sp != '\0'))
	       {
	       sp++;
	       }
	    
	    if ((sp > ip->name) && isalnum((int)*(sp-1))) /* Username contains number*/
	       {
	       sp++;
	       }
	    else
	       {
	       break;
	       }
	    }
	 
	 sscanf(sp,"%d",&pid);
	 
	 if (pid != -1)
	    {
	    break;
	    }
	 }  
      
      if (pid == -1)
	 {
	 snprintf(OUTPUT,bufsize*2,"Unable to extract pid while looking for %s\n",pp->expr);
	 CfLog(cfverbose,OUTPUT,"");
	 continue;
	 }
      
      Debug2("Found matching pid %d\n",pid);
      
      matches++;
      
      if (pid == 1 && pp->signal == cfhup)
	 {
	 Verbose("(Okay to send HUP to init)\n");
	 }
      else if (pid < 4)
	 {
	 Verbose("%s: will not signal or restart processes 0,1,2,3\n",VPREFIX);
	 Verbose("%s: occurred while looking for %s\n",VPREFIX,pp->expr);
	 continue;
	 }
      
      if (pp->action == 'w')
	 {
	 snprintf(OUTPUT,bufsize*2,"Process alert: %s\n",procdata->name);
	 CfLog(cferror,OUTPUT,"");	 
	 snprintf(OUTPUT,bufsize*2,"Process alert: %s\n",ip->name);
	 CfLog(cferror,OUTPUT,"");
	 continue;
	 }
      
      if (pp->signal != cfnosignal)
	 {
	 snprintf(OUTPUT,bufsize*2,"Signalling process %d (%s) with %s\n",pid,pp->expr,SIGNALS[pp->signal]);
	 CfLog(cfinform,OUTPUT,"");
	 
	 if (!DONTDO)
	    {
	    if (pid == cfengine_pid)
	       {
	       CfLog(cfverbose,"Cfengine will not kill itself!\n","");
	       continue;
	       }

	    if (pp->action == 'm')
	       {
	       sprintf(saveuid,"%d",pid);
	       PrependItem(killlist,saveuid,"");
	       }
	    else
	       {
	       if ((ret = kill((pid_t)pid,pp->signal)) < 0)
		  {
		  snprintf(OUTPUT,bufsize*2,"Couldn't send signal to pid %d\n",pid);
		  CfLog(cfverbose,OUTPUT,"kill");
		  
		  continue;
		  }
	       
	       if ((pp->signal == cfkill || pp->signal == cfterm) && ret >= 0)
		  {
		  snprintf(OUTPUT,bufsize*2,"Killed: %s\n",ip->name);
		  CfLog(cfinform,OUTPUT,"");
		  }
	       }
	    }
	 }
      }
   }

 for (i = 0; i < noproccols; i++)
    {
    if (names[i] != NULL)
       {
       free(names[i]);
       }
    }

 regfree(&rx);
 return matches;
}

/**********************************************************************************/

void DoSignals(pp,list)

struct Process *pp;
struct Item *list;

{ struct Item *ip;
  pid_t pid;
  int ret;

Verbose("DoSignals(%s)\n",pp->expr);
  
if (list == NULL)
   {
   return;
   }

if (pp->signal == cfnosignal)
   {
   Verbose ("No signal to send for %s\n",pp->expr);
   return;
   }

for (ip = list; ip != NULL; ip=ip->next)
   {
   pid = (pid_t)-1;
   
   sscanf(ip->name,"%d",&pid);

   if (pid == (pid_t)-1)
      {
      CfLog(cferror,"Software error: Unable to decypher pid list","");
      return;
      }

   if (!DONTDO)
      {
      if ((ret = kill((pid_t)pid,pp->signal)) < 0)
	 {
	 snprintf(OUTPUT,bufsize*2,"Couldn't send signal to pid %d\n",pid);
	 CfLog(cfverbose,OUTPUT,"kill");
	 
	 return;
	 }
   
      if ((pp->signal == cfkill || pp->signal == cfterm) && ret >= 0)
	 {
	 snprintf(OUTPUT,bufsize*2,"Killed: %s\n",ip->name);
	 CfLog(cfinform,OUTPUT,"");
	 }
      }
   }
}
