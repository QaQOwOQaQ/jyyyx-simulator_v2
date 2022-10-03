CC = gcc

CFLAGS = -Wall -g -O2 -Werror -std=gnu99 -Wno-unused-function

BIN_HARDWARE = ./bin/test_hadrware
BIN_ELF      = ./bin/test_elf

SRC_DIR = ./src

# debug
COMMON = $(SRC_DIR)/common/print.c  $(SRC_DIR)/common/convert.c

# hardware
CPU = $(SRC_DIR)/hardware/cpu/mmu.c $(SRC_DIR)/hardware/cpu/isa.c
MEMORY = $(SRC_DIR)/hardware/memory/dram.c

# main
TEST_HARDWARE = $(SRC_DIR)/tests/test_hardware.c
TEST_ELF      = $(SRC_DIR)/tests/test_elf.c

# link
LINK = $(SRC_DIR)/linker/parseELF.c $(SRC_DIR)/linker/staticlink.c

# run
.PHONY:hardware
hardware:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON) $(CPU) $(MEMORY) $(TEST_HARDWARE) -o $(BIN_HARDWARE)
	./$(BIN_HARDWARE)

./PHONY:link
link:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON) $(LINK)  $(TEST_ELF) -o $(BIN_ELF)
	./$(BIN_ELF)

clean:
	rm -f *.o *~ $(BIN_HARDWARE) $(BIN_ELF)