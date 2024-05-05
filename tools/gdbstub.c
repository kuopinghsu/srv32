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

#ifdef GDBSTUB

// do not check coverage here
// LCOV_EXCL_START
#include <errno.h>

#include "mini-gdbstub/include/gdbstub.h"
#include "rvsim.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

static inline bool srv_is_halted(struct rv *rv) {
    return __atomic_load_n(&rv->halt, __ATOMIC_RELAXED);
}

static inline bool srv_is_interrupt(struct rv *rv) {
    return __atomic_load_n(&rv->is_interrupted, __ATOMIC_RELAXED);
}

static gdb_action_t srv_cont(void *args) {
    struct rv *rv = (struct rv *) args;

    for (; !srv_is_halted(rv) && !srv_is_interrupt(rv);) {
        if (find(rv->root, rv->pc) != NULL)
            break;

        srv32_step(rv);
    }

    /* Clear the interrupt if it's pending */
    __atomic_store_n(&rv->is_interrupted, false, __ATOMIC_RELAXED);

    return ACT_RESUME;
}

static gdb_action_t srv_stepi(void *args) {
    struct rv *rv = (struct rv *) args;

    if (VERBOSE) fprintf(stderr, "stepi\n");

    srv32_step(rv);

    return ACT_RESUME;
}

static int srv_read_reg(void *args, int regno, size_t *value) {
    struct rv *rv = (struct rv *) args;

    if (regno > REGNUM)
        return EFAULT;

    if (regno == REGNUM)
        *value = rv->pc;
    else
        *value = srv32_read_regs(rv, regno);

    return 0;
}

static int srv_write_reg(void *args, int regno, size_t value) {
    struct rv *rv = (struct rv *) args;

    if (regno > REGNUM)
        return EFAULT;

    if (regno == REGNUM)
        rv->pc = value;
    else
        srv32_write_regs(rv, regno, value);

    return 0;
}

static int srv_read_mem(void *args, size_t addr, size_t len, void *val) {
    struct rv *rv = (struct rv *) args;

    if (VERBOSE) fprintf(stderr, "read_mem(0x%08x, %d)\n", (uint32_t)addr, (int32_t)len);

    srv32_read_mem(rv, addr, len, val);

    return 0;
}

static int srv_write_mem(void *args, size_t addr, size_t len, void *val) {
    struct rv *rv = (struct rv *) args;

    if (VERBOSE) fprintf(stderr, "write_mem(0x%08x, %d)\n", (uint32_t)addr, (int32_t)len);

    srv32_write_mem(rv, addr, len, val);

    return 0;
}

static bool srv_set_bp(void *args, size_t addr, bp_type_t type) {
    struct rv *rv = (struct rv *) args;

    if (VERBOSE) fprintf(stderr, "set_bp 0x%08x\n", (uint32_t)addr);

    if (type != BP_SOFTWARE)
        return true;

    insert(&rv->root, addr);

    return true;
}

static bool srv_del_bp(void *args, size_t addr, bp_type_t type) {
    struct rv *rv = (struct rv *) args;

    if (VERBOSE) fprintf(stderr, "del_bp 0x%08x\n", (uint32_t)addr);

    // It's fine when there's no matching breakpoint, just doing nothing
    if (type != BP_SOFTWARE || find(rv->root, addr) == NULL)
        return true;

    rv->root = delete(rv->root, addr);

    return true;
}

static void srv_on_interrupt(void *args) {
    struct rv *rv = (struct rv*)args;

    /* Notify the emulator to break out the for loop in srv_cont */
    __atomic_store_n(&rv->is_interrupted, true, __ATOMIC_RELAXED);
}

const struct target_ops gdbstub_ops = {
    .read_reg = srv_read_reg,
    .write_reg = srv_write_reg,
    .read_mem = srv_read_mem,
    .write_mem = srv_write_mem,
    .cont = srv_cont,
    .stepi = srv_stepi,
    .set_bp = srv_set_bp,
    .del_bp = srv_del_bp,
    .on_interrupt = srv_on_interrupt,
};

// LCOV_EXCL_STOP
#endif // GDBSTUB

