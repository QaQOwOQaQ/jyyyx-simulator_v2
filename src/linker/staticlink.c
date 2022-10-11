#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <headers/common.h>
#include <headers/linker.h>
#include <headers/instruction.h>

#define MAX_SYMBOL_MAP_LENGTH (64)      
#define MAX_SECTION_BUFFER_LENGTH (64)  
#define MAX_RELOCATION_LINES (64)       // 最大重定位信息个数


// internal mapping between source and destination symbol entries
// 经过重定位之后的elf文件中的符号s，它属于那个源文件(*elf)，
// 在源文件符号表的那个条目中(*src)，在当前文件符号表的那个条目中(*dst)
typedef struct 
{
    elf_t *elf;         // src elf file
    st_entry_t *src;    // src symbol
    st_entry_t *dst;    // dst symbol: used for relocation - find the function referencing the undefined symbol
} smap_t; // symbol map



/* ------------------------------------ */
/* Symbol Processing                    */
/* ------------------------------------ */
// 符号处理
static void symbol_processing(elf_t **srcs, int num_srcs, elf_t *dst, 
    smap_t *smap_table, int *smap_count);
static void simple_resolution(st_entry_t *sym, elf_t *sym_elf, smap_t *candidate);

/* ------------------------------------ */
/* Section Merging                      */
/* ------------------------------------ */
// 可重定位目标文件中的相同节合并成可执行目标文件中的段
static void compute_section_header(elf_t *dst, smap_t *smap_table, int *smap_count);
static void merge_section(elf_t **srcs, int num_srcs, elf_t *dst, 
    smap_t *smap_table, int *smap_count);


/* ------------------------------------ */
/* Relocation                           */
/* ------------------------------------ */
// 重定位
static void relocation_processing(elf_t **srcs, int num_srcs, elf_t *dst,
    smap_t *smap_table, int *smap_count);
static void R_X86_64_32_handler(elf_t *dst, sh_entry_t *sh,
    int row_referencing, int col_referencing, int addend,
    st_entry_t *sym_referenced);
static void R_X86_64_PC32_handler(elf_t *dst, sh_entry_t *sh,
    int row_referencing, int col_referencing, int addend,
    st_entry_t *sym_referenced);

typedef void(*rela_handler_t)(elf_t *dst, sh_entry_t *sh,
    int row_referencing, int col_referencing, int addend,
    st_entry_t *sym_referenced);

static rela_handler_t handler_table[3] = {
    &R_X86_64_32_handler,       // 0
    &R_X86_64_PC32_handler,     // 1
    // linux commit b21ebf2: x86: Treat R_X86_64_PLT32 as R_X86_64_PC32
    // https://github.com/torvalds/linux/commit/b21ebf2fb4cde1618915a97cc773e287ff49173e
    &R_X86_64_PC32_handler,     // 2 PLT32 <==> PC32
};

/* ------------------------------------ */
/* Helper                               */
/* ------------------------------------ */
static const char* get_stb_string(st_bind_t bind);  // symbol table bind
static const char* get_stt_string(st_type_t type);  // symbol table type


