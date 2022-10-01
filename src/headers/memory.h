// include guards to prevent double declaration of any identifiers 
#ifndef MEMORY_GUARD
#define MEMORY_GUARD

#include <stdint.h>
#include "cpu.h"

/*========================================*/
/*      physical memory on dram chips     */
/*========================================*/

// physical momory space is decided by the physical address
// in this simulator, there are 4 + 6 + 6 = 16 bits physical address
// then the pyhsical space is (1 << 16) = 65536
// total 16 physical memory
#define PHYSICAL_MEMORY_SPACE    65536
#define MAX_INDEX_PHYSICAL_PAGE  15




// physical memory
// 16 physical memory pages
uint8_t pm[PHYSICAL_MEMORY_SPACE];

/*============================*/
/*      memory R/W            */
/*============================*/

// used by instructions: read or write uint8_t DRAM
uint64_t read64bits_dram (uint64_t paddr, core_t *cr);
void     write64bits_dram(uint64_t paddr, uint64_t data, core_t *cr);

// cpu get the instruction at dram, so it's necessary to set the interface for cpu to w/r the instruction in dram
void readinst_dram (uint64_t padrr, char *buf,       core_t *cr);
void writeinst_dram(uint64_t paddr, const char *str, core_t *cr); 
/*================================================================================*/
/*                              [Warning]                                         */
/*  In the theory, the area where code is stored in memory should be read-only,   */
/*  but we nedd to write to the memory code area, so this function is set         */
/*================================================================================*/


#endif