/* 

        Copyright (C) 2001-
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
/* File: cfenvd.c                                                            */
/*                                                                           */
/* Description: Long term state registry                                     */
/*                                                                           */
/* Based in part on results of the ECG project, by Mark, Sigmund Straumsnes  */
/* and Hårek Haugerud, Oslo University College 1998                          */
/*                                                                           */
/*****************************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"
#ifdef HAVE_SYS_LOADAVG_H
# include <sys/loadavg.h>
#else
# define LOADAVG_5MIN    1
#endif


/*****************************************************************************/
/* Globals                                                                   */
/*****************************************************************************/

#define CFGRACEPERIOD 4.0     /* training period in units of counters (weeks,iterations)*/
#define cf_noise_threshold 10 /* number that does not warrent large anomaly status */
#define big_number 100000

unsigned int HISTOGRAM[2*CF_NETATTR+ATTR*2+5+PH_LIMIT][7][CF_GRAINS];

int HISTO = false;
int NUMBER_OF_USERS;
int ROOTPROCS;
int OTHERPROCS;
int DISKFREE;
int LOADAVG;
int INCOMING[ATTR];
int OUTGOING[ATTR];
int NETIN[CF_NETATTR];
int NETOUT[CF_NETATTR];
int PH_SAMP[PH_LIMIT];
int PH_LAST[PH_LIMIT];
int PH_DELTA[PH_LIMIT];
int SLEEPTIME = 2.5 * 60; /* Should be a fraction of 5 minutes */
int BATCH_MODE = false;

double ITER = 0.0;        /* Iteration since start */
double AGE,WAGE;          /* Age and weekly age of database */

char OUTPUT[CF_BUFSIZE*2];

char BATCHFILE[CF_BUFSIZE];
char STATELOG[CF_BUFSIZE];
char ENV_NEW[CF_BUFSIZE];
char ENV[CF_BUFSIZE];

short TCPDUMP = false;
short TCPPAUSE = false;
FILE *TCPPIPE;

struct Averages LOCALAV;
struct Item *ALL_INCOMING = NULL;
struct Item *ALL_OUTGOING = NULL;
struct Item *NETIN_DIST[CF_NETATTR];
struct Item *NETOUT_DIST[CF_NETATTR];


double ENTROPY = 0.0;
double LAST_HOUR_ENTROPY = 0.0;
double LAST_DAY_ENTROPY = 0.0;

struct Item *PREVIOUS_STATE = NULL;
struct Item *ENTROPIES = NULL;

struct option CFDENVOPTIONS[] =
   {
   {"help",no_argument,0,'h'},
   {"debug",optional_argument,0,'d'}, 
   {"verbose",no_argument,0,'v'},
   {"no-fork",no_argument,0,'F'},
   {"histograms",no_argument,0,'H'},
   {"tcpdump",no_argument,0,'T'},
   {"file",optional_argument,0,'f'},
   {NULL,0,0,0}
   };

short NO_FORK = false;

/*****************************************************************************/

char *PH_BINARIES[PH_LIMIT] =   /* Miss leading slash */
   {
   "usr/sbin/atd",
   "sbin/getty",
   "bin/bash",
   "usr/sbin/exim",
   "bin/run-parts",
   };

/*******************************************************************/
/* Prototypes                                                      */
/*******************************************************************/

void CheckOptsAndInit ARGLIST((int argc,char **argv));
void Syntax ARGLIST((void));
void StartServer ARGLIST((int argc, char **argv));
void *ExitCleanly ARGLIST((void));
void yyerror ARGLIST((char *s));
void FatalError ARGLIST((char *s));
void RotateFiles ARGLIST((char *s, int n));

void GetDatabaseAge ARGLIST((void));
void LoadHistogram  ARGLIST((void));
void GetQ ARGLIST((void));
char *GetTimeKey ARGLIST((void));
struct Averages EvalAvQ ARGLIST((char *timekey));
void ArmClasses ARGLIST((struct Averages newvals,char *timekey));

void GatherProcessData ARGLIST((void));
void GatherDiskData ARGLIST((void));
void GatherLoadData ARGLIST((void));
void GatherSocketData ARGLIST((void));
void GatherPhData ARGLIST((void));
struct Averages *GetCurrentAverages ARGLIST((char *timekey));
void UpdateAverages ARGLIST((char *timekey, struct Averages newvals));
void UpdateDistributions ARGLIST((char *timekey, struct Averages *av));
double WAverage ARGLIST((double newvals,double oldvals, double age));
double SetClasses ARGLIST((char *name,double variable,double av_expect,double av_var,double localav_expect,double localav_var,struct Item **classlist,char *timekey));
void SetVariable ARGLIST((char *name,double now, double average, double stddev, struct Item **list));
void RecordChangeOfState  ARGLIST((struct Item *list,char *timekey));
double RejectAnomaly ARGLIST((double new,double av,double var,double av2,double var2));
int HashPhKey ARGLIST((char *s));
void SetEntropyClasses ARGLIST((char *service,struct Item *list,char *inout));
void AnalyzeArrival ARGLIST((char *tcpbuffer));
void ZeroArrivals ARGLIST((void));
void TimeOut ARGLIST((void));
void IncrementCounter ARGLIST((struct Item **list,char *name));
void SaveTCPEntropyData ARGLIST((struct Item *list,int i, char *inout));

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (int argc,char **argv)

{
CheckOptsAndInit(argc,argv);
GetNameInfo();
GetInterfaceInfo();
GetV6InterfaceInfo();  
StartServer(argc,argv);
return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(int argc,char **argv)

{ extern char *optarg;
 int optindex = 0;
 int c, i,j,k;

umask(077);
sprintf(VPREFIX,"cfenvd"); 
openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);

strcpy(CFLOCK,"cfenvd");
 
IGNORELOCK = false; 
OUTPUT[0] = '\0';

while ((c=getopt_long(argc,argv,"d:vhHFVT",CFDENVOPTIONS,&optindex)) != EOF)
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
  printf("cfenvd: Debug mode: running in foreground\n");
                break;

      case 'v': VERBOSE = true;
         break;

      case 'V': printf("GNU %s-%s daemon\n%s\n",PACKAGE,VERSION,COPYRIGHT);
         printf("This program is covered by the GNU Public License and may be\n");
  printf("copied free of charge. No warrenty is implied.\n\n");
                exit(0);
         break;

      case 'F': NO_FORK = true;
         break;

      case 'H': HISTO = true;
         break;

      case 'T': TCPDUMP = true;
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
 
sprintf(VBUFF,"%s/test",CFWORKDIR);
MakeDirectoriesFor(VBUFF,'y');
sprintf(VBUFF,"%s/state/test",CFWORKDIR);
MakeDirectoriesFor(VBUFF,'y');
strncpy(VLOCKDIR,CFWORKDIR,CF_BUFSIZE-1);
strncpy(VLOGDIR,CFWORKDIR,CF_BUFSIZE-1);

for (i = 0; i < ATTR; i++)
   {
   sprintf(VBUFF,"%s/state/cf_incoming.%s",CFWORKDIR,ECGSOCKS[i][1]);
   CreateEmptyFile(VBUFF);
   sprintf(VBUFF,"%s/state/cf_outgoing.%s",CFWORKDIR,ECGSOCKS[i][1]);
   CreateEmptyFile(VBUFF);
   }

for (i = 0; i < CF_NETATTR; i++)
   {
   NETIN_DIST[i] = NULL;
   NETOUT_DIST[i] = NULL;
   }
 
sprintf(VBUFF,"%s/state/cf_users",CFWORKDIR);
CreateEmptyFile(VBUFF);
 
snprintf(AVDB,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_AVDB_FILE);
snprintf(STATELOG,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_STATELOG_FILE);
snprintf(ENV_NEW,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_ENVNEW_FILE);
snprintf(ENV,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_ENV_FILE);

if (!BATCH_MODE)
   {
   GetDatabaseAge();
   LOCALAV.expect_number_of_users = 0.0; 
   LOCALAV.expect_rootprocs = 0.0;
   LOCALAV.expect_otherprocs = 0.0;
   LOCALAV.expect_diskfree = 0.0;
   LOCALAV.expect_loadavg = 0.0;
   LOCALAV.var_number_of_users = 0.0; 
   LOCALAV.var_rootprocs = 0.0;
   LOCALAV.var_otherprocs = 0.0;
   LOCALAV.var_diskfree = 0.0;
   LOCALAV.var_loadavg = 0.0;

   for (i = 0; i < ATTR; i++)
      {
      LOCALAV.expect_incoming[i] = 0.0;
      LOCALAV.expect_outgoing[i] = 0.0;
      LOCALAV.var_incoming[i] = 0.0;
      LOCALAV.var_outgoing[i] = 0.0;
      }

   for (i = 0; i < CF_NETATTR; i++)
      {
      LOCALAV.expect_netin[i] = 0.0;
      LOCALAV.expect_netout[i] = 0.0;
      LOCALAV.var_netin[i] = 0.0;
      LOCALAV.var_netout[i] = 0.0;
      }

   for (i = 0; i < PH_LIMIT; i++)
      {
      LOCALAV.expect_pH[i] = 0.0;
      LOCALAV.var_pH[i] = 0.0;
      }

   }

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < 2*CF_NETATTR+ATTR*2+5+PH_LIMIT; j++)
      {
      for (k = 0; k < CF_GRAINS; k++)
         {
         HISTOGRAM[i][j][k] = 0;
         }
      }
   }
 
 for (i = 0; i < PH_LIMIT; i++)
    {
    PH_SAMP[i] = PH_LAST[i] = 0.0;
    }
 