/* ------------------------------------ */
/* Exposed Interface for Static Linking */
/* ------------------------------------ */
void link_elf(elf_t **srcs, int num_srcs, elf_t *dst)   //将num_srcs个源文件链接在一起放到dst中
{   
    printf("-------------for debug: begin link!-----------------\n");
    // reset the destination since it's a new file
    memset(dst, 0, sizeof(elf_t));

    // create the map table
    // create the smap table to connect the source elf files and destination elf file symbols
    int smap_count = 0;
    smap_t smap_table[MAX_SYMBOL_MAP_LENGTH];

    // update the smap table - symbol processing
    // 处理所有文件中的所有符号，并放入symbol map当中
    symbol_processing(srcs, num_srcs, dst, (smap_t *)&smap_table, &smap_count);

    printf("--------------smap---------------------------\n");

    for(int i = 0; i < smap_count; i ++ )
    {
        st_entry_t *ste = smap_table[i].src;
        debug_print(DEBUG_LINKER, "[smap]: %s\t%d\t%d\t%s\t%d\t%d\n",
            ste->st_name,
            ste->bind,
            ste->type,
            ste->st_shndx,
            ste->st_value,
            ste->st_size);
    }   

    // compute dstination Section Header Table and write into buffer
    // UPDATE section header table: computer runtime address of each section
    // UPDATE buffer: EOF file header: file line count, section header table line count, section table
    // compute running address of each section: .text, .rodata, .data, .symtab
    // eof starting from 0x400000
    compute_section_header(dst, smap_table, &smap_count);
    // 经过compute_section_header，我们已经解决了elf文件中的：line_count，sht_count，sht。
    // 还需要解决符号表信息和重定位信息。

    // malloc the dst.symt
    dst->symt_count = smap_count;
    dst->symt = malloc(dst->symt_count * sizeof(st_entry_t));

    // to this point, the EOF file header and section header table is placed （此时，放置了ELF文件头和节头部表
    // merge the left sections and relocate the entries in .text and .data（合并左侧部分并重新定位 .text 和 .data 中的条目
    
    // merge the symbol content from ELF src into dst sections
    // 解决符号表信息symt_count, symt
    merge_section(srcs, num_srcs, dst, smap_table, &smap_count);

    printf("----------------------------\n");
    printf("after merging the sections\n");
    for(int i = 0; i < dst->line_count; i ++ )
    {
        printf("%s\n", dst->buffer[i]);
    }

    // relocation: update the relocation entries from ELF files into EOF buffer
    // 最后，解决重定位信息
    relocation_processing(srcs, num_srcs, dst, smap_table, &smap_count);

    // finally, check EOF file
    if((DEBUG_VERBOSE_SET & DEBUG_LINKER) != 0)
    {
        printf("------\nFinal output EOF:\n");
        for(int i = 0; i < dst->line_count; i ++ )
        {
            printf("%s\n", dst->buffer[i]);
        }
    }
}


// 符号处理，** 将结果放在映射表smap中
static void symbol_processing(elf_t **srcs, int num_srcs, elf_t *dst, smap_t *smap_table, int *smap_count)
{
    // for every elf files
    for(int i = 0; i < num_srcs; i ++ )
    {
        elf_t *elfp = srcs[i]; // 取得这个文件

        // for every symbol from this elf file
        for(int j = 0; j < elfp->symt_count; j ++ )
        {
            st_entry_t *sym = &(elfp->symt[j]); // 取得这个文件中的符号
            if(sym->bind == STB_LOCAL)
            {
                // insert the static (local) symbol to new elf with confidence
                // complier would check if the symbol is redeclared in one *.c file(因为编译器会解决重定义的问题,所以在链接时就需要考虑了(肯定不会重定义,不然就不会通过编译))
                assert((*smap_count) < MAX_SYMBOL_MAP_LENGTH);
                // even if local symbol has the same name, just insert in into dst
                smap_table[*smap_count].elf = elfp;
                smap_table[*smap_count].src = sym;
                // we have not create dst here
                (*smap_count) ++ ;
            }
            else if(sym->bind == STB_GLOBAL)
            {
                // 此时可能存在符号冲突

                // for other bind: STB_GLOBAL, etc. It's possible have name conflict
                // check if this symbol has been cached in the map
                for(int k = 0; k < (*smap_count); k ++ ) // 遍历已经添加到smap中的符号check是否矛盾
                {
                    // check name conflict 
                    // what if the cached symbol is STB_LOCAL? I don't know, Error?
                    st_entry_t *candidate = smap_table[k].src; // 取得这个符号,作为“候选符号”
                    if(candidate->bind == STB_GLOBAL && 
                        (strcmp(candidate->st_name, sym->st_name) == 0)) // 如果这个候选符号与当前符号冲突
                    {
                        // having name conflict, do simple symbol resolution
                        // pick one symbol from current sym and cached map[k]
                        simple_resolution(sym, elfp, &smap_table[k]); // 符号决议，选择一个优先级更高的放到smap当中
                        goto NEXT_SYMBOL_PROCESS;
                    }
                }

                // not find ang name conflict 
                // cached current symbol sym to the map since there is no name conflict 
                assert((*smap_count) <= MAX_SYMBOL_MAP_LENGTH);
                // update map table
                smap_table[*smap_count].elf = elfp;
                smap_table[*smap_count].src = sym;
                (*smap_count) ++ ;
            }
            NEXT_SYMBOL_PROCESS : 
            // do nothing
            ;
        }
    }

    // all the elf files have been processed 
    // cleanup: check if there is any undefined symbols in the map table
    // 处理结束之后，可能还存在没有解决的符号，例如未初始化的全局变量，它还在COMMON节中，需要将它放在.bss节中
    for(int i = 0; i < (*smap_count); i ++ )
    {
        st_entry_t *s = smap_table[i].src;

        // check no more SHN_UNDEF here
        assert(strcmp(s->st_shndx, "SHN_UNDEF") != 0);
        assert(s->type != STT_NOTYPE);

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
            s->st_value = 0;
        }
    }
}

