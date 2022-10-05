CC = gcc

CFLAGS = -Wall -g -O2 -Werror -std=gnu99 -Wno-unused-function

BIN_MACHINE = ./bin/test_machine
BIN_ELF      = ./bin/test_elf

SRC_DIR = ./src

# debug
COMMON = $(SRC_DIR)/common/print.c  $(SRC_DIR)/common/convert.c

# machine
CPU = $(SRC_DIR)/hardware/cpu/mmu.c $(SRC_DIR)/hardware/cpu/isa.c
MEMORY = $(SRC_DIR)/hardware/memory/dram.c

# main
TEST_HARDWARE = $(SRC_DIR)/tests/test_machine.c
TEST_ELF      = $(SRC_DIR)/tests/test_elf.c

# link
LINK = $(SRC_DIR)/linker/parseELF.c $(SRC_DIR)/linker/staticlink.c

# run
./PHONY:link
link:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON) $(LINK)  $(TEST_ELF) -o $(BIN_ELF)
	./$(BIN_ELF)


.PHONY:machine
machine:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON) $(CPU) $(MEMORY) $(TEST_HARDWARE) -o $(BIN_MACHINE)
	./$(BIN_MACHINE)

clean:
	rm -f *.o *~ $(BIN_MACHINE) $(BIN_ELF)