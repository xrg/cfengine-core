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
/*  Parse Zone for cfengine                                        */
/*                                                                 */
/*  This is wide screen entertainment. Resize                      */
/*  your window before viewing!                                    */
/*                                                                 */
/*  The routines here are called by the lexer and the yacc parser  */
/*                                                                 */
/*******************************************************************/

 /*
    The parser uses the classification of strings into id's items
    paths, varpaths etc. Each complete action is gradually assembled
    by setting flags based on what action, class etc is being
    parsed. e.g. HAVEPATH, FROM_LINK etc.. these are typed as
    "flag" in the variables file. When all the necessary critera
    are met, or the beginning of a new definition is found
    the action gets completed and installed in the actions lists.

  */

#define INET

#include <stdio.h>
#include "cf.defs.h"
#include "cf.extern.h"

extern FILE *yyin;

/*******************************************************************/

int ParseBootFiles()

{ char filename[bufsize], *sp;
  static char *bootf = "update.conf"; 
  struct stat s;

if (MINUSF)
   {
   Verbose("Reading special file - ignoring %s",bootf);
   return true;
   }
  
filename[0] = '\0';
PARSING = true;

NewParser();

if ((sp=getenv(CFINPUTSVAR)) != NULL)
   {
   if (! LastFileSeparator(bootf))     /* Don't prepend to absolute names */
      { 
      strcpy(filename,sp);
      if (! IsAbsoluteFileName(filename))
	 {
	 Verbose("CFINPUTS was not an absolute path, overriding with %s\n",WORKDIR);
	 snprintf(filename,bufsize,"%s/inputs",WORKDIR);
	 }
      
      AddSlash(filename);
      }
   }
else
   {
   if (! LastFileSeparator(bootf))     /* Don't prepend to absolute names */
      { 
      strcpy(filename,WORKDIR);
      AddSlash(filename);
      strcat(filename,"inputs/");
      }
   }

strcat(filename,bootf);

Verbose("Looking for a bootstrap file %s\n",filename);

if (stat(filename,&s) == -1)
   {
   Verbose("(No bootstrap file)\n");
   DeleteParser();
   return false;
   }
 
ParseFile(filename,sp);
   
PARSING = false;

DeleteParser();
 
Debug("(END OF PARSING)\n");
Verbose("Finished with %s\n",bootf); 

return true; 
}

/*******************************************************************/

void ParseInputFiles()

{ struct Item *ptr;
  char filename[bufsize], *sp;
  void TimeOut();

PARSING = true;

NewParser(); 
  
if ((strcmp(VINPUTFILE,".") == 0)||(strcmp(VINPUTFILE,"-") == 0)) /* read from standard input */
   {
   yyin = stdin;

   if (!feof(yyin))
      {
      yyparse();
      }

   InstallPending(ACTION);
   }
else
   {
   filename[0] = '\0';

   if ((sp=getenv(CFINPUTSVAR)) != NULL)
      {
      if (! LastFileSeparator(VINPUTFILE))     /* Don't prepend to absolute names */
	 { 
	 strcpy(filename,sp);

	 if (! IsAbsoluteFileName(filename))
	    {
	    Verbose("CFINPUTS was not an absolute path, overriding with %s\n",WORKDIR);
	    snprintf(filename,bufsize,"%s/inputs",WORKDIR);
	    }
	 AddSlash(filename);
	 }
      }
   else
      {
      if (! LastFileSeparator(VINPUTFILE))     /* Don't prepend to absolute names */
	 { 
	 strcpy(filename,WORKDIR);              /* Used to set default configdir at configure */
	 AddSlash(filename);
	 strcat(filename,"inputs/");
	 }
      }

   strcat(filename,VINPUTFILE);

   ParseFile(filename,sp);
   }
   
for (ptr = VIMPORT; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   Verbose("Import file %s\n",ptr->name);
   
   filename[0] = '\0';

   if ((sp=getenv(CFINPUTSVAR)) != NULL)
      {
      if (! LastFileSeparator(ptr->name))   
         { 
         strcpy(filename,sp);	
	 AddSlash(filename);
         }
      }
   else
      {
      if (! LastFileSeparator(VINPUTFILE))     /* Don't prepend to absolute names */
	 { 
	 strcpy(filename,WORKDIR);              /* Used to set default configdir at configure */
	 AddSlash(filename);
	 strcat(filename,"inputs/");
	 }
      }

   if (LastFileSeparator(ptr->name)) 
      { 
      strcpy(filename,ptr->name);
      } 
   else 
      {  
      strcat(filename,ptr->name);
      }
   ParseFile(filename,sp);
   }

VCURRENTFILE[0]='\0';       /* Zero filename for subsequent errors */
PARSING = false;

DeleteParser();
 
Debug("(END OF PARSING)\n");
}

