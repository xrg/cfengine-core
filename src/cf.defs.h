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
/*  HEADER for cfengine                                            */
/*                                                                 */
/*******************************************************************/

#include "conf.h"
#include <stdio.h>

/*#if defined(STDC_HEADERS) || defined(SUN4)*/
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#else
#define _SYS_NMLN       257

struct utsname
  {
   char    sysname[_SYS_NMLN];
   char    nodename[_SYS_NMLN];
   char    release[_SYS_NMLN];
   char    version[_SYS_NMLN];
   char    machine[_SYS_NMLN];
   };

#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_SYSTEMINFO_H
# include <sys/systeminfo.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(s) ((unsigned)(s) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(s) (((s) & 255) == 0)
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(s) ((s) & 0)  /* Can't use for BSD */
#endif
#ifndef WTERMSIG
#define WTERMSIG(s) ((s) & 0)
#endif

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <errno.h>

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#   include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include <signal.h>

#include <syslog.h>
extern int errno;

/* Do this for ease of configuration from the Makefile */

#ifdef HPuUX
#define HPUX
#endif

#ifdef SunOS
#define SUN4
#endif

/* end of patch */

#ifdef AIX
#ifndef ps2
#include <sys/statfs.h>
#endif
#endif

#ifdef SOLARIS
#include <sys/statvfs.h>
#undef nfstype
#endif

#ifndef HAVE_BCOPY
#define bcopy(fr,to,n)  memcpy(to,fr,n)  /* Eliminate ucblib */
#define bcmp(s1, s2, n) memcmp ((s1), (s2), (n))
#define bzero(s, n)     memset ((s), 0, (n))
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_MALLOC_H
#ifndef OPENBSD
#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#endif
#endif

#include <fcntl.h>

#ifdef HAVE_VFS_H
# include <sys/vfs.h>
#endif

#ifdef HPUX
# include <sys/dirent.h>
#endif

#ifdef HAVE_UTIME_H
# include <utime.h>      /* use utime not utimes for portability */
#elif TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#elif HAVE_SYS_TIME_H
#  include <sys/time.h>
#elif ! defined(AOS)
#  include <time.h>
#endif

#ifdef HAVE_TIME_H
# include <time.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <pwd.h>
#include <grp.h>

#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#ifndef AOS
# include <arpa/inet.h>
#endif
#include <netdb.h>
#if !defined LINUX && !defined NT
#include <sys/protosw.h>
#undef sgi
#include <net/route.h>
#endif

#ifdef LINUX
#ifdef __GLIBC__
# include <net/route.h>
# include <netinet/in.h>
#else
# include <linux/route.h>
# include <linux/in.h>
#endif
#endif

#ifdef HAVE_RXPOSIX_H
# include <rxposix.h>
#elif  HAVE_REGEX_H
# include <regex.h>
#else
# include "../pub/gnuregex.h"
#endif

#ifndef HAVE_SNPRINTF
#include "../pub/snprintf.h"
#endif


/*******************************************************************/
/* Various defines                                                 */
/*******************************************************************/

#define true  1
#define false 0
#define bufsize 4096
#define buffer_margin 32
#define maxvarsize 1024
#define maxlinksize 256
#define maxlinklevel 4
#define maxargs 31
#define cfmaxiplen 64       /* numerical ip length */
#define noproccols 16
#define hashtablesize 1021  /* prime number */
#define macroalphabet 61    /* a-z, A-Z plus a bit */
#define maxshellargs 30
#define samemode 0
#define sameowner (uid_t)-1
#define unknown_owner (uid_t)-2
#define samegroup (gid_t)-1
#define unknown_group (gid_t)-2
#define cfnosize    -1
#define extra_space 8      /* pads items during AppendItem for eol handling in editfiles */
#define cfinfinity (int)999999999
#define ticksperday 86400 /* 60 * 60 *24 */
#define cf_not_connected -1
#define recursion_limit 100
#define cf_monday_morning 342000

#define exec_ifelapsed 5
#define exec_expireafter 10


/* Need this to to avoid conflict with solaris 2.6 and db.h */

#ifdef SOLARIS
# define u_int32_t uint32_t
# define u_int16_t uint16_t
# define u_int8_t uint8_t
#endif


