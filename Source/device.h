/* device.h - Header file for device.c
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

#ifndef TRANSADF_DEVICE_H
#define TRANSADF_DEVICE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif /* EXEC_IO_H */


/* Structure to carry information about a DOS Device */
struct DeviceInfo {
  STRPTR  dosName;          /* DOS name eg DF0:, RAD:, etc. without ':'.  */
  STRPTR  deviceName;       /* Controlling device eg trackdisk.device.    */
  ULONG   deviceUnit;       /* Device unit number.                        */
  
  ULONG   trackSize;        /* Size of a single track in bytes.           */
  ULONG   numHeads;         /* Number of heads (surfaces) on device.      */
  ULONG   lowTrack;         /* Lowest track number, (almost) always zero. */
  ULONG   highTrack;        /* Hightest track number, 159 for DD floppys. */
};

extern ULONG trackSize;

/*---------------------*/
/* Function Prototypes */
/*---------------------*/

struct DeviceInfo *getDeviceInfo (STRPTR dosDev);
struct IOStdReq   *openDev (STRPTR devName, ULONG devUnit);
void   closeDev  (struct IOStdReq *diskReq);
BYTE readTrack   (APTR rBuffer, UBYTE rNumTracks, UBYTE rTrackNum,
                  struct IOStdREq *rDiskReq);
BYTE writeTrack  (APTR wBuffer, UBYTE wNumTracks, UBYTE wTrackNum, 
                  struct IOStdReq *wDiskReq);
BYTE formatTrack (APTR fBuffer, UBYTE fNumTracks, UBYTE fTrackNum, 
                  struct IOStdReq *fDiskReq);
BYTE flushTrack  (struct IOStdReq *DiskReq);


#endif /* TRANSADF_DEVICE_H */
