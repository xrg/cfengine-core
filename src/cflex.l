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
/*  LEXER for cfengine                                             */
/*                                                                 */
/*                                                                 */
/*******************************************************************/


 /* Here we define a two state lexer. The states exclude
    certain regular expression matches so as to more easily
    distinguish between class constructions like 

       compound.class::

    identifiers like

       group = ( .... )

    and items inside the parentheses like

       .... = ( alpha beta gamma )

    The state
    <IDENT> recognizes identifiers and <INITIAL> recognizes what
    is an item in a LisT.

 */


#include "cfparse.h"
#include "cf.defs.h"
#include "cf.extern.h"


  /* Note the %o %a declarations to allow more internal workspace */
%}

%option noyywrap
%s IDENT
%o 30000
%a 16000
%p 9000
%n 800

actiont   copy:|resolve:|files:|tidy:|homeservers:|binservers:|mailserver:|disks:|mountables:|links:|shellcommands:|disable:|directories:|ignore:|interfaces:|defaultroute:|miscmounts:|unmount:|processes:|required:|broadcast:
space     [ \t]+
comment   #.*
linedir   #line[ \t]+[0-9]+.*
function  [a-zA-Z_]+\([^)]*\)
qstring   \"[^"\n]*\"|\'[^'\n]*\'|`[^`\n]*`
item      [+]?[-.,!@a-zA-Z0-9_\200-\377]+
id        [.|!,;a-zA-Z0-9_\200-\377]+
classt    [.|!,;a-zA-Z0-9_()\200-\377]+::
path      [-:.,;/a-zA-Z0-9%_\200-\377]+
varpath   [/$<][-.,;a-zA-Z0-9<>%@+:$*?(){}/_\200-\377]*
varitem   [-.,a-zA-Z0-9<>$*?()={}/_\200-\377]*
wildcard  [^\135 ()>\t\n|&@\175\200-\377]+
newline   [\n]
lbrack    \(
rbrack    \)
lbrace    \{
rbrace    \}
equals    =
forcearrow ->!
lforcearrow \+>!
arrow     ->
larrow    \+>

%%


admit:                 {
                       DaemonOnly;

                       BEGIN IDENT;
                       SetAction(admit);
                       return ADMIT;
                       }


grant:                 {
                       DaemonOnly;

                       BEGIN IDENT;
                       SetAction(admit);
                       return ADMIT;
                       }

deny:                  {
                       DaemonOnly;

                       BEGIN IDENT;
                       SetAction(deny);
                       return DENY;
                       }

control:               {
                       BEGIN IDENT;
                       SetAction(control);
                       return CONTROL;
                       }

strategies:            {
                       BEGIN IDENT;
                       SetAction(strategies);
                       return STRATEGIES;
                       }

groups:                {
                       BEGIN IDENT;
                       SetAction(groups);
                       return GROUPS;
                       }

classes:               {                       /* Synonym for groups */
                       BEGIN IDENT;
                       SetAction(groups);
                       return GROUPS;
                       }

acl:                   {
                       CfengineOnly;

                       BEGIN IDENT;
                       SetAction(acls);
                       return ACL;
                       }

filters:               {
                       CfengineOnly;

                       BEGIN INITIAL;
                       SetAction(filters);
                       return FILTERS;
                       }

{actiont}              {
                       CfengineOnly;

                       BEGIN INITIAL;
                       SetAction(ActionStringToCode(yytext));
                       return ACTIONTYPE;
                       }

import:                {
                       BEGIN INITIAL;
                       SetAction(ActionStringToCode(yytext));
                       return ACTIONTYPE;
                       }

editfiles:             {
                       CfengineOnly;

                       BEGIN INITIAL;
                       SetAction(editfiles);
                       return EDITFILES;
                       }

{newline}              {
                       LINENUMBER++;
                       }

{space}                {
                       }

{linedir}              {
                       ResetLine((char *)yytext+6);
                       } 

{comment}              {
                       }


{lforcearrow}          {
                       strcpy(LINKFROM,CURRENTPATH);
                       ACTION_IS_LINKCHILDREN = true;
                       FORCELINK='y';
                       return LARROW;
                       }


{forcearrow}           {
                       strcpy(LINKFROM,CURRENTPATH);
                       ACTION_IS_LINK = true;
                       FORCELINK='y';
                       return ARROW;
                       }

{arrow}                {
                       strcpy(LINKFROM,CURRENTPATH);
                       ACTION_IS_LINK = true;
                       FORCELINK='n';
                       return ARROW;
                       }


{larrow}               {
                       strcpy(LINKFROM,CURRENTPATH);
                       ACTION_IS_LINKCHILDREN = true;
                       FORCELINK='n';
                       return LARROW;
                       }


{equals}               {
                       return EQUALS;
                       }


{id}::                 {
                       Debug1("Class:: %s\n",yytext);   


                       yytext[strlen((char *)yytext)-2] = '\0';
                       HandleClass(yytext);
                       return PARSECLASS;
                       }


{classt}               {
                       Debug1("Class:: %s\n",yytext);   

                       yytext[strlen((char *)yytext)-2] = '\0';
                       HandleClass(yytext);
                       return PARSECLASS;
                       }

<IDENT>{id}            {
                       HandleId(yytext);
                       return(ID);
                       }


{lbrack}               {
                       Debug1("LEFTBRACK\n");
                       BEGIN INITIAL;
                       return LBRACK;
                       }

{lbrace}               {
                       bzero(FILTERNAME,bufsize); /* Reset */
                       bzero(STRATEGYNAME,bufsize); /* Reset */

                       Debug1("LBRACE\n");
                       return LBRACE;
                       }

