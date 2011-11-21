///
/// @file		comcap_tst.c
/// @author	crazy_chu
/// @date		2008-6-20
/// 

#ifndef _COMCAP_TST_HEADER_
#define _COMCAP_TST_HEADER_

// ��ȡ��ָ����
#define CCPT_CMD_GETPCK	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
// ʹ��
#define CCPT_CMD_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
// ��ֹ
#define CCPT_CMD_DISABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ���ֲ������͡�
#define CCPT_IN	1
#define CCPT_OUT 2

// �������еİ��ṹ
typedef struct CCPT_PCK_ {
	unsigned long com_id;			// com�����к�
	unsigned long opr_type;			// �������ͣ�����д��
	unsigned long pid;					// ��������pid
	unsigned long data_length;		// ���ݳ���		
	unsigned char buf[1];				// ���ݻ���
} CCPT_PCK,*PCCPT_PCK;

#endif // _COMCAP_TST_HEADER_