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
 


/*******************************************************************/
/*                                                                 */
/*  Class string evaluation toolkit for cfengine                   */
/*                                                                 */
/*  Dependency: item.c toolkit                                     */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*********************************************************************/
/* Object variables                                                  */
/*********************************************************************/

char *DAYTEXT[] =
   {
   "Monday",
   "Tuesday",
   "Wednesday",
   "Thursday",
   "Friday",
   "Saturday",
   "Sunday"
   };

char *MONTHTEXT[] =
   {
   "January",
   "February",
   "March",
   "April",
   "May",
   "June",
   "July",
   "August",
   "September",
   "October",
   "November",
   "December"
   };


/*********************************************************************/
/* Level 1                                                           */
/*********************************************************************/

int Day2Number(datestring)

char *datestring;

{ int i = 0;

for (i = 0; i < 7; i++)
   {
   if (strncmp(datestring,DAYTEXT[i],3) == 0)
      {
      return i;
      }
   }

return -1;
}

/*********************************************************************/

void AddInstallable(classlist)

char *classlist;

{ char *sp, currentitem[maxvarsize];

if (classlist == NULL)
   {
   return;
   }

Debug("AddInstallable(%s)\n",classlist);
  
 for (sp = classlist; *sp != '\0'; sp++)
   {
   currentitem[0] = '\0';

   sscanf(sp,"%[^,:.]",currentitem);

   sp += strlen(currentitem);

   if (! IsItemIn(VALLADDCLASSES,currentitem))
      {
      AppendItem(&VALLADDCLASSES,currentitem,NULL);
      }

   if (*sp == '\0')
      {
      break;
      }
   }
}

/*********************************************************************/

void AddMultipleClasses(classlist)

char *classlist;

{ char *sp, currentitem[maxvarsize],local[maxvarsize];
 
if ((classlist == NULL) || strlen(classlist) == 0)
   {
   return;
   }

bzero(local,maxvarsize);
strcpy(local,classlist);

Debug("AddMultipleClasses(%s)\n",local);

for (sp = local; *sp != '\0'; sp++)
   {
   bzero(currentitem,maxvarsize);

   sscanf(sp,"%250[^.:,]",currentitem);

   sp += strlen(currentitem);

   AddClassToHeap(CanonifyName(currentitem));
   }
}

/*********************************************************************/

void AddTimeClass(str)

char *str;

{ int i;
  char buf2[10], buf3[10], buf4[10], buf5[10], buf[10], out[10];
  
for (i = 0; i < 7; i++)
   {
   if (strncmp(DAYTEXT[i],str,3)==0)
      {
      AddClassToHeap(DAYTEXT[i]);
      break;
      }
   }

sscanf(str,"%*s %s %s %s %s",buf2,buf3,buf4,buf5);

/* Hours */

sscanf(buf4,"%[^:]",buf);
sprintf(out,"Hr%s",buf);
AddClassToHeap(out);
bzero(VHR,3);
strncpy(VHR,buf,2); 

/* Minutes */

sscanf(buf4,"%*[^:]:%[^:]",buf);
sprintf(out,"Min%s",buf);
AddClassToHeap(out);
bzero(VMINUTE,3);
strncpy(VMINUTE,buf,2); 
 
sscanf(buf,"%d",&i);

switch ((i / 5))
   {
   case 0: AddClassToHeap("Min00_05");
           break;
   case 1: AddClassToHeap("Min05_10");
           break;
   case 2: AddClassToHeap("Min10_15");
           break;
   case 3: AddClassToHeap("Min15_20");
           break;
   case 4: AddClassToHeap("Min20_25");
           break;
   case 5: AddClassToHeap("Min25_30");
           break;
   case 6: AddClassToHeap("Min30_35");
           break;
   case 7: AddClassToHeap("Min35_40");
           break;
   case 8: AddClassToHeap("Min40_45");
           break;
   case 9: AddClassToHeap("Min45_50");
           break;
   case 10: AddClassToHeap("Min50_55");
            break;
   case 11: AddClassToHeap("Min55_00");
            break;
   }

/* Add quarters */ 

switch ((i / 15))
   {
   case 0: AddClassToHeap("Q1");
           sprintf(out,"Hr%s_Q1",VHR);
	   AddClassToHeap(out);
           break;
   case 1: AddClassToHeap("Q2");
           sprintf(out,"Hr%s_Q2",VHR);
	   AddClassToHeap(out);
           break;
   case 2: AddClassToHeap("Q3");
           sprintf(out,"Hr%s_Q3",VHR);
	   AddClassToHeap(out);
           break;
   case 3: AddClassToHeap("Q4");
           sprintf(out,"Hr%s_Q4",VHR);
	   AddClassToHeap(out);
           break;
   }
 

/* Day */

sprintf(out,"Day%s",buf3);
AddClassToHeap(out);
bzero(VDAY,3);
strncpy(VDAY,buf3,2);
 
/* Month */

for (i = 0; i < 12; i++)
   {
   if (strncmp(MONTHTEXT[i],buf2,3)==0)
      {
      AddClassToHeap(MONTHTEXT[i]);
      bzero(VMONTH,4);
      strncpy(VMONTH,MONTHTEXT[i],3);
      break;
      }
   }

/* Year */

strcpy(VYEAR,buf5); 

sprintf(out,"Yr%s",buf5);
AddClassToHeap(out);
}

