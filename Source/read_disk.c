/* read_disk.c - Read a disk straight into a file
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

#include "read_disk.h"
#include "main.h"
#include "mem_chunks.h"
#include "device.h"
#include "util.h"
#include "errors.h"


#define RD_TBTRACKS 1                           /* Tracks in rd_TrackBuf */
#define RD_TBSIZE   (RD_TBTRACKS * trackSize)   /* Bytes in rd_TrackBuf  */

UBYTE *rd_TrackBuf;    /* Our input buffer - reads from disk. */


/*
** Read a disk into a file.
** Expects all fields of adfPkt to be set.
*/
void readDisk (struct ADF_Packet *adfPkt)
{
  BYTE IOError;
  LONG DOSError, RWSize;
  int i;
  
  
  /* Output info header */
  FPrintf (StdOut, "Reading from %s unit %ld (%s:) to %s.\n",
                   adfPkt->devInfo->deviceName,
                   adfPkt->devInfo->deviceUnit,
                   adfPkt->devInfo->dosName,
                   adfPkt->ADFileName);
  
  FPrintf (StdOut, "Starting at track %ld, Ending at track %ld.\n",
                   (adfPkt->startTrack/adfPkt->devInfo->numHeads),
                   (adfPkt->endTrack/adfPkt->devInfo->numHeads));
  
  /* Allocate track buffer */
  rd_TrackBuf = (UBYTE *) myAllocMem (RD_TBSIZE, MEMF_CLEAR);
  if (!rd_TrackBuf)
  {
    /* Out of memory! */
    FPrintf (StdErr, "%s: Out of memory.\n", ProgName);
    cleanExit (RETURN_FAIL, ERROR_NO_FREE_STORE);
  }
  
  /* Start reading */
  for (i = adfPkt->startTrack; i <= adfPkt->endTrack; i++)
  {
    /* Check for Control-C break */
    if (CTRL_C)
    {
      FPutC (StdOut, '\n');
      FPrintf (StdErr, "%s - %s\n", breakText, ProgName);
      cleanExit (RETURN_WARN, NULL);
    }
    
    /* Update progress information */
    FPrintf (StdOut, "\rReading track %ld side %ld",
                     (i/adfPkt->devInfo->numHeads),
                     (i%adfPkt->devInfo->numHeads));
    Flush (StdOut);
    
    /* Fill the buffer from disk */
    IOError = readTrack (rd_TrackBuf, RD_TBTRACKS, i, adfPkt->diskReq);
    if (IOError)
    {
      FPutC (StdOut, '\n');
      FPrintf (StdErr, "%s: Error reading from %s: - ", ProgName,
                                                        adfPkt->devInfo->dosName);
      reportIOError (IOError);
      cleanExit (RETURN_ERROR, NULL);
    }
    
    /* Write the buffer to the output file */
    RWSize = Write (adfPkt->ADFile, rd_TrackBuf, RD_TBSIZE);
    if (RWSize != RD_TBSIZE)
    {
      DOSError = IoErr();
      
      FPutC (StdOut, '\n');
      FPrintf (StdErr, "%s: Error writing to %s - ",ProgName,
                                                    adfPkt->ADFileName);
      reportDOSError (DOSError);
      cleanExit (RETURN_ERROR, DOSError);
    }
  }
  
  /* Free buffer */
  myFreeMem (rd_TrackBuf);
}
