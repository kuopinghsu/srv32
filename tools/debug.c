// Copyright © 2020 Kuoping Hsu
// debug.c: debug interactive mode for ISS
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdint.h>

#include "opcode.h"

extern CSR csr;
extern int32_t pc;
extern int32_t prev_pc;

#ifdef RV32E_ENABLED
extern int32_t regs[16];
#else
extern int32_t regs[32];
#endif // RV32E_ENABLED

extern int *mem;
extern int *imem;
extern int *dmem;
extern char *regname[32];

static void debug_usage(void) {
    printf(
"Interactive command\n"
"regs                       # dump registers\n"
"csrs                       # dunp csr registers\n"
"until <val>                # run until pc htis <val>\n"
"help|h                     # help\n"
"quit|q                     # quit\n"
"pc                         # show pc\n"
"step [count]               # run\n"
"\n"
    );
}

static void dump_regs(void) {
    int i;

    printf("%7s: %08x\n", "pc", pc);

#ifdef RV32E_ENABLED
    for(i = 0; i < 16; i++) {
#else
    for(i = 0; i < 32; i++) {
#endif
        printf("%7s: %08x", regname[i], regs[i]);
        printf("%s", (i % 4 == 3) ? "\n" : "    ");
    }
}

static void dump_csrs(void) {
    printf("time     : %08x_%08x\n", csr.time.d.hi, csr.time.d.lo);
    printf("cycle    : %08x_%08x\n", csr.cycle.d.hi, csr.cycle.d.lo);
    printf("instret  : %08x_%08x\n", csr.instret.d.hi, csr.instret.d.lo);
    printf("mtime    : %08x_%08x\n", csr.mtime.d.hi, csr.mtime.d.lo);
    printf("mtimecmp : %08x_%08x\n", csr.mtimecmp.d.hi, csr.mtimecmp.d.lo);
    printf("mvendorid: %08x\n", csr.mvendorid);
    printf("marchid  : %08x\n", csr.marchid);
    printf("mimpid   : %08x\n", csr.mimpid);
    printf("mhartid  : %08x\n", csr.mhartid);
    printf("mscratch : %08x\n", csr.mscratch);
    printf("mstatus  : %08x\n", csr.mstatus);
    printf("mstatush : %08x\n", csr.mstatush);
    printf("misa     : %08x\n", csr.misa);
    printf("mie      : %08x\n", csr.mie);
    printf("mtvec    : %08x\n", csr.mtvec);
    printf("mepc     : %08x\n", csr.mepc);
    printf("mcause   : %08x\n", csr.mcause);
    printf("mip      : %08x\n", csr.mip);
    printf("mtval    : %08x\n", csr.mtval);
    printf("msip     : %08x\n", csr.msip);
    printf("\n");
}

static int isspace_ascii(int c)
{
  return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

static char * trim(char * s, int size) {
    int i;
    int l = strnlen(s, size);
    char *str = s;

    while(isspace_ascii((int)s[l - 1])) --l;
    while(*s && isspace_ascii((int)s[0])) { ++s, --l; }
    for(i = 0; i < l; i++)
        str[i] = s[i];
    str[i] = 0;
    return str;
}

void debug(void) {
    static int running = 0;
    static char cmd[1024];
    static char cmd_last[1024];
    static int until_pc = 0;
    static int count = 0;
    static int count_en = 0;

    if (!running) {
        dump_regs();
        printf("(rvsim) ");
        fgets(cmd, sizeof(cmd), stdin);
        trim(cmd, sizeof(cmd));
        if (cmd[0] == 0) {
            strncpy(cmd, cmd_last, sizeof(cmd));
        } else {
            strncpy(cmd_last, cmd, sizeof(cmd));
        }
        if (!strncmp(cmd, "help", sizeof(cmd)) || !strncmp(cmd, "h", sizeof(cmd))) {
            debug_usage();
            return;
        }
        if (!strncmp(cmd, "quit", sizeof(cmd)) || !strncmp(cmd, "q", sizeof(cmd))) {
            exit(0);
        }
        if (!strncmp(cmd, "until", sizeof("until")-1)) {
            until_pc = 0;
            count_en = 0;
            sscanf(cmd, "until %i", &until_pc);
            running = 1;
            return;
        }
        if (!strncmp(cmd, "regs", sizeof(cmd))) {
            dump_regs();
            return;
        }
        if (!strncmp(cmd, "csrs", sizeof(cmd))) {
            dump_csrs();
            return;
        }
        if (!strncmp(cmd, "pc", sizeof(cmd))) {
            printf("pc %08x\n", pc);
            return;
        }
        if (!strncmp(cmd, "step", sizeof("step")-1)) {
            count_en = 1;
            count = 1;
            sscanf(cmd, "step %i", &count);
            if (count > 1)
                running = 1;
            return;
        }
        if (cmd[0] != 0)
            printf("Unknow command %s\n", cmd);
    } else {
        if (until_pc == prev_pc) {
            running = 0;
            until_pc = 0;
            return;
        }

        if (count_en) {
            if (count <= 2) {
                running = 0;
                count_en = 0;
            } else {
                count--;
            }
            return;
        }
    }
}

