#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../headers/linker.h"
#include "../headers/common.h"

int read_elf(const char *filename, uint64_t bufaddr);
void parse_table_entry(char *str, char ***ent);
void free_table_entry(char **ent, int n);

void parse_sh(char *str, sh_entry_t *sh);
void parse_sh_entry(char *str, sh_entry_t *sh);

void parse_elf(char *filename, elf_t *elf);
void free_elf(elf_t *elf);

int main()
{
    elf_t elf;
    parse_elf("./files/exe/sum.elf.txt", &elf);
    free_elf(&elf);

    return 0;
}