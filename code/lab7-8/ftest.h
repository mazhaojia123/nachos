//
// Created by mzj on 2022/5/5.
//

#ifndef NACHOS_FTEST_H
#define NACHOS_FTEST_H

extern void Append(char *from, char *to, int half), NAppend(char *from, char *to);
extern void Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);

extern char *EraseStrChar(char *str,char c);
extern char* itostr(int num);
extern char *setStrLength(char *str, int len);
extern char *numFormat(int num);
#endif //NACHOS_FTEST_H
