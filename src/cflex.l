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

#include "cfparse.h"
#include "cf.defs.h"
#include "cf.extern.h"


  /* Note the %o %a declarations to allow more internal workspace */

%}

%option noyywrap
%s INCONTROL BRACELIST INASSIGN CFSERVD
%o 30000
%a 16000
%p 12000
%n 2000

actiont   copy:|resolve:|files:|tidy:|homeservers:|binservers:|mailserver:|disks:|mountables:|links:|shellcommands:|disable:|directories:|ignore:|interfaces:|defaultroute:|miscmounts:|unmount:|processes:|required:|broadcast:|alerts:|packages:|methods:|rename:

space     [ \t]+

comment   #.*

linedir   #line[ \t]+[0-9]+.*
function  [a-zA-Z0-9_]+\([^)]*\)

qstring  \"((\\\")|[^"])*\"|\'((\\\')|[^'])*\'|`[^`]*`


item      [+]?[-.,!@a-zA-Z0-9_\200-\377]+

id        [a-zA-Z0-9_\200-\377]+

lvalue    [a-zA-Z0-9_\200-\377]+

classt    [.|!,;a-zA-Z0-9_()\200-\377]+::

option    [a-z0-9]+=[^ \t\n]+

varobj    [^ \t\n]+

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
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN CFSERVD;
                       SetAction(admit);
                       return ADMIT;
                       }


grant:                 {
                       DaemonOnly;
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN CFSERVD;
                       SetAction(admit);
                       return ADMIT;
                       }

deny:                  {
                       DaemonOnly;
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN CFSERVD;
                       SetAction(deny);
                       return DENY;
                       }

control:               {
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN INCONTROL;
                       SetAction(control);
                       return CONTROL;
                       }


strategies:            {
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN BRACELIST;
                       SetAction(strategies);
                       return STRATEGIES;
                       }

groups:                {
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN INCONTROL;
                       SetAction(groups);
                       return GROUPS;
                       }

classes:               {                                           /* Synonym for groups */
                       InstallPending(ACTION);             /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN INCONTROL;
                       SetAction(groups);
                       return GROUPS;
                       }

acl:                   {
                       CfengineOnly;

                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN BRACELIST;
                       SetAction(acls);
                       return ACL;
                       }

filters:               {
                       CfengineOnly;

                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN BRACELIST;
                       SetAction(filters);
                       return FILTERS;
                       }

{actiont}              {
                       CfengineOnly;

                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN INITIAL;
                       SetAction(ActionStringToCode(yytext));
                       return ACTIONTYPE;
                       }

import:                {
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN INITIAL;
                       SetAction(ActionStringToCode(yytext));
                       return ACTIONTYPE;
                       }

editfiles:             {
                       CfengineOnly;

                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       BEGIN BRACELIST;
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
                       Debug1("LFARROW %s\n",yytext);
                       strcpy(LINKFROM,CURRENTOBJECT);
                       ACTION_IS_LINKCHILDREN = true;
                       FORCELINK='y';
                       return LARROW;
                       }


{forcearrow}           {
                       Debug1("FARROW %s\n",yytext);
                       strcpy(LINKFROM,CURRENTOBJECT);
                       ACTION_IS_LINK = true;
                       FORCELINK='y';
                       return ARROW;
                       }

{arrow}                {
                       Debug1("ARROW %s\n",yytext);
                       strcpy(LINKFROM,CURRENTOBJECT);
                       ACTION_IS_LINK = true;
                       FORCELINK='n';
                       return ARROW;
                       }


{larrow}               {
                       Debug1("LARROW %s\n",yytext);
                       strcpy(LINKFROM,CURRENTOBJECT);
                       ACTION_IS_LINKCHILDREN = true;
                       FORCELINK='n';
                       return LARROW;
                       }


{equals}               {
                       Debug1("EQUALS %s\n",yytext);
                       return EQUALS;
                       }


{classt}               {
                       Debug1("\nNEW CLASS PREDICATE:: %s\n",yytext);

                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       yytext[strlen((char *)yytext)-2] = '\0';
                       HandleClass(yytext);
                       return PARSECLASS;
                       }

<INCONTROL>{lvalue}    {
                       Debug1("LVALUE %s\n",yytext);
                       HandleLValue(yytext);
                       return LVALUE;
                       }

<BRACELIST>{id}:       {
                       Debug1("BRACELIST-CLASSIFIER %s\n",yytext);
                       HandleBraceObjectClassifier(yytext);
                       return BCLASS;
                       }

<BRACELIST>{id}        {
                       Debug1("BRACELIST-ID %s\n",yytext);

                       if (ACTION == editfiles)   /* Syntax different for editfiles :(  */
                          {
                          HandleBraceObjectClassifier(yytext);
                          }
                       else
                          {
                          HandleBraceObjectID(yytext);
                          }
                       return ID;
                       }


