/*
 * Fire.cpp - The Fire Web Server
 * Copyright (C) 2007 Tom R. Dial  <tdial@kavaga.com>
 *
 * Notes:
 *  1.  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *      EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *      OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *      NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *      HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *      WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *      FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *      OTHER DEALINGS IN THE SOFTWARE.
 *
 *  2.  Much error handling has been omitted for the sake of clarity.
 *
 *  3.  While the article states that the server avoids resource
 *      contention, there is clearly some contention for the heap.
 *
 *  4.  Make sure to use the multithreaded version(s) of the runtime.
 *
 *  5.  Visual Studio 2005 Users - If you're having trouble building, make
 *      sure that you have the "Use Multi-Byte Character Set" option selected
 *      in the project settings property page.  The default is Unicode.
 *
 *  6.  Please send questions, comments, or bug reports to the e-mail
 *      address above.
 */
#include "stdafx.h"
#include <winsock2.h>
#include <mswsock.h>       
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <shlwapi.h>
#include <string>
#include <sstream>

#pragma warning(disable:4702)   // Release build of vector issues warning.
#include <vector>
#pragma warning(default:4702)

#pragma comment(lib,"ws2_32")   // Standard socket API.
#pragma comment(lib,"mswsock")  // AcceptEx, TransmitFile, etc,.
#pragma comment(lib,"shlwapi")  // UrlUnescape.

// Constants
enum
{
	WORKER_THREAD_COUNT			=	4,
	MAX_CONCURRENT_CONNECTIONS	=	64,
	DEFAULT_PORT                =	8080,
	DEFAULT_LISTEN_QUEUE_SIZE	=	8,
	DEFAULT_READ_BUFFER_SIZE	=   1024,
	MAX_URL_LENGTH				=	512,
	ACCEPT_ADDRESS_LENGTH       =   ((sizeof( struct sockaddr_in) + 16)),

	COMPLETION_KEY_NONE         =   0,
	COMPLETION_KEY_SHUTDOWN     =   1,
	COMPLETION_KEY_IO           =   2
};

// Global event handle; Signaled by the ConsoleCtrlHandler on CTRL-C.
HANDLE StopEvent = 0;

// Stores working directory of executable, used to construct local URI.
char RootDirectory[_MAX_PATH]={0};

// Handler for console control events.
BOOL WINAPI ConsoleCtrlHandler(DWORD Ctrl)
{
    switch ( Ctrl )
    {
    case CTRL_C_EVENT:      // Falls through..
    case CTRL_CLOSE_EVENT:
        SetEvent( StopEvent );
        return TRUE;
    default:
        return FALSE;
    }
}

