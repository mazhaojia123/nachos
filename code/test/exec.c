//
// Created by mzj on 2022/4/27.
//

#include "syscall.h"

int main()
{
    SpaceId pid;
    pid=Exec("../test/halt.noff");
    Halt();
}