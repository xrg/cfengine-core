/*****************************************************************************/
/*                                                                           */
/* File: cfshow.c                                                            */
/*                                                                           */
/* Created: Wed Sep 22 16:47:23 2004                                         */
/*                                                                           */
/* Author: Mark                                      >                       */
/*                                                                           */
/* Description: Print data from the Berkeeley databases in reable form       */
/*                                                                           */
/*****************************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/
/* GLOBAL VARIABLES                                                */
/*******************************************************************/

struct option CFDOPTIONS[] =
   {
   { "help",no_argument,0,'h' },
   { "debug",optional_argument,0,'d' }, 
   { "verbose",no_argument,0,'v' },
   { "locks",no_argument,0,'l'},
   { "last-seen",no_argument,0,'s'},
   { "checksum",no_argument,0,'c'},
   { "active",no_argument,0,'a'},
   { "version",no_argument,0,'V'},
   { NULL,0,0,0 }
   };

enum databases
   {
   cf_db_lastseen,
   cf_db_locks,
   cf_db_active,
   cf_db_checksum,
   };

enum databases TODO = -1;

#define CF_ACTIVE 1
#define CF_INACTIVE 0

/*******************************************************************/
/* Functions internal to cfservd.c                                 */
/*******************************************************************/

void CheckOptsAndInit (int argc,char **argv);
void Syntax (void);
void PrintDB(void);
void ShowLastSeen(void);
void ShowChecksums(void);
void ShowLocks(int active);
char *ChecksumDump(unsigned char digest[EVP_MAX_MD_SIZE+1]);

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (int argc,char **argv)

{
CheckOptsAndInit(argc,argv);

PrintDB();
return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(int argc,char **argv)

{ extern char *optarg;
  int optindex = 0;
  int c;

while ((c=getopt_long(argc,argv,"hdvaVlsc",CFDOPTIONS,&optindex)) != EOF)
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
                
                VERBOSE = true;
                printf("cfexecd Debug mode: running in foreground\n");
                break;

      case 'v': VERBOSE = true;
         break;

      case 'V': printf("GNU %s-%s db tool\n%s\n",PACKAGE,VERSION,COPYRIGHT);
          printf("This program is covered by the GNU Public License and may be\n");
          printf("copied free of charge. No warrenty is implied.\n\n");
          exit(0);
          break;
          
      case 'a':
          TODO = cf_db_active;
          break;

      case 'l':
          TODO = cf_db_locks;
          break;

      case 's':
          TODO = cf_db_lastseen;
          break;

      case 'c':
          TODO = cf_db_checksum;
          break;
          
      default:  Syntax();
          exit(1);
          
      }
  }


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
}

/********************************************************************/

void PrintDB()

{
switch (TODO)
   {
   case cf_db_lastseen:
       ShowLastSeen();
       break;
   case cf_db_locks:
       ShowLocks(CF_INACTIVE);
       break;
   case cf_db_active:
       ShowLocks(CF_ACTIVE);
       break;
   case cf_db_checksum:
       ShowChecksums();
       break;
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine db tool\n%s-%s\n%s\n",PACKAGE,VERSION,COPYRIGHT);
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

void ShowLastSeen()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  time_t now = time(NULL);
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE];
  static struct LastSeen entry;
  double average = 0;
  int ret;
  
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open last-seen database %s\n",name);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open last-seen database %s\n",name);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   printf("Error reading from last-seen database: ");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Initialize the key/data return pair. */

 
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry)); 
 
 /* Walk through the database and print out the key/data pairs. */

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   time_t then;
   char tbuf[CF_BUFSIZE];

   memcpy(&then,value.data,sizeof(then));
   strcpy(hostname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      then = (time_t)entry.lastseen;
      average = (double)entry.expect_lastseen;
      }
   else
      {
      continue;
      }

   snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&then));
   tbuf[strlen(tbuf)-1] = '\0';

   printf("%s at [%s] i.e. not seen for !%.2f! hours; <delta_t> = {%.2f} hours\n",hostname,tbuf,((double)(now-then))/3600.0,average/3600.0);
   }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*******************************************************************/

