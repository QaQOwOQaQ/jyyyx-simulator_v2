CC = gcc

CFLAGS = -Wall -g -O2 -Werror -std=gnu99 -Wno-unused-function

BIN_MACHINE = ./bin/test_machine
BIN_ELF     = ./bin/test_elf
test_mesi   = ./bin/test_mesi
test_false_sharing = ./bin/test_false_sharing

SRC_DIR = ./src

# debug
COMMON = $(SRC_DIR)/common/print.c  $(SRC_DIR)/common/convert.c

# machine
CPU = $(SRC_DIR)/hardware/cpu/mmu.c  $(SRC_DIR)/hardware/cpu/isa.c  $(SRC_DIR)/hardware/cpu/sram.c
MEMORY = $(SRC_DIR)/hardware/memory/dram.c  $(SRC_DIR)/hardware/memory/swap.c
ALGORITHM = $(SRC_DIR)

# main
TEST_HARDWARE = $(SRC_DIR)/hardware/cpu/isa.c
TEST_ELF      = $(SRC_DIR)/mains/test_elf.c
TEST_MESI     = $(SRC_DIR)/mains/mesi.c
TEST_FALSE_SHARING = $(SRC_DIR)/mains/false_sharing.c

# link
LINK = $(SRC_DIR)/linker/parseELF.c $(SRC_DIR)/linker/staticlink.c

# run
.PHONY:link
link:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON) $(LINK)  $(TEST_ELF) -o $(BIN_ELF)
	$(BIN_ELF)


.PHONY:machine
machine:
	$(CC) $(CFLAGS) -I$(SRC_DIR) -DDEBUG_INSTRUCTION_CYCLE $(COMMON) $(CPU) $(MEMORY) $(TEST_HARDWARE) -o $(BIN_MACHINE)
	$(BIN_MACHINE)

mesi: 
	$(CC) $(TEST_MESI) -o $(test_mesi) 
	$(test_mesi)

mesi_debug: 
	$(CC) $(CFLAGS) $(TEST_MESI) -DDEBUG -o $(test_mesi)
	$(test_mesi)

false_sharing:
	$(CC) $(CFLAGS) -pthread $(TEST_FALSE_SHARING) -o $(test_false_sharing)
	$(test_false_sharing)

clean:
	rm -f *.o *~ 