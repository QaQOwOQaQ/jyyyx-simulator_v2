// Instruction Set Architecture
// A define between hardware and software

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <headers/cpu.h>
#include <headers/common.h>
#include <headers/memory.h>
#include <headers/instruction.h>
 
/*====================================*/
/*      pase assembly instruction     */
/*====================================*/

// function to map the string assembly code to inst_t instance 
static void     parse_instruction(const char *str, inst_t *inst); // parse instruction:str to assembly_inst_type:inst
static void     parse_operand    (const char *str, od_t   *od); // parse instruction:str to operand:od
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


static uint64_t reflect_registers(const char *str)
{
    // reflect means "映射";    
    // %rax --> tmp --> ().reg.rax (即，我们要先知道是哪个寄存器tmp，然后再转化为runtime对应的register的address)
    
    cpu_reg_t *reg = &cpu_reg;
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

static void parse_instruction(const char *str, inst_t *inst)
{
    char op_str[64] = {'\0'},  src_str[64] = {'\0'}, dst_str[64] = {'\0'};
    int op_len = 0, src_len = 0, dst_len = 0;

    int str_len = strlen(str);
    int count_brackets = 0;
    int state = 0;
    /*==============================================*/ 
    /*      inst : [__] op [__] od_1 , od_2 [__]    */
    /*      state:  0   1    2   3   4   5   6      */
    /*==============================================*/

    for(int i = 0; i < str_len; i ++ )
    {
        char c = str[i];
        // 1th
        if(c == '(' || c == ')')
        {
            count_brackets ++ ;
        }
        // 2th
        if(state == 0 && c != ' ') 
        {
            state = 1;
        }
        else if(state == 1 && c == ' ')
        {
            state = 2;
            // the current character is a space and can be skipped directly 
            continue;
        }
        else if(state == 2 && c != ' ')
        {
            state = 3;
        }
        else if(state == 3 && c == ',' && (count_brackets == 0 || count_brackets == 2))
        {
            state = 4;
            continue;
        }
        else if(state == 4 && c != ' ' && c != ',')
        {
            state = 5;
        }
        else if(state == 5 && c == ' ')
        {
            state = 6;
            continue;
        }
        // 3th
        if(state == 1)
        {
            op_str[op_len ++ ] = c;
            continue;
        }
        else if(state == 3)
        {
            src_str[src_len ++ ] = c;
            continue;
        }
        else if(state == 5)
        {
            dst_str[dst_len ++] = c;
            continue;
        }
    }

    // parse done: op_str, src_str, dst_str 
    parse_operand(src_str, &inst->src);
    parse_operand(dst_str, &inst->dst);
/*
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
*/
    // the instruction type match can use "trie\prefix tree"
    if(strcmp(op_str, "mov") == 0 || strcmp(op_str, "movq") == 0)
    {
        inst->op = INST_MOV;
    }
    else if(strcmp(op_str, "push") == 0)
    {
        inst->op = INST_PUSH;
    }
    else if(strcmp(op_str, "pop") == 0)
    {
        inst->op = INST_POP;
    }else if(strcmp(op_str, "leaveq") == 0)
    {
        inst->op = INST_LEAVE;
    }
    else if(strcmp(op_str, "callq") == 0)
    {
        inst->op = INST_CALL;
    }
    else if(strcmp(op_str, "retq") == 0)
    {
        inst->op = INST_RET;
    }
    else if(strcmp(op_str, "add") == 0)
    {
        inst->op = INST_ADD;
    }
    else if(strcmp(op_str, "sub") == 0)
    {
        inst->op = INST_SUB;
    }
    else if(strcmp(op_str, "cmpq") == 0)
    {
        inst->op = INST_CMP;
    }
    else if(strcmp(op_str, "jne") == 0)
    {
        inst->op = INST_JNE;
    }
    else if(strcmp(op_str, "jmp") == 0)
    {
        inst->op = INST_JMP;
    }
    else
    {
        printf("Instruction parse error: %s\n", op_str);
        exit(0);
    }

    debug_print(DEBUG_PARSEINST, 
                "[%s (%d)] [%s (%d)] [%s (%d)]\n", 
                op_str, inst->op, src_str, inst->src.type, dst_str, inst->dst.type
    );

}

// static
void parse_operand(const char *str, od_t *od)
{
    // str: assembly code string, e.g. move %rsp %rbp
    // od : pointer to the address to sotre the parsed operand
    // () : active core pocessor

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
        od->reg1 = reflect_registers(str);
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
            od->reg1 = reflect_registers(reg1);
        }

        if(reg2_len > 0)
        {
            od->reg2 = reflect_registers(reg2);
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

static void mov_handler     (od_t *src_od, od_t *dst_od);
static void push_handler    (od_t *src_od, od_t *dst_od);
static void pop_handler     (od_t *src_od, od_t *dst_od);
static void leave_handler   (od_t *src_od, od_t *dst_od);
static void call_handler    (od_t *src_od, od_t *dst_od);
static void ret_handler     (od_t *src_od, od_t *dst_od);
static void add_handler     (od_t *src_od, od_t *dst_od);
static void sub_handler     (od_t *src_od, od_t *dst_od);
static void cmp_handler     (od_t *src_od, od_t *dst_od);
static void jne_handler     (od_t *src_od, od_t *dst_od);
static void jmp_handler     (od_t *src_od, od_t *dst_od);

// handler table storing the handlers to different instruction types
typedef void (*handler_t)(od_t *, od_t *);
// look-up table of pointers to function
static handler_t handler_table[NUM_INSTRTYPE] = {
    &mov_handler,       // 0, mov
    &push_handler,      // 1
    &pop_handler,       // 2
    &leave_handler,     // 3
    &call_handler,      // 4
    &ret_handler,       // 5
    &add_handler,       // 6
    &sub_handler,       // 7
    &cmp_handler,       // 8
    &jne_handler,       // 9
    &jmp_handler        // 10
};



// reset the condition flags
// inline to reduce cost

// update the rip pointer to the next instruction sequentially
static inline void increase_pc()
{
    // we are handling the fixed-length of assembly string here
    // but there size can be variable as true x86 instructions 
    // that's because the operand's sizes follow the specific encoding rule
    // the risc-v is a fixed length ISA
    cpu_pc.rip = cpu_pc.rip + sizeof(char) * MAX_INSTRUCTION_CHAR;
}

static inline void reset_cflags()
{
    cpu_flags.__flags_value = 0;
}

// instruction handlers
// the following src and dst is an addr
static void mov_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);
    uint64_t dst = decode_operand(dst_od);

    if(src_od->type == REG && dst_od->type == REG)
    {
        // src: register
        // dst: register
        *(uint64_t *)dst = *(uint64_t *)src;
        increase_pc();
        reset_cflags();
        return ;
    }
    else if(src_od->type == REG && dst_od->type >= MEM_IMM)
    {
        // src: register
        // dst: virtual address
        cpu_write64bits_dram(va2pa(dst), *(uint64_t *)src);
        increase_pc();
        reset_cflags();
        return ;
    }
    else if(src_od->type >= MEM_IMM && dst_od->type == REG)
    {
        // src: virtual address
        // dst: register
        *(uint64_t *)dst = cpu_read64bits_dram(va2pa(src));
        increase_pc();
        reset_cflags();
        return ;
    }
    else if(src_od->type == IMM && dst_od->type == REG)
    {
        // src: immediate number (uint64_t bit map)
        // dst: register
        *(uint64_t *)dst = src;
        increase_pc();
        reset_cflags();
        return ;
    }
}

