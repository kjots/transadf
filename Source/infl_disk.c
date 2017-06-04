/* infl_disk.c = Inflate a compressed file onto a disk
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

#include <string.h>

#include "zlib.h"

#include "infl_disk.h"
#include "main.h"
#include "mem_chunks.h"
#include "device.h"
#include "util.h"
#include "errors.h"


#define ID_FBSIZE   (32*1024)                   /* Bytes in id_FileBuf   */
#define ID_TBTRACKS 1                           /* Tracks in id_TrackBuf */
#define ID_TBSIZE   (ID_TBTRACKS * trackSize)   /* Bytes in id_TrackBuf  */

UBYTE *id_FileBuf;     /* Our input buffer - reads from file.     */
UBYTE *id_TrackBuf;    /* Our output buffer - writes to disk.     */
UBYTE *id_VTrackBuf;   /* Buffer for verifiying data if required. */

/*
** Inflate a file into a disk.
** Expects all fields of adfPkt to be set.
*/
void inflDisk (struct ADF_Packet *adfPkt, STRPTR origName, ULONG fileType)
{
  static z_stream infl_stream;
  ULONG nextTrack, CRC, USize, origCRC, origUSize;
  LONG DOSError;
  BYTE IOError;
  int zerr, winbits;
  
  
  /* Output info header */
  FPrintf (StdOut, "Inflating from %s to %s unit %ld (%s:).\n",
                   adfPkt->ADFileName,
                   adfPkt->devInfo->deviceName,
                   adfPkt->devInfo->deviceUnit,
                   adfPkt->devInfo->dosName);
  
  FPrintf (StdOut, "Starting at track %ld, Ending at track %ld.\n",
                   (adfPkt->startTrack/adfPkt->devInfo->numHeads),
                   (adfPkt->endTrack/adfPkt->devInfo->numHeads));
  
  FPuts (StdOut, "Input is a ");
  switch (fileType) {
  case FT_ZLIB:  FPuts (StdOut, "ZLib");  break;
  case FT_GZIP:  FPuts (StdOut, "GZip");  break;
  case FT_PKZIP: FPuts (StdOut, "PKZip"); break;
  }
  FPuts (StdOut, " file, decompressing.\n");
  
  
  /* Allocate the buffers */
  id_TrackBuf = (UBYTE *)myAllocMem (ID_TBSIZE, MEMF_CLEAR);
  id_FileBuf  = (UBYTE *)myAllocMem (ID_FBSIZE, MEMF_CLEAR);
  if (!id_TrackBuf || !id_FileBuf)
  {
    /* No Memory */
    FPrintf (StdErr, "%s: Out of memory.\n", ProgName);
    cleanExit(RETURN_FAIL, ERROR_NO_FREE_STORE);
  }

  if (adfPkt->verify)
  {
    id_VTrackBuf = (UBYTE *) myAllocMem (ID_TBSIZE, MEMF_CLEAR);
    if (!id_VTrackBuf)
    {
      /* Out of memory! */
      FPrintf (StdErr, "%s: Out of memory.\n", ProgName);
      cleanExit (RETURN_FAIL, ERROR_NO_FREE_STORE);
    }
  }
  
  /* Consume file header */
  if (skipHead (adfPkt->ADFile, origName, fileType) == FALSE)
  {
    DOSError = IoErr();
    
    FPrintf (StdErr, "%s: Couldn't read file header.\n",ProgName);
    
    if (DOSError)
      reportDOSError (DOSError);
    
    cleanExit (RETURN_FAIL, NULL);
  }
  
  
  /* Initialise the z_stream */
  if (fileType == FT_ZLIB) winbits = 15;
  else winbits = -15; /* windowBits is passed < 0 to suppress zlib header */    
  infl_stream.zalloc = Z_NULL;
  infl_stream.zfree  = Z_NULL;
  infl_stream.opaque = Z_NULL;
  zerr = inflateInit2 (&infl_stream, winbits);
  if (zerr != Z_OK)
  {
    FPrintf (StdErr, "%s: Inflate Init Error - ", ProgName);
    reportZLibError (zerr);
    if (infl_stream.msg) FPrintf (StdErr, "\t(%s)\n", infl_stream.msg);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Start inflating */
  infl_stream.avail_in  = 0;            /* Input buffer is empty    */
  infl_stream.next_out  = id_TrackBuf;  /* Pointer to output buffer */
  infl_stream.avail_out = ID_TBSIZE;    /* output buffer is empty   */
  CRC = crc32 (NULL, Z_NULL, 0);        /* Initialise the CRC       */
  nextTrack = adfPkt->startTrack;
  while (1)
  {
    if (infl_stream.avail_in == 0)  /* Input is empty */
    {
      /* Fill the input buffer from file */
      infl_stream.next_in  = id_FileBuf;
      infl_stream.avail_in = Read (adfPkt->ADFile, id_FileBuf, ID_FBSIZE);
      if (infl_stream.avail_in == 0)
      {
        FPrintf (StdErr, "%s: Error - Unexpected End-Of-File.\n", ProgName);
        cleanExit (RETURN_ERROR, NULL);
      }
    }
    
    /* Fill the output buffer with inflated data */
    while (1)
    {
      /* Check for Control-C break */
      if (CTRL_C)
      {
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s - %s\n", breakText, ProgName);
        inflateEnd (&infl_stream);
        cleanExit (RETURN_WARN, NULL);
      }
      
      /* Upate progress report */
      FPuts (StdOut, "\rInflating  ");
      Flush (StdOut);
      
      zerr = inflate (&infl_stream, Z_NO_FLUSH);
      if (zerr < Z_OK)
      {
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s: Inflate error - ", ProgName);
        reportZLibError (zerr);
        if (infl_stream.msg) FPrintf (StdErr, "\t%s\n", infl_stream.msg);
        inflateEnd (&infl_stream);
        cleanExit (RETURN_FAIL, NULL);
      }
      
      /* Only flush the output buffer when it is completely full */
      if (infl_stream.avail_out == 0)
      {
        if (nextTrack <= adfPkt->endTrack)
        {
          /* Update the CRC */
          CRC = crc32 (CRC, id_TrackBuf, ID_TBSIZE);
          
          if (adfPkt->format)
          {
            /* Format the track */
          
            /* Update progress report */
            FPrintf (StdOut, "\rFormatting track %ld side %ld",
                             (nextTrack/adfPkt->devInfo->numHeads),
                             (nextTrack%adfPkt->devInfo->numHeads));
            Flush (StdOut);
            
            /* Format the track */
            IOError = formatTrack (id_TrackBuf, ID_TBTRACKS, nextTrack,
                                   adfPkt->diskReq);
            if (IOError)
            {
              FPutC (StdOut, '\n');
              FPrintf (StdErr, "%s: Error formatting %s: - ",
                               ProgName, adfPkt->devInfo->dosName);
              reportIOError (IOError);
              inflateEnd (&infl_stream);
              cleanExit (RETURN_FAIL, NULL);
            }
          }

          /* Update progress report */
          FPrintf (StdOut, "\rWriting    track %ld side %ld",
                           (nextTrack/adfPkt->devInfo->numHeads),
                           (nextTrack%adfPkt->devInfo->numHeads));
          Flush (StdOut);
          
          /* Write to the disk */
          IOError = writeTrack (id_TrackBuf, ID_TBTRACKS, nextTrack,
                                adfPkt->diskReq);
          if (IOError)
          {
            FPutC (StdOut, '\n');
            FPrintf (StdErr, "%s: Error writing to %s: - ",
                             ProgName, adfPkt->devInfo->dosName);
            reportIOError (IOError);
            inflateEnd (&infl_stream);
            cleanExit (RETURN_FAIL, NULL);
          }
          
          if (adfPkt->verify)
          {
            /* Verify that the data was written correctly */
            FPrintf (StdOut, "\rVerifying track %ld side %ld",
                             (nextTrack/adfPkt->devInfo->numHeads),
                             (nextTrack%adfPkt->devInfo->numHeads));
            Flush (StdOut);
            
            flushTrack (adfPkt->diskReq);
            IOError = readTrack (id_VTrackBuf, ID_TBTRACKS, nextTrack, 
                                 adfPkt->diskReq);
            if (IOError)
            {
              FPutC (StdOut, '\n');
              FPrintf (StdErr, "%s: Error reading from %s: - ", ProgName,
                                                            adfPkt->devInfo->dosName);
              reportIOError (IOError);
              inflateEnd (&infl_stream);
              cleanExit (RETURN_ERROR, NULL);
            }
      
            /* Check that the two buffers are identical */
            if (memcmp (id_TrackBuf, id_VTrackBuf, ID_TBSIZE) != 0)
            {
              /* Verification error */
              FPutC (StdOut, '\n');
              FPrintf (StdErr, "%s: Verification Error on %s:\n", ProgName,
                                                            adfPkt->devInfo->deviceUnit);
              inflateEnd (&infl_stream);
              cleanExit (RETURN_ERROR, NULL);
            }
          }
          
          infl_stream.next_out  = id_TrackBuf;
          infl_stream.avail_out = ID_TBSIZE;
          nextTrack++;
        }
        else
          /* We've reached the end */
          zerr = Z_STREAM_END;
      }
      else
        /* The buffer is not full, move along */
        break;
      
      if (zerr == Z_STREAM_END)
        /* Reached endTrack, or we reached the end of the */
        /* compressed data stream.                        */
        break;
    }
    if (zerr == Z_STREAM_END) break;
  }
  USize = infl_stream.total_out;
  
  FPrintf (StdOut, "\rInflated: %ld ==> %ld         ", infl_stream.total_in,
                                                       USize);
  Flush (StdOut);
  
  /* End the deflate process */
  zerr = inflateEnd (&infl_stream);
  if (zerr != Z_OK)
  {
    FPutC (StdOut, '\n');
    FPrintf (StdErr, "%s: Inflate End Error - ", ProgName);
    reportZLibError (zerr);
    if (infl_stream.msg) FPrintf (StdErr, "\t(%s)\n", infl_stream.msg);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Check the CRC and USize */
  if ( readTail (adfPkt->ADFile, &origCRC, &origUSize, fileType) == FALSE)
  {
    FPutC (StdOut, '\n');
    FPrintf (StdErr, "%s: Couldn't read CRC or Size from file.\n", 
                     ProgName);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  if (fileType != FT_ZLIB)
  {
    if (CRC != origCRC)
    {
      FPutC (StdOut, '\n');
      FPuts (StdErr, "Error: CRC Mismatch!\n");
      cleanExit (RETURN_ERROR, NULL);
    }
    
    if (USize != origUSize)
    {
      FPutC (StdOut, '\n');
      FPuts (StdErr, "Error: Size mismatch!\n");
      cleanExit (RETURN_ERROR, NULL);
    }
  }
  
  /* Free and clear buffers */
  myFreeMem (id_TrackBuf);
  myFreeMem (id_FileBuf);
  if (id_VTrackBuf)
    myFreeMem (id_VTrackBuf);
}
