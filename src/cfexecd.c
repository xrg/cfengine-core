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

#ifdef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
pthread_attr_t PTHREADDEFAULTS;
pthread_mutex_t MUTEX_COUNT = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
pthread_mutex_t MUTEX_HOSTNAME = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#else
# if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
pthread_attr_t PTHREADDEFAULTS;
pthread_mutex_t MUTEX_COUNT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_HOSTNAME = PTHREAD_MUTEX_INITIALIZER;
# endif
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
   { "no-splay",required_argument,0,'q' },
   { "no-fork",no_argument,0,'F' },
   { "once",no_argument,0,'1'},
   { "foreground",no_argument,0,'g'},
   { "parse-only",no_argument,0,'p'},
   { "ld-library-path",required_argument,0,'L'},
   { NULL,0,0,0 }
   };

char MAILTO[CF_BUFSIZE];
char MAILFROM[CF_BUFSIZE];
int MAXLINES = -1;
const int INF_LINES = -2;

/*******************************************************************/
/* Functions internal to cfservd.c                                 */
/*******************************************************************/

void CheckOptsAndInit ARGLIST((int argc,char **argv));
void StartServer ARGLIST((int argc, char **argv));
void Syntax ARGLIST((void));
void *ExitCleanly ARGLIST((void));
void *LocalExec ARGLIST((void *dummy));
void MailResult ARGLIST((char *filename, char *to));
int ScheduleRun ARGLIST((void));
void AddClassToHeap ARGLIST((char *class));
void DeleteClassFromHeap ARGLIST((char *class));
int Dialogue  ARGLIST((int sd,char *class));
int OpeningCeremony(int sd);
void GetCfStuff ARGLIST((void));
void Banner ARGLIST((char * s));

/* 
 * HvB: Bas van der Vlies
*/
int CompareResult ARGLIST((char *filename, char *prev_file));
int FileChecksum ARGLIST((char *filename, unsigned char *digest, char type));

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (int argc,char **argv)

{
CheckOptsAndInit(argc,argv);

StartServer(argc,argv);

if (!ONCE)
   {
   RestoreExecLock(); 
   ReleaseCurrentLock();
   }

 return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(int argc,char **argv)

{ extern char *optarg;
  int optindex = 0;
  char ld_library_path[CF_BUFSIZE];
  int c;

ld_library_path[0] = '\0';

Banner("Check options");

NOSPLAY = false; 

sprintf(VPREFIX, "cfexecd"); 
openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);

while ((c=getopt_long(argc,argv,"L:d:vhpFV1g",CFDOPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
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
                VERBOSE = true;
                printf("cfexecd Debug mode: running in foreground\n");
                break;

      case 'v': VERBOSE = true;
         break;

      case 'V': printf("GNU %s-%s daemon\n%s\n",PACKAGE,VERSION,COPYRIGHT);
          printf("This program is covered by the GNU Public License and may be\n");
          printf("copied free of charge. No warrenty is implied.\n\n");
          exit(0);
          break;
          
      case 'q': NOSPLAY = true;
          break;
          
      case 'p': PARSEONLY = true;
          break;
          
      case 'g': NO_FORK = true;
         break;
         
      case 'L': 
          snprintf(ld_library_path,CF_BUFSIZE-1,"LD_LIBRARY_PATH=%s",optarg);
          if (putenv(strdup(ld_library_path)) != 0)
             {
             }
          
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

 /* XXX Initialize workdir for non privileged users */

strcpy(CFWORKDIR,WORKDIR);

if (getuid() > 0)
   {
   char *homedir;
   if ((homedir = getenv("HOME")) != NULL)
      {
      strcpy(CFWORKDIR,homedir);
      strcat(CFWORKDIR,"/.cfagent");
      }
   }

snprintf(VBUFF,CF_BUFSIZE,"%s/inputs/update.conf",CFWORKDIR);
MakeDirectoriesFor(VBUFF,'y');
snprintf(VBUFF,CF_BUFSIZE,"%s/bin/cfagent -D from_cfexecd",CFWORKDIR);
MakeDirectoriesFor(VBUFF,'y');
snprintf(VBUFF,CF_BUFSIZE,"%s/outputs/spooled_reports",CFWORKDIR);
MakeDirectoriesFor(VBUFF,'y');

snprintf(VBUFF,CF_BUFSIZE,"%s/inputs",CFWORKDIR);
chmod(VBUFF,0700); 
snprintf(VBUFF,CF_BUFSIZE,"%s/outputs",CFWORKDIR);
chmod(VBUFF,0700);

strncpy(VLOCKDIR,CFWORKDIR,CF_BUFSIZE-1);
strncpy(VLOGDIR,CFWORKDIR,CF_BUFSIZE-1);

VCANONICALFILE = strdup(CanonifyName(VINPUTFILE));
GetNameInfo();

strcpy(VUQNAME,VSYSNAME.nodename);

MAILTO[0] = '\0';
MAILFROM[0] = '\0';
VIPADDRESS[0] = '\0';
VMAILSERVER[0] = '\0';
}

/*******************************************************************/

void StartServer(int argc,char **argv)

{ int pid,time_to_run = false;
  time_t now = time(NULL); 

Banner("Starting server");
  
if ((!NO_FORK) && (fork() != 0))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"cfexecd starting %.24s\n",ctime(&now));
   CfLog(cfinform,OUTPUT,"");
   exit(0);
   }

if (!ONCE)
   {
   if (!GetLock("cfexecd","execd",0,0,VUQNAME,now))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"cfexecd: Couldn't get a lock -- exists or too soon: IfElapsed %d, ExpireAfter %d\n",0,0);
      CfLog(cfverbose,OUTPUT,"");
      return;
      }

   SaveExecLock(); 
   }
 
