/* 
        Copyright (C) 1995-
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
/* Cfexecd : local scheduling daemon                               */
/*                                                                 */
/*******************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"

#ifdef NT
#include <process.h>
#endif

/*******************************************************************/
/* Pthreads                                                        */
/*******************************************************************/


#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#ifdef HAVE_SCHED_H
# include <sched.h>
#endif

#ifdef HAVE_PTHREAD_H
pthread_attr_t PTHREADDEFAULTS;
pthread_mutex_t MUTEX_COUNT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_HOSTNAME = PTHREAD_MUTEX_INITIALIZER;
#endif

/*******************************************************************/
/* GLOBAL VARIABLES                                                */
/*******************************************************************/

int NO_FORK = false;
int ONCE = false;

struct option CFDOPTIONS[] =
   {
   { "help",no_argument,0,'h' },
   { "debug",optional_argument,0,'d' }, 
   { "verbose",no_argument,0,'v' },
   { "file",required_argument,0,'f' },
   { "no-fork",no_argument,0,'F' },
   { "once",no_argument,0,'1'},
   { "foreground",no_argument,0,'g'},
   { "parse-only",no_argument,0,'p'},
   { "ld-library-path",required_argument,0,'L'},
   { NULL,0,0,0 }
   };

char MAILTO[bufsize];

/*******************************************************************/
/* Functions internal to cfservd.c                                 */
/*******************************************************************/

void CheckOptsAndInit ARGLIST((int argc,char **argv));
void StartServer ARGLIST((int argc, char **argv));
void Syntax ARGLIST((void));
void ExitCleanly ARGLIST((void));
void *LocalExec ARGLIST((void *dummy));
void MailResult ARGLIST((char *filename, char *to));
int ScheduleRun ARGLIST((void));
void AddClassToHeap ARGLIST((char *class));
void DeleteClassFromHeap ARGLIST((char *class));
void Dialogue  ARGLIST((int sd,char *class));
void GetCfStuff ARGLIST((void));


/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (argc,argv)

int argc;
char **argv;

