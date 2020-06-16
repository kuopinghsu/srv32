#include <machine/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>

#define MEMIO_PUTC          0x8000001c
#define MEMIO_EXIT          0x8000002c

extern int errno;

volatile char _ch;
int _putchar(char ch) {
    //_ch = ch;
    *(volatile char*)MEMIO_PUTC = (char)ch;
    return 0;
}

/* Write to a file.  */
ssize_t
_write(int file, const void *ptr, size_t len)
{
    const char *buf = (char*)ptr;
    int i;
    for(i=0; i<len; i++)
        _putchar(buf[i]);
    return len;
}

int _fstat(int file, struct stat *st)
{
    //errno = ENOENT;
    return -1;
}

int _close(int file)
{
    //errno = ENOENT;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    //errno = ENOENT;
    return -1;
}

int _read(int file, char *ptr, int len)
{
    //errno = ENOENT;
    return -1;
}

void _exit(int code) {
    *(volatile char*)MEMIO_EXIT = code;
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

