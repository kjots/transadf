/* gzip.c - Handle all GZip-file specific tasks
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

/*--------------------*/
/* GZip file routines */
/*--------------------*/

#include <exec/types.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

#include <string.h>

#include "gzip.h"
#include "main.h"
#include "util.h"
#include "errors.h"


/*-----------------------*/
/* Structures and macros */
/*-----------------------*/

struct GZHead {
  UWORD  MagicNum;
  UBYTE  CMethod;    
  UBYTE  Flags;
  ULONG  Date;
  UBYTE  EFlags;
  UBYTE  OS;
}; /* sizeof = 10 */

struct GZTail {
  ULONG  CRC;
  ULONG  USize;
};

#define GZ_MAGNUM    0x1F8B

/* Flags */
#define GZ_FTEXT     0x01  /* Bit 0           */
#define GZ_FHCRC     0x02  /* Bit 1           */
#define GZ_FEXTRA    0x04  /* Bit 2           */
#define GZ_FNAME     0x08  /* Bit 3           */
#define GZ_FCOMMENT  0x10  /* Bit 4           */
#define GZ_RESERVED  0xE0  /* Bits 5, 6 and 7 */


/* 
** Output a GZip Header to the specified file.
** Return TRUE if no errors. else FALSE.
*/
BOOL writeGZHead (BPTR outFile, STRPTR origName)
{
  UWORD nlen;
  struct GZHead gzHead = {GZ_MAGNUM,  /* GZip Magic Number.       */
                          8,          /* Method - deflate.        */
                          0,          /* Flags (to be filled in). */
                          0,          /* Date  (to be filled in). */
                          0,          /* Extra flags.             */
                          1};         /* OS Code (1 = Amiga)      */
  
  /* Fill in the flags */
  if (origName)
    gzHead.Flags |= GZ_FNAME;  /* Original name supplied */
  
  /* Fill in the date (little endian) */
  gzHead.Date = LEL (unixDate ());
  
  /* Write the header to the file */
  if ( Write (outFile, &gzHead, 10) != 10) return FALSE;
  
  /* Write extra data if applicable */
  if (origName)
  {
    nlen = strlen (origName) + 1;
    if ( Write (outFile, origName, nlen) != nlen) return FALSE;
  }
  
  /* Header written sucessfully */
  return TRUE;
}


/*
** Finish writing a GZip file.
** Return TRUE if no errors. else FALSE.
*/
BOOL finishGZFile (BPTR outFile, ULONG CRC, ULONG USize)
{
  struct GZTail gzTail;
  
  gzTail.CRC   = LEL (CRC);
  gzTail.USize = LEL (USize);
  
  if ( Write (outFile, &gzTail, 8) != 8) return FALSE;
  
  return TRUE;
}


/*
** Skip the GZip Header of a specified file.
** Return TRUE if no errors. else FALSE.
*/
BOOL skipGZHead (BPTR inFile)
{
  struct GZHead gzHead;
  UBYTE Char, Flags;
  UWORD ESize;
  
  /* Read in the header */
  if ( Read (inFile, &gzHead, 10) != 10) return FALSE;
  
  /* Check the compression method and the reserved flags   */
  /* (We assume the magic number has already been checked) */
  if (gzHead.CMethod != 8)
  {
    FPrintf (StdErr,
             "%s: Error - Only Deflate compression method supported.\n",
             ProgName);
    cleanExit (RETURN_ERROR, NULL);
  }
  
  Flags = gzHead.Flags;
  if (Flags & GZ_RESERVED)
  {
    FPrintf (StdErr,
             "%s: Warning - Reserved flags set, decompression may fail.\n",
             ProgName);
  }
  
  /* Skip the rest of the header depending on the flags */
  
  if (Flags & GZ_FEXTRA)  /* Extra field */
  {
    if ( Read (inFile, &ESize, 2) != 2) return FALSE;
    ESize = LES (ESize);
    Seek (inFile, ESize, OFFSET_CURRENT);
  }
  
  if (Flags & GZ_FNAME)  /* File Name */
  {
    Char = 1;
    while (Char)
      if (Read (inFile, &Char, 1) != 1) return FALSE;      
  }
  
  if (Flags & GZ_FCOMMENT)  /* File Comment */
  {
    Char = 1;
    while (Char)
      if (Read (inFile, &Char, 1) != 1) return FALSE;      
  }
  
  if (Flags & GZ_FHCRC)  /* Header CRC */
    Seek (inFile, 2, OFFSET_CURRENT);
  
  /* inFile is now at compressed data */
  return TRUE;
}


/*
** Read the end of a GZip file and return the CRC
** and USize in supplied arrays.
** Return TRUE if no errors, else FALSE.
*/
BOOL readGZTail (BPTR inFile, ULONG *CRC, ULONG *USize)
{
  struct GZTail gzTail;
  
  Seek (inFile, -8, OFFSET_END);
  if ( Read (inFile, &gzTail, 8) != 8) return FALSE;
  
  *CRC   = LEL (gzTail.CRC);
  *USize = LEL (gzTail.USize);
  
  return TRUE;
}
