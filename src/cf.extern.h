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
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */
 

/*******************************************************************/
/*                                                                 */
/*  extern HEADER for cfengine                                     */
/*                                                                 */
/*******************************************************************/

extern RSA *PRIVKEY, *PUBKEY;

/* cfengine */

extern char PADCHAR;
extern struct cfagent_connection *CONN;
extern int AUTHENTICATED;

extern char CFLOCK[bufsize];
extern char CFLOG[bufsize];
extern char CFLAST[bufsize];
extern char LOCKDB[bufsize];


extern char *tzname[2];
extern char *optarg;
extern int optind;
extern struct option OPTIONS[];
extern int CFSIGNATURE;
extern char CFDES1[8];
extern char CFDES2[8];
extern char CFDES3[8];

extern char CFPUBKEYFILE[bufsize];
extern char CFPRIVKEYFILE[bufsize];
extern char AVDB[1024];


extern char *VPRECONFIG;
extern char *VRCFILE;

extern char *VARCH;
extern char *VARCH2;
extern char VYEAR[];
extern char VDAY[];
extern char VMONTH[];
extern char VHR[];
extern char VMINUTE[];
extern char VSEC[];
extern char *ACTIONTEXT[];
extern char *ACTIONID[];
extern char *BUILTINS[];
extern char *CLASSTEXT[];
extern char *CLASSATTRIBUTES[CLSSATTR][ATTRDIM];
extern char *FILEACTIONTEXT[];
extern char *COMMATTRIBUTES[];
extern char VINPUTFILE[];
extern char *VCANONICALFILE;
extern char VCURRENTFILE[];
extern char VLOGFILE[];
extern char *CHDIR;
extern char *VSETUIDLOG;
extern FILE *VLOGFP;
extern char VEDITABORT[];
extern char LISTSEPARATOR;
extern char REPOSCHAR;
extern char DISCOMP;
extern char USESHELL;
extern char PREVIEW;
extern char PURGE;
extern char CHECKSUM;
extern char COMPRESS;
extern int  CHECKSUMUPDATES;
extern int  DISABLESIZE;

extern char VLOGDIR[];
extern char VLOCKDIR[];

extern struct tm TM1;
extern struct tm TM2;

extern int ERRORCOUNT;
extern int NUMBEROFEDITS;
extern time_t CFSTARTTIME;
extern time_t CFINITSTARTTIME;
extern int CF_TIMEOUT;

extern struct utsname VSYSNAME;

extern int LINENUMBER;
extern mode_t DEFAULTMODE;
extern mode_t DEFAULTSYSTEMMODE;
extern int HAVEUID;
extern char *VUIDNAME;
extern char *VGIDNAME;
extern char CFSERVER[];
extern char *PROTOCOL[];
extern char VIPADDRESS[];
extern char VPREFIX[];
extern int VRECURSE;
extern int VAGE;
extern int RPCTIMEOUT;
extern char MOUNTMODE;
extern char DELETEDIR;
extern char DELETEFSTAB;
extern char FORCE;
extern char FORCEIPV4;
extern char FORCELINK;
extern char FORCEDIRS;
extern char STEALTH;
extern char PRESERVETIMES;
extern char TRUSTKEY;
extern char FORK;

extern short COMPATIBILITY_MODE;
extern short LINKSILENT;
extern short UPDATEONLY;
extern char  LINKTYPE;
extern char  AGETYPE;
extern char  COPYTYPE;
extern char  DEFAULTCOPYTYPE;
extern char  LINKDIRS;
extern char  LOGP;
extern char  INFORMP;

extern char *FILTERNAME;
extern char *STRATEGYNAME;
extern char *CURRENTITEM;
extern char *CURRENTPATH;
extern char *GROUPBUFF;
extern char *ACTIONBUFF;
extern char *CLASSBUFF;
extern char ALLCLASSBUFFER[bufsize];
extern char CHROOT[bufsize];
extern char ELSECLASSBUFFER[bufsize];
extern char *LINKFROM;
extern char *LINKTO;
extern char *ERROR;
extern char *MOUNTFROM;
extern char *MOUNTONTO;
extern char *MOUNTOPTS;
extern char *DESTINATION;
extern char *IMAGEACTION;

extern char *EXPR;
extern char *CURRENTAUTHPATH;
extern char *RESTART;
extern char *FILTERDATA;
extern char *STRATEGYDATA;

extern short PROSIGNAL;
extern char  PROACTION;
extern char PROCOMP;
extern char IMGCOMP;

extern int IMGSIZE;


extern char *CHECKSUMDB;
extern char *COMPRESSCOMMAND;

