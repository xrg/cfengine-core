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
void Syntax ARGLIST((void));
void EmptyActionSequence ARGLIST((void));
void GetEnvironment ARGLIST((void));
int NothingLeftToDo ARGLIST((void));
void SummarizeObjects ARGLIST((void));
void SetContext ARGLIST((char *id));
void DeleteCaches ARGLIST((void));
void QueryCheck(void);

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main(int argc,char *argv[])

{ struct Item *ip;
    
SetContext("global");
SetSignals(); 
 
signal (SIGTERM,HandleSignal);                   /* Signal Handler */
signal (SIGHUP,HandleSignal);
signal (SIGINT,HandleSignal);
signal (SIGPIPE,HandleSignal);
signal (SIGUSR1,HandleSignal);
signal (SIGUSR2,HandleSignal);

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

 if (IsPrivileged() && !MINUSF && !PRSCHEDULE)
    {
    SetContext("update");

    if (ParseInputFile("update.conf"))
       {
       CheckSystemVariables();

       if (!PARSEONLY && (QUERYVARS == NULL))
          {
          DoTree(2,"Update");
          DoAlerts();
          EmptyActionSequence();
          DeleteClassesFromContext("update");
          DeleteCaches();
          }

       VIMPORT = NULL;
       }
    else
       {
       Verbose("Skipping update.conf (-F=%d)\n",MINUSF);
       }
    
    if (ERRORCOUNT > 0)
       {
       exit(1);
       }
    }
 
 if (UPDATEONLY)
    {
    return 0;
    }
 
 SetContext("main");
 
 if (!PARSEONLY)
    {
    PersistentClassesToHeap();
    GetEnvironment();
    }
 
 ParseInputFile(VINPUTFILE);
 CheckFilters();
 EchoValues();
 
/* CheckForMethod(); Move to InitializeAction */
 
 if (PRSYSADM)                                           /* -a option */
    {
    printf("%s\n",VSYSADM);
    exit (0);
    }

 if (PRSCHEDULE)
    {
    for (ip = SCHEDULE; ip != NULL; ip=ip->next)
       {
       printf("[%s]\n",ip->name);
       }
    
    printf("\n");
    exit(0);
    }
 
 if (ERRORCOUNT > 0)
    {
    exit(1);
    }
 
 if (PARSEONLY)                            /* Establish lock for root */
    {
    exit(0);
    } 
 
 CfOpenLog();
 CheckSystemVariables();
 
 SetReferenceTime(false); /* Reset */

 QueryCheck();
     
 DoTree(4,"Main Tree"); 
 DoAlerts();
 
 CheckMethodReply();
 
 if (OptionIs(CONTEXTID,"ChecksumPurge", true)) 
    {
    ChecksumPurge();
    }
 
 SummarizeObjects();
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

ISCFENGINE = true;
VFACULTY[0] = '\0';
VSYSADM[0] = '\0';
VNETMASK[0]= '\0';
VBROADCAST[0] = '\0';
VMAILSERVER[0] = '\0';
ALLCLASSBUFFER[0] = '\0';
VREPOSITORY = strdup("\0");

strcpy(METHODNAME,"cf-nomethod"); 
METHODREPLYTO[0] = '\0';
METHODFOR[0] = '\0';
 
#ifndef HAVE_REGCOMP
re_syntax_options |= RE_INTERVALS;
#endif
 
strcpy(VINPUTFILE,"cfagent.conf");
strcpy(VNFSTYPE,"nfs");

IDClasses();
 
/* Note we need to fix the options since the argv mechanism doesn't */
/* work when the shell #!/bla/cfengine -v -f notation is used.      */
/* Everything ends up inside a single argument! Here's the fix      */

cfargc = 1;
cfargv[0]="cfagent";

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

 /* XXX Initialize workdir for non privileged users */

 strcpy(CFWORKDIR,WORKDIR);

 if (getuid() > 0)
    {
    char *homedir;
    if ((homedir = getenv("HOME")) != NULL)
       {
       strcpy(CFWORKDIR,homedir);
       strcat(CFWORKDIR,"/.cfagent");
       }
    }
 
 sprintf(ebuff,"%s/state/cf_procs",CFWORKDIR);
 
 if (stat(ebuff,&statbuf) == -1)
    {
    CreateEmptyFile(ebuff);
    }
 
 strcpy(VLOGDIR,CFWORKDIR); 
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

 AddInstallable("no_default_route");
 CfenginePort();
 StrCfenginePort();
}

