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
 

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/
/* Varstring: variable names                                         */
/*********************************************************************/

char *VVNAMES[] =
   {
   "faculty",
   "site",
   "host",
   "fqhost",
   "ipaddress",
   "binserver",
   "sysadm",
   "domain",
   "timezone",
   "netmask",
   "nfstype",
   "sensiblesize",
   "sensiblecount",
   "editfilesize",
   "editbinfilesize",
   "actionsequence",
   "mountpattern",
   "homepattern",
   "addclasses",
   "addinstallable",
   "schedule",
   "access",
   "class",
   "arch",
   "ostype",
   "date",
   "year",
   "month",
   "day",
   "hr",
   "min",
   "allclasses",
   "excludecopy",
   "excludelink",
   "copylinks",
   "linkcopies",
   "repository",
   "spc",
   "tab",
   "lf",
   "cr",
   "n",
   "dblquote",
   "quote",
   "dollar",
   "repchar",
   "split",
   "underscoreclasses",
   "interfacename",
   "expireafter",
   "ifelapsed",
   "fileextensions",
   "suspiciousnames",
   "spooldirectories",
   "allowconnectionsfrom", /* nonattackers */
   "denyconnectionsfrom",
   "allowmultipleconnectionsfrom",
   "trustkeysfrom",
   "dynamicaddresses",
   "allowusers",
   "skipverify",
   "defaultcopytype",
   "allowredefinitionof",
   NULL
   };

/*********************************************************************/
/* TOOLKIT : Varstring expansion                                     */
/*********************************************************************/

int TrueVar(var)

char *var;

{ char buff[bufsize];
  char varbuf[maxvarsize]; 
 
if (GetMacroValue(var))
   {
   bzero(buff,bufsize);
   snprintf(varbuf,maxvarsize,"$(%s)",var);
   ExpandVarstring(varbuf,buff,NULL);

   if (strcmp(ToLowerStr(buff),"on") == 0)
      {
      return true;
      }
   
   if (strcmp(ToLowerStr(buff),"true") == 0)
      {
      return true;
      }
   }
 
return false;
}

/*********************************************************************/

int IsVarString(str)

char *str;

{ char *sp;
  char left = 'x', right = 'x';
  int dollar = false;
  int bracks = 0, vars = 0;

for (sp = str; *sp != '\0' ; sp++)       /* check for varitems */
   {
   switch (*sp)
      {
      case '$': dollar = true;
                break;
      case '(':
      case '{': 
                if (dollar)
		    {
		    left = *sp;    
		    bracks++;
		    }
                break;
      case ')':
      case '}': 
                if (dollar)
		    {
		    bracks--;
		    right = *sp;
		    }
                break;
      }

   if (left == '(' && right == ')' && dollar)
      {
      vars++;
      dollar=false;
      }

   if (left == '{' && right == '}' && dollar)
      {
      vars++;
      dollar = false;
      }
   }


if (bracks != 0)
   {
   yyerror("Incomplete variable syntax or bracket mismatch");
   return false;
   }

return vars;
}

/*********************************************************************/

char ExpandVarstring(string,buffer,bserver) 

char *string, *buffer, *bserver;

