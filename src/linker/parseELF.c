#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <headers/linker.h>
#include <headers/common.h>

/*    what we do in this file:
static int parse_table_entry(char *str, char ***ent);            // 将一个CSV(逗号分割值)格式的行(str)解析为很多个列(ent),并将结果保存到ent中
static void free_table_entry(char **ent, int n);                // 释放ent
static void parse_sh(char *str, sh_entry_t *sh);                // 解析 section header(str)中的内容到sh(section header)
void print_sh_entry(sh_entry_t *sh);                            // 打印 section header 表条目的信息
void parse_symtab(char *str, st_entry_t *ste);                  // 解析符号表str中的内容到ste(symbol table entry)
static void print_symtab_entry(st_entry_t *ste);                // 打印符号表条目的信息
static void parse_relocation(char *str, rl_entry_t *rte);       // 解析重定位信息(str)到rte(relocation table entry)
static void print_relocation_entry(rl_entry_t *rte);            // 打印重定位表条目信息
static int read_elf(const char *filename, uint64_t bufaddr);    // 读入一个elf文件到地址为 bufaddr处
void parse_elf(char *filename, elf_t *elf);                     // 解析elf文件
void write_eof(const char *filename, elf_t *eof);               // 将elf文件写入eof(executable object file)文件中
*/

static int parse_table_entry(char *str, char ***ent)
{
    // 解析一行CSV格式的条目
    assert(str != NULL); // by xjy，让下面的count_col=1更合理
    // column0, column1, column2, column3, ... 
    // parse line as table entries
    int count_col = 1;
    // 我们这里通过逗号的数量来计算列的数量，为什么列数初始化为1
    // C与指针提到过：先考虑特殊情况，当没有逗号时，就有一列，因此，初始化为1
    int len = strlen(str);

    // 计算列(column)的个数
    for(int i = 0; i < len; i ++ )
    {
        if(str[i] == ',')   
        {
            count_col ++ ;
        }
    }

    // malloc and create list

    char *arr[count_col]; // by xjy, 与作者的写法不一致，作者定义的是一个指向指针的指针(malloc两次)，而我的是一个指针数组(只malloc一次)
    *ent = arr;

    int col_index = 0;
    int col_width = 0;
    char col_buf[32]; // 暂时保存列，不用在堆上分配空间
    for(int i = 0; i < len + 1; i ++ ) // i < len + 1方便判断结尾，避免特判增加额外代码
    {
        if(str[i] == ',' || str[i] == '\0') // 读取到一个列
        {
            assert(col_index < count_col);

            // malloc and copy
            // 将信息保存到堆中
            arr[col_index] = malloc((col_width + 1) * sizeof(char)); // 为字符串分配空间别忘了终结符
            for(int j = 0; j < col_width; j ++ )
            {
                arr[col_index][j] = col_buf[j];
            }
            arr[col_index][col_width] = '\0'; // 别忘了哦

            // update
            col_index ++ ;
            col_width = 0;
        }
        else 
        {
            assert(col_width < 32);
            col_buf[col_width] = str[i];
            col_width ++ ;
        }
    }
    return count_col;
}

static void free_table_entry(char** ent, int n)
{
    // 当我们把堆中的信息写入到符号表/节头部表中之后，就可以释放掉堆中的信息了
    for(int i = 0; i < n; i ++ )
    {
        free(ent[i]);
    }
}

static void parse_sh(char *str, sh_entry_t *sh) // section header
{
    // 解析一行节头部表信息
    // .text,0x0,4,22
    char **cols;
    int num_cols = parse_table_entry(str, &cols);
    assert(num_cols == 4);
    
    assert(sh != NULL);
    strcpy(sh->sh_name, cols[0]);
    sh->sh_addr = string2uint(cols[1]);
    sh->sh_offset = string2uint(cols[2]);
    sh->sh_size = string2uint(cols[3]);
    
    free_table_entry(cols, num_cols);
}

void print_sh_entry(sh_entry_t *sh)
{
    debug_print(DEBUG_LINKER, "%s\t%x\t%d\t%d\n",
        sh->sh_name,
        sh->sh_addr,
        sh->sh_offset,
        sh->sh_size
        );
}

