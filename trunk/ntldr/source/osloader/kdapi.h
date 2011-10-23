//********************************************************************
//	created:	11:8:2008   14:31
//	file:		kdapi.h
//	author:		tiamo
//	purpose:	kernel debugger api
//********************************************************************

#pragma once

//
// breakpoint table count
//
#define BREAKPOINT_TABLE_SIZE							32

//
// packet max size
//
#define PACKET_MAX_SIZE									4000

//
// initial packet id
//
#define INITIAL_PACKET_ID								0x80800000

//
// Or in with INITIAL_PACKET_ID to force a packet ID reset.
//
#define SYNC_PACKET_ID									0x00000800

//
// breakIn packet
//
#define BREAKIN_PACKET									0x62626262

//
// breakin packet
//
#define BREAKIN_PACKET_BYTE								0x62

//
// packet lead in sequence
//
#define PACKET_LEADER									0x30303030

//
// leader byte
//
#define PACKET_LEADER_BYTE								0x30

//
// control packet leader
//
#define CONTROL_PACKET_LEADER							0x69696969

//
// control packet leader byte
//
#define CONTROL_PACKET_LEADER_BYTE						0x69

//
// packet trailing byte
//
#define PACKET_TRAILING_BYTE							0xAA

//
// packet types
//
#define PACKET_TYPE_UNUSED								0

//
// state change
//
#define PACKET_TYPE_KD_STATE_CHANGE						1

//
// state manipulate
//
#define PACKET_TYPE_KD_STATE_MANIPULATE					2

//
// debug io
//
#define PACKET_TYPE_KD_DEBUG_IO							3

//
// ack,packet-control type
//
#define PACKET_TYPE_KD_ACKNOWLEDGE						4

//
// resent packet-control type
//
#define PACKET_TYPE_KD_RESEND							5

//
// reset packet-control type
//
#define PACKET_TYPE_KD_RESET							6

//
// state change 64
//
#define PACKET_TYPE_KD_STATE_CHANGE64					7

//
// break in
//
#define PACKET_TYPE_KD_POLL_BREAKIN						8

//
// trace io
//
#define PACKET_TYPE_KD_TRACE_IO							9

//
// control request
//
#define PACKET_TYPE_KD_CONTROL_REQUEST					10

//
// file io
//
#define PACKET_TYPE_KD_FILE_IO							11

//
// max
//
#define PACKET_TYPE_MAX									12

//
// If the packet type is PACKET_TYPE_KD_STATE_CHANGE(64), then the format of the packet data is as follows:
//
#define DbgKdExceptionStateChange						0x00003030L

//
// load symbols state change
//
#define DbgKdLoadSymbolsStateChange						0x00003031L

//
// command string state change
//
#define DbgKdCommandStringStateChange					0x00003032

//
// max instruction bytes
//
#define DBGKD_MAXSTREAM									0x10

//
// control report flags include segments
//
#define REPORT_INCLUDES_SEGS							0x0001

//
// include cs
//
#define REPORT_INCLUDES_CS								0x0002

//
// wait status
//
typedef enum
{
	//
	// error
	//
	ContinueError										= FALSE,

	//
	// success
	//
	ContinueSuccess										= TRUE,

	//
	// reselected
	//
	ContinueProcessorReselected,

	//
	// next processor
	//
	ContinueNextProcessor
}KCONTINUE_STATUS;

//
// packet received
//
#define KDP_PACKET_RECEIVED								0

//
// timeout
//
#define KDP_PACKET_TIMEOUT								1

//
// resend
//
#define KDP_PACKET_RESEND								2

//
// port get sucess
//
#define CP_GET_SUCCESS									0

//
// time out
//
#define CP_GET_NODATA									1

//
// error
//
#define CP_GET_ERROR									2

//
// breakpoint flags in use
//
#define KD_BREAKPOINT_IN_USE							0x00000001

//
// need write
//
#define KD_BREAKPOINT_NEEDS_WRITE						0x00000002

