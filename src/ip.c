/* 

        Copyright (C) 1995-
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
/* File: ip.c                                                                */
/*                                                                           */
/* Created: Sun Feb 17 13:47:19 2002                                         */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*****************************************************************************/

int RemoteConnect(char *host,char forceipv4,short oldport, char *newport) 

{ int err;

#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
 
if (forceipv4 == 'n')
   {
   struct addrinfo query, *response, *ap;
   struct addrinfo query2, *response2, *ap2;
   int err,connected = false;
   
   memset(&query,0,sizeof(struct addrinfo));   

   query.ai_family = AF_UNSPEC;
   query.ai_socktype = SOCK_STREAM;

   if ((err = getaddrinfo(host,newport,&query,&response)) != 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to find hostname or cfengine service: (%s/%s) %s",host,STR_CFENGINEPORT,gai_strerror(err));
      CfLog(cfinform,OUTPUT,"");
      return false;
      }
   
   for (ap = response; ap != NULL; ap = ap->ai_next)
      {
      Verbose("Connect to %s = %s on port %s\n",host,sockaddr_ntop(ap->ai_addr),newport);
      
      if ((CONN->sd = socket(ap->ai_family,ap->ai_socktype,ap->ai_protocol)) == -1)
         {
         CfLog(cfinform,"Couldn't open a socket","socket");      
         continue;
         }
      
      if (BINDINTERFACE[0] != '\0')
         {
         memset(&query2,0,sizeof(struct addrinfo));   
         
         query.ai_family = AF_UNSPEC;
         query.ai_socktype = SOCK_STREAM;
         
         if ((err = getaddrinfo(BINDINTERFACE,NULL,&query2,&response2)) != 0)
            {
            snprintf(OUTPUT,CF_BUFSIZE,"Unable to lookup hostname or cfengine service: %s",gai_strerror(err));
            CfLog(cferror,OUTPUT,"");
            return false;
            }
         
         for (ap2 = response2; ap2 != NULL; ap2 = ap2->ai_next)
            {
            if (bind(CONN->sd, ap2->ai_addr, ap2->ai_addrlen) == 0)
               {
               freeaddrinfo(response2);
               response2 = NULL;
               break;
               }
            }
         
         if (response2)
            {
            freeaddrinfo(response2);
            }
         }
      
      signal(SIGALRM,(void *)TimeOut);
      alarm(CF_TIMEOUT);
      
      if (connect(CONN->sd,ap->ai_addr,ap->ai_addrlen) >= 0)
         {
         connected = true;
         alarm(0);
         signal(SIGALRM,SIG_DFL);
         break;
         }
      
      alarm(0);
      signal(SIGALRM,SIG_DFL);
      }
   
   if (connected)
      {
      CONN->family = ap->ai_family;
      snprintf(CONN->remoteip,CF_MAX_IP_LEN-1,"%s",sockaddr_ntop(ap->ai_addr));
      }
   else
      {
      close(CONN->sd);
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't connect to host %s\n",host);
      CONN->sd = CF_NOT_CONNECTED;
      }
   
   if (response != NULL)
      {
      freeaddrinfo(response);
      }
   
   if (!connected)
      {
      return false;
      }
   }
 
 else
     
#endif /* ---------------------- only have ipv4 ---------------------------------*/ 

   {
   struct hostent *hp;
   struct sockaddr_in cin;
   memset(&cin,0,sizeof(cin));
   
   if ((hp = gethostbyname(host)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to look up IP address of %s",host);
      CfLog(cferror,OUTPUT,"gethostbyname");
      return false;
      }
   
   cin.sin_port = oldport;
   cin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   cin.sin_family = AF_INET; 
   
   Verbose("Connect to %s = %s, port =%u\n",host,inet_ntoa(cin.sin_addr),SHORT_CFENGINEPORT);
    
   if ((CONN->sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
      {
      CfLog(cferror,"Couldn't open a socket","socket");
      return false;
      }

   if (BINDINTERFACE[0] != '\0')
      {
      Verbose("Cannot bind interface with this OS.\n");
      /* Could fix this - any point? */
      }
   
   CONN->family = AF_INET;
   snprintf(CONN->remoteip,CF_MAX_IP_LEN-1,"%s",inet_ntoa(cin.sin_addr));
    
   signal(SIGALRM,(void *)TimeOut);
   alarm(CF_TIMEOUT);
    
   if (err=connect(CONN->sd,(void *)&cin,sizeof(cin)) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't connect to host %s\n",host);
      CfLog(cfinform,OUTPUT,"connect");
      return false;
      }
   
   alarm(0);
   signal(SIGALRM,SIG_DFL);
   }

LastSeen(host,cf_connect);
return true; 
}



/*******************************************************************/

void CfenginePort()

{ struct servent *server;
 
if ((server = getservbyname(CFENGINE_SERVICE,"tcp")) == NULL)
   {
   CfLog(cflogonly,"Couldn't get cfengine service, using default","getservbyname");
   SHORT_CFENGINEPORT = htons((unsigned short)5308);
   }
else
   {
   SHORT_CFENGINEPORT = server->s_port;
   }

Verbose("Setting cfengine new port to %u\n",SHORT_CFENGINEPORT);
}

/*****************************************************************************/

void StrCfenginePort()

{ struct servent *server;

if ((server = getservbyname(CFENGINE_SERVICE,"tcp")) == NULL)
   {
   CfLog(cflogonly,"Couldn't get cfengine service, using default","getservbyname");
   snprintf(STR_CFENGINEPORT,15,"5308");
   }
else
   {
   snprintf(STR_CFENGINEPORT,15,"%d",ntohs(server->s_port));
   }

Verbose("Setting cfengine old port to %s\n",STR_CFENGINEPORT);
}

/*****************************************************************************/

char *Hostname2IPString(char *hostname)

{ static char ipbuffer[CF_SMALLBUF];
  int err;
 
#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)

 struct addrinfo query, *response, *ap;

 memset(&query,0,sizeof(struct addrinfo));   
 query.ai_family = AF_UNSPEC;
 query.ai_socktype = SOCK_STREAM;

 memset(ipbuffer,0,CF_SMALLBUF-1);
 
if ((err = getaddrinfo(hostname,NULL,&query,&response)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unable to lookup hostname (%s) or cfengine service: %s",hostname,gai_strerror(err));
   CfLog(cferror,OUTPUT,"");
   return hostname;
   }
 
for (ap = response; ap != NULL; ap = ap->ai_next)
   {
   strncpy(ipbuffer,sockaddr_ntop(ap->ai_addr),64);
   Debug("Found address (%s) for host %s\n",ipbuffer,hostname);

   if (strlen(ipbuffer) == 0)
      {
      snprintf(ipbuffer,CF_SMALLBUF-1,"Empty IP result for %s",hostname);
      }
   freeaddrinfo(response);   
   return ipbuffer;
   }
#else
 struct hostent *hp;
 struct sockaddr_in cin;
 memset(&cin,0,sizeof(cin));

 memset(ipbuffer,0,CF_SMALLBUF-1);

if ((hp = gethostbyname(hostname)) != NULL)
   {
   cin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   strncpy(ipbuffer,inet_ntoa(cin.sin_addr),CF_SMALLBUF-1);
   Verbose("Found address (%s) for host %s\n",ipbuffer,hostname);
   return ipbuffer;
   }
#endif
   
snprintf(ipbuffer,CF_SMALLBUF-1,"Unknown IP %s",hostname);
return ipbuffer;
}


/*****************************************************************************/

char *IPString2Hostname(char *ipaddress)

{ static char hostbuffer[MAXHOSTNAMELEN];
  int err;

#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)

 struct addrinfo query, *response, *ap;

memset(&query,0,sizeof(query));
memset(&response,0,sizeof(response));

query.ai_flags = AI_CANONNAME;

memset(hostbuffer,0,MAXHOSTNAMELEN);

if ((err = getaddrinfo(ipaddress,NULL,&query,&response)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unable to lookup IP address (%s): %s",ipaddress,gai_strerror(err));
   CfLog(cferror,OUTPUT,"");
   snprintf(hostbuffer,MAXHOSTNAMELEN-1,"(Non registered IP)"); 
   return hostbuffer;
   }

for (ap = response; ap != NULL; ap = ap->ai_next)
   {   
   if ((err = getnameinfo(ap->ai_addr,ap->ai_addrlen,hostbuffer,MAXHOSTNAMELEN,0,0,0)) != 0)
      {
      snprintf(hostbuffer,MAXHOSTNAMELEN-1,"(Non registered IP)");
      freeaddrinfo(response);
      return hostbuffer;
      }
   
   Debug("Found address (%s) for host %s\n",hostbuffer,ipaddress);
   freeaddrinfo(response);
   return hostbuffer;
   }

 snprintf(hostbuffer,MAXHOSTNAMELEN-1,"(Non registered IP)");
 
#else

struct hostent *hp;
struct sockaddr_in myaddr;
struct in_addr iaddr;
  
memset(hostbuffer,0,MAXHOSTNAMELEN);

if ((iaddr.s_addr = inet_addr(ipaddress)) != -1)
   {
   hp = gethostbyaddr((void *)&iaddr,sizeof(struct sockaddr_in),AF_INET);
  
   if ((hp == NULL) || (hp->h_name == NULL))
      {
      strcpy(hostbuffer,"(Non registered IP)");
      return hostbuffer;
      }

   strncpy(hostbuffer,hp->h_name,MAXHOSTNAMELEN-1);
   }
else
   {
   strcpy(hostbuffer,"(non registered IP)");
   }

#endif

return hostbuffer;
}



/*********************************************************************/

struct cfagent_connection *NewAgentConn()

{ struct cfagent_connection *ap;

if ((ap = (struct cfagent_connection *)malloc(sizeof(struct cfagent_connection))) == NULL)
   {
   return NULL;
   }

Debug("New server connection...\n");
ap->sd = CF_NOT_CONNECTED;
ap->family = AF_INET; 
ap->trust = false;
ap->localip[0] = '\0';
ap->remoteip[0] = '\0';
ap->session_key = NULL;
ap->error = false; 
return ap;
};

/*********************************************************************/

void DeleteAgentConn(struct cfagent_connection *ap)

{
if (ap->session_key != NULL)
   {
   free(ap->session_key);
   }

free(ap);
ap = NULL; 
}

