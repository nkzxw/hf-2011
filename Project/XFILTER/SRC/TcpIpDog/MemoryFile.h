//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//
//
//

#ifndef MEMORY_FILE_H
#define MEMORY_FILE_H

extern void SetXfilterProcessId(DWORD dwProcessId, char* sXfilterName);
extern void SetWorkMode(BYTE bWorkMode);
extern void SetMemoryFile(DWORD hMemoryFile);
extern void SetMaxSize(DWORD dwMaxSize);
extern void SetBaseAddress(DWORD dwBaseAddress);
extern void SetRefresh(BYTE bIsRefresh);
extern void SetUpdateVersion(DWORD dwUpdateVersion);
extern void RefenceUpdateVersion();
extern BYTE		GetWorkMode();
extern HANDLE	GetMemoryFile();
extern BYTE		GetRefresh();
extern DWORD	GetUpdateVersion();

class CMemoryFile
{
public:
	CMemoryFile();
	virtual ~CMemoryFile();
	int CheckAcl(SESSION *session);
	//
	// 2002/05/24 add
	//
	BOOL IsXfilter();

	//
	// 2002/08/20 add
	//
	void UnmapUserMemory();

private:
	//
	// 2002/05/24 add
	//
	int CheckWorkMode();

	int CheckApp(SESSION* session);
	int CheckWeb(SESSION* session);
	int CheckTorjan();
	int ProtectApp();

	int FindTime(CTime time, PSESSION session);
	int FindIp(DWORD Ip, PSESSION session);
	BOOL FindIpEx(PXACL_IP pIp, DWORD dwIp);

	int  GetAccessFromSecurity(BYTE bAction);
	int	 GetAccessWithoutAcl(SESSION *session);
	int	 GetAccessFromWorkMode();
	BOOL IsLocalIp(DWORD *ip);
	BOOL IsSuperProcess(LPCSTR sProcessName);
	BOOL Open();
	BOOL Refence();
	void Derefence();
	void SetWindowsVersion();
	BOOL FindString(TCHAR *pString, TCHAR *pSource);
	BOOL CompareString(TCHAR **pString, LPCTSTR pSource, int nCount);
	int  CheckSubWorkMode(BYTE bWorkMode, SESSION *session);
	int  CheckQueryEx(BYTE bQueryEx);
	int	 Query(SESSION* session, BYTE bStatus);

	DWORD Nat(PVOID pAddress);

private:
	PCHAR			m_pMemoryFile;
	PXACL_HEADER	m_pAclHeader;
	DWORD			m_dwUpdateVersion;
	DWORD			m_dwOffset;

	BOOL			m_bIsWin9x;

	//
	// 2002/08/20 add
	//
	HANDLE			m_hFile;
};

#endif //MOMORY_FILE_H
