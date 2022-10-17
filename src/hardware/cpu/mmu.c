// Memory Management unit
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <headers/common.h>
#include <headers/cpu.h>
#include <headers/memory.h>
#include <headers/address.h>

static uint64_t page_walk(uint64_t vaddr_value);
static void page_fault_handler(pte4_t *pet, address_t vaddr); 

int swap_in(uint64_t daddr, uint64_t ppn);


uint64_t va2pa(uint64_t vaddr)
{
    // use page table to transfer virtual adress to physical adddress
    return page_walk(vaddr);
}

// input - virtual address
// output - physical address
// page_walk: “walk” at page table's pointer: CR3->PGD->PUD->PMD->PT->PPN
/* 解释：由于我们的内存pm太小，只有16KB，无法存放多级页表(我们使用四级页表)，
因此我们将多级页表放在程序的heap中（其实这个heap就是我们assemble simulator的heap）
这样cr3原本是指向一个pa的，现在指向了heap中的一个地址
作者说：这是一个妥协，只是为了方便coding
*/
static uint64_t page_walk(uint64_t vaddr_value)
{
    // 转换地址类型，方便我们直接得到ppn，ppo等，而不是各种位运算（当然直接使用位运算是可行的，但是太麻烦且不好拓展）
    address_t vaddr = {
        .vaddr_value = vaddr_value,
    };
    // CR3-> PGD -> PUD -> PMD -> PT -> PPN
    int page_table_size = PAGE_TABLE_ENTRY_NUM * sizeof(pte123_t);
    
    // CR3 register's value is malloced on the heap of this simulator
    pte123_t *pgd = (pte123_t *)cpu_controls.cr3;           // page global directory
    assert(pgd != NULL); // 肯定存在
    
    if (pgd[vaddr.vpn1].present == 1) // vaddr.vpn1 is the offset of the page table - pgd's starting address
    {
        // starting PHYSICAL PAGE NUMBER of the next level page table
        // aka. high bits starting address of the page table
        // aka. (also known as，亦称、也被称为) 
        // 下一级页表的起始地址，同时我们也认为它是下一级页表的页号
        pte123_t *pud = pgd[vaddr.vpn1].paddr;  // 下一级页表的paddr

        if(pud[vaddr.vpn2].present == 1)
        {
            // find pmd ppn
            pte123_t *pmd = (pte123_t *)pud[vaddr.vpn2].paddr;

            if(pmd[vaddr.vpn3].present == 1)
            {
                // find pt pno
                
                pte4_t *pt = (pte4_t *)(pmd[vaddr.vpn3].paddr);

                if(pt[vaddr.vpn4].present == 1)
                {
                    // find page table entry
                    address_t paddr = {
                        .ppo = vaddr.vpo,    // page table size 
                        .ppn = pt[vaddr.vpn4].ppn
                    };

                    return paddr.paddr_value;
                }
                else 
                {
                    // page table entry not exist
#ifdef DEBUG_PAGE_WALY
                    printf("page walk level[4]: pt[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn4);
#endif        
                    // map the physical page and the virtual page
                    // search paddr from main memory and disk
                    
                    // TODO: raise exception 14(paging fault here)
                    // 这里的缺页处理应该交给kernal处理，但是在我们这里我们相当于交给hardware处理了

                    // because this page not exits mm now, so we have to find it in disk
                    // siwtch privilege from user mode(ring 3) to kernel mode(ring 0)
                    page_fault_handler(&pt[vaddr.vpn4], vaddr);
                }
            }
            else 
            {
                // pt - level 4 not exits
#ifdef DEBUG_PAGE_WALY
               printf("page walk level[3]: pmd[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn3);
#endif        
                pte4_t *pt = malloc(page_table_size);
                memset(pt, 0, sizeof(page_table_size));

                // set page table entry
                pmd[vaddr.vpn3].present = 1;
                pmd[vaddr.vpn3].paddr   = (uint64_t)pt;

                // TODO: page fault (缺页，与中断有关)
                // map the physical page and the virtual page
                exie(0);
            }
        }
        else 
        {        
            // pmd - level 3 not exits
#ifdef DEBUG_PAGE_WALY
            printf("page walk level[2]: pud[%lx].present == 0\n\tmalloc new page middle table for it\n", vaddr.vpn2);
#endif        
            pte123_t *pmd = malloc(page_table_size);
            memset(pmd, 0, sizeof(page_table_size));

            // set page table entry
            pud[vaddr.vpn2].present = 1;
            pud[vaddr.vpn2].paddr   = (uint64_t)pmd;

            // TODO: page fault (缺页，与中断有关)
            // map the physical page and the virtual page
            exie(0);
        }
    }
    else 
    {
        // pud - level 2 not exits
#ifdef DEBUG_PAGE_WALY
        printf("page walk level[1]: pgd[%lx].present == 0\n\tmalloc new page upper table for it\n", vaddr.vpn1);
#endif        
        pte123_t *pud = malloc(page_table_size);
        memset(pud, 0, sizeof(page_table_size));

        // set page table entry
        // we only use the bit of present and paddr, and ignore other bits
        pgd[vaddr.vpn1].present = 1;
        pgd[vaddr.vpn1].paddr   = (uint64_t)pud;

        // TODO: page fault (缺页，与中断有关)
        // map the physical page and the virtual page
        exie(0);
    }
}

static void page_fault_handler(pte4_t *pte, address_t vaddr)
{
    // select one victim physical page to swap
    assert(pte->present == 0); // means this page not in physical memory

    // this is the selected ppn for vaddr
    int ppn = -1;
    pte4_t *victim = NULL;
    uint64_t daddr = -1; 
    int lru_ppn = -1, lru_time = -1;
    
    // 1. try to request oen free physical page fro mDROM
    // kernal's responsibility
    /* 由于我们需要根据 ppn 的值来判断这一页是否存储在内存中（present）
       而我们通过页表又无法得知这个信息，我们只能通过 vaddr 得映射到 paddr 从而判断 present
       而无法通过 paddr 直接得到 present
       所以我们需要增加一个反向映射由 paddr 得到 present
       所以 add 一个 struct -- page_map(memory.h)
    */
    for(int i = 0; i < MAX_NUM_PHYSICAL_PAGE; i ++ )
    {
        if(page_map[i].pte4->present == 0)
        {
            printf("PageFault: use free ppn %d\n", i);
            
            // found i as free page
            ppn = i;
            page_map[ppn].allocated = 1;   // allocated for vaddr
            page_map[ppn].dirty = 0;       // allocated for clean
            page_map[ppn].time = 0;        // most recently used physical page
            page_map[ppn].pte4 = pte;
            
            // update old ptr
            pte->present = 1;
            pte->dirty = 0;
            pte->ppn = ppn;
            
            return ;
        }
    }

    // 2.no free physical page: select a clean page(LRU) overwrite
    // in this case, there is no DRAM - DISK translation
    lru_ppn = -1;
    lru_time = -1;
    
    for(int i = 0; i < MAX_NUM_PHYSICAL_PAGE; i ++ )
    {
        if(page_map[i].dirty == 0 && lru_time < page_map[i].time)
        {
            lru_time = page_map[i].time;
            lru_ppn = i;
        }
    }

    // found a clean physical page, dont need to page put to disk
    if(lru_ppn != -1 && lru_time < MAX_NUM_PHYSICAL_PAGE)
    {
        ppn = lru_ppn;

        // page out，将原本的映射删除
        victim = page_map[lru_ppn].pte4;
        
        // reverse mapping
        /* 维护虚拟页到磁盘的映射
           1. 如果当前页在内存中，映射关系保存在page_map
           2. 当前页不在内存中，映射关系由page table保存 */
        victim->pte_value = 0; // reset
        victim->present = 0;
        victim->daddr = page_map[lru_ppn].daddr;

        // load page from disk to physical memory first
        /* 由于当前pte的present==0，因此它一定不存在于物理内存当中
           所以它一定存在于磁盘当中，并且保存了磁盘的地址daddr
           由于下面的 pte->value=0 会导致我们丢失磁盘地址
           因此我们必须在这里把磁盘页加载到 ppn */
        daddr = pte->daddr;
        swap_in(daddr, ppn);

        pte->pte_value = 0;
        pte->present = 1;
        pte->dirty = 0;
        pte->ppn = ppn;
            
        page_map[lru_ppn].allocated = 1;
        page_map[lru_ppn].dirty = 0;
        page_map[lru_ppn].pte4 = pte;
        page_map[lru_ppn].time = 0;
        page_map[ppn].daddr = daddr; // 由于现在当前页在内存当中，因此将daddr放在page_map中

        return ;
    }

    // 3. no free and no clean physical page
    // write back (swap out) the DIRTY victim to disk
    lru_ppn = -1;
    lru_time = -1;
    
    for(int i = 0; i < MAX_NUM_PHYSICAL_PAGE; i ++ )
    {
        if(lru_time < page_map[i].time)
        {
            lru_time = page_map[i].time;
            lru_ppn = i;
        }
    }
    
    assert(lru_ppn >= 0 && lru_ppn <= MAX_NUM_PHYSICAL_PAGE);

    ppn = lru_ppn;

    // reverse mapping
    victim = page_map[lru_ppn].pte4;
    
    // write back
    /* ppn 与磁盘 swap 建立映射 */
    swap_out(page_map[ppn].daddr, ppn);

    victim->pte_value = 0; // reset
    victim->present = 0;
    victim->daddr = page_map[lru_ppn].daddr;

    // load page from disk to physical memory first
    daddr = pte->daddr;
    swap_in(daddr, ppn);

    pte->pte_value = 0;
    pte->present = 1;
    pte->dirty = 0;
    pte->ppn = ppn;
        
    page_map[lru_ppn].allocated = 1;
    page_map[lru_ppn].dirty = 0;
    page_map[lru_ppn].pte4 = pte;
    page_map[lru_ppn].time = 0;
    page_map[ppn].daddr = daddr; // 由于现在当前页在内存当中，因此将daddr放在page_map中

    return ;

    /* 可以发现，最坏的情况下我们需要扫描三次物理页，这效率也太低了
        因此这里的函数实现肯定有优化的方法，只不过我们为了简单。。
    */
}