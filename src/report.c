/* 

        Copyright (C) 1995-2000
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
/* File: report.c                                                            */
/*                                                                           */
/*****************************************************************************/

#define INET

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

void ListActionSequence()

{ struct Item *ptr;

printf("\nAction sequence = (");

for (ptr=VACTIONSEQ; ptr!=NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   }

printf(")\n");
}

/*******************************************************************/

void ListDefinedClasses()

{ struct Item *ptr;

printf ("\nDefined Classes = ( ");

for (ptr = VHEAP; ptr != NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   }

printf (")\n");

printf ("\nNegated Classes = ( ");

for (ptr = VNEGHEAP; ptr != NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   }

printf (")\n\n");

printf ("Installable classes = ( ");

for (ptr = VALLADDCLASSES; ptr != NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   }

printf (")\n");

if (VEXCLUDECOPY != NULL)
   {
   printf("Patterns to exclude from copies: = (");
   
   for (ptr = VEXCLUDECOPY; ptr != NULL; ptr=ptr->next)
      {
      printf("%s ",ptr->name);
      }

   printf (")\n");
   }

if (VSINGLECOPY != NULL)
   {
   printf("Patterns to copy one time: = (");

   for (ptr = VSINGLECOPY; ptr != NULL; ptr=ptr->next)
      {
      printf("%s ",ptr->name);
      }

   printf (")\n");
   }

if (VAUTODEFINE != NULL)
   {
   printf("Patterns to autodefine: = (");

   for (ptr = VAUTODEFINE; ptr != NULL; ptr=ptr->next)
      {
      printf("%s ",ptr->name);
      }

   printf (")\n");
   }

if (VEXCLUDELINK != NULL)
   {
   printf("Patterns to exclude from links: = (");
   
   for (ptr = VEXCLUDELINK; ptr != NULL; ptr=ptr->next)
      {
      printf("%s ",ptr->name);
      }

   printf (")\n");
   }

if (VCOPYLINKS != NULL)
   {
   printf("Patterns to copy instead of link: = (");
   
   for (ptr = VCOPYLINKS; ptr != NULL; ptr=ptr->next)
      {
      printf("%s ",ptr->name);
      }

   printf (")\n");
   }

if (VCOPYLINKS != NULL)
   {
   printf("Patterns to link instead of copy: = (");
   
   for (ptr = VLINKCOPIES; ptr != NULL; ptr=ptr->next)
      {
      printf("%s ",ptr->name);
      }

   printf (")\n");
   }

}

/*********************************************************************/

void ListDefinedHomePatterns()

{ struct Item *ptr;


printf ("\nDefined wildcards to match home directories = ( ");

for (ptr = VHOMEPATLIST; ptr != NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   }

printf (")\n");
}

/*********************************************************************/

void ListDefinedBinservers()

{ struct Item *ptr;

printf ("\nDefined Binservers = ( ");

for (ptr = VBINSERVERS; ptr != NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   
   if (ptr->classes)
      {
      printf("(pred::%s), ",ptr->classes);
      }
   }

printf (")\n");
}


/*******************************************************************/

void ListDefinedStrategies()

{ struct Strategy *ptr;
  struct Item *ip;
 
printf("\nDEFINED STRATEGIES\n\n");

 for (ptr = VSTRATEGYLIST; ptr != NULL; ptr=ptr->next)
    {
    printf("%s (type=%c)\n",ptr->name,ptr->type);
    if (ptr->strategies)
       {
       for (ip = ptr->strategies; ip !=NULL; ip=ip->next)
          {
          printf("  %s - weight %s\n",ip->name,ip->classes);
          }
       }
    printf("\n");
    } 
}

/*******************************************************************/

void ListDefinedVariables()

{ struct cfObject *cp = NULL;

 printf("\nDEFINED MACRO/VARIABLES (by contexts)\n");
 
 for (cp = VOBJ; cp != NULL; cp=cp->next)
    {
    printf("\nOBJECT: %s\n",cp->scope);
    PrintHashTable(cp->hashtable);
    }
}


