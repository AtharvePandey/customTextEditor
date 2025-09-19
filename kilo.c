
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

//this header file allows us to use error numbers
#include <errno.h>

//  ___end of includes___

// any global variables

//this saves our termial state so we can reset out of raw mode after program exits
struct termios globalTerminalState;


//  ___start of function implementations___


//this function is for error handling, using perror and exit syscalls
void error(const char * s){ //a const string, we make const to ensure string doesnt get changed within function
    perror(s);
    exit(1);
}


//resets anything after program exits
void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &globalTerminalState) == -1 ? error("error disabling raw mode in disableRawMode()") : 0;
}

//this function turns off some default terminal macros
//so that our text editor program behaves as it should...
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

    //ISIG
    //  What it does:
    //      Handles signal interrupts like ctrl c and z

    //IXON
    //  What it does:
    //      Handles ctrl s and q. ctrl s would stop terminal from showing output
    //      ctrl q would resume that; we don't need it so we will turn it off

    //IEXTEN
    //  What it does:
    //      Handles ctrl v, which allows us to send control characters, but we dont
    //      need it

    //ICRNL
    //  What it does:
    //      Handles ctrl m; now all letters are there, but m because it is a special character
    //      so this disables that behavior

    //OPOST
    //  What it does:
    //      To get the text-editor behavior, we need to stop having newline be translated
    //      into \r\n (new line with return so it goes to the start)

    // basically termios.c_lflag is a 32bit binary value
    // each bit is either 1 or 0
    // to turn of echo, the flag is 00000000000000000000000000001000 (echo is 4th bit)
    // and so we invert it, and then & with c_lflag to set that bit to 0

    if(tcgetattr(STDIN_FILENO, &globalTerminalState) == -1){// populates raw with this shells attributes
        error("error getting attributes in enableRawMode()");
    } 
    atexit(disableRawMode);                        // when the program exits, reset the terminal to default by calling function
    // atexit comes from stdlib
    struct termios raw = globalTerminalState; // make a copy here of the og default state

    // tcflag_t flag = raw.c_lflag;
    // //unsigned long
    // printf("flag for local modes: %ld", flag);

    raw.c_lflag &= ~(ECHO); // this sets the echo flag to off
    raw.c_lflag &= ~(ICANON); //following same logic as above, we disable icanon
    raw.c_lflag &= ~(ISIG); //same here
    raw.c_lflag &= ~(IEXTEN);

    raw.c_iflag &= ~(ISIG);
    raw.c_iflag &= ~(ICRNL);

    raw.c_oflag &= ~(OPOST); //now we write \r\n wherever we want a newline

    //below we turn off a bunch of semi random flags
    //at some point someone decided these flags have to be turned off
    //for rawmode, and although it might not make a difference
    //we will do it too
    raw.c_cflag |= (CS8);
    raw.c_iflag &= ~(BRKINT);
    raw.c_iflag &= ~(INPCK);
    raw.c_iflag &= ~(ISTRIP);

    //we set the c_cc field for control characters
    //vmin and vtime are gonna be 2 macros within the field that we set
    //vmin is the minimum number of bytes the terminal should read before returning
    //and vtime is the amount of waiting time before read() returns

    raw.c_cc[VMIN] = 0; //return 0 if nothing is read
    raw.c_cc[VTIME] = 1; //wait 1/10ths of a second before read() returns


    // now to update the terminal, we just have to
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
        error("error setting attributes in enableRawMode()");
    }
    // the TCSAFLUSH macro just waits to update changes after we flush input commands
}

int main()
{

    enableRawMode();

    while (1){
        char c = '\0'; // this is a character, can store 1 byte of input null by default
        if(read(STDIN_FILENO, &c, 1) == -1 && errno == EAGAIN){
            error("error in reading input from read()");
        }  
        //for every character typed, we want to display it to the shell
        //but to do that, we need to makesure the character is printable
        if(iscntrl(c)){ //tests if the character is a control character i.e \t
            //if it is a control character, we print the ascii value
            printf("%d\r\n", c);
        }else{
            //lets print ascii and how the character is via %c
            printf("ascii: %d, character: '%c'\r\n", c, c);
        }
        if(c == 'q'){
            break;
        }
    }

    return 0;
}
