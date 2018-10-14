
// ----------------------------------------------------------------------
// Small inter-computer chat program using UDP broadcasted messages
// to send and listen for text typed from the console. Since all frames 
// are broadcasted, there is no effort for security, all computers on
// the ethernet subnet will be able to see every frame.
//
// The text that gets sent and received gets logged to a text file
// if logging is enabled through the WANT_LOGGING defined constant.
//
// If more than one copy of the program is executed on the same 
// computer, the transmit and the receive UDP port numbers are swapped
// for all instantiations after the first one. Generally this is done
// for testing of the program locally on a single machine.
//
// Typing "exit" causes the program to terminate.
//
// Typing ":send" followed by a file name will cause the program to
// send that file to all chat programs that can hear. The file gets
// created in the directory that the chat program was launched within,
// and a check to make sure that the file does not already exist is
// done.
//
// Typing ":get" followed by a path and file name will cause the
// program to send a request to all listening devices to send a copy 
// of that file to the system that issued the :get, placing the file
// in the firestory where the program was launched from, changing the
// file name to avoid duplicate file names as needed.
//
// If logging is enabled using the WANT_LOGGING defined constant,
// typing :log will toggle logging on or off. Logging is enabled by
// default.
//
// Of course none of this is even remotely concerned with security.
// Anyone running WireShark or some other packet sniffer will see
// everything that you do, and the ability to send and receive files 
// using this software opens up your computers and entire network to 
// being cracked wide open.
//
// Because of that, you should be aware that using this software is
// not recommended unless you know what you're doing or you don't
// care that every device on your network has visible access to the
// data you send and receive. This software is not intended to be 
// used by anyone who values the security of their data, computers,
// networks, or any device connected to their networks -- obviously
// (at least it should be obvious.)
//
// Also note that we do not attempt to be MISRA compliant.
//
// Fredric L. Rice, June 2018
// http://www.crystallake.name
// fred @ crystal lake . name
// 
// ----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ChatClass.h"        // For UDP functionality
#include "ChatDefines.h"      // For defined constants
#if WANT_LOGGING
#include "LoggingClass.h"     // For logging functionality
#endif

// ----------------------------------------------------------------------
// Local data storage. We impose a limit on the data entered via 
// console for no other reason to avoid automated flooding of the 
// network via bad actors.
//
// ----------------------------------------------------------------------

static char console_in_data[ MAX_CONSOLE_IN_SIZE ];
static int  console_in_count;

// ----------------------------------------------------------------------
// Accumulates console input up to the maximum byte count which will 
// fit in to the local buffer console_in_data[] until a carriage return 
// is entered at which point the function returns the number of bytes 
// read. This is done rather than use ANSI console function calls to
// avoid blocking.
//
// ----------------------------------------------------------------------

static int accumulate_console_input( void )
{
    int read_count = 0;

    // See if there is console input waiting. If the buffer
    // is full, we ask for zero bytes
    read_count = read( 0, &console_in_data[ console_in_count ], 
        ( sizeof( console_in_data ) - ( console_in_count + 1 ) ) );

    if ( read_count > 0 )
    {
        // Keep track of how many bytes have been read so far
        console_in_count += read_count;

        // Was the last byte a line feed or carriage return?
        if ( console_in_data[ console_in_count - 1 ] == ASCII_CARRIAGE_RETURN ||
             console_in_data[ console_in_count - 1 ] == ASCII_LINE_FEED )
        {
            // NULL terminate the input so we can use it has a
            // NULL-terminated string
            console_in_data[ console_in_count ] = ASCII_NULL_ZERO;

            // Report on how many bytes have been read
            return console_in_count;
        }
    }

    // Report that no input exists yet
    return 0;
}

// ----------------------------------------------------------------------
// main() The main entry point
//
// Scan the console input for keyboard or piped script commands, 
// looking for commands to exit the program, or to send files, or to
// request files, or to enable/disable logging.
//
// Also scans for inbound UDP frames containing file transfer frames
// or text message frames. If a file transfer is in progress this
// function also checks to see if any transfered have timed out.
//
// ----------------------------------------------------------------------

int main( const int argc, const char * argv[] )
{
    int  read_count    = 0;
    int  while_running = true;
    bool logging_on    = true;

    // Instantiate a UDP Interface
    ChatClass udp_interface( DEFAULT_UDP_PORT_BASE );

#if WANT_LOGGING
    // Instantiate a Logging Interface
    LoggingClass log_interface;

    // Create a logging file
    log_interface.logging_create_log( );
#endif

    // Initialize our local data
    console_in_count = 0;

    // Set the console input to non-blocking
    (void)udp_interface.set_non_blocking( 0 );

    // Check for inbound UDP frames and for ourbound console input
    while( while_running )
    {
        // See if there is inbound data
        read_count = udp_interface.read_data( );

        // A byte count of greater than 0 indicates that there is
        // data that is likely a test message. File transfer frames
        // get processed by the Chat Class and only indicates that
        // there is data for us to process here if any inbound
        // data was not already processed and handled.
        if ( read_count > 0 )
        {
            // Make sure that the inbound datais NULL terminated
            udp_interface.udp_inbound_buffer[ read_count ] = ASCII_NULL_ZERO;

            // Treat the inbound UDP frame as a NULL-terminated string
            (void)printf( "%s", udp_interface.udp_inbound_buffer );

#if WANT_LOGGING
            // Log that inbound text
            log_interface.logging_write_log( udp_interface.udp_inbound_buffer );
#endif
        }

        // See if there is console input to send
        read_count = accumulate_console_input( );

        // Console input gets accumulated until a new line is entered
        if ( read_count > 0 )
        {
            // Is the operator asking to exit the program?
            if (! strncmp( console_in_data, command_exit, strlen( command_exit ) ) )
	        {
                while_running = false;
            }
#if ALLOW_COMMAND_SEND
            else if (! strncmp( console_in_data, command_send, strlen( command_send ) ) )
            {
                // Send an unsolicited file to all listening destinations.
                // Indicate that the send is not in response to a get
                // reqest received from another system.
                udp_interface.send_file( &console_in_data[ 5 ], false );
            }
#endif
#if ALLOW_COMMAND_GET
            else if (! strncmp( console_in_data, command_get, strlen( command_get ) ) )
            {
                // Query all listening destinations for a file and if
                // it is located, send it to the system that requested it
                udp_interface.get_file( &console_in_data[ 4 ] );
            }
#endif
#if WANT_LOGGING
    #if ALLOW_COMMAND_LOG
            else if (! strncmp( console_in_data, command_log, strlen( command_log ) ) )
            {
                // Toggle whether logging should be on or off
                logging_on = !logging_on;

                // Let the logger know
                log_interface.logging_enable_disable( logging_on );

                (void)printf(" Logging has been turned %s\n", 
                    logging_on ? "ON" : "OFF" );
            }
    #endif
#endif
            else
            {
                // Send the console input to all listening devices
                udp_interface.send_text( console_in_data );

#if WANT_LOGGING
                // Log that outbound text
                log_interface.logging_write_log( console_in_data );
#endif
            }

            // Start accumulating a new line from the console
            console_in_count = 0;
        }

        // See if we were receiving a file that timed out
        (void)udp_interface.transfer_timed_out( );

        // To avoid hard loops we delay a number of milliseconds
        usleep( MAIN_LOOP_SLEEP_DELAY );
    }

    // Set the console back to blocking 
    (void)udp_interface.set_blocking( 0 );

    // Terminate and report an Error Level Value of 0 to indicate no problems
    return ERRORLEVEL_NO_PROBLEM;
}

