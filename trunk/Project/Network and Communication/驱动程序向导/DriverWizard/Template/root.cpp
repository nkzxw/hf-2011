//////////////////////////////////////////////////
// $$root$$.cpp�ļ�


extern "C"
{
	#include <ntddk.h>
}


// �����������ʱ����DriverEntry����
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{

	// �����������DriverEntry����ִ�н��
	return STATUS_DEVICE_CONFIGURATION_ERROR;
}
