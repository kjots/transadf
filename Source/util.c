/* util.c - Miscellaneous functions and macros
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

/*------------------------------------*/
/* Miscellaneous functions and macros */
/*------------------------------------*/

#include <exec/types.h>
#include <dos/dos.h>
#ifndef COMPILE_LITE
#  include <clib/dos_protos.h>

#  include <time.h>
#endif /* COMPILE_LITE */

#include <string.h>

#include "util.h"
#ifndef COMPILE_LITE
#  include "gzip.h"
#  include "pkzip.h"
#endif /* COMPILE_LITE */


/*
** Convert a BString to a CString.
** String must be duplicated if you wish to keep it.
*/
STRPTR b2cstr (BSTR bstring)
{
  static char cstring[255];
  STRPTR realbstr;
  UBYTE len;
  int i;
  
  realbstr = (STRPTR) BADDR (bstring);
  len = realbstr[0];
  
  for (i=0; i<len; i++)
    cstring[i] = realbstr[i+1];
  cstring[i] = '\0';
  
  return cstring;
}


/* The following routines are only used in conjunction with de/compression */
#ifndef COMPILE_LITE

/* 
** Return the file type, based on the 'magic number'
** ie the first few charcters.
** This function preserves current file position.
*/
ULONG getFileType (BPTR file)
{
  UBYTE mag_num[4];
  static UBYTE pkzip_mag[4] = {'P', 'K', 0x03, 0x04};
  static UBYTE gzip_mag[2]  = {0x1F, 0x8B};
  static UBYTE dos_mag[3]   = {'D',  'O',  'S'};
  LONG oldp;
  
  oldp = Seek (file, 0, OFFSET_BEGINNING);
  if ( Read (file, mag_num, 4) != 4) return FT_UNKNOWN;
  Seek (file, oldp, OFFSET_BEGINNING);
  
  /* These are straight-forward comparisons */
  if (!memcmp (mag_num, pkzip_mag, 4))
    return FT_PKZIP;
  if (!memcmp (mag_num, gzip_mag, 2))
    return FT_GZIP;
  if (!memcmp (mag_num, dos_mag, 3))
    return FT_DOS;
  
  /* Zlib is a little tricky - The first char is usually 0x78, and the */
  /* first two chars, when viewed together as an unsigned 16-bit int,  */
  /* is a multiple of 31.                                              */
  /* This probably needs a litte more work :)                          */
  if ((mag_num[0] == 0x78) && !(((UWORD *)mag_num)[0] % 31))
    return FT_ZLIB;
  
  /* If we get here, the type is unknown */
  return FT_UNKNOWN;
}


/*
** Output a Header to the specified file depending on fileType.
** Return TRUE if no errors. else FALSE.
*/
BOOL writeHead (BPTR outFile, STRPTR origName, ULONG fileType)
{
  /* Assume that the file is at the start */

  switch (fileType)
  {
  case FT_ZLIB:
    /* No action necessary */
    return TRUE;
  
  case FT_GZIP:
    return writeGZHead (outFile, origName);
  
  case FT_PKZIP:
    return writePKZHead (outFile, origName);
  
  case FT_PKZIP_ADD:
    return writePKZHeadAdd (outFile, origName);
  }
  
  /* Unknown type, error! */
  return FALSE;
}


/*
** Finish writing a file depending on fileType.
** Return TRUE if no errors. else FALSE.
*/
BOOL finishFile (BPTR outFile, ULONG CRC, ULONG CSize, ULONG USize, 
                ULONG fileType)
{
  switch (fileType)
  {
  case FT_ZLIB:
    /* No action necessary */
    return TRUE;
  
  case FT_GZIP:
    return finishGZFile (outFile, CRC, USize);
  
  case FT_PKZIP:
    return finishPKZFile (outFile, CRC, CSize, USize);
    
  case FT_PKZIP_ADD:
    return finishPKZFileAdd (outFile, CRC, CSize, USize);
  }
  
  /* Unknown type, error! */
  return FALSE;
}


/*
** Skip the header of a specified file depending on file type.
** Return TRUE if no errors. else FALSE.
*/
BOOL skipHead (BPTR inFile, STRPTR origName, ULONG fileType)
{
  /* Assume that the file is at the start */

  switch (fileType)
  {
  case FT_ZLIB:
    /* No action necessary */
    return TRUE;
  
  case FT_GZIP:
    return skipGZHead (inFile);
  
  case FT_PKZIP:
    return skipPKZHead (inFile, origName);
  }
  
  /* Unknown type, error! */
  return FALSE;
}


/*
** Read the a file depending on fileType and return the CRC
** and USize in supplied arrays.
** Return TRUE if no errors, else FALSE.
*/
BOOL readTail (BPTR inFile, ULONG *CRC, ULONG *USize, ULONG fileType)
{
  switch (fileType)
  {
  case FT_ZLIB:
    /* No action necessary */
    return TRUE;
  
  case FT_GZIP:
    return readGZTail (inFile, CRC, USize);
  
  case FT_PKZIP:
    return readPKZTail (inFile, CRC, USize);
  }
  
  /* Unknown type, error! */
  return FALSE;
}


/*
** Return the current date and time in UNIX format.
** (ie No. seconds after 1-Jan-1970).
*/
ULONG unixDate (void)
{
  struct DateStamp ds;
  
  DateStamp (&ds);
  
  /* Need to add 2922 Days (1970->1978) */
  
  return ((ds.ds_Tick / TICKS_PER_SECOND) + 
          (ds.ds_Minute * 60) +
          ((ds.ds_Days + 2922) * 86400));
}


/*
** Return the current date and time in DOS format.
*/
ULONG  dosDate (void)
{
  /* The DOS date format is as follows:
   * 7 bits: Year after 1980 (0 - 127).
   * 4 bits: Month (1 - 12).
   * 5 bits: Day (1 - 31).
   * (16 bits)
   * 
   * 5 bits: Hour (0 - 23).
   * 6 bits: Minute (0 - 59).
   * 5 bits: Seconds / 2 (0 - 29).
   * (16 bits)
   * 
   * Total = 32 bits = 1 long word.
   */
  
  time_t t;
  struct tm *tp;
  
  /* Get the time */
  t = time (NULL); tp = localtime (&t);
  
  /* Adjust year to within DOS range */
  if (tp->tm_year < 80) return 0x00210000;   /* 1-1-1980, 00:00:00 */
  
  return (((tp->tm_year - 80) << 25) | ((tp->tm_mon+1) << 21) | 
          (tp->tm_mday << 16) | (tp->tm_hour << 11) | (tp->tm_min << 5) |
          (tp->tm_sec >> 1));
}


#endif /* COMPILE_LITE */
