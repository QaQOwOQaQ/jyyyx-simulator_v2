// Instruction Set Architecture
// A define between hardware and software

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/common.h"
#include "headers/memory.h"
#include "headers/common.h"

/*==========================================*/
/*      Instruction Set Architecture        */
/*==========================================*/

// data structions
typedef enum INST_OPERATOR
{
    INST_MOV,       // 0
    INST_PUSH,      // 1
    INST_POP,       // 2
    INST_LEAVE,     // 3
    INST_CALL,      // 4
    INST_RET,       // 5
    INST_ADD,       // 6
    INST_SUB,       // 7
    INST_CMP,       // 8
    INST_JNE,       // 9
    INST_JMP,       // 10
} op_t;

typedef enum OPERAND_TYPE
{
    EMPTY,                  // 0
    IMM,                    // 1
    REG,                    // 2
    MEM_IMM,                // 3
    MEM_REG1,               // 4
    MEM_IMM_REG1,           // 5
    MEM_REG1_REG2,          // 6
    MEM_IMM_REG1_REG2,      // 7
    MEM_REG2_SCAL,          // 8
    MEM_IMM_REG2_SCAL,      // 9
    MEM_REG1_REG2_SCAL,     // 10
    MEM_IMM_REG1_REG2_SCAL, // 11
} od_type_t;

typedef struct OPERAND_STRUCT
{
    od_type_t  type;    // IMM, REG, MEM
    uint64_t   imm;     // immediate number
    uint64_t   scal;    // scale number(1,2,4,8) to register 2
    uint64_t   reg1;    // main register
    uint64_t   reg2;    // register 2
} od_t;

// local variables are allcated in stack in run-time
// we don't consider local STATIC variables
// ref: Computer System: A Programmer's Perspective 3rd
// Chapter 7 Linking: 7.5 Symbols and symbol Tables
typedef struct INST_STRUCT
{
    op_t op;
    od_t src;
    od_t dst;
} inst_t;


/*====================================*/
/*      pase assembly instruction     */
/*====================================*/

// function to map the string assembly code to inst_t instance 
static void     parse_instruction(const char *str, inst_t *inst, core_t *cr);
static void     parse_operand   (const char *str, od_t   *od,   core_t *cr);
static uint64_t decode_operand  (od_t *od);


// interpret the operand 
// the return val is a address whaterver type is IMM, REG or MEM
static uint64_t decode_operand(od_t *od)
{
    if(od->type == IMM)
    {
        // immediate signed number can be negative: convert to bitmap
        return *(uint64_t *)&od->imm;
    }
    else if(od->type == REG)
    {
        // dedault register 1
        return od->reg1;
    }
    else
    {
        // access memory: return the physical address
        uint64_t vaddr = 0; // default to EMPTY

        if(od->type == MEM_IMM)
        {
            vaddr = od->imm;
        }
        else if(od->type == MEM_REG1)
        {
            vaddr = *(uint64_t *)od->reg1;
        }
        else if(od->type == MEM_IMM_REG1)
        {
            vaddr = od->imm + (*(uint64_t *)od->reg1);
        }
        else if(od->type == MEM_REG1_REG2)
        {
            vaddr = (*(uint64_t *)od->reg1) + (*(uint64_t *)od->reg2);
        }
        else if(od->type == MEM_IMM_REG1_REG2)
        {
            vaddr = od->imm + (*(uint64_t *)od->reg1) + (*(uint64_t *)od->reg2);
        }
        else if(od->type == MEM_REG2_SCAL)
        {
            vaddr = (*(uint64_t *)od->reg2) * od->scal;
        }
        else if(od->type == MEM_IMM_REG2_SCAL)
        {
            vaddr = od->imm + (*(uint64_t *)od->reg2) * od->scal;
        }
        else if(od->type == MEM_REG1_REG2_SCAL)
        {
            vaddr = (*(uint64_t *)od->reg1) + (*(uint64_t *)od->reg2) * od->scal;
        }
        else if(od->type == MEM_IMM_REG1_REG2_SCAL)
        {
            vaddr = od->imm + (*(uint64_t *)od->reg1) + (*(uint64_t *)od->reg2) * od->scal;
        }
        return vaddr;
    }

    // empty
    return 0;
}


