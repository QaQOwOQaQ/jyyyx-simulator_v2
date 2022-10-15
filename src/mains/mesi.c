/* MESI Protocol */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

typedef enum // MESI state
{
    MODIFIED,   // exclusive dirtym, global = 1
    EXCLUSIVE,  // exclusive clean,  global = 2
    SHARED,     // shared  clean,    global >= 2   
    INVALID     // invalid,       
} state_t;

typedef struct // cache line
{
    state_t state;
    int value;
} line_t;


#define NUM_PROCESSOR (2048) // count of processor core

line_t cache[NUM_PROCESSOR]; //不同处理器上每个处理器维护的cache_line

int mem_value = 256; // memory value

int check_state()
{
    int m_count = 0;
    int e_count = 0;
    int s_count = 0;
    int i_count = 0;

    for(int i = 0; i < NUM_PROCESSOR; i ++ )
    {
        if(cache[i].state == MODIFIED)
        {
            m_count += 1;
        }
        else if(cache[i].state == EXCLUSIVE)
        {
            e_count += 1;
        }
        else if(cache[i].state == SHARED)
        {
            s_count += 1;
        }
        else if(cache[i].state == INVALID)
        {
            i_count ++ ;
        }
    }
    
    /*+-------------------+*/
    /*|   | M | E | S | I |*/
    /*+-------------------+*/
    /*| M | × | × | × | √ |*/
    /*+-------------------+*/
    /*| E | × | × | × | √ |*/
    /*+-------------------+*/
    /*| S | × | × | √ | √ |*/
    /*+-------------------+*/
    /*| I | √ | √ | √ | √ +*/
    /*+-------------------+*/
    
    #ifdef DEBUG
    printf("M: %d\t E: %d\t S: %d\t I: %d\t\n", m_count, e_count, s_count, i_count);
    #endif

    // valid
    if((m_count == 1 && i_count == (NUM_PROCESSOR - 1)) || 
       (e_count == 1 && i_count == (NUM_PROCESSOR - 1)) ||
       (s_count >= 2 && i_count == (NUM_PROCESSOR - s_count)) || 
       (i_count == NUM_PROCESSOR) )
    {
        return 1; 
    }

    // invalid
    return 0;   
}

// i - the index of precessor core
// read_value - the address of read value
// int return -- if this event is related with target cache line （判断这个地址是否和我们有关，因为cache line可能加载不同的物理地址
int read_cacheline(int i, int *read_value)
{
    if(cache[i].state == MODIFIED)
    {
        // read hit
        #ifdef DEBUG
        printf("[%d] read hit; exclusive dirty value %d\n", i, cache[i].value);
        #endif

        *read_value = cache[i].value;
        return 1;
    }
    else if(cache[i].state == EXCLUSIVE)
    {
        // read hit
        #ifdef DEBUG
        printf("[%d] read hit; exclusive clean value %d\n", i, cache[i].value);
        #endif
        
        *read_value = cache[i].value;
        return 1;
    }
    else if(cache[i].state == SHARED)
    {
        // read hit
        #ifdef DEBUG
        printf("[%d] read hit; shared clean value %d\n", i, cache[i].value);
        #endif

        *read_value = cache[i].value;

        return 1;  
    }
    else // cahe[i].state == invalid;
    {
        // read miss
        // bus boardcase read miss(sniff(嗅探)算法，类似计网)
        // read value from other processor
        for(int j = 0; j < NUM_PROCESSOR; j ++ ) // 找到一个有数据的核心共享数据
        {
            if(i == j)  
            {
                continue;
            }
            if(cache[j].state == MODIFIED)
            {
                // write back
                // because we need to change core[j]'s state from modified to shared
                // so we must make sure the value is clean
                mem_value = cache[j].value;
                cache[j].state = SHARED;

                // update read miss cache
                cache[i].state = SHARED;
                cache[i].value = cache[j].value;

                *read_value = cache[i].value;

                #ifdef DEBUG
                printf("[%d] read miss; [%d] supplied dirty value %d; write back\n", i, j, cache[j].value);
                #endif

                return 1;
            }
            else if(cache[j].state == EXCLUSIVE)
            {
                cache[i].state = SHARED;
                cache[i].value = cache[j].value;

                // there are exactly 2 copied in processor
                cache[j].state = SHARED;

                *read_value = cache[i].value;
                
                #ifdef DEBUG
                printf("[%d] read miss; [%d] suuplies exclusive clean value %d; s_count == 2;\n", i, j, cache[j].value);
                #endif
                
                return 1;
            }
            else if(cache[j].state == SHARED)
            {
                // s_count >= 3
                cache[i].state = SHARED;
                cache[i].value = cache[j].value;
                
                *read_value = cache[i].value;

                #ifdef DEBUG
                printf("[%d] read miss; [%d] supplies shared clean value %d; s_count >= 3;\n", i, j, cache[j].value);
                #endif

                return 1;
            }
        }

        // all other are invalid
        // read from dram
        cache[i].value = mem_value;
        cache[i].state = EXCLUSIVE;
        #ifdef DEBUG
        printf("[%d] read miss; memory supplies clean value %d; e_count == 1;\n", i, mem_value);
        #endif

        *read_value = cache[i].value;
        
        return 1;
    }

    // shouldn't go here
    return 0;
}

