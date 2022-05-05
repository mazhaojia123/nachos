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

BitMap *bitmap;
bool ThreadMap[MAX_USERPOCESSES]; // lab78: 这个初始化其实默认了还没有分配

AddrSpace::AddrSpace(OpenFile *executable) {
    NoffHeader noffH;
    unsigned int i, size;

    // --------------- !! added by yourself ???? ----------------------
    // 一次最多允许 MAX_USERPROCESSES user processes execcutables concurrently.
    // spaceID, i.e. pid
    // lab78: 也就是说，这里我们首先尝试去分配一个 spaceID
    bool hasAvailablePid = false;
    for (int i = 100; i < MAX_USERPOCESSES; i++) { // 0~99 是内核
        // lab78: 这里的 threadMap 是什么东西？
        if (!ThreadMap[i]) {
            ThreadMap[i] = true;
            spaceID = i;  // lab78: 这个到底是从哪来的？？？
            hasAvailablePid = true;
            break;
        }
    }
    if (!hasAvailablePid) {
        printf("Too many process in Nachos. \n");
        return;
    }

    // lab78: 也就是说第一次到此时才会创建全局的物理页的映射
    if (bitmap == NULL)
        bitmap = new BitMap(NumPhysPages);

    // lab6: 首先把 noffH 给读出来
    executable->ReadAt((char *) &noffH, sizeof(noffH), 0);
    // lab6: 为什么要做 SwapHeader?
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    // lab6: 计算整个地址空间的大小 ?
    //  并且额外计算一部分栈的空间
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
           + UserStackSize;    // we need to increase the size
    // to leave room for the stack

    // lab5: 首先计算最多多少页，然后再去，再去针对 size 对页数做一个取整
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);        // check we're not trying
    // to run anything too big --
    // at least until we have
    // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
          numPages, size);

//// first, set up the translation
//    // lab6: 创建页表, 并做初始化
//    //  其实我们发现，载入程序的时候，该文件的 code 和 data 都放入了物理内存
//    //  而且采取了物理地址 == 虚拟地址的方法
//    pageTable = new TranslationEntry[numPages];
//    for (i = 0; i < numPages; i++) {
//        pageTable[i].virtualPage = i;    // for now, virtual page # = phys page #
//        pageTable[i].physicalPage = i;
//        pageTable[i].valid = TRUE;
//        pageTable[i].use = FALSE;
//        pageTable[i].dirty = FALSE;
//        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
//        // a separate page, we could set its
//        // pages to be read-only
//    }
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


// zero out the entire address space, to zero the unitialized data segment
// and the stack segment
    // lab78: 我们在思考一个问题，下面这句应当注释掉，因为我们使用了位视图
    //  用来分配物理内存的空间，所以没必要先清零内存。
//    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        // lab6: 如果代码区不为空，那么就读到主存的某个位置去？
        // lab78：如果这样读进来岂不是有问题？
        //  我们似乎需要将几个段的开头都读进来
//        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
//                           noffH.code.size, noffH.code.inFileAddr);
        // lab78: 我们首先按照一个不太对的方法来实现吧：因为可能潜在的内存不连续
        int code_page = noffH.code.virtualAddr / PageSize;
        int code_offset = noffH.code.virtualAddr % PageSize;
        int code_phy_addr = pageTable[code_page].physicalPage * PageSize + code_offset;
        executable->ReadAt(&(machine->mainMemory[code_phy_addr]),
                           noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        // lab6: 如果代码的数据区不为空，那么就读到主存的某个位置上去
        //  这里有一个和上面同样的问题 —— 为什么是虚地址, 但是使用的是物理内存?
//        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
//                           noffH.initData.size, noffH.initData.inFileAddr);
        // lab78: 我们首先按照一个不太对的方法来实现吧：因为可能潜在的内存不连续
        int data_page = noffH.initData.virtualAddr / PageSize;
        int data_offset = noffH.initData.virtualAddr % PageSize;
        int data_phy_addr = pageTable[data_page].physicalPage * PageSize + data_offset;
        executable->ReadAt(&(machine->mainMemory[data_phy_addr]),
                           noffH.initData.size, noffH.initData.inFileAddr);
    }



    for (int i=3;i<10;i++)    //up to open 10 file for each process
        fileDescriptor[i] = NULL;

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

