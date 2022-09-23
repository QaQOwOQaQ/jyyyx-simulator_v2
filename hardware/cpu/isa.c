// Instruction Set Architecture
// A define between hardware and software

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../headers/common.h"
#include "../../headers/memory.h"
#include "../../headers/cpu.h"

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
static void     parse_instruction(const char *str, inst_t *inst, core_t *cr); // parse instruction:str to assembly_inst_type:inst
static void     parse_operand    (const char *str, od_t   *od,   core_t *cr); // parse instruction:str to operand:od
static uint64_t decode_operand   (od_t *od);


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

// lookup table
static const char *reg_name_list[72] = {
    "%rax", "%eax", "%ax", "%ah", "%al",
    "%rbx", "%ebx", "%bx", "%bh", "%bl",
    "%rcx", "%ecx", "%cx", "%ch", "%cl",
    "%rdx", "%edx", "%dx", "%dh", "%dl",
    "%rsi", "%esi", "%si", "%sih", "%sil",
    "%rdi", "%edi", "%di", "%dih", "%dil",
    "%rbp", "%ebp", "%bp", "%bph", "%bpl",
    "%rsp", "%esp", "%sp", "%sph", "%spl",
    /*----------------------------------*/
    "%r8", "%r8d", "%r8w", "%r8b",
    "%r9", "%r9d", "%r9w", "%r9b",
    "%r10", "%r10d", "%r10w", "%r10b",
    "%r11", "%r11d", "%r11w", "%r11b",
    "%r12", "%r12d", "%r12w", "%r12b",
    "%r13", "%r13d", "%r13w", "%r13b",
    "%r14", "%r14d", "%r14w", "%r14b",
    "%r15", "%r15d", "%r15w", "%r15b",
};


static uint64_t reflect_registers(const char *str, core_t *cr)
{
    // reflect means "映射";    
    // %rax --> tmp --> cr.reg.rax (即，我们要先知道是哪个寄存器tmp，然后再转化为runtime对应的register的address)
    
    reg_t *reg = &(cr->reg);
    uint64_t reg_addr[72] = {
        (uint64_t)&(reg->rax), (uint64_t)&(reg->eax), (uint64_t)&(reg->ax), (uint64_t)&(reg->ah), (uint64_t)&(reg->al),
        (uint64_t)&(reg->rbx), (uint64_t)&(reg->ebx), (uint64_t)&(reg->bx), (uint64_t)&(reg->bh), (uint64_t)&(reg->bl),
        (uint64_t)&(reg->rcx), (uint64_t)&(reg->ecx), (uint64_t)&(reg->cx), (uint64_t)&(reg->ch), (uint64_t)&(reg->cl),
        (uint64_t)&(reg->rdx), (uint64_t)&(reg->edx), (uint64_t)&(reg->dx), (uint64_t)&(reg->dh), (uint64_t)&(reg->dl),
        (uint64_t)&(reg->rsi), (uint64_t)&(reg->esi), (uint64_t)&(reg->si), (uint64_t)&(reg->sih), (uint64_t)&(reg->sil),
        (uint64_t)&(reg->rdi), (uint64_t)&(reg->edi), (uint64_t)&(reg->di), (uint64_t)&(reg->dih), (uint64_t)&(reg->dil),
        (uint64_t)&(reg->rbp), (uint64_t)&(reg->ebp), (uint64_t)&(reg->bp), (uint64_t)&(reg->bph), (uint64_t)&(reg->bpl),
        (uint64_t)&(reg->rsp), (uint64_t)&(reg->esp), (uint64_t)&(reg->sp), (uint64_t)&(reg->sph), (uint64_t)&(reg->spl),
        /*------------------------------------------------------------------------------------------------------------*/
        (uint64_t)&(reg->r8), (uint64_t)&(reg->r8d), (uint64_t)&(reg->r8w), (uint64_t)&(reg->r8b), 
        (uint64_t)&(reg->r9), (uint64_t)&(reg->r9d), (uint64_t)&(reg->r9w), (uint64_t)&(reg->r9b), 
        (uint64_t)&(reg->r10), (uint64_t)&(reg->r10d), (uint64_t)&(reg->r10w), (uint64_t)&(reg->r10b), 
        (uint64_t)&(reg->r11), (uint64_t)&(reg->r11d), (uint64_t)&(reg->r11w), (uint64_t)&(reg->r11b), 
        (uint64_t)&(reg->r12), (uint64_t)&(reg->r12d), (uint64_t)&(reg->r12w), (uint64_t)&(reg->r12b), 
        (uint64_t)&(reg->r13), (uint64_t)&(reg->r13d), (uint64_t)&(reg->r13w), (uint64_t)&(reg->r13b), 
        (uint64_t)&(reg->r14), (uint64_t)&(reg->r14d), (uint64_t)&(reg->r14w), (uint64_t)&(reg->r14b), 
        (uint64_t)&(reg->r15), (uint64_t)&(reg->r15d), (uint64_t)&(reg->r15w), (uint64_t)&(reg->r15b),   
    };
    
    for(int i = 0; i < 72; i ++ )
    {
        /*          [bug !!!]
            * I make a fucking bug becease I forget a fact that: if(a == b) then strcmp(a,b) == 0;
            * fucking bug I write is that: if(strcmp(str, reg_name_list[i]))    {...}
        */
       
        if(strcmp(str, reg_name_list[i]) == 0)   // to make %rax --> tmp
        {
            // now we konw that tmp is the index inside reg_name_list (我们知道这个东西是什么了)
            return reg_addr[i];
        }
    }

    // else
    printf("parse register %s error\n", str);
    exit(0);
}

