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
#include "rvsim.h"
#include "riscv-disas.h"

// do not check coverage here
// LCOV_EXCL_START
extern const char *regname[32];

static void debug_usage(void) {
    printf(
"Interactive command\n"
"csrs                       # dunp csr registers\n"
"mem <addr> <len>           # dump memory\n"
"help|h                     # help\n"
"list [count]               # list disassembly code (defualt 16)\n"
"pc                         # show pc\n"
"quit|q                     # quit\n"
"regs                       # dump registers\n"
"step [count]               # run\n"
"until <val>                # run until pc htis <val>\n"
"\n"
    );
}

static void dump_mem(struct rv *rv, int32_t addr, int32_t len) {
    int start = (int)(addr / 16)*16;
    int total = len + (addr % 16);
    char* ptr = srv32_get_memptr(rv, start);
    int i;

    for(i = 0; i < total; i++) {
        if (i % 16 == 0)
            printf("%08x ", start+i);

        if (start + i >= addr)
            printf("%02x", (ptr[i]&0xff));
        else
            printf("  ");

        printf("%s", (i % 16 == 15) ? "\n" : " ");
    }

    if (total % 16 != 0)
        printf("\n");
}

static int show_pc(struct rv *rv, int pc) {
    char buf[80] = {0};
    rv_inst inst;

    srv32_read_mem(rv, pc, sizeof(rv_inst), &inst);

    disasm_inst(buf, sizeof(buf), rv32, pc, inst);
    printf("%7s: %08x %s\n", "pc", pc, buf);

    return (int)inst_length(inst);
}

static void dump_regs(struct rv *rv) {
    int i;

    show_pc(rv, rv->pc);

    printf("\n");

    for(i = 0; i < REGNUM; i++) {
        printf("%7s: %08x", regname[i], srv32_read_regs(rv, i));
        printf("%s", (i % 4 == 3) ? "\n" : "    ");
    }
}

static void dump_csrs(struct rv *rv) {
    printf("time     : %08x_%08x\n", rv->csr.time.d.hi, rv->csr.time.d.lo);
    printf("cycle    : %08x_%08x\n", rv->csr.cycle.d.hi, rv->csr.cycle.d.lo);
    printf("instret  : %08x_%08x\n", rv->csr.instret.d.hi, rv->csr.instret.d.lo);
    printf("mtime    : %08x_%08x\n", rv->csr.mtime.d.hi, rv->csr.mtime.d.lo);
    printf("mtimecmp : %08x_%08x\n", rv->csr.mtimecmp.d.hi, rv->csr.mtimecmp.d.lo);
    printf("mvendorid: %08x\n", rv->csr.mvendorid);
    printf("marchid  : %08x\n", rv->csr.marchid);
    printf("mimpid   : %08x\n", rv->csr.mimpid);
    printf("mhartid  : %08x\n", rv->csr.mhartid);
    printf("mscratch : %08x\n", rv->csr.mscratch);
    printf("mstatus  : %08x\n", rv->csr.mstatus);
    printf("mstatush : %08x\n", rv->csr.mstatush);
    printf("misa     : %08x\n", rv->csr.misa);
    printf("mie      : %08x\n", rv->csr.mie);
    printf("mtvec    : %08x\n", rv->csr.mtvec);
    printf("mepc     : %08x\n", rv->csr.mepc);
    printf("mcause   : %08x\n", rv->csr.mcause);
    printf("mip      : %08x\n", rv->csr.mip);
    printf("mtval    : %08x\n", rv->csr.mtval);
    printf("msip     : %08x\n", rv->csr.msip);
    printf("\n");
}

static int isspace_ascii(int c)
{
    return c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r' || c == ' ';
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

void debug(struct rv *rv) {
    static int running = 0;
    static char cmd[1024];
    static char cmd_last[1024];
    static int until_pc = 0;
    static int count = 0;
    static int count_en = 0;

    if (running) {
        if (until_pc == rv->pc) {
            running = 0;
            until_pc = 0;
        }

        if (count_en) {
            if (count <= 1) {
                running = 0;
                count_en = 0;
            } else {
                show_pc(rv, rv->pc);
                count--;
            }
        }
    }

    if (!running) {
        dump_regs(rv);
        do {
            printf("(rvsim) ");

            if (!fgets(cmd, sizeof(cmd), stdin))
                continue;

            trim(cmd, sizeof(cmd));

            if (cmd[0] == 0) {
                strncpy(cmd, cmd_last, sizeof(cmd));
            } else {
                strncpy(cmd_last, cmd, sizeof(cmd));
            }

            if (!strncmp(cmd, "help", sizeof(cmd)) || !strncmp(cmd, "h", sizeof(cmd))) {
                debug_usage();
                continue;
            }

            if (!strncmp(cmd, "quit", sizeof(cmd)) || !strncmp(cmd, "q", sizeof(cmd))) {
                exit(0);
            }

            if (!strncmp(cmd, "until", sizeof("until")-1)) {
                until_pc = 0;
                count_en = 0;
                sscanf(cmd, "until %i", &until_pc);
                running = 1;
                break;
            }

            if (!strncmp(cmd, "regs", sizeof(cmd))) {
                dump_regs(rv);
                continue;
            }

            if (!strncmp(cmd, "mem", sizeof("mem")-1)) {
                int addr=0, len=0;
                sscanf(cmd, "mem %i %i", &addr, &len);

                if (!len) {
                    printf("memory size should be > 0\n");
                    continue;
                }

                dump_mem(rv, addr, len);
                continue;
            }

            if (!strncmp(cmd, "csrs", sizeof(cmd))) {
                dump_csrs(rv);
                continue;
            }

            if (!strncmp(cmd, "pc", sizeof(cmd))) {
                printf("pc %08x\n", rv->pc);
                continue;
            }

            if (!strncmp(cmd, "step", sizeof("step")-1)) {
                count_en = 1;
                count = 1;
                sscanf(cmd, "step %i", &count);

                if (count > 1)
                    running = 1;

                break;
            }

            if (!strncmp(cmd, "list", sizeof("list")-1)) {
                int n = 16;
                int i, addr;

                sscanf(cmd, "list %i", &n);

                for (i = 0, addr = rv->pc; i < n; i++) {
                    int inst_len = show_pc(rv, addr);
                    addr += inst_len;
                }

                continue;
            }

            printf("Unknow command %s\n", cmd);

        } while(1);
    }
}
// LCOV_EXCL_STOP

