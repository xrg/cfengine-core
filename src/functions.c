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
/* There is a protocol to these function calls - must return a val/class     */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include <math.h>

/*********************************************************************/

int IsBuiltinFunction(char *item)

{ char name[CF_MAXVARSIZE],args[CF_BUFSIZE];
  char c1 = '?',c2 = '?' ;

Debug("IsBuiltinFunction(%s)\n",item);
  
name[0] = '\0';
args[0] = '\0';
    
if (*item == '!')
   {
   item++;
   }
 
sscanf(item,"%255[!a-zA-Z0-9_]%c%255[^)]%c",name,&c1,args,&c2);

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

char *EvaluateFunction(char *f,char *value)

{ enum builtin fn;
  char name[CF_MAXVARSIZE],vargs[CF_BUFSIZE],args[CF_EXPANDSIZE];
  int negated = false;

if (*f == '!')
   {
   negated = true;
   f++;
   }
 
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
   case fn_hostrange:
       HandleHostRange(args,value);
       break;
   case fn_isdefined:
       HandleIsDefined(args,value);
       break;
   case fn_strcmp:
       HandleStrCmp(args,value);
       break;
   case fn_regcmp:
       HandleRegCmp(args,value);
       break;
   case fn_showstate:
       HandleShowState(args,value);
       break;
   case fn_friendstat:
       HandleFriendStatus(args,value);
       break;
   case fn_readfile:
       HandleReadFile(args,value);
       break;
   case fn_readarray:
       HandleReadArray(args,value);
       break;
   case fn_readtable:
       HandleReadTable(args,value);
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

   case fn_module:
       HandlePrepModule(args,value);
       break;

   case fn_adj:
       HandleAssociation(args,value);
       break;
       
   case fn_returnvars:
       
       if (ScopeIsMethod())
          {
          HandleReturnValues(args,value);
          ACTIONPENDING = false;
          }
       else
          {
          snprintf(OUTPUT,CF_BUFSIZE,"Function %s can only be used within a private method context",f);
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
          snprintf(OUTPUT,CF_BUFSIZE,"Function %s can only be used within a private method context",f);
          yyerror(OUTPUT);
          }
       break;
       
   default: snprintf(OUTPUT,CF_BUFSIZE,"No such builtin function %s\n",f);
       CfLog(cferror,OUTPUT,"");
   }
 
 if (negated)
    {
    if (strcmp(value,CF_NOCLASS) == 0)
       {
       strcpy(value,CF_ANYCLASS);
       return value;
       }
    
    if (strcmp(value,CF_ANYCLASS) == 0)
       {
       strcpy(value,CF_NOCLASS);
       return value;
       }
    }
 
return value;
}

/*********************************************************************/
/* level 2                                                           */
/*********************************************************************/

enum builtin FunctionStringToCode(char *str)

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
  snprintf(OUTPUT,CF_BUFSIZE,"Internal function (%s) not recognized",str);
  yyerror(OUTPUT);
  FatalError("Could not parse function");
  }

return (enum builtin) i;
}

/*********************************************************************/

void GetRandom(char *args,char *value)

{ int result,count=0,from=-1,to=-1;
  char argv[2][CF_BUFSIZE];
 
if (ACTION != control)
   {
   yyerror("Use of RandomInt(a,b) outside of variable assignment");
   }
  
FunctionArgs(args,argv,2); 
from = atoi(argv[0]);
to = atoi(argv[1]);
 
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
snprintf(value,CF_BUFSIZE,"%u",result); 
}

/*********************************************************************/

void HandleStatInfo(enum builtin fn,char *args,char *value)

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

void HandleIPRange(char *args,char *value)

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

void HandleHostRange(char *args,char *value)

{ struct Item *list = NULL;

 Debug("SRDEBUG in HandleHostRange()\n"); 
 Debug("SRDEBUG args=%s value=%s\n",args,value);

 if (!FuzzyHostParse(args))
    {
    strcpy(value,CF_NOCLASS);
    return;
    }
 /* VDEFAULTBINSERVER.name is relative domain name */
  /* (see nameinfo.c ~line 145)                     */

 if (FuzzyHostMatch(args,VDEFAULTBINSERVER.name) == 0)
    {
    Debug("SRDEBUG SUCCESS!\n");
    strcpy(value,CF_ANYCLASS);
    }
 else
    {
    Debug("SRDEBUG FAILURE\n");
    strcpy(value,CF_NOCLASS);
    }
 
 return;
}