void parse_symtab(char *str, st_entry_t *ste) // symbol table entry
{
    // sum, STB_GLOBAL, STT_FUNC, .TEXT. 0, 22
    char **cols;
    int num_cols = parse_table_entry(str, &cols);
    assert(num_cols == 6);

    assert(ste != NULL);

    strcpy(ste->st_name, cols[0]);
    // select symbol bind
    if(strcmp(cols[1], "STB_LOCAL") == 0)
    {
        ste->bind = STB_LOCAL;
    }
    else if(strcmp(cols[1], "STB_GLOBAL") == 0)
    {
        ste->bind = STB_GLOBAL;
    }

    else if(strcmp(cols[1], "STB_WEAK") == 0)
    {
        ste->bind = STB_WEAK;
    }
    else 
    {
        printf("symbol type is neither LOCAL, GLOBAL, nor WEAK!\n");
        exit(0);
    }
    
    // select symbol type
    if(strcmp(cols[2], "STT_NOTYPE") == 0)
    {
        ste->type = STT_NOTYPE;
    }
    else if(strcmp(cols[2], "STT_OBJECT") == 0)
    {
        ste->type = STT_OBJECT;
    }

    else if(strcmp(cols[2], "STT_FUNC") == 0)
    {
        ste->type = STT_FUNC;
    }
    else 
    {
        printf("symbol type is neither NOTYPE, OBJECT, nor FUNC!\n");
        exit(0);
    }

    strcpy(ste->st_shndx, cols[3]);
    ste->st_value = string2uint(cols[4]);
    ste->st_size = string2uint(cols[5]);
    free_table_entry(cols, num_cols);
}

static void print_symtab_entry(st_entry_t *ste)
{
    debug_print(DEBUG_LINKER, "%s\t%d\t%d\t%s\t%d\t%d\n",
        ste->st_name,
        ste->bind,
        ste->type,
        ste->st_shndx,
        ste->st_value,
        ste->st_size);
}
static void parse_relocation(char *str, rl_entry_t *rte)
{
    // 4,7,R_x86_64_PC32,0,-4
    char **cols;
    int num_cols = parse_table_entry(str, &cols);
    assert(num_cols == 5);

    rte->r_row = string2uint(cols[0]);
    rte->r_col = string2uint(cols[1]);
    
    // select relocation type
    if(strcmp(cols[2], "R_X86_64_32") == 0)
    {
        rte->type = R_X86_64_32;
    }
    else if(strcmp(cols[2], "R_X86_64_PC32") == 0)
    {
        rte->type = R_X86_64_PC32;
    }
    else if(strcmp(cols[2], "R_X86_64_PLT32") == 0)
    {
        rte->type = R_X86_64_PLT32;
    }
    else 
    {
        printf("relocation type is neiter R_X86_64_32, R_X86_64_PC32, nor R_X86_64_PLT32\n");
        exit(0);
    }
    rte->sym = string2uint(cols[3]);

    uint64_t bitmap = string2uint(cols[4]);
    rte->r_addend = *(int64_t *)&bitmap;

    free_table_entry(cols, num_cols);
}

static void print_relocation_entry(rl_entry_t *rte)
{
    debug_print(DEBUG_LINKER, "%d\t%d\t%d\t%d\t%d\n",
        rte->r_row,
        rte->r_col,
        rte->type,
        rte->sym,
        rte->r_addend);
}

static int read_elf(const char *filename, uint64_t bufaddr)
{
    // 读取elf文件中的内容到elf_t中的buffer(堆上)
    // open file and read
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
    {
        debug_print(DEBUG_LINKER, "unable to open file %s\n", filename);
        exit(1);
    }

    // read text file line by line
    char line[MAX_ELF_FILE_WIDTH];
    int line_counter = 0;

    while(fgets(line, MAX_ELF_FILE_WIDTH, fp) != NULL) // 每次读取一行
    {
        // 读取elf文件中的内容到stack上的line[]并写入到bufaddr处
        int len = strlen(line);

         // 如果读取的是空行或者注释行，跳过
        if( (len == 0) || 
            (len >= 1 && (line[0] == '\n' || line[0] == '\r' || line[0] == '\t' || line[0] == ' ')) || 
            (len >= 2 && (line[0] == '/' && line[1] == '/')) )
        {
            continue;
        }

        // to this line, this line is not white and contains effective information
        if(line_counter < MAX_ELF_FILE_LENGTH)
        {
            // store this line to buffer[line_counter]
            uint64_t addr = bufaddr + line_counter * MAX_ELF_FILE_WIDTH * sizeof(char); // fixed width of line
            char *linebuf = (char *)addr;   // point ptr(lienbuf) to buffer's address

            int i = 0;
            while(i < len && i < MAX_ELF_FILE_WIDTH) 
            {
                if(line[i] == '\n' || line[i] == '\r' || // 忽略末尾的回车 
                    ((i + 1 < len) && line[i] == '/' && line[i + 1] == '/')) // 忽略语句内部(末尾)注释
                {
                    break;
                }
                // else 
                linebuf[i] = line[i];
                i ++ ;
            }
            linebuf[i] = '\0'; // add NUL
            line_counter ++ ;
        }
        else 
        {
            debug_print(DEBUG_LINKER, "elf file %s is too long(>%d)\n", filename, MAX_ELF_FILE_WIDTH);
            exit(0);
        }
    }

    fclose(fp);
    // buffer中第一行的数据应该等于读取的elf文件中有效(非注释，空行)行数
    assert(string2uint((char *)bufaddr) == line_counter);
    return line_counter;
}

