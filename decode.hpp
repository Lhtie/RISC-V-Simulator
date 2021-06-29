#ifndef RISC_V_SIMULATOR_DECODE_HPP
#define RISC_V_SIMULATOR_DECODE_HPP

#include <iostream>
#include "hex.hpp"
using namespace std;

extern unsigned reg[32];
extern unsigned char mem[500000];
extern unsigned pc;

void type_U(unsigned x, unsigned &rd, unsigned &imm){
    rd = get(x, 7, 11);
    imm = 0;
    set(imm, 12, 31, get(x, 12, 31));
}
void type_J(unsigned x, unsigned &rd, unsigned &imm){
    rd = get(x, 7, 11);
    imm = 0;
    set(imm, 12, 19, get(x, 12, 19));
    set(imm, 11, 11, get(x, 20, 20));
    set(imm, 1, 10, get(x, 21, 30));
    set(imm, 20, 20, get(x, 31, 31));
    imm = sext(imm, 21);
}
void type_I(unsigned x, unsigned &rd, unsigned &rs1, unsigned &imm){
    rd = get(x, 7, 11);
    rs1 = get(x, 15, 19);
    imm = sext(get(x, 20, 31), 12);
}
void type_B(unsigned x, unsigned &imm, unsigned &rs1, unsigned &rs2){
    imm = 0;
    set(imm, 11, 11, get(x, 7, 7));
    set(imm, 1, 4, get(x, 8, 11));
    set(imm, 5, 10, get(x, 25, 30));
    set(imm, 12, 12, get(x, 31, 31));
    imm = sext(imm, 13);
    rs1 = get(x, 15, 19);
    rs2 = get(x, 20, 24);
}
void type_S(unsigned x, unsigned &imm, unsigned &rs1, unsigned &rs2){
    imm = 0;
    set(imm, 0, 4, get(x, 7, 11));
    set(imm, 5, 10, get(x, 25, 30));
    set(imm, 11, 11, get(x, 31, 31));
    imm = sext(imm, 12);
    rs1 = get(x, 15, 19);
    rs2 = get(x, 20, 24);
}
void type_R(unsigned x, unsigned &rd, unsigned &rs1, unsigned &rs2){
    rd = get(x, 7, 11);
    rs1 = get(x, 15, 19);
    rs2 = get(x, 20, 24);
}
void decode(unsigned command){
    unsigned opcode = get(command, 0, 6), imm, rd, rs1, rs2;
    switch (opcode) {
        case 55: {
            type_U(command, rd, imm);
            reg[rd] = imm;
            pc += 4;
            break;
        }
        case 23: {
            type_U(command, rd, imm);
            reg[rd] = pc + imm;
            pc += 4;
            break;
        }
        case 111: {
            type_J(command, rd, imm);
            if (rd) reg[rd] = pc + 4;
            pc += imm;
            break;
        }
        case 103: {
            type_I(command, rd, rs1, imm);
            unsigned t = pc + 4;
            pc = (reg[rs1] + imm) & ~1;
            if (rd) reg[rd] = t;
            break;
        }
        case 99: {
            type_B(command, imm, rs1, rs2);
            unsigned funct3 = get(command, 12, 14);
            switch (funct3) {
                case 0: {
                    if (reg[rs1] == reg[rs2]) pc += imm; else pc += 4;
                    break;
                }
                case 1: {
                    if (reg[rs1] != reg[rs2]) pc += imm; else pc += 4;
                    break;
                }
                case 4: {
                    if ((signed)reg[rs1] < (signed)reg[rs2]) pc += imm; else pc += 4;
                    break;
                }
                case 5: {
                    if ((signed)reg[rs1] >= (signed)reg[rs2]) pc += imm; else pc += 4;
                    break;
                }
                case 6: {
                    if (reg[rs1] < reg[rs2]) pc += imm; else pc += 4;
                    break;
                }
                case 7: {
                    if (reg[rs1] >= reg[rs2]) pc += imm; else pc += 4;
                    break;
                }
                default: break;
            }
            break;
        }
        case 3: {
            type_I(command, rd, rs1, imm);
            unsigned funct3 = get(command, 12, 14);
            switch (funct3) {
                case 0: {
                    reg[rd] = sext((unsigned)mem[reg[rs1] + imm], 8);
                    break;
                }
                case 1: {
                    unsigned pos = reg[rs1] + imm;
                    reg[rd] = sext(((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos], 16);
                    break;
                }
                case 2: {
                    unsigned pos = reg[rs1] + imm;
                    reg[rd] = ((unsigned)mem[pos+3] << 24) + ((unsigned)mem[pos+2] << 16) + ((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos];
                    break;
                }
                case 4: {
                    reg[rd] = (unsigned)mem[reg[rs1] + imm];
                    break;
                }
                case 5: {
                    unsigned pos = reg[rs1] + imm;
                    reg[rd] = ((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos];
                    break;
                }
                default: break;
            }
            pc += 4;
            break;
        }
        case 35: {
            type_S(command, imm, rs1, rs2);
            unsigned funct3 = get(command, 12, 14);
            switch (funct3) {
                case 0: {
                    mem[reg[rs1] + imm] = (unsigned char)get(reg[rs2], 0, 7);
                    break;
                }
                case 1: {
                    unsigned pos = reg[rs1] + imm;
                    mem[pos] = (unsigned char)get(reg[rs2], 0, 7), mem[pos+1] = (unsigned char)get(reg[rs2], 8, 15);
                    break;
                }
                case 2: {
                    unsigned pos = reg[rs1] + imm;
                    mem[pos] = (unsigned char)get(reg[rs2], 0, 7);
                    mem[pos+1] = (unsigned char)get(reg[rs2], 8, 15);
                    mem[pos+2] = (unsigned char)get(reg[rs2], 16, 23);
                    mem[pos+3] = (unsigned char)get(reg[rs2], 24, 31);
                    break;
                }
                default: break;
            }
            pc += 4;
            break;
        }
        case 19: {
            type_I(command, rd, rs1, imm);
            unsigned funct3 = get(command, 12, 14);
            switch (funct3) {
                case 0: reg[rd] = reg[rs1] + imm; break;
                case 2: reg[rd] = (signed)reg[rs1] < (signed)imm; break;
                case 3: reg[rd] = reg[rs1] < imm; break;
                case 4: reg[rd] = reg[rs1] ^ imm; break;
                case 6: reg[rd] = reg[rs1] | imm; break;
                case 7: reg[rd] = reg[rs1] & imm; break;
                case 1: {
                    unsigned shamt = get(imm, 0, 4);
                    reg[rd] = reg[rs1] << shamt;
                    break;
                }
                case 5: {
                    unsigned shamt = get(imm, 0, 4);
                    if (!get(imm, 10, 10)){
                        reg[rd] = reg[rs1] >> shamt;
                    } else {
                        reg[rd] = sext(reg[rs1] >> shamt, 32 - shamt);
                    }
                }
                default: break;
            }
            pc += 4;
            break;
        }
        case 51: {
            type_R(command, rd, rs1, rs2);
            unsigned funct3 = get(command, 12, 14), funct7 = get(command, 25, 31);
            switch (funct3) {
                case 0: {
                    if (funct7 >> 5) reg[rd] = reg[rs1] - reg[rs2];
                    else reg[rd] = reg[rs1] + reg[rs2];
                    break;
                }
                case 1: reg[rd] = reg[rs1] << get(reg[rs2], 0, 4); break;
                case 2: reg[rd] = (signed)reg[rs1] < (signed)reg[rs2]; break;
                case 3: reg[rd] = reg[rs1] < reg[rs2]; break;
                case 4: reg[rd] = reg[rs1] ^ reg[rs2]; break;
                case 5: {
                    unsigned shamt = get(reg[rs2], 0, 4);
                    if (funct7 >> 5) reg[rd] = sext(reg[rs1] >> shamt, 32 - shamt);
                    else reg[rd] = reg[rs1] >> shamt;
                }
                case 6: reg[rd] = reg[rs1] | reg[rs2]; break;
                case 7: reg[rd] = reg[rs1] & reg[rs2]; break;
                default: break;
            }
            pc += 4;
            break;
        }
        default: pc += 4, cout << "Error" << endl; break;
    }
}

#endif //RISC_V_SIMULATOR_DECODE_HPP
