#ifndef LINKER_GUARD
#define LINKER_GUARD

#include <stdint.h>
#include <stdlib.h>

#define MAX_CHAR_SECTION_NAME (32)

typedef struct  // 节头部表条目
{
    char sh_name[MAX_CHAR_SECTION_NAME];
    uint64_t sh_addr;   // run-time address, elf is not loaded, so the run-time address is 0.(运行时地址)
    uint64_t sh_offset; // lien offset of effective line index.(在节头部表中的偏移量)
    uint64_t sh_size;   // sizeof of this section.(该节的条目数)
} sh_entry_t; // section header entry

#define MAX_CHAR_SYMBOL_NAME (64)

typedef enum
{
    STB_LOCAL,
    STB_GLOBAL,
    STB_WEAK
} st_bind_t; // symbol table bind

typedef enum 
{
    STT_NOTYPE,
    STT_OBJECT,
    STT_FUNC
} st_type_t; // symbol table type

typedef struct // 符号表条目
{
    char st_name[MAX_CHAR_SECTION_NAME];
    st_bind_t bind; // binding：约束(，捆绑，结合, ...)
    st_type_t type;
    char st_shndx[MAX_CHAR_SECTION_NAME];
    uint64_t st_value; // in section offset(在当前section中的偏移值)
    uint64_t st_size;   // size of symbol
} st_entry_t; // symbol table entry, bytes occupied

/*======================================*/
/*      relocation information          */
/*======================================*/

typedef enum
{
    R_X86_64_32,
    R_X86_64_PC32,
    R_X86_64_PLT32
} reltype_t;


// relocation entry type
typedef struct 
{
    /*  this is what's different in our inplementation.
        Instead of byte offset, we use line index + char offset to locate the sumbol */
    uint64_t r_row; // line index in buffer section
                    // for .rel.text, that's the line index in .text section
                    // for .rel.date, that's the line index in .data section
    uint64_t r_col;    // char offset in the buffer lien
    reltype_t type; // relocation type, absolute or relative ...
    uint32_t sym;   // symbol table index
    uint64_t r_addend; // constant part of relocation expression
} rl_entry_t;

#define MAX_ELF_FILE_LENGTH (64)     // max 64 effective lines
#define MAX_ELF_FILE_WIDTH  (128)    // max 128 chars per line

typedef struct 
{
    char buffer[MAX_ELF_FILE_LENGTH][MAX_ELF_FILE_WIDTH]; // store source elf file contents
    
    uint64_t line_count; // sum of lines
    
    uint64_t sht_count;   // sum of section header table
    sh_entry_t *sht;         // section header entry
    
    uint64_t symt_count;     // symbol table entry count
    st_entry_t *symt;       // symbol table entry

    // relocation entry information
    uint64_t reltext_count;
    rl_entry_t *reltext;

    uint64_t reldata_count;
    rl_entry_t *reldata;
    
} elf_t;

void parse_elf(char *filename, elf_t *elf);
void free_elf(elf_t *elf);
void link_elf(elf_t **srcs, int num_srcs, elf_t *dst);
void write_eof(const char *filename, elf_t *eof);

#endif 