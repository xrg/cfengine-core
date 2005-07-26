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
#ifdef IRIX
#include <sys/syssgi.h>
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
# ifdef _SIZEOF_ADDR_IFREQ
#  define SIZEOF_IFREQ(x) _SIZEOF_ADDR_IFREQ(x)
# else
#  define SIZEOF_IFREQ(x) \
          ((x).ifr_addr.sa_len > sizeof(struct sockaddr) ? \
           (sizeof(struct ifreq) - sizeof(struct sockaddr) + \
            (x).ifr_addr.sa_len) : sizeof(struct ifreq))
# endif
#else
# define SIZEOF_IFREQ(x) sizeof(struct ifreq)
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
#ifdef IRIX
  char real_version[256]; /* see <sys/syssgi.h> */
#endif
#ifdef HAVE_SYSINFO
  long sz;
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
#elif defined IRIX
/* This gets us something like `6.5.19m' rather than just `6.5'.  */ 
 syssgi (SGI_RELEASE_NAME, 256, real_version);
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
               snprintf(VBUFF,CF_BUFSIZE,"_%s",CLASSTEXT[i]);
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

for (sp2=sp; *sp2 != '\0'; sp2++)  /* Add some domain hierarchy classes */
   {
   if (*sp2 == '.')
      {
      if (*(sp2+1) != '\0')
         {
         Debug("Defining domain #%s#\n",(sp2+1));
         AddClassToHeap(CanonifyName(sp2+1));
         }
      else
         {
         Debug("Domain rejected\n");
         }      
      }
   }

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
      snprintf(VBUFF,CF_BUFSIZE,"_%s",CLASSTEXT[i]);
      }
   else
      {
      snprintf(VBUFF,CF_BUFSIZE,"%s",CLASSTEXT[i]);
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

snprintf(VBUFF,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,VSYSNAME.release);
AddClassToHeap(CanonifyName(VBUFF));

#ifdef IRIX
/* Get something like `irix64_6_5_19m' defined as well as
   `irix64_6_5'.  Just copying the latter into VSYSNAME.release
   wouldn't be backwards-compatible.  */
snprintf(VBUFF,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,real_version);
AddClassToHeap(CanonifyName(VBUFF));
#endif

AddClassToHeap(CanonifyName(VSYSNAME.machine));
 
Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

snprintf(VBUFF,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,VSYSNAME.machine);
AddClassToHeap(CanonifyName(VBUFF));

Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

snprintf(VBUFF,CF_BUFSIZE,"%s_%s_%s",VSYSNAME.sysname,VSYSNAME.machine,VSYSNAME.release);
AddClassToHeap(CanonifyName(VBUFF));

Verbose("Additional hard class defined as: %s\n",CanonifyName(VBUFF));

#ifdef HAVE_SYSINFO
#ifdef SI_ARCHITECTURE
sz = sysinfo(SI_ARCHITECTURE,VBUFF,CF_BUFSIZE);
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
#ifdef SI_PLATFORM
sz = sysinfo(SI_PLATFORM,VBUFF,CF_BUFSIZE);
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

snprintf(VBUFF,CF_BUFSIZE,"%s_%s_%s_%s",VSYSNAME.sysname,VSYSNAME.machine,VSYSNAME.release,VSYSNAME.version);

if (strlen(VBUFF) < CF_MAXVARSIZE-2)
   {
   VARCH = strdup(CanonifyName(VBUFF));
   }
else
   {
   Verbose("cfengine internal: $(arch) overflows CF_MAXVARSIZE! Truncating\n");
   VARCH = strdup(CanonifyName(VSYSNAME.sysname));
   }

snprintf(VBUFF,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,VSYSNAME.machine);

VARCH2 = strdup(CanonifyName(VBUFF));
 
AddClassToHeap(VARCH);

Verbose("Additional hard class defined as: %s\n",VARCH);

if (! found)
   {
   CfLog(cferror,"Cfengine: I don't understand what architecture this is!","");
   }

strcpy(VBUFF,"compiled_on_"); 
strcat(VBUFF,CanonifyName(AUTOCONF_SYSNAME));

AddClassToHeap(CanonifyName(VBUFF));

Verbose("\nGNU autoconf class from compile time: %s\n\n",VBUFF);

/* Get IP address from nameserver */

if ((hp = gethostbyname(VSYSNAME.nodename)) == NULL)
   {
   return;
   }
else
   {
   memset(&cin,0,sizeof(cin));
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
  struct ifreq ifbuf[8192],ifr, *ifp;
  struct ifconf list;
  struct sockaddr_in *sin;
  struct hostent *hp;
  char *sp;
  char ip[CF_MAXVARSIZE];
  char name[CF_MAXVARSIZE];
            

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

for (j = 0,len = 0,ifp = list.ifc_req; len < list.ifc_len; len+=SIZEOF_IFREQ(*ifp),j++,ifp=(struct ifreq *)((char *)ifp+SIZEOF_IFREQ(*ifp)))
   {
   if (ifp->ifr_addr.sa_family == 0)
       {
       continue;
       }

   Verbose("Interface %d: %s\n", j+1, ifp->ifr_name);

   if(UNDERSCORE_CLASSES)
      {
      snprintf(VBUFF, CF_BUFSIZE, "_net_iface_%s", CanonifyName(ifp->ifr_name));
      }
   else
      {
      snprintf(VBUFF, CF_BUFSIZE, "net_iface_%s", CanonifyName(ifp->ifr_name));
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
            if (hp->h_name != NULL)
               {
               Debug("Adding hostip %s..\n",inet_ntoa(sin->sin_addr));
               AddClassToHeap(CanonifyName(inet_ntoa(sin->sin_addr)));
               Debug("Adding hostname %s..\n",hp->h_name);
               AddClassToHeap(CanonifyName(hp->h_name));

               if (hp->h_aliases != NULL)
                  {
                  for (i=0; hp->h_aliases[i] != NULL; i++)
                     {
                     Debug("Adding alias %s..\n",hp->h_aliases[i]);
                     AddClassToHeap(CanonifyName(hp->h_aliases[i]));
                     }
                  }
               }               

            }
         
         /* Old style compat */
         strcpy(ip,inet_ntoa(sin->sin_addr));
         AppendItem(&IPADDRESSES,ip,"");
         
         for (sp = ip+strlen(ip)-1; *sp != '.'; sp--)
            {
            }
         *sp = '\0';
         AddClassToHeap(CanonifyName(ip));
         
            
         /* New style */
         strcpy(ip,"ipv4_");
         strcat(ip,inet_ntoa(sin->sin_addr));
         AddClassToHeap(CanonifyName(ip));
         snprintf(name,CF_MAXVARSIZE-1,"ipv4[%s]",CanonifyName(ifp->ifr_name));
         AddMacroValue(CONTEXTID,name,inet_ntoa(sin->sin_addr));
         
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
 
close(fd);
}

/*********************************************************************/

void GetV6InterfaceInfo(void)

{ FILE *pp;
  char buffer[CF_BUFSIZE]; 
 
/* Whatever the manuals might say, you cannot get IPV6
   interface configuration from the ioctls. This seems
   to be implemented in a non standard way across OSes
   BSDi has done getifaddrs(), solaris 8 has a new ioctl, Stevens
   book shows the suggestion which has not been implemented...
*/
 
 Verbose("Trying to locate my IPv6 address\n");

 switch (VSYSTEMHARDCLASS)
    {
    case cfnt:
        /* NT cannot do this */
        break;

    case irix:
    case irix4:
    case irix64:
        
        if ((pp = cfpopen("/usr/etc/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }
        
        break;

    case hp:
        
        if ((pp = cfpopen("/usr/sbin/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }

        break;

    case aix:
        
        if ((pp = cfpopen("/etc/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }

        break;
        
    default:
        
        if ((pp = cfpopen("/sbin/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }
        
        while (!feof(pp))
           {    
           fgets(buffer,CF_BUFSIZE-1,pp);
           
           if (StrStr(buffer,"inet6"))
              {
              struct Item *ip,*list = NULL;
              char *sp;
              
              list = SplitStringAsItemList(buffer,' ');
              
              for (ip = list; ip != NULL; ip=ip->next)
                 {
                 for (sp = ip->name; *sp != '\0'; sp++)
                    {
                    if (*sp == '/')  /* Remove CIDR mask */
                       {
                       *sp = '\0';
                       }
                    }
                 
                 if (IsIPV6Address(ip->name) && (strcmp(ip->name,"::1") != 0))
                    {
                    Verbose("Found IPv6 address %s\n",ip->name);
                    AppendItem(&IPADDRESSES,ip->name,"");
                    AddClassToHeap(CanonifyName(ip->name));
                    }
                 }
              
              DeleteItemList(list);
              }
           }
        
        cfpclose(pp);
        break;
    }
}

/*********************************************************************/

void AddNetworkClass(char *netmask) /* Function contrib David Brownlee <abs@mono.org> */

{ struct in_addr ip,nm;
  char *sp,nmbuf[CF_MAXVARSIZE],ipbuf[CF_MAXVARSIZE];

    /*
     * Has to differentiate between cases such as:
     *  192.168.101.1/24 -> 192.168.101   and
     *  192.168.101.1/26 -> 192.168.101.0 
     * We still have the, um... 'interesting' Class C default Network Class
     * set by GetNameInfo()
     */

    /* This is also a convenient method to ensure valid dotted quad */

  if ((nm.s_addr = inet_addr(netmask)) != -1 && (ip.s_addr = inet_addr(VIPADDRESS)) != -1)
     {
     ip.s_addr &= nm.s_addr; /* Will not work with IPv6 */
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

void SetDomainName(char *sp)           /* Bas van der Vlies */

{ char fqn[CF_MAXVARSIZE];
  char *ptr;
  char buffer[CF_BUFSIZE];

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

if (strstr(VFQNAME,".") == 0 && (strcmp(VDOMAIN,CF_START_DOMAIN) != 0))
   {
   strcat(VFQNAME,".");
   strcat(VFQNAME,VDOMAIN);
   }

AddClassToHeap(CanonifyName(VDOMAIN));
DeleteClassFromHeap("undefined_domain");
}


/*******************************************************************/

void SetReferenceTime(int setclasses)

{ time_t tloc;
 char vbuff[CF_BUFSIZE];
 
if ((tloc = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","");
   }

CFSTARTTIME = tloc;

snprintf(vbuff,CF_BUFSIZE,"%s",ctime(&tloc));

Verbose("Reference time set to %s\n",ctime(&tloc));

if (setclasses)
   {
   AddTimeClass(vbuff);
   }
}


/*******************************************************************/

void SetStartTime(int setclasses)

{ time_t tloc;
 
if ((tloc = time((time_t *)NULL)) == -1)
   {
   CfLog(cferror,"Couldn't read system clock\n","");
   }

CFINITSTARTTIME = tloc;

Debug("Job start time set to %s\n",ctime(&tloc));
}