/*********************************************************************/

void HandleCompareStat(enum builtin fn,char *args,char *value)

{ struct stat frombuf,tobuf;
  char *sp,argv[2][CF_BUFSIZE];
  int count = 0;

FunctionArgs(args,argv,2); 
strcpy(value,CF_NOCLASS);
 
if (stat(argv[0],&frombuf) == -1)
   {
   return;
   }

if (stat(argv[1],&tobuf) == -1)
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

void HandleFunctionExec(char *args,char *value)

{ char command[CF_MAXVARSIZE];

 if (ACTION != control)
   {
   yyerror("Use of ExecResult(s) outside of variable assignment");
   }
 
if (*args == '/')
   {
   strncpy(command,args,CF_MAXVARSIZE);
   GetExecOutput(command,value);
   Chop(value);
   value[CF_MAXVARSIZE-1] = '\0';  /* Truncate to CF_MAXVARSIZE */
   }
 else
    {
    yyerror("ExecResult(/command) must specify an absolute path");
    } 
}

/*********************************************************************/

void HandleReturnsZero(char *args,char *value)

{ char command[CF_BUFSIZE];

if (ACTION != groups)
   {
   yyerror("Use of ReturnsZero(s) outside of class assignment");
   }

Debug("HandleReturnsZero(%s)\n",args); 
 
if (*args == '/')
   {
   strncpy(command,args,CF_BUFSIZE);
   
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

void HandleIsDefined(char *args,char *value)

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

void HandleSyslogFn(char *args,char *value)

{ char argv[2][CF_BUFSIZE];
  int priority = LOG_ERR;

FunctionArgs(args,argv,2);

value[0] = '\0';

if (PARSING)
   {
   strcpy(value,"doinstall");
   return;
   }
 
if (strcmp(argv[0],"LOG_EMERG") == 0)
   {
   priority = LOG_EMERG;
   }
else if (strcmp(argv[0],"LOG_ALERT") == 0)
   {
   priority = LOG_ALERT;
   }
else if (strcmp(argv[0],"LOG_CRIT") == 0)
   {
   priority = LOG_CRIT;
   }
else if (strcmp(argv[0],"LOG_NOTICE") == 0)
   {
   priority = LOG_NOTICE;
   } 
else if (strcmp(argv[0],"LOG_WARNING") == 0)
   {
   priority = LOG_WARNING;
   }
else if (strcmp(argv[0],"LOG_ERR") == 0)
   {
   priority = LOG_ERR;
   }
else
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unknown syslog priority (%s) - changing to LOG_ERR",argv[0]);
   CfLog(cferror,OUTPUT,"");
   priority = LOG_ERR;
   }

Debug("Alerting to syslog(%s,%s)\n",argv[0],argv[1]);

if (!DONTDO)
   {
   syslog(priority," %s",argv[1]);
   } 
}


/*********************************************************************/

void HandleStrCmp(char *args,char *value)

{ char *sp,argv[2][CF_BUFSIZE];
  int count = 0;

FunctionArgs(args,argv,2); 
 
if (strcmp(argv[0],argv[1]) == 0)
   {
   strcpy(value,CF_ANYCLASS); 
   }
else
   {
   strcpy(value,CF_NOCLASS);
   } 
}

/*********************************************************************/

void HandleRegCmp(char *args,char *value)

{ char *sp,argv[2][CF_BUFSIZE];
  struct Item *list = NULL, *ret; 
  int count = 0;

FunctionArgs(args,argv,2);

if (ACTION != groups)
   {
   yyerror("Function RegCmp() used outside of classes/groups");
   return;
   }

list = SplitStringAsItemList(argv[1],LISTSEPARATOR);

ret = LocateNextItemMatching(list,argv[0]);
     
if (ret != NULL)
   {
   strcpy(value,CF_ANYCLASS); 
   }
else
   {
   strcpy(value,CF_NOCLASS);
   }
 
DeleteItemList(list); 
}


/*********************************************************************/

void HandleReadFile(char *args,char *value)

{ char *sp,argv[2][CF_BUFSIZE];
  int i,val = 0;
  FILE *fp;

FunctionArgs(args,argv,2);
 
val = atoi(argv[1]);

if ((fp = fopen(argv[0],"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open ReadFile(%s)\n",argv[0]);
   CfLog(cferror,OUTPUT,"fopen");
   return;
   }

if (val > CF_BUFSIZE - CF_BUFFERMARGIN)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"ReadFile() will not read more than %d bytes",CF_BUFSIZE - CF_BUFFERMARGIN);
   CfLog(cferror,OUTPUT,"");
   fclose(fp);
   return;
   }

memset(value,0,CF_BUFSIZE); 
fread(value,val,1,fp);

for (i = val+1; i >= 0; i--)
   {   
   if (value[i] == EOF)
      {
      value[i] = '\0';
      break;
      }
    }
 
fclose(fp); 
}


/*********************************************************************/

void HandleReadArray(char *args,char *value)

{ char argv[5][CF_BUFSIZE];
  char *sp,*filename=argv[0],*maxbytes=argv[4],*formattype=argv[1];
  char *commentchar=argv[3],*sepchar=argv[2],buffer[CF_BUFSIZE];
  int i=0,val = 0,read = 0;
  struct Item *list = NULL,*ip;
  FILE *fp;

FunctionArgs(args,argv,5);
 
val = atoi(maxbytes);

if ((fp = fopen(filename,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open ReadFile(%s)\n",filename);
   CfLog(cferror,OUTPUT,"fopen");
   return;
   }
 
if (strlen(sepchar) > 1)
   {
   yyerror("List separator declared is not a single character in ReadArray");
   }

while (!feof(fp))
   {
   memset(buffer,0,CF_BUFSIZE);
   fgets(buffer,CF_BUFSIZE,fp);

   if ((read > val) && (val > 0))
      {
      Verbose("Breaking off file read after %d bytes\n",read);
      break;
      }
   
   read += strlen(buffer);

   if (strlen(commentchar) > 0)
      {
      for (sp = buffer; sp < buffer+CF_BUFSIZE; sp++) /* Remove comments */
         {
         if (strncmp(sp,commentchar,strlen(commentchar)) == 0)
            {
            *sp = '\0';
            break;
            }
         }
      }
   
   Chop(buffer);
   
   if (strlen(buffer) == 0)
      {
      continue;
      }
   
   if (strcmp(formattype,"autokey") == 0)
      {
      Debug("AUTOKEY: %s(%s)\n",CURRENTITEM,buffer);
      list = SplitStringAsItemList(buffer,*sepchar);
      
      for (ip = list; ip != NULL; ip=ip->next)
         {
         char lvalue[CF_BUFSIZE];
         i++;
         Debug("Setting %s[%d] = %s\n",CURRENTITEM,i,ip->name);
         
         snprintf(lvalue,CF_BUFSIZE-1,"%s[%d]",CURRENTITEM,i);
         InstallControlRValue(lvalue,ip->name);
         snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
         }
      }
   else if (strcmp(formattype,"textkey") == 0)
      {
      char argv[2][CF_BUFSIZE],lvalue[CF_BUFSIZE];
      if (!FunctionArgs(buffer,argv,2))
         {
         break;
         }
      
      Debug("Setting %s[%s] = %s\n",CURRENTITEM,argv[0],argv[1]);
      
      snprintf(lvalue,CF_BUFSIZE-1,"%s[%s]",CURRENTITEM,argv[0]);
      InstallControlRValue(lvalue,argv[1]);
      snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
      }
   else
      {
      yyerror("No such file format specifier");
      }
   }
 
 fclose(fp); 
 snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
}

/*********************************************************************/

void HandleReadTable(char *args,char *value)

{  char argv[5][CF_BUFSIZE];
  char *sp,*filename=argv[0],*maxbytes=argv[4],*formattype=argv[1];
  char *commentchar=argv[3],*sepchar=argv[2],buffer[CF_BUFSIZE];
  int i=0,j=0,val = 0,read = 0;
  struct Item *list = NULL,*ip;
  FILE *fp;

FunctionArgs(args,argv,5);
 
val = atoi(maxbytes);

if ((fp = fopen(filename,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open ReadFile(%s)\n",filename);
   CfLog(cferror,OUTPUT,"fopen");
   return;
   }
 
if (strlen(sepchar) > 1)
   {
   yyerror("List separator declared is not a single character in ReadArray");
   }

while (!feof(fp))
   {
   memset(buffer,0,CF_BUFSIZE);
   fgets(buffer,CF_BUFSIZE,fp);

   i++;
   j=0;
   
   if ((read > val) && (val > 0))
      {
      Verbose("Breaking off file read after %d bytes\n",read);
      break;
      }
   
   read += strlen(buffer);

   if (strlen(commentchar) > 0)
      {
      for (sp = buffer; sp < buffer+CF_BUFSIZE; sp++) /* Remove comments */
         {
         if (strncmp(sp,commentchar,strlen(commentchar)) == 0)
            {
            *sp = '\0';
            break;
            }
         }
      }
   
   Chop(buffer);
   
   if (strlen(buffer) == 0)
      {
      continue;
      }
   
   if (strcmp(formattype,"autokey") == 0)
      {
      Debug("AUTOKEY: %s(%s)\n",CURRENTITEM,buffer);
      list = SplitStringAsItemList(buffer,*sepchar);
      
      for (ip = list; ip != NULL; ip=ip->next)
         {
         char lvalue[CF_BUFSIZE];
         j++;
         Debug("Setting %s[%d][%d] = %s\n",CURRENTITEM,i,j,ip->name);
         
         snprintf(lvalue,CF_BUFSIZE-1,"%s[%d][%d]",CURRENTITEM,i,j);
         InstallControlRValue(lvalue,ip->name);
         snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
         }
      }
   else if (strcmp(formattype,"textkey") == 0)
      {
      char argv[2][CF_BUFSIZE],lvalue[CF_BUFSIZE];
      if (!FunctionArgs(buffer,argv,3))
         {
         break;
         }
      
      Debug("Setting %s[%s] = %s\n",CURRENTITEM,argv[0],argv[1]);
      
      snprintf(lvalue,CF_BUFSIZE-1,"%s[%s][%s]",CURRENTITEM,argv[0],argv[1]);
      InstallControlRValue(lvalue,argv[2]);
      snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
      }
   else
      {
      yyerror("No such file format specifier");
      }
   }
 
fclose(fp); 
snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
}


/*********************************************************************/

void HandleReturnValues(char *args,char *value)

{
Verbose("This is a method with return value list: (%s)\n",args);

 if (strlen(METHODRETURNVARS) == 0)
    {
    strncpy(METHODRETURNVARS,args,CF_BUFSIZE-1);
    }
 else
    {
    yyerror("Redefinition of method return values");
    }

strcpy(value,"noinstall");
}

/*********************************************************************/

void HandleReturnClasses(char *args,char *value)

{
Verbose("This is a method with return class list: %s\n",args);

 if (strlen(METHODRETURNCLASSES) == 0)
    {
    strncpy(METHODRETURNCLASSES,args,CF_BUFSIZE-1);
    }
 else
    {
    yyerror("Redefinition of method return classes");
    }

 strcpy(value,"noinstall");
}

/*********************************************************************/

void HandleShowState(char *args,char *value)

{ struct stat statbuf;
  char buffer[CF_BUFSIZE],vbuff[CF_BUFSIZE];
  struct Item *addresses = NULL,*ip;
  FILE *fp;
  int i = 0, tot=0, min_signal_diversity = 1,conns=1,classes=0;
  int maxlen = 0,count;
  double *dist = NULL, S = 0.0;
  char *offset = NULL;
  
  
if ((ACTION != alerts) && PARSING)
   {
   yyerror("Use of ShowState(type) outside of alert declaration");
   }

Debug("ShowState(%s)\n",args); 

if (PARSING)
   {
   strcpy(value,"doinstall");
   return;
   }
 
snprintf(buffer,CF_BUFSIZE-1,"%s/state/cf_%s",WORKDIR,args);

if (stat(buffer,&statbuf) == 0)
   {
   if ((fp = fopen(buffer,"r")) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Could not open state memory %s\n",buffer);
      CfLog(cfinform, OUTPUT,"fopen");
      return;
      }

   printf("%s: -----------------------------------------------------------------------------------\n",VPREFIX);
   printf("%s: In the last 40 minutes, the peak state was:\n",VPREFIX);
   while(!feof(fp))
      {
      char *sp,local[CF_BUFSIZE],remote[CF_BUFSIZE];
      buffer[0] = local[0] = remote[0] = '\0';

      memset(vbuff,0,CF_BUFSIZE);
      fgets(buffer,CF_BUFSIZE,fp);

      if (strlen(buffer) > 0)
         {
         Verbose("%s: (%2d) %s",VPREFIX,conns,buffer);
         
         if (IsSocketType(args))
            {
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
               
               strncpy(vbuff,remote,CF_BUFSIZE-1);
               DePort(vbuff);
               }
            }
         else if (IsTCPType(args))
            {
            count = 1;
            sscanf(buffer,"%d %[^\n]",&count,remote);
            AppendItem(&addresses,remote,"");
            SetItemListCounter(addresses,remote,count);
            conns += count;
            continue;
            }
         else      
            {
            if (offset == NULL)
               {
               if (offset = strstr(buffer,"CMD"))
                  {
                  }
               else if (offset = strstr(buffer,"COMMAND"))
                  {
                  }
               
               if (offset == NULL)
                  {
                  continue;
                  }
               }
            
            strncpy(vbuff,offset,CF_BUFSIZE-1);
            Chop(vbuff);
            }
         
         if (!IsItemIn(addresses,vbuff))
            {
            conns++;
            AppendItem(&addresses,vbuff,"");
            IncrementItemListCounter(addresses,vbuff);
            }
         else
            {
            conns++;    
            IncrementItemListCounter(addresses,vbuff);
            }
         }
      }
   
   fclose(fp);
   conns--;
   
   if (IsSocketType(args)||IsTCPType(args))
      {
      if (addresses != NULL)
         {
         printf(" {\n");
         }
      
      for (ip = addresses; ip != NULL; ip=ip->next)
         {
         tot+=ip->counter;
         
         buffer[0] = '\0';
         sscanf(ip->name,"%s",buffer);
         
         if (!IsIPV4Address(buffer) && !IsIPV6Address(buffer))
            {
            Verbose("\nRejecting address %s\n",ip->name);
            continue;
            }

         printf(" DNS key: %s = %s (%d/%d)\n",buffer,IPString2Hostname(buffer),ip->counter,conns);
         
         if (strlen(ip->name) > maxlen)
            {
            maxlen = strlen(ip->name);
            }
         }
      
      if (addresses != NULL)
         {
         printf(" -\n");
         }
      }
   else
      {
      for (ip = addresses; ip != NULL; ip=ip->next)
         {
         tot+=ip->counter;
         }
      }
   
   for (ip = addresses; ip != NULL; ip=ip->next)
      {
      int s;
      
      if (maxlen > 17) /* ipv6 */
         {
         printf(" Frequency: %-40s|",ip->name);
         }
      else
         {
         printf(" Frequency: %-17s|",ip->name);
         }
      
      for (s = 0; (s < ip->counter) && (s < 50); s++)
         {
         if (s < 48)
            {
            putchar('*');
            }
         else
            {
            putchar('+');
            }
         }
      printf(" \t(%d/%d)\n",ip->counter,conns);
      }
   
   if (addresses != NULL)
      {
      printf(" }\n");
      }
   
   dist = (double *) malloc((tot+1)*sizeof(double));
   
   if (conns > min_signal_diversity)
      {
      for (i = 0,ip = addresses; ip != NULL; i++,ip=ip->next)
         {
         dist[i] = ((double)(ip->counter))/((double)tot);
         
         S -= dist[i]*log(dist[i]);
         }
      
      printf(" -\n Scaled entropy of addresses = %.1f %%\n",S/log((double)tot)*100.0);
      printf(" (Entropy = 0 for single source, 100 for flatly distributed source)\n -\n");
      }
   
   printf("%s: -----------------------------------------------------------------------------------\n",VPREFIX);
   snprintf(buffer,CF_BUFSIZE,"State of %s peaked at %s\n",args,ctime(&statbuf.st_mtime));
   strcpy(value,buffer);
   }
else 
   {
   snprintf(buffer,CF_BUFSIZE,"State parameter %s is not known or recorded\n",args);   
   strcpy(value,buffer);
   }

if (dist)
   {
   free((char *)dist);
   }
}

/*********************************************************************/

void HandleFriendStatus(char *args,char *value)

{ char argv[1][CF_BUFSIZE];
  int time = -1;
  
if ((ACTION != alerts) && PARSING)
   {
   yyerror("Use of FriendStatus(type) outside of alert declaration");
   }
 
FunctionArgs(args,argv,1);

if (PARSING)
   {
   strcpy(value,"doinstall");
   return;
   }

time = atoi(argv[0]);

if (time >= 0)
   {
   CheckFriendConnections(time);
   }
 
strcpy(value,""); /* No reply */
}

/*********************************************************************/

void HandleSetState(char *args,char *value)

{ char argv[3][CF_BUFSIZE];
  char *name=argv[0],*ttlbuf=argv[1],*policy=argv[2];
  unsigned int ttl = 0;

value[0] = '\0';
 
FunctionArgs(args,argv,3);
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

void HandleUnsetState(char *args,char *value)

{ char arg1[CF_BUFSIZE];
 
value[0] = '\0';
OneArg(args,arg1);

Debug("HandleUnsetState(%s)\n",arg1); 
DeletePersistentClass(arg1);
}

/*********************************************************************/

void HandlePrepModule(char *args,char *value)

{ char argv[2][CF_BUFSIZE];
 
value[0] = '\0';
FunctionArgs(args,argv,2);

Debug("PrepModule(%s,%s)\n",argv[0],argv[1]);
 
if (CheckForModule(argv[0],argv[1]))
   {
   strcpy(value,CF_ANYCLASS);
   }
else
   {
   strcpy(value,CF_NOCLASS);
   }
}

/*********************************************************************/

void HandleAssociation(char *args,char *value)

{ char argv[2][CF_BUFSIZE],lvalue[CF_BUFSIZE];
 
value[0] = '\0';
FunctionArgs(args,argv,2);

Debug("HandleAssociation(%s <-> %s)\n",argv[0],argv[1]);

snprintf(lvalue,CF_BUFSIZE-1,"%s[%s]",CURRENTITEM,argv[0]);
InstallControlRValue(lvalue,argv[1]);
snprintf(value,CF_BUFSIZE-1,"CF_ASSOCIATIVE_ARRAY%s",args);
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

void OneArg(args,arg1)

char *args,*arg1;

{ char one[CF_BUFSIZE];

memset(one,0,CF_BUFSIZE);
 
if (strchr(args,','))
   {
   yyerror("Illegal argument to unary function");
   return;
   }

strcpy(arg1,UnQuote(args));
}

/*********************************************************************/

int FunctionArgs(char *args,char arg[][CF_BUFSIZE],int number)

{ char *argv[10];
  char *sp,**start;
  int count = 0, i;

for (i = 0; i < number; i++)
   {
   arg[i][0] = '\0';
   argv[i] = (char *)malloc(CF_BUFSIZE);
   memset(argv[i],0,CF_BUFSIZE);
   }
 
start = malloc(sizeof(char *)*(number+1));

start[0] = args; 

for (sp = args; *sp != '\0'; sp++)
   {
   if (*sp == '\"')
      {
      while (*++sp != '\"')
         {
         }
      continue;
      }
   
   if (*sp == '\'')
      {
      while (*++sp != '\'')
         {
         }
      continue;
      }
   
   if (*sp == ',')
      {
      if (++count > number-1)
         {
         break;
         }
      
      start[count] = sp+1;
      }
   }
 
 if (count != number-1)
    {
    if (PARSING)
       {
       snprintf(OUTPUT,CF_BUFSIZE,"Function or format of input file requires %d argument items",number);
       yyerror(OUTPUT);
       }
    else
       {
       snprintf(OUTPUT,CF_BUFSIZE,"Assignment (%s) with format error",args);
       CfLog(cferror,OUTPUT,"");
       }
    return false;
    }
 
 for (i = 0; i < number-1; i++)
    {
    strncpy(argv[i],start[i],start[i+1]-start[i]-1);
    }
 
 sscanf(start[number-1],"%[^)]",argv[number-1]); 
 
 for (i = 0; i < number; i++)
    {
    strncpy(arg[i],UnQuote(argv[i]),100);
    free(argv[i]);
    }
 
free(start);
return true;
}

