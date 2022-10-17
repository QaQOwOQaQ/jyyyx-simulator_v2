#ifndef CPU_GUARD
#define CPU_GUARD

#include <stdint.h>
#include <stdlib.h>


/*=============================*/
/*          registers          */
/*=============================*/


typedef struct
{
    union // return value
    {
        uint64_t rax;
        uint32_t eax;
        uint16_t ax;
        struct 
        {
            uint8_t al;
            uint8_t ah;
        };
    }; 
    union // callee saved
    {
        uint64_t rbx;
        uint32_t ebx;
        uint16_t bx;
        struct 
        {
            uint8_t bl;
            uint8_t bh;
        };
    };
    union // 4th argument
    {
        uint64_t rcx;
        uint32_t ecx;
        uint16_t cx;
        struct 
        {
            uint8_t cl;
            uint8_t ch;
        };
    };
    union // 3th argument
    {
        uint64_t rdx;
        uint32_t edx;
        uint16_t dx;
        struct 
        {
            uint8_t dl;
            uint8_t dh;
        };
    };
    union // 2th argument
    {
        uint64_t rsi;
        uint32_t esi;
        uint16_t si;
        struct 
        {
            uint8_t sil;
            uint8_t sih;
        };
    };
    union // 1th argument
    {
        uint64_t rdi;
        uint32_t edi;
        uint16_t di;
        struct 
        {
            uint8_t dil;
            uint8_t dih;
        };
    };
    union // callee saved frame pointer
    {
        uint64_t rbp;
        uint32_t ebp;
        uint16_t bp;
        struct 
        {
            uint8_t bpl;
            uint8_t bph;
        };
    };
    union // stack top pointer
    {
        uint64_t rsp;
        uint32_t esp;
        uint16_t sp;
        struct 
        {
            uint8_t spl;
            uint8_t sph;
        };
    };
    union // 5th argument
    {
        uint64_t r8;
        uint32_t r8d;
        uint16_t r8w;
        uint8_t  r8b;
    };
    union // 6th argument
    {
        uint64_t r9;
        uint32_t r9d;
        uint16_t r9w;
        uint8_t  r9b;
    };
    union // caller saved
    {
        uint64_t r10;
        uint32_t r10d;
        uint16_t r10w;
        uint8_t  r10b;
    };
    union // caller saved
    {
        uint64_t r11;
        uint32_t r11d;
        uint16_t r11w;
        uint8_t  r11b;
    };
    union // callee saved
    {
        uint64_t r12;
        uint32_t r12d;
        uint16_t r12w;
        uint8_t  r12b;
    }; 
    union // callee saved 
    {
        uint64_t r13;
        uint32_t r13d;
        uint16_t r13w;
        uint8_t  r13b;
    };
    union // callee saved
    {
        uint64_t r14;
        uint32_t r14d;
        uint16_t r14w;
        uint8_t  r14b;
    };
    union // callee saved
    {
        uint64_t r15;
        uint32_t r15d;
        uint16_t r15w;
        uint8_t  r15b;
    };
} cpu_reg_t;
cpu_reg_t cpu_reg;


/*===================================*/
/*              cpu core             */
/*===================================*/

// condition code flags of most recent (latest) operation
// condition codes will only be set by the following integer arithmetic instructions

/* integer arithmetic instructions
    inc     increment 1
    dec     decrement 1
    neg     negate
    not     complement
    ----------------------------
    add     add
    sub     subtract
    imul    multiply
    xor     exclusive or
    or      or
    and     and
    ----------------------------
    sal     left shift
    shl     left shift (same as sal)
    sar     arithmetic right shift
    shr     logical right shift
*/

/* comparison and test instructions
    cmp     compare
    test    test
*/

// the 4 flags be a uint64_t in total
typedef union {
    /*
        the past implementation is too watseful(using a uint64 to represent a flag)
        we can only use a bit to represent it by making full use of C language characteristics
        which is the strong bit operation

        and the format using structs and unions to represent something is very useful and common
    */
    uint64_t __flags_value;
    
    /*=================================================*/
    /*             | CF | ZF | SF | OF |               */
    /* (low_addr) [0____15___31___47___63] (high_addr) */         
    /*=================================================*/

    struct 
    {
        // carry flag: detect overflow for unsigned operations
        uint16_t CF;
        // zero flag: result is zero ? 1 : 0
        uint16_t ZF;
        // sign flag: result is negatove: 1 : 0 --> highest bit
        uint16_t SF;
        // overflow flag: detect overflow for signed operations
        uint16_t OF;
    };
} cpu_flags_t;
cpu_flags_t cpu_flags;

// program count or instruction pointer
typedef union
{
    uint64_t rip;
    uint64_t eip;
} cpu_pc_t;
cpu_pc_t cpu_pc;

// control registers
typedef struct 
{
    uint64_t cr0;
    uint64_t cr1;
    uint64_t cr2;
    uint64_t cr3;   /* should be a 40-bit PPN for PGD in DRAM
                       but we are using 48-bit virtual address on simulator;s heap
                       by maloc() */
} cpu_cr_t;
cpu_cr_t cpu_controls;

// move to common.h to be shared by linker
// #define MAX_INSTRUCTION_CHAR 64

#define NUM_INSTRTYPE 14

// CPU's instruction cycle: excution of instrctions
void instruction_cycle();

/*----------------------------------*/
// place the functions here because they requires the core_t type


/*----------------------------------*/
// mmu function


// transllate the virtual addres to physical address in MMU
// each MU is owned by each core
uint64_t va2pa(uint64_t vaddr);


// end of include guard
#endif
