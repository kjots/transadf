/* args.h - Header file for args.c
** Copyright (C) 1998 Karl J. Ots
** 
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef TRANSADF_ARGS_H
#define TRANSADF_ARGS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */


/*---------------------------------*/
/* Global variables and structures */
/*---------------------------------*/

extern const char TA_Template[];

struct TA_Args {
  STRPTR  Drive;            /* Drive we will be operatin on               */
  STRPTR  File;             /* File we will be operatin on                */
  ULONG   Start;            /* Track to start operation (inclusive)       */
  LONG    End;              /* Track to end operation (inclusive)         */
  BOOL    WriteDisk;        /* TRUE if we're writing to Disk              */
  BOOL    Verify;           /* TRUE if we're verifiying WriteDisk         */
  BOOL    Format;           /* TRUE if we're formatting while we write    */
  
  BOOL    ZLib;             /* TRUE if we're writing to a ZLib file       */
  BOOL    GZip;             /* TRUE if we're writing to a GZip file       */
  BOOL    PKZip;            /* TRUE if we're writing to a PKZip file      */
  BOOL    PKZAdd;           /* TRUE if we're adding to above PKZip file   */
  
  STRPTR  OrigName;         /* Original filename to store in PK/GZip file */
  ULONG   Level;            /* Compression level                          */
};
  


/*---------------------*/
/* Function Prototypes */
/*---------------------*/

struct TA_Args *getArgs (void);


#endif /* TRANSADF_ARGS_H */