static void parse_instruction(const char *str, inst_t *inst, core_t *cr)
{

}

// static
void parse_operand(const char *str, od_t *od, core_t *cr)
{
    // str: assembly code string, e.g. move %rsp %rbp
    // od : pointer to the address to sotre the parsed operand
    // cr : active core pocessor

    od->type = EMPTY;
    od->imm = 0;
    od->reg1 = 0;
    od->reg2 = 0;
    od->scal = 0;

    int str_len = strlen(str);
    if(str_len == 0)
    {
        // speciall judge: empty operand string
        return ;
    }

    if(str[0] == '$')
    {
        // immediate number
        od->type = IMM;
        // try to parse immediate number
        od->imm = string2uint_range(str, 1, -1);
        return ;
    }
    else if(str[0] == '%') 
    {
        // register
        od->type = REG;
        od->reg1 = reflect_registers(str, cr);
        return ;
    }
    else 
    {
        /*=====================================================*/
        /*                  |memory access|                    */
        /*-----------------------------------------------------*/
        /*    instruction    :  imm(%reg1,%reg2,scal)          */
        /*-----------------------------------------------------*/
        /*    count_brackets :   0    1    1     1             */
        /*    count_comma    :   0    0    1     2             */
        /*=====================================================*/
        
        /*
            * we have two ways to process operand
            * First is use really automaton such as DFA
            * Second is use count-check, just like a implicit(隐式) automaton
            * we choice the Second way, just becasue it's simple
        */
        char imm[64] = {'\0'}, reg1[64] = {'\0'}, reg2[64] = {'\0'}, scal[64] = {'\0'};
        int imm_len = 0, reg1_len = 0, reg2_len = 0, scal_len = 0;

        int c_brackets = 0; // count_brackets ()
        int c_comma = 0;    // count_comma     ,
        
        for(int i = 0; i < str_len; i ++ )
        {
            char c = str[i];
            if(c == '(' || c == ')')
            {
                c_brackets ++ ;
                continue;
            }
            else if(c == ',')
            {
                c_comma ++ ;
                continue;
            }
            else
            {
                // parse imm(reg1,reg2,scl)
                // e.g. no space here, look at gdb, no space also
                // such as: imm(reg1, reg2, reg3) is invalid
                
                // following notes like "xxx" means the char we are processing now
                // "???"" means the string we have already processed
                if(c_brackets == 0)
                {
                    imm[imm_len ++ ] = c;
                    continue;
                }
                else if(c_brackets == 1)
                {
                    if(c_comma == 0)
                    {
                        reg1[reg1_len ++ ] = c;
                        continue;
                    }
                    else if(c_comma == 1)
                    {
                        reg2[reg2_len ++ ] = c;
                        continue;
                    }
                    else if(c_comma == 2)
                    {
                        scal[scal_len ++ ] = c;
                        continue;
                    }
                }
            }
        }

        if(scal_len > 0)
        {
            od->scal = string2uint(scal);
            if(od->scal != 1 && od->scal != 2 && od->scal != 4 && od->scal != 8)
            {
                printf("%s is not a legal scaler(1 || 2 || 4 || 8)\n", scal);
                exit(0);
            }
        }

        if(reg1_len > 0)
        {
            printf("debug_reg1: %s\n", reg1);
            od->reg1 = reflect_registers(reg1, cr);
        }

        if(reg2_len > 0)
        {
            printf("debug_reg2: %s\n", reg2);
            od->reg2 = reflect_registers(reg2, cr);
        }

        if(imm_len > 0)
        {
            od->imm = string2uint(imm);
        }
        
        //  set operand type
        if(c_brackets == 0) // [imm]
        {
            od->type = MEM_IMM; // 1
        }
        else    // [reg] && [scal]
        {
            if(c_comma == 0) // [reg1]
            {
                if(imm_len == 0) 
                {
                    od->type = MEM_REG1; // 2
                }
                else 
                {
                    od->type = MEM_IMM_REG1; // 3
                }
            }
            else if(c_comma == 1) // [reg1] [reg2]
            {
                // if we have reg2, we must have reg1
                // because if we dont have reg1, why need reg2?
                // we can use reg1 to repace reg2
                if(imm_len == 0)
                {
                    od->type = MEM_REG1_REG2; // 4
                }
                else 
                {
                    od->type = MEM_IMM_REG1_REG2; // 5
                }
            }
            else if(c_comma == 2)   // [reg1] [reg2] [scal]
            {
                if(imm_len == 0)
                {
                    if(reg1_len == 0)
                    {
                        od->type = MEM_REG2_SCAL; // 6
                    }
                    else
                    {
                        od->type = MEM_REG1_REG2_SCAL; // 7
                    }
                }
                else 
                {
                    if(reg1_len == 0)
                    {
                        od->type = MEM_IMM_REG2_SCAL; // 8
                    }
                    else
                    {
                        od->type = MEM_IMM_REG1_REG2_SCAL; // 9
                    }
                }
            }
        }
    }
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
    cr->flags.__flag_values = 0;
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
        write64bits_dram(va2pa(cr->reg.rsp, cr), *(uint64_t *)src, cr);
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
        uint64_t old_val = read64bits_dram(va2pa((cr->reg).rsp, cr), cr);
        (cr->reg).rsp = (cr->reg).rsp + 8;
        *(uint64_t *)src = old_val;
        next_rip(cr);
        reset_cflags(cr);
        return ;
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
    if((DEBUG_VERBOSE_SET & DEBUG_REGISTERS) == 0X0)
    {
        return ;
    }

    reg_t reg = cr->reg;

    printf("rax = 0x%-16lx\nrbx = 0x%-16lx\nrcx = 0x%-16lx\nrdx = 0x%-16lx\n", reg.rax, reg.rbx, reg.rcx, reg.rdx);
    printf("rsi = 0x%-16lx\nrdi = 0x%-16lx\nrbp = 0x%-16lx\nrsp = 0x%-16lx\n", reg.rsi, reg.rdi, reg.rbp, reg.rsp);
    printf("rip = 0x%-16lx\n", cr->rip);
    printf("CF = %u\tZF = %u\tSF = %u\tOF = %u\n", cr->flags.CF, cr->flags.ZF, cr->flags.SF, cr->flags.OF);
}