// i - the index of precessor core
// write_value - the value to be written the pyhsical address
// int return -- if this event is related with target physical address
int write_cacheline(int i, int write_value)
{
    if(cache[i].state == MODIFIED)
    {
        // global only have one exclusive modified
        // write hit
        cache[i].value = write_value;

        #ifdef DEBUG
        printf("[%d] write hit; update to value %d;\n", i, cache[i].value);
        #endif 

        return 1;
    }
    else if(cache[i].state == EXCLUSIVE)
    {
        // global only have one exclusive clean
        // write hit
        cache[i].value = write_value;
        cache[i].state == MODIFIED;

        #ifdef DEBUG
        printf("[%d] write hit; update to value %d;\n", i, cache[i].value);
        #endif 

        return 1;
    }
    else if(cache[i].state == SHARED)
    {
        // boardcast write invalid
        // 只要 write，肯定就存在一个 modified，那么就肯定不能存在 shared
        for(int j = 0; j < NUM_PROCESSOR; j ++ )
        {
            if(i == j)
            {
                continue;
            }
        
            // because of shared, so all the cache which shared have the same value, so do not need write back
            /* and all the shared cache have the same copy, now cache[i] which is shared have a new data
               because other shared cache's data is not the copy from cache[i] now, so they are valid */
            cache[j].state = INVALID;
            cache[j].value = 0;
        }

        cache[i].state = MODIFIED;
        cache[i].value = write_value;

        #ifdef DEBUG
        printf("[%d] write hit; boardcast invalid; update to value %d;\n", i, cache[i].value);
        #endif
    }
    else if(cache[i].state == INVALID)
    {
        for(int j = 0; j < NUM_PROCESSOR; j ++ )
        {
            if(i == j)
            {
                continue;
            }
            if(cache[j].state == MODIFIED)
            {
                // write back
                /* 为什么这里要写回呢？既然数据已经修改了，那么原来的数据不就没用了吗？
                 * 确实是这样。
                 * 作者的解释：“mesi协议并不是图方便的协议，它有一些物理硬件上的设计，
                 * 这些设计导致我们不能直接把 cache[j] “报废掉”，而是必须先把它的数据写回内存”。
                 * 总了，就是出于一致性方面的考虑。作者也记不清了
                */
                mem_value = cache[j].value;
                
                // invalid cache[j]
                cache[j].state = INVALID;
                cache[j].value = 0;

                // write allocate
                cache[i].value = mem_value;

                // update cache[i] to modified
                cache[i].state = MODIFIED;
                cache[i].value = write_value;

                #ifdef DEBUG
                printf("[%d] write miss; boardcast invalid to M; update to value %d;\n", i, cache[i].value);
                #endif

                return 1;
            }
            else if(cache[j].state == EXCLUSIVE)
            {
                // clean value - not need write back
                cache[j].state = INVALID;
                cache[j].value = 0;

                // update cache[i]
                cache[i].state = MODIFIED;
                cache[i].value = write_value;

                #ifdef DEBUG
                printf("[%d] write miss; boardcast invalid to E; update to value %d;\n", i, cache[i].value);
                #endif

                return 1;
            }
            else if(cache[j].state == SHARED)
            {
                // clean value - not need to write back
                for(int k = 0; k < NUM_PROCESSOR; k ++ )
                {
                    if(i == k)
                    {
                        continue;
                    }
                    cache[k].state = INVALID;
                    cache[k].value = 0;
                }

                cache[i].state = MODIFIED;
                cache[i].value = write_value;

                #ifdef DEBUG
                printf("[%d] write miss; boardcast invalid to S; update to value %d;\n", i, cache[i].value);
                #endif

                return 1;
            }

        }

        // all other is invalid
        // write allocate
        cache[i].value = mem_value;
        cache[i].state = MODIFIED;
        
        // update
        cache[i].value = write_value;
        
        #ifdef DEBUG
        printf("[%d] write miss; all invalid; update to value %d;\n", i, cache[i].value);
        #endif

        return 1;
    }

    // invalid case
    return 0;
}

