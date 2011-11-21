//=============================================================================================
/*
	XFile.h
	File operate function of ACL file

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/03
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================
#ifndef XFILE_H
#define XFILE_H

#include <afxtempl.h>
#include <io.h>
#include "debug.h"

//
// 2002/12/20 add for v2.1.0
//
#define COMP_ACL_VERSION_START		2
#define COMP_ACL_VERSION_END		2
#define COMP_ACL_MAJOR_START		0
#define COMP_ACL_MAJOR_END			1
//#define COMP_ACL_MINOR_START		0
//#define COMP_ACL_MINOR_END			0

//=============================================================================================
// class of Acl file

#define LAST_COUNT			6

class CAclFile
{
private:
	CRITICAL_SECTION	gCriticalSectionFile;

private:
	BOOL	Create(LPCTSTR sMemFile, DWORD dwMaxSize);
	void	InitDefaultValue();
	int		CreateAcl(const TCHAR *sPathName);
	int		WriteAcl(const TCHAR *sPathName);
	int		WriteRecord(CFile *File, DWORD* pRecord, DWORD dwRecordLen, DWORD* FilePosition);
	int		OpenAcl();
	int		AddRecord(
				PXACL_RECORD pAdd, 
				PXACL_RECORD* ppFirst, 
				DWORD* pCount
				);
	int		UpdateRecord(PXACL_RECORD pUpdate, PXACL_RECORD pFirst, int nRecordLen);
	int		DelRecord(PXACL_RECORD* ppEntry, DWORD dwId, DWORD* pCount);
	void	CloseMemFile();
	void	RepairPoint(PXACL_RECORD* pAclRecord, DWORD dwBaseAddress);
	PVOID	FindRecord(PXACL_RECORD pFirst, DWORD dwId);
	char*	CreateRecord(PVOID pRecord, int nLenth);

	BOOL	StopFilter();
	BOOL	ContinueFilter();
	void	WaitRefence();

public:
	CAclFile();
	virtual ~CAclFile();
	BOOL	UpdateFile(DWORD nPosition, PVOID pBuf, int nLenth);
	void	CloseAcl();
	int		ReadAcl(BOOL IsDLL = FALSE, HINSTANCE instance = NULL);
	int		SaveAcl();
	int		AddAcl(void *pAddAcl, int AclType);
	int		UpdateAcl(void *pAcl, int AclType);
	int		DelAcl(DWORD dwId, int AclType);
	PVOID	FindAcl(DWORD dwId, int AclType);
	CString GetErrorString(int iErrorCode);
	PXACL_HEADER GetHeader(){return m_pAclHeader;}
	BOOL	Lock();
	void	Unlock();
	HANDLE	GetMemoryFileHandle(){return m_hMemFile;}
	void	SetDllHandle(XF_IO_CONTROL pDllIoControl)
			{
				m_pDllIoControl = pDllIoControl;
			}
	BOOL	SetDllAclMemoryHandle(HANDLE hFile);

public:
	static	int		GetBit		(BYTE bit, int index, int count = 1);
	static	int		SetBit		(BYTE* bit, int index, BOOL isTrue);
	static	CString	DIPToSIP	(DWORD* pIP);
	static  CString	GetAppPath	(BOOL IsDLL = FALSE, HINSTANCE instance = NULL,  BOOL IsFullPathName = FALSE);
	static	CString GetPath		(TCHAR *sFilename);
	static	CString GetName		(TCHAR *sFilename);

private:
	CFile			m_AclFile;
	PCHAR			m_pMemFile;
	PXACL_HEADER	m_pAclHeader;
	HANDLE			m_hMemFile;
	PCHAR			m_pCurrentPoint;
	DWORD			m_dwMaxSize;
	DWORD			m_dwMaxOffset;
	CString			m_sPathName;

	XF_IO_CONTROL	m_pDllIoControl;
};


#define		MAX_RECORD_LENTH_BUFFER		1024

typedef struct _ACL_HISTORY
{
	BYTE	bOptType;
	BYTE	bButtonStatus;
	BYTE	pRecord[MAX_RECORD_LENTH_BUFFER];
	BYTE	pRecordOld[MAX_RECORD_LENTH_BUFFER];
	BYTE	bSet;
	BYTE	bQueryEx;
} ACL_HISTORY, *PACL_HISTORY;

#define OPT_TYPE_ADD			1
#define OPT_TYPE_EDIT			2
#define OPT_TYPE_DELETE			3
#define OPT_TYPE_SET			4
#define OPT_TYPE_QUERY_EX		5

#define ERROR_INVALID_OPTION	-1000
#define ERROR_NO_ENOUGH_BUFFER	-1001
#define ERROR_INVALID_RECORD	-1002
#define ERROR_NO_ACL_FILE		-1003

class CAclHistory
{
private:
	void SetSet(int nAclType, BYTE bOptType, BYTE bSet);
	void RemoveArray();
	BOOL IsEof(){return !m_arForward.GetSize();}
	BOOL IsBof(){return !m_arHistory.GetSize();}
	int GetHistoryCount(){return m_arHistory.GetSize();}
	int GetForwardCount(){return m_arForward.GetSize();}
	int GetAclType(){return m_nAclType;}
	int GetAclLenth(int nAclType);

public:
	CAclHistory(int nAclType = ACL_TYPE_ACL, CAclFile *pAclFile = NULL);
	virtual ~CAclHistory();
	void InitHistory(int nAclType, CAclFile *pAclFile)
			{
				m_nAclType = nAclType;
				m_pAclFile = pAclFile;
			}
	int AddHistory(
		IN		BYTE	bOptType, 
		IN		BYTE	bButtonStatus, 
		IN		char	*pRecord = NULL, 
		IN		char	*pRecordOld = NULL,
		IN		BYTE	bSet = 255, 
		IN		BYTE	bQueryEx = 255
		);
	int Apply();
	int Cancel();
	int Back();
	int Forward();
	char* FindAcl(DWORD dwId);

public:
	CArray<ACL_HISTORY, ACL_HISTORY> m_arHistory;
	CArray<ACL_HISTORY, ACL_HISTORY> m_arForward;
	BYTE m_bSet;
	BYTE m_bQueryEx;
private:
	int	m_nAclType;
	int m_nAclLenth;
	ACL_HISTORY m_AclHistory;
	CAclFile *m_pAclFile;
};

#endif