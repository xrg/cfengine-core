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

 /* This is for matching ranges of IP addresses, like CIDR e.g.

 Range1 = ( 128.39.89.250/24 )
 Range2 = ( 128.39.89.100-101 )
 
 */

struct Item *list;
char *item;

{ struct Item *ptr; 

Debug("\nFuzzyItemIn(LIST,%s)\n",item);
 
if ((item == NULL) || (strlen(item) == 0))
   {
   return true;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   Debug(" Try FuzzySetMatch(%s,%s)\n",ptr->name,item);
   
   if (FuzzySetMatch(ptr->name,item) == 0)
      {
      return(true);
      }
   }
 
return(false);
}

/*********************************************************************/

struct Item *ConcatLists (list1, list2)

struct Item *list1, *list2;

/* Notes: * Refrain from freeing list2 after using ConcatLists
          * list1 must have at least one element in it */

{ struct Item *endOfList1;

if (list1 == NULL)
   {
   FatalError("ConcatLists: first argument must have at least one element");
   }

for (endOfList1=list1; endOfList1->next!=NULL; endOfList1=endOfList1->next)
   {
   }
endOfList1->next = list2;
return list1;
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
/* Fuzzy Match                                                       */
/*********************************************************************/

int FuzzyMatchParse(s)

char *s;

{ char *sp;
  short isCIDR = false, isrange = false, isv6 = false, isv4 = false, isADDR = false; 
  char address[128];
  int mask,count = 0;

Debug("Check ParsingIPRange(%s)\n",s);

for (sp = s; *sp != '\0'; sp++)  /* Is this an address or hostname */
   {
   if (!isxdigit((int)*sp))
      {
      isADDR = false;
      break;
      }
   
   if (*sp == ':')              /* Catches any ipv6 address */
      {
      isADDR = true;
      break;
      }

   if (isdigit((int)*sp))      /* catch non-ipv4 address - no more than 3 digits */
      {
      count++;
      if (count > 3)
	 {
	 isADDR = false;
	 break;
	 }
      }
   else
      {
      count = 0;
      }
   }

if (! isADDR)
   {
   return true;
   }
 
if (strstr(s,"/") != 0)
   {
   isCIDR = true;
   }

if (strstr(s,"-") != 0)
   {
   isrange = true;
   }

if (strstr(s,".") != 0)
   {
   isv4 = true;
   }

if (strstr(s,":") != 0)
   {
   isv6 = true;
   }

if (isv4 && isv6)
   {
   yyerror("Mixture of IPv6 and IPv4 addresses");
   return false;
   }

if (isCIDR && isrange)
   {
   yyerror("Cannot mix CIDR notation with xx-yy range notation");
   return false;
   }

if (isv4 && isCIDR)
   {
   if (strlen(s) > 4+3*4+1+2)  /* xxx.yyy.zzz.mmm/cc */
      {
      yyerror("IPv4 address looks too long");
      return false;
      }
   
   address[0] = '\0';
   mask = 0;
   sscanf(s,"%16[^/]/%d",address,&mask);

   if (mask < 8)
      {
      snprintf(OUTPUT,bufsize,"Mask value %d in %s is less than 8",mask,s);
      yyerror(OUTPUT);
      return false;
      }

   if (mask > 30)
      {
      snprintf(OUTPUT,bufsize,"Mask value %d in %s is silly (> 30)",mask,s);
      yyerror(OUTPUT);
      return false;
      }
   }


if (isv4 && isrange)
   {
   long i, from = -1, to = -1, cmp = -1;
   char *sp1,buffer1[8],buffer2[8];
   
   sp1 = s;
   
   for (i = 0; i < 4; i++)
      {
      sscanf(sp1,"%[^.]",buffer1);
      sp1 += strlen(buffer1)+1;
      
      if (strstr(buffer1,"-"))
	 {
	 sscanf(buffer1,"%ld-%ld",&from,&to);
	 sscanf(buffer2,"%ld",&cmp);
	 if (from < 0 || to < 0)
	    {
	    yyerror("Error in IP range - looks like address, or bad hostname");
	    return false;
	    }
	 
	 if (to < from)
	    {
	    yyerror("Bad IP range");
	    return false;
	    }
	 
	 if ((from >= cmp) || (cmp > to))
	    {
	    return false;
	    }
	 }
      else
	 {
	 sscanf(buffer1,"%ld",&from);
	 sscanf(buffer2,"%ld",&cmp);
	 
	 if (from != cmp)
	    {
	    return false;
	    }
	 }
      }
   }

if (isv6 && isCIDR)
   {
   char address[128];
   int mask,blocks;

   if (strlen(s) < 20)
      {
      yyerror("IPv6 address looks too short");
      return false;
      }

   if (strlen(s) > 42)
      {
      yyerror("IPv6 address looks too long");
      return false;
      }

   address[0] = '\0';
   mask = 0;
   sscanf(s,"%40[^/]/%d",address,&mask);
   blocks = mask/8;
   
   if (mask % 8 != 0)
      {
      CfLog(cferror,"Cannot handle ipv6 masks which are not 8 bit multiples (fix me)","");
      return false;
      }
   
   if (mask > 15)
      {
      yyerror("IPv6 CIDR mask is too large");
      return false;
      }
   }

return true; 
}

/*********************************************************************/

int FuzzySetMatch(s1,s2)

/* Match two IP strings - with : or . in hex or decimal
   s1 is the test string, and s2 is the reference e.g.
   FuzzySetMatch("128.39.74.10/23","128.39.75.56") == 0 */

char *s1,*s2;

{ char *sp;
  short isCIDR = false, isrange = false, isv6 = false, isv4 = false;
  char address[128];
  int mask,significant;
  unsigned long a1,a2;

if (strstr(s1,"/") != 0)
   {
   isCIDR = true;
   }

if (strstr(s1,"-") != 0)
   {
   isrange = true;
   }

if (strstr(s1,".") != 0)
   {
   isv4 = true;
   }

if (strstr(s1,":") != 0)
   {
   isv6 = true;
   }

if (isv4 && isv6)
   {
   snprintf(OUTPUT,bufsize,"Mixture of IPv6 and IPv4 addresses: %s",s1);
   CfLog(cferror,OUTPUT,"");
   return -1;
   }

if (isCIDR && isrange)
   {
   snprintf(OUTPUT,bufsize,"Cannot mix CIDR notation with xxx-yyy range notation: %s",s1);
   CfLog(cferror,OUTPUT,"");
   return -1;
   }

if (!(isv6 || isv4))
   {
   snprintf(OUTPUT,bufsize,"Not a valid address range - or not a fully qualified name: %s",s1);
   CfLog(cferror,OUTPUT,"");
   return -1;
   }

if (!(isrange||isCIDR)) 
   {
   return strncmp(s1,s2,strlen(s1)); /* do partial string match */
   }

 
if (isv4)
   {
   struct sockaddr_in addr1,addr2;
   int shift;

   bzero(&addr1,sizeof(struct sockaddr_in));
   bzero(&addr2,sizeof(struct sockaddr_in));
   
   if (isCIDR)
      {
      address[0] = '\0';
      mask = 0;
      sscanf(s1,"%16[^/]/%d",address,&mask);
      shift = 32 - mask;
      
      bcopy((struct sockaddr_in *) sockaddr_pton(AF_INET,address),&addr1,sizeof(struct sockaddr_in));
      bcopy((struct sockaddr_in *) sockaddr_pton(AF_INET,s2),&addr2,sizeof(struct sockaddr_in));

      a1 = htonl(addr1.sin_addr.s_addr);
      a2 = htonl(addr2.sin_addr.s_addr);

      a1 = a1 >> shift;
      a2 = a2 >> shift;

      if (a1 == a2)
	 {
	 return 0;
	 }
      else
	 {
	 return -1;
	 }
      }
   else
      {
      long i, from = -1, to = -1, cmp = -1;
      char *sp1,*sp2,buffer1[8],buffer2[8];

      sp1 = s1;
      sp2 = s2;
      
      for (i = 0; i < 4; i++)
	 {
         if (sscanf(sp1,"%[^.]",buffer1) <= 0)
            {
            break;
            }
	 sp1 += strlen(buffer1)+1;
	 sscanf(sp2,"%[^.]",buffer2);
	 sp2 += strlen(buffer2)+1;

	 if (strstr(buffer1,"-"))
	    {
	    sscanf(buffer1,"%ld-%ld",&from,&to);
	    sscanf(buffer2,"%ld",&cmp);

	    if (from < 0 || to < 0)
	       {
	       Debug("Couldn't read range\n");
	       return -1;
	       }

	    if ((from > cmp) || (cmp > to))
	       {
	       Debug("Out of range %d > %d > %d (range %s)\n",from,cmp,to,buffer2);
	       return -1;
	       }
	    }
	 else
	    {
	    sscanf(buffer1,"%ld",&from);
	    sscanf(buffer2,"%ld",&cmp);

	    if (from != cmp)
	       {
	       Debug("Unequal\n");
	       return -1;
	       }
	    }

	 Debug("Matched octet %s with %s\n",buffer1,buffer2);
	 }

      Debug("Matched IP range\n");
      return 0;
      }
   }

#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
if (isv6)
   {
   struct sockaddr_in6 addr1,addr2;
   int blocks, i;

   bzero(&addr1,sizeof(struct sockaddr_in6));
   bzero(&addr2,sizeof(struct sockaddr_in6));
   
   if (isCIDR)
      {
      address[0] = '\0';
      mask = 0;
      sscanf(s1,"%40[^/]/%d",address,&mask);
      blocks = mask/8;

      if (mask % 8 != 0)
	 {
	 CfLog(cferror,"Cannot handle ipv6 masks which are not 8 bit multiples (fix me)","");
	 return -1;
	 }

      bcopy((struct sockaddr_in6 *) sockaddr_pton(AF_INET6,address),&addr1,sizeof(struct sockaddr_in6));
      bcopy((struct sockaddr_in6 *) sockaddr_pton(AF_INET6,s2),&addr2,sizeof(struct sockaddr_in6));

      for (i = 0; i < blocks; i++) /* blocks < 16 */
	 {
	 if (addr1.sin6_addr.s6_addr[i] != addr2.sin6_addr.s6_addr[i])
	    {
	    return -1;
	    }
	 }
      return 0;
      }
   else
      {
      long i, from = -1, to = -1, cmp = -1;
      char *sp1,*sp2,buffer1[16],buffer2[16];

      sp1 = s1;
      sp2 = s2;
      
      for (i = 0; i < 8; i++)
	 {
	 sscanf(sp1,"%[^:]",buffer1);
	 sp1 += strlen(buffer1)+1;
	 sscanf(sp2,"%[^:]",buffer2);
	 sp2 += strlen(buffer2)+1;

	 if (strstr(buffer1,"-"))
	    {
	    sscanf(buffer1,"%lx-%lx",&from,&to);
	    sscanf(buffer2,"%lx",&cmp);

	    if (from < 0 || to < 0)
	       {
	       return -1;
	       }

	    if ((from >= cmp) || (cmp > to))
	       {
	       printf("%x < %x < %x\n",from,cmp,to);
	       return -1;
	       }
	    }
	 else
	    {
	    sscanf(buffer1,"%ld",&from);
	    sscanf(buffer2,"%ld",&cmp);

	    if (from != cmp)
	       {
	       return -1;
	       }
	    }
	 }
      
      return 0;
      }
   }
#endif 

return -1; 
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


