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
/* Based on the results of the ECG project, by Mark, Sigmund Straumsnes      */
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
#include <math.h>
#include <db.h>

/*****************************************************************************/
/* Globals                                                                   */
/*****************************************************************************/

#define CFGRACEPERIOD 4.0   /* training period in units of counters (weeks,iterations)*/

unsigned int HISTOGRAM[ATTR*2+5+PH_LIMIT][7][GRAINS];

int HISTO = false;
int NUMBER_OF_USERS;
int ROOTPROCS;
int OTHERPROCS;
int DISKFREE;
int LOADAVG;
int INCOMING[ATTR];
int OUTGOING[ATTR];
int PH_SAMP[PH_LIMIT];
int PH_LAST[PH_LIMIT];
int PH_DELTA[PH_LIMIT];
int SLEEPTIME = 5 * 60;
int BATCH_MODE = false;
double ITER = 0.0;        /* Iteration since start */
double AGE,WAGE;          /* Age and weekly age of database */

char OUTPUT[bufsize*2];

char BATCHFILE[bufsize];
char STATELOG[bufsize];
char ENV_NEW[bufsize];
char ENV[bufsize];

struct Averages LOCALAV;
struct Item *ALL_INCOMING = NULL;
struct Item *ALL_OUTGOING = NULL;

double ENTROPY = 0.0;
double LAST_HOUR_ENTROPY = 0.0;
double LAST_DAY_ENTROPY = 0.0;

struct Item *PREVIOUS_STATE = NULL;

struct option CFDENVOPTIONS[] =
   {
   {"help",no_argument,0,'h'},
   {"debug",optional_argument,0,'d'}, 
   {"verbose",no_argument,0,'v'},
   {"no-fork",no_argument,0,'F'},
   {"histograms",no_argument,0,'H'},
   {"file",optional_argument,0,'f'},
   {NULL,0,0,0}
   };

short NO_FORK = false;

/*****************************************************************************/

enum socks
   {
   netbiosns,
   netbiosdgm,
   netbiosssn,
   irc,
   cfengine,
   nfsd,
   smtp,
   www,
   ftp,
   ssh,
   telnet
   };

char *ECGSOCKS[ATTR][2] =
   {
   {"137","netbiosns"},
   {"138","netbiosdgm"},
   {"139","netbiosssn"},
   {"194","irc"},
   {"5308","cfengine"},
   {"2049","nfsd"},
   {"25","smtp"},
   {"80","www"},
   {"21","ftp"},
   {"22","ssh"},
   {"23","telnet"},
   };

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
void DoBatch ARGLIST((void));
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
void SetClasses ARGLIST((double expect,double delta,double sigma,double lexpect,double ldelta,double lsigma,char *name,struct Item **list,char *timekey));
void SetVariable ARGLIST((char *name,double now, double average, double stddev, struct Item **list));
void RecordChangeOfState  ARGLIST((struct Item *list,char *timekey));
double RejectAnomaly ARGLIST((double new,double av,double var,double av2,double var2));
int HashPhKey ARGLIST((char *s));

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (argc,argv)

int argc;
char **argv;

{
CheckOptsAndInit(argc,argv);
GetNameInfo();

if (BATCH_MODE)
   {
   DoBatch();
   }
else
   {
   StartServer(argc,argv);
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
 int c, i,j,k;

umask(077);
sprintf(VPREFIX,"cfenvd"); 
openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);

strcpy(CFLOCK,"cfenvd");
 
IGNORELOCK = false; 
OUTPUT[0] = '\0';

while ((c=getopt_long(argc,argv,"d:f:vhHFV",CFDENVOPTIONS,&optindex)) != EOF)
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

      case 'f': /* This is for us Oslo folks to test against old data in batch */

	        strcpy(BATCHFILE,optarg);
	        NO_FORK = true;
		BATCH_MODE = true;
		printf("Working in batch mode on file %s\n",BATCHFILE);
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
		
      default:  Syntax();
                exit(1);

      }
   }

LOGGING = true;                    /* Do output to syslog */
 
sprintf(VBUFF,"%s/test",WORKDIR);
MakeDirectoriesFor(VBUFF,'y');
sprintf(VBUFF,"%s/state/test",WORKDIR);
MakeDirectoriesFor(VBUFF,'y');
strncpy(VLOCKDIR,WORKDIR,bufsize-1);
strncpy(VLOGDIR,WORKDIR,bufsize-1);

