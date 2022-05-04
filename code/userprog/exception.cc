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
            default:
                printf("Unexpected syscall %d %d\n", which, type);
                ASSERT(FALSE);
        }
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}

