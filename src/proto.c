/* 

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
/* File Image copying                                              */
/*                                                                 */
/* client part for remote copying                                  */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/

int BadProtoReply(buf)

char *buf;

{
return (strncmp(buf,"BAD:",4) == 0);
}

/*********************************************************************/

int OKProtoReply(buf)

char *buf;

{
return(strncmp(buf,"OK:",3) == 0);
}

/*********************************************************************/

int FailedProtoReply(buf)

char *buf;

{
return(strncmp(buf,CFFAILEDSTR,strlen(CFFAILEDSTR)) == 0);
}

/*********************************************************************/

int IdentifyForVerification(sd,localip,family)

int sd,family;
char *localip;

{ char sendbuff[bufsize],dnsname[maxvarsize];
  struct sockaddr_in myaddr;
  struct in_addr *iaddr;
  struct hostent *hp;
  int len;
  struct passwd *user_ptr;
  char *uname;
  
bzero(sendbuff,bufsize);
bzero(dnsname,maxvarsize);
 
if (strcmp(VDOMAIN,CF_START_DOMAIN) == 0)
   {
   CfLog(cferror,"Undefined domain name","");
   return false;
   }
 
/* First we need to find out the IP address and DNS name of the socket
   we are sending from. This is not necessarily the same as VFQNAME if
   the machine has a different uname from its IP name (!) This can
   happen on stupidly set up machines or on hosts with multiple
   interfaces, with different names on each interface ... */ 

 switch (family)
    {
    case AF_INET: len = sizeof(struct sockaddr_in);
	break;
#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)
    case AF_INET6: len = sizeof(struct sockaddr_in6);
	break;
#endif
    default:
	CfLog(cferror,"Software error in IdentifyForVerification","");
    }

if (getsockname(sd,(struct sockaddr *)&myaddr,&len) == -1)
   {
   CfLog(cferror,"Couldn't get socket address\n","getsockname");
   return false;
   }
 
snprintf(localip,cfmaxiplen-1,"%s",sockaddr_ntop((struct sockaddr *)&myaddr)); 

Debug("Identifying this agent as %s i.e. %s, with signature %d\n",localip,VFQNAME,CFSIGNATURE);

#if defined(HAVE_GETADDRINFO) && !defined(DARWIN)

if (getnameinfo((struct sockaddr *)&myaddr,sizeof(myaddr),dnsname,maxvarsize,NULL,0,0) != 0)
   {
   CfLog(cferror,"Couldn't lookup IP6 address\n","gethostbyaddr");
   return false;
   }
 
#else 

iaddr = &(myaddr.sin_addr); 
hp = gethostbyaddr((void *)iaddr,sizeof(myaddr.sin_addr),family);

if ((hp == NULL) || (hp->h_name == NULL))
   {
   CfLog(cferror,"Couldn't lookup IP address\n","gethostbyaddr");
   return false;
   }

strcpy(dnsname,hp->h_name);
 
if ((strstr(hp->h_name,".") == 0) && (strlen(VDOMAIN) > 0))
   {
   strcat(dnsname,".");
   strcat(dnsname,VDOMAIN);
   } 
#endif 
 
user_ptr = getpwuid(getuid());
uname = user_ptr ? user_ptr->pw_name : "UNKNOWN";

/* Some resolvers will not return FQNAME and missing PTR will give numerical result */

if ((strlen(VDOMAIN) > 0) && !IsIPV6Address(dnsname) && !strchr(dnsname,'.'))
   {
   strcat(dnsname,".");
   strncat(dnsname,VDOMAIN,maxvarsize/2);
   }  
 
snprintf(sendbuff,bufsize,"CAUTH %s %s %s %d",localip,dnsname,uname,CFSIGNATURE);

SendTransaction(sd,sendbuff,0,CF_DONE);
return true;
}

/*********************************************************************/

int KeyAuthentication(ip)

struct Image *ip;

