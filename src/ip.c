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

int RemoteConnect(host,forceipv4)          /* Handle ipv4 or ipv6 connection */

char *host, forceipv4;

{ int err;

#ifdef HAVE_GETADDRINFO /* --------------we have ipv6 ------------------- */
 
if (forceipv4 == 'n')
   {
   struct addrinfo query, *response, *ap;
   int err,connected = false;
   
   bzero(&query,sizeof(struct addrinfo));   

   query.ai_family = AF_UNSPEC;
   query.ai_socktype = SOCK_STREAM;
   
   if ((err = getaddrinfo(host,"5308",&query,&response)) != 0)
      {
      snprintf(OUTPUT,bufsize,"Unable to lookup hostname or cfengine service: %s",gai_strerror(err));
      CfLog(cferror,OUTPUT,"");
      return false;
      }
   
   for (ap = response; ap != NULL; ap = ap->ai_next)
      {
      Verbose("Connect to %s = %s on port cfengine\n",host,sockaddr_ntop(ap->ai_addr));
      
      if ((CONN->sd = socket(ap->ai_family,ap->ai_socktype,ap->ai_protocol)) == -1)
	 {
	 CfLog(cfinform,"Couldn't open a socket","socket");      
	 continue;
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
      snprintf(CONN->remoteip,cfmaxiplen-1,"%s",sockaddr_ntop(ap->ai_addr));
      }
   else
      {
      close(CONN->sd);
      snprintf(OUTPUT,bufsize*2,"Couldn't connect to host %s\n",host);
      CONN->sd = cf_not_connected;
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
   bzero(&cin,sizeof(cin));
   
   if ((hp = gethostbyname(host)) == NULL)
      {
      snprintf(OUTPUT,bufsize,"Unable to look up IP address of %s",host);
      CfLog(cferror,OUTPUT,"gethostbyname");
      return false;
      }
   
   cin.sin_port = CfenginePort();
   cin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   cin.sin_family = AF_INET; 
   
   Verbose("Connect to %s = %s, port h=%d\n",host,inet_ntoa(cin.sin_addr),htons(PORTNUMBER));
    
   if ((CONN->sd = socket(AF_INET,SOCK_STREAM,0)) == -1)
      {
      CfLog(cferror,"Couldn't open a socket","socket");
      return false;
      }
   
   CONN->family = AF_INET;
   snprintf(CONN->remoteip,cfmaxiplen-1,"%s",inet_ntoa(cin.sin_addr));
    
   signal(SIGALRM,(void *)TimeOut);
   alarm(CF_TIMEOUT);
    
   if (err=connect(CONN->sd,(void *)&cin,sizeof(cin)) == -1)
      {
      snprintf(OUTPUT,bufsize*2,"Couldn't connect to host %s\n",host);
      CfLog(cfinform,OUTPUT,"connect");
      return false;
      }
   
   alarm(0);
   signal(SIGALRM,SIG_DFL);
   }
return true; 
}



/*******************************************************************/

short CfenginePort()

{
struct servent *server;

if ((server = getservbyname(CFENGINE_SERVICE,"tcp")) == NULL)
   {
   CfLog(cflogonly,"Couldn't get cfengine service, using default","getservbyname");
   return htons(5308);
   }

return server->s_port;
}


/*******************************************************************/

int IsIPV6Address(name)

/* make this more reliable ... does anyone have : in hostname? */

char *name;

{
if (strstr(name,":") == NULL)
   {
   return false;
   }
 
return true;
}

