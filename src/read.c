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
 
/*===========================================================================*/
/*                                                                           */
/* File: read.c                                                              */
/*                                                                           */
/* Created: Mon Sep 29 09:22:33 1997                                         */
/*                                                                           */
/* Revision: $Id$                                                            */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/*===========================================================================*/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/

int ReadLine(buff,size,fp)

char *buff;
int size;
FILE *fp;

{
buff[0] = '\0';
buff[size - 1] = '\0';                        /* mark end of buffer */

if (fgets(buff, size, fp) == NULL)
   {
   *buff = '\0';                   /* EOF */
   return false;
   }
else
   {
   char *tmp;

   /* remove newline */

   if ((tmp = strrchr(buff, '\n')) != NULL)
      {
      *tmp = '\0';
      }
   }
 
return true; 
}
