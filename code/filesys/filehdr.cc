// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

FileHeader::FileHeader(int type,BitMap *freeMap)
{
    DEBUG('f', "Create a file header.\n");
    if(type == 0)
        fileType = TYPE_FILE;
    else if(type == 1)
        fileType = TYPE_DIR;
    else if(type == 2)
        fileType = TYPE_MAP;
    else
        ASSERT(false);
    createTime = accessTime = modifiedTime = stats->totalTicks;

    secondIndex = freeMap->Find();
    ASSERT(secondIndex >= 0);
    char buffer[SectorSize];
    memset(buffer, -1, sizeof(buffer));
    synchDisk->WriteSector(secondIndex, buffer);
}
//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    //printf("fileSize: %d, allocate: %d\n", fileSize, numSectors);
    ASSERT((numSectors&0xfffffc00)==0);
    int indexII = (numSectors-1) >> 5;
    int indexI = (numSectors-1) & 31;
    if (freeMap->NumClear() < numSectors+indexII+1)
	return FALSE;		// not enough space

    int buffer[32];
    int tempBuffer[32];
    synchDisk->ReadSector(secondIndex, (char *)buffer);
    for(int i = 0; i < indexII; ++i)
    {
        buffer[i] = freeMap->Find();
        memset(tempBuffer, -1, sizeof(tempBuffer));
        for(int j = 0; j < 32; ++j)
        {
            tempBuffer[j] = freeMap->Find();
        }
        synchDisk->WriteSector(buffer[i], (char *)tempBuffer);
    }
    buffer[indexII] = freeMap->Find();
    memset(tempBuffer, -1, sizeof(tempBuffer));
    for(int j = 0; j <= indexI; ++j)
        tempBuffer[j] = freeMap->Find();
    synchDisk->WriteSector(buffer[indexII], (char *)tempBuffer);
    synchDisk->WriteSector(secondIndex, (char *)buffer);
    return TRUE;
}

bool FileHeader::AddSpace(int size, BitMap *freeMap)
{
    int oriIndexII = (numSectors-1) >> 5;
    int oriIndexI = (numSectors-1) & 31;
    numBytes += size;
    int addSectors = divRoundUp(numBytes, SectorSize)-numSectors;
    numSectors += addSectors;
    ASSERT((numSectors&0xfffffc00)==0);
    if(addSectors == 0)
        return true;

    int indexII = (numSectors-1) >> 5;
    int indexI = (numSectors-1) & 31;
    //printf("sectors: total %d, add %d; indexII: ori %d, now %d; indexI: ori %d, now %d\n",
    //numSectors, addSectors, oriIndexII, indexII, oriIndexI, indexI);
    if (freeMap->NumClear() < indexII-oriIndexII+addSectors)
	    return FALSE;
    int buffer[32];
    int tempBuffer[32];
    synchDisk->ReadSector(secondIndex, (char *)buffer);
    synchDisk->ReadSector(buffer[oriIndexII], (char *)tempBuffer);
    if(indexII == oriIndexII)
    {
        for(int i = oriIndexI+1; i <= indexI; ++i)
            tempBuffer[i] = freeMap->Find();
    }
    else
    {
        for(int i = oriIndexI+1; i < 32; ++i)
            tempBuffer[i] = freeMap->Find();
    }
    synchDisk->WriteSector(buffer[oriIndexII], (char *)tempBuffer);
    for(int i = 1; i < indexII-oriIndexII; ++i)
    {
        buffer[oriIndexII+i] = freeMap->Find();
        memset(tempBuffer, -1, sizeof(tempBuffer));
        for(int j = 0; j < 32; ++j)
        {
            tempBuffer[j] = freeMap->Find();
        }
        synchDisk->WriteSector(buffer[oriIndexII+i], (char *)tempBuffer);
    }
    buffer[indexII] = freeMap->Find();
    memset(tempBuffer, -1, sizeof(tempBuffer));
    for(int i = 0; i <= indexI; ++i)
        tempBuffer[i] = freeMap->Find();
    synchDisk->WriteSector(buffer[indexII], (char *)tempBuffer);
    synchDisk->WriteSector(secondIndex, (char *)buffer);
    return true;
}
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    int buffer[32];
    synchDisk->ReadSector(secondIndex, (char *)buffer);

    int tempBuffer[32];
    for(int i = 0; i < 32; ++i)
    {
        if(buffer[i] != -1)
        {
            synchDisk->ReadSector(buffer[i], (char *)tempBuffer);
            freeMap->Clear(buffer[i]);
            for(int j = 0; j < 32; ++j)
            {
                if(tempBuffer[j] != -1)
                    freeMap->Clear(tempBuffer[j]);
            }
        }
    }
    freeMap->Clear(secondIndex);
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    accessTime = stats->totalTicks;
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    modifiedTime = stats->totalTicks;
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    ASSERT(offset <= numBytes);
    int sectorNum = offset / SectorSize;
    int indexII = sectorNum >> 5;
    int indexI = sectorNum & 31;

    int buffer[32];
    synchDisk->ReadSector(secondIndex, (char *)buffer);
    synchDisk->ReadSector(buffer[indexII], (char *)buffer);
    return(buffer[indexI]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  Created time: %d.  Access time: %d. Modified time:%d\n", numBytes, (int)createTime, (int)accessTime, (int)modifiedTime);
    printf("File blocks:\n");
    int buffer[32];
    synchDisk->ReadSector(secondIndex, (char *)buffer);
    int tempBuffer[32];
    for(int i = 0; i < 32; ++i)
    {
        if(buffer[i] == -1)
            break;
        
        synchDisk->ReadSector(buffer[i], (char *)tempBuffer);
        for(int j = 0; j < 32; ++j)
        {
            if(tempBuffer[j] == -1)
                break;
            
            printf("%d ", tempBuffer[j]);
        }
    }
    printf("\nFile contents:\n");
    // for (i = k = 0; i < numSectors; i++) {
	// synchDisk->ReadSector(dataSectors[i], data);
    //     for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	//     if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
	// 	printf("%c", data[j]);
    //         else
	// 	printf("\\%x", (unsigned char)data[j]);
	// }
    //     printf("\n"); 
    // }

    int k = 0;
    for(int i = 0; i < 32; ++i)
    {
        if(buffer[i] == -1)
            break;
        
        synchDisk->ReadSector(buffer[i], (char *)tempBuffer);
        for(int j = 0; j < 32; ++j)
        {
            if(tempBuffer[j] == -1)
                break;
            
            synchDisk->ReadSector(tempBuffer[j], data);
            for (int m = 0; (m < SectorSize) && (k < numBytes); m++, k++) 
            {
	            if ('\040' <= data[m] && data[m] <= '\176')   // isprint(data[j])
		            printf("%c", data[m]);
                else
		            printf("\\%x", (unsigned char)data[m]);
	        }
            printf("\n"); 
        }
    }
    delete [] data;
}
