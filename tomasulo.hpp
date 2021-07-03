#ifndef RISC_V_SIMULATOR_TOMASULO_HPP
#define RISC_V_SIMULATOR_TOMASULO_HPP

#include "decode.hpp"
#include "execute.hpp"

const int N_reg = 32, N_fq =  32, N_rs = 32, N_rob = 32, N_slb = 32;
extern unsigned char mem[500000];
extern unsigned pc;

struct Register{
    unsigned reg, tag;
} reg_prev[N_reg], reg_nxt[N_reg];

bool stall_fetch_queue = false;
bool stall_fetch_to_issue = true;
bool stall_issue_to_fetch = true;
bool stall_issue_to_slb = true;
bool stall_issue_to_rs = true;
bool stall_issue_to_rob = true;
bool stall_issue_to_regfile = true;
bool stall_rs_to_ex = true;
bool stall_ex_to_rob = true;
bool stall_ex_to_rs = true;
bool stall_ex_to_slb = true;
bool stall_rob_to_commit = true;
bool stall_commit_to_regfile = true;
bool stall_slb_to_rob_prev = true;
bool stall_slb_to_rob_nxt = true;
bool stall_slb_to_rs_prev = true;
bool stall_slb_to_rs_nxt = true;
bool stall_slb_to_slb_prev = true;
bool stall_slb_to_slb_nxt = true;
bool stall_clear_rs = true;
bool stall_clear_slb = true;
bool stall_clear_rob = true;
bool stall_clear_fq = true;
bool stall_clear_regtag = true;

struct Fetch_Queue{
    struct node{
        unsigned inst, pc;
    } queue[N_fq + 1];
    unsigned head, tail, size;

    Fetch_Queue() : head(1), tail(1), size(0) {}
    void push(unsigned x, unsigned y) {
        queue[tail] = (node) {x, y};
        tail = tail % N_fq + 1, size++;
    }
    void pop(){ head = head % N_fq + 1, size--; }
    node &operator[](unsigned pos) { return queue[pos]; }
    node &front(){ return queue[head];}
} fq_prev, fq_nxt;

struct Reservation_Station{
    bool busy;
    optype op;
    unsigned rob_id, q1, q2, v1, v2, imm, rd, pc_c;
} rs_prev[N_rs], rs_nxt[N_rs];

struct Reorder_Buffer{
    struct node{
        bool stall;
        optype op;
        unsigned rd, rs_id, slb_id, inst;
        pair<unsigned, pair<bool, unsigned>> res;
    } queue[N_rob + 1];
    unsigned head, tail, size;

    Reorder_Buffer() : head(1), tail(1), size(0) {}
    node &operator[](unsigned pos) { return queue[pos]; }
    node &front(){ return queue[head]; }
} rob_prev, rob_nxt;

struct Store_Load_Buffer{
    struct node{
        optype op;
        unsigned time, rob_id, rd, q1, q2, v1, v2, imm;
    } queue[N_slb + 1];
    unsigned head, tail, size;

    Store_Load_Buffer() : head(1), tail(1), size(0) {}
    node &operator[](unsigned pos) { return queue[pos]; }
    node &front(){ return queue[head]; }
} slb_prev, slb_nxt;

struct fetch_to_issue_type{ unsigned inst, pc; } fetch_to_issue;

bool issue_to_fetch;

struct issue_to_rob_type{
    optype op;
    unsigned tail, id, rd, rs_id, slb_id, inst;
} issue_to_rob;

struct issue_to_slb_type{
    optype op;
    unsigned tail, id, time, rob_id, rd, rs1, rs2, imm;
} issue_to_slb;

struct issue_to_rs_type{
    optype op;
    unsigned id, rob_id, imm, rd, rs1, rs2, pc_c;
} issue_to_rs;

struct issue_to_regfile_type{ unsigned rd, rob_id; } issue_to_regfile;

struct rs_to_ex_type{
    optype op;
    unsigned imm, v1, v2, pc_c, rob_id, rs_id;
} rs_to_ex;