if (!NO_FORK)
  {
  ActAsDaemon(0);
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
         if (!GetLock("cfd","exec",CF_EXEC_IFELAPSED,CF_EXEC_EXPIREAFTER,VUQNAME,time(NULL)))
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"cfexecd: Couldn't get exec lock -- exists or too soon: IfElapsed %d, ExpireAfter %d\n",CF_EXEC_IFELAPSED,CF_EXEC_EXPIREAFTER);
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
#endif
#ifndef NT
#if (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
         
         pthread_attr_init(&PTHREADDEFAULTS);
         pthread_attr_setdetachstate(&PTHREADDEFAULTS,PTHREAD_CREATE_DETACHED);
         
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
         pthread_attr_setstacksize(&PTHREADDEFAULTS,(size_t)2048*1024);
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
  char cfcom[CF_BUFSIZE];
  static char line[CF_BUFSIZE];

/* Do not init variables here, in case we can use previous values. */
  
snprintf(cfcom,CF_BUFSIZE-1,"%s/bin/cfagent -Q smtpserver,sysadm,fqhost,ipaddress,EmailMaxLines,EmailFrom,EmailTo -D from_cfexecd",CFWORKDIR);

if ((pp=cfpopen(cfcom,"r")) ==  NULL)
   {
   CfLog(cferror,"Couldn't start cfengine!","cfpopen");
   line[0] = '\0';
   return;
   }

