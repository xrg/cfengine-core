/* 

        Copyright (C) 1995-
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
/* File: functions.c                                                         */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*********************************************************************/

int IsBuiltinFunction(item)

char *item;

{ char name[maxvarsize],args[bufsize];
  char c1 = '?',c2 = '?' ;

name[0] = '\0';
args[0] = '\0';
    
sscanf(item,"%255[a-zA-Z0-9_]%c%255[^)]%c",name,&c1,args,&c2);

if (c1 != '(' || c2 != ')')
   {
   if (PARSING)
      {
      yyerror("Empty or bad function argument");
      }
   return false;
   }

Debug("IsBuiltinFunction: %s(%s)\n",name,args);

 if (strlen(args) == 0)
   {
   if (PARSING)
      {
      yyerror("Empty function argument");
      }
   return false;
   }
 else
    {
    }
return true; 
}

/*********************************************************************/

char *EvaluateFunction(f,value)

char *f,*value;

{ enum builtin fn;
  char name[maxvarsize],vargs[bufsize],args[bufsize];

sscanf(f,"%255[^(](%255[^)])",name,vargs);
ExpandVarstring(vargs,args,NULL); 
Debug("HandleFunction: %s(%s)\n",name,args);

switch (fn = FunctionStringToCode(name))
   {
   case fn_randomint:
       GetRandom(args,value);
       break;
   case fn_newerthan:
   case fn_accessedbefore:
   case fn_changedbefore:
       HandleCompareStat(fn,args,value);
       break;
   case fn_fileexists:
   case fn_isdir:
   case fn_islink:
   case fn_isplain:
       HandleStatInfo(fn,args,value);
       break;
   case fn_execresult:
       HandleFunctionExec(args,value);
       break;
   case fn_returnszero:
       HandleReturnsZero(args,value);
       break;
   case fn_iprange:
       HandleIPRange(args,value);
       break;
   case fn_isdefined:
       HandleIsDefined(args,value);
       break;
   case fn_strcmp:
       HandleStrCmp(args,value);
       break;
   case fn_showstate:
       HandleShowState(args,value);
       break;
   case fn_readfile:
       HandleReadFile(args,value);
       break;
   case fn_syslog:
       HandleSyslogFn(args,value);
       break;

   case fn_setstate:
       HandleSetState(args,value);
       break;
       
   case fn_unsetstate:
       HandleUnsetState(args,value);
       break;
       
   case fn_returnvars:

       if (ScopeIsMethod())
	  {
	  HandleReturnValues(args,value);
	  ACTIONPENDING = false;
	  }
       else
	  {
	  snprintf(OUTPUT,bufsize,"Function %s can only be used within a private method context",f);
	  yyerror(OUTPUT);
	  }
       break;
       
   case fn_returnclasses:
       
       if (ScopeIsMethod())
	  {
	  HandleReturnClasses(args,value);
	  ACTIONPENDING = false;
	  }
       else
	  {
	  snprintf(OUTPUT,bufsize,"Function %s can only be used within a private method context",f);
	  yyerror(OUTPUT);
	  }
       break;
       
   default: snprintf(OUTPUT,bufsize,"No such builtin function %s\n",f);
       CfLog(cferror,OUTPUT,"");
   }
 
return value;
}

/*********************************************************************/
/* level 2                                                           */
/*********************************************************************/

enum builtin FunctionStringToCode(str)

char *str;

{ char *sp;
  int i;
  enum builtin fn;

fn = nofn;
 
for (sp = str; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   if (*sp == ':')
      {
      *sp = '\0';
      }
   }

for (i = 1; BUILTINS[i] != NULL; i++)
   {
   if (strcmp(BUILTINS[i],str) == 0)
      {
      fn = (enum builtin) i;
      break;
      }
   }

if (fn == nofn)
  {
  snprintf(OUTPUT,bufsize,"Internal function (%s) not recognized",str);
  yyerror(OUTPUT);
  FatalError("Could not parse function");
  }

return (enum builtin) i;
}

/*********************************************************************/

void GetRandom(args,value)

char *args,*value;

{ int result,count=0,from=-1,to=-1;
 char tbuff[bufsize],fbuff[bufsize];

if (ACTION != control)
   {
   yyerror("Use of RandomInt(a,b) outside of variable assignment");
   }
  

TwoArgs(args,fbuff,tbuff); 
from = atoi(fbuff);
to = atoi(tbuff);
 
if ((from < 0) || (to < 0) || (from == to))
   {
   yyerror("RandomInt(a,b) must have different non-negative arguments < INT_MAX");
   return;
   }

if (from > to)
   {
   yyerror("RandomInt(a,b) - b was less than a");
   return;
   }

result = from + (int)(drand48()*(double)(to-from));
Debug("RandomInt(%u)\n",result);
snprintf(value,bufsize,"%u",result); 
}

