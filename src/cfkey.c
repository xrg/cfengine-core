/* 

        Copyright (C) 1999
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
/* File: cfkey.c                                                             */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

char CFLOCK[bufsize];

void Initialize ARGLIST((void));
int RecursiveTidySpecialArea ARGLIST((char *name, struct Tidy *tp, int maxrecurse, struct stat *sb));

/*****************************************************************************/

int main()

{ unsigned long err;
  RSA *pair;
  FILE *fp;
  struct stat statbuf;
  int fd;
  static char *passphrase = "Cfengine passphrase";
  EVP_CIPHER *cipher = EVP_des_ede3_cbc();


Initialize();
 
if (stat(CFPRIVKEYFILE,&statbuf) != -1)
   {
   printf("A key file already exists at %s.\n",CFPRIVKEYFILE);
   return 1;
   }

if (stat(CFPUBKEYFILE,&statbuf) != -1)
   {
   printf("A key file already exists at %s.\n",CFPUBKEYFILE);
   return 1;
   }

printf("Making a key pair for cfengine, please wait, this could take a minute...\n"); 

pair = RSA_generate_key(2048,35,NULL,NULL);

if (pair == NULL)
   {
   err = ERR_get_error();
   printf("Error = %s\n",ERR_reason_error_string(err));
   return 1;
   }

if (VERBOSE)
   {
   RSA_print_fp(stdout,pair,0);
   }
 
fd = open(CFPRIVKEYFILE,O_WRONLY | O_CREAT | O_TRUNC, 0600);

if (fd < 0)
   {
   printf("Ppen %s failed: %s.",CFPRIVKEYFILE,strerror(errno));
   return 0;
   }
 
if ((fp = fdopen(fd, "w")) == NULL )
   {
   printf("fdopen %s failed: %s.",CFPRIVKEYFILE, strerror(errno));
   close(fd);
   return 0;
   }

printf("Writing private key to %s\n",CFPRIVKEYFILE);
 
if (!PEM_write_RSAPrivateKey(fp,pair,cipher,passphrase,strlen(passphrase),NULL,NULL))
    {
    err = ERR_get_error();
    printf("Error = %s\n",ERR_reason_error_string(err));
    return 1;
    }
 
fclose(fp);
 
fd = open(CFPUBKEYFILE,O_WRONLY | O_CREAT | O_TRUNC, 0600);

if (fd < 0)
   {
   printf("open %s failed: %s.",CFPUBKEYFILE,strerror(errno));
   return 0;
   }
 
if ((fp = fdopen(fd, "w")) == NULL )
   {
   printf("fdopen %s failed: %s.",CFPUBKEYFILE, strerror(errno));
   close(fd);
   return 0;
   }

printf("Writing public key to %s\n",CFPUBKEYFILE);
 
if(!PEM_write_RSAPublicKey(fp,pair))
   {
   err = ERR_get_error();
   printf("Error = %s\n",ERR_reason_error_string(err));
   return 1;
   }

fclose(fp);

 
snprintf(VBUFF,bufsize,"%s/randseed",VLOGDIR);
RAND_write_file(VBUFF);
chmod(VBUFF,0644); 
return 0;
}

/*******************************************************************/
/* Level 1                                                         */
/*******************************************************************/

void Initialize()

{
umask(077);
strcpy(VLOCKDIR,WORKDIR);
strcpy(VLOGDIR,WORKDIR); 

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();

CheckWorkDirectories();
RandomSeed();
}

/*******************************************************************/
/* linker tricks                                                   */
/*******************************************************************/

void AddMultipleClasses(classlist)

char *classlist;

{
}

void ReleaseCurrentLock()
{
}


int RecursiveTidySpecialArea(name,tp,maxrecurse,sb)

char *name;
struct Tidy *tp;
int maxrecurse;
struct stat *sb;

{
 return true;
}