struct ex_to_rob_type{
    unsigned rob_id;
    pair<unsigned, pair<bool, unsigned>> res;
} ex_to_rob, slb_to_rob_prev, slb_to_rob_nxt;

struct ex_to_rs_or_slb_type{
    unsigned rob_id, res;
}ex_to_rs, ex_to_slb, slb_to_rs_prev, slb_to_rs_nxt, slb_to_slb_prev, slb_to_slb_nxt;

struct rob_to_commit_type{
    optype op;
    unsigned rd, rob_id, slb_id, inst;
    pair<unsigned, pair<bool, unsigned>> res;
} rob_to_commit;

struct commit_to_regfile_type{
    optype op;
    unsigned rd, res;
} commit_to_regfile;

unsigned fetch(unsigned pos){
    return ((unsigned)mem[pos+3] << 24) + ((unsigned)mem[pos+2] << 16) + ((unsigned)mem[pos+1] << 8) + (unsigned)mem[pos];
}
void run_inst_fetch_queue(){
    /*
    在这一部分你需要完成的工作：
    1. 实现一个先进先出的指令队列
    2. 读取指令并存放到指令队列中
    3. 准备好下一条issue的指令
    tips: 考虑边界问题（满/空...）
    */
    stall_fetch_to_issue = true;
    if (!stall_clear_fq){
        fq_nxt.head = fq_nxt.tail = 1, fq_nxt.size = 0;
        return ;
    }
    stall_fetch_queue = fq_prev.size == N_fq;
    if (!stall_fetch_queue) {
        fq_nxt.push(fetch(pc), pc);
        pc += 4;
    }
    if (!stall_issue_to_fetch && issue_to_fetch){
        if (fq_prev.size > 1) {
            const auto &res = fq_prev[fq_prev.head % N_fq + 1];
            fetch_to_issue.inst = res.inst, fetch_to_issue.pc = res.pc;
            stall_fetch_to_issue = false;
        }
    } else {
        if (fq_prev.size) {
            const auto &res = fq_prev[fq_prev.head];
            fetch_to_issue.inst = res.inst, fetch_to_issue.pc = res.pc;
            stall_fetch_to_issue = false;
        }
    }
    if (!stall_issue_to_fetch){
        if (issue_to_fetch) fq_nxt.pop();
    }
}
void run_issue(){
    /*
    在这一部分你需要完成的工作：
    1. 从run_inst_fetch_queue()中得到issue的指令
    2. 对于issue的所有类型的指令向ROB申请一个位置（或者也可以通过ROB预留位置），并通知regfile修改相应的值
    2. 对于 非 Load/Store的指令，将指令进行分解后发到Reservation Station
      tip: 1. 这里需要考虑怎么得到rs1、rs2的值，并考虑如当前rs1、rs2未被计算出的情况，参考书上内容进行处理
           2. 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能
              而对于rs1、rs2不ready的情况，只需要stall即可，有兴趣的同学可以考虑下怎么样直接从EX完的结果更快的得到计算结果
    3. 对于 Load/Store指令，将指令分解后发到SLBuffer(需注意SLBUFFER也该是个先进先出的队列实现)
    tips: 考虑边界问题（是否还有足够的空间存放下一条指令）
    */
    stall_issue_to_fetch = stall_issue_to_rs = stall_issue_to_rob = stall_issue_to_slb = stall_issue_to_rob = stall_issue_to_regfile = true;
    if (!stall_fetch_to_issue) {
        stall_issue_to_fetch = false;
        issue_to_fetch = true;
        if (rob_prev.size < N_rob) {
            optype op;
            unsigned imm = 0, rd = 0, rs1 = 0, rs2 = 0, pc_c = fetch_to_issue.pc;
            decode(fetch_to_issue.inst, op, imm, rd, rs1, rs2);
            if (op <= sw) {
                if (slb_prev.size < N_slb) {
                    stall_issue_to_slb = stall_issue_to_rob = false;
                    unsigned rob_id = rob_prev.tail, slb_id = slb_prev.tail;
                    issue_to_rob = (issue_to_rob_type) {op, rob_id % N_rob + 1, rob_id, rd, 0, slb_id, fetch_to_issue.inst};
                    issue_to_slb = (issue_to_slb_type) {op, slb_id % N_slb + 1, slb_id, 3, rob_id, rd, rs1, rs2, imm};
                    if (op <= lhu)
                        if (rd) {
                            stall_issue_to_regfile = false;
                            issue_to_regfile = (issue_to_regfile_type) {rd, rob_id};
                        }
                } else issue_to_fetch = false;
            } else {
                unsigned x = 0;
                for (; x < N_rs; ++x) if (!rs_prev[x].busy) break;
                if (x < N_rs) {
                    stall_issue_to_rs = stall_issue_to_rob = false;
                    unsigned rob_id = rob_prev.tail;
                    issue_to_rob = (issue_to_rob_type) {op, rob_id % N_rob + 1, rob_id, rd, x, 0, fetch_to_issue.inst};
                    issue_to_rs = (issue_to_rs_type) {op, x, rob_id, imm, rd, rs1, rs2, pc_c};
                    if (op >= lui && op <= jalr || op >= addi)
                        if (rd) {
                            stall_issue_to_regfile = false;
                            issue_to_regfile = (issue_to_regfile_type) {rd, rob_id};
                        }
                } else issue_to_fetch = false;
            }
        } else issue_to_fetch = false;
    }
}
void run_regfile(){
    /*
    每个寄存器会记录Q和V，含义参考ppt。这一部分会进行写寄存器，内容包括：根据issue和commit的通知修改对应寄存器的Q和V。
    tip: 请注意issue和commit同一个寄存器时的情况
    */
    if (!stall_commit_to_regfile){
        if (commit_to_regfile.op <= lhu || commit_to_regfile.op >= lui) {
            if (commit_to_regfile.rd) {
                reg_nxt[commit_to_regfile.rd].reg = commit_to_regfile.res;
                reg_nxt[commit_to_regfile.rd].tag = 0;
            }
        } else {
            unsigned pos = commit_to_regfile.rd, v2 = commit_to_regfile.res;
            switch (commit_to_regfile.op) {
                case sb: mem[pos] = (unsigned char)get(v2, 0, 7); break;
                case sh: mem[pos] = (unsigned char)get(v2, 0, 7), mem[pos+1] = (unsigned char)get(v2, 8, 15); break;
                case sw:
                    mem[pos] = (unsigned char)get(v2, 0, 7), mem[pos+1] = (unsigned char)get(v2, 8, 15);
                    mem[pos+2] = (unsigned char)get(v2, 16, 23), mem[pos+3] = (unsigned char)get(v2, 24, 31);
                    break;
                default: break;
            }
        }
    }
    if (!stall_issue_to_regfile){
        reg_nxt[issue_to_regfile.rd].tag = issue_to_regfile.rob_id;
    }
    if (!stall_clear_regtag){
        for (int i = 0; i < N_reg; ++i)
            reg_nxt[i].tag = 0;
    }
}
void run_reservation(){
    /*
    在这一部分你需要完成的工作：
    1. 设计一个Reservation Station，其中要存储的东西可以参考CAAQA或其余资料，至少需要有用到的寄存器信息等
    2. 如存在，从issue阶段收到要存储的信息，存进Reservation Station（如可以计算也可以直接进入计算）
    3. 从Reservation Station或者issue进来的指令中选择一条可计算的发送给EX进行计算
    4. 根据上个周期EX阶段或者SLBUFFER的计算得到的结果遍历Reservation Station，更新相应的值
    */
    if (!stall_issue_to_rs){
        issue_to_rs_type &x = issue_to_rs;
        rs_nxt[x.id].busy = true;
        rs_nxt[x.id].op = x.op, rs_nxt[x.id].imm = x.imm, rs_nxt[x.id].rd = x.rd, rs_nxt[x.id].pc_c = x.pc_c, rs_nxt[x.id].rob_id = x.rob_id;
        if (!reg_prev[x.rs1].tag)
            rs_nxt[x.id].q1 = 0, rs_nxt[x.id].v1 = reg_prev[x.rs1].reg;
        else{
            unsigned rob_id = reg_prev[x.rs1].tag;
            if (!stall_ex_to_rs && ex_to_rs.rob_id == rob_id)
                rs_nxt[x.id].q1 = 0, rs_nxt[x.id].v1 = ex_to_rs.res;
            else if (!stall_slb_to_rs_prev && slb_to_rs_prev.rob_id == rob_id)
                rs_nxt[x.id].q1 = 0, rs_nxt[x.id].v1 = slb_to_rs_prev.res;
            else if (!rob_prev[rob_id].stall)
                rs_nxt[x.id].q1 = 0, rs_nxt[x.id].v1 = rob_prev[rob_id].res.first;
            else rs_nxt[x.id].q1 = rob_id;
        }
        if (!reg_prev[x.rs2].tag)
            rs_nxt[x.id].q2 = 0, rs_nxt[x.id].v2 = reg_prev[x.rs2].reg;
        else {
            unsigned rob_id = reg_prev[x.rs2].tag;
            if (!stall_ex_to_rs && ex_to_rs.rob_id == rob_id)
                rs_nxt[x.id].q2 = 0, rs_nxt[x.id].v2 = ex_to_rs.res;
            else if (!stall_slb_to_rs_prev && slb_to_rs_prev.rob_id == rob_id)
                rs_nxt[x.id].q2 = 0, rs_nxt[x.id].v2 = slb_to_rs_prev.res;
            else if (!rob_prev[rob_id].stall)
                rs_nxt[x.id].q2 = 0, rs_nxt[x.id].v2 = rob_prev[rob_id].res.first;
            else rs_nxt[x.id].q2 = rob_id;
        }
    }
    stall_rs_to_ex = true;
    for (unsigned i = 0; i < N_rs; ++i){
        if (rs_prev[i].busy && rs_prev[i].q1 == 0 && rs_prev[i].q2 == 0){
            stall_rs_to_ex = false;
            rs_to_ex = (rs_to_ex_type){rs_prev[i].op, rs_prev[i].imm, rs_prev[i].v1, rs_prev[i].v2, rs_prev[i].pc_c, rs_prev[i].rob_id, i};
            rs_nxt[i].busy = false;
            break;
        }
    }
    if (!stall_ex_to_rs){
        for (int i = 0; i < N_rs; ++i)
            if (rs_prev[i].busy){
                if (rs_prev[i].q1 == ex_to_rs.rob_id)
                    rs_nxt[i].q1 = 0, rs_nxt[i].v1 = ex_to_rs.res;
                if (rs_prev[i].q2 == ex_to_rs.rob_id)
                    rs_nxt[i].q2 = 0, rs_nxt[i].v2 = ex_to_rs.res;
            }
    }
    if (!stall_slb_to_rs_prev){
        for (int i = 0; i < N_rs; ++i)
            if (rs_prev[i].busy){
                if (rs_prev[i].q1 == slb_to_rs_prev.rob_id)
                    rs_nxt[i].q1 = 0, rs_nxt[i].v1 = slb_to_rs_prev.res;
                if (rs_prev[i].q2 == slb_to_rs_prev.rob_id)
                    rs_nxt[i].q2 = 0, rs_nxt[i].v2 = slb_to_rs_prev.res;
            }
    }
    if (!stall_clear_rs){
        for (int i = 0; i < N_rs; ++i)
            rs_nxt[i].busy = false;
        stall_rs_to_ex = true;
    }
}
void run_ex() {
    /*
    在这一部分你需要完成的工作：
    根据Reservation Station发出的信息进行相应的计算
    tips: 考虑如何处理跳转指令并存储相关信息
          Store/Load的指令并不在这一部分进行处理
    */
    stall_ex_to_rob = stall_ex_to_rs = stall_ex_to_slb = true;
    if (!stall_rs_to_ex){
        rs_to_ex_type &x = rs_to_ex;
        auto res = execute(x.op, x.imm, x.v1, x.v2, x.pc_c);
        stall_ex_to_rob = false;
        ex_to_rob = (ex_to_rob_type){x.rob_id, res};
        stall_ex_to_rs = false;
        ex_to_rs = (ex_to_rs_or_slb_type){x.rob_id, res.first};
        stall_ex_to_slb = false;
        ex_to_slb = (ex_to_rs_or_slb_type){x.rob_id, res.first};
    }
}
void run_slbuffer(){
    /*
    在这一部分中，由于SLBUFFER的设计较为多样，在这里给出两种助教的设计方案：
    1. 1）通过循环队列，设计一个先进先出的SLBUFFER，同时存储 head1、head2、tail三个变量。
       其中，head1是真正的队首，记录第一条未执行的内存操作的指令；tail是队尾，记录当前最后一条未执行的内存操作的指令。
       而head2负责确定处在head1位置的指令是否可以进行内存操作，其具体实现为在ROB中增加一个head_ensure的变量，每个周期head_ensure做取模意义下的加法，直到等于tail或遇到第一条跳转指令，
       这个时候我们就可以保证位于head_ensure及之前位置的指令，因中间没有跳转指令，一定会执行。因而，只要当head_ensure当前位置的指令是Store、Load指令，我们就可以向slbuffer发信息，增加head2。
       简单概括即对head2之前的Store/Load指令，我们根据判断出ROB中该条指令之前没有jump指令尚未执行，从而确定该条指令会被执行。
       2）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。
       3）此外，在我们的设计中，将SLBUFFER进行内存访问时计算需要访问的地址和对应的读取/存储内存的操作在SLBUFFER中一并实现，
       也可以考虑分成两个模块，该部分的实现只需判断队首的指令是否能执行并根据指令相应执行即可。
    2. 1）SLB每个周期会查看队头，若队头指令还未ready，则阻塞。

       2）当队头ready且是load指令时，SLB会直接执行load指令，包括计算地址和读内存，
       然后把结果通知ROB，同时将队头弹出。ROB commit到这条指令时通知Regfile写寄存器。

       3）当队头ready且是store指令时，SLB会等待ROB的commit，commit之后会SLB执行这
       条store指令，包括计算地址和写内存，写完后将队头弹出。

       4）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。
    */
    if (!stall_issue_to_slb){
        slb_nxt.tail = issue_to_slb.tail, slb_nxt.size++;
        issue_to_slb_type &x = issue_to_slb;
        slb_nxt[x.id] = (Store_Load_Buffer::node){x.op, x.time, x.rob_id, x.rd, 0, 0, 0, 0, x.imm};
        if (!reg_prev[x.rs1].tag)
            slb_nxt[x.id].q1 = 0, slb_nxt[x.id].v1 = reg_prev[x.rs1].reg;
        else {
            unsigned rob_id = reg_prev[x.rs1].tag;
            if (!stall_ex_to_slb && ex_to_slb.rob_id == rob_id)
                slb_nxt[x.id].q1 = 0, slb_nxt[x.id].v1 = ex_to_slb.res;
            else if (!stall_slb_to_slb_prev && slb_to_slb_prev.rob_id == rob_id)
                slb_nxt[x.id].q1 = 0, slb_nxt[x.id].v1 = slb_to_slb_prev.res;
            else if (!rob_prev[rob_id].stall)
                slb_nxt[x.id].q1 = 0, slb_nxt[x.id].v1 = rob_prev[rob_id].res.first;
            else slb_nxt[x.id].q1 = rob_id;
        }
        if (!reg_prev[x.rs2].tag)
            slb_nxt[x.id].q2 = 0, slb_nxt[x.id].v2 = reg_prev[x.rs2].reg;
        else {
            unsigned rob_id = reg_prev[x.rs2].tag;
            if (!stall_ex_to_slb && ex_to_slb.rob_id == rob_id)
                slb_nxt[x.id].q2 = 0, slb_nxt[x.id].v2 = ex_to_slb.res;
            else if (!stall_slb_to_slb_prev && slb_to_slb_prev.rob_id == rob_id)
                slb_nxt[x.id].q2 = 0, slb_nxt[x.id].v2 = slb_to_slb_prev.res;
            else if (!rob_prev[rob_id].stall)
                slb_nxt[x.id].q2 = 0, slb_nxt[x.id].v2 = rob_prev[rob_id].res.first;
            else slb_nxt[x.id].q2 = rob_id;
        }
    }
    if (!stall_ex_to_slb){
        for (unsigned i = slb_prev.head; i != slb_prev.tail; i = i % N_slb + 1){
            if (slb_prev[i].q1 == ex_to_slb.rob_id)
                slb_nxt[i].q1 = 0, slb_nxt[i].v1 = ex_to_slb.res;
            if (slb_prev[i].q2 == ex_to_slb.rob_id)
                slb_nxt[i].q2 = 0, slb_nxt[i].v2 = ex_to_slb.res;
        }
    }
    if (!stall_slb_to_slb_prev){
        for (unsigned i = slb_prev.head; i != slb_prev.tail; i = i % N_slb + 1){
            if (slb_prev[i].q1 == slb_to_slb_prev.rob_id)
                slb_nxt[i].q1 = 0, slb_nxt[i].v1 = slb_to_slb_prev.res;
            if (slb_prev[i].q2 == slb_to_slb_prev.rob_id)
                slb_nxt[i].q2 = 0, slb_nxt[i].v2 = slb_to_slb_prev.res;
        }
    }
    stall_slb_to_rob_nxt = stall_slb_to_rs_nxt = stall_slb_to_slb_nxt = true;
    if (slb_prev.size){
        const auto &x = slb_prev.front();
        if (x.q1 == 0 && x.q2 == 0){
            if (x.time == 0){
                if (x.op <= lui){
                    auto res = execute(x.op, x.imm, x.v1, x.v2, 0);
                    stall_slb_to_rob_nxt = false;
                    slb_to_rob_nxt = (ex_to_rob_type){x.rob_id, res};
                    stall_slb_to_slb_nxt = false;
                    slb_to_slb_nxt = (ex_to_rs_or_slb_type){x.rob_id, res.first};
                    stall_slb_to_rs_nxt = false;
                    slb_to_rs_nxt = (ex_to_rs_or_slb_type){x.rob_id, res.first};
                } else {
                    stall_slb_to_rob_nxt = false;
                    slb_to_rob_nxt = (ex_to_rob_type){x.rob_id, pair<unsigned, pair<bool, unsigned>>()};
                }
                slb_nxt.head = slb_prev.head % N_slb + 1, slb_nxt.size--;
            } else slb_nxt.front().time--;
        }
    }
    if (!stall_clear_slb){
        slb_nxt.head = slb_nxt.tail = 1, slb_nxt.size = 0;
        stall_slb_to_rob_nxt = stall_slb_to_rs_nxt = true;
    }
}
void run_rob(){
    /*
    在这一部分你需要完成的工作：
    1. 实现一个先进先出的ROB，存储所有指令
    1. 根据issue阶段发射的指令信息分配空间进行存储。
    2. 根据EX阶段和SLBUFFER的计算得到的结果，遍历ROB，更新ROB中的值
    3. 对于队首的指令，如果已经完成计算及更新，进行commit
    */
    if (!stall_issue_to_rob){
        rob_nxt.tail = issue_to_rob.tail, rob_nxt.size++;
        issue_to_rob_type &x = issue_to_rob;
        rob_nxt[x.id] = (Reorder_Buffer::node){true, x.op, x.rd, x.rs_id, x.slb_id, x.inst, pair<unsigned, pair<bool, unsigned>>()};
    }
    if (!stall_ex_to_rob){
        rob_nxt[ex_to_rob.rob_id].res = ex_to_rob.res;
        rob_nxt[ex_to_rob.rob_id].stall = false;
    }
    if (!stall_slb_to_rob_prev){
        rob_nxt[slb_to_rob_prev.rob_id].res = slb_to_rob_prev.res;
        rob_nxt[slb_to_rob_prev.rob_id].stall = false;
    }
    stall_rob_to_commit = true;
    if (rob_prev.size) {
        const auto &x = rob_prev.front();
        if (!x.stall) {
            stall_rob_to_commit = false;
            rob_to_commit = (rob_to_commit_type) {x.op, x.rd, rob_prev.head, x.slb_id, x.inst, x.res};
            rob_nxt.head = rob_prev.head % N_rob + 1, rob_nxt.size--;
        }
    }
    if(!stall_clear_rob){
        rob_nxt.head = rob_nxt.tail = 1, rob_nxt.size = 0;
        stall_rob_to_commit = true;
    }
}
void run_commit(){
    /*
    在这一部分你需要完成的工作：
    1. 根据ROB发出的信息更新寄存器的值，包括对应的ROB和是否被占用状态（注意考虑issue和commit同一个寄存器的情况）
    2. 遇到跳转指令更新pc值，并发出信号清空所有部分的信息存储（这条对于很多部分都有影响，需要慎重考虑）
    */
    stall_commit_to_regfile = stall_clear_rs = stall_clear_slb = stall_clear_rob = stall_clear_fq = stall_clear_regtag = true;
    if (!stall_rob_to_commit){
        rob_to_commit_type &x = rob_to_commit;
//        cout << hex << x.inst << endl;
//        cout << dec << (unsigned)mem[99424] << ' ' << (x.rd == 15 ? x.res.first : reg_prev[15].reg) << endl;
//        if (x.inst == 0x3ce7a023)
//            cout << 1 << endl;
        if (x.op <= lhu || x.op >= lui && x.op <= jalr || x.op >= addi) {
            stall_commit_to_regfile = false;
            commit_to_regfile = (commit_to_regfile_type) {x.op, x.rd, x.res.first};
        }
        if (x.op >= jal && x.op <= bgeu){
            if (x.res.second.first){
                pc = x.res.second.second;
                stall_clear_rs = stall_clear_slb = stall_clear_rob = stall_clear_fq = stall_clear_regtag = false;
            }
        }
        if (x.op >= sb && x.op <= sw){
            stall_commit_to_regfile = false;
            unsigned pos = slb_prev[x.slb_id].v1 + slb_prev[x.slb_id].imm, v2 = slb_prev[x.slb_id].v2;
            commit_to_regfile = (commit_to_regfile_type) {x.op, pos, v2};
        }
    }
}
void update(){
    /*
    在这一部分你需要完成的工作：
    对于模拟中未完成同步的变量（即同时需记下两个周期的新/旧信息的变量）,进行数据更新。
    */
    fq_prev = fq_nxt, slb_prev = slb_nxt, rob_prev = rob_nxt;
    for (int i = 0; i < N_reg; ++i) reg_prev[i] = reg_nxt[i];
    for (int i = 0; i < N_rs; ++i) rs_prev[i] = rs_nxt[i];
    stall_slb_to_rob_prev = stall_slb_to_rob_nxt;
    stall_slb_to_rs_prev = stall_slb_to_rs_nxt;
    stall_slb_to_slb_prev = stall_slb_to_slb_nxt;
    slb_to_rob_prev = slb_to_rob_nxt;
    slb_to_rs_prev = slb_to_rs_nxt;
    slb_to_slb_prev = slb_to_slb_nxt;
}
void tomasulo(){
    pc = 0;
    for (unsigned cycle = 0; ; cycle++){
        /*在这里使用了两阶段的循环部分：
          1. 实现时序电路部分，即在每个周期初同步更新的信息。
          2. 实现逻辑电路部分，即在每个周期中如ex、issue的部分
          已在下面给出代码
        */
        run_rob();
        run_slbuffer();
        run_reservation();
        run_regfile();
        run_inst_fetch_queue();
        update();

        run_ex();
        run_issue();
        run_commit();

        if (!stall_rob_to_commit && rob_to_commit.inst == 0x0ff00513){
            cout << (reg_prev[10].reg & 255u) << endl; break;
        }
    }
};

#endif //RISC_V_SIMULATOR_TOMASULO_HPP
