/* cfengine for GNU
 
        Copyright (C) 1995,2001
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
 

/*********************************************************************/
/*                                                                   */
/* TOOLKIT : Locks and Signals                                       */
/*                                                                   */
/*********************************************************************/

/* A log file of the run times is kept for each host separately.
   This records each atomic lock and the time at which it
   completed, for use in computing the elapsed time. The file
   format is:

   %s time:operation:operand

   Each operation (independently of operand) has a "last" inode
   which keeps the time at which is last completed, for use in
   calculating IfElapsed. The idea here is that the elapsed time
   is from the time at which the last operation of this type
   FINISHED. This is different from a lock (which is used to
   allow several sub operations to coexist). Here we are
   limiting activity in general to avoid "spamming".

   Each atomic operation (including operand) has a lock. The
   removal of this lock causes the "last" file to be updated.
   This is used to actually prevent execution of an atom
   which is already being executed. If this lock has existed
   for longer than the ExpireAfter time, the process owning
   the lock is killed and the lock is re-established. The
   lock file contains the pid. 

   This is robust to hanging locks and can be thought of as
   a garbage collection mechanism for these locks.

   Last files are just inodes (empty files) so they use no disk.
   The locks (which never exceed the no of running processes)
   contain the pid.

   */

#include "cf.defs.h"
#include "cf.extern.h"

# include <db.h>

DB *DBP;

struct LockData
   {
   pid_t pid;
   time_t time;
   };

/********************************************************************/

void PreLockState()

{
 strcpy(CFLOCK,"pre-lock-state");
}

/********************************************************************/

void SaveExecLock()

{
 strcpy(SAVELOCK,CFLOCK);
}


/********************************************************************/

void RestoreExecLock()

{
 strcpy(CFLOCK,SAVELOCK);
}

/********************************************************************/

void HandleSignal(int signum)
 
{
snprintf(OUTPUT,CF_BUFSIZE*2,"Received signal %d (%s) while doing [%s]",signum,SIGNALS[signum],CFLOCK);
Chop(OUTPUT);
CfLog(cferror,OUTPUT,"");
snprintf(OUTPUT,CF_BUFSIZE*2,"Logical start time %s ",ctime(&CFSTARTTIME));
Chop(OUTPUT);
CfLog(cferror,OUTPUT,"");
snprintf(OUTPUT,CF_BUFSIZE*2,"This sub-task started really at %s\n",ctime(&CFINITSTARTTIME));

CfLog(cferror,OUTPUT,"");

fflush(stdout);

if (signum == SIGTERM || signum == SIGINT || signum == SIGHUP || signum == SIGSEGV || signum == SIGKILL)
   {
   ReleaseCurrentLock();
   closelog();
   exit(0);
   }
}

/************************************************************************/

void InitializeLocks()