static void push_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);
    if(src_od->type == REG)
    {
        // src: register
        // dst: empty
        cpu_reg.rsp = cpu_reg.rsp - 8;
        // do not write:cpu_reg.rsp  **bug**
        cpu_write64bits_dram(va2pa(cpu_reg.rsp), *(uint64_t *)src);
        increase_pc();
        reset_cflags();
        return ;
    }
}

static void pop_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);

    if(src_od->type == REG)
    {
        // src: register
        // dst: empty
        uint64_t old_val = cpu_read64bits_dram(va2pa(cpu_reg.rsp));
        cpu_reg.rsp = cpu_reg.rsp + 8;
        *(uint64_t *)src = old_val;
        increase_pc();
        reset_cflags();
        return ;
    }
}

static void leave_handler(od_t *src_od, od_t *dst_od)
{
    // Leave means the function execute done
    // This instruction thkes no arguments.
    // It is equivalent to executing the following two instruction:
    // 1. moveq %rbp,%rsp
    // 2. pop %rbp
    cpu_reg.rsp = cpu_reg.rbp;         
    uint64_t old_val = cpu_read64bits_dram(va2pa(cpu_reg.rsp));
    cpu_reg.rbp = old_val;
    cpu_reg.rsp = cpu_reg.rsp + 8;      
    increase_pc();
    reset_cflags();

    // you can find the two instructions is the reverse operation of the begining two instructions when you ()eate a new stack:
    // 1. push %rbp
    // 2. moveq %rsp %rbp
}