{ char sendbuffer[bufsize],in[bufsize],*out,*decrypted_cchall;
 BIGNUM *nonce_challenge, *bn = NULL;
 unsigned long err;
 unsigned char digest[EVP_MAX_MD_SIZE];
 int encrypted_len,nonce_len = 0,len;
 char cant_trust_server, keyname[bufsize];
 RSA *server_pubkey = NULL;

if (COMPATIBILITY_MODE)
   {
   return true;
   }

if (PUBKEY == NULL || PRIVKEY == NULL) 
   {
   CfLog(cferror,"No public/private key pair found\n","");
   return false;
   }
 
Debug("KeyAuthentication()\n");
 
/* Generate a random challenge to authenticate the server */
 
nonce_challenge = BN_new();
BN_rand(nonce_challenge,256,0,0);

nonce_len = BN_bn2mpi(nonce_challenge,in);
ChecksumString(in,nonce_len,digest,'m');

/* We assume that the server bound to the remote socket is the official one i.e. = root's */

snprintf(keyname,bufsize,"root-%s",CONN->remoteip); 

if (server_pubkey = HavePublicKey(keyname))
   {
   cant_trust_server = 'y';
   encrypted_len = BN_num_bytes(server_pubkey->n);
   }
else 
   {
   cant_trust_server = 'n';                      /* have to trust server, since we can't verify id */
   encrypted_len = nonce_len;
   }
 
snprintf(sendbuffer,bufsize,"SAUTH %c %d %d",cant_trust_server,encrypted_len,nonce_len);
 
if ((out = malloc(encrypted_len)) == NULL)
   {
   FatalError("memory failure");
   }

if (server_pubkey != NULL)
   {
   if (RSA_public_encrypt(nonce_len,in,out,server_pubkey,RSA_PKCS1_PADDING) <= 0)
      {
      err = ERR_get_error();
      snprintf(OUTPUT,bufsize,"Public encryption failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,OUTPUT,"");
      return false;
      }
   
   bcopy(out,sendbuffer+CF_RSA_PROTO_OFFSET,encrypted_len); 
   }
else
   {
   bcopy(in,sendbuffer+CF_RSA_PROTO_OFFSET,nonce_len); 
   }

/* proposition C1 - Send challenge / nonce */
 
SendTransaction(CONN->sd,sendbuffer,CF_RSA_PROTO_OFFSET+encrypted_len,CF_DONE);

BN_free(bn);
BN_free(nonce_challenge);
free(out);

if (DEBUG||D2)
   {
   RSA_print_fp(stdout,PUBKEY,0);
   }

/*Send the public key - we don't know if server has it */ 
/* proposition C2 */

bzero(sendbuffer,bufsize); 
len = BN_bn2mpi(PUBKEY->n,sendbuffer); 
SendTransaction(CONN->sd,sendbuffer,len,CF_DONE); /* No need to encrypt the public key ... */

/* proposition C3 */ 
bzero(sendbuffer,bufsize);   
len = BN_bn2mpi(PUBKEY->e,sendbuffer); 
SendTransaction(CONN->sd,sendbuffer,len,CF_DONE);

/* check reply about public key - server can break connection here */

/* proposition S1 */  
bzero(in,bufsize);  
ReceiveTransaction(CONN->sd,in,NULL);

if (BadProtoReply(in))
   {
   CfLog(cferror,in,"");
   return false;
   }

/* Get challenge response - should be md5 of challenge */

/* proposition S2 */   
bzero(in,bufsize);  
ReceiveTransaction(CONN->sd,in,NULL);

if (!ChecksumsMatch(digest,in,'m')) 
   {
   snprintf(OUTPUT,bufsize,"Challenge response from server %s/%s was incorrect!",ip->server,CONN->remoteip);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
else
   {
   if (cant_trust_server == 'y')  /* challenge reply was correct */ 
      {
      snprintf(OUTPUT,bufsize,"Strong authentication of server=%s connection confirmed\n",ip->server);
      CfLog(cfverbose,OUTPUT,"");
      }
   else
      {
      if (ip->trustkey == 'y')
	 {
	 snprintf(OUTPUT,bufsize,"Trusting server identity and willing to accept key from %s=%s",ip->server,CONN->remoteip);
	 CfLog(cferror,OUTPUT,"");
	 }
      else
	 {
	 snprintf(OUTPUT,bufsize,"Not authorized to trust the server=%s's public key (trustkey=false)\n",ip->server);
	 CfLog(cferror,OUTPUT,"");
	 return false;
	 }
      }
   }

/* Receive counter challenge from server */ 

Debug("Receive counter challenge from server\n");  
/* proposition S3 */   
bzero(in,bufsize);  
encrypted_len = ReceiveTransaction(CONN->sd,in,NULL);


if ((decrypted_cchall = malloc(encrypted_len)) == NULL)
   {
   FatalError("memory failure");
   }
 
if (RSA_private_decrypt(encrypted_len,in,decrypted_cchall,PRIVKEY,RSA_PKCS1_PADDING) <= 0)
   {
   err = ERR_get_error();
   snprintf(OUTPUT,bufsize,"Private decrypt failed = %s, abandoning\n",ERR_reason_error_string(err));
   CfLog(cferror,OUTPUT,"");
   return false;
   }

/* proposition C4 */   
ChecksumString(decrypted_cchall,nonce_len,digest,'m');
Debug("Replying to counter challenge with md5\n"); 
SendTransaction(CONN->sd,digest,16,CF_DONE);
free(decrypted_cchall); 

/* If we don't have the server's public key, it will be sent */
 
if (server_pubkey == NULL)
   {
   RSA *newkey = RSA_new();

   Debug("Collecting public key from server!\n"); 

   /* proposition S4 - conditional */  
   if ((len = ReceiveTransaction(CONN->sd,in,NULL)) == 0)
      {
      CfLog(cferror,"Protocol error in RSA authentation from IP %s\n",ip->server);
      return false;
      }
   
   if ((newkey->n = BN_mpi2bn(in,len,NULL)) == NULL)
      {
      err = ERR_get_error();
      snprintf(OUTPUT,bufsize,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,OUTPUT,"");
      return false;
      }

   /* proposition S5 - conditional */  
   if ((len=ReceiveTransaction(CONN->sd,in,NULL)) == 0)
      {
      CfLog(cfinform,"Protocol error in RSA authentation from IP %s\n",ip->server);
      return false;
      }
   
   if ((newkey->e = BN_mpi2bn(in,len,NULL)) == NULL)
      {
      err = ERR_get_error();
      snprintf(OUTPUT,bufsize,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,OUTPUT,"");
      return false;
      }

   SavePublicKey(keyname,newkey);
   RSA_free(newkey);
   }
else
   {
   RSA_free(server_pubkey);
   }
 
/* proposition C5 */ 
GenerateRandomSessionKey();

if (CONN->session_key == NULL)
   {
   CfLog(cferror,"A random session key could not be established","");
   return false;
   }
 
SendTransaction(CONN->sd,CONN->session_key,16,CF_DONE); 
return true; 
}

