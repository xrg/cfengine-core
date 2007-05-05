/* cfengine for GNU
 
        Copyright (C) 2003
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
 

#include "cf.defs.h"
#include "cf.extern.h"

/****************************************************************************/

/* Local prototypes that nobody else should care about... */

void ParseEVR(char * evr, const char **ep, const char **vp, const char **rp);
void ParseSUNVR(char * vr, int *major, int *minor, int *micro);
void ParseAIXVR(char * vr, int *ver, int *release, int *maint, int *fix);
int rpmvercmp(const char *a, const char *b);
int xislower(int c);
int xisupper(int c);
int xisalpha(int c);
int xisalnum(int c);
int xisdigit(int c);

/****************************************************************************/

/* Make sure we are insensitive to the locale so everyone gets the
 * same experience from these. */

int xislower(int c)
{
 return (c >= 'a' && c <= 'z');
}

int xisupper(int c)
{
 return (c >= 'A' && c <= 'Z');
}

int xisalpha(int c)
{
return (xislower(c) || xisupper(c));
}

int xisdigit(int c)
{
 return (c >= '0' && c <= '9');
}

int xisalnum(int c)
{
 return (xisalpha(c) || xisdigit(c));
}

/*********************************************************************/
/* RPM */
/* returns: 1 - found a match
 *          0 - found no match
 */

/*********************************************************************/

int RPMPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  struct Item *evrlist = NULL;
  struct Item *evr;
  int epochA = 0; /* What is installed.  Assume 0 if we don't get one. */
  int epochB = 0; /* What we are looking for.  Assume 0 if we don't get one. */
  const char *eA = NULL; /* What is installed */
  const char *eB = NULL; /* What we are looking for */
  const char *vA = NULL; /* What is installed */
  const char *vB = NULL; /* What we are looking for */
  const char *rA = NULL; /* What is installed */
  const char *rB = NULL; /* What we are looking for */
  enum cmpsense result;
  int match = 0;

if (GetMacroValue(CONTEXTID,"RPMcommand"))
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s -q --queryformat \"%%{EPOCH}:%%{VERSION}-%%{RELEASE}\\n\" %s",GetMacroValue(CONTEXTID,"RPMcommand"),package);
   }
else
   {
   snprintf(VBUFF,CF_BUFSIZE,"/bin/rpm -q --queryformat \"%%{EPOCH}:%%{VERSION}-%%{RELEASE}\\n\" %s", package);
   }

if ((pp = cfpopen(VBUFF, "r")) == NULL)
  {
  Verbose("Could not execute the RPM command.  Assuming package not installed.\n");
  return 0;
  }

 while(!feof(pp))
    {
    *VBUFF = '\0';

    ReadLine(VBUFF,CF_BUFSIZE,pp);

    if (*VBUFF != '\0')
       {
       AppendItem(&evrlist,VBUFF,"");
       }
    }
 
/* Non-zero exit status means that we could not find the package, so
 * Zero the list and bail. */

 if (cfpclose(pp) != 0)
    {
    DeleteItemList(evrlist);
    evrlist = NULL;
    }
 
 if (evrlist == NULL)
    {
    Verbose("RPM Package %s not installed.\n", package);
    return 0;
    }
 

Verbose("RPMCheckPackage(): Requested %s %s %s\n", package, CMPSENSETEXT[cmp],(version[0] ? version : "ANY"));

/* If no version was specified, just return 1, because if we got this far
 * some package by that name exists. */

if (!version[0])
  {
  DeleteItemList(evrlist);
  return 1;
  }

/* Parse the EVR we are looking for once at the start */
 
