#include <iostream>
#include "decode.hpp"
using namespace std;

unsigned reg[32];
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
void run() {
    pc = 0;
    for (; ; ){
        unsigned command = fetch(pc);
        if (command == 267388179) break;
        decode(command);
        cout << hex << command << endl;
        cout << dec << (unsigned)mem[99424] << ' ' << reg[15] << endl;
//        for (int i = 0; i < 32; ++i)
//            cout << reg[i] << ' ';
//        cout << endl;
    }
    cout << (reg[10] & 255u) << endl;
}

int main() {
    freopen("../../data/basicopt1.data", "r", stdin);
    freopen("output_std.txt", "w", stdout);
    input_data();
    run();
    return 0;
}