/*******************************************************************/

int Month2Number(string)

char *string;

{ int i;

if (string == NULL)
   {
   return -1;
   }
 
for (i = 0; i < 12; i++)
   {
   if (strncmp(MONTHTEXT[i],string,strlen(string))==0)
      {
      return i+1;
      break;
      }
   }

return -1;
}

/*******************************************************************/

void AddClassToHeap(class)

char *class;

{
Debug("AddClassToHeap(%s)\n",class);

 if (IsItemIn(VHEAP,class))
   {
   return;
   }

AppendItem(&VHEAP,class,CONTEXTID);
}

/*********************************************************************/

void DeleteClassFromHeap(class)

char *class;

{
DeleteItemMatching(&VHEAP,class);
}

/*********************************************************************/

void DeleteClassesFromContext(context)

char *context;

{ struct Item *ip;

Verbose("Purging private classes from context %s\n",context);
 
for (ip = VHEAP; ip != NULL; ip=ip->next)
   {
   if (strcmp(ip->classes,context) == 0)
      {
      Debug("Deleting context private class %s from heap\n",ip->name);
      DeleteItem(&VHEAP,ip);
      }
   }
}

/*********************************************************************/

int IsHardClass(sp)  /* true if string matches a hardwired class e.g. hpux */

char *sp;

{ int i;

for (i = 2; CLASSTEXT[i] != '\0'; i++)
   {
   if (strcmp(CLASSTEXT[i],sp) == 0)
      {
      return(true);
      }
   }

for (i = 0; i < 7; i++)
   {
   if (strcmp(DAYTEXT[i],sp)==0)
      {
      return(false);
      }
   }

return(false);
}

/*******************************************************************/

int IsSpecialClass(class)

char *class;

{ int value = -1;

if (strncmp(class,"IfElapsed",strlen("IfElapsed")) == 0)
   {
   sscanf(class,"IfElapsed%d",&value);

   if (value < 0)
      {
      Silent("%s: silly IfElapsed parameter in action sequence, using default...\n",VPREFIX);
      return true;
      }

   if (!PARSING)
      {
      VIFELAPSED = value;
      
      Verbose("                  IfElapsed time: %d minutes\n",VIFELAPSED);
      return true;
      }
   }

if (strncmp(class,"ExpireAfter",strlen("ExpireAfter")) == 0)
   {
   sscanf(class,"ExpireAfter%d",&value);

   if (value <= 0)
      {
      Silent("%s: silly ExpireAter parameter in action sequence, using default...\n",VPREFIX);
      return true;
      }

   if (!PARSING)
      {
      VEXPIREAFTER = value;
      Verbose("\n                  ExpireAfter time: %d minutes\n",VEXPIREAFTER); 
      return true;
      }
   }

return false;
}