void parse_elf(char *filename, elf_t *elf)
{
    assert(elf != NULL);
    
    // 先把elf文件的有效内容读取到buffer
    // First, read the contents of elf file in elf_t->buffer
    int line_count = read_elf(filename, (uint64_t)(&(elf->buffer)));

    // view the contents
    for(int i = 0; i < line_count; i ++ )
    {
        printf("[%d]\t%s\n", i, elf->buffer[i]);
    }

    // Second. parse section headers
    elf->sht_count = string2uint(elf->buffer[1]); // section header(.text, .bss, ...) count
    elf->sht = malloc(elf->sht_count * sizeof(sh_entry_t));

    sh_entry_t *symt_sh = NULL;
    sh_entry_t *rtext_sh = NULL;
    sh_entry_t *rdata_sh = NULL;
    for(int i = 0; i < elf->sht_count; i ++ )
    {
        // 2 means: 1(总行数) + 1(节头部条目数)
        parse_sh(elf->buffer[2 + i], &(elf->sht[i]));
        print_sh_entry(&(elf->sht[i]));

        if(strcmp(elf->sht[i].sh_name, ".symtab") == 0)
        {
            // this is the section header for symbol table
            symt_sh = &(elf->sht[i]);
        }
        else if(strcmp(elf->sht[i].sh_name, ".rel.text") == 0)
        {
            // this is the section header for symbol table
            rtext_sh = &(elf->sht[i]);
        }
        else if(strcmp(elf->sht[i].sh_name, ".rel.data") == 0)
        {
            // this is the section header for symbol table
            rdata_sh = &(elf->sht[i]);
        }
    }

    assert(symt_sh != NULL); // 不能啥都没有吧
    
    // Third. parse symbol table
    elf->symt_count = symt_sh->sh_size; // 符号表条目个数
    elf->symt = malloc(elf->symt_count * sizeof(st_entry_t)); // 为symbol table entry分配空间
    for(int i = 0; i < symt_sh->sh_size; i ++ )
    {
        parse_symtab(elf->buffer[i + symt_sh->sh_offset], &(elf->symt[i]));
        print_symtab_entry(&(elf->symt[i]));
    }

    // Fourth. parse relocation table
    if(rtext_sh != NULL)
    {
        elf->reltext_count = rtext_sh->sh_size;
        elf->reltext = malloc(elf->reltext_count * sizeof(rl_entry_t));
        for(int i = 0; i < rtext_sh->sh_size; i ++ )
        {
            parse_relocation(
                elf->buffer[i + rtext_sh->sh_offset],
                &(elf->reltext[i])
            );
            int st = elf->reltext[i].sym;
            assert(st >= 0 && st < elf->symt_count);
            
            print_relocation_entry(&(elf->reltext[i]));
        }
    }
    else 
    {
        elf->reltext_count = 0;
        elf->reltext = NULL;
    }

    // .rel.data
    if(rdata_sh != NULL)
    {
        elf->reldata_count = rdata_sh->sh_size;
        elf->reldata = malloc(elf->reldata_count * sizeof(rl_entry_t));
        for(int i = 0; i < rdata_sh->sh_size; i ++ )
        {
            parse_relocation(
                elf->buffer[i + rdata_sh->sh_offset],
                &(elf->reldata[i])
            );
            int st = elf->reldata[i].sym;
            assert(st >= 0 && st < elf->symt_count);
            
            print_relocation_entry(&(elf->reldata[i]));
        }
    }
    else 
    {
        elf->reldata_count = 0;
        elf->reldata = NULL;
    }
}

void write_eof(const char *filename, elf_t *eof)
{
    // open elf file
    FILE *fp = fopen(filename, "w");
    if(fp == NULL)
    {
        debug_print(DEBUG_LINKER, "unable to open file %s\n", filename);
        exit(1);
    }
    
    // write eof to file
    for(int i = 0; i < eof->line_count; i ++ )
    {
        fprintf(fp, "%s\n", eof->buffer[i]);
    }

    fclose(fp);
}

void free_elf(elf_t *elf)
{
    assert(elf != NULL);
    free(elf->sht);
    free(elf->symt);
    free(elf->reltext);
    free(elf->reldata);
}





































