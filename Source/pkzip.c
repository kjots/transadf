/* pkzip.c - Handle all PKZip-file specific tasks
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

/*---------------------*/
/* PKZip file routines */
/*---------------------*/

#include <exec/types.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pkzip.h"
#include "main.h"
#include "util.h"
#include "errors.h"

/* Private functions */
LONG GotoFirstField (BPTR file, ULONG FieldID);


/*----------------------------------*/
/* Constants, structures and macros */
/*----------------------------------*/

extern const char ZipComment[];  /* Defined in version.c */

/* PKZip Local Header structure */
struct PKZHead {
  ULONG  MagicNum;          /* Magic Number, = 0x504B0304 ('P','K',3,4).  */
  UBYTE  MinVer;            /* Min. required UnZip version (20 for defl). */
  UBYTE  MinOS;             /* Min. requires OS, MS-DOS = 0.              */
  UWORD  Flags;             /* Flags.                                     */
  UWORD  CMethod;           /* Compression method, deflate = 8.           */
  ULONG  Date;              /* Last moddification date.                   */
  ULONG  CRC;               /* Uncompressed Data CRC.                     */
  ULONG  CSize;             /* Compressed data length.                    */
  ULONG  USize;             /* Uncompressed data length.                  */ 
  UWORD  FNameLen;          /* File name length.                          */
  UWORD  EFieldLen;         /* Extra field length.                        */ 
}; /* sizeof = 30 */


/* PKZip Central Record structure */
struct PKZCenRec {
  ULONG  MagicNum;          /* Magic Number, = 0x504B0102 ('P','K',1,2).  */
  UBYTE  ZipVer;            /* Zip version number x10 (ie 20 = v2.0).     */
  UBYTE  HostOS;            /* Host OS, Amiga = 1.                        */
  UBYTE  MinVer;            /* Min. required UnZip version (20 for defl). */
  UBYTE  MinOS;             /* Min. requires OS, MS-DOS = 0.              */
  UWORD  Flags;             /* Flags.                                     */
  UWORD  CMethod;           /* Compression method, deflate = 8.           */
  ULONG  Date;              /* Last moddification date.                   */
  ULONG  CRC;               /* Uncompressed Data CRC.                     */
  ULONG  CSize;             /* Compressed data length.                    */
  ULONG  USize;             /* Uncompressed data length.                  */
  UWORD  FNameLen;          /* File name length.                          */
  UWORD  EFieldLen;         /* Extra field length.                        */
  UWORD  FCommentLen;       /* File comment length.                       */
  UWORD  DiskNum;           /* Disk number in multi-part zip.             */
  UWORD  FileType;          /* File Type, set bit 0 for text.             */
  UWORD  MSDAttrib;         /* MS-DOS File Attributes.                    */
  UWORD  Attrib;            /* File Attributes, Amiga rw-d = 0x0D04 (LE). */
  ULONG  LHeadOff;          /* Offset of local header.                    */
}; /* sizeof = 46 */


struct PKZEndCRec {
  ULONG  MagicNum;          /* Magic Number, = 0x504B0506 ('P','K',5,6).  */ 
  UWORD  DiskNum;           /* Number of this disk in multi-disk zip.     */
  UWORD  CentRecDisk;       /* Disk number with start of Central Record.  */
  UWORD  EntriesOnDisk;     /* Number of entries on this disk.            */
  UWORD  CenRecEntries;     /* Total number of Central Record Entries.    */
  ULONG  CenRecSize;        /* Total Size of Central Record.              */
  ULONG  CenRecOffset;      /* Offset of start of Central Record.         */
  UWORD  CommentLen;        /* Length of Zip Comment.                     */
}; /* sizeof = 22 */


#define LHD_MAGNUM   0x504B0304  /* Local Header Magic Number          */
#define CRC_MAGNUM   0x504B0102  /* Central Record Magic Number        */
#define ECR_MAGNUM   0x504B0506  /* End of Central Record Magic Number */


/* Used to store the date/time that compression started */
ULONG  startDate;

