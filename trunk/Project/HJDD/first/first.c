///
/// @file first.c
/// @author crazy_chu
/// @date2008-11-1
/// 

#include <ntddk.h>
	
// �ṩһ��Unload����ֻ��Ϊ��
VOID DriverUnload(PDRIVER_OBJECT driver)
{
	// ����ʵ��������ʲô��������ֻ��ӡһ�仰:
	DbgPrint("first: Our driver is unloading��\r\n");
}

// DriverEntry����ں������൱��main��
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
#if DBG
//       _asm int 3
#endif
	// �������ǵ��ں�ģ�����ڣ�����������д��������д�Ķ�����
	// ���������ӡһ�仰����Ϊ��Hello,world�� ���������ֳ�Ц������
	// ���Ǵ�ӡһ���ġ�
	DbgPrint("first: Hello, my salary!");

	// ����һ��ж�غ�����������������˳���
	driver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}
