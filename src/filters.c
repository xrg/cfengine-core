/* cfengine for GNU
 
        Copyright (C) 1995/6,2000
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
/* File: filters.c                                                           */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/
/* Parser                                                          */
/*******************************************************************/

void InstallFilter(filter)

char *filter;

{ struct Filter *ptr;
  int i;
 
Debug1("InstallFilter(%s)\n",filter);

if (FilterExists(filter))
   {
   yyerror("Redefinition of existing filter");
   return;
   }

if (! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing Edit no match\n");
   return;
   }

if ((ptr = (struct Filter *)malloc(sizeof(struct Filter))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallFilterFilter() #1");
   }

if ((ptr->alias = strdup(filter)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallFilterFilter() #2");
   }
 
if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallFilterFilter() #3");
   }
 
if (VFILTERLISTTOP == NULL)                 /* First element in the list */
   {
   VFILTERLIST = ptr;
   }
else
   {
   VFILTERLISTTOP->next = ptr;
   }

for (i = 0; i < NoFilter; i++)
   {
   ptr->criteria[i] = NULL;
   }
 
ptr->next = NULL;
ptr->defines = NULL;
ptr->elsedef = NULL; 
VFILTERLISTTOP = ptr;
}

/********************************************************************/

void InstallFilterTest(alias,type,data)

char *alias,*type,*data;

{ int crit = (int) FilterActionsToCode(type), i = -1;
  struct Filter *fp;
  char buffer[bufsize],units='x',*sp;
  time_t now;
  
Debug("InstallFilterTest(%s,%s,%s)\n",alias,type,data);

if (time(&now) == -1)
   {
   FatalError("Unable to access clock");
   }
 
buffer[0] = '\0'; 
ExpandVarstring(data,buffer,NULL);
 
if (crit == NoFilter)
   {
   yyerror("Unknown filter criterion");
   }
else
   {
   for (fp = VFILTERLIST; fp != NULL; fp = fp->next)
      {
      if (strcmp(alias,fp->alias) == 0)
	 {
         switch (crit)
	    {
	    case filterdefclasses:
		fp->defines = strdup(buffer);
		break;
	    case filterelsedef:
		fp->elsedef = strdup(buffer);
		break;
	    case filterresult:
		IsInstallable(buffer); /* syntax check */
		break;

	    case filterfromctime:
		now = Date2Number(buffer,now);
		break;
	    case filtertoctime:
		now = Date2Number(buffer,now);
		break;
	    case filterfrommtime:
		now = Date2Number(buffer,now);
		break;
	    case filtertomtime:
		now = Date2Number(buffer,now);
		break;
	    case filterfromatime:
		now = Date2Number(buffer,now);
		break;
	    case filtertoatime:
		now = Date2Number(buffer,now);
		break;
	    case filterfromsize:
	    case filtertosize:
		sscanf(buffer,"%d%c",&i,&units);

		if (i < 0)
		   {
		   yyerror("filter size attribute with silly value (must be a non-negative number)");
		   }
		
		switch (units)
		   {
		   case 'k':
		   case 'K': i *= 1024;
		       break;
		   case 'm':
		   case 'M': i = i * 1024 * 1024;
		       break;
		   }

		sprintf(buffer,"%d",i);
		break;
	    case filterexecregex:
		for (sp = buffer+strlen(buffer)-1; (*sp != '(') && (sp > buffer); sp--)
		   {
		   }
		
		sscanf(sp+1,"%256[^)]",VBUFF);
		if (!RegexOK(VBUFF))
		   {
		   yyerror("Regular expression error");
		   }
		break;
		
	    case filternameregex:
		if (!RegexOK(buffer))
		   {
		   yyerror("Regular expression error");
		   }
		break;
	    case filterexec:
		/* test here */
		break;
	    case filtersymlinkto:
		break;
	    }
	 
	 if ((fp->criteria[crit] = strdup(buffer)) == NULL)
	    {
	    CfLog(cferror,"Couldn't allocate filter memory","strdup");
	    FatalError("Dying..");
	    }
	 else
	    {
	    return;
	    }
	 }
      }
   }
}

/********************************************************************/

void CheckFilters()

{ struct Filter *fp;
  time_t t;

if (time(&t) == -1)
   {
   FatalError("Clock unavailable");
   }

for (fp = VFILTERLIST; fp != NULL; fp = fp->next)
   {
   if (fp->criteria[filterresult] == NULL)
       {
       snprintf(OUTPUT,bufsize*2,"No result specified for filter %s",fp->alias);
       CfLog(cferror,OUTPUT,"");
       FatalError("Consistency error");
       }
       
   if ((fp->criteria[filterfromctime]!=NULL && fp->criteria[filtertoctime]==NULL) ||
       (fp->criteria[filterfromctime]==NULL && fp->criteria[filtertoctime]!=NULL))
      {
      snprintf(OUTPUT,bufsize*2,"Incomplete ctime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }
      
   if ((fp->criteria[filterfromatime]!=NULL && fp->criteria[filtertoatime]==NULL) ||
       (fp->criteria[filterfromatime]==NULL && fp->criteria[filtertoatime]!=NULL))
      {
      snprintf(OUTPUT,bufsize*2,"Incomplete atime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }
      
   if ((fp->criteria[filterfrommtime]!=NULL && fp->criteria[filtertomtime]==NULL) ||
       (fp->criteria[filterfrommtime]==NULL && fp->criteria[filtertomtime]!=NULL))
      {
      snprintf(OUTPUT,bufsize*2,"Incomplete mtime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   if ((fp->criteria[filterfromstime]!=NULL && fp->criteria[filtertostime]==NULL) ||
       (fp->criteria[filterfromstime]==NULL && fp->criteria[filtertostime]!=NULL))
      {
      snprintf(OUTPUT,bufsize*2,"Incomplete stime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   if ((fp->criteria[filterfromttime]!=NULL && fp->criteria[filtertottime]==NULL) ||
       (fp->criteria[filterfromttime]==NULL && fp->criteria[filtertottime]!=NULL))
      {
      snprintf(OUTPUT,bufsize*2,"Incomplete ttime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   
   if ((fp->criteria[filterfromsize]!=NULL && fp->criteria[filtertosize]==NULL) ||
       (fp->criteria[filterfromsize]==NULL && fp->criteria[filtertosize]!=NULL))
      {
      snprintf(OUTPUT,bufsize*2,"Incomplete size limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   if (fp->criteria[filterfromatime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromatime],t) > Date2Number(fp->criteria[filtertoatime],t))
	 {
	 snprintf(OUTPUT,bufsize*2,"To/From atimes silly in filter %s (from > to)",fp->alias);
	 CfLog(cferror,OUTPUT,"");
	 FatalError("Consistency errors");
	 }
      }

   if (fp->criteria[filterfromctime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromctime],t) > Date2Number(fp->criteria[filtertoctime],t))
	 {
	 snprintf(OUTPUT,bufsize*2,"To/From ctimes silly in filter %s (from > to)",fp->alias);
	 CfLog(cferror,OUTPUT,"");
	 FatalError("Consistency errors");
	 }
      }

   if (fp->criteria[filterfrommtime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfrommtime],t) > Date2Number(fp->criteria[filtertomtime],t))
	 {
	 snprintf(OUTPUT,bufsize*2,"To/From mtimes silly in filter %s (from > to)",fp->alias);
	 CfLog(cferror,OUTPUT,"");
	 FatalError("Consistency errors");
	 }
      }
   
   if (fp->criteria[filterfromstime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromstime],t) > Date2Number(fp->criteria[filtertostime],t))
	 {
	 snprintf(OUTPUT,bufsize*2,"To/From stimes silly in filter %s (from > to)",fp->alias);
	 CfLog(cferror,OUTPUT,"");
	 FatalError("Consistency errors");
	 }
      }

   if (fp->criteria[filterfromttime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromttime],t) > Date2Number(fp->criteria[filtertottime],t))
	 {
	 snprintf(OUTPUT,bufsize*2,"To/From ttimes silly in filter %s (from > to)",fp->alias);
	 CfLog(cferror,OUTPUT,"");
	 FatalError("Consistency errors");
	 }
      if (strncmp(fp->criteria[filterfromttime],"accumulated",strlen("accumulated")) != 0)
	 {
	 yyerror("Must use accumulated time in FromTtime");
	 }
      if (strncmp(fp->criteria[filtertottime],"accumulated",strlen("accumulated")) != 0)
	 {
	 yyerror("Must use accumulated time in ToTtime");
	 }
      }

   if (fp->criteria[filterfromsize] != NULL)
      {
      if (strcmp(fp->criteria[filterfromsize],fp->criteria[filtertosize]) > 0)
	 {
	 snprintf(OUTPUT,bufsize*2,"To/From size is silly in filter %s (from > to)",fp->alias);
	 CfLog(cferror,OUTPUT,"");
	 FatalError("Consistency errors");
	 }
      }

   }
}
   
/********************************************************************/

enum filternames FilterActionsToCode(filtertype)

char *filtertype;

{ int i;

Debug1("FilterActionsToCode(%s)\n",filtertype);

if (filtertype[strlen(filtertype)-1] != ':')
   {
   yyerror("Syntax error in filter type");
   return NoFilter;
   }

filtertype[strlen(filtertype)-1] = '\0';
 
for (i = 0; VFILTERNAMES[i] != '\0'; i++)
   {
   if (strcmp(VFILTERNAMES[i],filtertype) == 0)
      {
      return (enum filternames) i;
      }
   }

return (NoFilter);
}

/*******************************************************************/

int FilterExists(name)

char *name;

{ struct Filter *ptr;

for (ptr=VFILTERLIST; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->alias,name) == 0)
      {
      return true;
      }
   }
return false;
}