//
// suspened
//
#define KD_BREAKPOINT_SUSPENDED							0x00000004

//
// need replace
//
#define KD_BREAKPOINT_NEEDS_REPLACE						0x00000008

//
// manipulate api number
//
#define DbgKdMinimumManipulate							0x00003130

//
// read virtual memory
//
#define DbgKdReadVirtualMemoryApi						0x00003130

//
// write virtual memory
//
#define DbgKdWriteVirtualMemoryApi						0x00003131

//
// get context
//
#define DbgKdGetContextApi								0x00003132

//
// set context
//
#define DbgKdSetContextApi								0x00003133

//
// write breakpoint
//
#define DbgKdWriteBreakPointApi							0x00003134

//
// restore breakpoint
//
#define DbgKdRestoreBreakPointApi						0x00003135

//
// continue
//
#define DbgKdContinueApi								0x00003136

//
// read control space
//
#define DbgKdReadControlSpaceApi						0x00003137

//
// write control space
//
#define DbgKdWriteControlSpaceApi						0x00003138

//
// read io space
//
#define DbgKdReadIoSpaceApi								0x00003139

//
// write io space
//
#define DbgKdWriteIoSpaceApi							0x0000313A

//
// reboot
//
#define DbgKdRebootApi									0x0000313B

//
// continue2
//
#define DbgKdContinueApi2								0x0000313C

//
// read physical memory
//
#define DbgKdReadPhysicalMemoryApi						0x0000313D

//
// write physical memory
//
#define DbgKdWritePhysicalMemoryApi						0x0000313E

//
// query special call
//
#define DbgKdQuerySpecialCallsApi						0x0000313F

//
// set special call
//
#define DbgKdSetSpecialCallApi							0x00003140

//
// clear special call
//
#define DbgKdClearSpecialCallsApi						0x00003141

//
// set internal bp
//
#define DbgKdSetInternalBreakPointApi					0x00003142

//
// get internal bp
//
#define DbgKdGetInternalBreakPointApi					0x00003143

//
// read io space ex
//
#define DbgKdReadIoSpaceExtendedApi						0x00003144

//
// write io space ex
//
#define DbgKdWriteIoSpaceExtendedApi					0x00003145

//
// get version
//
#define DbgKdGetVersionApi								0x00003146

//
// write breakpoint ex
//
#define DbgKdWriteBreakPointExApi						0x00003147

//
// restore breakpoint ex
//
#define DbgKdRestoreBreakPointExApi						0x00003148

//
// bugcheck system
//
#define DbgKdCauseBugCheckApi							0x00003149

//
// switch processor
//
#define DbgKdSwitchProcessor							0x00003150

//
// page in
//
#define DbgKdPageInApi									0x00003151

//
// read msr
//
#define DbgKdReadMachineSpecificRegister				0x00003152

//
// write msr
//
#define DbgKdWriteMachineSpecificRegister				0x00003153

//
// old Vlm1
//
#define OldVlm1											0x00003154

//
// old vlm2
//
#define OldVlm2											0x00003155

//
// search memory
//
#define DbgKdSearchMemoryApi							0x00003156

//
// get bus data
//
#define DbgKdGetBusDataApi								0x00003157

//
// set bus data
//
#define DbgKdSetBusDataApi								0x00003158

//
// check low memory
//
#define DbgKdCheckLowMemoryApi							0x00003159

//
// clear all interrupt bps
//
#define DbgKdClearAllInternalBreakpointsApi				0x0000315A

//
// fill memory
//
#define DbgKdFillMemoryApi								0x0000315B

//
// query memory
//
#define DbgKdQueryMemoryApi								0x0000315C

//
// switch partition
//
#define DbgKdSwitchPartition							0x0000315D

//
// max
//
#define DbgKdMaximumManipulate							0x0000315E

//
// print string api
//
#define DbgKdPrintStringApi								0x00003230

//
// get string api
//
#define DbgKdGetStringApi								0x00003231

//
// create remote file api
//
#define DbgKdCreateFileApi								0x00003430