/*******************************************************************/

void PreNetConfig()                           /* Execute a shell script */

{ struct stat buf;
  char comm[CF_BUFSIZE],ebuff[CF_EXPANDSIZE];
  char *sp;
  FILE *pp;

if (NOPRECONFIG)
   {
   CfLog(cfverbose,"Ignoring the cf.preconf file: option set","");
   return;
   }

strcpy(VPREFIX,"cfengine:");
strcat(VPREFIX,VUQNAME);
 
if ((sp=getenv(CF_INPUTSVAR)) != NULL)
   {
   snprintf(comm,CF_BUFSIZE,"%s/%s",sp,VPRECONFIG);

   if (stat(comm,&buf) == -1)
       {
       CfLog(cfverbose,"No preconfiguration file","");
       return;
       }
   
   snprintf(comm,CF_BUFSIZE,"%s/%s %s 2>&1",sp,VPRECONFIG,CLASSTEXT[VSYSTEMHARDCLASS]);
   }
else
   {
   snprintf(comm,CF_BUFSIZE,"%s/%s",CFWORKDIR,VPRECONFIG);
   
   if (stat(comm,&buf) == -1)
      {
      CfLog(cfverbose,"No preconfiguration file\n","");
      return;
      }
   
   snprintf(comm,CF_BUFSIZE,"%s/%s %s",CFWORKDIR,VPRECONFIG,CLASSTEXT[VSYSTEMHARDCLASS]);
   }

 if (S_ISDIR(buf.st_mode) || S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode))
    {
    snprintf(OUTPUT,CF_BUFSIZE*2,"Error: %s was not a regular file\n",VPRECONFIG);
    CfLog(cferror,OUTPUT,"");
    FatalError("Aborting.");
    }
 
Verbose("\n\nExecuting Net Preconfiguration script...%s\n\n",VPRECONFIG);
 