/*********************************************************************/

time_t Date2Number(string,now)

char *string;
time_t now;

 /*

  FromCtime: "date(1999,2,22,0,0,0)"
  FromMtime: "tminus(0,0,2,0,0,0)"
    
  */

{ char type[16],datestr[63];
  int year=-1,month=-1,day=-1,hr=-1,min=-1,sec=-1; 
  time_t time;
  struct tm tmv;
  
if (string == NULL || strlen (string) == 0)
   {
   return now;
   }

if (strcmp(string,"now") == 0)
   {
   return now;
   }

if (strcmp(string,"inf")==0)
   {
   return (time_t) (now|0x0FFFFFFF);  /* time_t is signed long, assumes 64 bits */
   }
 
sscanf(string,"%15[^(](%d,%d,%d,%d,%d,%d)",type,&year,&month,&day,&hr,&min,&sec);

if (year*month*day*hr*min*sec < 0)
   {
   yyerror("Bad relative time specifier, should be type(year,month,day,hr,min,sec)");
   return now;
   }

if (strcmp(type,"date")==0)
    
   {
   if (year < 1970 || year > 3000)
      {
      yyerror("Year value is silly (1970-3000)");
      return now;
      }
   
   if (month < 1 || month > 12)
      {
      yyerror("Month value is silly (1-12)");
      return now;
      }
   
   if (day < 1 || day > 31)
      {
      yyerror("Day value is silly (1-31)");
      return now;
      }
   
   if (hr > 23)
      {
      yyerror("Hour value is silly (0-23)");
      return now;
      }
   
   if (min > 59)
      {
      yyerror("Minute value is silly (0-59)");
      return now;
      }
   
   if (sec > 59)
      {
      yyerror("Second value is silly (0-59)");
      return now;
      }
   
   Debug("Time is %s or (y=%d,m=%d,d=%d,h=%d,m=%d,s=%d)\n",datestr,year,month,day,hr,min,sec);
   
   tmv.tm_year = year - 1900;
   tmv.tm_mon  = month -1;
   tmv.tm_mday = day;
   tmv.tm_hour = hr;
   tmv.tm_min  = min;
   tmv.tm_sec  = sec;
   tmv.tm_isdst= -1;
   
   if ((time=mktime(&tmv))== -1)
      {
      yyerror("Illegal time value");
      return now;
      }
   
   Debug("Time computed from input was: %s\n",ctime(&time));
   return time; 
   }
 
else if (strcmp(type,"tminus")==0)
    
   {
   if (year > 30)
      {
      yyerror("Relative Year value is silly (0-30 year ago)");
      return now;
      }
   
   Debug("Time is %s or (y=%d,m=%d,d=%d,h=%d,m=%d,s=%d)\n",datestr,year,month,day,hr,min,sec);
   
   time = CFSTARTTIME;

   time -= sec;
   time -= min * 60;
   time -= hr * 3600;
   time -= day * 24 * 3600;
   time -= month * 30 * 24 * 3600;
   time -= year * 365 * 24 * 3600;

   Debug("Total negative offset = %.1f minutes\n",(double)(CFSTARTTIME-time)/60.0);
   Debug("Time computed from input was: %s\n",ctime(&time));
   return time; 
   }
 
else if (strcmp(type,"accumulated")==0)
    
   {
   sscanf(string,"%8[^(](%d,%d,%d,%d,%d,%d)",type,&year,&month,&day,&hr,&min,&sec);
   if (year*month*day*hr*min*sec < 0)
      {
      yyerror("Bad cumulative time specifier, should be type(year,month,day,hr,min,sec)");
      return now;
      }

   time = 0;
   time += sec;
   time += min * 60;
   time += hr * 3600;
   time += day * 24 * 3600;
   time += month * 30 * 24 * 3600;
   time += year * 365 * 24 * 3600;
   return time;
   }

yyerror("Illegal time specifier"); 
return now;
}


