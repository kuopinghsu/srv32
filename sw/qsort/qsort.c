#include <stdio.h>
#include <stdlib.h>

typedef double DATA;

DATA values[] = {
    -192.293, 382.19, 382.18, -192.294, .000001, 283874923.123, 0.0000001
};
#define N (sizeof(values)/sizeof(DATA))

int comp (const void *a, const void *b) {
    return ( *(DATA*)a <  *(DATA*)b) ? -1 :
           ( *(DATA*)a == *(DATA*)b) ? 0 : 1;
}

int main(void) {
    int i;
    printf("Before sorting: \n");
    for (i=0; i<N; i++) printf("%e ", values[i]);

    qsort(values, N, sizeof(DATA), comp);

    printf("\nAfter sorting: \n");
    for (i=0; i<N; i++) printf("%e ", values[i]);

    printf("\n");
    return 0;
}