static inline int symbol_priority(st_entry_t *sym) // 作者定义的是：symbol_precedence
{
    /* 优先选择优先级高的 */
    // use inline function to improve efficiency in run-time by preprocessing
    /* we do not consider weak because is's very rare
        and we do not consider local because it's not conflicting

            bind        type        shndx               prec
            --------------------------------------------------
            global      notype      undef               0 - undefined
            global      object      common              1 - tentative
            global      object      data,bss,rodata     2 - defined
            global      func        text                2 - defined        
    */
   assert(sym->bind == STB_GLOBAL);

   // get priority of the symbol
   if(strcmp(sym->st_shndx, "SHN_UNDEF") == 0 && sym->type == STT_NOTYPE)
   {
        // Undefined: symbol referenced but not assigned a storage address
        return 0;
   }

   if(strcmp(sym->st_shndx, "COMMON") == 0 && sym->type == STT_OBJECT)
   {
        // Tentative: section to be decide after symbol resolution
        return 1;
   } 
   if((strcmp(sym->st_shndx, ".text")   == 0 && sym->type == STT_FUNC)   || 
      (strcmp(sym->st_shndx, ".data")   == 0 && sym->type == STT_OBJECT) || 
      (strcmp(sym->st_shndx, ".rodata") == 0 && sym->type == STT_OBJECT) || 
      (strcmp(sym->st_shndx, ".bss")    == 0 && sym->type == STT_OBJECT))
    {
        // Defined
        return 2;
    }

    debug_print(DEBUG_LINKER, "symbol resolution: cannot determine the symbol \"%s\" precedence", sym->st_name);
    exit(0);
}

static void simple_resolution(st_entry_t *sym, elf_t *sym_elf, smap_t *candidate) // 矛盾符号的简单决议
{// elf文件(sym_elf)中的符号(sym)与已经在smap中的符号(candidate)冲突，解决冲突并把优先级更高的符号放到smap(candidate)中。

    // sym: symbol from current elf file
    // candidate: pointer to the internal map table slot: src -> dst  (指向内部映射表槽的指针:src->dst
    
    // rule1: multiple strong symbols with the same name are not allowed
    // rule2: given a strong symbol and multiple weak symbols with the same name, choose the strong symbol
    // rule3: given multiple weak symbols with the same name, choose any of the weak symbols

    // First, get the priority of the two symbols
    int pre1 = symbol_priority(sym);
    int pre2 = symbol_priority(candidate->src);
    
    // 将优先级更高的保存到candidate
    /*  all possible cases
        00,01,02
        10,11,12
        20,21,22 */

    if(pre1 == 2 && pre2 == 2) // case: 22,
    { 
        // 出现多个强符号，Error
        debug_print(DEBUG_LINKER, "symbol resolution: strong symbol \"%s\" is redeclared as \"%s\"\n", sym->st_name, candidate->src->st_name);
        exit(0);
    }
    else if(pre1 != 2 && pre2 != 2) // case: 00, 01, 10, 11
    {
        // 没有强符号,如果原来的符号优先级更高，返回原符号(此时需要替换candidate)
        // 否则，直接返回candidate，什么都不做
        if(pre1 > pre2)
        {
            // select sym as best match
            candidate->src = sym;
            candidate->elf = sym_elf;
        }
        // else pre1 < pre2  
        return ;
    }
    else if(pre1 == 2) // case: 20, 21
    {
        // replace candidate
        candidate->src = sym;
        candidate->elf = sym_elf;
    }   
    // else  pre2 == 2 // case: 02, 12
    // select candidate, do nothing
}

