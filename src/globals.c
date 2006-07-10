/* cfengine for GNU
 
        Copyright (C) 1995/6
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
/*  GLOBAL variables for cfengine                                  */
/*                                                                 */
/*  Although these are global in C, they are layed out in          */
/*  terms of their ownership to certain logical "objects",         */
/*  to illustrate the object oriented structure.                   */
/*                                                                 */
/*******************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"

#define PUBLIC     /* Just for fun */
#define PRIVATE
#define PROTECTED

/*******************************************************************/
/*                                                                 */
/* Global : Truly global variables here                            */
/*                                                                 */
/*******************************************************************/

char VBUFF[CF_BUFSIZE]; /* General workspace, contents not guaranteed */
char OUTPUT[CF_BUFSIZE*2];
int AUTHENTICATED = false;
int CHECKSUMUPDATES = false;

int PASS;

char *CHECKSUMDB;
char PADCHAR = ' ';
char CONTEXTID[32];
char CFPUBKEYFILE[CF_BUFSIZE];
char CFPRIVKEYFILE[CF_BUFSIZE];
char AVDB[CF_MAXVARSIZE];
char CFWORKDIR[CF_BUFSIZE];
char PIDFILE[CF_BUFSIZE];

RSA *PRIVKEY = NULL, *PUBKEY = NULL;

#ifdef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
pthread_mutex_t MUTEX_SYSCALL = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
pthread_mutex_t MUTEX_LOCK = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#else
# if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
pthread_mutex_t MUTEX_SYSCALL = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MUTEX_LOCK = PTHREAD_MUTEX_INITIALIZER;
# endif
#endif

/*******************************************************************/
/*                                                                 */
/* cfd.main object : the root application object                   */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

  PROTECTED  struct Auth *VADMIT = NULL;
  PROTECTED  struct Auth *VADMITTOP = NULL;
  PROTECTED  struct Auth *VDENY = NULL;
  PROTECTED  struct Auth *VDENYTOP = NULL;

/*******************************************************************/
/*                                                                 */
/* cfengine.main object : the root application object              */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

  PUBLIC char *COPYRIGHT = "Free Software Foundation 1994-\nDonated by Mark Burgess, Faculty of Engineering,\nOslo University College, Norway";

  PRIVATE char *VPRECONFIG = "cf.preconf";
  PRIVATE char *VRCFILE = "cfrc";

  PUBLIC char *VSETUIDLOG = NULL;
  PUBLIC char *VARCH = NULL;
  PUBLIC char *VARCH2 = NULL;
  PUBLIC char *VREPOSITORY = NULL;
  PUBLIC char *COMPRESSCOMMAND = NULL;

  PUBLIC char VPREFIX[CF_MAXVARSIZE];

  PUBLIC char VINPUTFILE[CF_BUFSIZE];
  PUBLIC char VCURRENTFILE[CF_BUFSIZE];
  PUBLIC char VLOGFILE[CF_BUFSIZE];
  PUBLIC char ALLCLASSBUFFER[CF_ALLCLASSSIZE];
  PUBLIC char ELSECLASSBUFFER[CF_BUFSIZE];
  PUBLIC char FAILOVERBUFFER[CF_BUFSIZE];
  PUBLIC char CHROOT[CF_BUFSIZE];
  PUBLIC char EDITBUFF[CF_BUFSIZE];

  PUBLIC short DEBUG = false;
  PUBLIC short D1 = false;
  PUBLIC short D2 = false;
  PUBLIC short D3 = false;
  PUBLIC short D4 = false;
  PUBLIC short LASTSEEN = true;
  PUBLIC short VERBOSE = false;
  PUBLIC short INFORM = false;
  PUBLIC short CHECK = false;
  PUBLIC short EXCLAIM = true;
  PUBLIC short COMPATIBILITY_MODE = false;
  PUBLIC short LOGGING = false;
  PUBLIC short INFORM_save;
  PUBLIC short LOGGING_save;
  PUBLIC short CFPARANOID = false;
  PUBLIC short SHOWACTIONS = false;
  PUBLIC short LOGTIDYHOMEFILES = true;
  PUBLIC short UPDATEONLY = false;
  PUBLIC short SKIPIDENTIFY = false;
  PUBLIC short ALL_SINGLECOPY = false;

  PUBLIC char FORK = 'n';

  PRIVATE   int RPCTIMEOUT = 60;          /* seconds */
  PROTECTED int SENSIBLEFILECOUNT = 2;
  PROTECTED int SENSIBLEFSSIZE = 1000;

  PUBLIC time_t CFSTARTTIME;
  PUBLIC time_t CFINITSTARTTIME;

  PUBLIC dev_t ROOTDEVICE = 0;

  PUBLIC char  STR_CFENGINEPORT[16];
  PUBLIC unsigned short SHORT_CFENGINEPORT;

  PUBLIC enum classes VSYSTEMHARDCLASS;

  PUBLIC struct Item VDEFAULTBINSERVER =      /* see GetNameInfo(), main.c */
      {
      'n',
      NULL,
      NULL,
      0,
      0,
      0,
      NULL
      };

  PUBLIC struct utsname VSYSNAME;                           /* For uname (2) */

  PUBLIC mode_t DEFAULTMODE = (mode_t) 0755;
  PUBLIC mode_t DEFAULTSYSTEMMODE = (mode_t) 0644;

  PROTECTED int VIFELAPSED = 1;
  PROTECTED int VEXPIREAFTER = 120;
  PROTECTED int VDEFAULTIFELAPSED = 1;     
  PROTECTED int VDEFAULTEXPIREAFTER = 120; /* minutes */

  PUBLIC struct cfagent_connection *CONN = NULL;

  PUBLIC struct Item *VEXCLUDECACHE = NULL;
  PUBLIC struct Item *QUERYVARS = NULL;

  PUBLIC struct cfObject *OBJECTLIST = NULL;

  PUBLIC struct Item *IPADDRESSES = NULL;

 /*******************************************************************/
 /* Anomaly                                                         */
 /*******************************************************************/

