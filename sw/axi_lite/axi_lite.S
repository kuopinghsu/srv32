    .section .text
    .globl _start
_start:
    li      t0, 0x180               # External RAM scratch word
    li      t1, 0x12345678
    sw      t1, 0(t0)
    li      t1, 0xa5
    sb      t1, 1(t0)
    li      t1, 0x80fe
    sh      t1, 2(t0)

    lw      t1, 0(t0)
    li      t2, 0x80fea578
    bne     t1, t2, fail
    lbu     t1, 1(t0)
    li      t2, 0xa5
    bne     t1, t2, fail
    lh      t1, 2(t0)
    li      t2, -32514              # Sign-extended 0x80fe
    bne     t1, t2, fail

    li      t3, 0x10000000          # UART TXDATA
    la      t4, message
next_byte:
    lbu     t1, 0(t4)
    beqz    t1, done
wait_uart:
    lw      t2, 4(t3)               # UART TXSTATUS
    andi    t2, t2, 1
    beqz    t2, wait_uart
    sb      t1, 0(t3)
    addi    t4, t4, 1
    j       next_byte

done:
    li      t1, 1
    sw      t1, 4(t0)
    j       .
fail:
    li      t1, -1
    sw      t1, 4(t0)
    j       .

message:
    .asciz "AXI-Lite\n"
    .balign 4