/*******************************************************************/
/* Filter code                                                     */
/* These functions return true if the input survives the filters   */
/* Several filters are ANDed together....                          */
/*******************************************************************/

int ProcessFilter(proc,filterlist,names,start,end)

char *proc, **names;
int *start,*end;
struct Item *filterlist;

{ struct Item *tests = NULL;
  struct Filter *fp;
  int result = true, tmpres, i;
  char *line[noproccols];
  
Debug("ProcessFilter(%.6s...)\n",proc);

if (filterlist == NULL)
   {
   return true;
   }

if (strlen(proc) == 0)
   {
   return false;
   }
 
SplitLine(proc,filterlist,names,start,end,line);

for (fp = VFILTERLIST; fp != NULL; fp=fp->next)
   {
   if (IsItemIn(filterlist,fp->alias))
      {
      Debug ("Applying filter %s\n",fp->alias);

      if (fp->criteria[filterresult] == NULL)
	 {
	 fp->criteria[filterresult] = strdup("Owner.PID.PPID.PGID.RSize.VSize.Status.Command.TTime.STime.TTY.Priority.Threads");
	 }
      
      DoProc(&tests,fp->criteria,names,line);

      if (tmpres = EvaluateORString(fp->criteria[filterresult],tests))
	 {
	 AddMultipleClasses(fp->defines);
	 }
      else
	 {
	 AddMultipleClasses(fp->elsedef);
	 }

      result &= tmpres;
      }   
   }

for (i = 0; i < noproccols; i++)
   {
   if (line[i] != NULL)
      {
      free(line[i]);
      }
   }

DeleteItemList(tests);
return result; 
}

