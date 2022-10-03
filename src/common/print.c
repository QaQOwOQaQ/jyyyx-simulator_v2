#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <headers/common.h>

// wrapper of stdio print
// controlled by the debug verbose bit set
// open_set is the debug-function-sets that we have to execute
uint64_t debug_print(uint64_t open_set, const char *format, ...)
{
    if((open_set & DEBUG_VERBOSE_SET) == 0X0)
    {
        return 0x1;
    }

    /*======= review the usage of stdarg =======*/
    /*      1th: declare   the args             */
    /*      2th: start     the args             */
    /*      3th: use       the args             */
    /*      4th: clost     the args             */
    /*==========================================*/

    // implement of std print()
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);

    return 0x0;
}