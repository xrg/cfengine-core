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
/*  cfengine function prototypes                                   */
/*                                                                 */
/*  contributed by Stuart Sharpe, September 2000                   */
/*                                                                 */
/*******************************************************************/

/* Define ARGLIST according to whether or not we can use ANSI prototypes */

/* Define PROTOTYPES as well, for the benefit of the MD5 stuff in ../pub */

#ifdef __STDC__
# define ARGLIST(x) x
# ifndef const
#  define const const
# endif
#else
# ifdef __cplusplus
#  define ARGLIST(x) x
# ifndef const
#  define const const
# endif
# else
#  define ARGLIST(x) ()
# endif
#endif

/* pub/full-write.c */

int cf_full_write ARGLIST((int desc, char *ptr, size_t len));

/* 2Dlist.c */

void Set2DList ARGLIST((struct TwoDimList *list));
char *Get2DListEnt ARGLIST((struct TwoDimList *list));
void Build2DListFromVarstring ARGLIST((struct TwoDimList **TwoDimlist, char *varstring, char sep));
int IncrementTwoDimList ARGLIST((struct TwoDimList *from, struct TwoDimList *list));
int EndOfTwoDimList ARGLIST((struct TwoDimList *list));
struct TwoDimList *list;void AppendTwoDimItem ARGLIST((struct TwoDimList **liststart, struct Item *itemlist, char sep));
void Delete2DList ARGLIST((struct TwoDimList *item));

/* acl.c */

void aclsortperror ARGLIST((int error));

#if defined SOLARIS && defined HAVE_SYS_ACL_H
struct acl;
enum cffstype StringToFstype ARGLIST((char *string));
struct CFACL *GetACL ARGLIST((char *acl_alias));
int ParseSolarisMode ARGLIST((char* mode, mode_t oldmode));
int BuildAclEntry ARGLIST((struct stat *sb, char *acltype, char *name, struct acl *newaclbufp));
#endif

void InstallACL ARGLIST((char *alias, char *classes));

void AddACE ARGLIST((char *acl, char *string, char *classes));
int CheckACLs ARGLIST((char *filename, enum fileactions action, struct Item *acl_aliases));
enum cffstype StringToFstype ARGLIST((char *string));
struct CFACL *GetACL ARGLIST((char *acl_alias));

int CheckPosixACE ARGLIST((struct CFACE *aces, char method, char *filename, enum fileactions action));

/* cfd.c 

  Function prototypes for cfd.c are in cfd.c itself, 
  since they are not called from elsewhere.
*/

/* cfengine.c

  Function prototypes for cfengine.c are in cfengine.c itself, 
  since they are not called from elsewhere.
*/

/* cflex.l */

int yylex ARGLIST((void));

/* cfparse.y */

void yyerror ARGLIST((char *s));
int yyparse ARGLIST((void));

/* cfrun.c

  Function prototypes for cfrun.c are in cfrun.c itself, 
  since they are not called from elsewhere.
*/

/* checksums.c */

int CompareCheckSums ARGLIST((char *file1, char *file2, struct Image *ip, struct stat *sstat, struct stat *dstat));
int CompareBinarySums ARGLIST((char *file1, char *file2, struct Image *ip, struct stat *sstat, struct stat *dstat));

/* functions.c */

char *EvaluateFunction ARGLIST((char *function, char *value));
enum builtin FunctionStringToCode ARGLIST((char *str));
int IsBuiltinFunction  ARGLIST((char *function));

/* granules.c  */

char *ConvTimeKey ARGLIST((char *str));
char *GenTimeKey ARGLIST((time_t now));

/* chflags.c */

void ParseFlagString ARGLIST((char *flagstring, u_long *plusmask, u_long *minusmask));

/* client.c */

int OpenServerConnection ARGLIST((struct Image *ip));
void CloseServerConnection ARGLIST((void));
int cf_rstat ARGLIST((char *file, struct stat *buf, struct Image *ip, char *stattype));
CFDIR *cf_ropendir ARGLIST((char *dirname, struct Image *ip));
void FlushClientCache ARGLIST((struct Image *ip));
int CompareMD5Net ARGLIST((char *file1, char *file2, struct Image *ip));
int CopyRegNet ARGLIST((char *source, char *new, struct Image *ip, off_t size));
int GetCachedStatData ARGLIST((char *file, struct stat *statbuf, struct Image *ip, char *stattype));
void CacheData ARGLIST((struct cfstat *data, struct Image *ip));
void FlushToEnd ARGLIST((int sd, int toget));
int cfprintf(char *out, int len2, char *in1, char *in2, char *in3);
struct cfagent_connection *NewAgentConn ARGLIST((void));
void DeleteAgentConn ARGLIST((struct cfagent_connection *ap));
int RemoteConnect ARGLIST((char *host,int ipv4));

/* comparray.c */

int FixCompressedArrayValue ARGLIST((int i, char *value, struct CompressedArray **start));
void DeleteCompressedArray ARGLIST((struct CompressedArray *start));
int CompressedArrayElementExists ARGLIST((struct CompressedArray *start, int key));
char *CompressedArrayValue ARGLIST((struct CompressedArray *start, int key));

