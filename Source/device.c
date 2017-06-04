/* device.c - Handle all device IO.
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

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <string.h>

#include "device.h"
#include "main.h"
#include "util.h"
#include "errors.h"


/* Global variable that holds the size of a single track */
/* I hate the fact tat this is global :(                 */
ULONG trackSize;

/* Priate Functions */
BYTE rwTrack (APTR  Buffer, UWORD NumTracks, UWORD TrackNum, 
              struct IOStdReq *DiskReq);

extern struct DosLibrary *DOSBase;


/*
** Get information about dosDev.
** Returns a DeviceInfo structure or NULL.
*/
struct DeviceInfo *
getDeviceInfo (STRPTR dosDev)
{
  static struct DeviceInfo  devInfo;
  struct DeviceInfo        *rDevInfo = NULL;
  struct DosList           *dosList;
  struct FileSysStartupMsg *suMsg;
  struct DosEnvec          *dosEnv;
  int l;
  
  /* Strip ':' from dosDev if present */
  dosDev = strdup (dosDev);
  l = strlen (dosDev) - 1;
  if (dosDev[l] == ':') dosDev[l] = 0;
  
  /* Make sure the list doesn't change while we're looking at it */
  dosList = LockDosList (LDF_DEVICES | LDF_READ);
  if (( dosList = FindDosEntry (dosList, dosDev, LDF_DEVICES) ))
  {
    if (( suMsg = (struct FileSysStartupMsg *) 
                  BADDR (dosList->dol_misc.dol_handler.dol_Startup) ))
    {
      dosEnv = (struct DosEnvec *) BADDR (suMsg->fssm_Environ);
      
      devInfo.dosName    = dosDev;
      devInfo.deviceName = strdup (b2cstr (suMsg->fssm_Device));
      devInfo.deviceUnit = suMsg->fssm_Unit;
      
      devInfo.trackSize  = (dosEnv->de_SizeBlock << 2) * dosEnv->de_BlocksPerTrack;
      devInfo.numHeads   = dosEnv->de_Surfaces;
      devInfo.lowTrack   = dosEnv->de_LowCyl * dosEnv->de_Surfaces;
      devInfo.highTrack  = ((dosEnv->de_HighCyl + 1) * dosEnv->de_Surfaces) - 1;
      
      /* Set global track size */
      trackSize = devInfo.trackSize;
      
      /* Set the return type */
      rDevInfo = &devInfo;          
    }
  }
  /* Allow others to use the DOS Lists */
  UnLockDosList (LDF_DEVICES | LDF_READ);

  return rDevInfo;
}


/*
** Open an exec device.  Allocate all structures nessesary.
** Return an IO Request structure or NULL if failure.
*/
struct IOStdReq *
openDev (STRPTR devName, ULONG devUnit)
{
  struct MsgPort    *diskPort;
  struct IOStdReq   *diskReq;
  BYTE   IOError;
    
  /* Attempt to create a Message Port */
  diskPort = CreateMsgPort();
  if (diskPort)
  {
    /* Attempt to create an IO Request */
    diskReq = (struct IOStdReq *)
              CreateIORequest (diskPort, sizeof (struct IOStdReq));
    if (diskReq)
    {
      /* Attempt to open device */
      IOError = OpenDevice (devName, devUnit, 
                           (struct IORequest *)diskReq, NULL);
      if (!IOError)
      {
        /* Success - ready to go */
        return diskReq;
      }
      else
      {
        /* Couldn't open device */
        FPrintf (StdErr, "%s: Couldn't open %s unit %d - ", ProgName,
                                                            devName,
                                                            devUnit);
        reportIOError (IOError);
      }
      
      DeleteIORequest (diskReq);        
    }
    else
    {
      /* Couldn't create an IO Request */
      FPrintf (StdErr,
               "%s: Couldn't create an IO Request for %s IO.\n",
               ProgName, devName);
    }
    
    DeleteMsgPort (diskPort);
  }
  else
  {
    /* Couln't create a Message Port */
    FPrintf (StdErr,
             "%s: Couldn't create a Message Port for %s IO.\n",
             ProgName, devName);
  }
  
  return NULL;
}


/*
** Close exec device associated with diskReq.
** It is safe to call this with diskReq == NULL.
*/
void
closeDev (struct IOStdReq *diskReq)
{
  struct MsgPort *diskPort;
  
  if (diskReq)
  {
    diskPort = diskReq->io_Message.mn_ReplyPort;
    
    /* Flush any unwritten buffers */
    flushTrack (diskReq);
    
    /* Turn off the drive motor */
    diskReq->io_Command = TD_MOTOR;
    diskReq->io_Length  = 0;
    DoIO ((struct IORequest *)diskReq);
    
    /* Close the device and release memory */
    CloseDevice ((struct IORequest *) diskReq);
    DeleteIORequest (diskReq);
    DeleteMsgPort (diskPort);
  }
}


/* 
** Read a number of complete tracks from the specified unit.
** Return DoIO()'s return value.
*/
BYTE 
readTrack (APTR   rBuffer,
           UBYTE  rNumTracks,
           UBYTE  rTrackNum,
           struct IOStdReq *rDiskReq)
{
  rDiskReq->io_Command = CMD_READ;
  return rwTrack (rBuffer, rNumTracks, rTrackNum, rDiskReq);
}


/*
** Write a number of complete tracks to the specified unit.
** Return DoIO()'s return value.
*/
BYTE 
writeTrack (APTR   wBuffer, 
            UBYTE  wNumTracks, 
            UBYTE  wTrackNum, 
            struct IOStdReq *wDiskReq)
{
  wDiskReq->io_Command = CMD_WRITE;
  return rwTrack (wBuffer, wNumTracks, wTrackNum, wDiskReq);
}


/*
** Format a number of complete tracks on the the specified device.
** This can only be called if device either supports or ignores
** the TD_FORMAT command.  If it implemets the value of TD_FORMAT
** (= CMD_NONSTD+2 = 11) as some other command, the results are undefined.
** Return DoIO()'s return value.  This may be IOERR_NOCMD, 
** which can be safely ignored.
*/
BYTE 
formatTrack (APTR   fBuffer, 
             UBYTE  fNumTracks, 
             UBYTE  fTrackNum, 
             struct IOStdReq *fDiskReq)
{
  fDiskReq->io_Command = TD_FORMAT;
  return rwTrack (fBuffer, fNumTracks, fTrackNum, fDiskReq);
}


/*
** Flush the track buffer on the specified unit.
** Return DoIO()'s return value.
*/
BYTE 
flushTrack (struct IOStdReq *DiskReq)
{
  DiskReq->io_Command = CMD_UPDATE;
  return DoIO ((struct IORequest *)DiskReq);
}


/*
** Perform track-sized IO on specified unit.
** Return DoIO()'s return value
*/
BYTE
rwTrack (APTR  Buffer,
         UWORD NumTracks,
         UWORD TrackNum,
         struct IOStdReq *DiskReq)
{
  DiskReq->io_Flags   = 0;
  DiskReq->io_Data    = Buffer;
  DiskReq->io_Length  = NumTracks * trackSize;
  DiskReq->io_Offset  = TrackNum  * trackSize;
  
  return DoIO ((struct IORequest *) DiskReq);
}