/*********************************************************************/

void HandleStatInfo(fn,args,value)

enum builtin fn;
char *args,*value;

{ struct stat statbuf;

if (strchr(args,','))
   {
   yyerror("Illegal argument to unary class-function");
   return;
   }

if (lstat(args,&statbuf) == -1)
   {
   strcpy(value,CF_NOCLASS);
   return;
   }
 else
    {
    if (fn == fn_fileexists)
       {
       strcpy(value,CF_ANYCLASS);
       return;
       }
    }
 
strcpy(value,CF_NOCLASS);

 switch(fn)
    {
    case fn_isdir:
	if (S_ISDIR(statbuf.st_mode))
	   {
	   strcpy(value,CF_ANYCLASS);
	   return;
	   }
	break;
    case fn_islink:
	if (S_ISLNK(statbuf.st_mode))
	   {
	   strcpy(value,CF_ANYCLASS);
	   return;
	   }
	break;
    case fn_isplain:
	if (S_ISREG(statbuf.st_mode))
	   {
	   strcpy(value,CF_ANYCLASS);
	   return;
	   }
	break;
    }
 
strcpy(value,CF_NOCLASS);
}

/*********************************************************************/

void HandleIPRange(args,value)

char *args,*value;

{ struct stat statbuf;
 
if (strchr(args,','))
   {
   yyerror("Illegal argument to unary class-function");
   return;
   }
 
strcpy(value,CF_NOCLASS);

if (!FuzzyMatchParse(args))
   {
   return;
   }

if (FuzzySetMatch(args,VIPADDRESS) == 0)
   {
   strcpy(value,CF_ANYCLASS);
   }
}

/*********************************************************************/

void HandleCompareStat(fn,args,value)

enum builtin fn;
char *args,*value;

{ struct stat frombuf,tobuf;
  char *sp,from[bufsize],to[bufsize];
  int count = 0;

TwoArgs(args,from,to); 
strcpy(value,CF_NOCLASS);
 
if (stat(from,&frombuf) == -1)
   {
   return;
   }

if (stat(to,&tobuf) == -1)
   {
   return;
   }

switch(fn)
   {
   case fn_newerthan:
       if (frombuf.st_mtime < tobuf.st_mtime)
	  {
	  strcpy(value,CF_ANYCLASS);
	  return;
	  }
       break;

   case fn_accessedbefore:
       if (frombuf.st_atime < tobuf.st_atime)
	  {
	  strcpy(value,CF_ANYCLASS);
	  return;
	  }
       break;

   case fn_changedbefore:
       if (frombuf.st_ctime < tobuf.st_ctime)
	  {
	  strcpy(value,CF_ANYCLASS);
	  return;
	  }       
       break;
   }

strcpy(value,CF_NOCLASS);
}


/*********************************************************************/

void HandleFunctionExec(args,value)

char *args,*value;

{ char command[maxvarsize];

 if (ACTION != control)
   {
   yyerror("Use of ExecResult(s) outside of variable assignment");
   }
 
if (*args == '/')
   {
   strncpy(command,args,maxvarsize);
   GetExecOutput(command,value);
   Chop(value);
   value[maxvarsize-1] = '\0';  /* Truncate to maxvarsize */
   }
 else
    {
    yyerror("ExecResult(/command) must specify an absolute path");
    } 
}

/*********************************************************************/

void HandleReturnsZero(args,value)

char *args,*value;

{ char command[bufsize];

if (ACTION != groups)
   {
   yyerror("Use of ReturnsZero(s) outside of class assignment");
   }

Debug("HandleReturnsZero(%s)\n",args); 
 
if (*args == '/')
   {
   strncpy(command,args,bufsize);
   
   if (ShellCommandReturnsZero(command))
      {
      strcpy(value,CF_ANYCLASS);
      return;
      }
   }
 else
    {
    yyerror("ExecResult(/command) must specify an absolute path");
    }

strcpy(value,CF_NOCLASS); 
}


/*********************************************************************/

void HandleIsDefined(args,value)

char *args,*value;

{
if (ACTION != groups)
   {
   yyerror("Use of IsDefined(s) outside of class assignment");
   }

Debug("HandleIsDefined(%s)\n",args); 
 
if (GetMacroValue(CONTEXTID,args))
   {
   strcpy(value,CF_ANYCLASS);
   return;
   }

strcpy(value,CF_NOCLASS); 
}

/*********************************************************************/

void HandleSyslogFn(args,value)

char *args,*value;