// Writes the date in the HTTP format to a stream.
std::ostream& operator<<(std::ostream& lhs, const FILETIME& ft)
{
    SYSTEMTIME rhs={0};
    FileTimeToSystemTime( &ft, &rhs );

    const char* Month[] = { "\0", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    const char* Day[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

    char Tmp[100]={0};

    // Format date according to the HTTP specification
    sprintf( Tmp, "Date: %s, %02d %s %04d %02d:%02d:%02d GMT",
        Day[rhs.wDayOfWeek], rhs.wDay, Month[rhs.wMonth], rhs.wYear,
        rhs.wHour, rhs.wMinute, rhs.wSecond );

    return ( lhs << Tmp );
}

// Returns the MIME type for a file.
const char* MimeTypeFromPath(const char* Path)
{
    struct 
    {
        const char* Key;
        const char* Value;
    } 
    Table[] = 
    {
        { "",      "application/x-msdownload" },  // Default mime type.
        { ".html", "text/html" },
        { ".htm",  "text/html" },
        { ".txt",  "text/plain" },
        { ".cpp",  "text/plain" },
        { ".h",    "text/plain" },
        { ".rtf",  "text/richtext" },
        { ".wav",  "audio/wav" },
        { ".gif",  "image/gif" },
        { ".jpg",  "image/jpeg" },
        { ".tif",  "image/tiff" },
        { ".tiff", "image/tiff" },
        { ".png",  "image/png" },
        { ".bmp",  "image/bmp" }
    };
    const size_t TableSize = sizeof(Table) / sizeof(Table[0]);
     
    char Extension[MAX_PATH]={0};
    _splitpath( Path, 0, 0, 0, Extension );

    for (size_t i=1; i<TableSize; i++)
    {
        if ( 0 == _stricmp( Table[i].Key, Extension ) )
        {
            return Table[i].Value;
        }
    }
    return Table[0].Value; // Not found, tell browser to download it.
}

// Returns a pointer to the first occurrence of the byte pattern or NULL.
const unsigned char* memscan(const unsigned char* pBuf, size_t BufLen,
	const unsigned char* pPat, size_t PatLen)
{
	if ( PatLen > BufLen )
	{
		return 0;
	}

	size_t Scans = BufLen - PatLen + 1;
	for (size_t i=0; i<Scans; i++)
	{
		const unsigned char* pb = pBuf + i;
		const unsigned char* pp = pPat;
		size_t j = 0;
		for (j=0; j<PatLen; j++)
		{
			if ( *pb != *pp )
			{
				break;
			}
			pb++;
			pp++;
		}
		if ( PatLen == j )
		{
			return ( pBuf + i );
		}
	}
    return 0;
}

// Class representing a single connection.
class Connection : public OVERLAPPED
{
    Connection(const Connection&);
    Connection& operator=(const Connection&);
    
    enum STATE
    {
        WAIT_ACCEPT   =   0,
        WAIT_REQUEST  =   1,
        WAIT_TRANSMIT =   2,
        WAIT_RESET    =   3,
    };

public:
    Connection(SOCKET Listener, HANDLE IoPort) : myListener(Listener)  
    {
        Internal = 0;
        InternalHigh = 0;
        Offset = 0;
        OffsetHigh = 0;
        hEvent = 0;
        myState = WAIT_ACCEPT;

        ZeroMemory( myAddrBlock, ACCEPT_ADDRESS_LENGTH*2 );
        ZeroMemory( myReadBuf, DEFAULT_READ_BUFFER_SIZE );
        myRequest.reserve( DEFAULT_READ_BUFFER_SIZE );
        myFile = INVALID_HANDLE_VALUE;
        ZeroMemory( &myTransmitBuffers, sizeof(TRANSMIT_FILE_BUFFERS) );

        mySock = WSASocket( PF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 
            WSA_FLAG_OVERLAPPED );

        // Associate the client socket with the I/O Completion Port.
        CreateIoCompletionPort( reinterpret_cast<HANDLE>(mySock), IoPort, 
            COMPLETION_KEY_IO, 0 );

        IssueAccept();
    }

    ~Connection()
    {
        shutdown( mySock, SD_BOTH );
		closesocket( mySock );
    }

    // Issue an asynchronous accept.
    void IssueAccept()
    {   
        myState = WAIT_ACCEPT;
        DWORD ReceiveLen = 0; // This gets thrown away, but must be passed.
        AcceptEx( myListener, mySock, myAddrBlock, 0, ACCEPT_ADDRESS_LENGTH, 
            ACCEPT_ADDRESS_LENGTH, &ReceiveLen, this );
    }

    // Complete the accept and update the client socket's context.
    void CompleteAccept()
    {
        setsockopt( mySock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
            (char*)&myListener, sizeof(SOCKET) );

        // Transition to "reading request" state.
        IssueRead();
    }

    // Issue an asynchronous read operation.
    void IssueRead(void)
    {
        myState = WAIT_REQUEST;
        ReadFile( (HANDLE)mySock, myReadBuf, DEFAULT_READ_BUFFER_SIZE, 
            0, (OVERLAPPED*)this );
    }

    // Complete the read operation, appending the request with the latest data.
    void CompleteRead(size_t NumBytesRead)
    {
        size_t CurSize = myRequest.size();
        myRequest.resize( CurSize + NumBytesRead );
        memcpy( &(myRequest[CurSize]), myReadBuf, NumBytesRead );

        // Has the client finished sending the request?
        if ( IsRequestComplete( NumBytesRead ) )
        {
            // Yes. Transmit the response.
            IssueTransmit();
        }
        else
        {
            // The client is not finished. If data was read this pass, we
            // assume the connection is still good and read more.  If not,
            // we assume that the client closed the socket prematurely.
            if ( NumBytesRead )
            {
                IssueRead();
            }
            else
            {
                IssueReset();
            }
        }
    }

    // Scan the request buffer to see if we've reached the terminator.
    bool IsRequestComplete(size_t NumBytesRead)
    {
        // Valid HTTP requests are supposed to end with \r\n\r\n
		const unsigned char Term[] = { '\r', '\n', '\r', '\n' };

		size_t CurSize = myRequest.size();
		size_t LastSize = CurSize - NumBytesRead;
		size_t Start = ( LastSize > 3 ) ? ( LastSize - 3 ) : 0;

        return ( 0 != memscan( (unsigned char*)&(myRequest[0]) + Start, 
			CurSize - Start, Term, sizeof(Term) ));
    }

    // Parse the request, and transmit the response.
    void IssueTransmit()
    {
        myState = WAIT_TRANSMIT;

        // Simplified parsing of the request: just ignore first token.
        char* Method = strtok( (&myRequest[0]), " ");
        if (!Method)
        {
            IssueReset();
            return;
        }

        // Second token is the resource (which is URL-encoded)
        char* EscapedUri = strtok( 0, " " ); // Yes, strtok is MT safe..
        if (!EscapedUri)
        {
            IssueReset();
            return;
        }

        char UnescapedUri[ MAX_URL_LENGTH ]={0};
        DWORD UnescapedSize = MAX_URL_LENGTH - 1;
        UrlUnescape( EscapedUri, UnescapedUri, &UnescapedSize, 0 );

        char Uri[ MAX_URL_LENGTH ] = {0};
        strcat( Uri, RootDirectory );
        strcat( Uri, UnescapedUri );

        myFile = CreateFile( Uri, GENERIC_READ, FILE_SHARE_READ, 0, 
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0 );

        if ( INVALID_HANDLE_VALUE != myFile )
        {
            BY_HANDLE_FILE_INFORMATION Info={0};
            GetFileInformationByHandle( myFile, &Info );

            // Format response header using a stringstream.
            std::stringstream Response;
   
            FILETIME LastWriteTime={0};
            GetFileTime( myFile, 0, 0, &LastWriteTime );

            Response << "HTTP/1.1 200 OK\r\n"  
                     << LastWriteTime << "\r\n"
                     << "Content-Type: " << MimeTypeFromPath(Uri) << "\r\n"
                     << "Content-Length: " << Info.nFileSizeLow << "\r\n"
                     << "Connection: close\r\n\r\n";

            myResponseHeaders = Response.str();
            myTransmitBuffers.Head = (LPVOID)(myResponseHeaders.c_str()); 
            myTransmitBuffers.HeadLength = (DWORD)myResponseHeaders.size(); 

            TransmitFile( mySock, myFile, Info.nFileSizeLow, 0, this, 
                &myTransmitBuffers, 0 );
        }
        else 
        {
            // Not found.
            myResponseHeaders = "HTTP/1.1 404 Not Found\r\n";
            myTransmitBuffers.Head = (void*)myResponseHeaders.c_str();
            myTransmitBuffers.HeadLength = (DWORD)myResponseHeaders.size();
            TransmitFile( mySock, 0, 0, 0, this, &myTransmitBuffers, 0 );
        }
    }

    void CompleteTransmit()
    {
        // Issue the reset; this prepares the socket for reuse.
        IssueReset();
    }

    void IssueReset()
    {
        myState = WAIT_RESET;
        TransmitFile( mySock, 0, 0, 0, this, 0,
            TF_DISCONNECT | TF_REUSE_SOCKET );
    }

    void CompleteReset(void)
    {
        ClearBuffers();
        IssueAccept();
    }

    void ClearBuffers(void)
    {
        // If a file was being transmitted, release it.
        if ( myFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle( myFile );
            myFile = INVALID_HANDLE_VALUE;
        }

        ZeroMemory( myAddrBlock, ACCEPT_ADDRESS_LENGTH * 2 );
        ZeroMemory( myReadBuf, DEFAULT_READ_BUFFER_SIZE );
        myRequest.clear();
        myResponseHeaders = "";
        ZeroMemory( &myTransmitBuffers, sizeof(TRANSMIT_FILE_BUFFERS) );
    }

    // The main handler for the connection, responsible for state transitions.
    void OnIoComplete(DWORD NumTransferred)
    {
        switch ( myState )
        {
        case WAIT_ACCEPT:
            CompleteAccept();
            break;

        case WAIT_REQUEST:
            CompleteRead( NumTransferred );
            break;

        case WAIT_TRANSMIT:
            CompleteTransmit();
            break;

        case WAIT_RESET:
            CompleteReset();
            break;
        }
    }

private:
    int myState;
    SOCKET mySock;
    SOCKET myListener;
    BYTE myAddrBlock[ ACCEPT_ADDRESS_LENGTH * 2 ];
    char myReadBuf[ DEFAULT_READ_BUFFER_SIZE ];
    std::vector<char> myRequest;
    HANDLE myFile;
    std::string myResponseHeaders;
    TRANSMIT_FILE_BUFFERS myTransmitBuffers;
};


// Worker thread procedure.
unsigned int __stdcall WorkerProc(void* IoPort)
{
    for (;;)
    {
        BOOL Status = 0;
        DWORD NumTransferred = 0;
        ULONG_PTR CompKey = COMPLETION_KEY_NONE;
        LPOVERLAPPED pOver = 0;

        Status = GetQueuedCompletionStatus( reinterpret_cast<HANDLE>(IoPort), 
            &NumTransferred, &CompKey, &pOver,INFINITE );

        Connection* pConn = reinterpret_cast<Connection*>( pOver );

        if ( FALSE == Status )
        {
            // An error occurred; reset to a known state.
            if ( pConn )
            {
                pConn->IssueReset(); 
            }
        }
        else if ( COMPLETION_KEY_IO == CompKey )
        {
             pConn->OnIoComplete( NumTransferred );
        }
        else if ( COMPLETION_KEY_SHUTDOWN == CompKey )
        {
            break;
        }
    }
    return 0;
}

int main(int /*argc*/, char* /*argv*/[])
{
    // Initialize the Microsoft Windows Sockets Library
    WSADATA Wsa={0};
    WSAStartup( MAKEWORD(2,2), &Wsa );

    GetCurrentDirectory( _MAX_PATH, RootDirectory );
    StopEvent = CreateEvent( 0, FALSE, FALSE, 0 );

    // Setup a console control handler: We stop the server on CTRL-C
    SetConsoleCtrlHandler( ConsoleCtrlHandler, TRUE );

	HANDLE IoPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, 0, 
        WORKER_THREAD_COUNT );

	SOCKET Listener = WSASocket( PF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 
        WSA_FLAG_OVERLAPPED );
    struct sockaddr_in Addr={0};
    Addr.sin_family = AF_INET;
    Addr.sin_addr.S_un.S_addr = INADDR_ANY;
    Addr.sin_port = htons( DEFAULT_PORT );

    bind( Listener, (struct sockaddr*)&Addr, sizeof(struct sockaddr_in) );
    listen( Listener, DEFAULT_LISTEN_QUEUE_SIZE );    
    HANDLE Workers[WORKER_THREAD_COUNT]={0};
    unsigned int WorkerIds[WORKER_THREAD_COUNT]={0};

	size_t i;
    for (i=0; i<WORKER_THREAD_COUNT; i++)
    {
        Workers[i] = (HANDLE)_beginthreadex( 0, 0, WorkerProc, IoPort, 0, 
			WorkerIds+i );
    }

    CreateIoCompletionPort( (HANDLE)Listener, IoPort, COMPLETION_KEY_IO, 0 );
    Connection* Connections[MAX_CONCURRENT_CONNECTIONS]={0};
    for (i=0; i<MAX_CONCURRENT_CONNECTIONS; i++)
    {
        Connections[i] = new Connection( Listener, IoPort );
    }

    WaitForSingleObject( StopEvent, INFINITE );

	SetConsoleCtrlHandler( NULL, FALSE );

	for (i=0; i<WORKER_THREAD_COUNT; i++)
    {
        PostQueuedCompletionStatus( IoPort, 0, COMPLETION_KEY_SHUTDOWN, 0 );
    }

    // Wait for all of the worker threads to terminate...
    WaitForMultipleObjects( WORKER_THREAD_COUNT, Workers, TRUE, INFINITE );
    for (i=0; i<WORKER_THREAD_COUNT; i++)
    {
        CloseHandle( Workers[i] );
    }

	CloseHandle( StopEvent );
	shutdown( Listener, SD_BOTH );
	closesocket( Listener );
	CloseHandle( IoPort );

    for (i=0; i<MAX_CONCURRENT_CONNECTIONS; i++)
    {
        delete( Connections[i] );
    }

	WSACleanup();

	return 0;
}

