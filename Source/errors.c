/* errors.c - Output various error messages
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

#include <exec/types.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

#include "errors.h"
#include "main.h"
#include "zlib.h"


/* Private functions */
void reportTDError (BYTE TDError);


/*
** Output a DOS error value, as returned by IoErr(), as a text string
** to StdErr.
*/
void reportDOSError (LONG DOSError)
{
  char ErrorString[80];
  
  if (Fault (DOSError, "DOS Error", ErrorString, 79))
    FPrintf (StdErr, "%s.\n", ErrorString);
  else
    FPrintf (StdErr, "DOS Error: %ld.\n", DOSError);
}


/* 
** Output an IO Error, as returned by DoIO(), as a text 
** string to stderr.
*/
void reportIOError (BYTE IOError)
{
  STRPTR ErrStr;
  
  /* Check to see if this is a TrackDisk error */
  if ((IOError >= TDERR_NotSpecified) && (IOError <= TDERR_PostReset))
  {
    reportTDError (IOError);
    return;
  }
  
  switch (IOError) {
  case IOERR_OPENFAIL:
    ErrStr = "Open failure";
    break;
  case IOERR_ABORTED:
    ErrStr = "IO aborted";
    break;
  case IOERR_NOCMD:
    ErrStr = "Unknown command";
    break;
  case IOERR_BADLENGTH:
    ErrStr = "Bad length";
    break;
  case IOERR_BADADDRESS:
    ErrStr = "Bad address";
    break;
  case IOERR_UNITBUSY:
    ErrStr = "Unit is busy";
    break;
  case IOERR_SELFTEST:
    ErrStr = "Self test failure";
    break;
  default:
    ErrStr = "Unknown Error";
  }
  
  FPrintf (StdErr, "%s (%ld).\n", ErrStr, IOError);
}


/*
** Output a TrackDisk IO Error, as returned by DoIO(),
** as a text string to stderr.
*/
void reportTDError (BYTE TDError)
{
  STRPTR ErrStr;

  switch (TDError) {
  case TDERR_NotSpecified:
    ErrStr = "Not specified";
    break;
  case TDERR_NoSecHdr:
    ErrStr = "No sector header";
    break;
  case TDERR_BadSecPreamble:
    ErrStr = "Bad sector preamble";
    break;
  case TDERR_BadSecID:
    ErrStr = "Bad sector ID";
    break;
  case TDERR_BadHdrSum:
    ErrStr = "Bad header checksum";
    break;
  case TDERR_BadSecSum:
    ErrStr = "Bad sector checksum";
    break;
  case TDERR_TooFewSecs:
    ErrStr = "Not enough sectors"; 
    break;
  case TDERR_BadSecHdr:
    ErrStr = "Bad sector header";
    break;
  case TDERR_WriteProt:
    ErrStr = "Disk is write protected";
    break;
  case TDERR_DiskChanged:
    ErrStr = "No disk in drive";
    break;
  case TDERR_SeekError:
    ErrStr = "Seek error";
    break;
  case TDERR_NoMem:
    ErrStr = "Out of memory";
    break;
  case TDERR_BadUnitNum:
    ErrStr = "No such unit";
    break;
  case TDERR_BadDriveType:
    ErrStr = "Unknown drive type";
    break;
  case TDERR_DriveInUse:
    ErrStr = "Drive is in use";
    break;
  case TDERR_PostReset:
    ErrStr = "Post Reset";
    break;
  }
  
  FPrintf (StdErr, "%s (%ld).\n", ErrStr, TDError);
}



/*
** Output a Zlib error as a test string.
*/
void reportZLibError (LONG ZLibError)
{
  STRPTR ErrStr;
  
  switch (ZLibError) {
  case Z_ERRNO:
    ErrStr = "Error Number";
    break;
  case Z_STREAM_ERROR:
    ErrStr = "Stream Error";
    break;
  case Z_DATA_ERROR:
    ErrStr = "Data Error";
    break;
  case Z_MEM_ERROR:
    ErrStr = "Memory Error";
    break;
  case Z_BUF_ERROR:
    ErrStr = "Buffer Error";
    break;
  case Z_VERSION_ERROR:
    ErrStr = "Version Error";
    break;  
  default:
    ErrStr = NULL;
  }

  FPuts (StdErr, "ZLib Error: ");
  if (ErrStr)
    FPuts (StdErr, ErrStr);
  else
    FPrintf (StdErr, "Unknown (%ld)", ZLibError);
  FPuts (StdErr, ".\n");
}