/*******************************************************************/
/* Class array limits                                              */
/* This is the only place you ever need to edit anything           */
/*******************************************************************/

#define CLSSATTR 35         /* increase this for each new class added */
                            /* It defines the array size for class data */
#define ATTRDIM 3           /* Only used in CLASSATTRUBUTES[][] defn */

   /* end class array limits */

/*******************************************************************/

#define AVDB_FILE     "cf_averages.db"
#define STATELOG_FILE "state_log"
#define ENV_NEW_FILE  "env_data.new"
#define ENV_FILE      "env_data"

#define CFINPUTSVAR "CFINPUTS"          /* default name for file path var */
#define CFALLCLASSESVAR "CFALLCLASSES"  /* default name for CFALLCLASSES env */
#define INFINITERECURSE -99             /* code used to signify inf in recursion */
#define CF_TRUNCATE -1
#define CF_EMPTYFILE -2
#define CF_USELOGFILE true              /* synonyms for tidy.c */
#define CF_NOLOGFILE  false
#define CF_SAVED ".cfsaved"
#define CF_EDITED ".cfedited"
#define CF_NEW ".cfnew"
#define CFD_TERMINATOR "---cfXen/gine/cfXen/gine---"
#define CFD_TRUE "CFD_TRUE"
#define CFD_FALSE "CFD_FALSE"
#define CF_ANYCLASS "any"
#define CF_NOCLASS "XX_CF_opposite_any_XX"
#define CF_NOUSER -99
#define CF_RSA_PROTO_OFFSET 24
#define CF_PROTO_OFFSET 16
#define CF_SMALL_OFFSET 2
#define CF_MD5_LEN 16
#define CF_SHA_LEN 20

#define CF_DONE 't'
#define CF_MORE 'm'

#define CFFAILEDSTR "BAD: Host authentication failed. Did you forget the domain name?"
#define CFCHANGEDSTR1 "BAD: File changed "   /* Split this so it cannot be recognized */
#define CFCHANGEDSTR2 "while copying"

#define CF_START_DOMAIN "undefined.domain"

#define CFLOGSIZE 1048576                  /* Size of lock-log before rotation */

/* Output control defines */

#define Verbose if (VERBOSE || DEBUG || D2) printf
#define EditVerbose  if (EDITVERBOSE || DEBUG || D2) printf
#define Debug3  if (D3 || DEBUG || D2) printf
#define Debug2  if (DEBUG || D2) printf
#define Debug1  if (DEBUG || D1) printf
#define Debug   if (DEBUG || D1 || D2) printf
#define DebugVoid if (false) printf
#define Silent if (! SILENT || VERBOSE || DEBUG || D2) printf
#define DaemonOnly if (ISCFENGINE) yyerror("This belongs in cfservd.conf")
#define CfengineOnly if (! ISCFENGINE) yyerror("This belongs in cfagent.conf")

/* GNU REGEX */

#define BYTEWIDTH 8

/*****************************************************************************/

#define GRAINS   64
#define ATTR     11
#define PH_LIMIT 10
#define CFWEEK   (7.0*24.0*3600.0)
#define MEASURE_INTERVAL (5.0*60.0)


struct Averages
   {
   double expect_number_of_users;
   double expect_rootprocs;
   double expect_otherprocs;
   double expect_diskfree;
   double expect_loadavg;
   double expect_incoming[ATTR];
   double expect_outgoing[ATTR];
   double expect_pH[PH_LIMIT];
      
   double var_number_of_users;
   double var_rootprocs;
   double var_otherprocs;
   double var_diskfree;
   double var_loadavg;
   double var_incoming[ATTR];
   double var_outgoing[ATTR];
   double var_pH[PH_LIMIT];      
   };

/*******************************************************************/
/* Copy file defines                                               */
/*******************************************************************/

            /* Based heavily on cp.c in GNU-fileutils */

#ifndef DEV_BSIZE
#ifdef BSIZE
#define DEV_BSIZE BSIZE
#else /* !BSIZE */
#define DEV_BSIZE 4096
#endif /* !BSIZE */
#endif /* !DEV_BSIZE */
 
/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Optimal I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of 512-byte blocks in the file
   (including indirect blocks). */

