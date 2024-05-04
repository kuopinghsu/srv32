#include <stdio.h>
#include <unistd.h>
#include <termios.h>

// do not check coverage here
// LCOV_EXCL_START
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
// LCOV_EXCL_STOP