{ time_t starttime = time(NULL);
 
CheckOptsAndInit(argc,argv);

if (!ONCE)
   {
   /* Hacked timeouts - the normal cfengine lock does not seem appropriate
    * here - I simply want to get the lock as long as no other cfexecd is
    * running.
    */

   if (!GetLock("cfexecd","execd",0,0,VUQNAME,starttime))
      {
      snprintf(OUTPUT,bufsize*2,"cfexecd: Couldn't get a lock -- exists or too soon: IfElapsed %d, ExpireAfter %d\n",0,0);
      CfLog(cfverbose,OUTPUT,"");
      return 1;
      }
   }
   
StartServer(argc,argv);

if (!ONCE)
   {
   ReleaseCurrentLock();
   }
 
return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(argc,argv)

int argc;
char **argv;

{ extern char *optarg;
  int optindex = 0;
  char ld_library_path[bufsize];
  int c;

ld_library_path[0] = '\0';
  
openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);

while ((c=getopt_long(argc,argv,"L:d:f:vhpFV1g",CFDOPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'f': strncpy(VINPUTFILE,optarg,bufsize-1);
                break;

      case 'd': 

                switch ((optarg==NULL)?3:*optarg)
                   {
                   case '1': D1 = true;
                             break;
                   case '2': D2 = true;
                             break;
                   default:  DEBUG = true;
                             break;
                   }
		
		NO_FORK = true;
		printf("cfexecd Debug mode: running in foreground\n");
                break;

      case 'v': VERBOSE = true;
	        break;

      case 'V': printf("GNU %s-%s daemon\n%s\n",PACKAGE,VERSION,COPYRIGHT);
	        printf("This program is covered by the GNU Public License and may be\n");
		printf("copied free of charge. No warrenty is implied.\n\n");
                exit(0);
	        break;

      case 'p': PARSEONLY = true;
	        break;

      case 'g': NO_FORK = true;
	        break;

      case 'L': Verbose("Setting LD_LIBRARY_PATH=%s\n",optarg);
	        snprintf(ld_library_path,bufsize-1,"LD_LIBRARY_PATH=%s",optarg);
		putenv(ld_library_path);
		break;

      case 'F':
      case '1': ONCE = true;
		NO_FORK = true;
	        break;

      default:  Syntax();
                exit(1);

      }
   }

LOGGING = true;                    /* Do output to syslog */

snprintf(VBUFF,bufsize,"%s/inputs/update.conf",WORKDIR);
MakeDirectoriesFor(VBUFF,'y');
snprintf(VBUFF,bufsize,"%s/bin/cfagent",WORKDIR);
MakeDirectoriesFor(VBUFF,'y');
snprintf(VBUFF,bufsize,"%s/outputs/spooled_reports",WORKDIR);
MakeDirectoriesFor(VBUFF,'y');

snprintf(VBUFF,bufsize,"%s/inputs",WORKDIR);
chmod(VBUFF,0700); 
snprintf(VBUFF,bufsize,"%s/outputs",WORKDIR);
chmod(VBUFF,0700);

strncpy(VLOCKDIR,WORKDIR,bufsize-1);
strncpy(VLOGDIR,WORKDIR,bufsize-1);

GetNameInfo();

strcpy(VUQNAME,VSYSNAME.nodename);
}


/*******************************************************************/

void StartServer(argc,argv)

int argc;
char **argv;

{ int time_to_run = false;
  time_t now = time(NULL); 

if ((!NO_FORK) && (fork() != 0))
   {
   snprintf(OUTPUT,bufsize*2,"cfexecd starting %.24s\n",ctime(&now));
   CfLog(cfinform,OUTPUT,"");
   exit(0);
   }

if (!NO_FORK)
  {
#ifdef HAVE_SETSID
  setsid();
#endif 
  fclose (stdin);
  fclose (stdout);
  fclose (stderr);
  closelog();
  }

signal(SIGINT,(void *)ExitCleanly);
signal(SIGTERM,(void *)ExitCleanly);
signal(SIGHUP,SIG_IGN);
signal(SIGPIPE,SIG_IGN);
 
umask(077);

if (ONCE)
   {
   GetCfStuff();
   LocalExec(NULL);
   }
else
   { char **nargv;
     int i;
     int pid;
#ifdef HAVE_PTHREAD_H
     pthread_t tid;
#endif

   /*
    * Append --once option to our arguments for spawned monitor process.
    */
   nargv = malloc(sizeof(char *) * argc+2);
   
   for (i = 0; i < argc; i++)
      {
      nargv[i] = argv[i];
      }
   
   nargv[i++] = strdup("--once");
   nargv[i++] = NULL;
   
   GetCfStuff();
   
   while (true)
      {
      time_to_run = ScheduleRun();
      
      if (time_to_run)
	 {
	 if (!GetLock("cfd","exec",exec_ifelapsed,exec_expireafter,VUQNAME,time(NULL)))
	    {
	    snprintf(OUTPUT,bufsize*2,"cfexecd: Couldn't get exec lock -- exists or too soon: IfElapsed %d, ExpireAfter %d\n",exec_ifelapsed,exec_expireafter);
	    CfLog(cfverbose,OUTPUT,"");
	    continue;
	    }
	 
	 GetCfStuff();
	 
#ifdef NT 
	 /*
	  * Spawn a separate process - spawn will work if the cfexecd binary
	  * has changed (where cygwin's fork() would fail).
	  */
	 
	 Debug("Spawning %s\n", nargv[0]);
	 pid = spawnvp((int)_P_NOWAIT, nargv[0], nargv);
	 if (pid < 1)
	    {
	    CfLog(cferror,"Can't spawn run","spawnvp");
	    }
	 
#elseif (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
	 
	 pthread_attr_init(&PTHREADDEFAULTS);
	 pthread_attr_setdetachstate(&PTHREADDEFAULTS,PTHREAD_CREATE_DETACHED);

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
	 pthread_attr_setstacksize(&PTHREADDEFAULTS,(size_t)1024*1024);
#endif

	 if (pthread_create(&tid,&PTHREADDEFAULTS,LocalExec,NULL) != 0)
	    {
	    CfLog(cferror,"Can't create thread!","pthread_create");
	    LocalExec(NULL);
	    }

	 pthread_attr_destroy(&PTHREADDEFAULTS);
#else
         LocalExec(NULL);	 
#endif
	 
	 ReleaseCurrentLock();
	 }
      }
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine daemon: scheduler\n%s-%s\n%s\n",PACKAGE,VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; CFDOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",CFDOPTIONS[i].name,(char)CFDOPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@gnu.org (News: gnu.cfengine.bug)\n");
printf("General help to help-cfengine@gnu.org (News: gnu.cfengine.help)\n");
printf("Info & fixes at http://www.iu.hio.no/cfengine\n");
}

/*******************************************************************/

void GetCfStuff()

{ FILE *pp;
  char cfcom[bufsize];
  static char line[bufsize];

snprintf(cfcom,bufsize-1,"%s/bin/cfagent -z",WORKDIR);
 
if ((pp=cfpopen(cfcom,"r")) ==  NULL)
   {
   CfLog(cferror,"Couldn't start cfengine!","cfpopen");
   line[0] = '\0';
   return;
   }

line[0] = '\0'; 
fgets(line,bufsize,pp); 
Chop(line); 

while (strstr(line,":"))
   {
   line[0] = '\0'; 
   fgets(line,bufsize,pp); 
   Chop(line);    
   }

if (strstr(line,"No SMTP"))
   {
   CfLog(cferror,"cfengine defines no SMTP server (not even localhost)","");
   CfLog(cferror,"Need: smtpserver = ( ?? ) in control ","");
   }

strcpy(VMAILSERVER,line); 
 
Debug("Got cfengine SMTP server as (%s)\n",VMAILSERVER); 

line[0] = '\0'; 
fgets(line,bufsize,pp); 
Chop(line); 

if (strlen(line) == 0)
   {
   CfLog(cferror,"cfengine defines no system administrator address","");
   CfLog(cferror,"Need: sysadm = ( ??@?? ) in control ","");
   }

strcpy(MAILTO,line); 
Debug("Got cfengine sysadm variable (%s)\n",MAILTO); 

line[0] = '\0'; 
fgets(line,bufsize,pp); 
Chop(line); 
strcpy(VFQNAME,line); 
Debug("Got full qualified name (%s)\n",VFQNAME); 

line[0] = '\0'; 
fgets(line,bufsize,pp); 
Chop(line); 
strcpy(VIPADDRESS,line); 
Debug("Got IP (%s)\n",VIPADDRESS); 

/* Now get scheduling constraints */

DeleteItemList(SCHEDULE);
SCHEDULE = NULL; 
 
while (!feof(pp))
   {
   line[0] = '\0';
   VBUFF[0] = '\0';
   fgets(line,bufsize,pp);
   sscanf(line,"[%[^]]",VBUFF);

   if (strlen(VBUFF)==0)
      {
      continue;
      }
   
   if (!IsItemIn(SCHEDULE,VBUFF))
      {
      AppendItem(&SCHEDULE,VBUFF,NULL);
      }
   }

cfpclose(pp);

if (SCHEDULE == NULL)
   {
   Verbose("No schedule defined in cfagent.conf, defaulting to Min00_05\n");
   AppendItem(&SCHEDULE,"Min00_05",NULL);
   }
}

/*********************************************************************/

int ScheduleRun()

{ time_t now;
  char timekey[64];
  struct Item *ip;

Verbose("Sleeping...\n");
sleep(60);          /* 1 Minute resolution is enough */ 

now = time(NULL);

snprintf(timekey,63,"%s",ctime(&now)); 
AddTimeClass(timekey); 

for (ip = SCHEDULE; ip != NULL; ip = ip->next)
   {
   Verbose("Checking schedule %s...\n",ip->name);
   if (IsDefinedClass(ip->name))
      {
      Verbose("Waking up the agent at %s ~ %s \n",timekey,ip->name);
      DeleteItemList(VHEAP);
      VHEAP = NULL;
      GetNameInfo();
      return true;
      }
   }

DeleteItemList(VHEAP);
GetNameInfo();
return false;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void ExitCleanly()

{
ReleaseCurrentLock();
closelog();
 
exit(0);
}

/**************************************************************/

void *LocalExec(dummy)

void *dummy;

{ FILE *pp; 
  char line[bufsize],filename[bufsize],*sp;
  char cmd[bufsize];
  int print;
  time_t starttime = time(NULL);
  FILE *fp;
#ifdef HAVE_PTHREAD_SIGMASK
 sigset_t sigmask;

sigemptyset(&sigmask);
pthread_sigmask(SIG_BLOCK,&sigmask,NULL); 
#endif

 
Verbose("------------------------------------------------------------------\n\n");
Verbose("  LocalExec at %s\n",ctime(&starttime));
Verbose("------------------------------------------------------------------\n"); 

/* Need to make sure we have LD_LIBRARY_PATH here or children will die  */
 
snprintf(cmd,bufsize-1,"%s/bin/cfagent",WORKDIR);
snprintf(line,100,CanonifyName(ctime(&starttime)));
snprintf(filename,bufsize-1,"%s/outputs/cf_%s_%s",WORKDIR,CanonifyName(VFQNAME),line);


/* What if no more processes? Could sacrifice and exec() - but we need a sentinel */

if ((fp = fopen(filename,"w")) == NULL)
   {
   snprintf(OUTPUT,bufsize,"Couldn't open %s\n",filename);
   CfLog(cferror,OUTPUT,"fopen");
   return NULL;
   }
 
if ((pp = cfpopen(cmd,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize,"Couldn't open pipe to command %s\n",cmd);
   CfLog(cferror,OUTPUT,"cfpopen");
   return NULL;
   }
 
while (!feof(pp) && ReadLine(line,bufsize,pp))
   {
   if (ferror(pp))
      {
      fflush(pp);
      break;
      }	 
   
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
      fprintf(fp,"%s\n",line);
      
      /* If we can't send mail, log to syslog */
      
      if (strlen(MAILTO) == 0)
	 {
	 strcat(line,"\n");
	 CfLog(cfinform,line,"");
	 }
      
      line[0] = '\0';
      }
   }
 
cfpclose(pp);
Debug("Closing fp\n");
fclose(fp);
closelog();
  
MailResult(filename,MAILTO);
return NULL; 
}

/******************************************************************/
/* Level 4                                                        */
/******************************************************************/

void MailResult(file,to)

char *file,*to;

{ int sd, sent, count = 0;
  struct hostent *hp;
  struct sockaddr_in raddr;
  struct servent *server;
  struct stat statbuf;
  FILE *fp;

if ((strlen(VMAILSERVER) == 0) || (strlen(to) == 0))
   {
   /* Syslog should have done this*/
   return;
   }
  
if (stat(file,&statbuf) == -1)
   {
   exit(0);
   }
 
if (statbuf.st_size == 0)
   {
   unlink(file);
   Debug("Nothing to report in %s\n",file);
   return;
   }

Debug("Mailing results of (%s) to (%s)\n",file,to);
 
if (strlen(to) == 0)
   {
   return;
   }

if ((fp=fopen(file,"r")) == NULL)
   {
   snprintf(VBUFF,bufsize-1,"Couldn't open file %s",file);
   CfLog(cferror,VBUFF,"fopen");
   return;
   }
 
 
if ((hp = gethostbyname(VMAILSERVER)) == NULL)
   {
   printf("Unknown host: %s\n", VMAILSERVER);
   printf("Make sure that fully qualified names can be looked up at your site!\n");
   printf("i.e. www.gnu.org, not just www. If you use NIS or /etc/hosts\n");
   printf("make sure that the full form is registered too as an alias!\n");
   return;
   }

if ((server = getservbyname("smtp","tcp")) == NULL)
   {
   perror("getservbyname");
   return;
   }

bzero(&raddr,sizeof(raddr));

raddr.sin_port = (unsigned int) server->s_port;
raddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
raddr.sin_family = AF_INET;  

if ((sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
   {
   CfLog(cferror,"Couldn't open a socket","socket");
   return;
   }
   
if (connect(sd,(void *) &raddr,sizeof(raddr)) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't connect to host %s\n",VMAILSERVER);
   CfLog(cfinform,OUTPUT,"connect");
   close(sd);
   return;
   }

sprintf(VBUFF,"HELO %s\n",VFQNAME); 
Dialogue(sd,VBUFF);

sprintf(VBUFF,"MAIL FROM: <cfengine@%s>\n",VFQNAME);
Dialogue(sd,VBUFF);

sprintf(VBUFF,"RCPT TO: <%s>\n",to);
Dialogue(sd,VBUFF);

Dialogue(sd,"DATA\n"); 

sprintf(VBUFF,"Subject: (%s/%s)\n",VFQNAME,VIPADDRESS); 
sent=send(sd,VBUFF,strlen(VBUFF),0);
sprintf(VBUFF,"To: %s\n\n",to); 
sent=send(sd,VBUFF,strlen(VBUFF),0);

while(!feof(fp))
   {
   VBUFF[0] = '\0';
   fgets(VBUFF,bufsize,fp);
   count++;
   sent=send(sd,VBUFF,strlen(VBUFF),0);
   
   if (count > 100)
      {
      sprintf(VBUFF,"\n[Mail truncated by cfengine. File is at %s on %s]\n",file,VFQNAME);
      sent=send(sd,VBUFF,strlen(VBUFF),0);
      break;
      }
   } 

Dialogue(sd,".\n"); 
Dialogue(sd,"QUIT\n");

 
fclose(fp);
close(sd); 
Debug("Done sending mail\n");
}

/******************************************************************/
/* Level 5                                                        */
/******************************************************************/

void Dialogue(sd,s)

int sd;
char *s;

{ int sent;
  char ch,buf[bufsize];

snprintf(buf,bufsize-1,s);
sent=send(sd,s,strlen(s),0);

Debug("SENT(%d)->%s",sent,s);

while (recv(sd,&ch,1,0))
   {
   if (ch == '\n' || ch == '\0')
      {
      break;
      }
   } 
}

/******************************************************************/
/*  Dummies                                                       */
/******************************************************************/

void RotateFiles(name,number)

char *name;
int number;

{
 /* dummy */
}


void RecursiveTidySpecialArea(name,tp,maxrecurse)

char *name;
struct Tidy *tp;
int maxrecurse;

{
}


void yyerror(s)

char *s;

{
 printf("%s\n",s);
}

/* EOF */