if ((pp = cfpopen(comm,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Failed to open pipe to %s\n",comm);
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

   ReadLine(ebuff,CF_BUFSIZE,pp);

   if (feof(pp))
      {
      break;
      }
   
   if (strstr(ebuff,"cfengine-preconf-abort"))
      {
      exit(2);
      }

   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error running preconfig\n","ferror");
      break;
      }

   CfLog(cfinform,ebuff,"");
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

{ char filename[CF_BUFSIZE], buffer[CF_BUFSIZE], *mp;
  char class[CF_MAXVARSIZE], variable[CF_MAXVARSIZE], value[CF_MAXVARSIZE];
  int c;
  FILE *fp;

filename[0] = buffer[0] = class[0] = variable[0] = value[0] = '\0';
LINENUMBER = 0;

snprintf(filename,CF_BUFSIZE,"%s/inputs/%s",CFWORKDIR,VRCFILE);
if ((fp = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   return;
   }

while (!feof(fp))
   {
   ReadLine(buffer,CF_BUFSIZE,fp);
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Malformed line (missing :) in resource file %s - skipping\n",VRCFILE);
      CfLog(cferror,OUTPUT,"");
      continue;
      }

   sscanf(buffer,"%[^.].%[^:]:%[^\n]",class,variable,value);

   if (class[0] == '\0' || variable[0] == '\0' || value[0] == '\0')
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"%s:%s - Bad resource\n",VRCFILE,buffer);
      CfLog(cferror,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"class=%s,variable=%s,value=%s\n",class,variable,value);
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

   snprintf(OUTPUT,CF_BUFSIZE*2,"Redefining resource %s as %s (%s)\n",variable,value,class);
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
                        snprintf(OUTPUT,CF_BUFSIZE,"Bad resource %s in %s\n",variable,VRCFILE);
                        FatalError(OUTPUT);
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

{ char env[CF_BUFSIZE],class[CF_BUFSIZE],name[CF_MAXVARSIZE],value[CF_MAXVARSIZE];
  FILE *fp;
  struct stat statbuf;
  time_t now = time(NULL);
  
Verbose("Looking for environment from cfenvd...\n");
snprintf(env,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_ENV_FILE);

if (stat(env,&statbuf) == -1)
   {
   Verbose("\nUnable to detect environment from cfenvd\n\n");
   return;
   }

 if (statbuf.st_mtime < (now - 60*60))
    {
    Verbose("Environment data are too old - discarding\n");
    unlink(env);
    return;
    }
 
 if (!GetMacroValue(CONTEXTID,"env_time"))
    {
    snprintf(value,CF_MAXVARSIZE-1,"%s",ctime(&statbuf.st_mtime));
    Chop(value);
    AddMacroValue(CONTEXTID,"env_time",value);
    }
 else
    {
    CfLog(cferror,"Reserved variable $(env_time) in use","");
    }

Verbose("Loading environment...\n");
 
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
Verbose("Environment data loaded\n\n"); 
}

/*******************************************************************/

void EchoValues()

{ struct Item *ip,*p2;
  char ebuff[CF_EXPANDSIZE];

ebuff[0] = '\0';
  
Verbose("Accepted domain name: %s\n\n",VDOMAIN); 

if (GetMacroValue(CONTEXTID,"OutputPrefix"))
   {
   ExpandVarstring("$(OutputPrefix)",ebuff,NULL);
   }

if (strlen(ebuff) != 0)
   {
   strncpy(VPREFIX,ebuff,40);  /* No more than 40 char prefix (!) */
   }
else
   {
   strcpy(VPREFIX,"cfengine:");
   strcat(VPREFIX,VUQNAME);
   }

p2 = SortItemListNames(VHEAP);
VHEAP = p2;


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
   if (VDEFAULTROUTE != NULL)
      {
      if (IsDefinedClass(VDEFAULTROUTE->classes))
         {
         printf("\nDefault route for packets %s\n\n",VDEFAULTROUTE->name);
         }
      }
   printf("\nFile repository = %s\n\n",VREPOSITORY);
   printf("\nNet interface name = %s\n",VIFDEV[VSYSTEMHARDCLASS]);
   printf("------------------------------------------------------------\n");
   ListDefinedVariables();
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
   ListDefinedPackages();
   printf("------------------------------------------------------------\n");
   ListDefinedMethods();
   printf("------------------------------------------------------------\n");

   
   if (IGNORELOCK)
      {
      printf("\nIgnoring locks...\n");
      }
   }
}

/*******************************************************************/

void CheckSystemVariables()

{ char id[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];
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
   FatalError("Access denied - user not listed in access list");
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


 if (IsDefinedClass("nt"))
    {
    AddClassToHeap("windows");
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
   Verbose("\n[LogDirectory is no longer runtime configurable: use configure --with-workdir=CFWORKDIR ]\n\n");
   }

Verbose("LogDirectory = %s\n",VLOGDIR);
  
LoadSecretKeys();
 
if (GetMacroValue(CONTEXTID,"childlibpath"))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"LD_LIBRARY_PATH=%s",GetMacroValue(CONTEXTID,"childlibpath"));
   if (putenv(strdup(OUTPUT)) == 0)
      {
      Verbose("Setting %s\n",OUTPUT);
      }
   else
      {
      Verbose("Failed to set %s\n",GetMacroValue(CONTEXTID,"childlibpath"));
      }
   }

if (GetMacroValue(CONTEXTID,"BindToInterface"))
   {
   ExpandVarstring("$(BindToInterface)",ebuff,NULL);
   strncpy(BINDINTERFACE,ebuff,CF_BUFSIZE-1);
   Debug("$(BindToInterface) Expanded to %s\n",BINDINTERFACE);
   } 

if (GetMacroValue(CONTEXTID,"MaxCfengines"))
   {
   activecfs = atoi(GetMacroValue(CONTEXTID,"MaxCfengines"));
 
   locks = CountActiveLocks();
 
   if (locks >= activecfs)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Too many cfagents running (%d/%d)\n",locks,activecfs);
      CfLog(cferror,OUTPUT,"");
      closelog();
      exit(1);
      }
   }


if (OptionIs(CONTEXTID,"SkipIdentify",true))
   {
   SKIPIDENTIFY = true;
   }

if (OptionIs(CONTEXTID,"Verbose",true))
   {
   VERBOSE = true;
   }

if (OptionIs(CONTEXTID,"LastSeen",false))
   {
   LASTSEEN = false;
   }

if (OptionIs(CONTEXTID,"Inform",true))
   {
   INFORM = true;
   } 

if (OptionIs(CONTEXTID,"Exclamation",false))
   {
   EXCLAIM = false;
   } 

INFORM_save = INFORM;

if (OptionIs(CONTEXTID,"Syslog",true))
   {
   LOGGING = true;
   }

LOGGING_save = LOGGING;

if (OptionIs(CONTEXTID,"DryRun",true))
   {
   DONTDO = true;
   AddClassToHeap("opt_dry_run");
   }

