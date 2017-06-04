/* defl_disk.h - Header file for defl_disk.c
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

#ifndef TRANDADF_DEFL_DISK_H
#define TRANDADF_DEFL_DISK_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef TRANSADF_MAIN_H
#include "main.h"
#endif /* TRANSADF_MAIN_H */


/*---------------------*/
/* Function prototypes */
/*---------------------*/

void deflDisk (struct ADF_Packet *adfPkt, ULONG deflLevel, STRPTR origName,
               ULONG fileType);


#endif /* TRANDADF_DEFL_DISK_H */
