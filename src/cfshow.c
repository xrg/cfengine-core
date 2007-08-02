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
   { "performance",no_argument,0,'p'},
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
   cf_db_performance
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
void ShowPerformance(void);
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

while ((c=getopt_long(argc,argv,"hdvaVlscp",CFDOPTIONS,&optindex)) != EOF)
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

      case 'p':
          TODO = cf_db_performance;
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
   case cf_db_performance:
       ShowPerformance();
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
  double now = (double)time(NULL),average = 0, var = 0;
  double ticksperhr = (double)CF_TICKS_PER_HOUR;
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE];
  struct QPoint entry;
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
   double then;
   time_t fthen;
   char tbuf[CF_BUFSIZE],addr[CF_BUFSIZE];

   memcpy(&then,value.data,sizeof(then));
   strcpy(hostname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      then = entry.q;
      average = (double)entry.expect;
      var = (double)entry.var;
      }
   else
      {
      continue;
      }

   fthen = (time_t)then;                            /* format date */
   snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&fthen));
   tbuf[strlen(tbuf)-9] = '\0';                     /* Chop off second and year */

   if (strlen(hostname+1) > 15)
      {
      snprintf(addr,15,"...%s",hostname+strlen(hostname)-10); /* ipv6 */
      }
   else
      {
      snprintf(addr,15,"%s",hostname+1);
      }
   
   printf("IP %c %25.25s %15.15s  @ [%s] not seen for (%.2f) hrs, Av %.2f +/- %.2f hrs\n",
          *hostname,
          IPString2Hostname(hostname+1),
          addr,
          tbuf,
          ((double)(now-then))/ticksperhr,
          average/ticksperhr,
          sqrt(var)/ticksperhr);
   }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*******************************************************************/

void ShowPerformance()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  double now = (double)time(NULL),average = 0, var = 0;
  double ticksperminute = 60.0;
  char name[CF_BUFSIZE],eventname[CF_BUFSIZE];
  struct Event entry;
  int ret;
  
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_PERFORMANCE);

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
   double measure;
   time_t then;
   char tbuf[CF_BUFSIZE],addr[CF_BUFSIZE];

   memcpy(&then,value.data,sizeof(then));
   strcpy(eventname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));

      then    = entry.t;
      measure = entry.Q.q/ticksperminute;;
      average = entry.Q.expect/ticksperminute;;
      var     = entry.Q.var;

      snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&then));
      tbuf[strlen(tbuf)-9] = '\0';                     /* Chop off second and year */

      printf("(%7.4f mins @ %s) Av %7.4f +/- %7.4f for %s \n",measure,tbuf,average,sqrt(var)/ticksperminute,eventname);
      }
   else
      {
      continue;
      }
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
  char checksumdb[CF_BUFSIZE];
  struct stat statbuf;
 
snprintf(checksumdb,CF_BUFSIZE,"%s/%s",CFWORKDIR,CF_CHKDB);
  
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
    char type;
    char strtype[CF_MAXVARSIZE];
    char name[CF_BUFSIZE];
    struct Checksum_Value chk_val;
    unsigned char digest[EVP_MAX_MD_SIZE+1];
    
    memset(digest,0,EVP_MAX_MD_SIZE+1);
    memset(&chk_val,0,sizeof(chk_val));
    
    memcpy(&chk_val,value.data,sizeof(chk_val));
    memcpy(digest,chk_val.mess_digest,EVP_MAX_MD_SIZE+1);

    strncpy(strtype,key.data,CF_MAXDIGESTNAMELEN);
    strncpy(name,key.data+CF_CHKSUMKEYOFFSET,CF_BUFSIZE-1);

    type = ChecksumType(strtype);

    printf("%c %s ",type,strtype);
    printf("%s = ",name);
    printf("%s\n",ChecksumPrint(type,digest));
    /* attr_digest too here*/

    memset(&key,0,sizeof(key));
    memset(&value,0,sizeof(value));
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


/*********************************************************************/

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


int CompareMD5Net (char *file1, char *file2, struct Image *ip)
{
 return 0;
}

void yyerror(char *s)
{
}
