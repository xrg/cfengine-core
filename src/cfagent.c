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

/*******************************************************************/
/*                                                                 */
/*  Cfengine : a site configuration langugae                       */
/*                                                                 */
/*  Module: (main) cfengine.c                                      */
/*                                                                 */
/*  Mark Burgess 1994/96                                           */
/*                                                                 */
/*******************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/
/* Functions internal to cfengine.c                                */
/*******************************************************************/

int main ARGLIST((int argc,char *argv[]));
void Initialize ARGLIST((int argc, char **argv));
void PreNetConfig ARGLIST((void));
void ReadRCFile ARGLIST((void));
void EchoValues ARGLIST((void));
void CheckSystemVariables ARGLIST((void));
void SetReferenceTime ARGLIST((int setclasses));
void SetStartTime ARGLIST((int setclasses));
void DoTree ARGLIST((int passes, char *info));
enum aseq EvaluateAction ARGLIST((char *action, struct Item **classlist, int pass));
void CheckOpts ARGLIST((int argc, char **argv));
int GetResource ARGLIST((char *var));
void BuildClassEnvironment ARGLIST((void));
void CheckForModule ARGLIST((char *actiontxt, char *args));
void Syntax ARGLIST((void));
void EmptyActionSequence ARGLIST((void));
void GetEnvironment ARGLIST((void));
int NothingLeftToDo ARGLIST((void));
void SummarizeObjects ARGLIST((void));
void CheckClass ARGLIST((char *class, char *source));
void SetContext ARGLIST((char *id));
void DeleteCaches ARGLIST((void));

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main(argc,argv)

char *argv[];
int argc;

{ struct Item *ip;

SetContext("global");
 
signal (SIGTERM,HandleSignal);                   /* Signal Handler */
signal (SIGHUP,HandleSignal);
signal (SIGINT,HandleSignal);
signal (SIGPIPE,HandleSignal);

Initialize(argc,argv);

SetReferenceTime(true);
SetStartTime(false);
 
if (! NOHARDCLASSES)
   {
   GetNameInfo();
   GetInterfaceInfo();
   GetV6InterfaceInfo();
   GetEnvironment();
   }

PreNetConfig();

ReadRCFile(); /* Should come before parsing so that it can be overridden */

if (IsPrivileged() && !MINUSF && !PRMAILSERVER)
   {
   SetContext("update");
   if (ParseBootFiles())
      {
      CheckSystemVariables();
      if (!PARSEONLY)
	 {
	 SetStrategies(); 
	 DoTree(1,"Update");
	 EmptyActionSequence();
	 DeleteClassesFromContext("update");
	 DeleteCaches();
	 }
      }
   }

DeleteMacros(CONTEXTID);
 
if (UPDATEONLY)
   {
   return 0;
   }

SetContext("main");

if (!PARSEONLY)
   {
   GetEnvironment();
   }
 
ParseInputFiles();
CheckFilters();
SetStrategies(); 
EchoValues();
 
if (PRSYSADM)                                           /* -a option */
   {
   printf("%s\n",VSYSADM);
   exit (0);
   }

if (PRMAILSERVER)
   {
   if (GetMacroValue(CONTEXTID,"smtpserver"))
      {
      printf("%s\n",GetMacroValue(CONTEXTID,"smtpserver"));
      }
   else
      {
      printf("No SMTP server defined\n");
      }
   printf("%s\n",VSYSADM);
   printf("%s\n",VFQNAME);
   printf("%s\n",VIPADDRESS);

   if (GetMacroValue(CONTEXTID,"EmailMaxLines"))
      {
      printf("%s\n", GetMacroValue(CONTEXTID,"EmailMaxLines"));
      }
   else
      {
      /* User has not expressed a preference -- let cfexecd decide */
      printf("%s", "-1\n");
      }

   for (ip = SCHEDULE; ip != NULL; ip=ip->next)
      {
      printf("[%s]\n",ip->name);
      }

   printf("\n");
   exit(0);
   }

if (PARSEONLY)                            /* Establish lock for root */
   {
   exit(0);
   } 
 
openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_USER);

CheckSystemVariables();

SetReferenceTime(false); /* Reset */

DoTree(2,"Main Tree"); 
DoAlerts();

SummarizeObjects();
 
closelog();
return 0;
}

/*******************************************************************/
/* Level 1                                                         */
/*******************************************************************/
 
void Initialize(argc,argv)

char *argv[];
int argc;