/*******************************************************************/

void NewParser()

{
 Debug("New Parser Object::");
 VUIDNAME = (char *) malloc(bufsize);
 VGIDNAME = (char *) malloc(bufsize);
 FILTERNAME = (char *) malloc(bufsize);
 STRATEGYNAME = (char *) malloc(bufsize);
 CURRENTITEM = (char *) malloc(bufsize);
 GROUPBUFF = (char *) malloc(bufsize);
 ACTIONBUFF = (char *) malloc(bufsize);
 CURRENTPATH = (char *) malloc(bufsize);
 CURRENTAUTHPATH = (char *) malloc(bufsize);
 CLASSBUFF = (char *) malloc(bufsize);
 LINKFROM = (char *) malloc(bufsize);
 LINKTO = (char *) malloc(bufsize);
 ERROR = (char *) malloc(bufsize);
 EXPR = (char *) malloc(bufsize);
 MOUNTFROM = (char *) malloc(bufsize);
 MOUNTONTO = (char *) malloc(bufsize);
 *MOUNTFROM = '\0';
 *MOUNTONTO = '\0';
 MOUNTOPTS = (char *) malloc(bufsize);

 DESTINATION = (char *) malloc(bufsize);

 IMAGEACTION = (char *) malloc(bufsize);

 CHDIR = (char *) malloc(bufsize);
 RESTART = (char *) malloc(bufsize);
 LOCALREPOS = (char *) malloc(bufsize);
 FILTERDATA = (char *) malloc(bufsize);
 STRATEGYDATA = (char *) malloc(bufsize);
}

/*******************************************************************/

void DeleteParser()

{ Debug("Delete Parser Object::");

free(VUIDNAME);
free(VGIDNAME);
free(FILTERNAME);
free(STRATEGYNAME);
free(CURRENTITEM);
free(GROUPBUFF);
free(ACTIONBUFF);
free(CURRENTPATH);
free(CURRENTAUTHPATH);
free(CLASSBUFF);
free(LINKFROM);
free(LINKTO);
free(ERROR);
free(EXPR);
free(RESTART); 
free(MOUNTFROM);
free(MOUNTONTO);
free(MOUNTOPTS);
free(DESTINATION);
free(IMAGEACTION);
free(CHDIR);
free(LOCALREPOS);
free(FILTERDATA);
free(STRATEGYDATA);
}

/*******************************************************************/

void SetAction (action)

enum actions action;

{
InstallPending(ACTION);   /* Flush any existing actions */

Debug1("\nBEGIN NEW ACTION %s\n",ACTIONTEXT[action]);

ACTION = action;
strcpy(ACTIONBUFF,ACTIONTEXT[action]);

switch (ACTION)
   {
   case interfaces:
   case files:
   case makepath:
   case tidy:
   case disable:
   case filters:
   case strategies:		   
   case image:
   case links:
   case required:
   case disks:
   case shellcommands:
   case alerts:
   case unmounta:
   case processes:  InitializeAction();
   }

*CURRENTAUTHPATH = '\0';

strcpy(CLASSBUFF,CF_ANYCLASS);    /* default class */
}

/*******************************************************************/

void HandleId(id)

char *id;

{
Debug1("HandleId(%s) in action %s\n",id,ACTIONTEXT[ACTION]);

if (ACTION == control)
   {
   if ((CONTROLVAR = ScanVariable(id)) != nonexistentvar)
      {
      strcpy(CURRENTITEM,id);
      return;
      }
   else
      {
      if (IsDefinedClass(CLASSBUFF))
	 {
	 RecordMacroId(id);
	 }
      return;
      }
   }

if (ACTION == groups)
   { int count = 1;
     char *cid = id;

   while (*cid != '\0')
      {
      if (*cid++ == '.')
         {
         count++;
         }
      }

   if (strcmp(id,CF_ANYCLASS) == 0)
      {
      yyerror("Reserved class <any>");
      }

   if (count > 1)                              /* compound class */
      {
      yyerror("Group with compound identifier");
      FatalError("Dots [.] not allowed in group identifiers");
      }

   if (IsHardClass(id))
      {
      yyerror("Reserved class name (choose a different name)");
      }

   strcpy(GROUPBUFF,id);
   }


switch(ACTION)   /* Check for IP names in cfd */
   {
   case admit:
                 FuzzyMatchParse(id);       
                 InstallAuthItem(CURRENTAUTHPATH,id,&VADMIT,&VADMITTOP,CLASSBUFF);
		 break;
   case deny:
                 FuzzyMatchParse(id);       
                 InstallAuthItem(CURRENTAUTHPATH,id,&VDENY,&VDENYTOP,CLASSBUFF);
		 break;

   case acls:
		 strcpy(CURRENTITEM,id);
                 InstallACL(id,CLASSBUFF);
		 break;

   case strategies:
		 if (strlen(STRATEGYNAME) == 0)
		    {
		    strcpy(STRATEGYNAME,id);
		    InstallStrategy(id,CLASSBUFF);
		    }
                 else
		    {
		    Debug("Found class %s in strategy %s\n",id,STRATEGYNAME);
		    strcpy(CURRENTITEM,id);
		    }
		 ACTIONPENDING = true;
		 break;
   }

strcpy(CLASSBUFF,CF_ANYCLASS);    /* default class */
Debug1("Done with HandleId()\n"); 
}