#ifndef HAVE_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined(_POSIX_SOURCE) || !defined(BSIZE) /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) (((statbuf).st_size + 512 - 1) / 512)
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) (st_blocks ((statbuf).st_size))
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
                               ? (statbuf).st_blksize : DEV_BSIZE)
# if defined(hpux) || defined(__hpux__) || defined(__hpux)
/* HP-UX counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and BSD filesystems with NFS.  */
#  define ST_NBLOCKS(statbuf) ((statbuf).st_blocks * 2)
# else /* !hpux */
#  if defined(_AIX) && defined(_I386)
/* AIX PS/2 counts st_blocks in 4K units.  */
#    define ST_NBLOCKS(statbuf) ((statbuf).st_blocks * 8)
#  else /* not AIX PS/2 */
#    define ST_NBLOCKS(statbuf) ((statbuf).st_blocks)
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_ST_BLOCKS */

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

/*******************************************************************/
/* Client server defines                                           */
/*******************************************************************/

#define CFENGINE_SERVICE "cfengine"

enum PROTOS
   {
   cfd_exec,
   cfd_auth,
   cfd_get,
   cfd_opendir,
   cfd_synch,
   cfd_classes,
   cfd_md5,
   cfd_smd5,
   cfd_cauth,
   cfd_sauth,
   cfd_ssynch,
   cfd_sget,
   cfd_bad
   };

#define CF_WORDSIZE 8 /* Number of bytes in a word */

enum cf_filetype
   {
   cf_reg,
   cf_link,
   cf_dir,
   cf_fifo,
   cf_block,
   cf_char,
   cf_sock
   };


struct cfstat
   {
   char             *cf_filename;   /* What file are we statting? */
   char             *cf_server;     /* Which server did this come from? */
   enum cf_filetype  cf_type;       /* enum filetype */
   mode_t            cf_lmode;      /* Mode of link, if link */
   mode_t            cf_mode;       /* Mode of remote file, not link */
   uid_t             cf_uid;        /* User ID of the file's owner */
   gid_t             cf_gid;        /* Group ID of the file's group */
   off_t             cf_size;       /* File size in bytes */
   time_t            cf_atime;      /* Time of last access */
   time_t            cf_mtime;      /* Time of last data modification */
   time_t            cf_ctime;      /* Time of last file status change */
   char              cf_makeholes;  /* what we need to know from blksize and blks */
   char             *cf_readlink;   /* link value or NULL */
   int               cf_failed;     /* stat returned -1 */
   int               cf_nlink;      /* Number of hard links */
   int               cf_ino;        /* inode number on server */
   struct cfstat    *next;
   };

struct cfdir
   {
   DIR         *cf_dirh;
   struct Item *cf_list;
   struct Item *cf_listpos;  /* current pos */
   };

typedef struct cfdir CFDIR;

struct cfdirent
   {
   struct dirent *cf_dirp;
   char   d_name[bufsize];   /* This is bigger than POSIX */
   };


enum cfsizes
   {
   cfabs,
   cfpercent
   };


/*******************************************************************/

enum builtin
   {
   nofn,
   fn_randomint,
   fn_newerthan,
   fn_accessedbefore,
   fn_changedbefore,
   fn_fileexists,
   fn_isdir,
   fn_islink,
   fn_isplain,
   fn_execresult,
   fn_returnszero,
   fn_iprange,
   fn_isdefined,
   fn_strcmp,
   fn_showstate
   };

/*******************************************************************/

enum actions
   {
   none,
   control,
   alerts,
   groups,
   image,
   resolve,
   processes,
   files,
   tidy,
   homeservers,
   binservers,
   mailserver,
   required,
   disks,
   mountables,
   links,
   import,
   shellcommands,
   disable,
   makepath,
   ignore,
   broadcast,
   defaultroute,
   misc_mounts,
   editfiles,
   unmounta,
   admit,
   deny,
   acls,
   interfaces,
   filters,
   strategies
   };

/*******************************************************************/

enum classes
   {
   empty,
   soft,
   sun4,
   ultrx,
   hp,
   aix,
   linuxx,
   solaris,
   osf,
   digital,
   sun3,
   irix4,
   irix,
   irix64,
   freebsd,
   solarisx86,
   bsd4_3,
   newsos,
   netbsd,
   aos,
   bsd_i,
   nextstep,
   crayos,
   GnU,
   cfnt,
   unix_sv,
   openbsd,
   cfsco,
   darwin,
   ux4800,
   unused1,
   unused2,
   unused3
   };


