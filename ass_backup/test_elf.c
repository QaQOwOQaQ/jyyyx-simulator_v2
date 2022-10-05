#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <headers/linker.h>
#include <headers/common.h>

int read_elf(const char *filename, uint64_t bufaddr);
void parse_table_entry(char *str, char ***ent);
void free_table_entry(char **ent, int n);

void parse_sh(char *str, sh_entry_t *sh);
void parse_sh_entry(char *str, sh_entry_t *sh);

void parse_elf(char *filename, elf_t *elf);
void free_elf(elf_t *elf);

void link_elf(elf_t **srcs, int num_srcs, elf_t *dst);

int main()
{
    elf_t src[2];
    parse_elf("./files/exe/sum.elf.txt",  &src[0]);
    printf("--------------------------------\n");
    parse_elf("./files/exe/main.elf.txt", &src[1]);
    
    elf_t dst;
    elf_t *srcp[2] = {&src[0], &src[1]};
    link_elf((elf_t **)&srcp                            , 2, &dst); 
     
    free_elf(&src[0]);
    free_elf(&src[1]);
    
    return 0;
}