//
// read remote file api
//
#define DbgKdReadFileApi								0x00003431

//
// write remote file api
//
#define DbgKdWriteFileApi								0x00003432

//
// close remote file api
//
#define DbgKdCloseFileApi								0x00003433


//
// protocol versions 1
//
#define DBGKD_64BIT_PROTOCOL_VERSION1					5

//
// protocol versions 2
//
#define DBGKD_64BIT_PROTOCOL_VERSION2					6

//
// query virtual memory address spaces
//
#define DBGKD_QUERY_MEMORY_VIRTUAL						0

//
// query process memory address spaces
//
#define DBGKD_QUERY_MEMORY_PROCESS						0

//
// query session memory address spaces
//
#define DBGKD_QUERY_MEMORY_SESSION						1

//
// query kernel memory address spaces
//
#define DBGKD_QUERY_MEMORY_KERNEL						2

//
// query memory flags READ
//
#define DBGKD_QUERY_MEMORY_READ							0x01

//
// query memory flags WRITE
//
#define DBGKD_QUERY_MEMORY_WRITE						0x02

//
// query memory flags EXECUTE
//
#define DBGKD_QUERY_MEMORY_EXECUTE						0x04

//
// query memory flags FIXED
//
#define DBGKD_QUERY_MEMORY_FIXED						0x08

//
// get version flags
// kernel is MP built
//
#define DBGKD_VERS_FLAG_MP								0x0001

//
// DebuggerDataList is valid
//
#define DBGKD_VERS_FLAG_DATA							0x0002

//
// native pointers are 64 bits
//
#define DBGKD_VERS_FLAG_PTR64							0x0004

//
// No MM - don't decode PTEs
//
#define DBGKD_VERS_FLAG_NOMM							0x0008

//
// hardware stepping support
//
#define DBGKD_VERS_FLAG_HSS								0x0010

//
// multiple OS partitions exist
//
#define DBGKD_VERS_FLAG_PARTITIONS						0x0020

//
// KD_PACKETS are the low level data format used in KD.
// all packets begin with a packet leader, byte count, packet type.
// the sequence for accepting a packet is:
//
//  - read 4 bytes to get packet leader.
//	  if read times out (10 seconds) with a short read, or if packet leader is incorrect, then retry the read.
//
//  - next read 2 byte packet type.
//	  if read times out (10 seconds) with a short read, or if packet type is bad, then start again looking for a packet leader.
//
//  - next read 4 byte packet Id.
//	  if read times out (10 seconds) with a short read, or if packet Id is not what we expect, then ask for resend and restart again looking for a packet leader.
//
//  - next read 2 byte byte count.
//	  if read times out (10 seconds) with a short read, or if byte count is greater than PACKET_MAX_SIZE, then start again looking for a packet leader.
//
//  - next read 4 byte packet data checksum.
//
//  - The packet data immediately follows the packet.
//	  there should be ByteCount bytes following the packet header.read the packet data, if read times out (10 seconds) then start again looking for a packet leader.
//
typedef struct _KD_PACKET
{
	//
	// leader
	//
	ULONG												PacketLeader;

	//
	// type
	//
	USHORT												PacketType;

	//
	// byte count
	//
	USHORT												ByteCount;

	//
	// packet id
	//
	ULONG												PacketId;

	//
	// checksum
	//
	ULONG												Checksum;
}KD_PACKET,*PKD_PACKET;

//
// control report
//
typedef struct _DBGKD_CONTROL_REPORT
{
	//
	// kerenl dr6
	//
	ULONG												Dr6;

	//
	// kernel dr7
	//
	ULONG												Dr7;

	//
	// instruction count
	//
	USHORT												InstructionCount;

	//
	// report flags
	//
	USHORT												ReportFlags;

	//
	// instruction stream
	//
	UCHAR												InstructionStream[DBGKD_MAXSTREAM];

	//
	// cs
	//
	USHORT												SegCs;

	//
	// ds
	//
	USHORT												SegDs;

	//
	// es
	//
	USHORT												SegEs;

	//
	// fs
	//
	USHORT												SegFs;

	//
	// eflags
	//
	ULONG												EFlags;

	//
	// pading
	//
	ULONG												Padding[2];
}DBGKD_CONTROL_REPORT,*PDBGKD_CONTROL_REPORT;