static void call_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);

    // src: imediate number: virtual address of target function starting
    // dst: empty
    
    // push the return value
    cpu_reg.rsp = cpu_reg.rsp - 8;
    cpu_write64bits_dram(va2pa(cpu_reg.rsp), cpu_pc.rip + sizeof(char) * MAX_INSTRUCTION_CHAR);

    // jump to target function address
    // TODO: support PC relative addressing
    cpu_pc.rip = src;
    reset_cflags();
}

static void ret_handler(od_t *src_od, od_t *dst_od)
{
    // src: empty
    // dst: empty
    
    // pop rsp
    uint64_t ret_addr = cpu_read64bits_dram(va2pa(cpu_reg.rsp));
    (cpu_reg.rsp) = cpu_reg.rsp + 8; /* debug 2 hour because I forget to add that sentense, why didn't change rsp when you got ret_addr*/
    cpu_pc.rip = ret_addr;
    reset_cflags();
}

static void add_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);
    uint64_t dst = decode_operand(dst_od);
    if(src_od->type == REG && dst_od->type == REG)
    {
        // src: register (value: int64_t bit map)
        // dst: register (value: int64_t bit map)
        uint64_t val = (*(uint64_t *)src) + (*(uint64_t *)dst);
        
        int val_sign = ((val >> 63) * 0x1);
        int src_sign = ((*(uint64_t *)src >> 63) & 0x1);
        int dst_sign = ((*(uint64_t *)dst >> 63) & 0x1);

        // set condition flag
        cpu_flags.CF = (val < *(uint64_t *)src);   // unsigne
        cpu_flags.ZF = (val == 0);
        cpu_flags.SF = val_sign;
        cpu_flags.OF = ((!(src_sign ^ dst_sign)) & (src_sign ^ val_sign));   // signed
        //cpu_flags.OF = (src_sign == 0 && dst_sign == 0 && val_sign == 1) || (src_sign == 1 && dst_sign == 1 && val_sign == 0);

        // update registers
        *(uint64_t *)dst = val;
        // signede and unsigned value follow the same addition. e.g.
        increase_pc();
        return ;
    }
}   

static void sub_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);
    uint64_t dst = decode_operand(dst_od);

    if(src_od->type == IMM && dst_od->type == REG)
    {
        // src: immediate
        // dst: register (value: int64_t bit map)
        // dst = dst - src = dst + (-src)
        // use (~x+1) but (-x) is a better way, we interpret it at bit mapping
        uint64_t val = *(uint64_t *)dst + (~src + 1); // operation over the bit map
        
        int val_sign = ((val >> 63) & 0x1);
        int src_sign = ((src >> 63) & 0x1);
        int dst_sign = ((*(uint64_t *)dst >> 63) & 0x1);

        // set condition flag
        cpu_flags.CF = (val > (*(uint64_t *)dst));   // unsigne
        cpu_flags.ZF = (val == 0);
        cpu_flags.SF = val_sign;
        cpu_flags.OF = ((src_sign ^ dst_sign) & (val_sign ^ src_sign));  // signed
        //cpu_flags.OF = (src_sign == 1 && dst_sign == 0 && val_sign == 1) || (src_sign == 0 && dst_sign == 1 && val_sign == 0);

        // update registers
        *(uint64_t *)dst = val;
        // signede and unsigned value follow the same addition. e.g.
        increase_pc();
        return ;
    }
}

static void cmp_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);
    uint64_t dst = decode_operand(dst_od);

    if(src_od->type == IMM && dst_od->type >= MEM_IMM)
    {
        // src: immediate
        // dst: access memory
        // cmp src dst --> dst-src --> s2+(-s1)
        uint64_t dval = cpu_read64bits_dram(va2pa(dst));
        uint64_t val = dval + (~src + 1);

        int val_sign = ((val  >> 63) & 0x1);
        int src_sign = ((src  >> 63) & 0x1);
        int dst_sign = ((dval >> 63) & 0x1);

        // set condition flag
        cpu_flags.CF = (val > dval);   // unsigne
        cpu_flags.ZF = (val == 0);
        cpu_flags.SF = val_sign;
        cpu_flags.OF = ((src_sign ^ dst_sign) & (val_sign ^ src_sign));  // signed
        //cpu_flags.OF = (src_sign == 1 && dst_sign == 0 && val_sign == 1) || (src_sign == 0 && dst_sign == 1 && val_sign == 0);

        increase_pc();
        return ;
    }
}

