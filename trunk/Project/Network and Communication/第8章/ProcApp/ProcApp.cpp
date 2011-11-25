//////////////////////////////////////////////////////////
// ProcApp.cpp�ļ�


#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "ProcDrv.h"

int main()
{
	// ��ȡ������������ProcDrv.sys������Ŀ¼
	// ע�⣬��Ӧ�ý�ProcDrv���̱��������ProcDrv.sys�ļ����Ƶ���ǰ����Ŀ¼��
	char szDriverPath[256];
	char szLinkName[] = "slNTProcDrv";
	char* p;
	::GetFullPathName("ProcDrv.sys", 256, szDriverPath, &p);


	// ��SCM������
	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSCM == NULL)
	{
		printf(" �򿪷�����ƹ�����ʧ�ܣ���������Ϊ����ӵ��AdministratorȨ��\n");
		return -1;
	}

	// ������򿪷���
	SC_HANDLE hService = ::CreateService(hSCM, szLinkName, szLinkName, SERVICE_ALL_ACCESS, 
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, 
				szDriverPath, NULL, 0, NULL, NULL, NULL);
	if(hService == NULL)
	{
		int nError = ::GetLastError();
		if(nError == ERROR_SERVICE_EXISTS || nError == ERROR_SERVICE_MARKED_FOR_DELETE)
		{
			hService = ::OpenService(hSCM, szLinkName, SERVICE_ALL_ACCESS);
		}
	}
	if(hService == NULL)
	{
		printf(" �����������\n");
		return -1;
	}
	// ��������
	if(!::StartService(hService, 0, NULL))  // �������DriverEntry����
	{
		int nError = ::GetLastError();
		if(nError != ERROR_SERVICE_ALREADY_RUNNING)
		{	
			printf(" �����������%d \n", nError);
			return -1;
		}
	}


	// �򿪵����������������豸�ľ��
	char sz[256] = "";
	wsprintf(sz, "\\\\.\\%s", szLinkName);
	HANDLE hDriver = ::CreateFile(sz, 
		GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hDriver == INVALID_HANDLE_VALUE)
	{	
		printf(" ���豸ʧ�ܣ� \n");
		return -1;
	}

	// ���¼��ں˶��󣬵ȴ�����������¼�֪ͨ
	HANDLE hProcessEvent = ::OpenEvent(SYNCHRONIZE, FALSE, "NTProcDrvProcessEvent");
	CALLBACK_INFO callbackInfo, callbackTemp = { 0 };
	while(::WaitForSingleObject(hProcessEvent, INFINITE) == WAIT_OBJECT_0)
	{
		// �����������Ϳ��ƴ���
		DWORD nBytesReturn;
		BOOL bRet = ::DeviceIoControl(hDriver, IOCTL_NTPROCDRV_GET_PROCINFO, 
									NULL, 0, &callbackInfo, sizeof(callbackInfo), &nBytesReturn, NULL);
		if(bRet)
		{
			if(callbackInfo.hParentId != callbackTemp.hParentId 
						|| callbackInfo.hProcessId != callbackTemp.hProcessId 
							|| callbackInfo.bCreate != callbackTemp.bCreate)
			{
				if(callbackInfo.bCreate)
				{
					printf("	�н��̱�������PID: %d \n", callbackInfo.hProcessId);
				}
				else
				{
					printf("	�н��̱���ֹ��PID: %d \n", callbackInfo.hProcessId);
				}

				callbackTemp = callbackInfo;
			}
		//	break;   
		}
		else
		{
			printf(" ��ȡ������Ϣʧ�ܣ�\n");
			break;
		}
	
	}

	::CloseHandle(hDriver);

	// �ȴ�������ȫֹͣ����
	SERVICE_STATUS ss;
	::ControlService(hService, SERVICE_CONTROL_STOP, &ss);
	// ��SCM���ݿ���ɾ������
	::DeleteService(hService);		
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hSCM);
	return 0;
}