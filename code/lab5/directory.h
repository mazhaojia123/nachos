// directory.h 
//	Data structures to manage a UNIX-like directory of file names.
// 
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and 
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
//#include <queue>

#define FileNameMaxLen        9    // for simplicity, we assume
// file names are <= 9 characters long

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.

class DirectoryEntry {
public:
    bool inUse;                // Is this directory entry in use?
    int sector;                // Location on disk to find the
    //   FileHeader for this file
    char name[FileNameMaxLen + 1];    // Text name for file, with +1 for
    // the trailing '\0'
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk. 

class Directory {
public:
    Directory(int size);        // Initialize an empty directory
    // with space for "size" files
    ~Directory();            // De-allocate the directory

    void FetchFrom(OpenFile *file);    // Init directory contents from disk
    void WriteBack(OpenFile *file);    // Write modifications to
    // directory contents back to disk

    int Find(char *name);        // Find the sector number of the
    // FileHeader for file: "name"

    bool Add(char *name, int newSector);  // Add a file name into the directory

    bool Remove(char *name);        // Remove a file from the directory

    // 只读
    void List();            // Print the names of all the files
    //  in the directory

    void Print();            // Verbose print of the contents
    //  of the directory -- all the file
    //  names and their contents.

private:
    int tableSize;            // Number of directory entries
    DirectoryEntry *table;        // Table of pairs:
    // <file name, file header location>

    int FindIndex(char *name);        // Find the index into the directory
    //  table corresponding to "name"
};


struct CatalogNode {
    CatalogNode() {
        rPath = "";
        parent = nullptr;
        sibling = nullptr; // 右孩子
        child = nullptr; // 左孩子？？？

        inUse = true;
        sector = 0;
    }

    //lab5: <inUse, sector, name/rpath>
    bool inUse;
    int sector;
    std::string rPath;

    CatalogNode *sibling;
    CatalogNode *parent;
    CatalogNode *child;
};

class CatalogTree {
public:
    CatalogTree();

    ~CatalogTree();

    CatalogNode *getCurNode() {
        return currentNode;
    }

    std::vector<CatalogNode *> getChildren(); // 看成链表

    void eraseChild(const std::string &theRPath); // 看成二叉树

    void insertChild(const std::string &theRPath); // 看成链表

    void setPath(const std::string &thePath); // 从语义

    // lab5: 将currentNode指向绝对路径/相对路径所指向的节点，并返回true
    //  如果没有这样的节点返回 false.
    //  我们可以看成是 setPath的升级版，失败会返回 true/false 信息
    bool findSetPath(const std::string &thePath);

    void setPathWithParent(); // 从语义

    std::vector<std::string> getFPath(); // 从语义

    bool save(); // 看成二叉树

    bool load(); // 看成二叉树

    void setTable(DirectoryEntry *theTable) {
        table = theTable;
    }

    static std::vector<std::string> split(const std::string &s);

    void draw();

private:
    void erase(CatalogNode *cur); // 将当前的目录所在的子树释放掉

    void printBT(const std::string &prefix, const CatalogNode *node, bool isLeft);

    static void ascendInsert(CatalogNode *theHead, CatalogNode *thePar, const std::string &rPath);

    void preSave(CatalogNode *cur);

    void preLoad(CatalogNode *&cur);

    void updatePar(CatalogNode *);

//    void dispose(CatalogNode *cur);

    CatalogNode root; // 根目录是不能删除掉的, 根目录默认是一个空的字符串
    CatalogNode *currentNode; // 指向当前所在的节点
//    std::ofstream output;
//    std::ifstream input;

    int offset;

    DirectoryEntry *table;
};


class MultiLevelDir {
public:
    MultiLevelDir(int size);        // Initialize an empty directory
    // with space for "size" files
    ~MultiLevelDir();            // De-allocate the directory

    void FetchFrom(OpenFile *file);    // Init directory contents from disk
    void WriteBack(OpenFile *file);    // Write modifications to
    // directory contents back to disk

    int Find(char *name);        // Find the sector number of the
    // FileHeader for file: "name"

    bool Add(char *name, int newSector);  // Add a file name into the directory

    bool Remove(char *name);        // Remove a file from the directory

    void List();            // Print the names of all the files
    //  in the directory
    void Print();            // Verbose print of the contents
    //  of the directory -- all the file
    //  names and their contents.

private:
    int tableSize;            // Number of directory entries
    DirectoryEntry *table;        // Table of pairs:
    // <file name, file header location>

    int FindIndex(char *name);        // Find the index into the directory
    //  table corresponding to "name"

    CatalogTree t;      // lab5: 目录树对象
};


#endif // DIRECTORY_H