/*******************************************************************/

void HandleClass (id)

char *id;

{ int members;

InstallPending(ACTION);

Debug1("HandleClass(%s)\n",id);

if ((members = CompoundId(id)) > 1)             /* Parse compound id */
   {
   Debug1("Compound class = (%s) with %d members\n",id,members);
   }
else
   {
   Debug1("Simple identifier or class = (%s)\n",id);
   CONTROLVAR = ScanVariable(id);
   }
}

/*******************************************************************/

void HandleItem (item)

char *item;

{ char local[bufsize];
 
Debug1("HandleItem(%s)\n",item);

if (strcmp(item,"+") == 0 || strcmp(item,"-") == 0)
   {
   yyerror("+/- not bound to identifier");
   }

if (IsBuiltinFunction(item))
   {
   local[0] = '\0';
   strcpy(local,EvaluateFunction(item,local));
   switch (ACTION)
      {
      case groups: HandleGroupItem(local,simple);
	  break;
      case control: InstallLocalInfo(local);
	  break;
      case alerts:
	  InstallPending(ACTION);
	  AppendItem(&VALERTS,item,CLASSBUFF);
	  break;
      default: yyerror("Builtin function outside control, classes or alerts");
      }
   }
else if (item[0] == '+')                               /* Lookup in NIS */
   {
   item++;

   if (item[0] == '@')                               /* Irrelevant */
      {
      item++;
      }

   Debug1("Netgroup item, lookup NIS group (%s)\n",item);

   strcpy(CURRENTITEM,item);

   if (ACTION == groups)
      {
      HandleGroupItem(item,netgroup);
      }
   else
      {
      yyerror("Netgroup reference outside group: action or illegal octal number");
      FatalError("Netgroups may only define internal groups or illegal octal file action.");
      }
   }
else if ((item[0] == '-') && (ACTION != processes))
   {
   item++;

   if (item[0] == '@')                               /* Irrelevant */
      {
      item++;

      if (ACTION == groups)
         {
         HandleGroupItem(item,groupdeletion);
         }
      else
         {
         yyerror("Netgroup reference outside group: action or illegal octal number");
         FatalError("Netgroups may only define internal groups or illegal octal file action.");
         }
      }
   else
      {
      if (ACTION == groups)
         {
         HandleGroupItem(item,deletion);
         }
      else
	 {
	 if (ACTION != processes)
	    {
	    yyerror("Illegal deletion sign or octal number?");
	    FatalError("The deletion operator may only be in the groups: action");
	    }
         }
      }


   }
else if (item[0] == '\"' || item[0] == '\'' || item[0] == '`') 
   {
   *(item+strlen(item)-1) = '\0';

   if (ACTION == groups)                     /* This test should be redundant */
      {
      HandleGroupItem(item,classscript);
      }
   }
else
   {
   Debug1("simple item = (%s)\n",item);
  
   /* CONTROLVAR set by CompoundId via HandleId */

   switch(ACTION)
      {
      case control:     InstallLocalInfo(item);
			if (CONTROLVAR == cfautodef)
			   {
			   PrependItem(&VAUTODEFINE,item,CLASSBUFF);
			   AddInstallable(CLASSBUFF);
			   }
                        break;
      case groups:      HandleGroupItem(item,simple);
                        break;
      case resolve:     AppendNameServer(item);
                        break;
      case image:
      case files:       HandleFileItem(item);
                        break;
      case tidy:        strcpy(CURRENTITEM,item);
                        break;
      case homeservers: InstallHomeserverItem(item);
                        break;
      case binservers:  InstallBinserverItem(item);
                        break;
      case mailserver:  yyerror("Give whole rpc path to mailserver");
                        break;
      case disks:
      case required:    yyerror("Required filesystem must be an absolute path");
                        FatalError("Fatal error");
                        break;
      case mountables:  
	                InstallPending(ACTION);
			strcpy(CURRENTPATH,item);
			ACTIONPENDING = true;
			break;

      case links:       if (ACTION_IS_LINKCHILDREN && strcmp (item,"linkchildren") == 0)
                           {
                           strcpy(LINKTO,item);
                           }
                        else
                           {
                           yyerror("Links requires path or varitem");
                           }
                        break;
      case import:      AppendImport(item);
                        break;
      case shellcommands:
	                InstallPending(ACTION);
			strcpy(CURRENTPATH,item);
			ACTIONPENDING = true;
                        break;
      case makepath:    yyerror("makepath needs an abolute pathname");
                        FatalError("Fatal Error");
      case disable:     yyerror("disable needs an absolute path name");
                        FatalError("Fatal Error");
      case broadcast:   InstallBroadcastItem(item);
                        break;
			
      case interfaces:  if (strlen(VIFNAME)==0)
	                   {			   
			   strcpy(VIFNAME,item);
			   }
                        else
			   {
			   InstallPending(ACTION);
			   strcpy(VIFNAME,item);
			   }
	                break;
			
      case defaultroute:InstallDefaultRouteItem(item);
                        break;
      case misc_mounts: if (MOUNT_FROM && MOUNT_ONTO && ((strcmp(item,"rw") == 0 || strcmp(item,"ro") == 0)))
                           {
			   Debug1("Miscmount mode found, old style\n");
			   
			   if (strcmp(item,"rw") == 0)
			      {
			      MOUNTMODE='w';
			      break;
			      }
			   
			   if (strcmp(item,"ro") == 0)
			      {
			      MOUNTMODE='o';
			      break;
			      }
			   }
      
                        yyerror("miscmounts: host:/frompath /mounton_path ro|rw\n");
                        break;

      case unmounta:    yyerror("Umount must be in the format machine:directory");
                        break;

      case editfiles:   /* action recorded in CURRENTITEM, installed from qstring in lexer */
	                if (isdigit((int)*item) || strcmp(item,"inf") == 0)
			   {
			   HandleEdit(CURRENTPATH,CURRENTITEM,item);
			   break;
			   }
			
                        strcpy(CURRENTITEM,item);
                        break;

      case ignore:      AppendIgnore(item);
                        break;

      case processes:   if (strcmp(item,"restart") == 0)
	                   {
			   HAVE_RESTART = true;
			   ACTIONPENDING = false;
			   return;
			   }

                        if (ACTIONPENDING)
			   {
			   InstallPending(ACTION);          /* Flush any existing actions */
                           InitializeAction();
			   }
      
                        if (strcmp(item,"SetOptionString") == 0)
			   {
			   Debug("Found SetOptionString\n");
                           if (EXPR[0] == '\0')
			      {
   			      strcpy(EXPR,item);
			      ACTIONPENDING = false;
			      HAVE_RESTART = true;    /* save option string in restart */
			      return;
			      }
			   else
			      {
			      yyerror("Inappropriate placement of SetOptionString");
			      return;
			      }
			   }

	                if (EXPR[0] == '\0')
	                   {
			   if (HAVE_RESTART)
			      {
			      yyerror("Missing search expression");
			      }
			   strcpy(EXPR,item);
			   ACTIONPENDING = true;
			   }
                        else
			   {
			   if (HAVE_RESTART)
			      {
			      strcpy(RESTART,item);
			      HAVE_RESTART= false;
			      ACTIONPENDING = true;
			      }
			   }
                        break;


      case filters:
	                if (strlen(FILTERNAME)==0)
			   {
			   strcpy(FILTERNAME,item);
			   InstallFilter(item);
			   }
			else
			   {
			   strcpy(FILTERDATA,item);
			   InstallPending(ACTION);
			   }
			break;

      case strategies:
	               if (strlen(STRATEGYNAME)==0)
			  {
			  yyerror("No strategy alias");
			  }
		       else
			  {
			  strcpy(STRATEGYDATA,item);
			  InstallPending(ACTION);
			  }
		       break;

      case alerts:
                 if (strcmp(CLASSBUFF,"any") == 0)
		    {
		    yyerror("Alert specified with no class membership");
		    }
		 else
		    {
		    strcpy(CURRENTITEM,item);
		    ACTIONPENDING=true;
		    }
		 
   	         break;

      default:          yyerror("Unknown item or out of context");
      }
   }
}

