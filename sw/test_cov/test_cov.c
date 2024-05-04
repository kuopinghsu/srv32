
// This test is intended to cover instructions not tested in the general test
// to improve code coverage.
#include <stdint.h>
#include "rvconfig.h"

char v = 0;

static void test_load_store(volatile int *ptr) {
    volatile int8_t *n1 = (int8_t *)ptr;
    volatile int16_t *n2 = (int16_t *)ptr;
    volatile int32_t *n3 = (int32_t *)ptr;

    *n1 = v;
    (*n1)++;
    (*n2)++;
    (*n3)++;

    (void)n1;
    (void)n2;
    (void)n3;
}

static void test_mul_div(volatile int *ptr1, volatile int *ptr2) {
    volatile int n = *(volatile int *)ptr1 * *(volatile int *)ptr2;
    n = *(volatile unsigned int *)ptr1 * *(volatile unsigned *)ptr2;
    n = *(volatile int *)ptr1 / *(volatile int *)ptr2;
    n = *(volatile unsigned int *)ptr1 / *(volatile unsigned *)ptr2;
    n = *(volatile int *)ptr1 % *(volatile int *)ptr2;
    n = *(volatile unsigned int *)ptr1 % *(volatile unsigned *)ptr2;

    asm volatile("mulh %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("mulhsu %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("mulhu %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("div %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("divu %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("rem %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("remu %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));

    (void)n;
}

static void test_misc(volatile int *ptr1, volatile int *ptr2) {
    volatile int n;
    asm volatile("slt %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    asm volatile("sra %0, %1, %2" : "=r"(n) : "r"(ptr1), "r"(ptr2));
    (void)n;
}

int main(void) {
    static int n = 0;
    static int n1 = 3, n2 = 4;

    test_load_store(&n);
    test_mul_div(&n1, &n2);
    test_misc(&n, &n2);
    
    return 0;
}

