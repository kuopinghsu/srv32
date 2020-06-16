    .section .reset, "ax"
    .global _start
_start:
    addi    x2, x0, 10
    addi    x3, x0, 11
    addi    x0, x0, 0

    la      t0, _bss_start
    la      t1, _bss_end

_bss_clear:
    sw      zero,0(t0)
    addi    t0, t0, 4
    bltu    t0, t1, _bss_clear

    la      sp, _stack
    call    main
    tail    exit