{ char *sp, *cfargv[maxargs];
  int i, cfargc, seed;
  struct stat statbuf;
  unsigned char s[16];
  
strcpy(VDOMAIN,CF_START_DOMAIN);

PreLockState();
SetSignals(); 
 
ISCFENGINE = true;
VFACULTY[0] = '\0';
VSYSADM[0] = '\0';
VNETMASK[0]= '\0';
VBROADCAST[0] = '\0';
VMAILSERVER[0] = '\0';
VDEFAULTROUTE[0] = '\0';
ALLCLASSBUFFER[0] = '\0';
VREPOSITORY = strdup("\0");

 
#ifndef HAVE_REGCOMP
re_syntax_options |= RE_INTERVALS;
#endif
 
strcpy(VINPUTFILE,"cfagent.conf");
strcpy(VNFSTYPE,"nfs");

AddClassToHeap("any");      /* This is a reserved word / wildcard */

snprintf(VBUFF,bufsize,"cfengine_%s",CanonifyName(VERSION));
AddClassToHeap(VBUFF);

if (stat("/etc/redhat-release",&statbuf) != -1)
   {
   Verbose("This appears to be a redhat system.\n");
   AddClassToHeap("redhat");
   linux_redhat_version();
   }

if (stat("/etc/SuSE-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a SuSE system.\n");
   AddClassToHeap("SuSE");
   }

if (stat("/etc/slackware-release",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a slackware system.\n");
   AddClassToHeap("slackware");
   }
 
if (stat("/etc/debian_version",&statbuf) != -1)
   {
   Verbose("\nThis appears to be a debian system.\n");
   AddClassToHeap("debian");
   debian_version();
   }
 
/* Note we need to fix the options since the argv mechanism doesn't */
 /* work when the shell #!/bla/cfengine -v -f notation is used.      */
 /* Everything ends up inside a single argument! Here's the fix      */

cfargc = 1;
cfargv[0]="cfengine";

for (i = 1; i < argc; i++)
   {
   sp = argv[i];

   while (*sp != '\0')
      {
      while (*sp == ' ' && *sp != '\0') /* Skip to arg */
         {
	 if (*sp == ' ')
	    {
	    *sp = '\0'; /* Break argv string */
	    }
	 sp++;
         }

      cfargv[cfargc++] = sp;
      
      while (*sp != ' ' && *sp != '\0') /* Skip to white space */
         {
	 sp++;
         }
      }
   }

VDEFAULTBINSERVER.name = "";

VEXPIREAFTER = VDEFAULTEXPIREAFTER;
VIFELAPSED = VDEFAULTIFELAPSED;
TRAVLINKS = false;
 
sprintf(VBUFF,"%s/cf_procs",WORKDIR);
 
 if (stat(VBUFF,&statbuf) == -1)
    {
    CreateEmptyFile(VBUFF);
    }
 
strcpy(VLOGDIR,WORKDIR); 
strcpy(VLOCKDIR,VLOGDIR);  /* Same since 2.0.a8 */

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();
CheckWorkDirectories();
RandomSeed();
 
RAND_bytes(s,16);
s[15] = '\0';
seed = ElfHash(s);
srand48((long)seed);  
CheckOpts(cfargc,cfargv);
}

/*******************************************************************/

void PreNetConfig()                           /* Execute a shell script */

{ struct stat buf;
  char comm[bufsize];
  char *sp;
  FILE *pp;

if (NOPRECONFIG)
   {
   CfLog(cfverbose,"Ignoring the cf.preconf file: option set","");
   return;
   }

strcpy(VPREFIX,"cfengine:");
strcat(VPREFIX,VUQNAME);
 
if ((sp=getenv(CFINPUTSVAR)) != NULL)
   {
   snprintf(comm,bufsize,"%s/%s",sp,VPRECONFIG);

   if (stat(comm,&buf) == -1)
       {
       CfLog(cfverbose,"No preconfiguration file","");
       return;
       }
   
   snprintf(comm,bufsize,"%s/%s %s 2>&1",sp,VPRECONFIG,CLASSTEXT[VSYSTEMHARDCLASS]);
   }
else
   {
   snprintf(comm,bufsize,"%s/%s",WORKDIR,VPRECONFIG);

   if (stat(comm,&buf) == -1)
       {
       CfLog(cfverbose,"No preconfiguration file\n","");
       return;
       }
      
   snprintf(comm,bufsize,"%s/%s %s",WORKDIR,VPRECONFIG,CLASSTEXT[VSYSTEMHARDCLASS]);
   }

if (S_ISDIR(buf.st_mode) || S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode))
   {
   snprintf(OUTPUT,bufsize*2,"Error: %s was not a regular file\n",VPRECONFIG);
   CfLog(cferror,OUTPUT,"");
   FatalError("Aborting.");
   }

Verbose("\n\nExecuting Net Preconfiguration script...%s\n\n",VPRECONFIG);
 
if ((pp = cfpopen(comm,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Failed to open pipe to %s\n",comm);
   CfLog(cferror,OUTPUT,"");
   return;
   }

while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error running preconfig\n","ferror");
      break;
      }

   ReadLine(VBUFF,bufsize,pp);

   if (feof(pp))
      {
      break;
      }
   
   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error running preconfig\n","ferror");
      break;
      }

   CfLog(cfinform,VBUFF,"");
   }

cfpclose(pp);
}

/*******************************************************************/

void DeleteCaches()

{
/* DeleteItemList(VEXCLUDECACHE); ?? */
}

/*******************************************************************/

void ReadRCFile()