{ int errno;

if (IGNORELOCK)
   {
   return;
   }

snprintf(LOCKDB,CF_BUFSIZE,"%s/cfengine_lock_db",VLOCKDIR);

if ((errno = db_create(&DBP,NULL,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open lock database %s\n",LOCKDB);
   CfLog(cferror,OUTPUT,"db_open");
   IGNORELOCK = true;
   return;
   }



#ifdef CF_OLD_DB
if ((errno = DBP->open(DBP,LOCKDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = DBP->open(DBP,NULL,LOCKDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open lock database %s\n",LOCKDB);
   CfLog(cferror,OUTPUT,"db_open");
   IGNORELOCK = true;
   return;
   }

}

/************************************************************************/

void CloseLocks()

{
if (IGNORELOCK)
   {
   return;
   }

DBP->close(DBP,0);
}

/************************************************************************/

int GetLock(char *operator,char *operand,int ifelapsed,int expireafter,char *host,time_t now)

{ unsigned int pid;
  int err; 
  time_t lastcompleted = 0, elapsedtime;

if (IGNORELOCK)
   {
   return true;
   }

if (now == 0)
   {
   if ((now = time((time_t *)NULL)) == -1)
      {
      printf("Couldn't read system clock\n");
      }
   return true;
   }

Debug("GetLock(%s,%s,time=%d), ExpireAfter=%d, IfElapsed=%d\n",operator,operand,now,expireafter,ifelapsed);

memset(CFLOCK,0,CF_BUFSIZE);
memset(CFLAST,0,CF_BUFSIZE); 
 
snprintf(CFLOG,CF_BUFSIZE,"%s/cfengine.%.40s.runlog",VLOGDIR,host);
snprintf(CFLOCK,CF_BUFSIZE,"lock.%.100s.%.40s.%s.%.100s",VCANONICALFILE,host,operator,operand);
snprintf(CFLAST,CF_BUFSIZE,"last.%.100s.%.40s.%s.%.100s",VCANONICALFILE,host,operator,operand);
 
if (strlen(CFLOCK) > MAX_FILENAME)
   {
   CFLOCK[MAX_FILENAME] = '\0';  /* most nodenames are 255 chars or less */
   }

if (strlen(CFLAST) > MAX_FILENAME)
   {
   CFLAST[MAX_FILENAME] = '\0';  /* most nodenames are 255 chars or less */
   }

/* Look for non-existent (old) processes */

lastcompleted = GetLastLock();
elapsedtime = (time_t)(now-lastcompleted) / 60;

if (elapsedtime < 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Another cfengine seems to have done %s.%s since I started\n",operator, operand);
   CfLog(cfverbose,OUTPUT,"");
   return false;
   }

if (elapsedtime < ifelapsed)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Nothing scheduled for %s.%s (%u/%u minutes elapsed)\n",operator,operand,elapsedtime,ifelapsed);
   CfLog(cfverbose,OUTPUT,"");
   return false;
   }

/* Look for existing (current) processes */

lastcompleted = CheckOldLock();
elapsedtime = (time_t)(now-lastcompleted) / 60;
    
if (lastcompleted != 0)
   {
   if (elapsedtime >= expireafter)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Lock %s expired...(after %u/%u minutes)\n",CFLOCK,elapsedtime,expireafter);
      CfLog(cfinform,OUTPUT,"");
      
      pid = GetLockPid(CFLOCK);

      if (pid == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal pid in corrupt lock %s - ignoring lock\n",CFLOCK);
         CfLog(cferror,OUTPUT,"");
         }
      else
         {
         Verbose("Trying to kill expired process, pid %d\n",pid);
         
         err = 0;
         
/*  if (((err = kill(pid,SIGCONT)) == -1) && (errno != ESRCH))
    {
    sleep(3);
    err=0;
    
    Does anyone remember who put this in or why it was here?
*/
         
         if ((err = kill(pid,SIGINT)) == -1)
            {
            sleep(1);
            err=0;
            
            if ((err = kill(pid,SIGTERM)) == -1)
               {   
               sleep(5);
               err=0;
               
               if ((err = kill(pid,SIGKILL)) == -1)
                  {
                  sleep(1);
                  }
               }
            }
         
         if (err == 0 || errno == ESRCH)
            {
            LockLog(pid,"Lock expired, process killed",operator,operand);
            unlink(CFLOCK);
            }
         else
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to kill expired cfagent process %d from lock %s, exiting this time..\n",pid,CFLOCK);
            CfLog(cferror,OUTPUT,"kill");
            
            FatalError("");
            }
         }
      }
   else
      {
      Verbose("Couldn't obtain lock for %s (already running!)\n",CFLOCK);
      return false;
      }
   }
 
 SetLock();
 return true;
}
 
/************************************************************************/

void ReleaseCurrentLock()

{
if (IGNORELOCK)
   {
   return;
   }

Debug("ReleaseCurrentLock(%s)\n",CFLOCK);

if (strlen(CFLAST) == 0)
   {
   return;
   }


if (DeleteLock(CFLOCK) == -1)
   {
   Debug("Unable to remove lock %s\n",CFLOCK);
   return;
   }

if (PutLock(CFLAST) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to create %s\n",CFLAST);
   CfLog(cferror,OUTPUT,"creat");
   return;
   }

 
LockLog(getpid(),"Lock removed normally ",CFLOCK,"");
strcpy(CFLOCK,"no_active_lock");
}


/************************************************************************/

int CountActiveLocks()

 /* Count the number of active locks == number of cfengines running */

{ int count = 0;
  DBT key,value;
  DBC *dbcp;  
  struct LockData entry;
  time_t elapsedtime;

Debug("CountActiveLocks()\n"); 

if (IGNORELOCK)
   {
   return 1;
   }

memset(&value,0,sizeof(value)); 
memset(&key,0,sizeof(key));    

InitializeLocks();
 
if ((errno = DBP->cursor(DBP, NULL, &dbcp, 0)) != 0)
   {
   CfLog(cfverbose,"Couldn't dump lock db","");
   return -1;
   }

while ((errno = dbcp->c_get(dbcp, &key, &value, DB_NEXT)) == 0)
   {
   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      
      elapsedtime = (time_t)(CFSTARTTIME-entry.time) / 60;
      
      if (elapsedtime >= VEXPIREAFTER)      
         {
         Debug("LOCK-DB-EXPIRED: %s %s\n",ctime(&(entry.time)),key.data);
         continue;
         }
      
      Debug("LOCK-DB-DUMP   : %s %s\n",ctime(&(entry.time)),key.data);
      
      if (strncmp(key.data,"lock",4) == 0)
         {
         count++;
         }
      }
   }

CloseLocks(); 

Debug("Found %d active/concurrent cfengines",count); 
return count;
}

/************************************************************************/
/* Level 2                                                              */
/************************************************************************/

time_t GetLastLock()

{ time_t mtime;

Debug("GetLastLock()\n");

if ((mtime = GetLockTime(CFLAST)) == -1)
   {   
   /* Do this to prevent deadlock loops from surviving if IfElapsed > T_sched */
   
   if (PutLock(CFLAST) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to lock %s\n",CFLAST);
      CfLog(cferror,OUTPUT,"");
      return 0;
      }

   return 0;
   }
else
   {
   return mtime;
   }
}

/************************************************************************/

time_t CheckOldLock()

{ time_t mtime;

Debug("CheckOldLock(%s)\n",CFLOCK);

if ((mtime = GetLockTime(CFLOCK)) == -1)
   {
   Debug("Unable to find lock data %s\n",CFLOCK);
   return 0;
   }
else
   {
   Debug("Lock %s last ran at %s\n",CFLOCK,ctime(&mtime));
   return mtime;
   }
}

/************************************************************************/

void SetLock()

{
Debug("SetLock(%s)\n",CFLOCK);

if (PutLock(CFLOCK) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"GetLock: can't open new lock file %s\n",CFLOCK);
   CfLog(cferror,OUTPUT,"fopen");
   FatalError("");;
   }
}