if (GetMacroValue(CONTEXTID,"BinaryPaddingChar"))
   {
   strcpy(ebuff,GetMacroValue(CONTEXTID,"BinaryPaddingChar"));
   
   if (ebuff[0] == '\\')
      {
      switch (ebuff[1])
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
      PADCHAR = ebuff[0];
      }
   }
 
 
if (OptionIs(CONTEXTID,"Warnings",true))
   {
   WARNINGS = true;
   }

if (OptionIs(CONTEXTID,"NonAlphaNumFiles",true))
   {
   NONALPHAFILES = true;
   }

if (OptionIs(CONTEXTID,"SecureInput",true))
   {
   CFPARANOID = true;
   }

if (OptionIs(CONTEXTID,"ShowActions",true))
   {
   SHOWACTIONS = true;
   }

 if (GetMacroValue(CONTEXTID,"Umask"))
   {
   mode_t val;
   val = (mode_t)atoi(GetMacroValue(CONTEXTID,"Umask"));
   if (umask(val) == (mode_t)-1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't set umask to %o\n",val);
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
   ExpandVarstring("$(ChecksumDatabase)",ebuff,NULL);

   CHECKSUMDB = strdup(ebuff);

   if (*CHECKSUMDB != '/')
      {
      FatalError("$(ChecksumDatabase) does not expand to an absolute filename\n");
      }
   }
else
   {
   snprintf(ebuff,CF_BUFSIZE,"%s/checksum.db",CFWORKDIR);
   CHECKSUMDB = strdup(ebuff);
   }

if (SHOWDB)
   {
   printf("%s\n",CHECKSUMDB);
   exit(0);
   }
 
Verbose("Checksum database is %s\n",CHECKSUMDB); 
 
if (GetMacroValue(CONTEXTID,"CompressCommand"))
   {
   ExpandVarstring("$(CompressCommand)",ebuff,NULL);

   COMPRESSCOMMAND = strdup(ebuff);
   
   if (*COMPRESSCOMMAND != '/')
      {
      FatalError("$(CompressCommand) does not expand to an absolute filename\n");
      }
   }

if (OptionIs(CONTEXTID,"ChecksumUpdates",true))
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
snprintf(ebuff,CF_BUFSIZE,"%s+%s+%d",VFQNAME,VIPADDRESS,getuid());
hash = Hash(ebuff);

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
            Verbose("Sleeping for SplayTime %d seconds\n\n",(int)(time*60*hash/CF_HASHTABLESIZE));
            sleep((int)(time*60*hash/CF_HASHTABLESIZE));
            }
         }
      else
         {
         Verbose("Time splayed once already - not repeating\n");
         }
      }
   } 
 
 if (OptionIs(CONTEXTID,"LogTidyHomeFiles",false))
    {
    LOGTIDYHOMEFILES = false;
    } 
}


/*******************************************************************/

void QueryCheck()

{ char src[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];
  struct Item *ip;
 
if (QUERYVARS == NULL)
   {
   return;
   }

for (ip = QUERYVARS; ip != NULL; ip=ip->next)
   {
   snprintf(src,CF_MAXVARSIZE,"$(%s)",ip->name);
   ExpandVarstring(src,ebuff,"");
   printf("%s=%s\n",ip->name,ebuff);
   }

exit(0);
}

/*******************************************************************/

void DoTree(int passes,char *info)