{ char filename[bufsize], buffer[bufsize], *sp, *mp;
  char class[maxvarsize], variable[maxvarsize], value[maxvarsize];
  int c;
  FILE *fp;

filename[0] = buffer[0] = class[0] = variable[0] = value[0] = '\0';
LINENUMBER = 0;

if ((sp=getenv(CFINPUTSVAR)) != NULL)
   {
   strcpy(filename,sp);
   if (filename[strlen(filename)-1] != '/')
      {
      strcat(filename,"/");
      }
   }

strcat(filename,VRCFILE);

if ((fp = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   return;
   }

while (!feof(fp))
   {
   ReadLine(buffer,bufsize,fp);
   LINENUMBER++;
   class[0]='\0';
   variable[0]='\0';
   value[0]='\0';

   if (strlen(buffer) == 0 || buffer[0] == '#')
      {
      continue;
      }

   if (strstr(buffer,":") == 0)
      {
      snprintf(OUTPUT,bufsize*2,"Malformed line (missing :) in resource file %s - skipping\n",VRCFILE);
      CfLog(cferror,OUTPUT,"");
      continue;
      }

   sscanf(buffer,"%[^.].%[^:]:%[^\n]",class,variable,value);

   if (class[0] == '\0' || variable[0] == '\0' || value[0] == '\0')
      {
      snprintf(OUTPUT,bufsize*2,"%s:%s - Bad resource\n",VRCFILE,buffer);
      CfLog(cferror,OUTPUT,"");
      snprintf(OUTPUT,bufsize*2,"class=%s,variable=%s,value=%s\n",class,variable,value);
      CfLog(cferror,OUTPUT,"");
      FatalError("Bad resource");
      }

   if (strcmp(CLASSTEXT[VSYSTEMHARDCLASS],class) != 0) 
      {
      continue;  /* No point if not equal*/
      }

   if ((mp = strdup(value)) == NULL)
      {
      perror("malloc");
      FatalError("malloc in ReadRCFile");
      }

   snprintf(OUTPUT,bufsize*2,"Redefining resource %s as %s (%s)\n",variable,value,class);
   CfLog(cfverbose,OUTPUT,"");

   c = VSYSTEMHARDCLASS;

   switch (GetResource(variable))
      {
      case rmountcom:
                        VMOUNTCOMM[c] = mp;
                        break;

      case runmountcom:
                        VUNMOUNTCOMM[c] = mp;
                        break;
      case rethernet:
                        VIFDEV[c] = mp;
                        break;
      case rmountopts:
                        VMOUNTOPTS[c] = mp;
                        break;
      case rfstab:
                        VFSTAB[c] = mp;
                        break;
      case rmaildir:
                        VMAILDIR[c] = mp;
                        break;
      case rnetstat:
                        VNETSTAT[c] = mp;
                        break;
      case rpscomm:
	                VPSCOMM[c] = mp;
	                break;
      case rpsopts:
	                VPSOPTS[c] = mp;
	                break;
      default:
                        snprintf(VBUFF,bufsize,"Bad resource %s in %s\n",variable,VRCFILE);
                        FatalError(VBUFF);
                        break;
      }
   }

fclose(fp);
}

/*******************************************************************/

void EmptyActionSequence()

{
DeleteItemList(VACTIONSEQ);
VACTIONSEQ = NULL;
}

/*******************************************************************/

void GetEnvironment()

{ char env[bufsize],class[bufsize],name[maxvarsize],value[maxvarsize];
  FILE *fp;
  struct stat statbuf;

Debug1("GetEnvironment()\n");
snprintf(env,bufsize,"%s/%s",WORKDIR,ENV_FILE);

if (stat(env,&statbuf) == -1)
   {
   Verbose("\nUnable to detect environment from cfenvd\n\n");
   return;
   }

 if (!GetMacroValue(CONTEXTID,"env_time"))
    {
    snprintf(value,maxvarsize-1,"%s",ctime(&statbuf.st_mtime));
    Chop(value);
    AddMacroValue(CONTEXTID,"env_time",value);
    }
 else
    {
    CfLog(cferror,"Reserved variable $(env_time) in use","");
    }
 
if ((fp = fopen(env,"r")) == NULL)
   {
   Verbose("\nUnable to detect environment from cfenvd\n\n");
   return;
   }

while (!feof(fp))
   {
   class[0] = '\0';
   name[0] = '\0';
   value[0] = '\0';
   fscanf(fp,"%256s",class);

   if (feof(fp))
      {
      break;
      }

   if (strstr(class,"="))
      {
      sscanf(class,"%255[^=]=%255s",name,value);
      if (!GetMacroValue(CONTEXTID,name))
	 {
	 AddMacroValue(CONTEXTID,name,value);
	 }
      }
   else
      {
      AddClassToHeap(class);
      }
   }
 
fclose(fp); 
}

/*******************************************************************/

void EchoValues()

{ struct Item *ip;
  int n = 0;
  
if (strstr(VSYSNAME.nodename,ToLowerStr(VDOMAIN)))
   {
   strcpy(VFQNAME,VSYSNAME.nodename);
   
   while(VSYSNAME.nodename[n++] != '.')
      {
      }
   
   strncpy(VUQNAME,VSYSNAME.nodename,n-1);
   }
else
   {
   snprintf(VFQNAME,bufsize,"%s.%s",VSYSNAME.nodename,ToLowerStr(VDOMAIN));
   strcpy(VUQNAME,VSYSNAME.nodename);
   }
 
Verbose("Accepted domain name: %s\n\n",VDOMAIN); 
bzero(VBUFF,bufsize);

if (GetMacroValue(CONTEXTID,"OutputPrefix"))
   {
   ExpandVarstring("$(OutputPrefix)",VBUFF,NULL);
   }

if (strlen(VBUFF) != 0)
   {
   strncpy(VPREFIX,VBUFF,40);  /* No more than 40 char prefix (!) */
   }
else
   {
   strcpy(VPREFIX,"cfengine:");
   strcat(VPREFIX,VUQNAME);
   }


if (VERBOSE || DEBUG || D2 || D3)
   {
   struct Item *ip;
   
   ListDefinedClasses();

   printf("\nGlobal expiry time for locks: %d minutes\n",VEXPIREAFTER);
   printf("\nGlobal anti-spam elapse time: %d minutes\n\n",VIFELAPSED);

   printf("Extensions which should not be directories = ( ");
   for (ip = EXTENSIONLIST; ip != NULL; ip=ip->next)
      {
      printf("%s ",ip->name);
      }
   printf(")\n");

   printf("Suspicious filenames to be warned about = ( ");
   for (ip = SUSPICIOUSLIST; ip != NULL; ip=ip->next)
      {
      printf("%s ",ip->name);
      }
   printf(")\n");
   
   }

if (DEBUG || D2 || D3)
   {
   printf("\nFully qualified hostname is: %s\n",VFQNAME);
   printf("Unqualified hostname is: %s\n",VUQNAME);
   printf("\nSystem administrator mail address is: %s\n",VSYSADM);
   printf("Sensible size = %d\n",SENSIBLEFSSIZE);
   printf("Sensible count = %d\n",SENSIBLEFILECOUNT);
   printf("Edit File (Max) Size = %d\n\n",EDITFILESIZE);
   printf("Edit Binary File (Max) Size = %d\n\n",EDITBINFILESIZE);
   printf("------------------------------------------------------------\n");
   ListDefinedInterfaces();
   printf("------------------------------------------------------------\n");
   ListDefinedBinservers();
   printf("------------------------------------------------------------\n");
   ListDefinedHomeservers();
   printf("------------------------------------------------------------\n");
   ListDefinedHomePatterns();
   printf("------------------------------------------------------------\n");
   ListActionSequence();

   printf("\nWill need to copy from the following trusted sources = ( ");

   for (ip = VSERVERLIST; ip !=NULL; ip=ip->next)
      {
      printf("%s ",ip->name);
      }
   printf(")\n");
   printf("\nUsing mailserver %s\n",VMAILSERVER);
   printf("\nLocal mountpoints: ");
   for (ip = VMOUNTLIST; ip != NULL; ip=ip->next)
      {
      printf ("%s ",ip->name);
      }
   printf("\n");
   printf("\nDefault route for packets %s\n\n",VDEFAULTROUTE);
   printf("\nFile repository = %s\n\n",VREPOSITORY);
   printf("\nNet interface name = %s\n",VIFDEV[VSYSTEMHARDCLASS]);
   printf("------------------------------------------------------------\n");
   ListDefinedAlerts();
   printf("------------------------------------------------------------\n");
   ListDefinedStrategies();
   printf("------------------------------------------------------------\n");
   ListDefinedResolvers();
   printf("------------------------------------------------------------\n");
   ListDefinedRequired();
   printf("------------------------------------------------------------\n");
   ListDefinedMountables();
   printf("------------------------------------------------------------\n");
   ListMiscMounts();
   printf("------------------------------------------------------------\n");
   ListUnmounts();
   printf("------------------------------------------------------------\n");
   ListDefinedMakePaths();
   printf("------------------------------------------------------------\n");
   ListDefinedImports();
   printf("------------------------------------------------------------\n");
   ListFiles();
   printf("------------------------------------------------------------\n");
   ListACLs();
   printf("------------------------------------------------------------\n");
   ListFilters();
   printf("------------------------------------------------------------\n");   
   ListDefinedIgnore();
   printf("------------------------------------------------------------\n");
   ListFileEdits();
   printf("------------------------------------------------------------\n");
   ListProcesses();
   printf("------------------------------------------------------------\n");
   ListDefinedImages();
   printf("------------------------------------------------------------\n");
   ListDefinedTidy();
   printf("------------------------------------------------------------\n");
   ListDefinedDisable();
   printf("------------------------------------------------------------\n");
   ListDefinedLinks();
   printf("------------------------------------------------------------\n");
   ListDefinedLinkchs();
   printf("------------------------------------------------------------\n");
   ListDefinedScripts();
   printf("------------------------------------------------------------\n");

   
   if (IGNORELOCK)
      {
      printf("\nIgnoring locks...\n");
      }
   }
}

/*******************************************************************/

void CheckSystemVariables()

{ char id[maxvarsize];
  int time, hash, activecfs, locks;

Debug2("\n\n");

if (VACTIONSEQ == NULL)
   {
   Warning("actionsequence is empty ");
   Warning("perhaps cfagent.conf/update.conf have not yet been set up?");
   }

sprintf(id,"%d",geteuid());   /* get effective user id */

if (VACCESSLIST != NULL && !IsItemIn(VACCESSLIST,id))
   {
   FatalError("Access denied");
   }

Debug2("cfagent -d : Debugging output enabled.\n");

if (DONTDO && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -n: Running in ``All talk and no action'' mode\n");
   }

if (TRAVLINKS && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -l : will traverse symbolic links\n");
   printf("             WARNING: This is inherently insecure, if there are untrusted users!\n");
   }

if ( ! IFCONF && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -i : suppressing interface configuration\n");
   }

if ( NOFILECHECK && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -c : suppressing file checks\n");
   }

if ( NOSCRIPTS && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -s : suppressing script execution\n");
   }

if ( NOTIDY && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -t : suppressing tidy function\n");
   }

if ( NOMOUNTS && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -m : suppressing mount operations\n");
   }

if ( MOUNTCHECK && (VERBOSE || DEBUG || D2))
   {
   printf("cfagent -C : check mount points\n");
   }


if (ERRORCOUNT > 0)
   {
   FatalError("Execution terminated after parsing due to errors in program");
   }
 
VCANONICALFILE = strdup(CanonifyName(VINPUTFILE));

if (GetMacroValue(CONTEXTID,"LockDirectory"))
   {
   Verbose("\n[LockDirectory is no longer used - same as LogDirectory]\n\n");
   }

if (GetMacroValue(CONTEXTID,"LogDirectory"))
   {
   Verbose("\n[LogDirectory is no longer runtime configurable: use configure --with-workdir=WORKDIR ]\n\n");
   }

Verbose("LogDirectory = %s\n",VLOGDIR);
  
LoadSecretKeys();
 
if (GetMacroValue(CONTEXTID,"libpath"))
   { 
   snprintf(VBUFF,bufsize,"LD_LIBRARY_PATH=%s",GetMacroValue(CONTEXTID,"libpath"));
   putenv(VBUFF);
   }

if (GetMacroValue(CONTEXTID,"MaxCfengines"))
   {
   activecfs = atoi(GetMacroValue(CONTEXTID,"MaxCfengines"));
 
   locks = CountActiveLocks();
 
   if (locks >= activecfs)
      {
      snprintf(OUTPUT,bufsize*2,"Too many cfagents running (%d/%d)\n",locks,activecfs);
      CfLog(cferror,OUTPUT,"");
      closelog();
      exit(1);
      }
   }

if (GetMacroValue(CONTEXTID,"Verbose") && (strcmp(GetMacroValue(CONTEXTID,"Verbose"),"on") == 0))
   {
   VERBOSE = true;
   }

if (GetMacroValue(CONTEXTID,"Inform") && (strcmp(GetMacroValue(CONTEXTID,"Inform"),"on") == 0))
   {
   INFORM = true;
   } 

 if (GetMacroValue(CONTEXTID,"Exclamation") && (strcmp(GetMacroValue(CONTEXTID,"Exclamation"),"off") == 0))
   {
   EXCLAIM = false;
   } 

INFORM_save = INFORM;
 
if (GetMacroValue(CONTEXTID,"Syslog") && (strcmp(GetMacroValue(CONTEXTID,"Syslog"),"on") == 0))
   {
   LOGGING = true;
   }

LOGGING_save = LOGGING;

if (GetMacroValue(CONTEXTID,"DryRun") && (strcmp(GetMacroValue(CONTEXTID,"DryRun"),"on") == 0))
   {
   DONTDO = true;
   AddClassToHeap("opt_dry_run");
   }

if (GetMacroValue(CONTEXTID,"BinaryPaddingChar"))
   {
   strcpy(VBUFF,GetMacroValue(CONTEXTID,"BinaryPaddingChar"));
   
   if (VBUFF[0] == '\\')
      {
      switch (VBUFF[1])
	 {
	 case '0': PADCHAR = '\0';
	     break;
	 case '\n': PADCHAR = '\n';

	     break;
	 case '\\': PADCHAR = '\\';
	 }
      }
   else
      {
      PADCHAR = VBUFF[0];
      }
   }

 
if (GetMacroValue(CONTEXTID,"Warnings") && (strcmp(GetMacroValue(CONTEXTID,"Warnings"),"on") == 0))
   {
   WARNINGS = true;
   }

if (GetMacroValue(CONTEXTID,"NonAlphaNumFiles") && (strcmp(GetMacroValue(CONTEXTID,"NonAlphaNumFiles"),"on") == 0))
   {
   NONALPHAFILES = true;
   }

if (GetMacroValue(CONTEXTID,"SecureInput") && (strcmp(GetMacroValue(CONTEXTID,"SecureInput"),"on") == 0))
   {
   CFPARANOID = true;
   }

if (GetMacroValue(CONTEXTID,"ShowActions") && (strcmp(GetMacroValue(CONTEXTID,"ShowActions"),"on") == 0))
   {
   SHOWACTIONS = true;
   }

 if (GetMacroValue(CONTEXTID,"Umask"))
   {
   mode_t val;
   val = (mode_t)atoi(GetMacroValue(CONTEXTID,"Umask"));
   if (umask(val) == (mode_t)-1)
      {
      snprintf(OUTPUT,bufsize*2,"Can't set umask to %o\n",val);
      CfLog(cferror,OUTPUT,"umask");
      }
   }

if (GetMacroValue(CONTEXTID,"DefaultCopyType"))
   {
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"mtime") == 0)
      {
      DEFAULTCOPYTYPE = 'm';
      }
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"checksum") == 0)
      {
      DEFAULTCOPYTYPE = 'c';
      }
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"binary") == 0)
      {
      DEFAULTCOPYTYPE = 'b';
      }
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"ctime") == 0)
      {
      DEFAULTCOPYTYPE = 't';
      }
   }
 
