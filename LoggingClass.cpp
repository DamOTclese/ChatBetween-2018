
// ----------------------------------------------------------------------
// LoggingClass -- Small log file class which creates a log file using
// a name based upon the current date and time, and allows 
// NULL-terminated ASCII text to be appended to the growing log.
//
// Logging may be enabled or disabled through a method, however 
// logging is default enabled.
//
// See main.c for disclaimers and other information.
//
// Fredric L. Rice, June 2018
// http://www.crystallake.name
// fred @ crystal lake . name
// 
// ----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "LoggingClass.h"       // Our own class and defined constants

// ----------------------------------------------------------------------
// Local data storage
//
// ----------------------------------------------------------------------

    static const char * months[ ] = 
    { 
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
    } ;

// ----------------------------------------------------------------------
// LoggingClass Constructor
//
// The file name stored locally in the class gets set to all zeros,
// and the pointer to the log file hangle gets initialized to NULL.
//
// ----------------------------------------------------------------------

LoggingClass::LoggingClass( void ) : write_log_p( NULL ), logging_enabled( true )
{
    // Make sure that the log file name starts out filled with zeros
    (void)memset( log_file_name, ASCII_NULL_ZERO, sizeof( log_file_name ) );
}

// ----------------------------------------------------------------------
// LoggingClass Destructor
//
// If the log file handle is open, the file gets flushed, the file is
// closed, and the handle is set to indicate that the file is closed.
//
// ----------------------------------------------------------------------

LoggingClass::~LoggingClass( void )
{
    if ( ( FILE *)NULL != write_log_p )
    {
        (void)fflush( write_log_p );

        (void)fclose( write_log_p );

        write_log_p = (FILE *)NULL;
    }
}

// ----------------------------------------------------------------------
// LoggingClass Logging Create Log
//
// The log file name is created using the current local date and time,
// then the file is opened for write text.
//
// ----------------------------------------------------------------------

void LoggingClass::logging_create_log( void )
{
    struct tm our_time;
    time_t    current_time = 0L;

    // Make sure that the file is not already opened
    if ( (FILE *)NULL == write_log_p )
    {
        // Find out what the current time is
        current_time = time( NULL );

        // Convert that Linux epoc in to local time structure
        (void)localtime_r( &current_time, &our_time );

        // Build a log file name
        (void)sprintf( log_file_name, 
            "%02d%s%02d%02d-%02d-%02d-chatlog.txt",
            our_time.tm_mday, months[ our_time.tm_mon ], ( our_time.tm_year + 1900 ),
            our_time.tm_hour, our_time.tm_min, our_time.tm_sec );

        // Attempt to create a log file using that file name
        if ( ( FILE *)NULL == (write_log_p = fopen( log_file_name, "w+t") ) )
        {
            (void)printf( "I am unable to create log file [%s]\n", log_file_name );
        }
    }
}

// ----------------------------------------------------------------------
// LoggingClass Logging Write Log
//
// The text passed to this method by NULL-terminated string is written
// to the end of the log file. The function assumes that there is room
// on the file system and that the file system is fast enough to store
// all data in the write request.
//
// ----------------------------------------------------------------------

void LoggingClass::logging_write_log( const char * this_text_p )
{
    // Make sure that the file is open and
    // Make sure that logging is enabled
    if ( (FILE *)NULL != write_log_p && true == logging_enabled )
    {
        // Write the string and assume that it was successful
        (void)fputs( this_text_p, write_log_p );

        // Avoid Linux's flushing process and request it now
        (void)fflush( write_log_p );
    }
}

// ----------------------------------------------------------------------
// LoggingClass Logging Enable Disable
//
// The logging can be turned on or off depending upon the argument
// passed to the function.
//
// ----------------------------------------------------------------------

void LoggingClass::logging_enable_disable( const bool logging_on_off )
{
    logging_enabled = logging_on_off;
}