//
// exception 64
//
typedef struct _DBGKM_EXCEPTION64
{
	//
	// exception record
	//
	EXCEPTION_RECORD64									ExceptionRecord;

	//
	// first chance
	//
	ULONG												FirstChance;
}DBGKM_EXCEPTION64,*PDBGKM_EXCEPTION64;

//
// symbols info
//
typedef struct _KD_SYMBOLS_INFO
{
	//
	// base of dll
	//
	ULONG												BaseOfDll;

	//
	// process id
	//
	ULONG												ProcessId;

	//
	// checksum
	//
	ULONG												CheckSum;

	//
	// size of image
	//
	ULONG												SizeOfImage;
}KD_SYMBOLS_INFO,*PKD_SYMBOLS_INFO;

//
// load symbols
//
typedef struct _DBGKD_LOAD_SYMBOLS64
{
	//
	// path name length
	//
	ULONG												PathNameLength;

	//
	// base of dll
	//
	ULONG64												BaseOfDll;

	//
	// process id
	//
	ULONG64												ProcessId;

	//
	// checksum
	//
	ULONG												CheckSum;

	//
	// size of image
	//
	ULONG												SizeOfImage;

	//
	// unload
	//
	BOOLEAN												UnloadSymbols;
}DBGKD_LOAD_SYMBOLS64,*PDBGKD_LOAD_SYMBOLS64;

//
// wait state change
//
typedef struct _DBGKD_WAIT_STATE_CHANGE64
{
	//
	// new state
	//
	ULONG												NewState;

	//
	// processor level
	//
	USHORT												ProcessorLevel;

	//
	// processor
	//
	USHORT												Processor;

	//
	// processor count
	//
	ULONG												NumberProcessors;

	//
	// thread
	//
	ULONG64												Thread;

	//
	// eip
	//
	ULONG64												ProgramCounter;

	union
	{
		//
		// exception
		//
		DBGKM_EXCEPTION64								Exception;

		//
		// load symbols
		//
		DBGKD_LOAD_SYMBOLS64							LoadSymbols;
	}u;

	//
	// control report
	//
	DBGKD_CONTROL_REPORT								ControlReport;
}DBGKD_WAIT_STATE_CHANGE64,*PDBGKD_WAIT_STATE_CHANGE64;

//
// control sets for supported architectures
//
typedef struct _X86_DBGKD_CONTROL_SET
{
	//
	// trace flag
	//
	ULONG												TraceFlag;

	//
	// dr7
	//
	ULONG												Dr7;

	//
	// symbol start
	//
	ULONG												CurrentSymbolStart;

	//
	// symbol end
	//
	ULONG												CurrentSymbolEnd;
}DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;

//
// debug print string
//
typedef struct _DBGKD_PRINT_STRING
{
	//
	// length
	//
	ULONG												LengthOfString;
}DBGKD_PRINT_STRING,*PDBGKD_PRINT_STRING;

//
// debug get string
//
typedef struct _DBGKD_GET_STRING
{
	//
	// prompt string length
	//
	ULONG												LengthOfPromptString;

	//
	// read string length
	//
	ULONG												LengthOfStringRead;
}DBGKD_GET_STRING,*PDBGKD_GET_STRING;

//
// debug io
//
typedef struct _DBGKD_DEBUG_IO
{
	//
	// api number
	//
	ULONG												ApiNumber;

	//
	// processor level
	//
	USHORT												ProcessorLevel;

	//
	// processor
	//
	USHORT												Processor;

	//
	// param
	//
	union
	{
		//
		// debug print
		//
		DBGKD_PRINT_STRING								PrintString;

		//
		// rtl assert
		//
		DBGKD_GET_STRING								GetString;
	}u;
}DBGKD_DEBUG_IO,*PDBGKD_DEBUG_IO;

