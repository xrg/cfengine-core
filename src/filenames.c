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
 

/*********************************************************************/
/*                                                                   */
/*  TOOLKITS: "sharable" library                                     */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/
/* Level 1                                                           */
/*********************************************************************/

int IsAbsoluteFileName(f)

char *f;

{
#ifdef NT
if (IsFileSep(f[0]) && IsFileSep(f[1]))
   {
   return true;
   }
if ( isalpha(f[0]) && f[1] == ':' && IsFileSep(f[2]) )
   {
   return true;
   }
#endif
if (*f == '/')
   {
   return true;
   }

return false;
}


/*******************************************************************/

int RootDirLength(f)

  /* Return length of Initial directory in path - */

char *f;

{
#ifdef NT
  int len;

if (IsFileSep(f[0]) && IsFileSep(f[1]))
   {
   /* UNC style path */

   /* Skip over host name */
   for (len=2; !IsFileSep(f[len]); len++)
      {
      if (f[len] == '\0')
	 {
	 return len;
	 }
      }

   /* Skip over share name */
   for (len++; !IsFileSep(f[len]); len++)
      {
      if (f[len] == '\0')
	 {
	 return len;
	 }
      }

   /* Skip over file separator */
   len++;

   return len;
   }
if ( isalpha(f[0]) && f[1] == ':' && IsFileSep(f[2]) )
   {
   return 3;
   }
#endif
if (*f == '/')
   {
   return 1;
   }

return 0;
}

/*******************************************************************/

void AddSlash(str)

char *str;

{
if ((strlen(str)== 0) || (str == NULL))
   {
   return;
   }
 
if (!IsFileSep(str[strlen(str)-1]))
   {
   strcat(str,FILE_SEPARATOR_STR);
   }
}

/*********************************************************************/

void DeleteSlash(str)

char *str;

{
if ((strlen(str)== 0) || (str == NULL))
   {
   return;
   }

 if (IsFileSep(str[strlen(str)-1]))
   {
   str[strlen(str)-1] = '\0';
   }
}

/*********************************************************************/

void DeleteNewline(str)

char *str;

{
if ((strlen(str)== 0) || (str == NULL))
   {
   return;
   }

/* !!! remove carriage return too? !!! */
 if (str[strlen(str)-1] == '\n')
   {
   str[strlen(str)-1] = '\0';
   }
}

/*********************************************************************/

char *LastFileSeparator(str)

  /* Return pointer to last file separator in string, or NULL if 
     string does not contains any file separtors */

char *str;

{ char *sp;

/* Walk through string backwards */
 
sp = str + strlen(str) - 1;

 while (sp >= str) 
   {
   if (IsFileSep(*sp))
      {
      return sp;
      }
   sp--;
   }

return NULL;
}

/*********************************************************************/

int ChopLastNode(str)

  /* Chop off trailing node name (possible blank) starting from
     last character and removing up to the first / encountered 
     e.g. /a/b/c -> /a/b
          /a/b/ -> /a/b                                        */

char *str;

{ char *sp;

if ((sp = LastFileSeparator(str)) == NULL)
   {
   return false;
   }
else
   {
   *sp = '\0';
   return true;
   }
}

/*********************************************************************/

char *CanonifyName(str)

char *str;

{ static char buffer[bufsize];
  char *sp;

bzero(buffer,bufsize);
strcpy(buffer,str);

for (sp = buffer; *sp != '\0'; sp++)
    {
    if (!isalnum((int)*sp) || *sp == '.')
       {
       *sp = '_';
       }
    }

return buffer;
}

/*********************************************************************/

char *Space2Score(str)

char *str;

{ static char buffer[bufsize];
  char *sp;

bzero(buffer,bufsize);
strcpy(buffer,str);

for (sp = buffer; *sp != '\0'; sp++)
    {
    if (*sp == ' ')
       {
       *sp = '_';
       }
    }

return buffer;
}

/*********************************************************************/

char *ASUniqueName(str) /* generates a unique action sequence name */

char *str;

{ static char buffer[bufsize];
  struct Item *ip;

bzero(buffer,bufsize);
strcpy(buffer,str);

for (ip = VADDCLASSES; ip != NULL; ip=ip->next)
   {
   if (strlen(buffer)+strlen(ip->name)+3 > maxlinksize)
      {
      break;
      }
   
   strcat(buffer,".");
   strcat(buffer,ip->name);
   }

return buffer;
}

/*********************************************************************/

char *ReadLastNode(str)

/* Return the last node of a pathname string  */

char *str;

{ char *sp;
  
if ((sp = LastFileSeparator(str)) == NULL)
   {
   return str;
   }
else
   {
   return sp + 1;
   }
}

/*********************************************************************/

int MakeDirectoriesFor(file,force)  /* Make all directories which underpin file */

