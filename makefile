CFLAGS = -Wall -g -O2 -std=gnu99 -Wno-unused-function

EXE_HARDWARE = exe_hardware

# debug
COMMON = common/print.c  common/convert.c

# hardware
CPU = hardware/cpu/mmu.c hardware/cpu/isa.c
MEMORY = hardware/memory/dram.c

# main
MAIN_HARDWARE = main_hardware.c

.PHONY:hardware
hardware:
	clear
	gcc $(CFLAGS)  $(MEMORY) $(COMMON) $(CPU) $(MAIN_HARDWARE) -o $(EXE_HARDWARE)
	./$(EXE_HARDWARE)

clean:
	rm -f *.o *~ $(EXE_HARDWARE)