{ char from[bufsize],to[bufsize];
  int priority = LOG_ERR;

TwoArgs(args,from,to);

value[0] = '\0';

if (strcmp(from,"LOG_EMERG") == 0)
   {
   priority = LOG_EMERG;
   }
else if (strcmp(from,"LOG_ALERT") == 0)
   {
   priority = LOG_ALERT;
   }
else if (strcmp(from,"LOG_CRIT") == 0)
   {
   priority = LOG_CRIT;
   }
else if (strcmp(from,"LOG_NOTICE") == 0)
   {
   priority = LOG_NOTICE;
   } 
else if (strcmp(from,"LOG_WARNING") == 0)
   {
   priority = LOG_WARNING;
   }
else if (strcmp(from,"LOG_ERR") == 0)
   {
   priority = LOG_ERR;
   }
else
   {
   snprintf(OUTPUT,bufsize,"Unknown syslog priority (%s) - changing to LOG_ERR",from);
   CfLog(cferror,OUTPUT,"");
   priority = LOG_ERR;
   }

Debug("Alerting to syslog(%s,%s)\n",from,to);

if (!DONTDO)
   {
   syslog(priority," %s",to);
   } 
}


/*********************************************************************/

void HandleStrCmp(args,value)

char *args,*value;

{ char *sp,from[bufsize],to[bufsize];
  int count = 0;

TwoArgs(args,from,to); 
 
if (strcmp(from,to) == 0)
   {
   strcpy(value,CF_ANYCLASS); 
   }
else
   {
   strcpy(value,CF_NOCLASS);
   } 
}


/*********************************************************************/

void HandleReadFile(args,value)

char *args,*value;