static void jne_handler(od_t *src_od, od_t *dst_od)
{
    uint64_t src = decode_operand(src_od);
    /*  
        In the assembly code of jmp, the incoming source operand is a register-type value,
        you can see: like: "jmp 0x400380".         
        so the number of 0x400380 will be interpret as a register in our simulator(beceuse the number without '$'),
        but the number should be interpret as a immediately, emmmm...., very odd.
        so you shoudldn't add type-check of source like: if(src_od->type == IMM) {...}
        because the src type as a register
        the solution to solve it is don't check the type
        very easy solution 
    */
    if(cpu_flags.ZF != 1)
    {
        // success to jump
        cpu_pc.rip = src;
    }
    else
    {
        // go on the next instruction
        increase_pc();
    }
    reset_cflags();
}

static void jmp_handler(od_t *src_od, od_t *dst_od)
{
    // like jne, the number of src also is a immediate but the type of it is a register
    uint64_t src= decode_operand(src_od);
    cpu_pc.rip = src;
}


// instruction cycle is implemented in CPU
// the only exposed interface outside CPU
void instruction_cycle()
{
    /*  fetch inst --> decode --> get operands --> execute --> write back  */

    // FETCH: get the instruction string by program counter
    char inst_str[MAX_INSTRUCTION_CHAR + 10];
    cpu_readinst_dram(va2pa(cpu_pc.rip), inst_str);

#ifdef DEBUG_INSTRUCTION_CYCLE
    printf("%8lx        %s\n", cpu_pc.rip, inst_str);
#endif 

    // DECODE: decode the rum-time instruction operands
    inst_t inst;
    parse_instruction(inst_str, &inst);

#ifdef DEBUG_INSTRUCTION_CYCLE
    printf("[debug]Inst: %s\n", inst_str);
#endif 

    // EXCUTE: get the function pointer or handler by the operator
    handler_t handler = handler_table[inst.op];
    handler(&(inst.src), &(inst.dst));
}

#ifdef DEBUG_PARSE_INSTRUCTION

static int operand_equal(od_t *a, od_t *b)
{
    if (a == NULL && b == NULL)
    {
        return 1;
    }

    if (a == NULL || b == NULL)
    {
        return 0;
    }

    int equal = 1;
    equal = equal && (a->type == b->type);
    equal = equal && (a->imm == b->imm);
    equal = equal && (a->scal == b->scal);
    equal = equal && (a->reg1 == b->reg1);
    equal = equal && (a->reg2 == b->reg2);

    return equal;
}

static int instruction_equal(inst_t *a, inst_t *b)
{
    if (a == NULL && b == NULL)
    {
        return 1;
    }

    if (a == NULL || b == NULL)
    {
        return 0;
    }

    int equal = 1;
    equal = equal && (a->op == b->op);
    equal = equal && operand_equal(&a->src, &b->src);
    equal = equal && operand_equal(&a->dst, &b->dst);

    return equal;
}

