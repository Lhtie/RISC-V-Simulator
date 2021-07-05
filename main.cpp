#include <iostream>
#include "tomasulo.hpp"
using namespace std;

unsigned char mem[500000];
unsigned pc;

void input_data(){
    char ch;
    unsigned pos;
    cin >> ch;
    while (ch == '@'){
        cin >> hex >> pos;
        while ((cin >> ch) && ch != '@'){
            unsigned char x = (ch <= '9' ? ch - '0' : ch - 'A' + 10) * 16;
            cin >> ch;
            x += ch <= '9' ? ch - '0' : ch - 'A' + 10;
            mem[pos++] = x;
        }
    }
}

int main() {
    input_data();
    tomasulo();
    return 0;
}