{lbrack}               {
                       Debug1("LEFTBRACK\n");
                       BEGIN INASSIGN;
                       return LBRACK;
                       }

{lbrace}               {
                       bzero(FILTERNAME,bufsize); /* Reset */
                       bzero(STRATEGYNAME,bufsize); /* Reset */

                       Debug1("LBRACE\n");
                       return LBRACE;
                       }

{rbrack}               {
                       Debug1("RIGHTBRACK\n\n");
                       BEGIN INCONTROL;
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

                       InstallPending(ACTION);
                       InitializeAction();

                       return RBRACE;
                       }



<INASSIGN>{qstring}    {
                       char *sp = NULL;
                       int less = 0;
                       Debug1("RVAL-QSTRING %s\n",yytext);

                       if ((sp = malloc(strlen(yytext)+1)) == NULL)
                          {
                          FatalError("Malloc failure in parsing");
                          }

                       if((less = RemoveEscapeSequences(yytext,sp)) > 0)
                       {
                       Debug1("QSTRING-LESS %d\n",less);
                       yyless(less);
                       }

                       switch (ACTION)
                          {
                          case control:	
                             InstallControlRValue(CURRENTITEM,sp);
                             free(sp);
                             return RVALUE;
                          case groups:
                             HandleGroupRValue(sp);
                             free(sp);
                             return RVALUE;
                          }
                       }


{qstring}              {
                       char *sp = NULL;
                       int less = 0;
                       Debug1("QSTRING %s\n",yytext);

                       if ((sp = malloc(strlen(yytext)+1)) == NULL)
                          {
                          FatalError("Malloc failure in parsing");
                          }

                       if((less = RemoveEscapeSequences(yytext,sp)) > 0)
                       {
                       Debug1("QSTRING-LESS %d\n",less);
                       yyless(less);
                       }

                       HandleQuotedString(sp);
                       free(sp);
                       return QSTRING;
                       }

<INASSIGN>{function}   {
                       Debug1("RVAL-FUNCTION %s\n",yytext);

                       HandleFunctionObject(yytext);
                       return FUNCTION;
                       }


{function}             {
                       InstallPending(ACTION);           /* Flush any existing actions */
                       InitializeAction();                 /* Clear data for files/dirs  */

                       Debug1("FUNCTION %s\n",yytext);

                       if (ACTION == alerts)
                          {
                          ACTIONPENDING = true;
                          }

                       HandleFunctionObject(yytext);
                       return FUNCTION;
                       }

{option}               {
                       Debug1("\nOPTION %s\n",yytext);
                       HandleOption(yytext);
                       return OPTION;
                       }

<INITIAL>{varobj}      {
                       if ((ACTION_IS_LINK || ACTION_IS_LINKCHILDREN) && (strlen(LINKTO)==0))
                          {
                          Debug("Pending link destination...\n");
                          }
                       else
                          {
                          /* Check anomalour process syntax */
                          if (ACTION == processes && 
                              (strcmp(ToLowerStr(yytext),"restart") == 0 || HAVE_RESTART || 
                               strcmp(ToLowerStr(yytext),"setoptionstring") == 0))
                             {
                             }
                          else if (ACTIONPENDING)
                             {
                             InstallPending(ACTION);             /* Flush any existing actions */
                             InitializeAction();                 /* Clear data for files/dirs  */
                             }
                          }

                       Debug1("VAROBJ %s\n",yytext);

                       IsVarString(yytext);        /* Syntax check */
                       HandleVarObject(yytext);
                       return VAROBJ;
                       }

<INASSIGN>{varobj}     {
                       Debug1("RVAL-VAROBJ %s\n",yytext);

                       switch(ACTION)
                          {
                          case control:
                             InstallControlRValue(CURRENTITEM,yytext);
                             return RVALUE;
                          case groups:
                             HandleGroupRValue(yytext);
                             return RVALUE;
                          default:
                             yyerror("Misplaced assignment");
                          }
                       }

<BRACELIST>{varobj}    {
                       Debug1("BRACELIST-VAROBJ %s\n",yytext);

                       if (ACTION == acls && strlen(CURRENTOBJECT) > 0) /* Irregular acl syntax */
                          {
                          HandleBraceObjectClassifier(yytext);
                          }
                       else
                          {
                          HandleBraceObjectID(yytext);
                          }
                       return VAROBJ;
                       }

<CFSERVD>{varobj}      {
                       Debug1("\nCFSERVD-VAROBJ %s\n",yytext);

                       HandleServerRule(yytext);
                       return ACCESSOBJ;
                       }

.                      {
	               printf("Stray character (%s)",yytext);
                       return yytext[0];
                       }

%%

/* EOF */