{ struct Item *action;
 
for (PASS = 1; PASS <= passes; PASS++)
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
      
      if ((PASS > 1) && NothingLeftToDo())
         {
         break;
         }
      
      Verbose("\n*********************************************************************\n");
      Verbose(" %s Sched: %s pass %d @ %s",info,action->name,PASS,ctime(&CFINITSTARTTIME));
      Verbose("*********************************************************************\n\n");
      
      switch(ACTION = EvaluateAction(action->name,&VADDCLASSES,PASS))
         {
         case mountinfo:
             if (PASS == 1)
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
             if (!NOMOUNTS && PASS == 1)
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
         case renam:
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
             
             if (PASS > 1)
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
             
         case meths:
             if (!NOMETHODS)
                {
                DoMethods();
                }
             break;
             
         case pkgs:
             CheckPackages();
             break;
             
         case plugin:
             break;
             
         default:  
             snprintf(OUTPUT,CF_BUFSIZE*2,"Undefined action %s in sequence\n",action->name);
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
  struct Tidy *vtidy;
  struct Package *vpkg;

if (IsWildItemIn(VACTIONSEQ,"process*"))
   {
   for (vproclist = VPROCLIST; vproclist != NULL; vproclist=vproclist->next)
      {
      if ((vproclist->done == 'n') && IsDefinedClass(vproclist->classes))
         {
         printf("Still waiting for rule:: Proc <%s> / %s\n",vproclist->expr,vproclist->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"shellcommand*"))
   {
   for (vscript = VSCRIPT; vscript != NULL; vscript=vscript->next)
      {
      if (vscript->done == 'n'  && IsDefinedClass(vscript->classes))
         {
         printf("Still waiting for rule:: Shell <%s> / %s\n",vscript->name,vscript->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"file*"))
   {
   for (vfile = VFILE; vfile != NULL; vfile=vfile->next)
      {
      if (vfile->done == 'n' && IsDefinedClass(vfile->classes))
         {
         printf("Still waiting for rule:: File <%s>/ %s\n",vfile->path,vfile->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"tid*"))
   {
   for (vtidy = VTIDY; vtidy != NULL; vtidy=vtidy->next)
      {
      if (vtidy->done == 'n')
         {
         printf("Still waiting for rule:: Tidy <%s>\n",vtidy->path);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"editfile*"))
   {
   for (veditlist = VEDITLIST; veditlist != NULL; veditlist=veditlist->next) 
      { 
      struct Edlist *ep;
      int something_to_do = false;
      
      for (ep = veditlist->actions; ep != NULL; ep=ep->next)
         {
         if (IsDefinedClass(ep->classes))
            {
            something_to_do = true;
            printf("Defined Edit %s / %s\n",ep->data,ep->classes);
            break;
            }
         }
   
      if (veditlist->done == 'n' && something_to_do)
         {
         printf("Still waiting for rule:: Edit <%s>\n",veditlist->fname);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"process*"))
   {
   for (vdisablelist = VDISABLELIST; vdisablelist != NULL; vdisablelist=vdisablelist->next)
      {
      if (vdisablelist->done == 'n' && IsDefinedClass(vdisablelist->classes))
         {
         printf("Still waiting for rule:: Disable <%s> / %s\n",vdisablelist->name,vdisablelist->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"director*"))
   {
   for (vmakepath = VMAKEPATH; vmakepath != NULL; vmakepath=vmakepath->next)
      {
      if (vmakepath->done == 'n' && IsDefinedClass(vmakepath->classes))
         {
         printf("Still waiting for rule:: makePath <%s>\n",vmakepath->path);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"link*"))
   {
   for (vlink = VLINK; vlink != NULL; vlink=vlink->next)
      {
      if (vlink->done == 'n' && IsDefinedClass(vlink->classes))
         {
         printf("Still waiting for rule:: Link <%s>\n",vlink->from);
         return false;
         }
      }
   
   for (vchlink = VCHLINK; vchlink != NULL; vchlink=vchlink->next)
      {
      if (vchlink->done == 'n' && IsDefinedClass(vchlink->classes))
         {
         printf("Still waiting for rule:: CLink <%s>\n",vlink->from);
         return false;
         }
      }
   }


if (IsWildItemIn(VACTIONSEQ,"unmoun*"))
   {
   for (vunmount = VUNMOUNT; vunmount != NULL; vunmount=vunmount->next)
      {
      if (vunmount->done == 'n' && IsDefinedClass(vunmount->classes))
         {
         printf("Still waiting for rule:: Umount <%s>\n",vunmount->name);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"packag*"))
   {
   for (vpkg = VPKG; vpkg != NULL; vpkg=vpkg->next)
      {
      if (vpkg->done == 'n' && IsDefinedClass(vpkg->classes))
         {
         printf("Still waiting for rule:: Packages <%s>\n",vpkg->name);
         return false;
         }
      }
   }

return true; 
}

/*******************************************************************/

enum aseq EvaluateAction(char *action,struct Item **classlist,int pass)

{ int i,j = 0;
  char *sp,cbuff[CF_BUFSIZE],actiontxt[CF_BUFSIZE],mod[CF_BUFSIZE],args[CF_BUFSIZE];
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
      snprintf(OUTPUT,CF_BUFSIZE*2,"Error in action sequence: %s\n",action);
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

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
  struct Item *actionList;
  int optindex = 0;
  int c;

  
while ((c=getopt_long(argc,argv,"WbBzMgAbKqkhYHd:vlniIf:pPmcCtsSaeEVD:N:LwxXuUj:o:Z:Q:",OPTIONS,&optindex)) != EOF)
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

      case 'f':
          strncpy(VINPUTFILE,optarg, CF_BUFSIZE-1);
          VINPUTFILE[CF_BUFSIZE-1] = '\0';
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
             case '4': D4 = true;
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
          
      case 'D': AddMultipleClasses(optarg);
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
          
      case 'l': FatalError("Option -l is deprecated -- too dangerous");
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
          
      case 'z':
          PRSCHEDULE = true;
          IGNORELOCK = true;
          PARSEONLY = true;
          break;
          
      case 'Z': strncpy(METHODMD5,optarg,CF_BUFSIZE-1);
          Debug("Got method call reference %s\n",METHODMD5);
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

      case 'W': SHOWDB = true; 
          break;
          
      case 'Y': CFPARANOID = true;
          break;
          
      case 'j': actionList = SplitStringAsItemList(optarg, ',');
          VJUSTACTIONS = ConcatLists(actionList, VJUSTACTIONS);
          break;
          
      case 'o': actionList = SplitStringAsItemList(optarg, ',');
          VAVOIDACTIONS = ConcatLists(actionList, VAVOIDACTIONS);
          break;

      case 'Q':
          if (optarg == NULL || strlen(optarg) == 0)
             {
             exit(1);
             }
          
          IGNORELOCK = true;
          NOSPLAY = true;
          QUERYVARS = SplitStringAsItemList(optarg,',');
          break;
          
      default:  Syntax();
          exit(1);
          
      }
  }
}


/*******************************************************************/

int GetResource(char *var)

{ int i;

for (i = 0; VRESOURCES[i] != '\0'; i++)
   {
   if (strcmp(VRESOURCES[i],var)==0)
      {
      return i;
      }
   }

snprintf (OUTPUT,CF_BUFSIZE,"Unknown resource %s in %s",var,VRCFILE);
FatalError(OUTPUT);
return 0;
}

/*******************************************************************/

void BuildClassEnvironment()

{ struct Item *ip;
 int size = 0;
 char file[CF_BUFSIZE], *sp;
 FILE *fp;
 
Debug("(BuildClassEnvironment)\n");

snprintf(ALLCLASSBUFFER,CF_BUFSIZE,"%s=",CF_ALLCLASSESVAR);

if (!IsPrivileged())
   {
   Verbose("\n(Non privileged user...)\n\n");
   
   if ((sp = getenv("HOME")) == NULL)
      {
      FatalError("You do not have a HOME variable pointing to your home directory");
      }  

   snprintf(file,CF_BUFSIZE,"%s/.cfagent/allclasses",sp);
   }
else
   {
   snprintf(file,CF_BUFSIZE,"%s/state/allclasses",WORKDIR);
   }


if ((fp = fopen(file,"w")) == NULL)
   {
   CfLog(cfinform,"Could not open allclasses cache file","");
   return;
   }

for (ip = VHEAP; ip != NULL; ip=ip->next)
   {
   if (IsDefinedClass(ip->name))
      {
      if ((size += strlen(ip->name)) > CF_ALLCLASSSIZE - CF_BUFFERMARGIN)
         {
         Verbose("Class buffer overflowed, dumping class environment for modules\n");
         Verbose("This would probably crash the exec interface on most machines\n");
         }
      else
         {
         size++; /* Allow for : separator */
         strcat(ALLCLASSBUFFER,ip->name);
         strcat(ALLCLASSBUFFER,":");
         }

      fprintf(fp,"%s\n",ip->name);
      }
   }
 
 for (ip = VALLADDCLASSES; ip != NULL; ip=ip->next)
    {
    if (IsDefinedClass(ip->name))
      {
      if ((size += strlen(ip->name)) > 4*CF_BUFSIZE - CF_BUFFERMARGIN)
         {
         Verbose("Class buffer overflowed, dumping class environment for modules\n");
         Verbose("This would probably crash the exec interface on most machines\n");
         }
      else
         {
         size++; /* Allow for : separator */
         strcat(ALLCLASSBUFFER,ip->name);
         strcat(ALLCLASSBUFFER,":");         
         }
      
      fprintf(fp,"%s\n",ip->name);
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

 fclose(fp);
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

printf("\nDebug levels: 1=parsing, 2=running, 3=summary, 4=expression eval\n");

printf("\nBug reports to bug-cfengine@gnu.org (News: gnu.cfengine.bug)\n");
printf("General help to help-cfengine@gnu.org (News: gnu.cfengine.help)\n");
printf("Info & fixes at http://www.iu.hio.no/cfengine\n");
}
/* EOF */
