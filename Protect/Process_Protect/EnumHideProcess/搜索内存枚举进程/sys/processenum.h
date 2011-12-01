#ifndef MYSAFE_PSENUM_DRIVER_H__
#define MYSAFE_PSENUM_DRIVER_H__

#define PROCESS_ENUM_DEVICE       0x8000

// 获取进程列表
#define IOCTL_GETPROCESSPTR	  CTL_CODE(PROCESS_ENUM_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
// METHOD_OUT_DIRECT

typedef struct Processinfo {
	unsigned long pEProcess;
	unsigned long PId;
	char Name[60];
} Processinfo;

#endif