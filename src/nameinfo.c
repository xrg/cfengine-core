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
/*  TOOLKITS: "object" library                                       */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include "../pub/global.h"

/* FreeBSD has size macro defined as ifreq is variable size */
#ifdef _SIZEOF_ADDR_IFREQ
#define SIZEOF_IFREQ(x) _SIZEOF_ADDR_IFREQ(x)
#else
#define SIZEOF_IFREQ(x) sizeof(struct ifreq)
#endif

/*******************************************************************/

void GetNameInfo()

{ int i,found = false;
  char *sp,*sp2;
  time_t tloc;
  struct hostent *hp;
  struct sockaddr_in cin;
#ifdef AIX
  char real_version[_SYS_NMLN];
#endif
#ifdef HAVE_SYSINFO
#ifdef SI_ARCHITECTURE
  long sz;
#endif
#endif

Debug("GetNameInfo()\n");
  
VFQNAME[0] = VUQNAME[0] = '\0';
  
if (uname(&VSYSNAME) == -1)
   {
   perror("uname ");
   FatalError("Uname couldn't get kernel name info!!\n");
   }

#ifdef AIX
snprintf(real_version,_SYS_NMLN,"%.80s.%.80s", VSYSNAME.version, VSYSNAME.release);
strncpy(VSYSNAME.release, real_version, _SYS_NMLN);
#endif 

for (sp = VSYSNAME.sysname; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

for (sp = VSYSNAME.machine; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

for (i = 0; CLASSATTRIBUTES[i][0] != '\0'; i++)
   {
   if (WildMatch(CLASSATTRIBUTES[i][0],ToLowerStr(VSYSNAME.sysname)))
      {
      if (WildMatch(CLASSATTRIBUTES[i][1],VSYSNAME.machine))
         {
         if (WildMatch(CLASSATTRIBUTES[i][2],VSYSNAME.release))
            {
	    if (UNDERSCORE_CLASSES)
	       {
	       snprintf(VBUFF,bufsize,"_%s",CLASSTEXT[i]);
	       AddClassToHeap(VBUFF);
	       }
	    else
	       {
               AddClassToHeap(CLASSTEXT[i]);
	       }
            found = true;
	    VSYSTEMHARDCLASS = (enum classes) i;
            break;
            }
         }
      else
         {
         Debug2("Cfengine: I recognize %s but not %s\n",VSYSNAME.sysname,VSYSNAME.machine);
         continue;
         }
      }
   }

if ((sp = malloc(strlen(VSYSNAME.nodename)+1)) == NULL)
   {
   FatalError("malloc failure in initialize()");
   }

strcpy(sp,VSYSNAME.nodename);
SetDomainName(sp);

for (sp2=sp; *sp2 != '\0'; sp2++)  /* Truncate fully qualified name */
   {
   if (*sp2 == '.')
      {
      *sp2 = '\0';
      Debug("Truncating fully qualified hostname %s to %s\n",VSYSNAME.nodename,sp);
      break;
      }
   }

 
VDEFAULTBINSERVER.name = sp;

AddClassToHeap(CanonifyName(sp));

 
if ((tloc = time((time_t *)NULL)) == -1)
   {
   printf("Couldn't read system clock\n");
   }

if (VERBOSE || DEBUG || D2 || D3)
   {
   if (UNDERSCORE_CLASSES)
      {
      snprintf(VBUFF,bufsize,"_%s",CLASSTEXT[i]);
      }
   else
      {
      snprintf(VBUFF,bufsize,"%s",CLASSTEXT[i]);
      }

   if (ISCFENGINE)
      {
      printf ("GNU Configuration Engine - \n%s\n%s\n\n",VERSION,COPYRIGHT);
      }
   else
      {
      printf ("GNU Cfengine server daemon - \n%s\n%s\n\n",VERSION,COPYRIGHT);      
      }

   printf ("------------------------------------------------------------------------\n\n");
   printf ("Host name is: %s\n",VSYSNAME.nodename);
   printf ("Operating System Type is %s\n",VSYSNAME.sysname);
   printf ("Operating System Release is %s\n",VSYSNAME.release);
   printf ("Architecture = %s\n\n\n",VSYSNAME.machine);
   printf ("Using internal soft-class %s for host %s\n\n",VBUFF,CLASSTEXT[VSYSTEMHARDCLASS]);
   printf ("The time is now %s\n\n",ctime(&tloc));
   printf ("------------------------------------------------------------------------\n\n");

   }


sprintf(VBUFF,"%d_bit",sizeof(long)*8);
AddClassToHeap(VBUFF);
Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

snprintf(VBUFF,bufsize,"%s_%s",VSYSNAME.sysname,VSYSNAME.release);
AddClassToHeap(CanonifyName(VBUFF));

AddClassToHeap(CanonifyName(VSYSNAME.machine));
 
Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

snprintf(VBUFF,bufsize,"%s_%s",VSYSNAME.sysname,VSYSNAME.machine);
AddClassToHeap(CanonifyName(VBUFF));

Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

snprintf(VBUFF,bufsize,"%s_%s_%s",VSYSNAME.sysname,VSYSNAME.machine,VSYSNAME.release);
AddClassToHeap(CanonifyName(VBUFF));

Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

#ifdef HAVE_SYSINFO
#ifdef SI_ARCHITECTURE
sz = sysinfo(SI_ARCHITECTURE,VBUFF,bufsize);
if (sz == -1)
  {
  Verbose("cfengine internal: sysinfo returned -1\n");
  }
else
  {
  AddClassToHeap(CanonifyName(VBUFF));
  Verbose("Additional hard class defined as: %s\n",VBUFF);
  }
#endif
#endif

snprintf(VBUFF,bufsize,"%s_%s_%s_%s",VSYSNAME.sysname,VSYSNAME.machine,VSYSNAME.release,VSYSNAME.version);

if (strlen(VBUFF) < maxvarsize-2)
   {
   VARCH = strdup(CanonifyName(VBUFF));
   }
else
   {
   Verbose("cfengine internal: $(arch) overflows maxvarsize! Truncating\n");
   VARCH = strdup(CanonifyName(VSYSNAME.sysname));
   }

snprintf(VBUFF,bufsize,"%s_%s",VSYSNAME.sysname,VSYSNAME.machine);

VARCH2 = strdup(CanonifyName(VBUFF));
 
AddClassToHeap(VARCH);

Verbose("Additional hard class defined as: %s\n",VARCH);

if (! found)
   {
   CfLog(cferror,"Cfengine: I don't understand what architecture this is!","");
   }

strcpy(VBUFF,"compiled_on_"); 
strcat(VBUFF,AUTOCONF_SYSNAME);

AddClassToHeap(CanonifyName(VBUFF));

Verbose("\nGNU autoconf class from compile time: %s\n\n",VBUFF);

/* Get IP address from nameserver */

if ((hp = gethostbyname(VSYSNAME.nodename)) == NULL)
   {
   return;
   }
else
   {
   bzero(&cin,sizeof(cin));
   cin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   Verbose("Address given by nameserver: %s\n",inet_ntoa(cin.sin_addr));
   strcpy(VIPADDRESS,inet_ntoa(cin.sin_addr));
   
   for (i=0; hp->h_aliases[i]!= NULL; i++)
      {
      Debug("Adding alias %s..\n",hp->h_aliases[i]);
      AddClassToHeap(CanonifyName(hp->h_aliases[i])); 
      }
   }
}

/*********************************************************************/

void GetInterfaceInfo(void)

{ int fd,len,i,j;
  struct ifreq ifbuf[512],ifr, *ifp;
  struct ifconf list;
  struct sockaddr_in *sin;
  struct hostent *hp;
  char *sp;

Debug("GetInterfaceInfo()\n");

if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
   {
   CfLog(cferror,"Couldn't open socket","socket");
   exit(1);
   }

list.ifc_len = sizeof(ifbuf);
list.ifc_req = ifbuf;

#ifdef SIOCGIFCONF
if (ioctl(fd, SIOCGIFCONF, &list) == -1 || (list.ifc_len < (sizeof(struct ifreq))))
#else
if (ioctl(fd, OSIOCGIFCONF, &list) == -1 || (list.ifc_len < (sizeof(struct ifreq))))
#endif
   {
   CfLog(cferror,"Couldn't get interfaces","ioctl");
   exit(1);
   }

for (j = 0,len = 0,ifp = list.ifc_req; len < list.ifc_len; len+=SIZEOF_IFREQ(*ifp),j++,ifp=&ifbuf[j])
   {
   if (ifp->ifr_addr.sa_family == 0)
       {
       continue;
       }

   Verbose("Interface %d: %s\n", j+1, ifp->ifr_name);
   if(UNDERSCORE_CLASSES)
   {
   snprintf(VBUFF, bufsize, "_net_iface_%s", CanonifyName(ifp->ifr_name));
   }
   else
   {
   snprintf(VBUFF, bufsize, "net_iface_%s", CanonifyName(ifp->ifr_name));
   }
   AddClassToHeap(VBUFF);

   if (ifp->ifr_addr.sa_family == AF_INET)
      {
      strncpy(ifr.ifr_name,ifp->ifr_name,sizeof(ifp->ifr_name));
      
      if (ioctl(fd,SIOCGIFFLAGS,&ifr) == -1)
         {
         CfLog(cferror,"No such network device","ioctl");
	 close(fd);
         return;
         }

      /* Used to check if interface was "up"
	 if ((ifr.ifr_flags & IFF_UP) && !(ifr.ifr_flags & IFF_LOOPBACK))
         Now check whether it is configured ...
      */
      
      if ((ifr.ifr_flags & IFF_BROADCAST) && !(ifr.ifr_flags & IFF_LOOPBACK))
         {
         sin=(struct sockaddr_in *)&ifp->ifr_addr;
   
         if ((hp = gethostbyaddr((char *)&(sin->sin_addr.s_addr),sizeof(sin->sin_addr.s_addr),AF_INET)) == NULL)
            {
            Debug("Host information for %s not found\n", inet_ntoa(sin->sin_addr));
            }
         else
            {
	    char ip[maxvarsize];
	    
            if (hp->h_name != NULL)
               {
               Debug("Adding hostip %s..\n",inet_ntoa(sin->sin_addr));
               AddClassToHeap(CanonifyName(inet_ntoa(sin->sin_addr)));
               Debug("Adding hostname %s..\n",hp->h_name);
               AddClassToHeap(CanonifyName(hp->h_name));

               for (i=0;hp->h_aliases[i]!=NULL;i++)
                  {
                  Debug("Adding alias %s..\n",hp->h_aliases[i]);
                  AddClassToHeap(CanonifyName(hp->h_aliases[i]));
                  }

	       /* Old style compat */
	       strcpy(ip,inet_ntoa(sin->sin_addr));
	       
	       for (sp = ip+strlen(ip)-1; *sp != '.'; sp--)
		  {
		  }
	       *sp = '\0';
	       AddClassToHeap(CanonifyName(ip));

	       /* New style */
	       strcpy(ip,"ipv4_");
	       strcat(ip,inet_ntoa(sin->sin_addr));
	       AddClassToHeap(CanonifyName(ip));

	       for (sp = ip+strlen(ip)-1; (sp > ip); sp--)
		  {
		  if (*sp == '.')
		     {
		     *sp = '\0';
		     AddClassToHeap(CanonifyName(ip));
		     }
		  }
               }
            }
         }
      }
   
   ifp=(struct ifreq *)((char *)ifp+SIZEOF_IFREQ(*ifp));
   }
 
close(fd);
}

/*********************************************************************/

void GetV6InterfaceInfo(void)

{
/* Whatever the manuals might say, you cannot get IPV6
   interface configuration from the ioctls. This seems
   to be implemented in a non standard way across OSes
BSDi has done getifaddrs(), solaris 8 has a new ioctl, Stevens
book shows the suggestion which has not been implemented...
*/
 
 Verbose("Sorry - there is no current standard way to find out my IPv6 address (!!)\n");
}

/*********************************************************************/

void AddNetworkClass(netmask) /* Function contrib David Brownlee <abs@mono.org> */

char *netmask;

{ struct in_addr ip,nm;
  char *sp,nmbuf[maxvarsize],ipbuf[maxvarsize];

    /*
     * Has to differentiate between cases such as:
     *		192.168.101.1/24 -> 192.168.101  	and
     *		192.168.101.1/26 -> 192.168.101.0 
     * We still have the, um... 'interesting' Class C default Network Class
     * set by GetNameInfo()
     */

    /* This is also a convenient method to ensure valid dotted quad */
  if ((nm.s_addr = inet_addr(netmask)) != -1 && (ip.s_addr = inet_addr(VIPADDRESS)) != -1)
    {
    ip.s_addr &= nm.s_addr;	/* Will not work with IPv6 */
    strcpy(ipbuf,inet_ntoa(ip));
    
    strcpy(nmbuf,inet_ntoa(nm));
    
    while( (sp = strrchr(nmbuf,'.')) && strcmp(sp,".0") == 0 )
       {
       *sp = 0;
       *strrchr(ipbuf,'.') = 0;
       }
    AddClassToHeap(CanonifyName(ipbuf)); 
    }
}



/*********************************************************************/

void SetDomainName(sp)           /* Bas van der Vlies */

char *sp;

{ char fqn[maxvarsize];
  char *ptr;
  char buffer[bufsize];

if (gethostname(fqn, sizeof(fqn)) != -1)
   {
   strcpy(VFQNAME,fqn);
   strcpy(buffer,VFQNAME);
   AddClassToHeap(CanonifyName(buffer));
   AddClassToHeap(CanonifyName(ToLowerStr(buffer)));

   if (strstr(fqn,"."))
      {
      ptr = strchr(fqn, '.');
      strcpy(VDOMAIN, ++ptr);
      }
   }

if (strstr(VFQNAME,".") == 0)
   {
   strcat(VFQNAME,".");
   strcat(VFQNAME,VDOMAIN);
   }

AddClassToHeap(CanonifyName(VDOMAIN));
DeleteClassFromHeap("undefined_domain");
}

/*****************************************************************************/
/* TOOLKIT                                                                   */
/* INET independent address/struct conversion routines                       */
/*****************************************************************************/

char *sockaddr_ntop(struct sockaddr *sa)

{ 
#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
 static char addrbuf[INET6_ADDRSTRLEN];
 void *addr;
#else
 static char addrbuf[20];
 struct in_addr addr;
#endif
 
switch (sa->sa_family)
   {
   case AF_INET:
       Debug("IPV4 address\n");
       snprintf(addrbuf,20,"%.19s",inet_ntoa(((struct sockaddr_in *)sa)->sin_addr));
       break;

#ifdef AF_LOCAL
   case AF_LOCAL:
       Debug("Local socket\n") ;
       strcpy(addrbuf, "127.0.0.1") ;
       break;
#endif

#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
   case AF_INET6:
       Debug("IPV6 address\n");
       addr = &((struct sockaddr_in6 *)sa)->sin6_addr;
       inet_ntop(sa->sa_family,addr,addrbuf,sizeof(addrbuf));
       break;
#endif
   default:
       Debug("Address family was %d\n",sa->sa_family);
       FatalError("Software failure in sockaddr_ntop\n");
   }

Debug("sockaddr_ntop(%s)\n",addrbuf);
return addrbuf;
}

/*****************************************************************************/

 /* Example:
   
 struct sockaddr_in *p;
 struct sockaddr_in6 *p6;
 
 p = (struct sockaddr_in *) sockaddr_pton(AF_INET,"128.39.89.10");
 p6 = (struct sockaddr_in6 *) sockaddr_pton(AF_INET6,"2001:700:700:3:290:27ff:fea2:477b");

 printf("Coded %s\n",sockaddr_ntop((struct sockaddr *)p));

 */

/*****************************************************************************/

void *sockaddr_pton(af,src)

int af;
void *src;

{ int err;
#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
  static struct sockaddr_in6 adr6;
#endif
  static struct sockaddr_in adr; 
  
switch (af)
   {
   case AF_INET:
       bzero(&adr,sizeof(adr));
       adr.sin_family = AF_INET;
       adr.sin_addr.s_addr = inet_addr(src);
       Debug("Coded ipv4 %s\n",sockaddr_ntop((struct sockaddr *)&adr));
       return (void *)&adr;
       
#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
   case AF_INET6:
       memset(&adr6,0,sizeof(adr6)); 
       adr6.sin6_family = AF_INET6;
       err = inet_pton(AF_INET6,src,&(adr6.sin6_addr));

       if (err > 0)
	  {
	  Debug("Coded ipv6 %s\n",sockaddr_ntop((struct sockaddr *)&adr6));
	  return (void *)&adr6;
	  }
       else
	  {
	  return NULL;
	  }
       break;
#endif
   default:
       Debug("Address family was %d\n",af);
       FatalError("Software failure in sockaddr_pton\n");
   }

 return NULL; 
}
