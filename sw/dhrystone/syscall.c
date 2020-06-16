
extern long time();
extern long insn();

long time()
{
    int cycles;
    asm volatile ("rdcycle %0" : "=r"(cycles));
    return cycles;
}

long insn()
{
    int insns;
    asm volatile ("rdinstret %0" : "=r"(insns));
    return insns;
}

