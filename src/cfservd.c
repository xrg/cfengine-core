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
/*  Cfservd : remote server daemon                                 */
/*                                                                 */
/*******************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"
#include "cfservd.h"
#include <db.h>

/*******************************************************************/
/* Pthreads                                                        */
/*******************************************************************/

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
pthread_attr_t PTHREADDEFAULTS;
pthread_mutex_t MUTEX_COUNT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_HOSTNAME = PTHREAD_MUTEX_INITIALIZER;
#endif

/*******************************************************************/
/* GLOBAL VARIABLES                                                */
/*******************************************************************/

int CLOCK_DRIFT = 3600;   /* 1hr */
int CFD_MAXPROCESSES = 0;
int ACTIVE_THREADS = 0;
int NO_FORK = false;
int MULTITHREAD = false;
int CHECK_RFC931 = false;
int CFD_INTERVAL = 0;
int DENYBADCLOCKS = true;
int MULTIPLECONNS = false;
int TRIES = 0;
int MAXTRIES = 5;
int LOGCONNS = false;

/*
 * HvB
*/
char BINDINTERFACE[bufsize];

struct option CFDOPTIONS[] =
   {
   { "help",no_argument,0,'h' },
   { "debug",optional_argument,0,'d' }, 
   { "verbose",no_argument,0,'v' },
   { "file",required_argument,0,'f' },
   { "no-fork",no_argument,0,'F' },
   { "parse-only",no_argument,0,'p'},
   { "multithread",no_argument,0,'m'},
   { "ld-library-path",required_argument,0,'L'},
   { NULL,0,0,0 }
   };


struct Item *CONNECTIONLIST = NULL;


/*******************************************************************/
/* Functions internal to cfservd.c                                 */
/*******************************************************************/

void CheckOptsAndInit ARGLIST((int argc,char **argv));
void CheckVariables ARGLIST((void));
void SummarizeParsing ARGLIST((void));
void StartServer ARGLIST((int argc, char **argv));
int OpenReceiverChannel ARGLIST((void));
void Syntax ARGLIST((void));
void PurgeOldConnections ARGLIST((struct Item **list,time_t now));
void SpawnConnection ARGLIST((int sd_reply, char *ipaddr));
void CheckFileChanges ARGLIST((int argc, char **argv, int sd));
void *HandleConnection ARGLIST((struct cfd_connection *conn));
int BusyWithConnection ARGLIST((struct cfd_connection *conn));
void *ExitCleanly ARGLIST((int signum));
int MatchClasses ARGLIST((struct cfd_connection *conn));
void DoExec ARGLIST((struct cfd_connection *conn, char *sendbuffer, char *args));
int GetCommand ARGLIST((char *str));
int VerifyConnection ARGLIST((struct cfd_connection *conn, char *buf));
void RefuseAccess ARGLIST((struct cfd_connection *conn, char *sendbuffer, int size, char *errormsg));
int AccessControl ARGLIST((char *filename, struct cfd_connection *conn, int encrypt));
int CheckStoreKey  ARGLIST((struct cfd_connection *conn, RSA *key));
int StatFile ARGLIST((struct cfd_connection *conn, char *sendbuffer, char *filename));
void CfGetFile ARGLIST((struct cfd_get_arg *args));
void CompareLocalChecksum ARGLIST((struct cfd_connection *conn, char *sendbuffer, char *recvbuffer));
int CfOpenDirectory ARGLIST((struct cfd_connection *conn, char *sendbuffer, char *dirname));
void Terminate ARGLIST((int sd));
void DeleteAuthList ARGLIST((struct Auth *ap));
int AllowedUser ARGLIST((char *user));
void ReplyNothing ARGLIST((struct cfd_connection *conn));
struct cfd_connection *NewConn ARGLIST((int sd));
void DeleteConn ARGLIST((struct cfd_connection *conn));
time_t SecondsTillAuto ARGLIST((void));
void SetAuto ARGLIST((int seconds));
int cfscanf ARGLIST((char *in, int len1, int len2, char *out1, char *out2, char *out3));
int AuthenticationDialogue ARGLIST((struct cfd_connection *conn,char *buffer));
char *MapAddress ARGLIST((char *addr));
int IsWildKnownHost ARGLIST((RSA *oldkey,RSA *newkey,char *addr,char *user));
void AddToKeyDB ARGLIST((RSA *key,char *addr));
int SafeOpen ARGLIST((char *filename));
void SafeClose ARGLIST((int fd));

/*
 * HvB
*/
unsigned find_inet_addr ARGLIST((char *host));

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (argc,argv)

int argc;
char **argv;

{
CheckOptsAndInit(argc,argv);
GetInterfaceInfo();
ParseInputFile("cfservd.conf");
CheckVariables();
SummarizeParsing();

if (PARSEONLY)
   {
   exit(0);
   }

StartServer(argc,argv);

/* Never exit here */
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
  struct stat statbuf;
  int c;

SetContext("server");
sprintf(VPREFIX, "cfservd");
CfOpenLog();
strcpy(VINPUTFILE,CFD_INPUT);
strcpy(CFLOCK,"cfservd");
OUTPUT[0] = '\0';

/*
 * HvB: Bas van der Vlies
*/
BINDINTERFACE[0] = '\0';

SetSignals();
 
ISCFENGINE = false;   /* Switch for the parser */
PARSEONLY  = false;

ld_library_path[0] = '\0';

InstallObject("server"); 

AddClassToHeap("any");      /* This is a reserved word / wildcard */

while ((c=getopt_long(argc,argv,"L:d:f:vmhpFV",CFDOPTIONS,&optindex)) != EOF)
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
		printf("cfservd: Debug mode: running in foreground\n");
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

      case 'F': NO_FORK = true;
	        break;

      case 'L': Verbose("Setting LD_LIBRARY_PATH=%s\n",optarg);
	        snprintf(ld_library_path,bufsize-1,"LD_LIBRARY_PATH=%s",optarg);
    	        putenv(ld_library_path);
		break;
	  
      case 'm': /* No longer needed */
	        break;

      default:  Syntax();
                exit(1);

      }
   }


LOGGING = true;                    /* Do output to syslog */
 


GetNameInfo();


if (stat("/etc/redhat-release",&statbuf) != -1)
   {
   Verbose("This appears to be a redhat system.\n");
   AddClassToHeap("redhat");
   linux_redhat_version();
   }

if (stat("/etc/SuSE-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a SuSE system.\n");
   AddClassToHeap("SuSE");
   linux_suse_version();
   }

if (stat("/etc/slackware-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a slackware system.\n");
   AddClassToHeap("slackware");
   }
 
