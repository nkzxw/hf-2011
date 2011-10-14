#ifndef _PROCESS_MONITOR_H_
#define _PROCESS_MONITOR_H_

#define IOCTL_PROCESSMONITOR_GET_PROCINFO    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x081a, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DEVICE_NAME 		L"\\Device\\ProcessMonitor"
#define DEVICE_LINK_NAME 		L"\\DosDevices\\ProcessMonitor"

#define DOS_NAME			"\\\\.\\ProcessMonitor"
#define GLOBAL_DOS_NAME			"\\\\.\\Global\\ProcessMonitor"

#define MONITOR_PROCESS_EVENT  		L"\\BaseNamedObjects\\MonitorProcessEvent"

#endif //_PROCESS_MONITOR_H_