static void compute_section_header(elf_t *dst, smap_t *smap_table, int *smap_count)
{
/*  
    在此之前，我们已经把所有文件的所有符号处理到smap_table(symbol table map)中，接下来我们需要根据这些符号
    计算出新的elf文件需要的总行数，节的个数，每个节的信息
    并将上述信息写入到目标elf文件(dst)中。
*/

    // we only have .text, .rodata, .data as symbols in the section
    // .bss is not taking any section memory
    int count_text = 0, count_rodata = 0, count_data = 0;
    for(int i = 0; i < *smap_count; i ++ )
    {
        st_entry_t *sym = smap_table[i].src;
        if(strcmp(sym->st_shndx, ".text") == 0)
        {
            count_text += sym->st_size;
        }
        else if(strcmp(sym->st_shndx, ".rodata") == 0)
        {
            count_rodata += sym->st_size;
        }
        else if(strcmp(sym->st_shndx, ".data") == 0)
        {
            count_data += sym->st_size;
        }
    }

    printf("debug_count: %d-%d-%d\n", count_text, count_rodata, count_data);

    // count the section: with .symbol table
    dst->sht_count = 1 + (count_text != 0) + (count_rodata != 0) + (count_data != 0); // 1 means .symtab(总不能没有符号吧)
    // count the total lines
    dst->line_count = 1 + 1 + dst->sht_count + count_text + count_rodata + count_data + *smap_count;
    // the target dst: line_count, sht_count, sht, .text, .rodata, .data, .symbol
    // print(write) to buffer
    sprintf(dst->buffer[0], "%ld", dst->line_count);
    sprintf(dst->buffer[1], "%ld", dst->sht_count);

    // compute the run-time address of the sections: compact in memory
    uint64_t text_runtime_addr = 0x00400000;
    uint64_t rodata_runtime_addr = text_runtime_addr + count_text * MAX_INSTRUCTION_CHAR * sizeof(char);
    uint64_t data_runtime_addr = rodata_runtime_addr + count_rodata * sizeof(uint64_t);
    uint64_t symtab_runtime_addr = 0; // For EOF, .symtab is not loaded into run-time memory but still on disk

    // write the section header table
    assert(dst->sht == NULL);
    dst->sht = malloc(dst->sht_count * sizeof(sh_entry_t)); // 为section header table申请空间

    // write in .text, .rodata, .data order
    uint64_t section_offset = 1 + 1 + dst->sht_count;
    int sh_index = 0;
    sh_entry_t *sh = NULL;
    if(count_text > 0)
    {
        // get the pointer
        assert(sh_index < dst->sht_count);
        sh = &(dst->sht[sh_index]); // 指针指向我们在堆中申请的空间

        // write the fields
        // 想堆中写入数据
        strcpy(sh->sh_name, ".text");
        sh->sh_addr   = text_runtime_addr;
        sh->sh_offset = section_offset;
        sh->sh_size   = count_text;

        // write to buffer
        sprintf(dst->buffer[2 + sh_index], "%s,0x%lx,%ld,%ld", 
            sh->sh_name, sh->sh_addr, sh->sh_offset, sh->sh_size);
        
        // update the index
        sh_index ++ ;
        section_offset += sh->sh_size;
    }
    // else skip .text
    
    if(count_rodata > 0)
    {
        // get the pointer
        assert(sh_index < dst->sht_count);
        sh = &(dst->sht[sh_index]);

        // write the fileds(写字段)
        strcpy(sh->sh_name, ".data");
        sh->sh_addr   = rodata_runtime_addr;
        sh->sh_offset = section_offset;
        sh->sh_size   = count_rodata;

        // write to buffer
        sprintf(dst->buffer[2 + sh_index], "%s,0x%lx,%ld,%ld",
            sh->sh_name, sh->sh_addr, sh->sh_offset, sh->sh_size);

        // update the index
        sh_index ++ ;
        section_offset += sh->sh_size;
    }
    // else skip .rodata

    if(count_data)
    {
        //get the pointer
        assert(sh_index < dst->sht_count);
        sh = &(dst->sht[sh_index]);

        // write the fields
        strcpy(sh->sh_name, ".data");
        sh->sh_addr   = data_runtime_addr;
        sh->sh_offset = section_offset;
        sh->sh_size   = count_data;

        // write to buffer
        sprintf(dst->buffer[2 + sh_index], "%s,0x%lx,%ld,%ld",
            sh->sh_name, sh->sh_addr, sh->sh_offset, sh->sh_size);
        // update the index
        sh_index ++ ;
        section_offset += sh->sh_size;
    }
    // else skip .data


    // .symtab
    // get the poiner
    assert(sh_index < dst->sht_count);
    sh = &(dst->sht[sh_index]);

    // write the fields
    strcpy(sh->sh_name, ".symtab");
    sh->sh_addr   = symtab_runtime_addr;
    sh->sh_offset = section_offset;
    sh->sh_size   = *smap_count;

    // 已经写完了节头部表的信息:.text, .rodata, .data, .symtab
    assert(sh_index + 1 == dst->sht_count);

    // print and check
    if((DEBUG_VERBOSE_SET & DEBUG_LINKER) != 0)
    {
        printf("------------------------\n");
        printf("Destination ELF's SHT in Buffer:\n");
        for(int i = 0; i < 2 + dst->sht_count; i ++ )
        {
            printf("%s\n", dst->buffer[i]);
        }
    }
}