ParseEVR(version, &eB, &vB, &rB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

 for (evr = evrlist; evr != NULL; evr=evr->next)
   {
   char *evrstart;
   evrstart = evr->name;

   /* Start out assuming the comparison will be equal. */
   result = cmpsense_eq;

   /* RPM returns the string "(none)" for the epoch if there is none
    * instead of the number 0.  This will cause ParseEVR() to misinterpret
    * it as part of the version component, since epochs must be numeric.
    * If we get "(none)" at the start of the EVR string, we must be careful
    * to replace it with a 0 and reset evrstart to where the 0 is.  Ugh.
    */

   if (!strncmp(evrstart, "(none)", strlen("(none)")))
     {
     /* We have no EVR in the installed package.  Fudge it. */
     evrstart = strchr(evrstart, ':') - 1;
     *evrstart = '0';
     }

   Verbose("RPMCheckPackage(): Trying installed version %s\n", evrstart);
   ParseEVR(evrstart, &eA, &vA, &rA);

   /* Get the epochs at ints */
   epochA = atol(eA);   /* The above code makes sure we always have this. */

   if (eB && *eB) /* the B side is what the user entered.  Better check. */
     {
     epochB = atol(eB);
     }

   /* First try the epoch. */

   if (epochA > epochB)
     {
     result = cmpsense_gt;
     }

   if (epochA < epochB)
     {
     result = cmpsense_lt;
     }

   /* If that did not decide it, try version.  We must *always* have
    * a version string.  That's just the way it is.*/

   if (result == cmpsense_eq)
     {
     switch (rpmvercmp(vA, vB))
       {
       case 1:    result = cmpsense_gt;
                  break;
       case -1:   result = cmpsense_lt;
                  break;
       }
     }

   /* if we wind up here, everything rides on the release if both have it.
    * RPM always stores a release internally in the database, so the A side
    * will have it.  It's just a matter of whether or not the user cares
    * about it at this point. */

   if ((result == cmpsense_eq) && (rB && *rB))
      {
      switch (rpmvercmp(rA, rB))
         {
         case 1:  result = cmpsense_gt;
             break;
         case -1: result = cmpsense_lt;
             break;
         }
      }

   Verbose("Comparison result: %s\n",CMPSENSETEXT[result]);
   
   switch(cmp)
      {
      case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
      }
   
   /* If we find a match, just return it now, and don't bother checking
    * anything else RPM returned, if it returns multiple packages */
   
   if (match)
      {
      DeleteItemList(evrlist);
      return 1;
      }
   }
 
/* If we manage to make it out of the loop, we did not find a match. */

DeleteItemList(evrlist);
return 0;
}

/*********************************************************************************/

int InstallPackage(char *name, enum pkgmgrs pkgmgr)

{ char rawinstcmd[CF_BUFSIZE];
 /* Make the instcmd twice the normal buffer size since the package list
    limit is CF_BUFSIZE so this can obviously get larger! */
 char instcmd[CF_BUFSIZE*2];
 char line[CF_BUFSIZE];
 char *percent;
 FILE *pp;
 char *ptr, *arg_ptr, *next_ptr;
 int instcmd_args, package_args;
 int instcmd_tail_len = 0;
 int instcmd_len, arg_len;

if (DONTDO)
   {
   Verbose("Need to install package %s\n",name);
   return 0;
   }

/* Determine the command to use for the install. */
switch(pkgmgr)
   {
   /* RPM */
   case pkgmgr_rpm:

       if (!GetMacroValue(CONTEXTID,"RPMInstallCommand"))
          {
          Verbose("RPMInstallCommand NOT Set.  Package Installation Not Possible!\n");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"RPMInstallCommand"),CF_BUFSIZE);
       break;
       
       /* Debian */
   case pkgmgr_dpkg:

       if (!GetMacroValue(CONTEXTID,"DPKGInstallCommand"))
          {
          Verbose("DPKGInstallCommand NOT Set.  Package Installation Not Possible!\n");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"DPKGInstallCommand"),CF_BUFSIZE);
       break;
       
       /* Solaris */
   case pkgmgr_sun:

       if (!GetMacroValue(CONTEXTID,"SUNInstallCommand"))
          {
          Verbose("SUNInstallCommand NOT Set.  Package Installation Not Possible!\n");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"SUNInstallCommand"),CF_BUFSIZE);
       break;
       
       /* AIX */
   case pkgmgr_aix:
	if (!GetMacroValue(CONTEXTID,"AIXInstallCommand"))
          {
          Verbose("AIXInstallCommand NOT Set.  Package Installation Not Possible!\n");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"AIXInstallCommand"),CF_BUFSIZE);
	
       break;

       /* Portage */
   case pkgmgr_portage:
       
       if (!GetMacroValue(CONTEXTID,"PortageInstallCommand"))
          {
          Verbose("PortageInstallCommand NOT Set.  Package Installation Not Possible!\n");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"PortageInstallCommand"),CF_BUFSIZE);
       break;
       
       /* FreeBSD */
   case pkgmgr_freebsd:
       
       if (!GetMacroValue(CONTEXTID,"FreeBSDInstallCommand"))
          {
          Verbose("FreeBSDInstallCommand NOT Set.  Package Installation Not Possible!\n");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"FreeBSDInstallCommand"),CF_BUFSIZE);
       break;
       
       /* Default */
   default:
       Verbose("InstallPackage(): Unknown package manager %d\n",pkgmgr);
       break;
   }

/* Common to all pkg managers */

    /* How many words are there in the package manager invocation? */
for (ptr = rawinstcmd, instcmd_args = 1; NULL != ptr; ptr = strchr(++ptr, ' '))
   {
   ++instcmd_args;
   }

