#include <stdio.h>
#include <string.h>
#include <headers/cpu.h>
#include <headers/memory.h>
#include <headers/common.h>

#define MAX_NUM_INSTRUCTION_CYCLE 100


/*==============================*/
/*      test procedure          */
static void TestAddFunctionCallAndComputation();
static void TestString2Uint();
static void TestSumRecursiveCondition();
void TestParseOperand();        // define at file isa.c
void TestParseInstruction();    // define at file isa.c

// interface
void write64bits_dram(uint64_t paddr, uint64_t data, core_t *cr);
uint64_t read64bits_dram(uint64_t paddr, core_t *cr);
/*          Ending               */
/*===============================*/



// symbols from isa and sram
void print_register(core_t *cr);
void print_stack(core_t *cr);

int main()
{
    printf("main start success==========>\n");
    // TestAddFunctionCallAndComputation();     // Success
    // TestString2Uint();                       // Success
    // TestParseOperand();                      // Success
    // TestParseInstruction();                  // Success
    // TestAddFunctionCallAndComputation();     // Success
    // TestSumRecursiveCondition();             // Success
    
    printf("main end success============>\n");  // Success
    return 0;
}

/*           Implementation            */
static void TestString2Uint() // Test: Successful
{
    const int n = 25;
    const char *nums[25] = { // pointers array
        "0",        // 0
        "-0",
        "0x00",
        "0x01",
        "0x000a",
        "-1",
        "00",
        "0012",
        "0x12",
        "12",       // 9
        "-12",
        "-0x12",
        "0xab",
        "-0xab",
        "2147483647",
        "2147483648",       // 15
        "-2147483647",
        "-2147483648",
        "0x8000000000000000",
        "0xffffffffffffffff",
        // "     0x     ",      // 20
        // "0 0",
        // "0xx",
        "1111",     // 23
        "-0xabcd",
        "0xabcd",
        "0x100",
        "0x010",
    };
    for(int i = 0; i < n; i ++ )
    {
        printf("%16s  <---> 0x%-16lx\n", nums[i], string2uint(nums[i]));
    }
}

