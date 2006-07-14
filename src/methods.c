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
/* File: methods.c                                                           */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*******************************************************************/

void CheckForMethod()

{ struct Item *ip;
  int i = 0;

if (strcmp(METHODNAME,"cf-nomethod") == 0)
   {
   return;
   }

if (! MINUSF)
   {
   FatalError("Input files claim to be a module but this is a parent process\n");
   }

Verbose("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n+\n");
Verbose("+ This is a private method: %s\n",METHODNAME);

if (METHODARGS == NULL)
   {
   FatalError("This module was declared a method but no MethodParameters declaration was given");
   }
else
   {
   Verbose("+\n+ Method argument prototype = (");

   i = 1;
   
   for (ip = METHODARGS; ip != NULL; ip=ip->next)
      {
      i++;
      }

   METHODARGV = (char **) malloc(sizeof(char *) * i); 
   
   i = 0;

   for (ip = METHODARGS; ip != NULL; ip=ip->next)
      {
      /* Fill this temporarily with the formal parameters */
      METHODARGV[i++] = ip->name;
      Verbose("%s ",ip->name);
      }

   METHODARGC = i;
   
   Verbose(")\n+\n");
   }
 
Verbose("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");

Verbose("Looking for a data package for this method (%s)\n",METHODMD5);

if (!ChildLoadMethodPackage(METHODNAME,METHODMD5))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"No valid incoming request to execute method (%s)\n",METHODNAME);
   CfLog(cfinform,OUTPUT,"");
   exit(0);
   }

Debug("Method package looks ok -- proceeding\n"); 
}

/*******************************************************************/

void CheckMethodReply()

{
if (ScopeIsMethod())
   {
   if (strlen(METHODREPLYTO) > 0)
      {
      Banner("Method reply message");
      DispatchMethodReply();
      }
   }
}

/*****************************************************************************/

void DispatchNewMethod(struct Method *ptr)

{ struct Item *ip;
  char label[CF_BUFSIZE]; 
  char serverip[CF_SMALLBUF],clientip[CF_SMALLBUF];

serverip[0] = clientip[0] = '\0';
  
for (ip = ptr->servers; ip != NULL; ip=ip->next)
   {  
   if ((strcmp(ip->name,"localhost") == 0) || (strcmp(ip->name,"::1") == 0) || (strcmp(ip->name,"127.0.0.1") == 0))
      {
      Verbose("\nDispatch method localhost:%s to %s/rpc_in\n",ptr->name,VLOCKDIR);

      ptr->invitation = 'y';
      snprintf(label,CF_BUFSIZE-1,"%s/rpc_in/localhost+localhost+%s+%s",VLOCKDIR,ptr->name,ChecksumPrint('m',ptr->digest));
      EncapsulateMethod(ptr,label);
      }
   else
      {
      strncpy(serverip,Hostname2IPString(ip->name),63);

      if (strlen(ptr->forcereplyto) > 0)
         {
         strncpy(clientip,ptr->forcereplyto,63);
         }
      else
         {
         strncpy(clientip,Hostname2IPString(VFQNAME),63);
         }
     
      if (strcmp(clientip,serverip) == 0)
         {
         Verbose("Invitation to accept remote method %s on this host\n",ip->name);
         Verbose("(Note that this is not a method to be executed as server localhost)\n");
         continue;
         }
      
      Verbose("Hailing server (%s) from our calling id %s\n",serverip,VFQNAME);
      snprintf(label,CF_BUFSIZE-1,"%s/rpc_out/%s+%s+%s+%s",VLOCKDIR,clientip,serverip,ptr->name,ChecksumPrint('m',ptr->digest));
      EncapsulateMethod(ptr,label);
      Debug("\nDispatched method %s to %s/rpc_out\n",label,VLOCKDIR);
      }
   } 
}

/*****************************************************************************/

struct Item *GetPendingMethods(int state)

{ char filename[CF_MAXVARSIZE],path[CF_BUFSIZE],name[CF_BUFSIZE];
  char client[CF_BUFSIZE],server[CF_BUFSIZE],extra[CF_BUFSIZE],digeststring[CF_BUFSIZE];
  DIR *dirh;
  struct dirent *dirp;
  struct Method *mp;
  struct Item *list = NULL, *ip;
  struct stat statbuf;
  int belongs_here;
  
Debug("GetPendingMethods(%d) in (%s/rpc_in)\n",state,VLOCKDIR);


Debug("Processing applications...\n");
 
snprintf(filename,CF_MAXVARSIZE-1,"%s/rpc_in",VLOCKDIR);
 
if ((dirh = opendir(filename)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",filename);
   CfLog(cfverbose,OUTPUT,"opendir");
   return NULL;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,filename,NULL))
      {
      continue;
      }

   snprintf(path,CF_BUFSIZE-1,"%s/%s",filename,dirp->d_name);

   if (stat(path,&statbuf) == -1)
      {
      continue;
      }

   if (statbuf.st_mtime < CFSTARTTIME - (VEXPIREAFTER * 60))
      {
      Verbose("Purging expired incoming method %s (expireafter=%d)\n",path,VEXPIREAFTER);
      unlink(path);
      continue;
      }
   
   SplitMethodName(dirp->d_name,client,server,name,digeststring,extra);
   
   if (strlen(name) == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to extract method name from package (%s)",dirp->d_name);
      CfLog(cfinform,OUTPUT,"");
      continue;
      }

   belongs_here = false;

   for (ip = IPADDRESSES; ip != NULL; ip = ip->next)
      {
      if (strcmp(server,ip->name) == 0 || strcmp(client,ip->name) == 0)
         {
         belongs_here = true;
         }      
      }

   if (strcmp(server,"localhost") == 0)
      {
      belongs_here = true;
      }
   
   if (!belongs_here)
      {
      Verbose("Purging stray community file that does not belong here: %s\n",dirp->d_name);
      unlink(path);
      continue;
      }
         