/*******************************************************************/

void ListACLs()

{ struct CFACL *ptr;
  struct CFACE *ep;

printf("\nDEFINED ACCESS CONTROL BODIES\n\n");

for (ptr = VACLLIST; ptr != NULL; ptr=ptr->next)
   {
   printf("%s (type=%d,method=%c)\n",ptr->acl_alias,ptr->type,ptr->method);
   
   for (ep = ptr->aces; ep != NULL; ep=ep->next)
      {
      if (ep->name != NULL)
         {
         printf(" Type = %s, obj=%s, mode=%s (classes=%s)\n",ep->acltype,ep->name,ep->mode,ep->classes);
         }
      }
   printf("\n");
   }

}

/*********************************************************************/
/* Promises                                                          */
/*********************************************************************/

void ListDefinedInterfaces()

{ struct Interface *ifp;

 printf ("\nDEFINED INTERFACE PROMISES\n\n");
 
for (ifp = VIFLIST; ifp !=NULL; ifp=ifp->next)
   {
   InterfacePromise(ifp);
   printf("\n");
   }
}

/*********************************************************************/

void InterfacePromise(struct Interface *ifp)

{
printf("Interface %s promise if context is [%s]\n",ifp->ifdev,ifp->classes);
printf("  Body:\n");
printf("     Address ipv4=%s\n",ifp->ipaddress);
printf("     netmask=%s and broadcast=%s\n",ifp->netmask,ifp->broadcast);
}

/*********************************************************************/

void ListDefinedLinks()

{ struct Link *ptr;
  
printf ("\nDEFINED LINK PROMISES\n\n");

for (ptr = VLINK; ptr != NULL; ptr=ptr->next)
   {
   LinkPromise(ptr,"be a link");
   printf("\n");
   }

}

/*********************************************************************/

void ListDefinedLinkchs()

{ struct Link *ptr;
  struct Item *ip;

printf ("\nDEFINED CHILD LINK PROMISES\n\n");

for (ptr = VCHLINK; ptr != NULL; ptr=ptr->next)
   {
   LinkPromise(ptr,"link its children");
   printf("\n");
   }
}

/*********************************************************************/

void LinkPromise(struct Link *ptr, char *type)

