/* version.c - Usage and version information
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

/*-------------------------------*/
/* Version information and usage */
/*-------------------------------*/

/* COMPILE_LITE takes precedence over COMPILE_RT */
#ifdef COMPILE_LITE
#  ifdef COMPILE_RT
#    undef COMPILE_RT
#  endif /* COMPILE_RT */
#endif /* COMPILE_LITE */

#include <exec/types.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

#include "version.h"
#include "main.h"
#include "args.h"

#ifndef COMPILE_LITE
#include "zlib.h"
#endif /* COMPILE_LITE */


/* Any procedure that needs to access these macros should be kept here.  */
/* This prevents any module from needing to be recompiled just to change */
/* the program's name, version or date.                                  */

#ifdef COMPILE_LITE
#  define PROG_NAME  "TransADF-Lite"
#else /* ifndef COMPILE_LITE */
#  ifdef COMPILE_RT
#    define PROG_NAME  "TransADF-RT"
#  else /* ifndef COMPILE_RT ( && ifndef COMPILE_LITE) */
#    define PROG_NAME  "TransADF"
#  endif /* COMPILE_RT */
#endif /* COMPILE_LITE */

#define PROG_VER   "4.0.46"
#define PROG_DATE  "5th October 1998"

const char VerString[] = "$VER: "PROG_NAME" "PROG_VER" ("PROG_DATE")\r\n\0";
#ifndef COMPILE_LITE
const char ZipComment[] = "Created with "PROG_NAME" v"PROG_VER".\n\
"PROG_NAME" Copyright (c) Karl J. Ots, "PROG_DATE".";
#endif /* COMPILE_LITE */


/*
** Output program usage to StdErr and call cleanExit()
*/
void outputUsage (void)
{
  FPuts (StdErr,
        "\033[1m"PROG_NAME" v"PROG_VER" © "PROG_DATE" Karl J. Ots\033[0m\n");
#ifndef COMPILE_LITE
  FPrintf (StdErr, " Incorperating ZLib %s", zlibVersion());
#  ifdef COMPILE_RT
  FPrintf (StdErr, ", using z.library %ld.%ld", ZBase->lib_Version,
                                                ZBase->lib_Revision);
#  endif /* COMPILE_RT */
  FPuts (StdErr, "\n");
#endif /* COMPILE_LITE */
  
  FPrintf (StdErr, "\nUsage:\t%s\n\n", TA_Template);
  
  FPuts (StdErr, "\tDRIVE\tDisk drive to operate on (Required).\n");
  FPuts (StdErr, "\tFILE\tName of Amiga Disk File or Archive (Required).\n");
  FPuts (StdErr, "\tSTART\tTrack to begin operation at (Default 0).\n");
  FPuts (StdErr, "\tEND\tTrack to stop operation at (Default 79).\n");
  FPuts (StdErr, "\tWRITE\tWrite from FILE to DRIVE.\n");
  FPuts (StdErr, "\tVERIFY\tVerify that data is written to the disk correctly.\n");
  FPuts (StdErr, "\tFORMAT\tFormat each track before writing data (READ DOCS!).\n");
#ifdef COMPILE_LITE
  FPuts (StdErr, "\tZLIB, GZIP, PKZIP, NAME, LEVEL, ADD\n");
  FPuts (StdErr, "\t\tDisabled in Lite version.\n");
#else /* ifndef COMPILE_LITE */
  FPuts (StdErr, "\tZLIB\tCompress disk into a ZLib file.\n");
  FPuts (StdErr, "\tGZIP\tCompress disk into a GZip file.\n");
  FPuts (StdErr, "\tPKZIP\tCompress disk into a PKZip file.\n");
  FPuts (StdErr, "\tNAME\tName to store in file or file to dearchive.\n");
  FPuts (StdErr, "\tLEVEL\tCompression level, 1 - 9 (Default 6).\n");
  FPuts (StdErr, "\tADD\tAdd disk to PKZip archive.\n");
#endif /* COMPILE_LITE */
  FPuts (StdErr, "\n");
  cleanExit (RETURN_FAIL, ERROR_REQUIRED_ARG_MISSING);
}