/* copy.c */

void CheckForHoles ARGLIST((struct stat *sstat, struct Image *ip));
int CopyRegDisk ARGLIST((char *source, char *new, struct Image *ip));
int EmbeddedWrite ARGLIST((char *new,int dd,char *buf,struct Image *ip,int towrite,int *last_write_made_hole,int n_read));

/* dce_acl.c */

struct CFACE;
int CheckDFSACE ARGLIST((struct CFACE *aces, char method, char *filename, enum fileactions action));

/* df.c */

int GetDiskUsage  ARGLIST((char *file, enum cfsizes type));

/* do.c */

void GetHomeInfo ARGLIST((void));
void GetMountInfo ARGLIST((void));
void MakePaths ARGLIST((void));
void MakeChildLinks ARGLIST((void));
void MakeLinks ARGLIST((void));
void MailCheck ARGLIST((void));
void ExpiredUserCheck ARGLIST((char *spooldir, int always));
void MountFileSystems ARGLIST((void));
void CheckRequired ARGLIST((void));
void TidyFiles ARGLIST((void));
void TidyHome ARGLIST((void));
void TidyHomeDir ARGLIST((struct TidyPattern *ptr, char *subdir));
void Scripts ARGLIST((void));
void GetSetuidLog ARGLIST((void));
void CheckFiles ARGLIST((void));
void SaveSetuidLog ARGLIST((void));
void DisableFiles ARGLIST((void));
void MountHomeBinServers ARGLIST((void));
void MountMisc ARGLIST((void));
void Unmount ARGLIST((void));
void EditFiles ARGLIST((void));
void CheckResolv ARGLIST((void));
void MakeImages ARGLIST((void));
void ConfigureInterfaces ARGLIST((void));
void CheckTimeZone ARGLIST((void));
void CheckProcesses ARGLIST((void));
int RequiredFileSystemOkay ARGLIST((char *name));
void InstallMountedItem ARGLIST((char *host, char *mountdir));
void InstallMountableItem ARGLIST((char *path, char *mnt_opts, flag readonly));
void AddToFstab ARGLIST((char *host, char *mountpt, char *rmountpt, char *mode, char *options, int ismounted));
int CheckFreeSpace ARGLIST((char *file, struct Disk *ptr));
void CheckHome ARGLIST((struct File *ptr));
void EditItemsInResolvConf ARGLIST((struct Item *from, struct Item **list));
int TZCheck ARGLIST((char *tzsys, char *tzlist));
void ExpandWildCardsAndDo ARGLIST((char *wildpath, char *buffer, void (*function)(char *path, void *ptr), void *argptr));
int TouchDirectory ARGLIST((struct File *ptr));
void RecFileCheck ARGLIST((char *startpath, void *vp));
int MatchStringInFstab ARGLIST((char *str));

/* edittools.c */

int DoRecursiveEditFiles ARGLIST((char *name, int level, struct Edit *ptr,struct stat *sb));
void DoEditHomeFiles ARGLIST((struct Edit *ptr));
void WrapDoEditFile ARGLIST((struct Edit *ptr, char *filename));
void DoEditFile ARGLIST((struct Edit *ptr, char *filename));
int IncrementEditPointer ARGLIST((char *str, struct Item *liststart));
int ResetEditSearch  ARGLIST((char *str, struct Item *list));
int ReplaceEditLineWith  ARGLIST((char *string));
int RunEditScript  ARGLIST((char *script, char *fname, struct Item **filestart, struct Edit *ptr));
void DoFixEndOfLine ARGLIST((struct Item *list, char *type));
void HandleAutomountResources ARGLIST((struct Item **filestart, char *opts));
void CheckEditSwitches ARGLIST((char *filename, struct Edlist *actions));
void AddEditfileClasses  ARGLIST((struct Edit *list, int editsdone));
struct Edlist *ThrowAbort ARGLIST((struct Edlist *from));
struct Edlist *SkipToEndGroup ARGLIST((struct Edlist *ep, char *filename));
int BinaryEditFile ARGLIST((struct Edit *ptr, char *filename));
int LoadBinaryFile ARGLIST((char *source, off_t size, void *memseg));
int SaveBinaryFile ARGLIST((char *file, off_t size, void *memseg, char *repository));
void WarnIfContainsRegex ARGLIST((void *memseg, off_t size, char *data, char *filename));
void WarnIfContainsFilePattern ARGLIST((void *memseg, off_t size, char *data, char *filename));
int BinaryReplaceRegex ARGLIST((void *memseg, off_t size, char *search, char *replace, char *filename));

/* crypto.c */

void LoadSecretKeys ARGLIST((void));
void MD5Random ARGLIST((unsigned char digest[EVP_MAX_MD_SIZE+1]));
int EncryptString ARGLIST((char *in, char *out, unsigned char *key, int len));
int DecryptString ARGLIST((char *in, char *out, unsigned char *key, int len));
RSA *HavePublicKey ARGLIST((char *ipaddress));
void SavePublicKey ARGLIST((char *ipaddress, RSA *key));
void DeletePublicKey ARGLIST((char *ipaddress));
void GenerateRandomSessionKey ARGLIST((void));

