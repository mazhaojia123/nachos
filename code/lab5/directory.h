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

    // ??????
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
        sibling = nullptr; // ?????????
        child = nullptr; // ??????????????????

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

    std::vector<CatalogNode *> getChildren(); // ????????????

    void eraseChild(const std::string &theRPath); // ???????????????

    void insertChild(const std::string &theRPath); // ????????????

    void setPath(const std::string &thePath); // ?????????

    // lab5: ???currentNode??????????????????/??????????????????????????????????????????true
    //  ????????????????????????????????? false.
    //  ????????????????????? setPath?????????????????????????????? true/false ??????
    bool findSetPath(const std::string &thePath);

    void setPathWithParent(); // ?????????

    std::vector<std::string> getFPath(); // ?????????

    bool save(); // ???????????????

    bool load(); // ???????????????

    void setTable(DirectoryEntry *theTable) {
        table = theTable;
    }

    static std::vector<std::string> split(const std::string &s);

    void draw();

private:
    void erase(CatalogNode *cur); // ??????????????????????????????????????????

    void printBT(const std::string &prefix, const CatalogNode *node, bool isLeft);

    static void ascendInsert(CatalogNode *theHead, CatalogNode *thePar, const std::string &rPath);

    void preSave(CatalogNode *cur);

    void preLoad(CatalogNode *&cur);

    void updatePar(CatalogNode *);

//    void dispose(CatalogNode *cur);

    CatalogNode root; // ??????????????????????????????, ???????????????????????????????????????
    CatalogNode *currentNode; // ???????????????????????????
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

    CatalogTree t;      // lab5: ???????????????
};


#endif // DIRECTORY_H
