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


/*********************************************************************/
/*                                                                   */
/*  TOOLKIT: the "item" object library for cfengine                  */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*********************************************************************/
/* TOOLKIT : Item list                                               */
/*********************************************************************/

int IsItemIn(list,item)

struct Item *list;
char *item;

{ struct Item *ptr; 

if ((item == NULL) || (strlen(item) == 0))
   {
   return true;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->name,item) == 0)
      {
      return(true);
      }
   }
 
return(false);
}


/*********************************************************************/

int IsFuzzyItemIn(list,item)

struct Item *list;
char *item;

{ struct Item *ptr; 

Debug("FuzzyItemIn(%s)\n",item);
 
if ((item == NULL) || (strlen(item) == 0))
   {
   return true;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strncmp(ptr->name,item,strlen(ptr->name)) == 0)
      {
      return(true);
      }
   }
 
return(false);
}

/*********************************************************************/

void PrependItem (liststart,itemstring,classes)

struct Item **liststart;
char *itemstring,*classes;

{ struct Item *ip;
  char *sp,*spe = NULL;

EditVerbose("Prepending %s\n",itemstring);

if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((sp = malloc(strlen(itemstring)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((classes != NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

strcpy(sp,itemstring);
ip->name = sp;
ip->next = *liststart;
*liststart = ip;

if (classes != NULL)
   {
   strcpy(spe,classes);
   ip->classes = spe;
   }
else
   {
   ip->classes = NULL;
   }

NUMBEROFEDITS++;
}

/*********************************************************************/

void AppendItem (liststart,itemstring,classes)

struct Item **liststart;
char *itemstring,*classes;

{ struct Item *ip, *lp;
  char *sp,*spe = NULL;

EditVerbose("Appending [%s]\n",itemstring);

if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((sp = malloc(strlen(itemstring)+extra_space)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if (*liststart == NULL)
   {
   *liststart = ip;
   }
else
   {
   for (lp = *liststart; lp->next != NULL; lp=lp->next)
      {
      }

   lp->next = ip;
   }

if ((classes!= NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

strcpy(sp,itemstring);
ip->name = sp;
ip->next = NULL;

if (classes != NULL)
   {
   strcpy(spe,classes);
   ip->classes = spe;
   }
else
   {
   ip->classes = NULL;
   }

NUMBEROFEDITS++;
}

/*********************************************************************/

void DeleteItemList(item)                /* delete starting from item */
 
struct Item *item;

{
if (item != NULL)
   {
   DeleteItemList(item->next);
   item->next = NULL;

   if (item->name != NULL)
      {
      free (item->name);
      }

   if (item->classes != NULL)
      {
      free (item->classes);
      }

   free((char *)item);
   }
}

/*********************************************************************/

void DeleteItem(liststart,item)
 
struct Item **liststart,*item;

{ struct Item *ip, *sp;

if (item != NULL)
   {
   EditVerbose("Delete Item: %s\n",item->name);

   if (item->name != NULL)
      {
      free (item->name);
      }

   if (item->classes != NULL)
      {
      free (item->classes);
      }

   sp = item->next;

   if (item == *liststart)
      {
      *liststart = sp;
      }
   else
      {
      for (ip = *liststart; ip->next != item; ip=ip->next)
         {
         }

      ip->next = sp;
      }

   free((char *)item);

   NUMBEROFEDITS++;
   }
}

/*********************************************************************/


void DebugListItemList(liststart)

struct Item *liststart;

{ struct Item *ptr;

for (ptr = liststart; ptr != NULL; ptr=ptr->next)
   {
   printf("CFDEBUG: [%s]\n",ptr->name);
   }
}

/*********************************************************************/

int ItemListsEqual(list1,list2)

struct Item *list1, *list2;

{ struct Item *ip1, *ip2;

ip1 = list1;
ip2 = list2;

while (true)
   {
   if ((ip1 == NULL) && (ip2 == NULL))
      {
      return true;
      }

   if ((ip1 == NULL) || (ip2 == NULL))
      {
      return false;
      }
   
   if (strcmp(ip1->name,ip2->name) != 0)
      {
      return false;
      }

   ip1 = ip1->next;
   ip2 = ip2->next;
   }
}

/*********************************************************************/
/* String Handling                                                   */
/*********************************************************************/

struct Item *SplitStringAsItemList(string,sep)

 /* Splits a string containing a separator like : 
    into a linked list of separate items, */

char *string;
char sep;

{ struct Item *liststart = NULL;
  char format[9], *sp;
  char node[maxvarsize];
  
Debug("SplitStringAsItemList(%s,%c)\n",string,sep);

sprintf(format,"%%255[^%c]",sep);   /* set format string to search */

for (sp = string; *sp != '\0'; sp++)
   {
   bzero(node,maxvarsize);
   sscanf(sp,format,node);

   if (strlen(node) == 0)
      {
      continue;
      }
   
   sp += strlen(node)-1;

   AppendItem(&liststart,node,NULL);

   if (*sp == '\0')
      {
      break;
      }
   }

return liststart;
}