{ char *sp,*env;
  char varstring = false;
  char currentitem[bufsize],name[maxvarsize],scanstr[6];
  int len;
  time_t tloc;
  
buffer[0] = '\0';

if (string == 0 || strlen(string) == 0)
   {
   return false;
   }

for (sp = string; /* No exit */ ; sp++)       /* check for varitems */
   {
   currentitem[0] = '\0';

   sscanf(sp,"%[^$]",currentitem);

   if (BufferOverflow(buffer,currentitem))
      {
      FatalError("Can't expand varstring");
      }
   
   strcat(buffer,currentitem);
   sp += strlen(currentitem);

   if (*sp == '$')
      {
      switch (*(sp+1))
         {
         case '(': 
                   varstring = ')';
                   break;
         case '{': 
                   varstring = '}';
                   break;
         default: 
                   strcat(buffer,"$");
                   continue;
         }
      sp++;
      }

   currentitem[0] = '\0';

   if (*sp == '\0')
      {
      break;
      }
   else
      {
      snprintf(scanstr,6,"%%[^%c]",varstring);   /* select the correct terminator */
      sscanf(++sp,scanstr,currentitem);               /* reduce item */

      switch (ScanVariable(currentitem))
         {
         case cffaculty:
         case cfsite:
                   if (VFACULTY[0] == '\0')
                      {
                      yyerror("faculty/site undefined variable");
                      }

		   if (BufferOverflow(buffer,VFACULTY))
		      {
		      FatalError("Can't expand varstring");
		      }
                   strcat(buffer,VFACULTY);
                   break;


         case cfhost:
	            if (strlen(VUQNAME) == 0)
		       {
		       if (BufferOverflow(buffer,VDEFAULTBINSERVER.name))
			  {
			  FatalError("Can't expand varstring");
			  }
		       strcat(buffer,VDEFAULTBINSERVER.name);
		       }
		    else
		       {
		       if (BufferOverflow(buffer,VUQNAME))
			  {
			  FatalError("Can't expand varstring");
			  }
		       strcat(buffer,VUQNAME);
		       }
                    break;

	 case cffqhost:
	     	    if (BufferOverflow(buffer,VFQNAME))
		      {
		      FatalError("Can't expand varstring");
		      }
                    strcat(buffer,VFQNAME);
                    break;

	 case cfnetmask:
	     	    if (BufferOverflow(buffer,VNETMASK))
		      {
		      FatalError("Can't expand varstring");
		      }
                    strcat(buffer,VNETMASK);
                    break;
		    

	 case cfipaddr:
	           if (BufferOverflow(buffer,VIPADDRESS))
		      {
		      FatalError("Can't expand varstring");
		      }
		   strcat(buffer,VIPADDRESS);
		   break;
		   
         case cfbinserver:
                    if (ACTION != links && ACTION != required)
                       {
                       yyerror("Inappropriate use of variable binserver");
                       FatalError("Bad variable");
                       }

		    if (BufferOverflow(buffer,"$(binserver)"))
		      {
		      FatalError("Can't expand varstring");
		      }
                    strcat(buffer,"$(binserver)");
                    break;

         case cfsysadm:
                   if (VSYSADM[0] == '\0')
                      {
                      yyerror("sysadm undefined variable");
                      }

		   if (BufferOverflow(buffer,VSYSADM))
		      {
		      FatalError("Can't expand varstring");
		      }
                   strcat(buffer,VSYSADM);
                   break;

         case cfdomain:
                   if (VDOMAIN[0] == '\0')
                      {
                      yyerror("domain undefined variable");
                      }

		   if (BufferOverflow(buffer,VDOMAIN))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,ToLowerStr(VDOMAIN));
                   break;

         case cfnfstype:
	           if (BufferOverflow(buffer,VNFSTYPE))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,VNFSTYPE);
                   break;

         case cftimezone:
                   if (VTIMEZONE == NULL)
                      {
                      yyerror("timezone undefined variable");
                      }

		   if (BufferOverflow(buffer,VTIMEZONE->name))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,VTIMEZONE->name);
                   break;

         case cfclass:
	           if (BufferOverflow(buffer,CLASSTEXT[VSYSTEMHARDCLASS]))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,CLASSTEXT[VSYSTEMHARDCLASS]);
                   break;

         case cfarch:
	           if (BufferOverflow(buffer,VARCH))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,VARCH);
                   break;

         case cfarch2:
	           if (BufferOverflow(buffer,VARCH2))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,VARCH2);
                   break;
		   

	 case cfdate:

	           if ((tloc = time((time_t *)NULL)) == -1)
		      {
		      snprintf(OUTPUT,bufsize*2,"Couldn't read system clock\n");
		      CfLog(cferror,"Couldn't read clock","time");
		      }

	           if (BufferOverflow(buffer,ctime(&tloc)))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   else
		      {
		      strcat(buffer,Space2Score(ctime(&tloc)));
		      Chop(buffer);
		      }

	           break;
		   
	 case cfyear:
	           if (BufferOverflow(buffer,VYEAR))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   else
		      {
		      strcat(buffer,VYEAR);
		      }
	           break;

	 case cfmonth:
	          if (BufferOverflow(buffer,VMONTH))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   else
		      {
		      strcat(buffer,VMONTH);
		      }
	           break;

	 case cfday:
	           if (BufferOverflow(buffer,VDAY))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   else
		      {
		      strcat(buffer,VDAY);
		      }
	           break;
	 case cfhr:
	           if (BufferOverflow(buffer,VHR))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   else
		      {
		      strcat(buffer,VHR);
		      }
	           break;

	 case cfmin:
	           if (BufferOverflow(buffer,VMINUTE))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   else
		      {
		      strcat(buffer,VMINUTE);
		      }
	           break;

	 case cfallclass:
                   if (strlen(ALLCLASSBUFFER) == 0)
		      {
		      snprintf(name,maxvarsize,"$(%s)",currentitem);
		      strcat(buffer,name);
		      }
	     
	           if (BufferOverflow(buffer,ALLCLASSBUFFER))
		      {
		      FatalError("Can't expandvarstring");
		      }
                   strcat(buffer,ALLCLASSBUFFER);
                   break;

	 case cfspc:
	     	   if (BufferOverflow(buffer," "))
		      {
		      FatalError("Can't expandvarstring");
		      }
	           strcat(buffer," ");
		   break;

	 case cftab:
	     	   if (BufferOverflow(buffer," "))
		      {
		      FatalError("Can't expandvarstring");
		      }
	           strcat(buffer,"\t");
		   break;

	 case cflf:
	     	   if (BufferOverflow(buffer," "))
		      {
		      FatalError("Can't expandvarstring");
		      }
	           strcat(buffer,"\012");
		   break;
		   
         case cfcr:
	     	   if (BufferOverflow(buffer," "))
		      { 
		      FatalError("Can't expandvarstring");
		      }
	           strcat(buffer,"\015");
		   break;

         case cfn:
	     	   if (BufferOverflow(buffer," "))
		      { 
		      FatalError("Can't expandvarstring");
		      }
	           strcat(buffer,"\n");
		   break;

	 case cfdblquote:
	     	   if (BufferOverflow(buffer," "))
		      { 
		      FatalError("Can't expandvarstring");
		      }
	           strcat(buffer,"\"");
		   break;
         case cfquote:
		  if (BufferOverflow(buffer," "))
		     { 
		     FatalError("Can't expandvarstring");
		     }
		  strcat(buffer,"\'");
		  break;
		   
         case cfdollar:

		   if (!PARSING)
		      {
		      if (BufferOverflow(buffer," "))
			 { 
			 FatalError("Can't expandvarstring");
			 }
		      strcat(buffer,"$");
		      }
		   else
		      {
		      if (BufferOverflow(buffer,"$(dollar)"))
			 { 
			 FatalError("Can't expandvarstring");
			 }
		      strcat(buffer,"$(dollar)");
		      }
		   break;
		   

	 case cfrepchar:
	     	   if (BufferOverflow(buffer," "))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   len = strlen(buffer);
	           buffer[len] = REPOSCHAR;
		   buffer[len+1] = '\0';
	           break;

	 case cflistsep:
	     	   if (BufferOverflow(buffer,""))
		      {
		      FatalError("Can't expandvarstring");
		      }
		   len = strlen(buffer);
	           buffer[len] = LISTSEPARATOR;
		   buffer[len+1] = '\0';
                   break;

         default:  
                   if ((env = GetMacroValue(currentitem)) != NULL)
                      {
 		      if (BufferOverflow(buffer,env))
	   	         {
		         FatalError("Can't expandvarstring");
		         }
                      strcat(buffer,env);
                      break;
                      }

                   Debug("Currently non existent variable $(%s)\n",currentitem);

		   snprintf(name,maxvarsize,"${%s}",currentitem);
                   strcat(buffer,name);
         }

      sp += strlen(currentitem);
      currentitem[0] = '\0';
      }
   }

