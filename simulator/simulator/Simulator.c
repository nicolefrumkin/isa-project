#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_CYCLES 0xFFFFFFFF
#define SECTORS 128
#define NUM_OF_INTS_IN_SECTOR 128
#define MAX_LINE_SIZE  501

typedef struct Instruction {
    int opcode;
    int rd;
    int rs;
    int rt;
    int imm;
    int PC;
} Instruction;

//========declaration=========
void executeOperations(Instruction* instArr, int* R, int numOfInst, FILE* traceFile, FILE* hwregtraceFile, int* IORegisters, FILE* cyclesFile, FILE* display7seg, FILE* ledsFile);
void writeRegistersToFile(FILE* regoutFile);
void writeTraceToFile(int PC, Instruction inst, int registerArray[16], FILE* traceFile);
int saveLinesToInstArr(FILE* meminFile, Instruction* instArr);
int findInstructionIndexByPC(Instruction* instArr, int numOfInst, int wanted_PC);
void readMem(FILE* meminFile, int* mem);
void writeMemOut(FILE* memOut, int* memOutArr);
void writeMonitor(FILE* monitor, int frameBuffer[256][256]);
void writeDisplay(FILE* display7seg, int* IORegisters, Instruction instArr[]);
int irqCheck(int* IORegisters, Instruction instArr[]);
void initializeDisk(FILE* diskinFile);
void handleDiskOperations();
void writeDiskToFile(FILE* diskoutFile);
void readIrq2(FILE* irq2In);
void checkIfIrq2Happend();
void writeMonitorYuv(FILE* monitorYuv, int frameBuffer[256][256]);


const char* registerNames[] = {
"irq0enable",
"irq1enable",
"irq2enable",
"irq0status",
"irq1status",
"irq2status",
"irqhandler",
"irqreturn",
"clks",
"leds",
"display7seg",
"timerenable",
"timercurrent",
"timermax",
"diskcmd",
"disksector",
"diskbuffer",
"diskstatus",
"reserved",
"reserved",
"monitoraddr",
"monitordata",
"monitorcmd"
};

//=======Global variables============
int memory[4096] = { 0 };
int IORegisters[23] = { 0 };
int registerArray[16] = { 0 };
int disk[SECTORS][NUM_OF_INTS_IN_SECTOR] = { 0 };
int clockCycles = 0;
int irq2In[4096] = { 0 };
int frameBuffer[256][256] = { 0 };
int disk_start_cyc = 0;


