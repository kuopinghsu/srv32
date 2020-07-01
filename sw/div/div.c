#include <stdio.h>

#define DIV(r,a,b) asm volatile ("div %0, %1, %2" : "=r"(r) : "r"(a), "r"(b) :)
#define DIVU(r,a,b) asm volatile ("divu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b) :)
#define REM(r,a,b) asm volatile ("rem %0, %1, %2" : "=r"(r) : "r"(a), "r"(b) :)
#define REMU(r,a,b) asm volatile ("remu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b) :)

int result[100] = {1};

int main(void) {
    int *ptr = result;

    DIV(*ptr++, 0, 0);
    DIV(*ptr++, 0, 1);
    DIV(*ptr++, 0, -1);
    DIV(*ptr++, 0, 0x7fffffff);
    DIV(*ptr++, 0, 0x80000000);

    DIV(*ptr++, 1, 0);
    DIV(*ptr++, 1, 1);
    DIV(*ptr++, 1, -1);
    DIV(*ptr++, 1, 0x7fffffff);
    DIV(*ptr++, 1, 0x80000000);

    DIV(*ptr++, -1, 0);
    DIV(*ptr++, -1, 1);
    DIV(*ptr++, -1, -1);
    DIV(*ptr++, -1, 0x7fffffff);
    DIV(*ptr++, -1, 0x80000000);

    DIV(*ptr++, 0x7fffffff, 0);
    DIV(*ptr++, 0x7fffffff, 1);
    DIV(*ptr++, 0x7fffffff, -1);
    DIV(*ptr++, 0x7fffffff, 0x7fffffff);
    DIV(*ptr++, 0x7fffffff, 0x80000000);

    DIV(*ptr++, 0x80000000, 0);
    DIV(*ptr++, 0x80000000, 1);
    DIV(*ptr++, 0x80000000, -1);
    DIV(*ptr++, 0x80000000, 0x7fffffff);
    DIV(*ptr++, 0x80000000, 0x80000000);

    DIVU(*ptr++, 0, 0);
    DIVU(*ptr++, 0, 1);
    DIVU(*ptr++, 0, -1);
    DIVU(*ptr++, 0, 0x7fffffff);
    DIVU(*ptr++, 0, 0x80000000);

    DIVU(*ptr++, 1, 0);
    DIVU(*ptr++, 1, 1);
    DIVU(*ptr++, 1, -1);
    DIVU(*ptr++, 1, 0x7fffffff);
    DIVU(*ptr++, 1, 0x80000000);

    DIVU(*ptr++, -1, 0);
    DIVU(*ptr++, -1, 1);
    DIVU(*ptr++, -1, -1);
    DIVU(*ptr++, -1, 0x7fffffff);
    DIVU(*ptr++, -1, 0x80000000);

    DIVU(*ptr++, 0x7fffffff, 0);
    DIVU(*ptr++, 0x7fffffff, 1);
    DIVU(*ptr++, 0x7fffffff, -1);
    DIVU(*ptr++, 0x7fffffff, 0x7fffffff);
    DIVU(*ptr++, 0x7fffffff, 0x80000000);

    DIVU(*ptr++, 0x80000000, 0);
    DIVU(*ptr++, 0x80000000, 1);
    DIVU(*ptr++, 0x80000000, -1);
    DIVU(*ptr++, 0x80000000, 0x7fffffff);
    DIVU(*ptr++, 0x80000000, 0x80000000);

    REM(*ptr++, 0, 0);
    REM(*ptr++, 0, 1);
    REM(*ptr++, 0, -1);
    REM(*ptr++, 0, 0x7fffffff);
    REM(*ptr++, 0, 0x80000000);

    REM(*ptr++, 1, 0);
    REM(*ptr++, 1, 1);
    REM(*ptr++, 1, -1);
    REM(*ptr++, 1, 0x7fffffff);
    REM(*ptr++, 1, 0x80000000);

    REM(*ptr++, -1, 0);
    REM(*ptr++, -1, 1);
    REM(*ptr++, -1, -1);
    REM(*ptr++, -1, 0x7fffffff);
    REM(*ptr++, -1, 0x80000000);

    REM(*ptr++, 0x7fffffff, 0);
    REM(*ptr++, 0x7fffffff, 1);
    REM(*ptr++, 0x7fffffff, -1);
    REM(*ptr++, 0x7fffffff, 0x7fffffff);
    REM(*ptr++, 0x7fffffff, 0x80000000);

    REM(*ptr++, 0x80000000, 0);
    REM(*ptr++, 0x80000000, 1);
    REM(*ptr++, 0x80000000, -1);
    REM(*ptr++, 0x80000000, 0x7fffffff);
    REM(*ptr++, 0x80000000, 0x80000000);

    REMU(*ptr++, 0, 0);
    REMU(*ptr++, 0, 1);
    REMU(*ptr++, 0, -1);
    REMU(*ptr++, 0, 0x7fffffff);
    REMU(*ptr++, 0, 0x80000000);

    REMU(*ptr++, 1, 0);
    REMU(*ptr++, 1, 1);
    REMU(*ptr++, 1, -1);
    REMU(*ptr++, 1, 0x7fffffff);
    REMU(*ptr++, 1, 0x80000000);

    REMU(*ptr++, -1, 0);
    REMU(*ptr++, -1, 1);
    REMU(*ptr++, -1, -1);
    REMU(*ptr++, -1, 0x7fffffff);
    REMU(*ptr++, -1, 0x80000000);

    REMU(*ptr++, 0x7fffffff, 0);
    REMU(*ptr++, 0x7fffffff, 1);
    REMU(*ptr++, 0x7fffffff, -1);
    REMU(*ptr++, 0x7fffffff, 0x7fffffff);
    REMU(*ptr++, 0x7fffffff, 0x80000000);

    REMU(*ptr++, 0x80000000, 0);
    REMU(*ptr++, 0x80000000, 1);
    REMU(*ptr++, 0x80000000, -1);
    REMU(*ptr++, 0x80000000, 0x7fffffff);
    REMU(*ptr++, 0x80000000, 0x80000000);

    return 0;
}

