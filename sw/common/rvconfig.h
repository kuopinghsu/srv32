#ifndef __RVCONFIG_H
#define __RVCONFIG_H

#define MTIME_BASE      0x90000000
#define MTIMECMP_BASE   0x90000008
#define MSIP_BASE       0x90000010

#define MSIP_SWIRQ      0
#define MSIP_EXIRQ      16

#define _CSRR_MEPC()        ({ int result; __asm volatile("csrr %0, mepc" : "=r"(result)); result; })
#define _CSRW_MEPC(v)       __asm volatile("csrw mepc, %0" : : "r"(v))
#define _CSRR_MCAUSE()      ({ int result; __asm volatile("csrr %0, mcause" : "=r"(result)); result; })
#define _CSRW_MCAUSE(v)     __asm volatile("csrw mcause, %0" : : "r"(v))
#define _CSRR_MSTATUS()     ({ int result; __asm volatile("csrr %0, mstatus" : "=r"(result)); result; })
#define _CSRW_MSTATUS(v)    __asm volatile("csrw mstatus, %0" : : "r"(v))
#define _CSRR_MTVAL()       ({ int result; __asm volatile("csrr %0, mtval" : "=r"(result)); result; })
#define _CSRW_MTVAL(v)      __asm volatile("csrw mtval, %0" : : "r"(v))
#define _CSRR_MIE()         ({ int result; __asm volatile("csrr %0, mie" : "=r"(result)); result; })
#define _CSRW_MIE(v)        __asm volatile("csrw mie, %0" : : "r"(v))
#define _CSRR_MIP()         ({ int result; __asm volatile("csrr %0, mip" : "=r"(result)); result; })
#define _CSRW_MIP(v)        __asm volatile("csrw mip, %0" : : "r"(v))

#define _CSRR_RDCYCLE()     ({ int result; __asm volatile("csrr %0, cycle" : "=r"(result)); result; })
#define _CSRW_RDCYCLE(v)    __asm volatile("csrw cycle, %0" : : "r"(v))
#define _CSRR_RDCYCLEH()    ({ int result; __asm volatile("csrr %0, cycleh" : "=r"(result)); result; })
#define _CSRW_RDCYCLEH(v)   __asm volatile("csrw cycleh, %0" : : "r"(v))
#define _CSRR_RDINSTRET()   ({ int result; __asm volatile("csrr %0, instret" : "=r"(result)); result; })
#define _CSRW_RDINSTRET(v)  __asm volatile("csrw instret, %0" : : "r"(v))
#define _CSRR_RDINSTRETH()  ({ int result; __asm volatile("csrr %0, instreth" : "=r"(result)); result; })
#define _CSRW_RDINSTRETH(v) __asm volatile("csrw instreth, %0" : : "r"(v))
#define _CSRR_MVENDORID()   ({ int result; __asm volatile("csrr %0, mvendorid" : "=r"(result)); result; })
#define _CSRW_MVENDORID(v)  __asm volatile("csrw mvendorid, %0" : : "r"(v))
#define _CSRR_MARCHID()     ({ int result; __asm volatile("csrr %0, marchid" : "=r"(result)); result; })
#define _CSRW_MARCHID(v)    __asm volatile("csrw marchid, %0" : : "r"(v))
#define _CSRR_MIMPID()      ({ int result; __asm volatile("csrr %0, mimpid" : "=r"(result)); result; })
#define _CSRW_MIMPID(v)     __asm volatile("csrw mimpid, %0" : : "r"(v))
#define _CSRR_MHARTID()     ({ int result; __asm volatile("csrr %0, mhartid" : "=r"(result)); result; })
#define _CSRW_MHARTID(v)    __asm volatile("csrw mhartid, %0" : : "r"(v))
#define _CSRR_MISA()        ({ int result; __asm volatile("csrr %0, misa" : "=r"(result)); result; })
#define _CSRW_MISA(v)       __asm volatile("csrw misa, %0" : : "r"(v))

// no support CSR
#define _CSRR_DPC()         ({ int result; __asm volatile("csrr %0, dpc" : "=r"(result)); result; })
#define _CSRW_DPC(v)        __asm volatile("csrw dpc, %0" : : "r"(v))

static inline int  CSRR_MEPC(void)        { return _CSRR_MEPC(); }
static inline void CSRW_MEPC(int v)       { _CSRW_MEPC(v); }
static inline int  CSRR_MCAUSE(void)      { return _CSRR_MCAUSE(); }
static inline void CSRW_MCAUSE(int v)     { _CSRW_MCAUSE(v); }
static inline int  CSRR_MSTATUS(void)     { return _CSRR_MSTATUS(); }
static inline void CSRW_MSTATUS(int v)    { _CSRW_MSTATUS(v); }
static inline int  CSRR_MTVAL(void)       { return _CSRR_MTVAL(); }
static inline void CSRW_MTVAL(int v)      { _CSRW_MTVAL(v); }
static inline int  CSRR_MIE(void)         { return _CSRR_MIE(); }
static inline void CSRW_MIE(int v)        { _CSRW_MIE(v); }
static inline int  CSRR_MIP(void)         { return _CSRR_MIP(); }
static inline void CSRW_MIP(int v)        { _CSRW_MIP(v); }
static inline int  CSRR_RDCYCLE(void)     { return _CSRR_RDCYCLE(); }
static inline void CSRW_RDCYCLE(int v)    { _CSRW_RDCYCLE(v); }
static inline int  CSRR_RDCYCLEH(void)    { return _CSRR_RDCYCLEH(); }
static inline void CSRW_RDCYCLEH(int v)   { _CSRW_RDCYCLEH(v); }
static inline int  CSRR_RDINSTRET(void)   { return _CSRR_RDINSTRET(); }
static inline void CSRW_RDINSTRET(int v)  { _CSRW_RDINSTRET(v); }
static inline int  CSRR_RDINSTRETH(void)  { return _CSRR_RDINSTRETH(); }
static inline void CSRW_RDINSTRETH(int v) { _CSRW_RDINSTRETH(v); }
static inline int  CSRR_MVENDORID(void)   { return _CSRR_MVENDORID(); }
static inline void CSRW_MVENDORID(int v)  { _CSRW_MVENDORID(v); }
static inline int  CSRR_MARCHID(void)     { return _CSRR_MARCHID(); }
static inline void CSRW_MARCHID(int v)    { _CSRW_MARCHID(v); }
static inline int  CSRR_MIMPID(void)      { return _CSRR_MIMPID(); }
static inline void CSRW_MIMPID(int v)     { _CSRW_MIMPID(v); }
static inline int  CSRR_MHARTID(void)     { return _CSRR_MHARTID(); }
static inline void CSRW_MHARTID(int v)    { _CSRW_MHARTID(v); }
static inline int  CSRR_MISA(void)        { return _CSRR_MISA(); }
static inline void CSRW_MISA(int v)       { _CSRW_MISA(v); }

// no support CSR
static inline int  CSRR_DPC(void)         { return _CSRR_DPC(); }
static inline void CSRW_DPC(int v)        { _CSRW_DPC(v); }

#endif // __RVCONFIG_H

