#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <headers/address.h>
#include <headers/memory.h>

#define NUM_CACHE_LINE_PER_SET (8)  // cache 中每个组的 line count


uint8_t sram_cache_read(uint64_t paddr);
void sram_cache_write(uint64_t paddr, uint8_t data);

void bus_read_cacheline (uint64_t paddr, uint8_t *block);
void bus_write_cacheline(uint64_t paddr, uint8_t *block);


/* ========================  cache write policy  ============================
For problem：CPU 修改了 cache 中数据的副本，如何保证主存中数据母本的一致性 -- cache write policy

Two cases:
1.write hit：  
    1.1 write-through：同时写入到 Cache 和内存
    1.2 write_back   ：修改 Cache 中的数据，并不写入内存。只有当数据要被 replace 时才根据脏位判断是否需要通过总线(bus)写入内存
2.write miss：
    1.1 write allocate：将数据从主存调入Cache，然后在对 Cache 进行写，最后用写回法将 Cache 中的数据写回主存
    1.2 write no allocate：CPU 直接写入主存，不与 Cache 交互

3. scheme （方案, 计划, 格式, 制度）
其实上面的写命中和写不命中根据两种思想分别划分了两种方案：
    1. 直接与内存同步数据（direct）
    2. 延迟同步（lazy 思想）
所以说，我们可以根据这两种思想，搭配写命中和写不命中（写命中和写不命中的思想肯定要一致，不然就四不像了）
写穿搭配写不分配：在写不分配中，我们是希望不把数据直接写入内存的，因此先写入 Cache 观望一下，
                此时如果搭配写穿，那和写分配有啥区别（绕了一圈还是必须写入内存），所以说要搭配写回
写回搭配写分配  
[参考链接](https://blog.csdn.net/qq_41587740/article/details/109104962)

4. cache read
if hit, read cache 
else if miss，read memory and  write cache
==========================================================================*/

/*++++++++++++++ define cache struct start +++++++++++++*/

/*==========================*/
/*| vallid | clean | dirty |*/
/*==========================*/

// write-back and write-allocate
typedef enum // cache 行中的状态信息
{
    CACHE_LINE_INVALID, // 设置为 invalid 更好判断
    CACHE_LINE_CLEAN,   // In MESI: E, S
    CACHE_LINE_DIRTY   
} sram_cacheline_state_t;

/*+---------------------+*/
/*| state | tag | block |*/
/*+---------------------+*/
typedef struct // cache 行
{
    sram_cacheline_state_t state;
    int time;      // timer to find LRU line inside one set 
    uint64_t tag;
    uint8_t block[(1 << SRAM_CACHE_OFFSET_LENGTH)];
} sram_cacheline_t;


typedef struct // cache 组
{
    sram_cacheline_t lines[NUM_CACHE_LINE_PER_SET];
} sram_cacheset_t;

typedef struct // cache
{
    sram_cacheset_t sets[(1 << SRAM_CACHE_INDEX_LENGTH)];
} sram_cache_t;

static sram_cache_t cache;
/*++++++++++++++ define cache struct end +++++++++++++*/


