//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//=============================================================================================
//
// 日志操作类
//
//

#include "stdafx.h"
#include "XLogFile.h"

CXLogFile::CXLogFile()
{
	InitializeCriticalSection(&gCriticalSectionLog);

	m_LogFile.m_hFile	= NULL;
	m_bQueryed = FALSE;
}

CXLogFile::~CXLogFile()
{
	CloseLog();
}

//=============================================================================================
// Private log file operator function

//
// 创建日志文件
//
int CXLogFile::CreateLog(const TCHAR *sPathName)
{
	TRY
	{
		m_LogFile.Open(	sPathName,
						CFile::modeCreate	| 
						CFile::modeWrite	| 
						CFile::typeBinary	| 
						CFile::shareExclusive
						);

		_tcscpy(m_LogHeader.Singnature, ACL_HEADER_SIGNATURE);
		m_LogHeader.RecordCount		= 0;
		m_LogHeader.CurrentPosition	= LOG_HEADER_LENTH;

		int		iRet;

		if(iRet = WriteHeader() != XERR_SUCCESS)
			return iRet;

		ODS2(_T("XFILE: Success create the file "),sPathName);

		CloseLog();
	}
	CATCH( CFileException, e )
	{
		ODS2(_T("XFILE: Can't create the file:"), sPathName);

		return XERR_FILE_CREATE_FAILURE;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 保存文件头到日志文件
//
int CXLogFile::WriteHeader()
{
	TRY
	{
		m_LogFile.SeekToBegin();
		m_LogFile.Write(&m_LogHeader, LOG_HEADER_LENTH);
	}
	CATCH( CFileException, e )
	{
		return XERR_FILE_WRITER_HEADER_ERROR;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 打开日志文件
//
int CXLogFile::OpenLog()
{
	if(_taccess(m_sPathName, 0) == -1)	// file not exists & create
	{
		int		iRet;
		if((iRet = CreateLog(m_sPathName)) != XERR_SUCCESS)
			return iRet;
	}

	TRY												// open file
	{
		m_LogFile.Open(	m_sPathName,
						CFile::modeReadWrite   	|
						CFile::typeBinary		| 
						CFile::shareDenyNone   
						);

		m_LogFile.Read(&m_LogHeader, LOG_HEADER_LENTH);

		if(_tcscmp(m_LogHeader.Singnature, ACL_HEADER_SIGNATURE) != 0)
			return XERR_FILE_INVALID_SIGNATURE;

		if(!m_LogFile.GetStatus(m_FileStatus))
			return XERR_FILE_GET_STATUS_ERROR;

		if(m_FileStatus.m_attribute == 0x01)
			return XERR_FILE_READ_ONLY;
	}
	CATCH( CFileException, e )
	{
		return XERR_FILE_CAN_NOT_OPEN;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 关闭日志文件
//
int CXLogFile::CloseLog()
{
	if(m_LogFile.m_hFile == NULL)
		return XERR_SUCCESS;

	m_LogFile.Close();
	m_LogFile.m_hFile = NULL;

	return XERR_SUCCESS;
}

//=============================================================================================
// Public log file operator function

//
// 增加一条日志记录
//
int CXLogFile::AddLog(PPACKET_LOG pLog, long MaxLogSize)
{
	int		iRet;

	if(m_LogFile.m_hFile == NULL)
	{
		if((iRet = OpenLog()) != XERR_SUCCESS)
		{
			CloseLog();
			return iRet;
		}
		DP1("AddLog Success Open Log: %s\n", m_sPathName);
	}

	DWORD PreAddFileSize = MaxLogSize * FILE_SIZE_1M_BYTES - RESULT_ONE_RECORD_LENTH;
	if(m_LogHeader.CurrentPosition > PreAddFileSize)
	{
		m_LogHeader.CurrentPosition = LOG_HEADER_LENTH;

		if(iRet = WriteHeader() != XERR_SUCCESS)
			return iRet;
	}

	TRY												
	{
		m_LogFile.Seek(m_LogHeader.CurrentPosition, CFile::begin);
		m_LogFile.Write(pLog, sizeof(PACKET_LOG));

		ODS("AddLog Success Write");
		m_LogHeader.CurrentPosition = m_LogFile.GetPosition();
		m_LogHeader.RecordCount ++;

		if(iRet = WriteHeader() != XERR_SUCCESS)
			return iRet;
		ODS("AddLog Success WriteHeader");
	}
	CATCH( CFileException, e )
	{
		ODS("AddLog Error");
		return XERR_FILE_ADD_ERROR;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 查找日志记录
//
int CXLogFile::FindLog(LOG_FIND *logfind, long MaxLogSize)
{
	if(m_LogFile.m_hFile == NULL)
	{
		int		iRet;
		if((iRet = OpenLog()) == XERR_FILE_CAN_NOT_OPEN)
		{
			CloseLog();
			return iRet;
		}
		DP1("FindLog Success Open Log: %s\n", m_sPathName);
	}

	m_LogFile.SeekToBegin();
	m_LogFile.Read(&m_LogHeader, LOG_HEADER_LENTH);

	PACKET_LOG	Log;
	DWORD		ulLenth				= m_LogFile.GetLength();
	DWORD		ulMaxLenth			= MaxLogSize * FILE_SIZE_1M_BYTES;
	DWORD		ulCurrentPosition	= 0;
	int			iIsChangeDirection	= 0;
	
	logfind->ulChangeDirectionIndex = 0;
	logfind->ulRecordCount			= 0;
	logfind->ulStartPosition		= 0;

	TRY												
	{
		m_LogFile.Seek(LOG_HEADER_LENTH, CFile::begin);
		ulCurrentPosition = m_LogFile.GetPosition();

		while(ulCurrentPosition < ulLenth && ulCurrentPosition < ulMaxLenth)
		{
			m_LogFile.Read(&Log, sizeof(PACKET_LOG));

			ulCurrentPosition = m_LogFile.GetPosition();
			
			if(Log.tStartTime	>= logfind->tStartTime
				&& Log.tEndTime <= logfind->tEndTime)
			{
				if(logfind->ulRecordCount != 1 
					&& m_LogHeader.CurrentPosition == (ulCurrentPosition - sizeof(PACKET_LOG)))
				{
					iIsChangeDirection				= 2;
					logfind->ulChangeDirectionIndex = logfind->ulRecordCount;
					logfind->ulStartPosition		= ulCurrentPosition - sizeof(PACKET_LOG);
				}

				logfind->ulRecordCount ++;
				
				if(logfind->ulRecordCount == 1 || iIsChangeDirection == 1)
					logfind->ulStartPosition = ulCurrentPosition - sizeof(PACKET_LOG);

				if(iIsChangeDirection == 1)
					iIsChangeDirection = 2;
			}
			else if(logfind->ulRecordCount > 0 && iIsChangeDirection != 1)
			{
				iIsChangeDirection				= 1;
				logfind->ulChangeDirectionIndex = logfind->ulRecordCount;
			}
		}

		logfind->ulChangeDirectionIndex = logfind->ulRecordCount - logfind->ulChangeDirectionIndex;
	}
	CATCH( CFileException, e )
	{
		ODS(_T("XFILE: Find Log Error."));

		return XERR_FILE_RECORD_CAN_NOT_FIND;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//===============================================================================
// 界面相关日志操作函数
//

//
// 初始化界面元素
//
void CXLogFile::InitDlgResource(PGUI_UNIT pGuiUnit, BYTE bQueryType, CString sLogFileName, WORD wLogMaxSize)
{
	m_bQueryType = bQueryType;
	m_GuiUnit = *pGuiUnit;
	m_sPathName	= GetAppPath() + sLogFileName;
	m_wLogMaxSize = wLogMaxSize;

	memset(m_GuiStatus.m_sSpace, ' ', 128);
	m_GuiStatus.m_sSpace[127] = 0;

	SetPageLable();

	CTimeSpan	ts(ACL_MAX_TIME);
	m_GuiUnit.m_pStartDate->SetTime(&(CTime::GetCurrentTime() - ts));

	SetButton();

	switch(m_bQueryType)
	{
	case LOG_TYPE_APP:
		AddListHead(m_GuiUnit.m_pListLog, MONITOR_APP_HEADER, MONITOR_APP_HEADER_COUNT, MONITOR_APP_HEADER_LENTH);
		break;
	case LOG_TYPE_NNB:
		AddListHead(m_GuiUnit.m_pListLog, MONITOR_NNB_HEADER, MONITOR_NNB_HEADER_COUNT, MONITOR_NNB_HEADER_LENTH);
		break;
	case LOG_TYPE_ICMP:
		AddListHead(m_GuiUnit.m_pListLog, MONITOR_ICMP_HEADER, MONITOR_ICMP_HEADER_COUNT, MONITOR_ICMP_HEADER_LENTH);
		break;
	}
}

//
// 查询操作
//
void CXLogFile::OnLogQueryButtonQuery() 
{
	LOG_FIND	find;

	find.tStartTime = GetTime(TRUE);
	find.tEndTime	= GetTime(FALSE);
	SaveTimeStatus();

	if(find.tStartTime > find.tEndTime)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_START_TIME_MIN_END_TIME);
		return;
	}

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if(FindLog(&find, m_wLogMaxSize) != XERR_SUCCESS 
		|| find.ulRecordCount == 0)
	{
		SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
		AfxMessageBox(GUI_ACL_MESSAGE_CAN_NOT_FIND_RECORD);
		return;
	}

	SetButton();
	
	m_ulStartPosition	= find.ulStartPosition;
	m_ulRecordCount		= find.ulRecordCount;
	m_ulChangeDirectionIndex = find.ulChangeDirectionIndex;
	m_iCurrentPage		= 1;
	m_iPageCount		= m_ulRecordCount / LOG_QUERY_PAGE_SIZE;

	if((m_ulRecordCount % LOG_QUERY_PAGE_SIZE) != 0)
		m_iPageCount ++;

	if(m_iPageCount > 1)
		SetButton(FALSE, TRUE);

	ShowRecordPage();

	SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
}

//
// 显示一页日志记录
//
int CXLogFile::ShowRecordPage()
{
	if(m_iCurrentPage <= 0 || m_iCurrentPage > m_iPageCount)
		return XERR_LOG_NO_CAN_SHOW_RECORD;

	DWORD ulShowedCount	= (m_iCurrentPage - 1) * LOG_QUERY_PAGE_SIZE;
	DWORD ulRecordCount	= (m_iCurrentPage != m_iPageCount) ? LOG_QUERY_PAGE_SIZE : 
							  (m_ulRecordCount - ulShowedCount);
	DWORD ulStartPosition = 0;
	if(m_ulChangeDirectionIndex > 0 && ulShowedCount >= m_ulChangeDirectionIndex)
		ulStartPosition	= LOG_HEADER_LENTH + (ulShowedCount - m_ulChangeDirectionIndex) * sizeof(PACKET_LOG);
	else
		ulStartPosition	= m_ulStartPosition + ulShowedCount * sizeof(PACKET_LOG);

	SetPageLable(ulRecordCount);

	PACKET_LOG Log;

	TRY
	{
		m_GuiUnit.m_pListLog->DeleteAllItems();

		m_LogFile.Seek(ulStartPosition, CFile::begin);

		for(DWORD i = 1; i <= ulRecordCount; i++)
		{
			m_LogFile.Read(&Log, sizeof(Log));
			switch(m_bQueryType)
			{
			case LOG_TYPE_APP:
				AddApp(m_GuiUnit.m_pListLog, &Log, -1, FALSE, TRUE);
				break;
			case LOG_TYPE_NNB:
				AddNnb(m_GuiUnit.m_pListLog, &Log, -1, FALSE, TRUE);
				break;
			case LOG_TYPE_ICMP:
				AddIcmp(m_GuiUnit.m_pListLog, &Log, -1, FALSE, TRUE);
				break;
			}

			if(m_ulChangeDirectionIndex > 0 && (ulShowedCount + i) == m_ulChangeDirectionIndex)
				m_LogFile.Seek(LOG_HEADER_LENTH, CFile::begin);
		}

	}
	CATCH( CFileException, e )
	{
		return XERR_LOG_READ_FILE_ERROR;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 得到查询条件的开始/结束时间
//
CTime CXLogFile::GetTime(BOOL IsStart)
{
	CTime t(0), td1, tt1;

	if(IsStart)
	{
		m_GuiUnit.m_pStartDate->GetTime(td1);
		m_GuiUnit.m_pStartTime->GetTime(tt1);
	}
	else
	{
		m_GuiUnit.m_pEndDate->GetTime(td1);
		m_GuiUnit.m_pEndTime->GetTime(tt1);
	}

	CTimeSpan ts = tt1.GetHour() * 3600 + tt1.GetMinute() * 60 + tt1.GetSecond()
		- td1.GetHour() * 3600 - td1.GetMinute() * 60 - td1.GetSecond();

	t = td1 + ts;

	return t;
}

//
// 清除显示列表
//
void CXLogFile::OnLogQueryButtonDelete() 
{
	m_GuiUnit.m_pListLog->DeleteAllItems();	

	m_iPageCount	= 0;
	m_iCurrentPage	= 0;
	m_ulRecordCount = 0;

	SetPageLable();

	SetButton();
}

//
// 上一页
//
void CXLogFile::OnLogQueryButtonBack() 
{
	if(!m_GuiUnit.m_pButtonPrevPage->IsWindowEnabled())
		return;

	SetButton(TRUE, TRUE);

	m_iCurrentPage --;

	if(m_iCurrentPage <= 1)
		SetButton(FALSE, TRUE);

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowRecordPage();
	SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
}

//
// 下一页
//
void CXLogFile::OnLogQueryButtonNext() 
{
	if(!m_GuiUnit.m_pButtonNextPage->IsWindowEnabled())
		return;

	SetButton(TRUE, TRUE);
		
	m_iCurrentPage ++;

	if(m_iCurrentPage >= m_iPageCount)
		SetButton(TRUE, FALSE);

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowRecordPage();
	SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
}

//
// 设置按钮的有效状态
//
void CXLogFile::SetButton(BOOL IsBack, BOOL IsNext)
{
	m_GuiUnit.m_pButtonPrevPage->EnableWindow(IsBack);
	m_GuiUnit.m_pButtonNextPage->EnableWindow(IsNext);
	m_GuiStatus.m_bEnablePrev = IsBack;
	m_GuiStatus.m_bEnableNext = IsNext;
}

//
// 设置状态标签
//
void CXLogFile::SetPageLable(DWORD ulRecordCount)
{
	sprintf(m_GuiStatus.m_sQueryInfo, GUI_LOG_QUERY_RESULT_LIST_LABLE, 
		m_iCurrentPage,
		m_iPageCount, 
		ulRecordCount,
		m_ulRecordCount
		);

	m_GuiUnit.m_pLabelQuery->SetWindowText(m_GuiStatus.m_sSpace);
	m_GuiUnit.m_pLabelQuery->SetWindowText(m_GuiStatus.m_sQueryInfo);
}

//
// 刷新界面显示
//
void CXLogFile::RefreshGuiStatus()
{
	SetButton(m_GuiStatus.m_bEnablePrev, m_GuiStatus.m_bEnableNext);
	m_GuiUnit.m_pLabelQuery->SetWindowText(m_GuiStatus.m_sSpace);
	m_GuiUnit.m_pLabelQuery->SetWindowText(m_GuiStatus.m_sQueryInfo);
	if(m_bQueryed)
	{
		m_GuiUnit.m_pStartDate->SetTime(&m_GuiStatus.m_StartDate);
		m_GuiUnit.m_pStartTime->SetTime(&m_GuiStatus.m_StartTime);
		m_GuiUnit.m_pEndDate->SetTime(&m_GuiStatus.m_EndDate);
		m_GuiUnit.m_pEndTime->SetTime(&m_GuiStatus.m_EndTime);
	}
}

//
// 得到时间
//
void CXLogFile::SaveTimeStatus()
{
	m_bQueryed = TRUE;
	m_GuiUnit.m_pStartDate->GetTime(m_GuiStatus.m_StartDate);
	m_GuiUnit.m_pStartTime->GetTime(m_GuiStatus.m_StartTime);
	m_GuiUnit.m_pEndDate->GetTime(m_GuiStatus.m_EndDate);
	m_GuiUnit.m_pEndTime->GetTime(m_GuiStatus.m_EndTime);
}

#pragma comment( exestr, "B9D3B8FD2A7A6E7169686B6E672B")