int main(int argc, char* argv[]) {
    Instruction instArr[4096];

    // Opening files
    FILE* meminFile = fopen(argv[1], "r");
    if (!meminFile) {
        perror("Failed to open memin file");
        return 1;
    }
    FILE* diskinFile = fopen(argv[2], "r");
    if (!diskinFile) {
        perror("Failed to open diskin.txt");
        return;
    }
    FILE* irq2inFile = fopen(argv[3], "r");
    if (!irq2inFile) {
        perror("Failed to open irq2in file");
        return 1;
    }
    FILE* memOut = fopen(argv[4], "w");
    if (memOut == NULL) {
        perror("Failed to open file memOut");
        return 1;
    }

    FILE* regoutFile = fopen(argv[5], "w");
    if (!regoutFile) {
        perror("Failed to open file regOut");
        return 1;
    }
    FILE* traceFile = fopen(argv[6], "w");
    if (traceFile == NULL) {
        perror("Failed to open file tracefile");
        return;
    }
    FILE* hwregtraceFile = fopen(argv[7], "w");
    if (hwregtraceFile == NULL) {
        perror("Failed to open file hwtracefile");
        return;
    }
    FILE* cyclesFile = fopen(argv[8], "w");
    if (cyclesFile == NULL) {
        perror("Failed to open file");
        return;
    }
    FILE* ledsFile = fopen(argv[9], "w");
    if (ledsFile == NULL) {
        perror("Failed to open file leds");
        return;
    }
    FILE* display7seg = fopen(argv[10], "w");
    if (display7seg == NULL) {
        perror("Failed to open file display7reg");
        return;
    }
    FILE* diskoutFile = fopen(argv[11], "w");
    if (!diskoutFile) {
        perror("Failed to open diskout.txt");
        return;
    }
    FILE* monitor = fopen(argv[12], "w");
    if (monitor == NULL) {
        perror("Failed to open file monitor");
        return;
    }
    FILE* monitorYuv = fopen(argv[13], "wb");
    if (monitorYuv == NULL) {
        perror("Failed to open file monitor");
        return;
    }

    initializeDisk(diskinFile);
    // iterates through file lines and saves each instruction in an array, also returns number of instructions 
    int numOfInst = saveLinesToInstArr(meminFile, instArr);
    rewind(meminFile);
    //updateMonitor(instArr, IORegisters, frameBuffer);

    readMem(meminFile, memory);
    readIrq2(irq2inFile);
    // iterates through instruction array and executes actions
    executeOperations(instArr, registerArray, numOfInst, traceFile, hwregtraceFile, IORegisters, cyclesFile, display7seg, ledsFile);
    writeDiskToFile(diskoutFile);
    writeRegistersToFile(regoutFile);
    writeMonitor(monitor, frameBuffer);
    writeMemOut(memOut, memory);
    writeMonitorYuv(monitorYuv, frameBuffer);

    //closing files
    fclose(meminFile);
    fclose(diskinFile);
    fclose(irq2inFile);
    fclose(memOut);
    fclose(regoutFile);
    fclose(traceFile);
    fclose(hwregtraceFile);
    fclose(cyclesFile);
    fclose(ledsFile);
    fclose(display7seg);
    fclose(diskoutFile);
    fclose(monitor);
    fclose(monitorYuv);



    return 0;
}

int saveLinesToInstArr(FILE* meminFile, Instruction* instArr) {
    int PC = 0;
    int numOfInst = 0;
    char line[256];

    while (fgets(line, sizeof(line), meminFile)) {

        int opcode, rd, rs, rt;
        int imm = 0;
        Instruction instr;

        // Parse the first line to extract opcode, rd, rs, rt
        if (sscanf(line, "%2x%1x%1x%1x", &opcode, &rd, &rs, &rt) != 4) {
            fprintf(stderr, "Invalid instruction format: %s", line);
            continue;
        }

        instr.opcode = opcode;
        instr.rd = rd;
        instr.rs = rs;
        instr.rt = rt;
        instr.imm = 0;  // Default imm value
        instr.PC = PC;

        // If rd, rs, or rt is 1, read the next line to get imm
        if (rd == 1 || rs == 1 || rt == 1) {
            if (fgets(line, sizeof(line), meminFile)) {
                // Read the imm value as a hexadecimal
                unsigned int imm_unsigned;
                if (sscanf(line, "%5x", &imm_unsigned) != 1) {
                    fprintf(stderr, "Invalid imm format: %s", line);
                }
                else {
                    // Check if the value is negative
                    if (imm_unsigned & 0x80000) {  // Check if the sign bit (for 20-bit value) is set
                        imm = imm_unsigned | 0xFFF00000;  // Sign-extend to 32 bits
                    }
                    else {
                        imm = imm_unsigned;
                    }
                    instr.imm = imm;
                }
                PC++;
            }
        }
        PC++;
        instArr[numOfInst++] = instr;
    }
    return numOfInst;
}


