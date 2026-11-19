// CSR Read-Only Exception Test
//
// Exercises the concerning illegal-instruction exceptions (mcause=2) on read-only
// CSR accesses:
//
//   Testcase 1 – csrrsi with uimm≠0 on cycle  (0xC00) must trap    (write to RO)
//   Testcase 2 – csrrc  with rs1≠x0 on instret (0xC02) must trap   (write to RO)
//   Testcase 3 – csrrci with uimm=0  on time   (0xC01) must NOT trap (pure read)
//   Testcase 4 – csrrs  with rs1≠x0 on machine info CSRs (0xF11-0xF14) must trap
//
// RISC-V privileged spec rule (Zicsr):
//   A write is attempted only if rs1 ≠ x0 (CSRRS/CSRRC) or uimm ≠ 0 (CSRRSI/CSRRCI).
//   Writing a CSR whose address bits[11:10] = 0b11 (read-only) raises
//   an Illegal Instruction exception (mcause = 2).

#include <stdint.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Minimal I/O
// ---------------------------------------------------------------------------

static void puts_raw(const char *s) {
    while (*s) putchar(*s++);
}

static void print_hex(uint32_t v) {
    const char h[] = "0123456789abcdef";
    puts_raw("0x");
    for (int i = 7; i >= 0; i--)
        putchar(h[(v >> (i * 4)) & 0xf]);
}

// ---------------------------------------------------------------------------
// Trap tracking (volatile so the compiler cannot eliminate the stores)
// ---------------------------------------------------------------------------

static volatile int      g_trapped;
static volatile uint32_t g_mcause;
static volatile uint32_t g_mepc;

// Override the weak user_trap_handler from startup.S.
// The startup assembly calls user_trap_handler(void); read CSR values directly.
void user_trap_handler(void) {
    uint32_t mcause, mepc;
    asm volatile("csrr %0, mcause" : "=r"(mcause));
    asm volatile("csrr %0, mepc"   : "=r"(mepc));
    g_trapped = 1;
    g_mcause  = mcause;
    g_mepc    = mepc;
    // Advance mepc past the 4-byte faulting instruction so execution resumes
    // after the test instruction rather than retrying it forever.
    asm volatile("csrw mepc, %0" :: "r"(mepc + 4));
}

static void before_test(void) {
    g_trapped = 0;
    g_mcause  = 0;
    g_mepc    = 0;
}

// ---------------------------------------------------------------------------
// Pass/fail accounting
// ---------------------------------------------------------------------------

static int g_pass;
static int g_fail;

#define ILLOP_CAUSE 2u

// Expect an Illegal Instruction exception to have been raised.
static void check_trapped(const char *name) {
    if (g_trapped && (g_mcause == ILLOP_CAUSE)) {
        puts_raw("  PASS: ");
        puts_raw(name);
        puts_raw("\n");
        g_pass++;
    } else {
        puts_raw("  FAIL: ");
        puts_raw(name);
        puts_raw("  (trapped=");
        putchar(g_trapped ? '1' : '0');
        puts_raw(", mcause=");
        print_hex(g_mcause);
        puts_raw(")\n");
        g_fail++;
    }
}

// Expect NO exception; print the value returned by the CSR read.
static void check_no_trap(const char *name, uint32_t rd_val) {
    if (!g_trapped) {
        puts_raw("  PASS: ");
        puts_raw(name);
        puts_raw("  (rd=");
        print_hex(rd_val);
        puts_raw(")\n");
        g_pass++;
    } else {
        puts_raw("  FAIL: ");
        puts_raw(name);
        puts_raw("  (unexpected trap, mcause=");
        print_hex(g_mcause);
        puts_raw(")\n");
        g_fail++;
    }
}

// ---------------------------------------------------------------------------
// Helpers that force a specific source register via GCC register variables.
// Using a named register variable prevents GCC from substituting x0 even
// when the value happens to be 0 (relevant for testcase 4 where x15=0 but
// the instruction has rs1=x15, not rs1=x0, so it must still trap).
// ---------------------------------------------------------------------------

