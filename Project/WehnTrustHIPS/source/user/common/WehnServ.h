#ifndef _WEHNTRUST_COMMON_WEHNSERV_H
#define _WEHNTRUST_COMMON_WEHNSERV_H

////
//
// User<->Service notification interface
//
////

//
// The name of the service.
//
#define WEHNSERV_SVC_NAME        "WehnServ"
//
// The pipe to communicate with.
//
#define WEHNSERV_PIPE_NAME       L"\\\\.\\pipe\\wehnserv"
#define WEHNSERV_PIPE_NAME_ANSI  "\\\\.\\pipe\\wehnserv"
#define WEHNSERV_PIPE_NT_NAME    L"\\Device\\NamedPipe\\wehnserv"
#define WEHNSERV_PIPE_NT_NAME_SZ 52

//
// This structure contains the types of exploits that WehnTrust is capable of
// protecting against with custom features other than ASLR.
//
typedef enum _EXPLOIT_TYPE
{
	StackOverflow,
	SehOverwrite,
	FormatStringBug
} EXPLOIT_TYPE, *PEXPLOIT_TYPE;

//
// This structure contains information that explains what type of exploitation
// attempt was detected and what information lead to its detection.  This
// structure is communicated over a named pipe to the WehnServ service as part
// of a WEHNSERV_REQUEST.
//
typedef struct _EXPLOIT_INFORMATION
{
	//
	// The type of exploitation event that occurred.
	//
	EXPLOIT_TYPE Type;

	union
	{
		//
		// SEH overwrite exploitation information.
		//
		struct
		{
			//
			// The handler of the invalid SEH frame.
			//
			PVOID   InvalidFrameHandler;
			
			//
			// The next pointer of the invalid SEH frame.
			//
			PVOID   InvalidFrameNextPointer;

			//
			// Whether or not the next pointer contains a short jump instruction
			// that is typical of most SEH overwrites.
			//
			BOOLEAN NextContainsShortJumpInstruction;

		} Seh;

		//
		// Stack overflow information.
		//
		struct
		{
			//
			// The value of the frame pointer that was detected as being invalid.
			//
			PVOID InvalidFramePointer;

			//
			// The address that the fault occurred at.
			//
			PVOID FaultAddress;

		} Stack;

		//
		// Format string abuse information.
		//
		struct
		{
			//
			// Whether or not the format string is unicode.
			//
			BOOLEAN UnicodeFormatString;

			//
			// The format string that was supplied (UNICODE or ANSI)
			//
			union
			{
				WCHAR w[128];
				CHAR  a[128];
			} String;

			//
			// The number of format specifiers.
			//
			ULONG NumberOfFormatSpecifiers;

			//
			// The number of arguments pushed.
			//
			ULONG NumberOfArguments;

			//
			// The return address of the symbol.
			//
			PVOID ReturnAddress;

		} FormatString;

	};

} EXPLOIT_INFORMATION, *PEXPLOIT_INFORMATION;

//
// This enumeration contains the various types of packets that can be sent.
//
typedef enum _WEHNSERV_REQUEST_TYPE
{
	WehnServUnknown            = 0,
	WehnServExploitInformation = 1,
	WehnServWaitForEvent       = 10,
} WEHNSERV_REQUEST_TYPE, *PWEHNSERV_REQUEST_TYPE;

//
// This structure is sent across a named pipe to the WehnServ service
// in the form of a request.
//
typedef struct _WEHNSERV_REQUEST
{
	//
	// The type of packet being transmitted.
	//
	WEHNSERV_REQUEST_TYPE Type;

	//
	// The process identifier that the packet is originating from.  This field is
	// not used if the server is able to detect the client's PID.
	//
	ULONG SourceProcessId;

	union
	{
		//
		// Exploitation information.
		//
		EXPLOIT_INFORMATION ExploitInformation;
	};

} WEHNSERV_REQUEST, *PWEHNSERV_REQUEST;

//
// The options of exit methods to use after an exploitation attempts occur.
//
typedef enum _SELECTED_EXIT_METHOD
{
	UseExitThread,
	UseExitProcess
} SELECTED_EXIT_METHOD, *PSELECTED_EXIT_METHOD;

typedef enum _NOTIFY_EVENT_TYPE
{
	WehnServNotifyExploitationEvent = 1,
} NOTIFY_EVENT_TYPE, *PNOTIFY_EVENT_TYPE;

//
// This structure is sent as a response to a WehnServ request packet from the
// WehnServ service.
//
typedef struct _WEHNSERV_RESPONSE
{
	union
	{
		struct
		{
			//
			// The exit method that should be used.
			//
			SELECTED_EXIT_METHOD ExitMethod;

		} ExploitInformation;

		//
		// This element is used in response to WehnServWaitForEvent requests.
		//
		struct
		{
			NOTIFY_EVENT_TYPE Type;

			ULONG ProcessId;

			union
			{
				EXPLOIT_INFORMATION ExploitInformation;
			};

		} Event;
	};

} WEHNSERV_RESPONSE, *PWEHNSERV_RESPONSE;

#endif
