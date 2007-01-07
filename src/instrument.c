
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


/***************************************************************/

void LastSeen(char *hostname,enum roles role)

{ DBT key,value;
  DB *dbp;
  DB_ENV *dbenv = NULL;
  char name[CF_BUFSIZE],databuf[CF_BUFSIZE];
  time_t lastseen,now = time(NULL);
  static struct LastSeen entry;
  double average;

if (strlen(hostname) == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"LastSeen registry found empty hostname with role %d",role);
   CfLog(cflogonly,OUTPUT,"");
   return;
   }
  
snprintf(name,CF_BUFSIZE-1,"%s/%s",VLOCKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return;
   }

memset(&value,0,sizeof(value)); 
memset(&key,0,sizeof(key));       

switch (role)
   {
   case cf_accept:
       snprintf(databuf,CF_BUFSIZE-1,"%s (hailed us)",Hostname2IPString(hostname));
       break;
   case cf_connect:
       snprintf(databuf,CF_BUFSIZE-1,"%s (answered us)",Hostname2IPString(hostname));
       break;
   }
 
key.data = databuf;
key.size = strlen(databuf)+1;

if ((errno = dbp->get(dbp,NULL,&key,&value,0)) == 0)
   {
   memcpy(&entry,value.data,sizeof(entry));   
   average = (double)entry.expect_lastseen;
   lastseen = entry.lastseen;

   /* Update the geometrical memory of the expectation value for this arrival-process */
   
   entry.lastseen = now;
   entry.expect_lastseen = (0.7 * average + 0.3 * (double)(now - lastseen));
   
   key.data = databuf;
   key.size = strlen(databuf)+1;
   
   Verbose("Updating last-seen time for %s\n",hostname);
   
   if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
      {
      CfLog(cferror,"","db->del");
      }
   
   key.data = databuf;
   key.size = strlen(databuf)+1;
   value.data = (void *) &entry;
   value.size = sizeof(entry);
   
   if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
      {
      CfLog(cferror,"put failed","db->put");
      }
 
   dbp->close(dbp,0);
   }
else
   {
   key.data = databuf;
   key.size = strlen(databuf)+1;

   entry.lastseen = now;
   entry.expect_lastseen = 0;
   
   value.data = (void *) &entry;
   value.size = sizeof(entry);
   
   if ((errno = dbp->put(dbp,NULL,&key,&value,0)) != 0)
      {
      CfLog(cferror,"put failed","db->put");
      }
   
   dbp->close(dbp,0);
   }
}

/***************************************************************/

void CheckFriendConnections(int hours)

/* Go through the database of recent connections and check for
   Long Time No See ...*/

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret, secs = 3600*hours, criterion;
  time_t now = time(NULL),splaytime = 0,lsea = -1;
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE];
  static struct LastSeen entry;
  double average = 0;

if (GetMacroValue(CONTEXTID,"SplayTime"))
   {
   splaytime = atoi(GetMacroValue(CONTEXTID,"SplayTime"));
   if (splaytime < 0)
      {
      splaytime = 0;   
      }
   }

Verbose("CheckFriendConnections(%d)\n",hours);
snprintf(name,CF_BUFSIZE-1,"%s/%s",VLOCKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = dbp->open(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = dbp->open(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
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

 /* Initialize the key/data return pair. */

 
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry)); 
 
 /* Walk through the database and print out the key/data pairs. */

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   time_t then;

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

   if (secs == 0)
      {
      criterion = (now > then + splaytime + (int)(average+0.5));
      }
   else
      {
      criterion = (now > then + splaytime + secs);
      }
   
   if (GetMacroValue(CONTEXTID,"LastSeenExpireAfter"))
      {
      lsea = atoi(GetMacroValue(CONTEXTID,"LastSeenExpireAfter"));
      lsea *= 3600*24;
      }

   if (lsea < 0)
      {
      lsea = CF_WEEK;
      }

   if ((int)average > lsea)   /* Don't care about outliers */
      {
      criterion = false;
      }

   if (average < 1800)  /* anomalous couplings do not count*/
      {
      criterion = false;
      }
   
   snprintf(OUTPUT,CF_BUFSIZE,"Host %s last at %s\ti.e. not seen for %.2f hours\n\t(Expected <delta_t> = %.2f secs (= %.2f hours) (Expires %d days)",hostname,ctime(&then),((double)(now-then))/3600.0,average,average/3600.0,lsea/3600/24);

   if (now > lsea + then + splaytime + 2*3600)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"INFO: Giving up on %s, last seen more than %d days ago at %s.",hostname,lsea/3600/24,ctime(&then));
      CfLog(cferror,OUTPUT,"");
      
      if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
         {
         CfLog(cferror,"","db_store");
         }
      }

   if (criterion)
      {
      CfLog(cferror,OUTPUT,"");
      }
   else
      {      
      CfLog(cfinform,OUTPUT,"");
      }
   }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