char *file, force;

{ char *sp,*spc;
  char currentpath[bufsize];
  char pathbuf[bufsize];
  struct stat statbuf;
  mode_t mask;
  int rootlen;
  char Path_File_Separator;
    
if (!IsAbsoluteFileName(file))
   {
   snprintf(OUTPUT,bufsize*2,"Will not create directories for a relative filename %s\nHas no invariant meaning\n",file);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

strncpy(pathbuf,file,bufsize-1);                                      /* local copy */

/* skip link name */
sp = LastFileSeparator(pathbuf);
if (sp == NULL)
   {
   sp = pathbuf;
   }
*sp = '\0';

DeleteSlash(pathbuf); 
 
if (lstat(pathbuf,&statbuf) != -1)
   {
   if (S_ISLNK(statbuf.st_mode))
      {
      Verbose("%s: INFO: %s is a symbolic link, not a true directory!\n",VPREFIX,pathbuf);
      }

   if (force == 'y')   /* force in-the-way directories aside */
      {
      if (!S_ISDIR(statbuf.st_mode))  /* if the dir exists - no problem */
	 {
	 if (ISCFENGINE)
	    {
	    struct Tidy tp;
	    struct TidyPattern tpat;
	    struct stat sbuf;
	    
	    strcpy(currentpath,pathbuf);
	    DeleteSlash(currentpath);
	    strcat(currentpath,".cf-moved");
	    snprintf(OUTPUT,bufsize,"Moving obstructing file/link %s to %s to make directory",pathbuf,currentpath);
	    CfLog(cferror,OUTPUT,"");
	    
	    /* If cfagent, remove an obstructing backup object */
	    
	    if (lstat(currentpath,&sbuf) != -1)
	       {
	       if (S_ISDIR(sbuf.st_mode))
		  {
		  tp.maxrecurse = 2;
		  tp.tidylist = &tpat;
		  tp.next = NULL;
		  tp.path = currentpath;
		  
		  tpat.recurse = INFINITERECURSE;
		  tpat.age = 0;
		  tpat.size = 0;
		  tpat.pattern = strdup("*");
		  tpat.classes = strdup("any");
		  tpat.defines = NULL;
		  tpat.elsedef = NULL;
		  tpat.dirlinks = 'y';
		  tpat.travlinks = 'n';
		  tpat.rmdirs = 'y';
		  tpat.searchtype = 'a';
		  tpat.log = 'd';
		  tpat.inform = 'd';
		  tpat.next = NULL;
		  RecursiveTidySpecialArea(currentpath,&tp,INFINITERECURSE);
		  free(tpat.pattern);
		  free(tpat.classes);
		  
		  if (rmdir(currentpath) == -1)
		     {
		     snprintf(OUTPUT,bufsize*2,"Couldn't remove directory %s while trying to remove a backup\n",currentpath);
		     CfLog(cfinform,OUTPUT,"rmdir");
		     }
		  }
	       else
		  {
		  if (unlink(currentpath) == -1)
		     {
		     snprintf(OUTPUT,bufsize*2,"Couldn't remove file/link %s while trying to remove a backup\n",currentpath);
		     CfLog(cfinform,OUTPUT,"rmdir");
		     }
		  }
	       }
	      
	    /* And then move the current object out of the way...*/
	    
	    if (rename(pathbuf,currentpath) == -1)
	       {
	       snprintf(OUTPUT,bufsize*2,"Warning. The object %s is not a directory.\n",pathbuf);
	       CfLog(cfinform,OUTPUT,"");
	       CfLog(cfinform,"Could not make a new directory or move the block","rename");
	       return(false);
	       }
	    }
	 }
      else
	 {
	 if (! S_ISLNK(statbuf.st_mode) && ! S_ISDIR(statbuf.st_mode))
	    {
	    snprintf(OUTPUT,bufsize*2,"Warning. The object %s is not a directory.\n",pathbuf);
	    CfLog(cfinform,OUTPUT,"");
	    CfLog(cfinform,"Cannot make a new directory without deleting it!\n\n","");
	    return(false);
	    }
	 }
      }
   }

/* Now we can make a new directory .. */ 

currentpath[0] = '\0';
 
rootlen = RootDirLength(sp);
strncpy(currentpath, file, rootlen);
for (sp = file+rootlen, spc = currentpath+rootlen; *sp != '\0'; sp++)
   {
   if (!IsFileSep(*sp) && *sp != '\0')
      {
      *spc = *sp;
      spc++;
      }
   else
      {
      Path_File_Separator = *sp;
      *spc = '\0';

      if (strlen(currentpath) == 0)
         {
         }
      else if (stat(currentpath,&statbuf) == -1)
         {
         Debug2("cfengine: Making directory %s, mode %o\n",currentpath,DEFAULTMODE);

         if (! DONTDO)
            {
	    mask = umask(0);

            if (mkdir(currentpath,DEFAULTMODE) == -1)
               {
               snprintf(OUTPUT,bufsize*2,"Unable to make directories to %s\n",file);
               CfLog(cferror,OUTPUT,"mkdir");
	       umask(mask);
               return(false);
               }
	    umask(mask);
            }
         }
      else
         {
         if (! S_ISDIR(statbuf.st_mode))
            {
            snprintf(OUTPUT,bufsize*2,"Cannot make %s - %s is not a directory! (use forcedirs=true)\n",pathbuf,currentpath);
	    CfLog(cferror,OUTPUT,"");
            return(false);
            }
         }

      /* *spc = FILE_SEPARATOR; */
      *spc = Path_File_Separator;
      spc++;
      }
   }

Debug("Directory for %s exists. Okay\n",file);
return(true);
}

/*********************************************************************/

int BufferOverflow(str1,str2)                   /* Should be an inline ! */

char *str1, *str2;

{ int len = strlen(str2);

if ((strlen(str1)+len) > (bufsize - buffer_margin))
   {
   snprintf(OUTPUT,bufsize*2,"Buffer overflow constructing string. Increase bufsize macro.\n");
   CfLog(cferror,OUTPUT,"");
   printf("%s: Tried to add %s to %s\n",VPREFIX,str2,str1);
   return true;
   }

return false;
}

/*********************************************************************/

void Chop(str)

char *str;

{
if ((str == NULL) || (strlen(str)==0))
   {
   return;
   }

if (isspace((int)str[strlen(str)-1]))
   {
   str[strlen(str)-1] = '\0';
   }
}



/*********************************************************************/

int CompressPath(dest,src)

char *dest, *src;

{ char *sp;
  char node[bufsize];
  int nodelen;
  int rootlen;

Debug2("CompressPath(%s,%s)\n",dest,src);

bzero(dest,bufsize);

rootlen = RootDirLength(src);
strncpy(dest, src, rootlen);
for (sp = src+rootlen; *sp != '\0'; sp++)
   {
   if (IsFileSep(*sp))
      {
      continue;
      }

   for (nodelen = 0; sp[nodelen] != '\0' && !IsFileSep(sp[nodelen]); nodelen++)
      {
      if (nodelen > maxlinksize)
	 {
	 CfLog(cferror,"Link in path suspiciously large","");
	 return false;
	 }
      }
   strncpy(node, sp, nodelen);
   node[nodelen] = '\0';

   sp += nodelen - 1;

   if (strcmp(node,".") == 0)
      {
      continue;
      }

   if (strcmp(node,"..") == 0)
      {
      if (! ChopLastNode(dest))
	 {
	 Debug("cfengine: used .. beyond top of filesystem!\n");
	 return false;
	 }
      continue;
      }
   else
      {
      AddSlash(dest);
      }
   
   if (BufferOverflow(dest,node))
      {
      return false;
      }
   
   strcat(dest,node);
   }
return true;
}

/*********************************************************************/
/* TOOLKIT : String                                                  */
/*********************************************************************/

char ToLower (ch)

char ch;

{
if (isdigit((int)ch) || ispunct((int)ch))
   {
   return(ch);
   }

if (islower((int)ch))
   {
   return(ch);
   }
else
   {
   return(ch - 'A' + 'a');
   }
}


/*********************************************************************/

char ToUpper (ch)

char ch;

{
if (isdigit((int)ch) || ispunct((int)ch))
   {
   return(ch);
   }

if (isupper((int)ch))
   {
   return(ch);
   }
else
   {
   return(ch - 'a' + 'A');
   }
}

/*********************************************************************/

char *ToUpperStr (str)

char *str;

{ static char buffer[bufsize];
  int i;

bzero(buffer,bufsize);
  
if (strlen(str) >= bufsize)
   {
   char *tmp;
   tmp = malloc(40+strlen(str));
   sprintf(tmp,"String too long in ToUpperStr: %s",str);
   FatalError(tmp);
   }

for (i = 0; str[i] != '\0'; i++)
   {
   buffer[i] = ToUpper(str[i]);
   }

buffer[i] = '\0';

return buffer;
}


/*********************************************************************/

char *ToLowerStr (str)

char *str;

{ static char buffer[bufsize];
  int i;

bzero(buffer,bufsize);

if (strlen(str) >= bufsize)
   {
   char *tmp;
   tmp = malloc(40+strlen(str));
   sprintf(tmp,"String too long in ToLowerStr: %s",str);
   FatalError(tmp);
   }

for (i = 0; str[i] != '\0'; i++)
   {
   buffer[i] = ToLower(str[i]);
   }

buffer[i] = '\0';

return buffer;
}
