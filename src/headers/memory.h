// include guards to prevent double declaration of any identifiers 
#ifndef MEMORY_GUARD
#define MEMORY_GUARD

#include <stdint.h>
#include <headers/cpu.h>

/*========================================*/
/*      physical memory on dram chips     */
/*========================================*/

// physical momory space is decided by the physical address
// in this simulator, there are 4 + 6 + 6 = 16 bits physical address
// then the pyhsical space is (1 << 16) = 65536
// total 16 physical memory
#define PHYSICAL_MEMORY_SPACE    (65536)
#define MAX_INDEX_PHYSICAL_PAGE  (15)
#define MAX_NUM_PHYSICAL_PAGE    (16)   // 1 + MAX_INDEX_PHYSICAL_PAGE

#define PAGE_TABLE_ENTRY_NUM     (521)

// physical memory
// 16 physical memory pages
// only use for user process
uint8_t pm[PHYSICAL_MEMORY_SPACE];

// page table entry struct(8 bytes)
// 8 bytes = 64 bits
typedef union 
{
    uint64_t pte_value;

    struct 
    {
        uint64_t present        : 1;    // 子页表在内存中（1），不在（0）
        uint64_t readonly       : 1;    // 对于子页，只读或者读写访问权限 
        uint64_t usermoed       : 1;    // 处于什么状态对页面的换入和换出很关键，内核的页不能由用户swap
        uint64_t writethough    : 1;    // write through or write back
        uint64_t cachedisabled  : 1;    // 是否可以将页放入 cache
        uint64_t reference      : 1;    
        uint64_t unused6        : 1;    // not use 6
        uint64_t smallpage      : 1;    // page size, big for 4mb or small for 4kb
        uint64_t global         : 1;    
        uint64_t unused9_11     : 3;    // not use 9,10,11
        /*原本：
        uint64_t paddr          : 40;   
        uint64_t unused52_62    : 11;   
        
        由于前三级页表需要缓冲下一级页表的地址而不是ppn的地址，而我们的地址是malloc在heap上的
        又因为heap上的地址为48位，所以说我们需要从unused部分拿出10位供我们使用
        */
        uint64_t paddr          : 50;   
        uint64_t unused62       : 1;   
        uint64_t xdisabled      : 1;
    };


    // page is swap out in disk
    struct 
    {
        uint64_t _present       : 1;
        uint64_t daddr          : 63;   // disk address
    };

} pte123_t; // 123 means 前三级页表 - 索引下一级页表

typedef union 
{
    uint64_t pte_value;

    struct 
    {
        uint64_t present        : 1;    // 子页表在内存中（1），不在（0）
        uint64_t readonly       : 1;    // 对于子页，只读或者读写访问权限 
        uint64_t usermoed       : 1;    // 处于什么状态对页面的换入和换出很关键，内核的页不能由用户swap
        uint64_t writethough    : 1;    // write through or write back
        uint64_t cachedisabled  : 1;    // 是否可以将页放入 cache
        uint64_t reference      : 1;    
        uint64_t dirty          : 1;    // dirty value - 1; clean value - 0;    /*主要差别*/
        uint64_t zero7          : 1;    // alwalys 0
        uint64_t global         : 1;    
        uint64_t unused9_11     : 3;    // not use 9,10,11
        /* 因为第四级页表索引的是ppn(40bits)，而不是页表的地址(48bits)
           所以说这里我们不需要拓展paddr的范围 */
        uint64_t ppn            : 40;   
        uint64_t unused52_62    : 1;   
        uint64_t xdisabled      : 1;
    };


    // page is swap out in disk
    struct 
    {
        uint64_t _present       : 1;    // present 0
        uint64_t daddr          : 63;   // disk address
    }; 

} pte4_t; // 第四级页表 - 索引 ppn


// physical page descriptor
typedef struct 
{
    // three state
    int allocated;   // 1 means have allocated
    int dirty;       // 1 means dirty value
    int time;        // LRU cache

    pte4_t *pte4;    // the reversed mapping: from PPN to page table entry
    // really world: mapping to anno_vma or address_space
    // we simply the situation here
    // TODO: if mutiple process are using this page e.g. shared library
    /* 反向映射并不是直接由物理地址映射到page table，它需要间接通过addresss_space */
    uint64_t daddr;   // disk address, binding the reverse mapping with mapping to disl
} pd_t; // page descriptor

 // for each pagable (mappable) physical page, create one mapping
 // create one reversed mapping
 /* 这里的反向映射显然太浪费空间了，明显可以优化，但是我们没有.. */
pd_t page_map[MAX_NUM_PHYSICAL_PAGE];  // 反向映射表 ppn->pt



/*============================*/
/*      memory R/W            */
/*============================*/

// used by instructions: read or write uint8_t DRAM
uint64_t cpu_read64bits_dram (uint64_t paddr);
void     cpu_write64bits_dram(uint64_t paddr, uint64_t data);

// cpu get the instruction at dram, so it's necessary to set the interface for cpu to w/r the instruction in dram
void cpu_readinst_dram (uint64_t padrr, char *buf);
void cpu_writeinst_dram(uint64_t paddr, const char *str); 
/*================================================================================*/
/*                              [Warning]                                         */
/*  In the theory, the area where code is stored in memory should be read-only,   */
/*  but we nedd to write to the memory code area, so this function is set         */
/*================================================================================*/


#endif