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

/**************************************************************************/
/*                                                                        */
/* File: strategies.c                                                     */
/*                                                                        */
/**************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*****************************************************************************/

void InstallStrategy(alias,classes)

char *alias, *classes;

{ struct Strategy *ptr;
 
 Debug1("InstallStrategy(%s,%s)\n",alias,classes);
 
 if (! IsInstallable(classes))
    {
    Debug1("Not installing Strategy no match\n");
    return;
    }
 
 ExpandVarstring(alias,VBUFF,"");
 
 if ((ptr = (struct Strategy *)malloc(sizeof(struct Strategy))) == NULL)
    {
    FatalError("Memory Allocation failed for InstallStrategy() #1");
    }
 
 if ((ptr->name = strdup(VBUFF)) == NULL)
    {
    FatalError("Memory Allocation failed in InstallStrategy");
    }

 ExpandVarstring(classes,VBUFF,"");

 if ((ptr->classes = strdup(VBUFF)) == NULL)
    {
    FatalError("Memory Allocation failed in InstallStrategy");
    }
 
 if (VSTRATEGYLISTTOP == NULL)                 /* First element in the list */
    {
    VSTRATEGYLIST = ptr;
    }
 else
    {
    VSTRATEGYLISTTOP->next = ptr;
    }
 
 ptr->next = NULL;
 ptr->type = 'r';
 ptr->strategies = NULL;

 VSTRATEGYLISTTOP = ptr;
}

/*****************************************************************************/

void AddClassToStrategy(alias,class,value)

char *alias,*class,*value;

{ struct Strategy *sp;
  char buf[maxvarsize];
  int val = -1;

if (class[strlen(class)-1] != ':')
   {
   yyerror("Strategic class definition doesn't end in colon");
   return;
   }
  
bzero(buf,maxvarsize);
sscanf(class,"%[^:]",&buf);
 
ExpandVarstring(value,VBUFF,"");
Debug("AddClassToStrategy(%s,%s,%s)\n",alias,class,VBUFF);

sscanf(VBUFF,"%d",&val);

if (val <= 0)
   {
   yyerror("strategy distribution weight must be an integer");
   return;
   }
 
for (sp = VSTRATEGYLIST; sp != NULL; sp=sp->next)
   {
   if (strcmp(alias,sp->name) == 0)
      {
      AppendItem(&(sp->strategies),buf,VBUFF);
      AddInstallable(buf);
      }
   }
}

/*****************************************************************************/

void SetStrategies()

{ struct Strategy *ptr;
  struct Item *ip; 
  int total,count;
  double *array,*cumulative,cum,fluct;

Banner("Strategy evaluation");
 
for (ptr = VSTRATEGYLIST; ptr != NULL; ptr=ptr->next)
   {
   Verbose("Evaluating stratgey %s (type=%c)\n",ptr->name,ptr->type);
   if (ptr->strategies)
      {
      total = count = 0;
      for (ip = ptr->strategies; ip !=NULL; ip=ip->next)
	 {
	 count++;
	 total += atoi(ip->classes);
	 }

      count++;
      array = (double *)malloc(count*sizeof(double));
      cumulative = (double *)malloc(count*sizeof(double));
      count = 1;
      cum = 0.0;
      cumulative[0] = 0;
            
      for (ip = ptr->strategies; ip !=NULL; ip=ip->next)
	 {
	 array[count] = ((double)atoi(ip->classes))/((double)total);
	 cum += array[count];
	 cumulative[count] = cum;
	 Debug("%s : %f cum %f\n",ip->name,array[count],cumulative[count]);
	 count++;
	 }

      /* Get random number 0-1 */

      fluct = drand48();

      count = 1;
      cum = 0.0;

      for (ip = ptr->strategies; ip !=NULL; ip=ip->next)
	 {
	 Verbose("Class %d: %f-%f\n",count,cumulative[count-1],cumulative[count]);
	 if ((cumulative[count-1] < fluct) && (fluct < cumulative[count]))
	    {
	    Verbose(" - Choosing %s (%f)\n\n",ip->name,fluct);
	    AddClassToHeap(ip->name);
	    break;
	    }
	 count++;
	 }
      free(cumulative);
      free(array);
      }
   }

Banner("End strategy evaluation"); 
}

/*****************************************************************************/



/* END */
