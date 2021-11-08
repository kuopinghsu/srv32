// RISC-V Compliance Test Header File
// Copyright (c) 2017, Codasip Ltd. All Rights Reserved.
// See LICENSE for license details.
//
// Description: Common header file for RV32I tests

#ifndef _COMPLIANCE_TEST_H
#define _COMPLIANCE_TEST_H

#include "riscv_test.h"

//-----------------------------------------------------------------------
// RV Compliance Macros
//-----------------------------------------------------------------------

#define RV_COMPLIANCE_HALT                                                    \
        la      a0, begin_signature;                                          \
        la      a1, end_signature;                                            \
        li      a7, 0xbeef0000;                                               \
        addi    a7 , a7, 0x88;                                                \
        ecall;                                                                \
        li      a7, 0xbeef0000;                                               \
        addi    a7 , a7, 0x5d;                                                \
        ecall;                                                                \
1:      j       1b;

#define RV_COMPLIANCE_RV32M                                                   \
        RVTEST_RV32M                                                          \

#define RV_COMPLIANCE_CODE_BEGIN                                              \
        .section .text.trap, "ax";                                            \
__trap_handler:                                                               \
        csrr    t5, mepc;                                                     \
        addi    t5, t5, 4;                                                    \
        csrw    mepc, t5;                                                     \
        mret;                                                                 \
                                                                              \
        .section .reset, "ax";                                                \
        .global _start;                                                       \
_start:                                                                       \
        la      t0, __trap_handler;                                           \
        csrw    mtvec, t0;                                                    \
                                                                              \
        la      t0, _bss_start;                                               \
        la      t1, _bss_end;                                                 \
                                                                              \
_bss_clear:                                                                   \
        sw      zero,0(t0);                                                   \
        addi    t0, t0, 4;                                                    \
        bltu    t0, t1, _bss_clear;                                           \
                                                                              \
        la      sp, _stack;                                                   \
begin_testcode:

#define RV_COMPLIANCE_CODE_END                                                \
end_testcode:

#define RV_COMPLIANCE_DATA_BEGIN                                              \
        RVTEST_DATA_BEGIN                                                     \

#define RV_COMPLIANCE_DATA_END                                                \
        RVTEST_DATA_END                                                       \

#endif