// i - the index of precessor core
// int return -- if this event is related with target physical address
int evict_cacheline(int i) // evict: 驱逐
{
    if(cache[i].state == MODIFIED)
    {
        // write back
        mem_value = cache[i].value;

        // update old cache
        cache[i].state = INVALID;
        cache[i].value = 0;

        #ifdef DEBUG
        printf("[%d] evict; write back value %d\n", i, cache[i].value);
        #endif

        return 1;
    }
    else if(cache[i].state == EXCLUSIVE)
    {
        // clean value
        cache[i].state = INVALID;
        cache[i].value = 0;

        #ifdef DEBUG
        printf("[%d] evict;\n", i);
        #endif

        return 1;
    }
    else if(cache[i].state == SHARED)
    {
        // clean value
        cache[i].state = INVALID;
        cache[i].value = 0;
        
        // may left only one shared to be exclusive
        int s_count = 0;
        int last_s = -1;
        for(int j = 0; j < NUM_PROCESSOR; j ++ )
        {
            if(cache[j].state == SHARED)
            {
                s_count ++ ;
                last_s = j;
            }
        }
        if(s_count == 1)
        {
            cache[last_s].state = EXCLUSIVE;
        }

        
        #ifdef DEBUG
        printf("[%d] evict;\n", i);
        #endif

        return 1;        
    }
    else if(cache[i].state == INVALID)
    {
        // do nothing
        
        #ifdef DEBUG
        printf("evict a invalid cache;\n");
        #endif
       
        return 1;
    }
    // evict when cache line is Invalid 
    // not related with target physical addree
    return 0;
}

void print_cacheline()
{
    for(int i = 0; i < NUM_PROCESSOR; i ++ )
    {
        char c;
        switch (cache[i].state)
        {
            case MODIFIED:
                c = 'M';
                break;
            case EXCLUSIVE:
                c = 'E';
                break;
            case SHARED:
                c = 'S';               
                break;
            case INVALID:
                c = 'I';
                break;
            default:
                c = '?';
                break;
        }
        printf("\t[%d]          state %c        value %d\n", i, c, cache[i].value);
    }
    printf("\t                                memory value %d\n", mem_value);
}

int main()
{
    srand(time(NULL));

    int read_value;
    for(int i = 0; i < NUM_PROCESSOR; i ++ )
    {
        cache[i].value = 0; 
        cache[i].state = INVALID; // 不初始化的话默认state=0=mdified
    }

    #ifdef DEBUG
    print_cacheline();
    #endif

    for(int i = 0; i < 100000; i ++ )
    {
        int do_print = 0;
    
        int core_index = rand() % NUM_PROCESSOR;
        int op = rand() % 3; // read / write / evict
        
        if(op == 0)
        {
            #ifdef DEBUG
            printf("[[%d]]read: [%d]\n", i, core_index);
            #endif
            do_print = read_cacheline(core_index, &read_value);
        }
        else if(op == 1)
        {
            #ifdef DEBUG
            printf("[[%d]]write: [%d]\n", i, core_index);
            #endif
            do_print = write_cacheline(core_index, rand() % 1024);
        }
        else // op == 2
        {
            #ifdef DEBUG
            printf("[[%d]]evict: [%d]\n", i, core_index);
            #endif
            do_print = evict_cacheline(core_index);
        }

        #ifdef DEBUG
        if(do_print)
        {
            print_cacheline();
        }
        #endif
        
        if(check_state() == 0)
        {
            printf("faliled to check MESI state!\n");
            exit(0);
        }
        
        #ifdef DEBUG
        printf("\n");
        #endif
    }

    printf("pass!\n");

    return 0;
}


/* debug log
真是逆天，把该犯的错误都犯了属于是
1. if 判断中把 == 写为 = （顶级逆天）
2. 打混变量名
3. 忘记加 return 导致程序执行了不该执行的内容 
*/