/*******************************************************************/

enum fileactions
   {
   warnall,
   warnplain,
   warndirs,
   fixall,
   fixplain,
   fixdirs,
   touch,
   linkchildren,
   create,
   compress,
   alert
   };

/*******************************************************************/

enum fileattr  /* See COMMATTRIBUTES[] in globals.c  for matching entry */
   {
   cfrecurse,
   cfmode,
   cfowner,
   cfgroup,
   cfage,
   cfaction,
   cfpattern,
   cflinks,
   cftype,
   cfdest,
   cfforce,
   cfforcedirs,
   cfforceipv4,
   cfbackup,
   cfrotate,
   cfsize,
   cfmatches,
   cfsignal,
   cfexclude,
   cfcopy,
   cfsymlink,
   cfcptype,
   cflntype,
   cfinclude,
   cfdirlinks,
   cfrmdirs,
   cfserver,
   cfdefine,
   cfelsedef,
   cffailover,
   cftimeout,
   cffree,
   cfnofile,
   cfacl,
   cfpurge,
   cfuseshell,
   cfsetlog,
   cfsetinform,
   cfsetnetmask,
   cfsetbroadcast,
   cfignore,
   cfdeldir,
   cfdelfstab,
   cfstealth,
   cfchksum,
   cfflags,
   cfencryp,
   cfverify,
   cfroot,
   cftypecheck,
   cfumask,
   cfcompress,
   cffilter,
   cffork,
   cfchdir,
   cfchroot,
   cfpreview,
   cfrepository,
   cftimestamps,
   cftrustkey,
   cfcompat,
   cfmountoptions,
   cfreadonly,
   cfbad                        /* HvB must be as last */		
   };


/*******************************************************************/

enum itemtypes
   {
   simple,
   netgroup,
   classscript,
   deletion,
   groupdeletion
   };

/*********************************************************************/

enum vnames 
   {
   cffaculty,
   cfsite,
   cfhost,
   cffqhost,
   cfipaddr,
   cfbinserver,
   cfsysadm,
   cfdomain,
   cftimezone,
   cfnetmask,
   cfnfstype,
   cfssize,
   cfscount,
   cfeditsize,
   cfbineditsize,
   cfactseq,
   cfmountpat,
   cfhomepat,
   cfaddclass,
   cfinstallclass,
   cfschedule,
   cfaccess,
   cfclass,
   cfarch,
   cfarch2,
   cfdate,
   cfyear,
   cfmonth,
   cfday,
   cfhr,
   cfmin,
   cfallclass,
   cfexcludecp,
   cfsinglecp,
   cfautodef,
   cfexcludeln,
   cfcplinks,
   cflncopies,
   cfrepos,
   cfspc,
   cftab,
   cflf,
   cfcr,
   cfn,
   cfdblquote,
   cfquote,
   cfdollar,
   cfrepchar,
   cflistsep,
   cfunderscore,
   cfifname,
   cfexpireafter,
   cfifelapsed,
   cfextension,
   cfsuspicious,
   cfspooldirs,
   cfnonattackers,
   cfattackers,
   cfmulticonn,
   cftrustkeys,
   cfdynamic,
   cfallowusers,
   cfskipverify,
   cfdefcopy,
   cfredef,
   nonexistentvar
   };

/*******************************************************************/

enum resc
   {
   rmountcom,
   runmountcom,
   rethernet,
   rmountopts,
   runused,
   rfstab,
   rmaildir,
   rnetstat,
   rpscomm,
   rpsopts
   };

/*******************************************************************/

enum aseq
   {
   mkpaths,
   lnks,
   chkmail,
   requir,
   diskreq,
   tidyf,
   shellcom,
   chkfiles,
   disabl,
   mountresc,
   edfil,
   mountall,
   umnt,
   resolv,
   imag,
   netconfig,
   tzone,
   mountinfo,
   procs,
   non,
   plugin
   };

/*******************************************************************/

