// process.h

#ifndef __PROCESS_H__
#define __PROCESS_H__


#define BASE_PDB_PEDB_OFFSET			0x40
#define BASE_EDB_COMMAND_LINE_OFFSET	0x08

#define PS_SUCCESS					0
#define PS_BUFFER_SO_SMALL			-1
#define PS_INVALID_PARAMETER		-2

#define PS_SYSTEM_PROCESS			100
#define PS_USER_PROCESS				101


INT GetProcessFileName(char* buf, DWORD nSize, HANDLE ProcessHandle);
PVOID GetCurrentCommandLine(HANDLE ProcessHandle);


#endif // __PROCESS_H__
