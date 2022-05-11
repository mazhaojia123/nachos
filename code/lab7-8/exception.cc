// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "progtest.h"
#include "ctype.h"
#include "ftest.h"

AddrSpace *space;


// AdvancePC 用来增加 PC 寄存器，只有在系统调用的异常处理过程中被使用
// 防止我们重启系统调用的异常程序，造成无限循环
void AdvancePC() {
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(PCReg) + 4);
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which) {
    // lab78: 从 2 号寄存器里面拿到 ”系统调用号“
    int type = machine->ReadRegister(2);

//    // lab78, old: 这里处理的系统调用
//    if ((which == SyscallException) && (type == SC_Halt)) {
//        DEBUG('a', "Shutdown, initiated by user program.\n");
//        interrupt->Halt();
//    } else {
//        printf("Unexpected user mode exception %d %d\n", which, type);
//        ASSERT(FALSE);
//    }

    // lab78: 更改后的系统调用处理逻辑
    int i, addr;
    int spaceId;
    Thread *thread;
    char filename[50];
    OpenFile *executable;
    char *forkedThreadName;
    int ExitStatus;
    if (which == SyscallException) {
        switch (type) {
            case SC_Halt:
                DEBUG('a', "Shutdown, initiated by user program. \n");
                interrupt->Halt();
                break;
            case SC_Join:
                printf("Execute system call of Join(). \n");
                spaceId = machine->ReadRegister(4);
                currentThread->Join(spaceId);
                // 返回 Joinee 的退出码 waitProcessExitCode
                machine->WriteRegister(2, currentThread->waitProcessExitCode);
                printf("lab78: waitProcessExitCode is %d\n", currentThread->waitProcessExitCode);
                AdvancePC();
                break;
            case SC_Exit:
                ExitStatus = machine->ReadRegister(4);
                printf("lab78: ExitStatus is %d\n", ExitStatus);
                machine->WriteRegister(2, ExitStatus);
                currentThread->setExitStatus(ExitStatus);
                if (ExitStatus == 99) {
                    List *terminatedList = scheduler->getTerminatedList();
                    scheduler->emptyList(terminatedList);
                    // lab78: 我感觉这里有 BUG ！！！
                    // TODO: 没有让终止进程正确退出
                }
                printf("Execute system call of Exit(). \n");
                AdvancePC();
                currentThread->Finish();
                delete currentThread->space;
                break;
            case SC_Yield:
                currentThread->Yield();
                AdvancePC();
                break;
#ifdef FILESYS_STUB
                case SC_Exec:
                // lab78: 增加实现
                printf("Execute system call of Exec()\n");
                addr = machine->ReadRegister(4);
                i = 0;
                do {
                    // lab78: 从内存中读取文件的名字
                    machine->ReadMem(addr + i, 1, (int *) &filename[i]);
                } while (filename[i++] != '\0');

                printf("lab78: exec 文件名: %s\n", filename);
                executable = fileSystem->Open(filename);
                if (executable == NULL) {
                    printf("Unable to open file %s\n", filename);
                    return;
                }

                // lab78: 创建一个用户空间
                //  这个space要怎么把他传给？
                space = new AddrSpace(executable);
                delete executable;

                forkedThreadName = filename;

                // lab78: 创建一个新的核心线程，并将这个用户进程映射到核心线程上
                thread = new Thread(forkedThreadName);
                thread->Fork(StartProcess, space->getSpaceID());

                thread->space = space;      // 用户线程映射到核心线程

                // lab78: 触发线程的切换
                currentThread->Yield();
                // lab78: 我们把 spaceID 作为此调用的返回值返回
                machine->WriteRegister(2, space->getSpaceID());

                printf("Exec(%s): \n", filename);

                AdvancePC();
                break;
                case SC_Create: {
                    int base = machine->ReadRegister(4);
                    int value;
                    int count = 0;
                    char *FileName = new char[128];
                    do {
                        machine->ReadMem(base + count, 1, &value);
                        FileName[count] = *(char *) &value;
                        count++;
                    } while (*(char *) &value != '\0' && count < 128);

                    int fileDescriptor = OpenForWrite(FileName);
                    if (fileDescriptor == -1)
                        printf("create file %s failed ! \n", FileName);
                    else
                        printf("create file %s succeed! The file id is %d. \n", FileName, fileDescriptor);

                    Close(fileDescriptor);

                    machine->WriteRegister(2, fileDescriptor);
                    // lab78: 为什么没有返回值？
                    AdvancePC();
                    break;
                }
                case SC_Open: {
                    int base = machine->ReadRegister(4);
                    int value;
                    int count = 0;
                    char *FileName = new char[128];
                    do {
                        machine->ReadMem(base + count, 1, &value);
                        FileName[count] = *(char *) &value;
                        count++;
                    } while (*(char *) &value != '\0' && count < 128);

                    int fileDescriptor = OpenForReadWrite(FileName, FALSE);
                    if (fileDescriptor == -1)
                        printf("Open file %s failed!\n", FileName);
                    else
                        printf("Open file %s succeed! The file id is %d. \n", FileName, fileDescriptor);
                    machine->WriteRegister(2, fileDescriptor);
                    AdvancePC();
                    break;
                }
                case SC_Write: {
                    int base = machine->ReadRegister(4);
                    int size = machine->ReadRegister(5);
                    int fileId = machine->ReadRegister(6);
                    int value;
                    int count = 0;

                    printf("base=%d, size=%d, fileId=%d \n", base, size, fileId);
                    OpenFile *openfile = new OpenFile(fileId);
                    ASSERT(openfile != NULL);

                    char *buffer = new char[128];
                    do {
                        machine->ReadMem(base + count, 1, &value);
                        buffer[count] = *(char *) &value;
                        count++;
                    } while ((*(char *) &value != '\0') && (count < size));
                    buffer[size] = '\0';

                    int WritePosition;
                    if (fileId == 1)
                        WritePosition = 0;
                    else
                        WritePosition = openfile->Length();

                    int writtenBytes = openfile->WriteAt(buffer, size, WritePosition);
                    if ((writtenBytes) == 0)
                        printf("write file failed!\n");
                    else
                        printf("\"%s\" has wrote in file %d succeed!\n", buffer, fileId);
                    // lab78: 为什么之前没有将这个内容放到
                    machine->WriteRegister(2, size);
                    AdvancePC();

                    break;
                }
                case SC_Read: {
                    int base = machine->ReadRegister(4);
                    int size = machine->ReadRegister(5);
                    int fileId = machine->ReadRegister(6);

                    OpenFile *openfile = new OpenFile(fileId);
                    char buffer[size];
                    int readnum = 0;
                    readnum = openfile->Read(buffer, size);

                    for (int i = 0; i < size; i++)
                        if (!machine->WriteMem(base, 1, buffer[i]))
                            printf("This is something wrong.\n");
                    buffer[size] = '\0';
                    printf("read succeed! the content is \"%s\" , the length is %d\n", buffer, size);
                    machine->WriteRegister(2, readnum);
                    AdvancePC();
                    break;
                }
                case SC_Close: {
                    int fileId = machine->ReadRegister(4);
                    //printf("SC_Close: fileId in $4 = %d\n",fileId);
                    //void Close(int fd) in sysdep.cc

                    //OpenFile* openfile=new OpenFile(fileId);
                    //delete openfile;  //does not work well
                    Close(fileId);

                    printf("File %d  closed succeed!\n", fileId);
                    AdvancePC();
                    break;
                }
#else
#ifdef MYFILESYS

            case SC_Exec: {
                //DEBUG('f',"Execute system call Exec()\n");
                //read argument (i.e. filename) of Exec(filename)
                // lab78: 1. 读取输入的命令
                char filename[128];
                int addr = machine->ReadRegister(4);
                int ii = 0;
                //read filename from mainMemory
                do {
                    machine->ReadMem(addr + ii, 1, (int *) &filename[ii]);
                } while (filename[ii++] != '\0');

                //---------------------------------------------------------
                //
                // inner commands --begin
                //
                //---------------------------------------------------------
                //printf("Call Exec(%s)\n",filename);
                if (filename[0] == 'l' && filename[1] == 's')   //ls
                {
                    // lab78: 2-1. 实现 ls 命令
                    printf("File(s) on Nachos DISK:\n");
                    fileSystem->List();
                    machine->WriteRegister(2, 127);   //
                    AdvancePC();
                    break;
                } else if (filename[0] == 'r' && filename[1] == 'm')  //rm file
                {
                    // lab78: 2-2. 实现 rm 命令
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    fn[0] = ' ';
                    fn[1] = ' ';
                    char *file = EraseStrChar(fn, ' ');
                    if (file != NULL && strlen(file) > 0) {
                        fileSystem->Remove(file);
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("Remove: invalid file name.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                } //rm
                else if (filename[0] == 'c' && filename[1] == 'a' && filename[2] == 't')  //cat file
                {
                    // lab78: 2-3 实现了 cat 命令
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    fn[0] = ' ';
                    fn[1] = ' ';
                    fn[2] = ' ';
                    char *file = EraseStrChar(fn, ' ');
                    //printf("filename=%s, fn=%s, file=%s\n",filename,fn, file);
                    if (file != NULL && strlen(file) > 0) {
                        Print(file);
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("Cat: file not exists.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                } else if (filename[0] == 'c' && filename[1] == 'f')  //create a nachos file
                {  //create file
                    // lab78: 实现了创建文件的命令
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    //printf("filename=%s, fn=%s\n",filename,fn);
                    fn[0] = ' ';
                    fn[1] = ' ';

                    char *file = EraseStrChar(fn, ' ');
                    if (file != NULL && strlen(file) > 0) {
                        fileSystem->Create(file, 0);
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("Create: file already exists.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                } else if (filename[0] == 'c' && filename[1] == 'p')   //cp source dest
                {
                    // lab78: 实现了 cp 文件的命令
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    fn[0] = '#';
                    fn[1] = '#';
                    fn[2] = '#';
                    char source[128];
                    char dest[128];
                    int startPos = 3;
                    int j = 0;
                    int k = 0;
                    for (int i = startPos; i < 128 /*, fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            source[j++] = fn[i];
                        else
                            break;
                    source[j] = '\0';
                    j++;
                    //printf("j = %d\n",j);

                    for (int i = startPos + j; i < 128 /*,fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            dest[k++] = fn[i];
                        else
                            break;
                    dest[k] = '\0';

                    if (source == NULL || strlen(source) <= 0) {
                        printf("cp: Source file not exists.\n");
                        machine->WriteRegister(2, -1);
                        AdvancePC();
                        break;
                    }
                    if (dest != NULL && strlen(dest) > 0) {
                        NAppend(source, dest);
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("cp: Missing dest file.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                }
                //uap source(Unix) dest(nachos)
                else if ((filename[0] == 'u' && filename[1] == 'a' && filename[2] == 'p')
                         || (filename[0] == 'u' && filename[1] == 'c' && filename[2] == 'p')) {
                    // lab78:
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    fn[0] = '#';
                    fn[1] = '#';
                    fn[2] = '#';
                    fn[3] = '#';
                    char source[128];
                    char dest[128];
                    int startPos = 4;
                    int j = 0;
                    int k = 0;
                    for (int i = startPos; i < 128/*, fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            source[j++] = fn[i];
                        else
                            break;
                    source[j] = '\0';
                    j++;

                    for (int i = startPos + j; i < 128/*,fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            dest[k++] = fn[i];
                        else
                            break;
                    dest[k] = '\0';

                    if (source == NULL || strlen(source) <= 0) {
                        printf("uap or ucp: Source file not exists.\n");
                        machine->WriteRegister(2, -1);
                        AdvancePC();
                        break;
                    }
                    if (dest != NULL && strlen(dest) > 0) {
                        if (filename[0] == 'u' && filename[1] == 'c' && filename[2] == 'p')
                            Append(source, dest, 0); //append dest file at the end of source file
                        else
                            // lab78: 调用了 ftest 中的 Copy
                            Copy(source, dest);    //ucp
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("uap or ucp: Missing dest file.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                }
                    //nap source dest
                else if (filename[0] == 'n' && filename[1] == 'a' && filename[2] == 'p') {
                    // lab78: 实现了 nap
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    fn[0] = '#';
                    fn[1] = '#';
                    fn[2] = '#';
                    fn[3] = '#';
                    //char *file = EraseStrSpace(fn, ' ');
                    char source[128];
                    char dest[128];
                    int j = 0;
                    int k = 0;
                    int startPos = 4;
                    for (int i = startPos; i < 128/*, fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            source[j++] = fn[i];
                        else
                            break;
                    source[j] = '\0';
                    j++;
                    //printf("j = %d\n",j);

                    for (int i = startPos + j; i < 128/*,fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            dest[k++] = fn[i];
                        else
                            break;
                    dest[k] = '\0';

                    if (source == NULL || strlen(source) <= 0) {
                        printf("nap: Source file not exists.\n");
                        machine->WriteRegister(2, -1);
                        AdvancePC();
                        break;
                    }
                    if (dest != NULL && strlen(dest) > 0) {
                        // lab78: 调用了 NAppend，fstest.cc  中实现了
                        NAppend(source, dest);
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("ap: Missing dest file.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                }
                //rename source dest
                else if (filename[0] == 'r' && filename[1] == 'e' && filename[2] == 'n') {
                    // lab78: 这个功能不太重要，咱们就不管了
                    char fn[128];
                    strncpy(fn, filename, strlen(filename) - 5);
                    fn[strlen(filename) - 5] = '\0';
                    fn[0] = '#';
                    fn[1] = '#';
                    fn[2] = '#';
                    fn[3] = '#';
                    char source[128];
                    char dest[128];
                    int j = 0;
                    int k = 0;
                    int startPos = 4;
                    for (int i = startPos; i < 128/*, fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            source[j++] = fn[i];
                        else
                            break;
                    source[j] = '\0';
                    j++;
                    //printf("j = %d\n",j);

                    for (int i = startPos + j; i < 128/*,fn[i] != '\0'*/; i++)
                        if (fn[i] != ' ')
                            dest[k++] = fn[i];
                        else
                            break;
                    dest[k] = '\0';

                    if (source == NULL || strlen(source) <= 0) {
                        printf("rename: Source file not exists.\n");
                        machine->WriteRegister(2, -1);
                        AdvancePC();
                        break;
                    }
                    if (dest != NULL && strlen(dest) > 0) {
                        fileSystem->Rename(source, dest);
                        machine->WriteRegister(2, 127);
                    } else {
                        printf("rename: Missing dest file.\n");
                        machine->WriteRegister(2, -1);
                    }
                    AdvancePC();
                    break;
                } else if (strstr(filename, "format") != NULL)   //format
                {
                    // lab78: 实现了将操作系统中的 DISK 的格式化
                    printf("strstr(filename,\"format\"=%s \n", strstr(filename, "format"));
                    printf("WARNING: Format Nachos DISK will erase all the data on it.\n");
                    printf("Do you want to continue (y/n)? ");
                    char ch;
                    while (true) {
                        ch = getchar();
                        if (toupper(ch) == 'Y' || toupper(ch) == 'N')
                            break;
                    } //while
                    if (toupper(ch) == 'N') {
                        machine->WriteRegister(2, 127);   //
                        AdvancePC();
                        break;
                    }

                    printf("Format the DISK and create a file system on it.\n");
                    fileSystem->FormatDisk(true);
                    machine->WriteRegister(2, 127);   //
                    AdvancePC();
                    break;
                } else if (strstr(filename, "fdisk") != NULL)   //fdisk
                {
                    // lab78: 展示当前文件系统的信息
                    fileSystem->Print();
                    machine->WriteRegister(2, 127);   //
                    AdvancePC();
                    break;
                } else if (strstr(filename, "perf") != NULL)   //Performance
                {
                    // lab78: 查看当前文件系统的运行情况
                    PerformanceTest();
                    machine->WriteRegister(2, 127);   //
                    AdvancePC();
                    break;
                } else if (filename[0] == 'p' && filename[1] == 's')   //ps
                {
                    scheduler->PrintThreads();
                    machine->WriteRegister(2, 127);   //
                    AdvancePC();
                    break;
                } else if (strstr(filename, "help") != NULL)   //fdisk
                {
                    printf("Commands and Usage:\n");
                    printf("ls                : list files on DISK.\n");
                    printf("fdisk             : display DISK information.\n");
                    printf("format            : format DISK with creating a file system on it.\n");
                    printf("performence       : test DISK performence.\n");
                    printf("cf  name          : create a file \"name\" on DISK.\n");
                    printf("cp  source dest   : copy Nachos file \"source\" to \"dest\".\n");
                    printf("nap source dest   : Append Nachos file \"source\" to \"dest\"\n");
                    printf("ucp source dest   : Copy Unix file \"source\" to Nachos file \"dest\"\n");
                    printf("uap source dest   : Append Unix file \"source\" to Nachos file \"dest\"\n");
                    printf("cat name          : print content of file \"name\".\n");
                    printf("rm  name          : remove file \"name\".\n");
                    printf("rename source dest: Rename Nachos file \"source\" to \"dest\".\n");
                    printf("ps                : display the system threads.\n");
                    //-----------------------------------------------------------

                    machine->WriteRegister(2, 127);   //
                    AdvancePC();
                    break;
                } else  //check if the parameter of exec(file), i.e file, is valid
                {
                    if (strchr(filename, ' ') != NULL || strstr(filename, ".noff") == NULL)
                        //not an inner command, and not a valid Nachos executable, then return
                    {
                        machine->WriteRegister(2, -1);
                        AdvancePC();
                        break;
                    }
                }
                //---------------------------------------------------------
                //
                // inner commands --end
                //
                //---------------------------------------------------------
                //---------------------------------------------------------
                //
                // loading an executable and execute it
                //
                //---------------------------------------------------------
                // lab78: 此时不是系统内部命令，而是一个可执行文件
                //  1. 打开可执行文件
                //  2. 将可执行文件从 DISK 装入 内存（也就是虚拟机的内存数组）；
                //     创建地址空间，建立物理地址和逻辑地址的映射
                //  3. 创建一个核心线程，然后将该地址空间绑定到核心线程
                OpenFile *executable = fileSystem->Open(filename);
                if (executable == NULL) {
                    DEBUG('f', "\nUnable to open file %s\n", filename);
                    machine->WriteRegister(2, -1);
                    AdvancePC();
                    break;
                }

                space = new AddrSpace(executable);
                delete executable;            // close file

                DEBUG('H', "Execute system call Exec(\"%s\"), it's SpaceId(pid) = %d \n", filename,
                      space->getSpaceID());
                char *forkedThreadName = filename;

                char *fname = strrchr(filename, '/');
                if (fname != NULL)  // there exists "/" in filename
                    forkedThreadName = strtok(fname, "/");
                thread = new Thread(forkedThreadName);
                thread->Fork(StartProcess, space->getSpaceID());
                thread->space = space;
                printf("user process \"%s(%d)\" map to kernel thread \" %s \"\n", filename, space->getSpaceID(),
                       forkedThreadName);

                machine->WriteRegister(2, space->getSpaceID()); // lab78: 返回值

                currentThread->Yield();

                AdvancePC();
                break;
            }


            case SC_Create: {
                int base = machine->ReadRegister(4);
                int value;
                int count = 0;
                char *FileName = new char[128];

                do {
                    machine->ReadMem(base + count, 1, &value);
                    FileName[count] = *(char *) &value;
                    count++;
                } while (*(char *) &value != '\0' && count < 128);

                //when calling Create(),  thread go to sleep, waked up when I/O finish
                if (!fileSystem->Create(FileName, 0))  //call Create() in FILESYS,see filesys.h
                    printf("create file %s failed!\n", FileName);
                else
                    DEBUG('f', "create file %s succeed!\n", FileName);
                AdvancePC();
                break;
            }


            case SC_Open: {
                int base = machine->ReadRegister(4);
                int value;
                int count = 0;
                char *FileName = new char[128];

                do {
                    machine->ReadMem(base + count, 1, &value);
                    FileName[count] = *(char *) &value;
                    count++;
                } while (*(char *) &value != '\0' && count < 128);

                int fileid;
                //call Open() in FILESYS,see filesys.h,Nachos Open()
                OpenFile *openfile = fileSystem->Open(FileName);
                if (openfile == NULL) {   //file not existes, not found
                    printf("File \"%s\" not Exists, could not open it.\n", FileName);
                    fileid = -1;
                } else {  //file found
//set the opened file id in AddrSpace, which wiil be used in Read() and Write()
                    fileid = currentThread->space->getFileDescriptor(openfile);
                    if (fileid < 0)
                        printf("Too many files opened!\n");
                    else
                        DEBUG('f', "file :%s open secceed!  the file id is %d\n", FileName, fileid);
                }
                machine->WriteRegister(2, fileid);
                AdvancePC();
                break;
            }

            case SC_Write: {
                int base = machine->ReadRegister(4);  //buffer
                int size = machine->ReadRegister(5);   //bytes written to file
                int fileId = machine->ReadRegister(6); //fd
                int value;
                int count = 0;

                // printf("base=%d, size=%d, fileId=%d \n",base,size,fileId );
                OpenFile *openfile = new OpenFile(fileId);
                ASSERT(openfile != NULL);

                char *buffer = new char[128];
                do {
                    machine->ReadMem(base + count, 1, &value);
                    buffer[count] = *(char *) &value;
                    count++;
                } while ((*(char *) &value != '\0') && (count < size));
                buffer[size] = '\0';

//                OpenFile *openfile = currentThread->space->getFileId(fileId);
                openfile = currentThread->space->getFileId(fileId);
                //printf("openfile =%d\n",openfile);
                if (openfile == NULL) {
                    printf("Failed to Open file \"%d\" .\n", fileId);
                    AdvancePC();
                    break;
                }

                if (fileId == 1 || fileId == 2) {
                    openfile->WriteStdout(buffer, size);
                    delete[] buffer;
                    AdvancePC();
                    break;
                }

                int WritePosition = openfile->Length();

                openfile->Seek(WritePosition);  //append write
                //openfile->Seek(0);      //write form  begining

                int writtenBytes;
                //writtenBytes = openfile->AppendWriteAt(buffer,size,WritePosition);
                writtenBytes = openfile->Write(buffer, size);
                if ((writtenBytes) == 0)
                    DEBUG('f', "\nWrite file failed!\n");
                else {
                    if (fileId != 1 && fileId != 2) {
                        DEBUG('f', "\n\"%s\" has wrote in file %d succeed!\n", buffer, fileId);
                        DEBUG('H', "\n\"%s\" has wrote in file %d succeed!\n", buffer, fileId);
                        DEBUG('J', "\n\"%s\" has wrote in file %d succeed!\n", buffer, fileId);
                    }
                    //printf("\n\"%s\" has wrote in file %d succeed!\n",buffer,fileId);
                }

                //delete openfile;
                delete[] buffer;
                AdvancePC();
                break;
            }


            case SC_Read: {
                int base = machine->ReadRegister(4);
                int size = machine->ReadRegister(5);
                int fileId = machine->ReadRegister(6);

                OpenFile *openfile = currentThread->space->getFileId(fileId);

                char buffer[size];
                int readnum = 0;
                if (fileId == 0)  //stdin
                    readnum = openfile->ReadStdin(buffer, size);
                else
                    readnum = openfile->Read(buffer, size);

                for (int i = 0; i < readnum; i++)
                    machine->WriteMem(base, 1, buffer[i]);
                buffer[readnum] = '\0';

                for (int i = 0; i < readnum; i++)
                    if (buffer[i] >= 0 && buffer[i] <= 9)
                        buffer[i] = buffer[i] + 0x30;

                char *buf = buffer;
                if (readnum > 0) {
                    if (fileId != 0) {
                        DEBUG('f', "Read file (%d) succeed! the content is \"%s\" , the length is %d\n", fileId, buf,
                              readnum);
                        DEBUG('H', "Read file (%d) succeed! the content is \"%s\" , the length is %d\n", fileId, buf,
                              readnum);
                        DEBUG('J', "Read file (%d) succeed! the content is \"%s\" , the length is %d\n", fileId, buf,
                              readnum);
                    }
                } else
                    printf("\nRead file failed!\n");

                machine->WriteRegister(2, readnum);
                AdvancePC();
                break;
            }

            case SC_Close: {
                int fileId = machine->ReadRegister(4);
                OpenFile *openfile = currentThread->space->getFileId(fileId);
                if (openfile != NULL) {
                    openfile->WriteBack();  // write file header back to DISK

                    delete openfile;        // close file
                    currentThread->space->releaseFileDescriptor(fileId);

                    DEBUG('f', "File %d  closed succeed!\n", fileId);
                    DEBUG('H', "File %d  closed succeed!\n", fileId);
                    DEBUG('J', "File %d  closed succeed!\n", fileId);
                } else
                    printf("Failded to Close File %d.\n", fileId);
                AdvancePC();
                break;
            }
#endif // ifdef MYFILESYS
#endif // ifdef FILESYS_STUB
            default:
                printf("Unexpected syscall %d %d\n", which, type);
                ASSERT(FALSE);
        }
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}

