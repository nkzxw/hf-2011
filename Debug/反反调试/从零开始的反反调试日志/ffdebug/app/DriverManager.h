#pragma once

#include <afx.h>

class CDriverManager
{
public:
	CDriverManager(void);
	~CDriverManager(void);
private:
	CString m_strDriverName;
	CString m_strFullPathName ;
public:
	// Æô¶¯Çý¶¯³ÌÐò
	BOOL StartDriver(LPCTSTR lpDrivername);
	BOOL StopDriver(LPCTSTR lpDrivername);
	BOOL CallDriver(
		__in  LPCTSTR lpDrivername,
		__in  DWORD dwIoControlCode,
		__out LPVOID lpOutBuffer,
		__in  DWORD nOutBufferSize,
		__out LPDWORD lpBytesReturned
		) ;
};