/* ++++++++++++++ interface ++++++++++++++*/
/* LRU 替换思路：
    每次 read 之前，将 cache 中所有 lien 的计数器(time) +1，然后将读取的元素的计数器置为 0
    这样计数器最大的那个 line 就是最长时间没用到的 line 
*/
uint8_t sram_cache_read(uint64_t paddr_value)
{
    address_t paddr = {
        .paddr_value = paddr_value,
    };  // NB，这个初始化

    sram_cacheset_t *set = &(cache.sets[paddr.ci]); // 得到这个物理地址所在的 st

    // update LRU time
    // 这部分相当于预处理，找到 invalid 的行和最久没使用的行方便后面 miss 时更新
    sram_cacheline_t *victim = NULL; // 将被置换的行称为受害者(victim)，太形象了
    sram_cacheline_t *invalid = NULL;
    int max_time = -1;
    // 暴力循环更新，将所有行的计数器 + 1
    for(int i = 0; i < NUM_CACHE_LINE_PER_SET; i ++ )  // traver line 
    {   
        sram_cacheline_t *line = &(set->lines[i]);
        line->time ++ ;      
        if(max_time < line->time)
        {
            // select this line as victim by LRU policy
            // replace it with all lines are valid
            victim = line;
            max_time = line->time;
        }

        if(line->state == CACHE_LINE_INVALID)
        {
            // exits one invalid line as candidate for cache miss
            invalid = line;
        }
    }

    // try cache hit
    for(int i = 0; i < NUM_CACHE_LINE_PER_SET;  i ++ )
    {
        sram_cacheline_t line = set->lines[i];
        
        if(line.state != CACHE_LINE_INVALID && line.tag == paddr.ct)
        {
            // cache hit
            // find the byte    
            line.time = 0;

            return line.block[paddr.co];
        }
    }

    // cache miss:  load from memory
    
    // try to find one free cache line
    if(invalid != NULL)  // 优先使用未使用的块
    {
        // load date from DRAM to this invalid cache line
        bus_read_cacheline(paddr.address_value, &(invalid->block[0])); // bug: 这里第二个参数我们应该传入一个指针，但是我们传入了一个数组

        // update cache line state
        invalid->state = CACHE_LINE_CLEAN;

        // uodate LRU
        invalid->time = 0;

        // update tag
        invalid->tag = paddr.ct;

        return invalid->block[paddr.co];
    }

    // 只要执行到这里，victim一定是非空的，因为如果victim真的为空
    // 说明此时是冷启动状态，不存在任何 line
    // 但此时一定存在 invalid，所以绝对不会执行到这里
    assert(victim != NULL);
    // no free cache line, use LRU policy
    // 注意替换出去的 line 是否是 dirty 的
    if(victim->state == CACHE_LINE_DIRTY)
    {                            
        // write back the dirty line to dram
        bus_write_cacheline(paddr.paddr_value, victim);
    } 
    // update state
    // 此时该 line 属于未使用状态
    victim->state = CACHE_LINE_INVALID; 
    
    // read from dram
    // load date from DRAM to this invalid cache line
    // 现在该 line 被新数据占用了
    bus_read_cacheline(paddr.address_value, &(victim->block[0]));

    // update cache line state
    victim->state = CACHE_LINE_CLEAN;

    // uodate LRU
    victim->time = 0;

    // update tag
    victim->tag = paddr.ct;

    return victim->block[paddr.co];
}



void sram_cache_write(uint64_t paddr_value, uint8_t data)
{
    address_t paddr = {
        .paddr_value = paddr_value, // 将物理地址转换成 address_t 类型方便我们分隔offset，tag和index，
            // 实际上通过掩码也可以实现，不过不方便
    }; 

    sram_cacheset_t *set = &(cache.sets[paddr.ci]); // 只想该物理地址在 cache 中对应的 set

    // 更新 LRU time 并找到 invalid 的行(没使用的行)和最久未使用的行
    sram_cacheline_t *victim = NULL;
    sram_cacheline_t *invalid = NULL;
    int max_time = -1;

    for(int i = 0; i < NUM_CACHE_LINE_PER_SET; i ++ ) // 遍历该 set 中的所有行
    {
        sram_cacheline_t *line = &(set->lines[i]); // 取得行
        line->time ++ ;

        if(line->state == CACHE_LINE_INVALID) // 未使用
        {
            invalid = line;
        }

        if(max_time < line->time)
        {
            victim = line;
            max_time = line->time;
        }
    }

    // 查找 cache
    // 其实这一步是可以在上面的 for 里面做的，作者之所以这么写是为了让上面的"状态机"更情况
    for(int i = 0; i < NUM_CACHE_LINE_PER_SET; i ++ )
    {   
        sram_cacheline_t *line = &(set->lines[i]);

        // cache hit，写回
        if(line->state != CACHE_LINE_INVALID && line->tag == paddr.ct) 
        {
            // 更新 LRU time
            line->time = 0;

            // 将数据写入 cache
            line->block[paddr.co] = data;

            // 脏数据，更新 state
            line->state = CACHE_LINE_DIRTY;

            return ;
        }
    }

    // cache miss，写分配，找到一个 invalid 行或者 victim
    if(invalid != NULL) // 由 invalid，没使用的行
    {
        bus_read_cacheline(paddr.paddr_value, &(invalid->block)); // 将内存地址中的数据读入行的block

        invalid->state = CACHE_LINE_DIRTY; // 在 cache 中分配一行之后在 cache 中写，再根据写回法写入内存，所以并不一定写入内存，因此是脏数据 
        invalid->tag = paddr.ct;
        invalid->block[paddr.co] = data;

        return ;
    }

    // 一定存在 victim，除非你 cache 中一个 set 只有 0 行
    assert(victim != NULL);

    // 如果需要将一行从 cache 中移出，需要检查是否为脏数据
    if(victim->state == CACHE_LINE_DIRTY) // 是脏数据，写入内存
    {
        bus_write_cacheline(paddr.paddr_value, victim);
    }
    victim->state = CACHE_LINE_CLEAN;
    
    // 将数据写入 cache
    bus_read_cacheline(paddr.address_value, &(victim->block));
    victim->state = CACHE_LINE_DIRTY;
    victim->time = 0;
    victim->block[paddr.co] = data;
}   


