#include <machine/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>

#define HAVE_SYSCALL        0
#define MEMIO_PUTC          0x9000001c
#define MEMIO_GETC          0x90000020
#define MEMIO_EXIT          0x9000002c

// system call defined in the file /usr/include/asm-generic/unistd.h
enum {
    SYS_OPEN    = 0xbeef0031,
    SYS_LSEEK   = 0xbeef0032,
    SYS_CLOSE   = 0xbeef0039,
    SYS_READ    = 0xbeef003f,
    SYS_WRITE   = 0xbeef0040,
    SYS_FSTAT   = 0xbeef0050,
    SYS_EXIT    = 0xbeef005d,
    SYS_SBRK    = 0xbeef00d6
};

extern int errno;

static inline long
__internal_syscall(long n, long _a0, long _a1, long _a2, long _a3, long _a4, long _a5)
{
    register long a0 asm("a0") = _a0;
    register long a1 asm("a1") = _a1;
    register long a2 asm("a2") = _a2;
    register long a3 asm("a3") = _a3;
    register long a4 asm("a4") = _a4;
    register long a5 asm("a5") = _a5;

    #ifdef __riscv_32e
    register long syscall_id asm("t0") = n;
    #else
    register long syscall_id asm("a7") = n;
    #endif

    asm volatile ("ecall"
        : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(syscall_id));

    return a0;
}

int _putchar(char ch) {
    *(volatile char*)MEMIO_PUTC = (char)ch;
    return 0;
}

int _getchar(void) {
    return *(volatile char*)MEMIO_GETC;
}

/* open file */
ssize_t
_open(const char *pathname, int flags, int mode)
{
#if HAVE_SYSCALL
    int res = __internal_syscall(SYS_OPEN, (long)pathname, (long)flags, (long)mode, 0, 0, 0);
    return res;
#else
    return -1;
#endif
}

/* Write to a file.  */
ssize_t
_write(int file, const void *ptr, size_t len)
{
#if HAVE_SYSCALL
    int res = __internal_syscall(SYS_WRITE, (long)file, (long)ptr, (long)len, 0, 0, 0);
    return res;
#else
    const char *buf = (char*)ptr;
    int i;
    for(i=0; i<len; i++)
        _putchar(buf[i]);
    return len;
#endif
}

int _fstat(int file, struct stat *st)
{
    //errno = ENOENT;
    return -1;
}

int _close(int file)
{
#if HAVE_SYSCALL
    int res = __internal_syscall(SYS_CLOSE, (long)file, 0, 0, 0, 0, 0);
    return res;
#else
    //errno = ENOENT;
    return -1;
#endif
}

int _lseek(int file, int ptr, int dir)
{
#if HAVE_SYSCALL
    int res = __internal_syscall(SYS_LSEEK, (long)file, (long)ptr, (long)dir, 0, 0, 0);
    return res;
#else
    //errno = ENOENT;
    return -1;
#endif
}

#include <stdio.h>
int _read(int file, char *ptr, int len)
{
#if HAVE_SYSCALL
    int res = __internal_syscall(SYS_READ, (long)file, (long)ptr, (long)len, 0, 0, 0);
    return res;
#else
    char *buf = (char*)ptr;
    char c = 0;
    int i = 0;
    do {
        c = _getchar();
        buf[i] = c;
    } while(++i<len && c != '\n');
    return len;
#endif
}

void _exit(int code) {
#if HAVE_SYSCALL
    __internal_syscall(SYS_EXIT, code, 0, 0, 0, 0, 0);
#else
    *(volatile char*)MEMIO_EXIT = code;
#endif
    while(1);
}

void exit(int code) {
    _exit(code);
}

int _isatty(int file)
{
    return (file == STDIN_FILENO  ||
            file == STDERR_FILENO ||
            file == STDOUT_FILENO) ? 1 : 0;
}

extern char _end[];                /* _end is set in the linker command file */
extern char _stack[];              /* _stack is set in the linker command file */
char *heap_ptr;

char *
_sbrk (int nbytes)
{ 
    char *base;
  
    if (!heap_ptr)
        heap_ptr = (char *)&_end;

    if ((int)(heap_ptr+nbytes) >= (int)&_stack) {
        return 0;
    } else {
        base = heap_ptr;
        heap_ptr += nbytes;
    }
  
    return base;
}

int _kill(int pid, int sig)
{
    return -1;
}

int _getpid(void)
{
    return 0;
}