/* Used to store the location of the `current' PKZip header */
LONG   pkzHeadOffset;
STRPTR pkzOrigName;

/* Used to save the central record of archive when adding */
ULONG Adding;  /* Set to 0x12345678 if adding, cleared afterwards */
ULONG pkzOldCenRecSize;
APTR  pkzOldCenRec;

/* End of central record - one per zip archive */
struct PKZEndCRec pkzECRec;


/*
** Output a PKZip Header to the specified file.
** Return TRUE if no errors. else FALSE.
*/
BOOL writePKZHead (BPTR outFile, STRPTR origName)
{
  UBYTE Empty[30];
  UWORD nlen;
  
  /* Simply write an empty sizeof (struct PKZHead) + strlen (origName). */
  /* We'll fill in the details later.                                   */
  
  /* Note the date and time */
  startDate = dosDate();
  
  /* Make sure we have a name */
  if (!origName) origName = "disk.adf";
  
  /* Save the current position and name */
  pkzHeadOffset = Seek (outFile, 0, OFFSET_CURRENT);
  pkzOrigName = strdup (origName);
  
  /* Write the `header' and the name to the file */
  if ( Write (outFile, Empty, 30) != 30) return FALSE;
  
  /* Get the filename length and write the filename */
  nlen = strlen (origName);  
  if ( Write (outFile, origName, nlen) != nlen) return FALSE;
  
  /* PKZip file is now ready to recieve the compressed data */
  return TRUE;
}


