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
    address_t vaddr = {
        .vaddr_value = vaddr_value,
    };
    
    int page_table_size = PAGE_TABLE_ENTRY_NUM * sizeof(pte123_t);
    
    // CR3 register's value is malloced on the heap of this simulator
    pte123_t *pgd = (pte123_t *)cpu_controls.cr3;           // page global directory
    assert(pgd != NULL); // 肯定要存在
    
    if (pgd[vaddr.vpn1].present == 1)
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
                        .ppn = pt[vaddr.vpn4].paddr
                    };

                    return paddr.paddr_value;
                }
                else 
                {
                    // page table entry not exist
#ifdef DEBUG_PAGE_WALY
                    printf("page walk level4: pmd[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn4);
#endif        
                    // TODO: page fault (缺页，与中断有关)
                    // map the physical page and the virtual page
                    exie(0);
                }
            }
            else 
            {
                // pt - level 4 not exits
#ifdef DEBUG_PAGE_WALY
               printf("page walk level3: pmd[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn3);
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
            printf("page walk level2: pud[%lx].present == 0\n\tmalloc new page middle table for it\n", vaddr.vpn2);
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
        printf("page walk level1: pgd[%lx].present == 0\n\tmalloc new page upper table for it\n", vaddr.vpn1);
#endif        
        int page_table_size = PAGE_TABLE_ENTRY_NUM * sizeof(pte123_t);
        pte123_t *pud = malloc(page_table_size);
        memset(pud, 0, sizeof(page_table_size));

        // set page table entry
        pgd[vaddr.vpn1].present = 1;
        pgd[vaddr.vpn1].paddr   = (uint64_t)pud;

        // TODO: page fault (缺页，与中断有关)
        // map the physical page and the virtual page
        exie(0);
    }
}