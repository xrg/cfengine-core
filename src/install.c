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
/* Routines which install actions parsed by the parser             */
/*                                                                 */
/* Derived from parse.c (Parse object)                             */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

void InstallLocalInfo (varvalue)

char *varvalue;

{ int number = -1;
  char buffer[maxvarsize], command[maxvarsize], *sp;
  char value[bufsize];
  
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",value);
   return;
   }

if (strcmp(varvalue,CF_NOCLASS) == 0)
   {
   Debug1("Not installing %s, evaluated to false\n",value);
   return;
   }
 
ExpandVarstring(varvalue,value,NULL);

/* begin version compat */ 
if (strncmp(value,"exec ",5) == 0)
   {
   for (sp = value+4; *sp == ' '; sp++)
      {
      }

   if (*sp == '/')
      {
      strncpy(command,sp,maxvarsize);
      GetExecOutput(command,value);
      Chop(value);
      value[maxvarsize-1] = '\0';  /* Truncate to maxvarsize */
      }
   else
      {
      Warning("Exec string in control did not specify an absolute path");
      }
   }

/* end version 1 compat*/ 

if (IsBuiltinFunction(value))
   {
   strcpy(value,EvaluateFunction(value,VBUFF));
   }
 
Debug1("\n(Action is control, storing variable [%s=%s])\n",CURRENTITEM,value);

switch (ScanVariable(CURRENTITEM))
   {
   case cfsite:
   case cffaculty:  if (!IsDefinedClass(CLASSBUFF))
                      {
		      break;
		      }

                    if (VFACULTY[0] != '\0')
                       {
                       yyerror("Multiple declaration of variable faculty / site");
                       FatalError("Redefinition of basic system variable");
                       }

                    strcpy(VFACULTY,value);
                    break;

   case cfdomain:  if (!IsDefinedClass(CLASSBUFF))
                      {
		      break;
		      }

                   strcpy(VDOMAIN,value);
		  
		   if (!strstr(VSYSNAME.nodename,ToLowerStr(VDOMAIN)))
		      {
		      snprintf(VFQNAME,bufsize,"%s.%s",VSYSNAME.nodename,ToLowerStr(VDOMAIN));
		      strcpy(VUQNAME,VSYSNAME.nodename);
		      }
		   else
		      {
		      int n = 0;
		      strcpy(VFQNAME,VSYSNAME.nodename);
		      
		      while(VSYSNAME.nodename[n++] != '.')
			{
			}

                      strncpy(VUQNAME,VSYSNAME.nodename,n-1);		      
 		      }
		   
		   if (! NOHARDCLASSES)
		      {
		      if (strlen(VFQNAME) > maxvarsize-1)
			 {
		 	 FatalError("The fully qualified name is longer than maxvarsize!!");
			 }
		     
		      strcpy(buffer,VFQNAME);
		     
		      AddClassToHeap(CanonifyName(buffer));
		      }
                   break;

   case cfsysadm:  /* Can be redefined */
                   if (!IsDefinedClass(CLASSBUFF))
                      {
		      break;
		      }

                  strcpy(VSYSADM,value);
                  break;

   case cfnetmask:
                   if (!IsDefinedClass(CLASSBUFF))
                      {
		      break;
		      }

                  if (VNETMASK[0] != '\0')
                     {
                     yyerror("Multiple declaration of variable netmask");
                     FatalError("Redefinition of basic system variable");
                     }
                  strcpy(VNETMASK,value);
		  AddNetworkClass(VNETMASK);
                  break;


   case cfmountpat: SetMountPath(value);
                    break;

   case cfrepos:
                   SetRepository(value);
		   break;

   case cfhomepat:  
                  Debug1("Installing %s as home pattern\n",value);
                  AppendItem(&VHOMEPATLIST,value,CLASSBUFF);
                  break;

   case cfextension:
                  AppendItem(&EXTENSIONLIST,value,CLASSBUFF);
                  break;

   case cfsuspicious:
                  AppendItem(&SUSPICIOUSLIST,value,CLASSBUFF);
		  break;

   case cfschedule:
                  AppendItem(&SCHEDULE,value,CLASSBUFF);
		  break;

   case cfspooldirs:
                  AppendItem(&SPOOLDIRLIST,value,CLASSBUFF);
		  break;
		  
   case cfnonattackers:
                  AppendItem(&NONATTACKERLIST,value,CLASSBUFF);
		  break;
   case cfmulticonn:
                  AppendItem(&MULTICONNLIST,value,CLASSBUFF);
		  break;
   case cftrustkeys:
                  AppendItem(&TRUSTKEYLIST,value,CLASSBUFF);
		  break;
   case cfallowusers:
                  AppendItem(&ALLOWUSERLIST,value,CLASSBUFF);
		  break;		  
   case cfskipverify:
                  AppendItem(&SKIPVERIFY,value,CLASSBUFF);
		  break;
		  
   case cfattackers:
                  AppendItem(&ATTACKERLIST,value,CLASSBUFF);
		  break;
		  
   case cftimezone:
                   if (!IsDefinedClass(CLASSBUFF))
                      {
		      break;
		      }

                    AppendItem(&VTIMEZONE,value,NULL);
                    break;

   case cfssize: 
                  sscanf(value,"%d",&number);
                  if (number >= 0)
                     {
                     SENSIBLEFSSIZE = number;
                     }
                  else
                     {
                     yyerror("Silly value for sensiblesize (must be positive integer)");
                     }
                  break;

   case cfscount:
                  sscanf(value,"%d",&number);
                  if (number > 0)
                     {
                     SENSIBLEFILECOUNT = number;
                     }
                  else
                     {
                     yyerror("Silly value for sensiblecount (must be positive integer)");
                     }

                  break;

   case cfeditsize:
                  sscanf(value,"%d",&number);

                  if (number >= 0)
                     {
                     EDITFILESIZE = number;
                     }
                  else
                     {
                     yyerror("Silly value for editfilesize (must be positive integer)");
                     }

                  break;

   case cfbineditsize:
                  sscanf(value,"%d",&number);

                  if (number >= 0)
                     {
                     EDITBINFILESIZE = number;
                     }
                  else
                     {
                     yyerror("Silly value for editbinaryfilesize (must be positive integer)");
                     }

                  break;
		  
   case cfifelapsed:
                  sscanf(value,"%d",&number);

                  if (number >= 0)
                     {
                     VDEFAULTIFELAPSED = VIFELAPSED = number;
                     }
                  else
                     {
                     yyerror("Silly value for IfElapsed");
                     }

                  break;
		  
   case cfexpireafter:
                  sscanf(value,"%d",&number);

                  if (number > 0)
                     {
                     VDEFAULTEXPIREAFTER = VEXPIREAFTER = number;
                     }
                  else
                     {
                     yyerror("Silly value for ExpireAfter");
                     }

                  break;
		  
   case cfactseq:
                  AppendToActionSequence(value);
                  break;

   case cfaccess:
                  AppendToAccessList(value);
                  break;

   case cfnfstype:
                  strcpy(VNFSTYPE,value); 
                  break;

   case cfaddclass:
                  AddCompoundClass(value);
                  break;

   case cfinstallclass:
                  AddInstallable(value);
		  break;
		  
   case cfexcludecp:
                  PrependItem(&VEXCLUDECOPY,value,CLASSBUFF);
                  break;
		  
   case cfexcludeln:
                  PrependItem(&VEXCLUDELINK,value,CLASSBUFF);
                  break;
		  
   case cfcplinks:
                  PrependItem(&VCOPYLINKS,value,CLASSBUFF);
                  break;
   case cflncopies:
                  PrependItem(&VLINKCOPIES,value,CLASSBUFF);
                  break;

   case cfrepchar:
		    if (strlen(value) > 1)
			{
			yyerror("reposchar can only be a single letter");
			break;
			}
		     if (value[0] == '/')
			{
			yyerror("illegal value for reposchar");
			break;
			}
		     REPOSCHAR = value[0];
		     break;

   case cflistsep:
			if (strlen(value) > 1)
			   {
			   yyerror("listseparator can only be a single letter");
			   break;
			   }
			if (value[0] == '/')
			   {
			   yyerror("illegal value for listseparator");
			   break;
			   }
			LISTSEPARATOR = value[0];
			break;
			
   case cfunderscore:
                        if (strcmp(value,"on") == 0)
			   { char rename[maxvarsize];
			   UNDERSCORE_CLASSES=true;
			   Verbose("Resetting classes using underscores...\n");
			   while(DeleteItemContaining(&VHEAP,CLASSTEXT[VSYSTEMHARDCLASS]))
			      {
			      }

			   sprintf(rename,"_%s",CLASSTEXT[VSYSTEMHARDCLASS]);
			   
                           AddClassToHeap(rename);
			   break;
			   }

			if (strcmp(value,"off") == 0)
			   {
			   UNDERSCORE_CLASSES=false;
			   break;
			   }
			
                        yyerror("illegal value for underscoreclasses");
			break;

   case cfifname:    if (strlen(value)>15)
                        {
	     	        yyerror("Silly interface name, (should be something link eth0)");
	                }

                     strcpy(VIFNAMEOVERRIDE,value);
		     VIFDEV[VSYSTEMHARDCLASS] = VIFNAMEOVERRIDE; /* override */
		     Debug("Overriding interface with %s\n",VIFDEV[VSYSTEMHARDCLASS]);
		     break;

   case cfdefcopy: if (strcmp(value,"ctime") == 0)
                      {
		      DEFAULTCOPYTYPE = 't';
		      return;
		      }
                   else if (strcmp(value,"mtime") == 0)
		      {
		      DEFAULTCOPYTYPE = 'm';
		      return;
		      }
                   else if (strcmp(value,"checksum")==0 || strcmp(value,"sum") == 0)
		      {
		      DEFAULTCOPYTYPE = 'c';
		      return;
		      }
                   else if (strcmp(value,"byte")==0 || strcmp(value,"binary") == 0)
		      {
		      DEFAULTCOPYTYPE = 'b';
		      return;
		      }
                   yyerror("Illegal default copy type");
		   break;

   default:
                  if (IsDefinedClass(CLASSBUFF))
		     {
		     AddMacroValue(CURRENTITEM,value);
		     }
                  break;
   }
}

/*******************************************************************/

void HandleEdit(file,edit,string)      /* child routines in edittools.c */

char *file, *edit, *string;

{
if (string == NULL)
   {
   Debug1("Handling Edit of %s, action [%s] with no data\n",file,edit);
   }
else
   {
   Debug1("Handling Edit of %s, action [%s] with data <%s>\n",file,edit,string);
   }

if (EditFileExists(file))
   {
   AddEditAction(file,edit,string);
   }
else
   {
   InstallEditFile(file,edit,string);
   }
}

/********************************************************************/

void HandleOptionalFileAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("File attribute with no value");
   }

Debug1("HandleOptionalFileAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfrecurse: HandleRecurse(value);
                   break;
   case cfmode:    ParseModeString(value,&PLUSMASK,&MINUSMASK);
                   break;
   case cfflags:   ParseFlagString(value,&PLUSFLAG,&MINUSFLAG);
                   break;		   
   case cfowner:   if (strlen(value) < bufsize)
                      {
		      strcpy(VUIDNAME,value);
		      }
                   else
		      {
		      yyerror("Too many owners");
		      }
                   break;
   case cfgroup:   if (strlen(value) < bufsize)
                      {
		      strcpy(VGIDNAME,value);
		      }
                   else
		      {
		      yyerror("Too many groups");
		      }
                   break;
   case cfaction:  FILEACTION = (enum fileactions) GetFileAction(value);
                   break;
   case cflinks:   HandleTravLinks(value);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;
   case cfacl:     PrependItem(&VACLBUILD,value,CF_ANYCLASS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;		   
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfchksum:  HandleChecksum(value);
                   break;
   default:        yyerror("Illegal file attribute");
   }
}


/*******************************************************************/

void HandleOptionalImageAttribute(item)

char *item;