if (stat("/etc/debian_version",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a debian system.\n");
   AddClassToHeap("debian");
   debian_version();
   }

if (stat("/etc/UnitedLinux-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a UnitedLinux system.\n");
   AddClassToHeap("UnitedLinux");
   }

if ((CFINITSTARTTIME = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","time");
   }

if ((CFSTARTTIME = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","time");
   }
 
snprintf(VBUFF,bufsize,"%s/test",WORKDIR);

MakeDirectoriesFor(VBUFF,'y');
strncpy(VLOGDIR,WORKDIR,bufsize-1);
strncpy(VLOCKDIR,WORKDIR,bufsize-1);

VIFELAPSED = exec_ifelapsed;
VEXPIREAFTER = exec_expireafter;
 
strcpy(VDOMAIN,"undefined.domain");

VCANONICALFILE = strdup(CanonifyName(VINPUTFILE));
 
OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();
CheckWorkDirectories();

RandomSeed(); 
LoadSecretKeys();
}

/*******************************************************************/

void CheckVariables()

{ struct stat statbuf;
  int i, value = -1;

#ifdef HAVE_PTHREAD_H
 CfLog(cfinform,"cfservd Multithreaded version","");
#else
 CfLog(cfinform,"cfservd Single threaded version","");
#endif

strncpy(VFQNAME,VSYSNAME.nodename,maxvarsize-1);

if ((CFDSTARTTIME = time((time_t *)NULL)) == -1)
   {
   printf("Couldn't read system clock\n");
   }

if (OptionIs(CONTEXTID,"CheckIdent", true))
   {
   CHECK_RFC931 = true;
   }

if (OptionIs(CONTEXTID,"DenyBadClocks", false)) 
   {
   DENYBADCLOCKS = false;
   }

if (OptionIs(CONTEXTID,"LogAllConnections", true)) 
   {
   LOGCONNS = true;
   }

if (GetMacroValue(CONTEXTID,"ChecksumDatabase"))
   {
   ExpandVarstring("$(ChecksumDatabase)",VBUFF,NULL);

   CHECKSUMDB = strdup(VBUFF);

   if (*CHECKSUMDB != '/')
      {
      FatalError("$(ChecksumDatabase) does not expand to an absolute filename\n");
      }
   }

 
bzero(VBUFF,bufsize);
bzero(CFRUNCOMMAND,bufsize);
 
if (GetMacroValue(CONTEXTID,"cfrunCommand"))
   {
   ExpandVarstring("$(cfrunCommand)",VBUFF,NULL);

   if (*VBUFF != '/')
      {
      FatalError("$(cfrunCommand) does not expand to an absolute filename\n");
      }

   sscanf(VBUFF,"%4095s",CFRUNCOMMAND);
   Debug("cfrunCommand is %s\n",CFRUNCOMMAND);

   if (stat(CFRUNCOMMAND,&statbuf) == -1)
      {
      FatalError("$(cfrunCommand) points to a non-existent file\n");
      }
   }

if (GetMacroValue(CONTEXTID,"MaxConnections"))
   {
   bzero(VBUFF,bufsize);
   ExpandVarstring("$(MaxConnections)",VBUFF,NULL);
   Debug("$(MaxConnections) Expanded to %s\n",VBUFF);

   CFD_MAXPROCESSES = atoi(VBUFF);
   
   MAXTRIES = CFD_MAXPROCESSES / 3;  /* How many attempted connections over max
				        before we commit suicide? */
   if ((CFD_MAXPROCESSES < 1) || (CFD_MAXPROCESSES > 1000))
      {
      FatalError("cfservd MaxConnections with silly value");
      }
    }
else
   {
   CFD_MAXPROCESSES = 10;
   MAXTRIES = 5;
   }

CFD_INTERVAL = 0;

Debug("MaxConnections = %d\n",CFD_MAXPROCESSES);

CHECKSUMUPDATES = true;

if (OptionIs(CONTEXTID,"ChecksumUpdates", false))
  {
  CHECKSUMUPDATES = false;
  } 
 
i = 0;

if (strstr(VSYSNAME.nodename,ToLowerStr(VDOMAIN)))
   {
   strncpy(VFQNAME,VSYSNAME.nodename,maxvarsize-1);
   
   while(VSYSNAME.nodename[i++] != '.')
      {
      }
   
   strncpy(VUQNAME,VSYSNAME.nodename,i-1);
   }
else
   {
   snprintf(VFQNAME,bufsize,"%s.%s",VSYSNAME.nodename,ToLowerStr(VDOMAIN));
   strncpy(VUQNAME,VSYSNAME.nodename,maxvarsize-1);
   }

/*
 * HvB: Bas van der Vlies
 * bind to only one interface
*/
if (GetMacroValue(CONTEXTID,"BindToInterface"))
   {
   bzero(VBUFF,bufsize);
   ExpandVarstring("$(BindToInterface)",VBUFF,NULL);
   strncpy(BINDINTERFACE, VBUFF, bufsize-1);
   Debug("$(BindToInterface) Expanded to %s\n",BINDINTERFACE);
   }


}

/*******************************************************************/

void SummarizeParsing()

{ struct Auth *ptr;
  struct Item *ip,*ipr;

if (VERBOSE || DEBUG || D2 || D3)
   {
   ListDefinedClasses();
   }
 
if (DEBUG || D2 || D3)
   {
   printf("ACCESS GRANTED ----------------------:\n\n");

   for (ptr = VADMIT; ptr != NULL; ptr=ptr->next)
      {
      printf("Path: %s (encrypt=%d)\n",ptr->path,ptr->encrypt);

      for (ip = ptr->accesslist; ip != NULL; ip=ip->next)
	 {
	 printf("   Admit: %s root=",ip->name);
	 for (ipr = ptr->maproot; ipr !=NULL; ipr=ipr->next)
	    {
	    printf("%s,",ipr->name);
	    }
	 printf("\n");
	 }
      }

   printf("ACCESS DENIAL ------------------------ :\n\n");

   for (ptr = VDENY; ptr != NULL; ptr=ptr->next)
      {
      printf("Path: %s\n",ptr->path);

      for (ip = ptr->accesslist; ip != NULL; ip=ip->next)
	 {
	 printf("   Deny: %s\n",ip->name);
	 }      
      }
   
   printf("Host IPs allowed connection access :\n\n");

   for (ip = NONATTACKERLIST; ip != NULL; ip=ip->next)
      {
      printf("IP: %s\n",ip->name);
      }

   printf("Host IPs denied connection access :\n\n");

   for (ip = ATTACKERLIST; ip != NULL; ip=ip->next)
      {
      printf("IP: %s\n",ip->name);
      }

   printf("Host IPs allowed multiple connection access :\n\n");

   for (ip = MULTICONNLIST; ip != NULL; ip=ip->next)
      {
      printf("IP: %s\n",ip->name);
      }

   printf("Host IPs from whom we shall accept public keys on trust :\n\n");

   for (ip = TRUSTKEYLIST; ip != NULL; ip=ip->next)
      {
      printf("IP: %s\n",ip->name);
      }

   printf("Host IPs from NAT which we don't verify :\n\n");

   for (ip = SKIPVERIFY; ip != NULL; ip=ip->next)
      {
      printf("IP: %s\n",ip->name);
      }

   printf("Dynamical Host IPs (e.g. DHCP) whose bindings could vary over time :\n\n");

   for (ip = DHCPLIST; ip != NULL; ip=ip->next)
      {
      printf("IP: %s\n",ip->name);
      }

   }


if (ERRORCOUNT > 0)
   {
   FatalError("Execution terminated after parsing due to errors in program");
   }
}


/*******************************************************************/

void StartServer(argc,argv)

int argc;
char **argv;

{ char ipaddr[maxvarsize],intime[64];
  int sd,sd_reply,ageing;
  fd_set rset;
  time_t now;

#ifdef HAVE_GETADDRINFO
  int addrlen=sizeof(struct sockaddr_in6);
  struct sockaddr_in6 cin;
#else
  int addrlen=sizeof(struct sockaddr_in);
  struct sockaddr_in cin;
#endif
  
if ((sd = OpenReceiverChannel()) == -1)
   {
   CfLog(cferror,"Unable to start server","");
   exit(1);
   }

signal(SIGINT,(void*)ExitCleanly);
signal(SIGTERM,(void*)ExitCleanly);
signal(SIGHUP,SIG_IGN);
signal(SIGPIPE,SIG_IGN);
 
if (listen(sd,queuesize) == -1)
   {
   CfLog(cferror,"listen failed","listen");
   exit(1);
   }

Verbose("Listening for connections ...\n");

if ((!NO_FORK) && (fork() != 0))
   {
   snprintf(OUTPUT,bufsize*2,"cfservd starting %.24s\n",ctime(&CFDSTARTTIME));
   CfLog(cfinform,OUTPUT,"");
   exit(0);
   }

if (!NO_FORK)
   {
       ActAsDaemon(sd);
   }
 
ageing = 0;
FD_ZERO(&rset);
FD_SET(sd,&rset);

/* Andrew Stribblehill <ads@debian.org> -- close sd on exec */ 
fcntl(sd, F_SETFD, FD_CLOEXEC);
 
while (true)
   {
   if (ACTIVE_THREADS == 0)
      {
      CheckFileChanges(argc,argv,sd);
      }
   
   if ((select((sd+1),&rset,NULL,NULL,NULL) == -1) && (errno != EINTR))
      {
      CfLog(cferror, "select failed", "select");
      exit(1);
      }
   
   if ((sd_reply = accept(sd,(struct sockaddr *)&cin,&addrlen)) != -1)
      {
      if (ageing++ > CFD_MAXPROCESSES*50) /* Insurance against stale db */
	 {                                /* estimate number of clients */
	 unlink(CHECKSUMDB);              /* arbitrary policy ..        */
	 ageing = 0;
	 }

      bzero(ipaddr,maxvarsize);
      snprintf(ipaddr,maxvarsize-1,"%s",sockaddr_ntop((struct sockaddr *)&cin));

      Debug("Obtained IP address of %s on socket %d from accept\n",ipaddr,sd_reply);
      
      if ((NONATTACKERLIST != NULL) && !IsFuzzyItemIn(NONATTACKERLIST,MapAddress(ipaddr)))   /* Allowed Subnets */
	 {
	 snprintf(OUTPUT,bufsize*2,"Denying connection from non-authorized IP %s\n",ipaddr);
	 CfLog(cferror,OUTPUT,"");
	 close(sd_reply);
	 continue;
	 }
      
      if (IsFuzzyItemIn(ATTACKERLIST,MapAddress(ipaddr)))   /* Denied Subnets */
	 {
	 snprintf(OUTPUT,bufsize*2,"Denying connection from non-authorized IP %s\n",ipaddr);
	 CfLog(cferror,OUTPUT,"");
	 close(sd_reply);
	 continue;
	 }      
      
      if ((now = time((time_t *)NULL)) == -1)
	 {
	 now = 0;
	 }
      
      PurgeOldConnections(&CONNECTIONLIST,now);
      
      if (!IsFuzzyItemIn(MULTICONNLIST,MapAddress(ipaddr)))
	 {
	 if (IsItemIn(CONNECTIONLIST,MapAddress(ipaddr)))
	    {
	    snprintf(OUTPUT,bufsize*2,"Denying repeated connection from %s\n",ipaddr);
	    CfLog(cferror,OUTPUT,"");
	    close(sd_reply);
	    continue;
	    }
	 }

      if (LOGCONNS)
	 {
	 snprintf(OUTPUT,bufsize*2,"Accepting connection from %s\n",ipaddr);
	 CfLog(cflogonly,OUTPUT,"");
	 }

      snprintf(intime,63,"%d",(int)now);
      PrependItem(&CONNECTIONLIST,MapAddress(ipaddr),intime);
      SpawnConnection(sd_reply,ipaddr);
      }
   }
}


/*******************************************************************************/

/*
 * HvB: Bas van der Vlies
 * find_inet_addr - translate numerical or symbolic host name, from the
 *                  postfix sources and adjusted to cfengine style/syntax
*/
unsigned find_inet_addr(host)

char *host;
{

struct in_addr addr;
struct hostent *hp;

bzero(VBUFF,bufsize);

addr.s_addr = inet_addr(host);
if ((addr.s_addr == INADDR_NONE) || (addr.s_addr == 0)) 
   {
   if ((hp = gethostbyname(host)) == 0)
      {
      snprintf(VBUFF,bufsize,"\nhost not found: %s\n",host);
      FatalError(VBUFF);
      }
   if (hp->h_addrtype != AF_INET)
      {
      snprintf(VBUFF,bufsize,"unexpected address family: %d\n",hp->h_addrtype);
      FatalError(VBUFF);
      }
   if (hp->h_length != sizeof(addr))
      {
      snprintf(VBUFF,bufsize,"unexpected address length %d\n",hp->h_length);
      FatalError(VBUFF);
      }

   memcpy((char *) &addr, hp->h_addr, hp->h_length);
   }

return (addr.s_addr);
}


/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

int OpenReceiverChannel()

{ int sd,port;
  int yes=1;

  char *ptr = NULL;

  struct linger cflinger;
#ifdef HAVE_GETADDRINFO
    struct addrinfo query,*response,*ap;
#else
    struct sockaddr_in sin;
#endif
    
cflinger.l_onoff = 1;
cflinger.l_linger = 60;

port = CfenginePort();

#if HAVE_GETADDRINFO
  
bzero(&query,sizeof(struct addrinfo));

query.ai_flags = AI_PASSIVE;
query.ai_family = AF_UNSPEC;
query.ai_socktype = SOCK_STREAM;

/*
 * HvB : Bas van der Vlies
*/
if (BINDINTERFACE[0] != '\0' )
  {
  ptr = BINDINTERFACE;
  }

if (getaddrinfo(ptr,"5308",&query,&response) != 0)
   {
   CfLog(cferror,"DNS/service lookup failure","getaddrinfo");
   return -1;   
   }

sd = -1;
 
for (ap = response ; ap != NULL; ap=ap->ai_next)
   {
   if ((sd = socket(ap->ai_family,ap->ai_socktype,ap->ai_protocol)) == -1)
      {
      continue;
      }

   if (setsockopt(sd, SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof (int)) == -1)
      {
      CfLog(cferror,"Socket options were not accepted","setsockopt");
      exit(1);
      }
   
   if (setsockopt(sd, SOL_SOCKET, SO_LINGER,(char *)&cflinger,sizeof (struct linger)) == -1)
      {
      CfLog(cferror,"Socket options were not accepted","setsockopt");
      exit(1);
      }

   if (bind(sd,ap->ai_addr,ap->ai_addrlen) == 0)
      {
      Debug("Bound to address %s on %s=%d\n",sockaddr_ntop(ap->ai_addr),CLASSTEXT[VSYSTEMHARDCLASS],VSYSTEMHARDCLASS);

      if (VSYSTEMHARDCLASS == openbsd)
	 {
	 continue;  /* openbsd doesn't map ipv6 addresses */
	 }
      else
	 {
	 break;
	 }
      }
   
   CfLog(cferror,"Could not bind server address","bind");
   close(sd);
   sd = -1;
   }

if (sd < 0)
   {
   CfLog(cferror,"Couldn't open bind an open socket\n","");
   exit(1);
   }

if (response != NULL)
   {
   freeaddrinfo(response);
   }
#else 
 
bzero(&sin,sizeof(sin));

/*
 * HvB : Bas van der Vlies
*/
if (BINDINTERFACE[0] != '\0' )
   {
   sin.sin_addr.s_addr = find_inet_addr(BINDINTERFACE);
   }
else
  sin.sin_addr.s_addr = INADDR_ANY;

sin.sin_port = (unsigned short)(port); /*  Service returns network byte order */
sin.sin_family = AF_INET; 

if ((sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
   {
   CfLog(cferror,"Couldn't open socket","socket");
   exit (1);
   }

if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof (int)) == -1)
   {
   CfLog(cferror,"Couldn't set socket options","sockopt");
   exit (1);
   }

 if (setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *) &cflinger, sizeof (struct linger)) == -1)
   {
   CfLog(cferror,"Couldn't set socket options","sockopt");
   exit (1);
   }

if (bind(sd,(struct sockaddr *)&sin,sizeof(sin)) == -1) 
   {
   CfLog(cferror,"Couldn't bind to socket","bind");
   exit(1);
   }

#endif
 
return sd;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine daemon: server module\n%s-%s\n%s\n",PACKAGE,VERSION,COPYRIGHT);
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

/*********************************************************************/

void PurgeOldConnections(list,now)

   /* Some connections might not terminate properly. These should be cleaned
      every couple of hours. That should be enough to prevent spamming. */

struct Item **list;
time_t now;

{ struct Item *ip;
 int then=0;

if (list == NULL)
   {
   return;
   }

Debug("Purging Old Connections...\n");

for (ip = *list; ip != NULL; ip=ip->next)
   {
   sscanf(ip->classes,"%d",&then);

   if (now > then + 7200)
      {
      DeleteItem(list,ip);
      snprintf(OUTPUT,bufsize*2,"Purging IP address %s from connection list\n",ip->name);
      CfLog(cfverbose,OUTPUT,"");
      }
   }

Debug("Done purging\n");
}


/*********************************************************************/

void SpawnConnection(sd_reply,ipaddr)

int sd_reply;
char *ipaddr;

{ struct cfd_connection *conn;

#ifdef HAVE_PTHREAD_H
 pthread_t tid;
#endif

conn = NewConn(sd_reply);

strncpy(conn->ipaddr,ipaddr,cfmaxiplen-1);

Debug("New connection...(from %s/%d)\n",conn->ipaddr,sd_reply);
 
#if defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD

Debug("Spawning new thread...\n");

pthread_attr_init(&PTHREADDEFAULTS);
pthread_attr_setdetachstate(&PTHREADDEFAULTS,PTHREAD_CREATE_DETACHED);

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
pthread_attr_setstacksize(&PTHREADDEFAULTS,(size_t)1024*1024);
#endif
 
if (pthread_create(&tid,&PTHREADDEFAULTS,(void *)HandleConnection,(void *)conn) != 0)
   {
   CfLog(cferror,"pthread_create failed","create");
   HandleConnection(conn);
   }

pthread_attr_destroy(&PTHREADDEFAULTS);

#else

/* Can't fork here without getting a zombie unless we do some complex waiting? */

Debug("Single threaded...\n");

HandleConnection(conn);
 
#endif
}

/**************************************************************/

void CheckFileChanges(argc,argv,sd)

int argc;
char **argv;
int sd;

{ struct stat newstat;
  char filename[bufsize],*sp;
  int i;
  
bzero(&newstat,sizeof(struct stat));
bzero(filename,bufsize);

if ((sp=getenv(CFINPUTSVAR)) != NULL)
   {
   if (!IsAbsoluteFileName(VINPUTFILE))	/* Don't prepend to absolute names */
      { 
      strncpy(filename,sp,bufsize-1);
      AddSlash(filename);
      }
   }
else
   {
   if (!IsAbsoluteFileName(VINPUTFILE))	/* Don't prepend to absolute names */
      {
      strcpy(filename,WORKDIR);
      AddSlash(filename);
      strcat(filename,"inputs/");
      }
   }

strcat(filename,VINPUTFILE);

if (stat(filename,&newstat) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Input file %s missing or busy..\n",filename);
   CfLog(cferror,OUTPUT,filename);
   sleep(5);
   return;
   }

Debug("Checking file updates on %s (%x/%x)\n",filename, newstat.st_ctime, CFDSTARTTIME);

if (CFDSTARTTIME < newstat.st_mtime)
   {
   snprintf(OUTPUT,bufsize*2,"Rereading config files %s..\n",filename);
   CfLog(cfinform,OUTPUT,"");

   /* Free & reload -- lock this to avoid access errors during reload */

   DeleteItemList(VHEAP);
   DeleteItemList(VNEGHEAP);
   DeleteItemList(TRUSTKEYLIST);
   DeleteAuthList(VADMIT);
   DeleteAuthList(VDENY);
   strcpy(VDOMAIN,"undefined.domain");

   VADMIT = VADMITTOP = NULL;
   VDENY  = VDENYTOP  = NULL;
   VHEAP  = VNEGHEAP  = NULL;
   TRUSTKEYLIST = NULL;

   AddClassToHeap("any");
   GetNameInfo();
   ParseInputFile("cfservd.conf");
   CheckVariables();
   SummarizeParsing();
   }
}

/*********************************************************************/
/* Level 4                                                           */
/*********************************************************************/

void *HandleConnection(conn)

struct cfd_connection *conn;

{
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
#ifdef HAVE_PTHREAD_SIGMASK
 sigset_t sigmask;

sigemptyset(&sigmask);
pthread_sigmask(SIG_BLOCK,&sigmask,NULL); 
#endif

if (conn == NULL)
   {
   Debug("Null connection\n");
   return NULL;
   }
 
if (pthread_mutex_lock(&MUTEX_COUNT) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
   DeleteConn(conn);
   return NULL;
   }

ACTIVE_THREADS++;

if (pthread_mutex_unlock(&MUTEX_COUNT) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   }  

if (ACTIVE_THREADS >= CFD_MAXPROCESSES)
   {
   if (pthread_mutex_lock(&MUTEX_COUNT) != 0)
      {
      CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
      DeleteConn(conn);
      return NULL;
      }
   
   ACTIVE_THREADS--;
   
   if (TRIES++ > MAXTRIES)  /* When to say we're hung / apoptosis threshold */
      {
      CfLog(cferror,"Server seems to be paralyzed. DOS attack? Committing apoptosis...","");
      ExitCleanly(0);
      }

   if (pthread_mutex_unlock(&MUTEX_COUNT) != 0)
      {
      CfLog(cferror,"pthread_mutex_unlock failed","unlock");
      }

   snprintf(OUTPUT,bufsize,"Too many threads (>=%d) -- increase MaxConnections?",CFD_MAXPROCESSES);
   CfLog(cferror,OUTPUT,"");
   snprintf(OUTPUT,bufsize,"BAD: Server is currently too busy -- increase MaxConnections or Splaytime?");
   SendTransaction(conn->sd_reply,OUTPUT,0,CF_DONE);
   DeleteConn(conn);
   return NULL;
   }

TRIES = 0;   /* As long as there is activity, we're not stuck */
 
#endif
 
while (BusyWithConnection(conn))
   {
   }

#if defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD

 Debug("Terminating thread...\n");
 
if (pthread_mutex_lock(&MUTEX_COUNT) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
   DeleteConn(conn);
   return NULL;
   }

ACTIVE_THREADS--;

if (pthread_mutex_unlock(&MUTEX_COUNT) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   }
 
#endif

DeleteConn(conn);
return NULL; 
}

/*********************************************************************/

int BusyWithConnection(conn)

struct cfd_connection *conn;

  /* This is the protocol section. Here we must   */
  /* check that the incoming data are sensible    */
  /* and extract the information from the message */

{ time_t tloc, trem = 0;
  char recvbuffer[bufsize+128], sendbuffer[bufsize],check[bufsize];  
  char filename[bufsize],buffer[bufsize],args[bufsize],out[bufsize];
  long time_no_see = 0;
  int len=0, drift, plainlen, received;
  struct cfd_get_arg get_args;

bzero(recvbuffer,bufsize);
bzero(&get_args,sizeof(get_args));

if ((received = ReceiveTransaction(conn->sd_reply,recvbuffer,NULL)) == -1)
   {
   return false;
   }

if (strlen(recvbuffer) == 0)
   {
   Debug("cfservd terminating NULL transmission!\n");
   return false;
   }
  
Debug("Received: [%s] on socket %d\n",recvbuffer,conn->sd_reply);

switch (GetCommand(recvbuffer))
   {
   case cfd_exec:    bzero(args,bufsize);
                     sscanf(recvbuffer,"EXEC %[^\n]",args);

		     if (!conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (!AllowedUser(conn->username))
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (!conn->rsa_auth)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (!AccessControl(CFRUNCOMMAND,conn,false))
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;			
			}

     		     if (!MatchClasses(conn))
			{
			Terminate(conn->sd_reply);
			return false;
			}

		     DoExec(conn,sendbuffer,args);
		     Terminate(conn->sd_reply);
		     return false;

   case cfd_cauth:   conn->id_verified = VerifyConnection(conn,(char *)(recvbuffer+strlen("CAUTH ")));

                     if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			}
                     return conn->id_verified; /* are we finished yet ? */


   case cfd_sauth:   /* This is where key agreement takes place */

                     if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}
		     
		     if (!AuthenticationDialogue(conn,recvbuffer))
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}
	     
		     return true;

   case cfd_get:     bzero(filename,bufsize);
                     sscanf(recvbuffer,"GET %d %[^\n]",&(get_args.buf_size),filename);

		     if (get_args.buf_size < 0 || get_args.buf_size > bufsize)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

 	             if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (!AccessControl(filename,conn,false))
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return true;			
			}

		     bzero(sendbuffer,bufsize);

		     if (get_args.buf_size >= bufsize)
			{
			get_args.buf_size = 2048;
			}
		     
		     get_args.connect = conn;
		     get_args.encrypt = false;
		     get_args.replybuff = sendbuffer;
		     get_args.replyfile = filename;
		     
		     CfGetFile(&get_args);
		     
		     return true;

   case cfd_sget:    bzero(buffer,bufsize);
                     sscanf(recvbuffer,"SGET %d %d",&len,&(get_args.buf_size));
		     if (received != len+CF_PROTO_OFFSET)
			{
			Debug("Protocol error\n");
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}
		     
                     plainlen = DecryptString(recvbuffer+CF_PROTO_OFFSET,buffer,conn->session_key,len);

                     cfscanf(buffer,strlen("GET"),strlen("dummykey"),check,sendbuffer,filename);

		     if (strcmp(check,"GET") != 0)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return true;
			}

		     if (get_args.buf_size < 0 || get_args.buf_size > 8192)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (get_args.buf_size >= bufsize)
			{
			get_args.buf_size = 2048;
			}
		     
		     Debug("Confirm decryption, and thus validity of caller\n");
		     Debug("SGET %s with blocksize %d\n",filename,get_args.buf_size);

 	             if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (!AccessControl(filename,conn,true))
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;			
			}

		     bzero(sendbuffer,bufsize);
		     
		     get_args.connect = conn;
		     get_args.encrypt = true;
		     get_args.replybuff = sendbuffer;
		     get_args.replyfile = filename;
		     
		     CfGetFile(&get_args);
		     return true;
		     
   case cfd_opendir: bzero(filename,bufsize);
                     sscanf(recvbuffer,"OPENDIR %[^\n]",filename);
		     
		     if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (!AccessControl(filename,conn,true)) /* opendir don't care about privacy */
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;			
			}		     

		     CfOpenDirectory(conn,sendbuffer,filename);
                     return true;


   case cfd_ssynch:  bzero(buffer,bufsize);
                     sscanf(recvbuffer,"SSYNCH %d",&len);

		     if (received != len+CF_PROTO_OFFSET)
			{
			Debug("Protocol error: %d\n",len);
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}

		     if (conn->session_key == NULL)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}
		     
                     bcopy(recvbuffer+CF_PROTO_OFFSET,out,len);
		     
                     plainlen = DecryptString(out,recvbuffer,conn->session_key,len);
		     
		     if (strncmp(recvbuffer,"SYNCH",5) !=0)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return true;
			}
		     /* roll through, no break */
		     
   case cfd_synch:
		     if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}
		     
                     bzero(filename,bufsize);
                     sscanf(recvbuffer,"SYNCH %ld STAT %[^\n]",&time_no_see,filename);

		     trem = (time_t) time_no_see;

                     if (time_no_see == 0 || filename[0] == '\0')
			{
			break;
			}
		     
		     if ((tloc = time((time_t *)NULL)) == -1)
                        {
                        sprintf(conn->output,"Couldn't read system clock\n");
			CfLog(cfinform,conn->output,"time");
			SendTransaction(conn->sd_reply,"BAD: clocks out of synch",0,CF_DONE);
			return true;
                        }

		     drift = (int)(tloc-trem);

		     if (!AccessControl(filename,conn,true))
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return true;			
			}

		     if (DENYBADCLOCKS && (drift*drift > CLOCK_DRIFT*CLOCK_DRIFT))
			{
			snprintf(conn->output,bufsize*2,"BAD: Clocks are too far unsynchronized %ld/%ld\n",(long)tloc,(long)trem);
			CfLog(cfinform,conn->output,"");
			SendTransaction(conn->sd_reply,conn->output,0,CF_DONE);
			return true;
			}
		     else
			{
			Debug("Clocks were off by %ld\n",(long)tloc-(long)trem);
			StatFile(conn,sendbuffer,filename);
			}

		     return true;

   case cfd_smd5:    bzero(buffer,bufsize);
                     sscanf(recvbuffer,"SMD5 %d",&len);
		     
		     if (received != len+CF_PROTO_OFFSET)
			{
			Debug("Decryption error: %d\n",len);
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return true;
			}
		     
                     bcopy(recvbuffer+CF_PROTO_OFFSET,out,len);
                     plainlen = DecryptString(out,recvbuffer,conn->session_key,len);

		     if (strncmp(recvbuffer,"MD5",3) !=0)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return false;
			}
		     /* roll through, no break */
       
   case cfd_md5:
		     if (! conn->id_verified)
			{
			RefuseAccess(conn,sendbuffer,0,recvbuffer);
			return true;
			}
		     
                     bzero(filename,bufsize);
		     bzero(args,bufsize);
		     
                     CompareLocalChecksum(conn,sendbuffer,recvbuffer);
		     return true;

   }

