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

void GetRandom ARGLIST((char* args,char *value));
void HandleFunctionExec ARGLIST((char* args,char *value));
void HandleStatInfo ARGLIST((enum builtin fn,char* args,char *value));
void HandleCompareStat ARGLIST((enum builtin fn,char* args,char *value));
void HandleReturnsZero ARGLIST((char* args,char *value));
void HandleIPRange ARGLIST((char* args,char *value));
void HandleIsDefined ARGLIST((char* args,char *value));
void HandleStrCmp ARGLIST((char* args,char *value));
void HandleShowState ARGLIST((char* args,char *value));

/*********************************************************************/

int IsBuiltinFunction(item)

char *item;

{ char name[maxvarsize],args[bufsize];
  char c1 = '?',c2 = '?' ;

sscanf(item,"%255[a-zA-Z0-9_]%c%255[^)]%c",name,&c1,args,&c2);

if (c1 != '(' || c2 != ')')
   {
   return false;
   }
 
Debug("IsBuiltinFunction: %s(%s)\n",name,args);
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
   }
 
return value;
}

/*********************************************************************/
/* level 1                                                           */
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

for (i = 1; BUILTINS[i] != '\0'; i++)
   {
   if (strcmp(BUILTINS[i],str) == 0)
      {
      fn = (enum builtin) i;
      break;
      }
   }

if (fn == nofn)
  {
  snprintf(OUTPUT,bufsize,"Internal function %s not recognized",str);
  yyerror(OUTPUT);
  FatalError("Could not parse function");
  }

return (enum builtin) i;
}

/*********************************************************************/

void GetRandom(args,value)

char *args,*value;

{ int result,count=0,from=-1,to=-1;
  char *sp;

if (ACTION != control)
   {
   yyerror("Use of RandomInt(a,b) outside of variable assignment");
   }
  
for (sp = args; *sp != '\0'; sp++)
   {
   if (*sp == ',')
      {
      count++;
      }
   }

if (count != 1)
   {
   yyerror("RandomInt(a,b): argument error");
   return;
   }
 
sscanf(args,"%d,%d",&from,&to);

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

from[0] = '\0';
to[0] = '\0';

for (sp = args; *sp != '\0'; sp++)
   {
   if (*sp == ',')
      {
      count++;
      }
   }

if (count != 1)
   {
   yyerror("RandomInt(a,b): argument error");
   return;
   }
 
sscanf(args,"%[^,],%[^)]",from,to);
Debug("Comparing [%s] < [%s]\n",from,to);
 
if (from[0]=='\0' || to[0] == '\0')
   {
   yyerror("Argument error in class-function");
   return;
   }

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
       if (frombuf.st_mtime > tobuf.st_mtime)
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

void HandleStrCmp(args,value)

char *args,*value;

{ char *sp,from[bufsize],to[bufsize],*nfrom,*nto;
  int count = 0;

from[0] = '\0';
to[0] = '\0';

for (sp = args; *sp != '\0'; sp++)
   {
   if (*sp == ',')
      {
      count++;
      }
   }

if (count != 1)
   {
   yyerror("StrCmp(a,b): argument error");
   return;
   }
 
sscanf(args,"%[^,],%[^)]",from,to);
Debug("Comparing [%s] < [%s]\n",from,to);
 
if (from[0]=='\0' || to[0] == '\0')
   {
   yyerror("Argument error in class-function");
   return;
   }

nfrom = UnQuote(from);
nto = UnQuote(to);
 
if (strcmp(nfrom,nto) == 0)
   {
   strcpy(value,CF_ANYCLASS); 
   }
 else
    {
    strcpy(value,CF_NOCLASS);
    } 
}



/*********************************************************************/

void HandleShowState(args,value)

char *args,*value;

{ struct stat statbuf;
  char buffer[bufsize]; 
  FILE *fp;
  int i = 0;
  
if (PARSING)
   {
   return;
   }
  
if (ACTION != alerts)
   {
   yyerror("Use of ShowState(type) outside of alert declaration");
   }

Debug("ShowState(%s)\n",args); 

snprintf(buffer,bufsize,"%s/state/cf_%s",WORKDIR,args);

if (stat(buffer,&statbuf) == 0)
   {
   if ((fp = fopen(buffer,"r")) == NULL)
      {
      Verbose("Could not open state %s\n",buffer);
      return;
      }

   while(!feof(fp))
      {
      buffer[0] = '\0';
      
      fgets(buffer,bufsize,fp);
      i++;

      if (strlen(buffer) > 0)
	 {
	 printf("%s: (%2d) %s",VPREFIX,i,buffer);
	 }
      }
   
   fclose(fp);
   snprintf(buffer,bufsize,"State of %s recorded at %s\n",args,ctime(&statbuf.st_mtime));
   strcpy(value,buffer);
   }
else 
   {
   snprintf(buffer,bufsize,"State parameter %s is not known or recorded\n",args);
   strcpy(value,buffer);
   }
}

