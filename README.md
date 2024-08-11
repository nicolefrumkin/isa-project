# ISA Project

## Overview

The ISA Project simulates a simple instruction set architecture (ISA) processor. It includes a simulator for executing a set of instructions, handling interrupts, and performing disk operations. This project is designed to demonstrate how a basic ISA processor might operate and interact with various components such as memory, I/O registers, and a disk.

## Files

### Assembler

**`assembler.c`**

This file contains the source code for the ISA assembler. The assembler converts assembly language code into machine code that the simulator can execute. The main functions of the assembler include:

- Parsing assembly language instructions.
- Converting instructions and operands into machine code.
- Handling labels and resolving addresses.
- Generating binary output files for use with the simulator.

### Simulator

**`isa_simulator.c`**

This file contains the source code for the ISA simulator. It reads instructions and data from input files, executes the instructions, and writes the results to various output files. The main functions of the simulator include:

- Parsing instructions from an input file.
- Executing operations based on the instructions.
- Handling interrupts and I/O operations.
- Managing disk operations and interacting with memory.

### Assembly Code Example

**`fib.asm`**

This file contains an example of assembly code to compute Fibonacci numbers. It includes all necessary assembly instructions and is used to test the ISA simulator. The related output files from running this example are also included in the folder.

The simulator requires several input files to run:

- memin.txt - Contains the initial memory values.
- diskin.txt - Contains the disk data.
- irq2in.txt - Contains the IRQ2 input data.
- memOut.txt - Output file for memory values after execution.
- regOut.txt - Output file for register values.
- trace.txt - Trace file showing the execution steps.
- hwregtrace.txt - Trace file for I/O register operations.
- cycles.txt - File showing the total number of clock cycles.
- leds.txt - Output file for LED states.
- display7seg.txt - Output file for 7-segment display data.
- diskout.txt - Output file for disk data.
- monitor.txt - Output file for monitor framebuffer data.
- monitor.yuv - Output file for monitor data in YUV format.
