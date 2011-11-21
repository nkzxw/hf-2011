//=============================================================================================
/*
	XLogFile.h
	File operate function of log file

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/23
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.
*/
//=============================================================================================
// Log file

#ifndef XLOGFILE_H
#define XLOGFILE_H

#define LOG_QUERY_PAGE_SIZE			500

typedef struct _LOG_HEADER
{
	TCHAR	Singnature[16];
	DWORD	RecordCount;
	DWORD	CurrentPosition;
} LOG_HEADER, *PLOG_HEADER;

typedef struct _LOG_FIND
{
	IN	CTime	tStartTime;
	IN	CTime	tEndTime;
	OUT DWORD	ulStartPosition;
	OUT DWORD	ulRecordCount;
	OUT DWORD	ulChangeDirectionIndex;
} LOG_FIND, *PLOG_FIND;

#define LOG_HEADER_LENTH			sizeof(LOG_HEADER)
#define FILE_SIZE_1M_BYTES			1048576
#define RESULT_ONE_RECORD_LENTH		LOG_HEADER_LENTH + SESSION_LENTH

#define LOG_TYPE_APP	0
#define LOG_TYPE_NNB	1
#define LOG_TYPE_ICMP	2

static TCHAR *LOG_FILE_NAME[] = {
	_T("AppLog.dat"),
	_T("NnbLog.dat"),
	_T("IcmpLog.dat"),
};

typedef struct _GUI_UNIT
{
	CDateTimeCtrl	*m_pEndTime;
	CDateTimeCtrl	*m_pEndDate;
	CDateTimeCtrl	*m_pStartTime;
	CDateTimeCtrl	*m_pStartDate;
	CListCtrl		*m_pListLog;
	CStatic			*m_pLabelQuery;
	CButton			*m_pButtonQuery;
	CButton			*m_pButtonClear;
	CButton			*m_pButtonPrevPage;
	CButton			*m_pButtonNextPage;

} GUI_UNIT, *PGUI_UNIT;

typedef struct _GUI_UNIT_STATUS
{
	char			m_sQueryInfo[128];
	char			m_sSpace[128];
	BOOL			m_bEnableQuery;
	BOOL			m_bEnableClear;
	BOOL			m_bEnablePrev;
	BOOL			m_bEnableNext;
	CTime			m_StartDate;
	CTime			m_StartTime;
	CTime			m_EndDate;
	CTime			m_EndTime;

} GUI_UNIT_STATUS, *PGUI_UNIT_STATUS;

class CXLogFile
{
private:
	CRITICAL_SECTION	gCriticalSectionLog;

private:
	int		CreateLog(const TCHAR *sPathName);
	int		CloseLog();
	int		OpenLog();
	int		WriteHeader();

public:
	CXLogFile();
	virtual ~CXLogFile();

	int		AddLog(PPACKET_LOG pLog, long MaxLogSize = 5);
	int		FindLog(LOG_FIND *logfind,  long MaxLogSize = 5);

private:
	LOG_HEADER		m_LogHeader;
	CFileStatus		m_FileStatus;
	CFile			m_LogFile;
	CString			m_sPathName;

public:
	void InitDlgResource(PGUI_UNIT pGuiUnit, BYTE bQueryType, CString sLogFileName, WORD wLogMaxSize);
	void RefreshGuiStatus();
	void SaveTimeStatus();
	void OnLogQueryButtonQuery();
	void OnLogQueryButtonDelete();
	void OnLogQueryButtonBack();
	void OnLogQueryButtonNext();

private:
	CTime	GetTime(BOOL IsStart = TRUE);
	int		ShowRecordPage();
	void	SetButton(BOOL IsBack = FALSE, BOOL IsNext = FALSE);
	void	SetPageLable(DWORD ulRecordCount = 0);

private:
	int		m_iPageCount;
	int		m_iCurrentPage;
	DWORD	m_ulStartPosition;
	DWORD	m_ulRecordCount;
	DWORD	m_ulChangeDirectionIndex;

	BOOL				m_bQueryed;
	WORD				m_wLogMaxSize;
	BYTE				m_bQueryType;
	GUI_UNIT			m_GuiUnit;
	GUI_UNIT_STATUS		m_GuiStatus;
};

#endif