// terminal and shell are the same thing

/*** includes ***/

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

// this library will help us with reading character input
// i mean we already get read from unistd.h, but this is going to help us analyze the character
// before we print it to the console. E.g ASCII 0-31 are unprintable
// so we will use iscntrl() to check
#include <ctype.h>

// this header file allows us to use error numbers
#include <errno.h>

// this header file will let us find the window size of our terminal
// so that we can get our text editor to fit the screen
// provides an api ioctl which populates a struct that has 2 datapoints
// which give row/column of current terminal
// the winsize struct itself is defined in ttycom.h
// which internally includes below header file, so we can just include this one
#include <sys/ioctl.h>

/*** end includes ***/
////////////////////////////////////////////////////////////////////////////////
/*** defines ***/

// the macro sets the upper 3 bits of the character to 0. This mirrors what the Ctrl key
// does in the terminal: it strips bits 5 and 6 from whatever key
// you press in combination with Ctrl, and sends that to the shell
// in binary its 00011111; so x & 00011111 maps to ctrl + someLetter
#define CTRL_KEY(x) ((x) & 0x1f)

/*** end defines ***/
////////////////////////////////////////////////////////////////////////////////
/*** global variables ***/

// //this saves our termial state so we can reset out of raw mode after program exits
// struct termios globalTerminalState;

// better to encapsulate into a struct, so that we can add/group other stuff
struct globalTerminalState
{
    // this struct stores the global state of our terminal
    struct termios globalTerminalState;

    // this represents the number of rows of our terminal
    int rows;

    // this represents the number of cols of our terminal
    int cols;
};

struct globalTerminalState globalSettings;

/*** end global variables ***/
////////////////////////////////////////////////////////////////////////////////
/*** terminal ***/

// this function is for error handling, using perror and exit syscalls
void error(const char *s)
{ // a const string, we make const to ensure string doesnt get changed within function
    // its just good code practice to have a const variable when not being changed

    // before that, its better if we refresh the screen so no garbage is left when the program
    // is forced to quit

    // this logic to refresh screen and what it means is in refreshScreen() function
    // but thats defined below, so i can't call it here
    // so ill just copy paste the things in there into here rather than moving function
    // because the function might change in future.
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // this one moves cursor back up to top

    perror(s);
    exit(1);
}

// resets anything after program exits
void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &globalSettings.globalTerminalState) == -1)
    {
        error("error disabling raw mode in disableRawMode()");

        // macro STDIN_FILENO refers to the terminal
        //  TCSAFLUSH is basically when? when we flushed out the final output of this program
        // and we pass in address of the global terminal struct
    }
}

// this function turns off some default terminal macros
// so that our text editor program behaves as it should...
void enableRawMode()
{

    // we want to disable:

    // ECHO
    //   What it does:
    //       When enabled (default), anything you type in the terminal is echoed back to the screen.
    //       Example: You type abc, the terminal shows abc.
    //      Why turn it off?
    //          Password input → so typed characters don’t show.
    //          Interactive programs (games, editors) → they handle display themselves.
    //          Custom shells → to control exactly what’s visible.

    // ICANON
    //   What it does:
    //  When enabled (default): terminal is in canonical (line) mode.
    //  Input is line-buffered → you type a whole line and press Enter before the program sees it.
    //  Special line-editing keys work automatically (e.g., Backspace, Ctrl+U, Ctrl+D).
    //  When disabled: terminal is in non-canonical (raw) mode.
    //      Input is made available to the program immediately, character by character, without waiting for Enter.
    //      Special keys are no longer handled by the terminal (your program must handle them if you want that behavior).
    //  Why turn it off?
    //      Real-time applications like:
    //      Text editors (e.g., vim, nano, kilo) → need to process each keystroke instantly.
    //      Games in the terminal → move a character with w/a/s/d without waiting for Enter.
    //      REPLs / shells → might want custom line editing.

    // ISIG
    //   What it does:
    //       Handles signal interrupts like ctrl c and z

    // IXON
    //   What it does:
    //       Handles ctrl s and q. ctrl s would stop terminal from showing output
    //       ctrl q would resume that; we don't need it so we will turn it off

    // IEXTEN
    //   What it does:
    //       Handles ctrl v, which allows us to send control characters, but we dont
    //       need it

    // ICRNL
    //   What it does:
    //       Handles ctrl m; now all letters are there, but m because it is a special character
    //       so this disables that behavior

    // OPOST
    //   What it does:
    //       To get the text-editor behavior, we need to stop having newline be translated
    //       into \r\n (new line with return so it goes to the start)

    // basically termios.c_lflag is a 32bit binary value
    // each bit is either 1 or 0
    // to turn of echo, the flag is 00000000000000000000000000001000 (echo is 4th bit)
    // and so we invert it, and then & with c_lflag to set that bit to 0

    if (tcgetattr(STDIN_FILENO, &globalSettings.globalTerminalState) == -1)
    { // populates raw with this shells attributes
        error("error getting attributes in enableRawMode()");
    }
    atexit(disableRawMode); // when the program exits, reset the terminal to default by calling function
    // atexit comes from stdlib
    struct termios raw = globalSettings.globalTerminalState; // make a copy here of the og default state

    // tcflag_t flag = raw.c_lflag;
    // //unsigned long
    // printf("flag for local modes: %ld", flag);

    raw.c_lflag &= ~(ECHO);   // this sets the echo flag to off
    raw.c_lflag &= ~(ICANON); // following same logic as above, we disable icanon
    raw.c_lflag &= ~(ISIG);   // same here
    raw.c_lflag &= ~(IEXTEN);

    raw.c_iflag &= ~(ISIG);
    raw.c_iflag &= ~(ICRNL);

    raw.c_oflag &= ~(OPOST); // now we write \r\n wherever we want a newline

    // below we turn off a bunch of semi random flags
    // at some point someone decided these flags have to be turned off
    // for rawmode, and although it might not make a difference
    // we will do it too
    raw.c_cflag |= (CS8);
    raw.c_iflag &= ~(BRKINT);
    raw.c_iflag &= ~(INPCK);
    raw.c_iflag &= ~(ISTRIP);

    // we set the c_cc field for control characters
    // vmin and vtime are gonna be 2 macros within the field that we set
    // vmin is the minimum number of bytes the terminal should read before returning
    // and vtime is the amount of waiting time before read() returns

    raw.c_cc[VMIN] = 0;  // return 0 if nothing is read
    raw.c_cc[VTIME] = 1; // wait 1/10ths of a second before read() returns

    // now to update the terminal, we just have to
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        error("error setting attributes in enableRawMode()");
    }
    // the TCSAFLUSH macro just waits to update changes after we flush input commands
}