/************************************************************************/
/* Level 3                                                              */
/************************************************************************/

int PutLock(char *name)

{ DBT key,value;
  struct LockData entry;

Debug("PutLock(%s)\n",name);
  
memset(&value,0,sizeof(value)); 
memset(&key,0,sizeof(key));       
      
key.data = name;
key.size = strlen(name)+1;

InitializeLocks();

if (IGNORELOCK)
   {
   return 0;
   }
 
if ((errno = DBP->del(DBP,NULL,&key,0)) != 0)
   {
   Debug("Unable to delete lock [%s]: %s\n",name,db_strerror(errno));
   }

key.data = name;
key.size = strlen(name)+1;
entry.pid = getpid();
entry.time = time((time_t *)NULL); 
value.data = &entry;
value.size = sizeof(entry);

#if defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD
 
if (pthread_mutex_lock(&MUTEX_LOCK) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
   }
 
#endif
 
if ((errno = DBP->put(DBP,NULL,&key,&value,0)) != 0)
   {
   CfLog(cferror,"put failed","db->put");
   CloseLocks();
   return -1;
   }      

#if defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD
 
if (pthread_mutex_unlock(&MUTEX_LOCK) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   }
 
#endif
 
CloseLocks(); 
return 0; 
}

/************************************************************************/

int DeleteLock(char *name)

{ DBT key;

if (strcmp(name,"pre-lock-state") == 0)
   {
   return 0;
   }
 
memset(&key,0,sizeof(key));       
      
key.data = name;
key.size = strlen(name)+1;

#if defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD

if (pthread_mutex_lock(&MUTEX_LOCK) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_lock");
   }
#endif 
  
InitializeLocks();
 
if ((errno = DBP->del(DBP,NULL,&key,0)) != 0)
   {
   Debug("Unable to delete lock [%s]: %s\n",name,db_strerror(errno));
   }

#if defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD

if (pthread_mutex_unlock(&MUTEX_LOCK) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   }
 
#endif
 
CloseLocks();
return 0; 
}

/************************************************************************/

time_t GetLockTime(char *name)

{ DBT key,value;
  struct LockData entry;

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
memset(&entry,0,sizeof(entry));
      
key.data = name;
key.size = strlen(name)+1;

InitializeLocks();
 
if ((errno = DBP->get(DBP,NULL,&key,&value,0)) != 0)
   {
   CloseLocks();
   return -1;
   }

if (value.data != NULL)
   {
   memcpy(&entry,value.data,sizeof(entry));
   CloseLocks();
   return entry.time;
   }

CloseLocks(); 
return -1; 
}

/************************************************************************/

pid_t GetLockPid(char *name)

{ DBT key,value;
  struct LockData entry;

memset(&value,0,sizeof(value));
memset(&key,0,sizeof(key));       
      
key.data = name;
key.size = strlen(name)+1;

InitializeLocks();
 
if ((errno = DBP->get(DBP,NULL,&key,&value,0)) != 0)
   {
   CloseLocks();
   return -1;
   }

if (value.data != NULL)
   {
   memcpy(&entry,value.data,sizeof(entry));
   CloseLocks();
   return entry.pid;
   }

CloseLocks(); 
return -1; 
}

/************************************************************************/

void LockLog(int pid,char *str,char *operator,char *operand)

{ FILE *fp;
  char buffer[CF_MAXVARSIZE];
  struct stat statbuf;
  time_t tim;

Debug("LockLog(%s)\n",str);

if ((fp = fopen(CFLOG,"a")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open lock-log file %s\n",CFLOG);
   CfLog(cferror,OUTPUT,"fopen");
   FatalError("");
   }

if ((tim = time((time_t *)NULL)) == -1)
   {
   Debug("Cfengine: couldn't read system clock\n");
   }

sprintf(buffer,"%s",ctime(&tim));

Chop(buffer);

fprintf(fp,"%s:%s:pid=%d:%s:%s\n",buffer,str,pid,operator,operand);

fclose(fp);

if (stat(CFLOG,&statbuf) != -1)
   {
   if (statbuf.st_size > CFLOGSIZE)
      {
      Verbose("Rotating lock-runlog file\n");
      RotateFiles(CFLOG,2);
      }
   }
}
