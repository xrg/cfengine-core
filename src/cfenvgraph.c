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

void CheckOpts(int argc, char **argv);
void Syntax(void);
void ReadAverages(void);
void SummarizeAverages(void);
void WriteGraphFiles(void);
void WriteHistograms(void);
void DiskArrivals(void);
void GetFQHN(void);
void OpenFiles(void);
void CloseFiles(void);

/*****************************************************************************/

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
unsigned int HISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS];
int SMOOTHHISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS];
char VFQNAME[CF_MAXVARSIZE];
int ERRNO;
time_t NOW;

DB *DBP;
static struct Averages ENTRY,MAX,MIN,DET;
char TIMEKEY[CF_SMALLBUF],FLNAME[CF_BUFSIZE],*sp;
double AGE;
FILE *FPAV=NULL,*FPVAR=NULL, *FPNOW=NULL;
FILE *FPE[CF_OBSERVABLES],*FPQ[CF_OBSERVABLES];

/*****************************************************************************/

int main (int argc,char **argv)

{
CheckOpts(argc,argv);
GetFQHN();
ReadAverages(); 
SummarizeAverages();
WriteGraphFiles();
WriteHistograms();
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

{ int i;
  DBT key,value;

printf("\nLooking for database %s\n",FILENAME);
printf("\nFinding MAXimum values...\n\n");
printf("N.B. socket values are numbers in CLOSE_WAIT. See documentation.\n"); 
  
if ((ERRNO = db_create(&DBP,NULL,0)) != 0)
   {
   printf("Couldn't create average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((ERRNO = DBP->open(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((ERRNO = DBP->open(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)    
#endif
   {
   printf("Couldn't open average database %s\n",FILENAME);
   DBP->err(DBP,ERRNO,NULL);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));

 for (i = 0; i < CF_OBSERVABLES; i++)
   {
   MAX.Q[i].var = MAX.Q[i].expect = MAX.Q[i].q = 0.01;
   MIN.Q[i].var = MIN.Q[i].expect = MIN.Q[i].q = 9999.0;
   FPE[i] = FPQ[i] = NULL;
   }
 
for (NOW = CF_MONDAY_MORNING; NOW < CF_MONDAY_MORNING+CF_WEEK; NOW += CF_MEASURE_INTERVAL)
   {
   memset(&key,0,sizeof(key));       
   memset(&value,0,sizeof(value));
   memset(&ENTRY,0,sizeof(ENTRY));

   strcpy(TIMEKEY,GenTimeKey(NOW));

   key.data = TIMEKEY;
   key.size = strlen(TIMEKEY)+1;
   
   if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
      {
      if (ERRNO != DB_NOTFOUND)
         {
         DBP->err(DBP,ERRNO,NULL);
         exit(1);
         }
      }
   
   if (value.data != NULL)
      {
      memcpy(&ENTRY,value.data,sizeof(ENTRY));
      
      for (i = 0; i < CF_OBSERVABLES; i++)
         {
         if (fabs(ENTRY.Q[i].expect) > MAX.Q[i].expect)
            {
            MAX.Q[i].expect = fabs(ENTRY.Q[i].expect);
            }

         if (fabs(ENTRY.Q[i].q) > MAX.Q[i].q)
            {
            MAX.Q[i].q = fabs(ENTRY.Q[i].q);
            }

         if (fabs(ENTRY.Q[i].expect) < MIN.Q[i].expect)
            {
            MIN.Q[i].expect = fabs(ENTRY.Q[i].expect);
            }
         
         if (fabs(ENTRY.Q[i].q) < MIN.Q[i].q)
            {
            MIN.Q[i].q = fabs(ENTRY.Q[i].q);
            }
         }
      }
   }
 
 DBP->close(DBP,0);
}

/*****************************************************************************/

void SummarizeAverages()

{ int i;
  DBT key,value;

printf(" x  yN (Variable content)\n---------------------------------------------------------\n");

 for (i = 0; i < CF_OBSERVABLES; i++)
   {
   printf("%2d. MAX <%-10s-in>   = %10f - %10f u %10f\n",i,OBS[i],MIN.Q[i].expect,MAX.Q[i].expect,sqrt(MAX.Q[i].var));
   }
 
if ((ERRNO = db_create(&DBP,NULL,0)) != 0)
   {
   printf("Couldn't open average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((ERRNO = DBP->open(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((ERRNO = DBP->open(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#endif
   {
   printf("Couldn't open average database %s\n",FILENAME);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
      
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE")+1;

if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
   {
   if (ERRNO != DB_NOTFOUND)
      {
      DBP->err(DBP,ERRNO,NULL);
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

{ int its,i,j,k, count = 0;
  DBT key,value;

if (TIMESTAMPS)
   {
   if ((NOW = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n");
      }
     
   sprintf(FLNAME,"cfenvgraphs-%s-%s",CanonifyName(VFQNAME),ctime(&NOW));
   }
else
   {
   sprintf(FLNAME,"cfenvgraphs-snapshot-%s",CanonifyName(VFQNAME));
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

OpenFiles();

if (TITLES)
   {
   for (i = 0; i < CF_OBSERVABLES; i+=2)
      {
      fprintf(FPAV,"# Column %d: %s\n",i,OBS[i]);
      fprintf(FPVAR,"# Column %d: %s\n",i,OBS[i]);
      fprintf(FPNOW,"# Column %d: %s\n",i,OBS[i]);
      }

   fprintf(FPAV,"##############################################\n");
   fprintf(FPVAR,"##############################################\n");
   fprintf(FPNOW,"##############################################\n");
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

      if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
         {
         if (ERRNO != DB_NOTFOUND)
            {
            DBP->err(DBP,ERRNO,NULL);
            exit(1);
            }
         }

      /* Work out local average over grain size "its" */
      
      if (value.data != NULL)
         {
         memcpy(&DET,value.data,sizeof(DET));
         
         for (i = 0; i < CF_OBSERVABLES; i++)
            {
            ENTRY.Q[i].expect += DET.Q[i].expect/(double)its;
            ENTRY.Q[i].var += DET.Q[i].var/(double)its;
            ENTRY.Q[i].q += DET.Q[i].q/(double)its;
            }         
         
         if (NOSCALING)
            {            
            for (i = 1; i < CF_OBSERVABLES; i++)
               {
               MAX.Q[i].expect = 1;
               MAX.Q[i].q = 1;
               }
            }
         }
      
      NOW += CF_MEASURE_INTERVAL;
      }

   /* Output the data in a plethora of files */
   
   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      fprintf(FPAV,"%f ",ENTRY.Q[i].expect/MAX.Q[i].expect);
      fprintf(FPVAR,"%f ",ENTRY.Q[i].var/MAX.Q[i].var);
      fprintf(FPNOW,"%f ",ENTRY.Q[i].q/MAX.Q[i].q);
      }                        
   
   fprintf(FPAV,"\n");
   fprintf(FPVAR,"\n");
   fprintf(FPNOW,"\n");
   
   if (SEPARATE)
      {
      for (i = 0; i < CF_OBSERVABLES; i++)
         {
         fprintf(FPE[i],"%d %f %f\n",count++, ENTRY.Q[i].expect/MAX.Q[i].expect, sqrt(ENTRY.Q[i].var)/MAX.Q[i].expect);
         /* Use same scaling for Q so graphs can be merged */
         fprintf(FPQ[i],"%d %f 0.0\n",count++, ENTRY.Q[i].q/MAX.Q[i].expect);
         }               
      }
   
   memset(&ENTRY,0,sizeof(ENTRY)); 
   }

DBP->close(DBP,0);

CloseFiles();
}

/*****************************************************************************/

void WriteHistograms()

{ int i,j,k;
 
/* Finally, look at the histograms */
 
 for (i = 0; i < 7; i++)
    {
    for (j = 0; j < CF_OBSERVABLES; j++)
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
    int weekly[CF_OBSERVABLES][CF_GRAINS];
    FILE *fp;
    
    snprintf(FLNAME,CF_BUFSIZE,"%s/state/histograms",CFWORKDIR);
    
    if ((fp = fopen(FLNAME,"r")) == NULL)
       {
       printf("Unable to load histogram data\n");
       exit(1);
       }
    
    for (position = 0; position < CF_GRAINS; position++)
       {
       fscanf(fp,"%d ",&position);
       
       for (i = 0; i < CF_OBSERVABLES; i++)
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
          for (j = 0; j < CF_OBSERVABLES; j++)
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
          for (j = 0; j < CF_OBSERVABLES; j++)
             {
             for (i = 0; i < 7; i++)  
                {
                SMOOTHHISTOGRAM[j][i][k] = (double) HISTOGRAM[j][i][k];
                }
             }
          }
       }
    
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       sprintf(FLNAME,"%s.distr",OBS[i]); 
       if ((FPQ[i] = fopen(FLNAME,"w")) == NULL)
          {
          perror("fopen");
          exit(1);
          }
       }
    
    /* Plot daily and weekly histograms */
    for (k = 0; k < CF_GRAINS; k++)
       {
       int a;
       
       for (j = 0; j < CF_OBSERVABLES; j++)
          {
          for (i = 0; i < 7; i++)  
             {
             weekly[j][k] += (int) (SMOOTHHISTOGRAM[j][i][k]+0.5);
             }
          }
       
       for (a = 0; a < CF_OBSERVABLES; a++)
          {
          fprintf(FPQ[a],"%d %d\n",k,weekly[a][k]);
          }
       }
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       fclose(FPQ[i]);
       }
    }
}


/*****************************************************************************/

void DiskArrivals(void)

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
      
      if ((ERRNO = db_create(&dbp,dbenv,0)) != 0)
         {
         printf("Couldn't open arrivals database %s\n",database);
         return;
         }
      
#ifdef CF_OLD_DB
      if ((ERRNO = dbp->open(dbp,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
      if ((ERRNO = dbp->open(dbp,NULL,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
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
         
         if ((ERRNO = dbp->get(dbp,NULL,&key,&value,0)) != 0)
            {
            if (ERRNO != DB_NOTFOUND)
               {
               DBP->err(DBP,ERRNO,NULL);
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

void OpenFiles()

{ int i;
 
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

sprintf(FLNAME,"cfenv-now"); 

if ((FPNOW = fopen(FLNAME,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }


/* Now if -s open a file foreach metric! */

if (SEPARATE)
   {
   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      sprintf(FLNAME,"%s.E-sigma",OBS[i]);
      
      if ((FPE[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      
      sprintf(FLNAME,"%s.q",OBS[i]);
      
      if ((FPQ[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }

      }
   }
}

/*********************************************************************/

void CloseFiles()

{ int i;
 
fclose(FPAV);
fclose(FPVAR);
fclose(FPNOW); 

if (SEPARATE)
   {
   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      fclose(FPE[i]);
      fclose(FPQ[i]);
      }
   }
}