sprintf (sendbuffer,"BAD: Request denied\n");
SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
CfLog(cfinform,"Closing connection\n",""); 
return false;
}

/**************************************************************/

void *ExitCleanly(signum)

int signum;

{ 
#ifdef HAVE_PTHREAD_H
/* Do we care about this? 
for (i = 0; i < MAXTHREAD ; i++)
   {
   if (THREADS[i] != NULL)
      {
      pthread_join(THREADS[i],(void **)NULL);
      }
   }
   */
#endif 
 
HandleSignal(signum);
return NULL;
}

/**************************************************************/
/* Level 4                                                    */
/**************************************************************/

int MatchClasses(conn)

struct cfd_connection *conn;

{ char recvbuffer[bufsize];
  struct Item *classlist = NULL, *ip;
  int count = 0;

Debug("Match classes\n");

while (true && (count < 10))  /* arbitrary check to avoid infinite loop, DoS attack*/
   {
   count++;

   if (ReceiveTransaction(conn->sd_reply,recvbuffer,NULL) == -1)
      {
      if (errno == EINTR) 
         {
         continue;
         }
      }

   Debug("Got class buffer %s\n",recvbuffer);

   if (strncmp(recvbuffer,CFD_TERMINATOR,strlen(CFD_TERMINATOR)) == 0)
      {
      if (count == 1)
	 {
	 Debug("No classes were sent, assuming no restrictions...\n");
	 return true;
	 }
      
      break;
      }
   
   classlist = SplitStringAsItemList(recvbuffer,' ');

   for (ip = classlist; ip != NULL; ip=ip->next)
      {
      if (IsDefinedClass(ip->name))
	 {
	 Debug("Class %s matched, accepting...\n",ip->name);
	 DeleteItemList(classlist);
	 return true;
	 }
      
      if (strncmp(ip->name,CFD_TERMINATOR,strlen(CFD_TERMINATOR)) == 0)
	 {
	 Debug("No classes matched, rejecting....\n");
	 ReplyNothing(conn);
	 DeleteItemList(classlist);
	 return false;
	 }
      }
   }

ReplyNothing(conn);
Debug("No classes matched, rejecting....\n");
DeleteItemList(classlist);
return false;
}


