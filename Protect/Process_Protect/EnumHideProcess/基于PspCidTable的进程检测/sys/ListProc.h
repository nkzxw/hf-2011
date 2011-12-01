#pragma once

//记录进程信息的结构体ProcessInfo
typedef struct PROCESS_PROC     
{
	ULONG addr;         //进程地址（对象（体）指针）           
	int pid;            //进程ID
	UCHAR name[16];     //进程名
	struct PROCESS_PROC *next;  //单向链表指针
} ProcessInfo;    

#define IOCTL_GETPROC_LIST   CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
