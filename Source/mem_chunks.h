/* mem_chunks.h - Header file for mem_chunks.c
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

#ifndef TRANSADF_MEM_CHUNKS_H
#define TRANSADF_MEM_CHUNKS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */


/*---------------------*/
/* Function Prototypes */
/*---------------------*/

void  initMemChunkList (void);
void  deleteMemChunkList (void);
void *myAllocMem (ULONG byteSize, ULONG attributes);
void  myFreeMem (void *memoryBlock);

#endif /* TRANSADF_MEM_CHUNKS_H */
 