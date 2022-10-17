#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <headers/cpu.h>
#include <headers/common.h>
#include <headers/memory.h>
#include <headers/address.h>

// each swap file is swap page
// each line of this swap page is one uint64
/* 怎么将内存中的内容写入swap呢？ 我们这里直接把swap空间当做一个文件，
然后把数据写入文件中。*/
#define SWAP_PAGE_FILE_LINES 512

// disk address counter
//static uint64_t internal_swap_daddr = 0;

int swap_in(uint64_t daddr, uint64_t ppn)
{
    FILE *fr;
    char filename[128];
    sprintf(filename, "../files/swap/swap-%ld.txt", daddr); // make file as swap
    fr = fopen(filename, "r");
    assert(fr != NULL);

    uint64_t ppn_ppo =  ppn << PHYSICAL_PAGE_OFFSET_LENGTH;
    char buf[64] = {0};
    for(int i = 0; i < SWAP_PAGE_FILE_LINES; i ++ )
    { 
        char *str = fgets(buf, 64, fr);
        *(uint64_t *)&(pm[ppn_ppo + i * 8]) = string2uint(str);
    }
    fclose(fr);

    return 0;
}

int swap_out(uint64_t daddr, uint64_t ppn)
{
    FILE *fw;
    char filename[128];
    sprintf(filename, "../files/swap/swap-%ld.txt", daddr); // make file as swap
    fw = fopen(filename, "w");
    assert(fw != NULL);

    uint64_t ppn_ppo =  ppn << PHYSICAL_PAGE_OFFSET_LENGTH;
    for(int i = 0; i < SWAP_PAGE_FILE_LINES; i ++ )
    {
        /* uint64 -> 8 byte -> 16 bit(hex) -> %16lx
           因为 pm 是 uint8，所以我们要取得 pm 的地址将它转化成一个指针
            然后通过给指针做类型转换(uint64_t *)就可以让这个指针以uint64的方式解引用
            就可以得到一个 uint64 的值了
        */        
        fprintf(fw, "0x%16lx", *(uint64_t *)&(pm[ppn_ppo + i * 8]));
    }
    fclose(fw);

    return 0;
}