extern char *HASH[hashtablesize];

extern char VBUFF[bufsize];
extern char OUTPUT[bufsize*2];

extern char VFACULTY[maxvarsize];
extern char VDOMAIN[maxvarsize];
extern char VSYSADM[maxvarsize];
extern char VNETMASK[maxvarsize];
extern char VBROADCAST[maxvarsize];
extern char VMAILSERVER[bufsize];
extern struct Item *VTIMEZONE;
extern char VDEFAULTROUTE[maxvarsize];
extern char VNFSTYPE[maxvarsize];
extern char *VREPOSITORY;
extern char *LOCALREPOS;
extern char VIFNAME[16];
extern char VIFNAMEOVERRIDE[16];
extern enum classes VSYSTEMHARDCLASS;
extern char VFQNAME[];
extern char VUQNAME[];
extern char LOGFILE[];

extern struct Item *VEXCLUDECOPY;
extern struct Item *VEXCLUDELINK;
extern struct Item *VCOPYLINKS;
extern struct Item *VLINKCOPIES;
extern struct Item *VEXCLUDEPARSE;
extern struct Item *VCPLNPARSE;
extern struct Item *VINCLUDEPARSE;
extern struct Item *VIGNOREPARSE;
extern struct Item *VACLBUILD;
extern struct Item *VFILTERBUILD;
extern struct Item *VSTRATEGYBUILD;

extern struct Item *VMOUNTLIST;
extern struct Item *VHEAP;      /* Points to the base of the attribute heap */
extern struct Item *VNEGHEAP;

/* HvB : Bas van der Vlies */
extern struct Mountables *VMOUNTABLES;  /* Points to the list of mountables */
extern struct Mountables *VMOUNTABLESTOP;
extern flag  MOUNT_RO;                  /* mount directory readonly */

extern struct Item *VMOUNTED;
extern struct Tidy *VTIDY;               /* Points to the list of tidy specs */
extern struct Disk *VREQUIRED;              /* List of required file systems */
extern struct Disk *VREQUIREDTOP;
extern struct ShellComm *VSCRIPT;              /* List of scripts to execute */
extern struct ShellComm *VSCRIPTTOP;
extern struct Interface *VIFLIST;
extern struct Interface *VIFLISTTOP;
extern struct Mounted *MOUNTED;             /* Files systems already mounted */
extern struct Item VDEFAULTBINSERVER;
extern struct Item *VBINSERVERS;
extern struct Link *VLINK;
extern struct File *VFILE;
extern struct Item *VHOMESERVERS;
extern struct Item *VSETUIDLIST;
extern struct Disable *VDISABLELIST;
extern struct Disable *VDISABLETOP;
extern struct File *VMAKEPATH;
extern struct File *VMAKEPATHTOP;
extern struct Link *VCHLINK;
extern struct Item *VIGNORE;
extern struct Item *VHOMEPATLIST;
extern struct Item *EXTENSIONLIST;
extern struct Item *SUSPICIOUSLIST;
extern struct Item *SCHEDULE;
extern struct Item *SPOOLDIRLIST;
extern struct Item *NONATTACKERLIST;
extern struct Item *MULTICONNLIST;
extern struct Item *TRUSTKEYLIST;
extern struct Item *DHCPLIST;
extern struct Item *ALLOWUSERLIST;
extern struct Item *SKIPVERIFY;
extern struct Item *ATTACKERLIST;
extern struct Item *MOUNTOPTLIST;
extern struct Item *VRESOLVE;
extern struct MiscMount *VMISCMOUNT;
extern struct MiscMount *VMISCMOUNTTOP;
extern struct Item *VIMPORT;
extern struct Item *VACTIONSEQ;
extern struct Item *VACCESSLIST;
extern struct Item *VADDCLASSES;
extern struct Item *VALLADDCLASSES;
extern struct Edit *VEDITLIST;
extern struct Edit *VEDITLISTTOP;
extern struct Filter *VFILTERLIST;
extern struct Filter *VFILTERLISTTOP;
extern struct Strategy *VSTRATEGYLIST;
extern struct Strategy *VSTRATEGYLISTTOP;

extern struct CFACL  *VACLLIST;
extern struct CFACL  *VACLLISTTOP;
extern struct UnMount *VUNMOUNT;
extern struct UnMount *VUNMOUNTTOP;
extern struct Item *VCLASSDEFINE;
extern struct Image *VIMAGE;
extern struct Image *VIMAGETOP;
extern struct Process *VPROCLIST;
extern struct Process *VPROCTOP;
extern struct Item *VSERVERLIST;

