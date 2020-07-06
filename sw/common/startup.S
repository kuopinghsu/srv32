    .section .reset, "ax"
    .global _start
_start:
    la      t0, trap_handler
    csrrw   zero, mtvec, t0

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
trap_handler:
    csrr    t5, mepc
    addi    t5, t5, 4
    csrw    mepc, t5
    mret

