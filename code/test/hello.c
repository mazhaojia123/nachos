//
// Created by mzj on 2022/5/5.
//
#include "syscall.h"

int
main() {
    OpenFileId output = ConsoleOutput;
    char buf[12];

    buf[0] = 'H';
    buf[1] = 'e';
    buf[2] = 'l';
    buf[3] = 'l';
    buf[4] = 'o';
    buf[5] = ' ';
    buf[6] = 'w';
    buf[7] = 'o';
    buf[8] = 'r';
    buf[9] = 'l';
    buf[10] = 'd';
    buf[11] = '\n';
    buf[12] = '\0';

    Write(buf, 12, output);

    Exit(0);
}
