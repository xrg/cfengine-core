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
/*  TOOLKITS: network library                                        */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*************************************************************************/
  
void TimeOut()
 
{
alarm(0);
Verbose("%s: Time out\n",VPREFIX);
}

/*************************************************************************/

int SendTransaction(sd,buffer,len,status)

char *buffer,status;
int sd,len;

{ char work[bufsize];
  int wlen;
 
if (len == 0) 
   {
   wlen = strlen(buffer);
   }
else
   {
   wlen = len;
   }
    
if (wlen > bufsize-8)
   {
   FatalError("SendTransaction software failure");
   }
 
snprintf(work,8,"%c %d",status,wlen);

bcopy(buffer,work+8,wlen);

Debug("Transaction Send[%s][Packed text]\n",work); 
 
if (SendSocketStream(sd,work,wlen+8,0) == -1)
   {
   return -1;
   }

return 0; 
}

/*************************************************************************/

int ReceiveTransaction(sd,buffer,more)

char *buffer;
int sd,*more;

{ char proto[9];
  char status;
  unsigned int len = 0;
 
bzero(proto,9);

if (RecvSocketStream(sd,proto,8,0) == -1)   /* Get control channel */
   {
   return -1;
   }

sscanf(proto,"%c %u",&status,&len);
Debug("Transaction Receive [%s][%s]\n",proto,proto+8);

if (len > bufsize - 8)
   {
   snprintf(OUTPUT,bufsize,"Bad transaction packet -- too long (%c %d) Proto = %s ",status,len,proto);
   CfLog(cferror,OUTPUT,"");
   return -1;
   }

if (strncmp(proto,"CAUTH",5) == 0)
   {
   Debug("Version 1 protocol connection attempted - no you don't!!\n");
   return -1;
   }
 
if (more != NULL)
   {
   switch(status)
      {
      case 'm': *more = true;
	  break;
      default: *more = false;
      }
   }
 
return RecvSocketStream(sd,buffer,len,0);
}

/*************************************************************************/
 
int RecvSocketStream(sd,buffer,toget,nothing)
 
int sd,toget,nothing;
char buffer[bufsize];
 
{ int already, got;
  static int fraction;

Debug("RecvSocketStream(%d)\n",toget);

if (toget > bufsize)
   {
   CfLog(cferror,"Bad software request for overfull buffer","");
   return -1;
   }
 
for (already = 0; already != toget; already += got)
   {
   got = recv(sd,buffer+already,toget-already,0);

   if (got == -1)
      {
      CfLog(cfverbose,"Couldn't recv","recv");
      return -1;
      }
 
   if (got == 0)   /* doesn't happen unless sock is closed */
      {
      Debug("Transmission empty or timed out...\n");
      fraction = 0;
      return already;
      }

   Debug("    (Concatenated %d from stream)\n",got);

   if (strncmp(buffer,"AUTH",4) == 0 && (already == bufsize))
      {
      fraction = 0;
      return already;
      }
   }

return toget;
}


/*************************************************************************/

/*
 * Drop in replacement for send but includes
 * guaranteed whole buffer sending.
 * Wed Feb 28 11:30:55 GMT 2001, Morten Hermanrud, mhe@say.no
 */

int SendSocketStream(sd,buffer,tosend,flags)
int sd,tosend,flags;
char buffer[bufsize];

{ int sent,already=0;

do
   {
   Debug("Attempting to send %d bytes\n",tosend-already);

   sent=send(sd,buffer+already,tosend-already,flags);
   
   switch(sent)
      {
      case -1:
	  CfLog(cfverbose,"Couldn't send","send");
	  return -1;
      default:
	  Debug("SendSocketStream, sent %d\n",sent);
	  already += sent;
	  break;
      }
   }

while(already < tosend);

return already;
}

/*************************************************************************/