{rbrack}               {
                       Debug1("RIGHTBRACK\n");
                       BEGIN IDENT;
                       return RBRACK;
                       }

{rbrace}               {
                       Debug1("RBRACE\n");

                       if (EDITGROUPLEVEL != 0)
                          {
                          yyerror("Editfiles Begin/End mismatch before or at line");
                          }

                       if (SEARCHREPLACELEVEL != 0)
                          {
                          yyerror("ReplaceAll/With mismatch before or at line");
                          }

                       if (FOREACHLEVEL != 0)
                          {
                          yyerror("ForEachLineIn/EndLoop mismatch before or at line");
                          }

                       if (MOUNT_FROM || MOUNT_ONTO)
                          {
                          yyerror("Missing rw/ro mode in mount syntax");
                          MOUNT_FROM = false;
                          MOUNT_ONTO = false;
                          }

                       return RBRACE;
                       }

{function}             {
                       HandleItem(yytext);
                       return ITEM;
                       }


<INITIAL>{item}        {
                       if (ACTION == shellcommands)
                          {
                          yyerror("Shell commands must be quoted strings");
                          }

                       if (ACTION == editfiles)
                          {
                          if (strcmp((char *)yytext,"EndGroup") == 0 || 
                              strcmp((char *)yytext,"GotoLastLine") == 0 || 
                              strcmp((char *)yytext,"AutoCreate") == 0  ||
                              strcmp((char *)yytext,"EndLoop") == 0  ||
                              strcmp((char *)yytext,"CatchAbort") == 0  ||
                              strcmp((char *)yytext,"EmptyEntireFilePlease") == 0)
                             {
                             HandleEdit(CURRENTPATH,yytext,NULL);
                             return ITEM;
                             }
                          }

                       if (ACTION != control && strcmp((char *)yytext,"home") == 0)
                          {
                          if (DEBUG || D1)
                             {
                             printf("SPECIAL VARIABLE home\n");
                             }
                          HandlePath(yytext);
                          return PATH;
                          }
                       else if (ACTION == tidy)
                          {
                          if (yytext[0] == '\\')       /* ignore \ in \# */
                             {
                             HandleWildcard(yytext+1);
                             }
                          else
                             {
                             HandleWildcard(yytext);
                             }
                          return WILDCARD;
                          }
                       else
                          {
                          HandleItem(yytext);
                          return ITEM;
                          }
                       }


<INITIAL>{path}        {
                       if (ACTION == shellcommands)
                          {
                          yyerror("Shell commands must be quoted strings");
                          }

                       if (strncmp("home",(char *)yytext,4)==0)
                          {
                          HandlePath(yytext);
                          return PATH;
                          }

                       if (ACTION == control)
                          {
                          HandleItem(yytext);
                          return ITEM;
                          }

                       if (ACTION != mountables  && ACTION != mailserver && ACTION != filters && ACTION != misc_mounts &&  ACTION != unmounta && !IsAbsoluteFileName(yytext))
                          {
                          if (CONTROLVAR == cfhomepat)
                             {
                             HandleWildcard(yytext);
                             return WILDCARD;
                             }

                          if (ACTION_IS_LINK || ACTION_IS_LINKCHILDREN)
                             {
                             HandlePath(yytext);
                             return PATH;
                             }

                          sprintf(ERROR,"%s is not an absolute pathname",yytext);
                          yyerror(ERROR);
                          }

                       HandlePath(yytext);
                       return PATH;
                       }

{qstring}              {
                       if (ACTION == editfiles)
                          {
                          *(yytext+strlen((char *)yytext)-1) = '\0';
                          HandleEdit(CURRENTPATH,CURRENTITEM,yytext+1);
                          return QSTRING;
                          }
                       else if (ACTION == filters)
                          {
                          *(yytext+strlen((char *)yytext)-1) = '\0';
                          HandleWildcard(yytext+1);
                          return WILDCARD;
                          }
                       else if (ACTION == groups)
                          {
                          HandleItem(yytext);
                          return ITEM;
                          }
                       else if (ACTION == control)
                          {
                          *(yytext+strlen((char *)yytext)-1) = '\0';
                          HandleItem(yytext+1);
                          Debug1("Quoted macro string %s\n",yytext+1);
                          return QSTRING;
                          }
                       else
                          {
                          *(yytext+strlen((char *)yytext)-1) = '\0';
 
                          if (yytext[1] == '/')
                             {
                             HandlePath(yytext+1);
                             }
                          else
                             {
                             HandleItem(yytext+1);
                             }
                          return QSTRING;
                          }
                       }


{varpath}              {
                       IsVarString(yytext);        /* Syntax check */
                       HandleVarpath(yytext);
                       return VARPATH;
                       }

{varitem}              {
                       if (ACTION_IS_LINK || ACTION_IS_LINKCHILDREN)
                          {
                          if (strlen(LINKTO) == 0)
                             {
                             HandlePath(yytext);
                             return PATH;
                             }
                          }

                       HandleWildcard(yytext);

                       if (IsVarString(yytext))
                          {
                          return VARITEM;
                          }

                       return WILDCARD;
                       }


{wildcard}             {
                       Debug1("LEX: wildcard, I think I lov you...but i wanna know for sure\n");

                       if (ACTION_IS_LINK || ACTION_IS_LINKCHILDREN)
                          {
                          if (strlen(LINKTO) == 0)
                             {
                             HandlePath(yytext);
                             return PATH;
                             }
                          }

                       if (IsVarString(yytext))
                          {
                          HandleWildcard(yytext);
                          return VARITEM;
                          }

                       HandleWildcard(yytext);
                       return WILDCARD;
                       }




.                      {
                       return yytext[0];
                       }

%%

/* EOF */