/* errors.c */

void FatalError ARGLIST((char *s));
void Warning ARGLIST((char *s));
void ResetLine ARGLIST((char *s));

/* eval.c */

int Day2Number ARGLIST((char *s));
int Month2Number ARGLIST((char *s));
void AddInstallable ARGLIST((char *classlist));
void AddMultipleClasses ARGLIST((char *classlist));
void AddTimeClass ARGLIST((char *str));
void AddClassToHeap ARGLIST((char *class));
void DeleteClassFromHeap ARGLIST((char *class));
int IsHardClass ARGLIST((char *sp));
int IsSpecialClass ARGLIST((char *class));
int IsExcluded ARGLIST((char *exception));
int IsDefinedClass ARGLIST((char *class));
int IsInstallable ARGLIST((char *class));
void AddCompoundClass ARGLIST((char *class));
void NegateCompoundClass ARGLIST((char *class, struct Item **heap));
int EvaluateORString ARGLIST((char *class, struct Item *list));
int EvaluateANDString ARGLIST((char *class, struct Item *list));
int GetORAtom ARGLIST((char *start, char *buffer));
int GetANDAtom ARGLIST((char *start, char *buffer));
int CountEvalAtoms ARGLIST((char *class));
enum actions ActionStringToCode  ARGLIST((char *str));
int IsBracketed ARGLIST((char *s));

/* filedir.c */

int IsHomeDir ARGLIST((char *name));
int EmptyDir ARGLIST((char *path));
int RecursiveCheck ARGLIST((char *name, mode_t plus, mode_t minus, enum fileactions action, struct UidList *uidlist, struct GidList *gidlist, int recurse, int rlevel, struct File *ptr,struct stat *sb));
void CheckExistingFile ARGLIST((char *file, mode_t plus, mode_t minus, enum fileactions action, struct UidList *uidlist, struct GidList *gidlist, struct stat *dstat, struct File *ptr, struct Item *acl_aliases));
void CheckCopiedFile ARGLIST((char *file, mode_t plus, mode_t minus, enum fileactions action, struct UidList *uidlist, struct GidList *gidlist, struct stat *dstat, struct stat *sstat, struct File *ptr, struct Item *acl_aliases));
int CheckOwner ARGLIST((char *file, enum fileactions action, struct UidList *uidlist, struct GidList *gidlist, struct stat *statbuf));
int CheckHomeSubDir ARGLIST((char *testpath, char *tidypath, int recurse));
int FileIsNewer ARGLIST((char *file1, char *file2));
int IgnoreFile  ARGLIST((char *pathto, char *name, struct Item *ignores));
void CompressFile ARGLIST((char *file));

/* filenames.c */

int IsAbsoluteFileName ARGLIST((char *f));
int RootDirLength ARGLIST((char *f));
void AddSlash ARGLIST((char *str));
void DeleteSlash ARGLIST((char *str));
void DeleteNewline ARGLIST((char *str));
char *LastFileSeparator ARGLIST((char *str));
int ChopLastNode ARGLIST((char *str));
char *CanonifyName ARGLIST((char *str));
char *Space2Score ARGLIST((char *str));
char *ASUniqueName ARGLIST((char *str));
char *ReadLastNode ARGLIST((char *str));
int MakeDirectoriesFor ARGLIST((char *file, char force));
int BufferOverflow ARGLIST((char *str1, char *str2));
void Chop ARGLIST((char *str));
int CompressPath ARGLIST((char *dest, char *src));
char ToLower  ARGLIST((char ch));
char ToUpper  ARGLIST((char ch));
char *ToUpperStr  ARGLIST((char *str));
char *ToLowerStr  ARGLIST((char *str));

/* filters.c */

void InstallFilter ARGLIST((char *filter));
void CheckFilters ARGLIST((void));
void InstallFilterTest ARGLIST((char *alias, char *type, char *data));
enum filternames FilterActionsToCode ARGLIST((char *filtertype));
int FilterExists ARGLIST((char *name));
int ProcessFilter ARGLIST((char *proc, struct Item *filterlist,char **names,int *start,int *stop));
void SplitLine ARGLIST((char *proc, struct Item *filterlist,char **names,int *start,int *stop,char **line));
int FileObjectFilter ARGLIST((char *file, struct stat *statptr, struct Item *filterlist, enum actions context));
time_t Date2Number ARGLIST((char *string, time_t now));
void Size2Number ARGLIST((char *buffer));
int FilterTypeMatch ARGLIST((struct stat *ptr,char *match));
int FilterOwnerMatch ARGLIST((struct stat *lstatptr,char *crit));
int FilterGroupMatch ARGLIST((struct stat *lstatptr,char *crit));
int FilterModeMatch ARGLIST((struct stat *lstatptr,char *crit));
int FilterTimeMatch ARGLIST((time_t stattime,char *from,char *to));
int FilterNameRegexMatch ARGLIST((char *file,char *crit));
int FilterExecRegexMatch ARGLIST((char *file,char *crit));
int FilterExecMatch ARGLIST((char *file,char *crit));
int FilterIsSymLinkTo ARGLIST((char *file,char *crit));
void DoFilter ARGLIST((struct Item **attr,char **crit,struct stat *lstatptr,char *filename));
void GetProcessColumns ARGLIST((char *proc,char **names,int *start,int *stop));
int FilterProcMatch ARGLIST((char *name1,char *name2,char *expr,char **names,char **line));
int FilterProcSTimeMatch  ARGLIST((char *name1,char *name2,char *expr1,char *expr2,char **names,char **line));
int FilterProcTTimeMatch  ARGLIST((char *name1,char *name2,char *expr1,char *expr2,char **names,char **line));
void DoProc ARGLIST((struct Item **attr,char **crit,char **names,char **line));