/*********************************************************************/

int IsExcluded(exception)

char *exception;

{
if (! IsDefinedClass(exception))
   {
   Debug2("%s is excluded!\n",exception);
   return true;
   }  

return false;
}

/*********************************************************************/

int IsDefinedClass(class) 

  /* Evaluates a.b.c|d.e.f etc and returns true if the class */
  /* is currently true, given the defined heap and negations */

char *class;

{
if (class == NULL)
   {
   return true;
   }
 
return EvaluateORString(class,VADDCLASSES);
}


/*********************************************************************/

int IsInstallable(class)

char *class;

  /* Evaluates to true if the class string COULD become true in */
  /* the course of the execution - but might not be true now    */

{ char buffer[bufsize], *sp;
  int i = 0;

for (sp = class; *sp != '\0'; sp++)
   {
   if (*sp == '!')
      {
      continue;         /* some actions might be presented as !class */
      }
   buffer[i++] = *sp; 
   }

buffer[i] = '\0';
 
return (EvaluateORString(buffer,VALLADDCLASSES)||EvaluateORString(class,VALLADDCLASSES)||EvaluateORString(class,VADDCLASSES));
}

/*********************************************************************/

void AddCompoundClass(class)

char *class;

{ char *sp = class;
  char cbuff[maxvarsize];

Debug1("AddCompoundClass(%s)",class);

while(*sp != '\0')
   {
   sscanf(sp,"%[^.]",cbuff);

   while ((*sp != '\0') && (*sp !='.'))
      {
      sp++;
      }

   if (*sp == '.')
      {
      sp++;
      }

   if (IsHardClass(cbuff))
      {
      FatalError("cfengine: You cannot use -D to define a reserved class!");
      }

   AddClassToHeap(cbuff);
   }
}

/*********************************************************************/

void NegateCompoundClass(class,heap)

char *class;
struct Item **heap;