// precodition: dst should konw the section offset of each section (前置条件，dst应该知道每个section的offset)
// merge the target section lines from ELF files and update dst symtab (合并ELF文件的目标section并且更新dst符号表)
static void merge_section(elf_t **srcs, int num_srcs, elf_t *dst, smap_t *smap_table, int *smap_count)
{
    /*
        我们的目的是合并所有的节：[1].text [2].rodata [3].data [4].symbol
        因此，我们首先要枚举每个节，找到每个文件在这个节当中的所有符号
        所以，首先枚举每个文件，先查找一个该文件是否存在这个节，如果不存在，直接遍历下一个文件
        否则，遍历改文件当前节中的所有符号
        完成之后，除了重定位信息的所有工作就都完成了
    */

    int line_written = 1 + 1 + dst->sht_count; // 
    int symt_written = 0;
    int sym_section_offset = 0;

    for(int section_index = 0; section_index < dst->sht_count; section_index ++ ) // (1)枚举每个section
    {
        // merge by the dst.sht order in symbol unit
        // get the section by section id
        sh_entry_t *target_sh = &dst->sht[section_index];
        sym_section_offset = 0;
        debug_print(DEBUG_LINKER, "[[%d]]merging section '%s'\n", section_index, target_sh->sh_name);

        // merge the section
        for(int i = 0; i < num_srcs; i ++ ) // (2)扫描每个文件
        {
            debug_print(DEBUG_LINKER, "\tfrom source elf [%d]\n", i);
            int src_section_index = -1;
            // scan every section in this elf
            for(int j = 0; j < srcs[i]->sht_count; j ++ ) // (3)判断该文件是否存在这个section，如果存在保存下标
            {
                // check if this ELF srcs[i] contains the same section as target_sh
                if(strcmp(target_sh->sh_name, srcs[i]->sht[j].sh_name) == 0)
                {
                    // we have found the same section name
                    src_section_index = j;
                    // break;
                }
            }

            // check if we have found this target section from src ELF
            if(src_section_index == -1)     // (3.1)当前不存在这个section，直接开始遍历下一个文件
            {
                // search for next EFL
                // because the current ELF srcs[o] does not contain the target_section header
                continue;
            }
            else                            // (3.2)存在这个section
            {
                // found the section int this ELF srcs[i]
                // check its symtab
                for(int j = 0; j < srcs[i]->symt_count; j ++ )      // (4)遍历当前文件中该section的所有符号
                {
                    // 取得当前symbol
                    st_entry_t *sym = &srcs[i]->symt[j];
                    // 如果当前符号所在节为枚举的section
                    if(strcmp(sym->st_shndx, target_sh->sh_name) == 0)
                    {
                        for(int k = 0; k < (*smap_count); k ++ )
                        {
                            // scan the cached dst symbols to check
                            // if this symbol should be merged into this section
                            if(sym == smap_table[k].src)                    // (4)将符号源信息写入buffer,并且将解析后的信息写入到symbol table
                            {
                                // exactly the cached symbol
                                debug_print(DEBUG_LINKER, "\t\tsymbol '%s'\n", sym->st_name);

                                // this symbol should be merged into dst's secton target_sh
                                // copy this symbol from srcs[i].buffer into dst.buffer
                                // srcs[i].buffer[sh_offset + st_value, sh_offset + st_value + st_size] inclusive
                                for(int t = 0; t < sym->st_size; t ++ )           
                                {
                                    int dst_index = line_written + t;
                                    int src_index = srcs[i]->sht[src_section_index].sh_offset + sym->st_value + t;

                                    assert(dst_index < MAX_ELF_FILE_LENGTH);
                                    assert(src_index < MAX_ELF_FILE_LENGTH);

                                    strcpy(dst->buffer[dst_index], srcs[i]->buffer[src_index]);
                                }
                                                                                 
                                // copy the symbol table entry from srcs[i].symt[j] to dst.symt[symt_written]
                                assert(symt_written < dst->symt_count);
                                // copy the entry
                                strcpy(dst->symt[symt_written].st_name, sym->st_name);
                                dst->symt[symt_written].bind = sym->bind;
                                dst->symt[symt_written].type = sym->type;
                                strcpy(dst->symt[symt_written].st_shndx, sym->st_shndx);
                                // MUST NOT BE A COMMON, so the section offset MUST NOT BE alligment
                                dst->symt[symt_written].st_value = sym_section_offset;
                                dst->symt[symt_written].st_size = sym->st_size;

                                // update the samp_table
                                // this will help the relocation
                                smap_table[k].dst = &dst->symt[symt_written];
                                                                                    
                                // update the counter
                                symt_written += 1;
                                line_written += sym->st_size;
                                sym_section_offset += sym->st_size;
                            }
                        }
                        // symbol srcs[o].symt[j] has been checked
                    }
                }
                // all symbols in ELF file srcs[i] has been checked
            }
        }
        // dst.sht[section_index] has been merged from src ELFs
    }
    // all .text, .rodata, .data sections in dst has been merged

    // finally, merge .symtab

    // print
    for(int i = 0; i < dst->symt_count; i ++ )
    {
        st_entry_t *sym = &dst->symt[i];
        sprintf(dst->buffer[line_written], "%s,%s,%s,%s,%ld,%ld", 
            sym->st_name, get_stb_string(sym->bind), get_stt_string(sym->type),
            sym->st_shndx, sym->st_value, sym->st_size);
        line_written ++ ;
    }
    assert(line_written == dst->line_count);
}