/* ifconf.c */

void IfConf  ARGLIST((char *vifdev, char *vnetmask, char *vbroadcast));
int GetIfStatus ARGLIST((int sk, char *vifdev, char *vnetmask, char *vbroadcast));
void SetIfStatus ARGLIST((int sk, char *vifdev, char *vnetmask, char *vbroadcast));
void GetBroadcastAddr ARGLIST((char *ipaddr, char *vifdev, char *vnetmask, char *vbroadcast));
void SetDefaultRoute ARGLIST((void));

/* image.c */

void RecursiveImage ARGLIST((struct Image *ip, char *from, char *to, int maxrecurse));
void CheckHomeImages ARGLIST((struct Image *ip));
void CheckImage ARGLIST((char *source, char *destination, struct Image *ip));
void PurgeFiles ARGLIST((struct Item *filelist, char *directory, struct Item *exclusions));
void ImageCopy ARGLIST((char *sourcefile, char *destfile, struct stat sourcestatbuf, struct Image *ip));
int cfstat ARGLIST((char *file, struct stat *buf, struct Image *ip));
int cflstat ARGLIST((char *file, struct stat *buf, struct Image *ip));
int cfreadlink ARGLIST((char *sourcefile, char *linkbuf, int buffsize, struct Image *ip));
CFDIR *cfopendir ARGLIST((char *name, struct Image *ip));
struct cfdirent *cfreaddir ARGLIST((CFDIR *cfdirh, struct Image *ip));
void cfclosedir ARGLIST((CFDIR *dirh));
int CopyReg  ARGLIST((char *source, char *dest, struct stat sstat, struct stat dstat, struct Image *ip));
void RegisterHardLink ARGLIST((int i, char *value, struct Image *ip));

/* init.c */

void CheckWorkDirectories ARGLIST((void));
void RandomSeed ARGLIST((void));
void SetSignals ARGLIST((void));

/* install.c */