static void TestParsingInstruction()
{
    printf("Testing instruction parsing ...\n");

    char assembly[15][MAX_INSTRUCTION_CHAR] = {
        "push   %rbp",              // 0
        "mov    %rsp,%rbp",         // 1
        "mov    %rdi,-0x18(%rbp)",  // 2
        "mov    %rsi,-0x20(%rbp)",  // 3
        "mov    -0x18(%rbp),%rdx",  // 4
        "mov    -0x20(%rbp),%rax",  // 5
        "add    %rdx,%rax",         // 6
        "mov    %rax,-0x8(%rbp)",   // 7
        "mov    -0x8(%rbp),%rax",   // 8
        "pop    %rbp",              // 9
        "retq",                     // 10
        "mov    %rdx,%rsi",         // 11
        "mov    %rax,%rdi",         // 12
        "callq  0",                 // 13
        "mov    %rax,-0x8(%rbp)",   // 14
    };

    inst_t std_inst[15] = {
        // push   %rbp
        {
            .op = INST_PUSH,
            .src = 
                {
                    .type = OD_REG,
                    .imm = 0,
                    .scal = 0,
                    .reg1 = (uint64_t)(&cpu_reg.rbp),
                    .reg2 = 0
                },
            .dst = 
                {
                    .type = OD_EMPTY,
                    .imm = 0,
                    .scal = 0,
                    .reg1 = 0,
                    .reg2 = 0
                }
        },        
        // mov    %rsp,%rbp
        {
            .op = INST_MOV, 
            .src = {
                .type = OD_REG, 
                .imm = 0, 
                .scal = 0, 
                .reg1 = (uint64_t)(&cpu_reg.rsp), 
                .reg2 = 0
            }, 
            .dst = {
                .type = OD_REG,
                .imm = 0, 
                .scal = 0, 
                .reg1 = (uint64_t)(&cpu_reg.rbp), 
                .reg2 = 0}
        },
        // mov    %rdi,-0x18(%rbp)
        {
            .op = INST_MOV, 
            .src = {
                .type = OD_REG, 
                .imm = 0, 
                .scal = 0, 
                .reg1 = (uint64_t)(&cpu_reg.rdi), 
                .reg2 = 0
            }, 
            .dst = {
                .type = OD_MEM_IMM_REG1, 
                .imm = 0x1LL + (~0x18LL), 
                .scal = 0, 
                .reg1 = (uint64_t)(&cpu_reg.rbp), 
                .reg2 = 0
            }
        },
        // mov    %rsi,-0x20(%rbp)
        {
            .op = INST_MOV, 
            .src = {
                .type = OD_REG, 
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rsi),
                .reg2 = 0
            }, 
            .dst = {
                .type = OD_MEM_IMM_REG1,
                .imm = 0x1LL + (~0x20LL),
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0
            }
        },
        // mov    -0x18(%rbp),%rdx
        {
            .op = INST_MOV, 
            .src = {
                .type = OD_MEM_IMM_REG1,
                .imm = 0x1LL + (~0x18LL),
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0
            },
            .dst = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rdx),
                .reg2 = 0
            }
        },
        // mov    -0x20(%rbp),%rax
        {
            .op = INST_MOV,
            .src = {
                .type = OD_MEM_IMM_REG1,
                .imm = 0x1LL + (~0x20LL),
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0
            },
            .dst = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rax),
                .reg2 = 0
            }
        },
        // add    %rdx,%rax
        {
            .op = INST_ADD,
            .src = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rdx),
                .reg2 = 0
            },
            .dst = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rax),
                .reg2 = 0
            }
        },
        // mov    %rax,-0x8(%rbp)
        {
            .op = INST_MOV,
            .src = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rax),
                .reg2 = 0
            },
            .dst = {
                .type = OD_MEM_IMM_REG1,
                .imm = 0x1LL + (~0x8LL),
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0
            }
        },
        // mov    -0x8(%rbp),%rax
        {
            .op = INST_MOV,
            .src = {
                .type = OD_MEM_IMM_REG1,
                .imm = 0x1LL + (~0x8LL),
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0},
            .dst = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rax),
                .reg2 = 0
            }
        },
        // pop    %rbp
        {
            .op = INST_POP,
            .src = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0
            },
            .dst = {
                .type = OD_EMPTY,
                .imm = 0,
                .scal = 0,
                .reg1 = 0,
                .reg2 = 0
            }
        },
        // retq
        {
            .op = INST_RET,
            .src = {
                .type = OD_EMPTY,
                .imm = 0,
                .scal = 0,
                .reg1 = 0,
                .reg2 = 0
            },
            .dst = {
                .type = OD_EMPTY,
                .imm = 0,
                .scal = 0,
                .reg1 = 0,
                .reg2 = 0
            }
        },
        // mov    %rdx,%rsi
        {
            .op = INST_MOV,
            .src = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rdx),
                .reg2 = 0
            },
            .dst = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rsi),
                .reg2 = 0
            }
        },
        // mov    %rax,%rdi
        {
            .op = INST_MOV,
            .src = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rax),
                .reg2 = 0
            },
            .dst = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rdi),
                .reg2 = 0
            }
        },
        // callq  0
        {
            .op = INST_CALL,
            .src = {
                .type = OD_MEM_IMM,
                .imm = 0,
                .scal = 0,
                .reg1 = 0,
                .reg2 = 0
            },
            .dst = {
                .type = OD_EMPTY,
                .imm = 0,
                .scal = 0,
                .reg1 = 0,
                .reg2 = 0
            }
        },
        // mov    %rax,-0x8(%rbp)
        {
            .op = INST_MOV,
            .src = {
                .type = OD_REG,
                .imm = 0,
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rax),
                .reg2 = 0
            },
            .dst = {
                .type = OD_MEM_IMM_REG1,
                .imm = 0x1LL + (~0x8LL),
                .scal = 0,
                .reg1 = (uint64_t)(&cpu_reg.rbp),
                .reg2 = 0
            }
        },
    };
    
    inst_t inst_parsed;

    for (int i = 0; i < 15; ++ i)
    {
        parse_instruction(assembly[i], &inst_parsed);
        assert(instruction_equal(&std_inst[i], &inst_parsed) == 1);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

static void TestParsingOperand()
{
    printf("Testing operand parsing ...\n");

    const char *strs[11] = {
        "$0x1234",
        "%rax",
        "0xabcd",
        "(%rsp)",
        "0xabcd(%rsp)",
        "(%rsp,%rbx)",
        "0xabcd(%rsp,%rbx)",
        "(,%rbx,8)",
        "0xabcd(,%rbx,8)",
        "(%rsp,%rbx,8)",
        "0xabcd(%rsp,%rbx,8)",
    };
    
    od_t std_ods[11] = 
    {
        // $0x1234
        {
            .type   = OD_IMM,
            .imm    = 0x1234,
            .scal   = 0,
            .reg1   = 0,
            .reg2   = 0
        },
        // %rax
        {
            .type   = OD_REG,
            .imm    = 0,
            .scal   = 0,
            .reg1   = (uint64_t)(&cpu_reg.rax),
            .reg2   = 0
        },
        // 0xabcd
        {
            .type   = OD_MEM_IMM,
            .imm    = 0xabcd,
            .scal   = 0,
            .reg1   = 0,
            .reg2   = 0
        },
        // (%rsp)
        {
            .type   = OD_MEM_REG1,
            .imm    = 0,
            .scal   = 0,
            .reg1   = (uint64_t)(&cpu_reg.rsp),
            .reg2   = 0
        },
        // 0xabcd(%rsp)
        {
            .type   = OD_MEM_IMM_REG1,
            .imm    = 0xabcd,
            .scal   = 0,
            .reg1   = (uint64_t)(&cpu_reg.rsp),
            .reg2   = 0
        },
        // (%rsp,%rbx)
        {
            .type   = OD_MEM_REG1_REG2,
            .imm    = 0,
            .scal   = 0,
            .reg1   = (uint64_t)(&cpu_reg.rsp),
            .reg2   = (uint64_t)(&cpu_reg.rbx)
        },
        // 0xabcd(%rsp,%rbx)
        {
            .type   = OD_MEM_IMM_REG1_REG2,
            .imm    = 0xabcd,
            .scal   = 0,
            .reg1   = (uint64_t)(&cpu_reg.rsp),
            .reg2   = (uint64_t)(&cpu_reg.rbx)
        },
        // (,%rbx,8)
        {
            .type   = OD_MEM_REG2_SCAL,
            .imm    = 0,
            .scal   = 8,
            .reg1   = 0,
            .reg2   = (uint64_t)(&cpu_reg.rbx)
        },
        // 0xabcd(,%rbx,8)
        {
            .type   = OD_MEM_IMM_REG2_SCAL,
            .imm    = 0xabcd,
            .scal   = 8,
            .reg1   = 0,
            .reg2   = (uint64_t)(&cpu_reg.rbx)
        },
        // (%rsp,%rbx,8)
        {
            .type   = OD_MEM_REG1_REG2_SCAL,
            .imm    = 0,
            .scal   = 8,
            .reg1   = (uint64_t)(&cpu_reg.rsp),
            .reg2   = (uint64_t)(&cpu_reg.rbx)
        },
        // 0xabcd(%rsp,%rbx,8)
        {
            .type   = OD_MEM_IMM_REG1_REG2_SCAL,
            .imm    = 0xabcd,
            .scal   = 8,
            .reg1   = (uint64_t)(&cpu_reg.rsp),
            .reg2   = (uint64_t)(&cpu_reg.rbx)
        },
    };
    
    od_t od_parsed;

    for (int i = 0; i < 11; ++ i)
    {
        parse_operand(strs[i], &od_parsed);
        assert(operand_equal(&std_ods[i], &od_parsed) == 1);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

int main()
{
    TestParsingOperand();
    TestParsingInstruction();

    finally_cleanup();

    return 0;
}

#endif

#ifdef DEBUG_INSTRUCTION_CYCLE

static void print_register()
{
    printf("rax = %16lx\trbx = %16lx\trcx = %16lx\trdx = %16lx\n",
        cpu_reg.rax, cpu_reg.rbx, cpu_reg.rcx, cpu_reg.rdx);
    printf("rsi = %16lx\trdi = %16lx\trbp = %16lx\trsp = %16lx\n",
        cpu_reg.rsi, cpu_reg.rdi, cpu_reg.rbp, cpu_reg.rsp);
    printf("rip = %16lx\n", cpu_pc.rip);
    printf("CF = %u\tZF = %u\tSF = %u\tOF = %u\n",
        cpu_flags.CF, cpu_flags.ZF, cpu_flags.SF, cpu_flags.OF);
}

static void print_stack()
{
    int n = 10;    
    uint64_t *high = (uint64_t*)&pm[va2pa(cpu_reg.rsp)];
    high = &high[n];
    uint64_t va = cpu_reg.rsp + n * 8;

    for (int i = 0; i < 2 * n; ++ i)
    {
        uint64_t *ptr = (uint64_t *)(high - i);
        printf("0x%16lx : %16lx", va, (uint64_t)*ptr);

        if (i == n)
        {
            printf(" <== rsp");
        }
        printf("\n");
        va -= 8;
    }
}

static void TestAddFunctionCallAndComputation()
{
    printf("Testing add function call ...\n");
 
    // init state
    cpu_reg.rax = 0xabcd;
    cpu_reg.rbx = 0x8000670;
    cpu_reg.rcx = 0x8000670;
    cpu_reg.rdx = 0x12340000;
    cpu_reg.rsi = 0x7ffffffee208;
    cpu_reg.rdi = 0x1;
    cpu_reg.rbp = 0x7ffffffee110;
    cpu_reg.rsp = 0x7ffffffee0f0;

    cpu_write64bits_dram(va2pa(0x7ffffffee110), 0x0000000000000000);    // rbp
    cpu_write64bits_dram(va2pa(0x7ffffffee108), 0x0000000000000000);
    cpu_write64bits_dram(va2pa(0x7ffffffee100), 0x0000000012340000);
    cpu_write64bits_dram(va2pa(0x7ffffffee0f8), 0x000000000000abcd);
    cpu_write64bits_dram(va2pa(0x7ffffffee0f0), 0x0000000000000000);    // rsp

    // 2 before call
    // 3 after call before push
    // 5 after rbp
    // 13 before pop
    // 14 after pop before ret
    // 15 after ret
    char assembly[15][MAX_INSTRUCTION_CHAR] = {
        "push   %rbp",              // 0
        "mov    %rsp,%rbp",         // 1
        "mov    %rdi,-0x18(%rbp)",  // 2
        "mov    %rsi,-0x20(%rbp)",  // 3
        "mov    -0x18(%rbp),%rdx",  // 4
        "mov    -0x20(%rbp),%rax",  // 5
        "add    %rdx,%rax",         // 6
        "mov    %rax,-0x8(%rbp)",   // 7
        "mov    -0x8(%rbp),%rax",   // 8
        "pop    %rbp",              // 9
        "retq",                     // 10
        "mov    %rdx,%rsi",         // 11
        "mov    %rax,%rdi",         // 12
        "callq  0x00400000",        // 13
        "mov    %rax,-0x8(%rbp)",   // 14
    };

    // copy to physical memory
    for (int i = 0; i < 15; ++ i)
    {
        cpu_writeinst_dram(va2pa(i * 0x40 + 0x00400000), assembly[i]);
    }
    cpu_pc.rip = MAX_INSTRUCTION_CHAR * sizeof(char) * 11 + 0x00400000;

    printf("begin\n");
    int time = 0;
    while (time < 15)
    {
        instruction_cycle();
#ifdef DEBUG_INSTRUCTION_CYCLE_INFO_REG_STACK
        print_register();
        print_stack();
#endif
        time ++;
    } 

    // gdb state ret from func
    assert(cpu_reg.rax == 0x1234abcd);
    assert(cpu_reg.rbx == 0x8000670);
    assert(cpu_reg.rcx == 0x8000670);
    assert(cpu_reg.rdx == 0xabcd);
    assert(cpu_reg.rsi == 0x12340000);
    assert(cpu_reg.rdi == 0xabcd);
    assert(cpu_reg.rbp == 0x7ffffffee110);
    assert(cpu_reg.rsp == 0x7ffffffee0f0);

    assert(cpu_read64bits_dram(va2pa(0x7ffffffee110)) == 0x0000000000000000); // rbp
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee108)) == 0x000000001234abcd);
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee100)) == 0x0000000012340000);
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee0f8)) == 0x000000000000abcd);
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee0f0)) == 0x0000000000000000); // rsp

    printf("\033[32;1m\tPass\033[0m\n");
}

