#include<stdlib.h>
#include<stdio.h>
#include<assert.h>
#include<string.h>
#include<headers/linker.h>
#include<headers/common.h>

#define MAX_SYMBOL_MAP_LENGTH 64
#define MAX_SECTION_BUFFER_LENGTH 64

// internal mapping between source and destination symbol entries
/*  每个符号位于的elf文件，是源文件的那个条目，目标文件的那个条目，重定位信息  */
/* 其实就是dst的符号表，但是我们需要一些额外信息，比如它来自那个源文件，是哪个符号和重定位信息等 */
typedef struct
{
    elf_t       *elf;   // src elf file
    st_entry_t  *src;   // src symbol
    // TODO:
    // relocation entry (referencing section, referenced symbol) converted to (referencing symbol, referenced symbol) entry
    st_entry_t  *dst;   // dst symbol: used for relocation - find the function referencing the undefined symbol
} smap_t;

static void symbol_processing(elf_t **srcs, int num_srcs, elf_t *dst, smap_t *map, int *smap_count);
static void simple_resolution(st_entry_t *sym, elf_t *symbol_elf, smap_t *candidate);

static void compute_section_header(elf_t *src, smap_t *smap_table, int *smap_count);
static void merge_section(elf_t **srcs, int num_srcs, elf_t *dst, smap_t *smap_table, int *smap_count);


/* ------------------------------------ */
/* Exposed Interface for Static Linking */
/* ------------------------------------ */
void link_elf(elf_t **srcs, int num_srcs, elf_t *dst)
{
    // reset the destination since it's a new file
    memset(dst, 0, sizeof(elf_t));

    // create the map table to connect the source elf files and destination elf file symbols
    int smap_count = 0;
    smap_t smap_table[MAX_SYMBOL_MAP_LENGTH];

    // update the smap table - symbol processing
    symbol_processing(srcs, num_srcs, dst, (smap_t *)&smap_table, &smap_count);

    printf("--------------------------\n");

    for(int i = 0; i < smap_count; i ++ )
    {
        st_entry_t *ste = smap_table[i].src;
        debug_print(DEBUG_LINKER, "%s\t%d\t%d\t%s\t%d\t%d\n",
            ste->st_name,
            ste->bind,
            ste->type,
            ste->st_shndx,
            ste->st_value,
            ste->st_size
        );
    }

    // compute dst Section Header Table and write into buffer
    // UPDATE section header table: compute runtime address of each section
    // UPDATE buffer: EOF file header: file line count, section header table line count, section header table
    // compute running address of each section: .text, .rodata, .data, .symtab
    // eof starting from 0x400000
    compute_section_header(dst, smap_table, &smap_count);

    // malloc the dst.symt
    dst->symt = malloc(dst->sht_count * sizeof(st_entry_t));

    // to this point, the EOF file header and section header table is placed 
    // merge the left section and relocation the entries in .text and .data

    //

    // merge the symbol content from ELF src into dst sections
}

static void symbol_processing(elf_t **srcs, int num_srcs, elf_t *dst, smap_t *smap_table, int *smap_count)
{
    // for every elf files
    for (int i = 0; i < num_srcs; ++ i)
    {
        elf_t *elfp = srcs[i];

        // for every symbol from this elf file
        for (int j = 0; j < elfp->symt_count; ++ j) // 遍历符号表
        {
            st_entry_t *sym = &(elfp->symt[j]);

            // weak的情况可以忽略
            if (sym->bind == STB_LOCAL)
            {
                assert(*smap_count < MAX_SYMBOL_MAP_LENGTH);
                smap_table[*smap_count].src = sym;
                smap_table[*smap_count].elf = elfp;
                (*smap_count) ++ ;
                // insert the static (local) symbol to new elf with confidence:
                // compiler would check if the symbol is redeclared in one *.c file
                // TODO: source symbol should be used by destination symbol
            }
            else if(sym->bind == STB_GLOBAL)
            {
                // for other bind: STB_GLOBAL, etc. it's possible to have name conflict
                // check if this symbol has been cached in the map
                for (int k = 0; k < *smap_count; ++ k)
                {
                    // TODO: check name conflict
                    // what if the cached symbol is STB_LOCAL?
                    st_entry_t *candidate = smap_table[k].src;
                    if (candidate->bind == STB_GLOBAL && 
                        (strcmp(candidate->st_name, sym->st_name) == 0))
                    {
                        // having name conflict, do simple symbol resolution
                        // pick one symbol from current sym and cached map[k]
                        simple_resolution(sym, elfp, &smap_table[k]);
                        goto NEXT_SYMBOL_PROCESS;
                    }
                }
                // 没有发现名字冲突
                // cache current symbol sym to the map since there is no name conflict
                assert(*smap_count <= MAX_SYMBOL_MAP_LENGTH);
                // update map table
                smap_table[*smap_count].src = sym;
                smap_table[*smap_count].elf = elfp;
                (*smap_count) ++ ;
                // TODO: update map table
            }
            NEXT_SYMBOL_PROCESS:
            // do nothing
            ;
        }
    }
    
    // all the elf files have been processed
    // cleanup: check if there is any undefined symbols in the map table
    for (int i = 0; i < *smap_count; ++ i)
    {
        st_entry_t *s = smap_table[i].src;
        // check no more SHN_UNDEF here
        // assert(strcmp(s->st_shndx, "SHN_UNDEF") != 0);
        // assert(s->type != STT_NOTYPE);

        // 如果最后还有COMMON段，就把它划分到bss段
        // 因为全局变量默认初始化为0
        // the remaining COMMON go to .bss
        if(strcmp(s->st_shndx, "COMMON") == 0)
        {
            char *bss = ".bss";
            for(int j = 0; j < MAX_CHAR_SECTION_NAME; j ++ ) 
            {
                if(j < 4)
                {
                    s->st_shndx[j] = bss[j];
                }
                else 
                {
                    s->st_shndx[j] = '\0';
                }
            }
            s->st_value = 0; // bss段是一个“虚空”的段，设为0
            s->st_size = 0;
        }
    }
}