/**************************************************************/

void DoExec(conn,sendbuffer,args)

struct cfd_connection *conn;
char *sendbuffer;
char *args;

{ char buffer[bufsize], line[bufsize], *sp;
  int print = false,i;
  FILE *pp;

bzero(buffer,bufsize);

if ((CFSTARTTIME = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","time");
   }

if (GetMacroValue(CONTEXTID,"cfrunCommand") == NULL)
   {
   Verbose("cfservd exec request: no cfrunCommand defined\n");
   sprintf(sendbuffer,"Exec request: no cfrunCommand defined\n");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return;
   }

for (sp = args; *sp != '\0'; sp++) /* Blank out -K -f */
   {
   if ((strncmp(sp,"-K",2) == 0) || (strncmp(sp,"-f",2) == 0))
      {
      *sp = ' ';
      *(sp+1) = ' ';
      }
   else if (strncmp(sp,"--no-lock",9) == 0)
      {
      for (i = 0; i < 9; i++)
	 {
	 *(sp+i) = ' ';
	 }
      }
   else if (strncmp(sp,"--file",7) == 0)
      {
      for (i = 0; i < 7; i++)
	 {
	 *(sp+i) = ' ';
	 }
      }
   }

/* 
if (!GetLock("cfservd","exec",VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
   {
   snprintf(sendbuffer,bufsize,"cfservd Couldn't get a lock -- too soon: IfElapsed %d, ExpireAfter %d\n",VIFELAPSED,VEXPIREAFTER);
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return;
   }
*/
   
ExpandVarstring("$(cfrunCommand) --no-splay --inform",buffer,"");
 
if (strlen(buffer)+strlen(args)+6 > bufsize)
   {
   snprintf(sendbuffer,bufsize,"Command line too long with args: %s\n",buffer);
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   ReleaseCurrentLock();
   return;
   }
else
   {
   if ((args != NULL) & (strlen(args) > 0))
      {
      strcat(buffer," ");
      strcat(buffer,args);

      snprintf(sendbuffer,bufsize,"cfservd Executing %s\n",buffer);
      SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
      }
   }

snprintf(conn->output,bufsize,"Executing command %s\n",buffer);
CfLog(cfinform,conn->output,""); 
 
if ((pp = cfpopen(buffer,"r")) == NULL)
   {
   snprintf(conn->output,bufsize,"Couldn't open pipe to command %s\n",buffer);
   CfLog(cferror,conn->output,"pipe");
   snprintf(sendbuffer,bufsize,"Unable to run %s\n",buffer);
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   ReleaseCurrentLock();
   return;
   }

while (!feof(pp))
   {
   if (ferror(pp))
      {
      fflush(pp);
      break;
      }

   ReadLine(line,bufsize,pp);
   
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
      snprintf(sendbuffer,bufsize,"%s\n",line);
      SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
      }
  }
      
cfpclose(pp);

/* ReleaseCurrentLock(); */
}

/**************************************************************/

int GetCommand (str)

char *str;

{ int i;
  char op[bufsize];

sscanf(str,"%4095s",op);

for (i = 0; PROTOCOL[i] != NULL; i++)
   {
   if (strcmp(op,PROTOCOL[i])==0)
      {
      return i;
      }
   }

return -1;
}

/*********************************************************************/

int VerifyConnection(conn,buf)

 /* Try reverse DNS lookup
    and RFC931 username lookup to check the authenticity. */

struct cfd_connection *conn;
char buf[bufsize];

