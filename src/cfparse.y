%{
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
/*  PARSER for cfengine                                            */
/*                                                                 */
/*******************************************************************/

#include <stdio.h>
#include "cf.defs.h"
#include "cf.extern.h"

extern char *yytext;

%}

%token ID LIST ITEM VARPATH PATH LBRACK RBRACK CONTROL GROUPS
%token ARROW EQUALS EDITFILES QSTRING VARITEM WILDCARD
%token LBRACE RBRACE PARSECLASS LARROW
%token ACL ADMIT DENY FILTERS STRATEGIES ACTIONTYPE

%%

specification:       { yyerror("Warning: invalid statement"); }
                     | statements;

statements:            statement
                     | statements statement;

statement:             CONTROL controllist
                     | CONTROL
                     | GROUPS declarations
                     | GROUPS
                     | ACTIONTYPE classlist
                     | ACTIONTYPE
                     | ACL objects
                     | ACL
                     | FILTERS objects
                     | FILTERS
                     | STRATEGIES objects
                     | STRATEGIES
                     | ADMIT classlist
                     | ADMIT
                     | DENY classlist
                     | DENY
                     | EDITFILES
                     | EDITFILES objects;

controllist:           declarations
                     | PARSECLASS declarations
                     | PARSECLASS
                     | controllist PARSECLASS
                     | controllist PARSECLASS declarations;

declarations:          declaration
                     | declarations declaration;

classlist:             list
                     | PARSECLASS list
                     | PARSECLASS
                     | classlist PARSECLASS
                     | classlist PARSECLASS list;

declaration:           ID EQUALS bracketlist;

bracketlist:           LBRACK list RBRACK;

list:                  entry
                     | list entry;

entry:                 ITEM              
                     | PATH ARROW PATH
                     | PATH ARROW VARPATH 
                     | PATH LARROW PATH
                     | PATH LARROW ITEM
                     | PATH LARROW VARPATH
                     | PATH ARROW WILDCARD
                     | VARPATH ARROW WILDCARD
                     | PATH LARROW WILDCARD
                     | VARPATH LARROW WILDCARD
                     | VARPATH ARROW PATH
                     | VARPATH ARROW VARPATH 
                     | VARPATH LARROW PATH
                     | VARPATH LARROW ITEM
                     | VARPATH LARROW VARPATH
                     | PATH
                     | ID
                     | VARPATH
                     | VARITEM
                     | WILDCARD       
                     | QSTRING;


objects:               objectbrackets
                     | PARSECLASS
                     | PARSECLASS objectbrackets
                     | objects PARSECLASS
                     | objects PARSECLASS objectbrackets;

objectbrackets:        objectbracket
                     | objectbrackets objectbracket;

objectbracket:         LBRACE PATH objlist RBRACE
                     | LBRACE VARPATH objlist RBRACE
                     | LBRACE ID objlist RBRACE
                     | LBRACE ITEM objlist RBRACE;

objlist:               obj
                     | objlist obj;

obj:                   ITEM QSTRING
                     | ITEM
                     | PATH
                     | WILDCARD
                     | WILDCARD QSTRING;

%%

/*****************************************************************/

void yyerror(s)

char *s;

{
fprintf (stderr, "cf:%s:%s:%d: %s \n",VPREFIX,VCURRENTFILE,LINENUMBER,s);

ERRORCOUNT++;

if (ERRORCOUNT > 10)
   {
   FatalError("Too many errors");
   }
}

/*****************************************************************/

/* EOF */