//
// read memory
//
typedef struct _DBGKD_READ_MEMORY64
{
	//
	// address
	//
	ULONG64												TargetBaseAddress;

	//
	// counte
	//
	ULONG												TransferCount;

	//
	// actual count
	//
	ULONG												ActualBytesRead;
}DBGKD_READ_MEMORY64,*PDBGKD_READ_MEMORY64;

//
// write memory
//
typedef struct _DBGKD_WRITE_MEMORY64
{
	//
	// address
	//
	ULONG64												TargetBaseAddress;

	//
	// count
	//
	ULONG												TransferCount;

	//
	// actual count
	//
	ULONG												ActualBytesWritten;
}DBGKD_WRITE_MEMORY64,*PDBGKD_WRITE_MEMORY64;

//
// get context
//
typedef struct _DBGKD_GET_CONTEXT
{
	//
	// dummy
	//
	ULONG												Unused;
}DBGKD_GET_CONTEXT,*PDBGKD_GET_CONTEXT;

//
// set context
//
typedef struct _DBGKD_SET_CONTEXT
{
	//
	// flags
	//
	ULONG												ContextFlags;
}DBGKD_SET_CONTEXT,*PDBGKD_SET_CONTEXT;

//
// write breakpoint
//
typedef struct _DBGKD_WRITE_BREAKPOINT64
{
	//
	// address
	//
	ULONG64												BreakPointAddress;

	//
	// handle
	//
	ULONG												BreakPointHandle;
}DBGKD_WRITE_BREAKPOINT64,*PDBGKD_WRITE_BREAKPOINT64;

//
// restore breakpoint
//
typedef struct _DBGKD_RESTORE_BREAKPOINT
{
	//
	// handle
	//
	ULONG												BreakPointHandle;
}DBGKD_RESTORE_BREAKPOINT,*PDBGKD_RESTORE_BREAKPOINT;

//
// continue
//
typedef struct _DBGKD_CONTINUE
{
	//
	// status
	//
	NTSTATUS											ContinueStatus;
}DBGKD_CONTINUE,*PDBGKD_CONTINUE;

//
// continue2
//
typedef struct _DBGKD_CONTINUE2
{
	//
	// status
	//
	NTSTATUS											ContinueStatus;

	//
	// control set
	//
	union
	{
		//
		// x86 control set
		//
		DBGKD_CONTROL_SET								ControlSet;

		//
		// reserved
		//
		ULONG											Reserved[7];
	};
}DBGKD_CONTINUE2, *PDBGKD_CONTINUE2;

//
// read/write io
//
typedef struct _DBGKD_READ_WRITE_IO64
{
	//
	// address
	//
	ULONG64												IoAddress;

	//
	// size
	//
	ULONG												DataSize;

	//
	// value
	//
	ULONG												DataValue;
} DBGKD_READ_WRITE_IO64, *PDBGKD_READ_WRITE_IO64;

//
// read/write io ex
//
typedef struct _DBGKD_READ_WRITE_IO_EXTENDED64
{
	//
	// size
	//
	ULONG												DataSize;

	//
	// interface type
	//
	ULONG												InterfaceType;

	//
	// bus
	//
	ULONG												BusNumber;

	//
	// address space
	//
	ULONG												AddressSpace;

	//
	// address
	//
	ULONG64												IoAddress;

	//
	// value
	//
	ULONG												DataValue;
}DBGKD_READ_WRITE_IO_EXTENDED64,*PDBGKD_READ_WRITE_IO_EXTENDED64;

//
// read/write msr
//
typedef struct _DBGKD_READ_WRITE_MSR
{
	//
	// index
	//
	ULONG												Msr;

	//
	// data low
	//
	ULONG												DataValueLow;

	//
	// data hi
	//
	ULONG												DataValueHigh;
}DBGKD_READ_WRITE_MSR,*PDBGKD_READ_WRITE_MSR;