/* This could probably be a bit more complete, but I don't think
   that anyone would want to expand the package name more than
   once in a single command invocation anyhow. */

if (percent = strstr(rawinstcmd, "%s"))
   {
   *percent = '\0';
   percent += 2;
   instcmd_tail_len = strlen(percent);
   --instcmd_args;
   }

instcmd_len = strlen(rawinstcmd);

/* Copy the initial part of the install command */
strncpy(instcmd, rawinstcmd, CF_BUFSIZE*2);

snprintf(OUTPUT,CF_BUFSIZE,"Installing package(s) %s using %s\n", name, instcmd);
CfLog(cfinform,OUTPUT,"");

if ((pp = cfpopen(instcmd, "r")) == NULL)
   {
   Verbose("Could not execute package install command\n");
   /* Return that the package is still not installed */
   return 0;
   }

  /* Loop over packages until we reach the maximum number that cfpopen
     can take */

ptr = name;

while (NULL != ptr)
   {
   arg_ptr = &instcmd[instcmd_len];
   
   for (package_args = instcmd_args;(package_args < CF_MAXSHELLARGS) && (NULL != ptr);package_args++)
      {
      next_ptr = strchr(ptr, ' ');
      if (next_ptr)
         {
         *next_ptr = '\0';
         }
      
      arg_len = strlen(ptr);
      *arg_ptr++ = ' ';
      strncpy(arg_ptr, ptr, &instcmd[CF_BUFSIZE*2] - arg_ptr);
      arg_ptr += arg_len;
      
      if (next_ptr)
         {
         *next_ptr++ = ' ';
         }
      
      ptr = next_ptr;
      }
   
   if (arg_ptr != &instcmd[instcmd_len])
      {      
      /* Have a full command line, so append the tail, if necessary,
         and run it */
      
      if (instcmd_tail_len > 0)
         {
         strncpy(arg_ptr, percent, &instcmd[CF_BUFSIZE*2] - arg_ptr);
         }
      
      Verbose("Installing package group using %s\n", instcmd);
      
      if ((pp = cfpopen(instcmd, "r")) == NULL)
         {
         Verbose("Could not execute package install command\n");
         /* Return that the package is still not installed */
         return 0;
         }
      
      while (!feof(pp))
         {
         ReadLine(line,CF_BUFSIZE-1,pp);
         snprintf(OUTPUT,CF_BUFSIZE,"Package install: %s\n",line);
         CfLog(cfinform,OUTPUT,""); 
         }
      
      if (cfpclose(pp) != 0)
         {
         Verbose("Package install command was not successful\n");
         return 0;
         }
      }
   }

return 1;
}

/*********************************************************************/

int RemovePackage(char *name, enum pkgmgrs pkgmgr)

{
Verbose("Package removal not yet implemented");
return 1;
}

/*********************************************************************/
/* Debian                                                            */
/*********************************************************************/

int DPKGPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
 struct Item *evrlist = NULL;
 struct Item *evr;
 char *evrstart;
 enum cmpsense result;
 int match = 0;
 char tmpBUFF[CF_BUFSIZE];
 
Verbose ("Package: ");
Verbose (package);
Verbose ("\n");

/* check that the package exists in the package database */
snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/apt-cache policy %s 2>&1 | grep -v " \
          "\"W: Unable to locate package \"", package);

