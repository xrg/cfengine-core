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
 

/*******************************************************************/
/*                                                                 */
/*  INET checking for cfengine                                     */
/*                                                                 */
/*  This is based on the action of "ifconfig" for IP protocols     */
/*  It assumes that we are on the internet and uses ioctl to get   */
/*  the necessary info from the device. Sanity checking is done... */
/*                                                                 */
/* Sockets are very poorly documented. The basic socket adress     */
/* struct sockaddr is a generic type. Specific socket addresses    */
/* must be specified depending on the family or protocol being     */
/* used. e.g. if you're using the internet inet protocol, then     */
/* the fmaily is AF_INT and the socket address type is sockadr_in  */
/* Although it is not obvious, the documentation assures us that   */
/* we can cast a pointer of one type into a pointer of the other:  */
/*                                                                 */
/* Here's an example                                               */
/*                                                                 */
/*   #include <netinet/in.h>                                       */
/*                                                                 */
/*        struct in_addr adr;                                      */
/* e.g.   adr.s_addr = inet_addr("129.240.22.34");                 */
/*        printf("addr is %s\n",inet_ntoa(adr));                   */
/*                                                                 */
/*                                                                 */
/* We have to do the following in order to convert                 */
/* a sockaddr struct into a sockaddr_in struct required by the     */
/* ifreq struct!! These calls have no right to work, but somehow   */
/* they do!                                                        */
/*                                                                 */
/* struct sockaddr_in sin;                                         */
/* sin.sin_addr.s_addr = inet_addr("129.240.22.34");               */
/*                                                                 */
/* IFR.ifr_addr = *((struct sockaddr *) &sin);                     */
/*                                                                 */
/* sin = *(struct sockaddr_in *) &IFR.ifr_addr;                    */
/*                                                                 */
/* printf("IP address: %s\n",inet_ntoa(sin.sin_addr));             */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

#if !defined(NT) && !defined(IRIX)

/* IRIX makes the routing stuff obsolete unless we do this */
# undef sgi

struct ifreq IFR;

char VNUMBROADCAST[256];

# define cfproto 0

# ifndef IPPROTO_IP     /* Old boxes, hpux 7 etc */
#  define IPPROTO_IP 0
# endif

# ifndef SIOCSIFBRDADDR
#  define SIOCSIFBRDADDR  SIOCGIFBRDADDR
# endif

/*******************************************************************/

void IfConf (vifdev,vnetmask,vbroadcast)

char *vifdev,*vnetmask, *vbroadcast;

{ int sk, flags, metric, isnotsane = false;

 
Verbose("Assumed interface name: %s %s %s\n",vifdev,vnetmask,vbroadcast);

if (!IsPrivileged())                            
   {
   printf("%s: Only root can configure the net interface.\n",VPREFIX);
   return;
   }

if (vnetmask[0] == '\0')
   {
   CfLog(cferror,"Program does not define a subnetmask","");
   return;
   }

if (vbroadcast[0] == '\0')
   {
   CfLog(cferror,"Program does not define a broadcast mode for this host","");
   return;
   }


strcpy(IFR.ifr_name,vifdev);
IFR.ifr_addr.sa_family = AF_INET;

if ((sk = socket(AF_INET,SOCK_DGRAM,IPPROTO_IP)) == -1)
   {
   CfLog(cferror,"","socket");
   FatalError("Error in IfConfig()");
   }

if (ioctl(sk,SIOCGIFFLAGS, (caddr_t) &IFR) == -1)   /* Get the device status flags */
   {
   CfLog(cferror,"No such network device","ioctl");
   return;
   }

flags = IFR.ifr_flags;
strcpy(IFR.ifr_name,vifdev);                   /* copy this each time */
 
if (ioctl(sk,SIOCGIFMETRIC, (caddr_t) &IFR) == -1)   /* Get the routing priority */
   {
   CfLog(cferror,"","ioctl");
   FatalError("Software error: error getting metric");
   }

metric = IFR.ifr_metric;

isnotsane = GetIfStatus(sk,vifdev,vnetmask,vbroadcast);

if (! DONTDO && isnotsane)
   {
   SetIfStatus(sk,vifdev,vnetmask,vbroadcast);
   GetIfStatus(sk,vifdev,vnetmask,vbroadcast);
   }

close(sk);
}


/*******************************************************************/

int GetIfStatus(sk,vifdev,vnetmask,vbroadcast)

int sk;
char *vifdev,*vnetmask,*vbroadcast;

