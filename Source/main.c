/* main.c - Entry point plus startup and shutdown routines
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

/*----------------*/
/* Main procedure */
/*----------------*/

/* COMPILE_LITE takes precedence over COMPILE_RT */
#ifdef COMPILE_LITE
#  ifdef COMPILE_RT
#    undef COMPILE_RT
#  endif /* COMPILE_RT */
#endif /* COMPILE_LITE */

#include <exec/types.h>
#include <exec/io.h>
#ifdef COMPILE_RT
#  include <exec/libraries.h>
#endif /* COMPILE_RT */
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "args.h"
#include "mem_chunks.h"
#include "device.h"
#include "read_disk.h"
#include "write_disk.h"
#ifndef COMPILE_LITE
#  include "defl_disk.h"
#  include "infl_disk.h"
#endif /* COMPILE_LITE */
#include "util.h"
#include "errors.h"
#include "version.h"


/*---------------------------------*/
/* Global constants and structures */
/*---------------------------------*/

const char breakText[]   = "***Break";

/*----------------------------*/
/* Standatd Input/Ouput/Error */
/*----------------------------*/

BPTR StdIn, StdOut, StdErr;
STRPTR ProgName;

/*
** Set handles for DOS Library StdIO functions, as well as
** find our command name.
*/
void setStdIO (void)
{
  struct Process *Proc = (struct Process *)FindTask (NULL);
  
  StdIn  = Proc->pr_CIS;  /* Quicker than Input() or Output() */
  StdOut = Proc->pr_COS;
  StdErr = Proc->pr_CES;
  if (!StdErr)
  {
    /* Use the file handle from the file number of stdio FILE stderr */
    StdErr = (BPTR) fdtofh (fileno (stderr));
    if (!StdErr) StdErr = StdOut;
  }
}


/*-------------------------------------------*/
/* Global variables and associated functions */
/*-------------------------------------------*/

#ifdef COMPILE_RT
/* z.library base pointer */
struct Library *ZBase;
#endif /* COMPILE_RT */

/* Device information. */
struct DeviceInfo *devInfo;

/* files */
BPTR   ADFile;
STRPTR DOSDev;

/* io */
struct IOExtTD *diskReq;


/*
** Initialise all globals to their default values.
*/
void initGlobals (void)
{
#ifdef COMPILE_RT
  ZBase = NULL;
#endif /* COMPILE_RT */
  
  setStdIO();
  
  /* Iniitalise our memory list */
  initMemChunkList();
  
  devInfo = NULL;
  ADFile  = NULL;
  DOSDev  = NULL;
  diskReq = NULL;
}


/*
** Exit cleanly.
*/
void cleanExit (ULONG rc, LONG rc2)
{
  /* Free all allocated memory */
  deleteMemChunkList();
  
  closeDev (diskReq);
  
  if (DOSDev) Inhibit (DOSDev,FALSE);
  if (ADFile) Close (ADFile);
  
  /* Turn the cursor on */
  FPuts (StdOut, "\033[1 p"); 
  Flush (StdOut);
  
#ifdef COMPILE_RT
  if (ZBase) CloseLibrary (ZBase);
#endif /* COMPILE_RT */
  
  SetIoErr (rc2);
  exit (rc);
}


