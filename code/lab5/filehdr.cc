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


bool FileHeader::Allocate(BitMap *freeMap, int fileSize, int incrementBytes) {
    if (numSectors > 30)
        return false;   // lab5: 超过了文件的限定大小

    // lab5: 在一个空文件后面追加内容
    if ((fileSize == 0) && (incrementBytes > 0)) {
        if (freeMap->NumClear() < 1) // lab5: 没有空闲块了
            return false;

        // lab5: 首先分配一个磁盘块，更新头文件信息; Find 设置了 位视图文件
        dataSectors[0] = freeMap->Find();
        numSectors = 1;
        numBytes = 0;
    }

    numBytes = fileSize;
    int offset = numBytes % SectorSize;
    int newSectorBytes = incrementBytes - (SectorSize - (offset + 1));
    if (newSectorBytes <= 0) {
        // lab5: 此时发现不需要新的扇区，直接修改文件头，然后返回
        numBytes = numBytes + incrementBytes;
        return true;
    }

    // lab5: 此时需要分配新的扇区
    int moreSectors = divRoundUp(newSectorBytes, SectorSize);
    if (numSectors + moreSectors > 30)
        return false;
    if (freeMap->NumClear() < moreSectors)
        return false;
    for (int i = numSectors; i < numSectors + moreSectors; i++) {
        dataSectors[i] = freeMap->Find();
    }
    numBytes = numBytes + incrementBytes;
    numSectors = numSectors + moreSectors;
    return true;
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
FileHeader::Allocate(BitMap *freeMap, int fileSize) {
    //lab5:
    //  给了 位视图 和 文件大小 从而做空间的分配
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
        return FALSE;        // not enough space

    for (int i = 0; i < numSectors; i++)
        dataSectors[i] = freeMap->Find();
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void
FileHeader::Deallocate(BitMap *freeMap) {
    for (int i = 0; i < numSectors; i++) {
        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
        freeMap->Clear((int) dataSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector) {
    // lab5: 把自己所占有的内存的首地址给 ＳynchDisk
    //  ＳynchDisk 会把这个内存当成 字符 数组来进行写入
    synchDisk->ReadSector(sector, (char *) this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector) {
    // lab5: 把自己所占有的内存的首地址给 ＳynchDisk
    //  ＳynchDisk 会把这个内存当成 字符 数组来进行 读取
    synchDisk->WriteSector(sector, (char *) this);
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
FileHeader::ByteToSector(int offset) {
    return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength() {
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print() {
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
            if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char) data[j]);
        }
        printf("\n");
    }
    delete[] data;
}