return varstring;
}

/*********************************************************************/

char ExpandVarbinserv(string,buffer,bserver) 

char *string, *buffer, *bserver;

{ char *sp;
  char varstring = false;
  char currentitem[bufsize], scanstr[6];

Debug("ExpandVarbinserv %s, ",string);

if (bserver != NULL)
   {
   Debug("(Binserver is %s)\n",bserver);
   }

buffer[0] = '\0';

for (sp = string; /* No exit */ ; sp++)       /* check for varitems */
   {
   currentitem[0] = '\0';

   sscanf(sp,"%[^$]",currentitem);

   strcat(buffer,currentitem);
   sp += strlen(currentitem);

   if (*sp == '$')
      {
      switch (*(sp+1))
         {
         case '(': 
                   varstring = ')';
                   break;
         case '{': 
                   varstring = '}';
                   break;
         default: 
                   strcat(buffer,"$");
                   continue;
         }
      sp++;
      }

   currentitem[0] = '\0';

   if (*sp == '\0')
      {
      break;
      }
   else
      {
      sprintf(scanstr,"%%[^%c]",varstring);   /* select the correct terminator */
      sscanf(++sp,scanstr,currentitem);               /* reduce item */

      switch (ScanVariable(currentitem))
         {
         case cfbinserver:
		    if (BufferOverflow(buffer,bserver))
		      {
		      FatalError("Can't expand varstring");
		      }
                    strcat(buffer,bserver);
                    break;
         }

      sp += strlen(currentitem);
      currentitem[0] = '\0';
      }
   }

return varstring;
}