/*
** Finish writing a PKZip file.
** Return TRUE if no errors. else FALSE.
*/
BOOL finishPKZFile (BPTR outFile, ULONG CRC, ULONG CSize, ULONG USize)
{
  LONG  CRecPos;
  ULONG Date;
  UWORD FNameLen, ZCommentLen;
  /* Local header - one per zipped file */
  struct PKZHead pkzHead     = {LHD_MAGNUM,  /* PKZip Magic Number         */
                                20,          /* Min. UnZip version (2.0)   */
                                0,           /* Min. OS (MS-DOS)           */
                                0,           /* Flags                      */
                                0x0800,      /* Compression Method (LE)    */
                                0,           /* Date (to be filled in)     */
                                0,           /* CRC (to be filled in)      */
                                0,           /* CSize (to be filled in)    */
                                0,           /* USize (to be filled in)    */
                                0,           /* Filename length (tbfi)     */
                                0};          /* Extra field length         */
  /* Central record entry - one per zipped file */
  struct PKZCenRec pkzCenRec = {CRC_MAGNUM,  /* Central Rec Magic Number   */
                                20,          /* Zip version (2.0)          */
                                1,           /* Host OS (Amiga)            */
                                20,          /* Min. UnZip version (2.0)   */
                                0,           /* Min. Host OS (MS-DOS)      */
                                0,           /* Flags                      */
                                0x0800,      /* Compression Mode (LE)      */
                                0,           /* Date (to be filled in)     */
                                0,           /* CRC (to be filled in)      */
                                0,           /* CSize (to be filled in)    */
                                0,           /* USize (to be filled in)    */
                                0,           /* File Name Length (tbfi)    */
                                0,           /* Extra field length         */
                                0,           /* File comment length        */
                                0,           /* Disk Number                */
                                0,           /* File Type (binary)         */
                                0,           /* MS-DOS file attributes     */
                                0x0D04,      /* File Attributes (rw-d)     */
                                0};          /* Local header offset (tbfi) */
  
  
  /* Seek to the local header and save the `current' position */
  CRecPos = Seek (outFile, pkzHeadOffset, OFFSET_BEGINNING);
  
  /* 'pre-process' CRC, CSize and USize */
  CRC   = LEL (CRC);
  CSize = LEL (CSize);
  USize = LEL (USize);
  Date  = LEL (startDate);
  FNameLen = strlen (pkzOrigName);
  ZCommentLen = strlen (ZipComment);
  
  /* Fill the structures */
  pkzHead.Date       = Date;
  pkzHead.CRC        = CRC;
  pkzHead.CSize      = CSize;
  pkzHead.USize      = USize;
  pkzHead.FNameLen   = LES (FNameLen);
  
  pkzCenRec.Date     = Date;
  pkzCenRec.CRC      = CRC;
  pkzCenRec.CSize    = CSize;
  pkzCenRec.USize    = USize;
  pkzCenRec.FNameLen = LES (FNameLen);
  pkzCenRec.LHeadOff = LEL (pkzHeadOffset);
  
  /* Fill the End of Central Record structure */
  pkzECRec.MagicNum      = ECR_MAGNUM;
  pkzECRec.DiskNum       = 0;
  pkzECRec.CentRecDisk   = 0;
  pkzECRec.CenRecOffset  = LEL (CRecPos);
  pkzECRec.CommentLen    = LES (ZCommentLen);
  if (Adding == 0x12345678)
  {
    pkzECRec.EntriesOnDisk = LES ( LES (pkzECRec.EntriesOnDisk) + 1);
    pkzECRec.CenRecEntries = LES ( LES (pkzECRec.CenRecEntries) + 1);
    pkzECRec.CenRecSize    = LEL (pkzOldCenRecSize + 46 + FNameLen);  
  }
  else
  {
    pkzECRec.EntriesOnDisk = LES (1);
    pkzECRec.CenRecEntries = LES (1);
    pkzECRec.CenRecSize    = LEL (46 + FNameLen);
  }
  
  /* Write the Local Header and seek to the Central Record offset */
  if ( Write (outFile, &pkzHead, 30) != 30) return FALSE;
  /* Write (outFile, pkzOrigName, FNameLen); */ /* Already done */
  Seek (outFile, CRecPos, OFFSET_BEGINNING);
  
  if (Adding == 0x12345678)
  {
    /* Write the old central record before adding the new one */
    if ( Write (outFile, pkzOldCenRec, pkzOldCenRecSize) != pkzOldCenRecSize)
      return FALSE;
  }
  
  /* Write the Central Record and origName */
  if ( Write (outFile, &pkzCenRec, 46) != 46) return FALSE;
  if ( Write (outFile, pkzOrigName, FNameLen) != FNameLen) return FALSE;
  
  /* Write the End of Central Record and File Comment */
  if ( Write (outFile, &pkzECRec, 22) != 22) return FALSE;
  if ( Write (outFile, ZipComment, ZCommentLen) != ZCommentLen) return FALSE;
  
  /* Don't need the name anymore */
  free (pkzOrigName);
  
  /* Don't need the old record anymore */
  if (Adding == 0x123456780) free (pkzOldCenRec);
  
  /* No longer adding */
  Adding = 0;
  
  /* Done! */
  return TRUE;
}


/*
** Output a PKZip Header to the specified file, adding
** it to an existing archive.
** Return TRUE if no errors. else FALSE.
*/
BOOL writePKZHeadAdd (BPTR outFile, STRPTR origName)
{
  LONG DOSError;

  /* We're adding */
  Adding = 0x12345678;
  FPuts (StdOut, "Updating archive.\n");

  /* Get the End of Central Record */
  if ( GotoFirstField (outFile, ECR_MAGNUM) == -1)
  {
    DOSError = IoErr();
    
    if (DOSError)
    {
      FPrintf (StdErr, "%s: Error reading file - ", ProgName);
      reportDOSError(DOSError);
    }
    else
      FPrintf (StdErr, "%s: Can't add ADF, Not a Zip file.\n", ProgName);
    
    cleanExit (RETURN_ERROR, NULL);
  }
  if ( Read (outFile, &pkzECRec, 22) != 22) return FALSE;

  /* We'll be adding this header at the current position of the Cent Rec */
  pkzHeadOffset = LEL (pkzECRec.CenRecOffset);
  
  /* Save the current central record for re-writing later */
  pkzOldCenRecSize = LEL (pkzECRec.CenRecSize);
  pkzOldCenRec = calloc (pkzOldCenRecSize, 1);
  if (!pkzOldCenRec)
  {
    /* Memory error */
    return FALSE;
  }
  Seek (outFile, pkzHeadOffset, OFFSET_BEGINNING);
  if ( Read (outFile, pkzOldCenRec, pkzOldCenRecSize) != pkzOldCenRecSize)
    return FALSE;
  
  /* Write our dummy header and get outa here */
  Seek (outFile, pkzHeadOffset, OFFSET_BEGINNING);  
  return writePKZHead (outFile, origName);
}


