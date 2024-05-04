// Copyright © 2020 Kuoping Hsu
// opcode.h
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

#ifndef __RVSIM_H__
#define __RVSIM_H__

#include <stdio.h>
#include <stdint.h>
#include "opcode.h"

#ifdef GDBSTUB
// LCOV_EXCL_START
#include "mini-gdbstub/include/gdbstub.h"
// LCOV_EXCL_STOP
#endif

#ifdef RV32E_ENABLED
#define SYS T0
#define REGNUM 16
#else
#define SYS A7
#define REGNUM 32
#endif // RV32E_ENABLED

#ifndef MEMSIZE
#define MEMSIZE (256)
#endif // MEMSIZE

#ifndef MEMBASE
#define MEMBASE (0)
#endif // MEMBASE

struct rv {
    // registers
    int32_t pc;
    int32_t prev_pc;
    int32_t regs[REGNUM];
    CSR csr;

    int32_t debug_en;
    int32_t branch_penalty;
    int32_t branch_predict;

    // memory
    int32_t mem_size;
    int32_t mem_base;
    int32_t *mem;

    #ifdef GDBSTUB
    bool bp_is_set;
    size_t bp_addr;
    bool halt;
    bool is_interrupted;
    gdbstub_t gdbstub;
    #endif

    // file handle for trace log
    FILE *ft;
};

int srv32_syscall(struct rv *rv, int func, int a0, int a1, int a2, int a3, int a4, int a5);
void srv32_tohost(struct rv *rv, int32_t ptr);
int srv32_fromhost(struct rv *rv);
void srv32_step(struct rv *rv);
int32_t srv32_read_regs(struct rv *rv, int n);
void srv32_write_regs(struct rv *rv, int n, int32_t v);
void *srv32_get_memptr(struct rv *rv, int32_t addr);
bool srv32_write_mem(struct rv *rv, int32_t addr, int32_t len, void *ptr);
bool srv32_read_mem(struct rv *rv, int32_t addr, int32_t len, void *ptr);

#endif // __RVSIM_H__