{ char ipstring[maxvarsize], fqname[maxvarsize], username[maxvarsize];
  char dns_assert[maxvarsize],ip_assert[maxvarsize];
  int matched = false;
  struct passwd *pw;
#ifdef HAVE_GETADDRINFO
  struct addrinfo query, *response=NULL, *ap;
  int err;
#else
  struct sockaddr_in raddr;
  int i,j,len = sizeof(struct sockaddr_in);
  struct hostent *hp = NULL;
  struct Item *ip_aliases = NULL, *ip_addresses = NULL;
#endif

Debug("Connecting host identifies itself as %s\n",buf);

bzero(ipstring,maxvarsize);
bzero(fqname,maxvarsize);
bzero(username,maxvarsize); 

sscanf(buf,"%255s %255s %255s",ipstring,fqname,username);

Debug("(ipstring=[%s],fqname=[%s],username=[%s],socket=[%s])\n",ipstring,fqname,username,conn->ipaddr); 

strncpy(dns_assert,ToLowerStr(fqname),maxvarsize-1);
strncpy(ip_assert,ipstring,maxvarsize-1);

/* It only makes sense to check DNS by reverse lookup if the key had to be accepted
   on trust. Once we have a positive key ID, the IP address is irrelevant fr authentication...
   We can save a lot of time by not looking this up ... */
 
if ((conn->trust == false) || IsFuzzyItemIn(SKIPVERIFY,MapAddress(conn->ipaddr)))
   {
   snprintf(conn->output,bufsize*2,"Allowing %s to connect without (re)checking ID\n",ip_assert);
   CfLog(cfverbose,conn->output,"");
   Verbose("Non-verified Host ID is %s (Using skipverify)\n",dns_assert);
   strncpy(conn->hostname,dns_assert,maxvarsize); 
   Verbose("Non-verified User ID seems to be %s (Using skipverify)\n",username); 
   strncpy(conn->username,username,maxvarsize);

   if ((pw=getpwnam(username)) == NULL) /* Keep this inside mutex */
      {      
      printf("username was");
      conn->uid = -2;
      }
   else
      {
      conn->uid = pw->pw_uid;
      }
   
   return true;
   }
 
if (strcmp(ip_assert,MapAddress(conn->ipaddr)) != 0)
   {
   Verbose("IP address mismatch between client's assertion (%s) and socket (%s) - untrustworthy connection\n",ip_assert,conn->ipaddr);
   return false;
   }

Verbose("Socket caller address appears honest (%s matches %s)\n",ip_assert,MapAddress(conn->ipaddr));
 
snprintf(conn->output,bufsize,"Socket originates from %s=%s\n",ip_assert,dns_assert);
CfLog(cfverbose,conn->output,""); 

Debug("Attempting to verify honesty by looking up hostname (%s)\n",dns_assert);

/* Do a reverse DNS lookup, like tcp wrappers to see if hostname matches IP */
 
#ifdef HAVE_GETADDRINFO

 Debug("Using v6 compatible lookup...\n"); 

bzero(&query,sizeof(struct addrinfo));
query.ai_family = AF_UNSPEC;
query.ai_socktype = SOCK_STREAM;
query.ai_flags = AI_PASSIVE;
 
if ((err=getaddrinfo(dns_assert,NULL,&query,&response)) != 0)
   {
   snprintf(conn->output,bufsize,"Unable to lookup %s (%s)",dns_assert,gai_strerror(err));
   CfLog(cferror,conn->output,"");
   }
 
for (ap = response; ap != NULL; ap = ap->ai_next)
   {
   Debug("CMP: %s %s\n",MapAddress(conn->ipaddr),sockaddr_ntop(ap->ai_addr));
   
   if (strcmp(MapAddress(conn->ipaddr),sockaddr_ntop(ap->ai_addr)) == 0)
      {
      Debug("Found match\n");
      matched = true;
      }
   }

if (response != NULL)
   {
   freeaddrinfo(response);
   }

#else 

Debug("IPV4 hostnname lookup on %s\n",dns_assert);

# ifdef HAVE_PTHREAD_H  
 if (pthread_mutex_lock(&MUTEX_HOSTNAME) != 0)
    {
    CfLog(cferror,"pthread_mutex_lock failed","unlock");
    exit(1);
    }
# endif
 
if ((hp = gethostbyname(dns_assert)) == NULL)
   {
   Verbose("cfservd Couldn't look up name %s\n",fqname);
   Verbose("     Make sure that fully qualified names can be looked up at your site!\n");
   Verbose("     i.e. www.gnu.org, not just www. If you use NIS or /etc/hosts\n");
   Verbose("     make sure that the full form is registered too as an alias!\n");

   snprintf(OUTPUT,bufsize,"DNS lookup of %s failed",dns_assert);
   CfLog(cflogonly,OUTPUT,"gethostbyname");
   matched = false;
   }
else
   {
   matched = true;

   Debug("Looking for the peername of our socket...\n");
   
   if (getpeername(conn->sd_reply,(struct sockaddr *)&raddr,&len) == -1)
      {
      CfLog(cferror,"Couldn't get socket address\n","getpeername");
      matched = false;
      }
   
   Verbose("Looking through hostnames on socket with IPv4 %s\n",sockaddr_ntop((struct sockaddr *)&raddr));
   
   for (i = 0; hp->h_addr_list[i]; i++)
      {
      Verbose("Reverse lookup address found: %d\n",i);
      if (memcmp(hp->h_addr_list[i],(char *)&(raddr.sin_addr),sizeof(raddr.sin_addr)) == 0)
	 {
	 Verbose("Canonical name matched host's assertion - id confirmed as %s\n",dns_assert);
	 break;
	 }
      }
   
   if (hp->h_addr_list[0] != NULL)
      {
      Verbose("Checking address number %d for non-canonical names (aliases)\n",i);
      for (j = 0; hp->h_aliases[j] != NULL; j++)
	 {
	 Verbose("Comparing [%s][%s]\n",hp->h_aliases[j],ip_assert);
	 if (strcmp(hp->h_aliases[j],ip_assert) == 0)
	    {
	    Verbose("Non-canonical name (alias) matched host's assertion - id confirmed as %s\n",dns_assert);
	    break;
	    }
	 }
      
      if ((hp->h_addr_list[i] != NULL) && (hp->h_aliases[j] != NULL))
	 {
	 snprintf(conn->output,bufsize,"Reverse hostname lookup failed, host claiming to be %s was %s\n",buf,sockaddr_ntop((struct sockaddr *)&raddr));
	 CfLog(cflogonly,conn->output,"");
	 matched = false;
	 }
      else
	 {
	 Verbose("Reverse lookup succeeded\n");
	 }
      }
   else
      {
      snprintf(conn->output,bufsize,"No name was registered in DNS for %s - reverse lookup failed\n",dns_assert);
      CfLog(cflogonly,conn->output,"");
      matched = false;
      }   
   }


if ((pw=getpwnam(username)) == NULL) /* Keep this inside mutex */
   {

   printf("username was");
   conn->uid = -2;
   }
else
   {
   conn->uid = pw->pw_uid;
   }

 
# ifdef HAVE_PTHREAD_H  
 if (pthread_mutex_unlock(&MUTEX_HOSTNAME) != 0)
    {
    CfLog(cferror,"pthread_mutex_unlock failed","unlock");
    exit(1);
    }
# endif

#endif

if (!matched)
   {
   snprintf(conn->output,bufsize,"Failed on DNS reverse lookup of %s\n",dns_assert);
   CfLog(cflogonly,conn->output,"gethostbyname");
   snprintf(conn->output,bufsize,"Client sent: %s",buf);
   CfLog(cflogonly,conn->output,"");
   return false;
   }
 
Verbose("Host ID is %s\n",dns_assert);
strncpy(conn->hostname,dns_assert,maxvarsize-1);
 
Verbose("User ID seems to be %s\n",username); 
strncpy(conn->username,username,maxvarsize-1);
 
return true;   
}


/**************************************************************/

int AllowedUser(user)

char *user;

{
if (IsItemIn(ALLOWUSERLIST,user))
   {
   Debug("User %s granted connection privileges\n",user);
   return true;
   }

Debug("User %s is not allowed on this server\n",user); 
return false;
}

/**************************************************************/

int AccessControl(filename,conn,encrypt)

char *filename;
struct cfd_connection *conn;
int encrypt;

{ struct Auth *ap;
  int access = false;
  char realname[bufsize];
  struct stat statbuf;

Debug("AccessControl(%s)\n",filename);
bzero(realname,bufsize);

if (lstat(filename,&statbuf) == -1)
   {
   snprintf(conn->output,bufsize*2,"Couldn't stat filename %s from host %s\n",filename,conn->hostname);
   CfLog(cfverbose,conn->output,"lstat");
   CfLog(cflogonly,conn->output,"lstat");   
   return false;
   }

if (S_ISLNK(statbuf.st_mode)) 
   {
   strncpy(realname,filename,bufsize);
   }
 else
    {
#ifdef HAVE_REALPATH
    if (realpath(filename,realname) == NULL)
       {
       snprintf(conn->output,bufsize*2,"Couldn't resolve filename %s from host %s\n",filename,conn->hostname);
       CfLog(cfverbose,conn->output,"lstat");
       CfLog(cflogonly,conn->output,"lstat");   
       return false;
       }
#else
    CompressPath(realname,filename); /* in links.c */
#endif
    }
 
Debug("AccessControl(%s,%s) encrypt request=%d\n",realname,conn->hostname,encrypt);
 
if (VADMIT == NULL)
   {
   Verbose("cfservd access list is empty, no files are visible\n");
   return false;
   }
 
conn->maproot = false;
 
for (ap = VADMIT; ap != NULL; ap=ap->next)
   {
   int res = false;

   if ((strlen(realname) > strlen(ap->path))  && strncmp(ap->path,realname,strlen(ap->path)) == 0 && realname[strlen(ap->path)] == '/')
      {
      res = true;    /* Substring means must be a / to link, else just a substring og filename */
      }

   if (strcmp(ap->path,realname) == 0)
      {
      res = true;    /* Exact match means single file to admit */
      }
   
   if (res)
      {
      Debug("Found a matching rule in access list (%s,%s)\n",realname,ap->path);
      if (stat(ap->path,&statbuf) == -1)
	 {
	 snprintf(OUTPUT,bufsize,"Warning cannot stat file object %s in admit/grant, or access list refers to dangling link\n",ap->path);
	 CfLog(cflogonly,OUTPUT,"");
	 continue;
	 }
      
      if (!encrypt && (ap->encrypt == true))
	 {
	 snprintf(conn->output,bufsize,"File %s requires encrypt connection...will not serve\n",ap->path);
	 CfLog(cferror,conn->output,"");
	 access = false;
	 }
      else
	 {
	 Debug("Checking whether to map root privileges..\n");
	 
	 if (IsWildItemIn(ap->maproot,conn->hostname) ||
	     IsWildItemIn(ap->maproot,MapAddress(conn->ipaddr)) ||
	     IsFuzzyItemIn(ap->maproot,MapAddress(conn->ipaddr)))
	    {
	    conn->maproot = true;
	    Debug("Mapping root privileges\n");
	    }
	 else
	    {
	    Debug("No root privileges granted\n");
	    }
	 
	 if (IsWildItemIn(ap->accesslist,conn->hostname) ||
	     IsWildItemIn(ap->accesslist,MapAddress(conn->ipaddr)) ||
	     IsFuzzyItemIn(ap->accesslist,MapAddress(conn->ipaddr)))
	    {
	    access = true;
	    Debug("Access privileges - match found\n");
	    }
	 }
      break;
      }
   }
 
for (ap = VDENY; ap != NULL; ap=ap->next)
   {
   if (strncmp(ap->path,realname,strlen(ap->path)) == 0)
      {
      if (IsWildItemIn(ap->accesslist,conn->hostname) ||
	  IsWildItemIn(ap->accesslist,MapAddress(conn->ipaddr)) ||
	  IsFuzzyItemIn(ap->accesslist,MapAddress(conn->ipaddr)))
         {
         access = false;
	 snprintf(conn->output,bufsize*2,"Host %s explicitly denied access to %s\n",conn->hostname,realname);
	 CfLog(cfverbose,conn->output,"");   
         break;
         }
      }
   }
 
if (access)
   {
   snprintf(conn->output,bufsize*2,"Host %s granted access to %s\n",conn->hostname,realname);
   CfLog(cfverbose,conn->output,"");
   }
else
   {
   snprintf(conn->output,bufsize*2,"Host %s denied access to %s\n",conn->hostname,realname);
   CfLog(cfverbose,conn->output,"");
   CfLog(cflogonly,conn->output,"");
   }


if (!conn->rsa_auth)
   {
   CfLog(cfverbose,"Cannot map root access without RSA authentication","");
   conn->maproot = false; /* only public files accessible */
   /* return false; */
   }
 
return access;
}

/**************************************************************/

int AuthenticationDialogue(conn,recvbuffer)

struct cfd_connection *conn;
char *recvbuffer;