if (GetMacroValue(CONTEXTID,"ChecksumDatabase"))
   {
   ExpandVarstring("$(ChecksumDatabase)",VBUFF,NULL);

   CHECKSUMDB = strdup(VBUFF);

   if (*CHECKSUMDB != '/')
      {
      FatalError("$(ChecksumDatabase) does not expand to an absolute filename\n");
      }
   }
else
   {
   snprintf(VBUFF,bufsize,"%s/checksum.db",WORKDIR);
   CHECKSUMDB = strdup(VBUFF);
   }

Verbose("Checksum database is %s\n",CHECKSUMDB); 

if (GetMacroValue(CONTEXTID,"CompressCommand"))
   {
   ExpandVarstring("$(CompressCommand)",VBUFF,NULL);

   COMPRESSCOMMAND = strdup(VBUFF);
   
   if (*COMPRESSCOMMAND != '/')
      {
      FatalError("$(ChecksumDatabase) does not expand to an absolute filename\n");
      }
   }

 
if (GetMacroValue(CONTEXTID,"ChecksumUpdates") && (strcmp(GetMacroValue(CONTEXTID,"ChecksumUpdates"),"on") == 0))
   {
   CHECKSUMUPDATES = true;
   } 
 
if (GetMacroValue(CONTEXTID,"TimeOut"))
   {
   time = atoi(GetMacroValue(CONTEXTID,"TimeOut"));

   if (time < 3 || time > 60)
      {
      CfLog(cfinform,"TimeOut not between 3 and 60 seconds, ignoring.\n","");
      }
   else
      {
      CF_TIMEOUT = time;
      }
   }

