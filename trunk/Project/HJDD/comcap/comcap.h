///
/// @file		copycat.h
/// @author	crazy_chu
/// @date		2008-6-19
/// 

#ifndef _COMCAP_HEADER_
#define _COMCAP_HEADER_

// ʹ�ܺ�����
void cppSetEnable(BOOLEAN enable);

// ���˵����е�irp�������е�irp
BOOLEAN ccpIrpFilter(PDEVICE_OBJECT device,PIRP irp,NTSTATUS *status);

// ��irp��ʱ����ˡ��������TRUE���ʾ�Ѿ��������irp.
BOOLEAN ccpFileIrpFilter(
	PDEVICE_OBJECT next_dev,
	PIRP irp,
	PIO_STACK_LOCATION irpsp,
	NTSTATUS *status);

// ж�ص�ʱ����á����Խ���󶨡�
void ccpUnload();

// ���������DriverEntry�е��ã����԰����еĴ��ڡ�
void ccpAttachAllComs(PDRIVER_OBJECT driver);

enum {
	CCP_IRP_PASS = 0,
	CCP_IRP_COMPLETED = 1,
	CCP_IRP_GO_ON = 2
};

extern int ccpIrpPreCallback(
	PDEVICE_OBJECT device,
	PDEVICE_OBJECT next_dev,
	PIRP irp,ULONG i,
	PVOID *context);

extern void ccpIrpPostCallback(
	PDEVICE_OBJECT device,
	PDEVICE_OBJECT next_dev,
	PIRP irp,
	PIO_STACK_LOCATION irpsp,
	ULONG i,
	PVOID context);

#endif