{ char *sp,filename[bufsize],maxbytes[bufsize];
  int val = 0;
  FILE *fp;

TwoArgs(args,filename,maxbytes);
 
val = atoi(maxbytes);

if ((fp = fopen(filename,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize,"Could open ReadFile(%s)\n",filename);
   CfLog(cferror,OUTPUT,"fopen");
   return;
   }

if (val > bufsize - buffer_margin)
   {
   snprintf(OUTPUT,bufsize,"ReadFile() will not read more than %d bytes",bufsize - buffer_margin);
   CfLog(cferror,OUTPUT,"");
   fclose(fp);
   return;
   }

bzero(value,bufsize); 
fread(value,val,1,fp);
 
fclose(fp); 
}


/*********************************************************************/

void HandleReturnValues(args,value)

char *args,*value;

{
Verbose("This is a method with return value list: (%s)\n",args);

 if (strlen(METHODRETURNVARS) == 0)
    {
    strncpy(METHODRETURNVARS,args,bufsize-1);
    }
 else
    {
    yyerror("Redefinition of method return values");
    }

strcpy(value,"noinstall");
}

/*********************************************************************/

void HandleReturnClasses(args,value)

char *args,*value;

{
Verbose("This is a method with return class list: %s\n",args);

 if (strlen(METHODRETURNCLASSES) == 0)
    {
    strncpy(METHODRETURNCLASSES,args,bufsize-1);
    }
 else
    {
    yyerror("Redefinition of method return classes");
    }

 strcpy(value,"noinstall");
}

/*********************************************************************/

void HandleShowState(args,value)

char *args,*value;

{ struct stat statbuf;
  char buffer[bufsize];
  struct Item *addresses = NULL,*ip;
  FILE *fp;
  int i = 0;
  
if (PARSING)
   {
   return;
   }
  
if ((ACTION != alerts) && PARSING)
   {
   yyerror("Use of ShowState(type) outside of alert declaration");
   }

Debug("ShowState(%s)\n",args); 

snprintf(buffer,bufsize,"%s/state/cf_%s",WORKDIR,args);

if (stat(buffer,&statbuf) == 0)
   {
   if ((fp = fopen(buffer,"r")) == NULL)
      {
      snprintf(OUTPUT,bufsize,"Could not open state memory %s\n",buffer);
      CfLog(cfinform, OUTPUT,"fopen");
      return;
      }

   printf("%s: -----------------------------------------------------------------------------------\n",VPREFIX);
   printf("%s: In the last 40 minutes, the peak state was:\n",VPREFIX);
   while(!feof(fp))
      {
      char *sp,local[bufsize],remote[bufsize];
      buffer[0] = local[0] = remote[0] = '\0';
      
      fgets(buffer,bufsize,fp);
      i++;

      if (strlen(buffer) > 0)
	 {
	 printf("%s: (%2d) %s",VPREFIX,i,buffer);

	 if (strncmp(args,"incoming",8) == 0 || strncmp(args,"outgoing",8) == 0)
	    {
	    if (strncmp(buffer,"tcp",3) == 0)
	       {
	       sscanf(buffer,"%*s %*s %*s %s %s",local,remote); /* linux-like */
	       }
	    else
	       {
	       sscanf(buffer,"%s %s",local,remote);             /* solaris-like */
	       }
	    
	    strncpy(VBUFF,remote,bufsize-1);
	    
	    for (sp = VBUFF+strlen(VBUFF)-1; isdigit((int)*sp); sp--)
	       {	    
	       }
	    
	    *sp = '\0';
	    
	    if (!IsItemIn(addresses,VBUFF))
	       {
	       AppendItem(&addresses,VBUFF,"");
	       }
	    }
	 }
      }
   
   fclose(fp);

   if (addresses != NULL)
      {
      printf("\n");
      }

   for (ip = addresses; ip != NULL; ip=ip->next)
      {
      printf(" DNS key: %s = %s\n",ip->name,IPString2Hostname(ip->name));
      }

   if (addresses != NULL)
      {
      printf("\n");
      }

   printf("%s: -----------------------------------------------------------------------------------\n",VPREFIX);
   snprintf(buffer,bufsize,"State of %s peaked at %s\n",args,ctime(&statbuf.st_mtime));
   strcpy(value,buffer);
   }
else 
   {
   snprintf(buffer,bufsize,"State parameter %s is not known or recorded\n",args);   
   strcpy(value,buffer);
   }
}

/*********************************************************************/

void HandleSetState(args,value)

char *args,*value;

{ char name[bufsize],ttlbuf[bufsize],policy[bufsize];
 unsigned int ttl = 0;

value[0] = '\0';
 
ThreeArgs(args,name,ttlbuf,policy);
ttl = atoi(ttlbuf);

Debug("HandleSetState(%s,%d,%s)\n",name,ttl,policy);
 
if (ttl == 0)
   {
   yyerror("No expiry date set on persistent class");
   return;
   }

if (strcmp(ToLowerStr(policy),"preserve") == 0)
   {
   if (!PARSING && !DONTDO)
      {
      AddPersistentClass(name,ttl,cfpreserve);
      }
   }
else if (strcmp(ToLowerStr(policy),"reset") == 0)
   {
   if (!PARSING & !DONTDO)
      {
      AddPersistentClass(name,ttl,cfreset);
      }
   }
else
   {
   yyerror("Unknown update policy");
   }
}

/*********************************************************************/

void HandleUnsetState(args,value)

char *args,*value;

{ char arg1[bufsize];
 
value[0] = '\0';
OneArg(args,arg1);

Debug("HandleUnsetState(%s)\n",arg1); 
DeletePersistentClass(arg1);
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void OneArg(args,arg1)

char *args,*arg1;

{ char one[bufsize];

bzero(one,bufsize);
 
if (strchr(args,','))
   {
   yyerror("Illegal argument to unary function");
   return;
   }

strcpy(arg1,UnQuote(args));
}

/*********************************************************************/

void TwoArgs(args,arg1,arg2)

char *args,*arg1,*arg2;

{ char one[bufsize],two[bufsize];
  char *sp;
  int count = 0;
 
bzero(one,bufsize);
bzero(two,bufsize); 
 
for (sp = args; *sp != '\0'; sp++)
   {
   if (*sp == ',')
      {
      count++;
      }
   }

if (count != 1)
   {
   yyerror("Two arguments to function required");
   return;
   }
 
sscanf(args,"%[^,],%[^)]",one,two);
Debug("TwoArgs [%s] [%s]\n",one,two);
 
if (one[0]=='\0' || two[0] == '\0')
   {
   yyerror("Argument error in cfunction");
   return;
   }

strcpy(arg1,UnQuote(one));
strcpy(arg2,UnQuote(two));
}


/*********************************************************************/

void ThreeArgs(args,arg1,arg2,arg3)

char *args,*arg1,*arg2,*arg3;

{ char one[bufsize],two[bufsize],three[bufsize];
  char *sp;
  int count = 0;
  
bzero(one,bufsize);
bzero(two,bufsize);
bzero(three,bufsize);
 
for (sp = args; *sp != '\0'; sp++)
   {
   if (*sp == ',')
      {
      count++;
      }
   }

if (count != 2)
   {
   yyerror("Three arguments to function required");
   return;
   }

 
sscanf(args,"%[^,],%[^,],%[^)]",one,two,three);
Debug("ThreeArgs [%s] [%s] [%s]\n",one,two,three);
 
if (one[0]=='\0' || two[0] == '\0' || three[0] == '\0')
   {
   yyerror("Argument error in function");
   return;
   }

strcpy(arg1,UnQuote(one));
strcpy(arg2,UnQuote(two));
strcpy(arg3,UnQuote(three));
}