// precondition: smap_table.dst is valid
static void relocation_processing(elf_t **srcs, int num_srcs, elf_t *dst,
    smap_t *smap_table, int *smap_count)
{
    sh_entry_t *eof_text_sh = NULL;
    sh_entry_t *eof_data_sh = NULL;
    for (int i = 0; i < dst->sht_count; ++ i)
    {
        if (strcmp(dst->sht[i].sh_name, ".text") == 0)
        {
            eof_text_sh = &(dst->sht[i]);
        }
        else if (strcmp(dst->sht[i].sh_name, ".data") == 0)
        {
            eof_data_sh = &(dst->sht[i]);
        }
    }

    // update the relocation entries: r_row, r_col, sym
    for (int i = 0; i < num_srcs; ++ i)
    {
        elf_t *elf = srcs[i];

        // .rel.text
        for (int j = 0; j < elf->reltext_count; ++ j)
        {
            rl_entry_t *r = &elf->reltext[j];

            // search the referencing symbol
            for (int k = 0; k < elf->symt_count; ++ k)
            {
                st_entry_t *sym = &elf->symt[k];

                if (strcmp(sym->st_shndx, ".text") == 0)
                {
                    // must be referenced by a .text symbol
                    // check if this symbol is the one referencing
                    int sym_text_start = sym->st_value;
                    int sym_text_end = sym->st_value + sym->st_size - 1;

                    if (sym_text_start <= r->r_row && r->r_row <= sym_text_end)
                    {
                        // symt[k] is referencing reltext[j].sym
                        // search the smap table to find the EOF location
                        int smap_found = 0;
                        for (int t = 0; t < *smap_count; ++ t)
                        {
                            if (smap_table[t].src == sym)
                            {
                                smap_found = 1;
                                st_entry_t *eof_referencing = smap_table[t].dst;

                                // search the being referenced symbol
                                for (int u = 0; u < *smap_count; ++ u)
                                {
                                    // what is the EOF symbol name?
                                    // how to get the referenced symbol name
                                    if (strcmp(elf->symt[r->sym].st_name, smap_table[u].dst->st_name) == 0 &&
                                        smap_table[u].dst->bind == STB_GLOBAL)
                                    {
                                        // till now, the referencing row and referenced row are all found
                                        // update the location
                                        st_entry_t *eof_referenced = smap_table[u].dst;

                                        (handler_table[(int)r->type])(
                                            dst, eof_text_sh,
                                            r->r_row - sym->st_value + eof_referencing->st_value, 
                                            r->r_col, 
                                            r->r_addend,
                                            eof_referenced);
                                        goto NEXT_REFERENCE_IN_TEXT;
                                    }
                                }
                            }
                        }
                        // referencing must be in smap_table
                        // because it has definition, is a strong symbol
                        assert(smap_found == 1);
                    }
                }
            }
            NEXT_REFERENCE_IN_TEXT:
            ;
        }

        // .rel.data
        for (int j = 0; j < elf->reldata_count; ++ j)
        {
            rl_entry_t *r = &elf->reldata[j];

            // search the referencing symbol
            for (int k = 0; k < elf->symt_count; ++ k)
            {
                st_entry_t *sym = &elf->symt[k];

                if (strcmp(sym->st_shndx, ".data") == 0)
                {
                    // must be referenced by a .data symbol
                    // check if this symbol is the one referencing
                    int sym_data_start = sym->st_value;
                    int sym_data_end = sym->st_value + sym->st_size - 1;

                    if (sym_data_start <= r->r_row && r->r_row <= sym_data_end)
                    {
                        // symt[k] is referencing reldata[j].sym
                        // search the smap table to find the EOF location
                        int smap_found = 0;
                        for (int t = 0; t < *smap_count; ++ t)
                        {
                            if (smap_table[t].src == sym)
                            {
                                smap_found = 1;
                                st_entry_t *eof_referencing = smap_table[t].dst;

                                // search the being referenced symbol
                                for (int u = 0; u < *smap_count; ++ u)
                                {
                                    // what is the EOF symbol name?
                                    // how to get the referenced symbol name
                                    if (strcmp(elf->symt[r->sym].st_name, smap_table[u].dst->st_name) == 0 &&
                                        smap_table[u].dst->bind == STB_GLOBAL)
                                    {
                                        // till now, the referencing row and referenced row are all found
                                        // update the location
                                        st_entry_t *eof_referenced = smap_table[u].dst;

                                        (handler_table[(int)r->type])(
                                            dst, eof_data_sh,
                                            r->r_row - sym->st_value + eof_referencing->st_value, 
                                            r->r_col, 
                                            r->r_addend,
                                            eof_referenced);
                                        goto NEXT_REFERENCE_IN_DATA;
                                    }
                                }
                            }
                        }
                        // referencing must be in smap_table
                        // because it has definition, is a strong symbol
                        assert(smap_found == 1);
                    }
                }
            }
            NEXT_REFERENCE_IN_DATA:
            ;
        }
    }
}