void ShowChecksums()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret;
  FILE *pp;
  char checksumdb[CF_BUFSIZE],cfcom[CF_BUFSIZE];
  struct stat statbuf;
   
if (stat("/usr/local/sbin/cfagent",&statbuf) != -1)
   {
   snprintf(cfcom,CF_BUFSIZE-1,"/usr/local/sbin/cfagent -z",CFWORKDIR);
   }
else
   {
   snprintf(cfcom,CF_BUFSIZE-1,"%s/bin/cfagent -z",CFWORKDIR);
   }

if ((pp=cfpopen(cfcom,"r")) ==  NULL)
   {
   CfLog(cferror,"Couldn't start cfengine!","cfpopen");
   checksumdb[0] = '\0';
   return;
   }

checksumdb[0] = '\0'; 
fgets(checksumdb,CF_BUFSIZE,pp); 
Chop(checksumdb); 

cfpclose(pp);
  
if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open checksum database %s\n",checksumdb);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,checksumdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,checksumdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open checksum database %s\n",checksumdb);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

 if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
    {
    printf("Error reading from checksum database");
    dbp->err(dbp, ret, "DB->cursor");
    return;
    }

 /* Initialize the key/data return pair. */

 memset(&key,0,sizeof(key));
 memset(&value,0,sizeof(value));
 
 /* Walk through the database and print out the key/data pairs. */

 while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
    {
    printf("%s = ",(char *)key.data);
    printf("%s\n",ChecksumDump(value.data));
    }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*********************************************************************/

void ShowLocks (int active)

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret;
  char lockdb[CF_BUFSIZE];
  struct LockData entry;

  
snprintf(lockdb,CF_BUFSIZE,"%s/cfengine_lock_db",CFWORKDIR);
  
if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open checksum database %s\n",lockdb);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,lockdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,lockdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open checksum database %s\n",lockdb);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

 if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
    {
    printf("Error reading from checksum database");
    dbp->err(dbp, ret, "DB->cursor");
    return;
    }

 /* Initialize the key/data return pair. */

 memset(&key,0,sizeof(key));
 memset(&value,0,sizeof(value));
 
 /* Walk through the database and print out the key/data pairs. */

 while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
    {
    if (active)
       {
       if (strncmp("lock",(char *)key.data,4) == 0)
          {
          printf("%s = ",(char *)key.data);
          
          if (value.data != NULL)
             {
             memcpy(&entry,value.data,sizeof(entry));
             printf("%s\n",ctime(&entry.time));
             }          
          }
       }
    else
       {
       if (strncmp("last",(char *)key.data,4) == 0)
          {
          printf("%s = ",(char *)key.data);
          
          if (value.data != NULL)
             {
             memcpy(&entry,value.data,sizeof(entry));
             printf("%s\n",ctime(&entry.time));
             }
          }
       }
    }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

char *ChecksumDump(unsigned char digest[EVP_MAX_MD_SIZE+1])

{ unsigned int i;
  static char buffer[EVP_MAX_MD_SIZE*4];
  int len = 1;

for (i = 0; buffer[i] != 0; i++)
   {
   len++;
   }

if (len == 16 || len == 20)
   {
   }
else
   {
   len = 16;
   }

switch(len)
   {
   case 20: sprintf(buffer,"SHA=  ");
       break;
   case 16: sprintf(buffer,"MD5=  ");
       break;
   }
  
for (i = 0; i < len; i++)
   {
   sprintf((char *)(buffer+4+2*i),"%02x", digest[i]);
   }

return buffer; 
}    


int RecursiveTidySpecialArea(char *name,struct Tidy *tp,int maxrecurse,struct stat *sb)

{
 return 1;
}


void ReleaseCurrentLock()
{
 return;
}

char *GetMacroValue(char *scope,char *name)

{
 return NULL;
}