struct sock ECGSOCKS[ATTR] = /* extended to map old to new using enum*/
   {
   {"137","netbiosns",ob_netbiosns_in,ob_netbiosns_out},
   {"138","netbiosdgm",ob_netbiosdgm_in,ob_netbiosdgm_out},
   {"139","netbiosssn",ob_netbiosssn_in,ob_netbiosssn_out},
   {"194","irc",ob_irc_in,ob_irc_out},
   {"5308","cfengine",ob_cfengine_in,ob_cfengine_out},
   {"2049","nfsd",ob_nfsd_in,ob_nfsd_out},
   {"25","smtp",ob_smtp_in,ob_smtp_out},
   {"80","www",ob_www_in,ob_www_out},
   {"21","ftp",ob_ftp_in,ob_ftp_out},
   {"22","ssh",ob_ssh_in,ob_ssh_out},
   {"443","wwws",ob_wwws_in,ob_wwws_out}
   };

char *TCPNAMES[CF_NETATTR] =
   {
   "icmp",
   "udp",
   "dns",
   "tcpsyn",
   "tcpack",
   "tcpfin",
   "misc"
   };

char *OBS[CF_OBSERVABLES] =
    {
    "users",
    "rootprocs",
    "otherprocs",
    "diskfree",
    "loadavg",
    "netbiosns_in",
    "netbiosns_out",
    "netbiosdgm_in",
    "netbiosdgm_out",
    "netbiosssn_in",
    "netbiosssn_out",
    "irc_in",
    "irc_out",
    "cfengine_in",
    "cfengine_out",
    "nfsd_in",
    "nfsd_out",
    "smtp_in",
    "smtp_out",
    "www_in",
    "www_out",
    "ftp_in",
    "ftp_out",
    "ssh_in",
    "ssh_out",
    "wwws_in",
    "wwws_out",
    "icmp_in",
    "icmp_out",
    "udp_in",
    "udp_out",
    "dns_in",
    "dns_out",
    "tcpsyn_in",
    "tcpsyn_out",
    "tcpack_in",
    "tcpack_out",
    "tcpfin_in",
    "tcpfin_out",
    "tcpmisc_in",
    "tcpmisc_out",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    "spare",
    };

 /*******************************************************************/
 /* Methods                                                         */
 /*******************************************************************/

  PUBLIC int GOTMETHODARGS = false;
  PUBLIC struct Item *METHODARGS = NULL;
  PUBLIC char ** METHODARGV = NULL;
  PUBLIC int METHODARGC = 0;
  PRIVATE char *VMETHODPROTO[] =
     {
     "NAME:",
     "TRUSTEDFILE:",
     "TIMESTAMP:",
     "REPLYTO:",
     "SENDCLASS:",
     "ATTACH-ARG:",
     "ISREPLY:",
     NULL
     };

  PUBLIC char METHODNAME[CF_BUFSIZE];
  PUBLIC char METHODFILENAME[CF_BUFSIZE];
  PUBLIC char METHODREPLYTO[CF_BUFSIZE]; 
  PUBLIC char METHODFOR[CF_BUFSIZE];
  PUBLIC char METHODFORCE[CF_BUFSIZE];
  PUBLIC struct Item *METHODRETURNVARS = NULL;
  PUBLIC struct Item *METHODRETURNCLASSES = NULL;
  PUBLIC char METHODMD5[CF_BUFSIZE];

 /*******************************************************************/
 /* Data/list structures - root pointers                            */
 /*******************************************************************/

  PUBLIC struct Item *ABORTHEAP = NULL;
  
  PROTECTED  struct Item *VTIMEZONE = NULL;
  PROTECTED  struct Item *VMOUNTLIST = NULL;
  PROTECTED  struct Item *VEXCLUDECOPY = NULL;
  PROTECTED  struct Item *VAUTODEFINE = NULL;
  PROTECTED  struct Item *VEXCLUDELINK = NULL;
  PROTECTED  struct Item *VCOPYLINKS = NULL;
  PROTECTED  struct Item *VLINKCOPIES = NULL;
  PROTECTED  struct Item *VEXCLUDEPARSE = NULL;
  PROTECTED  struct Item *VCPLNPARSE = NULL;
  PROTECTED  struct Item *VINCLUDEPARSE = NULL;
  PROTECTED  struct Item *VIGNOREPARSE = NULL;
  PROTECTED  struct Item *VSERVERLIST = NULL;
  PROTECTED  struct Item *VRPCPEERLIST = NULL;
  PROTECTED  struct Item *VREDEFINES = NULL;

  PROTECTED  struct Item *VHEAP = NULL;      /* Points to the base of the attribute heap */
  PROTECTED  struct Item *VNEGHEAP = NULL;


  PROTECTED  struct Mountables *VMOUNTABLES = NULL;         /* Points to the list of mountables */
  PROTECTED  struct Mountables *VMOUNTABLESTOP = NULL;

  PUBLIC struct cfObject *VOBJTOP = NULL;
  PUBLIC struct cfObject *VOBJ = NULL;

  PROTECTED  struct Item *VALERTS = NULL;
  PROTECTED  struct Item *VMOUNTED = NULL;
  PROTECTED  struct Tidy *VTIDY = NULL;               /* Points to the list of tidy specs */
  PROTECTED  struct Tidy *VTIDYTOP = NULL;
  PROTECTED  struct Item *VPROCESSES = NULL;                       /* Points to proc list */
  PROTECTED  struct Disk *VREQUIRED = NULL;              /* List of required file systems */
  PROTECTED  struct Disk *VREQUIREDTOP = NULL;
  PROTECTED  struct ShellComm *VSCRIPT = NULL;              /* List of scripts to execute */
  PROTECTED  struct ShellComm *VSCRIPTTOP = NULL;
  PROTECTED  struct Interface *VIFLIST = NULL;
  PROTECTED  struct Interface *VIFLISTTOP = NULL;
  PROTECTED  struct Mounted *MOUNTED = NULL;             /* Files systems already mounted */
  PROTECTED  struct MiscMount *VMISCMOUNT = NULL;
  PROTECTED  struct MiscMount *VMISCMOUNTTOP = NULL;
  PROTECTED  struct Item *VBINSERVERS = NULL;
  PROTECTED  struct Link *VLINK = NULL;
  PROTECTED  struct Link *VLINKTOP = NULL;
  PROTECTED  struct File *VFILE = NULL;
  PROTECTED  struct File *VFILETOP = NULL;
  PROTECTED  struct Image *VIMAGE = NULL;
  PROTECTED  struct Image *VIMAGETOP=NULL;
  PROTECTED  struct Method *VMETHODS = NULL;
  PROTECTED  struct Method *VMETHODSTOP=NULL;
  PROTECTED  struct Item *VHOMESERVERS = NULL;
  PROTECTED  struct Item *VSETUIDLIST = NULL;
  PROTECTED  struct Disable *VDISABLELIST = NULL;
  PROTECTED  struct Disable *VDISABLETOP = NULL;
  PROTECTED  struct File *VMAKEPATH = NULL;
  PROTECTED  struct File *VMAKEPATHTOP = NULL;
  PROTECTED  struct Link *VCHLINK = NULL;
  PROTECTED  struct Link *VCHLINKTOP = NULL;
  PROTECTED  struct Item *VIGNORE = NULL;
  PROTECTED  struct Item *VHOMEPATLIST = NULL;
  PROTECTED  struct Item *EXTENSIONLIST = NULL;
  PROTECTED  struct Item *SUSPICIOUSLIST = NULL;
  PROTECTED  struct Item *SCHEDULE = NULL;
  PROTECTED  struct Item *SPOOLDIRLIST = NULL;
  PROTECTED  struct Item *NONATTACKERLIST = NULL;
  PROTECTED  struct Item *MULTICONNLIST = NULL;
  PROTECTED  struct Item *TRUSTKEYLIST = NULL;
  PROTECTED  struct Item *DHCPLIST = NULL;
  PROTECTED  struct Item *ALLOWUSERLIST = NULL;
  PROTECTED  struct Item *SKIPVERIFY = NULL;
  PROTECTED  struct Item *ATTACKERLIST = NULL;
  PROTECTED  struct Item *MOUNTOPTLIST = NULL;
  PROTECTED  struct Item *VRESOLVE = NULL;
  PROTECTED  struct Item *VIMPORT = NULL;
  PROTECTED  struct Item *VACTIONSEQ=NULL;
  PROTECTED  struct Item *VACCESSLIST=NULL;
  PROTECTED  struct Item *VADDCLASSES=NULL;           /* Action sequence defs  */
  PROTECTED  struct Item *VALLADDCLASSES=NULL;        /* All classes */
  PROTECTED  struct Item *VJUSTACTIONS=NULL;
  PROTECTED  struct Item *VAVOIDACTIONS=NULL;
  PROTECTED  struct Item *VDEFAULTROUTE=NULL;

  PROTECTED  struct UnMount *VUNMOUNT=NULL;
  PROTECTED  struct UnMount *VUNMOUNTTOP=NULL;
  PROTECTED  struct Edit *VEDITLIST=NULL;
  PROTECTED  struct Edit *VEDITLISTTOP=NULL;
  PROTECTED  struct Filter *VFILTERLIST=NULL;
  PROTECTED  struct Filter *VFILTERLISTTOP=NULL;
  PROTECTED  struct CFACL  *VACLLIST=NULL;
  PROTECTED  struct CFACL  *VACLLISTTOP=NULL;
  PROTECTED  struct Strategy *VSTRATEGYLIST=NULL;
  PROTECTED  struct Strategy *VSTRATEGYLISTTOP=NULL;

  PROTECTED  struct Item *VCLASSDEFINE=NULL;
  PROTECTED  struct Process *VPROCLIST=NULL;
  PROTECTED  struct Process *VPROCTOP=NULL;
  PROTECTED  struct Item *VREPOSLIST=NULL;

  PROTECTED  struct Package *VPKG=NULL;    /* Head of the packages item list */
  PROTECTED  struct Package *VPKGTOP=NULL; /* The last packages item we added */


 /*********************************************************************/
 /* Resource names                                                    */
 /*********************************************************************/

  PRIVATE char *VRESOURCES[] = /* one for each major variable in class.c */
     {
     "mountcomm",
     "unmountcomm",
     "ethernet",
     "mountopts",
     "unused",
     "fstab",
     "maildir",
     "netstat",
     "pscomm",
     "psopts",
     NULL
     };


 /*******************************************************************/
 /* Reserved variables                                              */
 /*******************************************************************/

 PROTECTED char   VMAILSERVER[CF_BUFSIZE];

 PROTECTED char      VFACULTY[CF_MAXVARSIZE];
 PROTECTED char       VDOMAIN[CF_MAXVARSIZE];
 PROTECTED char       VSYSADM[CF_MAXVARSIZE];
 PROTECTED char      VNETMASK[CF_MAXVARSIZE];
 PROTECTED char    VBROADCAST[CF_MAXVARSIZE];
 PROTECTED char      VNFSTYPE[CF_MAXVARSIZE];
 PROTECTED char       VFQNAME[CF_MAXVARSIZE];
 PROTECTED char       VUQNAME[CF_MAXVARSIZE];
 PROTECTED char       LOGFILE[CF_MAXVARSIZE];

 PROTECTED char         VYEAR[5];
 PROTECTED char         VDAY[3];
 PROTECTED char         VMONTH[4];
 PROTECTED char         VHR[3];
 PROTECTED char         VMINUTE[3];
 PROTECTED char         VSEC[3];


 /*******************************************************************/
 /* Command line options                                            */
 /*******************************************************************/

  /* GNU STUFF FOR LATER #include "getopt.h" */
 
 
 PRIVATE struct option OPTIONS[49] =
      {
      { "help",no_argument,0,'h' },
      { "debug",optional_argument,0,'d' },
      { "method",required_argument,0,'Z' }, 
      { "verbose",no_argument,0,'v' },
      { "traverse-links",no_argument,0,'l' },
      { "recon",no_argument,0,'n' },
      { "dry-run",no_argument,0,'n'},
      { "just-print",no_argument,0,'n'},
      { "no-ifconfig",no_argument,0,'i' },
      { "file",required_argument,0,'f' },
      { "parse-only",no_argument,0,'p' },
      { "no-mount",no_argument,0,'m' },
      { "no-check-files",no_argument,0,'c' },
      { "no-check-mounts",no_argument,0,'C' },
      { "no-tidy",no_argument,0,'t' },
      { "no-commands",no_argument,0,'s' },
      { "sysadm",no_argument,0,'a' },
      { "version",no_argument,0,'V' },
      { "define",required_argument,0,'D' },
      { "negate",required_argument,0,'N' },
      { "undefine",required_argument,0,'N' },
      { "delete-stale-links",no_argument,0,'L' },
      { "no-warn",no_argument,0,'w' },
      { "silent",no_argument,0,'S' },
      { "quiet",no_argument,0,'w' },
      { "no-preconf",no_argument,0,'x' },
      { "no-links",no_argument,0,'X'},
      { "no-edits",no_argument,0,'e'},
      { "enforce-links",no_argument,0,'E'},
      { "no-copy",no_argument,0,'k'},
      { "use-env",no_argument,0,'u'},
      { "no-processes",no_argument,0,'P'},
      { "underscore-classes",no_argument,0,'U'},
      { "no-hard-classes",no_argument,0,'H'},
      { "no-splay",no_argument,0,'q'},
      { "no-lock",no_argument,0,'K'},
      { "auto",no_argument,0,'A'},
      { "inform",no_argument,0,'I'},
      { "no-modules",no_argument,0,'M'},
      { "force-net-copy",no_argument,0,'b'},
      { "secure-input",no_argument,0,'Y'},
      { "zone-info",no_argument,0,'z'},
      { "update-only",no_argument,0,'B'},
      { "check-contradictions",no_argument,0,'g'},
      { "just",required_argument,0,'j'},
      { "avoid",required_argument,0,'o'},
      { "query",required_argument,0,'Q'},
      { "csdb",no_argument,0,'W'},
      { NULL,0,0,0 }
      };


 /*********************************************************************/
 /* Actions                                                           */
 /*********************************************************************/


 PRIVATE char *ACTIONTEXT[] =
      {
      "",
      "Control Defintions:",
      "Alerts:",
      "Groups:",
      "File Imaging:",
      "Resolve:",
      "Processes:",
      "Files:",
      "Tidy:",
      "Home Servers:",
      "Binary Servers:",
      "Mail Server:",
      "Required Filesystems",
      "Disks (Required)",
      "Reading Mountables",
      "Links:",
      "Import files:",
      "User Shell Commands:",
      "Rename or Disable Files:",
      "Rename files:",
      "Make Directory Path:",
      "Ignore File Paths:",
      "Broadcast Mode:",
      "Default Packet Route:",
      "Miscellaneous Mountables:",
      "Edit Simple Text File:",
      "Unmount filesystems:",
      "Admit network access:",
      "Deny network access:",
      "Access control lists:",
      "Additional network interfaces:",
      "Search filter objects:",
      "Strategies:",
      "Package Checks:",
      "Method Function Calls",
      NULL
      };


 PRIVATE char *ACTIONID[] =    /* The actions which may be specified as indexed */
      {                        /* macros in the "special" section of the file   */
      "",
      "control",
      "alerts",
      "groups",
      "copy",
      "resolve",
      "processes",
      "files",
      "tidy",
      "homeservers",
      "binservers",
      "mailserver",
      "required",
      "disks",
      "mountables",
      "links",
      "import",
      "shellcommands",
      "disable",
      "rename",
      "directories",
      "ignore",
      "broadcast",
      "defaultroute",
      "miscmounts",
      "editfiles",
      "unmount",
      "admit",
      "deny",
      "acl",
      "interfaces",
      "filters",
      "strategies",
      "packages",
      "methods",
      NULL
      };

 PRIVATE char *BUILTINS[] =    /* The actions which may be specified as indexed */
      {
      "",
      "randomint",
      "isnewerthan",
      "accessedbefore",
      "changedbefore",
      "fileexists",
      "isdir",
      "islink",
      "isplain",
      "execresult",
      "execshellresult",
      "returnszero",
      "returnszeroshell",
      "iprange",
      "hostrange",
      "isdefined",
      "strcmp",
      "regcmp",
      "showstate",
      "friendstatus",
      "readfile",
      "returnvariables",
      "returnclasses",
      "syslog",
      "setstate",
      "unsetstate",
      "prepmodule",
      "a",
      "readarray",
      "readtable",
      "readlist",
      "selectpartitionleader",
      "selectpartitionneighbours",
      "selectpartitionneighbors",
      "isgreaterthan",
      "islessthan",
      "readtcp",
      NULL
      };

  /*********************************************************************/
  /* file/image actions                                                */
  /*********************************************************************/

  PROTECTED char *FILEACTIONTEXT[] = 
      {
      "warnall",
      "warnplain",
      "warndirs",
      "fixall",
      "fixplain",
      "fixdirs",
      "touch",
      "linkchildren",
      "create",
      "compress",
      "alert",
      NULL
      };

  /*********************************************************************/

  PRIVATE char *ACTIONSEQTEXT[] =
      {
      "directories",
      "links",
      "mailcheck",
      "required",
      "disks",
      "tidy",
      "shellcommands",
      "files",
      "disable",
      "rename",
      "addmounts",
      "editfiles",
      "mountall",
      "unmount",
      "resolve",
      "copy",
      "netconfig",
      "checktimezone",
      "mountinfo",
      "processes",
      "packages",
      "methods",
      "none",
      NULL
      };

  /*********************************************************************/
  /* Package check actions                                             */
  /*********************************************************************/

  /* The sense of the comparison (greater than, less than...) */

  PRIVATE char *CMPSENSETEXT[] =
      {
      "eq",
      "gt",
      "lt",
      "ge",
      "le",
      "ne",
      "none",
      NULL
      };

  /*********************************************************************/
  /* The names of the available package managers */

  PRIVATE char *PKGMGRTEXT[] =
      {
      "rpm",
      "dpkg",   /* aptget ? */
      "sun",    /* pkginfo/pkgadd/pkgrm */
      NULL
      };

  /*********************************************************************/
  /* The names of the possible package-related actions */
  PRIVATE char *PKGACTIONTEXT[] =
      {
      "install",
      "remove",
      NULL
      };