static void parse_instruction(const char *str, inst_t *inst, core_t *cr)
{

}

static void pase_operand(const char *str, op_t *od, core_t *cr)
{

}



/*================================*/
/*      instruction handlers      */
/*================================*/

// instruction (sub)set
// in this simulator , the instruction have been decoded and fetched
// so there will be no page fault during fetching
// overwise the instructions must handle the page fault (swap from disk) fist
// and then re-fetch the instruction and do decoding
// and finally re-run the instruction

static void mov_handler     (od_t *src_od, od_t *dst_od, core_t *cr);
static void push_handler    (od_t *src_od, od_t *dst_od, core_t *cr);
static void pop_handler     (od_t *src_od, od_t *dst_od, core_t *cr);
static void leave_handler   (od_t *src_od, od_t *dst_od, core_t *cr); 
static void call_handler    (od_t *src_od, od_t *dst_od, core_t *cr);
static void ret_handler     (od_t *src_od, od_t *dst_od, core_t *cr);
static void add_handler     (od_t *src_od, od_t *dst_od, core_t *cr);
static void sub_handler     (od_t *src_od, od_t *dst_od, core_t *cr);
static void cmp_handler     (od_t *src_od, od_t *dst_od, core_t *cr);   
static void jne_handler     (od_t *src_od, od_t *dst_od, core_t *cr);
static void jmp_handler     (od_t *src_od, od_t *dst_od, core_t *cr);

// handler table storing the handlers to different instruction types
typedef void (*handler_t)(od_t *, od_t *, core_t *);
// look-up table of pointers to function
static handler_t handler_table[NUM_INSTRTYPE] = {
    &mov_handler,       // 0
    &push_handler,      // 1
    &pop_handler,       // 2
    &leave_handler,     // 3
    &call_handler,      // 4
    &ret_handler,       // 5
    &add_handler,       // 6
    &sub_handler,       // 7
    &cmp_handler,       // 8
    &jne_handler,       // 9
    &jmp_handler,       // 10
};

// reset the condition flags
// inline to reduce cost
static inline void reset_cflags(core_t *cr)
{
    cr->CF = 0;
    cr->ZF = 0;
    cr->OF = 0;
    cr->SF = 0;
}

// update the rip pointer to the next instruction sequentially
static inline void next_rip(core_t *cr)
{
    // we are handling the fixed-length of assembly string here
    // but there size can be variable as true x86 instructions 
    // that's because the operand's sizes follow the specific encoding rule
    // the risc-v is a fixed length ISA
    cr->rip = cr->rip + sizeof(char) * MAX_INSTRUCTION_CHAR;
}

// instruction handlers
// the following src and dst is an addr
static void mov_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{
    uint64_t src = decode_operand(src_od);
    uint64_t dst = decode_operand(dst_od);

    if(src_od->type == REG && dst_od->type == REG)
    {
        // src: register
        // dst: register
        *(uint64_t *)dst = *(uint64_t *)src;
        next_rip(cr);
        reset_cflags(cr);
        return ;
    }
    else if(src_od->type == REG && dst_od->type >= MEM_IMM)
    {
        // src: register
        // dst: virtual address
        write64bits_dram(va2pa(dst, cr), *(uint64_t *)src, cr);
        next_rip(cr);
        reset_cflags(cr);
        return ;
    }
    else if(src_od->type >= IMM && dst_od->type == REG)
    {
        // src: virtual address
        // dst: register
        *(uint64_t *)dst = read64bits_dram(va2pa(src, cr), cr);
        next_rip(cr);
        reset_cflags(cr);
        return ;
    }
}

static void push_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{
    uint64_t src = decode_operand(src_od);
    if(src_od->type == REG)
    {
        // src: register
        // dst: empty
        (cr->reg).rsp = (cr->reg).rsp - 8;
        write64bits_ram(va2pa(cr->reg.rsp, cr), *(uint64_t *)src, cr);
        next_rip(cr);
        reset_cflags(cr);
    }
}

static void pop_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{
    uint64_t src = decode_operand(src_od);

    if(src_od->type == REG)
    {
        // src: register
        // dst: empty
        uint64_t old_val = read64bits_dram(va2pa2((cr->reg).rsp), cr);
        (cr->reg).rsp = (cr->reg).rsp + 8;
        next_rip(cr);
        reset_cflags(cr);
    }
}

