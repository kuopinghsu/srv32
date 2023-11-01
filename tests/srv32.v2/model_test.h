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

//RV_COMPLIANCE_HALT
#define RVMODEL_HALT                                                    \
        la      a1, htif_mem;                                           \
        la      a2, begin_signature;                                    \
        la      a3, end_signature;                                      \
        li      a4, 0x90000000;                                         \
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

// declare the start of your signature region here. Nothing else to be used here.
// The .align 4 ensures that the signature ends at a 16-byte boundary
#define RVMODEL_DATA_BEGIN                                              \
        .align 4;                                                       \
        .global begin_signature;                                        \
begin_signature:

// declare the end of the signature region here. Add other target specific contents here.
#define RVMODEL_DATA_END                                                \
        .align 4;                                                       \
        .global end_signature;                                          \
end_signature:                                                          \
        RVMODEL_DATA_SECTION

//RVMODEL_ROOT
#define RVMODEL_BOOT

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

#define CANARY

#endif // _COMPLIANCE_MODEL_H
