// include guards to prevent double declaration of any identifiers 
#ifndef MEMORY_GUARD
#define MOMORY_GUARD

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

#endif