while (!feof(pp))
   {
   char name[CF_BUFSIZE],content[CF_BUFSIZE];
   line[0] = '\0'; 
   fgets(line,CF_BUFSIZE,pp); 
   Chop(line); 
   sscanf(line,"%[^=]=%[^\n]",name,content);
    
   if (strcmp(name,"sysadm") == 0)
      {
      strncpy(MAILTO,content,CF_MAXVARSIZE-1);
      Debug("%s\n",line);
      continue;
      }

   if (strcmp(name,"smtpserver") == 0)
      {
      Debug("%s/%s\n",name,content);
      strncpy(VMAILSERVER,content,CF_MAXVARSIZE-1);
      continue;
      }
   
   if (strcmp(name,"fqhost") == 0)
      {
      Debug("%s/%s\n",name,content);
      strncpy(VFQNAME,content,CF_MAXVARSIZE-1);
      continue;
      }

   if (strcmp(name,"ipaddress") == 0)
      {
      Debug("%s/%s\n",name,content);
      strncpy(VIPADDRESS,content,CF_MAXVARSIZE-1);
      continue;
      }

   if (strcmp(name,"EmailMaxLines") == 0)
      {
      Debug("%s/%s\n",name,content);
      MAXLINES = 20;
      sscanf(content,"%d",&MAXLINES);
      continue;
      }

   if (strcmp(name,"EmailFrom") == 0)
      {
      Debug("%s/%s\n",name,content);
      if (strstr(content,"$") == NULL && strlen(content) > 0)
         {
         strncpy(MAILFROM,content,CF_MAXVARSIZE-1);
         continue;
         }
      }
   
   if (strcmp(name,"EmailTo") == 0)
      {
      Debug("%s/%s\n",name,content);
      if (strstr(content,"$") == NULL && strlen(content) > 0)
         {
         strncpy(MAILTO,content,CF_MAXVARSIZE-1);
         continue;
         }
      }
   }

cfpclose(pp);

Debug("Got(to:%s,serv:%s,host:%s,ip:%s,max:%d,from:%s)\n",MAILTO,VMAILSERVER,VFQNAME,VIPADDRESS,MAXLINES,MAILFROM); 
 
/* Now get schedule */
  
snprintf(cfcom,CF_BUFSIZE-1,"%s/bin/cfagent -z -D from_cfexecd",CFWORKDIR);
 
if ((pp=cfpopen(cfcom,"r")) ==  NULL)
   {
   CfLog(cferror,"Couldn't start cfengine!","cfpopen");
   line[0] = '\0';
   return;
   }

DeleteItemList(SCHEDULE);
SCHEDULE = NULL; 
 