// csrrs rd, csr_addr, <reg>  where <reg> is forced to a named GPR != x0
#define CSRRS_WITH_RS1(rd_out, csr_hex, rs1_reg, rs1_val) do {         \
    register uint32_t _rs1 asm(rs1_reg) = (rs1_val);                   \
    asm volatile("csrrs %0, " csr_hex ", %1"                           \
                 : "=r"(rd_out) : "r"(_rs1) : );                       \
} while (0)

// csrrc rd, csr_addr, <reg>
#define CSRRC_WITH_RS1(rd_out, csr_hex, rs1_reg, rs1_val) do {         \
    register uint32_t _rs1 asm(rs1_reg) = (rs1_val);                   \
    asm volatile("csrrc %0, " csr_hex ", %1"                           \
                 : "=r"(rd_out) : "r"(_rs1) : );                       \
} while (0)

// ---------------------------------------------------------------------------
// Main test
// ---------------------------------------------------------------------------

int main(void) {
    uint32_t rd = 0;

    puts_raw("CSR Read-Only Exception Test\n");
    puts_raw("============================\n\n");

    // -----------------------------------------------------------------------
    // Group 1: User-level counter CSRs  (cycle 0xC00, time 0xC01, instret 0xC02)
    //
    // These are read-only; any write attempt must raise Illegal Instruction.
    // A write is attempted when uimm≠0 (CSRRSI/CSRRCI) or rs1≠x0 (CSRRS/CSRRC).
    // -----------------------------------------------------------------------
    puts_raw("--- Group 1: User counter CSRs (cycle/time/instret) ---\n");

    // --- cycle (0xC00) ---

    // Testcase 1: csrrsi rd, cycle, 13  – uimm=13 ≠ 0 → write attempt → MUST trap
    // Encoding from the testcase: 0xc006be73 (rd=x23, uimm=13, csr=0xC00)
    before_test();
    asm volatile(".word 0xc006be73" ::: "x23");
    check_trapped("Testcase 1: csrrsi x23, cycle, 13  (uimm=13, must trap)");

    // csrrsi with uimm=0 on cycle → pure read → must NOT trap
    before_test();
    asm volatile("csrrsi x0, 0xC00, 0");   // rd=x0: discard timing-dependent value
    check_no_trap("csrrsi rd, cycle, 0  (uimm=0, pure read, no trap)", 0);

    // csrrs rd, cycle, x0 → pure read → must NOT trap
    before_test();
    asm volatile("csrrs x0, 0xC00, x0");   // rd=x0
    check_no_trap("csrrs  rd, cycle, x0  (rs1=x0, pure read, no trap)", 0);

    // csrrs rd, cycle, t0 (x5, any non-x0 register) → write attempt → MUST trap
    before_test();
    CSRRS_WITH_RS1(rd, "0xC00", "t0", 0x1);
    check_trapped("csrrs  rd, cycle, t0  (rs1=t0≠x0, must trap)");

    // csrrc rd, cycle, x0 → pure read → must NOT trap
    before_test();
    asm volatile("csrrc x0, 0xC00, x0");   // rd=x0
    check_no_trap("csrrc  rd, cycle, x0  (rs1=x0, pure read, no trap)", 0);

    // csrrc rd, cycle, t0 → write attempt → MUST trap
    before_test();
    CSRRC_WITH_RS1(rd, "0xC00", "t0", 0x1);
    check_trapped("csrrc  rd, cycle, t0  (rs1=t0≠x0, must trap)");

    // --- time (0xC01) ---

    // Testcase 3: csrrci x0, time, 0  – uimm=0 → pure read → must NOT trap
    // Using rd=x0 to avoid timing-dependent trace values (original rd=x20 in testcase desc)
    before_test();
    asm volatile("csrrci x0, 0xC01, 0");   // rd=x0: discard timing-dependent value
    check_no_trap("Testcase 3: csrrci x20, time, 0  (uimm=0, pure read, no trap)", 0);

    // csrrsi rd, time, 1 → uimm=1 ≠ 0 → write attempt → MUST trap
    before_test();
    asm volatile("csrrsi %0, 0xC01, 1" : "=r"(rd));
    check_trapped("csrrsi rd, time, 1  (uimm=1, must trap)");

    // csrrc rd, time, t1 (x6) → write attempt → MUST trap
    before_test();
    CSRRC_WITH_RS1(rd, "0xC01", "t1", 0x1);
    check_trapped("csrrc  rd, time, t1  (rs1=t1≠x0, must trap)");

    // --- instret (0xC02) ---

    // Testcase 2: csrrc x18, instret, x19  – rs1=x19 ≠ x0 → write attempt → MUST trap
    before_test();
    CSRRC_WITH_RS1(rd, "0xC02", "s3", 0x1bc141a3u);   // s3 = x19
    check_trapped("Testcase 2: csrrc x18, instret, x19  (rs1=x19≠x0, must trap)");

    // csrrc rd, instret, x0 → pure read → must NOT trap
    before_test();
    asm volatile("csrrc x0, 0xC02, x0");   // rd=x0: discard timing-dependent value
    check_no_trap("csrrc  rd, instret, x0  (rs1=x0, pure read, no trap)", 0);

    // csrrsi rd, instret, 7 → uimm=7 ≠ 0 → write attempt → MUST trap
    before_test();
    asm volatile("csrrsi %0, 0xC02, 7" : "=r"(rd));
    check_trapped("csrrsi rd, instret, 7  (uimm=7, must trap)");

    // -----------------------------------------------------------------------
    // Group 2: Machine information CSRs (0xF11–0xF14)
    //   mvendorid (0xF11), marchid (0xF12), mimpid (0xF13), mhartid (0xF14)
    //
    // Address bits[11:10]=0b11 → read-only.  rs1≠x0 (even when the register
    // value is 0 at runtime) constitutes a write attempt and must trap.
    // -----------------------------------------------------------------------
    puts_raw("\n--- Group 2: Machine info CSRs (mvendorid/marchid/mimpid/mhartid) ---\n");

    // Testcase 4a: csrrs x20, mvendorid, x15  where x15=0 (not x0) → MUST trap
    before_test();
    CSRRS_WITH_RS1(rd, "0xF11", "a5", 0);       // a5 = x15; value 0 but register is not x0
    check_trapped("Testcase 4a: csrrs rd, mvendorid, a5 (a5=x15, value=0 but rs1≠x0, must trap)");

    // csrrs rd, mvendorid, x0  → pure read → must NOT trap
    before_test();
    asm volatile("csrrs %0, 0xF11, x0" : "=r"(rd));
    check_no_trap("csrrs  rd, mvendorid, x0  (rs1=x0, pure read, no trap)", rd);

    // csrr rd, mvendorid  (pseudo; assembles as csrrs rd, mvendorid, x0) → must NOT trap
    before_test();
    asm volatile("csrr %0, mvendorid" : "=r"(rd));
    check_no_trap("csrr   rd, mvendorid  (pure read, no trap)", rd);

    // Testcase 4b: csrrs rd, marchid, t0 (rs1=x5 ≠ x0) → MUST trap
    before_test();
    CSRRS_WITH_RS1(rd, "0xF12", "t0", 1);
    check_trapped("Testcase 4b: csrrs rd, marchid, t0  (rs1=t0≠x0, must trap)");

    // csrr rd, marchid → pure read → must NOT trap
    before_test();
    asm volatile("csrr %0, marchid" : "=r"(rd));
    check_no_trap("csrr   rd, marchid  (pure read, no trap)", rd);

    // Testcase 4c: csrrs rd, mimpid, t0 → MUST trap
    before_test();
    CSRRS_WITH_RS1(rd, "0xF13", "t0", 1);
    check_trapped("Testcase 4c: csrrs rd, mimpid, t0  (rs1=t0≠x0, must trap)");

    // csrr rd, mimpid → pure read → must NOT trap
    before_test();
    asm volatile("csrr %0, mimpid" : "=r"(rd));
    check_no_trap("csrr   rd, mimpid  (pure read, no trap)", rd);

    // Testcase 4d: csrrs rd, mhartid, t0 → MUST trap
    before_test();
    CSRRS_WITH_RS1(rd, "0xF14", "t0", 1);
    check_trapped("Testcase 4d: csrrs rd, mhartid, t0  (rs1=t0≠x0, must trap)");

    // csrr rd, mhartid → pure read → must NOT trap
    before_test();
    asm volatile("csrr %0, mhartid" : "=r"(rd));
    check_no_trap("csrr   rd, mhartid  (pure read, no trap)", rd);

    // Also verify csrrc with rs1≠x0 traps for all four machine info CSRs
    before_test();
    CSRRC_WITH_RS1(rd, "0xF11", "t0", 1);
    check_trapped("csrrc  rd, mvendorid, t0  (rs1≠x0, must trap)");

    before_test();
    CSRRC_WITH_RS1(rd, "0xF12", "t0", 1);
    check_trapped("csrrc  rd, marchid,  t0  (rs1≠x0, must trap)");

    before_test();
    CSRRC_WITH_RS1(rd, "0xF13", "t0", 1);
    check_trapped("csrrc  rd, mimpid,   t0  (rs1≠x0, must trap)");

    before_test();
    CSRRC_WITH_RS1(rd, "0xF14", "t0", 1);
    check_trapped("csrrc  rd, mhartid,  t0  (rs1≠x0, must trap)");

    // Reserved memory and SYSTEM encodings must trap without side effects.
    volatile uint32_t sentinel = 0x12345678;
    before_test();
    asm volatile(".insn s 0x23, 3, zero, 0(%0)" :: "r"(&sentinel) : "memory");
    check_trapped("reserved STORE encoding");
    if (sentinel != 0x12345678) {
        puts_raw("  FAIL: reserved STORE changed memory\n");
        g_fail++;
    }

    before_test();
    asm volatile(".insn i 0x03, 3, zero, %0, 0" :: "r"(&sentinel) : "memory");
    check_trapped("reserved LOAD encoding");

    before_test();
    asm volatile(".word 0x00200073" ::: "memory");
    uint32_t mepc, mtval, mstatus;
    asm volatile("csrr %0, mepc" : "=r"(mepc));
    asm volatile("csrr %0, mtval" : "=r"(mtval));
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    check_trapped("reserved SYSTEM encoding");
    if (mepc != g_mepc + 4 || mtval != 0x00200073 || !(mstatus & (1u << 7))) {
        puts_raw("  FAIL: MRET did not preserve trap CSRs or restore MPIE\n");
        g_fail++;
    }

    before_test();
    asm volatile(".word 0x40004033" ::: "memory");
    check_trapped("reserved XOR funct7");

    before_test();
    asm volatile(".word 0x02001013" ::: "memory");
    check_trapped("reserved SLLI immediate");

    before_test();
    asm volatile(".word 0x00001067" ::: "memory");
    check_trapped("reserved JALR funct3");

    volatile uint32_t *timer_compare = (volatile uint32_t *)0x90000008;
    *timer_compare = 0x12345678;
    ((volatile uint8_t *)timer_compare)[1] = 0xa5;
    if (*timer_compare != 0x1234a578 ||
        ((volatile uint8_t *)timer_compare)[1] != 0xa5 ||
        *(volatile uint32_t *)0x90000014 != 0) {
        puts_raw("  FAIL: CLINT byte lanes or unmapped read\n");
        g_fail++;
    } else {
        puts_raw("  PASS: CLINT byte lanes and unmapped read\n");
        g_pass++;
    }

    // -----------------------------------------------------------------------
    // Summary
    // -----------------------------------------------------------------------
    puts_raw("\n============================\n");
    puts_raw("Results: ");
    {
        char buf[12];
        int n = g_pass, i = 0;
        if (n == 0) { buf[i++] = '0'; }
        else { char tmp[12]; int j=0; while(n>0){tmp[j++]='0'+(n%10);n/=10;} while(j>0)buf[i++]=tmp[--j]; }
        buf[i] = '\0';
        puts_raw(buf);
    }
    puts_raw(" passed, ");
    {
        char buf[12];
        int n = g_fail, i = 0;
        if (n == 0) { buf[i++] = '0'; }
        else { char tmp[12]; int j=0; while(n>0){tmp[j++]='0'+(n%10);n/=10;} while(j>0)buf[i++]=tmp[--j]; }
        buf[i] = '\0';
        puts_raw(buf);
    }
    puts_raw(" failed\n");
    if (g_fail == 0)
        puts_raw("ALL TESTS PASSED\n");
    else
        puts_raw("SOME TESTS FAILED\n");

    return g_fail ? 1 : 0;
}