   if (strlen(extra) != 0)
      {
      if (state == CF_METHODREPLY)
         {
         Debug("Found an attachment (%s) in reply to %s\n",extra,name);
         }
      else
         {
         Debug("Found an attachment (%s) to incoming method %s\n",extra,name);
         }
      continue;
      }
   
   if ((state == CF_METHODREPLY) && (strstr(name,":Reply") == 0))
      {
      Debug("Ignoring bundle (%s) from waiting function call\n",name);
      continue;
      }
   
   if ((state == CF_METHODEXEC) && (strstr(name,":Reply")))
      {
      Debug("Ignoring bundle (%s) from waiting reply\n",name);
      continue;
      }

   Verbose("Looking at method (%s) from (%s) intended for exec on (%s) with arghash %s\n",name,client,server,digeststring);

   ip = SplitStringAsItemList(name,':');
   
   if (mp = IsDefinedMethod(ip->name,digeststring))
      {
      if (IsItemIn(mp->servers,"localhost") || IsItemIn(mp->servers,IPString2Hostname(server)) || IsItemIn(mp->servers,VIPADDRESS) || IsItemIn(mp->servers,server) || IsItemIn(mp->servers,VFQNAME))
         {
         if (state == CF_METHODREPLY)
            {
            Verbose("Found a local approval to forward reply from local method (%s) to final destination sender %s\n",name,client);
            }
         else
            {
            if (IsDefinedClass(mp->classes))
               {
               Verbose("Found a local approval to execute this method (%s) on behalf of sender %s if (%s)\n",name,IPString2Hostname(client),mp->classes);
               }
            else
               {
               Verbose("No local approval in methods list on this host for received request (%s) if (%s)\n",dirp->d_name,mp->classes);
               continue;
               }
            }
         AppendItem(&list,dirp->d_name,"");
         mp->bundle = strdup(dirp->d_name);
         }
      else
         {
         Verbose("No local method match on this host for received request (%s) for host %s=%s=%s in\n",dirp->d_name,server,IPString2Hostname(server),VIPADDRESS);
         if (VERBOSE)
            {
            DebugListItemList(mp->servers);
            }
         unlink(path);
         }
      }
   
   DeleteItemList(ip);
   }
 
closedir(dirh);

 
if (list == NULL)
   {
   return NULL;
   }
 
return list;
}


/*****************************************************************************/

void EvaluatePendingMethod(char *methodname)

 /* Actually execute any method that is our responsibilty */

{ struct Method *ptr;
  char line[CF_BUFSIZE];
  char options[32];
  char client[CF_BUFSIZE],server[CF_BUFSIZE],name[CF_BUFSIZE],digeststring[CF_BUFSIZE],extra[CF_BUFSIZE];
  char *sp;
  char execstr[CF_BUFSIZE];
  int print;
  FILE *pp;

SplitMethodName(methodname,client,server,name,digeststring,extra);

/* Could check client for access granted? */

if (strcmp(server,"localhost") == 0 || strcmp(IPString2Hostname(server),VFQNAME) == 0)
   {
   Debug("EvaluatePendingMethod(%s)\n",name);
   }
else
   {
   Debug("Nothing to do for %s\n",methodname);
   return;
   }
 

options[0] = '\0';

if (INFORM)
   {
   strcat(options,"-I ");
   }

if (IGNORELOCK)
   {
   strcat(options,"-K ");
   }

if (VERBOSE)
   {
   strcat(options,"-v ");
   }

if (DEBUG || D2)
   {
   strcat(options,"-d2 ");
   }

ptr = IsDefinedMethod(name,digeststring);

strcat(options,"-Z ");
strcat(options,digeststring);
strcat(options," ");

snprintf(execstr,CF_BUFSIZE-1,"%s/bin/cfagent -f %s %s",CFWORKDIR,GetMethodFilename(ptr),options);

Verbose("Trying %s\n",execstr);
 
if (IsExcluded(ptr->classes))
   {
   Verbose("(Excluded on classes (%s))\n",ptr->classes);
   return;
   }
 
ResetOutputRoute(ptr->log,ptr->inform);

if (!GetLock(ASUniqueName("method-exec"),CanonifyName(methodname),ptr->ifelapsed,ptr->expireafter,VUQNAME,CFSTARTTIME))
   {
   return;
   }
 
snprintf(OUTPUT,CF_BUFSIZE*2,"Executing method %s...(uid=%d,gid=%d)\n",execstr,ptr->uid,ptr->gid);
CfLog(cfinform,OUTPUT,"");
 
if (DONTDO)
   {
   printf("%s: execute method %s\n",VPREFIX,ptr->name);
   }
else
   {    
   switch (ptr->useshell)
      {
      case 'y':  pp = cfpopen_shsetuid(execstr,"r",ptr->uid,ptr->gid,ptr->chdir,ptr->chroot);
   break;
      default:   pp = cfpopensetuid(execstr,"r",ptr->uid,ptr->gid,ptr->chdir,ptr->chroot);
   break;      
      }
   
   if (pp == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",execstr);
      CfLog(cferror,OUTPUT,"popen");
      ResetOutputRoute('d','d');
      ReleaseCurrentLock();
      return;
      } 
   
   while (!feof(pp))
      {
      if (ferror(pp))  /* abortable */
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",execstr);
         CfLog(cferror,OUTPUT,"ferror");
         break;
         }
      
      ReadLine(line,CF_BUFSIZE,pp);
      
      if (strstr(line,"cfengine-die"))
         {
         break;
         }
      
      if (ferror(pp))  /* abortable */
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",execstr);
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
      
      if (print)
         {
         printf("%s:%s: %s\n",VPREFIX,ptr->name,line);
         }
      }
   
   cfpclose(pp);  
   }
  
snprintf(OUTPUT,CF_BUFSIZE*2,"Finished local method %s processing\n",execstr);
CfLog(cfinform,OUTPUT,"");
 
