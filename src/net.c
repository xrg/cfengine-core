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

int SendTransaction(int sd,char *buffer,int len,char status)

{ char work[CF_BUFSIZE];
  int wlen;
 
if (len == 0) 
   {
   wlen = strlen(buffer);
   }
else
   {
   wlen = len;
   }
    
if (wlen > CF_BUFSIZE-CF_INBAND_OFFSET)
   {
   FatalError("SendTransaction software failure");
   }
 
snprintf(work,CF_INBAND_OFFSET,"%c %d",status,wlen);

memcpy(work+CF_INBAND_OFFSET,buffer,wlen);

Debug("Transaction Send[%s][Packed text]\n",work); 
 
if (SendSocketStream(sd,work,wlen+CF_INBAND_OFFSET,0) == -1)
   {
   return -1;
   }

return 0; 
}

/*************************************************************************/

int ReceiveTransaction(int sd,char *buffer,int *more)

{ char proto[CF_INBAND_OFFSET+1];
  char status = 'x';
  unsigned int len = 0;
 
memset(proto,0,CF_INBAND_OFFSET+1);

if (RecvSocketStream(sd,proto,CF_INBAND_OFFSET,0) == -1)   /* Get control channel */
   {
   return -1;
   }

sscanf(proto,"%c %u",&status,&len);
Debug("Transaction Receive [%s][%s]\n",proto,proto+CF_INBAND_OFFSET);

if (len > CF_BUFSIZE - CF_INBAND_OFFSET)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Bad transaction packet -- too long (%c %d) Proto = %s ",status,len,proto);
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
 
int RecvSocketStream(int sd,char buffer[CF_BUFSIZE],int toget,int nothing)
 
{ int already, got;
  static int fraction;

Debug("RecvSocketStream(%d)\n",toget);

if (toget > CF_BUFSIZE-1)
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
      buffer[already] = '\0';
      return already;
      }

   Debug("    (Concatenated %d from stream)\n",got);

   if (strncmp(buffer,"AUTH",4) == 0 && (already == CF_BUFSIZE))
      {
      fraction = 0;
      buffer[already] = '\0';
      return already;
      }
   }

buffer[toget] = '\0';
return toget;
}


/*************************************************************************/

/*
 * Drop in replacement for send but includes
 * guaranteed whole buffer sending.
 * Wed Feb 28 11:30:55 GMT 2001, Morten Hermanrud, mhe@say.no
 */

int SendSocketStream(int sd,char buffer[CF_BUFSIZE],int tosend,int flags)

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
 while (already < tosend); 

 return already;
}

/*************************************************************************/
