#ifndef _COMPLIANCE_MODEL_H
#define _COMPLIANCE_MODEL_H

#define RVMODEL_DATA_SECTION \
        .pushsection .tohost,"aw",@progbits;                                  \
        .align 8; .global tohost; tohost: .dword 0;                           \
        .align 8; .global fromhost; fromhost: .dword 0;                       \
        .popsection;                                                          \
        .align 8; .global begin_regstate; begin_regstate:                     \
        .word 128;                                                            \
        .align 8; .global end_regstate; end_regstate:                         \
        .word 4;

//RV_COMPLIANCE_HALT
// this will dump the results via the system call
#define RVMODEL_HALT                                                          \
        la      a0, begin_signature;                                          \
        la      a1, end_signature;                                            \
        li      a7, 0xbeef0000;                                               \
        addi    a7, a7, 0x88;                                                 \
        ecall;                                                                \
        li      a7, 0xbeef0000;                                               \
        addi    a7, a7, 0x5d;                                                 \
        ecall;                                                                \
1:      j       1b;

// declare the start of your signature region here. Nothing else to be used here.
// The .align 4 ensures that the signature ends at a 16-byte boundary
#define RVMODEL_DATA_BEGIN                                                    \
        .align 4;                                                             \
        .global begin_signature;                                              \
begin_signature:

// declare the end of the signature region here. Add other target specific contents here.
#define RVMODEL_DATA_END                                                      \
        .align 4;                                                             \
        .global end_signature;                                                \
end_signature:                                                                \
        RVMODEL_DATA_SECTION

//RVMODEL_ROOT
#ifndef __riscv_compressed
#define RVMODEL_BOOT                                                          \
core_init:                                                                    \
        la      t0, __trap_handler;                                           \
        csrw    mtvec, t0;                                                    \
        j       _start;                                                       \
                                                                              \
        .balign 4;                                                            \
__trap_handler:                                                               \
        csrr    t5, mepc;                                                     \
        addi    t5, t5, 4;                                                    \
        csrw    mepc, t5;                                                     \
        mret;                                                                 \
                                                                              \
_start:                                                                       \
        la      t0, _bss_start;                                               \
        la      t1, _bss_end;                                                 \
                                                                              \
_bss_clear:                                                                   \
        sw      zero,0(t0);                                                   \
        addi    t0, t0, 4;                                                    \
        bltu    t0, t1, _bss_clear;                                           \
                                                                              \
        la      sp, _stack
#else
#define RVMODEL_BOOT                                                          \
core_init:                                                                    \
        la      t0, __trap_handler;                                           \
        csrw    mtvec, t0;                                                    \
        j       _start;                                                       \
                                                                              \
        .balign 4;                                                            \
__trap_handler:                                                               \
        la      sp, end_signature;                                            \
        addi    sp, sp, 8;                                                    \
        sw      x8, 0(sp);                                                    \
        sw      x9, 4(sp);                                                    \
        csrr    x8, mcause;                                                   \
        blt     x8, zero, __trap_handler_irq;                                 \
        csrr    x8, mepc;                                                     \
__trap_handler_exc_c_check:                                                   \
        lh      x9, 0(x8);                                                    \
        andi    x9, x9, 3;                                                    \
        addi    x8, x8, +2;                                                   \
        csrw    mepc, x8;                                                     \
        addi    x8, zero, 3;                                                  \
        bne     x8, x9, __trap_handler_irq;                                   \
__trap_handler_exc_uncrompressed:                                             \
        csrr    x8, mepc;                                                     \
        addi    x8, x8, +2;                                                   \
        csrw    mepc, x8;                                                     \
__trap_handler_irq:                                                           \
        lw      x9, 4(sp);                                                    \
        lw      x8, 0(sp);                                                    \
        addi    sp, sp, -8;                                                   \
        mret;                                                                 \
                                                                              \
_start:                                                                       \
        la      t0, _bss_start;                                               \
        la      t1, _bss_end;                                                 \
                                                                              \
_bss_clear:                                                                   \
        sw      zero,0(t0);                                                   \
        addi    t0, t0, 4;                                                    \
        bltu    t0, t1, _bss_clear;                                           \
                                                                              \
        la      sp, _stack
#endif // RV32C_ENABLED

#define RVMODEL_IO_INIT
#define RVMODEL_IO_WRITE_STR(_R, _STR)
#define RVMODEL_IO_CHECK()

#define RVMODEL_IO_ASSERT_GPR_EQ(_S, _R, _I)
#define RVMODEL_IO_ASSERT_SFPR_EQ(_F, _R, _I)
#define RVMODEL_IO_ASSERT_DFPR_EQ(_D, _R, _I)
#define RVMODEL_SET_MSW_INT
#define RVMODEL_CLEAR_MSW_INT
#define RVMODEL_CLEAR_MTIMER_INT
#define RVMODEL_CLEAR_MEXT_INT

#endif // _COMPLIANCE_MODEL_H