{ char in[bufsize],*out, *decrypted_nonce;
  BIGNUM *counter_challenge = NULL;
  unsigned char digest[EVP_MAX_MD_SIZE+1];
  unsigned int crypt_len, nonce_len = 0,len = 0, encrypted_len, keylen;
  char sauth[10], iscrypt ='n';
  unsigned long err;
  RSA *newkey;

if (PRIVKEY == NULL || PUBKEY == NULL)
   {
   CfLog(cferror,"No public/private key pair exists, create one with cfkey\n","");
   return false;
   }
 
/* proposition C1 */
/* Opening string is a challenge from the client (some agent) */
 
sscanf(recvbuffer,"%s %c %d %d",sauth,&iscrypt,&crypt_len,&nonce_len);
 
if ((strcmp(sauth,"SAUTH") != 0) || (nonce_len == 0) || (crypt_len == 0))
   {
   CfLog(cfinform,"Protocol error in RSA authentation from IP %s\n",conn->hostname);
   return false;
   }

Debug("Challenge encryption = %c, nonce = %d, buf = %d\n",iscrypt,nonce_len,crypt_len);

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_lock failed","lock");
    }
#endif
 
if ((decrypted_nonce = malloc(crypt_len)) == NULL)
   {
   FatalError("memory failure");
   }
 
if (iscrypt == 'y')
   { 
   if (RSA_private_decrypt(crypt_len,recvbuffer+CF_RSA_PROTO_OFFSET,decrypted_nonce,PRIVKEY,RSA_PKCS1_PADDING) <= 0)
      {
      err = ERR_get_error();
      snprintf(conn->output,bufsize,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,conn->output,"");
      free(decrypted_nonce);

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
      if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
	 {
	 CfLog(cferror,"pthread_mutex_unlock failed","lock");
	 }
#endif 
      return false;
      }
   }
else
   {
   bcopy(recvbuffer+CF_RSA_PROTO_OFFSET,decrypted_nonce,nonce_len);  
   }

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","lock");
   }
#endif
 
/* Client's ID is now established by key or trusted, reply with md5 */

ChecksumString(decrypted_nonce,nonce_len,digest,'m');
free(decrypted_nonce);

/* Get the public key from the client */

 
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","lock");
   }
#endif
 
newkey = RSA_new();

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","lock");
   }
#endif


 
/* proposition C2 */ 
if ((len = ReceiveTransaction(conn->sd_reply,recvbuffer,NULL)) == -1)
   {
   CfLog(cfinform,"Protocol error 1 in RSA authentation from IP %s\n",conn->hostname);
   RSA_free(newkey);
   return false;
   }

if (len == 0)
   {
   CfLog(cfinform,"Protocol error 2 in RSA authentation from IP %s\n",conn->hostname);
   RSA_free(newkey);
   return false;
   }
   
if ((newkey->n = BN_mpi2bn(recvbuffer,len,NULL)) == NULL)
   {
   err = ERR_get_error();
   snprintf(conn->output,bufsize,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
   CfLog(cferror,conn->output,"");
   RSA_free(newkey);
   return false;
   }

/* proposition C3 */ 

if ((len=ReceiveTransaction(conn->sd_reply,recvbuffer,NULL)) == -1)
   {
   CfLog(cfinform,"Protocol error 3 in RSA authentation from IP %s\n",conn->hostname);
   RSA_free(newkey);
   return false;
   }

 if (len == 0)
   {
   CfLog(cfinform,"Protocol error 4 in RSA authentation from IP %s\n",conn->hostname);
   RSA_free(newkey);
   return false;
   }
 
if ((newkey->e = BN_mpi2bn(recvbuffer,len,NULL)) == NULL)
   {
   err = ERR_get_error();
   snprintf(conn->output,bufsize,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
   CfLog(cferror,conn->output,"");
   RSA_free(newkey);
   return false;
   }

if (DEBUG||D2)
   {
   RSA_print_fp(stdout,newkey,0);
   }
 
if (!CheckStoreKey(conn,newkey))    /* conceals proposition S1 */
   {
   if (!conn->trust)
      {
      RSA_free(newkey);       
      return false;
      }
   }

/* Reply with md5 of original challenge */

/* proposition S2 */ 
SendTransaction(conn->sd_reply,digest,16,CF_DONE);
 
/* Send counter challenge to be sure this is a live session */

counter_challenge = BN_new();
BN_rand(counter_challenge,256,0,0);
nonce_len = BN_bn2mpi(counter_challenge,in);
ChecksumString(in,nonce_len,digest,'m');
 
/* encrypted_len = BN_num_bytes(newkey->n);  encryption buffer is always the same size as n */

encrypted_len = RSA_size(newkey); /* encryption buffer is always the same size as n */ 

 
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","lock");
   }
#endif
 
if ((out = malloc(encrypted_len+1)) == NULL)
   {
   FatalError("memory failure");
   }

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","lock");
   }
#endif

 
if (RSA_public_encrypt(nonce_len,in,out,newkey,RSA_PKCS1_PADDING) <= 0)
   {
   err = ERR_get_error();
   snprintf(conn->output,bufsize,"Public encryption failed = %s\n",ERR_reason_error_string(err));
   CfLog(cferror,conn->output,"");
   RSA_free(newkey);
   free(out);
   return false;
   }

/* proposition S3 */ 
SendTransaction(conn->sd_reply,out,encrypted_len,CF_DONE);

/* if the client doesn't have our public key, send it */
 
if (iscrypt != 'y')
   {
   /* proposition S4  - conditional */
   bzero(in,bufsize); 
   len = BN_bn2mpi(PUBKEY->n,in);
   SendTransaction(conn->sd_reply,in,len,CF_DONE);

   /* proposition S5  - conditional */
   bzero(in,bufsize);  
   len = BN_bn2mpi(PUBKEY->e,in); 
   SendTransaction(conn->sd_reply,in,len,CF_DONE); 
   }

/* Receive reply to counter_challenge */

/* proposition C4 */ 
bzero(in,bufsize);
ReceiveTransaction(conn->sd_reply,in,NULL);
 
if (!ChecksumsMatch(digest,in,'m'))  /* replay / piggy in the middle attack ? */
   {
   BN_free(counter_challenge);
   free(out);
   RSA_free(newkey);
   snprintf(conn->output,bufsize,"Challenge response from client %s was incorrect - ID false?",conn->ipaddr);
   CfLog(cfinform,conn->output,"");
   return false; 
   }
else
   {
   if (!conn->trust)
      {
      snprintf(conn->output,bufsize,"Strongly authentication of client %s/%s",conn->hostname,conn->ipaddr);
      }
   else
      {
      snprintf(conn->output,bufsize,"Weak authentication of trusted client %s/%s (key accepted on trust).\n",conn->hostname,conn->ipaddr);
      }
   CfLog(cfverbose,conn->output,"");
   }

/* Receive random session key, blowfish style ... */ 

/* proposition C5 */
bzero(in,bufsize);
keylen = ReceiveTransaction(conn->sd_reply,in,NULL);

 
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","lock");
   }
#endif
 
conn->session_key = malloc(keylen);

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","lock");
   }
#endif

 
bcopy(in,conn->session_key,keylen);
Debug("Got a session key...\n"); 
 
BN_free(counter_challenge);
free(out);
RSA_free(newkey); 
conn->rsa_auth = true;
return true; 
}

/**************************************************************/

int StatFile(conn,sendbuffer,filename)

struct cfd_connection *conn;
char *sendbuffer, *filename;

/* Because we do not know the size or structure of remote datatypes,*/
/* the simplest way to transfer the data is to convert them into */
/* plain text and interpret them on the other side. */

{ struct cfstat cfst;
  struct stat statbuf;
  char linkbuf[bufsize];

  Debug("StatFile(%s)\n",filename);

bzero(&cfst,sizeof(struct cfstat));
  
if (strlen(ReadLastNode(filename)) > maxlinksize)
   {
   snprintf(sendbuffer,bufsize*2,"BAD: Filename suspiciously long [%s]\n",filename);
   CfLog(cferror,sendbuffer,"");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return -1;
   }

if (lstat(filename,&statbuf) == -1)
   {
   snprintf(sendbuffer,bufsize,"BAD: unable to stat file %s",filename);
   CfLog(cfverbose,sendbuffer,"lstat");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return -1;
   }

cfst.cf_readlink = NULL;
cfst.cf_lmode = 0;
cfst.cf_nlink = cfnosize;

bzero(linkbuf,bufsize);

if (S_ISLNK(statbuf.st_mode))
   {
   cfst.cf_type = cf_link;                   /* pointless - overwritten */
   cfst.cf_lmode = statbuf.st_mode & 07777;
   cfst.cf_nlink = statbuf.st_nlink;
       
   if (readlink(filename,linkbuf,bufsize-1) == -1)
      {
      sprintf(sendbuffer,"BAD: unable to read link\n");
      CfLog(cferror,sendbuffer,"readlink");
      SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
      return -1;
      }

   Debug("readlink: %s\n",linkbuf);

   cfst.cf_readlink = linkbuf;
   }

if (stat(filename,&statbuf) == -1)
   {
   snprintf(sendbuffer,bufsize,"BAD: unable to stat file %s\n",filename);
   CfLog(cfverbose,conn->output,"stat");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return -1;
   }

if (S_ISDIR(statbuf.st_mode))
   {
   cfst.cf_type = cf_dir;
   }

if (S_ISREG(statbuf.st_mode))
   {
   cfst.cf_type = cf_reg;
   }

if (S_ISSOCK(statbuf.st_mode))
   {
   cfst.cf_type = cf_sock;
   }

if (S_ISCHR(statbuf.st_mode))
   {
   cfst.cf_type = cf_char;
   }

if (S_ISBLK(statbuf.st_mode))
   {
   cfst.cf_type = cf_block;
   }

if (S_ISFIFO(statbuf.st_mode))
   {
   cfst.cf_type = cf_fifo;
   }

cfst.cf_mode     = statbuf.st_mode  & 07777;
cfst.cf_uid      = statbuf.st_uid   & 0xFFFFFFFF;
cfst.cf_gid      = statbuf.st_gid   & 0xFFFFFFFF;
cfst.cf_size     = statbuf.st_size;
cfst.cf_atime    = statbuf.st_atime;
cfst.cf_mtime    = statbuf.st_mtime;
cfst.cf_ctime    = statbuf.st_ctime;
cfst.cf_ino      = statbuf.st_ino;
cfst.cf_dev      = statbuf.st_dev;
cfst.cf_readlink = linkbuf;

if (cfst.cf_nlink == cfnosize)
   {
   cfst.cf_nlink = statbuf.st_nlink;
   }

#ifndef IRIX
if (statbuf.st_size > statbuf.st_blocks * DEV_BSIZE)
#else
# ifdef HAVE_ST_BLOCKS
if (statbuf.st_size > statbuf.st_blocks * DEV_BSIZE)
# else
if (statbuf.st_size > ST_NBLOCKS(statbuf) * DEV_BSIZE)
# endif
#endif
   {
   cfst.cf_makeholes = 1;   /* must have a hole to get checksum right */
   }
else
   {
   cfst.cf_makeholes = 0;
   }


bzero(sendbuffer,bufsize);

 /* send as plain text */

Debug("OK: type=%d\n mode=%o\n lmode=%o\n uid=%d\n gid=%d\n size=%ld\n atime=%d\n mtime=%d\n",
	cfst.cf_type,cfst.cf_mode,cfst.cf_lmode,cfst.cf_uid,cfst.cf_gid,(long)cfst.cf_size,
	cfst.cf_atime,cfst.cf_mtime);


snprintf(sendbuffer,bufsize,"OK: %d %d %d %d %d %ld %d %d %d %d %d %d %d",
	cfst.cf_type,cfst.cf_mode,cfst.cf_lmode,cfst.cf_uid,cfst.cf_gid,(long)cfst.cf_size,
	cfst.cf_atime,cfst.cf_mtime,cfst.cf_ctime,cfst.cf_makeholes,cfst.cf_ino,
	 cfst.cf_nlink,cfst.cf_dev);

SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
bzero(sendbuffer,bufsize);

if (cfst.cf_readlink != NULL)
   {
   strcpy(sendbuffer,"OK:");
   strcat(sendbuffer,cfst.cf_readlink);
   }
else
   {
   sprintf(sendbuffer,"OK:");
   }

SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
return 0;
}