static inline int symbol_precedence(st_entry_t *sym)
{
    // use inline function to imporve efficiency in run-time by preprocessing
    
    // TODO: get precedence of each symbol
    /*
        我们忽略weak的情况，因为它出现的次数很稀少
        我们忽略local的情况，因为它不会出现冲突（重复）
        bind    type    shndx               prec
        -------------------------------------------------
        global  notype  undef               0 - undefined
        global  object  COMMON              1 - tentative
        global  object  data,bss,rodata     2 - defined
        global  object  text                2 - defined
    */
    

    assert(sym->bind == STB_GLOBAL);

    if (strcmp(sym->st_shndx, "SHN_UNDEF") == 0 && sym->type == STT_NOTYPE)
    {
        // Undefined: symbols referenced but not assigned a storage address
        return 0;
    }
    if(strcmp(sym->st_shndx, "COMMON") == 0 && sym->type == STT_OBJECT)
    {
        // tentative: section to be decide after sybol resolution
        return 1;
    }
    if((strcmp(sym->st_shndx, ".text")   == 0  && sym->type == STT_FUNC)      || 
       (strcmp(sym->st_shndx, ".data")   == 0  && sym->type == STT_OBJECT)    || 
       (strcmp(sym->st_shndx, ".bss")    == 0  && sym->type == STT_OBJECT)    || 
       (strcmp(sym->st_shndx, ".rodata") == 0  && sym->type == STT_OBJECT)) 
    {
        // defined
        return 2;
    }
    debug_print(DEBUG_LINKER, "symbol resolution: cannot determine the symbol \"%s\" precedence", sym->st_name);
    exit(0);
}

static void simple_resolution(st_entry_t *sym, elf_t *sym_elf, smap_t *candidate)
{


    // sym: symbol from current elf file
    // candidate    : pointer to the internal map table slot: src -> dst
    
    // determines which symbol is the one to be kept with 3 rules
    // rule 1: multiple strong symbols with the same name are not allowed
    // rule 2: given a strong symbol and multiple weak symbols with the same name, choose the strong symbol
    // rule 3: given multiple weak symbols with the same name, choose any of the weak symbols
    int pre1 = symbol_precedence(sym);
    int pre2 = symbol_precedence(candidate->src);

    /*
        00 01 02 
        11 11 12
        20 21 22
    */

    // TODO: implement rule 1: 多个强符号是不允许存在的
    if (pre1 == 2 && pre2 == 2) // 强符号的优先级为2
    {
        debug_print(DEBUG_LINKER, 
            "symbol resolution: strong symbol \"%s\" is redeclared\n", sym->st_name);
        exit(0);
    }

    // TODO: implement rule 3
    if (pre1 != 2 && pre2 != 2) // 没有强符号
    {
        // use the strong one as best match
        if(pre1 > pre2) // pre1 = 0, pre2 = 1
        {
            candidate->src = sym;
            candidate->elf = sym_elf;
        }
        // else if pre1 = pre2 = 1 or pre1 = 1, pre2 = 0 , do nothing
        return;
    }

    // rule 2 and rule 3: old src is overriden by the current src: src has higher precedence
    // rule2
    if(pre1 == 2)
    {
        // 需要改变
        candidate->src = sym;
        candidate->elf = sym_elf;
        // count 不需要改变
    }

    // else case pre1 < pre2    -> continue
}
