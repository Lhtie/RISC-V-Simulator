#ifndef CODE_EXECUTE_HPP
#define CODE_EXECUTE_HPP

#include <iostream>
#include "decode.hpp"
#include "hex.hpp"
using namespace std;

extern unsigned char mem[500000];

pair<unsigned, pair<bool, unsigned> > execute(optype op, unsigned imm, unsigned v1, unsigned v2, unsigned pc_c){
    pair<bool, unsigned> nil = make_pair(false, 0);
    unsigned pos = v1 + imm;
    switch (op) {
        case lui: return make_pair(imm, nil);
        case auipc: return make_pair(pc_c + imm, nil);
        case jal: return make_pair(pc_c + 4, make_pair(true, pc_c + imm));
        case jalr: return make_pair(pc_c + 4, make_pair(true, (v1 + imm) & ~1));
        case beq: return make_pair(0, make_pair(v1 == v2, pc_c + imm));
        case bne: return make_pair(0, make_pair(v1 != v2, pc_c + imm));
        case blt: return make_pair(0, make_pair((signed)v1 < (signed)v2, pc_c + imm));
        case bge: return make_pair(0, make_pair((signed)v1 >= (signed)v2, pc_c + imm));
        case bltu: return make_pair(0, make_pair(v1 < v2, pc_c + imm));
        case bgeu: return make_pair(0, make_pair(v1 >= v2, pc_c + imm));
        case lb: return make_pair(sext((unsigned)mem[pos], 8), nil);
        case lh: return make_pair(sext(((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos], 16), nil);
        case lw: return make_pair(((unsigned)mem[pos+3] << 24) + ((unsigned)mem[pos+2] << 16) + ((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos], nil);
        case lbu: return make_pair((unsigned)mem[pos], nil);
        case lhu: return make_pair(((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos], nil);
        case addi: return make_pair(v1 + imm, nil);
        case slti: return make_pair((signed)v1 < (signed)imm, nil);
        case sltiu: return make_pair(v1 < imm, nil);
        case xori: return make_pair(v1 ^ imm, nil);
        case ori: return make_pair(v1 | imm, nil);
        case andi: return make_pair(v1 & imm, nil);
        case slli: return make_pair(v1 << get(imm, 0, 4), nil);
        case srli: return make_pair(v1 >> get(imm, 0, 4), nil);
        case srai: return make_pair(sext(v1 >> get(imm, 0, 4), 32 - get(imm, 0, 4)), nil);
        case add: return make_pair(v1 + v2, nil);
        case sub: return make_pair(v1 - v2, nil);
        case sll: return make_pair(v1 << get(v2, 0, 4), nil);
        case slt: return make_pair((signed)v1 < (signed)v2, nil);
        case sltu: return make_pair(v1 < v2, nil);
        case xor_: return make_pair(v1 ^ v2, nil);
        case srl: return make_pair(v1 >> get(v2, 0, 4), nil);
        case sra: return make_pair(sext(v1 >> get(v2, 0, 4), 32 - get(v2, 0, 4)), nil);
        case or_: return make_pair(v1 | v2, nil);
        case and_: return make_pair(v1 & v2, nil);
        default: break;
    }
    return make_pair(0, nil);
}

#endif //CODE_EXECUTE_HPP