/*******************************************************************/

int FileObjectFilter(file,lstatptr,filterlist,context)

char *file;
struct stat *lstatptr;
struct Item *filterlist;
enum actions context;

{ struct Item *tests = NULL;
  struct Filter *fp;  int result = true, tmpres;
  
Debug("FileObjectFilter(%s)\n",file);
 
if (filterlist == NULL)
   {
   return true;
   }

for (fp = VFILTERLIST; fp != NULL; fp=fp->next)
   {
   if (IsItemIn(filterlist,fp->alias))
      {
      Debug ("Applying filter %s\n",fp->alias);
      
      if (fp->criteria[filterresult] == NULL)
	 {
	 fp->criteria[filterresult] = strdup("Type.Owner.Group.Mode.Ctime.Mtime.Atime.Size.ExecRegex.NameRegex.IsSymLinkTo.Exec");
	 }
      
      DoFilter(&tests,fp->criteria,lstatptr,file);

      if (tmpres = EvaluateORString(fp->criteria[filterresult],tests))
	 {
	 AddMultipleClasses(fp->defines);
	 }
      else
	 {
	 AddMultipleClasses(fp->elsedef);
	 }

      result &= tmpres;
      }
   }
 
DeleteItemList(tests);
Debug("Filter result on %s was %d\n",file,result);
return result; 
}

/*******************************************************************/

void DoFilter(attr,crit,lstatptr,filename)

struct Item **attr;
char **crit,*filename;
struct stat *lstatptr;

{
if (crit[filtertype] != NULL)
   {
   if (FilterTypeMatch(lstatptr,crit[filtertype]))
      {
      PrependItem(attr,"Type","");
      }
   }

if (crit[filterowner] != NULL)
   {
   if (FilterOwnerMatch(lstatptr,crit[filterowner]))
      {
      PrependItem(attr,"Owner","");
      }
   }

if (crit[filtergroup] != NULL)
   {
   if (FilterGroupMatch(lstatptr,crit[filtergroup]))
      {
      PrependItem(attr,"Group","");
      }
   }

if (crit[filtermode] != NULL)
   {
   if (FilterModeMatch(lstatptr,crit[filtermode]))
      {
      PrependItem(attr,"Mode","");
      }
   }

