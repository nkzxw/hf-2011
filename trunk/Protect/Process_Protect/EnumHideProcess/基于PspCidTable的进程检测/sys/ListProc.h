#pragma once

//��¼������Ϣ�Ľṹ��ProcessInfo
typedef struct PROCESS_PROC     
{
	ULONG addr;         //���̵�ַ�������壩ָ�룩           
	int pid;            //����ID
	UCHAR name[16];     //������
	struct PROCESS_PROC *next;  //��������ָ��
} ProcessInfo;    

#define IOCTL_GETPROC_LIST   CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