/*******************************************************************/
/*                                                                 */
/* parse object : variables belonging to the Parse object          */
/*                                                                 */
/*******************************************************************/

  PUBLIC short ISCFENGINE;  /* for re-using parser code in cfd */

  PUBLIC  short SHOWDB = false;
  PUBLIC  short PARSING = false;
  PRIVATE short TRAVLINKS = false;
  PRIVATE short PTRAVLINKS = false;
  PRIVATE short DEADLINKS = true;
  PRIVATE short DONTDO = false;
  PRIVATE short IFCONF = true;
  PRIVATE short PARSEONLY = false;
  PRIVATE short GOTMOUNTINFO = true;
  PRIVATE short NOMOUNTS = false;
  PRIVATE short NOMODULES = false;
  PRIVATE short NOFILECHECK = false;
  PRIVATE short NOTIDY = false;
  PRIVATE short NOSCRIPTS = false;
  PRIVATE short PRSYSADM = false;
  PRIVATE short PRSCHEDULE = false;
  PRIVATE short MOUNTCHECK = false;
  PRIVATE short NOPROCS = false;
  PRIVATE short NOMETHODS = false;
  PRIVATE short NOEDITS = false;
  PRIVATE short KILLOLDLINKS = false;
  PRIVATE short IGNORELOCK = false;
  PRIVATE short NOPRECONFIG = false;
  PRIVATE short WARNINGS = true;
  PRIVATE short NONALPHAFILES = false;
  PRIVATE short MINUSF = false;
  PRIVATE short NOLINKS = false;
  PRIVATE short ENFORCELINKS = false;
  PRIVATE short NOCOPY = false;
  PRIVATE short FORCENETCOPY = false;
  PRIVATE short SILENT=false;
  PRIVATE short EDITVERBOSE=false;
  PRIVATE short LINKSILENT;

  PRIVATE short ROTATE=0;
  PRIVATE short USEENVIRON=false;
  PRIVATE short PROMATCHES=-1;
  PRIVATE short EDABORTMODE=false;
  PRIVATE short UNDERSCORE_CLASSES=false;
  PRIVATE short NOHARDCLASSES=false;
  PRIVATE short NOSPLAY = false;
  PRIVATE short DONESPLAY = false;

  PUBLIC  struct Item *VSINGLECOPY = NULL;

  PROTECTED  struct Item *VACLBUILD = NULL;
  PROTECTED  struct Item *VFILTERBUILD = NULL;
  PROTECTED  struct Item *VSTRATEGYBUILD = NULL;

  PRIVATE char NOABSPATH = 'n';
  PRIVATE char TIDYDIRS = 'n';
  PRIVATE char XDEV = false;
  PRIVATE char IMAGEBACKUP='y';
  PRIVATE char TRUSTKEY = 'n';
  PRIVATE char PRESERVETIMES = 'n';
  PRIVATE char TYPECHECK = 'y';
  PRIVATE char SCAN = 'n';
  PRIVATE char LINKTYPE = 's';
  PRIVATE char AGETYPE = 'a';
  PRIVATE char COPYTYPE = 't';
  PRIVATE char DEFAULTCOPYTYPE = 't';
  PRIVATE char REPOSCHAR = '_';
  PRIVATE char LISTSEPARATOR = ':';
  PRIVATE char LINKDIRS = 'k';
  PRIVATE char DISCOMP = '=';
  PRIVATE char USESHELL = 'y';  /* yes or no or dumb */
  PRIVATE char PREVIEW = 'n';  /* yes or no */
  PRIVATE char PURGE = 'n';
  PRIVATE char LOGP = 'd';  /* y,n,d=default*/
  PRIVATE char INFORMP = 'd';
  PRIVATE char MOUNTMODE = 'w';   /* o or w for rw/ro*/
  PRIVATE char DELETEDIR = 'y';   /* t=true */
  PRIVATE char DELETEFSTAB = 'y';
  PRIVATE char FORCE = 'n';
  PRIVATE char FORCEIPV4 = 'n';
  PRIVATE char FORCELINK = 'n';
  PRIVATE char FORCEDIRS = 'n';
  PRIVATE char STEALTH = 'n';
  PRIVATE char CHECKSUM = 'n'; /* n,m,s */
  PRIVATE char COMPRESS = 'n';
  

  PRIVATE char *FINDERTYPE;
  PRIVATE char *PARSEMETHODRETURNCLASSES;
  PRIVATE char *VUIDNAME;
  PRIVATE char *VGIDNAME;
  PRIVATE char *FILTERNAME;
  PRIVATE char *STRATEGYNAME;
  PRIVATE char *GROUPBUFF;
  PRIVATE char *ACTIONBUFF;
  PRIVATE char *CURRENTOBJECT;
  PRIVATE char *CURRENTITEM;
  PRIVATE char *CLASSBUFF;
  PRIVATE char *LINKFROM;
  PRIVATE char *LINKTO;
  PRIVATE char *ERROR;
  PRIVATE char *MOUNTFROM;
  PRIVATE char *MOUNTONTO;
  PRIVATE char *MOUNTOPTS;
  PRIVATE char *DESTINATION;
  PRIVATE char *IMAGEACTION;
  PRIVATE char VIFNAME[16];
  PRIVATE char VIFNAMEOVERRIDE[16];
  PRIVATE char *CHDIR;
  PRIVATE char *LOCALREPOS;
  PRIVATE char *EXPR;
  PRIVATE char *CURRENTAUTHPATH;
  PRIVATE char *RESTART;
  PRIVATE char *FILTERDATA;
  PRIVATE char *STRATEGYDATA;
  PRIVATE char *PKGVER;     /* value of ver option in packages: */

  PRIVATE short PROSIGNAL;
  PRIVATE char  PROACTION;

  PRIVATE char PROCOMP;
  PRIVATE char IMGCOMP;
  PRIVATE int IMGSIZE;

  PUBLIC int ERRORCOUNT = 0;
  PUBLIC int LINENUMBER = 1;

  PRIVATE int HAVEUID = 0;
  PRIVATE int DISABLESIZE=99999999;
  PRIVATE int TIDYSIZE=0;
  PRIVATE int VRECURSE;
  PRIVATE int VAGE;
  PRIVATE int VTIMEOUT=0;
  PRIVATE int PIFELAPSED=0;
  PRIVATE int PEXPIREAFTER=0;

  PRIVATE mode_t UMASK=0;
  PRIVATE mode_t PLUSMASK;
  PRIVATE mode_t MINUSMASK;

  PRIVATE u_long PLUSFLAG;
  PRIVATE u_long MINUSFLAG;

 /* Parsing flags etc */

  PUBLIC enum actions ACTION = none;
  PRIVATE enum vnames CONTROLVAR = nonexistentvar;
  PRIVATE enum fileactions FILEACTION = warnall;
  PRIVATE enum cmpsense CMPSENSE = cmpsense_eq; /* Comparison for packages: */
  PRIVATE enum pkgmgrs PKGMGR = pkgmgr_none;  /* Which package mgr to query */
  PRIVATE enum pkgmgrs DEFAULTPKGMGR = pkgmgr_none;
  PRIVATE enum pkgactions PKGACTION = pkgaction_none;

  PRIVATE flag ACTION_IS_LINK = false;
  PRIVATE flag ACTION_IS_LINKCHILDREN = false;
  PRIVATE flag MOUNT_ONTO = false;
  PRIVATE flag MOUNT_FROM = false;
  PRIVATE flag HAVE_RESTART = false;
  PRIVATE flag ACTIONPENDING = false;
  PRIVATE flag HOMECOPY = false;
  PRIVATE char ENCRYPT = 'n';
  PRIVATE char VERIFY = 'n';
  PRIVATE char COMPATIBILITY = 'n';

  /*
   * HvB: Bas van der Vlies
  */
  PRIVATE flag CF_MOUNT_RO = false;

  PRIVATE char *COMMATTRIBUTES[] =
     {
     "findertype",
     "recurse",
     "mode",
     "owner",
     "group",
     "age",
     "action",
     "pattern",
     "links",
     "type",
     "destination",
     "force",
     "forcedirs",
     "forceipv4",
     "forcereplyto",
     "backup",
     "rotate",
     "size",
     "matches",
     "signal",
     "exclude",
     "copy",
     "symlink",
     "copytype",
     "linktype",
     "include",
     "dirlinks",
     "rmdirs",
     "server",
     "define",
     "elsedefine",
     "failover",
     "timeout",
     "freespace",
     "nofile",
     "acl",
     "purge",
     "useshell",
     "syslog",
     "inform",
     "ipv4",
     "netmask",
     "broadcast",
     "ignore",
     "deletedir",
     "deletefstab",
     "stealth",
     "checksum",
     "flags",
     "encrypt",
     "verify",
     "root",
     "typecheck",
     "umask",
     "compress",
     "filter",
     "background",
     "chdir",
     "chroot",
     "preview",
     "repository",
     "timestamps",
     "trustkey",
     "oldserver",
     "mountoptions",      /* HvB : Bas van der Vlies */
     "readonly",          /* HvB : Bas van der Vlies */
     "version",
     "cmp",
     "pkgmgr",
     "xdev",
     "returnvars",
     "returnclasses",
     "sendclasses",
     "ifelapsed",
     "expireafter",
     "scanarrivals",
     "noabspath",
     NULL
     };



  PUBLIC char *VFILTERNAMES[] =
     {
     "Result", /* quoted string of combinatorics, classes of each result */
     "Owner",
     "Group",
     "Mode",
     "Type",
     "FromCtime",
     "ToCtime",
     "FromMtime",
     "ToMtime",
     "FromAtime",
     "ToAtime",
     "FromSize",
     "ToSize",
     "ExecRegex",
     "NameRegex",
     "DefineClasses",
     "ElseDefineClasses",
     "ExecProgram",
     "IsSymLinkTo",
     "PID",
     "PPID",
     "PGID",
     "RSize",
     "VSize",
     "Status",
     "Command",
     "FromTTime",
     "ToTTime",
     "FromSTime",
     "ToSTime",
     "TTY",
     "Priority",
     "Threads",
     "NoFilter",
     NULL
     };

