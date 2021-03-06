// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler() {
    readyList = new List;
#ifdef USER_PROGRAM
    // lab78: 如果 joinee 没有退出，joiner 进入等待
    waitingList = new List;
    // lab78: 线程调用了 Finish() 后进入这个状态
    //  Joiner 通过检查这个队列，确定 Joinee 是否已经退出
    terminatedList = new List;
#endif
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler() {
    delete readyList;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun(Thread *thread) {
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);
    readyList->Append((void *) thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun() {
    return (Thread *) readyList->Remove();
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	** already been changed from running to blocked or ready (depending). **
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run(Thread *nextThread) {
    Thread *oldThread = currentThread;

#ifdef USER_PROGRAM            // ignore until running user programs
    if (currentThread->space != NULL) {    // if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
        currentThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow();            // check if the old thread
    // had an undetected stack overflow

    currentThread = nextThread;            // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running

    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
          oldThread->getName(), nextThread->getName());

    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {        // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print() {
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

List *Scheduler::getTerminatedList() {
    return terminatedList;
}

List *Scheduler::getWaitingList() {
    return waitingList;
}

void Scheduler::deleteTerminatedThread(int spaceId) {
    if (terminatedList->IsEmpty())
        return;

    ListElement *cur = terminatedList->getFirst();
    ListElement *lst = terminatedList->getLast();
    ListElement *prv = NULL;


    // 检查第一个元素是不是
    Thread *now = (Thread *) cur->item;
    if (now->space->getSpaceID() == spaceId) {
        // 按理说他会自己管理首尾指针
        terminatedList->Remove();
        return;
    }

    if (cur == lst) {
        // 就一个元素直接返回
        return;
    }

    prv = cur;
    cur = prv->next;

    for (; cur != lst; cur = cur->next, prv = prv->next) {
        if (now->space->getSpaceID() == spaceId) {
            // 找到之后删除
            prv->next = cur->next;
            if (cur == terminatedList->getLast()) // 检查是不是最后一个元素
                terminatedList->setLast(prv);
            delete cur;
            return;
        }
    }
}

void Scheduler::PrintThreads() {
    printf("------------ current --------------\n");
    Thread *t = currentThread;
    if (t->space != NULL) {
        printf("spaceID: %d\n", t->space->getSpaceID());
    } else {
        printf("spaceID: NULL; 可能是创始线程\n", t->space->getSpaceID());
    }
    printf("------------ current --------------\n");
    printf("------------- ready ---------------\n");
    int len = readyList->ListLength();
    for (int i = 1; i <= len; i++) {
        Thread *t = (Thread*)readyList->getItem(i);
        if (t->space != NULL) {
            printf("spaceID: %d\n", t->space->getSpaceID());
        } else {
            printf("spaceID: NULL; 可能是创始线程\n", t->space->getSpaceID());
        }
    }
    printf("------------- ready ---------------\n");
    printf("----------- terminated ------------\n");
    len = terminatedList->ListLength();
    for (int i = 1; i <= len; i++) {
        Thread *t = (Thread*)terminatedList->getItem(i);
        if (t->space != NULL) {
            printf("spaceID: %d\n", t->space->getSpaceID());
        } else {
            printf("spaceID: NULL; 可能是创始线程\n", t->space->getSpaceID());
        }
    }
    printf("----------- terminated ------------\n");
    printf("------------ waiting --------------\n");
    len = waitingList->ListLength();
    for (int i = 1; i <= len; i++) {
        Thread *t = (Thread*)readyList->getItem(i);
        if (t->space != NULL) {
            printf("spaceID: %d\n", t->space->getSpaceID());
        } else {
            printf("spaceID: NULL; 可能是创始线程\n", t->space->getSpaceID());
        }
    }
    printf("------------ waiting --------------\n");
}