/***************************************************************/

void CfGetFile(args)

struct cfd_get_arg *args;

{ int sd,fd,n_read,total=0,cipherlen,sendlen=0,count = 0;
  char sendbuffer[bufsize+1],out[bufsize],*filename;
  struct stat statbuf;
  uid_t uid;
  unsigned char iv[] = {1,2,3,4,5,6,7,8}, *key;
  EVP_CIPHER_CTX ctx;

sd         = (args->connect)->sd_reply;
filename   = args->replyfile;
key        = (args->connect)->session_key;
uid        = (args->connect)->uid;

stat(filename,&statbuf);
Debug("CfGetFile(%s on sd=%d), size=%d\n",filename,sd,statbuf.st_size);

/* Now check to see if we have remote permission */

if (uid != 0 && !args->connect->maproot) /* should remote root be local root */
   {
   if (statbuf.st_uid == uid)
      {
      Debug("Caller %s is the owner of the file\n",(args->connect)->username);
      }
   else
      {
      /* We are not the owner of the file and we don't care about groups */
      if (statbuf.st_mode & S_IROTH)
	 {
	 Debug("Caller %s not owner of the file but permission granted\n",(args->connect)->username);
	 }
      else
	 {
	 Debug("Caller %s is not the owner of the file\n",(args->connect)->username);
	 RefuseAccess(args->connect,sendbuffer,args->buf_size,"");
	 return;
	 }
      }
   }

if (args->buf_size < 512)
   {
   snprintf(args->connect->output,bufsize,"blocksize for %s was only %d\n",filename,args->buf_size);
   CfLog(cferror,args->connect->output,"");
   }

if (args->encrypt)
   {
   EVP_CIPHER_CTX_init(&ctx);
   EVP_EncryptInit(&ctx,EVP_bf_cbc(),key,iv);

   }
 
if ((fd = SafeOpen(filename)) == -1)
   {
   snprintf(sendbuffer,bufsize,"Open error of file [%s]\n",filename);
   CfLog(cferror,sendbuffer,"open");
   snprintf(sendbuffer,bufsize,"%s",CFFAILEDSTR);
   SendSocketStream(sd,sendbuffer,args->buf_size,0);
   }
else
   {
   while(true)
      {
      bzero(sendbuffer,bufsize);

      Debug("Now reading from disk...\n");
      
      if ((n_read = read(fd,sendbuffer,args->buf_size)) == -1)
	 {
	 CfLog(cferror,"read failed in GetFile","read");
	 break;
	 }

      Debug("Read completed..\n");

      if (strncmp(sendbuffer,CFFAILEDSTR,strlen(CFFAILEDSTR)) == 0)
	 {
	 Debug("SENT FAILSTRING BY MISTAKE!\n");
	 }
      
      if (n_read == 0)
	 {
	 break;
	 }
      else
	 { int savedlen = statbuf.st_size;

	 /* This can happen with log files /databases etc */

	 if (count++ % 3 == 0) /* Don't do this too often */
	    {
	    Debug("Restatting %s\n",filename);
	    stat(filename,&statbuf);
	    }
	 
	 if (statbuf.st_size != savedlen)
	    {
	    snprintf(sendbuffer,bufsize,"%s%s: %s",CFCHANGEDSTR1,CFCHANGEDSTR2,filename);
	    if (SendSocketStream(sd,sendbuffer,args->buf_size,0) == -1)
	       {
	       CfLog(cfverbose,"Send failed in GetFile","send");
	       }
	    
	    Debug("Aborting transfer after %d: file is changing rapidly at source.\n",total);
	    break;
	    }
	 
	 if ((savedlen - total)/args->buf_size > 0)
	    {
	    sendlen = args->buf_size;
	    }
	 else if (savedlen != 0)
	    {
	    sendlen = (savedlen - total);
	    }
	 }

      total += n_read;

      if (args->encrypt)
	 {
	 if (!EVP_EncryptUpdate(&ctx,out,&cipherlen,sendbuffer,n_read))
	    {
	    close(fd);
	    return;
	    }

	 if (cipherlen)
	    {
	    if (SendTransaction(sd,out,cipherlen,CF_MORE) == -1)
	       {
	       CfLog(cfverbose,"Send failed in GetFile","send");
	       break;
	       }
	    }
	 }
      else
	 {
	 Debug("Sending data on socket (%d)\n",sendlen);
	 
	 if (SendSocketStream(sd,sendbuffer,sendlen,0) == -1)
	    {
	    CfLog(cfverbose,"Send failed in GetFile","send");
	    break;
	    }

	 Debug("Sending complete...\n");
	 }     
      }

   if (args->encrypt)
      {
      if (!EVP_EncryptFinal(&ctx,out,&cipherlen))
	 {
	 close(fd);
	 return;
	 }

      Debug("Cipher len of extra data is %d\n",cipherlen);
      SendTransaction(sd,out,cipherlen,CF_DONE);
      EVP_CIPHER_CTX_cleanup(&ctx);
      }

   close(fd);    
   }
 
Debug("Done with GetFile()\n"); 
}

/**************************************************************/

void CompareLocalChecksum(conn,sendbuffer,recvbuffer)

struct cfd_connection *conn;
char *sendbuffer, *recvbuffer;

{ unsigned char digest[EVP_MAX_MD_SIZE+1];
  char filename[bufsize];
  char *sp;
  int i;

sscanf(recvbuffer,"MD5 %[^\n]",filename);

sp = recvbuffer + strlen(recvbuffer) + CF_SMALL_OFFSET;
 
for (i = 0; i < CF_MD5_LEN; i++)
   {
   digest[i] = *sp++;
   }
 
Debug("CompareLocalChecksums(%s,%s)\n",filename,ChecksumPrint('m',digest));
bzero(sendbuffer,bufsize);

if (ChecksumChanged(filename,digest,cfverbose,true,'m'))
   {
   sprintf(sendbuffer,"%s",CFD_TRUE);
   Debug("Checksums didn't match\n");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   }
else
   {
   sprintf(sendbuffer,"%s",CFD_FALSE);
   Debug("Checksums matched ok\n");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   }
}

/**************************************************************/

int CfOpenDirectory(conn,sendbuffer,dirname)

struct cfd_connection *conn;
char *sendbuffer, *dirname;

{ DIR *dirh;
  struct dirent *dirp;
  int offset;

Debug("CfOpenDirectory(%s)\n",dirname);
  
if (*dirname != '/')
   {
   sprintf(sendbuffer,"BAD: request to access a non-absolute filename\n");
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return -1;
   }

if ((dirh = opendir(dirname)) == NULL)
   {
   Debug("cfengine, couldn't open dir %s\n",dirname);
   snprintf(sendbuffer,bufsize,"BAD: cfengine, couldn't open dir %s\n",dirname);
   SendTransaction(conn->sd_reply,sendbuffer,0,CF_DONE);
   return -1;
   }

/* Pack names for transmission */

bzero(sendbuffer,bufsize);

offset = 0;

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (strlen(dirp->d_name)+1+offset >= bufsize - maxlinksize)
      {
      SendTransaction(conn->sd_reply,sendbuffer,offset+1,CF_MORE);
      offset = 0;
      bzero(sendbuffer,bufsize);
      }

   strncpy(sendbuffer+offset,dirp->d_name,maxlinksize);
   offset += strlen(dirp->d_name) + 1;     /* + zero byte separator */
   }
 
strcpy(sendbuffer+offset,CFD_TERMINATOR);
SendTransaction(conn->sd_reply,sendbuffer,offset+2+strlen(CFD_TERMINATOR),CF_DONE);
Debug("END CfOpenDirectory(%s)\n",dirname);
closedir(dirh);
return 0;
}

/***************************************************************/

void Terminate(sd)

int sd;

{ char buffer[bufsize];

bzero(buffer,bufsize);

strcpy(buffer,CFD_TERMINATOR);

if (SendTransaction(sd,buffer,strlen(buffer)+1,CF_DONE) == -1)
   {
   CfLog(cfverbose,"","send");
   Verbose("Unable to reply with terminator...\n");
   }
}

/***************************************************************/

void DeleteAuthList(ap)

struct Auth *ap;

{
if (ap != NULL)
   {
   DeleteAuthList(ap->next);
   ap->next = NULL;

   DeleteItemList(ap->accesslist);

   free((char *)ap);
   }
}

/***************************************************************/
/* Level 5                                                     */
/***************************************************************/

void RefuseAccess(conn,sendbuffer,size,errmesg)

struct cfd_connection *conn;
char *sendbuffer,*errmesg;
int size;

{ char *hostname, *username, *ipaddr;
  static char *def = "?"; 

if (strlen(conn->hostname) == 0)
   {
   hostname = def;
   }
else
   {
   hostname = conn->hostname;
   }

if (strlen(conn->username) == 0)
   {
   username = def;
   }
else
   {
   username = conn->username;
   }

if (strlen(conn->ipaddr) == 0)
   {
   ipaddr = def;
   }
else
   {
   ipaddr = conn->ipaddr;
   }
 
snprintf(sendbuffer,bufsize,"%s",CFFAILEDSTR);
CfLog(cfinform,"Host authorization/authentication failed or access denied\n","");
SendTransaction(conn->sd_reply,sendbuffer,size,CF_DONE);
snprintf(sendbuffer,bufsize,"From (host=%s,user=%s,ip=%s)",hostname,username,ipaddr);
CfLog(cfinform,sendbuffer,"");
if (strlen(errmesg) > 0)
   {
   snprintf(OUTPUT,bufsize,"ID from connecting host: (%s)",errmesg);
   CfLog(cflogonly,OUTPUT,"");
   }
}

/***************************************************************/

void ReplyNothing(conn)

struct cfd_connection *conn;

{ char buffer[bufsize];

snprintf(buffer,bufsize,"Hello %s (%s), nothing relevant to do here...\n\n",conn->hostname,conn->ipaddr);

if (SendTransaction(conn->sd_reply,buffer,0,CF_DONE) == -1)
   {
   CfLog(cferror,"","send");
   }
}

