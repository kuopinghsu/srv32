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
    j       _trap_handler       // 0: Instruction address misaligned
    j       _trap_handler       // 1: Instruction access fault
    j       _trap_handler       // 2: Illegal instruction
    j       _trap_ebreak        // 3: Breakpint
    j       _trap_handler       // 4: Load address misaligned
    j       _trap_handler       // 5: Load access fault
    j       _trap_handler       // 6: Store/AMO address misaligned
    j       _trap_handler       // 7: Store/AMO access fault
1:  j       1b                  // 8:
1:  j       1b                  // 9:
1:  j       1b                  // 10:
    j       _trap_ecall         // 11: Environment call from M-mode
1:  j       1b                  // 12:
1:  j       1b                  // 13:
1:  j       1b                  // 14:
1:  j       1b                  // 15:

_trap_handler:
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5
    mret

_trap_ecall:
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5
    mret

_trap_ebreak:
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5
    mret