{ char value[bufsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%[^\n]",value);

if (value[0] == '\0')
   {
   yyerror("Copy attribute with no value");
   }

Debug1("HandleOptionalImageAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfmode:    ParseModeString(value,&PLUSMASK,&MINUSMASK);
                   break;
   case cfflags:   ParseFlagString(value,&PLUSFLAG,&MINUSFLAG);
                   break;
   case cfowner:   strcpy(VUIDNAME,value);
                   break;
   case cfgroup:   strcpy(VGIDNAME,value);
                   break;
   case cfdest:    strcpy(DESTINATION,value);
                   break;
   case cfaction:  strcpy(IMAGEACTION,value);
                   break;
   case cfcompat:  HandleCharSwitch("oldserver",value,&COMPATIBILITY);
                   break;
   case cfforce:   HandleCharSwitch("force",value,&FORCE);
                   break;
   case cfforcedirs: HandleCharSwitch("forcedirs",value,&FORCEDIRS);
                   break;		   
   case cfforceipv4: HandleCharSwitch("forceipv4",value,&FORCEIPV4);
                   break;		   
   case cfbackup:  HandleCharSwitch("backup",value,&IMAGEBACKUP);
                   break;
   case cfrecurse: HandleRecurse(value);
                   break;
   case cftype:    HandleCopyType(value);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfsymlink: PrependItem(&VCPLNPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;
   case cflntype:  HandleLinkType(value);
                   break;
   case cfserver:  HandleServer(value);
                   break;
   case cfencryp:  HandleCharSwitch("encrypt",value,&ENCRYPT);
                   break;
   case cfverify:  HandleCharSwitch("verify",value,&VERIFY);
                   break;
   case cfdefine:  HandleDefine(value);       
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;		   
   case cfsize:    HandleCopySize(value);
                   break;
   case cfacl:     PrependItem(&VACLBUILD,value,CF_ANYCLASS);
                   break;
   case cfpurge:   HandleCharSwitch("purge",value,&PURGE);
                   break;
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfstealth: HandleCharSwitch("stealth",value,&STEALTH);
                   break;
   case cftypecheck: HandleCharSwitch("typecheck",value,&TYPECHECK);
                   break;
   case cfrepository: strncpy(LOCALREPOS,value,bufsize-buffer_margin);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;

   case cftrustkey: HandleCharSwitch("trustkey",value,&TRUSTKEY);
                   break;
   case cftimestamps:
                   HandleTimeStamps(value);
                   break;

   default:        yyerror("Illegal copy attribute");
   }
}

/******************************************************************/

void HandleOptionalRequired(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Required/disk attribute with no value");
   }

Debug1("HandleOptionalRequiredAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cffree:    HandleRequiredSize(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;	   
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;

   default:        yyerror("Illegal disk/required attribute");
   }

}

/******************************************************************/

void HandleOptionalInterface(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("intefaces attribute with no value");
   }

Debug1("HandleOptionalRequiredAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfsetnetmask:   HandleNetmask(value);
                        break;
   case cfsetbroadcast: HandleBroadcast(value);
                        break;

   default:        yyerror("Illegal interfaces attribute");
   }

}


/******************************************************************/

void HandleOptionalUnMountAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Unmount attribute with no value");
   }

Debug1("HandleOptionalUnMountsItem(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfdeldir: if (strcmp(value,"true") == 0 || strcmp(value,"yes") == 0)
                     {
		     DELETEDIR = 'y';
		     break;
		     }
   
                  if (strcmp(value,"false") == 0 || strcmp(value,"no") == 0)
                     {
		     DELETEDIR = 'n';
		     break;
		     }

   case cfdelfstab: if (strcmp(value,"true") == 0 || strcmp(value,"yes") == 0)
                       {
		       DELETEFSTAB = 'y';
		       break;
		       }
   
                    if (strcmp(value,"false") == 0 || strcmp(value,"no") == 0)
                       {
		       DELETEFSTAB = 'n';
		       break;
		       }   
		    
   case cfforce:    if (strcmp(value,"true") == 0 || strcmp(value,"yes") == 0)
                       {
		       FORCE = 'y';
		       break;
		       }
   
                    if (strcmp(value,"false") == 0 || strcmp(value,"no") == 0)
                       {
		       FORCE = 'n';
		       break;
		       }

   default:         yyerror("Illegal unmount option"
                            " (deletedir/deletefstab/force)");
   }

}

/******************************************************************/

void HandleOptionalMiscMountsAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Miscmounts attribute with no value");
   }

Debug1("HandleOptionalMiscMOuntsItem(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfmode:    { struct Item *op;
   		     struct Item *next;

		      if (MOUNTOPTLIST)                     /* just in case */
			 DeleteItemList(MOUNTOPTLIST);
		      MOUNTOPTLIST = SplitStringAsItemList(value,',');

		      for (op = MOUNTOPTLIST; op != NULL; op = next)
			 {
			 next = op->next;  /* in case op is deleted */
			 Debug1("miscmounts option: %s\n", op->name);

			 if (strcmp(op->name,"rw") == 0)
			    {
			    MOUNTMODE='w';
			    DeleteItem(&MOUNTOPTLIST,op);
			    }
			 else if (strcmp(op->name,"ro") == 0)
			    {
			    MOUNTMODE='o';
			    DeleteItem(&MOUNTOPTLIST,op);
			    }
			 else if (strcmp(op->name,"bg") == 0)
			    {
			    /* backgrounded mounts can hang MountFileSystems */
			    DeleteItem(&MOUNTOPTLIST,op);
			    }
			 /* validate other mount options here */
			 }
		   }
		   break;
  
   default:        yyerror("Illegal miscmounts attribute (rw/ro)");
   }

}

/******************************************************************/

void HandleOptionalTidyAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Tidy attribute with no value");
   }

Debug1("HandleOptionalTidyAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfrecurse: HandleRecurse(value);
                   break;

   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;

   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;

   case cfinclude: 
   case cfpattern: strcpy(CURRENTITEM,value);
                   if (*value == '/')
		      {
		      yyerror("search pattern begins with / must be a relative name");
		      }
                   break;

   case cfage:     HandleAge(value);
                   break;

   case cflinks:   HandleTravLinks(value);
                   break;

   case cfsize:    HandleTidySize(value);
                   break;

   case cftype:    HandleTidyType(value);
                   break;

   case cfdirlinks:
                   HandleTidyLinkDirs(value);
		   break;

   case cfrmdirs:  HandleTidyRmdirs(value);
                   break;

   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;		   
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfcompress: HandleCharSwitch("compress",value,&COMPRESS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;

   default:        yyerror("Illegal tidy attribute");
   }
}

/******************************************************************/

void HandleOptionalDirAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(item,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Directory attribute with no value");
   }

Debug1("HandleOptionalDirAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfmode:    ParseModeString(value,&PLUSMASK,&MINUSMASK);
                   break;
   case cfflags:   ParseFlagString(value,&PLUSFLAG,&MINUSFLAG);
                   break;
   case cfowner:   strcpy(VUIDNAME,value);
                   break;
   case cfgroup:   strcpy(VGIDNAME,value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;		   
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;

   default:        yyerror("Illegal directory attribute");
   }
}


/*******************************************************************/

void HandleOptionalDisableAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Disable attribute with no value");
   }

Debug1("HandleOptionalDisableAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfaction:  if (strcmp(value,"warn") == 0)
                      {
		      PROACTION = 'w';
		      }
                   else if (strcmp(value,"delete") == 0)
		      {
		      PROACTION = 'd';
		      }
                   else
                      {
                      yyerror("Unknown action for disable");
                      }
                   break;

   case cftype:    HandleDisableFileType(value);
                   break;

   case cfrotate:  HandleDisableRotate(value);
                   break;
		   
   case cfsize:    HandleDisableSize(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;		   
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfrepository: strncpy(LOCALREPOS,value,bufsize-buffer_margin);
                   break;

   default:        yyerror("Illegal disable attribute");
   }
}


/*******************************************************************/

void HandleOptionalLinkAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Link attribute with no value");
   }

Debug1("HandleOptionalLinkAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfaction:  HandleLinkAction(value);
                   break;
   case cftype:    HandleLinkType(value);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;		   
   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;
   case cfcopy:    PrependItem(&VCPLNPARSE,value,CF_ANYCLASS);
                   break;
   case cfrecurse: HandleRecurse(value);
                   break;
   case cfcptype:  HandleCopyType(value);
                   break;
   case cfnofile:  HandleDeadLinks(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;		   
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;

   default:        yyerror("Illegal link attribute");
   }
}

/*******************************************************************/

void HandleOptionalProcessAttribute(item)

char *item;

{ char value[maxvarsize];

if (HAVE_RESTART)     /* wrote `restart' without following by ".." */
   {
   yyerror("Missing restart command (quoted string expected) or bad SetOptionString placement");
   }

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Process attribute with no value");
   }

Debug1("HandleOptionalProcessAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfaction:  if (strcmp(value,"signal") == 0 || strcmp(value,"do") == 0)
                      {
		      PROACTION = 's';
                      }
                   else if (strcmp(value,"bymatch") == 0)
		      {
		      PROACTION = 'm';
		      }
                   else if (strcmp(value,"warn") == 0)
		      {
		      PROACTION = 'w';
		      }
                   else
		      {
		      yyerror("Unknown action for processes");
		      }
                   break;
   case cfmatches: HandleProcessMatches(value);
                   break;
   case cfsignal:  HandleProcessSignal(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;
   case cfuseshell:HandleUseShell(value);
                   break;
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;
   case cfowner:   strcpy(VUIDNAME,value);
		   break;
   case cfgroup:   strcpy(VGIDNAME,value);
                   break;
   case cfchdir:   HandleChDir(value);
                   break;
   case cfchroot:  HandleChRoot(value);
                   break;
   case cfumask:   HandleUmask(value);
                   break;
		   
   default:        yyerror("Illegal process attribute");
   }
}

/*******************************************************************/

void HandleOptionalScriptAttribute(item)

char *item;

{ char value[maxvarsize];

VBUFF[0] = value[0] = '\0';

ExpandVarstring(item,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Shellcommand attribute with no value");
   }

Debug1("HandleOptionalScriptAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cftimeout:   HandleTimeOut(value);
                     break;
   case cfuseshell:  HandleUseShell(value);
                     break;
   case cfsetlog:    HandleCharSwitch("log",value,&LOGP);
                     break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                     break;
   case cfowner:     strcpy(VUIDNAME,value);
		     break;
   case cfgroup:     strcpy(VGIDNAME,value);
                     break;
   case cfdefine:    HandleDefine(value);
                     break;
   case cfelsedef:   HandleElseDefine(value);
                     break;
   case cfumask:     HandleUmask(value);
                     break;
   case cffork:      HandleCharSwitch("background",value,&FORK);
                     break;
   case cfchdir:     HandleChDir(value);
                     break;
   case cfchroot:    HandleChRoot(value);
                     break;		     
   case cfpreview:   HandleCharSwitch("preview",value,&PREVIEW);
                     break;		     

   default:         yyerror("Illegal shellcommand attribute");
   }

}

/*******************************************************************/

void HandleChDir(value)

char *value;

{
if (!IsAbsoluteFileName(value))
   {
   yyerror("chdir is not an absolute directory name");
   }

strcpy(CHDIR,value); 
}

/*******************************************************************/

void HandleChRoot(value)

char *value;

{
if (!IsAbsoluteFileName(value))
   {
   yyerror("chdir is not an absolute directory name");
   }
 
strcpy(CHROOT,value);  
}

/*******************************************************************/

void HandleFileItem(item)

char *item;

{ char err[100];

if (strcmp(item,"home") == 0)
   {
   ACTIONPENDING=true;
   strcpy(CURRENTPATH,"home");
   return;
   }

snprintf(err,100,"Unknown attribute %s",item);
yyerror(err);
}


/*******************************************************************/

void InstallBroadcastItem(item)

char *item;

{
Debug1("Install broadcast mode (%s)\n",item);

if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }

if (VBROADCAST[0] != '\0')
   {
   yyerror("Multiple declaration of variable broadcast");
   FatalError("Redefinition of basic system variable");
   }

if (strcmp("ones",item) == 0)
   {
   strcpy(VBROADCAST,"one");
   return;
   }

if (strcmp("zeroes",item) == 0)
   {
   strcpy(VBROADCAST,"zero");
   return;
   }

if (strcmp("zeros",item) == 0)
   {
   strcpy(VBROADCAST,"zero");
   return;
   }

yyerror ("Unknown broadcast mode (should be ones, zeros or zeroes)");
FatalError("Unknown broadcast mode");
}

/*******************************************************************/

void InstallDefaultRouteItem(item)

char *item;

