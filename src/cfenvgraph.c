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

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"
#include <math.h>
#include <db.h>

/*****************************************************************************/
/* Prototypes                                                                */
/*****************************************************************************/

int main ARGLIST((int argc, char **argv));
void CheckOpts ARGLIST((int argc, char **argv));
void Syntax ARGLIST((void));
void ReadAverages ARGLIST((void));
void SummarizeAverages ARGLIST((void));
void WriteGraphFiles ARGLIST((void));
void WriteHistograms ARGLIST((void));

/*****************************************************************************/

char *COPYRIGHT = "Free Software Foundation 2001-\nDonated by Mark Burgess, Faculty of Engineering,\nOslo University College, 0254 Oslo, Norway";

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
char FILENAME[bufsize];
unsigned int HISTOGRAM[ATTR*2+5+PH_LIMIT][7][GRAINS];
int SMOOTHHISTOGRAM[ATTR*2+5+PH_LIMIT][7][GRAINS];

/*****************************************************************************/

char *ECGSOCKS[ATTR][2] =
   {
   {".137","netbiosns"},
   {".138","netbiosdgm"},
   {".139","netbiosssn"},
   {".194","irc"},
   {".5308","cfengine"},
   {".2049","nfsd"},
   {".25","smtp"},
   {".80","www"},
   {".21","ftp"},
   {".22","ssh"},
   {".23","telnet"},
   };

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
static struct Averages ENTRY,MAX,DET;
char TIMEKEY[64],FLNAME[256],*sp;
double AGE;
FILE *FPAV=NULL,*FPVAR=NULL,*FPROOT=NULL,*FPUSER=NULL,*FPOTHER=NULL;
FILE *FPDISK=NULL,*FPLOAD=NULL,*FPIN[ATTR],*FPOUT[ATTR],*FPPH[PH_LIMIT],*fp;

/*****************************************************************************/

int main (argc,argv)

int argc;
char **argv;

{
CheckOpts(argc,argv);
ReadAverages(); 
SummarizeAverages();
WriteGraphFiles();
WriteHistograms();
return 0;
}

/*****************************************************************************/
/* Level 1                                                                   */
/*****************************************************************************/

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

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));

MAX.expect_number_of_users = 0.1;
MAX.expect_rootprocs = 0.1;
MAX.expect_otherprocs = 0.1;
MAX.expect_diskfree = 0.1;
MAX.expect_loadavg = 0.1; 

MAX.var_number_of_users = 0.1;
MAX.var_rootprocs = 0.1;
MAX.var_otherprocs = 0.1;
MAX.var_diskfree = 0.1;
MAX.var_loadavg = 0.1; 

for (i = 0; i < ATTR; i++)
   {
   MAX.var_incoming[i] = 0.1;
   MAX.var_outgoing[i] = 0.1;
   MAX.expect_incoming[i] = 0.1;
   MAX.expect_outgoing[i] = 0.1;
   }

for (NOW = cf_monday_morning; NOW < cf_monday_morning+CFWEEK; NOW += MEASURE_INTERVAL)
   {
   bzero(&key,sizeof(key));       
   bzero(&value,sizeof(value));
   bzero(&ENTRY,sizeof(ENTRY));

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
      bcopy(value.data,&ENTRY,sizeof(ENTRY));
      
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
	 MAX.expect_diskfree = fabs(ENTRY.expect_loadavg);
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
printf(" 1. MAX <number of users> = %10f +/- %10f\n",MAX.expect_number_of_users,sqrt(MAX.var_number_of_users));
printf(" 2. MAX <rootprocs>       = %10f +/- %10f\n",MAX.expect_rootprocs,sqrt(MAX.var_rootprocs));
printf(" 3. MAX <otherprocs>      = %10f +/- %10f\n",MAX.expect_otherprocs,sqrt(MAX.var_otherprocs));
printf(" 4. MAX <diskfree>        = %10f +/- %10f\n",MAX.expect_diskfree,sqrt(MAX.var_diskfree));
printf(" 5. MAX <loadavg>         = %10f +/- %10f\n",MAX.expect_loadavg,sqrt(MAX.var_loadavg)); 

 for (i = 0; i < ATTR*2; i+=2)
   {
   printf("%2d. MAX <%-10s-in>   = %10f +/- %10f\n",6+i,ECGSOCKS[i/2][1],MAX.expect_incoming[i/2],sqrt(MAX.var_incoming[i/2]));
   printf("%2d. MAX <%-10s-out>  = %10f +/- %10f\n",7+i,ECGSOCKS[i/2][1],MAX.expect_outgoing[i/2],sqrt(MAX.var_outgoing[i/2]));
   }

 for (i = 0; i < PH_LIMIT; i++)
   {
   if (PH_BINARIES[i] == NULL)
      {
      continue;
      }
   printf("%2d. MAX <%-10s-in>   = %10f +/- %10f\n",i+5+ATTR,PH_BINARIES[i],MAX.expect_pH[i],sqrt(MAX.var_pH[i]));
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

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));
      
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
   printf("\n\nDATABASE_AGE %.1f (weeks)\n\n",AGE/CFWEEK*MEASURE_INTERVAL);
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
     
   sprintf(FLNAME,"cfenvgraphs-%s",ctime(&NOW));

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
   sprintf(FLNAME,"cfenvgraphs-snapshot");
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

