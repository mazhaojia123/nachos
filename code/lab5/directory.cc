// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size) {
    // lab5: 就是分配一个线性表，用来全部的目录项
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory() {
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file) {
    // lab5: 把整个目录表取进来
    (void) file->ReadAt((char *) table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file) {
    // lab5: 把整个的目录表写回去
    (void) file->WriteAt((char *) table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name) {
    // lab5: 比对整个目录表，查看是否是目标路径
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1;        // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name) {
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector) {
    if (FindIndex(name) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            return TRUE;
        }
    return FALSE;    // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name) {
    int i = FindIndex(name);

    if (i == -1)
        return FALSE;        // name not in directory
    table[i].inUse = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List() {
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print() {
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse) {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}


CatalogTree::CatalogTree() {
    currentNode = &root;
}

CatalogTree::~CatalogTree() {
    // 将除了根节点之外的节点，都删除掉
    CatalogNode *cur = root.child;
    erase(cur); // 删掉整个的二叉结构
}

std::vector<CatalogNode *> CatalogTree::getChildren() {
    std::vector<CatalogNode *> res;
    CatalogNode *cur = currentNode->child;

    if (cur == nullptr) return res;
    else cur = cur->sibling;

    while (cur != nullptr) {
        res.push_back(cur);
        cur = cur->sibling;
    }
    return res;
}

void CatalogTree::eraseChild(const std::string &theRPath) {
    // 先将这个节点的孩子节点所在的二叉树全都删掉
    CatalogNode *pre = currentNode->child;

    // 如果没有孩子，他却要删？？？那咱就直接返回，反正没得删
    if (pre == nullptr) return;

    CatalogNode *cur = pre->sibling;
    while (cur != nullptr && cur->rPath != theRPath) {
        cur = cur->sibling;
        pre = pre->sibling;
    }
    if (cur == nullptr)
        return; // 没有找到这个节点

    erase(cur->child);

    // 把前后链表前后连起来
    pre->sibling = cur->sibling;

    // 把这个节点的空间释放
    delete cur;
}

void CatalogTree::insertChild(const std::string &theRPath) {
    if (currentNode->child == nullptr) {
        // 则给孩子链表设置一个头节点
        // 没有必要让他指向父亲
        // 字符串 @
        // 没有孩子
        currentNode->child = new CatalogNode;
        currentNode->child->rPath = "@"; // 专门表示头节点
    }
    ascendInsert(currentNode->child, currentNode, theRPath);

}

void CatalogTree::setPath(const std::string &thePath) {
    CatalogNode *cur = nullptr;

    if (thePath[0] == '/') { // 这是一个绝对路径
        CatalogNode *newCurrentNode = &root;
        std::vector<std::string> tmpPath = split(thePath);

        for (auto &x: tmpPath) {
            cur = newCurrentNode->child;
            while (cur != nullptr && cur->rPath != x)
                cur = cur->sibling;
            if (cur == nullptr) return;
            else newCurrentNode = cur;
        }

        currentNode = newCurrentNode;
    } else { // 相对路径
        cur = currentNode->child;
        while (cur != nullptr && cur->rPath != thePath)
            cur = cur->sibling;
        if (cur != nullptr)
            currentNode = cur; // 设置当前的路径为thePath
    }
}

void CatalogTree::setPathWithParent() {
    if (currentNode->parent == nullptr) return; // 根节点，直接返回

    currentNode = currentNode->parent; // 更新当前节点
}

std::vector<std::string> CatalogTree::getFPath() {
    CatalogNode *cur = currentNode;
    std::vector<std::string> res;

    while (cur != nullptr) {
        res.push_back(cur->rPath);
        cur = cur->parent;
    }
    reverse(res.begin(), res.end());

    return res;
}

void CatalogTree::preSave(CatalogNode *cur) {
    if (cur != nullptr) {
//        output << cur->rPath << '\n'; // 输出这个名字
        table[offset].inUse = cur->inUse;
        table[offset].sector = cur->sector;
        strcpy(table[offset].name, cur->rPath.c_str());
        offset++;

        preSave(cur->child);
        preSave(cur->sibling);
    } else {
//        output << "-1\n";
        table[offset].inUse = true;
        table[offset].sector = 0;
        strcpy(table[offset].name, "-1");
        offset++;
    }
}

bool CatalogTree::save() {
//    output.open(filePath);
//    if (!output.is_open()) return false;

//    std::vector<std::string> fullPath = getFPath(); // 先记下所在的路径

    offset = 0;
    preSave(root.child); // root的child为根的二叉树，打印到

//    for (int i = 1; i < fullPath.size(); i++)
//        output << fullPath[i] << ' ';

//    output.close();
    return true;
}

void CatalogTree::preLoad(CatalogNode *&cur) {

    if (strcmp(table[offset].name, "-1") == 0) {
        offset++;
        return;
    }

    cur = new CatalogNode;
    cur->inUse = table[offset].inUse;
    cur->sector = table[offset].sector;
    cur->rPath = table[offset].name;

    offset++;
    preLoad(cur->child);
    preLoad(cur->sibling);


//    std::string tmp;
//    input >> tmp;
//
//    if (tmp == "-1" || tmp.empty()) return; // 意味着读取失败
//
//    cur = new CatalogNode;
//    cur->rPath = tmp;
//
//    preLoad(cur->child);
//    preLoad(cur->sibling);
}

void CatalogTree::updatePar(CatalogNode *cur) {
    if (cur != nullptr) {
        currentNode = cur;
        std::vector<CatalogNode *> tmp = getChildren();
        for (auto &item: tmp)
            item->parent = cur;
        updatePar(cur->child);
        updatePar(cur->sibling);
    }
}

bool CatalogTree::load() {
    // 删掉之前的
    erase(root.child);

//    input.open(filePath);
//    if (!input.is_open()) return false;

    offset = 0;

    // 不断new出其他的节点
    preLoad(root.child);


    // 更新全体的父亲节点
    updatePar(&root);

    // 恢复currentNode, 重新指向root
    currentNode = &root;
//    std::string tmp;
//    while (input >> tmp) {
//        setPath(tmp);
//    }
//
//    input.close();
    return true;
}

void CatalogTree::erase(CatalogNode *cur) {
    // 后序思想
//    if (cur != nullptr) {
//        CatalogNode *theCur = cur->child;
//        while (theCur != nullptr) {
//            erase(theCur);
//            theCur = theCur->sibling;
//        }
//        delete cur;
//    }
    if (cur != nullptr) {
        erase(cur->child);
        erase(cur->sibling);
        delete cur;
    }
}

void CatalogTree::ascendInsert(CatalogNode *theHead, CatalogNode *thePar, const std::string &theRPath) {
    // 插入这个节点到链表，并且设置这个节点的父亲
    // 如果这个节点已经存在，不做任何操作
    CatalogNode *pre = theHead;
    CatalogNode *cur = theHead->sibling;

    while (cur != nullptr && cur->rPath < theRPath) {
        cur = cur->sibling;
        pre = pre->sibling;
    }
    if (cur != nullptr && cur->rPath == theRPath)
        return; // 这个节点已经存在，直接返回
    // 插入一个新的节点
    CatalogNode *tmp = new CatalogNode;
    tmp->rPath = theRPath;
    tmp->parent = thePar;
    tmp->sibling = cur;
    pre->sibling = tmp;
}

std::vector<std::string> CatalogTree::split(const std::string &s) {
    // 分成多个相对路径, 不包括开头的root -- "/"
    std::vector<std::string> res;
    std::string tmp;
    for (int i = 1; i < s.size(); i++) {
        if (s[i] != '/')
            tmp += s[i];
        else {
            res.push_back(tmp);
            tmp = "";
        }
    }
    res.push_back(tmp);
    return res;
}

bool CatalogTree::findSetPath(const std::string &thePath) {
    CatalogNode *cur = nullptr;

    if (thePath[0] == '/') { // 这是一个绝对路径
        CatalogNode *newCurrentNode = &root;
        std::vector<std::string> tmpPath = split(thePath);

        for (auto &x: tmpPath) {
            cur = newCurrentNode->child;
            while (cur != nullptr && cur->rPath != x)
                cur = cur->sibling;
            if (cur == nullptr) return false;
            else newCurrentNode = cur;
        }

        currentNode = newCurrentNode;
        return true;
    } else { // 相对路径
        cur = currentNode->child;
        while (cur != nullptr && cur->rPath != thePath)
            cur = cur->sibling;
        if (cur != nullptr) {
            currentNode = cur; // 设置当前的路径为thePath
            return true;
        } else return false;
    }
}

void CatalogTree::draw() {
    printBT("", &root, false);
}

void CatalogTree::printBT(const std::string &prefix, const CatalogNode *node, bool isLeft) {
    if (node != nullptr) {
        std::cout << prefix;

        std::cout << (isLeft ? "├── " : "");

        // print the value of the node
        if (node == &root)
            std::cout << "root" << std::endl;
        else
            std::cout << node->rPath << std::endl;

        // enter the next tree level - left and right branch
        printBT(prefix + (isLeft ? "│   " : "│   "), node->child, true);
        printBT(prefix + (isLeft ? "│   " : ""), node->sibling, false);
    }
}

MultiLevelDir::MultiLevelDir(int size) {
    table = new DirectoryEntry[size];
    tableSize = size;

    t.setTable(table);

    // todo: -------- to remove -----------
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = false;
}

MultiLevelDir::~MultiLevelDir() {
    delete[] table;
}

void MultiLevelDir::FetchFrom(OpenFile *file) {
    (void) file->ReadAt((char *) table, tableSize * sizeof(DirectoryEntry), 0);
    t.load();
}

void MultiLevelDir::WriteBack(OpenFile *file) {
    t.save();
    (void) file->WriteAt((char *) table, tableSize * sizeof(DirectoryEntry), 0);
}

// lab5: 返回文件头所在块的 sector 号
int MultiLevelDir::Find(char *name) {
//    int i = FindIndex(name);
//
//    if (i != -1)
//        return table[i].sector;
//    return -1;

    // 检查这个绝对路径，是否存在
    t.setPath("/");
    if (t.findSetPath(name)) {
        CatalogNode *cur = t.getCurNode();
        return cur->sector;
    }

    return -1;
}

bool MultiLevelDir::Add(char *name, int newSector) {
//    if (FindIndex(name) != -1)
//        return FALSE;
//
//    for (int i = 0; i < tableSize; i++)
//        if (!table[i].inUse) {
//            table[i].inUse = TRUE;
//            strncpy(table[i].name, name, FileNameMaxLen);
//            table[i].sector = newSector;
//            return TRUE;
//        }
//    return FALSE;    // no space.  Fix when we have extensible files.

    t.setPath("/");
    if (t.findSetPath(name)) // 这个目录项已经存在
        return false;

    // 不存在
    std::vector<std::string> tmpPath = CatalogTree::split(name);
    t.setPath("/");
    for (auto &x: tmpPath) {
        if (!t.findSetPath(x)) {// 没找到这个路径
            CatalogNode *old = t.getCurNode();
            t.insertChild(x);
            if (t.findSetPath(x)) {
                CatalogNode *now = t.getCurNode();
                printf("insert child success: %s-->%s \n", old->rPath.c_str(), now->rPath.c_str());
            } else {
                printf("insert child failed. \n");
                return false;
            }
        }
    }

    CatalogNode *cur = t.getCurNode();
    cur->sector = newSector;
    printf("set %s with hdr new sector %d\n", cur->rPath.c_str(), cur->sector);

    return true;
}

bool MultiLevelDir::Remove(char *name) {
//    int i = FindIndex(name);
//
//    if (i == -1)
//        return FALSE;        // name not in directory
//    table[i].inUse = FALSE;
//    return TRUE;

    if (t.findSetPath(name)) {
        t.setPathWithParent();
        std::vector<std::string> tmpPath = CatalogTree::split(name);
        t.eraseChild(*tmpPath.rbegin());
        if (t.findSetPath(name)) {
            printf("failed to delete %s\n", tmpPath.rbegin()->c_str());
            return false;
        }
        return true;
    }

    printf("failed to find %s\n", name);
    return false;
}

void MultiLevelDir::List() {
    t.draw();
}

void MultiLevelDir::Print() {
    t.draw();
}

//int MultiLevelDir::FindIndex(char *name) {
//    // lab5: 给出文件名，然后返回目录表所在的索引值。这是之前的写法。
//    //  现在我们希望返回一个CatalogNode的指针。
//
//    return 0;
//}