/*
** Finish writing a PKZip file, adding new a new record.
** Return TRUE if no errors. else FALSE.
*/
BOOL finishPKZFileAdd (BPTR outFile, ULONG CRC, ULONG CSize, ULONG USize)
{
  return finishPKZFile (outFile, CRC, CSize, USize);
}


/*
** Skip the PKZip Header of a specified file.
** If origName is specified, search for that name within file.
** Return TRUE if no errors. else FALSE.
*/
BOOL skipPKZHead (BPTR inFile, STRPTR origName)
{
  UBYTE FName[128];  /* This should be large enough */
  struct PKZHead pkzHead;
  struct PKZCenRec pkzCenRec;
  ULONG origNameLen, pOrigNameLen;
  LONG DOSError;
  STRPTR pOrigName;
  UWORD nlen, elen, clen;
  int i;
  
  if (origName)
  {
    /* Get the first central record entry */
    if ( GotoFirstField (inFile, CRC_MAGNUM) == -1)
    {
      DOSError = IoErr();
      
      if (DOSError)
      {
        FPrintf (StdErr, "%s: Error reading file - ", ProgName);
        reportDOSError(DOSError);
      }
      else
        FPrintf (StdErr, "%s: Error - Not a Zip file.\n", ProgName);
      
      cleanExit (RETURN_ERROR, NULL);
    }
    
    /* Parse the pattern */
    origNameLen = strlen (origName);
    for (i=0; i < origNameLen; i++)
      origName[i] = toupper (origName[i]);
    
    pOrigNameLen = (2 * origNameLen) + 2;
    pOrigName = calloc (pOrigNameLen, 1);
    if (!pOrigName)
    {
      FPrintf (StdErr, "%s: Error - Not enough memory to match filename.\n",
                       ProgName);
      cleanExit (RETURN_FAIL, ERROR_NO_FREE_STORE);
    }
    
    if (ParsePatternNoCase (origName, pOrigName, pOrigNameLen) == -1)
    {
      FPrintf (StdErr, "%s: Error - Can't match filename.\n", ProgName);
      cleanExit (RETURN_FAIL, NULL);
    }
    
    /* Now go through all the names and search for a match */
    while (1)
    {
      /* Read the entry */
      if ( Read (inFile, &pkzCenRec, 46) < 22) return FALSE;
      
      /* Test the current magic number */
      if (pkzCenRec.MagicNum != CRC_MAGNUM)
      {
        /* We went through the whole list without a match */
        FPrintf (StdErr, "%s: No match for filename.\n", ProgName);
        cleanExit (RETURN_WARN, NULL);
      }
      
      /* Get extra info lengths */
      nlen = LES (pkzCenRec.FNameLen);
      elen = LES (pkzCenRec.EFieldLen);
      clen = LES (pkzCenRec.FCommentLen);
      
      /* Read in the name */
      if ( Read (inFile, FName, nlen) != nlen) return FALSE;
      FName[nlen] = 0;
      
      /* Attempt to match the pattern */
      if (MatchPatternNoCase (pOrigName, FName))
      {
        /* Output information */
        FPrintf (StdOut, "Extracting %s.\n", FName);
      
        /* Got a name, seek to the start of this item */
        Seek (inFile, LEL (pkzCenRec.LHeadOff), OFFSET_BEGINNING);
        
        /* Break the loop */
        break;
      }
      
      /* No match, move along */
      Seek (inFile, elen + clen, OFFSET_CURRENT);
    }
    
    free (pOrigName);
  }
  
  /* Save the current position */
  pkzHeadOffset = Seek (inFile, 0, OFFSET_CURRENT);
  
  /* Consume the header and extra info */
  if ( Read (inFile, &pkzHead, 30) != 30) return FALSE;
  if (pkzHead.MagicNum != LHD_MAGNUM)
  {
    FPrintf (StdErr, 
             "%s: Error - Didn't read a Local header - not a Zip file.\n",
             ProgName);
    cleanExit (RETURN_ERROR, NULL);    
  }
  nlen = LES (pkzHead.FNameLen);
  elen = LES (pkzHead.EFieldLen);
  Seek (inFile, (nlen + elen), OFFSET_CURRENT);
  
  /* Check the compression */
  if ( LES (pkzHead.CMethod) != 8)
  {
    FPrintf (StdErr,
             "%s: Error - Only Deflate compression method supported.\n",
             ProgName);
    cleanExit (RETURN_ERROR, NULL);
  }
  
  /* inFile now points to the compressed data */
  return TRUE;
}