/***************************************************************/

char *MapAddress(unspec_address)

char *unspec_address;

{ /* Is the address a mapped ipv4 over ipv6 address */

if (strncmp(unspec_address,"::ffff:",7) == 0)
   {
   return (char *)(unspec_address+7);
   }
else
   {
   return unspec_address;
   }
}

/***************************************************************/

int CheckStoreKey(conn,key)

struct cfd_connection *conn;
RSA *key;

{ RSA *savedkey;
 char keyname[maxvarsize];

if (OptionIs(CONTEXTID,"HostnameKeys",true))
   {
   snprintf(keyname,maxvarsize,"%s-%s",conn->username,conn->hostname);
   }
else
   {
   snprintf(keyname,maxvarsize,"%s-%s",conn->username,MapAddress(conn->ipaddr));
   }
 
if (savedkey = HavePublicKey(keyname))
   {
   Verbose("A public key was already known from %s/%s - no trust required\n",conn->hostname,conn->ipaddr);

   Verbose("Adding IP %s to SkipVerify - no need to check this if we have a key\n",conn->ipaddr);
   PrependItem(&SKIPVERIFY,MapAddress(conn->ipaddr),NULL);
   
   if ((BN_cmp(savedkey->e,key->e) == 0) && (BN_cmp(savedkey->n,key->n) == 0))
      {
      Verbose("The public key identity was confirmed as %s@%s\n",conn->username,conn->hostname);
      SendTransaction(conn->sd_reply,"OK: key accepted",0,CF_DONE);
      RSA_free(savedkey);
      return true;
      }
   else
      {
      /* If we find a key, but it doesn't match, see if we permit dynamical IP addressing */
      
      if ((DHCPLIST != NULL) && IsFuzzyItemIn(DHCPLIST,MapAddress(conn->ipaddr)))
	 {
	 int result;
	 result = IsWildKnownHost(savedkey,key,MapAddress(conn->ipaddr),conn->username);
	 RSA_free(savedkey);
	 if (result)
	    {
	    SendTransaction(conn->sd_reply,"OK: key accepted",0,CF_DONE);
	    }
	 else
	    {
	    SendTransaction(conn->sd_reply,"BAD: keys did not match",0,CF_DONE);
	    }
	 return result;
	 }
      else /* if not, reject it */
	 {
	 Verbose("The new public key does not match the old one! Spoofing attempt!\n");
	 SendTransaction(conn->sd_reply,"BAD: keys did not match",0,CF_DONE);
	 RSA_free(savedkey);
	 return false;
	 }
      }
   
   return true;
   }
else if ((TRUSTKEYLIST != NULL) && IsFuzzyItemIn(TRUSTKEYLIST,MapAddress(conn->ipaddr)))
   {
   Verbose("Host %s/%s was found in the list of hosts to trust\n",conn->hostname,conn->ipaddr);
   conn->trust = true;
   /* conn->maproot = false; ?? */
   SendTransaction(conn->sd_reply,"OK: key was accepted on trust",0,CF_DONE);
   SavePublicKey(keyname,key);
   AddToKeyDB(key,MapAddress(conn->ipaddr));
   return true;
   }
else
   {
   Verbose("No previous key found, and unable to accept this one on trust\n");
   SendTransaction(conn->sd_reply,"BAD: key could not be accepted on trust",0,CF_DONE);
   return false; 
   }
}

/***************************************************************/

int IsWildKnownHost(oldkey,newkey,mipaddr,username)

RSA *oldkey, *newkey;
char *mipaddr, *username;

/* This builds security from trust only gradually with DHCP - takes time!
   But what else are we going to do? ssh doesn't have this problem - it
   just asks the user interactively. We can't do that ... */

{ DBT key,value;
  DB *dbp;
  int trust = false;
  char keyname[maxvarsize];
  char keydb[maxvarsize];

snprintf(keyname,maxvarsize,"%s-%s",username,mipaddr);
snprintf(keydb,maxvarsize,"%s/ppkeys/dynamic",WORKDIR); 

Debug("The key does not match the known, key but the host has dynamic IP...\n"); 
 
if ((TRUSTKEYLIST != NULL) && IsFuzzyItemIn(TRUSTKEYLIST,mipaddr))
   {
   Debug("We will accept a new key for this IP on trust\n");
   trust = true;
   }
else
   {
   Debug("Will not accept the new key, unless we have seen it before\n");
   }

/* If the host is allowed to have a variable IP range, we can accept
   the new key on trust for the given IP address provided we have seen
   the key before.  Check for it in a database .. */

Debug("Checking to see if we have seen the new key before..\n"); 
  
if ((errno = db_create(&dbp,NULL,0)) != 0)
   {
   sprintf(OUTPUT,"Couldn't open average database %s\n",keydb);
   CfLog(cferror,OUTPUT,"db_open");
   return false;
   }
 
#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,keydb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,keydb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   sprintf(OUTPUT,"Couldn't open average database %s\n",keydb);
   CfLog(cferror,OUTPUT,"db_open");
   return false;
   }

bzero(&key,sizeof(newkey));       
bzero(&value,sizeof(value));
      
key.data = newkey;
key.size = sizeof(RSA);

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
   {
   Debug("The new key is not previously known, so we need to use policy for trusting the host %s\n",mipaddr);

   if (trust)
      {
      Debug("Policy says to trust the changed key from %s and note that it could vary in future\n",mipaddr);
      bzero(&key,sizeof(key));       
      bzero(&value,sizeof(value));
      
      key.data = newkey;
      key.size = sizeof(RSA);
      
      value.data = mipaddr;
      value.size = strlen(mipaddr)+1;
      
      if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
	 {
	 dbp->err(dbp,errno,NULL);
	 }
      }
   else
      {
      Debug("Found no grounds for trusting this new key from %s\n",mipaddr);
      }
   }
else
   {
   snprintf(OUTPUT,bufsize,"Public key was previously owned by %s now by %s - updating\n",value.data,mipaddr);
   CfLog(cfverbose,OUTPUT,"");
   Debug("Now trusting this new key, because we have seen it before\n");
   DeletePublicKey(keyname);
   trust = true;
   }

/* save this new key in the database, for future reference, regardless
   of whether we accept, but only change IP if trusted  */ 

SavePublicKey(keyname,newkey);

dbp->close(dbp,0);
chmod(keydb,0644); 
 
return trust; 
}

/***************************************************************/

void AddToKeyDB(newkey,mipaddr)

RSA *newkey;
char *mipaddr;

{ DBT key,value;
  DB *dbp;
  char keydb[maxvarsize];

snprintf(keydb,maxvarsize,"%s/ppkeys/dynamic",WORKDIR); 
  
if ((DHCPLIST != NULL) && IsFuzzyItemIn(DHCPLIST,mipaddr))
   {
   /* Cache keys in the db as we see them is there are dynamical addresses */
   
   if ((errno = db_create(&dbp,NULL,0)) != 0)
      {
      sprintf(OUTPUT,"Couldn't open average database %s\n",keydb);
      CfLog(cferror,OUTPUT,"db_open");
      return;
      }

#ifdef CF_OLD_DB
   if ((errno = dbp->open(dbp,keydb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
   if ((errno = dbp->open(dbp,NULL,keydb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
      {
      sprintf(OUTPUT,"Couldn't open average database %s\n",keydb);
      CfLog(cferror,OUTPUT,"db_open");
      return;
      }
   
   bzero(&key,sizeof(key));       
   bzero(&value,sizeof(value));
   
   key.data = newkey;
   key.size = sizeof(RSA);
   
   value.data = mipaddr;
   value.size = strlen(mipaddr)+1;
   
   if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
      {
      dbp->err(dbp,errno,NULL);
      }
   
   dbp->close(dbp,0);
   chmod(keydb,0644); 
   }
}

/***************************************************************/
/* Toolkit/Class: conn                                         */
/***************************************************************/

struct cfd_connection *NewConn(sd)  /* construct */

int sd;

{ struct cfd_connection *conn;

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_lock failed","lock");
    }
#endif
 
conn = (struct cfd_connection *) malloc(sizeof(struct cfd_connection));

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_unlock failed","lock");
    }
#endif
 
if (conn == NULL)
   {
   CfLog(cferror,"Unable to allocate conn","malloc");
   ExitCleanly(0);
   }
 
conn->sd_reply = sd;
conn->id_verified = false;
conn->rsa_auth = false;
conn->trust = false; 
conn->hostname[0] = '\0';
conn->ipaddr[0] = '\0';
conn->username[0] = '\0'; 
conn->session_key = NULL;

 
Debug("*** New socket [%d]\n",sd);
 
return conn;
}

/***************************************************************/

void DeleteConn(conn) /* destruct */

struct cfd_connection *conn;

{
Debug("***Closing socket %d from %s\n",conn->sd_reply,conn->ipaddr);

close(conn->sd_reply);
 
if (conn->ipaddr != NULL)
   {
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
   if (pthread_mutex_lock(&MUTEX_COUNT) != 0)
      {
      CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
      DeleteConn(conn);
      return;
      }
#endif

   DeleteItemMatching(&CONNECTIONLIST,MapAddress(conn->ipaddr));

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
   if (pthread_mutex_unlock(&MUTEX_COUNT) != 0)
      {
      CfLog(cferror,"pthread_mutex_unlock failed","pthread_mutex_unlock");
      DeleteConn(conn);
      return;
      }
#endif
   }
 
free ((char *)conn);
}

/***************************************************************/
/* ERS                                                         */
/***************************************************************/

int SafeOpen(filename)

char *filename;

{ int fd;

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
    }
#endif
 
fd = open(filename,O_RDONLY);

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_unlock failed","pthread_mutex_unlock");
    }
#endif

return fd;
}

    
/***************************************************************/
    
void SafeClose(fd)

int fd;

{
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_lock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
    }
#endif
 
close(fd);

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
    {
    CfLog(cferror,"pthread_mutex_unlock failed","pthread_mutex_unlock");
    }
#endif
}

/***************************************************************/

int cfscanf(in,len1,len2,out1,out2,out3)
   
char *in,*out1,*out2,*out3;
int len1, len2;
   
{  
int len3=0;
char *sp;
   
   
sp = in;
bcopy(sp,out1,len1);
out1[len1]='\0';
   
sp += len1 + 1;
bcopy(sp,out2,len2);
   
sp += len2 + 1;
len3=strlen(sp);
bcopy(sp,out3,len3);
out3[len3]='\0';
   
return (len1 + len2 + len3 + 2);
}  
   
/***************************************************************/
/* Linking simplification                                      */
/***************************************************************/

int RecursiveTidySpecialArea(name,tp,maxrecurse,sb)

char *name;
struct Tidy *tp;
int maxrecurse;
struct stat *sb;

{
 return true;
}

int CompareMD5Net(file1,file2,ip)

char *file1, *file2;
struct Image *ip;

{
 return 0;
}

struct Method *IsDefinedMethod(name,s)

char *name,*s;

{
return NULL; 
}
/* EOF */



