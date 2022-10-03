// 将elf文件中的非空白行，非注释行加载到elf的buffer里面
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <headers/linker.h>
#include <headers/common.h>

/*  
    看到三个*别害怕！
    ent是一个指向（指向字符串的指针）的指针
    同时也是一个指向堆上的内存的指针
    字符串：char *
    指向字符串的指针：char ** 
    指向（指向字符串(数组的首地址)的指针）的指针：char ***
*/
static int parse_table_entry(char *str, char ***ent)
{
    // column0,column1,,column2,column3,... (CSV)
    // parse line as table entries
    int count_col = 1; // 从1开始是因为我们遇到一个逗号就++一次，而column的个数比逗号的个数多1
    int len = strlen(str);

    // count columns
    for(int i = 0; i < len; i ++ )
    {
        if(str[i] == ',')
        {
            count_col ++ ;
        }
    }

    // mallocand create list
    char **arr = malloc(count_col * sizeof(char *));
    *ent = arr;

    int col_index = 0;
    int col_width = 0;
    char col_buf[32];
    for(int i = 0; i < len + 1; i ++ ) // i<len+1就可以在字符串结束时取到字符串的终结符'\0'，从而避免特判
    {
        if(str[i] == ',' || str[i] == '\0') // read a column
        {
            assert(col_index < count_col);

            // malloc and copy
            char *col = malloc((col_width + 1) * sizeof(char)); // 别忘了'\0'也占用一个字节
            for(int j = 0; j < col_width; j ++ ) // move the content from stack to heap
            {
                col[j] = col_buf[j];
            }
            col[col_width] = '\0'; // easy to foeget

            // update
            arr[col_index] = col;
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

static void free_table_entry(char **ent, int n)
{
    for(int i = 0; i < n; i ++ )
    {
        free(ent[i]);
    }
    free(ent);
}

static void parse_sh(char *str, sh_entry_t *sh)
{
    char **cols; // will malloc memory in heap in function parse_table_entry
    int num_cols = parse_table_entry(str, &cols);
    assert(num_cols == 4);

    assert(sh != NULL);
    strcpy(sh->sh_name, cols[0]);
    sh->sh_addr   = string2uint(cols[1]);
    sh->sh_offset = string2uint(cols[2]);
    sh->sh_size   = string2uint(cols[3]);
    
    // 此时堆中已经对上面的四个数据存储了两次
    // 所以我们需要释放掉一份
    // cols相当于一个字符串数组，而sh是一个结构体
    free_table_entry(cols, num_cols); // malloc -- free
}

static void print_sh_entry(sh_entry_t *sh)
{
    debug_print(DEBUG_LINKER, "%s\t%x\t%d\t%d\n",
        sh->sh_name,
        sh->sh_addr,
        sh->sh_offset,
        sh->sh_size
    );
}


void parse_symtab(char *str, st_entry_t *ste) //ste:symbol table entry
{
    // sum,STB_GLOBAL,STT_FUNCTION,.text,0,22
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
    else // Error handling: match failure
    {
        printf("symbol bind is neither LOCAL, GLOBAL, nor WEAK!\n");
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
    else // Error handling: match failure
    {
        printf("symbol type is neither NOTYPE, OBJECT, nor FUNC!\n");
        exit(0);
    }

    strcpy(ste->st_shndx, cols[3]);
    ste->st_value = string2uint(cols[4]);
    ste->st_size  = string2uint(cols[5]);

    // the information of x has been parsed into y
    // so y can be freed
    free_table_entry(cols, num_cols);
}


void print_symtab_entry(st_entry_t *ste)
{
    debug_print(DEBUG_LINKER, "%s\t%d\t%d\t%s\t%d\t%d\n",
        ste->st_name,
        ste->bind,
        ste->type,
        ste->st_shndx,
        ste->st_value,
        ste->st_size
    );
}

static int read_elf(const char *filename, uint64_t bufaddr)
{
    // open file and read
    FILE *fp;
    fp = fopen(filename, "r");
    if(fp == NULL)  
    {
        debug_print(DEBUG_LINKER, "unable to open file %s\n", filename);
        exit(1);
    }

    // read text file line by line
    char line[MAX_ELF_FILE_WIDTH];
    int line_counter = 0;

    while(fgets(line, MAX_ELF_FILE_WIDTH, fp) != NULL)
    {
        int len = strlen(line);

        // check the effective line
        if(len == 0)    continue;
        if(len >= 1 && (line[0] == ' ' || line[0] == '\n' || line[0] == '\t' || line[0] == '\r'))  continue;
        if(len >= 2 && line[0] == '/' && line[1] == '/')  continue;

        // to this line, this line is not white and contains information
        if(line_counter < MAX_ELF_FILE_LENGTH) // lines not exceed the limit
        {
            // store this line to buffer[line_counter]
            uint64_t addr = bufaddr + line_counter * MAX_ELF_FILE_WIDTH * sizeof(char);
            char *linebuf = (char *)addr;

            int i = 0;
            while(i < len)
            {
                int j = i + 1;   
                if((line[i] == '\n') || (line[i] == '\r') || ((j < len) && line[i] == '/' && line[j] == '/'))  
                { // check inline "//" or with the end of '\n' or '\r'
                    break;
                }
                linebuf[i] = line[i];
                i ++ ;
            }
            linebuf[i] = '\0';
            line_counter ++ ;
        }
        else // Error: read too much lines!
        {
            fclose(fp);
            debug_print(DEBUG_LINKER, "elf file %s is too long (>%d)\n", filename, MAX_ELF_FILE_LENGTH);
            exit(1);
        }
    }
    fclose(fp);
    assert(string2uint((char *)bufaddr) == line_counter);
    return line_counter;
}

void parse_elf(char *filename, elf_t *elf)
{
    assert(elf != NULL);
    
    int line_count = read_elf(filename, (uint64_t)&(elf->buffer));
    for(int i = 0; i < line_count; i ++ )
    {
        printf("[%d]\t%s\n", i, elf->buffer[i]);
    }

    // parse section headers
    int sh_count = string2uint(elf->buffer[1]);
    elf->sht = malloc(sh_count * sizeof(sh_entry_t));
    
    sh_entry_t *symt_sh = NULL; // symbol table section's ptr
    for(int i = 0; i < sh_count; i ++ )
    {
        parse_sh(elf->buffer[2 + i], &(elf->sht[i]));
        print_sh_entry(&(elf->sht[i]));

        if(strcmp(elf->sht[i].sh_name, ".symtab") == 0)
        {
            // this is the section header for symbol table 
            symt_sh = &(elf->sht[i]);
        }
    }

    assert(symt_sh != NULL);

    // parse symbol table
    elf->symt_count = symt_sh->sh_size;
    elf->symt = malloc(elf->symt_count * sizeof(st_entry_t));
    for(int i = 0; i < symt_sh->sh_size; i ++ )
    {
        parse_symtab(
            elf->buffer[i + symt_sh->sh_offset],
            &(elf->symt[i])
        );
        print_symtab_entry(&(elf->symt[i]));
    }
}

void free_elf(elf_t *elf)
{
    assert(elf != NULL);
    free(elf->sht);
}