{ struct hostent *hp;
  struct in_addr inaddr;

Debug1("Install defaultroute mode (%s)\n",item);

if (!IsDefinedClass(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }

if (VDEFAULTROUTE[0] != '\0')
   {
   yyerror("Multiple declaration of variable defaultroute");
   FatalError("Redefinition of basic system variable");
   }

ExpandVarstring(item,VBUFF,NULL);

 if (inet_addr(VBUFF) == -1)
   {
   if ((hp = gethostbyname(VBUFF)) == NULL)
      {
      snprintf(OUTPUT,bufsize*2,"Bad address/host (%s) in default route\n",VBUFF);
      CfLog(cferror,OUTPUT,"gethostbyname");
      yyerror ("Bad specification of default packet route: hostname or decimal IP address");
      }
   else
      {
      bcopy(hp->h_addr,&inaddr, hp->h_length);
      strcpy(VDEFAULTROUTE,inet_ntoa(inaddr));
      }
   }
else
   {
   strcpy(VDEFAULTROUTE,VBUFF);
   }
}

/*******************************************************************/

void HandleGroupItem(item,type)

char *item;
enum itemtypes type;

{ char *machine, *user, *domain;

Debug1("Handling group item (%s) in group (%s), type=%d\n",item,GROUPBUFF,type);

switch (type)
   {
   case simple:    if (strcmp(item,VDEFAULTBINSERVER.name) == 0)
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

                   if (strcmp(item,VFQNAME) == 0)
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

                   if (IsItemIn(VHEAP,item))  /* group reference */
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

		   if (IsDefinedClass(item))
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

		   Debug("No match of class\n");

                   break;

   case netgroup:  setnetgrent(item);

                   while (getnetgrent(&machine,&user,&domain))
                      {
                      if (strcmp(machine,VDEFAULTBINSERVER.name) == 0)
                         {
                         Debug1("Matched %s in netgroup %s\n",machine,item);
                         AddClassToHeap(GROUPBUFF);
                         break;
                         }

		      if (strcmp(machine,VFQNAME) == 0)
                         {
                         Debug1("Matched %s in netgroup %s\n",machine,item);
                         AddClassToHeap(GROUPBUFF);
                         break;
                         }
                      }
                   
                   endnetgrent();
                   break;


   case groupdeletion: 

                   setnetgrent(item);

                   while (getnetgrent(&machine,&user,&domain))
                      {
                      if (strcmp(machine,VDEFAULTBINSERVER.name) == 0)
                         {
                         Debug1("Matched delete item %s in netgroup %s\n",machine,item);
                         DeleteItemStarting(&VHEAP,GROUPBUFF);
                         break;
                         }
		      
                      if (strcmp(machine,VFQNAME) == 0)
                         {
                         Debug1("Matched delete item %s in netgroup %s\n",machine,item);
                         DeleteItemStarting(&VHEAP,GROUPBUFF);
                         break;
                         }
		      
                      }
                   
                   endnetgrent();
                   break;

   case classscript:

                   VBUFF[0] = '\0';
                   ExpandVarstring (item+1,VBUFF,NULL);
                   if (VBUFF[0] != '/')
                      {
                      yyerror("Quoted scripts must begin with / for absolute path");
                      break;
                      }

		   SetClassesOnScript(VBUFF,GROUPBUFF,NULL,false);
                   break;

   case deletion:  if (IsDefinedClass(item))
                      {
                      DeleteItemStarting(&VHEAP,GROUPBUFF);
                      }
                   break;

   default:        yyerror("Software error");
                   FatalError("Unknown item type");
   }
}

/*******************************************************************/

void HandleHomePattern(pattern)

char *pattern;

{
VBUFF[0]='\0';
ExpandVarstring(pattern,VBUFF,"");
AppendItem(&VHOMEPATLIST,VBUFF,CLASSBUFF);
}

/*******************************************************************/

void AppendNameServer(item)

char *item;

{ 
Debug1("Installing item (%s) in the nameserver list\n",item);

if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }

AppendItem(&VRESOLVE,item,CLASSBUFF);
}

/*******************************************************************/

void AppendImport(item)

char *item;

{ 
if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }

if (strcmp(item,VCURRENTFILE) == 0)
   {
   yyerror("A file cannot import itself");
   FatalError("Infinite self-reference in class inheritance");
   }

Debug1(">>Installing item (%s) in the import list\n",item);

ExpandVarstring(item,VBUFF,"");
 
AppendItem(&VIMPORT,VBUFF,CLASSBUFF);
}


/*******************************************************************/

void InstallHomeserverItem(item)

char *item;

{
if (! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }

Debug1("Installing item (%s) in  list for homeservers (%s) in group (%s)\n",item,CLASSBUFF,GROUPBUFF);

AppendItem(&VHOMESERVERS,item,CLASSBUFF);
}

/*******************************************************************/

void InstallBinserverItem(item)           /* Install if matches classes */

char *item;

{
if (! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }

AppendItem(&VBINSERVERS,item,CLASSBUFF);
}

/*******************************************************************/

void InstallMailserverPath(path)

char *path;

{
if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",path);
   return;
   }

if (VMAILSERVER[0] != '\0')
   {
   FatalError("Redefinition of mailserver");
   }

strcpy(VMAILSERVER,path);

Debug1("Installing mailserver (%s) for group (%s)",path,GROUPBUFF);
}

/*******************************************************************/

void InstallLinkItem (from,to)

char *from, *to;

{ struct Link *ptr;
  char buffer[bufsize], buffer2[bufsize];
  
Debug1("Storing Link: (From)%s->(To)%s\n",from,to);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing link no match\n");
   return;
   }

ExpandVarstring(from,VBUFF,"");

 if (strlen(VBUFF) > 1)
   {
   DeleteSlash(VBUFF);
   }

ExpandVarstring(to,buffer,"");

if (strlen(VBUFF) > 1)
   { 
   DeleteSlash(buffer);
   }

ExpandVarstring(ALLCLASSBUFFER,buffer2,""); 
 
