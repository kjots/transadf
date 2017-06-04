/* main.h - Header file for main.c
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

#ifndef TRANSADF_MAIN_H
#define TRANSADF_MAIN_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif /* EXEC_IO_H */

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif /* DOS_DOS_H */

#ifdef COMPILE_RT
#  ifndef EXEC_LIBRARIES_H
#  include <exec/libraries.h>
#  endif /* EXEC_LIBRARIES_H */
extern struct Library *ZBase;
#  include "z_pragmas.h"
#endif /* COMPILE_RT */


#ifndef TRANSADF_DEVICE_H
#include "device.h"
#endif /* TRANDADF_DEVICE_H */

/*---------------------------------*/
/* Global variables and structures */
/*---------------------------------*/

/* Constant strings */
extern const char breakText[];

/* Standard IO Handles */
extern BPTR StdIn;
extern BPTR StdOut;
extern BPTR StdErr;

/* Program name */
extern STRPTR ProgName;

/* Device information */
extern struct DeviceInfo *devInfo;

/* This will be passed to the read/write routines */
struct ADF_Packet {
  /* Device information */
  struct DeviceInfo *devInfo;

  /* device IO */
  struct IOStdReq *diskReq;
  
  /* Amiga Disk File */
  BPTR   ADFile;
  STRPTR ADFileName;

  /* Track info */
  ULONG  startTrack;
  ULONG  endTrack;
  
  /* Verification & Formatting */
  BOOL   verify;
  BOOL   format;
};


/*---------------------*/
/* Function prototypes */
/*---------------------*/

void cleanExit (ULONG rc, LONG rc2);


#endif /* TRANSADF_MAIN_H */
