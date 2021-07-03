#ifndef RISC_V_SIMULATOR_DECODE_HPP
#define RISC_V_SIMULATOR_DECODE_HPP

#include <iostream>
#include "hex.hpp"
using namespace std;

enum optype{lb, lh, lw, lbu, lhu, sb, sh, sw, lui, auipc, jal, jalr, beq, bne,
        blt, bge, bltu, bgeu, addi, slti, sltiu, xori, ori, andi, slli, srli,
        srai, add, sub, sll, slt, sltu, xor_, srl, sra, or_, and_};

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
void decode(unsigned command, optype &op, unsigned &imm, unsigned &rd, unsigned &rs1, unsigned &rs2){
    unsigned opcode = get(command, 0, 6);
    switch (opcode) {
        case 55: type_U(command, rd, imm), op = lui; break;
        case 23: type_U(command, rd, imm), op = auipc; break;
        case 111: type_J(command, rd, imm), op = jal; break;
        case 103: type_I(command, rd, rs1, imm), op = jalr; break;
        case 99:
            type_B(command, imm, rs1, rs2);
            switch (get(command, 12, 14)) {
                case 0: op = beq; break;
                case 1: op = bne; break;
                case 4: op = blt; break;
                case 5: op = bge; break;
                case 6: op = bltu; break;
                case 7: op = bgeu; break;
            }
            break;
        case 3:
            type_I(command, rd, rs1, imm);
            switch (get(command, 12, 14)) {
                case 0: op = lb; break;
                case 1: op = lh; break;
                case 2: op = lw; break;
                case 4: op = lbu; break;
                case 5: op = lhu; break;
            }
            break;
        case 35:
            type_S(command, imm, rs1, rs2);
            switch (get(command, 12, 14)) {
                case 0: op = sb; break;
                case 1: op = sh; break;
                case 2: op = sw; break;
            }
            break;
        case 19:
            type_I(command, rd, rs1, imm);
            switch (get(command, 12, 14)) {
                case 0: op = addi; break;
                case 2: op = slti; break;
                case 3: op = sltiu; break;
                case 4: op = xori; break;
                case 6: op = ori; break;
                case 7: op = andi; break;
                case 1: op = slli; break;
                case 5: op = (imm >> 10) ? srai : srli; break;
            }
            break;
        case 51:
            type_R(command, rd, rs1, rs2);
            switch (get(command, 12, 14)) {
                case 0: op = (get(command, 25, 31) >> 5) ? sub : add; break;
                case 1: op = sll; break;
                case 2: op = slt; break;
                case 3: op = sltu; break;
                case 4: op = xor_; break;
                case 5: op = (get(command, 25, 31) >> 5) ? sra : srl; break;
                case 6: op = or_; break;
                case 7: op = and_; break;
            }
            break;
        default: break;
    }
}

#endif //RISC_V_SIMULATOR_DECODE_HPP