/*******************************************************************/
/*                                                                 */
/* editfiles object : variables belonging to Editfiles             */
/*                    editfiles uses Item                          */
/*                                                                 */
/*******************************************************************/

  PRIVATE char VEDITABORT[CF_BUFSIZE];

  PUBLIC int EDITFILESIZE = 10000;
  PUBLIC int EDITBINFILESIZE = 10000000;

  PRIVATE int NUMBEROFEDITS = 0;
  PRIVATE int CURRENTLINENUMBER = 1;           /* current line number in file */
  PRIVATE struct Item *CURRENTLINEPTR = NULL;  /* Ptr to current line */

  PRIVATE struct re_pattern_buffer *SEARCHPATTBUFF;
  PRIVATE struct re_pattern_buffer *PATTBUFFER;

  PRIVATE int EDITGROUPLEVEL=0;
  PRIVATE int SEARCHREPLACELEVEL=0;
  PRIVATE int FOREACHLEVEL = 0;

  PRIVATE int AUTOCREATED = 0;

  PRIVATE char COMMENTSTART[CF_MAXVARSIZE];
  PRIVATE char COMMENTEND[CF_MAXVARSIZE];

  PUBLIC char *VEDITNAMES[] =
     {
     "NoEdit",
     "DeleteLinesStarting",
     "DeleteLinesNotStarting",
     "DeleteLinesContaining",
     "DeleteLinesNotContaining",
     "DeleteLinesMatching",
     "DeleteLinesNotMatching",
     "DeleteLinesStartingFileItems",
     "DeleteLinesContainingFileItems",
     "DeleteLinesMatchingFileItems",
     "DeleteLinesNotStartingFileItems",
     "DeleteLinesNotContainingFileItems",
     "DeleteLinesNotMatchingFileItems",
     "AppendIfNoSuchLine",
     "PrependIfNoSuchLine",
     "WarnIfNoSuchLine",
     "WarnIfLineMatching",
     "WarnIfNoLineMatching",
     "WarnIfLineStarting",
     "WarnIfLineContaining",
     "WarnIfNoLineStarting",
     "WarnIfNoLineContaining",
     "HashCommentLinesContaining",
     "HashCommentLinesStarting",
     "HashCommentLinesMatching",
     "SlashCommentLinesContaining",
     "SlashCommentLinesStarting",
     "SlashCommentLinesMatching",
     "PercentCommentLinesContaining",
     "PercentCommentLinesStarting",
     "PercentCommentLinesMatching",
     "ResetSearch",
     "SetSearchRegExp",
     "LocateLineMatching",
     "InsertLine",
     "AppendIfNoSuchLinesFromFile",
     "IncrementPointer",
     "ReplaceLineWith",
     "ExpandVariables",
     "DeleteToLineMatching",
     "HashCommentToLineMatching",
     "PercentCommentToLineMatching",
     "SetScript",
     "RunScript",
     "RunScriptIfNoLineMatching",
     "RunScriptIfLineMatching",
     "AppendIfNoLineMatching",
     "PrependIfNoLineMatching",
     "DeleteNLines",
     "EmptyEntireFilePlease",
     "GotoLastLine",
     "BreakIfLineMatches",
     "BeginGroupIfNoMatch",
     "BeginGroupIfMatch",
     "BeginGroupIfNoLineMatching",
     "BeginGroupIfNoSuchLine",
     "BeginGroupIfLineMatching",
     "EndGroup",
     "Append",
     "Prepend",
     "SetCommentStart",
     "SetCommentEnd",
     "CommentLinesMatching",
     "CommentLinesStarting",
     "CommentToLineMatching",
     "CommentNLines",
     "UnCommentNLines",
     "ReplaceAll",
     "ReplaceFirst",
     "With",
     "SetLine",
     "FixEndOfLine",
     "AbortAtLineMatching",
     "UnsetAbort",
     "AutomountDirectResources",
     "UnCommentLinesContaining",
     "UnCommentLinesMatching",
     "InsertFile",
     "CommentLinesContaining",
     "BeginGroupIfFileIsNewer",
     "BeginGroupIfFileExists",
     "BeginGroupIfNoLineContaining",
     "BeginGroupIfLineContaining",
     "BeginGroupIfDefined",
     "BeginGroupIfNotDefined",
     "AutoCreate",
     "WarnIfFileMissing",
     "ForEachLineIn",
     "EndLoop",
     "ReplaceLinesMatchingField",
     "SplitOn",
     "AppendToLineIfNotContains",
     "DeleteLinesAfterThisMatching",
     "DefineClasses",
     "ElseDefineClasses",
     "CatchAbort",
     "Backup",
     "Syslog",
     "Inform",
     "Recurse",
     "EditMode",
     "WarnIfContainsString",
     "WarnIfContainsFile",
     "Ignore",
     "Exclude",
     "Include",
     "Repository",
     "Umask",
     "UseShell",
     "Filter",
     "DefineInGroup",
     "IfElapsed",
     "ExpireAfter",
     NULL
     };