/*---------------*/
/* Main function */
/*---------------*/
int main (int argc, char *argv[])
{
  struct ADF_Packet adfPkt;
  struct TA_Args *taArgs;
  
  STRPTR ADFileName;
  ULONG  startTrack, endTrack;
  ULONG  fileType = 0;
  
  BOOL   WriteDisk;
  LONG   ADFOpenMode = MODE_NEWFILE;  /* Default is to write to file */
  LONG   DOSError;

#ifndef COMPILE_LITE
  STRPTR origName;
  ULONG  cLevel;
#endif /* COMPILE_LITE */
  

  /* Initialise any global data */
  initGlobals();
  ProgName = FilePart (argv[0]);

  /* Collect arguments */
  taArgs = getArgs ();
  if (!taArgs) outputUsage ();

#ifdef COMPILE_RT
  /* Open z.library */
  ZBase = OpenLibrary ("z.library",0);
  if (!ZBase)
  {
    FPrintf (StdErr, "%s: Couldn't open z.library.\n", ProgName);
    cleanExit (RETURN_FAIL, NULL);
  }
#endif /* COMPILE_RT */
  
  /* Collect information about the given DOS device */
  devInfo = getDeviceInfo (taArgs->Drive);
  if (!devInfo)
  {
    FPrintf (StdErr, "%s: Error - Don't know how to talk to %s.\n",
                     ProgName, taArgs->Drive);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Analyse arguments */
  ADFileName = taArgs->File;
  startTrack = taArgs->Start;
  endTrack   = taArgs->End;
  WriteDisk  = taArgs->WriteDisk;
  
  /* Make the tracks correct */
  if (startTrack == 0) startTrack = devInfo->lowTrack;
  else                 startTrack = startTrack * devInfo->numHeads;
  if (endTrack == -1)  endTrack = devInfo->highTrack;
  else                 endTrack = ((endTrack + 1) * devInfo->numHeads) - 1;
  
#ifndef COMPILE_LITE
  if      (taArgs->ZLib)  fileType = FT_ZLIB;  
  else if (taArgs->GZip)  fileType = FT_GZIP;  
  else if (taArgs->PKZip) 
  {
    if (taArgs->PKZAdd)   fileType = FT_PKZIP_ADD;
    else                  fileType = FT_PKZIP;
  }
  origName = taArgs->OrigName;  
  cLevel = taArgs->Level;
#endif /* COMPILE_LITE */
  
  if ((startTrack < 0) || (startTrack > endTrack))
  {
    FPrintf (StdErr, "%s: Invalid Start/End track.\n", ProgName);
    cleanExit (RETURN_FAIL, NULL);
  }
  
  /* Inhibit DOS access to the drive we'll be operating on */  
  DOSDev = taArgs->Drive;
  if ( !Inhibit (DOSDev, DOSTRUE) )
  {
    DOSError = IoErr();
    FPrintf (StdErr, "%s: Error - Couldn't inhibit DOS access to %s.\n",
                     ProgName, DOSDev);
    DOSDev = NULL;
    cleanExit (RETURN_FAIL, DOSError);
  }
  
  /* Open the Amiga Disk File */
  if (WriteDisk) ADFOpenMode = MODE_OLDFILE;  /* Reading from file */
  else if (fileType == FT_PKZIP_ADD) ADFOpenMode = MODE_READWRITE;
  ADFile = Open (ADFileName, ADFOpenMode);
  if (!ADFile)
  {
    DOSError = IoErr();
    FPrintf (StdErr, "%s: Couldn't open %s for %s - ", ProgName, ADFileName,
                     (WriteDisk ? "input" : "output"));
    reportDOSError (DOSError);
    cleanExit (RETURN_FAIL, DOSError);
  }
  
  /* Attemp to open device */
  diskReq = openDev (devInfo->deviceName, devInfo->deviceUnit);
  if (!diskReq)
    /* A message has already been printed */
    cleanExit (RETURN_FAIL, NULL);
  
  /* The device in now open and ready for use */
  
  /* Turn off the cursor */
  FPuts (StdOut, "\033[0 p"); Flush (StdOut);
  
  /* Load packet */
  adfPkt.devInfo    = devInfo;
  adfPkt.diskReq    = diskReq;
  adfPkt.ADFile     = ADFile;
  adfPkt.ADFileName = ADFileName;
  adfPkt.startTrack = startTrack;
  adfPkt.endTrack   = endTrack;
  adfPkt.verify     = taArgs->Verify;
  adfPkt.format     = taArgs->Format;
    
  if (WriteDisk)
  {
    /* We're reading from the file and writing to the disk */
#ifndef COMPILE_LITE
    fileType = getFileType (ADFile);
    
    if ((fileType == FT_ZLIB) ||
        (fileType == FT_GZIP) ||
        (fileType == FT_PKZIP))
      inflDisk (&adfPkt, origName, fileType);
    else
#endif /* COMPILE_LITE */
      writeDisk (&adfPkt);
  }
  else
  {
    /* We're reading from the disk and writing to the file */
#ifndef COMPILE_LITE
    if (fileType)
      deflDisk (&adfPkt, cLevel, origName, fileType);
    else
#endif /* COMPILE_LITE */
      readDisk (&adfPkt);
  }
  
  
  FPuts (StdOut, "\nDone.\n");
  
  /* Let's get outa here! */
  cleanExit (RETURN_OK, NULL);
  
  /* Keep the compiler happy */
  return 0;
}
