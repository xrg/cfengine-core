/* 

        Copyright (C) 1995,96,97
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
/*  Cfengine : remote client example                               */
/*                                                                 */
/*  Mark Burgess 1997                                              */
/*  modified : Bas van der Vlies <basv@sara.nl> 1998               */
/*  modified : Andrew Mayhew <amayhew@icewire.com> 2000            */
/*  modified : Mark 2002, Bas 2002                                 */
/*                                                                 */
/*******************************************************************/

#define FileVerbose if (VERBOSE || DEBUG || D2) fprintf

#include "cf.defs.h"
#include "cf.extern.h"

int  MAXCHILD = 1;
int  FileFlag = 0;
int  TRUSTALL = false;
char OUTPUTDIR[bufsize];

struct Item *VCFRUNCLASSES = NULL;
struct Item *VCFRUNOPTIONHOSTS = NULL;
struct Item *VCFRUNHOSTLIST = NULL;

char VCFRUNHOSTS[bufsize] = "cfrun.hosts";
char CFRUNOPTIONS[bufsize];
char CFLOCK[bufsize] = "dummy";

/*******************************************************************/
/* Functions internal to cfrun.c                                   */
/*******************************************************************/

void CheckOptsAndInit  ARGLIST((int argc,char **argv));
int PollServer ARGLIST((char *host, char *options, int StoreInFile));
void SendClassData ARGLIST((int sd, char *sendbuffer));
void CheckAccess ARGLIST((char *users));
void cfrunSyntax ARGLIST((void));
void ReadCfrunConf ARGLIST((void));
int ParseHostname ARGLIST((char *hostname, char *new_hostname));

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (argc,argv)

int argc;
char **argv;