if ((pp = cfpopen_sh(VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute APT-command (apt-cache).\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("The package %s did not exist in the package database.\n",package);
   return 0;
   }

/* check what version is installed on the system (if any) */
snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/apt-cache policy %s", package);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute APT-command (apt-cache policy).\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      if (sscanf (VBUFF, "  Installed: %s", tmpBUFF) > 0)
         {
         AppendItem (&evrlist, tmpBUFF, "");
         }
      }
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("Something impossible happened... ('grep' exited abnormally).\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Assume that there is only one package in the list */
evr = evrlist;

if (evr == NULL)
   {
   /* We did not find a match, and returns */
   DeleteItemList (evrlist);
   return 0;
   }

evrstart = evr->name;

/* if version value is "(null)", the packages was not installed
   -> the package has no version and dpkg --compare-versions will
   treat "" as "no version" */

if (strncmp (evrstart, "(none)", strlen ("(none)")) == 0)
   {
   sprintf (evrstart, "\"\"");

   /* RB 34.02.06:
   *   Set compare result to nothing when (not)installed version is none
   *   because else we might return True for package check
   *   if checking without version/cmp statement.
   *
   *   Or else it will cause us to assume package is installed
   *   while it is actually not.
   */

   result = cmpsense_none;
   }
   else
     {

     /* start with assuming that the versions are equal */

     result = cmpsense_eq;
     }

if (strncmp (version, "(none)", strlen ("(none)")) == 0)
   {
   sprintf (version, "\"\"");
   }

/* the evrstart shall be a version number which we will
   compare to version using '/usr/bin/dpkg --compare-versions' */

/* check if installed version is gt version */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/dpkg --compare-versions %s gt %s", evrstart, version);
  
if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute DPKG-command.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

/* if dpkg --compare-versions exits with zero result the condition
   was satisfied, else not satisfied */
if (cfpclose (pp) == 0)
   {
   result = cmpsense_gt;
   }    

/* check if installed version is lt version */
snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/dpkg --compare-versions %s lt %s", evrstart, version);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute DPKG-command.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

/* if dpkg --compare-versions exits with zero result the condition
   was satisfied, else not satisfied */

if (cfpclose (pp) == 0)
   {
   result = cmpsense_lt;
   }    
  
Verbose ("Comparison result: %s\n", CMPSENSETEXT[result]);
  
switch (cmp)
   {
   case cmpsense_gt:
       match = (result == cmpsense_gt);
       break;
   case cmpsense_ge:
       match = (result == cmpsense_gt || result == cmpsense_eq);
       break;
   case cmpsense_lt:
       match = (result == cmpsense_lt);
       break;
   case cmpsense_le:
       match = (result == cmpsense_lt || result == cmpsense_eq);
       break;
   case cmpsense_eq:
       match = (result == cmpsense_eq);
       break;
   case cmpsense_ne:
       match = (result != cmpsense_eq);
       break;
   }

if (match)
   {
   DeleteItemList (evrlist);
   return 1;
   }

/* RB 24.03.06: Why do we need an extra blank line here?: ugly
 * 
 * Verbose("\n");
 */

/* if we manage to make it here, we did not find a match */
DeleteItemList (evrlist);
return 0;
}

/*********************************************************************/
/* Sun - pkginfo/pkgadd/pkgrm                                        */
/*********************************************************************/

int SUNPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  struct Item *evrlist = NULL;
  struct Item *evr;
  char *evrstart;
  enum cmpsense result;
  int match = 0;
  char tmpBUFF[CF_BUFSIZE];
  int majorA = 0;
  int majorB = 0;
  int minorA = 0;
  int minorB = 0;
  int microA = 0;
  int microB = 0;

Verbose ("Package: ");
Verbose (package);
Verbose ("\n");

/* check that the package exists in the package database */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/pkginfo -i -q %s", package);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute pkginfo -i -q.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("The package %s did not exist in the package database.\n",package);
   return 0;
   }

/* If no version was specified, we're just checking if the package
 * is present, not for a particular number, and we can skip the
 * version number fetch and check.
 */

if(!*version)
  {
  return 1;
  }