for (i = 0; i < ATTR; i++)
   {
   sprintf(VBUFF,"%s/state/cf_incoming.%s",WORKDIR,ECGSOCKS[i][1]);
   CreateEmptyFile(VBUFF);
   sprintf(VBUFF,"%s/state/cf_outgoing.%s",WORKDIR,ECGSOCKS[i][1]);
   CreateEmptyFile(VBUFF);
   }

sprintf(VBUFF,"%s/state/cf_users",WORKDIR);
CreateEmptyFile(VBUFF);
 
snprintf(AVDB,bufsize,"%s/state/%s",WORKDIR,AVDB_FILE);
snprintf(STATELOG,bufsize,"%s/state/%s",WORKDIR,STATELOG_FILE);
snprintf(ENV_NEW,bufsize,"%s/state/%s",WORKDIR,ENV_NEW_FILE);
snprintf(ENV,bufsize,"%s/state/%s",WORKDIR,ENV_FILE);

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

   for (i = 0; i < PH_LIMIT; i++)
      {
      LOCALAV.expect_pH[i] = 0.0;
      LOCALAV.var_pH[i] = 0.0;
      }

   }

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < ATTR*2+5+PH_LIMIT; j++)
      {
      for (k = 0; k < GRAINS; k++)
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
   snprintf(OUTPUT,bufsize,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,AVDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   snprintf(OUTPUT,bufsize,"Couldn't open average database %s\n",AVDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

chmod(AVDB,0644); 

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));
      
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
   WAGE = AGE / CFWEEK * MEASURE_INTERVAL;
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
   char filename[bufsize];
   
   snprintf(filename,bufsize,"%s/state/histograms",WORKDIR);
   
   if ((fp = fopen(filename,"r")) == NULL)
      {
      CfLog(cfverbose,"Unable to load histogram data","fopen");
      return;
      }

   for (position = 0; position < GRAINS; position++)
      {
      fscanf(fp,"%d ",&position);
      
      for (i = 0; i < 5 + 2*ATTR+PH_LIMIT; i++)
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

void DoBatch()

{ FILE *fp;
  char buffer[4096],time[256],timekey[256];
  float v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25,v26;
  DB *dbp;
  int i = 0;

sprintf(AVDB,"/tmp/cfenv.tmp.db");
unlink(AVDB);
 
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
  
if ((fp=fopen(BATCHFILE,"r")) == NULL)
   {
   printf("Cannot open %s\n",BATCHFILE);
   return;
   }

while (!feof(fp))
   {
   bzero(buffer,4096);
   fgets(buffer,1024,fp);
   if (i++ % 1024 == 0)
      {
      printf("   * Working %d ... *\r",i);
      }
   
   sscanf(buffer,"%[^,],%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f",time,&v1,&v2,&v3,&v4,&v5,&v6,&v7,&v8,&v9,&v10,&v11,&v12,&v13,&v14,&v15,&v16,&v17,&v18,&v19,&v20,&v21,&v22,&v23,&v24,&v25,&v26);

   Debug("%s,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",time,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25,v26);

   NUMBER_OF_USERS = (double)v1;
   ROOTPROCS = (double)v4;
   OTHERPROCS = (double)v5;
   DISKFREE = (double)v6;
   INCOMING[cfengine] = (double)v13;
   OUTGOING[cfengine] = (double)v14;
   INCOMING[nfsd] = (double)v15;
   OUTGOING[nfsd] = (double)v16;
   INCOMING[smtp] = (double)v17;
   OUTGOING[smtp] = (double)v18;
   INCOMING[www] = (double)v19;
   OUTGOING[www] = (double)v20;
   INCOMING[ftp] = (double)v21;
   OUTGOING[ftp] = (double)v22;
   INCOMING[ssh] = (double)v23;
   OUTGOING[ssh] = (double)v24;
   INCOMING[telnet] = (double)v25;
   OUTGOING[telnet] = (double)v26;

   strcpy(timekey,ConvTimeKey(time));

   (void)EvalAvQ(timekey);
   }

fclose(fp);
dbp->close(dbp,0);
printf("\nDone - database saved to %s\n",AVDB);
printf("Run cfenvgraph -f %s to generate graphs\n\n",AVDB); 
}

/*********************************************************************/

void StartServer(argc,argv)

int argc;
char **argv;

{ char *timekey;
  struct Averages averages;
  void HandleSignal();

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
    
while (true)
   {
   GetQ();
   timekey = GetTimeKey();
   averages = EvalAvQ(timekey);
   ArmClasses(averages,timekey);
   sleep(SLEEPTIME);
   ITER++;
   }
}

/*********************************************************************/

void yyerror(s)

char *s;

{
 /* Dummy */
}

/*********************************************************************/

void RotateFiles(name,number)

char *name;
int number;

{
 /* Dummy */
}

/*********************************************************************/

void FatalError(s)

char *s;

{
fprintf (stderr,"%s:%s:%s\n",VPREFIX,VCURRENTFILE,s);
closelog(); 
exit(1);
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void GetQ()

{
Debug("========================= GET Q ==============================\n");
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

struct Averages EvalAvQ(t)

char *t;

{ struct Averages *currentvals,newvals;
  int i; 
  double Number_Of_Users,Rootprocs,Otherprocs,Diskfree,LoadAvg;
  double Incoming[ATTR],Outgoing[ATTR],pH_delta[PH_LIMIT];

if ((currentvals = GetCurrentAverages(t)) == NULL)
   {
   CfLog(cferror,"Error reading average database","");
   exit(1);
   }

/* Discard any apparently anomalous behaviour before renormalizing database */

Number_Of_Users = RejectAnomaly(NUMBER_OF_USERS,
                                currentvals->expect_number_of_users,
                                currentvals->var_number_of_users,
                                LOCALAV.expect_number_of_users,
                                LOCALAV.var_number_of_users);
Rootprocs = RejectAnomaly(ROOTPROCS,
                          currentvals->expect_rootprocs,
                          currentvals->var_rootprocs,
                          LOCALAV.expect_rootprocs,
                          LOCALAV.var_rootprocs);

Otherprocs = RejectAnomaly(OTHERPROCS,
                           currentvals->expect_otherprocs,
                           currentvals->var_otherprocs,
                           LOCALAV.expect_otherprocs,
                           LOCALAV.var_otherprocs);

Diskfree = RejectAnomaly(DISKFREE,
                         currentvals->expect_diskfree,
                         currentvals->var_diskfree, 
                         LOCALAV.expect_diskfree,
                         LOCALAV.var_diskfree);

LoadAvg = RejectAnomaly(LOADAVG,
                         currentvals->expect_loadavg,
                         currentvals->var_loadavg, 
                         LOCALAV.expect_loadavg,
                         LOCALAV.var_loadavg);

 
for (i = 0; i < ATTR; i++)
   {
   Incoming[i] = RejectAnomaly(INCOMING[i],
                               currentvals->expect_incoming[i],
                               currentvals->var_incoming[i],
                               LOCALAV.expect_incoming[i],
                               LOCALAV.var_incoming[i]);

   Outgoing[i] = RejectAnomaly(OUTGOING[i],
                               currentvals->expect_outgoing[i],
                               currentvals->var_outgoing[i],
                               LOCALAV.expect_outgoing[i],
                               LOCALAV.var_outgoing[i]);
   }


for (i = 0; i < PH_LIMIT; i++)
   {
   pH_delta[i] = RejectAnomaly(PH_DELTA[i],
                               currentvals->expect_pH[i],
                               currentvals->var_pH[i],
                               LOCALAV.expect_pH[i],
                               LOCALAV.var_pH[i]);
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
 
Verbose("Users              = %4d -> (%f#%f) local [%f#%f]\n",NUMBER_OF_USERS,newvals.expect_number_of_users,sqrt(newvals.var_number_of_users),LOCALAV.expect_number_of_users,sqrt(LOCALAV.var_number_of_users));
Verbose("Rootproc           = %4d -> (%f#%f) local [%f#%f]\n",ROOTPROCS,newvals.expect_rootprocs,sqrt(newvals.var_rootprocs),LOCALAV.expect_rootprocs,sqrt(LOCALAV.var_rootprocs));
Verbose("Otherproc          = %4d -> (%f#%f) local [%f#%f]\n",OTHERPROCS,newvals.expect_otherprocs,sqrt(newvals.var_otherprocs),LOCALAV.expect_otherprocs,sqrt(LOCALAV.var_otherprocs));
Verbose("Diskpercent        = %4d -> (%f#%f) local [%f#%f]\n",DISKFREE,newvals.expect_diskfree,sqrt(newvals.var_diskfree),LOCALAV.expect_diskfree,sqrt(LOCALAV.var_diskfree));
Verbose("Load Average       = %4d -> (%f#%f) local [%f#%f]\n",LOADAVG,newvals.expect_loadavg,sqrt(newvals.var_loadavg),LOCALAV.expect_loadavg,sqrt(LOCALAV.var_loadavg)); 
 
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
   
   Verbose("%-15s-in = %4d -> (%f#%f) local [%f#%f]\n",ECGSOCKS[i][1],INCOMING[i],newvals.expect_incoming[i],sqrt(newvals.var_incoming[i]),LOCALAV.expect_incoming[i],sqrt(LOCALAV.var_incoming[i]));
   Verbose("%-14s-out = %4d -> (%f#%f) local [%f#%f]\n",ECGSOCKS[i][1],OUTGOING[i],newvals.expect_outgoing[i],sqrt(newvals.var_outgoing[i]),LOCALAV.expect_outgoing[i],sqrt(LOCALAV.var_outgoing[i]));
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

   Verbose("%-15s-in = %4d -> (%f#%f) local [%f#%f]\n",CanonifyName(PH_BINARIES[i]),PH_DELTA[i],newvals.expect_pH[i],sqrt(newvals.var_pH[i]),LOCALAV.expect_pH[i],sqrt(LOCALAV.var_pH[i]));

   }
 
UpdateAverages(t,newvals);
 
if (WAGE > CFGRACEPERIOD)
   {
   UpdateDistributions(t,currentvals);  /* Distribution about mean */
   }
 
return newvals;
}

/*********************************************************************/

void ArmClasses(av,timekey)

struct Averages av;
char *timekey;

{ double sigma,delta,lsigma,ldelta;
  struct Item *classlist = NULL, *ip;
  int i;
  FILE *fp;
 
delta = NUMBER_OF_USERS - av.expect_number_of_users;
sigma = sqrt(av.var_number_of_users);
ldelta = NUMBER_OF_USERS - LOCALAV.expect_number_of_users;
lsigma = sqrt(LOCALAV.var_number_of_users);
 
SetClasses(av.expect_number_of_users,delta,sigma,
	   LOCALAV.expect_number_of_users,ldelta,lsigma,
	   "Users",&classlist,timekey);

SetVariable("users",NUMBER_OF_USERS,av.expect_number_of_users,lsigma,&classlist);

delta = ROOTPROCS - av.expect_rootprocs;
sigma = sqrt(av.var_rootprocs);
ldelta = ROOTPROCS - LOCALAV.expect_rootprocs;
lsigma = sqrt(LOCALAV.var_rootprocs);

SetClasses(av.expect_rootprocs,delta,sigma,
	   LOCALAV.expect_rootprocs,ldelta,lsigma,
	   "RootProcs",&classlist,timekey);

SetVariable("rootprocs",ROOTPROCS,av.expect_rootprocs,lsigma,&classlist);

delta = OTHERPROCS - av.expect_otherprocs;
sigma = sqrt(av.var_otherprocs);
ldelta = OTHERPROCS - LOCALAV.expect_otherprocs;
lsigma = sqrt(LOCALAV.var_otherprocs);

SetClasses(av.expect_otherprocs,delta,sigma,
	   LOCALAV.expect_otherprocs,ldelta,lsigma,
	   "UserProcs",&classlist,timekey);


SetVariable("userprocs",OTHERPROCS,av.expect_otherprocs,lsigma,&classlist);

delta = DISKFREE - av.expect_diskfree;
sigma = sqrt(av.var_diskfree);
ldelta = DISKFREE - LOCALAV.expect_diskfree;
lsigma = sqrt(LOCALAV.var_diskfree);
 
SetClasses(av.expect_diskfree,delta,sigma,
	   LOCALAV.expect_diskfree,ldelta,lsigma,
	   "DiskFree",&classlist,timekey);

SetVariable("diskfree",DISKFREE,av.expect_diskfree,lsigma,&classlist);

SetClasses(av.expect_loadavg,delta,sigma,
	   LOCALAV.expect_loadavg,ldelta,lsigma,
	   "LoadAvg",&classlist,timekey);

SetVariable("loadavg",LOADAVG,av.expect_loadavg,lsigma,&classlist);
 
for (i = 0; i < ATTR; i++)
   {
   char name[256];
   strcpy(name,ECGSOCKS[i][1]);
   strcat(name,"_in");
   delta = INCOMING[i] - av.expect_incoming[i];
   sigma = sqrt(av.var_incoming[i]);
   ldelta = INCOMING[i] - LOCALAV.expect_incoming[i];
   lsigma = sqrt(LOCALAV.var_incoming[i]);
      
   SetClasses(av.expect_incoming[i],delta,sigma,
	      LOCALAV.expect_incoming[i],ldelta,lsigma,
	      name,&classlist,timekey);

   SetVariable(name,INCOMING[i],av.expect_incoming[i],lsigma,&classlist);

   strcpy(name,ECGSOCKS[i][1]);
   strcat(name,"_out");
   delta = OUTGOING[i] - av.expect_outgoing[i];
   sigma = sqrt(av.var_outgoing[i]);
   ldelta = OUTGOING[i] - LOCALAV.expect_outgoing[i];
   lsigma = sqrt(LOCALAV.var_outgoing[i]);
   
   SetClasses(av.expect_outgoing[i],delta,sigma,
	      LOCALAV.expect_outgoing[i],ldelta,lsigma,
	      name,&classlist,timekey);

   SetVariable(name,OUTGOING[i],av.expect_outgoing[i],lsigma,&classlist);
   }

for (i = 0; i < PH_LIMIT; i++)
   {
   if (PH_BINARIES[i] == NULL)
      {
      continue;
      }
   
   delta = PH_DELTA[i] - av.expect_pH[i];
   sigma = sqrt(av.var_pH[i]);
   ldelta = PH_DELTA[i] - LOCALAV.expect_pH[i];
   lsigma = sqrt(LOCALAV.var_pH[i]);
      
   SetClasses(av.expect_pH[i],delta,sigma,
	      LOCALAV.expect_pH[i],ldelta,lsigma,
	      CanonifyName(PH_BINARIES[i]),&classlist,timekey);

   SetVariable(CanonifyName(PH_BINARIES[i]),PH_DELTA[i],av.expect_pH[i],lsigma,&classlist);
   }

 
/*
if (WAGE > CFGRACEPERIOD)
   {
   if (!OrderedListsMatch(PREVIOUS_STATE,classlist))
      {
      RecordChangeOfState(classlist,timekey);
      }
   }
*/
   
unlink(ENV_NEW);
 
if ((fp = fopen(ENV_NEW,"w")) == NULL)
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
 
for (ip = ALL_INCOMING; ip != NULL; ip=ip->next)
   { char *sp;
     int print=true;
   
   for (sp = ip->name; *sp != '\0'; sp++)
      {
      if (!isdigit((int)*sp))
	 {
	 print = false;
	 }
      }

   if (print)
      {
      Debug("Port(in,%s) ",ip->name);
      fprintf(fp,"pin-%s\n",ip->name);
      }
   }

Debug("\n\n"); 

for (ip = ALL_OUTGOING; ip != NULL; ip=ip->next)
   { char *sp;
     int print=true;
     
   for (sp = ip->name; *sp != '\0'; sp++)
      {
      if (!isdigit((int)*sp))
	 {
	 continue;
	 }
      }

   if (print)
      {
      Debug("Port(out,%s) ",ip->name);
      /* fprintf(fp,"pout-%s\n",ip->name);   */
      }
   }

Debug("\n\n");  
fclose(fp);

rename(ENV_NEW,ENV);
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void GatherProcessData()

{ FILE *pp;
  char pscomm[bufsize];
  char user[maxvarsize];
  struct Item *list = NULL;
  
snprintf(pscomm,bufsize,"%s %s",VPSCOMM[VSYSTEMHARDCLASS],VPSOPTS[VSYSTEMHARDCLASS]);

NUMBER_OF_USERS = ROOTPROCS = OTHERPROCS = 0; 

if ((pp = cfpopen(pscomm,"r")) == NULL)
   {
   return;
   }

ReadLine(VBUFF,bufsize,pp); 

while (!feof(pp))
   {
   ReadLine(VBUFF,bufsize,pp);
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

snprintf(VBUFF,maxvarsize,"%s/state/cf_users",WORKDIR);
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

{ FILE *pp,*fpout;
  char local[bufsize],remote[bufsize],comm[bufsize];
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
   bzero(local,bufsize);
   bzero(remote,bufsize);
   
   ReadLine(VBUFF,bufsize,pp);

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
      if (strcmp(local+strlen(local)-strlen(ECGSOCKS[i][0]),ECGSOCKS[i][0]) == 0)
	 {
	 INCOMING[i]++;
	 AppendItem(&in[i],VBUFF,"");
	 }

      if (strcmp(remote+strlen(remote)-strlen(ECGSOCKS[i][0]),ECGSOCKS[i][0]) == 0)
	 {
	 OUTGOING[i]++;
	 AppendItem(&out[i],VBUFF,"");
	 }
      }
   }

cfpclose(pp);

for (i = 0; i < ATTR; i++)
   {
   Verbose("%s = (%d,%d)\n",ECGSOCKS[i][1],INCOMING[i],OUTGOING[i]);
 
   snprintf(VBUFF,maxvarsize,"%s/state/cf_incoming.%s",WORKDIR,ECGSOCKS[i][1]);
   SaveItemList(in[i],VBUFF,"none");
   Debug("Saved netstat data in %s\n",VBUFF); 
   snprintf(VBUFF,maxvarsize,"%s/state/cf_outgoing.%s",WORKDIR,ECGSOCKS[i][1]);
   SaveItemList(out[i],VBUFF,"none");
   DeleteItemList(in[i]);
   DeleteItemList(out[i]);
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
   snprintf(OUTPUT,bufsize*2,"Can't open directory /proc\n");
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
      continue;
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

struct Averages *GetCurrentAverages(timekey)

char *timekey;

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

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));
bzero(&entry,sizeof(entry));
      
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
 
dbp->close(dbp,0);

AGE++;
WAGE = AGE / CFWEEK * MEASURE_INTERVAL;

if (value.data != NULL)
   {
   bcopy(value.data,&entry,sizeof(entry));
   Debug("Previous values (%f,..) for time index %s\n\n",entry.expect_number_of_users,timekey);
   return &entry;
   }
else
   {
   Debug("No previous value for time index %s\n",timekey);
   return &entry;
   }
}

/*****************************************************************************/

void UpdateAverages(timekey,newvals)

char *timekey;
struct Averages newvals;

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

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));
      
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

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));

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

