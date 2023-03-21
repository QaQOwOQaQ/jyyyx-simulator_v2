// Dynammic Random Access Memory

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <headers/common.h>
#include <headers/cpu.h>
#include <headers/memory.h>
#include <headers/address.h>

uint8_t sram_cache_read(uint64_t paddr);
void sram_cache_write(uint64_t paddr, uint8_t data);

void bus_read_cacheline (uint64_t paddr, uint8_t *block);
void bus_write_cacheline(uint64_t paddr, uint8_t *block);

/*
Be careful with the x86-64 little-endian integer encoding
e.g. write 0x0000-7fd3-57a0-2ae0 to cache, the memory lapping should be: 
    e0 2a a0 57 d3 7f 00 00 
*/


/*=======================================================*/
/*                   by meself:                          */
/*          a address only can store 8 bit               */
/* so we have to use eight address to store a 64bit data */
/*=======================================================*/

// memory accessing used in struction
uint64_t cpu_read64bits_dram(uint64_t paddr)
{
#ifdef DEBUG_ENABLE_SRAM_CACHE
    // try to load uint64_t from SRAM cache
    // little-endian
    uint64_t _val = 0x0;
    for(int i = 0; i < sizeof(uint64_t); i ++ )
    {
        _val += (sram_cache_read(paddr + i) << (i * 8));
    }
    return _val;
#endif
    uint64_t val = 0x0;
    // read from DRAM directly
    // little-endian
    val += (((uint64_t)pm[paddr + 0]) << 0);
    val += (((uint64_t)pm[paddr + 1]) << 8);
    val += (((uint64_t)pm[paddr + 2]) << 16);
    val += (((uint64_t)pm[paddr + 3]) << 24);
    val += (((uint64_t)pm[paddr + 4]) << 32);
    val += (((uint64_t)pm[paddr + 5]) << 40);
    val += (((uint64_t)pm[paddr + 6]) << 48);
    val += (((uint64_t)pm[paddr + 7]) << 56);
    
    return val;
}

void cpu_write64bits_dram(uint64_t paddr, uint64_t data)
{
#ifdef DEBUG_ENABLE_SRAM_CACHE
    // try to write uint64_t to SRAM cache
    // little-endian
    for(int i = 0; i < sizeof(uint64_t); i ++ )
    {
        sram_cache_write(paddr + i, (data >> (i * sizeof(uint64_t))) & 0xff);
    }
    return ;
#endif 

    // write tp DRAM directly
    // little-endian
    pm[paddr + 0] = (data >> 0 ) & 0xff;
    pm[paddr + 1] = (data >> 8 ) & 0xff;
    pm[paddr + 2] = (data >> 16) & 0xff;
    pm[paddr + 3] = (data >> 24) & 0xff;
    pm[paddr + 4] = (data >> 32) & 0xff;
    pm[paddr + 5] = (data >> 40) & 0xff;
    pm[paddr + 6] = (data >> 48) & 0xff;
    pm[paddr + 7] = (data >> 56) & 0xff;   

}

void cpu_readinst_dram(uint64_t paddr, char *buf)
{
    for(int i = 0; i < MAX_INSTRUCTION_CHAR; i ++ )
    {
        buf[i] = (char)pm[paddr + i];
    }
}

void cpu_writeinst_dram(uint64_t paddr, const char *str)
{
    int len = strlen(str);
    assert(len <= MAX_INSTRUCTION_CHAR);
    // in our simulatation, the instruction is fixed length
    for(int i = 0; i < MAX_INSTRUCTION_CHAR; i ++ )
    {
        if(i < len)
        {
            pm[paddr + i] = (uint8_t)str[i];
        }
        else 
        {
            pm[paddr + i] = 0;    
        }
    }
}


/* interface of I/O Bus: read and write from cache between the SRAM cache and DRAM memory
    每次总线(bus)传输我们都传输一个 cache block
*/
void bus_read_cacheline(uint64_t paddr, uint8_t *block)
{
    uint64_t dram_base = (paddr >> SRAM_CACHE_OFFSET_LENGTH) << SRAM_CACHE_OFFSET_LENGTH;       /*
    将物理地址的偏移量部分置为0，即得到这个物理地址所在行的起始物理地址，因为我们每次往 cache 中读取或者写入的单位都是一行数据 */
 
    for(int i = 0; i < (1 << SRAM_CACHE_OFFSET_LENGTH); i ++ ) // block 的大小
    {
       block[i] = pm[dram_base + i];
    }
}

void bus_write_cacheline(uint64_t paddr, uint8_t *block)
{
    uint64_t dram_base = (paddr >> SRAM_CACHE_OFFSET_LENGTH) << SRAM_CACHE_OFFSET_LENGTH;

    for(int i = 0; i < (1 << SRAM_CACHE_OFFSET_LENGTH); i ++ ) 
    {
       pm[dram_base + i] = block[i];
    }
}