enum editnames
   {
   NoEdit,
   DeleteLinesStarting,
   DeleteLinesContaining,
   DeleteLinesMatching,
   AppendIfNoSuchLine,
   PrependIfNoSuchLine,
   WarnIfNoSuchLine,
   WarnIfLineMatching,
   WarnIfNoLineMatching,
   WarnIfLineStarting,
   WarnIfLineContaining,
   WarnIfNoLineStarting,
   WarnIfNoLineContaining,
   HashCommentLinesContaining,
   HashCommentLinesStarting,
   HashCommentLinesMatching,
   SlashCommentLinesContaining,
   SlashCommentLinesStarting,
   SlashCommentLinesMatching,
   PercentCommentLinesContaining,
   PercentCommentLinesStarting,
   PercentCommentLinesMatching,
   ResetSearch,
   SetSearchRegExp,
   LocateLineMatching,
   InsertLine,
   IncrementPointer,
   ReplaceLineWith,
   DeleteToLineMatching,
   HashCommentToLineMatching,
   PercentCommentToLineMatching,
   SetScript,
   RunScript,
   RunScriptIfNoLineMatching,
   RunScriptIfLineMatching,
   AppendIfNoLineMatching,
   PrependIfNoLineMatching,
   DeleteNLines,
   EmptyEntireFilePlease,
   GotoLastLine,
   BreakIfLineMatches,
   BeginGroupIfNoMatch,
   BeginGroupIfNoLineMatching,
   BeginGroupIfNoSuchLine,
   EndGroup,
   Append,
   Prepend,
   SetCommentStart,
   SetCommentEnd,
   CommentLinesMatching,
   CommentLinesStarting,
   CommentToLineMatching,
   CommentNLines,
   UnCommentNLines,
   ReplaceAll,
   With,
   SetLine,
   FixEndOfLine,
   AbortAtLineMatching,
   UnsetAbort,
   AutoMountDirectResources,
   UnCommentLinesContaining,
   UnCommentLinesMatching,
   InsertFile,
   CommentLinesContaining,
   BeginGroupIfFileIsNewer,
   BeginGroupIfFileExists,
   BeginGroupIfNoLineContaining,
   BeginGroupIfDefined,
   BeginGroupIfNotDefined,
   AutoCreate,
   ForEachLineIn,
   EndLoop,
   ReplaceLinesMatchingField,
   SplitOn,
   AppendToLineIfNotContains,
   DeleteLinesAfterThisMatching,
   DefineClasses,
   ElseDefineClasses,
   CatchAbort,
   EditBackup,
   EditLog,
   EditInform,
   EditRecurse,
   EditMode,
   WarnIfContainsString,
   WarnIfContainsFile,
   EditIgnore,
   EditExclude,
   EditInclude,
   EditRepos,
   EditUmask,
   EditUseShell,
   EditFilter
   };

enum RegExpTypes
   {
   posix,
   gnu,
   bsd
   };


/*******************************************************************/

enum SignalNames
   {
   cfnosignal,
   cfhup,
   cfint,
   cfquit,
   cfill,
   cftrap,
   cfiot,
   cfemt,
   cffpr,
   cfkill,
   cfbus,
   cfsegv,
   cfsys,
   cfpipe,
   cfalrm,
   cfterm,
   };

#define highest_signal 64

/*******************************************************************/

enum cfoutputlevel
   {
   cfsilent,
   cfinform,
   cfverbose,
   cfeditverbose,
   cferror,
   cflogonly
   };

/*******************************************************************/

enum modestate
   {
   wild,
   who,
   which
   };

enum modesort
   {
   unknown,
   numeric,
   symbolic
   };

/*******************************************************************/

typedef char flag;

/*******************************************************************/

struct cfagent_connection
   {
   int sd;
   int trust;
   int family;     /* AF_INET or AF_INET6 */
   char localip[cfmaxiplen];
   char remoteip[cfmaxiplen];
   unsigned char *session_key;
   short error;
   };

/*******************************************************************/

struct CompressedArray
   {
   int key;
   char *value;
   struct CompressedArray *next;
   };

/*******************************************************************/

struct Interface
   {
   char done;
   char *scope;
   char *ifdev;
   char *netmask;
   char *broadcast;
   char *classes;
   struct Interface *next;
   };

/*******************************************************************/

struct Item
   {
   char done;
   char *name;
   char *classes;
   struct Item *next;
   char *scope;
   };