 if (crit[filterfromatime] != NULL)
   {
   if (FilterTimeMatch(lstatptr->st_atime,crit[filterfromatime],crit[filtertoatime]))
      {
      PrependItem(attr,"Atime","");
      }
   }

if (crit[filterfromctime] != NULL)
   {
   if (FilterTimeMatch(lstatptr->st_ctime,crit[filterfromctime],crit[filtertoctime]))
      {
      PrependItem(attr,"Ctime","");
      }
   }

if (crit[filterfrommtime] != NULL)
   {
   if (FilterTimeMatch(lstatptr->st_mtime,crit[filterfrommtime],crit[filtertomtime]))
      {
      PrependItem(attr,"Mtime","");
      }
   }

if (crit[filternameregex] != NULL)
   {
   if (FilterNameRegexMatch(filename,crit[filternameregex]))
      {
      PrependItem(attr,"NameRegex","");
      }
   }

if ((crit[filtersymlinkto] != NULL) && (S_ISLNK(lstatptr->st_mode)))
   {
   if (FilterIsSymLinkTo(filename,crit[filtersymlinkto]))
      {
      PrependItem(attr,"IsSymLinkTo","");
      }
   }

if (crit[filterexecregex] != NULL)
   {
   if (FilterExecRegexMatch(filename,crit[filterexecregex]))
      {
      PrependItem(attr,"ExecRegex","");
      }
   }

if (crit[filterexec] != NULL)
   {
   if (FilterExecMatch(filename,crit[filterexec]))
      {
      PrependItem(attr,"Exec","");
      }
   }
}

/*******************************************************************/

void DoProc(attr,crit,names,line)

struct Item **attr;
char **crit,**names,**line;

{
if (crit[filterowner] != NULL)
   {
   if (FilterProcMatch("UID","UID",crit[filterowner],names,line))
      {
      PrependItem(attr,"Owner","");
      }
   }
else
   {
   PrependItem(attr,"Owner","");
   }

 if (crit[filterpid] != NULL)
   {
   if (FilterProcMatch("PID","PID",crit[filterpid],names,line))
      {
      PrependItem(attr,"PID","");
      }
   }
else
   {
   PrependItem(attr,"PID","");
   }

if (crit[filterppid] != NULL)
   {
   if (FilterProcMatch("PPID","PPID",crit[filterppid],names,line))
      {
      PrependItem(attr,"PPID","");
      }
   }
else
   {
   PrependItem(attr,"PPID","");
   }

if (crit[filterpgid] != NULL)
   {
   if (FilterProcMatch("PGID","PGID",crit[filterpgid],names,line))
      {
      PrependItem(attr,"PGID","");
      }
   }
else
   {
   PrependItem(attr,"PGID","");
   }

if (crit[filtersize] != NULL)
   {
   if (FilterProcMatch("SZ","VSZ",crit[filtersize],names,line))
      {
      PrependItem(attr,"VSize","");
      }
   }
else
   {
   PrependItem(attr,"VSize","");
   }

if (crit[filterrsize] != NULL)
   {
   if (FilterProcMatch("RSS","RSS",crit[filterrsize],names,line))
      {
      PrependItem(attr,"RSize","");
      }
   }
else
   {
   PrependItem(attr,"RSize","");
   }
 
if (crit[filterstatus] != NULL)
   {
   if (FilterProcMatch("S","STAT",crit[filterstatus],names,line))
      {
      PrependItem(attr,"Status","");
      }
   }
else
   {
   PrependItem(attr,"Status","");
   }
 
if (crit[filtercmd] != NULL)
   {
   if (FilterProcMatch("CMD","COMMAND",crit[filtercmd],names,line))
      {
      PrependItem(attr,"Command","");
      }
   }
else
   {
   PrependItem(attr,"Command","");
   }
 
if (crit[filterfromttime] != NULL)
   {
   if (FilterProcTTimeMatch("TIME","TIME",crit[filterfromttime],crit[filtertottime],names,line))
      {
      PrependItem(attr,"TTime","");
      }
   }
else
   {
   PrependItem(attr,"TTime","");
   }
 
if (crit[filterfromstime] != NULL)
   {
   if (FilterProcSTimeMatch("STIME","START",crit[filterfromstime],crit[filtertostime],names,line))
      {
      PrependItem(attr,"STime","");
      }
   }
else
   {
   PrependItem(attr,"STime","");
   }
 
if (crit[filtertty] != NULL)
   {
   if (FilterProcMatch("TTY","TTY",crit[filtertty],names,line))
      {
      PrependItem(attr,"TTY","");
      }
   }
else
   {
   PrependItem(attr,"TTY","");
   }

if (crit[filterpriority] != NULL)
   {
   if (FilterProcMatch("NI","PRI",crit[filterpriority],names,line))
      {
      PrependItem(attr,"Priority","");
      }
   }
else
   {
   PrependItem(attr,"Priority","");
   }
 
if (crit[filterthreads] != NULL)
   {
   if (FilterProcMatch("NLWP","NLWP",crit[filterthreads],names,line))
      {
      PrependItem(attr,"Threads","");
      }
   }
else
   {
   PrependItem(attr,"Threads","");
   }
} 