/***************************************************************************/

void HandlePath (path)

char *path;

{
if (ACTION == processes && ! ACTIONPENDING)
   {
   }
else
   {
   InstallPending(ACTION);           /* Flush any existing actions */
   InitializeAction();                 /* Clear data for files/dirs  */
   }

Debug1("path = (%s)\n",path);

strcpy(CURRENTPATH,path);                   /* Yes this must be here */

ACTIONPENDING = true;                     /* we're parsing an action */

if (ACTION_IS_LINK || ACTION_IS_LINKCHILDREN)  /* to-link (after ->) */
   {
   strcpy(LINKTO,CURRENTPATH);
   }
else
   {
   switch (ACTION)
      {
      case control:  if (CONTROLVAR == cfmountpat)
                        {
                        SetMountPath(path);
			break;
                        }

                     if (CONTROLVAR == cfrepos)
			{
			SetRepository(path);
			break;
			}

		     if (CONTROLVAR == cfrepchar)
			{
			if (strlen(path) > 1)
			   {
			   yyerror("reposchar can only be a single letter");
			   break;
			   }
			if (path[0] == '/')
			   {
			   yyerror("illegal value for reposchar");
			   break;
			   }
			REPOSCHAR = path[0];
			}

		     if (CONTROLVAR == cflistsep)
			{
			if (strlen(path) > 1)
			   {
			   yyerror("listseparator can only be a single letter");
			   break;
			   }
			if (path[0] == '/')
			   {
			   yyerror("illegal value for listseparator");
			   break;
			   }
			LISTSEPARATOR = path[0];
			}

                     if (CONTROLVAR == cfhomepat)
                        {
                        yyerror("Path relative to mountpath required");
                        FatalError("Absolute path was specified\n");
                        }

                    if (CONTROLVAR == nonexistentvar)
                       {
		       if (IsDefinedClass(CLASSBUFF))
			  {
			  AddMacroValue(CONTEXTID,CURRENTITEM,path);
			  }
                       }

                       break;
      case import:     AppendImport(path);
                       break;
      case links:      /* from link (see cf.l) */
                       break;
      case filters:    if (strlen(FILTERNAME) == 0)
	                  {
			  yyerror("empty or broken filter");
			  }
                       else
	                  {
			  strcpy(CURRENTITEM,path);
			  ACTIONPENDING = true;
			  }
	                break;
      case disks:
      case required:   strcpy(CURRENTPATH,path);
                       break;
      case shellcommands:
                       break;

      /* HvB : Bas van der Vlies */
      case mountables: 
		       break;

      case mailserver: InstallMailserverPath(path);
                       break;
      case tidy:       strcpy(CURRENTITEM,path);
                       break;
      case disable:    strcpy(CURRENTPATH,path);
                       ACTIONPENDING = true;
                       break;
      case makepath:   strcpy(CURRENTPATH,path);
                       break;
      case ignore:     AppendIgnore(path);
                       break;

      case misc_mounts:if (! MOUNT_FROM)
                          {
                          MOUNT_FROM = true;
                          strcpy(MOUNTFROM,CURRENTPATH);
                          }
                       else
                          {
                          if (MOUNT_ONTO)
			    {
                            yyerror ("Path not expected");
                            FatalError("miscmounts: syntax error");
                            }
                          MOUNT_ONTO = true;
                          strcpy(MOUNTONTO,CURRENTPATH);
                          }
                       break;

      case unmounta:   strcpy(CURRENTPATH,path);
                       break;
      case image:
      case files:      
                       break;

      case editfiles:  /* file recorded in CURRENTPATH */
                       break;

      case processes:   if (EXPR[0] == '\0')
	                   {
			   if (HAVE_RESTART)
			      {
			      yyerror("Missing search expression");
			      }
			   strcpy(EXPR,path);
			   }
                        else
			   {
			   if (HAVE_RESTART)
			      {
			      strcpy(RESTART,path);
			      HAVE_RESTART = false;
			      ACTIONPENDING = true;
			      }
			   }
		       break;

      default:         yyerror("Unknown command or name out of context");
      }
   }
}

