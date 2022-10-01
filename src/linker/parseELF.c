// 将elf文件中的非空白行，非注释行加载到elf的buffer里面
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../headers/linker.h"
#include "../headers/common.h"


int read_elf(const char *filename, uint64_t bufaddr)
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