{ struct sockaddr_in *sin;
  struct sockaddr_in netmask;
  int insane = false;
  struct hostent *hp;
  struct in_addr inaddr;

Verbose("Checking interface status...\n");
  
if ((hp = gethostbyname(VSYSNAME.nodename)) == NULL)
   {
   CfLog(cferror,"","gethostbyname");
   return false;
   }
else
   {
   bcopy(hp->h_addr,&inaddr, hp->h_length);
   Verbose("Address given by nameserver: %s\n",inet_ntoa(inaddr));
   }

strcpy(IFR.ifr_name,vifdev);

if (ioctl(sk,SIOCGIFADDR, (caddr_t) &IFR) == -1)   /* Get the device status flags */
   {
   return false;
   }

sin = (struct sockaddr_in *) &IFR.ifr_addr;


if (strcmp((char *)inet_ntoa(*(struct in_addr *)(hp->h_addr)),(char *)inet_ntoa(sin->sin_addr)) != 0)
   {
   CfLog(cferror,"This machine is configured with an address which differs from\n","");
   CfLog(cferror,"the nameserver's information! (Insane!)\n","");
   CfLog(cferror,"Don't quite know what to do...\n","");
   insane = true;
   }

if (ioctl(sk,SIOCGIFNETMASK, (caddr_t) &IFR) == -1) 
   {
   return false;
   }

netmask.sin_addr = ((struct sockaddr_in *) &IFR.ifr_addr)->sin_addr;

Verbose("Found netmask: %s\n",inet_ntoa(netmask.sin_addr));

strcpy(VBUFF,inet_ntoa(netmask.sin_addr));

if (strcmp(VBUFF,vnetmask))
   {
   CfLog(cferror,"The netmask is incorrectly configured, resetting...\n","");
   insane = true;
   }

if (ioctl(sk,SIOCGIFBRDADDR, (caddr_t) &IFR) == -1) 
   {
   return false;
   }

sin = (struct sockaddr_in *) &IFR.ifr_addr;
strcpy(VBUFF,inet_ntoa(sin->sin_addr));

Verbose("Found broadcast address: %s\n",inet_ntoa(sin->sin_addr));

GetBroadcastAddr(inet_ntoa(inaddr),vifdev,vnetmask,vbroadcast);

if (strcmp(VBUFF,VNUMBROADCAST) != 0)
   {
   snprintf(OUTPUT,bufsize*2,"Broadcast address was %s not %s (should be bit-type %s)\n",VBUFF,VNUMBROADCAST,vbroadcast);
   CfLog(cferror,OUTPUT,"");
   insane = true;
   }

return(insane);
}

/*******************************************************************/

void SetIfStatus(sk,vifdev,vnetmask,vbroadcast)

int sk;
char *vifdev,*vnetmask, *vbroadcast;

{ struct sockaddr_in *sin;
  struct sockaddr_in netmask, broadcast;

   /*********************************

   Don't try to set the address yet...

    if (ioctl(sk,SIOCSIFADDR, (caddr_t) &IFR) == -1) 
      {
      perror ("Can't set IP address");
      return;
      } 

   **********************************/

/* set netmask */

Verbose("Resetting interface...\n");

memset(&IFR, 0, sizeof(IFR));
strncpy(IFR.ifr_name,vifdev,sizeof(IFR.ifr_name)); 
netmask.sin_addr.s_addr = inet_network(vnetmask);
netmask.sin_family = AF_INET;
IFR.ifr_addr = *((struct sockaddr *) &netmask);

sin = (struct sockaddr_in *) &IFR.ifr_addr;

if (ioctl(sk,SIOCSIFNETMASK, (caddr_t) &IFR) < 0) 
   {
   CfLog(cferror,"Permission to reconfigure netmask denied.\n","ioctl");
   }
else
   {
   snprintf(OUTPUT,bufsize*2,"Set Netmask to: %s\n",inet_ntoa(netmask.sin_addr));
   CfLog(cfinform,OUTPUT,"");
   }

/* broadcast addr */

strcpy(IFR.ifr_name,vifdev);
broadcast.sin_addr.s_addr = inet_addr(VNUMBROADCAST);
IFR.ifr_addr = *((struct sockaddr *) &broadcast);
sin = (struct sockaddr_in *) &IFR.ifr_addr;

Verbose("Trying to set broad to %s = %s\n",VNUMBROADCAST,inet_ntoa(sin->sin_addr));
 
if (ioctl(sk,SIOCSIFBRDADDR, (caddr_t) &IFR) == -1) 
   {
   CfLog(cferror,"Permission to reconfigure broadcast denied.\n","ioctl");
   return;
   } 

if ((char *) sin->sin_addr.s_addr == NULL)
   {
   CfLog(cferror,"No broadcast address on socket after configuration!!\n","");
   }
else
   {
   snprintf(OUTPUT,bufsize*2,"Set Broadcast address to: %s\n",inet_ntoa(sin->sin_addr));
   CfLog(cfinform,OUTPUT,"");
   }
}

/*****************************************************/

void GetBroadcastAddr(ipaddr,vifdev,vnetmask,vbroadcast)

char *ipaddr,*vifdev,*vnetmask,*vbroadcast;