//
// query special calls
//
typedef struct _DBGKD_QUERY_SPECIAL_CALLS
{
	//
	// count
	//
	ULONG												NumberOfSpecialCalls;
}DBGKD_QUERY_SPECIAL_CALLS,*PDBGKD_QUERY_SPECIAL_CALLS;

//
// set special call
//
typedef struct _DBGKD_SET_SPECIAL_CALL64
{
	//
	// call
	//
	ULONG64												SpecialCall;
}DBGKD_SET_SPECIAL_CALL64,*PDBGKD_SET_SPECIAL_CALL64;

//
// set internal bp
//
typedef struct _DBGKD_SET_INTERNAL_BREAKPOINT64
{
	//
	// address
	//
	ULONG64												BreakpointAddress;

	//
	// flags
	//
	ULONG												Flags;
}DBGKD_SET_INTERNAL_BREAKPOINT64,*PDBGKD_SET_INTERNAL_BREAKPOINT64;

//
// get internal breakpoint
//
typedef struct _DBGKD_GET_INTERNAL_BREAKPOINT64
{
	//
	// address
	//
	ULONG64												BreakpointAddress;

	//
	// flags
	//
	ULONG												Flags;

	//
	// calls
	//
	ULONG												Calls;

	//
	// max calls per period
	//
	ULONG												MaxCallsPerPeriod;

	//
	// min instructions
	//
	ULONG												MinInstructions;

	//
	// max instructions
	//
	ULONG												MaxInstructions;

	//
	// total instructions
	//
	ULONG												TotalInstructions;
}DBGKD_GET_INTERNAL_BREAKPOINT64,*PDBGKD_GET_INTERNAL_BREAKPOINT64;

//
// get version64
//
typedef struct _DBGKD_GET_VERSION64
{
	//
	// major version
	//
	USHORT												MajorVersion;

	//
	// minor version
	//
	USHORT												MinorVersion;

	//
	// protocol version
	//
	UCHAR												ProtocolVersion;

	//
	// secondary version,cannot be 'A' for compat with dump header
	//
	UCHAR												KdSecondaryVersion;

	//
	// flags
	//
	USHORT												Flags;

	//
	// machine type
	//
	USHORT												MachineType;

	//
	// protocol command support descriptions.these allow the debugger to automatically adapt to different levels of command support in different kernels.
	// one beyond highest packet type understood, zero based.
	//
	UCHAR												MaxPacketType;

	//
	// one beyond highest state change understood, zero based.
	//
	UCHAR												MaxStateChange;

	//
	// one beyond highest state manipulate message understood, zero based.
	//
	UCHAR												MaxManipulate;

	// kind of execution environment the kernel is running in, such as a real machine or a simulator.
	// written back by the simulation if one exists.
	UCHAR												Simulation;

	//
	// pending
	//
	USHORT												Unused[1];

	//
	// base
	//
	ULONG64												KernBase;

	//
	// loaded module list
	//
	ULONG64												PsLoadedModuleList;

	//
	// components may register a debug data block for use by debugger extensions.this is the address of the list head.
	// there will always be an entry for the debugger.
	//
	ULONG64												DebuggerDataList;
}DBGKD_GET_VERSION64,*PDBGKD_GET_VERSION64;

//
// breakpoint ex
//
typedef struct _DBGKD_BREAKPOINTEX
{
	//
	// count
	//
	ULONG												BreakPointCount;

	//
	// continue status
	//
	NTSTATUS											ContinueStatus;
}DBGKD_BREAKPOINTEX,*PDBGKD_BREAKPOINTEX;

//
// search memory
//
typedef struct _DBGKD_SEARCH_MEMORY
{
	//
	// address
	//
	union
	{
		//
		// input
		//
		ULONG64											SearchAddress;

		//
		// output
		//
		ULONG64											FoundAddress;
	};

	//
	// length
	//
	ULONG64												SearchLength;

	//
	// pattern length
	//
	ULONG												PatternLength;
}DBGKD_SEARCH_MEMORY,*PDBGKD_SEARCH_MEMORY;