/*
** Read the header of a PKZip file and return the CRC
** and USize in supplied arrays.
** Return TRUE if no errors, else FALSE.
*/
BOOL readPKZTail (BPTR inFile, ULONG *CRC, ULONG *USize)
{
  struct PKZHead pkzHead;
  LONG oldp;
  
  /* Get the `current' header */
  oldp = Seek (inFile, pkzHeadOffset, OFFSET_BEGINNING);
  if ( Read (inFile, &pkzHead, 30) != 30) return FALSE;
  Seek (inFile, oldp, OFFSET_BEGINNING);
  
  /* Fill the return values */
  *CRC   = LEL (pkzHead.CRC);
  *USize = LEL (pkzHead.USize);
  
  return TRUE;
}


/*
** Jump to the start of the first field with FieldID.
** Return the offset as well as setting the file pointer.
** Return -1 for error.
*/
LONG GotoFirstField (BPTR file, ULONG FieldID)
{
  UBYTE Buffer[46];
  /* Convenient `gateways' */
  struct PKZHead *pkzHead = (struct PKZHead *) Buffer;
  struct PKZCenRec *pkzCenRec = (struct PKZCenRec *) Buffer;
  ULONG item, nextSeek;
  int cont = TRUE;
  
  
  do 
  {
    /* Read in an item */
    if ( Read (file, &item, 4) != 4) return -1;
    
    /* Identify item and get the rest of the field if required */
    switch (item) {
    case LHD_MAGNUM:
      /* Local header */
      
      /* Check if want to stop here */
      if (FieldID == LHD_MAGNUM)
        cont = FALSE;
      else
      {
        /* Move along */
        if ( Read (file, Buffer+4, 26) != 26) return -1;  /* 26 = 30 - 4 */
        nextSeek = LES (pkzHead->FNameLen) + LES (pkzHead->EFieldLen) + 
                   LEL (pkzHead->CSize);
        Seek (file, nextSeek, OFFSET_CURRENT);
      }
      break;
    
    case CRC_MAGNUM:
      /* Central record entry */
      
      /* Check if want to stop here */
      if (FieldID == CRC_MAGNUM)
        cont = FALSE;
      else
      {
        /* Move along */
        if ( Read (file, Buffer+4, 42) != 42) return -1;  /* 42 = 46 - 4 */
        nextSeek = LES (pkzCenRec->FNameLen) + LES (pkzCenRec->EFieldLen) + 
                   LES (pkzCenRec->FCommentLen);
        Seek (file, nextSeek, OFFSET_CURRENT);
      }
      break;
    
    case ECR_MAGNUM:
      /* End of central record */
      
      /* Final destination - stop here! */
      cont = FALSE;
      break;
    
    default:
      /* Can't id the field - error */
      return -1;
    }    
  } while (cont);
  
  /* Seek back to the start of the current item */
  Seek (file, -4, OFFSET_CURRENT);
  
  /* return the current offset */
  return Seek (file, 0, OFFSET_CURRENT);
}