ResetOutputRoute('d','d');
ReleaseCurrentLock();
 
return;
}


/*****************************************************************************/
/* Level 2                                                                   */
/*****************************************************************************/

int ParentLoadReplyPackage(char *methodname)

{ char client[CF_MAXVARSIZE],server[CF_BUFSIZE],name[CF_BUFSIZE],buf[CF_BUFSIZE];
  char line[CF_BUFSIZE],type[CF_SMALLBUF],arg[CF_BUFSIZE], **methodargv = NULL;
  char basepackage[CF_BUFSIZE],digeststring[CF_BUFSIZE],extra[CF_MAXVARSIZE];
  char *sp;
  struct Item *ip;
  struct Method *mp;
  int isreply = false, argnum = 0, i, methodargc = 0;
  FILE *fp,*fin;
  
Debug("\nParentLoadReplyPackage(%s)\n\nLook for method replies in %s/rpc_in\n",methodname,VLOCKDIR);

SplitMethodName(methodname,client,server,name,digeststring,extra);

for (sp = name; *sp != '\0'; sp++)
   {
   if (*sp == ':') /* Strip off :Reply */
      {
      *sp = '\0';
      break;
      }
   }

if ((mp = IsDefinedMethod(name,digeststring)) == NULL)
   {
   return false;
   }

i = 1;
   
for (ip = mp->return_vars; ip != NULL; ip=ip->next)
   {
   i++;
   }
 
methodargv = (char **)  malloc(i*sizeof(char *));
 
i = 0;
 
for (ip = mp->return_vars; ip != NULL; ip=ip->next)
   {
   /* Fill this temporarily with the formal parameters */
   methodargv[i++] = ip->name; 
   }
 
methodargc = i;
 
snprintf(basepackage,CF_BUFSIZE-1,"%s/rpc_in/%s",VLOCKDIR,methodname);
  
argnum = CountAttachments(basepackage);
 
if (argnum != methodargc)
   {
   Verbose("Number of return arguments (%d) does not match template (%d)\n",argnum,methodargc);
   Verbose("Discarding method %s\n",name);
   free((char *)methodargv);
   return false;
   }

snprintf(buf,CF_BUFSIZE,"rpc_in_%s_%s",methodname,digeststring);

if (IsDefinedClass(CanonifyName(buf)))
   {
   free((char *)methodargv);
   return false;
   }
else
   {
   Debug("No further replies can be accepted yet (tot %d mins)\n",mp->ifelapsed);
   AddPersistentClass(CanonifyName(buf),mp->ifelapsed,cfpreserve);
   }

argnum = 0;
Verbose("Opening bundle %s\n",methodname);
 
if ((fp = fopen(basepackage,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open a parameter bundle (%s) for method %s for parent reply",basepackage,name);
   CfLog(cfinform,OUTPUT,"fopen");
   free((char *)methodargv);
   return false;
   }

 while (!feof(fp))
    {
    memset(line,0,CF_BUFSIZE);
    fgets(line,CF_BUFSIZE,fp);
    type[0] = '\0';
    arg[0] = '\0';
    
    sscanf(line,"%63s %s",type,arg);
    
    switch (ConvertMethodProto(type))
       {
       case cfmeth_name:
           if (strncmp(arg,name,strlen(name)) != 0)
              {
              snprintf(OUTPUT,CF_BUFSIZE,"Method name %s did not match package name %s",name,arg);
              CfLog(cfinform,OUTPUT,"");
              fclose(fp);
              free((char *)methodargv);
              return false;
              }
           
           break;
           
       case cfmeth_isreply:
           isreply = true;
           break;
           
       case cfmeth_sendclass:
           
           if (IsItemIn(mp->return_classes,arg))
              {
              Verbose("Setting method classes %s_%s\n",name,arg);
              AddPrefixedMultipleClasses(name,arg);
              }
           else
              {
              Verbose("Method returned a class (%s) that was not in the return_classes access list\n",arg);
              }
           break;
           
       case cfmeth_attacharg:
           
           if (methodargv[argnum][0] == '/')
              {
              int val;
              struct stat statbuf;
              FILE *fin,*fout;
              
              if (stat (arg,&statbuf) == -1)
                 {
                 Verbose("Unable to stat file %s\n",arg);
                 return false;  
                 }
              
              val = statbuf.st_size;
              
              Debug("Install file in %s at %s\n",arg,methodargv[argnum]);
              
              if ((fin = fopen(arg,"r")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"could not open argument bundle %s\n",arg);
                 CfLog(cferror,OUTPUT,"fopen");
                 free((char *)methodargv);
                 return false;
                 }
              
              if ((fout = fopen(methodargv[argnum],"w")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"Could not write to local parameter file %s\n",methodargv[argnum]);
                 CfLog(cferror,OUTPUT,"fopen");
                 free((char *)methodargv);
                 fclose(fin);
                 return false;
                 }
              
              if (val > CF_BUFSIZE)
                 {
                 int count = 0;
                 while(!feof(fin))
                    {
                    fputc(fgetc(fin),fout);
                    count++;
                    if (count == val)
                       {
                       break;
                       }
                    Debug("Wrote #\n");
                    }
                 }
              else
                 {
                 memset(buf,0,CF_BUFSIZE); 
                 fread(buf,val,1,fin);
                 fwrite(buf,val,1,fout);
                 Debug("Wrote #\n");
                 }
              
              fclose(fin);
              fclose(fout);      
              }
           else
              {
              char argbuf[CF_BUFSIZE],newname[CF_MAXVARSIZE];
              memset(argbuf,0,CF_BUFSIZE);
              
              Debug("Read arg from file %s in parent load\n",arg);
              
              if ((fin = fopen(arg,"r")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"Missing module argument package (%s)\n",arg);
                 FatalError(OUTPUT);
                 }
              
              fread(argbuf,CF_BUFSIZE-1,1,fin);
              fclose(fin);

              if (IsItemIn(mp->return_vars,methodargv[argnum]))
                 {
                 snprintf(newname,CF_MAXVARSIZE-1,"%s.%s",name,methodargv[argnum]);
                 Verbose("Setting variable %s to %s\n",newname,argbuf);
                 AddMacroValue(CONTEXTID,newname,argbuf);
                 }
              else
                 {
                 Verbose("Method returned a variable (%s) that was not in the return_vars access list\n",argbuf);
                 }
              }
           
           Debug("Unlink %s\n",arg);
           unlink(arg);
           argnum++;
           
           break;
           
       default:
           Debug("Protocol error (%s) in package received for method %s",type,basepackage);
           break;
       }
    }
 
 fclose(fp); 
 free((char *)methodargv); 
  
unlink(basepackage);   /* Now remove the invitation */ 
 
if (!isreply)
   {
   Verbose("Reply package was not marked as a reply\n");
   ReleaseCurrentLock();
   return false;
   }

return true; 
}


/*****************************************************************************/

int ChildLoadMethodPackage(char *name,char *digeststring)

{ char line[CF_BUFSIZE],type[CF_SMALLBUF],arg[CF_BUFSIZE];
  char basepackage[CF_BUFSIZE],cffilename[CF_BUFSIZE],buf[CF_BUFSIZE],ebuff[CF_EXPANDSIZE];
  char clientip[CF_BUFSIZE],ipaddress[CF_BUFSIZE];
  struct stat statbuf;
  FILE *fp,*fin,*fout;
  int argnum = 0;
  
Verbose("Child is looking for its bundle: %s\n",name);

METHODFILENAME[0] = '\0'; 
 
if (!CheckForMethodPackage(name))
   {
   Verbose("Could not find anything looking like a current invitation\n");
   return false;
   }

if (strcmp(METHODREPLYTO,"localhost") != 0)
   {
   strncpy(clientip,Hostname2IPString(METHODREPLYTO),63);
   strncpy(ipaddress,Hostname2IPString(METHODFOR),63);     
   }
else
   {
   strncpy(clientip,"localhost",63);
   strncpy(ipaddress,"localhost",63);     
   }

snprintf(basepackage,CF_BUFSIZE-1,"%s/rpc_in/%s+%s+%s+%s",VLOCKDIR,clientip,ipaddress,METHODNAME,METHODMD5);

argnum = CountAttachments(basepackage);
 
if (argnum != METHODARGC)
   {
   Verbose("Number of return arguments (%d) does not match template (%d)\n",argnum,METHODARGC);
   Verbose("Discarding method %s\n",name);
   return false;
   }

argnum = 0; 
 
if ((fp = fopen(basepackage,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open a parameter bundle (%s) for method %s for child loading",basepackage,name);
   CfLog(cfinform,OUTPUT,"fopen");
   return false;
   }
 
while (!feof(fp))
   {
   memset(line,0,CF_BUFSIZE);
   fgets(line,CF_BUFSIZE,fp);
   type[0] = '\0';
   arg[0] = '\0';
   
   sscanf(line,"%63s %s",type,arg);
   
   switch (ConvertMethodProto(type))
      {
      case cfmeth_name:
          if (strcmp(arg,name) != 0)
             {
             snprintf(OUTPUT,CF_BUFSIZE,"Method name %s did not match package name %s",name,arg);
             CfLog(cfinform,OUTPUT,"");
             fclose(fp);
             return false;
             }
          break;
          
      case cfmeth_file:
          
          if (GetMacroValue(CONTEXTID,"moduledirectory"))
             {
             ExpandVarstring("$(moduledirectory)",ebuff,NULL);
             }
          else
             {
             snprintf(ebuff,CF_BUFSIZE,"%s/modules",VLOCKDIR);
             }
          
          snprintf(cffilename,CF_BUFSIZE-1,"%s/%s",ebuff,arg);
          
          if (lstat(cffilename,&statbuf) != -1)
             {
             if (S_ISLNK(statbuf.st_mode))
                {
                snprintf(OUTPUT,CF_BUFSIZE,"SECURITY ALERT. Method (%s) executable is a symbolic link",name);
                CfLog(cferror,OUTPUT,"");
                fclose(fp);
                continue;
                }
             }
          
          if (stat(cffilename,&statbuf) == -1)
             {
             snprintf(OUTPUT,CF_BUFSIZE,"Method name %s did not find executable name %s",name,cffilename);
             CfLog(cfinform,OUTPUT,"stat");
             fclose(fp);
             continue;
             }
          
          strncpy(METHODFILENAME,cffilename,CF_MAXVARSIZE);
          break;
          
      case cfmeth_replyto:
          
          Debug("Found reply to host: %s\n",arg);
          strncpy(METHODREPLYTO,arg,CF_MAXVARSIZE-1);
          break;
          
      case cfmeth_sendclass:

          Verbose("Defining transmitted class: %s\n",arg);

          if (!IsHardClass(arg))
             {
             AddMultipleClasses(arg);
             }
          else
             {
             snprintf(OUTPUT,CF_BUFSIZE,"Cannot redefine a hard class in a method (%s)",arg);
             CfLog(cfinform,OUTPUT,"");
             }
          break;
          
      case cfmeth_attacharg:
          
          Debug("Handling expected local argument (%s)\n",METHODARGV[argnum]);
          
          if (METHODARGV[argnum][0] == '/')
             {
             int val;
             struct stat statbuf;
             
             if (stat (arg,&statbuf) == -1)
                {
                Verbose("Unable to stat file %s\n",arg);
                return false;  
                }
             
             val = statbuf.st_size;
             
             Debug("Install file in %s at %s\n",arg,METHODARGV[argnum]);
             
             if ((fin = fopen(arg,"r")) == NULL)
                {
                snprintf(OUTPUT,CF_BUFSIZE,"could not open argument bundle %s\n",arg);
                CfLog(cferror,OUTPUT,"fopen");
                return false;
                }
             
             if ((fout = fopen(METHODARGV[argnum],"w")) == NULL)
                {
                snprintf(OUTPUT,CF_BUFSIZE,"Could not write to local parameter file %s\n",METHODARGV[argnum]);
                CfLog(cferror,OUTPUT,"fopen");
                fclose(fin);
                return false;
                }
             
             if (val > CF_BUFSIZE)
                {
                int count = 0;
                while(!feof(fin))
                   {
                   fputc(fgetc(fin),fout);
                   count++;
                   if (count == val)
                      {
                      break;
                      }
                   Debug("Wrote #\n");
                   }
                }
             else
                {
                memset(buf,0,CF_BUFSIZE); 
                fread(buf,val,1,fin);
                fwrite(buf,val,1,fout);
                Debug("Wrote #\n");
                }
             
             fclose(fin);
             fclose(fout);      
             }
          else
             {
             char argbuf[CF_BUFSIZE];
             memset(argbuf,0,CF_BUFSIZE);
             
             Debug("Read arg from file %s\n",arg);
             
             if ((fin = fopen(arg,"r")) == NULL)
                {
                snprintf(OUTPUT,CF_BUFSIZE,"Missing module argument package (%s)\n",arg);
                FatalError(OUTPUT);
                }
             
             fread(argbuf,CF_BUFSIZE-1,1,fin);
             fclose(fin);
             
             if (strcmp(METHODARGV[argnum],"void") != 0)
                {
                Verbose("Setting transmitted variable %s = ( %s )\n",METHODARGV[argnum],argbuf);
                AddMacroValue(CONTEXTID,METHODARGV[argnum],argbuf);
                }
             else
                {
                Verbose("Method had no arguments (void)\n");
                }
             }
          
          Debug("Removing arg %s\n",arg);
          unlink(arg);
          argnum++;
          break;
          
      default:
          break;
      }
   } 
 
fclose(fp);

if (strlen(METHODFILENAME) == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Method (%s) package had no filename to execute",name);
   CfLog(cfinform,OUTPUT,"");
   return false;
   }
 
if (strlen(METHODREPLYTO) == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Method (%s) package had no replyto address",name);
   CfLog(cfinform,OUTPUT,"");
   return false;
   }

Debug("Done loading %s\n",basepackage); 
unlink(basepackage);   /* Now remove the invitation */ 
return true; 
}

/*****************************************************************************/

void DispatchMethodReply()

{ char label[CF_BUFSIZE]; 
  char ipaddress[CF_SMALLBUF],clientip[CF_SMALLBUF];

Verbose("DispatchMethodReply()\n\n");

if (strcmp(METHODREPLYTO,"localhost") == 0 || strcmp(METHODREPLYTO,"::1") == 0 || strcmp(METHODREPLYTO,"127.0.0.1") == 0)
   {    
   snprintf(label,CF_BUFSIZE-1,"%s/rpc_in/localhost+localhost+%s:Reply+%s",VLOCKDIR,METHODNAME,METHODMD5);
   EncapsulateReply(label);
   }
else
   {
   strncpy(clientip,Hostname2IPString(METHODREPLYTO),63);
   strncpy(ipaddress,Hostname2IPString(VFQNAME),63);     

   Verbose("\nDispatch method %s:%s to %s/rpc_out\n",METHODREPLYTO,METHODNAME,VLOCKDIR);

   snprintf(label,CF_BUFSIZE-1,"%s/rpc_out/%s+%s+%s:Reply+%s",VLOCKDIR,clientip,ipaddress,METHODNAME,METHODMD5);
   EncapsulateReply(label);
   }
}


/*****************************************************************************/

char *GetMethodFilename(struct Method *ptr)

{ char line[CF_BUFSIZE],type[CF_SMALLBUF],arg[CF_BUFSIZE];
  char basepackage[CF_BUFSIZE],cffilename[CF_BUFSIZE],ebuff[CF_EXPANDSIZE];
  static char returnfile[CF_BUFSIZE];
  FILE *fp;
  struct stat statbuf;
  int numargs = 0, argnum = 0;

numargs = ListLen(ptr->send_args);

snprintf(basepackage,CF_BUFSIZE-1,"%s/rpc_in/%s",VLOCKDIR,ptr->bundle); 
 
if ((fp = fopen(basepackage,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open a parameter bundle (%s) for method %s to get filenames",ptr->bundle,ptr->name);
   CfLog(cfinform,OUTPUT,"fopen");
   return returnfile;
   }
 
 argnum = 0;
 
 while (!feof(fp))
    {
    memset(line,0,CF_BUFSIZE);
    fgets(line,CF_BUFSIZE,fp);
    type[0] = '\0';
    arg[0] = '\0';
    sscanf(line,"%63s %s",type,arg);
    
    switch (ConvertMethodProto(type))
       {
       case cfmeth_name:
           if (strcmp(arg,ptr->name) != 0)
              {
              snprintf(OUTPUT,CF_BUFSIZE,"Method name %s did not match package name %s",ptr->name,arg);
              CfLog(cfinform,OUTPUT,"");
              fclose(fp);
              return returnfile;
              }
           break;
           
       case cfmeth_file:
           
           memset(ebuff,0,CF_BUFSIZE);
           
           if (GetMacroValue(CONTEXTID,"moduledirectory"))
              {
              ExpandVarstring("$(moduledirectory)",ebuff,NULL);
              }
           else
              {
              snprintf(ebuff,CF_BUFSIZE,"%s/modules",VLOCKDIR);
              }
           
           snprintf(cffilename,CF_BUFSIZE-1,"%s/%s",ebuff,arg);
           
           if (lstat(cffilename,&statbuf) != -1)
              {
              if (S_ISLNK(statbuf.st_mode))
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"SECURITY ALERT. Method (%s) source is a symbolic link",ptr->name);
                 CfLog(cferror,OUTPUT,"");
                 fclose(fp);
                 return returnfile;
                 }
              }
           
           if (stat(cffilename,&statbuf) == -1)
              {
              snprintf(OUTPUT,CF_BUFSIZE,"Method name %s did not find package name %s",ptr->name,cffilename);
              CfLog(cfinform,OUTPUT,"stat");
              fclose(fp);
              return returnfile;
              }
           
           strncpy(returnfile,cffilename,CF_MAXVARSIZE);
           break;
       case cfmeth_attacharg:
           argnum++;
           break;
       default:
           break;
       }
    }
 
 fclose(fp);
 
 if (numargs != argnum)
    {
    Verbose("Method (%s) arguments did not agree with the agreed template in the config file (%d/%d)\n",ptr->name,argnum,numargs);
    return returnfile;
    }
 
return returnfile; 
}

/******************************************************************************/
/* Level 3                                                                    */
/******************************************************************************/

enum methproto ConvertMethodProto(char *name)

{ int i;
for (i = 0; VMETHODPROTO[i] != NULL; i++)
   {
   if (strcmp(name,VMETHODPROTO[i]) == 0)
      {
      break;
      }
   }

return (enum methproto) i;
}

/*****************************************************************************/

int CheckForMethodPackage(char *methodname)

{ DIR *dirh;
  struct Method *mp = NULL; 
  struct dirent *dirp;
  struct Item *methodID = NULL;
  char dirname[CF_MAXVARSIZE],path[CF_BUFSIZE];
  char name[CF_BUFSIZE],client[CF_BUFSIZE],server[CF_BUFSIZE],digeststring[CF_BUFSIZE],extra[256];
  struct stat statbuf;
  int gotmethod = false;
  
snprintf(dirname,CF_MAXVARSIZE-1,"%s/rpc_in",VLOCKDIR);
 
if ((dirh = opendir(dirname)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",dirname);
   CfLog(cfverbose,OUTPUT,"opendir");
   return false;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,dirname,NULL))
      {
      continue;
      }

   snprintf(path,CF_BUFSIZE-1,"%s/%s",dirname,dirp->d_name);

   if (stat(path,&statbuf) == -1)
      {
      continue;
      }

   Debug("Examining child's incoming (%s)\n",dirp->d_name);

   SplitMethodName(dirp->d_name,client,server,name,digeststring,extra);

   Debug("This request came from %s - our reply should be sent there!\n",client);
   strcpy(METHODREPLYTO,client);

   Debug("This request referred to us as %s - a plausible identity\n",client);
   strcpy(METHODFOR,server);

   if (strcmp(methodname,name) == 0)
      {
      gotmethod = true;
      break;
      }

   if (mp = IsDefinedMethod(name,digeststring))
      {
      if (statbuf.st_mtime < (CFSTARTTIME - (mp->expireafter * 60)))
         {
         Debug("Purging expired method %s\n",path);
         unlink(path);
         continue;
         }
      }
   else
      {
      if (statbuf.st_mtime < (CFSTARTTIME - 2*3600)) /* Default 2 hr expiry */
         {
         Debug("Purging expired method %s\n",path);
         unlink(path);
         continue;
         }
      }
   
   DeleteItemList(methodID);
   }
 
 closedir(dirh); 
 
if (gotmethod)
   {
   return true;
   }
else
   {
   return false;
   }
}

/*****************************************************************************/

struct Method *IsDefinedMethod(char *name,char *digeststring)

{ struct Method *mp;

for (mp = VMETHODS; mp != NULL; mp=mp->next)
   {
   Debug("Check for defined method (%s=%s)(%s)\n",name,mp->name,digeststring);
   
   if (strncmp(mp->name,name,strlen(mp->name)) == 0)
      {
      Debug("  Comparing digests: %s=%s\n",digeststring,ChecksumPrint('m',mp->digest));
      if (strcmp(digeststring,ChecksumPrint('m',mp->digest)) == 0)
         {
         Debug("Method %s is defined\n",name);
         return mp;
         }
      else
         {
         Verbose("Method %s found, but arguments did not match\n",name);
         }
      }
   }

Verbose("Method %s not defined\n",name); 
return NULL; 
}

/*****************************************************************************/

int CountAttachments(char *basepackage)

{ FILE *fp;
 int argnum = 0;
 char line[CF_BUFSIZE],type[CF_BUFSIZE],arg[CF_BUFSIZE];

Debug("CountAttachments(%s)\n",basepackage);
 
if ((fp = fopen(basepackage,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not open a parameter bundle (%s) while counting attachments\n",basepackage);
   CfLog(cfinform,OUTPUT,"fopen");
   return 0;
   }
 
while (!feof(fp))
   {
   memset(line,0,CF_BUFSIZE);
   fgets(line,CF_BUFSIZE,fp);
   type[0] = '\0';
   arg[0] = '\0';
   
   sscanf(line,"%63s %s",type,arg);
   
   switch (ConvertMethodProto(type))
      {
      case cfmeth_attacharg:      
          argnum++;
          break;
      }
   }
 
fclose(fp);
return argnum;
}

/******************************************************************************/
/* Tool level                                                                 */
/******************************************************************************/

void EncapsulateMethod(struct Method *ptr,char *name)

{ struct Item *ip;
  char expbuf[CF_EXPANDSIZE],filename[CF_BUFSIZE];
  int i = 0;
  FILE *fp;

if (strstr(name,":Reply")) /* TODO - check if remote spoofing this could be dangerous*/
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Method name (%s) should not contain a reply signal",name);
   CfLog(cferror,OUTPUT,"");
   }

if ((fp = fopen(name,"w")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could not dispatch method to %s\n",name);
   CfLog(cferror,OUTPUT,"fopen");
   return;
   }

 Debug("EncapsulatingMethod(%s)\n",name);
 
 fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_name],ptr->name);
 fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_file],ptr->file);
 fprintf(fp,"%s %lu\n",VMETHODPROTO[cfmeth_time],(unsigned long)CFSTARTTIME);
 
 if (ptr->servers == NULL || ptr->servers->name == NULL)
    {
    fprintf(fp,"%s localhost\n",VMETHODPROTO[cfmeth_replyto]);
    }
 else if (strcmp(ptr->servers->name,"localhost") == 0)
    {
    fprintf(fp,"%s localhost\n",VMETHODPROTO[cfmeth_replyto]);
    }
 else
    {
    /* Send FQNAME or VIPADDRESS or IPv6? This is difficult. Best to send IP to avoid
     lookups, and hosts might have a fixed name but variable IP ... get it from interface?
     unfortunately can't do this for IPv6 yet -- leave that for another day.*/

    if (strlen(ptr->forcereplyto) > 0)
       {
       fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_replyto],ptr->forcereplyto);
       }
    else
       {
       fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_replyto],VIPADDRESS);
       }
    }

 for (ip = ptr->send_classes; ip != NULL; ip=ip->next)
    {
    /* If classX is defined locally, activate class method_classX in method */
    if (IsDefinedClass(ip->name))
       {
       fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_sendclass],ip->name);
       }
    }

 for (ip = ptr->send_args; ip != NULL; ip=ip->next)
    {
    i++;

    if (strstr(name,"rpc_out"))  /* Remote copy needs to be transferred to incoming for later .. */
       {
       char correction[CF_BUFSIZE];
       snprintf(correction,CF_BUFSIZE-1,"%s/rpc_in/%s",VLOCKDIR,name+strlen(VLOCKDIR)+2+strlen("rpc_out"));
       fprintf(fp,"%s %s+%d\n",VMETHODPROTO[cfmeth_attacharg],correction,i);
       }
    else
       {
       fprintf(fp,"%s %s+%d\n",VMETHODPROTO[cfmeth_attacharg],name,i);
       }
    }

 fclose(fp);

 Debug("Packaging done\n");
 
 /* Now open a new file for each argument - this provides more policy control and security
    for remote access, since file sizes can be limited and there is less chance of buffer
    overflows */

 i = 0;
 
 for (ip = ptr->send_args; ip != NULL; ip=ip->next)
    {
    i++;    
    snprintf(filename,CF_BUFSIZE-1,"%s+%d",name,i);
    Debug("Create arg_file %s\n",filename);
    
    if (IsBuiltinFunction(ip->name))
       {
       char name[CF_BUFSIZE],args[CF_BUFSIZE],value[CF_BUFSIZE],sourcefile[CF_BUFSIZE];
       char *sp,maxbytes[CF_BUFSIZE],*nf,*nm;
       int count = 0,val = 0;
       FILE *fin,*fout;

       /*Expand var 2dim*/
       
       Debug("Evaluating readfile inclusion...\n");
       sscanf(ip->name,"%255[^(](%255[^)])",name,args);
       ExpandVarstring(name,expbuf,NULL);
       
       switch (FunctionStringToCode(expbuf))
          {      
          case fn_readfile:
              
              sourcefile[0] = '\0';
              maxbytes[0] = '\0';
              
              for (sp = args; *sp != '\0'; sp++)
                 {
                 if (*sp == ',')
                    {
                    count++;
                    }
                 }
              
              if (count != 1)
                 {
                 yyerror("ReadFile(filename,maxbytes): argument error");
                 return;
                 }
              
              sscanf(args,"%[^,],%[^)]",sourcefile,maxbytes);
              Debug("ReadFile [%s] < [%s]\n",sourcefile,maxbytes);
              
              if (sourcefile[0]=='\0' || maxbytes[0] == '\0')
                 {
                 yyerror("Argument error in class-function");
                 return;
                 }
              
              nf = UnQuote(sourcefile);
              nm = UnQuote(maxbytes);
              
              val = atoi(nm);
              
              if ((fin = fopen(nf,"r")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"could not open ReadFile(%s)\n",nf);
                 CfLog(cferror,OUTPUT,"fopen");
                 return;
                 }
              
              Debug("Writing file to %s\n",filename);
              
              if ((fout = fopen(filename,"w")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"Could not open argument attachment %s\n",filename);
                 CfLog(cferror,OUTPUT,"fopen");
                 fclose(fin);
                 return;
                 }
              
              if (val > CF_BUFSIZE)
                 {
                 int count = 0;
                 while(!feof(fin))
                    {
                    fputc(fgetc(fin),fout);
                    count++;
                    if (count == val)
                       {
                       break;
                       }
                    Debug("Wrote #\n");
                    }
                 }
              else
                 {
                 memset(value,0,CF_BUFSIZE); 
                 fread(value,val,1,fin);
                 fwrite(value,val,1,fout);
                 Debug("Wrote #\n");
                 }
              
              fclose(fin);
              fclose(fout);
              Debug("Done with file dispatch\n");
              break;
              
          default:
              snprintf(OUTPUT,CF_BUFSIZE,"Invalid function (%s) used in function argument processing",expbuf);
              CfLog(cferror,OUTPUT,"");
              
          }
       
       }
    else
       {
       if ((fp = fopen(filename,"w")) == NULL)
          {
          snprintf(OUTPUT,CF_BUFSIZE,"Could dispatch method to %s\n",name);
          CfLog(cferror,OUTPUT,"fopen");
          return;
          }
       
       ExpandVarstring(ip->name,expbuf,NULL);
       fwrite(expbuf,strlen(expbuf),1,fp);       
       fclose(fp);   
       }
    }
}