/*******************************************************************/

void HandleVarpath(varpath)         

  /* Expand <binserver> and <fac> etc. Note that the right hand  */
  /* side of links etc. gets expanded at runtime. <binserver> is */
  /* only legal on the right hand side.                          */

char *varpath;

{
InstallPending(ACTION);   /* Flush any existing actions */
InitializeAction();

Debug1("HandleVarpath(%s)\n",varpath);

if (IsWildCard(varpath) && ! (ACTION == files || ACTION == tidy || ACTION == admit || ACTION == deny || ACTION == import))
   {
   yyerror("Wildcards cannot be used in this context (possibly missing space?)");
   }

strcpy(CURRENTPATH,varpath);

ACTIONPENDING = true;
 
if (ACTION_IS_LINK || ACTION_IS_LINKCHILDREN)
   {
   strcpy(LINKTO,varpath);
   }
else 
   {
   switch (ACTION)
      {
      case tidy:       strcpy(CURRENTITEM,varpath);
                       break;
      case disks:    
      case required:   strcpy(CURRENTPATH,varpath);
                       break;
		       
      case makepath:   strcpy(CURRENTPATH,varpath);
                       break;
		       
      case control:    if (CONTROLVAR == cfmountpat)
                          {
                          SetMountPath(varpath);
			  break;
                          }
      
                        if (CONTROLVAR == cfrepos)
			   {
			   SetRepository(varpath);
			   break;
			   }

		     if (CONTROLVAR == cfrepchar)
			{
			if (strlen(varpath) > 1)
			   {
			   yyerror("reposchar can only be a single letter");
			   break;
			   }
			if (varpath[0] == '/')
			   {
			   yyerror("illegal value for reposchar");
			   break;
			   }
			REPOSCHAR = varpath[0];
			}

		     if (CONTROLVAR == cflistsep)
			{
			if (strlen(varpath) > 1)
			   {
			   yyerror("listseparator can only be a single letter");
			   break;
			   }
			if (varpath[0] == '/')
			   {
			   yyerror("illegal value for listseparator");
			   break;
			   }
			LISTSEPARATOR = varpath[0];
			}			

                        if (CONTROLVAR == cfhomepat)
                           {
                           yyerror("Home-pattern should be relative to mount-path, not absolute");
                           }

                        break;

      case ignore:      AppendIgnore(varpath);
                        break;


      case import:      AppendImport(varpath);
	                break;
			
      case links:       /* FROM LINK */
	                break;

      case defaultroute:InstallDefaultRouteItem(varpath);
                        break;
      case image:
      case files:
      case editfiles:
                        break;

      case disable:    strcpy(CURRENTPATH,varpath);
                       ACTIONPENDING = true;
                       break;
		       
      case processes:   if (EXPR[0] == '\0')
	                   {
			   if (HAVE_RESTART)
			      {
			      yyerror("Missing search expression");
			      }
			   strcpy(EXPR,varpath);
			   }
                        else
			   {
			   if (HAVE_RESTART)
			      {
			      strcpy(RESTART,varpath);
			      HAVE_RESTART = false;
			      ACTIONPENDING = true;
			      }
			   }
      
		        break;

      case mountables: break;

      case binservers: InstallBinserverItem(varpath);
	               break;

      case homeservers: InstallHomeserverItem(varpath);
                        break;

      case misc_mounts:if (! MOUNT_FROM)
                          {
                          MOUNT_FROM = true;
                          strcpy(MOUNTFROM,CURRENTPATH);
                          }
                       else
                          {
                          if (MOUNT_ONTO)
			    {
                            yyerror ("Path not expected");
                            FatalError("miscmounts: syntax error");
                            }
                          MOUNT_ONTO = true;
                          strcpy(MOUNTONTO,CURRENTPATH);
                          }
                       break;

			
      case unmounta:    strcpy(CURRENTPATH,varpath);
	                break;

      case filters:    if (strlen(FILTERNAME) == 0)
	                  {
			  yyerror("empty or broken filter");
			  }
                       else
	                  {
			  strcpy(CURRENTITEM,varpath);
			  }
	                break;
      case deny:
      case admit:       Debug("admit/deny varpath=%s\n",varpath);
	                strcpy(CURRENTAUTHPATH,varpath);
	                break;

      case groups:      yyerror("Variables in groups/classes need to be quoted");

      default:          yyerror("Variable or name out of context");
      }
   }
}

