#include <stdio.h>
#include <unistd.h>

int main(void) {
    char c;
    printf("Enter character (enter to exit): ");
    fflush(stdout);
    do {
        size_t ret = read(STDIN_FILENO, &c, 1);
        if (ret != 1) continue;
        if (c >= 'a' && c <= 'z')
            printf("%c", c-'a'+'A');
        else
            printf("%c", c);
        fflush(stdout);
    } while (c != '\n');
    printf("\n");
    return 0;
}
