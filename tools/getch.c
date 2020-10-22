#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int getch(void)
{
    struct termios oldattr, newattr;
    int ch;

    tcgetattr( STDIN_FILENO, &oldattr );

    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON /* | ECHO */ );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );

    ch = getchar();

    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );

    return ch;
}