void InstallLocalInfo  ARGLIST((char *varvalue));
void HandleEdit ARGLIST((char *file, char *edit, char *string));
void HandleOptionalFileAttribute ARGLIST((char *item));
void HandleOptionalMountablesAttribute ARGLIST((char *item));
void HandleOptionalImageAttribute ARGLIST((char *item));
void HandleOptionalRequired ARGLIST((char *item));
void HandleOptionalInterface ARGLIST((char *item));
void HandleOptionalUnMountAttribute ARGLIST((char *item));
void HandleOptionalMiscMountsAttribute ARGLIST((char *item));
void HandleOptionalTidyAttribute ARGLIST((char *item));
void HandleOptionalDirAttribute ARGLIST((char *item));
void HandleOptionalDisableAttribute ARGLIST((char *item));
void HandleOptionalLinkAttribute ARGLIST((char *item));
void HandleOptionalProcessAttribute ARGLIST((char *item));
void HandleOptionalScriptAttribute ARGLIST((char *item));
void HandleChDir ARGLIST((char *value));
void HandleChRoot ARGLIST((char *value));
void HandleFileItem ARGLIST((char *item));
void InstallBroadcastItem ARGLIST((char *item));
void InstallDefaultRouteItem ARGLIST((char *item));
void HandleGroupItem ARGLIST((char *item, enum itemtypes type));
void HandleHomePattern ARGLIST((char *pattern));
void AppendNameServer ARGLIST((char *item));
void AppendImport ARGLIST((char *item));
void InstallHomeserverItem ARGLIST((char *item));
void InstallBinserverItem ARGLIST((char *item));
void InstallMailserverPath ARGLIST((char *path));
void InstallLinkItem  ARGLIST((char *from, char *to));
void InstallLinkChildrenItem  ARGLIST((char *from, char *to));
void InstallRequiredPath ARGLIST((char *path, int freespace));
void AppendMountable ARGLIST((char *path));
void AppendUmount ARGLIST((char *path, char deldir, char delfstab, char force));
void AppendMiscMount ARGLIST((char *from, char *onto, char *perm));
void AppendIgnore ARGLIST((char *path));
void InstallPending ARGLIST((enum actions action));
int EditFileExists ARGLIST((char *file));
void GetExecOutput ARGLIST((char *command, char *buffer));
void InstallEditFile ARGLIST((char *file, char *edit, char *data));
void AddEditAction ARGLIST((char *file, char *edit, char *data));
enum editnames EditActionsToCode ARGLIST((char *edit));
void AppendInterface ARGLIST((char *ifname, char *netmask, char *broadcast));
void AppendScript ARGLIST((char *item, int timeout, char useshell, char *uidname, char *gidname));
void AppendDisable ARGLIST((char *path, char *type, short int rotate, char comp, int size));
void InstallTidyItem  ARGLIST((char *path, char *wild, int rec, short int age, char travlinks, int tidysize, char type, char ldirs, short int tidydirs, char *classes));
void InstallMakePath ARGLIST((char *path, mode_t plus, mode_t minus, char *uidnames, char *gidnames));
void HandleTravLinks ARGLIST((char *value));
void HandleTidySize ARGLIST((char *value));
void HandleUmask ARGLIST((char *value));
void HandleDisableSize ARGLIST((char *value));
void HandleCopySize ARGLIST((char *value));
void HandleRequiredSize ARGLIST((char *value));
void HandleTidyType ARGLIST((char *value));
void HandleTidyLinkDirs ARGLIST((char *value));
void HandleTidyRmdirs ARGLIST((char *value));
void HandleCopyBackup ARGLIST((char *value));
void HandleTimeOut ARGLIST((char *value));
void HandleUseShell ARGLIST((char *value));
void HandleFork ARGLIST((char *value));
void HandleChecksum ARGLIST((char *value));
void HandleTimeStamps ARGLIST((char *value));
int GetFileAction ARGLIST((char *action));
void InstallFileListItem ARGLIST((char *path, mode_t plus, mode_t minus, enum fileactions action, char *uidnames, char *gidnames, int recurse, char travlinks, char chksum));
void InstallProcessItem ARGLIST((char *expr, char *restart, short int matches, char comp, short int signal, char action, char *classes, char useshell, char *uidname, char *gidname));
void InstallImageItem ARGLIST((char *path, mode_t plus, mode_t minus, char *destination, char *action, char *uidnames, char *gidnames, int size, char comp, int rec, char type, char lntype, char *server));
void InstallAuthItem ARGLIST((char *path, char *attribute, struct Auth **list, struct Auth **listtop, char *classes));
int GetCommAttribute ARGLIST((char *s));
void HandleRecurse ARGLIST((char *value));
void HandleCopyType ARGLIST((char *value));
void HandleDisableFileType ARGLIST((char *value));
void HandleDisableRotate ARGLIST((char *value));
void HandleAge ARGLIST((char *days));
void HandleProcessMatches ARGLIST((char *value));
void HandleProcessSignal ARGLIST((char *value));
void HandleNetmask ARGLIST((char *value));
void HandleBroadcast ARGLIST((char *value));
void AppendToActionSequence  ARGLIST((char *action));
void AppendToAccessList  ARGLIST((char *user));
void HandleLinkAction ARGLIST((char *value));
void HandleDeadLinks ARGLIST((char *value));
void HandleLinkType ARGLIST((char *value));
void HandleServer ARGLIST((char *value));
void HandleDefine ARGLIST((char *value));
void HandleElseDefine ARGLIST((char *value));
struct UidList *MakeUidList ARGLIST((char *uidnames));
struct GidList *MakeGidList ARGLIST((char *gidnames));
void InstallTidyPath ARGLIST((char *path, char *wild, int rec, short int age, char travlinks, int tidysize, char type, char ldirs, short int tidydirs, char *classes));
void AddTidyItem ARGLIST((char *path, char *wild, int rec, short int age, char travlinks, int tidysize, char type, char ldirs, short int tidydirs, char *classes));
int TidyPathExists ARGLIST((char *path));
void AddSimpleUidItem ARGLIST((struct UidList **uidlist, int uid, char *uidname));
void AddSimpleGidItem ARGLIST((struct GidList **gidlist, int gid, char *gidname));
void InstallAuthPath ARGLIST((char *path, char *hostname, char *classes, struct Auth **list, struct Auth **listtop));
void AddAuthHostItem ARGLIST((char *path, char *attribute, char *classes, struct Auth **list));
int AuthPathExists ARGLIST((char *path, struct Auth *list));
int HandleAdmitAttribute ARGLIST((struct Auth *ptr, char *attribute));
void PrependTidy ARGLIST((struct TidyPattern **list, char *wild, int rec, short int age, char travlinks, int tidysize, char type, char ldirs, short int tidydirs, char *classes));
void HandleShortSwitch ARGLIST((char *name,char *value,short *flag));
void HandleCharSwitch ARGLIST((char *name,char *value,char *flag));

/* ip.c */

char *sockaddr_ntop ARGLIST((struct sockaddr *sa));
void *sockaddr_pton ARGLIST((int af,void *src));
short CfenginePort ARGLIST((void));
int IsIPV6Address ARGLIST((char *name));

/* item-ext.c */

