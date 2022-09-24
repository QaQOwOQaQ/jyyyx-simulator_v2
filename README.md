## After refactory by responsitory assembly

### 9.22 
The first code refactoring is completed, and the assembler can be successfully run, however, we haven't implemented the relevant function yet, so the assembler will end abnormally(core dumped) after first statement is excuted.

### 9.23
Complete function: String2Uint_range.

Optimization cpu_flags by using C language bit operation, we can represent four flags with only one uint64 instead of four uint64.

Complete function: Inst_parse and Operand_parse

### 9.24
Complete the parsing of an assemlby instruction, and separate out the instruction and operands

After a long time to debug, the program can finally run the assembler which executes the add(a+b) function, and test success.

I even reset the cpu_flags after I modifying the flags in the function jump_handler, which caused the execution to fail when jump returned to function main. After fixing this bug, the program can run the recursive get-sum assembler successfullu. So far, the first stage of the assembly simulator -- the instruction part is basically done.