void executeOperations(Instruction* instArr, int* R, int numOfInst, FILE* traceFile, FILE* hwregtraceFile, int* IORegisters, FILE* cyclesFile, FILE* display7seg, FILE* ledsFile) {
    int i = 0;
    int PC = 1;
    int line = 1;
    int j = 0;
    int irq = 0;
    int inIrq = 0; // if we are in irq change to 1
    int offsetRow = 0;
    int offsetCol = 0;


    while (i < numOfInst) {
        PC = instArr[i].PC;
        irq = ((IORegisters[0] && IORegisters[3]) || (IORegisters[1] && IORegisters[4]) || (IORegisters[2] && IORegisters[5]));
        if ((irq == 1) && (inIrq == 0)) // if irq =1 and we are not in irq
        {

            IORegisters[7] = PC; // irqreturn = pc
            PC = IORegisters[6]; // pc = irqhandler
            i = findInstructionIndexByPC(instArr, numOfInst, PC);
            inIrq = 1;

        }


        int opcode = instArr[i].opcode;
        int rd = instArr[i].rd;
        int rs = instArr[i].rs;
        int rt = instArr[i].rt;
        int imm = instArr[i].imm;

        if (rd == 1 || rs == 1 || rt == 1) {
            R[1] = imm;
        }
        else {
            R[1] = 0;
        }


        writeTraceToFile(PC, instArr[i], R, traceFile);

        if (opcode == 0x00) { // add
            R[rd] = R[rs] + R[rt];
            i++;
        }
        else if (opcode == 0x01) { // sub
            R[rd] = R[rs] - R[rt];
            i++;
        }
        else if (opcode == 0x02) { // mul
            R[rd] = R[rs] * R[rt];
            i++;
        }
        else if (opcode == 0x03) { // and
            R[rd] = R[rs] & R[rt];
            i++;
        }
        else if (opcode == 0x04) { // or
            R[rd] = R[rs] | R[rt];
            i++;
        }
        else if (opcode == 0x05) { // xor
            R[rd] = R[rs] ^ R[rt];
            i++;
        }
        else if (opcode == 0x06) { // sll
            R[rd] = R[rs] << R[rt];
            i++;
        }
        else if (opcode == 0x07) { // sra
            R[rd] = R[rs] >> R[rt]; // Arithmetic shift
            if (R[rs] & 0x80000000) { // If the sign bit is 1
                R[rd] |= ~((1 << (32 - R[rt])) - 1); // Sign extend
            }
            i++;
        }
        else if (opcode == 0x08) { // srl
            R[rd] = (uint32_t)R[rs] >> R[rt]; // Logical shift
            i++;
        }
        else if (opcode == 0x09) { // beq
            if (R[rs] == R[rt]) {
                PC = R[rd];
                i = findInstructionIndexByPC(instArr, numOfInst, PC);
            }
            else {
                i++;
            }
        }
        else if (opcode == 0x0A) { // bne
            if (R[rs] != R[rt]) {
                PC = R[rd];
                i = findInstructionIndexByPC(instArr, numOfInst, PC);
            }
            else {
                i++;
            }
        }
        else if (opcode == 0x0B) { // blt
            if (R[rs] < R[rt]) {
                PC = R[rd];
                i = findInstructionIndexByPC(instArr, numOfInst, PC);
            }
            else {
                i++;
            }
        }
        else if (opcode == 0x0C) { // bgt
            if (R[rs] > R[rt]) {
                PC = R[rd];
                i = findInstructionIndexByPC(instArr, numOfInst, PC);
            }
            else {
                i++;
            }
        }
        else if (opcode == 0x0D) { // ble
            if (R[rs] <= R[rt]) {
                PC = R[rd];
                i = findInstructionIndexByPC(instArr, numOfInst, PC);
            }
            else {
                i++;
            }
        }
        else if (opcode == 0x0E) { // bge
            if (R[rs] >= R[rt]) {
                PC = R[rd];
                i = findInstructionIndexByPC(instArr, numOfInst, PC);
            }
            else {
                i++;
            }
        }
        else if (opcode == 0x0F) { // jal
            if (rd == 1 || rs == 1 || rt == 1) {
                R[rd] = PC + 2; // Next instruction address
            }
            else {
                R[rd] = PC + 1;
            }
            PC = R[rs];
            i = findInstructionIndexByPC(instArr, numOfInst, PC);
        }
        else if (opcode == 0x10) { // lw
            R[rd] = memory[R[rs] + R[rt]];
            i++;
        }
        else if (opcode == 0x11) { // sw
            memory[R[rs] + R[rt]] = R[rd];
            i++;
        }
        else if (opcode == 0x12) { // reti
            PC = IORegisters[7];
            inIrq = 0; // we are not in irq already
            i = findInstructionIndexByPC(instArr, numOfInst, PC);
        }

        else if (opcode == 0x13) { // in
            int regNumber = R[rs] + R[rt];
            R[rd] = IORegisters[regNumber];
            int data = IORegisters[regNumber];
            fprintf(hwregtraceFile, "%d READ %s %08X\n", clockCycles, registerNames[regNumber], data);
            fflush(hwregtraceFile);
            i++;
        }
        else if (opcode == 0x14) { // out
            int regNumber = R[rs] + R[rt];
            IORegisters[regNumber] = R[rd];
            int data = IORegisters[regNumber];
            //write to trace file
            fprintf(hwregtraceFile, "%d WRITE %s %08X\n", clockCycles, registerNames[regNumber], data);
            fflush(hwregtraceFile);
            //write to display7seg file
            if (regNumber == 10)
            {
                fprintf(display7seg, "%d %08X\n", clockCycles, IORegisters[10]);
            }
            //write to leds file
            if (regNumber == 9) {
                fprintf(ledsFile, "%d %08X\n", clockCycles, IORegisters[9]);
            }
            //update monitor
            if (IORegisters[22] == 1) // if the monitorcmd is 1 and the opcode is out
            {
                offsetRow = IORegisters[20] / 256; // the framebuffer line
                offsetCol = IORegisters[20] - (256 * offsetRow); //the framebuffer column
                frameBuffer[offsetRow][offsetCol] = IORegisters[21]; // change data according to the offset
            }
            i++;

        }
        else if (opcode == 0x15) { // halt
            clockCycles++;
            printf("Halt execution.\n");
            break;
        }
        else {
            printf("Invalid opcode: %x\n instruction: %d", opcode, i);
        }
        handleDiskOperations();

        //check if ir0 happend
        if (IORegisters[11] == 1)//if timerenable is 1
        {
            if (IORegisters[12] == IORegisters[13]) // if timercurrent = timermax
            {
                IORegisters[3] = 1; // irqstatus0 = 1
                IORegisters[12] = 0; // timercurrent =0
            }
            else IORegisters[12] += 1; // if we are not in the timermax, timercurrent +1
        }

        //if irq2 is happend
        if (IORegisters[8] == irq2In[j]) // if the current cycle is equal to irq2 call 
        {
            IORegisters[5] = 1;
            j++;
        }
        clockCycles++;
        IORegisters[8] = clockCycles;
        line++;
    }

    fprintf(cyclesFile, "%d\n", clockCycles);
    fflush(cyclesFile);
}

