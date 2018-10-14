
// ----------------------------------------------------------------------
// Various MACROs and anything else that does not fit well anywhere else
//
// See main.c for disclaimers and other information.
//
// Fredric L. Rice, June 2018
// http://www.crystallake.name
// fred @ crystal lake . name
// 
// ----------------------------------------------------------------------

#ifndef _CHAT_DEFINES_H_
#define _CHAT_DEFINES_H_    1

// ----------------------------------------------------------------------
// We always allow the "exit" command to be entered from the console
// to terminate the program. The other commands are wapped in
// conditional compiles to enable or disable those commands. Set the
// value to 0 to not allow various commands.
//
// If you want a simple many-to-many chat program, set them all to 0.
//
// ----------------------------------------------------------------------

#define ALLOW_COMMAND_SEND  1
#define ALLOW_COMMAND_GET   1
#define ALLOW_COMMAND_LOG   1

// ----------------------------------------------------------------------
// You can turn logging off entirely by setting this value to 0 zero
//
// ----------------------------------------------------------------------

#define WANT_LOGGING        1

// ----------------------------------------------------------------------
// The console commands to control things can be redefined here.
//
// ----------------------------------------------------------------------

    const char *command_exit = "exit";
    const char *command_send = ":send";
    const char *command_get  = ":get";
    const char *command_log  = ":log";

// ----------------------------------------------------------------------
// The UDP port numbers used to transmit and receive are defined here
// by providing the base UDP port number. All UDP port numbers used
// in this program start from this base number.
//
// ----------------------------------------------------------------------

#define DEFAULT_UDP_PORT_BASE       5777

// ----------------------------------------------------------------------
// Maximum console input buffer size is defined here.
//
// ----------------------------------------------------------------------

#define MAX_CONSOLE_IN_SIZE         1024

// ----------------------------------------------------------------------
// Other defined constants that we will be using. We attempt to avoid
// hard-coded numbers in the source code.
//
// ----------------------------------------------------------------------

#define ASCII_NULL_ZERO             0x00
#define ASCII_LINE_FEED             0x0a
#define ASCII_CARRIAGE_RETURN       0x0d

// ----------------------------------------------------------------------
// The main function avoids hard looping by delying this number of
// microseconds. Typically we go for around 5 milliseconds.
//
// ----------------------------------------------------------------------

#define MAIN_LOOP_SLEEP_DELAY       5000

#endif
