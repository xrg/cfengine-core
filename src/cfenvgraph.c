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
/* Author: Mark                                      >                       */
/*                                                                           */
/* Revision: $Id$                                                            */
/*                                                                           */
/* Description:                                                              */
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
int HISTOGRAM[ATTR*2+4][7][GRAINS];

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

/*****************************************************************************/

int main (argc,argv)

int argc;
char **argv;

{ int errno,i,j,k,count=0, its;
  time_t now; 
  DBT key,value;
  DB *dbp;
  static struct Averages entry,max,det;
  char timekey[64],filename[256],*sp;
  double age;
  FILE *fpav=NULL,*fpvar=NULL,*fproot=NULL,*fpuser=NULL,*fpother=NULL;
  FILE *fpdisk=NULL,*fpin[ATTR],*fpout[ATTR],*fp;

CheckOpts(argc,argv);

printf("\nLooking for database %s\n",FILENAME);
printf("\nFinding maximum values...\n\n");
printf("N.B. socket values are numbers in CLOSE_WAIT. See documentation.\n"); 
  
if ((errno = db_create(&dbp,NULL,0)) != 0)
   {
   printf("Couldn't create average database %s\n",FILENAME);
   return 1;
   }
 
if ((errno = dbp->open(dbp,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
   {
   printf("Couldn't open average database %s\n",FILENAME);
   dbp->err(dbp,errno,NULL);
   return 1;
   }

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));

max.expect_number_of_users = 0.1;
max.expect_rootprocs = 0.1;
max.expect_otherprocs = 0.1;
max.expect_diskfree = 0.1;

max.var_number_of_users = 0.1;
max.var_rootprocs = 0.1;
max.var_otherprocs = 0.1;
max.var_diskfree = 0.1;

for (i = 0; i < ATTR; i++)
   {
   max.var_incoming[i] = 0.1;
   max.var_outgoing[i] = 0.1;
   max.expect_incoming[i] = 0.1;
   max.expect_outgoing[i] = 0.1;
   }

for (now = cf_monday_morning; now < cf_monday_morning+CFWEEK; now += MEASURE_INTERVAL)
   {
   bzero(&key,sizeof(key));       
   bzero(&value,sizeof(value));
   bzero(&entry,sizeof(entry));

   strcpy(timekey,GenTimeKey(now));

   key.data = timekey;
   key.size = strlen(timekey)+1;
   
   if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
      {
      if (errno != DB_NOTFOUND)
	 {
	 dbp->err(dbp,errno,NULL);
	 return 1;
	 }
      }
   
   
   if (value.data != NULL)
      {
      bcopy(value.data,&entry,sizeof(entry));
      
      if (fabs(entry.expect_number_of_users) > max.expect_number_of_users)
	 {
	 max.expect_number_of_users = fabs(entry.expect_number_of_users);
	 }
      if (fabs(entry.expect_number_of_users) > max.expect_number_of_users)
	 {
	 max.expect_number_of_users = fabs(entry.expect_number_of_users);
	 }      
      if (fabs(entry.expect_rootprocs) > max.expect_rootprocs)
	 {
	 max.expect_rootprocs = fabs(entry.expect_rootprocs);
	 }
      if (fabs(entry.expect_otherprocs) >  max.expect_otherprocs)
	 {
	 max.expect_otherprocs = fabs(entry.expect_otherprocs);
	 }      
      if (fabs(entry.expect_diskfree) > max.expect_diskfree)
	 {
	 max.expect_diskfree = fabs(entry.expect_diskfree);
	 }
      
      for (i = 0; i < ATTR; i++)
	 {
	 if (fabs(entry.expect_incoming[i]) > max.expect_incoming[i])
	    {
	    max.expect_incoming[i] = fabs(entry.expect_incoming[i]);
	    }
	 if (fabs(entry.expect_outgoing[i]) > max.expect_outgoing[i])
	    {
	    max.expect_outgoing[i] = fabs(entry.expect_outgoing[i]);
	    }
	 }

      if (fabs(entry.var_number_of_users) > max.var_number_of_users)
	 {
	 max.var_number_of_users = fabs(entry.var_number_of_users);
	 }
      if (fabs(entry.var_number_of_users) > max.var_number_of_users)
	 {
	 max.var_number_of_users = fabs(entry.var_number_of_users);
	 }      
      if (fabs(entry.var_rootprocs) > max.var_rootprocs)
	 {
	 max.var_rootprocs = fabs(entry.var_rootprocs);
	 }
      if (fabs(entry.var_otherprocs) >  max.var_otherprocs)
	 {
	 max.var_otherprocs = fabs(entry.var_otherprocs);
	 }      
      if (fabs(entry.var_diskfree) > max.var_diskfree)
	 {
	 max.var_diskfree = fabs(entry.var_diskfree);
	 }
      
      for (i = 0; i < ATTR; i++)
	 {
	 if (fabs(entry.var_incoming[i]) > max.var_incoming[i])
	    {
	    max.var_incoming[i] = fabs(entry.var_incoming[i]);
	    }
	 if (fabs(entry.var_outgoing[i]) > max.var_outgoing[i])
	    {
	    max.var_outgoing[i] = fabs(entry.var_outgoing[i]);
	    }
	 }

      }
   }

dbp->close(dbp,0);
printf(" x  yN (Variable content)\n---------------------------------------------------------\n");
printf(" 1. Max <number of users> = %10f +/- %10f\n",max.expect_number_of_users,sqrt(max.var_number_of_users));
printf(" 2. Max <rootprocs>       = %10f +/- %10f\n",max.expect_rootprocs,sqrt(max.var_rootprocs));
printf(" 3. Max <otherprocs>      = %10f +/- %10f\n",max.expect_otherprocs,sqrt(max.var_otherprocs));
printf(" 4. Max <diskfree>        = %10f +/- %10f\n",max.expect_diskfree,sqrt(max.var_diskfree));

 for (i = 0; i < ATTR*2; i+=2)
   {
   printf("%2d. Max <%-10s-in>   = %10f +/- %10f\n",5+i,ECGSOCKS[i/2][1],max.expect_incoming[i/2],sqrt(max.var_incoming[i/2]));
   printf("%2d. Max <%-10s-out>  = %10f +/- %10f\n",6+i,ECGSOCKS[i/2][1],max.expect_outgoing[i/2],sqrt(max.var_outgoing[i/2]));
   }

if ((errno = db_create(&dbp,NULL,0)) != 0)
   {
   printf("Couldn't open average database %s\n",FILENAME);
   return 1;
   }
 
if ((errno = dbp->open(dbp,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
   {
   printf("Couldn't open average database %s\n",FILENAME);
   return 1;
   }

bzero(&key,sizeof(key));       
bzero(&value,sizeof(value));
      
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE")+1;

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp,errno,NULL);
      return 1;
      }
   }
 
if (value.data != NULL)
   {
   age = *(double *)(value.data);
   printf("\n\nDATABASE_AGE %.1f (weeks)\n\n",age/CFWEEK*MEASURE_INTERVAL);
   }

if (TIMESTAMPS)
   {
   if ((now = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n");
      }
     
   sprintf(filename,"cfenvgraphs-%s",ctime(&now));

   for (sp = filename; *sp != '\0'; sp++)
      {
      if (isspace((int)*sp))
         {
         *sp = '_';
         }
      }    
   }
 else
   {
   sprintf(filename,"cfenvgraphs-snapshot");
   }

printf("Creating sub-directory %s\n",filename);

if (mkdir(filename,0755) == -1)
   {
   perror("mkdir");
   printf("Aborting\n");
   exit(0);
   }
 
if (chdir(filename))
   {
   perror("chdir");
   exit(0);
   }


printf("Writing data to sub-directory %s: \n   x,y1,y2,y3...\n ",filename);


sprintf(filename,"cfenv-average");

if ((fpav = fopen(filename,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }

sprintf(filename,"cfenv-stddev"); 

if ((fpvar = fopen(filename,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }


/* Now if -s open a file foreach metric! */

if (SEPARATE)
   {
   sprintf(filename,"users.cfenv"); 
   if ((fpuser = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(filename,"rootprocs.cfenv"); 
   if ((fproot = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(filename,"otherprocs.cfenv"); 
   if ((fpother = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(filename,"freedisk.cfenv"); 
   if ((fpdisk = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }

   for (i = 0; i < ATTR; i++)
      {
      sprintf(filename,"%s-in.cfenv",ECGSOCKS[i][1]); 
      if ((fpin[i] = fopen(filename,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }

      sprintf(filename,"%s-out.cfenv",ECGSOCKS[i][1]); 
      if ((fpout[i] = fopen(filename,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      }
   }

if (TITLES)
   {
   fprintf(fpav,"# Column 1: Users\n");
   fprintf(fpav,"# Column 2: Root Processes\n");
   fprintf(fpav,"# Column 3: Non-root Processes 3\n");
   fprintf(fpav,"# Column 4: Percent free disk\n");
     
   for (i = 0; i < ATTR*2; i+=2)
      {
      fprintf(fpav,"# Column %d: Incoming %s sockets\n",5+i,ECGSOCKS[i/2][1]);
      fprintf(fpav,"# Column %d: Outgoing %s sockets\n",6+i,ECGSOCKS[i/2][1]);
      }
   fprintf(fpav,"##############################################\n");
     
   fprintf(fpvar,"# Column 1: Users\n");
   fprintf(fpvar,"# Column 2: Root Processes\n");
   fprintf(fpvar,"# Column 3: Non-root Processes 3\n");
   fprintf(fpvar,"# Column 4: Percent free disk\n");
     
   for (i = 0; i < ATTR*2; i+=2)
      {
      fprintf(fpvar,"# Column %d: Incoming %s sockets\n",5+i,ECGSOCKS[i/2][1]);
      fprintf(fpvar,"# Column %d: Outgoing %s sockets\n",6+i,ECGSOCKS[i/2][1]);
      }
   fprintf(fpvar,"##############################################\n");
   }

if (HIRES)
   {
   its = 1;
   }
else
   {
   its = 12;
   }

now = cf_monday_morning;
bzero(&entry,sizeof(entry)); 
 
while (now < cf_monday_morning+CFWEEK)
   {
   for (j = 0; j < its; j++)
      {
      bzero(&key,sizeof(key));       
      bzero(&value,sizeof(value));
      
      strcpy(timekey,GenTimeKey(now));
      key.data = timekey;
      key.size = strlen(timekey)+1;
      
      if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
	 {
	 if (errno != DB_NOTFOUND)
	    {
	    dbp->err(dbp,errno,NULL);
	    return 1;
	    }
	 }
      
      if (value.data != NULL)
	 {
	 bcopy(value.data,&det,sizeof(det));
	 
	 entry.expect_number_of_users += det.expect_number_of_users/(double)its;
	 entry.expect_rootprocs += det.expect_rootprocs/(double)its;
	 entry.expect_otherprocs += det.expect_otherprocs/(double)its;
	 entry.expect_diskfree += det.expect_diskfree/(double)its;
	 entry.var_number_of_users += det.var_number_of_users/(double)its;
	 entry.var_rootprocs += det.var_rootprocs/(double)its;
	 entry.var_otherprocs += det.var_otherprocs/(double)its;
	 entry.var_diskfree += det.var_diskfree/(double)its;

	 for (i = 0; i < ATTR; i++)
	    {
	    entry.expect_incoming[i] += det.expect_incoming[i]/(double)its;
	    entry.expect_outgoing[i] += det.expect_outgoing[i]/(double)its;
	    entry.var_incoming[i] += det.var_incoming[i]/(double)its;
	    entry.var_outgoing[i] += det.var_outgoing[i]/(double)its;
	    }


	 if (NOSCALING)
	    {
	    max.expect_number_of_users = 1;
	    max.expect_rootprocs = 1;
	    max.expect_otherprocs = 1;
	    max.expect_diskfree = 1;

            for (i = 1; i < ATTR; i++)
	      {
	      max.expect_incoming[i] = 1;
	      max.expect_outgoing[i] = 1;
              }
            }
	 
	 if (j == its-1)
	    {
	    fprintf(fpav,"%d %f %f %f %f ",count++,
		 entry.expect_number_of_users/max.expect_number_of_users,
		 entry.expect_rootprocs/max.expect_rootprocs,
		 entry.expect_otherprocs/max.expect_otherprocs,
		 entry.expect_diskfree/max.expect_diskfree);
	 
	    for (i = 0; i < ATTR; i++)
	       {
	       fprintf(fpav,"%f %f "
		       ,entry.expect_incoming[i]/max.expect_incoming[i]
		       ,entry.expect_outgoing[i]/max.expect_outgoing[i]);
	       }
	    
	    fprintf(fpav,"\n");
	    
	    fprintf(fpvar,"%d %f %f %f %f ",count,
		    sqrt(entry.var_number_of_users)/max.expect_number_of_users,
		    sqrt(entry.var_rootprocs)/max.expect_rootprocs,
		    sqrt(entry.var_otherprocs)/max.expect_otherprocs,
		    sqrt(entry.var_diskfree)/max.expect_diskfree);
	    
	    for (i = 0; i < ATTR; i++)
	       {
	       fprintf(fpvar,"%f %f ",
		       sqrt(entry.var_incoming[i])/max.expect_incoming[i],
		       sqrt(entry.var_outgoing[i])/max.expect_outgoing[i]);
	       }
	    
	    fprintf(fpvar,"\n");

            if (SEPARATE)
	       {
               fprintf(fpuser,"%d %f %f\n",count,entry.expect_number_of_users/max.expect_number_of_users,sqrt(entry.var_number_of_users)/max.expect_number_of_users);
               fprintf(fproot,"%d %f %f\n",count,entry.expect_rootprocs/max.expect_rootprocs,sqrt(entry.var_rootprocs)/max.expect_rootprocs);
               fprintf(fpother,"%d %f %f\n",count,entry.expect_otherprocs/max.expect_otherprocs,sqrt(entry.var_otherprocs)/max.expect_otherprocs);
               fprintf(fpdisk,"%d %f %f\n",count,entry.expect_diskfree/max.expect_diskfree,sqrt(entry.var_diskfree)/max.expect_diskfree);

               for (i = 0; i < ATTR; i++)
		  {
                  fprintf(fpin[i],"%d %f %f\n",count,entry.expect_incoming[i]/max.expect_incoming[i],sqrt(entry.var_incoming[i])/max.expect_incoming[i]);
                  fprintf(fpout[i],"%d %f %f\n",count,entry.expect_outgoing[i]/max.expect_outgoing[i],sqrt(entry.var_outgoing[i])/max.expect_outgoing[i]);
		  }
	       }
 
	    bzero(&entry,sizeof(entry)); 
	    }
	 }
      
      now += MEASURE_INTERVAL;
      }
   }
 
dbp->close(dbp,0);

fclose(fpav);
fclose(fpvar); 

if (SEPARATE)
   {
   fclose(fproot);
   fclose(fpother);
   fclose(fpuser);
   fclose(fpdisk);
   for (i = 0; i < ATTR; i++)
      {
      fclose(fpin[i]);
      fclose(fpout[i]);
      }
   }

/* Finally, look at the histograms */

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < ATTR*2+4; j++)
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
   int weekly[ATTR*2+4][GRAINS];
   
   snprintf(filename,bufsize,"%s/histograms",WORKDIR);
   
   if ((fp = fopen(filename,"r")) == NULL)
      {
      printf("Unable to load histogram data\n");
      exit(1);
      }
   
   for (position = 0; position < GRAINS; position++)
      {
      fscanf(fp,"%d ",&position);
      
      for (i = 0; i < 4 + 2*ATTR; i++)
	 {
	 for (day = 0; day < 7; day++)
	    {
	    fscanf(fp,"%d ",&(HISTOGRAM[i][day][position]));
	    }

	 weekly[i][position] = 0;
	 }
      }
   
   fclose(fp);

   sprintf(filename,"users.distr"); 
   if ((fpuser = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(filename,"rootprocs.distr"); 
   if ((fproot = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(filename,"otherprocs.distr"); 
   if ((fpother = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   sprintf(filename,"freedisk.distr"); 
   if ((fpdisk = fopen(filename,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }

   for (i = 0; i < ATTR; i++)
      {
      sprintf(filename,"%s-in.distr",ECGSOCKS[i][1]); 
      if ((fpin[i] = fopen(filename,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }

      sprintf(filename,"%s-out.distr",ECGSOCKS[i][1]); 
      if ((fpout[i] = fopen(filename,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      }

   /* Plot daily and weekly histograms */
   for (k = 0; k < GRAINS; k++)
      {
      int a;
      
      for (j = 0; j < ATTR*2+4; j++)
	 {
	 for (i = 0; i < 7; i++)	 
	    {
	    weekly[j][k] += HISTOGRAM[j][i][k];
	    }
	 }

      fprintf(fpuser,"%d %d\n",k,weekly[0][k]);
      fprintf(fproot,"%d %d\n",k,weekly[1][k]);
      fprintf(fpother,"%d %d\n",k,weekly[2][k]);
      fprintf(fpdisk,"%d %d\n",k,weekly[3][k]);

      for (a = 0; a < ATTR; a++)
	 {
	 fprintf(fpin[a],"%d %d\n",k,weekly[4+a][k]);
	 fprintf(fpout[a],"%d %d\n",k,weekly[4+ATTR+a][k]);
	 }
      }
   
   fclose(fproot);
   fclose(fpother);
   fclose(fpuser);
   fclose(fpdisk);

   for (i = 0; i < ATTR; i++)
      {
      fclose(fpin[i]);
      fclose(fpout[i]);
      }
   }
 
return (0); 
}

/*****************************************************************************/

void CheckOpts(argc,argv)

char **argv;
int argc;

{ extern char *optarg;
  int optindex = 0;
  int c;

snprintf(FILENAME,bufsize,"%s/av.db",WORKDIR);

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


