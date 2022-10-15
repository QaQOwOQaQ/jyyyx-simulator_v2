// Memory Management unit
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <headers/common.h>
#include <headers/cpu.h>
#include <headers/memory.h>
#include <headers/address.h>

uint64_t va2pa(uint64_t vaddr)
{
    // use page table to transfer virtual adress to physical adddress
    
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

    // CR3 register's value is malloced on the heap of this simulator
    pte123_t *pgd = (pte123_t *)cpu_controls.cr3;

}