// relocation handlers
static uint64_t get_symbol_runtime_address(elf_t *dst, st_entry_t *sym)
{
    // TODO: get the run-time address of symbol
    // 首先要说明的是，我们的计算模型和官方的不一样
    // 在标准形式的elf中，如果两个符号的地址原本相差0x18，那么在加载之后依然是0x18
    // 但是我们的不一样，因为我们用一些结构体封装了一些信息，导致他们之间的大小发生了变化 
    // 更具体的来说，是每个条目的大小发生了变化
    // 例如汇编指令被我们封装在 inst_t 中
    // 可能原本两条汇编指令的地址相差 0x8
    // 现在变成了 sizeof(inst_t)，并且是定长的
    // 所有指令定长不太合适，哎，谁让我们的模型简单呢

    /*  ----------------------高地址 -------------------*/
    /* |--------------------| ：不考虑                  */
    /* |--------bss---------| ：不占空间                */
    /* |--------data--------| ：每个条目uint64_t，8字节  */
    /* |--------rodata------| ：每个条目uint64_t，8字节  */
    /* |--------text--------| ：每个条目sizeof(inst_t)  */
    /*  ----------------------低地址 ------------------ */

    // 从我们的简易模型中可以看出，如果想计算data节中的符号信息，就必须先计算出text和rodata节的总偏移：enyry_count * sizeof(entry)
    
    uint64_t base = 0x00400000;

    uint64_t text_base = base;
    uint64_t rodata_base = base;
    uint64_t data_base = base;

    int inst_size = sizeof(inst_t);
    int data_size = sizeof(uint64_t);

    // 先计算elf文件中每个section的偏移地址
    // must visit in .text, .rodata, .data in order
    sh_entry_t *sht = dst->sht;
    for(int i = 0; i < dst->sht_count; i ++ )
    {
        if(strcmp(sht[i].sh_name, ".text") == 0)
        {
            rodata_base = text_base + sht[i].sh_size * inst_size;
            data_base = rodata_base; // 其实就是不知道
        }
        else if(strcmp(sht[i].sh_name, ".rodata") == 0)
        {
            data_base = rodata_base + sht[i].sh_size * data_size;
        }
    }

    // 通过上面计算得到的section地址和当前符号在符号在section中的偏移(st_value)
    // 在我们的模型中，偏移量是根据行数确定的，太简陋了hh
    // check this symbol section
    if(strcmp(sym->st_shndx, ".text") == 0)
    {
        return text_base + inst_size * sym->st_value;
    }
    else if(strcmp(sym->st_shndx, ".rodata") == 0)
    {
        return rodata_base + data_base * sym->st_value;
    }   
    else if(strcmp(sym->st_shndx, ".data") == 0)
    {
        return data_base + data_size * sym->st_value;
    }
    
    // never go there
    printf("You shoud not go there wht get the symbol's run-time address!And you \n");
    return 0xFFFFFFFFFFFFFFFF;
}

