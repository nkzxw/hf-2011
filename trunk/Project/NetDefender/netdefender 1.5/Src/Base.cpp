// Base.cpp: implementation of the CBase class.
//
//////////////////////////////////////////////////////////////////////
#include "Base.h"
#pragma comment(lib, "psapi.lib")

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif
#include "Tlhelp32.h"

//init static members
DWORD CBase::								m_dwLastError = 0;
HMODULE CBase::								m_hModuleTcp = NULL;
HMODULE CBase::								m_hModuleUdp = NULL;
HMODULE CBase::								m_hModuleTH = NULL;
pAllocateAndGetTcpExTableFromStack CBase::	m_pGetTcpTableEx = NULL;
pAllocateAndGetUdpExTableFromStack CBase::	m_pGetUdpTableEx = NULL;
pCreateToolhelp32Snapshot CBase::			m_pToolhelp = NULL;
pProcess32First CBase::						m_pProc32First = NULL;
pProcess32Next CBase::						m_pProc32Next = NULL;
bool CBase::								m_bInit = false;

//////////////////////////////////////////////////////////////////////

// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CBase::CBase()
{
}

/////////////////////////////////////////////////////////////////////////

//
CBase::~CBase()
{
}

//////////////////////////////////////////////////////////////////////

//
DWORD CBase::Initialize(void)
{
	if(m_bInit == true)
	{
		return(0);	//no need to initialize
	}

	//prepare the path
	CString strSysPath = GetSysPath();

	//load "iphlpapi.dll"
	m_hModuleTcp = :: LoadLibrary(strSysPath + _T("\\system32\\iphlpapi.dll"));
	if(m_hModuleTcp == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	m_hModuleUdp = :: LoadLibrary(strSysPath + _T("\\system32\\iphlpapi.dll"));
	if(m_hModuleUdp == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	//point to the magic method e.g TCP
	m_pGetTcpTableEx = (pAllocateAndGetTcpExTableFromStack) GetProcAddress(m_hModuleTcp,
																		  "AllocateAndGetTcpExTableFromStack");
	if(m_pGetTcpTableEx == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	//point to the magic method e.g UDP
	m_pGetUdpTableEx = (pAllocateAndGetUdpExTableFromStack) GetProcAddress(m_hModuleUdp,
																		   "AllocateAndGetUdpExTableFromStack");
	if(m_pGetUdpTableEx == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	//-------------------------------------------------------------------
	//load "psapi functions from kernel32.dll"
	m_hModuleTH = :: LoadLibrary(strSysPath + _T("\\system32\\kernel32.dll"));
	if(m_hModuleTH == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	//get address of imported functions
	m_pToolhelp = (pCreateToolhelp32Snapshot) GetProcAddress(m_hModuleTH, "CreateToolhelp32Snapshot");
	if(m_pToolhelp == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	m_pProc32First = (pProcess32First) GetProcAddress(m_hModuleTH, "Process32First");
	if(m_pProc32First == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	m_pProc32Next = (pProcess32Next) GetProcAddress(m_hModuleTH, "Process32Next");
	if(m_pProc32Next == NULL)
	{
		return(m_dwLastError = :: GetLastError());
	}

	m_bInit = true;

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CBase::Unitialize(void)
{
	if(m_bInit == false)
	{
		return(0);	//no need to freed
	}

	if(FALSE == FreeLibrary(m_hModuleTcp))
	{
		return(m_dwLastError = :: GetLastError());
	}

	if(FALSE == FreeLibrary(m_hModuleUdp))
	{
		return(m_dwLastError = :: GetLastError());
	}

	if(FALSE == FreeLibrary(m_hModuleTH))
	{
		return(m_dwLastError = :: GetLastError());
	}

	m_bInit = false;

	return(0);
}

/////////////////////////////////////////////////////////////////////////

//
CString CBase::GetSysPath(void)
{
	//vars
	TCHAR	buffPath[MAX_PATH];
	memset(buffPath, '0', sizeof(buffPath));

	//get windows path
	//UINT uiPathLen = ::GetSystemWindowsDirectory(buffPath, MAX_PATH+1);
	UINT	uiPathLen = :: GetWindowsDirectory(buffPath, MAX_PATH + 1);
	if(uiPathLen == 0)
	{
		m_dwLastError = :: GetLastError();
		return(_T("error "));
	}

	//prepare the path
	CString strPath = buffPath;
	strPath.TrimRight();
	return(strPath);
}

/////////////////////////////////////////////////////////////////////////

//
int CBase::GetSysVer(void)
{
	OSVERSIONINFO	stOsVer;
	stOsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&stOsVer);
	if(stOsVer.dwMajorVersion < 5)
	{
		return(WIN_NT);
	}
	else
	{
		return(WIN_XX);
	}
}

/////////////////////////////////////////////////////////////////////////

//
CString CBase::GetProcById(DWORD dwPid)
{
	switch(GetSysVer())
	{
		case WIN_XX:
			return(GetProcById4Other(dwPid));

		case WIN_NT:
		default:
			return(_T("Invalid OS"));
	}
}

/////////////////////////////////////////////////////////////////////////

//
CString CBase::GetProcById4Other(DWORD dwPid)
{
	if((m_pToolhelp == NULL) || (m_pProc32First == NULL) || (m_pProc32Next == NULL))
	{
		return(_T("error"));
	}

	//vars
	HANDLE				hProcSnap = NULL;
	CString				strProcName(L"Unknown");
	PROCESSENTRY32_MY	stProc32Entry;
	stProc32Entry.dwSize = sizeof(PROCESSENTRY32_MY);

	//get a snapshoot of the running proc
	hProcSnap = m_pToolhelp(TH32CS_SNAPPROCESS, 0);
	if(hProcSnap == INVALID_HANDLE_VALUE)
	{
		m_dwLastError = :: GetLastError();
		return(_T("error "));
	}

	//gathering info
	if(FALSE == m_pProc32First(hProcSnap, &stProc32Entry))
	{
		m_dwLastError = :: GetLastError();
		return(_T("error "));
	}

	if(dwPid == stProc32Entry.th32ProcessID)
	{
		return(strProcName = stProc32Entry.szExeFile);
	}
	else
	{
		do
		{
			if(stProc32Entry.th32ProcessID == dwPid)
			{
				//free
				CloseHandle(hProcSnap);
				return(strProcName = stProc32Entry.szExeFile);
			}
		} while(m_pProc32Next(hProcSnap, &stProc32Entry));
	}

	return(strProcName);
}

/////////////////////////////////////////////////////////////////////////

//
CString CBase::GetProcPath(DWORD dwPid)
{
	HANDLE			hModuleSnap = NULL;
	MODULEENTRY32	me32 = { 0 };

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
	if(hModuleSnap == INVALID_HANDLE_VALUE)
	{
		return(GetProcPathEx(dwPid));
	}

	// Fill the size of the structure before using it.
	me32.dwSize = sizeof(MODULEENTRY32);

	// Walk the module list of the process, and find the module of
	// interest. Then copy the information to the buffer pointed
	// to by lpMe32 so that it can be returned to the caller.
	if(FALSE == Module32First(hModuleSnap, &me32))
	{
		CloseHandle(hModuleSnap);
		return(_T("error "));	// could not walk module list
	}

	// Do not forget to clean up the snapshot object.
	CloseHandle(hModuleSnap);
	return(me32.szExePath);
}

/////////////////////////////////////////////////////////////////////////

//
CString CBase::GetProcPathEx(DWORD dwPid)
{
	//query the whole path
	HANDLE	hndProc = :: OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPid);

	if(hndProc == NULL)
	{
		return(_T("error "));
	}

	//vars
	TCHAR	buffPath[MAX_PATH];
	memset(buffPath, '0', sizeof(buffPath));

	GetModuleFileNameEx(hndProc,NULL,buffPath,MAX_PATH);

	//:: GetProcessImageFileName(hndProc, buffPath, MAX_PATH);

	:: CloseHandle(hndProc);
	TRACE1("\n%s\n", buffPath);
	return(buffPath);
}
