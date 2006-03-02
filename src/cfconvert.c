/* 
        Copyright (C) 1994-
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

/*****************************************************************************/
/*                                                                           */
/* File: cfconvert.c                                                         */
/*                                                                           */
/* Created: Tue Feb 28 09:57:35 2006                                         */
/*                                                                           */
/* Author:                                           >                       */
/*                                                                           */
/* Revision: $Id$                                                            */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

struct option CONVOPTIONS[4] =
      {
      { "help",no_argument,0,'h' },
      { "debug",optional_argument,0,'d' },
      { "file",required_argument,0,'f' },
      { NULL,0,0,0 }
      };

/*******************************************************************/
/* Functions internal to cfengine.c                                */
/*******************************************************************/

int main (int argc,char *argv[]);
void Initialize (int argc, char **argv);
void CheckSystemVariables (void);
void CheckOpts (int argc, char **argv);
void Syntax (void);
int  NothingLeftToDo (void);
void SummarizeObjects (void);
void EchoValues (void);

void ConvShellCommands(void);
void ConvControl(void);

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main(int argc,char *argv[])

{ struct Item *ip;
    
Initialize(argc,argv); 

printf("--------------------------------------------------\n\n");
printf("Ignore this program - it is for developer use only\n\n");
printf("--------------------------------------------------\n\n");

ParseInputFile(VINPUTFILE);

if (ERRORCOUNT > 0)
   {
   printf("Broken input file - debug first with cfagent\n");
   exit(1);
   }
 
CheckSystemVariables();
EchoValues();


closelog();
return 0;
}

/*******************************************************************/
/* Level 1                                                         */
/*******************************************************************/
 
void Initialize(int argc,char *argv[])

{ char *sp, *cfargv[CF_MAXARGS];
  int i, cfargc, seed;
  struct stat statbuf;
  unsigned char s[16];
  char ebuff[CF_EXPANDSIZE];
  
strcpy(VDOMAIN,CF_START_DOMAIN);

PreLockState();

VFACULTY[0] = '\0';
VSYSADM[0] = '\0';
VNETMASK[0]= '\0';
VBROADCAST[0] = '\0';
VMAILSERVER[0] = '\0';
ALLCLASSBUFFER[0] = '\0';
VREPOSITORY = strdup("\0");
 
#ifndef HAVE_REGCOMP
re_syntax_options |= RE_INTERVALS;
#endif

CheckOpts(argc,argv);
}

/*******************************************************************/

void EchoValues()

{ struct Item *ip,*p2;
  char ebuff[CF_EXPANDSIZE];

  ebuff[0] = '\0';
  
if (strcmp(VINPUTFILE,"cfagent.conf") == 0)
   {
   printf("method cfagent\n{\n");
   }
else
   {
   printf("method %s\n{\n",ReadLastNode(VINPUTFILE));
   }


printf("control:\n\n");
printf("vars:\n\n");
ConvControl();
ConvShellCommands();

printf("\n}\n");
}

/*******************************************************************/

void CheckSystemVariables()

{ char id[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];
  int time, hash, activecfs, locks;

if (ERRORCOUNT > 0)
   {
   FatalError("Execution terminated after parsing due to errors in program");
   } 
}


/*******************************************************************/

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
  struct Item *actionList;
  int optindex = 0;
  int c;

  
while ((c=getopt_long(argc,argv,"hd:f:",CONVOPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'f':
          strncpy(VINPUTFILE,optarg, CF_BUFSIZE-1);
          VINPUTFILE[CF_BUFSIZE-1] = '\0';
          MINUSF = true;
          break;
          
      case 'd': 
          AddClassToHeap("opt_debug");
          switch ((optarg==NULL) ? '3' : *optarg)
             {
             case '1': D1 = true;
                 break;
             case '2': D2 = true;
                 break;
             case '3': D3 = true;
                 VERBOSE = true;
                 break;
             case '4': D4 = true;
                 break;
             default:  DEBUG = true;
                 break;
             }
          break;
                    
      case 'h': Syntax();
          exit(0);
          
          
      default:  Syntax();
          exit(1);
          
      }
  }
}


/*******************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine: A system configuration engine (cfagent)\n%s\n%s\n",VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; CONVOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",CONVOPTIONS[i].name,(char)CONVOPTIONS[i].val);
   }

printf("\nDebug levels: 1=parsing, 2=running, 3=summary, 4=expression eval\n");

printf("\nBug reports to bug-cfengine@gnu.org (News: gnu.cfengine.bug)\n");
printf("General help to help-cfengine@gnu.org (News: gnu.cfengine.help)\n");
printf("Info & fixes at http://www.cfengine.org\n");
}


/*******************************************************************/

void ConvControl()

{
}

/*******************************************************************/

void ConvShellCommands()

{ struct ShellComm *ptr;
 
printf("commands:\n\n");

for (ptr = VSCRIPT; ptr != NULL; ptr=ptr->next)
   {
   printf("  %s::\n\n",VSCRIPT->classes);
   printf("\n   %s\n various promises ...\n",ptr->name);
   printf("\ntimeout=%d\n uid=%d,gid=%d\n",ptr->timeout,ptr->uid,ptr->gid);
   printf(" umask = %o, background = %c\n",ptr->umask,ptr->fork);
   printf (" ChDir=%s, ChRoot=%s\n",ptr->chdir,ptr->chroot);
   printf(" IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
   
   if (ptr->defines)
      {
      printf(" Define %s\n",ptr->defines);
      }

   if (ptr->elsedef)
      {
      printf(" ElseDefine %s\n",ptr->elsedef);
      }

   }
 
}





/* EOF */
