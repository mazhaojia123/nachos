// openfile.h 
//	Data structures for opening, closing, reading and writing to 
//	individual files.  The operations supported are similar to
//	the UNIX ones -- type 'man open' to the UNIX prompt.
//
//	There are two implementations.  One is a "STUB" that directly
//	turns the file operations into the underlying UNIX operations.
//	(cf. comment in filesys.h).
//
//	The other is the "real" implementation, that turns these
//	operations into read and write disk sector requests. 
//	In this baseline implementation of the file system, we don't 
//	worry about concurrent accesses to the file system
//	by different threads -- this is part of the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef OPENFILE_H
#define OPENFILE_H

#include "copyright.h"
#include "utility.h"

#ifdef FILESYS_STUB            // Temporarily implement calls to

// Nachos file system as calls to UNIX!
// See definitions listed under #else
class OpenFile {
public:
    OpenFile(int f) {
        file = f;
        currentOffset = 0;
    }    // open the file
    ~OpenFile() { Close(file); }            // close the file

    int ReadAt(char *into, int numBytes, int position) {
        Lseek(file, position, 0);
        return ReadPartial(file, into, numBytes);
    }

    int WriteAt(char *from, int numBytes, int position) {
        Lseek(file, position, 0);
        WriteFile(file, from, numBytes);
        return numBytes;
    }

    int Read(char *into, int numBytes) {
        int numRead = ReadAt(into, numBytes, currentOffset);
        currentOffset += numRead;
        return numRead;
    }

    int Write(char *from, int numBytes) {
        int numWritten = WriteAt(from, numBytes, currentOffset);
        currentOffset += numWritten;
        return numWritten;
    }

    int Length() {
        Lseek(file, 0, 2);
        return Tell(file);
    }

private:
    int file;
    int currentOffset;
};

#else // FILESYS

#ifdef MYFILESYS

class FileHeader;

class OpenFile {
public:
    OpenFile(char *type) {}
    OpenFile(int sector);        // Open a file whose header is located
    // at "sector" on the disk
    ~OpenFile();            // Close the file

    void Seek(int position);        // Set the position from which to
    // start reading/writing -- UNIX lseek

    int Read(char *into, int numBytes); // Read/write bytes from the file,
    // starting at the implicit position.
    // Return the # actually read/written,
    // and increment position in file.
    int Write(char *from, int numBytes);

    int ReadAt(char *into, int numBytes, int position);

    // Read/write bytes from the file,
    // bypassing the implicit position.
    int WriteAt(char *from, int numBytes, int position);

    int Length();            // Return the number of bytes in the
    // file (this interface is simpler
    // than the UNIX idiom -- lseek to
    // end of file, tell, lseek back

    // lab5: 将内存中的文件头同步到磁盘上
    void WriteBack();

    // lab78: 增加了标准输入输出的输出形式
    int WriteStdout(char *from, int numBytes) {
        int file = 1;  //stdout
        WriteFile(file, from, numBytes);
        //retVal = write(fd, buffer, nBytes);
        return numBytes;
    }

    int ReadStdin(char *into, int numBytes) {
        int file = 0;  //stdin
        return ReadPartial(file, into, numBytes);
    }
private:
    // lab5: OpenFile 实际上操作的是？ SynchDisk
    //  我们根据 sector 号 来操作文件
    //  维护 以下三个信息 :
    //      1. 头文件（大小，扇区数，扇区号...）(读到内存中了)
    //      2. 头文件所在的扇区号
    //      3. seekPosition
    FileHeader *hdr;            // Header for this file
    int seekPosition;            // Current position within the file
    int hdrSector;
};

#else

class FileHeader;

class OpenFile {
public:
    OpenFile(char *type) {}

    OpenFile(int sector);        // Open a file whose header is located
    // at "sector" on the disk
    ~OpenFile();            // Close the file

    void Seek(int position);        // Set the position from which to
    // start reading/writing -- UNIX lseek

    int Read(char *into, int numBytes); // Read/write bytes from the file,
    // starting at the implicit position.
    // Return the # actually read/written,
    // and increment position in file.
    int Write(char *from, int numBytes);

    // lab5:
    //  只要给了目标内存地址，给了合法的其实偏移量和长度，我们都能读出来
    int ReadAt(char *into, int numBytes, int position);

    // Read/write bytes from the file,
    // bypassing the implicit position.
    int WriteAt(char *from, int numBytes, int position);

    int Length();            // Return the number of bytes in the
    // file (this interface is simpler
    // than the UNIX idiom -- lseek to
    // end of file, tell, lseek back

    // lab78: 增加了标准输入输出的输出形式
    int WriteStdout(char *from, int numBytes) {
        int file = 1;  //stdout
        WriteFile(file, from, numBytes);
        //retVal = write(fd, buffer, nBytes);
        return numBytes;
    }

    int ReadStdin(char *into, int numBytes) {
        int file = 0;  //stdin
        return ReadPartial(file, into, numBytes);
    }

private:
    FileHeader *hdr;            // Header for this file
    int seekPosition;            // Current position within the file
    int hdrSector;
};
#endif // MYFILESYS
#endif // FILESYS

#endif // OPENFILE_H