//
// get set bus data
//
typedef struct _DBGKD_GET_SET_BUS_DATA
{
	//
	// bus data type
	//
	ULONG												BusDataType;

	//
	// bus number
	//
	ULONG												BusNumber;

	//
	// slot number
	//
	ULONG												SlotNumber;

	//
	// offset
	//
	ULONG												Offset;

	//
	// length
	//
	ULONG												Length;
}DBGKD_GET_SET_BUS_DATA,*PDBGKD_GET_SET_BUS_DATA;

//
// fill memory
//
typedef struct _DBGKD_FILL_MEMORY
{
	//
	// address
	//
	ULONG64												Address;

	//
	// length
	//
	ULONG												Length;

	//
	// flags
	//
	USHORT												Flags;

	//
	// pattern length
	//
	USHORT												PatternLength;
}DBGKD_FILL_MEMORY,*PDBGKD_FILL_MEMORY;

//
// query memory
//
typedef struct _DBGKD_QUERY_MEMORY
{
	//
	// address
	//
	ULONG64												Address;

	//
	// reserved
	//
	ULONG64												Reserved;

	//
	// address space
	//
	ULONG												AddressSpace;

	//
	// flags
	//
	ULONG												Flags;
}DBGKD_QUERY_MEMORY,*PDBGKD_QUERY_MEMORY;

//
// switch partition
//
typedef struct _DBGKD_SWITCH_PARTITION
{
	//
	// partition
	//
	ULONG												Partition;
}DBGKD_SWITCH_PARTITION,*PDBGKD_SWITCH_PARTITION;

//
// manipulate state
//
typedef struct _DBGKD_MANIPULATE_STATE64
{
	//
	// api number
	//
	ULONG												ApiNumber;

	//
	// processor level
	//
	USHORT												ProcessorLevel;

	//
	// processor
	//
	USHORT												Processor;

	//
	// return status
	//
	NTSTATUS											ReturnStatus;

	//
	// parameters
	//
	union
	{
		//
		// read memory
		//
		DBGKD_READ_MEMORY64								ReadMemory;

		//
		// write memory
		//
		DBGKD_WRITE_MEMORY64							WriteMemory;

		//
		// get context
		//
		DBGKD_GET_CONTEXT								GetContext;

		//
		// set context
		//
		DBGKD_SET_CONTEXT								SetContext;

		//
		// write break point
		//
		DBGKD_WRITE_BREAKPOINT64						WriteBreakPoint;

		//
		// restore break point
		//
		DBGKD_RESTORE_BREAKPOINT						RestoreBreakPoint;

		//
		// continue
		//
		DBGKD_CONTINUE									Continue;

		//
		// continue2
		//
		DBGKD_CONTINUE2									Continue2;

		//
		// read write io
		//
		DBGKD_READ_WRITE_IO64							ReadWriteIo;

		//
		// read write io extened64
		//
		DBGKD_READ_WRITE_IO_EXTENDED64					ReadWriteIoExtended;

		//
		// query special calls
		//
		DBGKD_QUERY_SPECIAL_CALLS						QuerySpecialCalls;

		//
		// set special call64
		//
		DBGKD_SET_SPECIAL_CALL64						SetSpecialCall;

		//
		// set internal breakpoint64
		//
		DBGKD_SET_INTERNAL_BREAKPOINT64					SetInternalBreakpoint;

		//
		// get internal breakpoint64
		//
		DBGKD_GET_INTERNAL_BREAKPOINT64					GetInternalBreakpoint;

		//
		// get version64
		//
		DBGKD_GET_VERSION64								GetVersion64;

		//
		// breakpoint ex
		//
		DBGKD_BREAKPOINTEX								BreakPointEx;

		//
		// read write msr
		//
		DBGKD_READ_WRITE_MSR							ReadWriteMsr;

		//
		// search memory
		//
		DBGKD_SEARCH_MEMORY								SearchMemory;

		//
		// get set bus data
		//
		DBGKD_GET_SET_BUS_DATA							GetSetBusData;

		//
		// fill memory
		//
		DBGKD_FILL_MEMORY								FillMemory;

		//
		// query memory
		//
		DBGKD_QUERY_MEMORY								QueryMemory;

		//
		// switch partition
		//
		DBGKD_SWITCH_PARTITION							SwitchPartition;
	};
}DBGKD_MANIPULATE_STATE64,*PDBGKD_MANIPULATE_STATE64;

