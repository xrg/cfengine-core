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


char *HASH[hashtablesize];


/*******************************************************************/
/* Macro substitution based on HASH-table                          */
/*******************************************************************/

void InitHashTable()

{ int i;

for (i = 0; i < hashtablesize; i++)
   {
   HASH[i] = NULL;
   }
}

/*******************************************************************/

void PrintHashTable()

{ int i;

for (i = 0; i < hashtablesize; i++)
   {
   if (HASH[i] != NULL)
      {
      printf ("%d : %s\n",i,HASH[i]);
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

void AddMacroValue(name,value)                           /* Like putenv */

char *name,*value;

{ char *sp, buffer[bufsize],exp[bufsize];
  int slot;

Debug("AddMacroValue(%s=%s)\n",name,value);

if (name == NULL || value == NULL)
   {
   yyerror("Bad macro");
   CfLog(cferror,"Empty macro","");
   }

if (strlen(name) > maxvarsize)
   {
   yyerror("macro name too long");
   return;
   }

ExpandVarstring(value,exp,NULL);

snprintf(buffer,bufsize,"%s=%s",name,exp);

if ((sp = malloc(strlen(buffer)+1)) == NULL)
   {
   perror("malloc");
   FatalError("aborting");
   }

strcpy(sp,buffer);

slot = Hash(name);
 
if (HASH[slot] != 0)
   {
   Debug("Macro Collision!\n");
   if (CompareMacro(name,HASH[slot]) == 0)
      {
      if (PARSING && !IsItemIn(VREDEFINES,name))
	 {
	 snprintf(VBUFF,bufsize,"Redefinition of macro %s=%s",name,exp);
	 Warning(VBUFF);
	 }
      free(HASH[slot]);
      HASH[slot] = sp;
      return;
      }

   while ((HASH[++slot % hashtablesize] != 0))
      {
      if (slot == hashtablesize-1)
         {
         slot = 0;
         }
      if (slot == Hash(name))
         {
         FatalError("AddMacroValue - internal error #1");
         }

      if (CompareMacro(name,HASH[slot]) == 0)
	 {
	 snprintf(VBUFF,bufsize,"Redefinition of macro %s=%s",name,exp);
	 Warning(VBUFF);
	 free(HASH[slot]);
	 HASH[slot] = sp;
	 return;
	 }
      }
   }

HASH[slot] = sp;

Debug("Added Macro at hash address %d: %s\n",slot,sp);
}

/*******************************************************************/

char *GetMacroValue(name)

char *name;

{ char *sp;
  int slot,i;

i = slot = Hash(name);
 
if (CompareMacro(name,HASH[slot]) != 0)
   {
   while (true)
      {
      i++;

      if (i >= hashtablesize-1)
         {
         i = 0;
         }

      if (CompareMacro(name,HASH[i]) == 0)
         {
         for(sp = HASH[i]; *sp != '='; sp++)
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
   for(sp = HASH[slot]; *sp != '='; sp++)
      {
      }

   return(sp+1);
   }   
}

/*******************************************************************/

void DeleteMacro(id)

char *id;

{ int slot,i;

i = slot = Hash(id);
 
if (CompareMacro(id,HASH[slot]) != 0)
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

      if (CompareMacro(id,HASH[i]) == 0)
         {
         free(HASH[i]);
	 HASH[i] = NULL;
         }
      }
   }
else
   {
   free(HASH[i]);
   HASH[i] = NULL;
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

void DeleteMacros()

{ int i;

for (i = 0; i < hashtablesize; i++)
   {
   if (HASH[i] != NULL)
      {
      free(HASH[i]);
      HASH[i] = NULL;
      }
   }
}
