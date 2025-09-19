
//  ___start of includes___

// Provides functions and structures to control terminal I/O
// (e.g., tcgetattr/tcsetattr to get/set terminal attributes)
// we also use STDIN_FILENO here as our fd.

/*
    struct termios {
        tcflag_t c_iflag;   // input modes
        tcflag_t c_oflag;   // output modes
        tcflag_t c_cflag;   // control modes (baud rate, etc.)
        tcflag_t c_lflag;   // local modes (echo, canonical, etc.)
        cc_t     c_cc[NCCS]; // control characters (e.g., VMIN, VTIME)
    };
*/

// the tcgetattr function will fetch in above struct, already populated with flags
// and we will change some attributes, and then use tcsetattr to update terminal settings
#include <termios.h>

// Provides system calls like read(), write(), close()
// Defines macros for standard file descriptors:
// STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2
// we use STDIN_FILENO, which is 0 by default, this means the programs
// standard input stream (the shell)
#include <unistd.h>

// this is just to print stuff
#include <stdio.h>

// this library will allow us to use functions from c
#include <stdlib.h>

//this library will help us with reading character input
//i mean we already get read from unistd.h, but this is going to help us analyze the character
//before we print it to the console. E.g ASCII 0-31 are unprintable
//so we will use iscntrl() to check
#include <ctype.h>

//  ___end of includes___

// any global variables
struct termios globalTerminalState;

//  ___start of function implementations___

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &globalTerminalState);
}

void enableRawMode()
{

    // we want to disable:

    //ECHO
    //  What it does:
    //      When enabled (default), anything you type in the terminal is echoed back to the screen.
    //      Example: You type abc, the terminal shows abc.
    //     Why turn it off?
    //         Password input → so typed characters don’t show.
    //         Interactive programs (games, editors) → they handle display themselves.
    //         Custom shells → to control exactly what’s visible.

    //ICANON
    //  What it does:
        // When enabled (default): terminal is in canonical (line) mode.
        // Input is line-buffered → you type a whole line and press Enter before the program sees it.
        // Special line-editing keys work automatically (e.g., Backspace, Ctrl+U, Ctrl+D).
        // When disabled: terminal is in non-canonical (raw) mode.
        //     Input is made available to the program immediately, character by character, without waiting for Enter.
        //     Special keys are no longer handled by the terminal (your program must handle them if you want that behavior).
        // Why turn it off?
        //     Real-time applications like:
        //     Text editors (e.g., vim, nano, kilo) → need to process each keystroke instantly.
        //     Games in the terminal → move a character with w/a/s/d without waiting for Enter.
        //     REPLs / shells → might want custom line editing.

    // basically termios.c_lflag is a 32bit binary value
    // each bit is either 1 or 0
    // to turn of echo, the flag is 00000000000000000000000000001000 (echo is 4th bit)
    // and so we invert it, and then & with c_lflag to set that bit to 0

    tcgetattr(STDIN_FILENO, &globalTerminalState); // populates raw with this shells attributes
    atexit(disableRawMode);                        // when the program exits, reset the terminal to default by calling function
    // atexit comes from stdlib
    struct termios raw = globalTerminalState; // make a copy here of the og default state

    // tcflag_t flag = raw.c_lflag;
    // //unsigned long
    // printf("flag for local modes: %ld", flag);

    raw.c_lflag &= ~(ECHO); // this sets the echo flag to off
    raw.c_lflag &= ~(ICANON); //following same logic as above, we disable icanon

    // now to update the terminal, we just have to
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    // the TCSAFLUSH macro just waits to update changes after we flush input commands
}

int main()
{

    enableRawMode();

    char c; // this is a character, can store 1 byte of input

    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
        //for every character typed, we want to display it to the shell
        //but to do that, we need to makesure the character is printable
        if(iscntrl(c)){ //tests if the character is a control character

        }
    }

    return 0;
}