// gonna make 2 functions to split up the while loop, one will be calling read
// and making sure theres no error in that
// the other will check if the character is a ctrl key and process it accordingly

// this function will return a char, and is essentially gonna read in a character
// will throw error if there is a problem reading

char readKey()
{
    int readVal; // this will store the value read returns
    char c;

    while ((readVal = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (readVal == -1 && errno == EAGAIN)
        {
            error("error in reading input from read()");
        }
    }

    return c;
}

// this method is a backup for when the getRowColumn won't work
// that method will usually fail when the api doesnt work

int getCursorPosition(int *row, int *column)
{
    char buf[32];
    unsigned int i = 0;
    // the [6n command says give me col number
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    {
        return -1;
    }

    printf("\r\n"); // print a new line 
    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[')
    {
        return -1;
    }
    if (sscanf(&buf[2], "%d;%d", row, column) != 2)
    {
        return -1;
    }
    return 0;
}

// this function will set the row/column of the current terminal
// and return 1 upon success else -1

int getRowColumn(int *row, int *column)
{
    // per documentation , the struct is winsize (ctrl/cmd click the include above)
    struct winsize size;
    // the TIOCGWINSZ macro is just for getting windows size
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1 || size.ws_col == 0)
    {
        // if the api fails or if there aren't any columns
        // then that means we can't render anything...
        // the method should return -1, but theres a bruteforce way of finding what we want
        //"\x1b[999C\x1b[999B" remember, x1b[ is a common thing, C and B are cursor commands
        //  C moves right, B moves down
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
        {
            return -1;
        }
        // after moving cursor, we can get position
        return getCursorPosition(row, column);
    }
    else
    {
        // this is how you would return multiple values in c...by setting pointers to something
        *row = size.ws_row;
        *column = size.ws_col;
        return 1;
    }
}

/*** end terminal ***/
////////////////////////////////////////////////////////////////////////////////

/*** output ***/

// this function will start off by drawing ~ for each row
// we will start by doing what vim does in an empty file
//...draw ~
void drawRows()
{
    for (int i = 0; i < globalSettings.rows; i++)
    {
        write(STDOUT_FILENO, "~", 1);
        // we are writing out a ~, recall \r\n for new line, and 3 is the num bytes
        // and as mentioned above, the macro is for writing out to the shell

        //small error when the last line is an empty new line with no ~
        if(i < globalSettings.rows - 1){
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}

// this function is gonna be clearing the terminal everytime a new character is typed
// it will refresh the screen
void refreshScreen()
{
    // in write, the stdout_fileno macro says to write to the terminal
    // we are writing 4 bytes.

    // the first byte is \x1b which is an escape sequence
    // and the next 3 bytes are [2J;

    // writing an escape sequence to the terminal means starting with
    //\x1b, and then adding [ after. then based off a table of sequences
    // we can instruct the terminal to do certain things (change color, clear etc)
    // since all esc sequences have \x1b[, lets call it *;
    // now here are a few examples:
    /*
     *1J, erases everything in terminal up to the cursor
     *2J, erases entire screen
     *0J, erases from cursor to end of screen (also just *J)
     */
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // this one moves cursor back up to top

    // after we refresh the screen, lets revert back to the default
    // which is drawing ~
    drawRows();
    // and then we recenter the cursor
    write(STDOUT_FILENO, "\x1b[H", 3);

    // all the macros, and 'codes' or escape sequences are predefined in library
}

/*** end output ***/
////////////////////////////////////////////////////////////////////////////////

/*** input ***/

// this function will process key and check if it is a control key bind
void processKey()
{
    // first get key to process
    char c = readKey();
    switch (c)
    {
    case CTRL_KEY('q'):
    {
        // like in error()
        write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
        write(STDOUT_FILENO, "\x1b[H", 3);  // this one moves cursor back up to top
        exit(0);
        break;
    }
    }
}

/*** end input ***/
////////////////////////////////////////////////////////////////////////////////

// this method will populate the terminal's rows and column numbers
// and will act as initializing our global terminal struct
void setupGlobStruct()
{
    if (getRowColumn(&globalSettings.rows, &globalSettings.cols) == -1)
    {
        error("can't get windows size in getWindowSize()");
    }
}

/*** init ***/
int main()
{

    enableRawMode();
    setupGlobStruct();

    while (1)
    {
        refreshScreen();
        processKey();
    }

    return 0;
}

/*** end init ***/
////////////////////////////////////////////////////////////////////////////////