//
// create remote file,unicode filename follows as additional data.
//
typedef struct _DBGKD_CREATE_FILE
{
	//
	// access,.etc,GENERIC_READ
	//
	ULONG												DesiredAccess;

	//
	// attribute
	//
	ULONG												FileAttributes;

	//
	// share access
	//
	ULONG												ShareAccess;

	//
	// create disposition
	//
	ULONG												CreateDisposition;

	//
	// create options
	//
	ULONG												CreateOptions;

	//
	// returned file handle
	//
	ULONG64												Handle;

	//
	// file length
	//
	ULONG64												Length;
}DBGKD_CREATE_FILE,*PDBGKD_CREATE_FILE;

//
// read file
// data is returned as additional data in the response.
//
typedef struct _DBGKD_READ_FILE
{
	//
	// file handle in the create packet
	//
	ULONG64												Handle;

	//
	// offset
	//
	ULONG64												Offset;

	//
	// length
	//
	ULONG												Length;
}DBGKD_READ_FILE,*PDBGKD_READ_FILE;

//
// write file
//
typedef struct _DBGKD_WRITE_FILE
{
	//
	// file handle
	//
	ULONG64												Handle;

	//
	// offset
	//
	ULONG64												Offset;

	//
	// length
	//
	ULONG												Length;
}DBGKD_WRITE_FILE,*PDBGKD_WRITE_FILE;

//
// close file
//
typedef struct _DBGKD_CLOSE_FILE
{
	//
	// handle
	//
	ULONG64												Handle;
}DBGKD_CLOSE_FILE,*PDBGKD_CLOSE_FILE;

//
// remote file io
//
typedef struct _DBGKD_FILE_IO
{
	//
	// api number
	//
	ULONG												ApiNumber;

	//
	// result status
	//
	ULONG												ReturnStatus;

	union
	{
		//
		// make space?
		//
		ULONG64											ReserveSpace[7];

		//
		// create file
		//
		DBGKD_CREATE_FILE								CreateFile;

		//
		// read file
		//
		DBGKD_READ_FILE									ReadFile;

		//
		// write file
		//
		DBGKD_WRITE_FILE								WriteFile;

		//
		// close file
		//
		DBGKD_CLOSE_FILE								CloseFile;
	};
}DBGKD_FILE_IO,*PDBGKD_FILE_IO;


//
// breakpoint entry
//
typedef struct _BREAKPOINT_ENTRY
{
	//
	// flags
	//
	ULONG												Flags;

	//
	// address
	//
	ULONG64												Address;

	//
	// old content
	//
	UCHAR												Content;

}BREAKPOINT_ENTRY,*PBREAKPOINT_ENTRY;

//
// report exception state change
//
VOID BdReportExceptionStateChange(__in PEXCEPTION_RECORD ExceptionRecord,__in PCONTEXT Context);

//
// report load symbols state change
//
VOID BdReportLoadSymbolsStateChange(__in PSTRING Name,__in PKD_SYMBOLS_INFO SymInfo,__in BOOLEAN Unload,__in PCONTEXT Context);

//
// print string
//
BOOLEAN BdPrintString(__in PSTRING String);

//
// prompt string
//
BOOLEAN BdPromptString(__in PSTRING InputString,__in PSTRING OutputString);

//
// pull remote file
//
NTSTATUS BdPullRemoteFile(__in PCHAR FileName,__in ULONG Arg4,__in ULONG Arg8,__in ULONG ArgC,__in ULONG FileId);

//
// poll connection
//
VOID BdPollConnection();

//
// suspend all breakpoints
//
VOID BdSuspendAllBreakpoints();