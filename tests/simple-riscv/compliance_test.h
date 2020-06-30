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
        li      a7, 0x88;                                                     \
        ecall;                                                                \
        li      a7, 0x5d;                                                     \
        ecall;                                                                \
1:      j       1b;

#define RV_COMPLIANCE_RV32M                                                   \
        RVTEST_RV32M                                                          \

#define RV_COMPLIANCE_CODE_BEGIN                                              \
        .section .reset, "ax";                                                \
        .global _start;                                                       \
_start:                                                                       \
        addi    x2, x0, 10;                                                   \
        addi    x3, x0, 11;                                                   \
        addi    x0, x0, 0;                                                    \
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