/*******************************************************************/

struct TwoDimList
   {
   short is2d;                  /* true if list > 1 */
   short rounds;
   char  sep;                   /* char used to divide into strings with 1 variable */
   struct Item *ilist;          /* Each node contains a list */
   struct Item *current;        /* A static working pointer */
   struct TwoDimList *next;
   };

/*******************************************************************/

struct Process
   {
   char done;
   char *scope;
   char           *expr;          /* search regex */
   char           *restart;       /* shell comm to be done after */
   char           *chdir;
   char           *chroot;
   uid_t          uid;
   gid_t          gid;
   mode_t         umask;
   short          matches;
   char           comp;
   char           *defines;
   char           *elsedef;
   short          signal;
   char           action;
   char           *classes;
   char           useshell;
   char           log;
   char           inform;
   struct Item    *exclusions;
   struct Item    *inclusions;
   struct Item    *filters;
   struct Process *next;
   };

/*******************************************************************/

/*
 * HvB : Bas van der Vlies
*/
struct Mountables
   {
   char         	done;
   char                 *scope;
   char			readonly;	/* y/n - true false */
   char			*filesystem;
   char			*mountopts;
   struct Mountables	*next;
   };

/*******************************************************************/

struct Tidy
   {
   int          maxrecurse;              /* sets maxval */
   char         done;       /* Too intensive in Tidy Pattern */
   char         *scope;
   char        *path;
   struct Item *exclusions;
   struct Item *ignores;      
   struct TidyPattern *tidylist;
   struct Tidy        *next;   
   };

   /**** SUB CLASS *********************************************/

      struct TidyPattern
         {
         int                recurse;
         short              age;
         int                size;              /* in bytes */
         char               *pattern;        /* synonym for pattern */
         struct Item        *filters;
	 char               *classes;
	 char               *defines;
	 char		    *elsedef;
         char               compress;
         char               travlinks;
         char               dirlinks;          /* k=keep, t=tidy */
         char               rmdirs;            /* t=true, f=false */
         char               searchtype;        /* a, m, c time */
	 char               log;
	 char               inform;
         struct TidyPattern *next;
         };


/*******************************************************************/

struct Mounted
   {
   char *scope;
   char *name;
   char *on;
   char *options;
   char *type;
   };

/*******************************************************************/

struct MiscMount
   {
   char done;
   char *scope;
   char *from;
   char *onto;
   char *options;
   char *classes;
   struct MiscMount *next;
   };

/*******************************************************************/

struct UnMount
   {
   char done;
   char *scope;
   char *name;
   char *classes;
   char deletedir;  /* y/n - true false */
   char deletefstab;
   char force;
   struct UnMount *next;
   };

/*******************************************************************/

struct File
   {
   char   done;
   char *scope;
   char   *path;
   char   *defines;
   char   *elsedef;
   enum   fileactions action;
   mode_t plus;
   mode_t minus;
   int    recurse;
   char   travlinks;
   struct Item *exclusions;
   struct Item *inclusions;
   struct Item *filters;
   struct Item    *ignores;      
   char   *classes;
   struct UidList *uid;
   struct GidList *gid;
   struct File *next;
   struct Item *acl_aliases;
   char   log;
   char   compress;
   char   inform;
   char   checksum;   /* m=md5 n=none */
   u_long plus_flags;    /* for *BSD chflags */
   u_long minus_flags;    /* for *BSD chflags */
   };

/*******************************************************************/

struct Disk
   {
   char  done;
   char *scope;
   char  *name;
   char  *classes;
   char  *define;
   char  *elsedef;
   char   force;	/* HvB: Bas van der Vlies */
   int    freespace;
   struct Disk *next;
   char   log;
   char   inform;
   };

/*******************************************************************/

struct Disable
   {
   char  done;
   char *scope;
   char  *name;
   char  *classes;
   char  *type;
   char  *repository;
   short  rotate;
   int    size;
   char   comp;
   char   action;   /* d=delete,w=warn */
   char   *defines;
   char   *elsedef;
   struct Item  *filters;
   struct Disable *next;
   char   log;
   char   inform;
   };

/*******************************************************************/