/*******************************************************************/

void HandleWildcard(wildcard)

char *wildcard;

{
Debug1("wildcard = (%s)\n",wildcard);

ACTIONPENDING = true;

switch (ACTION)

   {
   case ignore: AppendIgnore(wildcard);
                break;

   case control: 
                 if (CONTROLVAR == cfhomepat)
                     {
                     if (*wildcard == '/')
                        {
                        yyerror("Home pattern specified as absolute path (should be relative to mountpath)");
                        }


                     Debug1(">>Installing wildcard %s as a home pattern\n",wildcard);
                     HandleHomePattern(wildcard);
		     }
                 else if (CONTROLVAR == nonexistentvar)
                     {
		     if (IsDefinedClass(CLASSBUFF))
			{
			AddMacroValue(CONTEXTID,CURRENTITEM,wildcard);
			}
                     }
                 else if ( CONTROLVAR == cfexcludecp ||
			   CONTROLVAR == cfexcludeln ||
 		           CONTROLVAR == cfcplinks   ||
                           CONTROLVAR == cflncopies  ||
		           CONTROLVAR == cfrepchar   ||
                           CONTROLVAR == cflistsep   )
		    {
                    if (IsAbsoluteFileName(wildcard))
                       {
                       yyerror("Pattern should be a relative name, not an absolute path");
                       }
                    InstallLocalInfo(wildcard);
                    }
                 else
                    {
		    if (IsDefinedClass(CLASSBUFF))
		       {
		       RecordMacroId(wildcard);
		       }
                    }
                 break;
		 
   case files:
                 HandleOptionalFileAttribute(wildcard);
		 break;
   case image:
                 HandleOptionalImageAttribute(wildcard);
		 break;
   case tidy:
                 HandleOptionalTidyAttribute(wildcard);
                 break;
		 
   case makepath:
                 HandleOptionalDirAttribute(wildcard);
		 break;

   case disable:
                 HandleOptionalDisableAttribute(wildcard);
                 break;
   case links:
                 HandleOptionalLinkAttribute(wildcard);
                 break;
   case processes:
                 HandleOptionalProcessAttribute(wildcard);
                 break;

   case misc_mounts:

                 if (!MOUNT_FROM)
		    {
		    MOUNT_FROM = true;
		    strcpy(MOUNTFROM,wildcard);
		    ACTIONPENDING = false;
		    }
		 else if (!MOUNT_ONTO)
		    {
		    MOUNT_ONTO = true;
		    strcpy(MOUNTONTO,wildcard);
		    ACTIONPENDING = false;
		    }
		 else
		    {
		    HandleOptionalMiscMountsAttribute(wildcard);
		    }
		 break;

   /* HvB : Bas van der Vlies */
   case mountables:
                 HandleOptionalMountablesAttribute(wildcard);
		 break;

   case unmounta:
                 HandleOptionalUnMountAttribute(wildcard);
		 break;
		 
   case shellcommands:
                 HandleOptionalScriptAttribute(wildcard);
		 break;

   case alerts:  yyerror("No attributes to alerts");
                 break;

   case disks:
   case required:
                 HandleOptionalRequired(wildcard);
		 break;

   case interfaces:
                 HandleOptionalInterface(wildcard);
		 break;

   case admit:
                 FuzzyMatchParse(wildcard);
                 InstallAuthItem(CURRENTAUTHPATH,wildcard,&VADMIT,&VADMITTOP,CLASSBUFF);
		 break;
   case deny:
                 FuzzyMatchParse(wildcard);       
                 InstallAuthItem(CURRENTAUTHPATH,wildcard,&VDENY,&VDENYTOP,CLASSBUFF);
		 break;

   case acls:
                 AddACE(CURRENTITEM,wildcard,CLASSBUFF);
		 break;

   case import:  AppendImport(wildcard);
                 break;

   case filters: if (strlen(FILTERNAME) == 0 || strlen(CURRENTITEM) == 0)
                    {
		    yyerror("Broken filter");
		    }
                 else
                    {
		    strcpy(FILTERDATA,wildcard);
		    InstallPending(ACTION);
		    }
                 break;

      case strategies:
                 if (strlen(STRATEGYNAME)==0)
		    {
		    yyerror("No strategy alias");
		    }
		 else
		    {
		    Debug("Found strategy-class %s in %s\n",wildcard,STRATEGYNAME);
		    strcpy(CURRENTITEM,wildcard);
		    }
		 break;


   default:
                 yyerror("Wildcards cannot be used in this context:");
   }
}


