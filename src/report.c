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

void ListDefinedInterfaces()

{ struct Interface *ifp;

 printf ("\nDEFINED INTERFACES\n\n");
 
for (ifp = VIFLIST; ifp !=NULL; ifp=ifp->next)
   {
   printf("Interface %s, netmask=%s, broadcast=%s\n",ifp->ifdev,ifp->netmask,ifp->broadcast);
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
      printf("(if defined %s), ",ptr->classes);
      }
   }

printf (")\n");
}

/*********************************************************************/

void ListDefinedLinks()

{ struct Link *ptr;
  struct Item *ip;
  
printf ("\nDEFINED LINKS\n\n");

for (ptr = VLINK; ptr != NULL; ptr=ptr->next)
   {
   printf("\nFrom %s -> %s force=%c, attr=%d type=%c nofile=%d\n",ptr->from,ptr->to,ptr->force,ptr->silent,ptr->type, ptr->nofile);
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
}

/*********************************************************************/

void ListDefinedLinkchs()

{ struct Link *ptr;
  struct Item *ip;

printf ("\nDEFINED CHILD LINKS\n\n");

for (ptr = VCHLINK; ptr != NULL; ptr=ptr->next)
   {
   printf("\n%s->%s force=%c attr=%d, rec=%d\n",ptr->from,ptr->to,
	  ptr->force,ptr->silent,ptr->recurse);
   
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

/*********************************************************************/

void ListDefinedResolvers()

{ struct Item *ptr;

printf ("\nDEFINED NAMESERVERS\n\n");

for (ptr = VRESOLVE; ptr != NULL; ptr=ptr->next)
   {
   printf("%s (%s)\n",ptr->name,ptr->classes);
   }
}

/*********************************************************************/

void ListDefinedAlerts()

{ struct Item *ptr;

printf ("\nDEFINED ALERTS\n\n");

for (ptr = VALERTS; ptr != NULL; ptr=ptr->next)
   {
   printf("%s: [%s]\n",ptr->classes,ptr->name);
   }
}

/*********************************************************************/

void ListDefinedScripts()

{ struct ShellComm *ptr;

printf ("\nDEFINED SHELLCOMMANDS\n\n");

for (ptr = VSCRIPT; ptr != NULL; ptr=ptr->next)
   {
   printf("\n%s\n timeout=%d\n uid=%d,gid=%d\n",ptr->name,ptr->timeout,ptr->uid,ptr->gid);
   printf(" umask = %o, background = %c\n",ptr->umask,ptr->fork);
   printf (" ChDir=%s, ChRoot=%s\n",ptr->chdir,ptr->chroot);
   
   if (ptr->defines)
      {
      printf(" Define %s\n",ptr->defines);
      }

   if (ptr->elsedef)
      {
      printf(" ElseDefine %s\n",ptr->elsedef);
      }

   if (ptr->classes)
      {
      printf(" Classes %s\n",ptr->classes);
      }
   }
}

/*********************************************************************/

void ListDefinedImages()

{ struct Image *ptr;
  struct UidList *up;
  struct GidList *gp;
  struct Item *iip, *svp;
  struct sockaddr_in raddr;
  
printf ("\nDEFINED FILE IMAGES\n\n");

for (svp = VSERVERLIST; svp != NULL; svp=svp->next) /* order servers */
   {
   for (ptr = VIMAGE; ptr != NULL; ptr=ptr->next)
      {
      if (strcmp(svp->name,ptr->server) != 0)  /* group together similar hosts so */
	 {                                     /* can can do multiple transactions */
	 continue;                             /* on one connection */
	 } 
      
      printf("\n%s\n +%o\n -%o\n dest: %s\n action: %s\n",ptr->path,ptr->plus,ptr->minus,
	     ptr->destination,ptr->action);

      printf(" Size %c %d\n",ptr->comp,ptr->size);
      
      if (ptr->recurse == INFINITERECURSE)
	 {
	 printf(" recurse=inf\n");
	 }
      else
	 {
	 printf(" recurse=%d\n",ptr->recurse);
	 }
      
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
      
      printf(")\n filters:");
      
      for (iip = ptr->filters; iip != NULL; iip=iip->next)
	 {
	 printf(" %s",iip->name);
	 }
      
      printf("\n exclude:");
      
      for (iip = ptr->acl_aliases; iip != NULL; iip=iip->next)
	 {
	 printf(" ACL object %s\n",iip->name);
	 }
      
      printf("\n ignore:");
      
      for (iip = ptr->ignores; iip != NULL; iip = iip->next)
	 {
	 printf(" %s",iip->name);
	 }
      
      printf("\n");
      printf(" symlink:");
      
      for (iip = ptr->symlink; iip != NULL; iip = iip->next)
	 {
	 printf(" %s",iip->name);
	 }
      
      printf("\n include:");
      
      for (iip = ptr->inclusions; iip != NULL; iip = iip->next)
	 {
	 printf(" %s",iip->name);
	 }
      printf("\n");
      
      printf(" classes = %s\n",ptr->classes);
      
      printf(" method = %c (time/checksum)\n",ptr->type);
      
      printf(" server = %s (encrypt=%c,verified=%c)\n",ptr->server,ptr->encrypt,ptr->verify);
      printf(" accept the server's public key on trust? %c\n",ptr->trustkey);
      
      printf(" purging = %c\n",ptr->purge);
      
      if (ptr->dns != NULL)
	 {
	 raddr.sin_addr.s_addr = (ptr->dns)->s_addr;
	 printf(" host entry cache = %d = %s\n",ptr->dns,inet_ntoa(raddr.sin_addr));
	 }
      
      if (ptr->defines)
	 {
	 printf(" Define %s\n",ptr->defines);
	 }
      
      if (ptr->elsedef)
	 {
	 printf(" ElseDefine %s\n",ptr->elsedef);
	 }

      if (ptr->failover)
	 {
	 printf(" FailoverClasses %s\n",ptr->failover);
	 }

      if (ptr->backup == 'n')
	 {
	 printf(" NOT BACKED UP\n");
	 }

      if (ptr->repository)
	 {
	 printf(" Local repository = %s\n",ptr->repository);
	 }
      
      if (ptr->stealth == 'y')
	 {
	 printf(" Stealth copy\n");
	 }

      if (ptr->preservetimes == 'y')
	 {
	 printf(" File times preserved\n");
	 }

      if (ptr->forcedirs == 'y')
	 {
	 printf(" Forcible movement of obstructing files in recursion\n");
	 }

      }
   }
}

/*********************************************************************/

void ListDefinedTidy()

{ struct Tidy *ptr;
  struct TidyPattern *tp;
  struct Item *ip;

printf ("\nDEFINED TIDY MASKS\n\n");

for (ptr = VTIDY; ptr != NULL; ptr=ptr->next)
   {
   if (ptr->maxrecurse == INFINITERECURSE)
      {
      printf("%s (maxrecurse = inf)\n",ptr->path);
      }
   else
      {
      printf("%s (maxrecurse = %d)\n",ptr->path,ptr->maxrecurse);
      }
   
   for (ip = ptr->exclusions; ip != NULL; ip = ip->next)
      {
      printf(" Exclude %s\n",ip->name);
      }
   
   for (ip = ptr->ignores; ip != NULL; ip = ip->next)
      {
      printf(" Ignore %s\n",ip->name);
      }
      
   for(tp = ptr->tidylist; tp != NULL; tp=tp->next)
      {
      printf("    pat=%s, %c-age=%d, size=%d, linkdirs=%c, rmdirs=%c, travlinks=%c compress=%c\n",
	     tp->pattern,tp->searchtype,tp->age,tp->size,tp->dirlinks,tp->rmdirs,tp->travlinks,tp->compress);
      
      if (tp->defines)
	 {
	 printf("       Define %s\n",tp->defines);
	 }
      
      if (tp->elsedef)
	 {
	 printf("       ElseDefine %s\n",tp->elsedef);
	 }
      
      for (ip = tp->filters; ip != NULL; ip=ip->next)
	 {
	 printf(" Filter %s\n",ip->name);
	 }   
      
      if (tp->recurse == INFINITERECURSE)
	 {
	 printf("       recurse=inf\n");
	 }
      else
	 {
	 printf("       recurse=%d\n",tp->recurse);
	 }
      }
   }
}

/*********************************************************************/

void ListDefinedMountables()

{ struct Mountables *ptr;

printf ("\nDEFINED MOUNTABLES\n\n");

for (ptr = VMOUNTABLES; ptr != NULL; ptr=ptr->next)
   {
   /* HvB: Bas van der Vlies */
   printf("%s ",ptr->filesystem);

   if ( ptr->readonly )
      printf(" ro\n");
   else
      printf(" rw\n");

   if ( ptr->mountopts != NULL )
      printf("\t %s\n", ptr->mountopts );
   }
}

/*********************************************************************/

void ListMiscMounts()

{ struct MiscMount *ptr;

printf ("\nDEFINED MISC MOUNTABLES\n\n");

for (ptr = VMISCMOUNT; ptr != NULL; ptr=ptr->next)
   {
   printf("%s on %s (%s)\n",ptr->from,ptr->onto,ptr->options);
   }
}

/*********************************************************************/

void ListDefinedRequired()

{ struct Disk *ptr;

printf ("\nDEFINED REQUIRE\n\n");

for (ptr = VREQUIRED; ptr != NULL; ptr=ptr->next)
   {
   /* HvB : Bas van der Vlies */
   printf("%s, freespace=%d, force=%c, define=%s\n",
	ptr->name,ptr->freespace, ptr->force,ptr->define);
   }
}

/*********************************************************************/

void ListDefinedHomeservers()

{ struct Item *ptr;

printf ("\nDefined home servers = ( ");

for (ptr = VHOMESERVERS; ptr != NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
      
   if (ptr->classes)
      {
      printf("(if defined %s), ",ptr->classes);
      }

   }

printf (")\n");
}

/*********************************************************************/

void ListDefinedDisable()

{ struct Disable *ptr;

printf ("\nDEFINED DISABLE\n\n");

for (ptr = VDISABLELIST; ptr != NULL; ptr=ptr->next)
   {
   printf("\n %s:\n rotate=%d, type=%s, size%c%d action=%c\n",
	  ptr->name,ptr->rotate,ptr->type,ptr->comp,ptr->size,ptr->action);

   if (ptr->repository)
      {
      printf(" Local repository = %s\n",ptr->repository);
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
}

/*********************************************************************/

void ListDefinedMakePaths()

{ struct File *ptr;
  struct UidList *up;
  struct GidList *gp;
  struct Item *ip;
  
printf ("\nDEFINED DIRECTORIES\n\n");

for (ptr = VMAKEPATH; ptr != NULL; ptr=ptr->next)
   {
   printf("%s\n +%o\n -%o\n %s\n",ptr->path,ptr->plus,ptr->minus,FILEACTIONTEXT[ptr->action]);

   if (ptr->recurse == INFINITERECURSE)
      {
      printf(" recurse=inf\n");
      }
   else
      {
      printf(" recurse=%d\n",ptr->recurse);
      }
   
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

/*********************************************************************/

void ListDefinedImports()

{ struct Item *ptr;

printf ("\nDEFINED IMPORTS\n\n");

for (ptr = VIMPORT; ptr != NULL; ptr=ptr->next)
   {
   printf("%s\n",ptr->name);
   }
}

/*********************************************************************/

void ListDefinedIgnore()

{ struct Item *ptr;

printf ("\nDEFINED IGNORE\n\n");

for (ptr = VIGNORE; ptr != NULL; ptr=ptr->next)
   {
   printf("%s\n",ptr->name);
   }
}

/*********************************************************************/

void ListFiles()

{ struct File *ptr;
  struct Item *ip;
  struct UidList *up;
  struct GidList *gp;

printf ("\nDEFINED FILES\n\n");

for (ptr = VFILE; ptr != NULL; ptr=ptr->next)
   {
   printf("\n%s\n +%o\n -%o\n +%o\n -%o\n %s\n travelinks=%c\n",
	  ptr->path,ptr->plus,ptr->minus,ptr->plus_flags,ptr->minus_flags,
	  FILEACTIONTEXT[ptr->action],ptr->travlinks);
   
   if (ptr->recurse == INFINITERECURSE)
      {
      printf(" recurse=inf\n");
      }
   else
      {
      printf(" recurse=%d\n",ptr->recurse);
      }
   
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

   for (ip = ptr->filters; ip != NULL; ip=ip->next)
      {
      printf(" Filter %s\n",ip->name);
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

void ListUnmounts()

{ struct UnMount *ptr;

printf("\nDEFINED UNMOUNTS\n\n");

for (ptr=VUNMOUNT; ptr!=NULL; ptr=ptr->next)
   {
   printf("%s (classes=%s) deletedir=%c deletefstab=%c force=%c\n",ptr->name,ptr->classes,ptr->deletedir,ptr->deletefstab,ptr->force);
   printf(" Context scope: %s\n",ptr->scope);
   }
}

/*******************************************************************/

void ListProcesses()

{ struct Process *ptr;
  struct Item *ip; 
  char *sp;

printf("\nDEFINED PROCESSES\n\n");

for (ptr = VPROCLIST; ptr != NULL; ptr=ptr->next)
   {
   if (ptr->restart == NULL)
      {
      sp = "";
      }
   else
      {
      sp = ptr->restart;
      }
   
   printf("\n%s\n Restart = %s (useshell=%c)\n matches: %c%d\n signal=%s\n action=%c\n",
	  ptr->expr,sp,ptr->useshell,ptr->comp,ptr->matches,SIGNALS[ptr->signal],ptr->action);

   printf (" ChDir=%s, ChRoot=%s\n",ptr->chdir,ptr->chroot);
   
   if (ptr->defines)
      {
      printf(" Define %s\n",ptr->defines);
      }

   if (ptr->elsedef)
      {
      printf(" ElseDefine %s\n",ptr->elsedef);
      }

   for (ip = ptr->exclusions; ip != NULL; ip = ip->next)
      {
      printf(" Exclude %s\n",ip->name);
      }

   for (ip = ptr->inclusions; ip != NULL; ip = ip->next)
      {
      printf(" Include %s\n",ip->name);
      }
   
   for (ip = ptr->filters; ip != NULL; ip = ip->next)
      {
      printf(" Filter %s\n",ip->name);
      }
   }

printf(")\n");
}

/*******************************************************************/

void ListACLs()

{ struct CFACL *ptr;
  struct CFACE *ep;

printf("\nDEFINED ACCESS CONTROL LISTS\n\n");

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

void ListFileEdits()

{ struct Edit *ptr;
  struct Edlist *ep;

printf("\nDEFINED FILE EDITS\n\n");

for (ptr=VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   printf("%s (%c)(r=%d)\n",ptr->fname,ptr->done,ptr->recurse);
   printf(" Context scope: %s\n",ptr->scope);
   
   if (ptr->repository)
      {
      printf(" Local repository = %s\n",ptr->repository);
      }
   
   for (ep = ptr->actions; ep != NULL; ep=ep->next)
      {
      if (ep->data == NULL)
         {
         printf(" %s [nodata]\n",VEDITNAMES[ep->code]);
         }
      else
         {
         printf(" %s [%s]\n",VEDITNAMES[ep->code],ep->data);
         }
      }
   printf("\n");
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