if (NOSPLAY)
   {
   return;
   }

time = 0;
hash = Hash(VFQNAME);

if (!NOSPLAY)
   {
   if (GetMacroValue(CONTEXTID,"SplayTime"))
      {
      time = atoi(GetMacroValue(CONTEXTID,"SplayTime"));
      
      if (time < 0)
	 {
	 CfLog(cfinform,"SplayTime with negative value, ignoring.\n","");
	 return;
	 }
      
      if (!DONESPLAY)
	 {
	 if (!PARSEONLY)
	    {
	    DONESPLAY = true;
	    Verbose("Sleeping for SplayTime %d seconds\n\n",(int)(time*60*hash/hashtablesize));
	    sleep((int)(time*60*hash/hashtablesize));
	    }
	 }
      else
	 {
	 Verbose("Time splayed once already - not repeating\n");
	 }
      }
   } 

if (GetMacroValue(CONTEXTID,"LogTidyHomeFiles") && (strcmp(GetMacroValue(CONTEXTID,"LogTidyHomeFiles"),"off") == 0))
   {
   LOGTIDYHOMEFILES = false;
   }


}

/*******************************************************************/

void SetReferenceTime(setclasses)

int setclasses;

{ time_t tloc;
 
if ((tloc = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","");
   }

CFSTARTTIME = tloc;

snprintf(VBUFF,bufsize,"%s",ctime(&tloc));

Verbose("Reference time set to %s\n",ctime(&tloc));

if (setclasses)
   {
   AddTimeClass(VBUFF);
   }
}


/*******************************************************************/

void DoTree(passes,info)

int passes;
char *info;

{ int pass;
  struct Item *action;
 
for (pass = 1; pass <= passes; pass++)
   {
   for (action = VACTIONSEQ; action !=NULL; action=action->next)
      {
      SetStartTime(false);

      if (VJUSTACTIONS && (!IsItemIn(VJUSTACTIONS, action->name)))
         {
         continue;
         }

      if (VAVOIDACTIONS && IsItemIn(VAVOIDACTIONS, action->name))
         {
         continue;
         }
      
      if (IsExcluded(action->classes))
	 {
	 continue;
	 }

      if ((pass > 1) && NothingLeftToDo())
	 {
	 continue;
	 }

      Verbose("\n*********************************************************************\n");
      Verbose(" %s Sched: %s pass %d @ %s",info,action->name,pass,ctime(&CFINITSTARTTIME));
      Verbose("*********************************************************************\n\n");
      
      switch(EvaluateAction(action->name,&VADDCLASSES,pass))
	 {
	 case mountinfo:
	     if (pass == 1)
		{
		GetHomeInfo();
		GetMountInfo();
		}
	     break;
	     
	 case mkpaths:
	     if (!NOFILECHECK)
		{
		GetSetuidLog();
		MakePaths();
		SaveSetuidLog();
		}
	     break;
	     
	 case lnks:	     
	     MakeChildLinks();
	     MakeLinks();	     
	     break;
	     
	 case chkmail:
	     MailCheck();	     
	     break;
	     
	 case mountall:
	     if (!NOMOUNTS )
		{
		MountFileSystems();
		}
	     break;
	     
	 case requir:
	 case diskreq:

	     CheckRequired();
	     break;
	     
	 case tidyf:
	     if (!NOTIDY) 
		{
		TidyFiles();
		}
	     break;
	     
	 case shellcom:
	     if (!NOSCRIPTS)
		{
		Scripts();
		}
	     break;
	     
	 case chkfiles:
	     if (!NOFILECHECK)
		{
		GetSetuidLog();
		CheckFiles();
		SaveSetuidLog();
		}
	     break;
	     
	 case disabl:
	     DisableFiles();
	     break;
	     
	 case mountresc:
	     if (!NOMOUNTS)
		{
		if (!GetLock("Mountres",CanonifyName(VFSTAB[VSYSTEMHARDCLASS]),0,VEXPIREAFTER,VUQNAME,0))
		   {
		   exit(0);    /* Note IFELAPSED must be zero to avoid conflict with mountresc */
		   }
		
		MountHomeBinServers();
		MountMisc();
		ReleaseCurrentLock();
		}
	     break;
	     
	 case umnt:
	     Unmount();
	     break;
	     
	 case edfil:
	     if (!NOEDITS)
		{
		EditFiles();
		}
	     break;
	     
	     
	 case resolv:

	     if (pass > 1)
		{
		continue;
		}
	     
	     if (!GetLock(ASUniqueName("resolve"),"",VIFELAPSED,VEXPIREAFTER,VUQNAME,CFSTARTTIME))
		{
		continue;
		}
	     
	     CheckResolv();
	     ReleaseCurrentLock();
	     break;
	     
	 case imag:
	     if (!NOCOPY)
		{
		GetSetuidLog();
		MakeImages();
		SaveSetuidLog();
		}
	     break;
	     
	 case netconfig:
	     
	     if (IFCONF)
		{
		ConfigureInterfaces();
		}
	     break;
	     
	 case tzone:
	     CheckTimeZone();
	     break;
	     
	 case procs:
	     if (!NOPROCS)
		{
		CheckProcesses();
		}
	     break;
	     
	 case plugin:      break;
	     
	 default:  
	     snprintf(OUTPUT,bufsize*2,"Undefined action %s in sequence\n",action->name);
	     FatalError(OUTPUT);
	     break;
	 }
      
      DeleteItemList(VADDCLASSES);
      VADDCLASSES = NULL;
      }
   }
}


/*******************************************************************/

int NothingLeftToDo()

   /* Check the *probable* source for additional action */

{ struct ShellComm *vscript;
  struct Link *vlink;
  struct File *vfile;
  struct Disable *vdisablelist;
  struct File *vmakepath;
  struct Link *vchlink;
  struct UnMount *vunmount;
  struct Edit *veditlist;
  struct Process *vproclist;

for (vproclist = VPROCLIST; vproclist != NULL; vproclist=vproclist->next)
   {
   if (vproclist->done == 'n')
      {
      return false;
      }
   }

for (vscript = VSCRIPT; vscript != NULL; vscript=vscript->next)
   {
   if (vscript->done == 'n')
      {
      return false;
      }
   }

for (vfile = VFILE; vfile != NULL; vfile=vfile->next)
   {
   if (vfile->done == 'n')
      {
      return false;
      }
   }

for (veditlist = VEDITLIST; veditlist != NULL; veditlist=veditlist->next)
   {
   if (veditlist->done == 'n')
      {
      return false;
      }
   }

for (vdisablelist = VDISABLELIST; vdisablelist != NULL; vdisablelist=vdisablelist->next)
   {
   if (vdisablelist->done == 'n')
      {
      return false;
      }
   }

for (vmakepath = VMAKEPATH; vmakepath != NULL; vmakepath=vmakepath->next)
   {
   if (vmakepath->done == 'n')
      {
      return false;
      }
   }

for (vlink = VLINK; vlink != NULL; vlink=vlink->next)
   {
   if (vlink->done == 'n')
      {
      return false;
      }
   }

for (vchlink = VCHLINK; vchlink != NULL; vchlink=vchlink->next)
   {
   if (vchlink->done == 'n')
      {
      return false;
      }
   }


for (vunmount = VUNMOUNT; vunmount != NULL; vunmount=vunmount->next)
   {
   if (vunmount->done == 'n')
      {
      return false;
      }
   }

return true; 
}

/*******************************************************************/

enum aseq EvaluateAction(action,classlist,pass)

char *action;
struct Item **classlist;
int pass;

{ int i,j = 0;
  char *sp,cbuff[bufsize],actiontxt[bufsize],mod[bufsize],args[bufsize];
  struct Item *ip;

cbuff[0]='\0';
actiontxt[0]='\0';
mod[0] = args[0] = '\0'; 
sscanf(action,"%s %[^\n]",mod,args);
sp = mod;

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
      snprintf(OUTPUT,bufsize*2,"Error in action sequence: %s\n",action);
      CfLog(cferror,OUTPUT,"");
      FatalError("You cannot add a reserved class!");
      }
 
   if (j == 1)
      {
      VIFELAPSED = VDEFAULTIFELAPSED;
      VEXPIREAFTER = VDEFAULTEXPIREAFTER;
      strcpy(actiontxt,cbuff);
      continue;
      }
   else
      {
      if ((strncmp(actiontxt,"module:",7) != 0) && ! IsSpecialClass(cbuff))
	 {
         AppendItem(classlist,cbuff,NULL);
	 }
      }
   }