{ struct Item *ip;
 
printf("%s will %s to %s\n",ptr->from,type,ptr->to);

printf(" Body force=%c, attr=%d type=%c nofile=%d\n",ptr->force,ptr->silent,ptr->type, ptr->nofile);

printf(" IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

for (ip = ptr->copy; ip != NULL; ip = ip->next)
   {
   printf(" Copy %s\n",ip->name);
   }

for (ip = ptr->exclusions; ip != NULL; ip = ip->next)
   {
   printf(" Exclude %s\n",ip->name);
   }

for (ip = ptr->inclusions; ip != NULL; ip = ip->next)
   {
   printf(" Include %s\n",ip->name);
   }

for (ip = ptr->ignores; ip != NULL; ip = ip->next)
   {
   printf(" Ignore %s\n",ip->name);
   }

for (ip = ptr->filters; ip != NULL; ip = ip->next)
   {
   printf(" Filters %s\n",ip->name);
   }

if (ptr->defines)
   {
   printf(" Define %s\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf(" ElseDefine %s\n",ptr->elsedef);
   }
}

/*********************************************************************/

void PromiseItem(struct Item *ptr)
    
{
printf(" Item: \"%s\" is promised",ptr->name);
printf(" if context matches [%s]\n",ptr->classes);
printf("   ifelapsed %d, expireafter %d\n",ptr->ifelapsed,ptr->expireafter);
}

/*********************************************************************/

void ListDefinedResolvers()

{ struct Item *ptr;

printf ("\nDEFINED RESOLVER CONFIGURATION PROMISES\n\n");

for (ptr = VRESOLVE; ptr != NULL; ptr=ptr->next)
   {
   PromiseItem(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void ListDefinedAlerts()

{ struct Item *ptr;

printf ("\nDEFINED ALERT PROMISES\n\n");

for (ptr = VALERTS; ptr != NULL; ptr=ptr->next)
   {
   PromiseItem(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void ListDefinedHomeservers()

{ struct Item *ptr;

printf ("\nUse home servers = ( ");

for (ptr = VHOMESERVERS; ptr != NULL; ptr=ptr->next)
   {
   PromiseItem(ptr);
   printf("\n");
   }

printf(" )\n");
}

/*********************************************************************/

void ListDefinedImports()

{ struct Item *ptr;

printf ("\nUSE IMPORTS\n\n");

for (ptr = VIMPORT; ptr != NULL; ptr=ptr->next)
   {
   PromiseItem(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void ListDefinedIgnore()

{ struct Item *ptr;

printf ("\nFILE SEARCH IGNORE\n\n");

for (ptr = VIGNORE; ptr != NULL; ptr=ptr->next)
   {
   PromiseItem(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void ListDefinedMethods()

{ struct Method *ptr;

printf ("\nMETHOD AGREEMENTS\n\n");

for (ptr = VMETHODS; ptr != NULL; ptr=ptr->next)
   {
   PromiseMethod(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseMethod(struct Method *ptr)

{ struct Item *ip;
 int i,amserver;

amserver = (IsItemIn(ptr->servers,"localhost")||
            IsItemIn(ptr->servers,IPString2Hostname(VFQNAME)) ||
            IsItemIn(ptr->servers,IPString2UQHostname(VUQNAME)) ||
            IsItemIn(ptr->servers,VIPADDRESS));

if (amserver)
   {
   printf("Promises to provide and execute method %s if context [%s]\n",ptr->name,ptr->classes);
   }
else
   {
   printf("Promise to use voluntary service %s provided by server list if context [%s]\n",ptr->name,ptr->classes);

   i = 1;
   
   for (ip = ptr->send_args; ip != NULL; ip=ip->next)
      {
      printf("   Provide argument %d: %s\n",i++,ip->name);
      }
   
   printf("   %s\n",ChecksumPrint('m',ptr->digest));
   
   i = 1;
   
   for (ip = ptr->send_classes; ip != NULL; ip=ip->next)
      {
      printf("   Provide class %d: %s\n",i++,ip->name);
      }
   
   i = 1;
   
   for (ip = ptr->servers; ip != NULL; ip=ip->next)
      {
      printf("   Encrypt for service provider %d: %s\n",i++,ip->name);
      }

   
   i = 1;
   
   if (ListLen(ptr->servers) > 1)
      {
      for (ip = ptr->return_vars; ip != NULL; ip=ip->next)
         {
         printf("   Return value %d: $(%s_X.%s) - X = 1,2,..\n",i++,ptr->name,ip->name);
         }
      
      i = 1;
      
      for (ip = ptr->return_classes; ip != NULL; ip=ip->next)
         {
         printf("   Will accept return class %d: %s_X_%s\n",i++,ptr->name,ip->name);
         }      
      }
   else
      {      
      for (ip = ptr->return_vars; ip != NULL; ip=ip->next)
         {
         printf("   Will accept return value %d: $(%s.%s)\n",i++,ptr->name,ip->name);
         }
      
      i = 1;
      
      for (ip = ptr->return_classes; ip != NULL; ip=ip->next)
         {
         printf("   Will accept return class %d: %s_%s\n",i++,ptr->name,ip->name);
         }
      }
   
   }

printf("   IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
printf("   Using executable file: %s\n",ptr->file);
printf("   Running with Uid=%d,Gid=%d\n",ptr->uid,ptr->gid);
printf("   Running in chdir=%s, chroot=%s\n",ptr->chdir,ptr->chroot);
}

/*********************************************************************/

void ListDefinedScripts()

{ struct ShellComm *ptr;

printf ("\nPROMISED SHELLCOMMANDS\n\n");

for (ptr = VSCRIPT; ptr != NULL; ptr=ptr->next)
   {
   PromiseShellCommand(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseShellCommand(struct ShellComm *ptr)

{
printf("\nWill execute \"%s\" if context [%s]\n",ptr->name,ptr->classes);

printf(" Behaviour body:\n");
printf("      Timeout=%d\n",ptr->timeout);
printf("      Uid=%d,Gid=%d\n",ptr->uid,ptr->gid);
printf("      Process umask = %o\n",ptr->umask);
printf("      Run in background (no wait) = %c\n",ptr->fork);
printf("      ChDir=%s, ChRoot=%s\n",ptr->chdir,ptr->chroot);
printf("      IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

if (ptr->defines)
   {
   printf(" Define %s on success\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf(" ElseDefine %s\n",ptr->elsedef);
   }
}

/*********************************************************************/

void ListDefinedImages()

{ struct Image *ptr;
  struct Item *svp;
  
printf ("\nDEFINED FILE IMAGE PROMISES\n\n");

for (svp = VSERVERLIST; svp != NULL; svp=svp->next) /* order servers */
   {
   for (ptr = VIMAGE; ptr != NULL; ptr=ptr->next)
      {
      if (strcmp(svp->name,ptr->server) != 0)  /* group together similar hosts so */
         {                                     /* can can do multiple transactions */
         continue;                             /* on one connection */
         } 
      
      PromiseFileCopy(ptr);
      printf("\n");
      }
   }
}

/*********************************************************************/

void PromiseFileCopy(struct Image *ptr)

{ struct UidList *up;
  struct GidList *gp;
  struct Item *iip;

printf("%s promises to be a copy of %s:/%s if context = %s\n",ptr->destination,ptr->server,ptr->path,ptr->classes);
  
printf("   Behaviour body:\n");
printf("         Action on deviation: %s\n",ptr->action);

printf("         Comparison method = %c (time/checksum)\n",ptr->type);

printf("         Ask server = %s (encrypt=%c,verified=%c)\n",ptr->server,ptr->encrypt,ptr->verify);
printf("         Accept the server's public key on trust? %c\n",ptr->trustkey);

printf("         Purge local files if not on server = %c\n",ptr->purge);

printf("         Override mode with: +%o,-%o\n",ptr->plus,ptr->minus);
printf("         Copy if size %c %d\n",ptr->comp,ptr->size);

if (ptr->recurse == CF_INF_RECURSE)
   {
   printf("         File search recursion limit: infinite\n");
   }
else
   {
   printf("         File search recursion limit %d\n",ptr->recurse);
   }

printf("         File search boundary (xdev) = %c\n",ptr->xdev);

printf("         Using uids = ( ");

for (up = ptr->uid; up != NULL; up=up->next)
   {
   printf("%d ",up->uid);
   }

printf(")\n         Using gids = ( ");

for (gp = ptr->gid; gp != NULL; gp=gp->next)
   {
   printf("%d ",gp->gid);
   }

printf(")\n         Using filters:");

for (iip = ptr->filters; iip != NULL; iip=iip->next)
   {
   printf(" %s",iip->name);
   }

printf("\n         Excluding file patterns:");

for (iip = ptr->acl_aliases; iip != NULL; iip=iip->next)
   {
   printf("         Using ACL object %s\n",iip->name);
   }

printf("\n         Ignoring file/directory patterns:");

for (iip = ptr->ignores; iip != NULL; iip = iip->next)
   {
   printf("           %s",iip->name);
   }

printf("\n");
printf("         Using symlink for patterns:");

for (iip = ptr->symlink; iip != NULL; iip = iip->next)
   {
   printf("           %s",iip->name);
   }

printf("\n         Including file patterns:");

for (iip = ptr->inclusions; iip != NULL; iip = iip->next)
   {
   printf("           %s",iip->name);
   }
printf("\n");


if (ptr->defines)
   {
   printf("         Defining %s if deviation corrected\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf("         ElseDefine %s\n",ptr->elsedef);
   }

if (ptr->failover)
   {
   printf("         Providing FailoverClasses %s if server unavailable\n",ptr->failover);
   }

switch (ptr->backup)
   {
   case 'n': printf("         Promise no backup copy\n");
       break;
   case 'y': printf("         Promise single backup archive\n");
       break;
   case 's': printf("         Promise Timestamped backups (full history)\n");
       break;
   default: printf ("         UNKNOWN BACKUP POLICY!!\n");
   }


if (ptr->repository)
   {
   printf("         Using Local repository = %s\n",ptr->repository);
   }

if (ptr->stealth == 'y')
   {
   printf("         Promise stealth copy\n");
   }

if (ptr->preservetimes == 'y')
   {
   printf("         Promise file times preserved\n");
   }

if (ptr->forcedirs == 'y')
   {
   printf("         Promise forcible movement of obstructing files\n");
   }

printf("         IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

}

/*********************************************************************/

void ListDefinedTidy()

{ struct Tidy *ptr;

printf ("\nDEFINED TIDY PROMISES\n\n");

for (ptr = VTIDY; ptr != NULL; ptr=ptr->next)
   {
   PromiseTidy(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseTidy(struct Tidy *ptr)

{ struct TidyPattern *tp;
  struct Item *ip;

if (ptr->maxrecurse == CF_INF_RECURSE)
   {
   printf("Promise to tidy/delete files ");
   }
else
   {
   printf("Promise to tidy/delete an object ");
   }


printf("in base directory: %s\n",ptr->path);

printf(" Constraint body:\n");

for(tp = ptr->tidylist; tp != NULL; tp=tp->next)
   {
   printf("    Use file pattern \"%s\" if context matches [%s]\n",tp->pattern,tp->classes);
   printf("       Use age policy %c-age=%d\n",tp->searchtype,tp->age);
   printf("       If size = %d\n",tp->size);

   if (tp->recurse == CF_INF_RECURSE)
      {
      printf("      Will descending into all child directories\n");
      }
   else
      {
      printf("       Pattern will descend %d levels\n",tp->recurse);
      }

   printf("       Options: Linkdirs=%c, Rmdirs=%c, Travlinks=%c, Compress=%c\n",tp->dirlinks,tp->rmdirs,tp->travlinks,tp->compress);
   
   if (tp->defines)
      {
      printf("       Define \"%s\" on deletion of pattern\n",tp->defines);
      }
   
   if (tp->elsedef)
      {
      printf("       ElseDefine %s\n",tp->elsedef);
      }
   
   for (ip = tp->filters; ip != NULL; ip=ip->next)
      {
      printf("       Use file filter %s\n",ip->name);
      }
   }

printf(" Behaviour body:\n    IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

printf("    Filesystem boundary policy (xdev) = %c\n",ptr->xdev);
   

for (ip = ptr->exclusions; ip != NULL; ip = ip->next)
   {
   printf("    Exclude file patterns %s\n",ip->name);
   }

for (ip = ptr->ignores; ip != NULL; ip = ip->next)
   {
   printf("    Ignore file patterns %s\n",ip->name);
   }

}

/*********************************************************************/

void ListDefinedMountables()

{ struct Mountables *ptr;

printf ("\nPROMISED MOUNTABLES\n\n");

for (ptr = VMOUNTABLES; ptr != NULL; ptr=ptr->next)
   {
   PromiseMountable(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseMountable(struct Mountables *ptr)

{
printf(" Promise to use filesystem %s if context matches [%s]\n",ptr->filesystem,ptr->classes);

if (ptr->readonly)
   {
   printf("  Using option ro\n");
   }
else
   {
   printf("  Using option rw\n");
   }

if (ptr->mountopts != NULL)
   {
   printf("  Using options %s\n", ptr->mountopts);
   }
}

/*********************************************************************/

void ListMiscMounts()

{ struct MiscMount *ptr;

printf ("\nPROMISED MISC MOUNTABLES\n\n");

for (ptr = VMISCMOUNT; ptr != NULL; ptr=ptr->next)
   {
   PromiseMiscMount(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseMiscMount(struct MiscMount *ptr)

{
printf("Promise to mount %s on %s mode if context is [%s]\n",ptr->from,ptr->onto, ptr->classes);
printf(" with mode %s and options %s\n",ptr->mode,ptr->options);
printf(" and behaviour IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
}

/*********************************************************************/

void ListDefinedRequired()

{ struct Disk *ptr;

printf ("\nDEFINED REQUIRE PROMISES\n\n");

for (ptr = VREQUIRED; ptr != NULL; ptr=ptr->next)
   {
   DiskPromises(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void DiskPromises(struct Disk *ptr)

{
printf("Filesystem %s is promised if context matches [%s]\n",ptr->name,ptr->classes);
printf("   Body attributes:\n     freespace=%d, force=%c, define=%s\n",ptr->freespace, ptr->force,ptr->define);
printf("      With behaviour IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
printf("      Using scanarrivals=%c\n",ptr->scanarrivals);
}

/*********************************************************************/

void ListDefinedDisable()

{ struct Disable *ptr;

printf ("\nDEFINED DISABLE PROMISES\n\n");

for (ptr = VDISABLELIST; ptr != NULL; ptr=ptr->next)
   {
   PromiseDisable(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseDisable(struct Disable *ptr)

{
if (strlen(ptr->destination) > 0)
   {
   printf("Promise to rename: %s to %s if context is [%s]\n",ptr->name,ptr->destination,ptr->classes);
   }
else
   {
   printf("Promise to disable/transform object %s if context is [%s]\n",ptr->name,ptr->classes);

   printf("   Constraint body:\n     Rotate=%d, type=%s, size %c %d, action=%c\n",
          ptr->rotate,ptr->type,ptr->comp,ptr->size,ptr->action);
   }

printf("   Behaviour body:\n     IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

if (ptr->repository)
   {
   printf("     Using Local repository = %s\n",ptr->repository);
   }

if (ptr->defines)
   {
   printf("   Define: %s if changes made\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf("   ElseDefine: %s if no changes made\n",ptr->elsedef);
   }
}

/*********************************************************************/

void ListDefinedMakePaths()

{ struct File *ptr;
  
printf ("\nPROMISED DIRECTORIES\n\n");

for (ptr = VMAKEPATH; ptr != NULL; ptr=ptr->next)
   {
   PromiseDirectories(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseDirectories(struct File *ptr)

{ struct UidList *up;
  struct GidList *gp;
  struct Item *ip;
  
printf("Promise attributes of %s if context matches",ptr->path,ptr->classes);
printf("  Body:");

printf("     Mode +%o\n -%o\n",ptr->plus,ptr->minus);

printf(" uids = ( ");

for (up = ptr->uid; up != NULL; up=up->next)
   {
   printf("%d ",up->uid);
   }

printf(")\n gids = ( ");

for (gp = ptr->gid; gp != NULL; gp=gp->next)
   {
   printf("%d ",gp->gid);
   }
printf(")\n\n");

for (ip = ptr->acl_aliases; ip != NULL; ip=ip->next)
   {
   printf(" ACL object %s\n",ip->name);
   }

printf("  Behaviour:\n     IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
printf("     Action on deviation %s\n",FILEACTIONTEXT[ptr->action]);


if (ptr->recurse == CF_INF_RECURSE)
   {
   printf(" Recurse depth infinite\n");
   }
else
   {
   printf(" Recursion depth = %d\n",ptr->recurse);
   }


if (ptr->defines)
   {
   printf(" Define %s on alteration\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf(" ElseDefine %s if no action\n",ptr->elsedef);
   }
}

/*********************************************************************/

void ListFiles()

{ struct File *ptr;

printf ("\nDEFINED FILE PROMISES\n\n");

for (ptr = VFILE; ptr != NULL; ptr=ptr->next)
   {
   PromiseFiles(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseFiles(struct File *ptr)

{ struct Item *ip;
  struct UidList *up;
  struct GidList *gp;

printf("File object %s will promise attributes if context matches [%s]\n",ptr->path,ptr->classes);

printf(" Attribute body:\n  Mode +%o\n -%o\n +%o\n -%o\n",ptr->plus,ptr->minus,ptr->plus_flags,ptr->minus_flags);

printf(" uids = ( ");

for (up = ptr->uid; up != NULL; up=up->next)
   {
   printf("%d ",up->uid);
   }

printf(")\n gids = ( ");

for (gp = ptr->gid; gp != NULL; gp=gp->next)
   {
   printf("%d ",gp->gid);
   }
printf(")\n");


for (ip = ptr->acl_aliases; ip != NULL; ip=ip->next)
   {
   printf(" ACL object %s\n",ip->name);
   }

printf(" Behaviour body:\n");

printf("     Action on deviation: %s\n",FILEACTIONTEXT[ptr->action]);
printf("     Traverse links=%c\n",ptr->travlinks);

printf("     IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

if (ptr->recurse == CF_INF_RECURSE)
   {
   printf("     Search recursion limit=inf\n");
   }
else
   {
   printf("     Search recursion limit=%d\n",ptr->recurse);
   }

printf("     Record checksum-type = %c\n",ptr->checksum);
printf("     Filesystem boundaries (xdev) = %c\n",ptr->xdev);


for (ip = ptr->filters; ip != NULL; ip=ip->next)
   {
   printf("     Using filter %s\n",ip->name);
   }   

for (ip = ptr->exclusions; ip != NULL; ip = ip->next)
   {
   printf("     Excluding file patterns %s\n",ip->name);
   }

for (ip = ptr->inclusions; ip != NULL; ip = ip->next)
   {
   printf("     Including file patterns %s\n",ip->name);
   }

for (ip = ptr->ignores; ip != NULL; ip = ip->next)
   {
   printf("     Ignoring file/directory patterns %s\n",ip->name);
   }

if (ptr->defines)
   {
   printf("     Define %s on convergent change\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf("     ElseDefine %s on no change\n",ptr->elsedef);
   }
}


/*******************************************************************/

void ListUnmounts()

{ struct UnMount *ptr;

printf("\nDEFINED UNMOUNT PROMISES\n\n");

for (ptr=VUNMOUNT; ptr!=NULL; ptr=ptr->next)
   {
   PromiseUnmount(ptr);
   printf("\n");
   }
}

/*******************************************************************/

void PromiseUnmount(struct UnMount *ptr)
    
{
printf("Promise to unmount %s if context matches [%s]\n",ptr->name,ptr->classes);
printf("Behaviour body:\n");
printf("   Deletedir=%c\n   deletefstab=%c\n   force=%c\n",ptr->deletedir,ptr->deletefstab,ptr->force);
printf("   IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
}

/*******************************************************************/

void ListProcesses()

{ struct Process *ptr;

printf("\nPROCESSES PROMISES\n\n");

for (ptr = VPROCLIST; ptr != NULL; ptr=ptr->next)
   {
   PromiseProcess(ptr);
   printf("\n");
   }
}

/*******************************************************************/

void PromiseProcess(struct Process *ptr)
    
{ struct Item *ip; 
  char *sp;

if (ptr->restart == NULL)
   {
   sp = "";
   }
else
   {
   sp = ptr->restart;
   }

printf("\nProcesses matching %s are promised if context is [%s]\n",ptr->expr,ptr->classes);
printf(" Match constraint body:\n");
printf("     Acceptable no. of matches: (%c)%d,",ptr->comp,ptr->matches);

for (ip = ptr->exclusions; ip != NULL; ip = ip->next)
   {
   printf("     Excluding patterns %s\n",ip->name);
   }

for (ip = ptr->inclusions; ip != NULL; ip = ip->next)
   {
   printf("     Including patterns %s\n",ip->name);
   }
   
for (ip = ptr->filters; ip != NULL; ip = ip->next)
   {
   printf("     Using filter %s\n",ip->name);
   }

printf(" Behaviour body:\n");

printf("     Matches will receive signal=%s\n action=%c\n",SIGNALS[ptr->signal],ptr->action);
printf("     Will be (re)started with command = %s\n",sp);
printf("     IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

printf(" Started process constraint body:\n");

printf("     Useshell=%c\n",ptr->useshell);
printf("     ChDir=%s\n",ptr->chdir);
printf("     ChRoot=%s\n",ptr->chroot);

if (ptr->defines)
   {
   printf("     Define %s if matches found\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf("     ElseDefine %s if no matches found\n",ptr->elsedef);
   }
}

/*******************************************************************/

void ListFileEdits()

{ struct Edit *ptr;

printf("\nDEFINED FILE EDIT PROMISES\n\n");

for (ptr=VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   PromiseFileEdits(ptr);
   printf("\n");
   }
}

/*******************************************************************/

void PromiseFileEdits(struct Edit *ptr)

{ struct Edlist *ep;
  struct Item *ip;

printf("Edit promises for %s\n",ptr->fname);

printf(" Behaviour body:\n");
printf("   IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);
printf("   File search recursion limit: %d\n",ptr->recurse);

if (ptr->repository)
   {
   printf("   Using local repository = %s\n",ptr->repository);
   }

for (ip = ptr->filters; ip != NULL; ip=ip->next)
   {
   printf ("   Using filter %s\n",ip->name);
   }

printf(" Body of convergent operations:\n");

for (ep = ptr->actions; ep != NULL; ep=ep->next)
   {
   if (ep->data == NULL)
      {
      printf("   Promise %s with body [nodata] if (%s)\n",VEDITNAMES[ep->code],ep->classes);
      }
   else
      {
      printf("   Promise %s with body [%s] if (%s)\n",VEDITNAMES[ep->code],ep->data,ep->classes);
      }
   }
}

/*******************************************************************/

void ListFilters()

{ struct Filter *ptr;
  int i;

printf("\nDEFINED FILTERS\n");

for (ptr=VFILTERLIST; ptr != NULL; ptr=ptr->next)
   {
   printf("\n%s :\n",ptr->alias);

   if (ptr->defines)
      {
      printf(" Defines: %s\n",ptr->defines);
      }

   if (ptr->elsedef)
      {
      printf(" ElseDefines: %s\n",ptr->elsedef);
      }
   
   for (i = 0; i < NoFilter; i++)
      {
      if (ptr->criteria[i] != NULL)
         {
         printf(" (%s) [%s]\n",VFILTERNAMES[i],ptr->criteria[i]);
         }
      }
   }
}


/*******************************************************************/

void ListDefinedPackages()

{ struct Package *ptr = NULL;

 printf("\nPROMISED PACKAGE CHECKS\n\n");

 for (ptr = VPKG; ptr != NULL; ptr = ptr->next)
    {
    PromisePackages(ptr);
    printf("\n");
    }
}

/*******************************************************************/

void PromisePackages(struct Package *ptr)

{
printf("Package %s promises\n", ptr->name);

printf(" Constraint body:\n");

if (ptr->ver && *(ptr->ver) != '\0')
   {
   printf("   Will have version %s %s\n",CMPSENSETEXT[ptr->cmp], ptr->ver);
   }
else
   {
   printf("   Can have any package version.\n");
   }

printf(" Behaviour body:\n");

if (ptr->action != pkgaction_none)
   {
   printf("   Promise to %s package\n",PKGACTIONTEXT[ptr->action]);
   }

printf("   Using Package database: %s\n", PKGMGRTEXT[ptr->pkgmgr]);
printf("   IfElapsed=%d, ExpireAfter=%d\n",ptr->ifelapsed,ptr->expireafter);

if (ptr->defines)
   {
   printf("   Define %s if matches\n",ptr->defines);
   }

if (ptr->elsedef)
   {
   printf("   ElseDefine %s if no match\n",ptr->elsedef);
   }
}