srand((unsigned int)time(NULL)); 
LoadHistogram(); 
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine environment daemon\n%s-%s\n%s\n",PACKAGE,VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; CFDENVOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",CFDENVOPTIONS[i].name,(char)CFDENVOPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@gnu.org (News: gnu.cfengine.bug)\n");
printf("General help to help-cfengine@gnu.org (News: gnu.cfengine.help)\n");
printf("Info & fixes at http://www.iu.hio.no/cfengine\n");
}

/*********************************************************************/

void GetDatabaseAge()

{ int errno;
  DBT key,value;
  DB *dbp;

if ((errno = db_create(&dbp,NULL,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   AGE = WAGE = 0;
   snprintf(OUTPUT,CF_BUFSIZE,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

chmod(AVDB,0644); 

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
      
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE")+1;

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp,errno,NULL);
      dbp->close(dbp,0);
      return;
      }
   }
 
dbp->close(dbp,0);

if (value.data != NULL)
   {
   AGE = *(double *)(value.data);
   WAGE = AGE / CF_WEEK * CF_MEASURE_INTERVAL;
   Debug("\n\nPrevious DATABASE_AGE %f\n\n",AGE);
   }
else
   {
   Debug("No previous AGE\n");
   AGE = 0.0;
   }
}

/*********************************************************************/

void LoadHistogram()

{ FILE *fp;
  int position,i,day; 

if (HISTO)
   {
   char filename[CF_BUFSIZE];
   
   snprintf(filename,CF_BUFSIZE,"%s/state/histograms",CFWORKDIR);
   
   if ((fp = fopen(filename,"r")) == NULL)
      {
      CfLog(cfverbose,"Unable to load histogram data","fopen");
      return;
      }

   for (position = 0; position < CF_GRAINS; position++)
      {
      fscanf(fp,"%d ",&position);
      
      for (i = 0; i < 5 + 2*CF_NETATTR + 2*ATTR + PH_LIMIT; i++)
         {
         for (day = 0; day < 7; day++)
            {
            fscanf(fp,"%d ",&(HISTOGRAM[i][day][position]));
            }
         }
      }
   
   fclose(fp);
   }
} 

/*********************************************************************/

void StartServer(int argc,char **argv)

{ char *timekey;
  struct Averages averages;
  void HandleSignal();
  char tcpbuffer[CF_BUFSIZE];
  int i;

if ((!NO_FORK) && (fork() != 0))
   {
   sprintf(OUTPUT,"cfenvd: starting\n");
   CfLog(cfinform,OUTPUT,"");
   exit(0);
   }

if (!NO_FORK)
   {
   ActAsDaemon(0);
   }

signal (SIGTERM,HandleSignal);                   /* Signal Handler */
signal (SIGHUP,HandleSignal);
signal (SIGINT,HandleSignal);
signal (SIGPIPE,HandleSignal);
signal (SIGSEGV,HandleSignal);

VCANONICALFILE = strdup("db");
 
if (!GetLock("cfenvd","daemon",0,1,"localhost",(time_t)time(NULL)))
   {
   return;
   }

if (TCPDUMP)
   {
   struct stat statbuf;
   char buffer[CF_MAXVARSIZE];
   sscanf(CF_TCPDUMP_COMM,"%s",buffer);
   
   if (stat(buffer,&statbuf) != -1)
      {
      if ((TCPPIPE = cfpopen(CF_TCPDUMP_COMM,"r")) == NULL)
         {
         TCPDUMP = false;
         }
      }
   else
      {
      TCPDUMP = false;
      }
   }
 
 while (true)
   {
   GetQ();
   timekey = GetTimeKey();
   averages = EvalAvQ(timekey);
   ArmClasses(averages,timekey);

   if (TCPDUMP)
      {
      memset(tcpbuffer,0,CF_BUFSIZE);      
      ZeroArrivals();
      signal(SIGALRM,(void *)TimeOut);
      alarm(SLEEPTIME);
      TCPPAUSE = false;
      
      while (!feof(TCPPIPE))
         {
         if (TCPPAUSE)
            {
            break;
            }
         
         fgets(tcpbuffer,CF_BUFSIZE-1,TCPPIPE);
         
         if (TCPPAUSE)
            {
            break;
            }

         if (strstr(tcpbuffer,":")) /* Error message protect sleeptime */
            {
            signal(SIGALRM,SIG_DFL);
            TCPDUMP = false;
            break;
            }
         
         AnalyzeArrival(tcpbuffer);
         }
      
      signal(SIGALRM,SIG_DFL);
      TCPPAUSE = false;
      fflush(TCPPIPE);
      
      for (i = 0; i < CF_NETATTR; i++)
         {
         Verbose(" > TCPDUMP FOUND: %d/%d pckts in %s \n",NETIN[i],NETOUT[i],TCPNAMES[i]);
         }
      }
   else
      {      
      sleep(SLEEPTIME);
      }
   
   ITER++;
   }
}

/*********************************************************************/

void yyerror(char *s)

{
 /* Dummy */
}

/*********************************************************************/

void RotateFiles(char *name,int number)

{
 /* Dummy */
}

/*********************************************************************/

void FatalError(char *s)

{
fprintf (stderr,"%s:%s:%s\n",VPREFIX,VCURRENTFILE,s);
closelog(); 
exit(1);
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/
  
void TimeOut()
 
{
alarm(0);
TCPPAUSE = true;
Verbose("Time out\n");
}

/*********************************************************************/

void GetQ()

{
Debug("========================= GET Q ==============================\n");

ENTROPIES = NULL;

GatherProcessData();
GatherLoadData(); 
GatherDiskData();
GatherSocketData();
GatherPhData(); 
}


/*********************************************************************/

char *GetTimeKey()

{ time_t now;
  char str[64];
  
if ((now = time((time_t *)NULL)) == -1)
   {
   exit(1);
   }

sprintf(str,"%s",ctime(&now));

return ConvTimeKey(str); 
}


/*********************************************************************/

struct Averages EvalAvQ(char *t)

{ struct Averages *currentvals,newvals;
  int i; 
  double Number_Of_Users,Rootprocs,Otherprocs,Diskfree,LoadAvg;
  double Incoming[ATTR],Outgoing[ATTR],pH_delta[PH_LIMIT],NetIn[CF_NETATTR],NetOut[CF_NETATTR];

if ((currentvals = GetCurrentAverages(t)) == NULL)
   {
   CfLog(cferror,"Error reading average database","");
   exit(1);
   }

/* Discard any apparently anomalous behaviour before renormalizing database */

Number_Of_Users = RejectAnomaly(NUMBER_OF_USERS,currentvals->expect_number_of_users,currentvals->var_number_of_users,LOCALAV.expect_number_of_users,LOCALAV.var_number_of_users);
Rootprocs = RejectAnomaly(ROOTPROCS,currentvals->expect_rootprocs,currentvals->var_rootprocs,LOCALAV.expect_rootprocs,LOCALAV.var_rootprocs);
Otherprocs = RejectAnomaly(OTHERPROCS,currentvals->expect_otherprocs,currentvals->var_otherprocs,LOCALAV.expect_otherprocs,LOCALAV.var_otherprocs);
Diskfree = RejectAnomaly(DISKFREE,currentvals->expect_diskfree,currentvals->var_diskfree,LOCALAV.expect_diskfree,LOCALAV.var_diskfree);
LoadAvg = RejectAnomaly(LOADAVG,currentvals->expect_loadavg,currentvals->var_loadavg,LOCALAV.expect_loadavg,LOCALAV.var_loadavg);

for (i = 0; i < ATTR; i++)
   {
   Incoming[i] = RejectAnomaly(INCOMING[i],currentvals->expect_incoming[i],currentvals->var_incoming[i],LOCALAV.expect_incoming[i],LOCALAV.var_incoming[i]);
   Outgoing[i] = RejectAnomaly(OUTGOING[i],currentvals->expect_outgoing[i],currentvals->var_outgoing[i],LOCALAV.expect_outgoing[i],LOCALAV.var_outgoing[i]);
   }


for (i = 0; i < PH_LIMIT; i++)
   {
   pH_delta[i] = RejectAnomaly(PH_DELTA[i],currentvals->expect_pH[i],currentvals->var_pH[i],LOCALAV.expect_pH[i],LOCALAV.var_pH[i]);
   }

for (i = 0; i < CF_NETATTR; i++)
   {
   NetIn[i] = RejectAnomaly(NETIN[i],currentvals->expect_netin[i],currentvals->var_netin[i],LOCALAV.expect_netin[i],LOCALAV.var_netin[i]);
   NetOut[i] = RejectAnomaly(NETOUT[i],currentvals->expect_netout[i],currentvals->var_netout[i],LOCALAV.expect_netout[i],LOCALAV.var_netout[i]);
   }
 
newvals.expect_number_of_users = WAverage(Number_Of_Users,currentvals->expect_number_of_users,WAGE);
newvals.expect_rootprocs = WAverage(Rootprocs,currentvals->expect_rootprocs,WAGE);
newvals.expect_otherprocs = WAverage(Otherprocs,currentvals->expect_otherprocs,WAGE);
newvals.expect_diskfree = WAverage(Diskfree,currentvals->expect_diskfree,WAGE);
newvals.expect_loadavg = WAverage(LoadAvg,currentvals->expect_loadavg,WAGE); 

LOCALAV.expect_number_of_users = WAverage(newvals.expect_number_of_users,LOCALAV.expect_number_of_users,ITER); 
LOCALAV.expect_rootprocs = WAverage(newvals.expect_rootprocs,LOCALAV.expect_rootprocs,ITER);
LOCALAV.expect_otherprocs = WAverage(newvals.expect_otherprocs,LOCALAV.expect_otherprocs,ITER);
LOCALAV.expect_diskfree = WAverage(newvals.expect_diskfree,LOCALAV.expect_diskfree,ITER);
LOCALAV.expect_loadavg = WAverage(newvals.expect_loadavg,LOCALAV.expect_loadavg,ITER); 
 
newvals.var_number_of_users = WAverage((Number_Of_Users-newvals.expect_number_of_users)*(Number_Of_Users-newvals.expect_number_of_users),currentvals->var_number_of_users,WAGE);
newvals.var_rootprocs = WAverage((Rootprocs-newvals.expect_rootprocs)*(Rootprocs-newvals.expect_rootprocs),currentvals->var_rootprocs,WAGE);
newvals.var_otherprocs = WAverage((Otherprocs-newvals.expect_otherprocs)*(Otherprocs-newvals.expect_otherprocs),currentvals->var_otherprocs,WAGE);
newvals.var_diskfree = WAverage((Diskfree-newvals.expect_diskfree)*(Diskfree-newvals.expect_diskfree),currentvals->var_diskfree,WAGE);
newvals.var_loadavg = WAverage((LoadAvg-newvals.expect_loadavg)*(LoadAvg-newvals.expect_loadavg),currentvals->var_loadavg,WAGE); 

LOCALAV.var_number_of_users = WAverage((Number_Of_Users-LOCALAV.expect_number_of_users)*(Number_Of_Users-LOCALAV.expect_number_of_users),LOCALAV.var_number_of_users,ITER);
LOCALAV.var_rootprocs = WAverage((Rootprocs-LOCALAV.expect_rootprocs)*(Rootprocs-LOCALAV.expect_rootprocs),LOCALAV.var_rootprocs,ITER);
LOCALAV.var_otherprocs = WAverage((Otherprocs-LOCALAV.expect_otherprocs)*(Otherprocs-LOCALAV.expect_otherprocs),LOCALAV.var_otherprocs,ITER);
LOCALAV.var_diskfree = WAverage((Diskfree-LOCALAV.expect_diskfree)*(Diskfree-LOCALAV.expect_diskfree),LOCALAV.var_diskfree,ITER);
LOCALAV.var_loadavg = WAverage((LoadAvg-LOCALAV.expect_loadavg)*(LoadAvg-LOCALAV.expect_loadavg),currentvals->var_loadavg,WAGE); 
 
Verbose("Users              = %4d -> (%lf#%lf) local [%lf#%lf]\n",NUMBER_OF_USERS,newvals.expect_number_of_users,sqrt(newvals.var_number_of_users),LOCALAV.expect_number_of_users,sqrt(LOCALAV.var_number_of_users));
Verbose("Rootproc           = %4d -> (%lf#%lf) local [%lf#%lf]\n",ROOTPROCS,newvals.expect_rootprocs,sqrt(newvals.var_rootprocs),LOCALAV.expect_rootprocs,sqrt(LOCALAV.var_rootprocs));
Verbose("Otherproc          = %4d -> (%lf#%lf) local [%lf#%lf]\n",OTHERPROCS,newvals.expect_otherprocs,sqrt(newvals.var_otherprocs),LOCALAV.expect_otherprocs,sqrt(LOCALAV.var_otherprocs));
Verbose("Diskpercent        = %4d -> (%lf#%lf) local [%lf#%lf]\n",DISKFREE,newvals.expect_diskfree,sqrt(newvals.var_diskfree),LOCALAV.expect_diskfree,sqrt(LOCALAV.var_diskfree));
Verbose("Load Average       = %4d -> (%lf#%lf) local [%lf#%lf]\n",LOADAVG,newvals.expect_loadavg,sqrt(newvals.var_loadavg),LOCALAV.expect_loadavg,sqrt(LOCALAV.var_loadavg)); 
 
for (i = 0; i < ATTR; i++)
   {
   newvals.expect_incoming[i] = WAverage(Incoming[i],currentvals->expect_incoming[i],WAGE);
   newvals.expect_outgoing[i] = WAverage(Outgoing[i],currentvals->expect_outgoing[i],WAGE);
   newvals.var_incoming[i] = WAverage((Incoming[i]-newvals.expect_incoming[i])*(Incoming[i]-newvals.expect_incoming[i]),currentvals->var_incoming[i],WAGE);
   newvals.var_outgoing[i] = WAverage((Outgoing[i]-newvals.expect_outgoing[i])*(Outgoing[i]-newvals.expect_outgoing[i]),currentvals->var_outgoing[i],WAGE);

   LOCALAV.expect_incoming[i] = WAverage(newvals.expect_incoming[i],LOCALAV.expect_incoming[i],ITER);
   LOCALAV.expect_outgoing[i] = WAverage(newvals.expect_outgoing[i],LOCALAV.expect_outgoing[i],ITER);
   LOCALAV.var_incoming[i] = WAverage((Incoming[i]-LOCALAV.expect_incoming[i])*(Incoming[i]-LOCALAV.expect_incoming[i]),LOCALAV.var_incoming[i],ITER);
   LOCALAV.var_outgoing[i] = WAverage((Outgoing[i]-LOCALAV.expect_outgoing[i])*(Outgoing[i]-LOCALAV.expect_outgoing[i]),LOCALAV.var_outgoing[i],ITER);
   
   Verbose("%-15s-in = %4d -> (%lf#%lf) local [%lf#%lf]\n",ECGSOCKS[i][1],INCOMING[i],newvals.expect_incoming[i],sqrt(newvals.var_incoming[i]),LOCALAV.expect_incoming[i],sqrt(LOCALAV.var_incoming[i]));
   Verbose("%-14s-out = %4d -> (%lf#%lf) local [%lf#%lf]\n",ECGSOCKS[i][1],OUTGOING[i],newvals.expect_outgoing[i],sqrt(newvals.var_outgoing[i]),LOCALAV.expect_outgoing[i],sqrt(LOCALAV.var_outgoing[i]));
   }

 
for (i = 0; i < CF_NETATTR; i++)
   {
   newvals.expect_netin[i] = WAverage(NetIn[i],currentvals->expect_netin[i],WAGE);
   newvals.expect_netout[i] = WAverage(NetOut[i],currentvals->expect_netout[i],WAGE);
   newvals.var_netin[i] = WAverage((NetIn[i]-newvals.expect_netin[i])*(NetIn[i]-newvals.expect_netin[i]),currentvals->var_netin[i],WAGE);
   newvals.var_netout[i] = WAverage((NetOut[i]-newvals.expect_netout[i])*(NetOut[i]-newvals.expect_netout[i]),currentvals->var_netout[i],WAGE);

   LOCALAV.expect_netin[i] = WAverage(newvals.expect_netin[i],LOCALAV.expect_netin[i],ITER);
   LOCALAV.expect_netout[i] = WAverage(newvals.expect_netout[i],LOCALAV.expect_netout[i],ITER);
   LOCALAV.var_netin[i] = WAverage((NetIn[i]-LOCALAV.expect_netin[i])*(NetIn[i]-LOCALAV.expect_netin[i]),LOCALAV.var_netin[i],ITER);
   LOCALAV.var_netout[i] = WAverage((NetOut[i]-LOCALAV.expect_netout[i])*(NetOut[i]-LOCALAV.expect_netout[i]),LOCALAV.var_netout[i],ITER);
   
   Verbose("%-15s-in = %4d -> (%lf#%lf) local [%lf#%lf]\n",TCPNAMES[i],NETIN[i],newvals.expect_netin[i],sqrt(newvals.var_netin[i]),LOCALAV.expect_netin[i],sqrt(LOCALAV.var_netin[i]));
   Verbose("%-14s-out = %4d -> (%lf#%lf) local [%lf#%lf]\n",TCPNAMES[i],NETOUT[i],newvals.expect_netout[i],sqrt(newvals.var_netout[i]),LOCALAV.expect_netout[i],sqrt(LOCALAV.var_netout[i]));
   }
 
for (i = 0; i < PH_LIMIT; i++)
   {
   if (PH_BINARIES[i] == NULL)
      {
      continue;
      }
   
   newvals.expect_pH[i] = WAverage(pH_delta[i],currentvals->expect_pH[i],WAGE);
   newvals.var_pH[i] = WAverage((pH_delta[i]-newvals.expect_pH[i])*(pH_delta[i]-newvals.expect_pH[i]),currentvals->var_pH[i],WAGE);

   LOCALAV.expect_pH[i] = WAverage(newvals.expect_pH[i],LOCALAV.expect_pH[i],ITER);
   LOCALAV.var_pH[i] = WAverage((pH_delta[i]-LOCALAV.expect_pH[i])*(pH_delta[i]-LOCALAV.expect_pH[i]),LOCALAV.var_pH[i],ITER);

   Verbose("%-15s-in = %4d -> (%lf#%lf) local [%lf#%lf]\n",CanonifyName(PH_BINARIES[i]),PH_DELTA[i],newvals.expect_pH[i],sqrt(newvals.var_pH[i]),LOCALAV.expect_pH[i],sqrt(LOCALAV.var_pH[i]));

   }
 
UpdateAverages(t,newvals);
 
if (WAGE > CFGRACEPERIOD)
   {
   UpdateDistributions(t,currentvals);  /* Distribution about mean */
   }
 
return newvals;
}

/*********************************************************************/

void ArmClasses(struct Averages av,char *timekey)

{ double sig;
  struct Item *classlist = NULL, *ip;
  int i;
  FILE *fp;

Debug("Arm classes for %s\n",timekey);
 
sig = SetClasses("Users",NUMBER_OF_USERS,av.expect_number_of_users,av.var_number_of_users,LOCALAV.expect_number_of_users,LOCALAV.var_number_of_users,&classlist,timekey);
SetVariable("users",NUMBER_OF_USERS,av.expect_number_of_users,sig,&classlist);

sig = SetClasses("RootProcs",ROOTPROCS,av.expect_rootprocs,av.var_rootprocs,LOCALAV.expect_rootprocs,LOCALAV.var_rootprocs,&classlist,timekey);
SetVariable("rootprocs",ROOTPROCS,av.expect_rootprocs,sig,&classlist);

sig = SetClasses("UserProcs",OTHERPROCS,av.expect_otherprocs,av.var_otherprocs,LOCALAV.expect_otherprocs,LOCALAV.var_otherprocs,&classlist,timekey);
SetVariable("userprocs",OTHERPROCS,av.expect_otherprocs,sig,&classlist);

sig = SetClasses("DiskFree",DISKFREE,av.expect_diskfree,av.var_diskfree,LOCALAV.expect_diskfree,LOCALAV.var_diskfree,&classlist,timekey);
SetVariable("diskfree",DISKFREE,av.expect_diskfree,sig,&classlist);

sig = SetClasses("LoadAvg",LOADAVG,av.expect_loadavg,av.var_loadavg,LOCALAV.expect_loadavg,LOCALAV.var_loadavg,&classlist,timekey);
SetVariable("loadavg",LOADAVG,av.expect_loadavg,sig,&classlist);
 
for (i = 0; i < ATTR; i++)
   {
   char name[256];
   strcpy(name,ECGSOCKS[i][1]);
   strcat(name,"_in");
   
   sig = SetClasses(name,INCOMING[i],av.expect_incoming[i],av.var_incoming[i],LOCALAV.expect_incoming[i],LOCALAV.var_incoming[i],&classlist,timekey);
   SetVariable(name,INCOMING[i],av.expect_incoming[i],sig,&classlist);

   strcpy(name,ECGSOCKS[i][1]);
   strcat(name,"_out");

   sig = SetClasses(name,OUTGOING[i],av.expect_outgoing[i],av.var_outgoing[i],LOCALAV.expect_outgoing[i],LOCALAV.var_outgoing[i],&classlist,timekey);
   SetVariable(name,OUTGOING[i],av.expect_outgoing[i],sig,&classlist);
   }

for (i = 0; i < PH_LIMIT; i++)
   {
   if (PH_BINARIES[i] == NULL)
      {
      continue;
      }
         
   sig = SetClasses(CanonifyName(PH_BINARIES[i]),PH_DELTA[i],av.expect_pH[i],av.var_pH[i],LOCALAV.expect_pH[i],LOCALAV.var_pH[i],&classlist,timekey);
   SetVariable(CanonifyName(PH_BINARIES[i]),PH_DELTA[i],av.expect_pH[i],sig,&classlist);
   }

for (i = 0; i < CF_NETATTR; i++)
   {
   char name[256];
   strcpy(name,TCPNAMES[i]);
   strcat(name,"_in");
   sig = SetClasses(name,NETIN[i],av.expect_netin[i],av.var_netin[i],LOCALAV.expect_netin[i],LOCALAV.var_netin[i],&classlist,timekey);
   SetVariable(name,NETIN[i],av.expect_netin[i],sig,&classlist);

   strcpy(name,TCPNAMES[i]);
   strcat(name,"_out");
   sig = SetClasses(name,NETOUT[i],av.expect_netout[i],av.var_netout[i],LOCALAV.expect_netout[i],LOCALAV.var_netout[i],&classlist,timekey);
   SetVariable(name,NETOUT[i],av.expect_netout[i],sig,&classlist);
   }

unlink(ENV_NEW);
 
if ((fp = fopen(ENV_NEW,"a")) == NULL)
   {
   DeleteItemList(PREVIOUS_STATE);
   PREVIOUS_STATE = classlist;
   return; 
   }

for (ip = classlist; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s\n",ip->name);
   }
 
DeleteItemList(PREVIOUS_STATE);
PREVIOUS_STATE = classlist;

for (ip = ENTROPIES; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s\n",ip->name);
   }

DeleteItemList(ENTROPIES); 
fclose(fp);

rename(ENV_NEW,ENV);
}

/*********************************************************************/

void AnalyzeArrival(char *arrival)

{ char src[CF_BUFSIZE],dest[CF_BUFSIZE], flag = '.';
  
src[0] = dest[0] = '\0';
    
 
if (strstr(arrival,"listening"))
   {
   return;
   }
 
Chop(arrival);      

 /* Most hosts have only a few dominant services, so anomalies will
    show up even in the traffic without looking too closely. This
    will apply only to the main interface .. not multifaces */
 
if (strstr(arrival,"tcp") || strstr(arrival,"ack"))
   {              
   sscanf(arrival,"%s %*c %s %c ",src,dest,&flag);
   DePort(src);
   DePort(dest);
   Debug("FROM %s, TO %s, Flag(%c)\n",src,dest,flag);
    
    switch (flag)
       {
       case 'S': Debug("%1.1f: TCP new connection from %s to %d - i am %s\n",ITER,src,dest,VIPADDRESS);
    if (IsInterfaceAddress(dest))
       {
       NETIN[tcpsyn]++;
       IncrementCounter(&(NETIN_DIST[tcpsyn]),src);
       }
    else if (IsInterfaceAddress(src))
       {
       NETOUT[tcpsyn]++;
       IncrementCounter(&(NETOUT_DIST[tcpsyn]),dest);
       }       
    break;
    
       case 'F': Debug("%1.1f: TCP end connection from %s to %s\n",ITER,src,dest);
    if (IsInterfaceAddress(dest))
       {
       NETIN[tcpfin]++;
       IncrementCounter(&(NETIN_DIST[tcpfin]),src);
       }
    else if (IsInterfaceAddress(src))
       {
       NETOUT[tcpfin]++;
       IncrementCounter(&(NETOUT_DIST[tcpfin]),dest);
       }       
    break;
    
       default: Debug("%1.1f: TCP established from %s to %s\n",ITER,src,dest);
    
    if (IsInterfaceAddress(dest))
       {
       NETIN[tcpack]++;
       IncrementCounter(&(NETIN_DIST[tcpack]),src);
       }
    else if (IsInterfaceAddress(src))
       {
       NETOUT[tcpack]++;
       IncrementCounter(&(NETOUT_DIST[tcpack]),dest);
       }       
    break;
       }
    
    }
 else if (strstr(arrival,".53"))
    {
    sscanf(arrival,"%s %*c %s %c ",src,dest,&flag);
    DePort(src);
    DePort(dest);
    
    Debug("%1.1f: DNS packet from %s to %s\n",ITER,src,dest);
    if (IsInterfaceAddress(dest))
       {
       NETIN[dns]++;
       IncrementCounter(&(NETIN_DIST[dns]),src);
       }
    else if (IsInterfaceAddress(src))
       {
       NETOUT[dns]++;
       IncrementCounter(&(NETOUT_DIST[tcpack]),dest);
       }       
    }
 else if (strstr(arrival,"udp"))
    {
    sscanf(arrival,"%s %*c %s %c ",src,dest,&flag);
    DePort(src);
    DePort(dest);
    
    Debug("%1.1f: UDP packet from %s to %s\n",ITER,src,dest);
    if (IsInterfaceAddress(dest))
       {
       NETIN[udp]++;
       IncrementCounter(&(NETIN_DIST[udp]),src);
       }
    else if (IsInterfaceAddress(src))
       {
       NETOUT[udp]++;
       IncrementCounter(&(NETOUT_DIST[udp]),dest);
       }       
    }
 else if (strstr(arrival,"icmp"))
    {
    sscanf(arrival,"%s %*c %s %c ",src,dest,&flag);
    DePort(src);
    DePort(dest);
    
    Debug("%1.1f: ICMP packet from %s to %s\n",ITER,src,dest);
    
    if (IsInterfaceAddress(dest))
       {
       NETIN[icmp]++;
       IncrementCounter(&(NETIN_DIST[icmp]),src);
       }
    else if (IsInterfaceAddress(src))
       {
       NETOUT[icmp]++;
       IncrementCounter(&(NETOUT_DIST[icmp]),src);
       }       
    }
 else
    {
    Debug("%1.1f: Miscellaneous undirected packet (%.100s)\n",ITER,arrival);

    /* Here we don't know what source will be, but .... */

    sscanf(arrival,"%s",src);
    
    if (!isdigit((int)*src))
       {
       Debug("Assuming continuation line...\n");
       return;
       }

    DePort(src);
    
    NETIN[tcpmisc]++;

    if (strstr(arrival,".138"))
       {
       snprintf(dest,CF_BUFSIZE-1,"%s NETBIOS",src);
       }
    else if (strstr(arrival,".2049"))
       {
       snprintf(dest,CF_BUFSIZE-1,"%s NFS",src);
       }
    else
       {
       strncpy(dest,src,60);
       }
    
    IncrementCounter(&(NETIN_DIST[tcpmisc]),dest);
    }
}

/*********************************************************************/
/* Level 4                                                           */
/*********************************************************************/

void GatherProcessData()

{ FILE *pp;
  char pscomm[CF_BUFSIZE];
  char user[CF_MAXVARSIZE];
  struct Item *list = NULL;
  
snprintf(pscomm,CF_BUFSIZE,"%s %s",VPSCOMM[VSYSTEMHARDCLASS],VPSOPTS[VSYSTEMHARDCLASS]);

NUMBER_OF_USERS = ROOTPROCS = OTHERPROCS = 0; 

if ((pp = cfpopen(pscomm,"r")) == NULL)
   {
   return;
   }

ReadLine(VBUFF,CF_BUFSIZE,pp); 

while (!feof(pp))
   {
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   sscanf(VBUFF,"%s",user);
   if (!IsItemIn(list,user))
      {
      PrependItem(&list,user,NULL);
      NUMBER_OF_USERS++;
      }

   if (strcmp(user,"root") == 0)
      {
      ROOTPROCS++;
      }
   else
      {
      OTHERPROCS++;
      }

   }

cfpclose(pp);

snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_users",CFWORKDIR);
SaveItemList(list,VBUFF,"none");
 
Verbose("(Users,root,other) = (%d,%d,%d)\n",NUMBER_OF_USERS,ROOTPROCS,OTHERPROCS);
}

/*****************************************************************************/

void GatherDiskData()

{
DISKFREE = GetDiskUsage("/",cfpercent);
Verbose("Disk free = %d %%\n",DISKFREE);
}


/*****************************************************************************/

void GatherLoadData()

{ double load[4] = {0,0,0,0}, sum = 0.0; 
 int i,n = 1;

Debug("GatherLoadData\n\n");

#ifdef HAVE_GETLOADAVG 
if ((n = getloadavg(load,LOADAVG_5MIN)) == -1)
   {
   LOADAVG = 0.0;
   }
else
   {
   for (i = 0; i < n; i++)
      {
      Debug("Found load average to be %lf of %d samples\n", load[i],n);
      sum += load[i];
      }
   }
#endif

/* Scale load average by 100 to make it visible */
 
LOADAVG = (int) (100.0 * sum);
Verbose("100 x Load Average = %d\n",LOADAVG);
}

/*****************************************************************************/

void GatherSocketData()

{ FILE *pp;
  char local[CF_BUFSIZE],remote[CF_BUFSIZE],comm[CF_BUFSIZE];
  struct Item *in[ATTR],*out[ATTR];
  char *sp;
  int i;
  
Debug("GatherSocketData()\n");
  
for (i = 0; i < ATTR; i++)
   {
   INCOMING[i] = OUTGOING[i] = 0;
   in[i] = out[i] = NULL;
   }

if (ALL_INCOMING != NULL)
   {
   DeleteItemList(ALL_INCOMING);
   ALL_INCOMING = NULL;
   }

if (ALL_OUTGOING != NULL)
   {
   DeleteItemList(ALL_OUTGOING);
   ALL_OUTGOING = NULL;
   } 
 
sscanf(VNETSTAT[VSYSTEMHARDCLASS],"%s",comm);

strcat(comm," -n"); 
 
if ((pp = cfpopen(comm,"r")) == NULL)
   {
   return;
   }

while (!feof(pp))
   {
   memset(local,0,CF_BUFSIZE);
   memset(remote,0,CF_BUFSIZE);
   
   ReadLine(VBUFF,CF_BUFSIZE,pp);

   if (strstr(VBUFF,"UNIX"))
      {
      break;
      }

   if (!strstr(VBUFF,"."))
      {
      continue;
      }

   /* Different formats here ... ugh.. */

   if (strncmp(VBUFF,"tcp",3) == 0)
      {
      sscanf(VBUFF,"%*s %*s %*s %s %s",local,remote); /* linux-like */
      }
   else
      {
      sscanf(VBUFF,"%s %s",local,remote);             /* solaris-like */
      } 

   if (strlen(local) == 0)
      {
      continue;
      }
   
   for (sp = local+strlen(local); (*sp != '.') && (sp > local); sp--)
      {
      }

   sp++;
   
   if ((strlen(sp) < 5) &&!IsItemIn(ALL_INCOMING,sp))
      {
      PrependItem(&ALL_INCOMING,sp,NULL);
      }
   
   for (sp = remote+strlen(remote); !isdigit((int)*sp); sp--)
      {
      }

   sp++;

   if ((strlen(sp) < 5) && !IsItemIn(ALL_OUTGOING,sp))
      {
      PrependItem(&ALL_OUTGOING,sp,NULL);
      }

   for (i = 0; i < ATTR; i++)
      {
      char *spend;
      
      for (spend = local+strlen(local)-1; isdigit((int)*spend); spend--)
         {
         }
      
      spend++;
      
      if (strcmp(spend,ECGSOCKS[i][0]) == 0)
         {
         INCOMING[i]++;
         AppendItem(&in[i],VBUFF,"");
         }
      
      for (spend = remote+strlen(remote)-1; isdigit((int)*spend); spend--)
         {
         }
      
      spend++;
      
      Debug("Comparing (%s) with (%s) in %s\n",spend,ECGSOCKS[i][0],remote);
      
      if (strcmp(spend,ECGSOCKS[i][0]) == 0)
         {
         OUTGOING[i]++;
         AppendItem(&out[i],VBUFF,"");
         }
      }
   }
 
 cfpclose(pp);
 
/* Now save the state for ShowState() alert function IFF the state is not smaller than the last or
   at least 40 minutes older. This mirrors the persistence of the maxima classes */
 
 for (i = 0; i < ATTR; i++)
    {
    struct stat statbuf;
    time_t now = time(NULL);
    
    Debug("save incoming %s\n",ECGSOCKS[i][1]);
    snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_incoming.%s",CFWORKDIR,ECGSOCKS[i][1]);
    if (stat(VBUFF,&statbuf) != -1)
       {
       if ((ByteSizeList(in[i]) < statbuf.st_size) && (now < statbuf.st_mtime+40*60))
          {
          Verbose("New state %s is smaller, retaining old for 40 mins longer\n",ECGSOCKS[i][1]);
          DeleteItemList(in[i]);
          continue;
          }
       }
    
    SetEntropyClasses(ECGSOCKS[i][1],in[i],"in");
    SaveItemList(in[i],VBUFF,"none");
    DeleteItemList(in[i]);
    Debug("Saved in netstat data in %s\n",VBUFF); 
    }
 
 for (i = 0; i < ATTR; i++)
    {
    struct stat statbuf;
    time_t now = time(NULL); 

    Debug("save outgoing %s\n",ECGSOCKS[i][1]);
    snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_outgoing.%s",CFWORKDIR,ECGSOCKS[i][1]);

    if (stat(VBUFF,&statbuf) != -1)
       {       
       if ((ByteSizeList(out[i]) < statbuf.st_size) && (now < statbuf.st_mtime+40*60))
          {
          Verbose("New state %s is smaller, retaining old for 40 mins longer\n",ECGSOCKS[i][1]);
          DeleteItemList(out[i]);
          continue;
          }
       }
    
    SetEntropyClasses(ECGSOCKS[i][1],out[i],"out");
    SaveItemList(out[i],VBUFF,"none");
    Debug("Saved out netstat data in %s\n",VBUFF); 
    DeleteItemList(out[i]);
    }
 
 
 for (i = 0; i < CF_NETATTR; i++)
    {
    struct stat statbuf;
    time_t now = time(NULL); 
    
    Debug("save incoming %s\n",TCPNAMES[i]);
    snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_incoming.%s",CFWORKDIR,TCPNAMES[i]);
    
    if (stat(VBUFF,&statbuf) != -1)
       {       
       if ((ByteSizeList(NETIN_DIST[i]) < statbuf.st_size) && (now < statbuf.st_mtime+40*60))
          {
          Verbose("New state %s is smaller, retaining old for 40 mins longer\n",TCPNAMES[i]);
          DeleteItemList(NETIN_DIST[i]);
          NETIN_DIST[i] = NULL;
          continue;
          }
       }
    
    SaveTCPEntropyData(NETIN_DIST[i],i,"in");
    SetEntropyClasses(TCPNAMES[i],NETIN_DIST[i],"in");
    DeleteItemList(NETIN_DIST[i]);
    NETIN_DIST[i] = NULL;
    }


 for (i = 0; i < CF_NETATTR; i++)
    {
    struct stat statbuf;
    time_t now = time(NULL); 
 
    Debug("save outgoing %s\n",TCPNAMES[i]);
    snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_outgoing.%s",CFWORKDIR,TCPNAMES[i]);
    
    if (stat(VBUFF,&statbuf) != -1)
       {       
       if ((ByteSizeList(NETOUT_DIST[i]) < statbuf.st_size) && (now < statbuf.st_mtime+40*60))
          {
          Verbose("New state %s is smaller, retaining old for 40 mins longer\n",TCPNAMES[i]);
          DeleteItemList(NETOUT_DIST[i]);
          NETOUT_DIST[i] = NULL;   
          continue;
          }
       }
    
    SaveTCPEntropyData(NETOUT_DIST[i],i,"out");
    SetEntropyClasses(TCPNAMES[i],NETOUT_DIST[i],"out");
    DeleteItemList(NETOUT_DIST[i]);
    NETOUT_DIST[i] = NULL;
    }
}

/*****************************************************************************/

void GatherPhData()

{ DIR *dirh;
  struct dirent *dirp;
  struct stat statbuf;
  char file[64];
  char key[256];
  FILE *fp;
  int i,h,pid,value,profile;
  
if (stat("/proc",&statbuf) == -1)
   {
   Debug("No /proc data\n");
   return;
   }

Debug("Saving last Ph snapshot to compute delta...\n");
 
for (i = 0; i < PH_LIMIT; i++)
   {
   PH_LAST[i] = PH_SAMP[i];
   }
 
Debug("Looking for proc data\n"); 

if ((dirh = opendir("/proc")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory /proc\n");
   CfLog(cfverbose,OUTPUT,"opendir");
   return;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   pid = atoi(dirp->d_name);
   if (pid > 0)
      {
      Debug("Found pid %d\n",pid);
      }
   else
      {
      continue;
      }

   snprintf(file,63,"/proc/%s/pH",dirp->d_name);
   
   if ((fp = fopen(file,"r")) == NULL)
      {
      Debug("Cannot open file %s\n",file);
      closedir(dirh);
      return;
      }

   key[0] = '\0';
   value = 0;
   profile = false;
   
   while (!feof(fp))
      {
      fgets(VBUFF,64,fp);
      
      if (strncmp(VBUFF,"No profile",strlen("No profile")) == 0)
         {
         break;
         }
      
      if (strncmp(VBUFF,"profile-count",strlen("profile-count")) == 0)
         {
         char *sp;
         
         for (sp = VBUFF+strlen("profile-count"); !isdigit((int)*sp) ; sp++)
            {
            }
         value = atoi(sp);
         profile = true;
         continue;
         }
      
      if (strncmp(VBUFF,"profile",strlen("profile")) == 0)
         {
         char *sp;
         
         for (sp = VBUFF+strlen("profile"); (*sp == ':') && isspace((int)*sp) ; sp++)
            {
            }
         
         Chop(sp);
         strncpy(key,sp,255);
         continue;
         }
      
      }
   
   if (strlen(key) == 0)
      {
      continue;
      }
   
   h = HashPhKey(key);
   PH_SAMP[h] = value;
   
   if (PH_LAST[h] == 0)
      {
      PH_DELTA[h] = 0;
      }
   else
      {
      PH_DELTA[h] = PH_SAMP[h]-PH_LAST[h];
      }
   
   Debug("Profile [%s] with value %d and delta %d\n",key,value,PH_DELTA[h]);
   
   
   fclose(fp);
   }
 
 closedir(dirh);
}

/*****************************************************************************/

struct Averages *GetCurrentAverages(char *timekey)

{ int errno;
  DBT key,value;
  DB *dbp;
  static struct Averages entry;
 
if ((errno = db_create(&dbp,NULL,0)) != 0)
   {
   sprintf(OUTPUT,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return NULL;
   }

#ifdef CF_OLD_DB 
if ((errno = dbp->open(dbp,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   sprintf(OUTPUT,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return NULL;
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
memset(&entry,0,sizeof(entry));
      
key.data = timekey;
key.size = strlen(timekey)+1;

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp,errno,NULL);
      dbp->close(dbp,0);
      return NULL;
      }
   }

AGE++;
WAGE = AGE / CF_WEEK * CF_MEASURE_INTERVAL;

if (value.data != NULL)
   {
   memcpy(&entry,value.data,sizeof(entry));
   Debug("Previous values (%lf,..) for time index %s\n\n",entry.expect_number_of_users,timekey);
   dbp->close(dbp,0);
   return &entry;
   }
else
   {
   Debug("No previous value for time index %s\n",timekey);
   dbp->close(dbp,0);
   return &entry;
   }
}

/*****************************************************************************/

void UpdateAverages(char *timekey,struct Averages newvals)

{ int errno;
  DBT key,value;
  DB *dbp;
 
if ((errno = db_create(&dbp,NULL,0)) != 0)
   {
   sprintf(OUTPUT,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB 
if ((errno = dbp->open(dbp,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   sprintf(OUTPUT,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
      
key.data = timekey;
key.size = strlen(timekey)+1;

value.data = &newvals;
value.size = sizeof(newvals);
 
if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
   {
   dbp->err(dbp,errno,NULL);
   dbp->close(dbp,0);
   return;
   } 

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));

value.data = &AGE;
value.size = sizeof(double);    
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE")+1;

if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
   {
   dbp->err(dbp,errno,NULL);
   dbp->close(dbp,0);
   return;
   }
 
dbp->close(dbp,0);
}

/*****************************************************************************/

void UpdateDistributions(char *timekey,struct Averages *av)

{ int position; 
  int day,i,time_to_update = true;
 
/* Take an interval of 4 standard deviations from -2 to +2, divided into CF_GRAINS
   parts. Centre each measurement on CF_GRAINS/2 and scale each measurement by the
   std-deviation for the current time.
 */
if (HISTO)
   {
   time_to_update = (int) (3600.0*rand()/(RAND_MAX+1.0)) > 2400;
   
   day = Day2Number(timekey);
   
   position = CF_GRAINS/2 + (int)(0.5+(NUMBER_OF_USERS - av->expect_number_of_users)*CF_GRAINS/(4*sqrt((av->var_number_of_users))));
   if (0 <= position && position < CF_GRAINS)
      {
      HISTOGRAM[0][day][position]++;
      }
   
   position = CF_GRAINS/2 + (int)(0.5+(ROOTPROCS - av->expect_rootprocs)*CF_GRAINS/(4*sqrt((av->var_rootprocs))));
   if (0 <= position && position < CF_GRAINS)
      {
      HISTOGRAM[1][day][position]++;
      }
   
   position = CF_GRAINS/2 + (int)(0.5+(OTHERPROCS - av->expect_otherprocs)*CF_GRAINS/(4*sqrt((av->var_otherprocs))));
   if (0 <= position && position < CF_GRAINS)
      {
      HISTOGRAM[2][day][position]++;
      }
   
   position = CF_GRAINS/2 + (int)(0.5+(DISKFREE - av->expect_diskfree)*CF_GRAINS/(4*sqrt((av->var_diskfree))));
   if (0 <= position && position < CF_GRAINS)
      {
      HISTOGRAM[3][day][position]++;
      }

   position = CF_GRAINS/2 + (int)(0.5+(LOADAVG - av->expect_loadavg)*CF_GRAINS/(4*sqrt((av->var_loadavg))));
   if (0 <= position && position < CF_GRAINS)
      {
      HISTOGRAM[4][day][position]++;
      }

   for (i = 0; i < ATTR; i++)
      {
      position = CF_GRAINS/2 + (int)(0.5+(INCOMING[i] - av->expect_incoming[i])*CF_GRAINS/(4*sqrt((av->var_incoming[i]))));
      if (0 <= position && position < CF_GRAINS)
         {
         HISTOGRAM[5+i][day][position]++;
         }
      
      position = CF_GRAINS/2 + (int)(0.5+(OUTGOING[i] - av->expect_outgoing[i])*CF_GRAINS/(4*sqrt((av->var_outgoing[i]))));
      if (0 <= position && position < CF_GRAINS)
         {
         HISTOGRAM[5+ATTR+i][day][position]++;
         }
      }

   for (i = 0; i < CF_NETATTR; i++)
      {
      position = CF_GRAINS/2 + (int)(0.5+(INCOMING[i] - av->expect_incoming[i])*CF_GRAINS/(4*sqrt((av->var_incoming[i]))));
      if (0 <= position && position < CF_GRAINS)
         {
         HISTOGRAM[5+i+2*ATTR][day][position]++;
         }
      
      position = CF_GRAINS/2 + (int)(0.5+(OUTGOING[i] - av->expect_outgoing[i])*CF_GRAINS/(4*sqrt((av->var_outgoing[i]))));
      if (0 <= position && position < CF_GRAINS)
         {
         HISTOGRAM[5+2*ATTR+CF_NETATTR+i][day][position]++;
         }
      }

   /* PUT TCP STUFF HERE... */
   
   for (i = 0; i < PH_LIMIT; i++)
      {
      if (PH_BINARIES[i] == NULL)
         {
         continue;
         }
      
      position = CF_GRAINS/2 + (int)(0.5+(PH_DELTA[i] - av->expect_pH[i])*CF_GRAINS/(4*sqrt((av->var_pH[i]))));
      if (0 <= position && position < CF_GRAINS)
         {
         HISTOGRAM[5+2*CF_NETATTR+2*2*ATTR+i][day][position]++;
         }
      }
   
   
   if (time_to_update)
      {
      FILE *fp;
      char filename[CF_BUFSIZE];
      
      snprintf(filename,CF_BUFSIZE,"%s/state/histograms",CFWORKDIR);
      
      if ((fp = fopen(filename,"w")) == NULL)
         {
         CfLog(cferror,"Unable to save histograms","fopen");
         return;
         }
      
      for (position = 0; position < CF_GRAINS; position++)
         {
         fprintf(fp,"%u ",position);
         
         for (i = 0; i < 5 + 2*CF_NETATTR+2*ATTR+PH_LIMIT; i++)
            {
            for (day = 0; day < 7; day++)
               {
               fprintf(fp,"%u ",HISTOGRAM[i][day][position]);
               }
            }
         fprintf(fp,"\n");
         }
      
      fclose(fp);
      }
   }
}

/*****************************************************************************/

double WAverage(double anew,double aold,double age)

 /* For a couple of weeks, learn eagerly. Otherwise variances will
    be way too large. Then downplay newer data somewhat, and rely on
    experience of a couple of months of data ... */

{ double av;
  double wnew,wold;

if (age < 2.0)  
   {
   wnew = 0.7;
   wold = 0.3;
   }
else
   {
   wnew = 0.3;
   wold = 0.7;
   }

av = (wnew*anew + wold*aold)/(wnew+wold); 

if (av > big_number) /* some kind of bug? */
   {
   return 10.0;
   }
 
return av;
}

/*****************************************************************************/

double SetClasses(char * name,double variable,double av_expect,double av_var,double localav_expect,double localav_var,struct Item **classlist,char *timekey)

{ char buffer[CF_BUFSIZE],buffer2[CF_BUFSIZE];
  double dev,delta,sigma,ldelta,lsigma,sig;

Debug("\n SetClasses(%s,X=%f,avX=%f,varX=%f,lavX=%f,lvarX=%f,%s)\n",name,variable,av_expect,av_var,localav_expect,localav_var,timekey);

delta = variable - av_expect;
sigma = sqrt(av_var);
ldelta = variable - localav_expect;
lsigma = sqrt(localav_var);
sig = sqrt(sigma*sigma+lsigma*lsigma); 

Debug(" delta = %f,sigma = %f, lsigma = %f, sig = %f\n",delta,sigma,lsigma,sig);
 
if (sigma == 0.0 || lsigma == 0.0)
   {
   Debug(" No sigma variation .. can't measure class\n");
   return sig;
   }

Debug("Setting classes for %s...\n",name);
 
if (fabs(delta) < cf_noise_threshold) /* Arbitrary limits on sensitivity  */
   {
   Debug(" Sensitivity too high ..\n");

   buffer[0] = '\0';
   strcpy(buffer,name);

   if ((delta > 0) && (ldelta > 0))
      {
      strcat(buffer,"_high");
      }
   else if ((delta < 0) && (ldelta < 0))
      {
      strcat(buffer,"_low");
      }
   else
      {
      strcat(buffer,"_normal");
      }
        
    dev = sqrt(delta*delta/(1.0+sigma*sigma)+ldelta*ldelta/(1.0+lsigma*lsigma));
        
    if (dev > 2.0*sqrt(2.0))
       {
       strcpy(buffer2,buffer);
       strcat(buffer2,"_microanomaly");
       AppendItem(classlist,buffer2,"2");
       AddPersistentClass(buffer2,40,cfpreserve); 
       }
   
   return sig; /* Granularity makes this silly */
   }
 else
    {
    buffer[0] = '\0';
    strcpy(buffer,name);  
    
    if ((delta > 0) && (ldelta > 0))
       {
       strcat(buffer,"_high");
       }
    else if ((delta < 0) && (ldelta < 0))
       {
       strcat(buffer,"_low");
       }
    else
       {
       strcat(buffer,"_normal");
       }
    
    dev = sqrt(delta*delta/(1.0+sigma*sigma)+ldelta*ldelta/(1.0+lsigma*lsigma));
    
    if (dev <= sqrt(2.0))
       {
       strcpy(buffer2,buffer);
       strcat(buffer2,"_normal");
       AppendItem(classlist,buffer2,"0");
       }
    else
       {
       strcpy(buffer2,buffer);
       strcat(buffer2,"_dev1");
       AppendItem(classlist,buffer2,"0");
       }
    
    /* Now use persistent classes so that serious anomalies last for about
       2 autocorrelation lengths, so that they can be cross correlated and
       seen by normally scheduled cfagent processes ... */
    
    if (dev > 2.0*sqrt(2.0))
       {
       strcpy(buffer2,buffer);
       strcat(buffer2,"_dev2");
       AppendItem(classlist,buffer2,"2");
       AddPersistentClass(buffer2,40,cfpreserve); 
       }
    
    if (dev > 3.0*sqrt(2.0))
       {
       strcpy(buffer2,buffer);
       strcat(buffer2,"_anomaly");
       AppendItem(classlist,buffer2,"3");
       AddPersistentClass(buffer2,40,cfpreserve); 
       }
    return sig; 
    }
}

/*****************************************************************************/

void SetVariable(char *name,double value,double average,double stddev,struct Item **classlist)


{ char var[CF_BUFSIZE];

sprintf(var,"value_%s=%d",name,(int)value);
AppendItem(classlist,var,"");

sprintf(var,"average_%s=%1.1f",name,average);
AppendItem(classlist,var,"");

sprintf(var,"stddev_%s=%1.1f",name,stddev);
AppendItem(classlist,var,""); 
}

/*****************************************************************************/

void RecordChangeOfState(struct Item *classlist,char *timekey)

{
}

/*****************************************************************************/

void ZeroArrivals()

{ int i;
 
for (i = 0; i < CF_NETATTR; i++)
   {
   NETIN[i] = 0;
   NETOUT[i] = 0;
   }
}

/*****************************************************************************/

double RejectAnomaly(double new,double average,double variance,double localav,double localvar)

{ double dev = sqrt(variance+localvar);          /* Geometrical average dev */
  double delta;
  int bigger;

if (average == 0)
   {
   return new;
   }

if (new > big_number)
   {
   return average;
   }

if ((new - average)*(new-average) < cf_noise_threshold*cf_noise_threshold)
   {
   return new;
   }

if (new - average > 0)
   {
   bigger = true;
   }
else
   {
   bigger = false;
   }

/* This routine puts some inertia into the changes, so that the system
   doesn't respond to every little change ...   IR and UV cutoff */
 
delta = sqrt((new-average)*(new-average)+(new-localav)*(new-localav));

if (delta > 4.0*dev)  /* IR */
   {
   srand48((unsigned int)time(NULL)); 
   
   if (drand48() < 0.7) /* 70% chance of using full value - as in learning policy */
      {
      return new;
      }
   else
      {
      if (bigger)
         {
         return average+2.0*dev;
         }
      else
         {
         return average-2.0*dev;
         }
      }
   }
else
   {
   return new;
   }
}

/***************************************************************/

int HashPhKey(char *key)

{ int hash;

/* Don't really know how to do this for the best yet,
   so just use a list of likely names as long as this
   is experimental .. */

for (hash = 0; hash < PH_LIMIT; hash++)
   {
   if (strstr(key,PH_BINARIES[hash]) == 0)
      {
      return hash;
      }
   }

return hash;
}

/***************************************************************/
/* Level 5                                                     */
/***************************************************************/

void SetEntropyClasses(char *service,struct Item *list,char *inout)

{ struct Item *ip, *addresses = NULL;
  char local[CF_BUFSIZE],remote[CF_BUFSIZE],class[CF_BUFSIZE],vbuff[CF_BUFSIZE], *sp;
  int i = 0, min_signal_diversity = 1,total=0;
  double *p = NULL, S = 0.0, percent = 0.0;


if (IsSocketType(service))
   {
   for (ip = list; ip != NULL; ip=ip->next)
      {   
      if (strlen(ip->name) > 0)
         {
         if (strncmp(ip->name,"tcp",3) == 0)
            {
            sscanf(ip->name,"%*s %*s %*s %s %s",local,remote); /* linux-like */
            }
         else
            {
            sscanf(ip->name,"%s %s",local,remote);             /* solaris-like */
            }
         
         strncpy(vbuff,remote,CF_BUFSIZE-1);
         
         for (sp = vbuff+strlen(vbuff)-1; isdigit((int)*sp); sp--)
            {     
            }
         
         *sp = '\0';
         
         if (!IsItemIn(addresses,vbuff))
            {
            total++;
            AppendItem(&addresses,vbuff,"");
            IncrementItemListCounter(addresses,vbuff);
            }
         else
            {
            total++;    
            IncrementItemListCounter(addresses,vbuff);
            }      
         }
      }
   
   p = (double *) malloc((total+1)*sizeof(double));
   
   if (total > min_signal_diversity)
      {
      for (i = 0,ip = addresses; ip != NULL; i++,ip=ip->next)
         {
         p[i] = ((double)(ip->counter))/((double)total);
         
         S -= p[i]*log(p[i]);
         }
      
      percent = S/log((double)total)*100.0;
      free(p);       
      }
   }
 else
    {
    int classes = 0;

    total = 0;
    
    for (ip = list; ip != NULL; ip=ip->next)
       {
       total += (double)(ip->counter);
       }
 
    p = (double *)malloc(sizeof(double)*total); 

    for (ip = list; ip != NULL; ip=ip->next)
       {
       p[classes++] = ip->counter/total;
       }
    
    for (i = 0; i < classes; i++)
       {
       S -= p[i] * log(p[i]);
       }
    
    if (classes > 1)
       {
       percent = S/log((double)classes)*100.0;
       }
    else
       {    
       percent = 0;
       }

    free(p);
    }
 
if (percent > 90)
   {
   snprintf(class,CF_MAXVARSIZE,"entropy_%s_%s_high",service,inout);
   AppendItem(&ENTROPIES,class,"");
   }
else if (percent < 20)
   {
   snprintf(class,CF_MAXVARSIZE,"entropy_%s_%s_low",service,inout);
   AppendItem(&ENTROPIES,class,"");
   }
else
   {
   snprintf(class,CF_MAXVARSIZE,"entropy_%s_%s_medium",service,inout);
   AppendItem(&ENTROPIES,class,"");
   }
 
DeleteItemList(addresses);
}

/***************************************************************/

void SaveTCPEntropyData(struct Item *list,int i,char *inout)

{ struct Item *ip;
  FILE *fp;
  char filename[CF_BUFSIZE];

Verbose("TCP Save %s\n",TCPNAMES[i]);
  
if (list == NULL)
   {
   Verbose("No %s-%s events\n",TCPNAMES[i],inout);
   return;
   }

 if (strncmp(inout,"in",2) == 0)
    {
    snprintf(filename,CF_BUFSIZE-1,"%s/state/cf_incoming.%s",CFWORKDIR,TCPNAMES[i]); 
    }
 else
    {
    snprintf(filename,CF_BUFSIZE-1,"%s/state/cf_outgoing.%s",CFWORKDIR,TCPNAMES[i]); 
    }

Verbose("TCP Save %s\n",filename);
 
if ((fp = fopen(filename,"w")) == NULL)
   {
   Verbose("Unable to write datafile %s\n",filename);
   return;
   }
 
for (ip = list; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%d %s\n",ip->counter,ip->name);
   }
 
fclose(fp);
}


/***************************************************************/

void IncrementCounter(struct Item **list,char *name)

{
if (!IsItemIn(*list,name))
   {
   AppendItem(list,name,"");
   IncrementItemListCounter(*list,name);
   }
else
   {
   IncrementItemListCounter(*list,name);
   } 
}

/***************************************************************/
/* Linking simplification                                      */
/***************************************************************/

int RecursiveTidySpecialArea(char *name,struct Tidy *tp,int maxrecurse,struct stat *sb)

{
return true;
}

/***************************************************************/

int Repository(char *file,char *repository)

{
 return false;
}


char *GetMacroValue(char *s,char *sp)

{
 return NULL;
}

void Banner(char *s)

{
}


void AddMacroValue(char *scope,char *name,char *value)

{
}
