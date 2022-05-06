// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"

#ifdef MYFILESYS

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector) {
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    seekPosition = 0;
    hdrSector = sector;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile() {
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position) {
    seekPosition = position;
}

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes) {
    int result = ReadAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

int
OpenFile::Write(char *into, int numBytes) {
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position) {
    // lab5: 在初始化一个 OpenFile 的时候，已经将 该文件的文件头
    //  从磁盘同步到内存中，形成了 fileHeader 这个结构了
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
        return 0;                // check request
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n",
          numBytes, position, fileLength);

    // lab5:
    //  position / SectorSize 在这个文件所控制的所有文件块中，计算得到我们从第几个块开始读取
    //  position + numByted - 1 在这个文件所控制的所有文件块中，我们最终结束的位置在第几个块中
    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize),
                              &buf[(i - firstSector) * SectorSize]);

    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete[] buf;
    return numBytes;
}


int
OpenFile::WriteAt(char *from, int numBytes, int position) {
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

//    if ((numBytes <= 0) || (position >= fileLength))
//        return 0;                // check request
    // lab5: 允许 position == fileLength ，也就是追加
    if ((numBytes <= 0) || (position > fileLength))
        return -1;                // check request

    // lab5: 这里的代码不允许写超过文件大小的内容。
//    if ((position + numBytes) > fileLength)
//        numBytes = fileLength - position;

    // lab5: 这里重新：
    //  1. 更新了 位视图
    //  2. 更新了头文件
    //  3. 进而增加了需要的块
    //  因为后续的写入都是根据头文件所指定的块来实现，所以这里就是需要修改的全部内容
    if ((position + numBytes) > fileLength) {
        int incrementBytes = (position + numBytes) - fileLength;
        BitMap *freeBitMap = fileSystem->getBitMap(); // TODO: 实现
        bool hdrRet;
        hdrRet = hdr->Allocate(freeBitMap, fileLength, incrementBytes); // TODO: 实现 ；修改了头文件
        if (!hdrRet)
            return -1;
        fileSystem->setBitMap(freeBitMap); // TODO: 实现; 修改了位视图
    }

    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n",
          numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    // lab5: 检查开始写的位置是不是这个块的起始位置
    //       检查最终结束的位置是不是最终块的end位置（下一个块的起始）
    firstAligned = (bool) (position == (firstSector * SectorSize));
    lastAligned = (bool) ((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    // lab5: 首先把他们的开头结尾都读出来
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize],
               SectorSize, lastSector * SectorSize);

// copy in the bytes we want to change
    // lab5: 实际上是做了这样的操作
    //  [ 起始块 ] [] [] ... [] [ 终止块 ] (逻辑文件示意图)
    //     |------- bcopy修改的 ----|
    //  开头结尾各有部分不变
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// write modified sectors back
    for (i = firstSector; i <= lastSector; i++)
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize),
                               &buf[(i - firstSector) * SectorSize]);
    delete[] buf;
    return numBytes;
}

//int
//OpenFile::WriteAt(char *from, int numBytes, int position) {
//    int fileLength = hdr->FileLength();
//    int i, firstSector, lastSector, numSectors;
//    bool firstAligned, lastAligned;
//    char *buf;
//
//    // lab5: 我们需要在这里修改 限制
//    if ((numBytes <= 0) || (position >= fileLength))
//        return 0;                // check request
//    if ((position + numBytes) > fileLength)
//        numBytes = fileLength - position;
//    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n",
//          numBytes, position, fileLength);
//
//    firstSector = divRoundDown(position, SectorSize);
//    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
//    numSectors = 1 + lastSector - firstSector;
//
//    buf = new char[numSectors * SectorSize];
//
//    firstAligned = (bool) (position == (firstSector * SectorSize));
//    lastAligned = (bool) ((position + numBytes) == ((lastSector + 1) * SectorSize));
//
//// read in first and last sector, if they are to be partially modified
//    if (!firstAligned)
//        ReadAt(buf, SectorSize, firstSector * SectorSize);
//    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
//        ReadAt(&buf[(lastSector - firstSector) * SectorSize],
//               SectorSize, lastSector * SectorSize);
//
//// copy in the bytes we want to change
//    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);
//
//// write modified sectors back
//    for (i = firstSector; i <= lastSector; i++)
//        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize),
//                               &buf[(i - firstSector) * SectorSize]);
//    delete[] buf;
//    return numBytes;
//}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() {
    return hdr->FileLength();
}

void OpenFile::WriteBack() {
    hdr->WriteBack(hdrSector);
}

#else
//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector) {
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    seekPosition = 0;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile() {
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position) {
    seekPosition = position;
}

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes) {
    int result = ReadAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

int
OpenFile::Write(char *into, int numBytes) {
    // lab5:
    //  1. 在当前的 seekPosition 处做修改
    //  2. 写完之后还会移动 seekPosition
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position) {
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
        return 0;                // check request
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n",
          numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize),
                              &buf[(i - firstSector) * SectorSize]);

    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete[] buf;
    return numBytes;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position) {
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

//    if ((numBytes <= 0) || (position >= fileLength))
//        return 0;                // check request
    // lab5: 允许 position == fileLength ，也就是追加
    if ((numBytes <= 0) || (position > fileLength))
        return -1;                // check request

    // lab5: 这里的代码不允许写超过文件大小的内容。
//    if ((position + numBytes) > fileLength)
//        numBytes = fileLength - position;

    // lab5: 这里重新：
    //  1. 更新了 位视图
    //  2. 更新了头文件
    //  3. 进而增加了需要的块
    //  因为后续的写入都是根据头文件所指定的块来实现，所以这里就是需要修改的全部内容
    if ((position + numBytes) > fileLength) {
        int incrementBytes = (position + numBytes) - fileLength;
        BitMap *freeBitMap = fileSystem->getBitMap(); // TODO: 实现
        bool hdrRet;
        hdrRet = hdr->Allocate(freeBitMap, fileLength, incrementBytes); // TODO: 实现
        if (!hdrRet)
            return -1;
        fileSystem->setBitMap(freeBitMap); // TODO: 实现
    }

    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n",
          numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    // lab5: 检查开始写的位置是不是这个块的起始位置
    //       检查最终结束的位置是不是最终块的end位置（下一个块的起始）
    firstAligned = (bool) (position == (firstSector * SectorSize));
    lastAligned = (bool) ((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    // lab5: 首先把他们的开头结尾都读出来
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize],
               SectorSize, lastSector * SectorSize);

// copy in the bytes we want to change
    // lab5: 实际上是做了这样的操作
    //  [ 起始块 ] [] [] ... [] [ 终止块 ] (逻辑文件示意图)
    //     |------- bcopy修改的 ----|
    //  开头结尾各有部分不变
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// write modified sectors back    
    for (i = firstSector; i <= lastSector; i++)
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize),
                               &buf[(i - firstSector) * SectorSize]);
    delete[] buf;
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() {
    return hdr->FileLength();
}
#endif