/******************************************************************************/

void EncapsulateReply(char *name)

{ FILE *fp;
  struct Item *ip;
  char expbuf[CF_EXPANDSIZE],filename[CF_BUFSIZE];
  int i = 0;

if ((fp = fopen(name,"w")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Could dispatch method to %s\n",name);
   CfLog(cferror,OUTPUT,"fopen");
   return;
   }

 Debug("EncapsulatingReply(%s)\n",name);
 
 fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_name],METHODNAME);
 fprintf(fp,"%s %lu\n",VMETHODPROTO[cfmeth_time],(unsigned long)CFSTARTTIME);
 fprintf(fp,"%s true\n",VMETHODPROTO[cfmeth_isreply]);

 Verbose("Checking whether to define any classes ...\n");
 
 for (ip = METHODRETURNCLASSES; ip != NULL; ip=ip->next)
    {
    /* If classX is defined locally, activate class method_classX in method */
    Debug("Is class %s defined in method?\n",ip->name);
    
    if (IsDefinedClass(ip->name))
       {
       Verbose("Packaging return class %s\n",ip->name);
       fprintf(fp,"%s %s\n",VMETHODPROTO[cfmeth_sendclass],ip->name);
       }
    else
       {
       Verbose("Not defining return class %s\n",ip->name);
       }
    }

 DeleteItemList(ip);
 
 for (ip = METHODRETURNVARS; ip != NULL; ip=ip->next)
    {
    i++;
    ExpandVarstring(ip->name,expbuf,NULL);
    
    if (strstr(name,"rpc_out"))  /* Remote copy needs to be transferred to incoming for later .. */
       {
       char correction[CF_BUFSIZE];
       snprintf(correction,CF_BUFSIZE-1,"%s/rpc_in/%s",VLOCKDIR,name+strlen(VLOCKDIR)+2+strlen("rpc_out"));
       fprintf(fp,"%s %s+%d (%s)\n",VMETHODPROTO[cfmeth_attacharg],correction,i,expbuf);
       }
    else
       {
       fprintf(fp,"%s %s+%d (%s)\n",VMETHODPROTO[cfmeth_attacharg],name,i,expbuf);
       }
    }

 DeleteItemList(ip);
 fclose(fp);

 Debug("Packaging done\n");
 
 /* Now open a new file for each argument - this provides more policy control and security
    for remote access, since file sizes can be limited and there is less chance of buffer
    overflows */

 i = 0;
 
 for (ip = METHODRETURNVARS; ip != NULL; ip=ip->next)
    {
    i++;    
    snprintf(filename,CF_BUFSIZE-1,"%s+%d",name,i);
    Debug("Create arg_file %s\n",filename);
    
    if (IsBuiltinFunction(ip->name))
       {
       char name[CF_BUFSIZE],args[CF_BUFSIZE],value[CF_BUFSIZE],sourcefile[CF_BUFSIZE];
       char *sp,maxbytes[CF_BUFSIZE],*nf,*nm;
       int count = 0,val = 0;
       FILE *fin,*fout;

       Debug("Evaluating readfile inclusion...\n");
       sscanf(ip->name,"%255[^(](%255[^)])",name,args);
       ExpandVarstring(name,expbuf,NULL);

       switch (FunctionStringToCode(expbuf))
          {      
          case fn_readfile:
              
              sourcefile[0] = '\0';
              maxbytes[0] = '\0';
              
              for (sp = args; *sp != '\0'; sp++)
                 {
                 if (*sp == ',')
                    {
                    count++;
                    }
                 }
              
              if (count != 1)
                 {
                 yyerror("ReadFile(filename,maxbytes): argument error");
                 return;
                 }
              
              sscanf(args,"%[^,],%[^)]",sourcefile,maxbytes);
              Debug("ReadFile [%s] < [%s]\n",sourcefile,maxbytes);
              
              if (sourcefile[0]=='\0' || maxbytes[0] == '\0')
                 {
                 yyerror("Argument error in class-function");
                 return;
                 }
              
              nf = UnQuote(sourcefile);
              nm = UnQuote(maxbytes);
              
              val = atoi(nm);
              
              if ((fin = fopen(nf,"r")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"could not open ReadFile(%s)\n",nf);
                 CfLog(cferror,OUTPUT,"fopen");
                 return;
                 }
              
              if ((fout = fopen(filename,"w")) == NULL)
                 {
                 snprintf(OUTPUT,CF_BUFSIZE,"Could not open argument attachment %s\n",filename);
                 CfLog(cferror,OUTPUT,"fopen");
                 fclose(fin);
                 return;
                 }
              
              if (val > CF_BUFSIZE)
                 {
                 int count = 0;
                 while(!feof(fin))
                    {
                    fputc(fgetc(fin),fout);
                    count++;
                    if (count == val)
                       {
                       break;
                       }      
                    }
                 }
              else
                 {
                 memset(value,0,CF_BUFSIZE); 
                 fread(value,val,1,fin);
                 fwrite(value,val,1,fout);
                 }
              fclose(fin);
              fclose(fout);
              break;
              
          default:
              snprintf(OUTPUT,CF_BUFSIZE,"Invalid function (%s) used in function argument processing",expbuf);
              CfLog(cferror,OUTPUT,"");
              
          }
       
       }
    else
       {
       if ((fp = fopen(filename,"w")) == NULL)
          {
          snprintf(OUTPUT,CF_BUFSIZE,"Could dispatch method to %s\n",name);
          CfLog(cferror,OUTPUT,"fopen");
          return;
          }
       
       ExpandVarstring(ip->name,expbuf,NULL);
       fwrite(expbuf,strlen(expbuf),1,fp);       
       fclose(fp);   
       }
    }
 
DeleteItemList(ip);
}

/******************************************************************************/

void SplitMethodName(char *name,char *client,char *server,char *methodname,char *digeststring,char *extra)

{
methodname[0] = '\0';
client[0] = '\0';
server[0] = '\0';
digeststring[0] = '\0';
extra[0] = '\0';
 
sscanf(name,"%1024[^+]+%1024[^+]+%1024[^+]+%[^+]+%64s",client,server,methodname,digeststring,extra);

Debug("Method client -> %s\n",client);
Debug("Method server -> %s\n",server);
Debug("Method name -> %s\n",methodname);
Debug("Method digeststring -> %s\n",digeststring);
Debug("Method extra -> %s\n",extra);
}
