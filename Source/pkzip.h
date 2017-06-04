/* pkzip.h - Header file for pkzip.c
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

#ifndef TRANSADF_PKZIP_H
#define TRANSADF_PKZIP_H

/*---------------------*/
/* PKZip file routines */
/*---------------------*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif /* DOS_DOS_H */


/*---------------------*/
/* Function prototypes */
/*---------------------*/

BOOL   writePKZHead     (BPTR outFile, STRPTR origName);
BOOL   finishPKZFile    (BPTR outFile, ULONG CRC, ULONG CSize, ULONG USize);
BOOL   writePKZHeadAdd  (BPTR outFile, STRPTR origName);
BOOL   finishPKZFileAdd (BPTR outFile, ULONG CRC, ULONG CSize, ULONG USize);
BOOL   skipPKZHead      (BPTR inFile, STRPTR origName);
BOOL   readPKZTail      (BPTR inFile, ULONG *CRC, ULONG *USize);


#endif /* TRANSADF_PKZIP_H */
