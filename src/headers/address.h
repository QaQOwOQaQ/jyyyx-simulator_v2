#ifndef ADDRESS_GUARD
#define ADDRESS_GUARD

#include <stdint.h>
#define SRAM_CACHE_TAG_LENGTH    (40)
#define SRAM_CACHE_INDEX_LENGTH  (6)
#define SRAM_CACHE_OFFSET_LENGTH (6)

#define PHYSICAL_PAGE_OFFSET_LENGTH (12)
#define PHYSICAL_PAGE_NUMBER_LENGTH (40)
#define PHYSICAL_ADDRESS_LENGTH     (52)

/* |============|=============|==============| */
/* | cache tag  | cache index | cache offset | */
/* |============|=============|==============| */
/*因为cache是组相连映射，所在index用来找到在cache中的那个组，
因为可以匹配任意一行，所以只能类似for循环暴力查找位于哪一行，
在每一行匹配valid是否有效，并且tag是否相同。tag就是用来让我们知道这个数据是否匹配的
offset用来找块偏移，（其实就是字节偏移？）*/

typedef union
{
    uint64_t address_value;
    
    // physical address : 52
    struct 
    {
        union 
        {
            uint64_t paddr_value : PHYSICAL_ADDRESS_LENGTH;
            struct 
            {
                uint64_t PPO : PHYSICAL_PAGE_OFFSET_LENGTH;
                uint64_t PPN : PHYSICAL_PAGE_NUMBER_LENGTH;    
            }; 
        };
        
    };
    struct 
    {
        uint64_t CO : SRAM_CACHE_OFFSET_LENGTH;
        uint64_t CI : SRAM_CACHE_INDEX_LENGTH;
        uint64_t CT : SRAM_CACHE_TAG_LENGTH;
    };
} address_t;



#endif 