/* check what version is installed on the system (if any) */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/pkginfo -i -l %s", package);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute pkginfo -i -l.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      if (sscanf (VBUFF, "   VERSION:  %s", tmpBUFF) > 0)
         {
         AppendItem (&evrlist, tmpBUFF, "");
         }
      }
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("pkginfo -i -l exited abnormally.\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Parse the Sun Version we are looking for once at the start */

ParseSUNVR(version, &majorB, &minorB, &microB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

for (evr = evrlist; evr != NULL; evr=evr->next)
   {
   char *evrstart;
   evrstart = evr->name;
   
   /* Start out assuming the comparison will be equal. */
   result = cmpsense_eq;
   
   ParseSUNVR(evrstart, &majorA, &minorA, &microA);
   
   /* Compare the major versions. */
   if(majorA > majorB)
      {
      result = cmpsense_gt;
      }
   if(majorA < majorB)
      {
      result = cmpsense_lt;
      }

   /* If the major versions are the same, check the minor versions. */
   if(result == cmpsense_eq)
      {
      if(minorA > minorB)
         {
         result = cmpsense_gt;
         }

      if(minorA < minorB)
         {
         result = cmpsense_lt;
         }
      
      /* If the minor versions match, compare the micro versions. */

      if(result == cmpsense_eq)
         {
         if(microA > microB)
            {
            result = cmpsense_gt;
            }
         if(microA < microB)
            {
            result = cmpsense_lt;
            }
         }
      }
   
   switch(cmp)
      {
      case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
      }

   /* If we find a match, skip checking the rest, and return this one. */
   if (match)
      {
      DeleteItemList(evrlist);
      return 1;
      }
   }

/* If we made it out of the loop, there were no matches. */
DeleteItemList(evrlist);
return 0;
}

/*********************************************************************/
/* Sun's manual pages say that the version number is a major, minor,
 * and optional micro version number.  This code checks for that.
 * It will not handle other arbitrary and strange values people might
 * put in like "2.6d.12a" or "1.11 beta" or "pre-release 7"
 */

void ParseSUNVR (char * vr, int *major, int *minor, int *micro)
{
  char *tmpcpy = strdup(vr);
  char *startMinor = NULL;
  char *startMicro = NULL;
  char *p = NULL;

  /* Set up values. */
  *major = 0;
  *minor = 0;
  *micro = 0;

  /* Break the copy in to major, minor, and micro. */
  for(p = tmpcpy; *p; p++)
     {
     if(*p == '.')
        {
        *p = '\0';
        if(startMinor == NULL)
           {
           startMinor = p+1;
           }
        else if (startMicro == NULL)
           {
           startMicro = p+1;
           }
        }
     }
  
  *major = atoi(tmpcpy);
  if(startMinor)
     {
     *minor = atoi(startMinor);
     }
  if(startMicro)
     {
     *micro = atoi(startMicro);
     }
  
  free(tmpcpy);  
}

/*********************************************************************/
/* AIX - lslpp/installp                                              */
/*********************************************************************/
int AIXPackageCheck(char *package,char *version,enum cmpsense cmp)
{ FILE *pp;
struct Item *evrlist = NULL;
struct Item *evr;
char *evrstart;
enum cmpsense result;
int match = 0;
char tmpBUFF[CF_BUFSIZE];
char *vs, *ve;
int verA = 0;
int verB = 0;
int releaseA = 0;
int releaseB = 0;
int maintA = 0;
int maintB = 0;
int fixA = 0;
int fixB = 0;

Verbose ("Package: ");
Verbose (package);
Verbose ("\n");

/* check that the package exists in the package database */
/* we're using -l here so that RPM pkgs aren't included.  These should
be treated seperatly */
snprintf(VBUFF, CF_BUFSIZE, "/usr/bin/lslpp -qcl %s", package);

if((pp = cfpopen (VBUFF, "r")) == NULL)
  {
  Verbose ("Could not execute lslpp -qcl.\n");
  return 0;
  }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("The package %s did not exist in the package database.\n",package);
   return 0;
   }

/* If no version was specified, we're just checking if the package
 * is present, not for a particular number, and we can skip the
 * version number fetch and check.
 */

if(!*version)
  {
  return 1;
  }

/* check what version is installed on the system (if any) */
/* now that we know its installed and not an rpm we can use -L to consolidate
the Usr and ROOT portions for the version check */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/lslpp -qcL %s", package);

if((pp = cfpopen (VBUFF, "r")) == NULL)
  {
  Verbose ("Could not execute lslpp -qcL.\n");
  return 0;
  }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      /* find third field of : delimited string */
      if (sscanf (VBUFF, "%*[^:]:%*[^:]:%[^:]:%*s",tmpBUFF) > 0)
         {
            AppendItem (&evrlist, tmpBUFF, "");
         }
      }
   }