struct Image
   {
   char   done;
   char   *scope;
   char   *path;
   char   *destination;
   char   *server;
   char   *repository;
   mode_t plus;
   mode_t minus;
   struct UidList *uid;
   struct GidList *gid;
   char   *action;                                   /* fix / warn */
   char   *classes;
   char   *defines;
   char   *elsedef;
   char   *failover;
   char   force;                                     /* true false */
   char   forcedirs;
   char   forceipv4;
   char   type;                         /* checksum, ctime, binary */
   char   linktype;         /* if file is linked instead of copied */
   char   stealth;               /* preserve times on source files */
   char   preservetimes;                 /* preserve times in copy */
   char   backup;
   int    recurse;
   int    makeholes;
   off_t  size;
   char   comp;
   char   purge;
   struct Item *exclusions;
   struct Item *inclusions;
   struct Item *filters;      
   struct Item *ignores;            
   struct Item *symlink;

   struct cfstat *cache;                              /* stat cache */
   struct CompressedArray *inode_cache;              /* inode cache */
 
   struct in_addr *dns;                      /* Cache gethostbyname */
   int    addrfamily;
   
   struct Image *next;
   struct Item *acl_aliases;
   char   log;
   char   inform;
   char   typecheck;
   char   trustkey;
   char   encrypt;
   char   verify;
   char   compat;
   u_long plus_flags;    /* for *BSD chflags */
   u_long minus_flags;    /* for *BSD chflags */      
   };

/*******************************************************************/

struct UidList
   {
   uid_t uid;
   char *uidname;				/* when uid is -2 */
   struct UidList *next;
   };

/*******************************************************************/

struct GidList
   {
   gid_t gid;
   char *gidname;				/* when gid is -2 */
   struct GidList *next;
   };

/*******************************************************************/

enum cffstype
   {
   posixfs,
   solarisfs,
   dfsfs,
   afsfs,
   hpuxfs,
   ntfs,
   badfs
   };

/*******************************************************************/

struct CFACL
   {
   char * acl_alias;
   enum   cffstype type;
   char   method;            /* a = append, o = overwrite */
   char   nt_acltype;
   struct CFACE *aces;
   struct CFACL *next;
   };

/*******************************************************************/

struct CFACE
   {
#ifdef NT
   char *access;     /* allowed / denied */
   long int NTMode;  /* NT's access mask */
#endif
   char *mode;        /* permission flags*/
   char *name;        /* id name */
   char *acltype;     /* user / group / other */
   char *classes;
   struct CFACE *next;
   };

/*******************************************************************/

struct Link
   {
   char   done;
   char   *scope;
   char   *from;
   char   *to;
   char   *classes;
   char   *defines;
   char   *elsedef;
   char   force;
   short  nofile;
   short  silent;
   char   type;
   char   copytype;
   int    recurse;
   struct Item *exclusions;
   struct Item *inclusions;
   struct Item *ignores;            
   struct Item *filters;      
   struct Item *copy;
   struct Link *next;
   char   log;
   char   inform;
   };

/*******************************************************************/

struct Edit
   {
   char done;     /* Have this here, too dangerous in Edlist */
   char *scope;
   char *fname;
   char *defines;
   char *elsedef;
   mode_t umask;
   char useshell;
   char *repository;
   int   recurse;
   char  binary;   /* y/n */
   struct Item *ignores;
   struct Item *exclusions;
   struct Item *inclusions;      
   struct Item  *filters;      
   struct Edlist *actions;
   struct Edit *next;
   };

   /**** SUB-CLASS ********************************************/

      struct Edlist
         {	 
         enum editnames code;
         char *data;
         struct Edlist *next;
         char *classes;
         };


/*******************************************************************/

enum filternames
   {
   filterresult,
   filterowner,
   filtergroup,
   filtermode,
   filtertype,
   filterfromctime,
   filtertoctime,
   filterfrommtime,
   filtertomtime,
   filterfromatime,
   filtertoatime,
   filterfromsize,
   filtertosize,
   filterexecregex,
   filternameregex,
   filterdefclasses,
   filterelsedef,
   filterexec,
   filtersymlinkto,
   filterpid,
   filterppid,
   filterpgid,
   filterrsize,
   filtersize,
   filterstatus,
   filtercmd,
   filterfromttime,
   filtertottime,   
   filterfromstime,
   filtertostime,
   filtertty,
   filterpriority,
   filterthreads,
   NoFilter
   };

