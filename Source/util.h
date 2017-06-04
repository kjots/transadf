/* util.h - Header file for util.c and util-asm.a
** Copyright (C) 1997,1998 Karl J. Ots
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

#ifndef TRANSADF_UTIL_H
#define TRANSADF_UTIL_H


/*------------------------------------*/
/* Miscellaneous functions and macros */
/*------------------------------------*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif /* DOS_DOS_H */

#ifndef CLIB_EXEC_PROTOS_H
#include <clib/exec_protos.h>
#endif /* CLIB_EXEC_PROTOS_H */

#ifndef CLIB_DOS_PROTOS_H
#include <clib/dos_protos.h>
#endif /* CLIB_DOS_PROTOS_H */

#ifndef TRANSADF_MAIN_H
#include "main.h"
#endif /* TRANSADF_MAIN_H */


/*---------------*/
/* Useful Macros */
/*---------------*/


/* Check to see if Control-C has been pressed */
#define CTRL_C         (SetSignal(NULL,NULL) & SIGBREAKF_CTRL_C)


/* File types as returned by getFileType() */
#define FT_UNKNOWN    0    /* Default file type.                          */
#define FT_DOS        1    /* AmigaDOS disk.                              */
#define FT_ZLIB       2    /* ZLib stream (as defined in RFC-1950).       */
#define FT_GZIP       3    /* GZip file (as defined in RFC-1952).         */
#define FT_PKZIP      4    /* 'Standard' Zip as used by PK- and Info-Zip. */
#define FT_PKZIP_ADD  5    /* Add a new file to a PKZip archive.          */


/*---------------------*/
/* Function prototypes */
/*---------------------*/

STRPTR b2cstr (BSTR bstring);

#ifndef COMPILE_LITE
ULONG  getFileType (BPTR file);
BOOL   writeHead (BPTR outFile, STRPTR origName, ULONG fileType);
BOOL   finishFile (BPTR outFile, ULONG CRC, ULONG CSize, ULONG USize, 
                   ULONG fileType);
BOOL   skipHead (BPTR inFile, STRPTR origName, ULONG fileType);
BOOL   readTail (BPTR inFile, ULONG *CRC, ULONG *USize, ULONG fileType);
ULONG  unixDate (void);
ULONG  dosDate (void);

/* These two change the byte-order of a supplied short or long respectively */
/* ie  LES (0x1234) ==> 0x3412,  LEL (0x12345678) ==> 0x78563412            */
/* They are defined in 'util-asm.a'                                         */
__regargs UWORD LES (register __D0 UWORD num);
__regargs ULONG LEL (register __D0 ULONG num);
#endif /* COMPILE_LITE */


#endif /* TRANSADF_UTIL_H */