/*******************************************************************/

int FilterTypeMatch(lstatptr,crit)

struct stat *lstatptr;
char *crit;

{ struct Item *attrib = NULL;

if (S_ISLNK(lstatptr->st_mode))
   {
   PrependItem(&attrib,"link","");
   }
 
if (S_ISREG(lstatptr->st_mode))
   {
   PrependItem(&attrib,"reg","");
   }

if (S_ISDIR(lstatptr->st_mode))
   {
   PrependItem(&attrib,"dir","");
   }
 
if (S_ISFIFO(lstatptr->st_mode))
    {
    PrependItem(&attrib,"fifo","");
    }
 
if (S_ISSOCK(lstatptr->st_mode))
   {
   PrependItem(&attrib,"socket","");
   }

if (S_ISCHR(lstatptr->st_mode))
   {
   PrependItem(&attrib,"char","");
   }

if (S_ISBLK(lstatptr->st_mode))
   {
   PrependItem(&attrib,"block","");
   }

#ifdef HAVE_DOOR_CREATE
if (S_ISDOOR(lstatptr->st_mode))
   {
   PrependItem(&attrib,"door","");
   }
#endif
 
if (EvaluateORString(crit,attrib))
   {
   DeleteItemList(attrib);
   return true;
   }
else
   {
   DeleteItemList(attrib);
   return false;
   }
}

/*******************************************************************/

int FilterProcMatch(name1,name2,expr,names,line)

char *name1,*name2,*expr;
char **names,**line;

{ int i;
  regex_t rx;
  regmatch_t pmatch;

if (CfRegcomp(&rx,expr,REG_EXTENDED) != 0)
   {
   return false;
   }
 
for (i = 0; names[i] != NULL; i++)
   {
   if ((strcmp(names[i],name1) == 0) || (strcmp(names[i],name2) == 0))
      {
      if (regexec(&rx,line[i],1,&pmatch,0) == 0)
	 {
	 if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(line[i])))
	    {
	    regfree(&rx);
	    return true;
	    }
	 }
      
      regfree(&rx);
      return false;      
      }
   }

regfree(&rx);
return false; 
}

/*******************************************************************/

int FilterProcSTimeMatch(name1,name2,fromexpr,toexpr,names,line)

char *name1,*name2,*fromexpr,*toexpr;
char **names,**line;

{ int i;
  time_t fromtime,totime, now = CFSTARTTIME,pstime;
  char year[5],month[4],hr[3],min[3],day[3],timestr[256];
  
bzero(year,5);
strcpy(year,VYEAR);
bzero(month,4); 
strcpy(month,VMONTH);
bzero(day,3); 
strcpy(day,VDAY);
bzero(hr,3); 
strcpy(hr,VHR);
bzero(min,3); 
strcpy(min,VMINUTE); 

fromtime = Date2Number(fromexpr,now);
totime = Date2Number(toexpr,now);

for (i = 0; names[i] != NULL; i++)
   {
   if ((strcmp(names[i],name1) == 0) || (strcmp(names[i],name2) == 0))
      {
      if (strstr(line[i],":")) /* Hr:Min:Sec */
	 {
         sscanf(line[i],"%2[^:]:%2[^:]:",hr,min);
	 snprintf(timestr,256,"date(%s,%d,%s,%s,%s,0)",year,Month2Number(month),day,hr,min);
	 }
      else                     /* date Month */
	 {
         sscanf(line[i],"%3[a-zA-Z] %2[0-9]",month,day);
	 snprintf(timestr,256,"date(%s,%d,%s,%s,%s,0)",year,Month2Number(month),day,hr,min);
	 }
      
      if (Month2Number(month) < 0)
	 {
	 continue;
	 }
      
      pstime = Date2Number(timestr,now);

      Debug("Stime %s converted to %s\n",timestr,ctime(&pstime));

      return ((fromtime < pstime) && (pstime < totime));
      }
   } 
return false;
}