static void leave_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{

}

static void call_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{
    uint64_t src = decode_operand(src_od);

    // src: imediate number: virtual address of target function starting
    // dst: empty
    
    // push the return value
    (cr->reg).rsp = (cr->reg).rsp - 8;
    write64bits_dram(va2pa((cr->reg).rsp, cr), cr->rip + sizeof(char) * MAX_INSTRUCTION_CHAR, cr);

    // jump to target function address
    cr->rip = src;
    reset_cflags(cr);
}

static void ret_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{
    // src: empty
    // dst: empty
    
    // pop rsp
    uint64_t ret_addr = read64bits_dram(va2pa((cr->reg).rsp, cr), cr);
    cr->rip = ret_addr;
    reset_cflags(cr);
}

static void add_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{
    uint64_t src = decode_operand(src_od);
    uint64_t dst = decode_operand(dst_od);
    if(src_od->type == REG && dst_od->type == REG)
    {
        // src: register (value: int64_t bit map)
        // dst: register (value: int64_t bit map)
        uint64_t val = *(uint64_t *)src + *(uint64_t *)src;

        // set condition flags

        // update registers
        *(uint64_t *)dst = val;
        // signede and unsigned value follow the same addition. e.g.
        next_rip(cr);
        return ;
    }
}   

static void sub_handler(od_t *src_od, od_t *dst_od, core_t * cr)
{

}

static void cmp_handler(od_t *src_od, od_t *dst_od, core_t * cr)
{

}

static void jne_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{

}

static void jmp_handler(od_t *src_od, od_t *dst_od, core_t *cr)
{

}


// instruction cycle is implemented in CPU
// the only exposed interface outside CPU
void instruction_cycle(core_t *cr)
{
    /*
        fetch inst --> decode --> get operands --> execute --> write back
    */

    // FETCH: get the instruction string by program counter
    const char *inst_str = (const char *)cr->rip;
    debug_print(DEBUG_INSTRUCTIONCYCLE, "%lx    %s\n", cr->rip, inst_str);

    // DECODE: decode the rum-time instruction operands
    inst_t inst;
    parse_instruction(inst_str, &inst, cr);

    // EXCUTE: get the function pointer or handler by the operator
    handler_t handler = handler_table[inst.op];
    handler(&(inst.src), &(inst.dst), cr);
}

void print_register(core_t *cr)
{
    if(DEBUG_VERBOSE_SET & DEBUG_REGISTERS == 0X0)
    {
        return ;
    }

    reg_t reg = cr->reg;

    printf("rax = 0x%-16lx\nrbx = 0x%-16lx\nrcx = 0x%-16lx\nrdx = 0x%-16lx\n", reg.rax, reg.rbx, reg.rcx, reg.rdx);
    printf("rsi = 0x%-16lx\nrdi = 0x%-16lx\nrbp = 0x%-16lx\nrsp = 0x%-16lx\n", reg.rsi, reg.rdi, reg.rbp, reg.rsp);
    printf("rip = 0x%-16lx\n", cr->rip);
    printf("CF = %u\tZF = %u\tSF = %u\tOF = %u\n", cr->CF, cr->ZF, cr->SF, cr->OF);
}

void print_stack(core_t *cr)
{
    if(DEBUG_VERBOSE_SET & DEBUG_PRINTSTACK == 0x0)
    {
        return ;
    }
    int n = 10;
    // let a pointer named high point to a physical address of rsp 
    // what we see is the virtual address, but the actual value is store in the physical address
    // so when have to point to the physical address
    uint64_t *high = (uint64_t *)&pm[va2pa((cr->reg).rsp, cr)];
    high = &high[n];
    uint64_t va = (cr->reg).rsp + n * 8;
    for(int i = 0; i < 2 * n; i ++ )
    {
        uint64_t *ptr = (uint64_t *)(high - i);
        // print virtual address instead of physical address 
        // because usually the address we see in the program is virtual address
        // including p/x in GDB
        printf("0x%16lx : %16lx", va, (uint64_t)*ptr);

        if(i == n)
        {
            printf(" <== rsp");
        }
        printf("\n");
        va -= 8;
    }
}















































































