void writeRegistersToFile(FILE* regoutFile) {
    for (int i = 2; i < 16; i++) {
        fprintf(regoutFile, "%08X\n", registerArray[i]);
    }
    fflush(regoutFile);
}

void writeTraceToFile(int PC, Instruction inst, int* R, FILE* traceFile) {
    // Write the PC and instruction fields
    fprintf(traceFile, "%03X %02X%01X%01X%01X", PC, inst.opcode, inst.rd, inst.rs, inst.rt);
    // Write the values of the R array
    for (int i = 0; i < 16; i++) {
        fprintf(traceFile, " %08X", R[i]);
    }
    fprintf(traceFile, "\n");
    fflush(traceFile);
}

int findInstructionIndexByPC(Instruction* instArr, int numOfInst, int wanted_PC) {
    for (int i = 0; i < numOfInst; i++) {
        if (instArr[i].PC == wanted_PC) {
            return i; // Return the index if PC matches
        }
    }
    return -1; // Return -1 if no match is found
}

void readMem(FILE* memFile, int* mem) {
    // Array to store the line including the newline character and null terminator
    char line[7];

    for (int i = 0; i < 4096; i++) { // Iterate over all lines
        if (fgets(line, sizeof(line), memFile)) {
            // Remove newline character if present
            line[strcspn(line, "\n")] = 0;

            // Convert string to integer assuming hexadecimal base
            mem[i] = (int)strtol(line, NULL, 16);
        }
        else {
            // Handle the case where fgets fails (e.g., end of file reached prematurely)
            mem[i] = 0;
        }
    }
}