/*******************************************************************/
/* Level 2                                                         */
/*******************************************************************/

void ParseFile(filename,env)

char *filename,*env;

{  
signal(SIGALRM,(void *)TimeOut);
alarm(RPCTIMEOUT);
 
if (!FileSecure(filename))
   {
   FatalError("Security exception");
   }
 
if ((yyin = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   printf("%s: Can't open file %s\n",VPREFIX,filename);

   if (env == NULL)
      {
      printf("%s: (%s is set to <nothing>)\n",VPREFIX,CFINPUTSVAR);
      }
   else
      {
      printf("%s: (%s is set to %s)\n",VPREFIX,CFINPUTSVAR,env);
      }
   
   exit (1);
   }
 
strcpy(VCURRENTFILE,filename);
 
Debug("BEGIN PARSING %s\n",VCURRENTFILE);
 
LINENUMBER=1;
 
while (!feof(yyin))
   { 
   yyparse();
   
   if (ferror(yyin))  /* abortable */
      {
      printf("%s: Error reading %s\n",VPREFIX,VCURRENTFILE);
      perror("cfengine");
      exit(1);
      }
   }
 
fclose (yyin);
 
alarm(0);
signal(SIGALRM,SIG_DFL);
InstallPending(ACTION);
}

/*******************************************************************/

int CompoundId(id)                       /* check for dots in the name */

char *id;

{ int count = 1;
  char *cid = id;

for (cid = id; *cid != '\0'; cid++)
   {
   if (*cid == '.' || *cid == '|')
      {
      count++;
      }
   }

bzero(CLASSBUFF,bufsize);
strcpy(CLASSBUFF,id);

return(count);
}


/*******************************************************************/

void RecordMacroId(name)

char *name;

{
Debug("RecordMacroId(%s)\n",name);
strcpy(CURRENTITEM,name); 

if (strcmp(name,"this") == 0)
   {
   yyerror("$(this) is a reserved variable");
   }
}

/*******************************************************************/
/* Toolkits Misc                                                   */
/*******************************************************************/

void InitializeAction()                                   /* Set defaults */

 {
 Debug1("InitializeAction()\n");
 
 ACTIONPENDING = false;

 PLUSMASK = (mode_t)0;
 MINUSMASK = (mode_t)0;
 PLUSFLAG = (u_long)0;
 MINUSFLAG = (u_long)0;
 VRECURSE = 0;
 VAGE = 99999;
 strcpy(VUIDNAME,"*");
 strcpy(VGIDNAME,"*");
 HAVE_RESTART = 0;
 FILEACTION=warnall;

 *CURRENTPATH = '\0';
 *CURRENTITEM = '\0';
 *DESTINATION = '\0';
 *IMAGEACTION = '\0';
 *LOCALREPOS = '\0';
 *EXPR = '\0';
 *RESTART = '\0';
 *FILTERDATA = '\0';
 *STRATEGYDATA = '\0';
 *CHDIR ='\0';
 CHROOT[0] = '\0';
 strcpy(VIFNAME,"");
 PTRAVLINKS = (short) '?';
 IMAGEBACKUP = 'y';
 ENCRYPT = 'n';
 VERIFY = 'n';
 ROTATE=0;
 TIDYSIZE=0;
 PROMATCHES=-1;
 PROACTION = 'd';
 PROSIGNAL=0;
 COMPRESS='n';
 AGETYPE='a';
 COPYTYPE = DEFAULTCOPYTYPE; /* 't' */
 LINKDIRS = 'k';
 USESHELL = 'y';
 LOGP = 'd';
 INFORMP = 'd';
 PURGE = 'n';
 TRUSTKEY = 'n';
 CHECKSUM = 'n';
 TIDYDIRS = false;
 VEXCLUDEPARSE = NULL;
 VINCLUDEPARSE = NULL;
 VIGNOREPARSE = NULL;
 VACLBUILD = NULL;
 VFILTERBUILD = NULL;
 VSTRATEGYBUILD = NULL;
 VCPLNPARSE = NULL;
 VTIMEOUT=0;

 bzero(ALLCLASSBUFFER,bufsize);
 bzero(ELSECLASSBUFFER,bufsize);
 
 strcpy(CFSERVER,"localhost");
 
 IMGCOMP = DISCOMP='>';
 IMGSIZE = DISABLESIZE=cfnosize;
 DELETEDIR = 'y';   /* t=true */
 DELETEFSTAB = 'y';
 FORCE = 'n';
 FORCEDIRS = 'n';
 STEALTH = 'n';
 PRESERVETIMES = 'n';
 TYPECHECK = 'y';
 UMASK = 077;     /* Default umask for scripts/files */
 FORK = 'n';
 PREVIEW = 'n';
 COMPATIBILITY = 'n';

 if (MOUNT_FROM && MOUNT_ONTO)
    {
    Debug("Resetting miscmount data\n");
    MOUNT_FROM = false;
    MOUNT_ONTO = false;
    MOUNTMODE='w';
    *MOUNTFROM = '\0';
    *MOUNTONTO = '\0';
    }

 /* 
  * HvB: Bas van der Vlies
 */
 MOUNT_RO=false;
 MOUNTOPTS[0]='\0';
 
 /* Make sure we don't clean the buffer in the middle of a link! */

 if ( ! ACTION_IS_LINK && ! ACTION_IS_LINKCHILDREN)
    {
    bzero(LINKFROM,bufsize);
    bzero(LINKTO,bufsize);  /* ALSO RESTART */
    LINKSILENT = false;
    LINKTYPE = 's';
    FORCELINK = 'n';
    DEADLINKS = false;
    }
 }

/*********************************************************************/

void SetMountPath (value)

char *value;

 { char buff[bufsize];

 ExpandVarstring(value,buff,"");
 
 Debug("Appending [%s] to mountlist\n",buff);
 
 AppendItem(&VMOUNTLIST,buff,CLASSBUFF);
 }

/*********************************************************************/

void SetRepository (value)

char *value;

 {
  if (*value != '/')
     {
     yyerror("File repository must be an absolute directory name");
     }
  
  if (strlen(VREPOSITORY) != 0)
     {
     yyerror("Redefinition of system variable repository");
     }

 ExpandVarstring(value,VBUFF,"");
 VREPOSITORY = strdup(VBUFF);
 }

/* EOF */
