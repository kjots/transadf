/* defl_disk.c - Delfate a disk into a compressed file
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
#include <exec/memory.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include "zlib.h"

#include "defl_disk.h"
#include "main.h"
#include "mem_chunks.h"
#include "device.h"
#include "util.h"
#include "errors.h"


#define DD_TBTRACKS 1                           /* Tracks in dd_TrackBuf */
#define DD_TBSIZE   (DD_TBTRACKS * trackSize)   /* Bytes in dd_TrackBuf  */
#define DD_FBSIZE   (32*1024)                   /* Bytes in dd_FileBuf   */

UBYTE *dd_TrackBuf;    /* Our input buffer - reads from disk. */
UBYTE *dd_FileBuf;     /* Our output buffer - writes to file. */


/*
** Deflate a disk into a file.
** Expects all fields of adfPkt to be set.
*/
void deflDisk (struct ADF_Packet *adfPkt, ULONG deflLevel, STRPTR origName,
               ULONG fileType)
{
  static z_stream defl_stream;
  ULONG nextTrack, CRC, USize, CSize;
  LONG DOSError;
  BYTE IOError;
  int zerr, ocnt, deflg, winbits;
  
  
  /* Output info header */
  FPrintf (StdOut, "Deflating from %s unit %ld (%s:) to %s.\n",
                   adfPkt->devInfo->deviceName,
                   adfPkt->devInfo->deviceUnit,
                   adfPkt->devInfo->dosName,
                   adfPkt->ADFileName);
  
  FPrintf (StdOut, "Starting at track %ld, Ending at track %ld.\n",
                   (adfPkt->startTrack/adfPkt->devInfo->numHeads),
                   (adfPkt->endTrack/adfPkt->devInfo->numHeads));
  
  FPuts (StdOut, "Compressing disk as a ");
  switch (fileType) {
  case FT_ZLIB:      FPuts (StdOut, "ZLib");  break;
  case FT_GZIP:      FPuts (StdOut, "GZip");  break;
  case FT_PKZIP:     
  case FT_PKZIP_ADD: FPuts (StdOut, "PKZip"); break;
  }
  FPuts (StdOut, " file.\n");
  
  
  /* Allocate the buffers */
  dd_TrackBuf = (UBYTE *)myAllocMem (DD_TBSIZE, MEMF_CLEAR);
  dd_FileBuf  = (UBYTE *)myAllocMem (DD_FBSIZE, MEMF_CLEAR);
  if (!dd_TrackBuf || !dd_FileBuf)
  {
    /* No Memory */
    FPrintf (StdErr, "%s: Out of memory.\n", ProgName);
    cleanExit(RETURN_FAIL, ERROR_NO_FREE_STORE);
  }
  
  /* Output the file header */
  if (writeHead (adfPkt->ADFile, origName, fileType) == FALSE)
  {
    DOSError = IoErr();
    
    FPrintf (StdErr, "%s: Error - Couldn't write file header.\n",ProgName);
    
    if (DOSError)
      reportDOSError(DOSError);
    
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Initialise the z_stream */
  if (fileType == FT_ZLIB) winbits = 15;
  else winbits = -15; /* windowBits is passed < 0 to suppress zlib header */    
  defl_stream.zalloc = Z_NULL;
  defl_stream.zfree  = Z_NULL;
  defl_stream.opaque = Z_NULL;
  zerr = deflateInit2 (&defl_stream, deflLevel,
                       Z_DEFLATED, winbits, 8, 0);
  if (zerr != Z_OK)
  {
    FPrintf (StdErr, "%s: Deflate Init Error - ", ProgName);
    reportZLibError (zerr);
    if (defl_stream.msg) FPrintf (StdErr, "\t(%s)\n", defl_stream.msg);
    cleanExit (RETURN_FAIL, NULL);
  }
    
  /* Start deflateing */
  defl_stream.avail_in  = 0;           /* Input buffer is empty    */
  defl_stream.next_out  = dd_FileBuf;  /* Pointer to output buffer */
  defl_stream.avail_out = DD_FBSIZE;   /* output buffer is empty   */
  deflg = Z_NO_FLUSH;
  CRC = crc32 (NULL, Z_NULL, 0);       /* Initialise the CRC       */
  nextTrack = adfPkt->startTrack;
  while (1)
  {
    if (defl_stream.avail_in == 0)  /* Input is empty */
      if (nextTrack <= adfPkt->endTrack)
      {
        /* Update progress report */
        FPrintf (StdOut, "\rReading   track %ld side %ld",
                         (nextTrack/adfPkt->devInfo->numHeads),
                         (nextTrack%adfPkt->devInfo->numHeads));
        Flush (StdOut);
        
        /* Fill the input buffer from disk */
        IOError = readTrack (dd_TrackBuf, DD_TBTRACKS, nextTrack, 
                             adfPkt->diskReq);
        if (IOError)
        {
          FPutC (StdOut, '\n');
          FPrintf (StdErr, "%s: Error reading from %s: - ",
                           ProgName, adfPkt->devInfo->dosName);
          reportIOError (IOError);
          deflateEnd (&defl_stream);
          cleanExit (RETURN_FAIL, NULL);
        }
        
        nextTrack++;
        defl_stream.next_in  = dd_TrackBuf;
        defl_stream.avail_in = DD_TBSIZE;
      }
    
    if (defl_stream.avail_in == 0)  /* Still zero - no more input */
      deflg = Z_FINISH;
    else
      /* New data - Update the CRC */
      CRC = crc32 (CRC, dd_TrackBuf ,defl_stream.avail_in);
    
    /* Fill the output buffer with deflated data, and keep filling */
    /* it until no more inflated data exists in the input buffer.  */
    while (1)
    {
      /* Check for Control-C break */
      if (CTRL_C)
      {
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s - %s\n", breakText, ProgName);
        deflateEnd (&defl_stream);
        cleanExit (RETURN_WARN, NULL);
      }
      
      /* Upate progress report */
      FPuts (StdOut, "\rDeflating ");
      Flush (StdOut);
      
      zerr = deflate (&defl_stream, deflg);
      if (zerr < Z_OK)
      {
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s: Deflate error - ", ProgName);
        reportZLibError (zerr);
        if (defl_stream.msg) FPrintf (StdErr, "\t%s\n", defl_stream.msg);
        deflateEnd (&defl_stream);
        cleanExit (RETURN_FAIL, NULL);
      }
      
      /* Flush the output buffer */
      ocnt = DD_FBSIZE - defl_stream.avail_out;
      if (ocnt) 
      {
        if ( Write (adfPkt->ADFile, dd_FileBuf, ocnt) != ocnt)
        {
          DOSError = IoErr();
          
          FPutC (StdOut, '\n');
          FPrintf (StdErr, "%s: Error writing deflated data - ", ProgName);
          reportDOSError(DOSError);
          
          cleanExit (RETURN_FAIL, DOSError);
        }
      }
      defl_stream.next_out  = dd_FileBuf;
      defl_stream.avail_out = DD_FBSIZE;
      
      if ((ocnt != DD_FBSIZE) || (zerr == Z_STREAM_END))
        /* Last deflate() did not completely fill the output buffer,  */
        /* thus no more data is pending, or we reached the end of the */
        /* compressed data stream.                                    */
        break;
    }
    if (zerr == Z_STREAM_END) break;
  }
  USize = defl_stream.total_in;
  CSize = defl_stream.total_out;
  
  FPrintf (StdOut, "\rDeflated: %ld ==> %ld (%ld%%).   ",
                   USize, CSize, ((CSize * 100) / USize));
  Flush (StdOut);
  
  /* End the deflate process */
  zerr = deflateEnd (&defl_stream);
  if (zerr != Z_OK)
  {
    FPutC (StdOut, '\n');
    FPrintf (StdErr, "%s: Deflate End Error - ", ProgName);
    reportZLibError (zerr);
    if (defl_stream.msg) FPrintf (StdErr, "\t(%s)\n", defl_stream.msg);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Finish the file */
  if ( finishFile (adfPkt->ADFile, CRC, CSize, USize, fileType) == FALSE)
  {
    FPutC (StdOut, '\n');
    FPrintf (StdErr, "%s: Error - Couldn't finish output file correctly.\n",
                     ProgName);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Free and clear buffers */
  myFreeMem (dd_TrackBuf);
  myFreeMem (dd_FileBuf);
}