void UpdateDistributions(timekey,av)

char *timekey;
struct Averages *av;

{ int position,olddist[GRAINS],newdist[GRAINS]; 
  int day,i,time_to_update = true;
 
/* Take an interval of 4 standard deviations from -2 to +2, divided into GRAINS
   parts. Centre each measurement on GRAINS/2 and scale each measurement by the
   std-deviation for the current time.
 */
if (HISTO)
   {
   time_to_update = (int) (3600.0*rand()/(RAND_MAX+1.0)) > 2400;
   
   day = Day2Number(timekey);
   
   position = GRAINS/2 + (int)(0.5+(NUMBER_OF_USERS - av->expect_number_of_users)*GRAINS/(4*sqrt((av->var_number_of_users))));
   if (0 <= position && position < GRAINS)
      {
      HISTOGRAM[0][day][position]++;
      }
   
   position = GRAINS/2 + (int)(0.5+(ROOTPROCS - av->expect_rootprocs)*GRAINS/(4*sqrt((av->var_rootprocs))));
   if (0 <= position && position < GRAINS)
      {
      HISTOGRAM[1][day][position]++;
      }
   
   position = GRAINS/2 + (int)(0.5+(OTHERPROCS - av->expect_otherprocs)*GRAINS/(4*sqrt((av->var_otherprocs))));
   if (0 <= position && position < GRAINS)
      {
      HISTOGRAM[2][day][position]++;
      }
   
   position = GRAINS/2 + (int)(0.5+(DISKFREE - av->expect_diskfree)*GRAINS/(4*sqrt((av->var_diskfree))));
   if (0 <= position && position < GRAINS)
      {
      HISTOGRAM[3][day][position]++;
      }

   position = GRAINS/2 + (int)(0.5+(LOADAVG - av->expect_loadavg)*GRAINS/(4*sqrt((av->var_loadavg))));
   if (0 <= position && position < GRAINS)
      {
      HISTOGRAM[4][day][position]++;
      }

   for (i = 0; i < ATTR; i++)
      {
      position = GRAINS/2 + (int)(0.5+(INCOMING[i] - av->expect_incoming[i])*GRAINS/(4*sqrt((av->var_incoming[i]))));
      if (0 <= position && position < GRAINS)
	 {
	 HISTOGRAM[5+i][day][position]++;
	 }
      
      position = GRAINS/2 + (int)(0.5+(OUTGOING[i] - av->expect_outgoing[i])*GRAINS/(4*sqrt((av->var_outgoing[i]))));
      if (0 <= position && position < GRAINS)
	 {
	 HISTOGRAM[5+ATTR+i][day][position]++;
	 }
      }

   for (i = 0; i < PH_LIMIT; i++)
      {
      if (PH_BINARIES[i] == NULL)
	 {
	 continue;
	 }
      
      position = GRAINS/2 + (int)(0.5+(PH_DELTA[i] - av->expect_pH[i])*GRAINS/(4*sqrt((av->var_pH[i]))));
      if (0 <= position && position < GRAINS)
	 {
	 HISTOGRAM[5+2*ATTR+i][day][position]++;
	 }
      }

   
   if (time_to_update)
      {
      FILE *fp;
      char filename[bufsize];
      
      snprintf(filename,bufsize,"%s/state/histograms",WORKDIR);
      
      if ((fp = fopen(filename,"w")) == NULL)
	 {
	 CfLog(cferror,"Unable to save histograms","fopen");
	 return;
	 }
      
      for (position = 0; position < GRAINS; position++)
	 {
	 fprintf(fp,"%u ",position);
	 
	 for (i = 0; i < 5 + 2*ATTR+PH_LIMIT; i++)
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

double WAverage(anew,aold,age)

 /* For a couple of weeks, learn eagerly. Otherwise variances will
    be way too large. Then downplay newer data somewhat, and rely on
    experience of a couple of months of data ... */

double anew,aold,age;

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
 
return av;
}

/*****************************************************************************/

void SetClasses(expect,delta,sigma,lexpect,ldelta,lsigma,name,classlist,timekey)

double expect,delta,sigma,lexpect,ldelta,lsigma;
char *name;
struct Item **classlist;
char *timekey;

{ char buffer[bufsize],buffer2[bufsize];
  double dev;
 
if (sigma == 0.0 || lsigma == 0.0)
   {
   return;
   }

if (expect < 1 && delta < 2)
   {
   return; /* Granularity makes this silly */
   }
 
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
 
if (dev > 2.0*sqrt(2.0))
   {
   strcpy(buffer2,buffer);
   strcat(buffer2,"_dev2");
   AppendItem(classlist,buffer2,"2");
   }
 
if (dev > 3.0*sqrt(2.0))
   {
   strcpy(buffer2,buffer);
   strcat(buffer2,"_anomalous");
   AppendItem(classlist,buffer2,"3");
   }
}

/*****************************************************************************/

void SetVariable(name,value,average,stddev,classlist)

char *name;
double value,average,stddev;
struct Item **classlist;

{ char var[bufsize];

sprintf(var,"value_%s=%d",name,(int)value);
AppendItem(classlist,var,"");

sprintf(var,"average_%s=%1.1f",name,average);
AppendItem(classlist,var,"");

sprintf(var,"stddev_%s=%1.1f",name,stddev);
AppendItem(classlist,var,""); 
}

/*****************************************************************************/

void RecordChangeOfState(classlist,timekey)

struct Item *classlist;
char *timekey;

{
/*  

 struct Item *ip;
  FILE *fp;
  int i = 0;

sprintf(OUTPUT,"Registered change of average state at %s",timekey);
CfLog(cflogonly,OUTPUT,"");

if ((fp = fopen(STATELOG,"a")) == NULL)
   {
   return;
   }

fprintf(fp,"%s ",timekey);
     
 
fprintf(fp,"\n");

fclose(fp);
chmod(STATELOG,0644); */
}

/*****************************************************************************/

double RejectAnomaly(new,average,variance,localav,localvar)

double new,average,variance,localav,localvar;

{ double dev = sqrt(variance+localvar);          /* Geometrical average dev */
 double delta;

if (average == 0)
   {
   return new;
   }

/* This routine puts some inertia into the changes, so that the system
   doesn't respond to every little change ...   IR and UV cutoff */
 
delta = sqrt((new-average)*(new-average)+(new-localav)*(new-localav));

if (delta > 4.0*dev)  /* IR */
   {
   return average + delta/2.0;    /* Damping */
   }
else
   {
   return new;
   }
}

/***************************************************************/

int HashPhKey(key)

char *key;

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

/***************************************************************/

int Repository(file,repository)

char *file, *repository;

{
 return false;
}