int OrderedListsMatch ARGLIST((struct Item *list1, struct Item *list2));
int RegexOK ARGLIST((char *string));
int IsWildItemIn ARGLIST((struct Item *list, char *item));
void InsertItemAfter  ARGLIST((struct Item **filestart, struct Item *ptr, char *string));
void InsertFileAfter  ARGLIST((struct Item **filestart, struct Item *ptr, char *string));
struct Item *LocateNextItemContaining ARGLIST((struct Item *list,char *string));
struct Item *LocateNextItemMatching ARGLIST((struct Item *list,char *string));
struct Item *LocateNextItemStarting ARGLIST((struct Item *list,char *string));
struct Item *LocateItemMatchingRegExp ARGLIST((struct Item *list,char *string));
struct Item *LocateItemContainingRegExp ARGLIST((struct Item *list,char *string));
int DeleteToRegExp ARGLIST((struct Item **filestart, char *string));
int DeleteItemStarting ARGLIST((struct Item **list,char *string));
int DeleteItemMatching ARGLIST((struct Item **list,char *string));
int DeleteItemContaining ARGLIST((struct Item **list,char *string));
int CommentItemStarting ARGLIST((struct Item **list, char *string, char *comm, char *end));
int CommentItemContaining ARGLIST((struct Item **list, char *string, char *comm, char *end));
int CommentItemMatching ARGLIST((struct Item **list, char *string, char *comm, char *end));
int UnCommentItemMatching ARGLIST((struct Item **list, char *string, char *comm, char *end));
int UnCommentItemContaining ARGLIST((struct Item **list, char *string, char *comm, char *end));
int CommentToRegExp ARGLIST((struct Item **filestart, char *string, char *comm, char *end));
int DeleteSeveralLines  ARGLIST((struct Item **filestart, char *string));
struct Item *GotoLastItem ARGLIST((struct Item *list));
int LineMatches  ARGLIST((char *line, char *regexp));
int GlobalReplace ARGLIST((struct Item **liststart, char *search, char *replace));
int CommentSeveralLines  ARGLIST((struct Item **filestart, char *string, char *comm, char *end));
int UnCommentSeveralLines  ARGLIST((struct Item **filestart, char *string, char *comm, char *end));
int ItemMatchesRegEx ARGLIST((char *item, char *regex));
void ReplaceWithFieldMatch ARGLIST((struct Item **filestart, char *field, char *replace, char split, char *filename));
void AppendToLine ARGLIST((struct Item *current, char *text, char *filename));
int CfRegcomp ARGLIST((regex_t *preg, const char *regex, int cflags));

/* item-file.c */

int LoadItemList ARGLIST((struct Item **liststart, char *file));
int SaveItemList ARGLIST((struct Item *liststart, char *file, char *repository));
int CompareToFile ARGLIST((struct Item *liststart, char *file));

/* item.c */

int IsItemIn ARGLIST((struct Item *list, char *item));
int IsClassedItemIn ARGLIST((struct Item *list, char *item));
int IsFuzzyItemIn ARGLIST((struct Item *list, char *item));
int FuzzySetMatch ARGLIST((char *s1, char *s2));
int FuzzyMatchParse ARGLIST((char *item));
void PrependItem  ARGLIST((struct Item **liststart, char *itemstring, char *classes));
void AppendItem  ARGLIST((struct Item **liststart, char *itemstring, char *classes));
void DeleteItemList ARGLIST((struct Item *item));
void DeleteItem ARGLIST((struct Item **liststart, struct Item *item));
void DebugListItemList ARGLIST((struct Item *liststart));
int ItemListsEqual ARGLIST((struct Item *list1, struct Item *list2));
struct Item *SplitStringAsItemList ARGLIST((char *string, char sep));

/* link.c */

struct Link;

int LinkChildFiles ARGLIST((char *from, char *to, char type, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));
void LinkChildren ARGLIST((char *path, char type, struct stat *rootstat, uid_t uid, gid_t gid, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));
int RecursiveLink ARGLIST((struct Link *lp, char *from, char *to, int maxrecurse));
int LinkFiles ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short nofile, struct Link *ptr));
int RelativeLink ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));
int AbsoluteLink ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));
int DoLink  ARGLIST((char *from, char *to, char *defines));
void KillOldLink ARGLIST((char *name, char *defines));
int HardLinkFiles ARGLIST((char *from, char *to, struct Item *inclusions, struct Item *exclusions, struct Item *copy, short int nofile, struct Link *ptr));
void DoHardLink  ARGLIST((char *from, char *to, char *defines));
int ExpandLinks ARGLIST((char *dest, char *from, int level));
char *AbsLinkPath ARGLIST((char *from, char *relto));

/* locks.c */

void PreLockState ARGLIST((void));
void InitializeLocks ARGLIST((void));
void CloseLocks ARGLIST((void));
void HandleSignal ARGLIST((int signum));
int GetLock ARGLIST((char *operator, char *operand, int ifelapsed, int expireafter, char *host, time_t now));
void ReleaseCurrentLock ARGLIST((void));
int CountActiveLocks ARGLIST((void));
time_t GetLastLock ARGLIST((void));
time_t CheckOldLock ARGLIST((void));
void SetLock ARGLIST((void));
void LockLog ARGLIST((int pid, char *str, char *operator, char *operand));
int PutLock ARGLIST((char *name));
int DeleteLock ARGLIST((char *name));
time_t GetLockTime ARGLIST((char *name));
pid_t GetLockPid ARGLIST((char *name));

