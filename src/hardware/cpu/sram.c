#include <stdint.h>
#include <headers/address.h>

#define NUM_CACHE_LINE_PER_SET (8)  // cache 中每个组的 line count

/*++++++++++++++ define cache struct start +++++++++++++*/

/*=========================*/
/*| vallid | clean | dirty |*/
/*=========================*/
typedef enum // cache 行中的状态信息
{
    CACHE_LINE_INVALID,
    CACHE_LINE_CLEAN,
    CACHE_LINE_DIRTY    // 我们的定义的是写回策略的cache 
} sram_cacheline_state;


/*=======================*/
/*| state | tag | block |*/
/*=======================*/
typedef struct // cache 行
{
    sram_cacheline_state state;
    uint64_t tag;
    uint64_t block[(1 << SRAM_CACHE_OFFSET_LENGTH)];
} sram_cacheline_t;


typedef struct // cache 组
{
    sram_cacheline_t lines[NUM_CACHE_LINE_PER_SET];
} sram_cacheset_t;

typedef struct // cache
{
    sram_cacheset_t sets[(1 << SRAM_CACHE_INDEX_LENGTH)];
} sram_cache_t;

/*++++++++++++++ define cache struct end +++++++++++++*/

static sram_cache_t cache;

/* ++++++++++++++ interface ++++++++++++++*/
uint8_t sram_cache_read(address_t paddr)
{
    sram_cacheset_t set = cache.sets[paddr.CI];
    for(int i = 0; i < NUM_CACHE_LINE_PER_SET;  i ++ )
    {
        sram_cacheline_t line = set.lines[i];
        
        if(line.state != CACHE_LINE_INVALID && line.tag == paddr.CT)
        {
            // cache hit
            // TODO: update LRY
            return line.block[paddr.address_value];
        }
    }

    // cache miss:  load from memory
    // TODO: update LRU
    // TODO: select one victim by replacement policy if set if full 

    return 0;
}

void sram_cache_write(address_t paddr, uint8_t data)
{
    return ;
}