static void write_relocation(char *dst, uint64_t val)
{
    char temp[20];
    sprintf(temp, "0x%016lx", val); // 这里不能直接sprintf到s中，因为sprintf会在字符串末尾添加'\0'结束导致数字后面的字符无法显示
    for(int i = 0; i < 18; i ++ )
    {
        dst[i] = temp[i];
    }
}

// 绝对寻址??
static void R_X86_64_32_handler(elf_t *dst, sh_entry_t *sh, int row_referencing, int col_referencing, int addend, st_entry_t *sym_referenced)
{
    // DEBUG message
    // printf("row = %d, col = %d, symbol referenced = %s\n", 
    //     row_referencing, col_referencing, sym_referenced->st_name);
        
    uint64_t sym_address = get_symbol_runtime_address(dst, sym_referenced);
    char *s = &dst->buffer[sh->sh_offset + row_referencing][col_referencing]; // 段在buffer中的偏移加上该行在段中的偏移就是该行在buffer中的偏移
    write_relocation(s, sym_address);

    // DEBUG message
    // for(int i = 0; i < 18; i ++ )  
    // {
    //     s[i] = '?';
    // }
}

// PC32 等价于?? PLT32,,, PC相对寻址???
static void R_X86_64_PC32_handler(elf_t *dst, sh_entry_t *sh, int row_referencing, int col_referencing, int addend, st_entry_t *sym_referenced)
{
    assert(strcmp(sh->sh_name, ".text") == 0);
    uint64_t sym_address = get_symbol_runtime_address(dst, sym_referenced);
    uint64_t rip_value = 0x00400000 + (row_referencing + 1) * sizeof(inst_t); // 因为我们算的是下一条指令的地址，所以有+1操作

    char *s = &dst->buffer[sh->sh_offset + row_referencing][col_referencing]; // 段在buffer中的偏移加上该行在段中的偏移就是该行在buffer中的偏移
    // rip + x = addr --> x = addr - rip
    // printf("debug!!!!!: %16lx\n", sym_address - rip_value);
    write_relocation(s, sym_address - rip_value);   //  - rip_value 
}

static const char *get_stb_string(st_bind_t bind)
{
    switch(bind)
    {
        case STB_GLOBAL:
            return "STT_GLOBAL";
        case STB_LOCAL :
            return "STB_LOCAL";
        case STB_WEAK : 
            return "STB_WEAK";
        default : 
            printf("incorrect symbol bind\n");
            exit(0);
    }
}

static const char *get_stt_string(st_type_t type)
{
    switch(type)
    {
        case STT_OBJECT:
            return "STT_OBJECT";
        case STT_FUNC :
            return "STT_FUNC";
        case STT_NOTYPE : 
            return "STT_NOTYPE";
        default : 
            printf("incorrect symbol type\n");
            exit(0);
    }
} 