/*******************************************************************/

int FilterProcTTimeMatch(name1,name2,fromexpr,toexpr,names,line)

char *name1,*name2,*fromexpr,*toexpr;
char **names,**line;

{ int i,hr=0,min=-1,sec=-1;
  time_t fromtime,totime,now = CFSTARTTIME,pstime = CFSTARTTIME;
  char timestr[256];

fromtime = Date2Number(fromexpr,now);
totime = Date2Number(toexpr,now);

for (i = 0; names[i] != NULL; i++)
   {
   if ((strcmp(names[i],name1) == 0) || (strcmp(names[i],name2) == 0))
      {
      if (strstr(line[i],":")) /* Hr:Min:Sec */
	 {
         sscanf(line[i],"%d:%d",&min,&sec);

	 if (sec < 0 || min < 0)
	    {
	    CfLog(cferror,"Parse error checking process time","");
	    return false;
	    }
	 
	 snprintf(timestr,256,"accumulated(0,0,0,%d,%d,%d)",hr,min,sec);
	 pstime = Date2Number(timestr,now);
	 }

      return ((fromtime < pstime) && (pstime < totime));
      }
   } 
return false;
}

/*******************************************************************/

int FilterOwnerMatch(lstatptr,crit)

char *crit;
struct stat *lstatptr;

{ struct Item *attrib = NULL;
  char buffer[64];
  struct passwd *pw;

sprintf(buffer,"%d",lstatptr->st_uid);
PrependItem(&attrib,buffer,""); 

if ((pw = getpwuid(lstatptr->st_uid)) != NULL)
   {
   PrependItem(&attrib,pw->pw_name,""); 
   }
else
   {
   PrependItem(&attrib,"none",""); 
   }
 
if (EvaluateORString(crit,attrib))
   {
   DeleteItemList(attrib);
   return true;
   }
else
   {
   DeleteItemList(attrib);
   return false;
   }
} 

/*******************************************************************/

int FilterGroupMatch(lstatptr,crit)

char *crit;
struct stat *lstatptr;

{ struct Item *attrib = NULL;
  char buffer[64];
  struct group *gr;

sprintf(buffer,"%d",lstatptr->st_gid);
PrependItem(&attrib,buffer,""); 

if ((gr = getgrgid(lstatptr->st_gid)) != NULL)
   {
   PrependItem(&attrib,gr->gr_name,""); 
   }
else
   {
   PrependItem(&attrib,"none",""); 
   }
 
if (EvaluateORString(crit,attrib))
   {
   DeleteItemList(attrib);
   return true;
   }
else
   {
   DeleteItemList(attrib);
   return false;
   }
} 

/*******************************************************************/

int FilterModeMatch(lstatptr,crit)

char *crit;
struct stat *lstatptr;

{ mode_t plusmask,minusmask;

ParseModeString(crit,&plusmask,&minusmask);

return ((lstatptr->st_mode & plusmask) && !(lstatptr->st_mode & minusmask));
} 

/*******************************************************************/

int FilterTimeMatch(stattime,from,to)

char *from,*to;
time_t stattime;

{ time_t fromtime,totime, now = CFSTARTTIME;

fromtime = Date2Number(from,now);
totime = Date2Number(to,now);

return ((fromtime < stattime) && (stattime < totime));
} 

/*******************************************************************/

int FilterNameRegexMatch(filename,crit)

char *filename,*crit;

{ regex_t rx;
  regmatch_t pmatch;

if (CfRegcomp(&rx,crit,REG_EXTENDED) != 0)
   {
   return false;
   }            

if (regexec(&rx,filename,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(filename)))
      {
      regfree(&rx);
      return true;
      }
  }

regfree(&rx);
return false;      
}

/*******************************************************************/

int FilterExecRegexMatch(filename,crit)

char *filename,*crit;