{ char *sp = class;
  char cbuff[maxvarsize];

Debug1("NegateCompoundClass(%s)",class);

while(*sp != '\0')
   {
   sscanf(sp,"%[^.]",cbuff);

   while ((*sp != '\0') && (*sp !='.'))
      {
      sp++;
      }

   if (*sp == '.')
      {
      sp++;
      }

   if (IsHardClass(cbuff))
      { char err[bufsize];
      yyerror("Illegal exception");
      sprintf (err,"Cannot negate the reserved class [%s]\n",cbuff);
      FatalError(err);
      }

   AppendItem(heap,cbuff,NULL);
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

int EvaluateORString(class,list)

char *class;
struct Item *list;

{ char *sp, cbuff[bufsize];
  int result = false;

if (class == NULL)
   {
   return false;
   }

for (sp = class; *sp != '\0'; sp++)
   {
   while (*sp == '|')
      {
      sp++;
      }

   bzero(cbuff,bufsize);

   sp += GetORAtom(sp,cbuff);

   if (strlen(cbuff) == 0)
      {
      break;
      }

   if (IsBracketed(cbuff)) /* Strip brackets */
      {
      cbuff[strlen(cbuff)-1] = '\0';

      result |= EvaluateORString(cbuff+1,list);
      }
   else
      {
      result |= EvaluateANDString(cbuff,list);
      }

   if (*sp == '\0')
      {
      break;
      }
   }

return result;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

int EvaluateANDString(class,list)

char *class;
struct Item *list;

{ char *sp, *atom;
  char cbuff[bufsize];
  int count = 1;
  int negation = false;

count = CountEvalAtoms(class);
sp = class;

while(*sp != '\0')
   {
   negation = false;

   while (*sp == '!')
      {
      negation = !negation;
      sp++;
      }

   bzero(cbuff,bufsize);

   sp += GetANDAtom(sp,cbuff) + 1;

   atom = cbuff;

     /* Test for parentheses */
   
   if (IsBracketed(cbuff))
      {
      atom = cbuff+1;

      cbuff[strlen(cbuff)-1] = '\0';

      if (EvaluateORString(atom,list))
	 {
	 if (negation)
	    {
	    return false;
	    }
	 else
	    {
	    count--;
	    }
	 }
      else
	 {
	 if (negation)
	    {
	    count--;
	    }
	 else
	    {
	    return false;
	    }
	 }

      continue;
      }
   else
      {
      atom = cbuff;
      }
   
   /* End of parenthesis check */
   
   if (*sp == '.')
      {
      sp++;
      }

   if (IsItemIn(VNEGHEAP,atom))
      {
      if (negation)
         {
	 count--;
	 }
      else
	 {
	 return false;
	 }
      } 
   else if (IsItemIn(VHEAP,atom))
      {
      if (negation)
         {
	 return false;
         }
      else
         {
         count--;
         }
      } 
   else if (IsItemIn(list,atom))
      {
      if (negation)
         {
         return false;
         }
      else
         {
         count--;
         }
      } 
   else if (negation)    /* ! (an undefined class) == true */
      {
      count--;
      }
   else
      {
      return false;
      }
   }

if (count == 0)
   {
   return(true);
   }
else
   {
   return(false);
   }
}

/*********************************************************************/

int GetORAtom(start,buffer)

char *start, *buffer;

{ char *sp = start;
  char *spc = buffer;
  int bracklevel = 0, len = 0;

while ((*sp != '\0') && !((*sp == '|') && (bracklevel == 0)))
   {
   if (*sp == '(')
      {
      bracklevel++;
      }

   if (*sp == ')')
      {
      bracklevel--;
      }

   *spc++ = *sp++;
   len++;
   }

*spc = '\0';

return len;
}

/*********************************************************************/
/* Level 4                                                           */
/*********************************************************************/

int GetANDAtom(start,buffer)

char *start, *buffer;

{ char *sp = start;
  char *spc = buffer;
  int bracklevel = 0, len = 0;

while ((*sp != '\0') && !((*sp == '.') && (bracklevel == 0)))
   {
   if (*sp == '(')
      {
      bracklevel++;
      }

   if (*sp == ')')
      {
      bracklevel--;
      }

   *spc++ = *sp++;

   len++;
   }

*spc = '\0';

return len;
}

/*********************************************************************/

int CountEvalAtoms(class)

char *class;

{ char *sp;
  int count = 0, bracklevel = 0;
  
for (sp = class; *sp != '\0'; sp++)
   {
   if (*sp == '(')
      {
      bracklevel++;
      continue;
      }

   if (*sp == ')')
      {
      bracklevel--;
      continue;
      }
   
   if ((bracklevel == 0) && (*sp == '.'))
      {
      count++;
      }
   }

if (bracklevel != 0)
   {
   sprintf(OUTPUT,"Bracket mismatch, in [class=%s], level = %d\n",class,bracklevel);
   yyerror(OUTPUT);;
   FatalError("Aborted");
   }

return count+1;
}

/*********************************************************************/
/* TOOLKIT : actions                                                 */
/*********************************************************************/

enum actions ActionStringToCode (str)

char *str;

{ char *sp;
  int i;
  enum actions action;

action = none;

for (sp = str; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   if (*sp == ':')
      {
      *sp = '\0';
      }
   }

for (i = 1; ACTIONID[i] != '\0'; i++)
   {
   if (strcmp(ACTIONID[i],str) == 0)
      {
      action = (enum actions) i;
      break;
      }
   }

if (action == none)
  {
  yyerror("Indexed macro specified no action");
  FatalError("Could not compile action");
  }

return (enum actions) i;
}

/*********************************************************************/

int IsBracketed(s)

 /* return true if the entire string is bracketed, not just if
    if contains brackets */

char *s;

{ int i, level= 0;

if (*s != '(')
   {
   return false;
   }

for (i = 0; i < strlen(s)-1; i++)
   {
   if (s[i] == '(')
      {
      level++;
      }
   
   if (s[i] == ')')
      {
      level--;
      }

   if (level == 0)
      {
      return false;  /* premature ) */
      }
   }

return true;
}
