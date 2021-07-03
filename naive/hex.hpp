#ifndef RISC_V_SIMULATOR_HEX_HPP
#define RISC_V_SIMULATOR_HEX_HPP

extern unsigned char mem[500000];

unsigned get(unsigned x, int l, int r){
    x >>= l;
    return x & ((1u << (r - l + 1)) - 1);
}
void set(unsigned &x, int l, int r, int unsigned y){
    x &= ~(((1u << (r - l + 1)) - 1) << l);
    x += (y & ((1u << (r - l + 1)) - 1)) << l;
}
unsigned sext(unsigned x, unsigned n){
    if (x & (1u << (n - 1))){
        return x - (1u << n);
    } else return x;
}

#endif //RISC_V_SIMULATOR_HEX_HPP
