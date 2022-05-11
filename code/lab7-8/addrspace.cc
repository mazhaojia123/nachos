// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "bitmap.h"
#include "openfile.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

BitMap *bitmap;                     // lab78: 用来映射物理内存帧
bool ThreadMap[MAX_USERPOCESSES];   // lab78: 用来维护每个pid的占用情况，true 表示该pid没有占用

AddrSpace::AddrSpace(OpenFile *executable) {
    NoffHeader noffH;
    unsigned int i, size;

    // lab78: 1. 分配 pid / spaceID
    bool hasAvailablePid = false;
    for (int i = 100; i < MAX_USERPOCESSES; i++) { // 0~99 是内核
        if (!ThreadMap[i]) {
            ThreadMap[i] = true;
            spaceID = i;
            hasAvailablePid = true;
            break;
        }
    }
    if (!hasAvailablePid) {
        printf("Too many process in Nachos. \n");
        return;
    }

    // lab78: 2. 检查位视图是否已经创建
    if (bitmap == NULL)
        bitmap = new BitMap(NumPhysPages);

    executable->ReadAt((char *) &noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // lab6: 计算整个地址空间的大小 ? 并且额外计算一部分栈的空间
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
           + UserStackSize;

    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    // lab6: 防止程序太大，内存超出我们的物理内存
    ASSERT(numPages <= NumPhysPages);

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
          numPages, size);

    // lab78: 3. 建立页表
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;    // for now, virtual page # = phys page #
        pageTable[i].physicalPage = bitmap->Find();
        ASSERT(pageTable[i].physicalPage != -1);
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
        // a separate page, we could set its
        // pages to be read-only
    }

    // lab78: 我们在思考一个问题，下面这句应当注释掉，因为我们使用了位视图
    //  用来分配物理内存的空间，所以没必要先清零内存。
    //  清零内存应该在机器的初始化时和释放物理内存时来做
//    bzero(machine->mainMemory, size);

    // lab78: 4. 将程序从文件系统读取到物理内存
    // TODO: 我们默认了程序的每一段可以完全放到我们从位视图找到的第一段空的空间
    //  但是如果出现了物理内存不连续的情况，可能会出现后放入的内存的程序覆盖了原先的
    //  的程序。换句话说，我们需要一帧一帧的将程序装入内存，而非一次一段。
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        int code_page = noffH.code.virtualAddr / PageSize;
        int code_offset = noffH.code.virtualAddr % PageSize;
        int code_phy_addr = pageTable[code_page].physicalPage * PageSize + code_offset;
        executable->ReadAt(&(machine->mainMemory[code_phy_addr]),
                           noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        int data_page = noffH.initData.virtualAddr / PageSize;
        int data_offset = noffH.initData.virtualAddr % PageSize;
        int data_phy_addr = pageTable[data_page].physicalPage * PageSize + data_offset;
        executable->ReadAt(&(machine->mainMemory[data_phy_addr]),
                           noffH.initData.size, noffH.initData.inFileAddr);
    }

    // lab78: 5. 初始化 file descriptor
    for (int i=3;i<10;i++)    //up to open 10 file for each process
        fileDescriptor[i] = NULL;

    // lab78: 这里的初始化很草率，是因为，它只是希望调用
    //  linux 的读写标准输入输出，并非需要打开真正的 Nachos 文件
    OpenFile *StdinFile = new OpenFile("stdin");
    OpenFile *StdoutFile = new OpenFile("stdout");
    fileDescriptor[0] =  StdinFile;
    fileDescriptor[1] =  StdoutFile;
    fileDescriptor[2] =  StdoutFile;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
    ThreadMap[spaceID] = 0;
    for (int i = 0; i < numPages; i++) {
        bitmap->Clear(pageTable[i].physicalPage);
    }
    delete[] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters() {
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}


int AddrSpace::getFileDescriptor(OpenFile * openfile)
{
    for (int i=3;i<10;i++)
    {
        if (fileDescriptor[i] == NULL)
        {
            fileDescriptor[i] = openfile;
            return i;
        }  //if
    }  //for
    return -1;
}

OpenFile* AddrSpace::getFileId(int fd)
{
    return fileDescriptor[fd];
}

void AddrSpace::releaseFileDescriptor(int fd)
{
    fileDescriptor[fd] = NULL;
}

