// include guard to prevent double declaration of any identifiers
// such as types, enum and static variables
#ifndef DEBUG_GUARD
#define DEBUG_GUARD

#include <stdint.h>

/*========================*/
/*      debug_type        */
/*========================*/

/*  powerful bit operations of C language       */
/*  if we can use a bit express one thing       */
/*  so we can use a number expree lots of thing */

#define DEBUG_INSTRUCTIONCYCLE      0x1     // instruction_cycle
#define DEBUG_REGISTERS             0X2     // registers
#define DEBUG_PRINTSTACK            0x4     // print_stack
#define DEBUG_PRINTCACHESET         0X8     // print_cache_set
#define DEBUG_MMU                   0x10    // mmu
#define DEBUG_LINKER                0X40    // linker
#define DEBUG_LOADER                0x80    // loader
#define DEBUG_PARSEINST             0x100   // parse_inst

#define DEBUG_VERBOSE_SET           0x1     // debug switch for debug_type  

// do page walk
#define DEBUG_ENABLE_PAGE_WALK      0

// use SRAM Cache for Memory acces
#define DEBUG_ENABLE_SRAM_CACHE     0

// print warpper
uint64_t debug_print(uint64_t open_set, const char *format, ... );


/* type converter function */
// uint32 to it's equivalent float with rounding(convert as bit level)
uint32_t uint2float(uint32_t u);

// convert string dec or hex to the integer bitmap
uint64_t string2uint(const char *str);
uint64_t string2uint_range(const char *str, int start, int end);

#endif