{ regex_t rx;
  regmatch_t pmatch;
  char buffer[bufsize],expr[bufsize],line[bufsize],*sp;
  FILE *pp;

AddMacroValue("this",filename);
VBUFF[0] = '\0'; 
ExpandVarstring(crit,VBUFF,NULL);
DeleteMacro("this");
  
bzero(buffer,bufsize);
bzero(line,bufsize); 
bzero(expr,bufsize);

for (sp = VBUFF+strlen(VBUFF)-1; (*sp != '(') && (sp > VBUFF); sp--)
   {
   }
 
sscanf(sp+1,"%256[^)]",expr);
strncpy(buffer,VBUFF,(int)(sp-VBUFF));
 
if (CfRegcomp(&rx,expr,REG_EXTENDED) != 0)
   {
   return false;
   }            

if ((pp = cfpopen(buffer,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open pipe to command %s\n",buffer);
   CfLog(cferror,OUTPUT,"cfpopen");
   return false;
   }
 
ReadLine(line,bufsize,pp);  /* One buffer only */

cfpclose(pp); 
 
if (regexec(&rx,line,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(line)))
      {
      regfree(&rx);
      return true;
      }
   }
 
regfree(&rx);
return false;      
}

/*******************************************************************/

int FilterIsSymLinkTo(filename,crit)

char *filename,*crit;

{ regex_t rx;
  regmatch_t pmatch;
  char buffer[bufsize];

if (CfRegcomp(&rx,crit,REG_EXTENDED) != 0)
   {
   return false;
   }

bzero(buffer,bufsize);
 
if (readlink(filename,buffer,bufsize-1) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"Unable to read link %s in filter",filename);
   CfLog(cferror,OUTPUT,"readlink");
   regfree(&rx);
   return false;      
   }

if (regexec(&rx,buffer,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(buffer)))
      {
      regfree(&rx);
      return true;
      }
  }

regfree(&rx);
return false;      
}

/*******************************************************************/

int FilterExecMatch(filename,crit)

char *filename, *crit;

  /* command can include $(this) for the name of the file */

{
AddMacroValue("this",filename);
VBUFF[0] = '\0'; 
ExpandVarstring(crit,VBUFF,NULL);
DeleteMacro("this");

Debug("Executing filter command [%s]\n",VBUFF);

if (ShellCommandReturnsZero(VBUFF))
   {
   return true;
   }
else
   {
   return false;
   }
}

/*******************************************************************/

void GetProcessColumns(proc,names,start,end)

char *proc, **names;
int *start,*end;

{ char *sp,title[16];
  int col,offset = 0;
  
for (col = 0; col < noproccols; col++)
   {
   start[col] = end[col] = -1;
   names[col] = NULL;
   }

col = 0; 
 
for (sp = proc; *sp != '\0'; sp++)
   {
   offset = sp - proc;

   if (isspace((int)*sp))
      {
      if (start[col] != -1)
	 {
	 Debug("End of %s is %d\n",title,offset-1);
	 end[col++] = offset - 1;
	 if (col > noproccols - 1)
	    {
	    CfLog(cferror,"Column overflow in process table","");
	    break;
	    }
	 }
      continue;
      }

   else if (start[col] == -1)
      {
      start[col] = offset;
      Debug("Start of %s is %d\n",title,offset);
      sscanf(sp,"%15s",title);
      names[col] = strdup(title);
      Debug("Col[%d]=%s\n",col,names[col]);
      }
   }

if (end[col] == -1)
   {
   Debug("End of %s is %d\n",title,offset);
   end[col] = offset;
   }
}

/*******************************************************************/

void SplitLine(proc,filterlist,names,start,end,line)

char *proc, **names, **line;
int *start,*end;
struct Item *filterlist;

{ int i,s,e;

for (i = 0; i < noproccols; i++)
   {
   line[i] = NULL;
   }
 
for (i = 0; names[i] != NULL; i++)
   {
   s = start[i];
   e = end[i];

   for (s = start[i]; (s >= 0) && !isspace((int)*(proc+s)); s--)
      {
      }

   if (s < 0)
      {
      s = 0;
      }
   
   if (isspace((int)proc[s]))
      {
      s++;
      }

   if (strcmp(names[i],"CMD") == 0 || strcmp(names[i],"COMMAND") == 0)
      {
      e = strlen(proc);
      }
   else
      {
      for (e = end[i]; (e <= end[i]+10) && !isspace((int)*(proc+e)); e++)
	 {
	 }
      
      if (isspace((int)proc[e]))
	 {
	 if (e > 0)
	    {
	    e--;
	    }
	 }
      }

   line[i] = (char *)malloc(e-s+2);

   bzero(line[i],(e-s+2));
   strncpy(line[i],(char *)(proc+s),(e-s+1));
   
   Debug("  %s=(%s) of [%s]\n",names[i],line[i],proc);
   }

Debug("------------------------------------\n"); 
}