/*******************************************************************/
/*                                                                 */
/* Processes object : Process                                      */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

 PUBLIC char *SIGNALS[highest_signal];  /* This is initialized to zero */

/*******************************************************************/
/*                                                                 */
/* Network client-server object : Net                              */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

  PRIVATE char CFSERVER[CF_MAXVARSIZE];
  PRIVATE char BINDINTERFACE[CF_BUFSIZE];
  PRIVATE unsigned short PORTNUMBER = 0;
  PRIVATE char VIPADDRESS[18];
  PRIVATE int  CF_TIMEOUT = 10;

  PRIVATE int  CFSIGNATURE = 0;
  PRIVATE char CFDES1[9];
  PRIVATE char CFDES2[9];
  PRIVATE char CFDES3[9];

  PUBLIC char *PROTOCOL[] =
   {
   "EXEC",
   "AUTH",  /* old protocol */
   "GET",
   "OPENDIR",
   "SYNCH",
   "CLASSES",
   "MD5",
   "SMD5",
   "CAUTH",
   "SAUTH",
   "SSYNCH",
   "SGET",
   NULL
   };

/*******************************************************************/
/*                                                                 */
/* Adaptive lock object : Lock                                     */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

  PUBLIC char VLOCKDIR[CF_BUFSIZE];
  PUBLIC char VLOGDIR[CF_BUFSIZE];

  PUBLIC char *VCANONICALFILE = NULL;

  PUBLIC FILE *VLOGFP = NULL;

  PUBLIC char CFLOCK[CF_BUFSIZE];
  PUBLIC char SAVELOCK[CF_BUFSIZE]; 
  PUBLIC char CFLOG[CF_BUFSIZE];
  PUBLIC char CFLAST[CF_BUFSIZE]; 
  PUBLIC char LOCKDB[CF_BUFSIZE];

/* EOF */

