    .section .reset, "ax"
    .global _start
_start:
    la      t0, trap_handler
    csrw    mtvec, t0
    csrrsi  zero, mtvec, 1      // set vector based trap

    la      t0, _bss_start
    la      t1, _bss_end

_bss_clear:
    sw      zero,0(t0)
    addi    t0, t0, 4
    bltu    t0, t1, _bss_clear

    la      sp, _stack
    call    main
    tail    exit

    .section .text.trap, "ax"
    .align 4
trap_handler:
    jal     x0, _trap_handler       // 0: Instruction address misaligned
    jal     x0, _trap_handler       // 1: Instruction access fault
    jal     x0, _trap_handler       // 2: Illegal instruction
    jal     x0, _trap_ebreak        // 3: Breakpint
    jal     x0, _trap_handler       // 4: Load address misaligned
    jal     x0, _trap_handler       // 5: Load access fault
    jal     x0, _trap_handler       // 6: Store/AMO address misaligned
    jal     x0, _trap_handler       // 7: Store/AMO access fault
1:  jal     x0, 1b                  // 8:
1:  jal     x0, 1b                  // 9:
1:  jal     x0, 1b                  // 10:
    jal     x0, _trap_ecall         // 11: Environment call from M-mode
1:  jal     x0, 1b                  // 12:
1:  jal     x0, 1b                  // 13:
1:  jal     x0, 1b                  // 14:
1:  jal     x0, 1b                  // 15:

    .weak   user_trap_handler

// ============================================================
_trap_handler:
    // store registers
    addi    sp, sp, -80
    sw      ra, 76(sp)
    sw      t0, 72(sp)
    sw      t1, 68(sp)
    sw      t2, 64(sp)
    sw      s0, 60(sp)
    sw      a0, 56(sp)
    sw      a1, 52(sp)
    sw      a2, 48(sp)
    sw      a3, 44(sp)
    sw      a4, 40(sp)
    sw      a5, 36(sp)
    sw      a6, 32(sp)
    sw      a7, 28(sp)
    sw      t3, 24(sp)
    sw      t4, 20(sp)
    sw      t5, 16(sp)
    sw      t6, 12(sp)
    addi    s0, sp, 80

    // check user defined handler
    lui     a5, %hi(user_trap_handler)
    addi    a5, a5, %lo(user_trap_handler)
    beq     a5, zero, 1f
    call    user_trap_handler
    j       2f

1:
    // default handler
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5

2:
    // restore registers
    lw      ra, 76(sp)
    lw      t0, 72(sp)
    lw      t1, 68(sp)
    lw      t2, 64(sp)
    lw      s0, 60(sp)
    lw      a0, 56(sp)
    lw      a1, 52(sp)
    lw      a2, 48(sp)
    lw      a3, 44(sp)
    lw      a4, 40(sp)
    lw      a5, 36(sp)
    lw      a6, 32(sp)
    lw      a7, 28(sp)
    lw      t3, 24(sp)
    lw      t4, 20(sp)
    lw      t5, 16(sp)
    lw      t6, 12(sp)
    addi    sp, sp, 80

    mret

// ============================================================
_trap_ecall:
    // store registers
    addi    sp, sp, -80
    sw      ra, 76(sp)
    sw      t0, 72(sp)
    sw      t1, 68(sp)
    sw      t2, 64(sp)
    sw      s0, 60(sp)
    sw      a0, 56(sp)
    sw      a1, 52(sp)
    sw      a2, 48(sp)
    sw      a3, 44(sp)
    sw      a4, 40(sp)
    sw      a5, 36(sp)
    sw      a6, 32(sp)
    sw      a7, 28(sp)
    sw      t3, 24(sp)
    sw      t4, 20(sp)
    sw      t5, 16(sp)
    sw      t6, 12(sp)
    addi    s0, sp, 80

    // check user defined handler
    lui     a5, %hi(user_trap_handler)
    addi    a5, a5, %lo(user_trap_handler)
    beq     a5, zero, 1f
    call    user_trap_handler
    j       2f

1:
    // default handler
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5

2:
    // restore registers
    lw      ra, 76(sp)
    lw      t0, 72(sp)
    lw      t1, 68(sp)
    lw      t2, 64(sp)
    lw      s0, 60(sp)
    lw      a0, 56(sp)
    lw      a1, 52(sp)
    lw      a2, 48(sp)
    lw      a3, 44(sp)
    lw      a4, 40(sp)
    lw      a5, 36(sp)
    lw      a6, 32(sp)
    lw      a7, 28(sp)
    lw      t3, 24(sp)
    lw      t4, 20(sp)
    lw      t5, 16(sp)
    lw      t6, 12(sp)
    addi    sp, sp, 80

    mret

// ============================================================
_trap_ebreak:
    // store registers
    addi    sp, sp, -80
    sw      ra, 76(sp)
    sw      t0, 72(sp)
    sw      t1, 68(sp)
    sw      t2, 64(sp)
    sw      s0, 60(sp)
    sw      a0, 56(sp)
    sw      a1, 52(sp)
    sw      a2, 48(sp)
    sw      a3, 44(sp)
    sw      a4, 40(sp)
    sw      a5, 36(sp)
    sw      a6, 32(sp)
    sw      a7, 28(sp)
    sw      t3, 24(sp)
    sw      t4, 20(sp)
    sw      t5, 16(sp)
    sw      t6, 12(sp)
    addi    s0, sp, 80

    // check user defined handler
    lui     a5, %hi(user_trap_handler)
    addi    a5, a5, %lo(user_trap_handler)
    beq     a5, zero, 1f
    call    user_trap_handler
    j       2f

1:
    // default handler
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5

2:
    // restore registers
    lw      ra, 76(sp)
    lw      t0, 72(sp)
    lw      t1, 68(sp)
    lw      t2, 64(sp)
    lw      s0, 60(sp)
    lw      a0, 56(sp)
    lw      a1, 52(sp)
    lw      a2, 48(sp)
    lw      a3, 44(sp)
    lw      a4, 40(sp)
    lw      a5, 36(sp)
    lw      a6, 32(sp)
    lw      a7, 28(sp)
    lw      t3, 24(sp)
    lw      t4, 20(sp)
    lw      t5, 16(sp)
    lw      t6, 12(sp)
    addi    sp, sp, 80

    mret