static void TestAddFunctionCallAndComputation()
{
    ACTIVE_CORE = 0x0;
    core_t *ac = (core_t *)&cores[ACTIVE_CORE];

    // init state
    ac->reg.rax = 0x12340000;
    ac->reg.rbx = 0x555555555190;
    ac->reg.rcx = 0x555555555190;
    ac->reg.rdx = 0xabcd;
    ac->reg.rsi = 0x7fffffffe4b8;
    ac->reg.rdi = 0x1;
    ac->reg.rbp = 0x7fffffffe3c0;
    ac->reg.rsp = 0x7fffffffe3a0;

    ac->flags.CF = 0;
    ac->flags.OF = 0;
    ac->flags.ZF = 0;
    ac->flags.SF = 0;
    
    write64bits_dram(va2pa(0x7fffffffe3a0, ac), 0x0000000000000000, ac);    // rbp
    write64bits_dram(va2pa(0x7fffffffe3a8, ac), 0x0000000012340000, ac);
    write64bits_dram(va2pa(0x7fffffffe3b0, ac), 0x000000000000abcd, ac);
    write64bits_dram(va2pa(0x7fffffffe3b8, ac), 0x0000000000000000, ac);
    write64bits_dram(va2pa(0x7fffffffe3c0, ac), 0x0000000000000000, ac);    // rsp

    /*          debug begin             */
    /*
    printf("debug begin test physical address-value---------------------->\n");
    printf("!!!!a0-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3a0, ac), ac))); 
    printf("!!!!a8-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3a8, ac), ac))); 
    printf("!!!!b0-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3b0, ac), ac))); 
    printf("!!!!c8-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3b8, ac), ac))); 
    printf("!!!!c0-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3c0, ac), ac))); 
    printf("debug end test physical address-value------------------------->\n");
    */
    /*          debug end               */


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
        "mov    %rdx,%rsi",         // 11: main entry point
        "mov    %rax,%rdi",         // 12:
        "callq  0x400000",                 // 13: 0 means assembly[0]
        "mov    %rax,-0x8(%rbp)",   // 14
    };  
    for (int i = 0; i < 15; ++ i)
    {
        // write instructions in physical memory
        // *40 means a instruction's length is 64(fixed length)
        writeinst_dram(va2pa(i * 0x40 + 0x00400000, ac), assembly[i], ac);
    }
    ac->rip = MAX_INSTRUCTION_CHAR * sizeof(char) * 11 + 0x00400000;
    // sprintf(assembly[13], "callq  $%p", &assembly[0]);
    
    print_register(ac);
    printf("Assemble Program Begin Run----->\n");

    int time = 0;
    while (time < 15)
    {
        /*  if you want to observe the dynamic changes of register and stack, 
            please modify VERBOSE in common.c and set BERBOSES = 0x6 will success */
        // printf("**Inst[%d]: ", time);
        instruction_cycle(ac);
        print_register(ac);
        print_stack(ac);
        time ++;
        // printf("\n");
    } 
    printf("Assemble Program End Run----->\n");

    // gdb state ret from func
    int match = 1;
    match = match && ac->reg.rax == 0x1234abcd;         if(!match)  {printf("!!!!rax=0x%16lx!\n", ac->reg.rax);}
    match = match && ac->reg.rbx == 0x555555555190;     if(!match)  {printf("!!!!rbx=0x%16lx!\n", ac->reg.rbx);}
    match = match && ac->reg.rcx == 0x555555555190;     if(!match)  {printf("!!!!rcx=0x%16lx!\n", ac->reg.rcx);}
    match = match && ac->reg.rdx == 0x12340000;         if(!match)  {printf("!!!!rdx=0x%16lx!\n", ac->reg.rdx);}
    match = match && ac->reg.rsi == 0xabcd;             if(!match)  {printf("!!!!rsi=0x%16lx!\n", ac->reg.rsi);}
    match = match && ac->reg.rdi == 0x12340000;         if(!match)  {printf("!!!!rdi=0x%16lx!\n", ac->reg.rdi);}
    match = match && ac->reg.rbp == 0x7fffffffe3c0;     if(!match)  {printf("!!!!rbp=0x%16lx!\n", ac->reg.rbp);}
    match = match && ac->reg.rsp == 0x7fffffffe3a0;     if(!match)  {printf("!!!!rsp=0x%16lx!\n", ac->reg.rsp);}

    if (match)
    {
        printf("register match\n");
    }
    else
    {
        printf("register mismatch\n");
    }

    match = match && (read64bits_dram(va2pa(0x7fffffffe3a0, ac), ac) == 0x0000000000000000); // rbp
    if(!match)  { printf("!!!!a0-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3a0, ac), ac))); }
    match = match && (read64bits_dram(va2pa(0x7fffffffe3a8, ac), ac) == 0x0000000012340000);
    if(!match)  { printf("!!!!a8-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3a8, ac), ac))); }
    match = match && (read64bits_dram(va2pa(0x7fffffffe3b0, ac), ac) == 0x000000000000abcd);
    if(!match)  { printf("!!!!b0-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3b0, ac), ac))); }
    match = match && (read64bits_dram(va2pa(0x7fffffffe3a8, ac), ac) == 0x0000000012340000);
    if(!match)  { printf("!!!!b8-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3b8, ac), ac))); }
    match = match && (read64bits_dram(va2pa(0x7fffffffe3c0, ac), ac) == 0x0000000000000000); // rsp
    if(!match)  { printf("!!!!c0-> 0x%lx !\n", (read64bits_dram(va2pa(0x7fffffffe3c0, ac), ac))); }

    if (match)
    {
        printf("memory match\n");
    }
    else
    {
        printf("memory mismatch\n");
    }
}

static void TestSumRecursiveCondition()
{
    ACTIVE_CORE = 0x0;
    core_t *cr = (core_t *)&cores[ACTIVE_CORE];

    // init state
    cr->reg.rax = 0x8000630;
    cr->reg.rbx = 0x0;
    cr->reg.rcx = 0x8000650;
    cr->reg.rdx = 0x7ffffffee328;
    cr->reg.rsi = 0x7ffffffee318;
    cr->reg.rdi = 0x1;
    cr->reg.rbp = 0x7ffffffee230;
    cr->reg.rsp = 0x7ffffffee220;

    cr->flags.__cpu_flag_value = 0;
   
    write64bits_dram(va2pa(0x7ffffffee230, cr), 0x0000000008000650, cr);    // rbp
    write64bits_dram(va2pa(0x7ffffffee228, cr), 0x0000000000000000, cr);
    write64bits_dram(va2pa(0x7ffffffee220, cr), 0x00007ffffffee310, cr);    // rsp

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
        "mov    $0x3,%edi",         // 16: entry main
        "callq  0x00400000",        // 17
        "mov    %rax,-0x8(%rbp)",   // 18
    };


    for (int i = 0; i < 19; ++ i)
    {
        // write instructions in physical memory
        // *40 means a instruction's length is 64(fixed length)
        writeinst_dram(va2pa(i * 0x40 + 0x00400000, cr), assembly[i], cr);
    }
    cr->rip = MAX_INSTRUCTION_CHAR * sizeof(char) * 16 + 0x00400000;
    
    printf("begin curision--------->\n");
    int time = 0;
    while((cr->rip <= 18 * 0x40 + 0x00400000) && time < MAX_NUM_INSTRUCTION_CYCLE)
    {
        instruction_cycle(cr);
        print_register(cr);
        print_stack(cr);
        time ++;
    }

    // gdb state ret from func
    int match = 1;
    match = match && cr->reg.rax == 0x6;                if(!match)  {printf("!!!!rax=0x%16lx!\n", cr->reg.rax);} 
    match = match && cr->reg.rbx == 0x0;     if(!match)  {printf("!!!!rbx=0x%16lx!\n", cr->reg.rbx);}
    match = match && cr->reg.rcx == 0x8000650;     if(!match)  {printf("!!!!rcx=0x%16lx!\n", cr->reg.rcx);}
    match = match && cr->reg.rdx == 0x3;                if(!match)  {printf("!!!!rdx=0x%16lx!\n", cr->reg.rdx);}
    match = match && cr->reg.rsi == 0x7ffffffee318;     if(!match)  {printf("!!!!rsi=0x%16lx!\n", cr->reg.rsi);}
    match = match && cr->reg.rdi == 0x0;                if(!match)  {printf("!!!!rdi=0x%16lx!\n", cr->reg.rdi);}
    match = match && cr->reg.rbp == 0x7ffffffee230;     if(!match)  {printf("!!!!rbp=0x%16lx!\n", cr->reg.rbp);}
    match = match && cr->reg.rsp == 0x7ffffffee220;     if(!match)  {printf("!!!!rsp=0x%16lx!\n", cr->reg.rsp);}
    
    if (match)
    {
        printf("register match\n");
    }
    else
    {
        printf("register mismatch\n");
    }

    match = match && (read64bits_dram(va2pa(0x7ffffffee230, cr), cr) == 0x0000000008000650); // rbp
    match = match && (read64bits_dram(va2pa(0x7ffffffee228, cr), cr) == 0x0000000000000006);
    match = match && (read64bits_dram(va2pa(0x7ffffffee220, cr), cr) == 0x00007ffffffee310); // rsp

    if (match)
    {
        printf("memory match\n");
    }
    else
    {
        printf("memory mismatch\n");
    }
}