
/* cfengine for GNU
 
        Copyright (C) 1995
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
/* File: instrument.c                                                        */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

# if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
pthread_mutex_t MUTEX_GETADDR = PTHREAD_MUTEX_INITIALIZER;
# endif

/* Alter this code at your peril. Berkeley DB is *very* sensitive to errors. */

/***************************************************************/

void RecordPerformance(char *eventname,time_t t,double value)

{ DB *dbp;
  DB_ENV *dbenv = NULL;
  char name[CF_BUFSIZE],databuf[CF_BUFSIZE];
  struct Event e,newe;
  double lastseen,delta2;
  int lsea = CF_WEEK;
  time_t now = time(NULL);

Debug("PerformanceEvent(%s,%.1f s)\n",eventname,value);

snprintf(name,CF_BUFSIZE-1,"%s/%s",VLOCKDIR,CF_PERFORMANCE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

if ((errno = dbp->open(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return;
   }

if (ReadDB(dbp,databuf,&e,sizeof(e)))
   {
   lastseen = now - e.t;
   newe.t = t;
   newe.Q.q = value;
   newe.Q.expect = SWAverage(value,e.Q.expect);
   delta2 = (value - e.Q.expect)*(value - e.Q.expect);
   newe.Q.var = SWAverage(delta2,e.Q.var);
   }
else
   {
   lastseen = 0.0;
   newe.t = t;
   newe.Q.q = value;
   newe.Q.expect = value;
   newe.Q.var = 0.0;
   }

if (lastseen > (double)lsea)
   {
   Verbose("Performance record %s expired\n",eventname);
   DeleteDB(dbp,eventname);   
   }
else
   {
   Verbose("Performance(%s): time=%.2f secs, av=%.2f +/- %.2f\n",eventname,value,newe.Q.expect,sqrt(newe.Q.var));
   WriteDB(dbp,eventname,&newe,sizeof(newe));
   }

dbp->close(dbp,0);
}

/***************************************************************/

void LastSeen(char *hostname,enum roles role)

{ DB *dbp;
  DB_ENV *dbenv = NULL;
  char name[CF_BUFSIZE],databuf[CF_BUFSIZE];
  time_t now = time(NULL);
  struct QPoint q,newq;
  double lastseen,delta2;
  int lsea = -1;

if (strlen(hostname) == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"LastSeen registry for empty hostname with role %d",role);
   CfLog(cflogonly,OUTPUT,"");
   return;
   }

Debug("LastSeen(%s) reg\n",hostname);

/* Tidy old versions - temporary */
snprintf(name,CF_BUFSIZE-1,"%s/%s",VLOCKDIR,CF_OLDLASTDB_FILE);
unlink(name);

snprintf(name,CF_BUFSIZE-1,"%s/%s",VLOCKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

if ((errno = dbp->open(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return;
   }

#ifdef HAVE_PTHREAD_H  
if (pthread_mutex_lock(&MUTEX_GETADDR) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","unlock");
   exit(1);
   }
#endif

switch (role)
   {
   case cf_accept:
       snprintf(databuf,CF_BUFSIZE-1,"-%s",Hostname2IPString(hostname));
       break;
   case cf_connect:
       snprintf(databuf,CF_BUFSIZE-1,"+%s",Hostname2IPString(hostname));
       break;
   }

#ifdef HAVE_PTHREAD_H  
if (pthread_mutex_unlock(&MUTEX_GETADDR) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   exit(1);
   }
#endif


if (GetMacroValue(CONTEXTID,"LastSeenExpireAfter"))
   {
   lsea = atoi(GetMacroValue(CONTEXTID,"LastSeenExpireAfter"));
   lsea *= CF_TICKS_PER_DAY;
   }

if (lsea < 0)
   {
   lsea = CF_WEEK;
   }
   
if (ReadDB(dbp,databuf,&q,sizeof(q)))
   {
   lastseen = (double)now - q.q;
   newq.q = (double)now;                   /* Last seen is now-then */
   newq.expect = SWAverage(lastseen,q.expect);
   delta2 = (lastseen - q.expect)*(lastseen - q.expect);
   newq.var = SWAverage(delta2,q.var);
   }
else
   {
   lastseen = 0.0;
   newq.q = (double)now;
   newq.expect = 0.0;
   newq.var = 0.0;
   }

if (lastseen > (double)lsea)
   {
   Verbose("Last seen %s expired\n",databuf);
   DeleteDB(dbp,databuf);   
   }
else
   {
   WriteDB(dbp,databuf,&newq,sizeof(newq));
   }

dbp->close(dbp,0);
}

/***************************************************************/

void CheckFriendConnections(int hours)

/* Go through the database of recent connections and check for
   Long Time No See ...*/

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret, secs = CF_TICKS_PER_HOUR*hours, criterion;
  time_t now = time(NULL),lsea = -1, tthen;
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE],datebuf[CF_MAXVARSIZE];
  char addr[CF_BUFSIZE],type[CF_BUFSIZE];
  struct QPoint entry;
  double then = 0.0, average = 0.0, var = 0.0, ticksperminute = 60.0;
  double ticksperhour = (double)CF_TICKS_PER_HOUR,ticksperday = (double)CF_TICKS_PER_DAY;

Verbose("CheckFriendConnections(%d)\n",hours);
snprintf(name,CF_BUFSIZE-1,"%s/%s",VLOCKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

if ((errno = dbp->open(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   CfLog(cferror,"Error reading from last-seen database","");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Walk through the database and print out the key/data pairs. */

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   time_t then;
   memset(&entry, 0, sizeof(entry)); 

   strcpy(hostname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      then = (time_t)entry.q;
      average = (double)entry.expect;
      var = (double)entry.var;
      }
   else
      {
      continue;
      }

   /* Got data, now get expiry criterion */

   if (secs == 0)
      {
      criterion = (now - then > (int)(average+sqrt(var)+0.5));
      }
   else
      {
      criterion = (now - then > secs);
      }

   if (GetMacroValue(CONTEXTID,"LastSeenExpireAfter"))
      {
      lsea = atoi(GetMacroValue(CONTEXTID,"LastSeenExpireAfter"));
      lsea *= CF_TICKS_PER_DAY;
      }

   if (lsea < 0)
      {
      lsea = CF_WEEK;
      }
   
   tthen = (time_t)then;

   snprintf(datebuf,CF_BUFSIZE-1,"%s",ctime(&tthen));
   datebuf[strlen(datebuf)-9] = '\0';                     /* Chop off second and year */

   snprintf(addr,15,"%s",hostname+1);

   switch(*hostname)
      {
      case '+':
          snprintf(type,CF_BUFSIZE,"last responded to hails");
          break;
      case'-':
          snprintf(type,CF_BUFSIZE,"last hailed us");
          break;
      }

   snprintf(OUTPUT,CF_BUFSIZE,"Host %s i.e. %s %s @ [%s] (overdue)",
            IPString2Hostname(hostname+1),
            addr,
            type,
            datebuf);

   if (criterion)
      {
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CfLog(cfverbose,OUTPUT,"");
      }

   snprintf(OUTPUT,CF_BUFSIZE,"i.e. (%.2f) hrs ago, Av %.2f +/- %.2f hrs\n",
            ((double)(now-then))/ticksperhour,
            average/ticksperhour,
            sqrt(var)/ticksperhour);
   
   if (criterion)
      {
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CfLog(cfverbose,OUTPUT,"");
      }
   
   if (now - then > lsea)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Giving up on host %s -- too long since last seen",IPString2Hostname(hostname+1));
      CfLog(cfinform,OUTPUT,"");
      DeleteDB(dbp,hostname);
      }

   memset(&value, 0, sizeof(value));
   memset(&key, 0, sizeof(key)); 
   }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}


/*****************************************************************************/
/* level 1                                                                   */
/*****************************************************************************/

int ReadDB(DB *dbp,char *name,void *ptr,int size)

{ DBT *key,value;
  
key = NewDBKey(name);
memset(&value,0,sizeof(DBT));

if ((errno = dbp->get(dbp,NULL,key,&value,0)) == 0)
   {
   memset(ptr,0,size);
   memcpy(ptr,value.data,size);
   
   Debug("READ %s\n",name);
   DeleteDBKey(key);
   return true;
   }
else
   {
   return false;
   }
}

/*****************************************************************************/

int WriteDB(DB *dbp,char *name,void *ptr,int size)

{ DBT *key,*value;
 
key = NewDBKey(name); 
value = NewDBValue(ptr,size);

if ((errno = dbp->put(dbp,NULL,key,value,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Checksum write failed: %s",db_strerror(errno));
   CfLog(cferror,OUTPUT,"db->put");
   
   DeleteDBKey(key);
   DeleteDBValue(value);
   return false;
   }
else
   {
   DeleteDBKey(key);
   DeleteDBValue(value);
   return true;
   }
}

/*****************************************************************************/

void DeleteDB(DB *dbp,char *name)

{ DBT *key;

key = NewDBKey(name);

if ((errno = dbp->del(dbp,NULL,key,0)) != 0)
   {
   CfLog(cferror,"","db_store");
   }

Debug("DELETED DB %s\n",name);
}


/*****************************************************************************/
/* Level 2                                                                   */
/*****************************************************************************/

DBT *NewDBKey(char *name)

{ char *dbkey;
  DBT *key;

if ((dbkey = malloc(strlen(name)+1)) == NULL)
   {
   FatalError("NewChecksumKey malloc error");
   }

if ((key = (DBT *)malloc(sizeof(DBT))) == NULL)
   {
   FatalError("DBT  malloc error");
   }

memset(key,0,sizeof(DBT));
memset(dbkey,0,strlen(name)+1);

strncpy(dbkey,name,strlen(name));

Debug("StringKEY => %s\n",dbkey);

key->data = (void *)dbkey;
key->size = strlen(name)+1;

return key;
}

/*****************************************************************************/

void DeleteDBKey(DBT *key)

{
free((char *)key->data);
free((char *)key);
}

/*****************************************************************************/

DBT *NewDBValue(void *ptr,int size)

{ void *val;
  DBT *value;

if ((val = (void *)malloc(size)) == NULL)
   {
   FatalError("NewDBKey malloc error");
   }

if ((value = (DBT *) malloc(sizeof(DBT))) == NULL)
   {
   FatalError("DBT Value malloc error");
   }

memset(value,0,sizeof(DBT)); 
memset(val,0,size);
memcpy(val,ptr,size);

value->data = val;
value->size = size;

return value;
}

/*****************************************************************************/

void DeleteDBValue(DBT *value)

{
free((char *)value->data);
free((char *)value);
}

/*****************************************************************************/
/* Toolkit                                                                   */
/*****************************************************************************/

double SWAverage(double anew,double aold)

{ double av;
  double wnew,wold;

wnew = 0.3;
wold = 0.7;

av = (wnew*anew + wold*aold)/(wnew+wold); 

return av;
}
