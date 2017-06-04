/* args.c - Collect program arguments
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
#include <dos/rdargs.h>
#include <clib/dos_protos.h>

#include <string.h>

#include "args.h"

const char TA_Template[] = "\
DRIVE/A,FILE/A,S=START/N,E=END/N,W=WRITE/S,V=VERIFY/S,F=FORMAT/S,\
Z=ZLIB/S,G=GZIP/S,P=PKZIP/S,N=NAME/K,L=LEVEL/N,A=ADD/S";

struct TA_RDArgs {
  STRPTR  DRIVE;
  STRPTR  FILE;
  ULONG  *START;
  ULONG  *END;
  ULONG   WRITE;
  ULONG   VERIFY;
  ULONG   FORMAT;
  ULONG   ZLIB;
  ULONG   GZIP;
  ULONG   PKZIP;
  STRPTR  NAME;
  ULONG  *LEVEL;
  ULONG   ADD;
};


/*
** Collect the progam arguments.
*/
struct TA_Args *getArgs (void)
{
  static struct TA_Args taArgs;
  struct RDArgs *rdargs;
  struct TA_RDArgs taRDArgs = {NULL,  NULL,  NULL,  NULL, FALSE, FALSE, FALSE,
                               FALSE, FALSE, FALSE, NULL, NULL,  FALSE};
  
  
  /* Analyse arguments */
  rdargs = ReadArgs (TA_Template, (ULONG *)&taRDArgs, NULL);
  if (!rdargs) return NULL;
  
  /* Fill in the defauts */
  taArgs.Drive     = NULL;
  taArgs.File      = NULL;
  taArgs.Start     = 0;
  taArgs.End       = -1;
  taArgs.WriteDisk = FALSE;
  taArgs.Verify    = FALSE;
  taArgs.Format    = FALSE;
  taArgs.ZLib      = FALSE;
  taArgs.GZip      = FALSE;
  taArgs.PKZip     = FALSE;
  taArgs.PKZAdd    = FALSE;
  taArgs.OrigName  = NULL;
  taArgs.Level     = 6;
  
  /* Copy arguments into the 'real' structure if required          */
  /* We'll assume that strdup() always works -- dumb but easier :) */
  if (taRDArgs.DRIVE)  taArgs.Drive     = strdup (taRDArgs.DRIVE);
  if (taRDArgs.FILE)   taArgs.File      = strdup (taRDArgs.FILE);
  if (taRDArgs.START)  taArgs.Start     = *(taRDArgs.START);
  if (taRDArgs.END)    taArgs.End       = *(taRDArgs.END);
  if (taRDArgs.WRITE)  taArgs.WriteDisk = TRUE;
  if (taRDArgs.VERIFY) taArgs.Verify    = TRUE;
  if (taRDArgs.FORMAT) taArgs.Format    = TRUE;
  if (taRDArgs.ZLIB)   taArgs.ZLib      = TRUE;
  if (taRDArgs.GZIP)   taArgs.GZip      = TRUE;
  if (taRDArgs.PKZIP)  taArgs.PKZip     = TRUE;
  if (taRDArgs.ADD)    taArgs.PKZAdd    = TRUE;
  if (taRDArgs.NAME)   taArgs.OrigName  = strdup (taRDArgs.NAME);
  if (taRDArgs.LEVEL)  taArgs.Level     = *(taRDArgs.LEVEL);
  
  /* Get rid of the temporary stuff */
  FreeArgs (rdargs);
  
  return &taArgs;
}
