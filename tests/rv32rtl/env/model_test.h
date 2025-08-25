#ifndef _COMPLIANCE_MODEL_H
#define _COMPLIANCE_MODEL_H

#define RVMODEL_DATA_SECTION \
        .pushsection .tohost,"aw",@progbits;                            \
        .align 8; .global tohost; tohost: .dword 0;                     \
        .align 8; .global fromhost; fromhost: .dword 0;                 \
        .popsection;                                                    \
        .align 8; .global begin_regstate; begin_regstate:               \
        .word 128;                                                      \
        .align 8; .global end_regstate; end_regstate:                   \
        .word 4;                                                        \
        .align 4; .global htif_mem; htif_mem:                           \
        .word 8;

#define RVMODEL_HALT                                                    \
        la      a1, htif_mem;                                           \
        la      a2, begin_signature;                                    \
        la      a3, end_signature;                                      \
        li      a4, 0xa0000000;                                         \
        addi    a4, a4, 0x30;                                           \
__dump:                                                                 \
        li      a0, 0x88;                                               \
        sw      a0, 0(a1);                                              \
        sw      a2, 4(a1);                                              \
        sw      a3, 8(a1);                                              \
        sw      a1, 0(a4);                                              \
__exit:                                                                 \
        li      a0, 0x5d;                                               \
        sw      a0, 0(a1);                                              \
        sw      zero, 4(a1);                                            \
        sw      a1, 0(a4);                                              \
1:      j       1b;

// see https://github.com/riscv-non-isa/riscv-arch-test/issues/659
// get rid of compressed instructions
#define RVMODEL_BOOT                                                    \
        .option norelax;

#define RVMODEL_DATA_BEGIN                                              \
        RVMODEL_DATA_SECTION                                            \
        .align 4;                                                       \
        .global begin_signature;                                        \
begin_signature:

#define RVMODEL_DATA_END                                                \
        .align 4;                                                       \
        .global end_signature;                                          \
end_signature:

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
