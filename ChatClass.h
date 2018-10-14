
// ----------------------------------------------------------------------
// ChatClass -- small class that implements a subnet-wide chat
// function interface via UDP frames transmitted and sent across two
// different port numbers as broadcasted UDP frames.
//
// See main.c for disclaimers and other information.
//
// Fredric L. Rice, June 2018
// http://www.crystallake.name
// fred @ crystal lake . name
// 
// ----------------------------------------------------------------------

#ifndef _CHATCLASS_H_
#define _CHATCLASS_H_      1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <vector>

// ----------------------------------------------------------------------
// General defined constants that we will be using. We attempt to avoid
// hard-coded numbers in the source code.
//
// ----------------------------------------------------------------------

#define ASCII_NULL_ZERO             0x00
#define ASCII_LINE_FEED             0x0a
#define ASCII_CARRIAGE_RETURN       0x0d

#define ALL_IP_ADDRESSES_BROADCAST  0xFFFFFFFFU
#define INTER_WRITE_HOLDOFF_DELAY   10000
#define SENT_CTRL_IP_SIZE           101
#define MAX_POPEN_RECORD_IN_SIZE    256
#define MAX_OUT_DATA_SIZE           1024
#define MAX_OUT_FILE_NAME_SIZE      256
#define MAX_FILE_WRITE_RETRY_COUNT  20
#define MAX_FILE_OVERWRITE_CHECK    20

// ----------------------------------------------------------------------
// When a handle is not open or otherwise defined, the variable used
// to hold the handle is assigned this value to indicate that it is
// not opened or used.
//
// ----------------------------------------------------------------------

#define HANDLE_NOT_VALID            (int)-1

// ----------------------------------------------------------------------
// ERRORLEVEL exit values are defined here
//
// ----------------------------------------------------------------------

#define ERRORLEVEL_NO_PROBLEM       0
#define ERRORLEVEL_NO_SOCKET        10
#define ERRORLEVEL_NO_BIND          11

// ----------------------------------------------------------------------
// The largest inbound UDP MTU we expect is 1500 bytes so we allocate
// a buffer that is larger than that. This constant defines the size
// of the inbound UDP buffer.
//
// ----------------------------------------------------------------------

#define UDP_IN_BUFFER_SIZE          (1024 * 2)

// ----------------------------------------------------------------------
// MACRO for removing leading white space of spaces and tabs
//
// ----------------------------------------------------------------------

#define skipspace(x) while( *(x) == 0x09 || *(x) == 0x20) { (x)++; }

// ----------------------------------------------------------------------
// When a file is transfered, a header is built indicating details
// about the transfer. The enumerated type here describes what kind of
// file transfer is being performed, an unsolicited send or a get.
//
// Note that we leave the enumeration as a signed integer.
//
// ----------------------------------------------------------------------

    enum transfer_type
    {
        trans_type_send        = 1,         // Result of a send_file()
        trans_type_get_request = 2          // Result of a get_file()
    } ;

// ----------------------------------------------------------------------
// When a file is sent unsolicited to all listening devices, it goes 
// with a header that looks like this. When a file is requested from
// all listening devices, this header is also used.
//
// If the path and file name length exceed the hard-coded length value
// offered here, the transfer will fail. Alter the code to accept a
// large value if you're on systems that have lengthy paths.
//
// ----------------------------------------------------------------------

#define XFER_HDR_CMD_SIZE       11
#define XFER_HDR_NAME_SZIE      101

    typedef struct FILE_TRANSFER_HEADER_T
    {
        char          header_command[ XFER_HDR_CMD_SIZE ];  // Currently always :xfer:
        char          file_name[ XFER_HDR_NAME_SZIE ];      // The path and file name
        int           file_size;                            // The number of bytes to expect
        transfer_type trans_type;                           // The type of transfer
    } file_transfer_header;

// ----------------------------------------------------------------------
// When a file is sent, the file on the receiving side maintains data
// variables to control and monitor the reception of the unsolicited 
// file.
//
// ----------------------------------------------------------------------

    typedef struct FILE_SENT_CONTROL_T
    {
        bool     in_file_transfer;                    // true if a file transfer is happening
        int      to_receive_count;                    // The number of bytes left to receive
        FILE   * out_file_p;                          // The output file being created
        time_t   transfer_start_time;                 // The time stamp of the latest inbound data
        char     ip_address[ SENT_CTRL_IP_SIZE ];     // IP address of remote device
    } file_sent_control;

// ----------------------------------------------------------------------
// If a control frame is not found, this is the index value that gets
// returned by the control search method. Typically the 
// find_send_control() method will return this when an entry is not
// found in the vector array of known devices currently transfering
// files.
//
// ----------------------------------------------------------------------

#define CONTROL_NOT_FOUND       (int)-1

// ----------------------------------------------------------------------
// The Chat Class is described here
//
// ----------------------------------------------------------------------

class ChatClass
{
    // Public methods and data which anybody may invoke
    public:
        ChatClass( const int this_port_number );
        ~ChatClass( void );

        void send_text( char * this_text_p );
        void send_data( const void * this_data_p, int this_size );
        int  read_data( void );
        int  set_non_blocking( const int this_socket );
        int  set_blocking( const int this_socket );
        void send_file ( char * path_and_name_p, const bool response_to_get_request );
        void get_file ( char * path_and_name_p );
        bool transfer_timed_out( void );

        // Make inbound UDP frames reachable by everyone. Typical
        // MTUs for UDP on the Internet are some 512 bytes however
        // fragmentation and re-assembly and such means that we
        // can get somewhere around 1500 byte frames in one gulp
        // on Ethernet and WiFi networks, so we allocate a buffer
        // larger than the maximum we expect.
        char udp_inbound_buffer[ UDP_IN_BUFFER_SIZE ];

    // Private methods and data
    private:
        int  how_many_are_running( void );
        void receive_file_start( char * this_data_p, int this_byte_size, const char * ip_address_p );
        bool receive_file_block( char * this_data_p, int this_byte_size, const char * ip_address_p );
        void file_transfer( char * this_data_p, const int this_byte_size, const char * ip_address_p );
        void get_file_request( const char * this_data_p );
        int  find_send_control( const char * ip_address_p );

        int                           base_port_number;
        int                           send_socket;
        int                           receive_socket;
        struct sockaddr_in            send_address;
        struct sockaddr_in            receive_address;
        std::vector<file_sent_control>send_control;
} ;

#endif