/* log.c */

void CfLog ARGLIST((enum cfoutputlevel level, char *string, char *errstr));
void ResetOutputRoute  ARGLIST((char log, char inform));
void ShowAction ARGLIST((void));

/* macro.c */

void InitHashTable ARGLIST((void));
void PrintHashTable ARGLIST((void));
int Hash ARGLIST((char *name));
int ElfHash ARGLIST((char *name));
void AddMacroValue ARGLIST((char *name, char *value));
char *GetMacroValue ARGLIST((char *name));
void RecordMacroId ARGLIST((char *name));
int CompareMacro ARGLIST((char *name, char *macro));
void DeleteMacros ARGLIST((void));
void DeleteMacro  ARGLIST((char *name));


/* misc.c */

int DirPush ARGLIST((char *name,struct stat *sb));
void DirPop ARGLIST((int goback,char *name,struct stat *sb));
void CheckLinkSecurity ARGLIST((struct stat *sb, char *name));
void GetNameInfo ARGLIST((void));
void AddNetworkClass ARGLIST((char *netmask));
void TruncateFile ARGLIST((char *name));
int FileSecure  ARGLIST((char *name));
int ChecksumChanged ARGLIST((char *filename, unsigned char digest[EVP_MAX_MD_SIZE+1], int warnlevel, int refresh, char type));
char *ChecksumPrint   ARGLIST((char type,unsigned char digest[EVP_MAX_MD_SIZE+1]));
void ChecksumFile  ARGLIST((char *filename,unsigned char digest[EVP_MAX_MD_SIZE+1],char type));
int ChecksumsMatch ARGLIST((unsigned char digest1[EVP_MAX_MD_SIZE+1],unsigned char digest2[EVP_MAX_MD_SIZE+1],char type));
void ChecksumString  ARGLIST((char *buffer,int len,unsigned char digest[EVP_MAX_MD_SIZE+1],char type));
int IgnoredOrExcluded ARGLIST((enum actions action, char *file, struct Item *inclusions, struct Item *exclusions));
void Banner ARGLIST((char *string));
void SetDomainName ARGLIST((char *sp));
void GetInterfaceInfo ARGLIST((void));
void GetV6InterfaceInfo ARGLIST((void));
void DebugBinOut ARGLIST((char *string, int len));
int ShellCommandReturnsZero ARGLIST((char *comm));
void SetClassesOnScript ARGLIST((char *comm, char *classes, char *elseclasses, int useshell));
/* modes.c */

void ParseModeString ARGLIST((char *modestring, mode_t *plusmask, mode_t *minusmask));
void CheckModeState ARGLIST((enum modestate stateA, enum modestate stateB,enum modesort modeA, enum modesort modeB, char ch));
void SetMask ARGLIST((char action, int value, int affected, mode_t *p, mode_t *m));

/* mount.c */

int MountPathDefined ARGLIST((void));
int MatchAFileSystem ARGLIST((char *server, char *lastlink));
int IsMountedFileSystem  ARGLIST((struct stat *childstat, char *dir, int rlevel));

/* net.c */

void TimeOut ARGLIST((void));
int SendTransaction ARGLIST((int sd, char *buffer,int len, char status));
int ReceiveTransaction ARGLIST((int sd, char *buffer,int *more));
int RecvSocketStream ARGLIST((int sd, char *buffer, int toget, int nothing));
int SendSocketStream ARGLIST((int sd, char *buffer, int toget, int flags));

/* parse.c */

void ParseInputFiles ARGLIST((void));
int ParseBootFiles ARGLIST((void));
void ParseFile ARGLIST((char *f,char *env));
void NewParser ARGLIST((void));
void DeleteParser ARGLIST((void));
void SetAction  ARGLIST((enum actions action));
void HandleId ARGLIST((char *id));
void HandleClass  ARGLIST((char *id));
void HandleItem  ARGLIST((char *item));
void HandlePath  ARGLIST((char *path));
void HandleVarpath  ARGLIST((char *varpath));
void HandleWildcard ARGLIST((char *wildcard));
int CompoundId ARGLIST((char *id));
void InitializeAction ARGLIST((void));
void SetMountPath  ARGLIST((char *value));
void SetRepository  ARGLIST((char *value));

/* strategies.c */

void InstallStrategy ARGLIST((char *value, char *classes));
void AddClassToStrategy ARGLIST((char *alias,char *class,char *value));
void SetStrategies ARGLIST((void));
void GetNonMarkov ARGLIST((void));

/* patches.c */

#ifndef HAVE_GETNETGRENT

void setnetgrent ARGLIST((const char *netgroup));
int getnetgrent ARGLIST((char **host, char **user, char **domain));
void endnetgrent ARGLIST((void));
#endif

#ifndef HAVE_UNAME
int uname  ARGLIST((struct utsname *name));
#endif

#ifndef HAVE_STRSTR
char *strstr ARGLIST((char *s1,char *s2));
#endif

#ifndef HAVE_STRDUP
char *strdup ARGLIST((char *str));
#endif

