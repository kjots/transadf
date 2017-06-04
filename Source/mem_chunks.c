/* mem_chunks.c - Simple routines that keep track of memory allocation.
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
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>

struct List MemChunkList;

struct MemChunk {
  struct Node mc_Node;
  
  ULONG  mc_ByteSize;
  void  *mc_Chunk;
};


/*
** Initalise the Memory Chunk List ready for use
*/
void
initMemChunkList (void)
{
  NewList (&MemChunkList);
}  


/*
** Clear the Memory Chunk List of all entries
*/
void
deleteMemChunkList (void)
{
  struct MemChunk *chunk;
  
  while (( chunk = (struct MemChunk *) RemTail (&MemChunkList) ))
    FreeMem ((void *)chunk, (chunk->mc_ByteSize + sizeof (struct MemChunk)));
}


/*
** Allocate a block of memory and add it to the Memory Chunk List
*/
void *
myAllocMem (ULONG byteSize, ULONG attributes)
{
  struct MemChunk *chunk;
  UBYTE *mem;
  
  mem = (UBYTE *)AllocMem ((byteSize + sizeof (struct MemChunk)), attributes);
  if (mem)
  {
    chunk = (struct MemChunk *)mem;
    mem += sizeof (struct MemChunk);
    
    chunk->mc_ByteSize = byteSize;
    chunk->mc_Chunk = (void *)mem;
    
    AddTail (&MemChunkList, (struct Node *)chunk);
  }
  
  return (void *)mem;
}


/*
** Remove a block from the Memory Chunk List and free it
*/
void
myFreeMem (void *memoryBlock)
{
  struct MemChunk *chunk;
  
  chunk = (struct MemChunk *)
          (((UBYTE *)memoryBlock) - sizeof (struct MemChunk));
  
  /* Make sure this memory pointer is part of the chunk list */
  if (memoryBlock == chunk->mc_Chunk)
  {
    Remove ((struct Node *)chunk);
    FreeMem ((void *)chunk, (chunk->mc_ByteSize + sizeof (struct MemChunk)));
  }
}