if ((ptr = (struct Link *)malloc(sizeof(struct Link))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #1");
   }
 
if ((ptr->from = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #2");
   }

if ((ptr->to = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #3");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #4");
   }

if ((ptr->defines = strdup(buffer2)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #4a");
   }

ExpandVarstring(ELSECLASSBUFFER,buffer,"");

if ((ptr->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for Installrequied() #2");
   }
 
AddInstallable(ptr->defines);
AddInstallable(ptr->elsedef); 
 
if (VLINKTOP == NULL)                 /* First element in the list */
   {
   VLINK = ptr;
   }
else
   {
   VLINKTOP->next = ptr;
   }

if (strlen(ptr->from) > 1)
   {
   DeleteSlash(ptr->from);
   }

if (strlen(ptr->to) > 1)
   {
   DeleteSlash(ptr->to);
   }

ptr->force = FORCELINK;
ptr->silent = LINKSILENT;
ptr->type = LINKTYPE;
ptr->copytype = COPYTYPE;
ptr->next = NULL;
ptr->copy = VCPLNPARSE;
ptr->exclusions = VEXCLUDEPARSE;
ptr->inclusions = VINCLUDEPARSE;
ptr->ignores = VIGNOREPARSE;
ptr->filters=VFILTERBUILD;
ptr->recurse = VRECURSE;
ptr->nofile = DEADLINKS;
ptr->log = LOGP;
ptr->inform = INFORMP;
ptr->done = 'n';

VLINKTOP = ptr;

if (ptr->recurse != 0)
   {
   yyerror("Recursion can only be used with +> multiple links");
   }

InitializeAction();
}

/*******************************************************************/

void InstallLinkChildrenItem (from,to)

char *from, *to;

{ struct Link *ptr;
  char *sp, buffer[bufsize];
  struct TwoDimList *tp = NULL;

Debug1("Storing Linkchildren item: %s -> %s\n",from,to);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing linkchildren no match\n");
   return;
   }

ExpandVarstring(from,VBUFF,"");
ExpandVarstring(ALLCLASSBUFFER,buffer,""); 

Build2DListFromVarstring(&tp,to,'/');
    
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if ((ptr = (struct Link *)malloc(sizeof(struct Link))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallListChildrenItem() #1");
      }

   if ((ptr->from = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallLinkchildrenItem() #2");
      }

   if ((ptr->to = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallLinkChildrenItem() #3");
      }

   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallLinkChildrenItem() #3");
      }

   if ((ptr->defines = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallListItem() #4a");
      }
   
   ExpandVarstring(ELSECLASSBUFFER,buffer,"");
   
   if ((ptr->elsedef = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for Installrequied() #2");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef); 
   
   if (VCHLINKTOP == NULL)                 /* First element in the list */
      {
      VCHLINK = ptr;
      }
   else
      {
      VCHLINKTOP->next = ptr;
      }

   ptr->force = FORCELINK;
   ptr->silent = LINKSILENT;
   ptr->type = LINKTYPE;
   ptr->next = NULL;
   ptr->copy = VCPLNPARSE;
   ptr->exclusions = VEXCLUDEPARSE;
   ptr->inclusions = VINCLUDEPARSE;
   ptr->ignores = VIGNOREPARSE;
   ptr->recurse = VRECURSE;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->done = 'n';

   VCHLINKTOP = ptr;

   if (ptr->recurse != 0 && strcmp(ptr->to,"linkchildren") == 0)
      {
      yyerror("Sorry don't know how to recurse with linkchildren keyword");
      }
   }

Delete2DList(tp);

InitializeAction();
}


/*******************************************************************/

void InstallRequiredPath(path,freespace)

char *path;
int freespace;

{ struct Disk *ptr;
  char buffer[bufsize];

Debug1("Installing item (%s) in the required list\n",path);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",path);
   return;
   }

VBUFF[0] = '\0';
ExpandVarstring(path,VBUFF,"");

if ((ptr = (struct Disk *)malloc(sizeof(struct Disk))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallRequired() #1");
   }

if ((ptr->name = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallRequired() #2");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallRequired() #2");
   }

ExpandVarstring(ALLCLASSBUFFER,buffer,"");

if ((ptr->define = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for Installrequied() #2");
   }

ExpandVarstring(ELSECLASSBUFFER,buffer,"");

if ((ptr->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for Installrequied() #2");
   }
 
if (VREQUIRED == NULL)                 /* First element in the list */
   {
   VREQUIRED = ptr;
   }
else
   {
   VREQUIREDTOP->next = ptr;
   }

AddInstallable(ptr->define);
AddInstallable(ptr->elsedef);
 
ptr->freespace = freespace;
ptr->next = NULL;
ptr->log = LOGP;
ptr->inform = INFORMP;
ptr->done = 'n'; 

VREQUIREDTOP = ptr;

InitializeAction();
}

/*******************************************************************/

void AppendMountable(path)

char *path;

{
Debug1("Adding mountable %s to list\n",path);

if (!IsInstallable(CLASSBUFF))
   {
   return;
   }
 
ExpandVarstring(path,VBUFF,"");
 
AppendItem(&VMOUNTABLES,VBUFF,CLASSBUFF);
}

/*******************************************************************/

void AppendUmount(path,deldir,delfstab,force)

char *path;
char deldir,delfstab,force;

{ struct UnMount *ptr;
 
if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",path);
   return;
   }

VBUFF[0] = '\0';
ExpandVarstring(path,VBUFF,"");

 if ((ptr = (struct UnMount *)malloc(sizeof(struct UnMount))) == NULL)
   {
   FatalError("Memory Allocation failed for AppendUmount() #1");
   }

if ((ptr->name = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendUmount() #2");
   }
 
if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendUmount() #5");
   }

if (VUNMOUNTTOP == NULL)                 /* First element in the list */
   {
   VUNMOUNT = ptr;
   }
else
   {
   VUNMOUNTTOP->next = ptr;
   }

ptr->next = NULL;
ptr->deletedir = deldir;  /* t/f - true false */
ptr->deletefstab = delfstab;
ptr->force = force;
ptr->done = 'n'; 
VUNMOUNTTOP = ptr;
}

/*******************************************************************/

void AppendMiscMount(from,onto,opts)

char *from, *onto, *opts;

{ struct MiscMount *ptr;

Debug1("Adding misc mountable %s %s (%s) to list\n",from,onto,opts);

if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",from);
   return;
   }

if ((ptr = (struct MiscMount *)malloc(sizeof(struct MiscMount))) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount #1");
   }

ExpandVarstring(from,VBUFF,"");
 
if ((ptr->from = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #2");
   }

ExpandVarstring(onto,VBUFF,"");

if ((ptr->onto = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #3");
   }

if ((ptr->options = strdup(opts)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #4");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #5");
   }

if (VMISCMOUNTTOP == NULL)                 /* First element in the list */
   {
   VMISCMOUNT = ptr;
   }
else
   {
   VMISCMOUNTTOP->next = ptr;
   }

ptr->next = NULL;
ptr->done = 'n'; 
VMISCMOUNTTOP = ptr;
}


/*******************************************************************/

void AppendIgnore(path)

char *path;

{ struct TwoDimList *tp = NULL;
  char *sp;

Debug1("Installing item (%s) in the ignore list\n",path);

if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",path);
   return;
   }

Build2DListFromVarstring(&tp,path,'/');
    
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   AppendItem(&VIGNORE,sp,CLASSBUFF);
   }

Delete2DList(tp);
}

/*******************************************************************/

void InstallPending(action)

enum actions action;

{
if (ACTIONPENDING)
   {
   Debug1("\n   [BEGIN InstallPending %s\n",ACTIONTEXT[action]);
   }
else
   {
   Debug1("   (No actions pending in %s)\n",ACTIONTEXT[action]);
   return;
   }

switch (action)
   {
   case filters:  InstallFilterTest(FILTERNAME,CURRENTITEM,FILTERDATA);
                  break;

   case strategies:
                  AddClassToStrategy(STRATEGYNAME,CURRENTITEM,STRATEGYDATA);
		  break;
   case files:
                  InstallFileListItem(CURRENTPATH,PLUSMASK,MINUSMASK,FILEACTION,VUIDNAME,
				      VGIDNAME,VRECURSE,(char)PTRAVLINKS,CHECKSUM);
                  break;

   case processes: InstallProcessItem(EXPR,RESTART,PROMATCHES,PROCOMP,
				      PROSIGNAL,PROACTION,CLASSBUFF,USESHELL,VUIDNAME,VGIDNAME);
                   break;
   case image:
                  InstallImageItem(CURRENTPATH,PLUSMASK,MINUSMASK,DESTINATION,
				   IMAGEACTION,VUIDNAME,VGIDNAME,IMGSIZE,IMGCOMP,
				   VRECURSE,COPYTYPE,LINKTYPE,CFSERVER);
                  break;

   case tidy:     if (VAGE >= 99999)
                     {
                     yyerror("Must specify an age for tidy actions");
                     return;
                     }
                  InstallTidyItem(CURRENTPATH,CURRENTITEM,VRECURSE,VAGE,(char)PTRAVLINKS,
				  TIDYSIZE,AGETYPE,LINKDIRS,TIDYDIRS,CLASSBUFF);
                  break;

   case makepath: InstallMakePath(CURRENTPATH,PLUSMASK,MINUSMASK,VUIDNAME,VGIDNAME);
                  break;

   case disable:  AppendDisable(CURRENTPATH,CURRENTITEM,ROTATE,DISCOMP,DISABLESIZE);
                  break;

   case shellcommands:
                  AppendScript(CURRENTPATH,VTIMEOUT,USESHELL,VUIDNAME,VGIDNAME);
		  break;

   case interfaces:
                  AppendInterface(VIFNAME,DESTINATION,CURRENTPATH);
		  break;

   case disks:
   case required:
                  InstallRequiredPath(CURRENTPATH,IMGSIZE);
                  break;

   case misc_mounts:
                  if ((strlen(MOUNTFROM) != 0) && (strlen(MOUNTONTO) != 0))
		     {
		     switch (MOUNTMODE)
			{
			case 'o': strcpy(MOUNTOPTS,"ro");
				  break;
			case 'w': strcpy(MOUNTOPTS,"rw");
				  break;
			default:  printf("Install pending, miscmount, shouldn't happen\n");
				  MOUNTOPTS[0] = '\0'; /* no mount mode set! */
			}
		  
		     if (strlen(MOUNTOPTS) != 0)     /* valid mount mode set */
			{ struct Item *op;
			for (op = MOUNTOPTLIST; op != NULL; op = op->next)
			   {
			   if (BufferOverflow(MOUNTOPTS,op->name))
			      {
			      printf(" culprit: InstallPending, skipping miscmount %s %s\n",
			             MOUNTFROM, MOUNTONTO);
			      return;
			      }
			   strcat(MOUNTOPTS,",");
			   strcat(MOUNTOPTS,op->name);
			   }
			AppendMiscMount(MOUNTFROM,MOUNTONTO,MOUNTOPTS);
			}
		     }
		  
		  InitializeAction();
		  break;

   case unmounta:
                  AppendUmount(CURRENTPATH,DELETEDIR,DELETEFSTAB,FORCE);
		  break;
		   
   case links:

                  if (LINKTO[0] == '\0')
                     {
                     return;
                     }

                  if (ACTION_IS_LINKCHILDREN)
                     {
                     InstallLinkChildrenItem(LINKFROM,LINKTO);
                     ACTION_IS_LINKCHILDREN = false;
                     }
                  else if (ACTION_IS_LINK)
                     {
                     InstallLinkItem(LINKFROM,LINKTO);
                     ACTION_IS_LINK = false;
                     }
                  else
                     {
                     return;                                   /* Don't have whole command */
                     }

                  break;
   }

LINKFROM[0] = '\0';
LINKTO[0] = '\0';
ACTIONPENDING = false;
CURRENTITEM[0] = '\0';
VRECURSE = 0;
VAGE=99999;
Debug1("   END InstallPending]\n\n");
}

/*******************************************************************/
/* Level 3                                                         */
/*******************************************************************/

void HandleCharSwitch(name,value,pflag)

char *name, *value, *pflag;

{
Debug1("HandleCharSwitch(%s=%s)\n",name,value);
  
if ((strcmp(value,"true") == 0) || (strcmp(value,"on") == 0))
   {
   *pflag = 'y';
   return;
   }
 
if ((strcmp(value,"false") == 0) || (strcmp(value,"off") == 0))
   {
   *pflag = 'n';
   return;
   }

printf("Switch %s=(true/false)|(on/off)",name); 
yyerror("Illegal switch value");
}

/*******************************************************************/


int EditFileExists(file)

char *file;

{ struct Edit *ptr;

VBUFF[0]='\0';                         /* Expand any variables */
ExpandVarstring(file,VBUFF,"");

for (ptr=VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->fname,VBUFF) == 0)
      {
      return true;
      }
   }
return false;
}

/********************************************************************/

void GetExecOutput(command,buffer)

/* Buffer initially contains whole exec string */

char *command, *buffer;

{ int offset = 0;
  char line[bufsize], *sp; 
  FILE *pp;

Debug1("GetExecOutput(%s,%s)\n",command,buffer);
  
if (DONTDO)
   {
   return;
   }

if ((pp = cfpopen(command,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open pipe to command %s\n",command);
   CfLog(cfinform,OUTPUT,"pipe");
   return;
   }

bzero(buffer,bufsize);
  
while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      fflush(pp);
      break;
      }

   ReadLine(line,bufsize,pp);

   if (ferror(pp))  /* abortable */
      {
      fflush(pp);
      break;
      }	 

   for (sp = line; *sp != '\0'; sp++)
      {
      if (*sp == '\n')
	 {
	 *sp = ' ';
	 }
      }

   if (strlen(line)+offset > bufsize-10)
      {
      snprintf(OUTPUT,bufsize*2,"Buffer exceeded %d bytes in exec %s\n",maxvarsize,command);
      CfLog(cferror,OUTPUT,"");
      break;
      }

   snprintf(buffer+offset,bufsize,"%s ",line);
   offset += strlen(line)+1;
   }

if (offset > 0)
   {
   Chop(buffer); 
   }

Debug("GetExecOutput got: [%s]\n",buffer);
 
cfpclose(pp);
}

/********************************************************************/

void InstallEditFile(file,edit,data)

char *file,*edit,*data;

{ struct Edit *ptr;

if (data == NULL)
   {
   Debug1("InstallEditFile(%s,%s,-)\n",file,edit);
   }
else
   {
   Debug1("InstallEditFile(%s,%s,%s)\n",file,edit,data);
   }

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing Edit no match\n");
   return;
   }

 ExpandVarstring(file,VBUFF,"");
 
 if ((ptr = (struct Edit *)malloc(sizeof(struct Edit))) == NULL)
    {
    FatalError("Memory Allocation failed for InstallEditFile() #1");
    }
 
 if ((ptr->fname = strdup(VBUFF)) == NULL)
    {
    FatalError("Memory Allocation failed for InstallEditFile() #2");
    }
 
 if (VEDITLISTTOP == NULL)                 /* First element in the list */
    {
    VEDITLIST = ptr;
    }
 else
    {
    VEDITLISTTOP->next = ptr;
    }
 
 if (strncmp(VBUFF,"home",4) == 0 && strlen(VBUFF) < 6)
    {
    yyerror("Can't edit home directories: missing a filename after home");
    }

 if (strlen(LOCALREPOS) > 0)
    {
    ExpandVarstring(LOCALREPOS,VBUFF,"");
    ptr->repository = strdup(VBUFF);
    }
 else
    {
    ptr->repository = NULL;
    }

 ptr->done = 'n';
 ptr->recurse = 0;
 ptr->useshell = 'y';
 ptr->binary = 'n';
 ptr->next = NULL;
 ptr->actions = NULL;
 ptr->filters = NULL;
 ptr->ignores = NULL;
 ptr->umask = UMASK;
 ptr->exclusions = NULL;
 ptr->inclusions = NULL; 
 VEDITLISTTOP = ptr;
 AddEditAction(file,edit,data);
}

/********************************************************************/

void AddEditAction(file,edit,data)

char *file,*edit,*data;

{ struct Edit *ptr;
  struct Edlist *top,*new;
  char varbuff[bufsize];

if (data == NULL)
   {
   Debug2("AddEditAction(%s,%s,-)\n",file,edit);
   }
else
   {
   Debug2("AddEditAction(%s,%s,%s)\n",file,edit,data);
   }

if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing Edit no match\n");
   return;
   }

for (ptr = VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   varbuff[0] = '\0';
   ExpandVarstring(file,varbuff,"");

   if (strcmp(ptr->fname,varbuff) == 0)
      {
      if ((new = (struct Edlist *)malloc(sizeof(struct Edlist))) == NULL)
         {
         FatalError("Memory Allocation failed for AddEditAction() #1");
         }

      if (ptr->actions == NULL)
         {
         ptr->actions = new;
         }
      else
         {
         for (top = ptr->actions; top->next != NULL; top=top->next)
            {
            }
         top->next = new;
         }

      if (data == NULL)
         {
         new->data = NULL;
         }
      else
         {
         VBUFF[0]='\0';                         /* Expand any variables */
         ExpandVarstring(data,VBUFF,"");

         if ((new->data = strdup(VBUFF)) == NULL)
            {
            FatalError("Memory Allocation failed for AddEditAction() #1");
            }
         }

      new->next = NULL;

      if ((new->classes = strdup(CLASSBUFF)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallEditFile() #3");
         }

      if ((new->code = EditActionsToCode(edit)) == NoEdit)
         {
         yyerror("Unknown edit action");
         }

      switch(new->code)
         {
	 case EditUmask:
	     HandleUmask(data);
	 case EditIgnore:
	     PrependItem(&(ptr->ignores),data,CF_ANYCLASS);
	     break;
	 case EditExclude:
	     PrependItem(&(ptr->exclusions),data,CF_ANYCLASS);
	     break;
	 case EditInclude:
	     PrependItem(&(ptr->inclusions),data,CF_ANYCLASS);
	     break;
	 case EditRecurse:
	     if (strcmp(data,"inf") == 0)
		{
		ptr->recurse = INFINITERECURSE;
		}
	     else
		{
		ptr->recurse = atoi(data);
		if (ptr->recurse < 0)
		   {
		   yyerror("Illegal recursion value");
		   }
		}
	       break;

	 case Append:
	     if (EDITGROUPLEVEL == 0)
		{
		yyerror("Append used outside of Group - non-convergent");
		}

	 case EditMode:
	     if (strcmp(data,"Binary") == 0)
		{
		ptr->binary = 'y';
		}
	     break;

         case BeginGroupIfNoMatch:
         case BeginGroupIfNoLineMatching:
         case BeginGroupIfNoLineContaining:
         case BeginGroupIfNoSuchLine:
	 case BeginGroupIfDefined:
         case BeginGroupIfNotDefined: 
	 case BeginGroupIfFileIsNewer:
	 case BeginGroupIfFileExists:
                EDITGROUPLEVEL++;
                break;
         case EndGroup:
                EDITGROUPLEVEL--;
		if (EDITGROUPLEVEL < 0)
		   {
		   yyerror("EndGroup without Begin");
		   }
                break;
         case ReplaceAll:
	        if (SEARCHREPLACELEVEL > 0)
		   {
		   yyerror("ReplaceAll without With before or at line");
		   }
		
                SEARCHREPLACELEVEL++;
                break;
         case With:
                SEARCHREPLACELEVEL--;
                break;
	 case ForEachLineIn:
	        if (FOREACHLEVEL > 0)
		   {
		   yyerror("Nested ForEach loops not allowed");
		   }
		
	        FOREACHLEVEL++;
		break;
	 case EndLoop:
	        FOREACHLEVEL--;
		if (FOREACHLEVEL < 0)
		   {
		   yyerror("EndLoop without ForEachLineIn");
		   }
		break;
	 case SetLine:
	        if (FOREACHLEVEL > 0)
		   {
		   yyerror("SetLine inside ForEachLineIn loop");
		   }
	        break;
	 case FixEndOfLine:
	        if (strlen(data) > extra_space - 1)
		   {
		   yyerror("End of line type is too long!");
		   printf("          (max %d characters allowed)\n",extra_space);
		   }
		break;
	 case ReplaceLinesMatchingField:
	        if (atoi(data) == 0)
		   {
		   yyerror("Argument must be an integer, greater than zero");
		   }
		break;
	 case ElseDefineClasses:
	 case EditFilter:
	 case DefineClasses:
	     AddInstallable(new->data);
	     break;
	 case EditRepos:
	     ptr->repository = strdup(data);
	     break;
         }

      return;
      }
   }

printf("cfengine: software error - no file matched installing %s edit\n",file);
}

/********************************************************************/

enum editnames EditActionsToCode(edit)

char *edit;

{ int i;

Debug2("EditActionsToCode(%s)\n",edit);

for (i = 0; VEDITNAMES[i] != '\0'; i++)
   {
   if (strcmp(VEDITNAMES[i],edit) == 0)
      {
      return (enum editnames) i;
      }
   }

return (NoEdit);
}


/********************************************************************/

void AppendInterface(ifname,netmask,broadcast)

char *ifname,*netmask,*broadcast;

{ struct Interface *ifp;
 
Debug1("Installing item (%s:%s:%s) in the interfaces list\n",ifname,netmask,broadcast);

if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",ifname);
   return;
   }

if (strlen(netmask) < 7)
   {
   yyerror("illegal or missing netmask");
   InitializeAction();
   return;
   }

if (strlen(broadcast) < 3)
   {
   yyerror("illegal or missing broadcast address");
   InitializeAction();
   return;
   }
 
if ((ifp = (struct Interface *)malloc(sizeof(struct Interface))) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #1");
   }

if ((ifp->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for Appendinterface() #2");
   }

if ((ifp->ifdev = strdup(ifname)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   }
 
if ((ifp->netmask = strdup(netmask)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   }

if ((ifp->broadcast = strdup(broadcast)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   } 

if (VIFLISTTOP == NULL)                 /* First element in the list */
   {
   VIFLIST = ifp;
   }
 else
    {
    VIFLISTTOP->next = ifp;
    }

ifp->next = NULL;
ifp->done = 'n';
 
VIFLISTTOP = ifp;
 
InitializeAction(); 
}

/*******************************************************************/

void AppendScript(item,timeout,useshell,uidname,gidname)

char *item, useshell,*uidname,*gidname;
int timeout;

{ struct TwoDimList *tp = NULL;
  struct ShellComm *ptr;
  struct passwd *pw;
  struct group *gw;
  char *sp, buf[bufsize];
  int uid = CF_NOUSER; 
  int gid = CF_NOUSER;
  
Debug1("Installing item (%s) in the script list\n",item);

if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match (%s)\n",item,CLASSBUFF);
   InitializeAction();
   return;
   }

Build2DListFromVarstring(&tp,item,' '); /* Must be at least one space between each var */

Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if (*sp != '/')
      {
      yyerror("scripts or commands must have absolute path names");
      printf ("cfengine: concerns: %s\n",sp);
      return;
      }

   if ((ptr = (struct ShellComm *)malloc(sizeof(struct ShellComm))) == NULL)
      {
      FatalError("Memory Allocation failed for AppendScript() #1");
      }

   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscript() #2");
      }

   if ((ptr->name = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscript() #3");
      }

   ExpandVarstring(CHROOT,buf,"");

   if ((ptr->chroot = strdup(buf)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscipt() #4b");
      }
   
   ExpandVarstring(CHDIR,buf,"");
   
   if ((ptr->chdir = strdup(buf)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscript() #4c");
      }
   
   if (VSCRIPTTOP == NULL)                 /* First element in the list */
      {
      VSCRIPT = ptr;
      }
   else
      {
      VSCRIPTTOP->next = ptr;
      }


   if (*uidname == '*')
      {
      ptr->uid = -1;      
      }
   else if (isdigit((int)*uidname))
      {
      sscanf(uidname,"%d",&uid);
      if (uid == CF_NOUSER)
	 {
	 yyerror("Unknown or silly user id");
	 return;
	 }
      else
	 {
	 ptr->uid = uid;
	 }
      }
   else if ((pw = getpwnam(uidname)) == NULL)
      {
      yyerror("Unknown or silly user id");
      return;
      }
   else
      {
      ptr->uid = pw->pw_uid;
      }

   if (*gidname == '*')
      {
      ptr->gid = -1;
      }
   else if (isdigit((int)*gidname))
      {
      sscanf(gidname,"%d",&gid);
      if (gid == CF_NOUSER)
	 {
	 yyerror("Unknown or silly group id");
	 continue;
	 }
      else
	 {
	 ptr->gid = gid;
	 }
      }
   else if ((gw = getgrnam(gidname)) == NULL)
      {
      yyerror("Unknown or silly group id");
      continue;
      }
   else
      {
      ptr->gid = gw->gr_gid;
      }
 
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->timeout = timeout;
   ptr->useshell = useshell;
   ptr->umask = UMASK;
   ptr->fork = FORK;
   ptr->preview = PREVIEW;
   ptr->next = NULL;
   ptr->done = 'n';

   ExpandVarstring(ALLCLASSBUFFER,VBUFF,"");
   
   if ((ptr->defines = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      }

   ExpandVarstring(ELSECLASSBUFFER,VBUFF,"");
   
   if ((ptr->elsedef = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);
   VSCRIPTTOP = ptr;
   }

Delete2DList(tp);
}

/********************************************************************/

void AppendDisable(path,type,rotate,comp,size)

char *path, *type, comp;
short rotate;
int size;

{ char *sp;
  struct Disable *ptr;
  struct TwoDimList *tp = NULL;
 
Debug1("Installing item (%s) in the disable list\n",path);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",path);
   return;
   }

Build2DListFromVarstring(&tp,path,' '); /* Must be at least one space between each var */

Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if (strlen(type) > 0 && strcmp(type,"plain") != 0 && strcmp(type,"file") !=0 && strcmp(type,"link") !=0
       && strcmp(type,"links") !=0 )
      {
      yyerror("Invalid file type in Disable");
      }

   if ((ptr = (struct Disable *)malloc(sizeof(struct Disable))) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #1");
      }
   
   if ((ptr->name = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #2");
      }
   
   ExpandVarstring(ALLCLASSBUFFER,VBUFF,"");

   if ((ptr->defines = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      } 

   ExpandVarstring(ELSECLASSBUFFER,VBUFF,"");

   if ((ptr->elsedef = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      } 
      
   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #4");
      }
   
   if (strlen(type) == 0)
      {
      sprintf(VBUFF,"all");
      }
   else
      {
      sprintf(VBUFF,"%s",type);
      }
   
   if ((ptr->type = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #4");
      }
   
   if (VDISABLETOP == NULL)                 /* First element in the list */
      {
      VDISABLELIST = ptr;
      }
   else
      {
      VDISABLETOP->next = ptr;
      }

   if (strlen(LOCALREPOS) > 0)
      {
      ExpandVarstring(LOCALREPOS,VBUFF,"");
      ptr->repository = strdup(VBUFF);
      }
   else
      {
      ptr->repository = NULL;
      }
   
   ptr->rotate = rotate;
   ptr->comp = comp;
   ptr->size = size;
   ptr->next = NULL;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->done = 'n';
   ptr->action = PROACTION;
   
   VDISABLETOP = ptr;
   InitializeAction();
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);
   }
 
 Delete2DList(tp);  
}

/*******************************************************************/

void InstallTidyItem(path,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes)

char *wild, *path;
short age,tidydirs;
int rec,tidysize;
char type, ldirs, *classes, travlinks;

{ struct TwoDimList *tp = NULL;
  char *sp;

if (strcmp(path,"/") != 0)
   {
   DeleteSlash(path);
   }

Build2DListFromVarstring(&tp,path,'/');

Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if (TidyPathExists(sp))
      {
      AddTidyItem(sp,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
      }
   else
      {
      InstallTidyPath(sp,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
      }
   }

Delete2DList(tp);
InitializeAction();
}

/*******************************************************************/

void InstallMakePath(path,plus,minus,uidnames,gidnames)

char *path;
mode_t plus,minus;
char *uidnames;
char *gidnames;

{ struct File *ptr;
  char buffer[bufsize]; 

Debug1("InstallMakePath (%s) (+%o)(-%o)(%s)(%s)\n",path,plus,minus,uidnames,gidnames);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing directory item, no match\n");
   return;
   }

VBUFF[0]='\0';                                /* Expand any variables */
ExpandVarstring(path,VBUFF,"");

if ((ptr = (struct File *)malloc(sizeof(struct File))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMakepath() #1");
   }

if ((ptr->path = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMakepath() #2");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMakepath() #3");
   }

ExpandVarstring(ALLCLASSBUFFER,buffer,""); 
 
if ((ptr->defines = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMakepath() #3a");
   }

ExpandVarstring(ELSECLASSBUFFER,buffer,""); 

if ((ptr->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMakepath() #3a");
   }

AddInstallable(ptr->defines);
AddInstallable(ptr->elsedef);  
 
if (VMAKEPATHTOP == NULL)                 /* First element in the list */
   {
   VMAKEPATH = ptr;
   }
else
   {
   VMAKEPATHTOP->next = ptr;
   }

ptr->plus = plus;
ptr->minus = minus;
ptr->recurse = 0;
ptr->action = fixdirs;
ptr->uid = MakeUidList(uidnames);
ptr->gid = MakeGidList(gidnames);
ptr->inclusions = NULL;
ptr->exclusions = NULL;
ptr->acl_aliases= VACLBUILD; 
ptr->log = LOGP;
ptr->filters = NULL;
ptr->inform = INFORMP;
ptr->plus_flags = PLUSFLAG;
ptr->minus_flags = MINUSFLAG;
ptr->done = 'n'; 
 
ptr->next = NULL;
VMAKEPATHTOP = ptr;
InitializeAction();
}

/*******************************************************************/

void HandleTravLinks(value)

char *value;

{
if (ACTION == tidy && strncmp(CURRENTPATH,"home",4) == 0)
   {
   yyerror("Can't use links= option with special variable home in tidy");
   yyerror("Use command line options instead.\n");
   }

if (PTRAVLINKS != '?')
   {
   Warning("redefinition of links= option");
   }

if ((strcmp(value,"stop") == 0) || (strcmp(value,"false") == 0))
   {
   PTRAVLINKS = (short) 'F';
   return;
   }

if ((strcmp(value,"traverse") == 0) || (strcmp(value,"follow") == 0) || (strcmp(value,"true") == 0))
   {
   PTRAVLINKS = (short) 'T';
   return;
   }

if ((strcmp(value,"tidy"))==0)
   {
   PTRAVLINKS = (short) 'K';
   return;
   }

yyerror("Illegal links= specifier");
}

/*******************************************************************/

void HandleTidySize(value)

char *value;

{ int num = -1;
  char *sp, units = 'k';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

if (strcmp(value,"empty") == 0)
   {
   TIDYSIZE = CF_EMPTYFILE;
   }
else
   {
   sscanf(value,"%d%c",&num,&units);

   if (num <= 0)
      {
      if (*value == '>')
	 {
	 sscanf(value+1,"%d%c",&num,&units);
	 if (num <= 0)
	    {
	    yyerror("size value must be a decimal number with units m/b/k");
	    }
	 }
      else
	 {
	 yyerror("size value must be a decimal number with units m/b/k");
	 }
      }

   switch (units)
      {
      case 'b': TIDYSIZE = num;
	        break;
      case 'm': TIDYSIZE = num * 1024 * 1024;
	        break;
      default:  TIDYSIZE = num * 1024;
      }
   }

}

/*******************************************************************/

void HandleUmask(value)

char *value;

{ mode_t num = -1;

Debug("HandleUmask(%s)",value);
 
sscanf(value,"%o",&num);

if (num <= 0)
   {
   yyerror("umask value must be an octal number >= zero");
   }

UMASK = num;
}

/*******************************************************************/

void HandleDisableSize(value)

char *value;

{ int i = -1;
  char *sp, units = 'b';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

switch (*value)
   {
   case '>': DISCOMP = '>';
             value++;
             break;
   case '<': DISCOMP = '<';
             value++;
             break;
   default : DISCOMP = '=';
   }

sscanf(value,"%d%c",&i,&units);

if (i < 1)
   {
   yyerror("disable size attribute with silly value (must be > 0)");
   }

switch (units)
   {
   case 'k': DISABLESIZE = i * 1024;
             break;
   case 'm': DISABLESIZE = i * 1024 * 1024;
             break;
   default:  DISABLESIZE = i;
   }
}

/*******************************************************************/

void HandleCopySize(value)

char *value;

{ int i = -1;
  char *sp, units = 'b';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

switch (*value)
   {
   case '>': IMGCOMP = '>';
             value++;
             break;
   case '<': IMGCOMP = '<';
             value++;
             break;
   default : IMGCOMP = '=';
   }

sscanf(value,"%d%c",&i,&units);

if (i < 0)
   {
   yyerror("copy size attribute with silly value (must be a non-negative number)");
   }

switch (units)
   {
   case 'k': IMGSIZE = i * 1024;
             break;
   case 'm': IMGSIZE = i * 1024 * 1024;
             break;
   default:  IMGSIZE = i;
   }
}

/*******************************************************************/

void HandleRequiredSize(value)

char *value;

{ int i = -1;
  char *sp, units = 'b';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

switch (*value)
   {
   case '>': IMGCOMP = '>';
             value++;
             break;
   case '<': IMGCOMP = '<';
             value++;
             break;
   default : IMGCOMP = '=';
   }

sscanf(value,"%d%c",&i,&units);

if (i < 1)
   {
   yyerror("disk/required size attribute with silly value (must be > 0)");
   }

switch (units)
   {
   case 'b': IMGSIZE = i / 1024;
             break;
   case 'm': IMGSIZE = i * 1024;
             break;
   case '%': IMGSIZE = -i;       /* -ve number signals percentage */
             break;
   default:  IMGSIZE = i;
   }
}

/*******************************************************************/

void HandleTidyType(value)

char *value;

{
if (strcmp(value,"a")== 0 || strcmp(value,"atime") == 0)
   {
   AGETYPE = 'a';
   return;
   }

if (strcmp(value,"m")== 0 || strcmp(value,"mtime") == 0)
   {
   AGETYPE = 'm';
   return;
   }

if (strcmp(value,"c")== 0 || strcmp(value,"ctime") == 0)
   {
   AGETYPE = 'c';
   return;
   }

yyerror("Illegal age search type, must be atime/ctime/mtime");
}

/*******************************************************************/

void HandleTidyLinkDirs(value)

char *value;

{
if (strcmp(value,"keep")== 0)
   {
   LINKDIRS = 'k';
   return;
   }

if ((strcmp(value,"tidy")== 0) || (strcmp(value,"delete") == 0))
   {
   LINKDIRS = 'y';
   return;
   }

yyerror("Illegal linkdirs value, must be keep/delete/tidy");
}

/*******************************************************************/

void HandleTidyRmdirs(value)

char *value;

{
if ((strcmp(value,"true") == 0)||(strcmp(value,"all") == 0))
   {
   TIDYDIRS = 1;
   return;
   }

if ((strcmp(value,"false") == 0)||(strcmp(value,"none") == 0))
   {
   TIDYDIRS = 0;
   return;
   }

if (strcmp(value,"sub") == 0)
   {
   TIDYDIRS = 2;
   return;
   }

yyerror("Illegal rmdirs value, must be true/false/sub");
}

/*******************************************************************/

void HandleTimeOut(value)

char *value;

{ int num = -1;

sscanf(value,"%d",&num);

if (num <= 0)
   {
   yyerror("timeout value must be a decimal number > 0");
   }

VTIMEOUT = num;
}


/*******************************************************************/

void HandleUseShell(value)

char *value;

{
 if (strcmp(value,"true") == 0)
   {
   USESHELL = 'y';
   return;
   }

if (strcmp(value,"false") == 0)
   {
   USESHELL = 'n';
   return;
   }

if (strcmp(value,"dumb") == 0)
   {
   USESHELL = 'd';
   return;
   }

yyerror("Illegal attribute for useshell= ");
}

/*******************************************************************/

void HandleChecksum(value)

char *value;

{
if (strcmp(value,"md5") == 0)
   {
   CHECKSUM = 'm';
   return;
   }

if (strncmp(value,"sha1",strlen(value)) == 0)
   {
   CHECKSUM = 's';
   return;
   } 

yyerror("Illegal attribute for checksum= ");
}

/*******************************************************************/

void HandleTimeStamps(value)

char *value;

{
if (strcmp(value,"preserve") == 0 || strcmp(value,"keep") == 0)
   {
   PRESERVETIMES = 'y';
   return;
   }

PRESERVETIMES = 'n';
}

/*******************************************************************/

int GetFileAction(action)

char *action;

{ int i;

for (i = 0; FILEACTIONTEXT[i] != '\0'; i++)
   {
   if (strcmp(action,FILEACTIONTEXT[i]) == 0)
      {
      return i;
      }
   }

yyerror("Unknown action type");
return (int) warnall;
}


/*******************************************************************/

void InstallFileListItem(path,plus,minus,action,uidnames,gidnames,recurse,travlinks,chksum)

char *path;
mode_t plus,minus;
enum fileactions action;
char *uidnames;
char *gidnames;
int recurse;
char travlinks,chksum;

{ struct File *ptr;
  char *spl;
  struct TwoDimList *tp = NULL;

Debug1("InstallFileaction (%s) (+%o)(-%o) (%s) (%d) (%c)\n",path,plus,minus,FILEACTIONTEXT[action],action,travlinks);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing file item, no match\n");
   return;
   }

 
Build2DListFromVarstring(&tp,path,'/');
    
Set2DList(tp);

for (spl = Get2DListEnt(tp); spl != NULL; spl = Get2DListEnt(tp))
   {
   if ((ptr = (struct File *)malloc(sizeof(struct File))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #1");
      }

   if ((ptr->path = strdup(spl)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #2");
      }

   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #3");
      }

   ExpandVarstring(ALLCLASSBUFFER,VBUFF,""); 

   if ((ptr->defines = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #3");
      }

   ExpandVarstring(ELSECLASSBUFFER,VBUFF,""); 

   if ((ptr->elsedef = strdup(VBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #3");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);   

   if (VFILETOP == NULL)                 /* First element in the list */
      {
      VFILE = ptr;
      }
   else
      {
      VFILETOP->next = ptr;
      }

   ptr->action = action;
   ptr->plus = plus;
   ptr->minus = minus;
   ptr->recurse = recurse;
   ptr->uid = MakeUidList(uidnames);
   ptr->gid = MakeGidList(gidnames);
   ptr->exclusions = VEXCLUDEPARSE;
   ptr->inclusions = VINCLUDEPARSE;
   ptr->filters = NULL;
   ptr->ignores = VIGNOREPARSE;
   ptr->travlinks = travlinks;
   ptr->acl_aliases  = VACLBUILD;
   ptr->filters = VFILTERBUILD;
   ptr->next = NULL;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->checksum = chksum;
   ptr->plus_flags = PLUSFLAG;
   ptr->minus_flags = MINUSFLAG;
   ptr->done = 'n';

   VFILETOP = ptr;
   Debug1("Installed file object %s\n",spl);
   }

Delete2DList(tp);

InitializeAction();
}


/*******************************************************************/

void InstallProcessItem(expr,restart,matches,comp,signal,action,classes,useshell,uidname,gidname)

char *expr, *restart, *classes, *uidname, *gidname;
short matches, signal;
char action, comp, useshell;

{ struct Process *ptr;
  char buf[bufsize];
  uid_t uid;
  gid_t gid;
  struct passwd *pw;
  struct group *gw;
  

Debug1("InstallProcessItem(%s,%s,%d,%d,%c)\n",expr,restart,matches,signal,action);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing process item, no match\n");
   return;
   }

if ((ptr = (struct Process *)malloc(sizeof(struct Process))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #1");
   }

ExpandVarstring(expr,buf,"");
 
if ((ptr->expr = strdup(buf)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #2");
   }

ExpandVarstring(restart,buf,"");
 
if ((ptr->restart = strdup(buf)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #3");
   }

ExpandVarstring(ALLCLASSBUFFER,buf,""); 
 
if ((ptr->defines = strdup(buf)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4");
   }

ExpandVarstring(ELSECLASSBUFFER,buf,""); 

if ((ptr->elsedef = strdup(buf)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4a");
   }

ExpandVarstring(CHROOT,buf,"");
 
if ((ptr->chroot = strdup(buf)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4b");
   }

ExpandVarstring(CHDIR,buf,"");
 
if ((ptr->chdir = strdup(buf)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4c");
   }
 
if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #5");
   }

if (VPROCTOP == NULL)                 /* First element in the list */
   {
   VPROCLIST = ptr;
   }
else
   {
   VPROCTOP->next = ptr;
   }

ptr->matches = matches;
ptr->comp = comp;
ptr->signal = signal;
ptr->action = action;
ptr->umask = UMASK;
ptr->useshell = useshell; 
ptr->next = NULL;
ptr->log = LOGP;
ptr->inform = INFORMP;
ptr->exclusions = VEXCLUDEPARSE;
ptr->inclusions = VINCLUDEPARSE;
ptr->filters = ptr->filters = VFILTERBUILD;
ptr->done = 'n';

   if (*uidname == '*')
      {
      ptr->uid = -1;      
      }
   else if (isdigit((int)*uidname))
      {
      sscanf(uidname,"%d",&uid);
      if (uid == CF_NOUSER)
	 {
	 yyerror("Unknown or silly restart user id");
	 return;
	 }
      else
	 {
	 ptr->uid = uid;
	 }
      }
   else if ((pw = getpwnam(uidname)) == NULL)
      {
      yyerror("Unknown or silly restart user id");
      return;
      }
   else
      {
      ptr->uid = pw->pw_uid;
      }

   if (*gidname == '*')
      {
      ptr->gid = -1;
      }
   else if (isdigit((int)*gidname))
      {
      sscanf(gidname,"%d",&gid);
      if (gid == CF_NOUSER)
	 {
	 yyerror("Unknown or silly restart group id");
	 return;
	 }
      else
	 {
	 ptr->gid = gid;
	 }
      }
   else if ((gw = getgrnam(gidname)) == NULL)
      {
      yyerror("Unknown or silly restart group id");
      return;
      }
   else
      {
      ptr->gid = gw->gr_gid;
      }

VPROCTOP = ptr;
InitializeAction();
AddInstallable(ptr->defines);
AddInstallable(ptr->elsedef);  
}

/*******************************************************************/

void InstallImageItem(path,plus,minus,destination,action,uidnames,gidnames,
		 size,comp,rec,type,lntype,server)

char *path, *destination, *action, *server;
mode_t plus,minus;
char *uidnames;
char *gidnames;
char type, lntype, comp;
int rec, size;

{ struct Image *ptr;
  char *spl; 
  char buf1[bufsize], buf2[bufsize], buf3[bufsize], buf4[bufsize];
  struct TwoDimList *tp = NULL;
  struct hostent *hp;
  
if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing copy item, no match\n");
   return;
   }
  
Debug1("InstallImageItem (%s) (+%o)(-%o) (%s), server=%s\n",path,plus,minus,destination,server);

if (strlen(action) == 0)   /* default action */
   {
   strcat(action,"fix");
   }

if (!(strcmp(action,"silent") == 0 || strcmp(action,"warn") == 0 || strcmp(action,"fix") == 0))
   {
   sprintf(VBUFF,"Illegal action in image/copy item: %s",action);
   yyerror(VBUFF);
   return;
   }

ExpandVarstring(path,buf1,"");
ExpandVarstring(server,buf3,"");

if (strlen(buf1) > 1)
   {
   DeleteSlash(buf1);
   }

if (!FORCENETCOPY && ((strcmp(buf3,VFQNAME) == 0) || (strcmp(buf3,VUQNAME) == 0) || (strcmp(buf3,VSYSNAME.nodename) == 0)))
   {
   Debug("Swapping %s for localhost\n",server);
   strcpy(buf3,"localhost");
   }

Build2DListFromVarstring(&tp,path,'/');  /* Must split on space in comm string */
    
Set2DList(tp);

 
for (spl = Get2DListEnt(tp); spl != NULL; spl = Get2DListEnt(tp))
   {
   if ((ptr = (struct Image *)malloc(sizeof(struct Image))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #1");
      }
   
   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #3");
      }
   
   if (strlen(buf3) > 128)
      {
      yyerror("Server name is too long");
      return;
      }
   
   if ((ptr->server = strdup(buf3)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #5");
      }
   
   if ((ptr->action = strdup(action)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #6");
      }
   
   ExpandVarstring(ALLCLASSBUFFER,buf4,"");
   
   if ((ptr->defines = strdup(buf4)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #7");
      }
   
   ExpandVarstring(ELSECLASSBUFFER,buf4,"");
   
   if ((ptr->elsedef = strdup(buf4)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #7");
      }
   
   if (strlen(destination) == 0)
      {
      strcpy(buf2,spl);
      }
   else
      {
      ExpandVarstring(destination,buf2,"");
      }

   if (strlen(buf2) > 1)
      {
      DeleteSlash(buf2);
      }
   
   if (!IsAbsoluteFileName(buf2))
      {
      if (strncmp(buf2,"home",4) == 0)
	 {
	 if (strlen(buf2) > 4 && buf2[4] != '/')
	    {
	    yyerror("illegal use of home or not absolute pathname");
	    return;
	    }
	 }
      else
	 {
	 yyerror("Image needs an absolute pathname");
	 return;
	 }
      }

   if ((ptr->destination = strdup(buf2)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #4");
      }

   if (IsDefinedClass(CLASSBUFF))
      {
      if ((strcmp(spl,buf2) == 0) && (strcmp(buf3,"localhost") == 0))
	 {
	 yyerror("image loop: file/dir copies to itself or missing destination file");
	 return;
	 }
      }

   if ((ptr->path = strdup(spl)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallImageItem() #2");
      }
      
   if (VIMAGETOP == NULL)
      {
      VIMAGE = ptr;
      }
   else
      {
      VIMAGETOP->next = ptr;
      }

   ptr->plus = plus;
   ptr->minus = minus;
   ptr->uid = MakeUidList(uidnames);
   ptr->gid = MakeGidList(gidnames);
   ptr->force = FORCE;
   ptr->forcedirs = FORCEDIRS;
   ptr->next = NULL;
   ptr->backup = IMAGEBACKUP;
   ptr->done = 'n';
   
   if (strlen(LOCALREPOS) > 0)
      {
      ExpandVarstring(LOCALREPOS,buf2,"");
      ptr->repository = strdup(buf2);
      }
   else
      {
      ptr->repository = NULL;
      }
   
   ptr->recurse = rec;
   ptr->type = type;
   ptr->stealth = STEALTH;
   ptr->preservetimes = PRESERVETIMES;
   ptr->encrypt = ENCRYPT;
   ptr->verify = VERIFY;
   ptr->size = size;
   ptr->comp = comp;
   ptr->linktype = lntype;
   ptr->symlink = VCPLNPARSE;
   ptr->exclusions = VEXCLUDEPARSE;
   ptr->inclusions = VINCLUDEPARSE;
   ptr->filters = VFILTERBUILD;
   ptr->ignores = VIGNOREPARSE;
   ptr->cache = NULL;
   ptr->purge = PURGE;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->plus_flags = PLUSFLAG;
   ptr->minus_flags = MINUSFLAG;
   ptr->typecheck = TYPECHECK;
   ptr->trustkey = TRUSTKEY;
   ptr->compat = COMPATIBILITY;
   ptr->forceipv4 = FORCEIPV4;

   if (ptr->compat == 'y' && ptr->encrypt == 'y')
      {
      yyerror("You cannot mix version1 compatibility with encrypted transfers");
      return;
      }
   
   if (ptr->purge == 'y' && strcmp(buf3,"localhost") == 0)
      {
      Verbose("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      Verbose("!! Purge detected in local (non-cfd) file copy to file %s\n",ptr->destination);
      Verbose("!! Do not risk purge if source %s is NFS mounted (see manual)\n",ptr->path);
      Verbose("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      }

   ptr->acl_aliases = VACLBUILD;
   
   if (strcmp(ptr->server,"localhost") != 0)
      {
      if ((hp = gethostbyname(buf3)) == NULL)
	 {
	 yyerror("DNS lookup failure. Unknown host");
	 printf("Culprit: %s\n",server);
	 printf("Make sure that fully qualified names can be looked up at your site!\n");
	 printf("i.e. prep.ai.mit.edu, not just prep. If you use NIS or /etc/hosts\n");
	 printf("make sure that the full form is registered too as an alias!\n");
	 perror("gethostbyname: ");
	 exit(1);
	 }

      ptr->dns = (struct in_addr *) malloc(sizeof(struct in_addr));
      
      bcopy((hp->h_addr),(ptr->dns),sizeof(struct in_addr));
      }
   else
      {
      ptr->dns = NULL;
      }
   
   ptr->inode_cache = NULL;
   
   VIMAGETOP = ptr;

   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);

   if (!IsItemIn(VSERVERLIST,ptr->server))   /* cache list of servers */
      {
      AppendItem(&VSERVERLIST,ptr->server,NULL);
      }
   }

/* Add to possible classes so actions will be installed */

   
Delete2DList(tp);

InitializeAction();
}

/*******************************************************************/

void InstallAuthItem(path,attribute,list,listtop,classes)

char *path, *attribute, *classes;
struct Auth **list, **listtop;

{ struct TwoDimList *tp = NULL;
  char *sp;

Debug1("InstallAuthItem(%s,%s)\n",path,attribute);

Build2DListFromVarstring(&tp,path,'/');
   
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if (AuthPathExists(sp,*list))
      {
      AddAuthHostItem(sp,attribute,classes,list);
      }
   else
      {
      InstallAuthPath(sp,attribute,classes,list,listtop);
      }
   }

Delete2DList(tp);
InitializeAction();
}

/*******************************************************************/

int GetCommAttribute(s)

char *s;

{ int i;
  char comm[maxvarsize];

for (i = 0; s[i] != '\0'; i++)
   {
   s[i] = ToLower(s[i]);
   }

comm[0]='\0';

sscanf(s,"%[^=]",comm);

Debug1("GetCommAttribute(%s)\n",comm);

for (i = 0; COMMATTRIBUTES[i] != NULL; i++)
   {
   if (strncmp(COMMATTRIBUTES[i],comm,strlen(comm)) == 0)
      {
      Debug1("GetCommAttribute - got: %s\n",COMMATTRIBUTES[i]);
      return i;
      }
   }

return cfbad;
}

/*******************************************************************/

void HandleRecurse(value)

char *value;

{ int n = -1;

if (strcmp(value,"inf") == 0)
   {
   VRECURSE = INFINITERECURSE;
   }
else
   {
   /* Bas
   if (strncmp(CURRENTPATH,"home",4) == 0)
      {
      Bas
      yyerror("Recursion is always infinite for home");
      return;
      }
   */

   sscanf(value,"%d",&n);

   if (n == -1)
      {
      yyerror("Illegal recursion specifier");
      }
   else
      {
      VRECURSE = n;
      }
   }
}

/*******************************************************************/

void HandleCopyType(value)

char *value;

{
if (strcmp(value,"ctime") == 0)
   {
   Debug1("Set copy by ctime\n");
   COPYTYPE = 't';
   return;
   }
else if (strcmp(value,"mtime") == 0)
   {
   Debug1("Set copy by mtime\n");
   COPYTYPE = 'm';
   return;
   }
 else if (strcmp(value,"checksum")==0 || strcmp(value,"sum") == 0)
   {
   Debug1("Set copy by md5 checksum\n");
   COPYTYPE = 'c';
   return;
   }
else if (strcmp(value,"byte")==0 || strcmp(value,"binary") == 0)
   {
   Debug1("Set copy by byte comaprison\n");
   COPYTYPE = 'b';
   return;
   }
yyerror("Illegal copy type");
}

/*******************************************************************/

void HandleDisableFileType(value)

char *value;

{
if (strlen(CURRENTITEM) != 0)
   {
   Warning("Redefinition of filetype in disable");
   }

if (strcmp(value,"link") == 0 || strcmp(value,"links") == 0)
   {
   strcpy(CURRENTITEM,"link");
   }
else if (strcmp(value,"plain") == 0 || strcmp(value,"file") == 0)
   {
   strcpy(CURRENTITEM,"file");
   }
else
   {
   yyerror("Disable filetype unknown");
   }
}

/*******************************************************************/

void HandleDisableRotate(value)

char *value;

{ int num = 0;

if (strcmp(value,"empty") == 0 || strcmp(value,"truncate") == 0)
   {
   ROTATE = CF_TRUNCATE;
   }
else
   {
   sscanf(value,"%d",&num);

   if (num == 0)
      {
      yyerror("disable/rotate value must be a decimal number greater than zero or keyword empty");
      }

   if (! SILENT && num > 99)
      {
      Warning("rotate value looks silly");
      }

   ROTATE = (short) num;
   }
}

/*******************************************************************/

void HandleAge(days)

char *days;

{ 
sscanf(days,"%d",&VAGE);
Debug1("HandleAge(%d)\n",VAGE);
}

/*******************************************************************/

void HandleProcessMatches(value)

char *value;

{ int i = -1;

switch (*value)
   {
   case '>': PROCOMP = '>';
             value++;
             break;
   case '<': PROCOMP = '<';
             value++;
             break;
   default : PROCOMP = '=';
   }

sscanf(value,"%d",&i);

if (i < 0) 
   {
   yyerror("matches attribute with silly value (must be >= 0)");
   }

PROMATCHES = (short) i;
}

/*******************************************************************/

void HandleProcessSignal(value)

char *value;

{ int i;
  char *sp;

for (i = 1; SIGNALS[i] != 0; i++)
   {
   for (sp = value; *sp != '\0'; sp++)
      {
      *sp = ToUpper(*sp);
      }
   
   if (strcmp(SIGNALS[i]+3,value) == 0)  /* 3 to cut off "sig" */
      {
      PROSIGNAL = (short) i;
      return;
      }
   }

i = 0;

sscanf(value,"%d",&i);

if (i < 1 && i > highest_signal)
   {
   yyerror("Unknown signal in attribute");
   }

PROSIGNAL = (short) i;
}

/*******************************************************************/

void HandleNetmask(value)

char *value;

{
 if (strlen(DESTINATION) == 0)
    {
    strcpy(DESTINATION,value);
    }
 else
    {
    yyerror("redefinition of netmask");
    }
}

/*******************************************************************/

void HandleBroadcast(value)

char *value;

{
if (strlen(CURRENTPATH) != 0)
    {
    yyerror("redefinition of broadcast address");
    }

if (strcmp("ones",value) == 0)
   {
   strcpy(CURRENTPATH,"one");
   return;
   }

if (strcmp("zeroes",value) == 0)
   {
   strcpy(CURRENTPATH,"zero");
   return;
   }

if (strcmp("zeros",value) == 0)
   {
   strcpy(CURRENTPATH,"zero");
   return;
   }

yyerror("Illegal broadcast specifier (ones/zeros)"); 
}

/*******************************************************************/

void AppendToActionSequence (action)

char *action;

{ int j = 0;
  char *sp,cbuff[bufsize],actiontxt[bufsize];

Debug1("Installing item (%s) in the action sequence list\n",action);

AppendItem(&VACTIONSEQ,action,CLASSBUFF);

if (strstr(action,"module"))
   {
   USEENVIRON=true;
   }
 
cbuff[0]='\0';
actiontxt[0]='\0';
sp = action;

while (*sp != '\0')
   {
   ++j;
   sscanf(sp,"%[^.]",cbuff);

   while ((*sp != '\0') && (*sp !='.'))
      {
      sp++;
      }
 
   if (*sp == '.')
      {
      sp++;
      }
 
   if (IsHardClass(cbuff))
      {
      char *tmp = malloc(strlen(action)+30);
      sprintf(tmp,"Error in action sequence: %s\n",action);
      yyerror(tmp);
      free(tmp);
      yyerror("You cannot add a reserved class!");
      return;
      }
 
   if (j == 1)
      {
      strcpy(actiontxt,cbuff);
      continue;
      }
   else if ((!IsSpecialClass(cbuff)) && (!IsItemIn(VALLADDCLASSES,cbuff)))
      {
      PrependItem(&VALLADDCLASSES,cbuff,NULL);
      }
   }
}

/*******************************************************************/

void AppendToAccessList (user)

char *user;

{ char id[maxvarsize];
  struct passwd *pw;

Debug1("Adding to access list for %s\n",user);

if (isalpha((int)user[0]))
   {
   if ((pw = getpwnam(user)) == NULL)
      {
      yyerror("No such user in password database");
      return;
      }

   sprintf(id,"%d",pw->pw_uid);
   AppendItem(&VACCESSLIST,id,NULL);
   }
else
   {
   AppendItem(&VACCESSLIST,user,NULL);
   }
}

/*******************************************************************/

void HandleLinkAction(value)

char *value;

{
if (strcmp(value,"silent") == 0)
   {
   LINKSILENT = true;
   return;
   }

yyerror("Invalid link action");
}

/*******************************************************************/

void HandleDeadLinks(value)

char *value;

{
if (strcmp(value,"kill") == 0)
   {
   DEADLINKS = false;
   return;
   }

if (strcmp(value,"force") == 0)
   {
   DEADLINKS = true;
   return;
   }
 
yyerror("Invalid deadlink option");
}

/*******************************************************************/

void HandleLinkType(value)

char *value;

{
if (strcmp(value,"hard") == 0)
   {
   if (ACTION_IS_LINKCHILDREN)
      {
      yyerror("hard links inappropriate for multiple linkage");
      }

   if (ACTION == image)
      {
      yyerror("hard links inappropriate for copy operation");
      }

   LINKTYPE = 'h';
   return;
   }

if (strcmp(value,"symbolic") == 0 || strcmp(value,"sym") == 0)
   {
   LINKTYPE = 's';
   return;
   }

if (strcmp(value,"abs") == 0 || strcmp(value,"absolute") == 0)
   {
   LINKTYPE = 'a';
   return;
   }

if (strcmp(value,"rel") == 0 || strcmp(value,"relative") == 0)
   {
   LINKTYPE = 'r';
   return;
   }
 
if (strcmp(value,"none") == 0 || strcmp(value,"copy") == 0)
   {
   LINKTYPE = 'n';
   return;
   }

 snprintf(OUTPUT,bufsize*2,"Invalid link type %s\n",value);
yyerror(OUTPUT);
}

/*******************************************************************/

void HandleServer(value)

char *value;

{
Debug("Server in copy set to : %s\n",value);
strcpy(CFSERVER,value);
}

/*******************************************************************/

void HandleDefine(value)

char *value;

{ char *sp;
 
Debug("Define response classes: %s\n",value);

if (strlen(value) > bufsize)
   {
   yyerror("class list too long - can't handle it!");
   }

/*if (!IsInstallable(value))
   {
   snprintf(OUTPUT,bufsize*2,"Undeclared installable define=%s (see AddInstallable)",value);
   yyerror(OUTPUT);
   }
*/
strcpy(ALLCLASSBUFFER,value);

for (sp = value; *sp != '\0'; sp++)
   {
   if (*sp == ':' || *sp == ',' || *sp == '.')
      {
      continue;
      }
   
   if (! isalnum((int)*sp) && *sp != '_')
      {
      snprintf(OUTPUT,bufsize*2,"Illegal class list in define=%s\n",value);
      yyerror(OUTPUT);
      }
   }
}

/*******************************************************************/

void HandleElseDefine(value)

char *value;

{ char *sp;
 
Debug("Define default classes: %s\n",value);

if (strlen(value) > bufsize)
   {
   yyerror("class list too long - can't handle it!");
   }

strcpy(ELSECLASSBUFFER,value);

for (sp = value; *sp != '\0'; sp++)
   {
   if (*sp == ':' || *sp == ',' || *sp == '.')
      {
      continue;
      }
   
   if (! isalnum((int)*sp) && *sp != '_')
      {
      snprintf(OUTPUT,bufsize*2,"Illegal class list in elsedefine=%s\n",value);
      yyerror(OUTPUT);
      }
   }
}

/*******************************************************************/
/* Level 4                                                         */
/*******************************************************************/

struct UidList *MakeUidList(uidnames)

char *uidnames;

{ struct UidList *uidlist;
  struct Item *ip, *tmplist;
  char uidbuff[bufsize];
  char *sp;
  int offset;
  struct passwd *pw;
  char *machine, *user, *domain, buffer[bufsize];
  int uid;
  int tmp;

uidlist = NULL;
buffer[0] = '\0';
 
ExpandVarstring(uidnames,buffer,"");
 
for (sp = buffer; *sp != '\0'; sp+=strlen(uidbuff))
   {
   if (*sp == ',')
      {
      sp++;
      }

   if (sscanf(sp,"%[^,]",uidbuff))
      {
      if (uidbuff[0] == '+')        /* NIS group - have to do this in a roundabout     */
         {                          /* way because calling getpwnam spoils getnetgrent */
         offset = 1;
         if (uidbuff[1] == '@')
            {
            offset++;
            }

         setnetgrent(uidbuff+offset);
         tmplist = NULL;

         while (getnetgrent(&machine,&user,&domain))
            {
            if (user != NULL)
               {
               AppendItem(&tmplist,user,NULL);
               }
            }
                   
         endnetgrent();

         for (ip = tmplist; ip != NULL; ip=ip->next)
            {
            if ((pw = getpwnam(ip->name)) == NULL)
               {
	       snprintf(OUTPUT,bufsize*2,"Unknown user [%s]\n",ip->name);
	       CfLog(cfverbose,OUTPUT,"getpwnam");
               continue;
               }
            else
               {
               uid = pw->pw_uid;
               }
            AddSimpleUidItem(&uidlist,uid); 
            }

         DeleteItemList(tmplist);
         continue;
         }

      if (isdigit((int)uidbuff[0]))
         {
         sscanf(uidbuff,"%d",&tmp);
         uid = (uid_t)tmp;
         }
      else
         {
         if (strcmp(uidbuff,"*") == 0)
            {
            uid = -1;                     /* signals wildcard */
            }
         else if ((pw = getpwnam(uidbuff)) == NULL)
            {
            printf("Unknown user %s\n",uidbuff);
            Warning("User is not known in this passwd domain");
            continue;
            }
         else
            {
            uid = pw->pw_uid;
            }
         }
      AddSimpleUidItem(&uidlist,uid);
      }
   }

if (uidlist == NULL)
   {
   AddSimpleUidItem(&uidlist,(uid_t) -1);
   }

return (uidlist);
}

/*********************************************************************/

struct GidList *MakeGidList(gidnames)

char *gidnames;

{ struct GidList *gidlist;
  char gidbuff[bufsize],buffer[bufsize];
  char err[bufsize];
  char *sp;
  struct group *gr;
  int gid;
  int tmp;

gidlist = NULL;
buffer[0] = '\0';
 
ExpandVarstring(gidnames,buffer,"");
 
for (sp = buffer; *sp != '\0'; sp+=strlen(gidbuff))
   {
   if (*sp == ',')
      {
      sp++;
      }

   if (sscanf(sp,"%[^,]",gidbuff))
      {
      if (isdigit((int)gidbuff[0]))
         {
         sscanf(gidbuff,"%d",&tmp);
         gid = (gid_t)tmp;
         }
      else
         {
         if (strcmp(gidbuff,"*") == 0)
            {
            gid = -1;                     /* signals wildcard */
            }
         else if ((gr = getgrnam(gidbuff)) == NULL)
            {
            sprintf(err,"Unknown group %s\n",gidbuff);
            Warning(err);
            continue;
            }
         else
            {
            gid = gr->gr_gid;
            }
         }
      AddSimpleGidItem(&gidlist,gid);
      }
   }

if (gidlist == NULL)
   {
   AddSimpleGidItem(&gidlist,(gid_t) -1);
   }

return(gidlist);
}


/*******************************************************************/

void InstallTidyPath(path,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes)

char *wild, *path;
short age,tidydirs;
int rec, tidysize;
char type, ldirs, *classes, travlinks;

{ struct Tidy *ptr;
  char *sp;
  int no_of_links = 0;

if (!IsInstallable(classes))
   {
   InitializeAction();
   Debug1("Not installing tidy path, no match\n");
   return;
   }

VBUFF[0]='\0';                                /* Expand any variables */
ExpandVarstring(path,VBUFF,"");

if (strlen(VBUFF) != 1)
   {
   DeleteSlash(VBUFF);
   }

if ((ptr = (struct Tidy *)malloc(sizeof(struct Tidy))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallTidyItem() #1");
   }

if ((ptr->path = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallTidyItem() #3");
   }

if (VTIDYTOP == NULL)                 /* First element in the list */
   {
   VTIDY = ptr;
   }
else
   {
   VTIDYTOP->next = ptr;
   }

if (rec != INFINITERECURSE && strncmp("home/",ptr->path,5) == 0) /* Is this a subdir of home wildcard? */
   {
   for (sp = ptr->path; *sp != '\0'; sp++)                     /* Need to make recursion relative to start */
      {                                                        /* of the search, not relative to home */
      if (*sp == '/')
         {
         no_of_links++;
         }
      }
   }

ptr->done = 'n'; 
ptr->tidylist = NULL;
ptr->maxrecurse = rec + no_of_links;
ptr->next = NULL;
ptr->exclusions = VEXCLUDEPARSE;
ptr->ignores = VIGNOREPARSE;      
   
VTIDYTOP = ptr;

AddTidyItem(path,wild,rec+no_of_links,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
}

/*********************************************************************/

void AddTidyItem(path,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes)

char *wild, *path;
short age,tidydirs;
int rec,tidysize;
char type, ldirs,*classes, travlinks;

{ char varbuff[bufsize];
  struct Tidy *ptr;

Debug1("AddTidyItem(%s,pat=%s,size=%d)\n",path,wild,tidysize);

if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing TidyItem no match\n");
   return;
   }

for (ptr = VTIDY; ptr != NULL; ptr=ptr->next)
   {
   varbuff[0] = '\0';
   ExpandVarstring(path,varbuff,"");

   if (strcmp(ptr->path,varbuff) == 0)
      {
      PrependTidy(&(ptr->tidylist),wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
      
      /* Must have the maximum recursion level here */

      if (rec == INFINITERECURSE || ((ptr->maxrecurse < rec) && (ptr->maxrecurse != INFINITERECURSE)))
	 {
         ptr->maxrecurse = rec;
	 }
      return;
      }
   }
}

/*********************************************************************/

int TidyPathExists(path)

char *path;

{ struct Tidy *tp;

VBUFF[0]='\0';                                /* Expand any variables */
ExpandVarstring(path,VBUFF,"");

for (tp = VTIDY; tp != NULL; tp=tp->next)
   {
   if (strcmp(tp->path,path) == 0)
      {
      Debug1("TidyPathExists(%s)\n",path);
      return true;
      }
   }

return false;
}


/*******************************************************************/
/* Level 5                                                         */
/*******************************************************************/

void AddSimpleUidItem(uidlist,uid)

struct UidList **uidlist;
int uid;

{ struct UidList *ulp, *u;

if ((ulp = (struct UidList *)malloc(sizeof(struct UidList))) == NULL)
   {
   FatalError("cfengine: malloc() failed #1 in AddSimpleUidItem()");
   }

ulp->uid = uid;
ulp->next = NULL;

if (*uidlist == NULL)
   {
   *uidlist = ulp;
   }
else
   {
   for (u = *uidlist; u->next != NULL; u = u->next)
      {
      }
   u->next = ulp;
   }
}

/*******************************************************************/

void AddSimpleGidItem(gidlist,gid)

struct GidList **gidlist;
int gid;

{ struct GidList *glp,*g;

if ((glp = (struct GidList *)malloc(sizeof(struct GidList))) == NULL)
   {
   FatalError("cfengine: malloc() failed #1 in AddSimpleGidItem()");
   }
 
glp->gid = gid;
glp->next = NULL;

if (*gidlist == NULL)
   {
   *gidlist = glp;
   }
else
   {
   for (g = *gidlist; g->next != NULL; g = g->next)
      {
      }
   g->next = glp;
   }
}


/***********************************************************************/

void InstallAuthPath(path,hostname,classes,list,listtop)

char *path, *hostname, *classes;
struct Auth **list, **listtop;

{ struct Auth *ptr;

if (!IsInstallable(classes))
   {
   Debug1("Not installing Auth path, no match\n");
   return;
   }

Debug1("InstallAuthPath(%s,%s)\n",path,hostname);

VBUFF[0]='\0';                                /* Expand any variables */
ExpandVarstring(path,VBUFF,"");

if (strlen(VBUFF) != 1)
   {
   DeleteSlash(VBUFF);
   }

if ((ptr = (struct Auth *)malloc(sizeof(struct Auth))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallAuthPath() #1");
   }

if ((ptr->path = strdup(VBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallAuthPath() #3");
   }

if (*listtop == NULL)                 /* First element in the list */
   {
   *list = ptr;
   }
else
   {
   (*listtop)->next = ptr;
   }

ptr->accesslist = NULL;
ptr->maproot = NULL;
ptr->encrypt = false; 
ptr->next = NULL;
*listtop = ptr;

AddAuthHostItem(ptr->path,hostname,classes,list);
}

/***********************************************************************/

void AddAuthHostItem(path,attribute,classes,list)

char *path, *attribute, *classes;
struct Auth **list;

{ char varbuff[bufsize];
  struct Auth *ptr;

Debug1("AddAuthHostItem(%s,%s)\n",path,attribute);

if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing TidyItem no match\n");
   return;
   }

for (ptr = *list; ptr != NULL; ptr=ptr->next)
   {
   varbuff[0] = '\0';
   ExpandVarstring(path,varbuff,"");

   if (strcmp(ptr->path,varbuff) == 0)
      {
      if (!HandleAdmitAttribute(ptr,attribute))
	 {
	 PrependItem(&(ptr->accesslist),attribute,classes);
	 }
      return;
      }
   }
}


/*********************************************************************/

int AuthPathExists(path,list)

char *path;
struct Auth *list;

{ struct Auth *ap;

Debug1("AuthPathExists(%s)\n",path);

VBUFF[0]='\0';                                /* Expand any variables */
ExpandVarstring(path,VBUFF,"");

if (VBUFF[0] != '/')
   {
   yyerror("Missing absolute path to a directory");
   FatalError("Cannot continue");
   }

for (ap = list; ap != NULL; ap=ap->next)
   {
   if (strcmp(ap->path,VBUFF) == 0)
      {
      return true;
      }
   }

return false;
}

/*********************************************************************/

int HandleAdmitAttribute(ptr,attribute)

struct Auth *ptr;
char *attribute;

{ char value[maxvarsize],buffer[bufsize],host[maxvarsize],*sp;

VBUFF[0] = value[0] = '\0';

ExpandVarstring(attribute,VBUFF,NULL);

sscanf(VBUFF,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   return false;
   }

Debug1("HandleAdmitFileAttribute(%s)\n",value);

switch(GetCommAttribute(attribute))
   {
   case cfencryp:  if ((strcmp(value,"on")==0) || (strcmp(value,"true")==0))
                      {
		      ptr->encrypt = true;
		      return true;
		      }
                   break;

   case cfroot:   
                  ExpandVarstring(value,buffer,"");
 
		  for (sp = buffer; *sp != '\0'; sp+=strlen(host))
		     {
		     if (*sp == ',')
			{
			sp++;
			}

		     host[0] = '\0';
		     
		     if (sscanf(sp,"%[^,]",host))
			{
			if (!strstr(host,"."))
			   {
			   if (strlen(host)+strlen(VDOMAIN) < maxvarsize-2)
			      {
			      strcat(host,".");
			      strcat(host,VDOMAIN);
			      }
			   else
			      {
			      yyerror("Server name too long");
			      }
			   }
			PrependItem(&(ptr->maproot),host,NULL);
			}
		     }
		  return true;
		  break;
   }

yyerror("Illegal admit attribute"); 
return false;
}


/*********************************************************************/

void PrependTidy(list,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes)

struct TidyPattern **list;
char *wild;
short age,tidydirs;
int rec,tidysize;
char type, ldirs, *classes, travlinks;

{ struct TidyPattern *tp;
  char *spe = NULL,*sp, buffer[bufsize];

if ((tp = (struct TidyPattern *)malloc(sizeof(struct TidyPattern))) == NULL)
   {
   perror("Can't allocate memory in PrependTidy()");
   FatalError("");
   }

if ((sp = strdup(wild)) == NULL)
   {
   FatalError("Memory Allocation failed for PrependTidy() #2");
   }

ExpandVarstring(ALLCLASSBUFFER,buffer,""); 

if ((tp->defines = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for PrependTidy() #2a");
   }

ExpandVarstring(ELSECLASSBUFFER,buffer,"");  

if ((tp->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for PrependTidy() #2a");
   }
 
AddInstallable(tp->defines);
AddInstallable(tp->elsedef); 
 
if ((classes!= NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   perror("Can't allocate memory in PrependItem()");
   FatalError("");
   }

if (travlinks == '?')
   {
   travlinks = 'F';  /* False is default */
   }

tp->size = tidysize;
tp->recurse = rec;
tp->age = age;
tp->searchtype = type;
tp->travlinks = travlinks;
tp->filters = VFILTERBUILD;
tp->pattern = sp;
tp->next = *list;
tp->dirlinks = ldirs;
tp->log = LOGP;
tp->inform = INFORMP;
tp->compress=COMPRESS;
 
switch (tidydirs)
   {
   case 0: tp->rmdirs = 'n';
           break;
   case 1: tp->rmdirs = 'y';
           break;
   case 2: tp->rmdirs = 's';
           break;
   default: FatalError("Software error in Prepend Tidy");
   }

*list = tp;

if (classes != NULL)
   {
   strcpy(spe,classes);
   tp->classes = spe;
   }
else
   {
   tp->classes = NULL;
   }
}


/* EOF */
