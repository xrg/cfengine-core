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
/* File: cfenvgraph.c                                                        */
/*                                                                           */
/* Created: Wed Apr 18 13:19:22 2001                                         */
/*                                                                           */
/* Author: Mark                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include <math.h>
#include <db.h>

/*****************************************************************************/
/* Prototypes                                                                */
/*****************************************************************************/

int main (int argc, char **argv);
void CheckOpts (int argc, char **argv);
void Syntax (void);
void ReadAverages (void);
void SummarizeAverages (void);
void WriteGraphFiles (void);
void WriteHistograms (void);
void FindHurstExponents (void);
void DiskArrivals (void);
void GetFQHN (void);
struct Averages FindHurstFunction (int sameples_per_grain, int grains);

/*****************************************************************************/

/* char *COPYRIGHT = "Free Software Foundation 2001-\nDonated by Mark Burgess, Faculty of Engineering,\nOslo University College, 0254 Oslo, Norway";
 */

struct option GRAPHOPTIONS[] =
   {
   { "help",no_argument,0,'h' },
   { "file",required_argument,0,'f' },
   { "titles",no_argument,0,'t'},
   { "timestamps",no_argument,0,'T'},
   { "resolution",no_argument,0,'r'},
   { "separate",no_argument,0,'s'},
   { "no-error-bars",no_argument,0,'e'},
   { "no-scaling",no_argument,0,'n'},
   { NULL,0,0,0 }
   };

int TITLES = false;
int TIMESTAMPS = false;
int HIRES = false;
int SEPARATE = false;
int ERRORBARS = true;
int NOSCALING = true;
char FILENAME[CF_BUFSIZE];
unsigned int HISTOGRAM[ATTR*2+CF_NETATTR*2+5+PH_LIMIT][7][CF_GRAINS];
int SMOOTHHISTOGRAM[ATTR*2+CF_NETATTR*2+5+PH_LIMIT][7][CF_GRAINS];
char VFQNAME[CF_MAXVARSIZE];

/*****************************************************************************/
/*
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
   {"443","wwws"},
   };

char *TCPNAMES[CF_NETATTR] =
   {
   "icmp",
   "udp",
   "dns",
   "tcpsyn",
   "tcpack",
   "tcpfin",
   "misc",
   };
*/
char *PH_BINARIES[PH_LIMIT] =   /* Miss leading slash */
   {
   "usr/sbin/atd",
   "sbin/getty",
   "bin/bash",
   "usr/sbin/exim",
   "bin/run-parts",
   };

int errno,i,j,k,count=0, its;
time_t NOW; 
DBT key,value;
DB *DBP;
static struct Averages ENTRY,MAX,MIN,DET;
char TIMEKEY[CF_SMALLBUF],FLNAME[CF_BUFSIZE],*sp;
double AGE;
FILE *FPAV=NULL,*FPVAR=NULL,*FPROOT=NULL,*FPUSER=NULL,*FPOTHER=NULL;
FILE *FPDISK=NULL,*FPLOAD=NULL,*FPIN[ATTR],*FPOUT[ATTR],*FPPH[PH_LIMIT],*fp;
FILE *FPNETIN[CF_NETATTR],*FPNETOUT[CF_NETATTR];

/*****************************************************************************/

int main (int argc,char **argv)

{
CheckOpts(argc,argv);
GetFQHN();
ReadAverages(); 
SummarizeAverages();
WriteGraphFiles();
WriteHistograms();
FindHurstExponents();
DiskArrivals();
return 0;
}

/*****************************************************************************/
/* Level 1                                                                   */
/*****************************************************************************/

void GetFQHN()

{ FILE *pp;
  char cfcom[CF_BUFSIZE];
  static char line[CF_BUFSIZE],*sp;

snprintf(cfcom,CF_BUFSIZE-1,"%s/bin/cfagent -Q fqhost",CFWORKDIR);
 
if ((pp=popen(cfcom,"r")) ==  NULL)
   {
   printf("Couldn't open cfengine data ");
   perror("popen");
   exit(0);
   }

line[0] = '\0'; 
fgets(line,CF_BUFSIZE,pp);
for (sp = line; *sp != '\0'; sp++)
   {
   if (*sp == '=')
      {
      sp++;
      break;
      }
   }

strcpy(VFQNAME,line);

if (strlen(VFQNAME) == 0)
   {
   struct utsname sys;
   if (uname(&sys) == -1)
      {
      perror("uname ");
      exit(0);
      }
   strcpy(VFQNAME,sys.sysname);
   } 
else
   {
   VFQNAME[strlen(VFQNAME)-1] = '\0';
   printf("Got fully qualified name (%s)\n",VFQNAME);
   }
 
pclose(pp);
}

/****************************************************************************/

void ReadAverages()