while (!feof(pp))
   {
   line[0] = '\0';
   VBUFF[0] = '\0';
   fgets(line,CF_BUFSIZE,pp);
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
VHEAP = NULL; 
GetNameInfo();
return false;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void *ExitCleanly()

{
ReleaseCurrentLock();
closelog();
 
exit(0);
}

/**************************************************************/

static char *timestamp(time_t stamp, char *buf, size_t len)
{
  struct tm *ltime;

  ltime = localtime(&stamp);
  snprintf(buf, len, "%04d-%02d-%02d--%02d-%02d-%02d",
		      ltime->tm_year+1900,
		      ltime->tm_mon+1,
		      ltime->tm_mday,
		      ltime->tm_hour,
		      ltime->tm_min,
		      ltime->tm_sec);
  return buf;
}

/**************************************************************/

void *LocalExec(void *dummy)

{ FILE *pp; 
  char line[CF_BUFSIZE],filename[CF_BUFSIZE],*sp;
  char cmd[CF_BUFSIZE];
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

if (NOSPLAY)
   {
   snprintf(cmd,CF_BUFSIZE-1,"%s/bin/cfagent -q",CFWORKDIR);
   }
else
   {
   snprintf(cmd,CF_BUFSIZE-1,"%s/bin/cfagent",CFWORKDIR);
   }
 
timestamp(starttime, line, CF_BUFSIZE);

snprintf(filename,CF_BUFSIZE-1,"%s/outputs/cf_%s_%s",CFWORKDIR,CanonifyName(VFQNAME),line);
 
/* What if no more processes? Could sacrifice and exec() - but we need a sentinel */

if ((fp = fopen(filename,"w")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Couldn't open %s\n",filename);
   CfLog(cferror,OUTPUT,"fopen");
   return NULL;
   }
 
if ((pp = cfpopen(cmd,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Couldn't open pipe to command %s\n",cmd);
   CfLog(cferror,OUTPUT,"cfpopen");
   fclose(fp);
   return NULL;
   }
 
while (!feof(pp) && ReadLine(line,CF_BUFSIZE,pp))
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
         strncat(line,"\n",CF_BUFSIZE-1-strlen(line));
         if ((strchr(line,'\n')) == NULL)
            {
            line[CF_BUFSIZE-2] = '\n';
            }
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

/*
 * HvB: Bas van der Vlies
 * This function is "stolen" from the checksum.c. Else we have to link so
 * many more files with cfexecd. There also some small changes.
*/

int FileChecksum(char *filename,unsigned char digest[EVP_MAX_MD_SIZE+1],char type)


{ FILE *file;
  EVP_MD_CTX context;
  int len, md_len;
  unsigned char buffer[1024];
  const EVP_MD *md = NULL;

Debug2("ChecksumFile(%c,%s)\n",type,filename);
OpenSSL_add_all_digests();

if ((file = fopen (filename, "rb")) == NULL)
   {
   printf ("%s can't be opened\n", filename);
   }
else
   {
   switch (type)
      {
      case 's': md = EVP_get_digestbyname("sha");
           break;
      case 'm': md = EVP_get_digestbyname("md5");
         break;
      default: FatalError("Software failure in ChecksumFile");
      }

   if (!md)
      {
      return 0;
      }
   
   EVP_DigestInit(&context,md);

   while (len = fread(buffer,1,1024,file))
      {
      EVP_DigestUpdate(&context,buffer,len);
      }

   EVP_DigestFinal(&context,digest,&md_len);

   /* Digest length stored in md_len */
   fclose (file);

   return(md_len);
   }

return 0; 
}

/*******************************************************************/

/*
 * HvB: Bas van der Vlies
 *  This function compare the current result with the previous run
 *  and returns:
 *    0 : if the files are the same
 *    1 : if the files differ
 *
 *  Changes made by: Jeff Wasilko always update the symlink
*/
int CompareResult(char *filename,char *prev_file)

{ int i;
  char digest1[EVP_MAX_MD_SIZE+1];
  char digest2[EVP_MAX_MD_SIZE+1];
  int  md_len1, md_len2;
  FILE *fp;
  int rtn = 0;

Verbose("Comparing files  %s with %s\n", prev_file, filename);

if ((fp=fopen(prev_file,"r")) != NULL)
   {
   fclose(fp);

   md_len1 = FileChecksum(prev_file, digest1, 'm');
   md_len2 = FileChecksum(filename,  digest2, 'm');

   if (md_len1 != md_len2)
      {
      rtn = 1;
      }
   else
      {
      for (i = 0; i < md_len1; i++)
          if (digest1[i] != digest2[i])
             {
      rtn = 1;
      break;
      }
      }
   }
else
   {
   /* no previous file */
   rtn = 1;
   }

/* always update the symlink. */   
unlink(prev_file);
if (symlink(filename, prev_file) == -1 )
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not link %s and %s",filename,prev_file);
   CfLog(cfinform,OUTPUT,"symlink");
   rtn = 1;
   }

return(rtn);
}



/***********************************************************************/

void MailResult(char *file,char *to)

{ int sd, sent, count = 0, anomaly = false;
  char domain[256];
  struct hostent *hp;
  struct sockaddr_in raddr;
  struct servent *server;
  struct stat statbuf;
  time_t now = time(NULL);
  FILE *fp;

  /* HvB: Bas van der Vlies */
  char prev_file[CF_BUFSIZE];

if ((strlen(VMAILSERVER) == 0) || (strlen(to) == 0))
   {
   /* Syslog should have done this*/
   return;
   }
  
if (stat(file,&statbuf) == -1)
   {
   exit(0);
   }

/* HvB: Bas van der Vlies */
snprintf(prev_file,CF_BUFSIZE-1,"%s/outputs/previous",CFWORKDIR);
 
if (statbuf.st_size == 0)
   {
   unlink(file);

   /* HvB: Bas van der Vlies 
    * also remove previous file 
   */
   if ((fp=fopen(prev_file, "r")) != NULL )
      {
      fclose(fp);
      unlink(prev_file);
      }

   Debug("Nothing to report in %s\n",file);
   return;
   }

/*
 * HvB: Check if the result is the same as the previous run.
 *
*/
if ( CompareResult(file,prev_file) == 0 ) 
   {
   Verbose("Previous output is the same as current so do not mail it\n");
   return;
   }

if (MAXLINES == 0)
   {
   Debug("Not mailing: EmailMaxLines was zero\n");
   return;
   }
 
Debug("Mailing results of (%s) to (%s)\n",file,to);
 
if (strlen(to) == 0)
   {
   return;
   }

/* Check first for anomalies - for subject header */
 
if ((fp=fopen(file,"r")) == NULL)
   {
   snprintf(VBUFF,CF_BUFSIZE-1,"Couldn't open file %s",file);
   CfLog(cferror,VBUFF,"fopen");
   return;
   }

 while (!feof(fp))
    {
    VBUFF[0] = '\0';
    fgets(VBUFF,CF_BUFSIZE,fp);
    if (strstr(VBUFF,"entropy"))
       {
       anomaly = true;
       break;
       }
    }
 
fclose(fp);
 
if ((fp=fopen(file,"r")) == NULL)
   {
   snprintf(VBUFF,CF_BUFSIZE-1,"Couldn't open file %s",file);
   CfLog(cferror,VBUFF,"fopen");
   return;
   }
 
Debug("Looking up hostname %s\n\n",VMAILSERVER);

if ((hp = gethostbyname(VMAILSERVER)) == NULL)
   {
   printf("Unknown host: %s\n", VMAILSERVER);
   printf("Make sure that fully qualified names can be looked up at your site!\n");
   printf("i.e. www.gnu.org, not just www. If you use NIS or /etc/hosts\n");
   printf("make sure that the full form is registered too as an alias!\n");
   fclose(fp);
   return;
   }

if ((server = getservbyname("smtp","tcp")) == NULL)
   {
   CfLog(cferror,"Unable to lookup smtp service","getservbyname");
   fclose(fp);
   return;
   }

memset(&raddr,0,sizeof(raddr));

raddr.sin_port = (unsigned int) server->s_port;
raddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
raddr.sin_family = AF_INET;  

Debug("Connecting...\n");

if ((sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
   {
   CfLog(cferror,"Couldn't open a socket","socket");
   fclose(fp);
   return;
   }
   
if (connect(sd,(void *) &raddr,sizeof(raddr)) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't connect to host %s\n",VMAILSERVER);
   CfLog(cfinform,OUTPUT,"connect");
   fclose(fp);
   close(sd);
   return;
   }

/* read greeting */
 
if (!Dialogue(sd,NULL))
   {
   goto mail_err;
   }
 
sprintf(VBUFF,"HELO %s\r\n",VFQNAME); 
Debug("%s",VBUFF);

if (!Dialogue(sd,VBUFF))
   {
   goto mail_err;
   }

if (strlen(MAILFROM) > 0)
   {
   sprintf(VBUFF,"MAIL FROM: <%s>\r\n",MAILFROM);
   Debug("%s",VBUFF);   
   }
else
   {
   domain[0] = '\0';
   sscanf(to,"%*[^@]@%.64s",domain);
   
   if (strlen(domain) > 0)
      {
      sprintf(VBUFF,"MAIL FROM: <cfengine@%s>\r\n",domain);
      Debug("%s",VBUFF);
      }
   else
      {
      sprintf(VBUFF,"MAIL FROM: <%s>\r\n",to);
      Debug("%s",VBUFF);   
      }
   }

if (!Dialogue(sd,VBUFF))
   {
   goto mail_err;
   }
 
sprintf(VBUFF,"RCPT TO: <%s>\r\n",to);
Debug("%s",VBUFF);

if (!Dialogue(sd,VBUFF))
    {
    goto mail_err;
    }

if (!Dialogue(sd,"DATA\r\n"))
    {
    goto mail_err;
    }

if (anomaly)
   {
   sprintf(VBUFF,"Subject: **!! (%s/%s)\r\n",VFQNAME,VIPADDRESS);
   Debug("%s",VBUFF);
   }
else
   {
   sprintf(VBUFF,"Subject: (%s/%s)\r\n",VFQNAME,VIPADDRESS);
   Debug("%s",VBUFF);
   }
 
sent=send(sd,VBUFF,strlen(VBUFF),0);

 
/* -- Added by Michael Rice <mrice@digitalmotorworks.com>*/

#if defined LINUX || defined NETBSD || defined FREEBSD || defined OPENBSD
strftime(VBUFF,CF_BUFSIZE,"Date: %a, %d %b %Y %H:%M:%S %z\r\n",localtime(&now));
sent=send(sd,VBUFF,strlen(VBUFF),0);
#endif

 if (strlen(MAILFROM) == 0)
    {
    sprintf(VBUFF,"From: cfengine@%s\r\n",VFQNAME);
    Debug("%s",VBUFF);
    }
 else
    {
    sprintf(VBUFF,"From: cfengine@%s\r\n",VFQNAME);
    Debug("%s",VBUFF);    
    }
 
sent=send(sd,VBUFF,strlen(VBUFF),0);

sprintf(VBUFF,"To: %s\r\n\r\n",to); 
Debug("%s",VBUFF);
sent=send(sd,VBUFF,strlen(VBUFF),0);

while(!feof(fp))
   {
   VBUFF[0] = '\0';
   fgets(VBUFF,CF_BUFSIZE,fp);
   Debug("%s",VBUFF);
   
   if (strlen(VBUFF) > 0)
      {
      VBUFF[strlen(VBUFF)-1] = '\r';
      strcat(VBUFF, "\n");
      count++;
      sent=send(sd,VBUFF,strlen(VBUFF),0);
      }
   
   if ((MAXLINES != INF_LINES) && (count > MAXLINES))
      {
      sprintf(VBUFF,"\r\n[Mail truncated by cfengine. File is at %s on %s]\r\n",file,VFQNAME);
      sent=send(sd,VBUFF,strlen(VBUFF),0);
      break;
      }
   } 

if (!Dialogue(sd,".\r\n"))
   {
   Debug("mail_err\n");
   goto mail_err;
   }
 
Dialogue(sd,"QUIT\r\n");
Debug("Done sending mail\n");
fclose(fp);
close(sd);
return;
 
mail_err: 

fclose(fp);
close(sd); 
sprintf(VBUFF, "Cannot mail to %s.", to);
CfLog(cferror,VBUFF,"");
}

/******************************************************************/
/* Level 5                                                        */
/******************************************************************/

int Dialogue(int sd,char *s)

{ int sent;
  char ch,f = '\0';
  int charpos,rfclinetype = ' ';

if ((s != NULL) && (*s != '\0'))
   {
   sent=send(sd,s,strlen(s),0);
   Debug("SENT(%d)->%s",sent,s);
   }
else
   {
   Debug("Nothing to send .. waiting for opening\n");
   }

charpos = 0;
 
while (recv(sd,&ch,1,0))
   {
   charpos++;
   
   if (f == '\0')
      {
      f = ch;
      }

   if (charpos == 4)  /* Multiline RFC in form 222-Message with hyphen at pos 4 */
      {
      rfclinetype = ch;
      }
   
   Debug("%c",ch);
   
   if (ch == '\n' || ch == '\0')
      {
      charpos = 0;
      
      if (rfclinetype == ' ')
         {
         break;
         }
      }
   }

return ((f == '2') || (f == '3')); /* return code 200 or 300 from smtp*/
}

/******************************************************************/
/*  Dummies                                                       */
/******************************************************************/

void Banner(char *string)

{
Verbose("---------------------------------------------------------------------\n");
Verbose("%s\n",string);
Verbose("---------------------------------------------------------------------\n\n");
}

/*********************************************************************/

void RotateFiles(char *name,int number)

{
 /* dummy */
}


int RecursiveTidySpecialArea(char *name,struct Tidy *tp,int maxrecurse,struct stat *sb)

{
 return true;
}


void yyerror(char *s)

{
 printf("%s\n",s);
}


char *GetMacroValue(char *s,char *sp)

{
 return NULL;
}


void AddMacroValue(char *scope,char *name,char *value)

{
}


/* EOF */



