
// ----------------------------------------------------------------------
// ChatClass -- Small class that implements a subnet-wide chat
// function interface via UDP frames transmitted and sent across two
// different port numbers as broadcasted UDP frames.
//
// The class allows for files to be broadcast to all receiving computers 
// (using the send_file() method) and allows for files to be requested 
// from all listening devices (using the get_file() method.)
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
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h> 
#include <time.h>
#include "ChatClass.h"          // Our own class and defined constants

// ----------------------------------------------------------------------
// ChatClass Constructor
//
// Port numbers are set to initial values, and the send and receive 
// socket numbers are set to indicate that the sockets have not yet 
// been created.
//
// The transmit and receive sockets are created, assigned to their UDP
// port numbers, and the receive socket is set to non-blocking.
//
// The transmit socket is set to allow broadcast. Some Linux 
// implimentations require the socket option to be enabled, some do not.
//
// ----------------------------------------------------------------------

ChatClass::ChatClass( const int this_port_number ) : base_port_number( this_port_number ), 
    send_socket( HANDLE_NOT_VALID ), receive_socket( HANDLE_NOT_VALID )
{
    const int running_count    = how_many_are_running( );
    int       transmit_port    = 0;
    int       receive_port     = 0;
    const int enable_broadcast = 1;
    const int enable_reuse     = 1;

    // Initialize class's private data
    (void)memset( (char *)&send_address,    ASCII_NULL_ZERO, sizeof( send_address ) );
    (void)memset( (char *)&receive_address, ASCII_NULL_ZERO, sizeof( receive_address ) );
    (void)memset( udp_inbound_buffer,       ASCII_NULL_ZERO, sizeof( udp_inbound_buffer ) );

    // Do we already have a copy of this program running besides us? 
    if ( running_count > 1 )
    {
        transmit_port = this_port_number;
        receive_port  = this_port_number + 1;
    }
    else
    {
        transmit_port = this_port_number + 1;
        receive_port  = this_port_number;
    }

    // Acquire a send socket
    if ( ( send_socket = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        (void)printf("I was unable to acquire a socket\n");

        exit( ERRORLEVEL_NO_SOCKET );
    }

    // Address the outbound frames as broadcasted to the selected UDP port number
    send_address.sin_family      = AF_INET;
    send_address.sin_addr.s_addr = htonl( ALL_IP_ADDRESSES_BROADCAST );
    send_address.sin_port        = htons( transmit_port );

    // Since all transmitted frames are broadcasted, enable that
    (void)setsockopt( send_socket, SOL_SOCKET, SO_BROADCAST, 
        &enable_broadcast, sizeof( enable_broadcast ) );

    // Acquire an isolated receive socket
    if ( ( receive_socket = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        (void)printf("I was unable to acquire a socket\n");

        exit( ERRORLEVEL_NO_SOCKET );
    }

    // Enable address re-use before we attempt to bind()
    (void)setsockopt( send_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
        &enable_reuse, sizeof( enable_reuse ) );

    receive_address.sin_family      = AF_INET;
    receive_address.sin_port        = htons( receive_port );
    receive_address.sin_addr.s_addr = htonl( INADDR_ANY );
     
    // Bind socket to port  
    if ( bind( receive_socket , (struct sockaddr*)&receive_address, sizeof(receive_address) ) == -1)
    {
        (void)printf("I was unable to bind() the receive socket port %d\n", receive_port );

        exit( ERRORLEVEL_NO_BIND );
    }

    // Make the receive socket non-blocking
    (void)set_non_blocking( receive_socket );
}

// ----------------------------------------------------------------------
// ChatClass Destructor
//
// Closes the transmit and receive sockets if they are assigned
// and closes the log file if it is open.
//
// ----------------------------------------------------------------------

ChatClass::~ChatClass( void )
{
    int  this_index     = 0;
    bool restart_search = true;

    // Make sure that the send socket is closed
    if ( send_socket != HANDLE_NOT_VALID )
    {
        (void)close( send_socket );

        // Flag the socket as closed
        send_socket = HANDLE_NOT_VALID;
    }

    // Make sure that the receive socket is closed
    if ( receive_socket != HANDLE_NOT_VALID )
    {
        (void)close( receive_socket );

        // Flag the socket as closed
        receive_socket = HANDLE_NOT_VALID;
    }

    // Go through any existing file transfer control blocks. If we
    // remove an entry from the vector, search from the beginning again.
    while ( true == restart_search )
    {
        restart_search = false;

        for (this_index = 0; this_index < send_control.size(); this_index++ )
        {
            // Did we some how end up with a file transfer file open?
            if ( (FILE *)NULL != send_control[ this_index ].out_file_p )
            {
                (void)fclose( send_control[ this_index ].out_file_p );

                // Flag the fact that the file is closed
                send_control[ this_index ].out_file_p = (FILE *)NULL;

                // Remove this entry from the vector array
                send_control.erase( send_control.begin() + this_index );

                // Start searching for an entry from the beginning
                restart_search = true;
                break;
            }
        }
    }
}

// ----------------------------------------------------------------------
// ChatClass Send Text
//
// The NULL-terminated string passed to the method by argument is
// sent on the transmit socket.
//
// ----------------------------------------------------------------------

void ChatClass::send_text( char * this_text_p )
{
    // Make sure that the transmit socket is open
    if ( send_socket != HANDLE_NOT_VALID )
    {
        int the_length = strlen( this_text_p );

        // We send a NULL character as well in the UDP frame
        this_text_p[ ++the_length ] = ASCII_NULL_ZERO;

        // Invoke the method which sends the ASCII text as data
        (void)send_data( this_text_p, the_length );
    }
}

// ----------------------------------------------------------------------
// ChatClass Send Data
//
// The data passed to the method by argument is sent out the transmit
// socket with the number of data bytes offered by argument.
//
// Because large blocks of data may be sent, the method checks to see
// if the transmit queue in the Linux Layer 2 and 3 is full, and if it
// is, the method sleeps for 10 milliseconds to allow the outbound
// data to empty before more data is transmitted. In this respect, 
// the method "blocks" until all of the outbound data is spooled out 
// to the UDP Linux outbound driver.
//
// ----------------------------------------------------------------------

void ChatClass::send_data( const void * this_data_p, int this_size )
{
    // Make sure that the transmit socket is open
    // and that there is data to send
    if ( send_socket != HANDLE_NOT_VALID && this_size > 0 )
    {
        int    bytes_sent  = 0;
        char * the_bytes_p = (char *)this_data_p;

        // while there are bytes still left to transmit
        while( this_size > 0 )
        {
            // Attempt to send all of the data that is left to send
            if ( ( bytes_sent = sendto( send_socket, the_bytes_p, this_size, 0, 
                (struct sockaddr *)&send_address, sizeof( send_address ) ) ) < 0) 
            {
                // There was a fatal error with sending the data
                (void)printf("I was unable to send data\n");

                // There can be no more bytes transmitted due to an error
                this_size = 0;
            }
            else
            {
                // See if there are bytes that still need to be sent
                // which were not sent previously. This assums that the
                // number of bytes sent are less than or equal to the
                // number of bytes we requested to be sent; we trust
                // the implementation of Linux.
                the_bytes_p += bytes_sent;
                this_size   -= bytes_sent;

                // Are there more bytes to send?
                if ( this_size > 0 )
                {
                    // Yes, so sleep some milliseconds to allow the data to flow
                    usleep( INTER_WRITE_HOLDOFF_DELAY );
                }
            }
        }
    }
}

// ----------------------------------------------------------------------
// ChatClass Read Data
//
// Data is received on the receive socket and if there is any found, 
// the data is optionally copied to the buffer passed to the method 
// by argument up to the number of bytes passed to the method by 
// argument.
//
// The inbound data frame is checked to see if it is a file transfer
// header block, and if it is, a file download is performed and the
// data is not reported back to the calling function. If we are in
// the middle of a file transfer, the data gets appended to the 
// growing file rather than gets passed to the calling function.
//
// Returns: The number of bytes received, else 0 or -1 if there was
// no data found.
//
// ----------------------------------------------------------------------

int ChatClass::read_data( void )
{
    int  struct_length                = sizeof( receive_address );
    int  read_count                   = 0;
    char from_ip[ SENT_CTRL_IP_SIZE ] = { 0 };

    // Make sure that the receive socket is open
    if ( receive_socket != HANDLE_NOT_VALID )
    {
        // Get a frame up to the size of the receive buffer less 1 byte.
        read_count = recvfrom( receive_socket, 
            &udp_inbound_buffer, sizeof(udp_inbound_buffer), 0, 
	        (struct sockaddr *)&receive_address, (socklen_t *)&struct_length );
    }

    // Did we receive an inbound frame?
    if ( read_count > 0 )
    {
        const char *ip_address_p = inet_ntoa(receive_address.sin_addr);

        // We receive a frame, is it a file transfer start command?
        if ( 0 == strncmp(udp_inbound_buffer, ":xfer:", 6 ) )
        {
            // Receive the first block of the inbound file and
            // mark the fact that we are receiving in to a file
            file_transfer( udp_inbound_buffer, read_count, ip_address_p );

            // The data was processed so report no more data
            read_count = 0;
        } 
        else 
        {
            // See if we are receiving in to a file from that device.
            // If we are, store the data in to the growing receive file
            // and if that was the last block of data, flag the
            // fact that we are no longer receiving in to a file            

            if ( true == receive_file_block( udp_inbound_buffer, read_count, ip_address_p ) )
            {
                // The data was processed so report no more data
                read_count = 0;
            }
        }

        // Store the IP address of the last-heard sender of the data
        // (Currently the IP address is not used.)
        (void)inet_ntop( AF_INET, &receive_address.sin_addr, 
            from_ip, sizeof( from_ip ) );
    }

    // Return the number of bytes read and not used, if any 
    return read_count;
}

// ----------------------------------------------------------------------
// ChatClass Set Non Blocking
//
// The socket passed to the method by argument is set to non-blocking
//
// ----------------------------------------------------------------------

int ChatClass::set_non_blocking( const int this_socket )
{
    // Make sure that the handle is valid
    if ( this_socket == HANDLE_NOT_VALID )
    {
        return false;
    }

    // Get the current settings, mask out blocking
    int flags = fcntl( this_socket, F_GETFL, 0 );

    flags |= O_NONBLOCK;

    // Set the new values and return the status
    return( fcntl( this_socket, F_SETFL, flags ) == 0 ) ? true : false;
}

// ----------------------------------------------------------------------
// ChatClass Set Blocking
//
// The socket passed to the method by argument is set to blocking
//
// ----------------------------------------------------------------------

int ChatClass::set_blocking( const int this_socket )
{
    // Make sure that the handle is valid
    if ( this_socket == HANDLE_NOT_VALID )
    {
        return false;
    }

    // Get the current settings, or in the blocking
    int flags = fcntl( this_socket, F_GETFL, 0 );

    flags &= ~O_NONBLOCK;

    // Set the new values and return the status
    return( fcntl( this_socket, F_SETFL, flags ) == 0 ) ? true : false;
}

// ----------------------------------------------------------------------
// ChatClass How Many Are Running
//
// A ps command gets shelled out which is piped to grep to extract a
// list of all processes which contain the word "chat" after which the
// result is examined line by line to count how many instances it 
// appears that the chat program is running.
//
// The function assumes that the name of the executable remains "chat"
// however argv[0] could conceivably be used to make the function more
// useful for any executable file name passed to it.
//
// Return: The number of instances of chat running on this system
//
// ----------------------------------------------------------------------

int ChatClass::how_many_are_running( void )
{
    FILE * popen_handle                          = popen("ps x | grep chat", "r");
    char   in_record[ MAX_POPEN_RECORD_IN_SIZE ] = { 0 };
    int    entry_count                           = 0;

    // Perform the ps command and pass the result to grep, opening
    // the result text file
    if ( (FILE *)NULL != popen_handle )
    {
        // While there are entries in the text file
        while(! feof( popen_handle ))
        {
            // Read a line from the result file
            (void)fgets(in_record, sizeof( in_record ) - 1, popen_handle );

            // Was that read the end of file?
            if (! feof( popen_handle ))
            {
                // Is it our grep command in the task list?
                if ( (char *)NULL == strstr( in_record, "grep" ) )
                {
                    // No so it's a copy of our program that is running so count it
                    entry_count++;
                }
            }
        }

        // Finished with the result file
        (void)pclose( popen_handle );
    }

    // The number of chat instances gets returned
    return entry_count;
}

// ----------------------------------------------------------------------
// ChatClass Send File
//
// When the operator wants to send a file broadcast to all receivers,
// this method is called. It extracts the path and file name from the
// console input and builds a file transfer header after verifying that
// the file to send exists, then it sends the file transfer header
// block to all receivers.
//
// The data in the file gets extracted and gets transmitted via UDP
// in blocks until all data gets sent. Because the sending method
// checks the Layer 2 and 3 queue, though we may flood the Ethernet
// interface, we should not lose data (keeping in mind that UDP is not
// assured delivery.)
//
// Note that this method also gets invoked if we receive a get request
// from a remote system. A flag indicats which it was, either an
// unsolicited send request from an operator on this system, or a get
// file request from a remote system.
//
// ----------------------------------------------------------------------

void ChatClass::send_file( char * path_and_name_p, const bool response_to_get_request )
{
    char                   outbound_data[ MAX_OUT_DATA_SIZE ] = { 0 };
    int                    out_count                          = 0;
    int                    stat_result                        = 0;
    int                    block_size                         = 0;
    int                    read_size                          = 0;
    char                 * file_name_p                        = (char *)NULL;
    file_transfer_header   file_header;
    struct stat            our_status;

    // Discard leading white space, if any
    skipspace( path_and_name_p );

    // Remove trailing endline characters if any
    while( path_and_name_p[ strlen( path_and_name_p ) - 1] == ASCII_CARRIAGE_RETURN ||
           path_and_name_p[ strlen( path_and_name_p ) - 1] == ASCII_LINE_FEED )
    {
        path_and_name_p[ strlen( path_and_name_p ) - 1 ] = ASCII_NULL_ZERO;
    }

    // See if the file offered exists
    if (0 == ( stat_result = stat( path_and_name_p, &our_status ) ) )
    {
        // The file may exist, we attempt to open for reading in
        // binary mode
        FILE * in_file_p = fopen( path_and_name_p, "rb" );

        if ( (FILE *)NULL != in_file_p )
        {
            // Store the size of the file
            out_count = our_status.st_size;

            // Plug the file transfer header
            (void)memset( (char *)&file_header, ASCII_NULL_ZERO, sizeof ( file_header ) );

            // Note the fact that this is a file transfer frame
            (void)strcpy(file_header.header_command, ":xfer:");

            // Note the number of bytes expected in the file
            file_header.file_size = out_count;

            // Flag the fact that this is a send
            file_header.trans_type = trans_type_send;

            // Do we have any path information?
            file_name_p = strrchr( path_and_name_p, '/' );
 
            if ( (char *)NULL != file_name_p )  
            {
                // There appears to be a delimiter so point past it
                file_name_p++;
            }
            else
            {
                // There does not appear to be path information
                file_name_p = path_and_name_p;
            }

            // Copy the file name in to the header
            (void)memcpy( file_header.file_name, file_name_p, 
                sizeof( file_header.file_name ) - 1 );

            // Send the header data to alert receivers that inbound 
            // data is coming and that it should be assembled in to a file
            send_data( ( char *)&file_header, sizeof( file_header ) );

            (void)printf("Sending %s of %d bytes\n", 
                file_name_p, file_header.file_size );

            // Go through the inbound file and break it up in to smaller
            // pieces, reading blocks and sending them one by one until
            // the entire file has been sent.
            while( out_count > 0 )
            {
                // Compute the size of the block of data to send
                if ( out_count > sizeof( outbound_data ) )
                {
                    block_size = sizeof( outbound_data );
                }
                else
                {
                    block_size = out_count;
                }

                // Read the next block of data to send    
                read_size = fread( outbound_data, 1, block_size, in_file_p );

                // Send the data that was read
                send_data( outbound_data, read_size );
 
                // Deduct the size we asked to be sent from the
                // overall size of the file to be sent
                out_count -= block_size;
            }

            // We are finished with the transfer and the inbound file
            (void)fclose( in_file_p );

            // The file transfer was completed
            return;
        }
    }

    // Was this an unsolicited send file request?
    if ( false == response_to_get_request ) 
    {   
        // It was not a get file request.
        // We did not send a file so report the fact
        (void)printf( "\nFile [%s] was not found\n", path_and_name_p );
    }
}

// ----------------------------------------------------------------------
// ChatClass Receive File Start
//
// After receving the header block for a file transfer, this function
// is called to handle it. The file name and path are extracted from the
// header, and the number of bytes that are expected gets extracted.
//
// The file name is checked to see if it exists in the directory where
// the program was executed, and if the file name exists, a number gets
// appended to the file name. We try a maximum of 20 file names before
// we simply drop the transfer request in which case the data from the
// file may flood in to our console output.
//
// NOTE: We could discard all of that data easily enough by setting the
// send_control.in_file_transfer flag to true. Since the file handle
// is not opened, all of the inbound data would be discarded. However
// we want to see the otherwise discarded data.
//
// ----------------------------------------------------------------------

void ChatClass::receive_file_start( char * this_data_p, int this_byte_size, const char * ip_address_p )
{
    char                 out_file_name[ MAX_OUT_FILE_NAME_SIZE ] = { 0 };
    bool                 have_file_name                          = false;
    int                  name_try_count                          = 0;
    int                  control_index                           = CONTROL_NOT_FOUND;
    file_transfer_header file_header;
    file_sent_control    this_control;

    // See if there is a transfer from this device in progress 
    control_index = find_send_control( ip_address_p );

    // A value not minus 1 means it's in the vector array
    if ( CONTROL_NOT_FOUND != control_index )
    {
        // Are we already receiving a file?
        if ( true == send_control[ control_index ].in_file_transfer && 
           (FILE *)NULL != send_control[ control_index ].out_file_p )
        {
            // This likely means that we were sent an incomplete file
            // and the effort is being retried. Close the file abruptly
            // and flag the fact that we are no longer receiving. We
            // can fail to get a complete file because UDP is not assured
            // delivery.
            (void)fclose( send_control[ control_index ].out_file_p );

            // Flag the fact that the file is no longer open
            send_control[ control_index ].out_file_p = (FILE *)NULL;

            // Flag the fact that we are no longer transfering a file
            send_control[ control_index ].in_file_transfer = false;

            // Note that there are no bytes left to receive
            send_control[ control_index ].to_receive_count = 0;

            (void)printf( "NOTE: Aborted previous file transfer from %s.\n", ip_address_p );
        }

        // Remove the existing entry from the vector array.
        // We only allow one file transfer at a time from a remote device.
        send_control.erase(send_control.begin() + control_index);
    }

    // Copy the starting data in to the header. Since the header is sent
    // as a UDP frame, and because inbound data may come back-to-back,
    // we handle the case where the header has been followed by a block
    // of inbound data (which may or may not happen depending upon the
    // implementation of Linux.)
    (void)memset( (char *)&file_header, ASCII_NULL_ZERO, sizeof( file_header) );

    (void)memcpy( (char *)&file_header, this_data_p, sizeof( file_header) );

    // Does the file we're supposed to receive contain data?
    if ( file_header.file_size == 0 )
    {
        // No, so just ignore the file transfer request
        return;
    }

    // We attempt to create a file name. If the file already exists
    // we change the name by adding a number to the end of the file
    // name, but we only try up to a maximum number of attempts.
    while( false == have_file_name && name_try_count++ < MAX_FILE_OVERWRITE_CHECK ) 
    {
        // Get the name of the file to create from the header
        (void)strcpy( out_file_name, file_header.file_name );

        if ( name_try_count > 0 )
        {
            char to_append[ 10 ] = { 0 };

            (void)sprintf( to_append, "%d", name_try_count );

            (void)strcat( out_file_name, to_append );
        }

        // Does the file already exist?
        this_control.out_file_p = fopen( out_file_name, "rb" );

        if ( (FILE *)NULL == this_control.out_file_p )
        {
            // Flag the fact that this file name does not exist already
            have_file_name = true;
        }
        else
        {
            // The file already exists
            (void)fclose( this_control.out_file_p );

            // Flag the fact that the file is closed
            this_control.out_file_p = (FILE *)NULL;
        }
    }

    // Did we get a good file name?
    if ( true == have_file_name )
    {
        // Flag the time when we started to receive data
        this_control.transfer_start_time = time( NULL );

        // Create the outbound file
        if ( (FILE *)NULL != ( this_control.out_file_p = fopen( out_file_name, "wb" ) ) )
        {
            // Flag the fact that we are receiving a file now
            this_control.in_file_transfer = true;

            // Set the size of data bytes that needs to be received 
            this_control.to_receive_count = file_header.file_size;

            // Store the IP address of the sending device. We
            // use the IP address to track multiple inbound files
            // from different devices. 
            (void)strcpy( this_control.ip_address, ip_address_p );

            (void)printf( "\nInbound file: %s with %d bytes from %s\n", 
                out_file_name, this_control.to_receive_count, ip_address_p );

            // Append the control block to the vector array
            send_control.push_back( this_control );

            // Deduct the size of the header from the inbound data
            this_byte_size -= sizeof( file_header );

            // Do we have any following data to save?
            if ( this_byte_size > 0 )
            {
                // Point to where the follow-up data is, past the header
                this_data_p += sizeof( file_header );

                // Store the follow-up data to the file
                (void)receive_file_block( this_data_p, this_byte_size, ip_address_p );
            }
        }
    }
    else
    {
        // Could not create a file name that would not overwrite
        // an existing file. We discard the inbound frame by doing 
        // nothing. If there are further inbound UDP frames, they
        // will be made available to the calling process every time 
        // the read_data() method is invoked.
        // 
        // This behavior could be changed to discard the inbound file
        // without being displayed to the console by uncommenting
        // the following:
        //
        // this_control.in_file_transfer = true;
        // this_control.to_receive_count = file_header.file_size;
        // (void)strcpy( this_control.ip_address, ip_address_p );
        // (void)printf("Can not save inbound file, file name collission\n");
    }
}

// ----------------------------------------------------------------------
// ChatClass File Transfer
//
// An inbound UDP frame indicates that a file transfer is requested.
// This may be an unsolicited send or a get request. This method copies
// the UDP frame's data in to the header structure to determine which
// type of transfer request it is, dispatching the functionality to the
// correct method to handle it.
//
// ----------------------------------------------------------------------

void ChatClass::file_transfer( char * this_data_p, const int this_byte_size, const char * ip_address_p )
{
    file_transfer_header file_header;

    // Copy the starting data in to the header. 
    (void)memset( (char *)&file_header, ASCII_NULL_ZERO, sizeof( file_header) );

    (void)memcpy( (char *)&file_header, this_data_p, sizeof( file_header) );

    // See if the file transfer is an unsolicited send or a get
    if ( file_header.trans_type == trans_type_send )
    {
        // It is an unsolicited send
        receive_file_start( this_data_p, this_byte_size, ip_address_p );
    }
    else if ( file_header.trans_type == trans_type_get_request )
    {
        // It is a get file request
        get_file_request( this_data_p );
    }
}

// ----------------------------------------------------------------------
// ChatClass Receive File Block
//
// When the send_control.in_file_transfer flag indicates that a file transfer is 
// taking place, and when the file handle shows that the destination 
// file is open, the data passed to this function by argument gets
// written to that file.
//
// NOTE: We could move the check for the open output file to avoid 
// the write() call and allow the data streaming in to us to be 
// discarded solely if the send_control.in_file_transfer flag is true.
//
// ----------------------------------------------------------------------

bool ChatClass::receive_file_block( char * this_data_p, int this_byte_size, const char * ip_address_p )
{
    int       write_result    = 0;
    const int orig_block_size = this_byte_size;
    int       write_try_count = 0;
    int       control_index   = CONTROL_NOT_FOUND;
    bool      are_receiving   = false;

    // See if there is a transfer from this device in progress 
    control_index = find_send_control( ip_address_p );

    // A value of CONTROL_NOT_FOUND means it's not in the vector array
    if ( CONTROL_NOT_FOUND == control_index )
    {
        // Report that we are not getting a file from this device
        return false;
    }

    // Make sure that we are receiving
    if ( true == send_control[ control_index ].in_file_transfer && 
        (FILE *)NULL != send_control[ control_index ].out_file_p )
    {
        // Write the data to the file until it's all written
        // attempting to send the block a maximum of X times
        while( this_byte_size > 0 && write_try_count < MAX_FILE_WRITE_RETRY_COUNT )
        {
            // Attempt to write the whole block
            write_result = fwrite( this_data_p, 1, this_byte_size, 
                send_control[ control_index ].out_file_p );

            // Did we send any data? If we run out of space on
            // the file system, we want to abandon the effort.
            if ( write_result > 0 )
            {
                // Deduct the bytes sent
                this_byte_size -= write_result;
                this_data_p    += write_result;

                // Write was successful so restart the try counter
                write_try_count = 0;
            }
            else
            {
                // Possibly a slow file system or out of space so
                // we delay a second and then incriment our attempt
                // counter to avoid stalling forever.
                sleep(1);

                write_try_count++;
            }
        }

        // Restart the timeout timer
        send_control[ control_index ].transfer_start_time = time( NULL );

        // Flag the fact that we received the data in to a file
        are_receiving = true;

        // Deduct what we expected to have written from the size of
        // the file that needs to be sent
        if ( send_control[ control_index ].to_receive_count >= orig_block_size )
        {
            send_control[ control_index ].to_receive_count -= orig_block_size;
        }

        // Did we get the whole file?
        if ( 0 == send_control[ control_index ].to_receive_count ) 
        {
            // Close the output file
            (void)fclose( send_control[ control_index ].out_file_p );

            // Flag the fact that it is closed
            send_control[ control_index ].out_file_p = (FILE *)NULL;

            // Flag the fact that we are no longer receiving a file
            send_control[ control_index ].in_file_transfer = false;

            // Stop the timer
            send_control[ control_index ].transfer_start_time = 0L;

            // Remove the entry from the vector array
            send_control.erase( send_control.begin() + control_index );
        }
    }

    // Report whether we received this data in to a file or not
    return are_receiving;
}

// ----------------------------------------------------------------------
// ChatClass Transfer Timed Out
//
// Checks to see if a file transfer "send" request has timed out. If the
// transfr timer is running it checks to see if the starting transfer 
// date/time shows that the current time is 10 seconds beyond the last 
// date/time that the timer was updated. Since the timer gets updated 
// every time a UDP block comes in, after 10 seconds of no activity the
// transfer can be assumed to have failed.
//
// The calling of this method is optional. Since UDP is not promised
// delivery, it is a good idea to call this method every now and then
// to see if a file transfer has timed out. If the process using this
// class does not perform file transfers, there is no reason to call
// this method.
//
// ----------------------------------------------------------------------

bool ChatClass::transfer_timed_out( void )
{
    int  this_index     = 0;
    bool any_timeouts   = false;
    bool restart_search = true;

    // Every time we remove a timed-out entry from the vector array
    // we restart the search for another entry that had timed out
    while ( true == restart_search )
    {
        restart_search = false;

        for (this_index = 0; this_index < send_control.size(); this_index++)
        {
            // Is the transfer timer running?
            if ( send_control[ this_index ].transfer_start_time > 0 )
            {
                time_t current_time = time( NULL );

                // The timer is running so see if 10 seconds have passed.
                // Note that this will not work well if the system's date
                // or time gets updated while the transfer is taking place.
                if ( current_time >= send_control[ this_index ].transfer_start_time + 10 )
                {
                    if ( (FILE *)NULL != send_control[ this_index ].out_file_p )
                    {
                        // Close the output file
                        (void)fclose( send_control[ this_index ].out_file_p );

                        // Flag the fact that it is closed
                        send_control[ this_index ].out_file_p = (FILE *)NULL;

                        (void)printf("NOTE: Inbound file transfer timed out.\n");
                    }

                    // Flag the fact that we are no longer receiving a file
                    send_control[ this_index ].in_file_transfer = false;

                    // Stop the timer
                    send_control[ this_index ].transfer_start_time = 0L;

                    // Report that the file transfer timed out
                    any_timeouts = true;

                    // Remove this entry from the vector array
                    send_control.erase( send_control.begin() + this_index );

                    // We removed an entry so start searching for a timeout
                    // from the beginning of the vector array again.
                    restart_search = true;
                    break;
                }
            }
        }
     }

    // Report on whether any file transfers timed out
    return any_timeouts;
}

// ----------------------------------------------------------------------
// ChatClass Get File
//
// The operator is reqesting that all listeners who have the file 
// located in the requested directory with the requested file name send
// it the contents of that file.
//
// This method builds a file transfer header requesting the file and
// sends it as a UDP frame.
//
// ----------------------------------------------------------------------

void ChatClass::get_file( char * path_and_name_p )
{
    file_transfer_header file_header;

    // Discard leading white space, if any
    skipspace( path_and_name_p );

    // Remove trailing endline characters if any
    while( path_and_name_p[ strlen( path_and_name_p ) - 1] == ASCII_CARRIAGE_RETURN ||
           path_and_name_p[ strlen( path_and_name_p ) - 1] == ASCII_LINE_FEED )
    {
        path_and_name_p[ strlen( path_and_name_p ) - 1 ] = ASCII_NULL_ZERO;
    }

    // Plug the file transfer header starting with all zeros
    (void)memset( (char *)&file_header, ASCII_NULL_ZERO, sizeof ( file_header ) );

    // Note the fact that this is a file transfer frame
    (void)strcpy(file_header.header_command, ":xfer:");

    // Note the number of bytes expected in the file is zero so far
    file_header.file_size = 0;

    // Flag the fact that this is a get
    file_header.trans_type = trans_type_get_request;
 
    // Copy the path and file name in to the header
    (void)memcpy( file_header.file_name, path_and_name_p, 
        sizeof( file_header.file_name ) - 1 );

    // Send the header data to request the file
    send_data( ( char *)&file_header, sizeof( file_header ) );

    // Indicate the fact that we requested the file
    (void)printf( "\nFile [%s] was requested\n", path_and_name_p );
}

// ----------------------------------------------------------------------
// ChatClass Get File Request
//
// The path and file name provided in a get file request is examined to 
// see if the file exists on this system, and if it does a response is
// created and the file is sent using the "send" file transfer 
// functionality. 
//
// ----------------------------------------------------------------------

void ChatClass::get_file_request( const char * this_data_p )
{
    file_transfer_header file_header;

    // Plug the file transfer header starting with all zeros
    (void)memset( (char *)&file_header, ASCII_NULL_ZERO, sizeof ( file_header ) );

    // Extract the data in to the data structure
    (void)memcpy( (char *)&file_header, this_data_p, sizeof( file_header ) );

    // We received a get file request. In order to handle it we
    // treat the request as if an operator performed a send file
    // request though we pass the method a flag indicating that 
    // it was a get rather than an unsolicited send.
    send_file( file_header.file_name, true );
}

// ----------------------------------------------------------------------
// ChatClass Find Send Control
//
// Goes through the vector array of send controls looking for an entry
// with an IP address that matches the IP address passed to the method
// by argument, and if it is found, returns the index in to the vector
// array, otherwise if the entry is not found returns -1 to indicate 
// that the entry does not exist.
//
// ----------------------------------------------------------------------

int ChatClass::find_send_control( const char * ip_address_p )
{
    int this_index = 0;

    // Go through all entries in the vector array
    for (this_index = 0; this_index < send_control.size(); this_index++)
    {
        // Is this entry the same as the device's IP address?
        if (0 == strcmp( send_control[ this_index ].ip_address, ip_address_p ) )
        {
            // Report the index of the existing device
            return this_index;
        }
    }

    // Indicate that an entry for the IP address device does not exist
    return CONTROL_NOT_FOUND;
}

