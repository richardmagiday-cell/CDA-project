/* CDA 3103 project
This program is written by: Richard Magiday */

#include "spimcore.h"

/* ALU */
/* 10 Points */
void ALU(unsigned A, unsigned B, char ALUControl, unsigned *ALUresult, char *Zero)
{
    switch (ALUControl)
    {
    case 0: // Z = A + B
        *ALUresult = A + B;
        break;
    case 1: // Z = A - B
        *ALUresult = A - B;
        break;
    case 2: // if A < B (signed), Z = 1; otherwise, Z = 0
        *ALUresult = ((int)A < (int)B) ? 1 : 0;
        break;
    case 3: // if A < B (unsigned), Z = 1; otherwise, Z = 0
        *ALUresult = (A < B) ? 1 : 0;
        break;
    case 4: // Z = A AND B
        *ALUresult = A & B;
        break;
    case 5: // Z = A OR B
        *ALUresult = A | B;
        break;
    case 6: // Z = Shift B left by 16 bits
        *ALUresult = B << 16;
        break;
    case 7: // Z = NOT A
        *ALUresult = ~A;
        break;
    default:
        printf("Invalid ALU Control Code\n");
        exit(1);
    }

    *Zero = (*ALUresult == 0) ? 1 : 0;
}

/* instruction fetch */
/* 10 Points */
int instruction_fetch(unsigned PC, unsigned *Mem, unsigned *instruction)
{
    if ((PC >> 2) >= (65536 >> 2))
        return 1;

    if (PC % 4 != 0)
        return 1;

    *instruction = Mem[PC >> 2];

    return 0;
}

/* instruction partition */
/* 10 Points */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1, unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
    *op = (instruction >> 26) & 0x3F;
    *r1 = (instruction >> 21) & 0x1F;
    *r2 = (instruction >> 16) & 0x1F;
    *r3 = (instruction >> 11) & 0x1F;
    *funct = instruction & 0x3F;
    *offset = instruction & 0xFFFF;
    *jsec = instruction & 0x3FFFFFF;
}

/* instruction decode */
/* 15 Points */
int instruction_decode(unsigned op, struct_controls *controls)
{
    controls->RegDst = 0;
    controls->Jump = 0;
    controls->Branch = 0;
    controls->MemRead = 0;
    controls->MemtoReg = 0;
    controls->ALUOp = 0;
    controls->MemWrite = 0;
    controls->ALUSrc = 0;
    controls->RegWrite = 0;

    switch (op)
    {
    case 0x00: // R-type
        controls->RegDst = 1;
        controls->ALUOp = 7;
        controls->RegWrite = 1;
        break;
    case 0x02: // Jump
        controls->Jump = 1;
        break;
    case 0x04: // Branch on Equal
        controls->Branch = 1;
        break;
    case 0x08: // Add Immediate
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
        break;
    case 0x0A: // Set Less Than Immediate (signed)
        controls->ALUOp = 2;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
        break;
    case 0x0B: // Set Less Than Immediate Unsigned
        controls->ALUOp = 3;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
        break;
    case 0x0F: // Load Upper Immediate
        controls->ALUOp = 6;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
        break;
    case 0x23: // Load Word
        controls->MemRead = 1;
        controls->MemtoReg = 1;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
        break;
    case 0x2B: // Store Word
        controls->MemWrite = 1;
        controls->ALUSrc = 1;
        break;
    default:
        return 1;
    }

    return 0;
}

/* Read Register */
/* 5 Points */
void read_register(unsigned r1, unsigned r2, unsigned *Reg, unsigned *data1, unsigned *data2)
{
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}

/* Sign Extend */
/* 10 Points */
void sign_extend(unsigned offset, unsigned *extended_value)
{
    if (offset & 0x8000)
        *extended_value = offset | 0xFFFF0000;
    else
        *extended_value = offset & 0x0000FFFF;
}

/* ALU operations */
/* 10 Points */
int ALU_operations(unsigned data1, unsigned data2, unsigned extended_value, unsigned funct, char ALUOp, char ALUSrc, unsigned *ALUresult, char *Zero)
{
    char ALUControl;
    unsigned operand2 = ALUSrc ? extended_value : data2;

    if (ALUOp == 7)
    { // R-type: use funct field
        switch (funct)
        {
        case 0x20:
            ALUControl = 0;
            break; // ADD
        case 0x22:
            ALUControl = 1;
            break; // SUB
        case 0x2A:
            ALUControl = 2;
            break; // SLT (signed)
        case 0x2B:
            ALUControl = 3;
            break; // SLTU (unsigned)
        case 0x24:
            ALUControl = 4;
            break; // AND
        case 0x25:
            ALUControl = 5;
            break; // OR
        default:
            return 1;
        }
    }
    else if (ALUOp == 0)
    {
        // BEQ (ALUSrc=0): subtract to test equality
        // ADDI, LW, SW (ALUSrc=1): add for address/immediate
        ALUControl = ALUSrc ? 0 : 1;
    }
    else
    {
        ALUControl = ALUOp; // 2=SLTI, 3=SLTIU, 6=LUI
    }

    ALU(data1, operand2, ALUControl, ALUresult, Zero);
    return 0;
}

/* Read / Write Memory */
/* 10 Points */
int rw_memory(unsigned ALUresult, unsigned data2, char MemWrite, char MemRead, unsigned *memdata, unsigned *Mem)
{
    if (MemWrite == 1)
    {
        if ((ALUresult % 4) == 0)
            Mem[ALUresult >> 2] = data2;
        else
            return 1;
    }

    if (MemRead == 1)
    {
        if ((ALUresult % 4) == 0)
            *memdata = Mem[ALUresult >> 2];
        else
            return 1;
    }

    return 0;
}

/* Write Register */
/* 10 Points */
void write_register(unsigned r2, unsigned r3, unsigned memdata, unsigned ALUresult, char RegWrite, char RegDst, char MemtoReg, unsigned *Reg)
{
    if (RegWrite)
    {
        unsigned dest = RegDst ? r3 : r2;
        unsigned value = MemtoReg ? memdata : ALUresult;
        Reg[dest] = value;
    }
}

/* PC update */
/* 10 Points */
void PC_update(unsigned jsec, unsigned extended_value, char Branch, char Jump, char Zero, unsigned *PC)
{
    *PC += 4;

    if (Branch && Zero)
        *PC += (extended_value << 2);

    if (Jump)
        *PC = (*PC & 0xF0000000) | (jsec << 2);
}