static void TestSumRecursiveCondition()
{
    printf("Testing sum recursive function call ...\n");

    // init state
    cpu_reg.rax = 0x8000630;
    cpu_reg.rbx = 0x0;
    cpu_reg.rcx = 0x8000650;
    cpu_reg.rdx = 0x7ffffffee328;
    cpu_reg.rsi = 0x7ffffffee318;
    cpu_reg.rdi = 0x1;
    cpu_reg.rbp = 0x7ffffffee230;
    cpu_reg.rsp = 0x7ffffffee220;

    cpu_flags.__flags_value = 0;

    cpu_write64bits_dram(va2pa(0x7ffffffee230), 0x0000000008000650);    // rbp
    cpu_write64bits_dram(va2pa(0x7ffffffee228), 0x0000000000000000);
    cpu_write64bits_dram(va2pa(0x7ffffffee220), 0x00007ffffffee310);    // rsp

    char assembly[19][MAX_INSTRUCTION_CHAR] = {
        "push   %rbp",              // 0
        "mov    %rsp,%rbp",         // 1
        "sub    $0x10,%rsp",        // 2
        "mov    %rdi,-0x8(%rbp)",   // 3
        "cmpq   $0x0,-0x8(%rbp)",   // 4
        "jne    0x400200",          // 5: jump to 8
        "mov    $0x0,%eax",         // 6
        "jmp    0x400380",          // 7: jump to 14
        "mov    -0x8(%rbp),%rax",   // 8
        "sub    $0x1,%rax",         // 9
        "mov    %rax,%rdi",         // 10
        "callq  0x00400000",        // 11
        "mov    -0x8(%rbp),%rdx",   // 12
        "add    %rdx,%rax",         // 13
        "leaveq ",                  // 14
        "retq   ",                  // 15
        "mov    $0x3,%edi",         // 16
        "callq  0x00400000",        // 17
        "mov    %rax,-0x8(%rbp)",   // 18
    };

    // copy to physical memory
    for (int i = 0; i < 19; ++ i)
    {
        cpu_writeinst_dram(va2pa(i * 0x40 + 0x00400000), assembly[i]);
    }
    cpu_pc.rip = MAX_INSTRUCTION_CHAR * sizeof(char) * 16 + 0x00400000;

    printf("begin\n");
    int time = 0;
    while ((cpu_pc.rip <= 18 * 0x40 + 0x00400000) &&
           time < MAX_NUM_INSTRUCTION_CYCLE)
    {
        instruction_cycle();
#ifdef DEBUG_INSTRUCTION_CYCLE_INFO_REG_STACK
        print_register();
        print_stack();
#endif
        time ++;
    } 

    // gdb state ret from func
    assert(cpu_reg.rax == 0x6);
    assert(cpu_reg.rbx == 0x0);
    assert(cpu_reg.rcx == 0x8000650);
    assert(cpu_reg.rdx == 0x3);
    assert(cpu_reg.rsi == 0x7ffffffee318);
    assert(cpu_reg.rdi == 0x0);
    assert(cpu_reg.rbp == 0x7ffffffee230);
    assert(cpu_reg.rsp == 0x7ffffffee220);
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee230)) == 0x0000000008000650); // rbp
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee228)) == 0x0000000000000006);
    assert(cpu_read64bits_dram(va2pa(0x7ffffffee220)) == 0x00007ffffffee310); // rsp

    printf("\033[32;1m\tPass\033[0m\n");
}

int main()
{
    TestAddFunctionCallAndComputation();
    TestSumRecursiveCondition();

    finally_cleanup();
    return 0;
}

#endif