extern struct Item *VREPOSLIST;

extern struct Auth *VADMIT;
extern struct Auth *VDENY;
extern struct Auth *VADMITTOP;
extern struct Auth *VDENYTOP;

/* Associated variables which simplify logic */

extern struct Link *VLINKTOP;
extern struct Link *VCHLINKTOP;
extern struct Tidy *VTIDYTOP;
extern struct File *VFILETOP;

extern char *COPYRIGHT;

extern short DEBUG;
extern short D1;
extern short D2;
extern short D3;

extern short PARSING;
extern short ISCFENGINE;

extern short VERBOSE;
extern short EXCLAIM;
extern short INFORM;
extern short CHECK;

extern short LOGGING;
extern short INFORM_save;
extern short LOGGING_save;
extern short CFPARANOID;
extern short SHOWACTIONS;
extern short LOGTIDYHOMEFILES;

extern short TIDYDIRS;
extern short TRAVLINKS;
extern short DEADLINKS;
extern short PTRAVLINKS;
extern short DONTDO;
extern short IFCONF;
extern short PARSEONLY;
extern short GOTMOUNTINFO;
extern short NOMOUNTS;
extern short NOMODULES;
extern short NOPROCS;
extern short NOFILECHECK;
extern short NOTIDY;
extern short NOSCRIPTS;
extern short PRSYSADM;
extern short PRMAILSERVER;
extern short MOUNTCHECK;
extern short NOEDITS;
extern short KILLOLDLINKS;
extern short IGNORELOCK;
extern short NOPRECONFIG;
extern short WARNINGS;
extern short NONALPHAFILES;
extern short MINUSF;
extern short NOLINKS;
extern short ENFORCELINKS;
extern short NOCOPY;
extern short FORCENETCOPY;
extern short SILENT;
extern short EDITVERBOSE;
extern char IMAGEBACKUP;
extern short ROTATE;
extern int   TIDYSIZE;
extern short USEENVIRON;
extern short PROMATCHES;
extern short EDABORTMODE;
extern short NOPROCS;
extern short UNDERSCORE_CLASSES;
extern short NOHARDCLASSES;
extern short NOSPLAY;
extern short DONESPLAY;
extern char TYPECHECK;

extern enum actions ACTION;
extern enum vnames CONTROLVAR;

extern mode_t PLUSMASK;
extern mode_t MINUSMASK;

extern u_long PLUSFLAG;
extern u_long MINUSFLAG;

extern flag  ACTION_IS_LINK;
extern flag  ACTION_IS_LINKCHILDREN;
extern flag  MOUNT_ONTO;
extern flag  MOUNT_FROM;
extern flag  HAVE_RESTART;
extern flag  ACTIONPENDING;
extern flag  HOMECOPY;
extern char ENCRYPT;
extern char VERIFY;
extern char COMPATIBILITY;

extern char *VPSCOMM[];
extern char *VPSOPTS[];
extern char *VMOUNTCOMM[];
extern char *VMOUNTOPTS[];
extern char *VIFDEV[];
extern char *VETCSHELLS[];
extern char *VRESOLVCONF[];
extern char *VHOSTEQUIV[];
extern char *VFSTAB[];
extern char *VMAILDIR[];
extern char *VNETSTAT[];
extern char *VFILECOMM[];
extern char *ACTIONSEQTEXT[];
extern char *VEDITNAMES[];
extern char *VFILTERNAMES[];
extern char *VUNMOUNTCOMM[];
extern char *VRESOURCES[];

extern int VTIMEOUT;
extern mode_t UMASK;

extern char *SIGNALS[];

extern char *tzname[2]; /* see man ctime */

extern int SENSIBLEFILECOUNT;
extern int SENSIBLEFSSIZE;
extern int EDITFILESIZE;
extern int EDITBINFILESIZE;
extern int VIFELAPSED;
extern int VEXPIREAFTER;
extern int VDEFAULTIFELAPSED;
extern int VDEFAULTEXPIREAFTER;
extern int AUTOCREATED;

extern enum fileactions FILEACTION;

extern unsigned short PORTNUMBER;

extern int CURRENTLINENUMBER;
extern struct Item *CURRENTLINEPTR;

extern int EDITGROUPLEVEL;
extern int SEARCHREPLACELEVEL;
extern int FOREACHLEVEL;

extern char COMMENTSTART[], COMMENTEND[];

/* GNU REGEXP */

extern struct re_pattern_buffer *SEARCHPATTBUFF;
extern struct re_pattern_buffer *PATTBUFFER;
