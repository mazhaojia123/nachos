/* noff.h 
 *     Data structures defining the Nachos Object Code Format
 *
 *     Basically, we only know about three types of segments:
 *	code (read-only), initialized data, and unitialized data
 */

#define NOFFMAGIC    0xbadfad    /* magic number denoting Nachos
					 * object code file 
					 */

typedef struct segment {
    // lab6: 虚拟空间的偏移量？
    int virtualAddr;        /* location of segment in virt addr space */
    // lab6: 是什么？这个文件内的 偏移量吗？
    int inFileAddr;        /* location of segment in this file */
    int size;            /* size of segment */
} Segment;

typedef struct noffHeader {
    // lab6:
    //  1. 魔数
    //  2. 代码段
    //  3. 初始化的数据段
    //  4. 未初始化的数据段
    //  有一个问题哈！内存里的是怎么排列的？segment 处的东西是 指针 还是 一个实际的对象
    //  我们 sizeof 它之后，大小为40, 按说应该一下子就应该内存处是一个实际的对象
    int noffMagic;        /* should be NOFFMAGIC */
    Segment code;        /* executable code segment */
    Segment initData;        /* initialized data segment */
    Segment uninitData;        /* uninitialized data segment --
				 * should be zero'ed before use 
				 */
} NoffHeader;
