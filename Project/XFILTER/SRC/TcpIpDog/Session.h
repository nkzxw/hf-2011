//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//		This module used to manage the ACL memory. First time, 
//		allocate full size of ACL file, and after, dynamic allocate 
//		page when add ACL's record.
//
//
//

#ifndef SESSION_H
#define SESSION_H

extern DWORD GetSessionCount();
extern HANDLE GetSessionFileHandle();

class CSession
{
public:
	CSession();
	virtual ~CSession();

	int	CreateSession(SOCKET s, int nProtocol, LPCTSTR sProcessName);
	int	DeleteSession(SOCKET s);
	int	FindSession(SOCKET s);
	int SetSession(
		DWORD		dwIndex, 
		BYTE		bDirection, 
		WORD		wPort, 
		DWORD		dwRemoteIp,
		DWORD		IsListen = FALSE
	);
	int SetSessionEx(
		DWORD		dwIndex, 
		BYTE		bDirection, 
		TCHAR		*pMemo, 
		int			ByteCount, 
		BOOL		isSend
	);
	int	FinallySession();
	int SendSessionToApp(SESSION *session);
	int SendSessionToAppEx(SESSION *session);
	PSESSION GetSession(DWORD dwIndex);

	void InitializeSessionBuffer();
	BOOL CreateMemFile();
	void CloseMemFile();

private:

	char*		m_pMemFile;
	PSESSION	m_pSession;
};

#endif //#define SESSION_H
