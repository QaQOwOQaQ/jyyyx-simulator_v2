// Memory Management unit
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <headers/common.h>
#include <headers/cpu.h>
#include <headers/memory.h>

uint64_t va2pa(uint64_t vaddr, core_t *cr)
{
    // mapping of virtual address to physical address through % operation
    return vaddr % PHYSICAL_MEMORY_SPACE;
}