if (cfpclose (pp) != 0)
   {   
   Verbose ("lslpp -qcL exited abnormally.\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Parse the AIX Version we are looking for once at the start */

ParseAIXVR(version, &verB, &releaseB, &maintB, &fixB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

for (evr = evrlist; evr != NULL; evr=evr->next)
   {
     char *evrstart;
     evrstart = evr->name;

     /* Start out assuming the comparison will be equal. */
     result = cmpsense_eq;
     
     ParseAIXVR(evrstart, &verA, &releaseA, &maintA, &fixA);

     /* Compare the major versions. */
     if(verA > verB)
        {
        result = cmpsense_gt;
        }
     if(verA < verB)
        {
        result = cmpsense_lt;
        }

     /* If the major versions are teh same, check the release */

     if(result == cmpsense_eq)
        {
        if(releaseA > releaseB)
           {
           result = cmpsense_gt;
           }
        if(releaseA < releaseB)
           {
           result = cmpsense_lt;
           }

     /* If the releases match, compare the maint version */

        if(result == cmpsense_eq)
           {

           if(maintA > maintB)
              {
              result = cmpsense_gt;
              }
           if(maintA < maintB)
              {
              result = cmpsense_lt;
              }

     /* If the maint versions match, compare the fix level */

           if(result == cmpsense_eq)
              {

           if(fixA > fixB)
              {
              result = cmpsense_gt;
              }
           if(fixA < fixB)
              {
              result = cmpsense_lt;
              }
           }
        }
     }

   switch(cmp)
     {
           case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
     } 

   /* If we find a match, skip checking the rest, and return this one. */
   if (match)
      {
      DeleteItemList(evrlist);
      return 1;
      }
   }
/* If we made it out of the loop, there were no matches. */
DeleteItemList(evrlist);
return 0;
}

/*********************************************************************/
/* AIX docs describe the version as:
 * Version.Release.Maintenance/Modification.Fix (V.R.M.F).  
 * any non digits [0-9] in these fields will be ignored and only the 
 * numeric digits will be extracted.  standalone non-digits will be 
 * treated as 0 for the entire field.  V.R.M.F shouldn't contain any 
 * non numeric data (this is enforced by IBM tools like mkinstallp)
 */ 

void ParseAIXVR(char * vr, int *ver, int *release, int *maint, int *fix)
{
int v,r,m,f;
  /* Set up values. */
  *ver = 0;
  *release = 0;
  *maint = 0;
  *fix = 0;
  if(sscanf(vr,"%d.%*s",&v) > 0)
    *ver = v;
  if(sscanf(vr,"%*[^.].%d.%*s",&r) > 0)
    *release = r;
  if(sscanf(vr,"%*[^.].%*[^.].%d.%*s",&m) > 0)
    *maint = m;
  if(sscanf(vr,"%*[^.].%*[^.].%*[^.].%d%*s",&f) > 0)
    *fix = f;
}


/*********************************************************************/
/* Gentoo Portage                                                    */
/*********************************************************************/

int PortagePackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  struct Item *ebuildlist = NULL;
  struct Item *ebuild;
  enum cmpsense result;
  int match = 0;
  char *comparehead = NULL;
  char *comparetail = NULL;
  char *installedhead = NULL;
  char *installedtail = NULL;

  /* Yes, it's an ugly python one-liner that does something beautiful */

snprintf(VBUFF,CF_BUFSIZE,"/usr/bin/python -c 'import portage, re, sys; "
         "[sys.stdout.write(re.sub(\"\\w*?\\-\\w*?\\/([a-zA-Z0-9_+]|\\-(?![0-9]))*\\-\", \"\", package, 1) + "
         "\"\\n\") for package in portage.db[\"/\"][\"vartree\"].dbapi.match("
         "\"%s\")]'", package);

if ((pp = cfpopen(VBUFF, "r")) == NULL)
   {
   Verbose("Could not execute the equery command.  Assuming package not installed.\n");
   return 0;
   }

while(!feof(pp))
   {
   *VBUFF = '\0';
   
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   
   if (*VBUFF != '\0')
      {
      AppendItem(&ebuildlist,VBUFF,"");
      }
   }

/* Non-zero exit status means that we could not find the package, so
 * zero the list and bail. */

if (cfpclose(pp) != 0)
   {
   DeleteItemList(ebuildlist);
   ebuildlist = NULL;
   }

if (ebuildlist == NULL)
   {
   Verbose("Package %s not installed.\n", package);
   return 0;
   }


Verbose("PortageCheckPackage(): Requested %s %s %s\n", package, CMPSENSETEXT[cmp],(version[0] ? version : "ANY"));

/* If no version was specified, just return 1, because if we got this far
 * some package by that name exists. */

if (!version[0])
   {
   DeleteItemList(ebuildlist);
   return 1;
   }

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */


for (ebuild = ebuildlist; ebuild != NULL; ebuild=ebuild->next)
  {
  char *ebuildver;
  ebuildver = ebuild->name;
  
  Verbose("PortageCheckPackage(): Trying installed version %s\n", ebuildver);
  
  /* Start out assuming the comparison will be equal. */
  result = cmpsense_eq;
  
  comparehead = version;
  comparetail = NULL;
  installedhead = ebuildver;
  installedtail = NULL;
  
  /* Iterate over version portions delimited by `-' */
  while (result == cmpsense_eq)
     {
     if (*comparehead == '\0' && *installedhead == '\0')
        {
        /* No substrings remain, break from while */
        break;
        }
     else if (*comparehead == '\0')
        {
        /* Installed version has more version substrings than given */
        result = cmpsense_gt;
        }
     else if (*installedhead == '\0')
        {
        /* Installed version has less version substrings than given */
        result = cmpsense_lt;
        }
     else
        {
        /* New substring in both to test */
        comparetail = strchr(comparehead, '-');
        installedtail = strchr(installedhead, '-');
        
        /* Throw a \0 over the `-' so just the substring is tested.
         * If the tail is less than the head, we must be at last substring,
         * as no `-'s were found (so the `\0' is already there) */
        if (comparetail > comparehead) *comparetail = '\0';
        if (installedtail > installedhead) *installedtail = '\0';
        
        switch (rpmvercmp(installedhead, comparehead))
           {
           case 1:
               result = cmpsense_gt;
               break;
           case -1:
               result = cmpsense_lt;
               break;
           }
        
        if (comparetail > comparehead)
           {
           /* Restore `-' at tail and move head just past it */
           *comparetail = '-';
           comparehead = comparetail + 1;
           }
        else
           {
           /* Move head to the end of the line (`\0') */
           comparehead = comparehead + strlen(comparehead);
           }
        if (installedtail > installedhead)
           {
           /* Restore `-' at tail and move head just past it */
           *installedtail = '-';
           installedhead = installedtail + 1;
           }
        else
           {
           /* Move head to the end of the line (`\0') */
           installedhead = installedhead + strlen(installedhead);
           }
        }
     }
  
  Verbose("Comparison result: %s\n",CMPSENSETEXT[result]);
  
  switch(cmp)
     {
     case cmpsense_gt:
         match = (result == cmpsense_gt);
         break;
     case cmpsense_ge:
         match = (result == cmpsense_gt || result == cmpsense_eq);
         break;
     case cmpsense_lt:
         match = (result == cmpsense_lt);
         break;
     case cmpsense_le:
         match = (result == cmpsense_lt || result == cmpsense_eq);
         break;
     case cmpsense_eq:
         match = (result == cmpsense_eq);
         break;
     case cmpsense_ne:
         match = (result != cmpsense_eq);
         break;
     }
  
  /* If we find a match, just return it now, and don't bother checking
   * other ebuilds */
  
  if (match)
     {
     DeleteItemList(ebuildlist);
     return 1;
     }
  }

/* if we manage to make it out of the loop, we did not find a match. */

DeleteItemList(ebuildlist);
return 0;
}

/*********************************************************************/
/* FreeBSD - pkg_info/pkg_add/pkg_delete                             */
/*********************************************************************/

int FreeBSDPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  int match = 0;
  char line[CF_BUFSIZE];
  char pkgname[CF_BUFSIZE];
  char *pkgversion;

  /* The package to install must contain a version number
   * The version starts after the last '-' in the pkgname
   */
   strncpy(pkgname, package, CF_BUFSIZE - 1);
   pkgversion = strrchr(pkgname, '-');
   *pkgversion = '\0';
   pkgversion += 1;

  Verbose("FreeBSDPackageCheck(): Requested version %s %s of %s\n", CMPSENSETEXT[cmp],(version[0] ? version : "ANY"), pkgname);

  /* If no version was specified, we're just checking if the package
   * is present, not for a particular number, so >0 will match.
   */
  if (!*version)
    {
    cmp = cmpsense_gt;
    version[0] = '0';
    version[1] = 0;
    }

  /* Convert to > or < ... */
  char compare_operator[3];
   switch(cmp)
      {
      case cmpsense_gt:
      	compare_operator[0] = '>';
      	compare_operator[1] = 0;
        break;
      case cmpsense_lt:
      	compare_operator[0] = '<';
      	compare_operator[1] = 0;
        break;
      case cmpsense_ge:
      	compare_operator[0] = '>';
      	compare_operator[1] = '=';
      	compare_operator[2] = 0;
        break;
      case cmpsense_le:
      	compare_operator[0] = '<';
      	compare_operator[1] = '=';
      	compare_operator[2] = 0;
        break;
      case cmpsense_ne:
      	compare_operator[0] = '!';
      	compare_operator[1] = '=';
      	compare_operator[2] = 0;
      default:
      	compare_operator[0] = '=';
      	compare_operator[1] = 0;
        break;
	  } 

  /* check what version is installed on the system (if any) */
  Verbose("FreeBSDPackageCheck(): Running /usr/sbin/pkg_info -qE '%s%s%s'\n", pkgname, compare_operator, version);
  snprintf (VBUFF, CF_BUFSIZE, "/usr/sbin/pkg_info -qE '%s%s%s'", pkgname, compare_operator, version);

  if ((pp = cfpopen (VBUFF, "r")) == NULL)
    {
    Verbose ("Could not execute pkg_info.\n");
    return 0;
    }

  while (!feof (pp))
    {
    *VBUFF = '\0';
    ReadLine (line, CF_BUFSIZE - 1, pp);
    snprintf(OUTPUT,CF_BUFSIZE,"Package install: %s\n",line);
    }

  if (cfpclose (pp) == 0)
    {
    Verbose ("The package and version requested are installed in the package database.\n",package);
    match=1;
    }
  else 
    {
    Verbose ("The package and version requested do not exist in the package database.\n",package);
    match=0;
    }
  return match;
}

