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


/*******************************************************************/
/* Macro substitution based on HASH-table                          */
/*******************************************************************/


void SetContext(id)

char *id;

{
InstallObject(id);
 
Verbose("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
Verbose(" * (Changing context state to: %s) *",id);
Verbose("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n\n"); 
strncpy(CONTEXTID,id,31);
}

/*******************************************************************/

void InitHashTable(table)

char **table;

{ int i;

for (i = 0; i < hashtablesize; i++)
   {
   table[i] = NULL;
   }
}

/*******************************************************************/

void PrintHashTable(table)

char **table;

{ int i;

for (i = 0; i < hashtablesize; i++)
   {
   if (table[i] != NULL)
      {
      printf ("   %d : %s\n",i,table[i]);
      }
   }
}

/*******************************************************************/

int Hash(name)

char *name;

{ int i, slot = 0;

 for (i = 0; name[i] != '\0'; i++)
   {
   slot = (macroalphabet * slot + name[i]) % hashtablesize;
   }

return slot;
}

/*******************************************************************/

int ElfHash(key)

char *key;

{ unsigned int h = 0;
  unsigned int g;

while (*key)
  {
  h = (h << 4) + *key++;

  g = h & 0xF0000000;         /* Assumes int is 32 bit */

  if (g) 
     {
     h ^= g >> 24;
     }

  h &= ~g;
  }

return (h % hashtablesize);
}

/*******************************************************************/

void AddMacroValue(scope,name,value)

char *scope,*name,*value;

{ char *sp, buffer[bufsize],exp[bufsize];
  struct cfObject *ptr; 
  int slot;

Debug("AddMacroValue(%s.%s=%s)\n",scope,name,value);

if (name == NULL || value == NULL || scope == NULL)
   {
   yyerror("Bad macro or scope");
   CfLog(cferror,"Empty macro","");
   }

if (strlen(name) > maxvarsize)
   {
   yyerror("macro name too long");
   return;
   }

ExpandVarstring(value,exp,NULL);

ptr = ObjectContext(scope);
 
snprintf(buffer,bufsize,"%s=%s",name,exp);

if ((sp = malloc(strlen(buffer)+1)) == NULL)
   {
   perror("malloc");
   FatalError("aborting");
   }

strcpy(sp,buffer);

slot = Hash(name);
 
if (ptr->hashtable[slot] != 0)
   {
   Debug("Macro Collision!\n");
   if (CompareMacro(name,ptr->hashtable[slot]) == 0)
      {
      if (PARSING && !IsItemIn(VREDEFINES,name))
	 {
	 snprintf(VBUFF,bufsize,"Redefinition of macro %s=%s (or perhaps missing quote)",name,exp);
	 Warning(VBUFF);
	 }
      free(ptr->hashtable[slot]);
      ptr->hashtable[slot] = sp;
      return;
      }

   while ((ptr->hashtable[++slot % hashtablesize] != 0))
      {
      if (slot == hashtablesize-1)
         {
         slot = 0;
         }
      if (slot == Hash(name))
         {
         FatalError("AddMacroValue - internal error #1");
         }

      if (CompareMacro(name,ptr->hashtable[slot]) == 0)
	 {
	 snprintf(VBUFF,bufsize,"Redefinition of macro %s=%s",name,exp);
	 Warning(VBUFF);
	 free(ptr->hashtable[slot]);
	 ptr->hashtable[slot] = sp;
	 return;
	 }
      }
   }

ptr->hashtable[slot] = sp;

Debug("Added Macro at hash address %d to object %s with value %s\n",slot,scope,sp);
}

/*******************************************************************/

char *GetMacroValue(scope,name)

char *scope,*name;

{ char *sp;
  int slot,i;
  struct cfObject *ptr;
  
/* Check the class.id identity to see if this is private ..
    and replace

     HASH[slot] by objectptr->hash[slot]

*/

  
ptr = ObjectContext(scope);	     
i = slot = Hash(name);
 
if (CompareMacro(name,ptr->hashtable[slot]) != 0)
   {
   while (true)
      {
      i++;

      if (i >= hashtablesize-1)
         {
         i = 0;
         }

      if (CompareMacro(name,ptr->hashtable[i]) == 0)
         {
         for(sp = ptr->hashtable[i]; *sp != '='; sp++)
            {
            }

         return(sp+1);
         }

      if (i == slot-1)
         {
         return(getenv(name));  /* Look in environment if not found */
         }
      }
   }
else
   {
   for(sp = ptr->hashtable[slot]; *sp != '='; sp++)
      {
      }

   return(sp+1);
   }   
}

/*******************************************************************/

void DeleteMacro(scope,id)

char *id, *scope;

{ int slot,i;
  struct cfObject *ptr;
  
i = slot = Hash(id);
ptr = ObjectContext(scope);
 
if (CompareMacro(id,ptr->hashtable[slot]) != 0)
   {
   while (true)
      {
      i++;

      if (i == slot)
	 {
	 Debug("No macro matched\n");
	 break;
	 }
      
      if (i >= hashtablesize-1)
         {
         i = 0;
         }

      if (CompareMacro(id,ptr->hashtable[i]) == 0)
         {
         free(ptr->hashtable[i]);
	 ptr->hashtable[i] = NULL;
         }
      }
   }
else
   {
   free(ptr->hashtable[i]);
   ptr->hashtable[i] = NULL;
   }   
}


/*******************************************************************/

int CompareMacro(name,macro)

char *name,*macro;

{ char buffer[bufsize];

if (macro == NULL || name == NULL)
   {
   return 1;
   }

sscanf(macro,"%[^=]",buffer);

Debug1("CompareMacro(%s,%s)=%s\n",name,macro,buffer);
return(strcmp(buffer,name));
}

/*******************************************************************/

void DeleteMacros(scope)

char *scope;

{ int i;
  struct cfObject *ptr;
  
ptr = ObjectContext(scope);
 
for (i = 0; i < hashtablesize; i++)
   {
   if (ptr->hashtable[i] != NULL)
      {
      free(ptr->hashtable[i]);
      ptr->hashtable[i] = NULL;
      }
   }
}

/*******************************************************************/

struct cfObject *ObjectContext(scope)

char *scope;

{ struct cfObject *cp = NULL;

 for (cp = VOBJ; cp != NULL; cp=cp->next)
    {
    if (strcmp(cp->scope,scope) == 0)
       {
       break;
       }
    }

return cp;
}

