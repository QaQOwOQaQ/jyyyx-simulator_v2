#ifndef LINKER_GUARD
#define LINKER_GUARD

#include <stdint.h>
#include <stdlib.h>


/*================================*/
/*      Section Header Table      */
/*================================*/

#define MAX_CHAR_SECTION_NAME (32) // a good hobby to add a barckets
typedef struct  
{
    char sh_name[MAX_CHAR_SECTION_NAME];
    uint64_t sh_addr;
    uint64_t sh_offset; // line offset or effective line index 我们这里的偏移都是以行为单位的，因为每个条目都占一行
    uint64_t sh_size;
} sh_entry_t;


/*==================================*/
/*          Symbol Table            */
/*==================================*/

#define MAX_CHAR_SYMBOL_NAME (64)

typedef enum // symbol table bind
{
    STB_LOCAL, 
    STB_GLOBAL, 
    STB_WEAK,
} st_bind_t;

typedef enum // sumbol table type
{
    STT_NOTYPE,
    STT_OBJECT,
    STT_FUNC,
} st_type_t;

typedef struct
{
    char st_name[MAX_CHAR_SYMBOL_NAME];
    st_bind_t bind;
    st_type_t type;
    char st_shndx[MAX_CHAR_SECTION_NAME]; // is a good way to use string to complete ??
    uint64_t st_value;  // in-section offset
    uint64_t st_size;   // count of lines of symbol
} st_entry_t;


/*================*/
/*       ELF      */
/*================*/

#define MAX_ELF_FILE_LENGTH (64)   // max 64 effective lines
#define MAX_ELF_FILE_WIDTH  (128)  // max 128 chars pre line
typedef struct 
{
    char buffer[MAX_ELF_FILE_LENGTH][MAX_ELF_FILE_WIDTH]; // buffer用来缓存elf文本源数据，很浪费空间，但是一切为了简单
    uint64_t line_count;
    
    uint64_t sht_count;
    sh_entry_t *sht;
    
    uint64_t symt_count; // symbol table 
    st_entry_t *symt;

    uint64_t reltext_count;
    rl_entry_t *reltext;

    uint64_t reldata_count;
    rl_entry_t *reldata;
} elf_t;



void parse_elf(char *filename, elf_t *elf);
void free_elf(elf_t *elf);
void link_elf(elf_t **srcs, int num_srcs, elf_t *dst);


#endif