BuildClassEnvironment();

if ((VERBOSE || DEBUG || D2) && *classlist != NULL)
   {
   printf("\n                  New temporary class additions\n");
   printf("                  --------( Pass %d )-------\n",pass);
   for (ip = *classlist; ip != NULL; ip=ip->next)
      {
      printf("                             %s\n",ip->name);
      }
   }

ACTION = none;
 
for (i = 0; ACTIONSEQTEXT[i] != NULL; i++)
   {
   if (strcmp(ACTIONSEQTEXT[i],actiontxt) == 0)
      {
      Debug("Actionsequence item %s\n",actiontxt);
      ACTION = i;
      return (enum aseq) i;
      }
   }

Debug("Checking if entry is a module\n");
 
if (strncmp(actiontxt,"module:",7) == 0)
   {
   if (pass == 1)
      {
      CheckForModule(actiontxt,args);
      }
   return plugin;
   }

Verbose("No action matched %s\n",actiontxt);
return(non);
}


/*******************************************************************/

void SummarizeObjects()

{ struct cfObject *op;

 Verbose("\n\n++++++++++++++++++++++++++++++++++++++++\n");
 Verbose("Summary of objects involved\n");
 Verbose("++++++++++++++++++++++++++++++++++++++++\n\n");
 
for (op = VOBJ; op != NULL; op=op->next)
   {
   Verbose("    %s\n",op->scope);
   }
}


