/* write-disk.c - Write a file straight onto a disk
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
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <string.h>

#include "write_disk.h"
#include "main.h"
#include "device.h"
#include "mem_chunks.h"
#include "util.h"
#include "errors.h"


#define WD_TBTRACKS 1                           /* Tracks in wd_TrackBuf */
#define WD_TBSIZE   (WD_TBTRACKS * trackSize)   /* Bytes in wd_TrackBuf  */

UBYTE *wd_TrackBuf;    /* Our input buffer - reads from file.    */
UBYTE *wd_VTrackBuf;   /* Buffer for verifying data if required. */

/*
** Writes a file into a disk.
** Expects all fields of adfPkt to be set.
*/
void writeDisk (struct ADF_Packet *adfPkt)
{
  BYTE IOError;
  LONG DOSError, RWSize;
  int i;
  
  
  /* Output info header */
  FPrintf (StdOut, "Writing from %s to %s unit %ld (%s:).\n", 
                   adfPkt->ADFileName,
                   adfPkt->devInfo->deviceName,
                   adfPkt->devInfo->deviceUnit,
                   adfPkt->devInfo->dosName);
  
  FPrintf (StdOut, "Starting at track %ld, Ending at track %ld.\n",
                   (adfPkt->startTrack/adfPkt->devInfo->numHeads),
                   (adfPkt->endTrack/adfPkt->devInfo->numHeads));
  
  /* Allocate track buffers */
  wd_TrackBuf = (UBYTE *) myAllocMem (WD_TBSIZE, MEMF_CLEAR);
  if (!wd_TrackBuf)
  {
    /* Out of memory! */
    FPrintf (StdErr, "%s: Out of memory.\n", ProgName);
    cleanExit (RETURN_FAIL, ERROR_NO_FREE_STORE);
  }
  
  if (adfPkt->verify)
  {
    wd_VTrackBuf = (UBYTE *) myAllocMem (WD_TBSIZE, MEMF_CLEAR);
    if (!wd_VTrackBuf)
    {
      /* Out of memory! */
      FPrintf (StdErr, "%s: Out of memory.\n", ProgName);
      cleanExit (RETURN_FAIL, ERROR_NO_FREE_STORE);
    }
  }
  
  /* Start writing */
  for (i = adfPkt->startTrack; i <= adfPkt->endTrack; i++)
  {
    /* Check for Control-C break */
    if (CTRL_C)
    {
      FPutC (StdOut, '\n');
      FPrintf (StdErr, "%s - %s\n", breakText, ProgName);
      cleanExit (RETURN_WARN, NULL);
    }
    
    /* Fill the buffer from file */
    RWSize = Read (adfPkt->ADFile, wd_TrackBuf, WD_TBSIZE);
    if (RWSize != WD_TBSIZE)
    {
      DOSError = IoErr();
      
      FPutC (StdOut, '\n');
      FPrintf (StdErr, "%s: Error reading from %s - ", ProgName,
                                                       adfPkt->ADFileName);
      if (DOSError)
        reportDOSError (DOSError);
      else
        FPuts (StdErr, "File to short.\n");
      
      cleanExit (RETURN_ERROR, DOSError);
    }
    
    if (adfPkt->format)
    {
      /* Format the track */
      
      /* Update progress information */
      FPrintf (StdOut, "\rFormatting track %ld side %ld", 
                       (i/adfPkt->devInfo->numHeads),
                       (i%adfPkt->devInfo->numHeads));
      Flush (StdOut);
      
      /* Format the current track */
      IOError = formatTrack (wd_TrackBuf, WD_TBTRACKS, i, adfPkt->diskReq);
      if (IOError)
      {
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s: Error formatting %s: - ",ProgName,
                                                       adfPkt->devInfo->dosName);
        reportIOError (IOError);
        cleanExit (RETURN_ERROR, NULL);
      }
    }
    
    /* Update progress information */
    FPrintf (StdOut, "\rWriting    track %ld side %ld", 
                     (i/adfPkt->devInfo->numHeads),
                     (i%adfPkt->devInfo->numHeads));
    Flush (StdOut);
    
    /* Write the buffer to the disk */
    IOError = writeTrack (wd_TrackBuf, WD_TBTRACKS, i, adfPkt->diskReq);
    if (IOError)
    {
      FPutC (StdOut, '\n');
      FPrintf (StdErr, "%s: Error writing to %s: - ",ProgName,
                                                     adfPkt->devInfo->dosName);
      reportIOError (IOError);
      cleanExit (RETURN_ERROR, NULL);
    }
    
    
    if (adfPkt->verify)
    {
      /* Verify that the data was written correctly */
      FPrintf (StdOut, "\rVerifying  track %ld side %ld", 
                       (i/adfPkt->devInfo->numHeads),
                       (i%adfPkt->devInfo->numHeads));
      Flush (StdOut);
      
      flushTrack (adfPkt->diskReq);
      IOError = readTrack (wd_VTrackBuf, WD_TBTRACKS, i, adfPkt->diskReq);
      if (IOError)
      {
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s: Error reading from %s - ", ProgName,
                                                         adfPkt->devInfo->dosName);
        reportIOError (IOError);
        cleanExit (RETURN_ERROR, NULL);
      }
      
      /* Check that the two buffers are identical */
      if (memcmp (wd_TrackBuf, wd_VTrackBuf, WD_TBSIZE) != 0)
      {
        /* Verification error */
        FPutC (StdOut, '\n');
        FPrintf (StdErr, "%s: Verification Error on %s:\n", ProgName,
                                                            adfPkt->devInfo->dosName);
        cleanExit (RETURN_ERROR, NULL);
      }
    }
  }
  
  /* Free buffers */
  myFreeMem (wd_TrackBuf);
  if (wd_VTrackBuf) 
    myFreeMem (wd_VTrackBuf); 
}
