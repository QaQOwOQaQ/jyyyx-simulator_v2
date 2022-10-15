#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <headers/linker.h>
#include <headers/common.h>

void parse_elf(char *filename, elf_t *elf);

int main()
{
    printf("-----------------test start!!!!--------------------\n");


    elf_t src[2];
    parse_elf("./files/exe/sum.elf.txt", &src[0]);
    printf("=============================================\n");
    parse_elf("./files/exe/main.elf.txt", &src[1]);
    printf("----------------------read and parse success------------------!\n");


    elf_t dst;
    elf_t *srcp[2] = {&src[0], &src[1]};
    link_elf((elf_t **)&srcp, 2, &dst);
    printf("-----------------link success--------------------\n");

    write_eof("./files/exe/output.eof.txt", &dst);
    printf("-----------------write eof success--------------------\n");

    free_elf(&src[0]);
    free_elf(&src[1]);
    free_elf(&dst);

    printf("-----------------test successful!!!!--------------------\n");
    return 0;
}