NOW = cf_monday_morning;
bzero(&ENTRY,sizeof(ENTRY)); 
 
while (NOW < cf_monday_morning+CFWEEK)
   {
   for (j = 0; j < its; j++)
      {
      bzero(&key,sizeof(key));       
      bzero(&value,sizeof(value));
      
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
	 bcopy(value.data,&DET,sizeof(DET));
	 
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

               for (i = 0; i < PH_LIMIT; i++)
		  {
		  if (PH_BINARIES[i] == NULL)
		     {
		     continue;
		     }
                  fprintf(FPPH[i],"%d %f %f\n",count,ENTRY.expect_pH[i]/MAX.expect_pH[i],sqrt(ENTRY.var_pH[i])/MAX.expect_pH[i]);
		  }	       
	       }
 
	    bzero(&ENTRY,sizeof(ENTRY)); 
	    }
	 }
      
      NOW += MEASURE_INTERVAL;
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
   for (j = 0; j < PH_LIMIT+ATTR*2+5; j++)
      {
      for (k = 0; k < GRAINS; k++)
	  {
	  HISTOGRAM[j][i][k] = 0;
	  }
      }
   }

if (SEPARATE)
   {
   int position,day;
   int weekly[ATTR*2+5][GRAINS];
   
   snprintf(FLNAME,bufsize,"%s/histograms",WORKDIR);
   
   if ((fp = fopen(FLNAME,"r")) == NULL)
      {
      printf("Unable to load histogram data\n");
      exit(1);
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

	 weekly[i][position] = 0;
	 }
      }
   
   fclose(fp);

   if (!HIRES)
      {
      /* Smooth daily and weekly histograms */
      for (k = 1; k < GRAINS-1; k++)
	 {
	 int a;
	 
	 for (j = 0; j < ATTR*2+5+PH_LIMIT; j++)
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
      for (k = 1; k < GRAINS-1; k++)
	 {
	 int a;
	 
	 for (j = 0; j < ATTR*2+5+PH_LIMIT; j++)
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

   for (i = 0; i < PH_LIMIT; i++)
      {
      if (PH_BINARIES[i] == NULL)
	 {
	 continue;
	 }

      sprintf(FLNAME,"%s.distr",CanonifyName(PH_BINARIES[i])); 
      if ((FPOUT[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      }

   /* Plot daily and weekly histograms */
   for (k = 0; k < GRAINS; k++)
      {
      int a;
      
      for (j = 0; j < ATTR*2+5+PH_LIMIT; j++)
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

      for (a = 0; a < PH_LIMIT; a++)
	 {
	 if (PH_BINARIES[a] == NULL)
	    {
	    continue;
	    }
	 fprintf(FPIN[a],"%d %d\n",k,weekly[5+ATTR+a][k]);
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

void CheckOpts(argc,argv)

char **argv;
int argc;

{ extern char *optarg;
  int optindex = 0;
  int c;

snprintf(FILENAME,bufsize,"%s/%s",WORKDIR,AVDB_FILE);

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

char *CanonifyName(str)

char *str;

{ static char buffer[bufsize];
  char *sp;

bzero(buffer,bufsize);
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