/*********************************************************************/
/* RPM Version string comparison logic
 *
 * ParseEVR and rpmvercmp are taken directly from the rpm 4.1 sources.
 * ParseEVR is taken from the parseEVR routine in lib/rpmds.c and rpmvercmp
 * is taken from llib/rpmvercmp.c
 */

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */

int rpmvercmp(const char * a, const char * b)

{
 char oldch1, oldch2;
 char * str1, * str2;
 char * one, * two;
 char *s1,*s2;
 int rc;
 int isnum;
 
 /* easy comparison to see if versions are identical */
 if (!strcmp(a, b)) return 0;
 
 one = str1 = s1 = strdup(a);
 two = str2 = s2 = strdup(b);
 
 /* loop through each version segment of str1 and str2 and compare them */
 
 while (*one && *two)
    {
    while (*one && !xisalnum(*one)) one++;
    while (*two && !xisalnum(*two)) two++;
    
    str1 = one;
    str2 = two;
    
    /* grab first completely alpha or completely numeric segment */
    /* leave one and two pointing to the start of the alpha or numeric */
    /* segment and walk str1 and str2 to end of segment */
    if (xisdigit(*str1))
       {
       while (*str1 && xisdigit(*str1)) str1++;
       while (*str2 && xisdigit(*str2)) str2++;
       isnum = 1;
       }
    else
       {
       while (*str1 && xisalpha(*str1)) str1++;
       while (*str2 && xisalpha(*str2)) str2++;
       isnum = 0;
       }
    
    /* save character at the end of the alpha or numeric segment */
    /* so that they can be restored after the comparison */
    
    oldch1 = *str1;
    *str1 = '\0';
    oldch2 = *str2;
    *str2 = '\0';
    
    /* take care of the case where the two version segments are */
    /* different types: one numeric, the other alpha (i.e. empty) */
    if (one == str1)
       {
       free(s1);
       free(s2);
       return -1; /* arbitrary */
       }
    
    if (two == str2)
       {
       free(s1);
       free(s2);
       return -1;
       }
    
    if (isnum)
       {
       /* this used to be done by converting the digit segments */
       /* to ints using atoi() - it's changed because long  */
       /* digit segments can overflow an int - this should fix that. */
       
       /* throw away any leading zeros - it's a number, right? */
       while (*one == '0') one++;
       while (*two == '0') two++;
       
       /* whichever number has more digits wins */
       if (strlen(one) > strlen(two))
          {
          free(s1);
          free(s2);
          return 1;
          }
       
       if (strlen(two) > strlen(one))
          {
          free(s1);
          free(s2);
          return -1;
          }
       }
    
    /* strcmp will return which one is greater - even if the two */
    /* segments are alpha or if they are numeric.  don't return  */
    /* if they are equal because there might be more segments to */
    /* compare */
    
    rc = strcmp(one, two);
    if (rc)
       {
       free(s1);
       free(s2);

       if (rc > 0)
          {
          return 1;
          }
       
       if (rc < 0)
          {
          return -1;
          }
       }
    
    /* restore character that was replaced by null above */
    
    *str1 = oldch1;
    one = str1;
    *str2 = oldch2;
    two = str2;
    }
 
 /* this catches the case where all numeric and alpha segments have */
 /* compared identically but the segment sepparating characters were */
 /* different */
 
 if ((!*one) && (!*two))
    {
    free(s1);
    free(s2);
    return 0;
    }
 
 /* whichever version still has characters left over wins */
 if (!*one)
    {
    free(s1);
    free(s2);
    return -1;
    }
 else
    {
    free(s1);
    free(s2);
    return 1;
    }
 
}

/*************************************************************************/

/**
 * Split EVR into epoch, version, and release components.
 * @param evr  [epoch:]version[-release] string
 * @retval *ep  pointer to epoch
 * @retval *vp  pointer to version
 * @retval *rp  pointer to release
 */


void ParseEVR(char * evr,const char ** ep,const char ** vp,const char ** rp)
{
 const char *epoch;
 const char *version;  /* assume only version is present */
 const char *release;
 char *s, *se;
 
 s = evr;

 while (*s && xisdigit(*s)) s++; /* s points to epoch terminator */

 se = strrchr(s, '-');  /* se points to version terminator */
 
 if (*s == ':')
    {
    epoch = evr;
    *s++ = '\0';
    version = s;
    if (*epoch == '\0') epoch = "0";
    }
 else
    {
    epoch = NULL; /* XXX disable epoch compare if missing */
    version = evr;
    }
 if (se)
    {
    *se++ = '\0';
    release = se;
    }
 else
    {
    release = NULL;
    }
 
 if (ep) *ep = epoch;
 if (vp) *vp = version;
 if (rp) *rp = release;
}
