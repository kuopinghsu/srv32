#include <stdio.h>

int main(void) {
    char c;
    printf("Enter character: ");
    fflush(stdout);
    do {
        c = getchar();
        if (c >= 'a' && c <= 'z')
            printf("%c", c-'a'+'A');
        else
            printf("%c", c);
    } while (c != 'x');
    printf("\n");
    return 0;
}