/*******************************************************************/
/* Level 2                                                         */
/*******************************************************************/

void CheckOpts(argc,argv)

char **argv;
int argc;

{ extern char *optarg;
  struct Item *actionList;
  int optindex = 0;
  int c;

while ((c=getopt_long(argc,argv,"bBzMgAbKqkhYHd:vlniIf:pPmcCtsSaeEVD:N:LwxXuUj:o:",OPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'E': printf("%s: the enforce-links option can only be used in interactive mode.\n",VPREFIX);
                printf("%s: Do you really want to blindly enforce ALL links? (y/n)\n",VPREFIX);
                if (getchar() != 'y')
                   {
                   printf("cfagent: aborting\n");
		   closelog();
                   exit(1);
                   }

                ENFORCELINKS = true;
                break;

      case 'B': UPDATEONLY = true;
  	        break;

      case 'f': strcpy(VINPUTFILE,optarg);
                MINUSF = true;
                break;

      case 'g': CHECK  = true;
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
                   default:  DEBUG = true;
                             break;
                   }
                break;

      case 'M':
		NOMODULES = true;
		break;
		
      case 'K': IGNORELOCK = true;
	        break;

      case 'A': IGNORELOCK = true;
	        break;
		
      case 'D': AddCompoundClass(optarg);
                break;

      case 'N': NegateCompoundClass(optarg,&VNEGHEAP);
                break;

      case 'b': FORCENETCOPY = true;
	        break;

      case 'e': NOEDITS = true;
                break;

      case 'i': IFCONF = false;
                break;

      case 'I': INFORM = true;
	        break;

      case 'v': VERBOSE = true;
                break;

      case 'l': TRAVLINKS = true;
                break;

      case 'n': DONTDO = true;
	        IGNORELOCK = true;
	        AddClassToHeap("opt_dry_run");
                break;

      case 'p': PARSEONLY = true;
                IGNORELOCK = true;
                break;

      case 'm': NOMOUNTS = true;
                break;

      case 'c': NOFILECHECK = true;
                break;

      case 'C': MOUNTCHECK = true;
                break;

      case 't': NOTIDY = true;
                break;

      case 's': NOSCRIPTS = true;
                break;

      case 'a': PRSYSADM = true;
                IGNORELOCK = true;
		PARSEONLY = true;
                break;

      case 'z': PRMAILSERVER = true;
	        IGNORELOCK = true;
		PARSEONLY = true;
		break;

      case 'L': KILLOLDLINKS = true;
                break;

      case 'V': printf("GNU cfengine %s\n%s\n",VERSION,COPYRIGHT);
	        printf("This program is covered by the GNU Public License and may be\n");
		printf("copied free of charge.  No warranty is implied.\n\n");
                exit(0);

      case 'h': Syntax();
                exit(0);

      case 'x': NOPRECONFIG = true;
                break;

      case 'w': WARNINGS = false;
                break;

      case 'X': NOLINKS = true;
                break;

      case 'k': NOCOPY = true;
                break;

      case 'S': SILENT = true;
                break;

      case 'u': USEENVIRON = true;
                break;

      case 'U': UNDERSCORE_CLASSES = true;
	        break;

      case 'H': NOHARDCLASSES = true;
	        break;

      case 'P': NOPROCS = true;
	        break;

      case 'q': NOSPLAY = true;
	        break;

      case 'Y': CFPARANOID = true;
	        break;

      case 'j': actionList = SplitStringAsItemList(optarg, ',');
                VJUSTACTIONS = ConcatLists(actionList, VJUSTACTIONS);
                break;

      case 'o': actionList = SplitStringAsItemList(optarg, ',');
                VAVOIDACTIONS = ConcatLists(actionList, VAVOIDACTIONS);
                break;		

      default:  Syntax();
                exit(1);

      }
   }
}

/*******************************************************************/

int GetResource(var)

char *var;

{ int i;

for (i = 0; VRESOURCES[i] != '\0'; i++)
   {
   if (strcmp(VRESOURCES[i],var)==0)
      {
      return i;
      }
   }

snprintf (VBUFF,bufsize,"Unknown resource %s in %s",var,VRCFILE);

FatalError(VBUFF);

return 0; /* This to placate insight */
}

/*******************************************************************/

void SetStartTime(setclasses)

int setclasses;