{ unsigned int na,nb,nc,nd;
  unsigned int ia,ib,ic,id;
  unsigned int ba,bb,bc,bd;
  unsigned netmask,ip,broadcast;

sscanf(vnetmask,"%u.%u.%u.%u",&na,&nb,&nc,&nd);

netmask = nd + 256*nc + 256*256*nb + 256*256*256*na;

sscanf(ipaddr,"%u.%u.%u.%u",&ia,&ib,&ic,&id);

ip = id + 256*ic + 256*256*ib + 256*256*256*ia;

if (strcmp(vbroadcast,"zero") == 0)
   {
   broadcast = ip & netmask;
   }
else if (strcmp(vbroadcast,"one") == 0)
   {
   broadcast = ip | (~netmask);
   }
else
   {
   return;
   }

ba = broadcast / (256 * 256 * 256);
bb = (broadcast / (256 * 256)) % 256;
bc = broadcast / (256) % 256;
bd = broadcast % 256;
sprintf(VNUMBROADCAST,"%u.%u.%u.%u",ba,bb,bc,bd);
}

/****************************************************************/
/*                                                              */
/* Routing Tables:                                              */
/*                                                              */
/* To check that we have at least one static route entry to     */
/* the nearest gateway -- i.e. the wildcard entry for "default" */
/* we need some way of accessing the routing tables. There is   */
/* no elegant way of doing this, alas.                          */
/*                                                              */
/****************************************************************/

void SetDefaultRoute()

{ int sk, defaultokay = 1;
  struct sockaddr_in sindst,singw;

# ifdef HAVE_ORTENTRY
   struct ortentry route;
# else
#  if HAVE_RTENTRY
   struct rtentry route;
#  endif
# endif

  FILE *pp;

Verbose("Looking at default route...\n");
  
if (!IsPrivileged())                            
   {
   snprintf(OUTPUT,bufsize*2,"Only root can set a default route.");
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if (VDEFAULTROUTE[0] == '\0')
   {
   Debug("cfengine: No default route is defined. Ignoring the routing tables.\n");
   return;
   }

if ((pp = cfpopen(VNETSTAT[VSYSTEMHARDCLASS],"r")) == NULL)
   {
   snprintf(OUTPUT,bufsize*2,"Failed to open pipe from %s\n",VNETSTAT[VSYSTEMHARDCLASS]);
   CfLog(cferror,OUTPUT,"popen");
   return;
   }

while (!feof(pp))
   {
   ReadLine(VBUFF,bufsize,pp);

   if (strncmp(VBUFF,"default",7) == 0)
      {
      if (strstr(VBUFF,VDEFAULTROUTE))
         {
	 Verbose("cfengine: default route is already set to %s\n",VDEFAULTROUTE);
         defaultokay = 1;
         break;
         }
      else
         {
         CfLog(cferror,"The default packet-route is incorrectly set\n","");
         CfLog(cferror,"Please correct this manually using route(1).\n","");
         break;
         }
      }
   defaultokay = 0;
   }

cfpclose(pp);

if (defaultokay)
   {
   return;
   }

# if defined HAVE_RTENTRY || defined HAVE_ORTENTRY
 
if ((sk = socket(AF_INET,SOCK_RAW,0)) == -1)
   {
   if ( VSYSTEMHARDCLASS == linuxx )
      {
      Debug ("No raw socket protocol for linux\n");
      return;
      }
   
   printf("System class %s\n",CLASSTEXT[VSYSTEMHARDCLASS]);
   perror("cfengine: socket");
   FatalError("Error in SetDefaultRoute()");
   }

sindst.sin_family = AF_INET;
singw.sin_family = AF_INET;

sindst.sin_addr.s_addr = INADDR_ANY;
singw.sin_addr.s_addr = inet_addr(VDEFAULTROUTE);

route.rt_dst = *(struct sockaddr *)&sindst;      /* This disgusting method is necessary */
route.rt_gateway = *(struct sockaddr *)&singw;
route.rt_flags = RTF_GATEWAY;

if (! DONTDO)
   {
   if (ioctl(sk,SIOCADDRT, (caddr_t) &route) == -1)   /* Get the device status flags */
      {
      CfLog(cferror,"","ioctl SIOCADDRT:");
      FatalError("Software error: set default route");
      }

   CfLog(cferror,"The routing table did not contain a default route.\n","");
   snprintf(OUTPUT,bufsize*2,"I'm setting it to %s\n",VDEFAULTROUTE);
   CfLog(cferror,OUTPUT,"");
   }

# else

/* Socket routing - don't really know how to do this yet */ 

Verbose("Sorry don't know how to do routing on this platform\n");
 
# endif
}

#else /* NT */

void IfConf (vifdev,vnetmask,vbroadcast)

char *vifdev,*vnetmask, *vbroadcast;

{
Verbose("Network configuration is not implemented on this OS\n");
}

void SetDefaultRoute()

{
Verbose("Setting default route is not implemented on this OS\n"); 
}

#endif