{ struct Item *ip;
  int 	 i=0;
  int	 status;
  int	 pid;

CheckOptsAndInit(argc,argv);

ip = VCFRUNHOSTLIST;

while (ip != NULL)
   {
   if (i < MAXCHILD)
      {
      if (fork() == 0) /* child */
	 {
	 printf("cfrun(%d):         .......... [ Hailing %s ] ..........\n",i,ip->name);
	 Debug("pid = %d i = %d\n", getpid(), i);
	 
	 if (PollServer(ip->name,ip->classes, FileFlag))
	    {
	    Verbose("Connection with %s completed\n\n",ip->name);
	    }
	 else
	    {
	    Verbose("Connection refused...\n\n");
	    }
	 exit(0);
	 }
      else
	 {
	 /* parent */
	 i++;
	 }
      }
   else 
      {
      pid = wait(&status);
      Debug("wait result pid = %d number %d\n", pid, i);
      i--;
      }
   if ( i < MAXCHILD )
      {
      ip = ip->next;
      }
   } 
 
 while (i > 0)
    {
    pid = wait(&status);
    Debug("Child pid = %d ended\n", pid);
    i--;
    }
 
 exit(0);
 return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(argc,argv)

int argc;
char **argv;

{ int optgroup = 0, i;
  struct Item *ip;

/* Separate command args into options and classes */
bzero(CFRUNOPTIONS,bufsize);

for (i = 1; i < argc; i++) 
   {
   if (optgroup == 0)
      {	 
      if (strncmp(argv[i],"-h",2) == 0)		 
	 {
	 cfrunSyntax();
	 }
      else if (strncmp(argv[i],"-f",2) == 0)
	 {
	 i++;
	 
	 if ((i >= argc) || (strncmp(argv[i],"-",1) == 0))
	    {
	    printf("Error: No filename listed after -f option.\n");
	    cfrunSyntax();
	    exit(0);
	    }
	 bzero(VCFRUNHOSTS,bufsize);
	 strcat(VCFRUNHOSTS,argv[i]);
	 Debug("cfrun: cfrun file = %s\n",VCFRUNHOSTS);
	 }
      else if (strncmp(argv[i],"-d",2) == 0)
	 {
	 DEBUG = true;
	 VERBOSE = true;
	 }
      else if (strncmp(argv[i],"-v",2) == 0)
	 {
	 VERBOSE=true;
	 }
      else if (strncmp(argv[i],"-T",2) == 0)
	 {
	 TRUSTALL = true;
	 }
      else if (strncmp(argv[i],"-S",2) == 0)
	 {
	 SILENT = true;
	 }
      else if (strncmp(argv[i],"--",2) == 0) 
	 {
	 optgroup++;
	 }
      else if (argv[i][0] == '-')
	 {
	 printf("Error: Unknown option.\n");
	 cfrunSyntax();
	 exit(0);
	 }
      else
	 {
	 AppendItem(&VCFRUNOPTIONHOSTS,argv[i],NULL); /* Restrict run hosts */
	 }
      }
   else if (optgroup == 1) 
      {
      if (strncmp(argv[i],"--",2) == 0)
	 {
	 optgroup++;
	 }
      else
	 {
	 strcat(CFRUNOPTIONS,argv[i]);
	 strcat(CFRUNOPTIONS," ");
	 }
      }
   else
      {
      AppendItem(&VCFRUNCLASSES,argv[i],"");
      }
   }
 
Debug("CFRUNOPTIONS string: %s\n",CFRUNOPTIONS);
   
for (ip = VCFRUNCLASSES; ip != NULL; ip=ip->next)
   {
   Debug("Class item: %s\n",ip->name);
   }

ReadCfrunConf(); 

GetNameInfo();

/* 
if (uname(&VSYSNAME) == -1)
   {
   perror("uname ");
   printf("cfrun: uname couldn't get kernel name info!!\n");
   exit(1);
   }

if ((strlen(VDOMAIN) > 0) && !strchr(VSYSNAME.nodename,'.'))
   {
   sprintf(VFQNAME,"%s.%s",VSYSNAME.nodename,VDOMAIN);
   }
else
   {
   sprintf(VFQNAME,"%s",VSYSNAME.nodename);
   }   
*/
 
 Debug("FQNAME = %s\n",VFQNAME);
 
sprintf(VPREFIX,"cfrun:%s",VFQNAME);
 
/* Read hosts file */

umask(077);
strcpy(VLOCKDIR,WORKDIR);
strcpy(VLOGDIR,WORKDIR); 

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();
CheckWorkDirectories();
LoadSecretKeys();
RandomSeed();

}


/********************************************************************/

int PollServer(host,options,StoreInFile)

char *host, *options;
int  StoreInFile;

{ struct hostent *hp;
  struct sockaddr_in raddr;
  char sendbuffer[bufsize],recvbuffer[bufsize];
  char filebuffer[bufsize],parsed_host[bufsize];
  char reply[8];
  struct servent *server;
  int err,n_read,first,port;
  char *sp,forceipv4='n';
  FILE  *fp;
  struct Image addresses;

CONN = NewAgentConn();

if (StoreInFile)
   {
   sprintf(filebuffer, "%s/%s", OUTPUTDIR, host);
   if ((fp = fopen(filebuffer, "w")) == NULL)
      {
      return false;
      }
   }
else
   {
   fp = stdout;
   }
 
port = ParseHostname(host,parsed_host);
 
FileVerbose(fp, "Connecting to server %s to port %d with options %s %s\n",parsed_host,port,options,CFRUNOPTIONS);

if ((hp = gethostbyname(parsed_host)) == NULL)
   {
   printf("Unknown host: %s\n", parsed_host);
   printf("Make sure that fully qualified names can be looked up at your site!\n");
   printf("i.e. www.gnu.org, not just www. If you use NIS or /etc/hosts\n");
   printf("make sure that the full form is registered too as an alias!\n");
   exit(1);
   }

 
bzero(&raddr,sizeof(raddr));
 
if (port)
   {
   raddr.sin_port = htons(port);
   }
else
   {
   if ((server = getservbyname(CFENGINE_SERVICE,"tcp")) == NULL)
      {
      CfLog(cferror,"Unable to find cfengine port","getservbyname");
      exit (1);
      }
   else
      {                                                       
      raddr.sin_port = (unsigned int) server->s_port;
      } 
   }
 
raddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
raddr.sin_family = AF_INET; 

snprintf(CONN->remoteip,cfmaxiplen,"%s",inet_ntoa(raddr.sin_addr));

addresses.trustkey = 'n'; 
addresses.encrypt = 'n';
addresses.server = strdup(parsed_host);
 
snprintf(sendbuffer,bufsize,"root-%s",CONN->remoteip);
 
if (HavePublicKey(sendbuffer) == NULL)
   {
   if (TRUSTALL)
      {
      printf("Accepting public key from %s\n",parsed_host);
      }
   else
      {
      printf("WARNING - You do not have a public key from host %s = %s\n",host,CONN->remoteip);
      printf("          Do you want to accept one on trust? (yes/no)\n\n--> ");
      
      while (true)
	 {
	 fgets(reply,8,stdin);
	 Chop(reply);
	 
	 if (strcmp(reply,"yes")==0)
	    {
	    addresses.trustkey = 'y';
	    break;
	    }
	 else if (strcmp(reply,"no")==0)
	    {
	    break;
	    }
	 else
	    {
	    printf("Please reply yes or no...(%s)\n",reply);
	    }
	 }
      }
   }
 
if (!RemoteConnect(parsed_host,forceipv4))
   {
   CfLog(cferror,"Couldn't open a socket","socket");
   if (CONN->sd != cf_not_connected)
      {
      close(CONN->sd);
      CONN->sd = cf_not_connected;
      }
   free(addresses.server);
   return false;
   }
 
if (!IdentifyForVerification(CONN->sd,CONN->localip,CONN->family))
   {
   printf("Unable to open a channel\n");
   close(CONN->sd);
   errno = EPERM;
   free(addresses.server);
   return false;
   }

if (!KeyAuthentication(&addresses))
   {
   snprintf(OUTPUT,bufsize,"Key-authentication for %s failed\n",VFQNAME);
   CfLog(cferror,OUTPUT,"");
   errno = EPERM;
   close(CONN->sd);
   free(addresses.server);
   return false;
   }
 
snprintf(sendbuffer,bufsize,"EXEC %s %s",options,CFRUNOPTIONS);

if (SendTransaction(CONN->sd,sendbuffer,0,CF_DONE) == -1)
   {
   printf("Transmission rejected");
   close(CONN->sd);
   free(addresses.server);
   return false;
   }

SendClassData(CONN->sd,sendbuffer);

FileVerbose(fp, "%s replies..\n\n",parsed_host);

first = true;
 
while (true)
   {
   bzero(recvbuffer,bufsize);

   if ((n_read = ReceiveTransaction(CONN->sd,recvbuffer,NULL)) == -1)
      {
      if (errno == EINTR) 
         {
         continue;
         }

      close(CONN->sd);
      free(addresses.server);
      return true;;
      }

   if ((sp = strstr(recvbuffer,CFD_TERMINATOR)) != NULL)
      {
      *sp = '\0';
      fprintf(fp,"%s",recvbuffer);
      break;
      }

   if ((sp = strstr(recvbuffer,"BAD:")) != NULL)
      {
      *sp = '\0';
      fprintf(fp,"%s",recvbuffer+4);
      }   

   if (n_read == 0)
      {
      if (!first)
	 {
	 fprintf(fp,"\n");
	 }
      break;
      }

   if (strlen(recvbuffer) == 0)
      {
      continue;
      }

   if (strstr(recvbuffer,"too soon"))
      {
      FileVerbose(fp,"%s",recvbuffer);
      }

   if (strstr(recvbuffer,"cfXen"))
      {
      fprintf(fp,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
      continue;
      }

   if (first)
      {
      fprintf(fp,"\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
      }
   
   first = false;

   fprintf(fp,"%s",recvbuffer);
   }

if (!first)
   {
   fprintf(fp,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
   }
 
close(CONN->sd);
free(addresses.server);
return true;
}

/********************************************************************/
/* Level 2                                                          */
/********************************************************************/


void ReadCfrunConf()

{ char filename[bufsize], *sp;
  char buffer[maxvarsize], options[bufsize], line[bufsize];
  FILE *fp;
  struct Item *ip; 

bzero(filename,bufsize);
  
if (((sp=getenv(CFINPUTSVAR)) != NULL) && (!strchr(VCFRUNHOSTS, '/')))
   {
   strcpy(filename,sp);
   if (filename[strlen(filename)-1] != '/')
      {
      strcat(filename,"/");
      }
   }

strcat(filename,VCFRUNHOSTS);

if ((fp = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   printf("Unable to open %s\n",filename);
   return;
   }

while (!feof(fp))
   {
   bzero(buffer,maxvarsize);
   bzero(options,bufsize);
   bzero(line,bufsize);

   ReadLine(line,bufsize,fp);

   if (strncmp(line,"domain",6) == 0)
      {
      sscanf(line,"domain = %295[^# \n]",VDOMAIN);
      Verbose("Domain name = %s\n",VDOMAIN);
      continue;
      }

   if (strncmp(line,"maxchild", strlen("maxchild")) == 0)
      {
      sscanf(line,"maxchild = %295[^# \n]", buffer);

      if ( (MAXCHILD = atoi(buffer)) == 0 )
	 {
	 MAXCHILD = 1;
	 }

      Verbose("cfrun: maxchild = %d\n", MAXCHILD);
      continue;
      }

   if (strncmp(line,"outputdir", strlen("outputdir")) == 0)
      {
      sscanf(line,"outputdir = %295[^# \n]", OUTPUTDIR);
      Verbose("cfrun: outputdir = %s\n", OUTPUTDIR);
      if ( opendir(OUTPUTDIR) == NULL)
	 {
         printf("Directory %s does not exists\n", OUTPUTDIR);
	 exit(1);
         }

      FileFlag=1;
      continue;
      }

   if (strncmp(line,"access",6) == 0)
      {
      for (sp = line; (*sp != '=') && (*sp != '\0'); sp++)
	 {
	 }

      if (*sp == '\0' || *(++sp) == '\0')
	 {
	 continue;
	 }

      CheckAccess(sp);
      continue;
      }

   sscanf(line,"%295s %[^#\n]",buffer,options);

   if (buffer[0] == '#')
      {
      continue;
      }

   if (strlen(buffer) == 0)
      {
      continue;
      }


   if (VCFRUNOPTIONHOSTS != NULL && !IsItemIn(VCFRUNOPTIONHOSTS,buffer))
      {
      Verbose("Skipping host %s\n",buffer);
      continue;
      }

   if ((!strstr(buffer,".")) && (strlen(VDOMAIN) > 0))
      {
      strcat(buffer,".");
      strcat(buffer,VDOMAIN);
      }
      
   if (!IsItemIn(VCFRUNHOSTLIST,buffer))
      {
      AppendItem(&VCFRUNHOSTLIST,buffer,options);
      }
   }
 
for (ip = VCFRUNHOSTLIST; ip != NULL; ip=ip->next)
   {
   Debug("host item: %s (%s)\n",ip->name,ip->classes);
   }

fclose(fp);
 
}


/********************************************************************/
 
int ParseHostname(hostname,new_hostname)

char *hostname;
char *new_hostname;

{ int port=0;

sscanf(hostname,"%[^:]:%d", new_hostname, &port);

return(port);
}


/********************************************************************/

void SendClassData(sd, sendbuffer)

int sd;
char *sendbuffer;

{ struct Item *ip;
  int used;
  char *sp;

sp = sendbuffer;
used = 0;
bzero(sendbuffer,bufsize);
  
for (ip = VCFRUNCLASSES; ip != NULL; ip = ip->next)
   {
   if (used + strlen(ip->name) +2 > bufsize)
      {
      if (SendTransaction(sd,sendbuffer,0,CF_DONE) == -1)
         {
         perror("send");
         return;
         }

      used = 0;
      sp = sendbuffer;
      bzero(sendbuffer,bufsize);
      }
   
   strcat(sendbuffer,ip->name);
   strcat(sendbuffer," ");

   sp += strlen(ip->name)+1;
   used += strlen(ip->name)+1;
   }

if (used + strlen(CFD_TERMINATOR) +2 > bufsize)
   {
   if (SendTransaction(sd,sendbuffer,0,CF_DONE) == -1)
      {
      perror("send");
      return;
      }

   used = 0;
   sp = sendbuffer;
   bzero(sendbuffer,bufsize);
   }
   
sprintf(sp,CFD_TERMINATOR);

if (SendTransaction(sd,sendbuffer,0,CF_DONE) == -1)
   {
   perror("send");
   return;
   }
}

/********************************************************************/

void CheckAccess(users)

char *users;

{ char id[maxvarsize], *sp;
  struct passwd *pw;
  int uid,myuid;

myuid = getuid();

if (myuid == 0)
   {
   return;
   }

for (sp = users; *sp != '\0'; sp++)
   {
   id[0] = '\0';

   sscanf(sp,"%295[^,:]",id);

   sp += strlen(id);
   
   if (isalpha((int)id[0]))
      {
      if ((pw = getpwnam(id)) == NULL)
	 {
	 printf("cfrun: No such user (%s) in password database\n",id);
	 continue;
	 }
      
      if (pw->pw_uid == myuid)
	 {
	 return;
	 }
      }
   else
      {
      uid = atoi(id);
      if (uid == myuid)
	 {
	 return;
	 }
      }
   }

 printf("cfrun: you have not been granted permission to run cfrun\n");
 exit(0);
}

/********************************************************************/

void cfrunSyntax()

{
 printf("Usage: cfrun [-f cfrun.hosts|-h|-d|-S|-T|-v] [-- OPTIONS [-- CLASSES]]\n\n");
 printf("-f cfrun.hosts\tcfrun file to read in list of hosts (see below for syntax.)\n");
 printf("-h\t\tGet this help message.\n");
 printf("-d\t\tDebug mode, turns on verbose as well.\n");
 printf("-S\t\tSilent mode.\n");
 printf("-T\t\tTrust all incoming public keys.\n");
 printf("-v\t\tVerbose mode.\n");
 printf("-- OPTIONS\tArguments to be passed to host application.\n");
 printf("-- CLASSES\tClasses to be defined for the hosts.\n\n");
 printf("e.g.  cfrun -- -- linux          Run on all linux machines\n");
 printf("      cfrun -- -p                Ping and parse on all hosts\n");
 printf("      cfrun host1 host2 -- -p    Ping and parse on named hosts\n");
 printf("      cfrun -v -- -p             Ping all, local verbose\n");
 printf("      cfrun -v -- -k -- solaris  Local verbose, all solaris, but no copy\n\n");
 printf("cfrun.hosts file syntax:\n");
 printf("# starts a comment\n");
 printf("domain = [domain]\t# Domain to use for connection(s).\n");
 printf("maxchild = [num]\t# Maximum number of children to spawn during run.\n");
 printf("outputdir = [dir]\t# Directory where to put host output files.\n");
 printf("access = [user]\t\t# User allowed to do cfrun?\n");
 printf("[host]\t\t\t# One host per line list to cycle through.\n");
 printf("\t\t\t# Only the hosts are required for cfrun to operate.\n");
 
 exit(0);
}

/*********************************************************************/

struct cfagent_connection *NewAgentConn()

{ struct cfagent_connection *ap = (struct cfagent_connection *)malloc(sizeof(struct cfagent_connection));

ap->sd = cf_not_connected;
ap->family = AF_INET; 
ap->trust = false;
ap->localip[0] = '\0';
ap->remoteip[0] = '\0';
ap->session_key = NULL;
return ap;
};

/*********************************************************************/

void DeleteAgentConn(ap)

struct cfagent_connection *ap;

{
if (ap->session_key != NULL)
   {
   free(ap->session_key);
   }

free(ap);
ap = NULL; 
}

/***************************************************************/
/* Linking simplification                                      */
/***************************************************************/

void FatalError(s)

char *s;

{
exit(1);
}


void RecursiveTidySpecialArea(name,tp,maxrecurse)

char *name;
struct Tidy *tp;
int maxrecurse;

{
}

int CompareMD5Net(file1,file2,ip)

char *file1, *file2;
struct Image *ip;

{
 return 0;
}


void yyerror(s)

char *s;

{
}

/* EOF */