{
printf("\nLooking for database %s\n",FILENAME);
printf("\nFinding MAXimum values...\n\n");
printf("N.B. socket values are numbers in CLOSE_WAIT. See documentation.\n"); 
  
if ((errno = db_create(&DBP,NULL,0)) != 0)
   {
   printf("Couldn't create average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((errno = DBP->open(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((errno = DBP->open(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)    
#endif
   {
   printf("Couldn't open average database %s\n",FILENAME);
   DBP->err(DBP,errno,NULL);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));

MAX.expect_number_of_users = 0.01;
MAX.expect_rootprocs = 0.01;
MAX.expect_otherprocs = 0.01;
MAX.expect_diskfree = 0.01;
MAX.expect_loadavg = 0.01; 

MAX.var_number_of_users = 0.01;
MAX.var_rootprocs = 0.01;
MAX.var_otherprocs = 0.01;
MAX.var_diskfree = 0.01;
MAX.var_loadavg = 0.01; 

MIN.expect_number_of_users = 9999.0;
MIN.expect_rootprocs = 9999.0;
MIN.expect_otherprocs = 9999.0;
MIN.expect_diskfree = 9999.0;
MIN.expect_loadavg = 9999.0; 

MIN.var_number_of_users = 9999.0;
MIN.var_rootprocs = 9999.0;
MIN.var_otherprocs = 9999.0;
MIN.var_diskfree = 9999.0;
MIN.var_loadavg = 9999.0; 

 
for (i = 0; i < ATTR; i++)
   {
   MAX.var_incoming[i] = 0.01;
   MAX.var_outgoing[i] = 0.01;
   MAX.expect_incoming[i] = 0.01;
   MAX.expect_outgoing[i] = 0.01;

   MIN.var_incoming[i] = 9999.0;
   MIN.var_outgoing[i] = 9999.0;
   MIN.expect_incoming[i] = 9999.0;
   MIN.expect_outgoing[i] = 9999.0;
   }

 
for (i = 0; i < CF_NETATTR; i++)
   {
   MAX.var_netin[i] = 0.01;
   MAX.var_netout[i] = 0.01;
   MAX.expect_netin[i] = 0.01;
   MAX.expect_netout[i] = 0.01;

   MIN.var_netin[i] = 9999.0;
   MIN.var_netout[i] = 9999.0;
   MIN.expect_netin[i] = 9999.0;
   MIN.expect_netout[i] = 9999.0;
   }
 
for (NOW = CF_MONDAY_MORNING; NOW < CF_MONDAY_MORNING+CF_WEEK; NOW += CF_MEASURE_INTERVAL)
   {
   memset(&key,0,sizeof(key));       
   memset(&value,0,sizeof(value));
   memset(&ENTRY,0,sizeof(ENTRY));

   strcpy(TIMEKEY,GenTimeKey(NOW));

   key.data = TIMEKEY;
   key.size = strlen(TIMEKEY)+1;
   
   if ((errno = DBP->get(DBP,NULL,&key,&value,0)) != 0)
      {
      if (errno != DB_NOTFOUND)
  {
  DBP->err(DBP,errno,NULL);
  exit(1);
  }
      }
   
   
   if (value.data != NULL)
      {
      memcpy(&ENTRY,value.data,sizeof(ENTRY));
      
      if (fabs(ENTRY.expect_number_of_users) > MAX.expect_number_of_users)
         {
         MAX.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }
      if (fabs(ENTRY.expect_number_of_users) > MAX.expect_number_of_users)
         {
         MAX.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }      
      if (fabs(ENTRY.expect_rootprocs) > MAX.expect_rootprocs)
         {
         MAX.expect_rootprocs = fabs(ENTRY.expect_rootprocs);
         }
      if (fabs(ENTRY.expect_otherprocs) >  MAX.expect_otherprocs)
         {
         MAX.expect_otherprocs = fabs(ENTRY.expect_otherprocs);
         }      
      if (fabs(ENTRY.expect_diskfree) > MAX.expect_diskfree)
         {
         MAX.expect_diskfree = fabs(ENTRY.expect_diskfree);
         }
      if (fabs(ENTRY.expect_loadavg) > MAX.expect_loadavg)
         {
         MAX.expect_loadavg = fabs(ENTRY.expect_loadavg);
         }
      
      for (i = 0; i < ATTR; i++)
         {
         if (fabs(ENTRY.expect_incoming[i]) > MAX.expect_incoming[i])
            {
            MAX.expect_incoming[i] = fabs(ENTRY.expect_incoming[i]);
            }
         if (fabs(ENTRY.expect_outgoing[i]) > MAX.expect_outgoing[i])
            {
            MAX.expect_outgoing[i] = fabs(ENTRY.expect_outgoing[i]);
            }
         }
      
      
      if (fabs(ENTRY.expect_number_of_users) < MIN.expect_number_of_users)
         {
         MIN.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }
      if (fabs(ENTRY.expect_number_of_users) < MIN.expect_number_of_users)
         {
         MIN.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }      
      if (fabs(ENTRY.expect_rootprocs) < MIN.expect_rootprocs)
         {
         MIN.expect_rootprocs = fabs(ENTRY.expect_rootprocs);
  }
      if (fabs(ENTRY.expect_otherprocs) < MIN.expect_otherprocs)
         {
         MIN.expect_otherprocs = fabs(ENTRY.expect_otherprocs);
         }      
      if (fabs(ENTRY.expect_diskfree) < MIN.expect_diskfree)
         {
         MIN.expect_diskfree = fabs(ENTRY.expect_diskfree);
         }
      if (fabs(ENTRY.expect_loadavg) < MIN.expect_loadavg)
         {
         MIN.expect_loadavg = fabs(ENTRY.expect_loadavg);
         }
      
      for (i = 0; i < ATTR; i++)
         {
         if (fabs(ENTRY.expect_incoming[i]) < MIN.expect_incoming[i])
            {
            MIN.expect_incoming[i] = fabs(ENTRY.expect_incoming[i]);
            }
         if (fabs(ENTRY.expect_outgoing[i]) < MIN.expect_outgoing[i])
            {
            MIN.expect_outgoing[i] = fabs(ENTRY.expect_outgoing[i]);
            }
         }
      
      for (i = 0; i < CF_NETATTR; i++)
         {
         if (fabs(ENTRY.expect_netin[i]) < MIN.expect_netin[i])
            {
            MIN.expect_netin[i] = fabs(ENTRY.expect_netin[i]);
            }
         if (fabs(ENTRY.expect_netout[i]) < MIN.expect_netout[i])
            {
            MIN.expect_netout[i] = fabs(ENTRY.expect_netout[i]);
            }
         }
      
      
      if (fabs(ENTRY.var_number_of_users) > MAX.var_number_of_users)
         {
         MAX.var_number_of_users = fabs(ENTRY.var_number_of_users);
         }
      if (fabs(ENTRY.var_number_of_users) > MAX.var_number_of_users)
         {
         MAX.var_number_of_users = fabs(ENTRY.var_number_of_users);
         }      
      if (fabs(ENTRY.var_rootprocs) > MAX.var_rootprocs)
         {
         MAX.var_rootprocs = fabs(ENTRY.var_rootprocs);
         }
      if (fabs(ENTRY.var_otherprocs) >  MAX.var_otherprocs)
         {
         MAX.var_otherprocs = fabs(ENTRY.var_otherprocs);
         }      
      if (fabs(ENTRY.var_diskfree) > MAX.var_diskfree)
         {
         MAX.var_diskfree = fabs(ENTRY.var_diskfree);
         }
      if (fabs(ENTRY.var_loadavg) > MAX.var_loadavg)
         {
         MAX.var_diskfree = fabs(ENTRY.var_loadavg);
         }
      
      for (i = 0; i < ATTR; i++)
         {
         if (fabs(ENTRY.var_incoming[i]) > MAX.var_incoming[i])
            {
            MAX.var_incoming[i] = fabs(ENTRY.var_incoming[i]);
            }
         if (fabs(ENTRY.var_outgoing[i]) > MAX.var_outgoing[i])
            {
            MAX.var_outgoing[i] = fabs(ENTRY.var_outgoing[i]);
            }
         }
      
      
      for (i = 0; i < CF_NETATTR; i++)
         {
         if (fabs(ENTRY.expect_netin[i]) > MAX.expect_netin[i])
            {
            MAX.expect_netin[i] = fabs(ENTRY.expect_netin[i]);
            }
         if (fabs(ENTRY.expect_netout[i]) > MAX.expect_netout[i])
            {
            MAX.expect_netout[i] = fabs(ENTRY.expect_netout[i]);
            }
         }

      for (i = 0; i < PH_LIMIT; i++)
         {
         if (PH_BINARIES[i] == NULL)
            {
            continue;
            }
         
         if (fabs(ENTRY.expect_pH[i]) > MAX.expect_pH[i])
            {
            MAX.expect_pH[i] = fabs(ENTRY.expect_pH[i]);
            }
         
         if (fabs(ENTRY.var_pH[i]) > MAX.var_pH[i])
            {
            MAX.var_pH[i] = fabs(ENTRY.var_pH[i]);
            }
         }
      
      
      }
   }
 
 DBP->close(DBP,0);
}

/*****************************************************************************/

void SummarizeAverages()

{
 
printf(" x  yN (Variable content)\n---------------------------------------------------------\n");
printf(" 1. MAX <number of users> = %10f - %10f u %10f\n",MIN.expect_number_of_users,MAX.expect_number_of_users,sqrt(MAX.var_number_of_users));
printf(" 2. MAX <rootprocs>       = %10f - %10f u %10f\n",MIN.expect_rootprocs,MAX.expect_rootprocs,sqrt(MAX.var_rootprocs));
printf(" 3. MAX <otherprocs>      = %10f - %10f u %10f\n",MIN.expect_otherprocs,MAX.expect_otherprocs,sqrt(MAX.var_otherprocs));
printf(" 4. MAX <diskfree>        = %10f - %10f u %10f\n",MIN.expect_diskfree,MAX.expect_diskfree,sqrt(MAX.var_diskfree));
printf(" 5. MAX <loadavg>         = %10f - %10f u %10f\n",MIN.expect_loadavg,MAX.expect_loadavg,sqrt(MAX.var_loadavg)); 

 for (i = 0; i < ATTR*2; i+=2)
   {
   printf("%2d. MAX <%-10s-in>   = %10f - %10f u %10f\n",6+i,ECGSOCKS[i/2][1],MIN.expect_incoming[i/2],MAX.expect_incoming[i/2],sqrt(MAX.var_incoming[i/2]));
   printf("%2d. MAX <%-10s-out>  = %10f - %10f u %10f\n",7+i,ECGSOCKS[i/2][1],MIN.expect_outgoing[i/2],MAX.expect_outgoing[i/2],sqrt(MAX.var_outgoing[i/2]));
   }
 
 for (i = 0; i < CF_NETATTR*2; i+=2)
   {
   printf("%2d. MAX <%-10s-in>   = %10f - %10f u %10f\n",6+ATTR+i,TCPNAMES[i/2],MIN.expect_netin[i/2],MAX.expect_netin[i/2],sqrt(MAX.var_netin[i/2]));
   printf("%2d. MAX <%-10s-out>  = %10f - %10f u %10f\n",7+ATTR+i,TCPNAMES[i/2],MIN.expect_netout[i/2],MAX.expect_netout[i/2],sqrt(MAX.var_netout[i/2]));
   }
 

 for (i = 0; i < PH_LIMIT; i++)
   {
   if (PH_BINARIES[i] == NULL)
      {
      continue;
      }
   printf("%2d. MAX <%-10s-in>   = %10f - %10f u %10f\n",i+5+CF_NETATTR+ATTR,PH_BINARIES[i],MIN.expect_pH[i],MAX.expect_pH[i],sqrt(MAX.var_pH[i]));
   }

 
if ((errno = db_create(&DBP,NULL,0)) != 0)
   {
   printf("Couldn't open average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((errno = DBP->open(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((errno = DBP->open(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#endif
   {
   printf("Couldn't open average database %s\n",FILENAME);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
      
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE")+1;

if ((errno = DBP->get(DBP,NULL,&key,&value,0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      DBP->err(DBP,errno,NULL);
      exit(1);
      }
   }
 
if (value.data != NULL)
   {
   AGE = *(double *)(value.data);
   printf("\n\nDATABASE_AGE %.1f (weeks)\n\n",AGE/CF_WEEK*CF_MEASURE_INTERVAL);
   }

}

/*****************************************************************************/

void WriteGraphFiles()

{
if (TIMESTAMPS)
   {
   if ((NOW = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n");
      }
     
   sprintf(FLNAME,"cfenvgraphs-%s-%s",VFQNAME,ctime(&NOW));

   for (sp = FLNAME; *sp != '\0'; sp++)
      {
      if (isspace((int)*sp))
         {
         *sp = '_';
         }
      }    
   }
 else
   {
   sprintf(FLNAME,"cfenvgraphs-snapshot-%s",VFQNAME);
   }

printf("Creating sub-directory %s\n",FLNAME);

if (mkdir(FLNAME,0755) == -1)
   {
   perror("mkdir");
   printf("Aborting\n");
   exit(0);
   }
 
if (chdir(FLNAME))
   {
   perror("chdir");
   exit(0);
   }


printf("Writing data to sub-directory %s: \n   x,y1,y2,y3...\n ",FLNAME);


sprintf(FLNAME,"cfenv-average");

if ((FPAV = fopen(FLNAME,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }

sprintf(FLNAME,"cfenv-stddev"); 

if ((FPVAR = fopen(FLNAME,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }


/* Now if -s open a file foreach metric! */

if (SEPARATE)
   {
   sprintf(FLNAME,"users.cfenv"); 
   if ((FPUSER = fopen(FLNAME,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(FLNAME,"rootprocs.cfenv"); 
   if ((FPROOT = fopen(FLNAME,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(FLNAME,"otherprocs.cfenv"); 
   if ((FPOTHER = fopen(FLNAME,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(FLNAME,"freedisk.cfenv"); 
   if ((FPDISK = fopen(FLNAME,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(FLNAME,"loadavg.cfenv"); 
   if ((FPLOAD = fopen(FLNAME,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }

   for (i = 0; i < ATTR; i++)
      {
      sprintf(FLNAME,"%s-in.cfenv",ECGSOCKS[i][1]); 
      if ((FPIN[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }

      sprintf(FLNAME,"%s-out.cfenv",ECGSOCKS[i][1]); 
      if ((FPOUT[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      }

   for (i = 0; i < CF_NETATTR; i++)
      {
      sprintf(FLNAME,"%s-in.cfenv",TCPNAMES[i]); 
      if ((FPNETIN[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }

      sprintf(FLNAME,"%s-out.cfenv",TCPNAMES[i]); 
      if ((FPNETOUT[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      }

   for (i = 0; i < PH_LIMIT; i++)
      {
      if (PH_BINARIES[i] == NULL)
  {
  continue;
  }
      
      sprintf(FLNAME,"%s.cfenv",CanonifyName(PH_BINARIES[i])); 
      if ((FPPH[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      }

   }

if (TITLES)
   {
   fprintf(FPAV,"# Column 1: Users\n");
   fprintf(FPAV,"# Column 2: Root Processes\n");
   fprintf(FPAV,"# Column 3: Non-root Processes 3\n");
   fprintf(FPAV,"# Column 4: Percent free disk\n");
   fprintf(FPAV,"# Column 5: Load average\n");
     
   for (i = 0; i < ATTR*2; i+=2)
      {
      fprintf(FPAV,"# Column %d: Incoming %s sockets\n",6+i,ECGSOCKS[i/2][1]);
      fprintf(FPAV,"# Column %d: Outgoing %s sockets\n",7+i,ECGSOCKS[i/2][1]);
      }

   for (i = 0; i < CF_NETATTR*2; i+=2)
      {
      fprintf(FPAV,"# Column %d: Incoming %s packets\n",6+ATTR+i,TCPNAMES[i/2]);
      fprintf(FPAV,"# Column %d: Outgoing %s packets\n",7+ATTR+i,TCPNAMES[i/2]);
      }

   for (i = 0; i < PH_LIMIT; i++)
      {
      if (PH_BINARIES[i] == NULL)
  {
  continue;
  }
      fprintf(FPAV,"# Column %d: pH %s \n",6+i,PH_BINARIES[i]);
      }

   fprintf(FPAV,"##############################################\n");
     
   fprintf(FPVAR,"# Column 1: Users\n");
   fprintf(FPVAR,"# Column 2: Root Processes\n");
   fprintf(FPVAR,"# Column 3: Non-root Processes 3\n");
   fprintf(FPVAR,"# Column 4: Percent free disk\n");
   fprintf(FPVAR,"# Column 5: Load Average\n");
     
   for (i = 0; i < ATTR*2; i+=2)
      {
      fprintf(FPVAR,"# Column %d: Incoming %s sockets\n",6+i,ECGSOCKS[i/2][1]);
      fprintf(FPVAR,"# Column %d: Outgoing %s sockets\n",7+i,ECGSOCKS[i/2][1]);
      }

   for (i = 0; i < CF_NETATTR*2; i+=2)
      {
      fprintf(FPVAR,"# Column %d: Incoming %s packets\n",6+ATTR+i,TCPNAMES[i/2]);
      fprintf(FPVAR,"# Column %d: Outgoing %s packets\n",7+ATTR+i,TCPNAMES[i/2]);
      }

   for (i = 0; i < PH_LIMIT; i++)
      {
      if (PH_BINARIES[i] == NULL)
         {
         continue;
         }
      fprintf(FPVAR,"# Column %d: pH %s \n",6+i,PH_BINARIES[i]);
      }

   fprintf(FPVAR,"##############################################\n");
   }

if (HIRES)
   {
   its = 1;
   }
else
   {
   its = 12;
   }

NOW = CF_MONDAY_MORNING;
memset(&ENTRY,0,sizeof(ENTRY)); 
 
while (NOW < CF_MONDAY_MORNING+CF_WEEK)
   {
   for (j = 0; j < its; j++)
      {
      memset(&key,0,sizeof(key));       
      memset(&value,0,sizeof(value));
      
      strcpy(TIMEKEY,GenTimeKey(NOW));
      key.data = TIMEKEY;
      key.size = strlen(TIMEKEY)+1;
      
      if ((errno = DBP->get(DBP,NULL,&key,&value,0)) != 0)
         {
         if (errno != DB_NOTFOUND)
            {
            DBP->err(DBP,errno,NULL);
            exit(1);
            }
         }
      
      if (value.data != NULL)
         {
         memcpy(&DET,value.data,sizeof(DET));
         
         ENTRY.expect_number_of_users += DET.expect_number_of_users/(double)its;
         ENTRY.expect_rootprocs += DET.expect_rootprocs/(double)its;
         ENTRY.expect_otherprocs += DET.expect_otherprocs/(double)its;
         ENTRY.expect_diskfree += DET.expect_diskfree/(double)its;
         ENTRY.expect_loadavg += DET.expect_loadavg/(double)its;
         ENTRY.var_number_of_users += DET.var_number_of_users/(double)its;
         ENTRY.var_rootprocs += DET.var_rootprocs/(double)its;
         ENTRY.var_otherprocs += DET.var_otherprocs/(double)its;
         ENTRY.var_diskfree += DET.var_diskfree/(double)its;
         ENTRY.var_loadavg += DET.var_loadavg/(double)its;
         
         for (i = 0; i < ATTR; i++)
            {
            ENTRY.expect_incoming[i] += DET.expect_incoming[i]/(double)its;
            ENTRY.expect_outgoing[i] += DET.expect_outgoing[i]/(double)its;
            ENTRY.var_incoming[i] += DET.var_incoming[i]/(double)its;
            ENTRY.var_outgoing[i] += DET.var_outgoing[i]/(double)its;
            }
         
         for (i = 0; i < CF_NETATTR; i++)
            {
            ENTRY.expect_netin[i] += DET.expect_netin[i]/(double)its;
            ENTRY.expect_netout[i] += DET.expect_netout[i]/(double)its;
            ENTRY.var_netin[i] += DET.var_netin[i]/(double)its;
            ENTRY.var_netout[i] += DET.var_netout[i]/(double)its;
            }
         
         for (i = 0; i< PH_LIMIT; i++)
            {
            if (PH_BINARIES[i] == NULL)
               {
               continue;
               }
            
            ENTRY.expect_pH[i] += DET.expect_pH[i]/(double)its;
            ENTRY.var_pH[i] += DET.var_pH[i]/(double)its;
            }
         
         
         if (NOSCALING)
            {
            MAX.expect_number_of_users = 1;
            MAX.expect_rootprocs = 1;
            MAX.expect_otherprocs = 1;
            MAX.expect_diskfree = 1;
            MAX.expect_loadavg = 1;
            
            for (i = 1; i < ATTR; i++)
               {
               MAX.expect_incoming[i] = 1;
               MAX.expect_outgoing[i] = 1;
               }
            
            for (i = 1; i < CF_NETATTR; i++)
               {
               MAX.expect_netin[i] = 1;
               MAX.expect_netout[i] = 1;
               }
            
            for (i = 1; i < PH_LIMIT; i++)
               {
               if (PH_BINARIES[i] == NULL)
                  {
                  continue;
                  }
               MAX.expect_pH[i] = 1;
               }
            
            }
         
         if (j == its-1)
            {
            fprintf(FPAV,"%d %f %f %f %f %f",count++,
                    ENTRY.expect_number_of_users/MAX.expect_number_of_users,
                    ENTRY.expect_rootprocs/MAX.expect_rootprocs,
                    ENTRY.expect_otherprocs/MAX.expect_otherprocs,
                    ENTRY.expect_diskfree/MAX.expect_diskfree,
                    ENTRY.expect_loadavg/MAX.expect_loadavg);
            
            for (i = 0; i < ATTR; i++)
               {
               fprintf(FPAV,"%f %f "
                       ,ENTRY.expect_incoming[i]/MAX.expect_incoming[i]
                       ,ENTRY.expect_outgoing[i]/MAX.expect_outgoing[i]);
               }
            
            for (i = 0; i < CF_NETATTR; i++)
               {
               fprintf(FPAV,"%f %f "
                       ,ENTRY.expect_netin[i]/MAX.expect_netin[i]
                       ,ENTRY.expect_netout[i]/MAX.expect_netout[i]);
               }
            
            for (i = 0; i < PH_LIMIT; i++)
               {
               if (PH_BINARIES[i] == NULL)
                  {
                  continue;
                  }
               fprintf(FPAV,"%f ",ENTRY.expect_pH[i]/MAX.expect_pH[i]);
               }
            
            
            fprintf(FPAV,"\n");
            
            fprintf(FPVAR,"%d %f %f %f %f %f",count,
                    sqrt(ENTRY.var_number_of_users)/MAX.expect_number_of_users,
                    sqrt(ENTRY.var_rootprocs)/MAX.expect_rootprocs,
                    sqrt(ENTRY.var_otherprocs)/MAX.expect_otherprocs,
                    sqrt(ENTRY.var_diskfree)/MAX.expect_diskfree,
                    sqrt(ENTRY.var_loadavg)/MAX.expect_loadavg);
            
            for (i = 0; i < ATTR; i++)
               {
               fprintf(FPVAR,"%f %f ",
                       sqrt(ENTRY.var_incoming[i])/MAX.expect_incoming[i],
                       sqrt(ENTRY.var_outgoing[i])/MAX.expect_outgoing[i]);
               }
            
            for (i = 0; i < CF_NETATTR; i++)
               {
               fprintf(FPVAR,"%f %f ",
                       sqrt(ENTRY.var_netin[i])/MAX.expect_netin[i],
                       sqrt(ENTRY.var_netout[i])/MAX.expect_netout[i]);
               }
            
            for (i = 0; i < PH_LIMIT; i++)
               {
               if (PH_BINARIES[i] == NULL)
                  {
                  continue;
                  }
               fprintf(FPVAR,"%f ",sqrt(ENTRY.var_pH[i])/MAX.expect_pH[i]);
               }
            
            fprintf(FPVAR,"\n");
            
            if (SEPARATE)
               {
               fprintf(FPUSER,"%d %f %f\n",count,ENTRY.expect_number_of_users/MAX.expect_number_of_users,sqrt(ENTRY.var_number_of_users)/MAX.expect_number_of_users);
               fprintf(FPROOT,"%d %f %f\n",count,ENTRY.expect_rootprocs/MAX.expect_rootprocs,sqrt(ENTRY.var_rootprocs)/MAX.expect_rootprocs);
               fprintf(FPOTHER,"%d %f %f\n",count,ENTRY.expect_otherprocs/MAX.expect_otherprocs,sqrt(ENTRY.var_otherprocs)/MAX.expect_otherprocs);
               fprintf(FPDISK,"%d %f %f\n",count,ENTRY.expect_diskfree/MAX.expect_diskfree,sqrt(ENTRY.var_diskfree)/MAX.expect_diskfree);
               fprintf(FPLOAD,"%d %f %f\n",count,ENTRY.expect_loadavg/MAX.expect_loadavg,sqrt(ENTRY.var_loadavg)/MAX.expect_loadavg);
               
               for (i = 0; i < ATTR; i++)
                  {
                  fprintf(FPIN[i],"%d %f %f\n",count,ENTRY.expect_incoming[i]/MAX.expect_incoming[i],sqrt(ENTRY.var_incoming[i])/MAX.expect_incoming[i]);
                  fprintf(FPOUT[i],"%d %f %f\n",count,ENTRY.expect_outgoing[i]/MAX.expect_outgoing[i],sqrt(ENTRY.var_outgoing[i])/MAX.expect_outgoing[i]);
                  }
               
               for (i = 0; i < CF_NETATTR; i++)
                  {
                  fprintf(FPNETIN[i],"%d %f %f\n",count,ENTRY.expect_netin[i]/MAX.expect_netin[i],sqrt(ENTRY.var_netin[i])/MAX.expect_netin[i]);
                  fprintf(FPNETOUT[i],"%d %f %f\n",count,ENTRY.expect_netout[i]/MAX.expect_netout[i],sqrt(ENTRY.var_netout[i])/MAX.expect_netout[i]);
                  }
               
               for (i = 0; i < PH_LIMIT; i++)
                  {
                  if (PH_BINARIES[i] == NULL)
                     {
                     continue;
                     }
                  fprintf(FPPH[i],"%d %f %f\n",count,ENTRY.expect_pH[i]/MAX.expect_pH[i],sqrt(ENTRY.var_pH[i])/MAX.expect_pH[i]);
                  }        
               }
            
            memset(&ENTRY,0,sizeof(ENTRY)); 
            }
         }
      
      NOW += CF_MEASURE_INTERVAL;
      }
   }
 
 DBP->close(DBP,0);
 
 fclose(FPAV);
 fclose(FPVAR); 
 
 if (SEPARATE)
    {
    fclose(FPROOT);
    fclose(FPOTHER);
    fclose(FPUSER);
    fclose(FPDISK);
    fclose(FPLOAD);
    
    for (i = 0; i < ATTR; i++)
       {
       fclose(FPIN[i]);
       fclose(FPOUT[i]);
       }
    
    for (i = 0; i < CF_NETATTR; i++)
       {
       fclose(FPNETIN[i]);
       fclose(FPNETOUT[i]);
       }
    
    for (i = 0; i < PH_LIMIT; i++)
       {
       if (PH_BINARIES[i] == NULL)
          {
          continue;
          }
       fclose(FPPH[i]);
       }
    }
 
}

/*****************************************************************************/

void WriteHistograms()

{
/* Finally, look at the histograms */

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < PH_LIMIT+ATTR*2+CF_NETATTR*2+5; j++)
      {
      for (k = 0; k < CF_GRAINS; k++)
         {
         HISTOGRAM[j][i][k] = 0;
         }
      }
   }
 
 if (SEPARATE)
    {
    int position,day;
    int weekly[CF_NETATTR*2+ATTR*2+5+PH_LIMIT][CF_GRAINS];
    
    snprintf(FLNAME,CF_BUFSIZE,"%s/state/histograms",CFWORKDIR);
    
    if ((fp = fopen(FLNAME,"r")) == NULL)
       {
       printf("Unable to load histogram data\n");
       exit(1);
       }
    
    for (position = 0; position < CF_GRAINS; position++)
       {
       fscanf(fp,"%d ",&position);
       
       for (i = 0; i < 5 + 2*CF_NETATTR+2*ATTR+PH_LIMIT; i++)
          {
          for (day = 0; day < 7; day++)
             {
             fscanf(fp,"%d ",&(HISTOGRAM[i][day][position]));
             }
          
          weekly[i][position] = 0;
          }
       }
    
    fclose(fp);
    
    if (!HIRES)
       {
       /* Smooth daily and weekly histograms */
       for (k = 1; k < CF_GRAINS-1; k++)
          {
          for (j = 0; j < ATTR*2+CF_NETATTR*2+5+PH_LIMIT; j++)
             {
             for (i = 0; i < 7; i++)  
                {
                SMOOTHHISTOGRAM[j][i][k] = ((double)(HISTOGRAM[j][i][k-1] + HISTOGRAM[j][i][k] + HISTOGRAM[j][i][k+1]))/3.0;
                }
             }
          }
       }
    else
       {
       for (k = 1; k < CF_GRAINS-1; k++)
          {
          for (j = 0; j < ATTR*2+CF_NETATTR*2+5+PH_LIMIT; j++)
             {
             for (i = 0; i < 7; i++)  
                {
                SMOOTHHISTOGRAM[j][i][k] = (double) HISTOGRAM[j][i][k];
                }
             }
          }
       }
    
    sprintf(FLNAME,"users.distr"); 
    if ((FPUSER = fopen(FLNAME,"w")) == NULL)
       {
       perror("fopen");
       exit(1);
       }
    sprintf(FLNAME,"rootprocs.distr"); 
    if ((FPROOT = fopen(FLNAME,"w")) == NULL)
       {
       perror("fopen");
       exit(1);
       }
    sprintf(FLNAME,"otherprocs.distr"); 
    if ((FPOTHER = fopen(FLNAME,"w")) == NULL)
       {
       perror("fopen");
       exit(1);
       }
    sprintf(FLNAME,"freedisk.distr"); 
    if ((FPDISK = fopen(FLNAME,"w")) == NULL)
       {
       perror("fopen");
       exit(1);
       }
    sprintf(FLNAME,"loadavg.distr"); 
    if ((FPLOAD = fopen(FLNAME,"w")) == NULL)
       {
       perror("fopen");
       exit(1);
       }
    
    for (i = 0; i < ATTR; i++)
       {
       sprintf(FLNAME,"%s-in.distr",ECGSOCKS[i][1]); 
       if ((FPIN[i] = fopen(FLNAME,"w")) == NULL)
          {
          perror("fopen");
          exit(1);
          }

       sprintf(FLNAME,"%s-out.distr",ECGSOCKS[i][1]); 
       if ((FPOUT[i] = fopen(FLNAME,"w")) == NULL)
          {
          perror("fopen");
          exit(1);
          }
       }
    
       for (i = 0; i < CF_NETATTR; i++)
          {
          sprintf(FLNAME,"%s-in.distr",TCPNAMES[i]); 
          if ((FPNETIN[i] = fopen(FLNAME,"w")) == NULL)
             {
             perror("fopen");
             exit(1);
             }
          sprintf(FLNAME,"%s-out.distr",TCPNAMES[i]); 

          if ((FPNETOUT[i] = fopen(FLNAME,"w")) == NULL)
             {
             perror("fopen");
             exit(1);
             }
          }
       
    for (i = 0; i < PH_LIMIT; i++)
       {
       if (PH_BINARIES[i] == NULL)
          {
          continue;
          }
       
       sprintf(FLNAME,"%s.distr",CanonifyName(PH_BINARIES[i])); 
       if ((FPPH[i] = fopen(FLNAME,"w")) == NULL)
          {
          perror("fopen");
          exit(1);
          }
       }
    
    /* Plot daily and weekly histograms */
    for (k = 0; k < CF_GRAINS; k++)
       {
       int a;
       
       for (j = 0; j < ATTR*2+CF_NETATTR*2+5+PH_LIMIT; j++)
          {
          for (i = 0; i < 7; i++)  
             {
             weekly[j][k] += (int) (SMOOTHHISTOGRAM[j][i][k]+0.5);
             }
          }
       
       fprintf(FPUSER,"%d %d\n",k,weekly[0][k]);
       fprintf(FPROOT,"%d %d\n",k,weekly[1][k]);
       fprintf(FPOTHER,"%d %d\n",k,weekly[2][k]);
       fprintf(FPDISK,"%d %d\n",k,weekly[3][k]);
       fprintf(FPLOAD,"%d %d\n",k,weekly[4][k]);
       
       for (a = 0; a < ATTR; a++)
          {
          fprintf(FPIN[a],"%d %d\n",k,weekly[5+a][k]);
          fprintf(FPOUT[a],"%d %d\n",k,weekly[5+ATTR+a][k]);
          }

       for (a = 0; a < CF_NETATTR; a++)
          {
          fprintf(FPNETIN[a],"%d %d\n",k,weekly[5+2*ATTR+a][k]);
          fprintf(FPNETOUT[a],"%d %d\n",k,weekly[5+2*ATTR+CF_NETATTR+a][k]);
          }

       for (a = 0; a < PH_LIMIT; a++)
          {
          if (PH_BINARIES[a] == NULL)
             {
             continue;
             }
          fprintf(FPIN[a],"%d %d\n",k,weekly[5+2*ATTR+2*CF_NETATTR+a][k]);
          }
       
       }
    
    fclose(FPROOT);
    fclose(FPOTHER);
    fclose(FPUSER);
    fclose(FPDISK);
    fclose(FPLOAD);
    
    for (i = 0; i < ATTR; i++)
       {
       fclose(FPIN[i]);
       fclose(FPOUT[i]);
       }

    for (i = 0; i < CF_NETATTR; i++)
       {
       fclose(FPNETIN[i]);
       fclose(FPNETOUT[i]);
       }
    
    for (i = 0; i < PH_LIMIT; i++)
       {
       if (PH_BINARIES[i] == NULL)
          {
          continue;
          }
       fclose(FPPH[i]);
       }   
    }
}

/*****************************************************************************/

void FindHurstExponents()

{ int delta_t[5],grains[5],i,j;
 int samples_per_grain[5];
 double dilatation, uncertainty;
 struct Averages H[5],M[5],h2;
 
/* Dilatation intervals */
 
 delta_t[0] = CF_MEASURE_INTERVAL*2;
 delta_t[1] = 3600;
 delta_t[2] = 6 * 3600;
 delta_t[3] = 24 * 3600;
 delta_t[4] = CF_WEEK; 
 
 memset(&h2,0,sizeof(struct Averages));
 
 for (i = 0; i < 5; i++)
    {
    grains[i] = CF_WEEK/delta_t[i];
    samples_per_grain[i] = delta_t[i]/CF_MEASURE_INTERVAL;
    H[i] = FindHurstFunction(samples_per_grain[i],grains[i]);
    }
 
 printf("\n============================================================================\n");
 printf("Fluctuation measures - Hurst exponent estimates\n");
 printf("============================================================================\n");
 
 for (i = 1; i < 5; i++)
    {
    dilatation = (double)delta_t[i]/(double)delta_t[0];
    M[i].expect_number_of_users = log(H[i].expect_number_of_users/H[0].expect_number_of_users)/log(dilatation);
    printf(" M[%d].users = %f\n",i,M[i].expect_number_of_users);
    
    M[i].expect_rootprocs = log(H[i].expect_rootprocs/H[0].expect_rootprocs)/log(dilatation);
    printf(" M[%d].rootprocs = %f\n",i,M[i].expect_rootprocs);
    
    M[i].expect_otherprocs = log(H[i].expect_otherprocs/H[0].expect_otherprocs)/log(dilatation);
    printf(" M[%d].userprocs = %f\n",i,M[i].expect_otherprocs);
   
    M[i].expect_diskfree = log(H[i].expect_diskfree/H[0].expect_diskfree)/log(dilatation);
    printf(" M[%d].diskfree = %f\n",i,M[i].expect_diskfree);
    
    M[i].expect_loadavg = log(H[i].expect_loadavg/H[0].expect_loadavg)/log(dilatation);
    printf(" M[%d].loadavg = %f\n",i,M[i].expect_loadavg);
    
    for (j = 0; j < ATTR; j++)
       {
       M[i].expect_incoming[j] = log(H[i].expect_incoming[j]/H[0].expect_incoming[j])/log(dilatation);
       printf(" M[%d].incoming.%s = %f\n",i,ECGSOCKS[j][1],M[i].expect_incoming[i]);
       M[i].expect_outgoing[j] = log(H[i].expect_outgoing[j]/H[0].expect_outgoing[j])/log(dilatation);
       printf(" M[%d].outgoing.%s = %f\n",i,ECGSOCKS[j][1],M[i].expect_outgoing[i]);
       }
    
    h2.expect_number_of_users += M[i].expect_number_of_users * M[i].expect_number_of_users/4.0;
    h2.expect_rootprocs += M[i].expect_rootprocs * M[i].expect_rootprocs/4.0;
    h2.expect_otherprocs += M[i].expect_otherprocs * M[i].expect_otherprocs/4.0;
    h2.expect_diskfree += M[i].expect_diskfree * M[i].expect_diskfree/4.0;
    h2.expect_loadavg += M[i].expect_loadavg * M[i].expect_loadavg/4.0;
    
    for (j = 0; j < ATTR; j++)
       {
       h2.expect_incoming[j] += M[i].expect_incoming[j] * M[i].expect_incoming[j]/4.0;
       h2.expect_outgoing[j] += M[i].expect_outgoing[j] * M[i].expect_outgoing[j]/4.0;
       }
    
    uncertainty = 1.0/fabs(1.0/H[i].expect_number_of_users - 1.0/H[0].expect_number_of_users)*sqrt(MAX.var_number_of_users/log(dilatation))/(MAX.expect_number_of_users*2.0);
    }
 
 printf("\n\nESTIMATED RMS HURST EXPONENTS...\n\n"); 
 printf("Hurst exponent for no. of users        = %.1f u %.2f - order of mag\n",sqrt(h2.expect_number_of_users),uncertainty);
 printf("Hurst exponent for rootprocs           = %.1f\n",sqrt(h2.expect_rootprocs));
 printf("Hurst exponent for otherprocs          = %.1f\n",sqrt(h2.expect_otherprocs));
 printf("Hurst exponent for diskfree            = %.1f\n",sqrt(h2.expect_diskfree));
 printf("Hurst exponent for loadavg             = %.1f\n",sqrt(h2.expect_loadavg));
 
 for (j = 0; j < ATTR; j++)
    {
    printf("Hurst exponent for incoming %10s = %.1f\n",ECGSOCKS[j][1],sqrt(h2.expect_incoming[j]));
    printf("Hurst exponent for outgoing %10s = %.1f\n",ECGSOCKS[j][1],sqrt(h2.expect_outgoing[j]));   
    } 
}


/*****************************************************************************/

void DiskArrivals()

{ DIR *dirh;
  FILE *fp; 
  struct dirent *dirp;
  int count = 0, index = 0, i;
  char filename[CF_BUFSIZE],database[CF_BUFSIZE];
  double val, maxval = 1.0, *array, grain = 0.0;
  time_t now;
  DBT key,value;
  DB *dbp = NULL;
  DB_ENV *dbenv = NULL;


if ((array = (double *)malloc((int)CF_WEEK)) == NULL)
   {
   printf("Memory error");
   perror("malloc");
   return;
   }
  
if ((dirh = opendir(CFWORKDIR)) == NULL)
   {
   printf("Can't open directory %s\n",CFWORKDIR);
   perror("opendir");
   return;
   }

printf("\n\nLooking for filesystem arrival process data in %s\n",CFWORKDIR); 

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (strncmp(dirp->d_name,"scan:",5) == 0)
      {
      printf("Found %s - generating X,Y plot\n",dirp->d_name);

      snprintf(database,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,dirp->d_name);
      
      if ((errno = db_create(&dbp,dbenv,0)) != 0)
         {
         printf("Couldn't open arrivals database %s\n",database);
         return;
         }
      
#ifdef CF_OLD_DB
      if ((errno = dbp->open(dbp,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
          if ((errno = dbp->open(dbp,NULL,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
             {
             printf("Couldn't open database %s\n",database);
             dbp->close(dbp,0);
             continue;
             }
      
      maxval = 1.0;
      grain = 0.0;
      count = 0.0;
      index = 0;
      
      for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING+CF_WEEK; now += CF_MEASURE_INTERVAL)
         {
         memset(&key,0,sizeof(key));       
         memset(&value,0,sizeof(value));
         
         strcpy(TIMEKEY,GenTimeKey(now));
         
         key.data = TIMEKEY;
         key.size = strlen(TIMEKEY)+1;
         
         if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
            {
            if (errno != DB_NOTFOUND)
               {
               DBP->err(DBP,errno,NULL);
               exit(1);
               }
            }
         
         if (value.data != NULL)
            {
            grain += (double)*(double *)(value.data);
            }
         else
            {
            grain = 0;
            }
         
         if (HIRES)
            {
            if (grain > maxval)
               {
               maxval = grain;
               }
            
            array[index] = grain;
            grain = 0.0;     
            index++;
            }
         else
            {
            if (count % 12 == 0)
               {
               if (grain > maxval)
                  {
                  maxval = grain;
                  }
               array[index] = grain;
               index++;
               grain = 0.0;
               }
            }            
         count++;
         }
      
      dbp->close(dbp,0);
      
      snprintf(filename,CF_BUFSIZE-1,"%s.cfenv",dirp->d_name);
      
      if ((fp = fopen(filename,"w")) == NULL)
         {
         printf("Unable to open %s for writing\n",filename);
         perror("fopen");
         return;
         }
      
      printf("Data points = %d\n",index);
      
      for (i = 0; i < index; i++)
         {
         if (i > 1 && i < index-1)
            {
            val = (array[i-1]+array[i]+array[i+1])/3.0;  /* Smoothing */
            }
         else
            {
            val = array[i];
            }
         fprintf(fp,"%d %f\n",i,val/maxval*50.0);
         }
      
      fclose(fp);      
      }
   }
 
 closedir(dirh);
}


/*****************************************************************************/
/* Level 2                                                                   */
/*****************************************************************************/

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
  int optindex = 0;
  int c;

 /* XXX Initialize workdir for non privileged users */

 strcpy(CFWORKDIR,WORKDIR);

 if (geteuid() > 0)
    {
    char *homedir;
    if ((homedir = getenv("HOME")) != NULL)
       {
       strcpy(CFWORKDIR,homedir);
       strcat(CFWORKDIR,"/.cfagent");
       }
    }
 
snprintf(FILENAME,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_AVDB_FILE);

while ((c=getopt_long(argc,argv,"Thtf:rsen",GRAPHOPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 't': TITLES = true;
                break;

      case 'f': strcpy(FILENAME,optarg);
         break;

      case 'T': TIMESTAMPS = true;
         break;

      case 'r': HIRES = true;
         break;

      case 's': SEPARATE = true;
                break;

      case 'e': ERRORBARS = false;
                break;

      case 'n': NOSCALING = true;
         break;

      default:  Syntax();
                exit(1);

      }
   }
}

/*****************************************************************************/

void Syntax()

{ int i;

printf("Cfengine Environment Graph Generator\n%s\n%s\n",VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; GRAPHOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",GRAPHOPTIONS[i].name,(char)GRAPHOPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@gnu.org (News: gnu.cfengine.bug)\n");
printf("General help to help-cfengine@gnu.org (News: gnu.cfengine.help)\n");
printf("Info & fixes at http://www.iu.hio.no/cfengine\n");
}


/*********************************************************************/

char *CanonifyName(char *str)

{ static char buffer[CF_BUFSIZE];
  char *sp;

memset(buffer,0,CF_BUFSIZE);
strcpy(buffer,str);

for (sp = buffer; *sp != '\0'; sp++)
    {
    if (!isalnum((int)*sp) || *sp == '.')
       {
       *sp = '_';
       }
    }

return buffer;
}

/*********************************************************************/

struct Averages FindHurstFunction(int samples_per_grain,int grains)

/* Find the average of (max-min) over all intervals of width delta_t */

{ static struct Averages lmin,lmax,av;
 int control = 0;

if ((errno = db_create(&DBP,NULL,0)) != 0)
   {
   printf("Couldn't create average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((errno = DBP->open(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((errno = DBP->open(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)    
#endif
   {
   printf("Couldn't open average database %s\n",FILENAME);
   DBP->err(DBP,errno,NULL);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
memset(&av,0,sizeof(av)); 

lmax.expect_number_of_users = 0.01;
lmax.expect_rootprocs = 0.01;
lmax.expect_otherprocs = 0.01;
lmax.expect_diskfree = 0.01;
lmax.expect_loadavg = 0.01; 

lmin.expect_number_of_users = 9999.0;
lmin.expect_rootprocs = 9999.0;
lmin.expect_otherprocs = 9999.0;
lmin.expect_diskfree = 9999.0;
lmin.expect_loadavg = 9999.0; 

 
for (i = 0; i < ATTR; i++)
   {
   lmax.expect_incoming[i] = 0.01;
   lmax.expect_outgoing[i] = 0.01;
   lmin.expect_incoming[i] = 9999.0;
   lmin.expect_outgoing[i] = 9999.0;
   }

count = 0;
 
for (NOW = CF_MONDAY_MORNING; NOW < CF_MONDAY_MORNING+CF_WEEK; NOW += CF_MEASURE_INTERVAL)
   {
   memset(&key,0,sizeof(key));       
   memset(&value,0,sizeof(value));
   memset(&ENTRY,0,sizeof(ENTRY));

   strcpy(TIMEKEY,GenTimeKey(NOW));

   key.data = TIMEKEY;
   key.size = strlen(TIMEKEY)+1;
   
   if ((errno = DBP->get(DBP,NULL,&key,&value,0)) != 0)
      {
      if (errno != DB_NOTFOUND)
         {
         DBP->err(DBP,errno,NULL);
         exit(1);
         }
      }
   
   count++;
   
   if (value.data != NULL)
      {
      memcpy(&ENTRY,value.data,sizeof(ENTRY));
      
      if (false) /* This conformal scaling has no effect on the Hurst parameter expect div by zero errors! */
         {
         ENTRY.expect_number_of_users = ENTRY.expect_number_of_users/sqrt(ENTRY.var_number_of_users);
         ENTRY.expect_rootprocs = ENTRY.expect_rootprocs/sqrt(ENTRY.var_rootprocs);
         ENTRY.expect_otherprocs = ENTRY.expect_otherprocs/sqrt(ENTRY.var_otherprocs);
         ENTRY.expect_diskfree = ENTRY.expect_diskfree/sqrt(ENTRY.var_diskfree);
         ENTRY.expect_loadavg = ENTRY.expect_loadavg/sqrt(ENTRY.var_loadavg);
         
         for (i = 0; i < ATTR; i++)
            {
            ENTRY.expect_incoming[i] = ENTRY.expect_incoming[i]/sqrt(ENTRY.var_incoming[i]);
            ENTRY.expect_outgoing[i] = ENTRY.expect_outgoing[i]/sqrt(ENTRY.var_outgoing[i]);
            }  
         }
      
      if (fabs(ENTRY.expect_number_of_users) > lmax.expect_number_of_users)
         {
         lmax.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }
      if (fabs(ENTRY.expect_rootprocs) > lmax.expect_rootprocs)
         {
         lmax.expect_rootprocs = fabs(ENTRY.expect_rootprocs);
         }
      if (fabs(ENTRY.expect_otherprocs) >  lmax.expect_otherprocs)
         {
         lmax.expect_otherprocs = fabs(ENTRY.expect_otherprocs);
         }      
      if (fabs(ENTRY.expect_diskfree) > lmax.expect_diskfree)
         {
         lmax.expect_diskfree = fabs(ENTRY.expect_diskfree);
         }
      if (fabs(ENTRY.expect_loadavg) > lmax.expect_loadavg)
         {
         lmax.expect_loadavg = fabs(ENTRY.expect_loadavg);
         }
      
      for (i = 0; i < ATTR; i++)
         {
         if (fabs(ENTRY.expect_incoming[i]) > lmax.expect_incoming[i])
            {
            lmax.expect_incoming[i] = fabs(ENTRY.expect_incoming[i]);
            }
         if (fabs(ENTRY.expect_outgoing[i]) > lmax.expect_outgoing[i])
            {
            lmax.expect_outgoing[i] = fabs(ENTRY.expect_outgoing[i]);
            }
         }
      
      
      if (fabs(ENTRY.expect_number_of_users) < lmin.expect_number_of_users)
         {
         lmin.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }
      if (fabs(ENTRY.expect_number_of_users) < lmin.expect_number_of_users)
         {
         lmin.expect_number_of_users = fabs(ENTRY.expect_number_of_users);
         }      
      if (fabs(ENTRY.expect_rootprocs) < lmin.expect_rootprocs)
         {
         lmin.expect_rootprocs = fabs(ENTRY.expect_rootprocs);
         }
      if (fabs(ENTRY.expect_otherprocs) < lmin.expect_otherprocs)
         {
         lmin.expect_otherprocs = fabs(ENTRY.expect_otherprocs);
         }      
      if (fabs(ENTRY.expect_diskfree) < lmin.expect_diskfree)
         {
         lmin.expect_diskfree = fabs(ENTRY.expect_diskfree);
         }
      if (fabs(ENTRY.expect_loadavg) < lmin.expect_loadavg)
         {
         lmin.expect_loadavg = fabs(ENTRY.expect_loadavg);
         }
      
      for (i = 0; i < ATTR; i++)
         {
         if (fabs(ENTRY.expect_incoming[i]) < lmin.expect_incoming[i])
            {
            lmin.expect_incoming[i] = fabs(ENTRY.expect_incoming[i]);
            }
         if (fabs(ENTRY.expect_outgoing[i]) < lmin.expect_outgoing[i])
            {
            lmin.expect_outgoing[i] = fabs(ENTRY.expect_outgoing[i]);
            }
         }      
      }
   
   /* For each grain, find the difference of the max and min values for final average */
   
   if (count == samples_per_grain)
      {
      count = 0;
      control += samples_per_grain;
      
      /* av += lmax - lmin; */
      
      av.expect_number_of_users += (lmax.expect_number_of_users - lmin.expect_number_of_users)/(double)grains;
      av.expect_rootprocs += (lmax.expect_rootprocs - lmin.expect_rootprocs)/(double)grains;
      av.expect_otherprocs += (lmax.expect_otherprocs - lmin.expect_otherprocs)/(double)grains;
      av.expect_diskfree += (lmax.expect_diskfree - lmin.expect_diskfree)/(double)grains;
      av.expect_loadavg += (lmax.expect_loadavg - lmin.expect_loadavg)/(double)grains;
      
      for (i = 0; i < ATTR; i++)
         {
         av.expect_incoming[i] = (lmax.expect_incoming[i] - lmin.expect_incoming[i])/(double)grains;
         av.expect_outgoing[i] = (lmax.expect_outgoing[i] - lmin.expect_outgoing[i])/(double)grains;
         }      
      
      lmax.expect_number_of_users = 0.01;
      lmax.expect_rootprocs = 0.01;
      lmax.expect_otherprocs = 0.01;
      lmax.expect_diskfree = 0.01;
      lmax.expect_loadavg = 0.01; 
      
      lmin.expect_number_of_users = 9999.0;
      lmin.expect_rootprocs = 9999.0;
      lmin.expect_otherprocs = 9999.0;
      lmin.expect_diskfree = 9999.0;
      lmin.expect_loadavg = 9999.0; 
      
      for (i = 0; i < ATTR; i++)
         {
         lmax.expect_incoming[i] = 0.01;
         lmax.expect_outgoing[i] = 0.01;
         lmin.expect_incoming[i] = 9999.0;
         lmin.expect_outgoing[i] = 9999.0;
         }
      }
   }
 
 printf("Scanned %d grains of size %d for Hurst function\n",control,samples_per_grain); 
 
DBP->close(DBP,0);
return(av);
}
