
// ----------------------------------------------------------------------
// LoggingClass -- Small log file class which creates a log file using
// a name based upon the current date and time, and allows 
// NULL-terminated ASCII text to be appended to the growing log.
//
// See main.c for disclaimers and other information.
//
// Fredric L. Rice, June 2018
// http://www.crystallake.name
// fred @ crystal lake . name
// 
// ----------------------------------------------------------------------

#ifndef _LOGGINGCLASS_H_
#define _LOGGINGCLASS_H_     1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------
// Other defined constants that we will be using. We attempt to avoid
// hard-coded numbers in the source code.
//
// ----------------------------------------------------------------------

#define ASCII_NULL_ZERO             0x00

// ----------------------------------------------------------------------
// Usually the log file is created in the current directory. This defined
// constant describes the maximum size of the file name which does not
// include the path.
//
// ----------------------------------------------------------------------

#define MAX_LOG_FILE_PATH_NAME      256

// ----------------------------------------------------------------------
// Our class is defined here.
//
// ----------------------------------------------------------------------

class LoggingClass
{
    public:
        LoggingClass( void );
        ~LoggingClass( void );

        void logging_create_log( void );
        void logging_write_log( const char * this_text_p );
        void logging_enable_disable( const bool logging_on_off );

    private:
        FILE * write_log_p;
        char   log_file_name[ MAX_LOG_FILE_PATH_NAME ];
        int    logging_enabled;
} ;

#endif