void writeMemOut(FILE* memOut, int* memOutArr)
{
    for (int i = 0; i < 4096; ++i) {
        fprintf(memOut, "%05X\n", memOutArr[i]);
    }
    fflush(memOut);

}

void writeMonitor(FILE* monitor, int frameBuffer[256][256])
{
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            fprintf(monitor, "%02X\n", frameBuffer[i][j]);
        }
        fflush(monitor);
    }
}

void initializeDisk(FILE* diskinFile) {
    char line[7]; // Buffer for reading 5 hex digits + null terminator
    int res;

    for (int sector = 0; sector < SECTORS; sector++) {
        for (int row = 0; row < NUM_OF_INTS_IN_SECTOR; row++) {
            if (fgets(line, sizeof(line), diskinFile)) {
                if (sscanf(line, "%x", &disk[sector][row]) != 1) {
                    disk[sector][row] = 0; // Set default value if parsing fails
                }
                else {
                    if (disk[sector][row] & 0x80000) { // Negative value handling
                        res = disk[sector][row];
                        res = ~res;
                        res = res & 0XFFFFF;
                        res += 1;
                        res *= (-1);
                        disk[sector][row] = res;
                    }
                }
            }
            else {
                disk[sector][row] = 0; // Default value if end of file is reached
            }
        }
    }
}
void handleDiskOperations() {
    // Check if the disk is free and there is a command to execute
    if (IORegisters[17] == 0 && IORegisters[14] != 0) {
        IORegisters[17] = 1; // Set disk status to busy
        if (IORegisters[14] == 1) { // Reading from disk to memory
            for (int i = 0; i < 128; i++) {
                memory[IORegisters[16] + i] = disk[IORegisters[15]][i];
            }
        }
        else if (IORegisters[14] == 2) { // Writing from memory to disk
            for (int i = 0; i < 128; i++) {
                disk[IORegisters[15]][i] = memory[IORegisters[16] + i];
            }
        }

        disk_start_cyc = IORegisters[8]; // Record the start cycle
    }

    // Check if 1024 clock cycles have passed since the start of the operation
    if (IORegisters[8] == disk_start_cyc + 1024) {
        IORegisters[4] = 1; // Set irq1status to 1 (indicating completion)
        IORegisters[17] = 0; // Set disk status to free
        IORegisters[14] = 0; // Clear disk command
    }
}

void writeDiskToFile(FILE* diskoutFile) {
    for (int sector = 0; sector < SECTORS; sector++) {
        for (int row = 0; row < NUM_OF_INTS_IN_SECTOR; row++) {
            // Write the value as a 5-digit hexadecimal number
            fprintf(diskoutFile, "%05X\n", disk[sector][row]);
            fflush(diskoutFile);
        }
    }
}
void readIrq2(FILE* irq2InFile)
{
    //convert a written file to an array
    int PC = 0;
    char line[5];
    for (PC = 0; PC < 4096; PC++) // pass over all the file
    {
        if (fgets(line, sizeof(line), irq2InFile))
        {
            irq2In[PC] = atoi(line); // change string to int
        }
    }

}
void checkIfIrq2Happend() {
    for (int i = 0; i < 4096; i++) {
        if (IORegisters[8] == irq2In[i]) // if the current cycle is equal to irq2 call 
        {
            IORegisters[5] = 1;
        }
    }
}

void writeMonitorYuv(FILE* monitorYuv, int frameBuffer[256][256]) {
    unsigned char yuvFrame[256 * 256];  // Create a buffer to store YUV data
    int i, j;

    // Convert each pixel in the frame buffer to YUV (assuming grayscale for simplicity)
    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) {
            // Assuming that the Y component is stored directly in the frameBuffer
            yuvFrame[i * 256 + j] = (unsigned char)frameBuffer[i][j];
        }
    }

    // Write the YUV data to the file
    fwrite(yuvFrame, 1, sizeof(yuvFrame), monitorYuv);
    fflush(monitorYuv);
}