{ time_t tloc;
 
if ((tloc = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","");
   }

CFINITSTARTTIME = tloc;

Verbose("Job start time set to %s\n",ctime(&tloc));
}

/*******************************************************************/

void BuildClassEnvironment()

{ struct Item *ip;

Debug("(BuildClassEnvironment)\n");

snprintf(ALLCLASSBUFFER,bufsize,"%s=",CFALLCLASSESVAR);

for (ip = VHEAP; ip != NULL; ip=ip->next)
   {
   if (IsDefinedClass(ip->name))
      {
      if (BufferOverflow(ALLCLASSBUFFER,ip->name))
	 {
	 printf("%s: culprit BuildClassEnvironment()\n",VPREFIX);
	 return;
	 }

      strcat(ALLCLASSBUFFER,ip->name);
      strcat(ALLCLASSBUFFER,":");
      }
   }

for (ip = VALLADDCLASSES; ip != NULL; ip=ip->next)
   {
   if (IsDefinedClass(ip->name))
      {
      if (BufferOverflow(ALLCLASSBUFFER,ip->name))
	 {
	 printf("%s: culprit BuildClassEnvironment()\n",VPREFIX);
	 return;
	 }
      
      strcat(ALLCLASSBUFFER,ip->name);
      strcat(ALLCLASSBUFFER,":");
      }
   }

Debug2("---\nENVIRONMENT: %s\n---\n",ALLCLASSBUFFER);

if (USEENVIRON)
   {
   if (putenv(ALLCLASSBUFFER) == -1)
      {
      perror("putenv");
      }
   }
}

/*******************************************************************/

void CheckForModule(actiontxt,args)

char *actiontxt,*args;

{ struct stat statbuf;
  char line[bufsize],command[bufsize],name[maxvarsize],content[bufsize],*sp;
  FILE *pp;
  int print;

if (NOMODULES)
   {
   return;
   }
  
bzero(VBUFF,bufsize);

if (GetMacroValue(CONTEXTID,"moduledirectory"))
   {
   ExpandVarstring("$(moduledirectory)",VBUFF,NULL);
   }
else
   {
   snprintf(OUTPUT,bufsize*2,"Plugin was called in ActionSequence but moduledirectory was not defined\n");
   CfLog(cfinform,OUTPUT,"");
   return;
   }

AddSlash(VBUFF);
strcat(VBUFF,actiontxt);
 
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(OUTPUT,bufsize*2,"(Plug-in %s not found)",VBUFF);
   Banner(OUTPUT);
   return;
   }

if ((statbuf.st_uid != 0) && (statbuf.st_uid != getuid()))
   {
   snprintf(OUTPUT,bufsize*2,"Module %s was not owned by uid=%d executing cfagent\n",VBUFF,getuid());
   CfLog(cferror,OUTPUT,"");
   return;
   }
  
snprintf(OUTPUT,bufsize*2,"Plug-in `%s\'",actiontxt);
Banner(OUTPUT);

strcat(VBUFF," ");

if (BufferOverflow(VBUFF,args))
   {
   snprintf(OUTPUT,bufsize*2,"Culprit: class list for module (shouldn't happen)\n" );
   CfLog(cferror,OUTPUT,"");
   return;
   }

strcat(VBUFF,args); 
ExpandVarstring(VBUFF,command,NULL); 
 
Verbose("Exec module [%s]\n",command); 
   
if ((pp = cfpopen(command,"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Couldn't open pipe from %s\n",actiontxt);
   CfLog(cferror,OUTPUT,"cfpopen");
   return;
   }

while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      snprintf(OUTPUT,bufsize*2,"Shell command pipe %s\n",actiontxt);
      CfLog(cferror,OUTPUT,"ferror");
      break;
      }
   
   ReadLine(line,bufsize,pp);

   if (strlen(line) > bufsize - 80)
      {
      snprintf(OUTPUT,bufsize*2,"Line from module %s is too long to be sensible\n",actiontxt);
      CfLog(cferror,OUTPUT,"");
      break;
      }
   
   if (ferror(pp))  /* abortable */
      {
      snprintf(OUTPUT,bufsize*2,"Shell command pipe %s\n",actiontxt);
      CfLog(cferror,OUTPUT,"ferror");
      break;
      }	 
   
   print = false;
   
   for (sp = line; *sp != '\0'; sp++)
      {
      if (! isspace((int)*sp))
	 {
	 print = true;
	 break;
	 }
      }
   
   switch (*line)
      {
      case '+':
	  Verbose("Activated classes: %s\n",line+1);
	  CheckClass(line+1,command);
	  AddMultipleClasses(line+1);
	  break;
      case '-':
	  Verbose("Deactivated classes: %s\n",line+1);
	  CheckClass(line+1,command);
	  NegateCompoundClass(line+1,&VNEGHEAP);
	  break;
      case '=':
          sscanf(line+1,"%[^=]=%[^\n]",name,content);
	  AddMacroValue(CONTEXTID,name,content);
	  break;

      default:
	  if (print)
	     {
	     snprintf(OUTPUT,bufsize,"%s: %s\n",actiontxt,line);
	     CfLog(cferror,OUTPUT,"");
	     }
      }
   }

cfpclose(pp);
}

/*******************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine: A system configuration engine (cfagent)\n%s\n%s\n",VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; OPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",OPTIONS[i].name,(char)OPTIONS[i].val);
   }

printf("\nDebug levels: 1=parsing, 2=running, 3=summary\n");

printf("\nBug reports to bug-cfengine@gnu.org (News: gnu.cfengine.bug)\n");
printf("General help to help-cfengine@gnu.org (News: gnu.cfengine.help)\n");
printf("Info & fixes at http://www.iu.hio.no/cfengine\n");
}


/******************************************************************/
/* Level 3                                                        */
/******************************************************************/

void CheckClass(name,source)

char *name,*source;

{ char *sp;

 for (sp = name; *sp != '\0'; sp++)
    {
    if (!isalnum((int)*sp) && (*sp != '_'))
       {
       snprintf(OUTPUT,bufsize,"Module class contained an illegal character (%c). Only alphanumeric or underscores in classes.",*sp);
       CfLog(cferror,OUTPUT,"");
       }
    }
 
}



/* EOF */