void print_stack(core_t *cr)
{
    if((DEBUG_VERBOSE_SET & DEBUG_PRINTSTACK) == 0x0)
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



// Test function: void parse_operand();
void TestParseOperand();
void TestParseOperand()
{
    printf("isa start------------>\n");
    core_t *ac = (core_t *)&cores[ACTIVE_CORE];
    const char *operand[12] = {
        "", // 0
        "$0x1234", // 1
        "%rax", // 2
        "0xabcd", // 3
        "(%rsp)", // 4
        "0xabcd(%rsp)", // 5
        "(%rsp,%rbx)", // 6
        "0xabcd(%rsp,%rbx)", // 7
        "(,%rbx,8)", // 8
        "0xabcd(,%rbx,8)", // 9
        "(%rsp,%rbx,8)", // 10
        "0xabcd(%rsp,%rbx,8)", // 11
    };

    printf("rax: %p\n", &((ac->reg).rax));
    printf("rsp: %p\n", &((ac->reg).rsp));
    printf("rbx: %p\n", &((ac->reg).rbx));

    for(int i = 0; i < 12; i ++ )
    {
        printf("<-------%dth-------h>\n", i + 1);
        od_t od;
        parse_operand(operand[i], &od, ac);

        printf("\n%s\n", operand[i]);
        printf("od_enum type: %d\n", od.type);
        printf("od_imm  0x%lx\n", od.imm);
        printf("od_reg1 0x%lx\n", od.reg1);
        printf("od_reg2 0x%lx\n", od.reg2);
        printf("od_scal 0x%lx\n", od.scal);    
    }

    printf("isa end-------------->\n");
}













































































