/*********************************************************************/

enum vnames ScanVariable(name)

char *name;

{ int i = nonexistentvar;

for (i = 0; VVNAMES[i] != '\0'; i++)
   {
   if (strcmp(VVNAMES[i],ToLowerStr(name)) == 0)
      {
      return (enum vnames) i;
      }
   }

return (enum vnames) i;
}


/*********************************************************************/

struct Item *SplitVarstring(varstring,sep)

 /* Splits a string containing a separator like : 
    into a linked list of separate items, */

char *varstring;
char sep;

{ struct Item *liststart = NULL;
  char format[6], *sp;
  char node[bufsize];
  char buffer[bufsize], variable[maxvarsize];
  char before[bufsize],after[bufsize],result[bufsize];
  int i;
  
Debug("SplitVarstring(%s,%c=%d)\n",varstring,sep,sep);

bzero(before,bufsize);
bzero(after,bufsize);

if (strcmp(varstring,"") == 0)   /* Handle path = / as special case */
   {
   AppendItem(&liststart,"/",NULL);
   return liststart;
   }

if (!IsVarString(varstring))
   {
   AppendItem(&liststart,varstring,NULL);
   return liststart;   
   }

sprintf(format,"%%[^%c]",sep);   /* set format string to search */

i = 0; /* extract variable */

for(sp = varstring; *sp != '$' && *sp != '\0' ; sp++)
   {
   before[i++] = *sp;
   }
before[i] = '\0';
i = 0;

while (*sp != ')' && *sp != '}' && *sp != '\0')
   {
   variable[i++] = *sp++;
   }
variable[i] = *sp++;
variable[i+1] = '\0';
i = 0;

if (strcmp(variable,"$(date)") == 0)        /* Exception! $(date) contains : but is not a list*/
   {
   ExpandVarstring(varstring,buffer,"");
   AppendItem(&liststart,buffer,NULL);
   return liststart;
   }
 
ExpandVarstring(variable,buffer,"");

while(*sp != '\0')
   {
   after[i++] = *sp++;
   }

for (sp = buffer; *sp != '\0'; sp++)
   {
   bzero(node,maxlinksize);
   sscanf(sp,format,node);

   if (strlen(node) == 0)
      {
      continue;
      }
   
   sp += strlen(node)-1;

   if (strlen(before)+strlen(node)+strlen(after) >= bufsize)
      {
      FatalError("Buffer overflow expanding variable string");
      printf("Concerns: %s%s%s in %s",before,node,after,varstring);
      }
   
   snprintf(result,bufsize,"%s%s%s",before,node,after);

   AppendItem(&liststart,result,NULL);

   if (*sp == '\0')
      {
      break;
      }
   }

return liststart;
}