struct Filter
   {
   char *alias;
   char *defines;
   char *elsedef;
   char *classes;
   char context;  /* f=file, p=process */

   char *criteria[NoFilter];  /* array of strings */
      
   struct Filter *next;
   };

/*******************************************************************/

struct ShellComm
   {
   char              done;
   char              *scope;
   char              *name;
   char              *classes;
   char              *chdir;
   char              *chroot;
   int               timeout;
   mode_t            umask;
   uid_t             uid;
   gid_t             gid;
   char              useshell;
   struct ShellComm  *next;
   char              log;
   char              inform;
   char              fork;
   char              *defines;
   char              *elsedef;
   char              preview;
   };

/*******************************************************************/

struct Auth
   {
   char *path;
   struct Item *accesslist;
   struct Item *maproot;    /* which hosts should have root read access */
   int encrypt;              /* which files HAVE to be transmitted securely */
   struct Auth *next;
   };

/*******************************************************************/

struct Strategy
   {
   char   *name;
   char   *classes;
   char   type;                 /* default r=random */
   struct Item *strategies;
   struct Strategy *next;
   };

/*******************************************************************/
/* Ultrix/BSD don't have all these from sys/stat.h                 */
/*******************************************************************/

# ifndef S_IFBLK
#  define S_IFBLK 0060000
# endif
# ifndef S_IFCHR
#  define S_IFCHR 0020000
# endif
# ifndef S_IFDIR
#  define S_IFDIR 0040000
# endif
# ifndef S_IFIFO
#  define S_IFIFO 0010000
# endif
# ifndef S_IFREG
#  define S_IFREG 0100000
# endif
# ifndef S_IFLNK
#  define S_IFLNK 0120000
# endif
# ifndef S_IFSOCK
#  define S_IFSOCK 0140000
# endif
# ifndef S_IFMT
#  define S_IFMT  00170000
# endif


#ifndef S_ISREG
# define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
# define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISLNK
# define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISFIFO
# define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISCHR
# define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
# define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISSOCK
# define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
#endif

#ifndef S_IRUSR
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
 
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
 
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#endif

/********************************************************************/
/* *BSD chflags stuff - Andreas.Klussmann@infosys.heitec.net        */
/********************************************************************/

# if !defined UF_NODUMP
#  define UF_NODUMP 0
# endif
# if !defined UF_IMMUTABLE
#  define UF_IMMUTABLE 0
# endif
# if !defined UF_APPEND
#  define UF_APPEND 0
# endif
# if !defined UF_OPAQUE
#  define UF_OPAQUE 0
# endif
# if !defined UF_NOUNLINK
#  define UF_NOUNLINK 0
# endif
# if !defined SF_ARCHIVED
#  define SF_ARCHIVED 0
# endif
# if !defined SF_IMMUTABLE
#  define SF_IMMUTABLE 0
# endif
# if !defined SF_APPEND
#  define SF_APPEND 0
# endif
# if !defined SF_NOUNLINK
#  define SF_NOUNLINK 0
# endif
# define CHFLAGS_MASK  ( UF_NODUMP | UF_IMMUTABLE | UF_APPEND | UF_OPAQUE | UF_NOUNLINK | SF_ARCHIVED | SF_IMMUTABLE | SF_APPEND | SF_NOUNLINK )

/* For cygwin32 */

#if !defined O_BINARY
#  define O_BINARY 0
#endif

/*******************************************************************/
/* File path manipulation primitives				   */
/*******************************************************************/

/* Defined maximum length of a filename. */

#ifdef NT
#  define MAX_FILENAME 227
#else
#  define MAX_FILENAME 254
#endif

/* File node separator (cygwin can use \ or / but prefer \ for communicating
 * with native windows commands). */

#ifdef NT
#  define IsFileSep(c) ((c) == '\\' || (c) == '/')
#  define FILE_SEPARATOR '\\'
#  define FILE_SEPARATOR_STR "\\"
#else
#  define IsFileSep(c) ((c) == '/')
#  define FILE_SEPARATOR '/'
#  define FILE_SEPARATOR_STR "/"
#endif


/********************************************************************/
/* All prototypes                                                   */
/********************************************************************/

#include "prototypes.h"