#ifndef HAVE_STRRCHR
char *strrchr ARGLIST((char *str,char ch));
#endif

#ifndef HAVE_STRERROR
char *strerror ARGLIST((int err));
#endif

#ifndef HAVE_PUTENV
int putenv  ARGLIST((char *s));
#endif

#ifndef HAVE_SETEUID
int seteuid ARGLIST((uid_t euid));
#endif

#ifndef HAVE_SETEUID
int setegid ARGLIST((gid_t egid));
#endif

int IsPrivileged ARGLIST((void));

/* popen.c */

FILE *cfpopensetuid ARGLIST((char *command, char *type, uid_t uid, gid_t gid, char *chdirv, char *chrootv));
FILE *cfpopen ARGLIST((char *command, char *type));
FILE *cfpopen_sh ARGLIST((char *command, char *type));
FILE *cfpopen_shsetuid ARGLIST((char *command, char *type, uid_t uid, gid_t gid, char *chdirv, char *chrootv));
int cfpclose ARGLIST((FILE *pp));
int cfpclose_def ARGLIST((FILE *pp, char *defines, char *elsedef));
int SplitCommand ARGLIST((char *comm, char (*arg)[4096]));

/* process.c */

int LoadProcessTable ARGLIST((struct Item **procdata, char *psopts));
void DoProcessCheck ARGLIST((struct Process *pp, struct Item *procdata));
int FindMatches ARGLIST((struct Process *pp, struct Item *procdata, struct Item **killlist));
void DoSignals ARGLIST((struct Process *pp,struct Item *list));

/* proto.c */
int IdentifyForVerification ARGLIST((int sd,char *localip, int family));
int KeyAuthentication ARGLIST((struct Image *ip));
int BadProtoReply  ARGLIST((char *buf));
int OKProtoReply  ARGLIST((char *buf));
int FailedProtoReply  ARGLIST((char *buf));

/* read.c */
int ReadLine ARGLIST((char *buff, int size, FILE *fp));

/* report.c */

void ListDefinedClasses ARGLIST((void));
void ListDefinedStrategies ARGLIST((void));
void ListDefinedInterfaces ARGLIST((void));
void ListDefinedHomePatterns ARGLIST((void));
void ListDefinedBinservers ARGLIST((void));
void ListDefinedLinks ARGLIST((void));
void ListDefinedLinkchs ARGLIST((void));
void ListDefinedResolvers ARGLIST((void));
void ListDefinedScripts ARGLIST((void));
void ListDefinedImages ARGLIST((void));
void ListDefinedTidy ARGLIST((void));
void ListDefinedMountables ARGLIST((void));
void ListMiscMounts ARGLIST((void));
void ListDefinedRequired ARGLIST((void));
void ListDefinedHomeservers ARGLIST((void));
void ListDefinedDisable ARGLIST((void));
void ListDefinedMakePaths ARGLIST((void));
void ListDefinedImports ARGLIST((void));
void ListDefinedIgnore ARGLIST((void));
void ListFiles ARGLIST((void));
void ListActionSequence ARGLIST((void));
void ListUnmounts ARGLIST((void));
void ListProcesses ARGLIST((void));
void ListACLs ARGLIST((void));
void ListFileEdits ARGLIST((void));
void ListFilters ARGLIST((void));

/* repository.c */

int Repository ARGLIST((char *file, char *repository));

/* rotate.c */

void RotateFiles ARGLIST((char *name, int number));

/* sensible.c */

int SensibleFile ARGLIST((char *nodename, char *path, struct Image *ip));

/* tidy.c */

int RecursiveHomeTidy ARGLIST((char *name, int level,struct stat *sb));
int TidyHomeFile ARGLIST((char *path, char *name,struct stat *statbuf, int level));
int RecursiveTidySpecialArea ARGLIST((char *name, struct Tidy *tp, int maxrecurse, struct stat *sb));
void TidyParticularFile ARGLIST((char *path, char *name, struct Tidy *tp, struct stat *statbuf, int is_dir, int level));
void DoTidyFile ARGLIST((char *path, char *name, struct TidyPattern *tlp, struct stat *statbuf, short int logging_this));
void DeleteTidyList ARGLIST((struct TidyPattern *list));

/* varstring.c */

int TrueVar ARGLIST((char *var));
int IsVarString ARGLIST((char *str));
char ExpandVarstring ARGLIST((char *string, char *buffer, char *bserver));
char ExpandVarbinserv ARGLIST((char *string, char *buffer, char *bserver));
enum vnames ScanVariable ARGLIST((char *name));
struct Item *SplitVarstring ARGLIST((char *varstring, char sep));

/* wildcard.c */

int IsWildCard ARGLIST((char *str));
int WildMatch ARGLIST((char *wildptr,char *cmpptr));
char *AfterSubString ARGLIST((char *big, char *small, int status, char lastwild));

/* wrapper.c */

void TidyWrapper ARGLIST((char *startpath, void *vp));
void RecHomeTidyWrapper ARGLIST((char *startpath, void *vp));
void CheckFileWrapper ARGLIST((char *startpath, void *vp));
void DirectoriesWrapper ARGLIST((char *dir, void *vp));
