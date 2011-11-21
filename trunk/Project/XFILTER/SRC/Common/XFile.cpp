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
//-----------------------------------------------------------
//
// 控管规则操作类
//
//


#include "stdafx.h"
#include "XFile.h"

//============================================================================================
//Acl file operation
//initialize public function

CAclFile::CAclFile()
{
	m_AclFile.m_hFile 	= NULL;
	m_pMemFile			= NULL;
	m_pAclHeader		= NULL;
	m_hMemFile			= NULL;
	m_pCurrentPoint		= NULL;

	m_sPathName.Empty();

	InitializeCriticalSection(&gCriticalSectionFile);
}

CAclFile::~CAclFile()
{
	XF_SetFilterMode(FALSE);
	WaitRefence();
	CloseAcl();
	CloseMemFile();
}

//
// 等待所有使用控管规则数据的操作退出
//
void CAclFile::WaitRefence()
{
	if(m_pAclHeader == NULL)
		return;
	m_pAclHeader->wPv = PV_LOCKED;
	int TimeOut = 50; // 5 seconds
	while(m_pAclHeader->wRefenceCount > 0 && TimeOut-- > 0)
		Sleep(PV_LOCK_WAIT_TIME);
	m_pAclHeader->wRefenceCount = 0;
	m_pAclHeader->wPv = PV_UNLOCK;
}

//
// 释放内存
//
void CAclFile::CloseMemFile()
{
	if(m_hMemFile != NULL && m_pMemFile != NULL)
	{
		// 2002/08/20 add
		XF_UnmapViewOfFile(m_hMemFile
			, IOCTL_XPACKET_UNMAP_ACL_BUFFER
			, m_pMemFile
			);
		XF_UnmapViewOfFile(m_hMemFile
			, IOCTL_XPACKET_UNMAP_BUFFER_POINT
			, (const void*)TYPE_IOCTL_UNMAP_BUFFER_1
			);
		XF_UnmapViewOfFile(m_hMemFile
			, IOCTL_XPACKET_UNMAP_BUFFER_POINT
			, (const void*)TYPE_IOCTL_UNMAP_BUFFER_2
			);

		XF_UnmapViewOfFile(m_hMemFile
			, IOCTL_XPACKET_FREE_ACL_BUFFER/*2002/08/20 add*/
			, m_pMemFile
			);
		CloseHandle(m_hMemFile);
		m_pMemFile = NULL;
		m_hMemFile = NULL;
	}
	m_pAclHeader = NULL;
	m_pCurrentPoint = NULL;
}

//
// 关闭控管规则文件
//
void CAclFile::CloseAcl()
{
	if(m_AclFile.m_hFile != NULL)
	{
		m_AclFile.Close();
		m_AclFile.m_hFile = NULL;
	}
}

//
// 创建控管规则的内存映射文件
//
BOOL CAclFile::Create(LPCTSTR sMemFile, DWORD dwMaxSize)
{
	StopFilter();
	WaitRefence();
	CloseMemFile();

	m_hMemFile = XF_CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE
		, GetSecurityAttributes()
		, PAGE_READWRITE
		, 0
		, dwMaxSize
		, sMemFile);
	if(m_hMemFile == NULL) return FALSE;

	m_pMemFile = (char*)XF_MapViewOfFile(m_hMemFile, FILE_MAP_WRITE, 0, 0, 0, 1);
	if(m_pMemFile == NULL)
	{
		CloseHandle(m_hMemFile);
		m_hMemFile = NULL;
		return FALSE;
	}
	return TRUE;
}

//
// 锁定内存
//
BOOL CAclFile::Lock()
{
	if(m_pAclHeader == NULL)
		return FALSE;
	while(m_pAclHeader->wPv != PV_UNLOCK)
		Sleep(PV_LOCK_WAIT_TIME);
	if(m_pAclHeader == NULL) return FALSE;
	ODS("Locked\n");
	m_pAclHeader->wPv = PV_LOCKED;
	int TimeOut = 50; // 5 seconds
	while(m_pAclHeader->wRefenceCount > 0 && TimeOut-- > 0)
		Sleep(PV_LOCK_WAIT_TIME);
	m_pAclHeader->wRefenceCount = 0;
	return TRUE;
}

//
// 解除锁定
//
void CAclFile::Unlock()
{
	if(m_pAclHeader == NULL)
		return;
	m_pAclHeader->wPv = PV_UNLOCK;
	ODS("Unlock\n");
}

//
// 通知XFILTER.DLL和XPACKET.SYS或者XPACKET.VXD停止过滤
//
BOOL CAclFile::StopFilter()
{
	if(m_pDllIoControl == NULL) return FALSE;
	XFILTER_IO_CONTROL IoControl;
	IoControl.Byte = TRUE;
	m_pDllIoControl(IO_CONTROL_SET_ACL_IS_REFRESH, &IoControl);
	XF_SetFilterMode(FALSE);
	return TRUE;
}

//
// 通知XFILTER.DLL和XPACKET.SYS或者XPACKET.VXD继续过滤
//
BOOL CAclFile::ContinueFilter()
{
	if(m_pDllIoControl == NULL) return FALSE;
	XFILTER_IO_CONTROL IoControl;
	IoControl.Byte = FALSE;
	m_pDllIoControl(IO_CONTROL_SET_ACL_IS_REFRESH, &IoControl);
	XF_SetFilterMode(TRUE);
	return TRUE;
}

//
// 通知XFILTER.DLL控管规则内存映射文件的句柄
//
BOOL CAclFile::SetDllAclMemoryHandle(HANDLE hFile)
{
	if(m_pDllIoControl == NULL) return FALSE;
	XFILTER_IO_CONTROL IoControl;
	IoControl.DWord = (DWORD)hFile;
	IoControl.Pointer = (BYTE*)m_dwMaxSize;
	IoControl.DWord2 = (DWORD)m_pAclHeader;
	m_pDllIoControl(IO_CONTROL_SET_ACL_MEMORY_FILE_HANDLE, &IoControl);
	return TRUE;
}


//============================================================================================
//private function

//
// 初始化 Acl Header
//
void CAclFile::InitDefaultValue()
{
	if(m_pMemFile != NULL) delete[] m_pMemFile;
	DWORD dwFileLen = ACL_HEADER_LENTH + ACL_TIME_TOTAL_LENTH + ACL_IP_LENTH * 2;
	m_pMemFile = new CHAR[dwFileLen];
	memset(m_pMemFile, 0, dwFileLen);

	m_pAclHeader = (PXACL_HEADER)m_pMemFile;

	// Initalize header default value
	_tcscpy(m_pAclHeader->sSignature, ACL_HEADER_SIGNATURE);
	m_pAclHeader->bMajor	= ACL_HEADER_MAJOR;
	m_pAclHeader->bMinor	= ACL_HEADER_MINOR;
	m_pAclHeader->uiSerial	= ACL_HEADER_SERIAL;
	m_pAclHeader->ulVersion	= ACL_HEADER_VERSION;
	m_pAclHeader->uiLogSize	= ACL_HEADER_LOG_SIZE;
	m_pAclHeader->bUpdateInterval = ACL_HEADER_UPDATE_INTERVAL;
	_tcscpy(m_pAclHeader->sUserName, ACL_HEADER_USER_NAME);
	_tcscpy(m_pAclHeader->sACode, ACL_HEADER_ACODE);
	_tcscpy(m_pAclHeader->sWebURL, ACL_HEADER_WEB_URL);
	_stprintf(m_pAclHeader->sCommandURL, ACL_HEADER_COMMAND_URL
		, GUI_LANGUAGE, ACL_HEADER_VERSION, ACL_HEADER_MAJOR, ACL_HEADER_MINOR);
	_tcscpy(m_pAclHeader->sUserRegisterURL, ACL_HEADER_REGISTER_URL);
	_tcscpy(m_pAclHeader->sEmail, ACL_HEADER_EMAIL);
	m_pAclHeader->ulHeaderLenth	= ACL_HEADER_LENTH;

	m_pAclHeader->bWriteLog = ACL_WRITE_LOG;
	m_pAclHeader->bAutoStart = ACL_AUTO_START;
	m_pAclHeader->bAudioAlert = ACL_AUDIO_ALERT;
	m_pAclHeader->bSplashAlert = ACL_SPLASH_ALERT;
	m_pAclHeader->bShowWelcome = ACL_SHOW_WELCOME;
	m_pAclHeader->bCheckTorjan = ACL_CHECK_TORJAN;
	m_pAclHeader->bCheckFile = ACL_CHECK_FILE;

	m_pAclHeader->bAppSet = ACL_APP_SET;
	m_pAclHeader->bAppQueryEx = ACL_QUERY_QUERY;
	m_pAclHeader->bWebSet = ACL_WEB_SET;
	m_pAclHeader->bWebQueryEx = ACL_QUERY_QUERY;
	m_pAclHeader->bNnbSet = ACL_NNB_SET;
	m_pAclHeader->bNnbQueryEx = ACL_QUERY_QUERY;
	m_pAclHeader->bIcmpSet = ACL_ICMP_SET;
	m_pAclHeader->bIcmpQueryEx = ACL_QUERY_QUERY;
	m_pAclHeader->bSecurity = ACL_SECURITY;
	m_pAclHeader->bWorkMode = ACL_WORK_MODE;

	//
	// Initalize intranet IP aria default value
	//
	m_pAclHeader->pAllIp 
		= (PXACL_IP)(ACL_HEADER_LENTH + ACL_TIME_TOTAL_LENTH + (DWORD)m_pMemFile);
	m_pAclHeader->dwAllIpCount	= 1;
	m_pAclHeader->pAllIp->dwId	= 1;
	m_pAclHeader->pAllIp->bNotAllowEdit = 1;

	m_pAclHeader->pIntranetIp 
		= (PXACL_IP)(ACL_HEADER_LENTH  + ACL_TIME_TOTAL_LENTH + ACL_IP_LENTH + (DWORD)m_pMemFile);
	m_pAclHeader->ulIntranetIPCount	= ACL_HEADER_INTRANET_IP_COUNT;

	m_pAclHeader->pIntranetIp->dwId		= 1;
	m_pAclHeader->pIntranetIp->ulStartIP= ACL_INTRANET_START_IP;
	m_pAclHeader->pIntranetIp->ulEndIP	= ACL_INTRANET_END_IP;
	m_pAclHeader->pIntranetIp->pNext	= NULL;

	//
	// Initalize time aria default value
	//
	m_pAclHeader->pTime	= (PXACL_TIME)(ACL_HEADER_LENTH + (DWORD)m_pMemFile);
	m_pAclHeader->ulTimeCount = ACL_HEADER_TIME_COUNT;
	for(DWORD i = 0; i < m_pAclHeader->ulTimeCount; i++)
	{
		(m_pAclHeader->pTime + i)->dwId = (DWORD)(i + 1);
		(m_pAclHeader->pTime + i)->bWeekDay	= ACL_WEEK[i];
		(m_pAclHeader->pTime + i)->tStartTime = ACL_TIME_START[i];
		(m_pAclHeader->pTime + i)->tEndTime	= ACL_TIME_END[i];

		if(i == m_pAclHeader->ulTimeCount - 1)
			(m_pAclHeader->pTime + i)->pNext = NULL;
		else
			(m_pAclHeader->pTime + i)->pNext = m_pAclHeader->pTime + i + 1;
	}
	m_pAclHeader->pTime->bNotAllowEdit = 1;
}

//
// 创建控管规则文件
//
int CAclFile::CreateAcl(const TCHAR *sPathName)
{
	InitDefaultValue();
	return WriteAcl(sPathName);
}

//
// 打开控管规则文件
//
int CAclFile::OpenAcl()
{
	int		iRet;

	if(_taccess(m_sPathName + ACL_FILE_NAME, 0) == -1
		&& (iRet = CreateAcl(m_sPathName + ACL_FILE_NAME)) != XERR_SUCCESS)	
		return iRet;

	TRY										
	{
		m_AclFile.Open(	m_sPathName + ACL_FILE_NAME,
						CFile::modeRead			|
						CFile::typeBinary		| 
						CFile::shareDenyWrite
						);
	}
	CATCH( CFileException, e )
	{
		return XERR_FILE_CAN_NOT_OPEN;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 保存控管规则记录到控管规则文件
//
int CAclFile::WriteRecord(CFile *File, DWORD* pRecord, DWORD dwRecordLen, DWORD* FilePosition)
{
	if(pRecord == NULL)	return XERR_SUCCESS;

	TRY
	{
		*FilePosition = File->GetPosition();

		DWORD dwTempLen = dwRecordLen - sizeof(DWORD);
		DWORD dwNext = 0;
		DWORD* pTempRecord = pRecord;
		while(pTempRecord != NULL)
		{
			if(*pTempRecord != NULL)
			{
				dwNext = File->GetPosition() + dwRecordLen;
				File->Write(&dwNext, sizeof(DWORD));
				File->Write(pTempRecord + 1, dwTempLen);
				pTempRecord = (DWORD*)*pTempRecord;
			}
			else
			{
				File->Write(pTempRecord, dwRecordLen);
				break;
			}
		}
	}
	CATCH( CFileException, e )
	{
		return XERR_FILE_SAVE_ERROR;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 保存整个控管规则文件
//
int CAclFile::WriteAcl(const TCHAR *sPathName)
{
	CFile	FileAcl;

	TRY
	{
		FileAcl.Open(	sPathName,
						CFile::modeCreate	| 
						CFile::modeWrite	| 
						CFile::typeBinary	| 
						CFile::shareExclusive
						);

		XACL_HEADER AclHeader = *m_pAclHeader;

		FileAcl.Seek(ACL_HEADER_LENTH, CFile::begin);

		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pTime
			, ACL_TIME_LENTH
			, (DWORD*)&AclHeader.pTime);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pAllIp
			, ACL_IP_LENTH
			, (DWORD*)&AclHeader.pAllIp);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pIntranetIp
			, ACL_IP_LENTH
			, (DWORD*)&AclHeader.pIntranetIp);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pDistrustIp
			, ACL_IP_LENTH
			, (DWORD*)&AclHeader.pDistrustIp);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pTrustIp
			, ACL_IP_LENTH
			, (DWORD*)&AclHeader.pTrustIp);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pCustomIp
			, ACL_IP_LENTH
			, (DWORD*)&AclHeader.pCustomIp);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pAcl
			, ACL_ACL_LENTH
			, (DWORD*)&AclHeader.pAcl);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pWeb
			, ACL_WEB_LENTH
			, (DWORD*)&AclHeader.pWeb);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pNnb
			, ACL_NNB_LENTH
			, (DWORD*)&AclHeader.pNnb);
		WriteRecord(&FileAcl
			, (DWORD*)m_pAclHeader->pIcmp
			, ACL_ICMP_LENTH
			, (DWORD*)&AclHeader.pIcmp);
		
		FileAcl.SeekToBegin();
		FileAcl.Write(&AclHeader, ACL_HEADER_LENTH);

		FileAcl.Close();
	}
	CATCH( CFileException, e )
	{
		if(FileAcl.m_hFile != NULL)	FileAcl.Close();
		return XERR_FILE_CREATE_FAILURE;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 增加一条控管规则记录
//
int	CAclFile::AddRecord(
	PXACL_RECORD pAdd, 
	PXACL_RECORD* ppFirst, 
	DWORD* pCount
)
{
	if(pAdd == NULL) return XERR_FILE_NOT_ENOUGH_MEMORY;

	if(!Lock()) return XERR_FILE_LOCK_ERROR;
	__try
	{
		pAdd->pNext = NULL;
		*pCount ++;

		PXACL_RECORD pRecord = *ppFirst;

		if(pRecord == NULL)
		{
			pAdd->dwId = 1;
			*ppFirst = pAdd;
			return XERR_SUCCESS;
		}

		while(pRecord != NULL) 
		{
			if(pRecord->pNext == NULL)
			{
				pAdd->dwId = pRecord->dwId + 1;
				pRecord->pNext = pAdd;
				break; 
			}
			pRecord = pRecord->pNext;
		}
		return XERR_SUCCESS;
	}
	__finally
	{
		Unlock();
	}

	return XERR_SUCCESS;
}

//
// 更新控管规则记录
//
int	CAclFile::UpdateRecord(PXACL_RECORD pUpdate, PXACL_RECORD pFirst, int nRecordLen)
{
	if(!Lock()) return XERR_FILE_LOCK_ERROR;
	__try
	{
		BYTE* pRecord = (BYTE*)FindRecord(pFirst, pUpdate->dwId);
		BYTE* pUpdateTemp = (BYTE*)pUpdate;
		int nRecordSize = sizeof(XACL_RECORD);
		if(pRecord != NULL)
		{
			memcpy(pRecord + nRecordSize, pUpdateTemp + nRecordSize, nRecordLen - nRecordSize);
			return XERR_SUCCESS;
		}
		return XERR_FILE_RECORD_CAN_NOT_FIND;
	}
	__finally
	{
		Unlock();
	}
	return XERR_FILE_RECORD_CAN_NOT_FIND;
}

//
// 删除控管规则记录
//
int CAclFile::DelRecord(PXACL_RECORD* ppEntry, DWORD dwId, DWORD* pCount)
{
	if(!Lock()) return XERR_FILE_LOCK_ERROR;
	__try
	{
		PXACL_RECORD pPrev = NULL;
		PXACL_RECORD pCurrent = *ppEntry;
		while(pCurrent != NULL)
		{
			if(pCurrent->dwId == dwId)
			{
				if(pPrev == NULL)
					*ppEntry = pCurrent->pNext;
				else
					pPrev->pNext = pCurrent->pNext;
				*pCount --;

				return XERR_SUCCESS;
			}
			pPrev = pCurrent;
			pCurrent = pCurrent->pNext;
		}
		return XERR_FILE_RECORD_CAN_NOT_FIND;
	}
	__finally
	{
		Unlock();
	}
	return	XERR_FILE_RECORD_CAN_NOT_FIND;
}

//
// 查找控管规则记录
//
PVOID CAclFile::FindRecord(PXACL_RECORD pFirst, DWORD dwId)
{
	PXACL_RECORD pCurrent = pFirst;
	while(pCurrent != NULL)
	{
		if(pCurrent->dwId == dwId)
		{
			break;
		}
		pCurrent = pCurrent->pNext;
	}
	return (PVOID)pCurrent;
}

//
// 修正链表的指针
//
void CAclFile::RepairPoint(PXACL_RECORD* pAclRecord, DWORD dwBaseAddress)
{
	if(*pAclRecord == NULL) return;
	DWORD tmp = (DWORD)(*pAclRecord);
	*pAclRecord = (PXACL_RECORD)(dwBaseAddress + (DWORD)(*pAclRecord));
	PXACL_RECORD pTempAclRecord = *pAclRecord;
	while(pTempAclRecord != NULL && pTempAclRecord->pNext != NULL)
	{
		pTempAclRecord->pNext = (PXACL_RECORD)(dwBaseAddress + (DWORD)pTempAclRecord->pNext);
		pTempAclRecord = pTempAclRecord->pNext;
	}
}

//
// 创建一条记录
//
char* CAclFile::CreateRecord(PVOID pRecord, int nLenth)
{
	if(!Lock()) return NULL;
	__try
	{
		if(m_dwMaxOffset < (DWORD)m_pCurrentPoint + nLenth)
		{
			if(SaveAcl() != XERR_SUCCESS
				|| ReadAcl() != XERR_SUCCESS)
				return NULL;
		}

		char* pBuf = m_pCurrentPoint;
		memcpy(pBuf, pRecord, nLenth);
		m_pCurrentPoint += nLenth;
		return pBuf;
	}
	__finally
	{
		Unlock();
	}
	return NULL;
}

//============================================================================================
//Export function

//
// 更新文件
//
BOOL CAclFile::UpdateFile(DWORD nPosition, PVOID pBuf, int nLenth)
{
	if(m_sPathName.IsEmpty() || m_AclFile.m_hFile != NULL
		|| _taccess(m_sPathName + ACL_FILE_NAME, 0) == -1)
		return FALSE;

	TRY										
	{
		m_AclFile.Open(	m_sPathName + ACL_FILE_NAME,
						CFile::modeWrite		|
						CFile::typeBinary		| 
						CFile::shareExclusive
						);
		m_AclFile.Seek(nPosition, CFile::begin);
		m_AclFile.Write(pBuf, nLenth);
		CloseAcl();
	}
	CATCH( CFileException, e )
	{
		return FALSE;
	}
	END_CATCH

	return TRUE;
}

//
// 从文件中读取控管规则
//
int CAclFile::ReadAcl(BOOL IsDLL, HINSTANCE instance)	//main function
{
	int	iRet = XERR_SUCCESS;

	if(IsDLL)
		m_sPathName = CAclFile::GetAppPath(IsDLL, instance);
	else
		m_sPathName = CAclFile::GetAppPath();

	if(m_AclFile.m_hFile == NULL && (iRet = OpenAcl()) != XERR_SUCCESS)
		return iRet;

	TRY
	{
		CFileStatus	m_FileStatus;
		if(!m_AclFile.GetStatus(m_FileStatus))
			return XERR_FILE_GET_STATUS_ERROR;
		if(m_FileStatus.m_size < ACL_HEADER_LENTH)
			return XERR_FILE_INVALID_SIGNATURE;

		m_dwMaxSize = m_FileStatus.m_size + MAX_MEMORY_FILE_SIZE;
		if(!Create(XFILTER_MEMORY_ACL_FILE, m_dwMaxSize))
			return XERR_FILE_CREATE_MEMORY_ERROR;
		m_pCurrentPoint = m_pMemFile + m_FileStatus.m_size;
		m_dwMaxOffset = (DWORD)m_pMemFile + m_dwMaxSize;

		m_AclFile.SeekToBegin();
		m_AclFile.Read(m_pMemFile,m_FileStatus.m_size);	//read the header
		m_pAclHeader = (PXACL_HEADER)m_pMemFile;

		if(_tcscmp(m_pAclHeader->sSignature , ACL_HEADER_SIGNATURE) != 0)
		{
			CloseAcl();
			return XERR_FILE_INVALID_SIGNATURE;
		}
		//
		// 2002/12/20 modify for v2.1.0
		//
		if(	m_pAclHeader->ulVersion < COMP_ACL_VERSION_START 
			|| m_pAclHeader->ulVersion > COMP_ACL_VERSION_END
			|| m_pAclHeader->bMajor < COMP_ACL_MAJOR_START
			|| m_pAclHeader->bMajor > COMP_ACL_MAJOR_END
			)
		{
			CloseAcl();
			return XERR_FILE_INVALID_VERSION;
		}

		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pTime, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pAllIp, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pIntranetIp, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pDistrustIp, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pTrustIp, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pCustomIp, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pAcl, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pWeb, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pNnb, (DWORD)m_pAclHeader);
		RepairPoint((PXACL_RECORD*)&m_pAclHeader->pIcmp, (DWORD)m_pAclHeader);
		
		CloseAcl();

		if(!SetDllAclMemoryHandle(m_hMemFile)) 
			return XERR_FILE_SET_DLL_FILE_HANDLE_ERROR;

		ContinueFilter();
	}
	CATCH( CFileException, e )
	{
		CloseAcl();
		return XERR_FILE_READ_ERROR;
	}
	END_CATCH

	return iRet;
}

//
// 保存控管规则到文件
//
int	CAclFile::SaveAcl()
{
	int		iRet;

	if((iRet = WriteAcl(m_sPathName + ACL_TEMP_FILE_NAME)) != XERR_SUCCESS)
		return iRet;

	TRY
	{
		CFile::Remove(m_sPathName + ACL_FILE_NAME);
		CFile::Rename(m_sPathName + ACL_TEMP_FILE_NAME, m_sPathName + ACL_FILE_NAME);
	}
	CATCH( CFileException, e )
	{
		return XERR_FILE_SAVE_ERROR;
	}
	END_CATCH

	return XERR_SUCCESS;
}

//
// 增加控管规则
//
int	CAclFile::AddAcl(void *pAddAcl, int AclType)
{
	if(pAddAcl == NULL) return XERR_FILE_INVALID_PARAMETER;

	PXACL_RECORD pAddRecord = NULL;

	switch(AclType)
	{
	case ACL_TYPE_ACL:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_ACL_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pAcl
			, &m_pAclHeader->ulAclCount);

	case ACL_TYPE_DISTRUST_IP:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_IP_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pDistrustIp
			, &m_pAclHeader->ulDistrustIPCount);

	case ACL_TYPE_TRUST_IP:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_IP_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pTrustIp
			, &m_pAclHeader->ulTrustIPCount);

	case ACL_TYPE_CUSTOM_IP:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_IP_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pCustomIp
			, &m_pAclHeader->ulCustomIPCount);

	case ACL_TYPE_INTRANET_IP:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_IP_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pIntranetIp
			, &m_pAclHeader->ulIntranetIPCount);

	case ACL_TYPE_WEB:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_WEB_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pWeb
			, &m_pAclHeader->dwWebCount);

	case ACL_TYPE_NNB:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_NNB_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pNnb
			, &m_pAclHeader->dwNnbCount);

	case ACL_TYPE_ICMP:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_ICMP_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pIcmp
			, &m_pAclHeader->dwIcmpCount);

	case ACL_TYPE_TIME:
		pAddRecord = (PXACL_RECORD)CreateRecord(pAddAcl, ACL_TIME_LENTH);
		return AddRecord(pAddRecord
			, (PXACL_RECORD*)&m_pAclHeader->pTime
			, &m_pAclHeader->ulTimeCount);

	default:
		return XERR_FILE_INVALID_PARAMETER;
	}

	return XERR_SUCCESS;
}

//
// 更新控管规则
//
int	CAclFile::UpdateAcl(void *pAcl, int AclType)
{
	switch(AclType)
	{
	case ACL_TYPE_ACL:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pAcl, ACL_ACL_LENTH);
	case ACL_TYPE_DISTRUST_IP:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pDistrustIp, ACL_IP_LENTH);
	case ACL_TYPE_TRUST_IP:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pTrustIp, ACL_IP_LENTH);
	case ACL_TYPE_CUSTOM_IP:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pCustomIp, ACL_IP_LENTH);
	case ACL_TYPE_INTRANET_IP:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pIntranetIp, ACL_IP_LENTH);
	case ACL_TYPE_WEB:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pWeb, ACL_WEB_LENTH);
	case ACL_TYPE_NNB:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pNnb, ACL_NNB_LENTH);
	case ACL_TYPE_ICMP:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pIcmp, ACL_ICMP_LENTH);
	case ACL_TYPE_TIME:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pTime, ACL_TIME_LENTH);
	case ACL_TYPE_ALL_IP:
		return UpdateRecord((PXACL_RECORD)pAcl, (PXACL_RECORD)m_pAclHeader->pAllIp, ACL_IP_LENTH);
	default:
		return XERR_FILE_INVALID_PARAMETER;
	}
}

//
// 删除控管规则
//
int	CAclFile::DelAcl(DWORD dwId, int AclType)
{
	switch(AclType)
	{
	case ACL_TYPE_ACL:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pAcl, dwId, &m_pAclHeader->ulAclCount);
	case ACL_TYPE_DISTRUST_IP:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pDistrustIp, dwId, &m_pAclHeader->ulDistrustIPCount);
	case ACL_TYPE_TRUST_IP:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pTrustIp, dwId, &m_pAclHeader->ulTrustIPCount);
	case ACL_TYPE_CUSTOM_IP:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pCustomIp, dwId, &m_pAclHeader->ulCustomIPCount);
	case ACL_TYPE_INTRANET_IP:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pIntranetIp, dwId, &m_pAclHeader->ulIntranetIPCount);
	case ACL_TYPE_WEB:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pWeb, dwId, &m_pAclHeader->dwWebCount);
	case ACL_TYPE_NNB:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pNnb, dwId, &m_pAclHeader->dwNnbCount);
	case ACL_TYPE_ICMP:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pIcmp, dwId, &m_pAclHeader->dwIcmpCount);
	case ACL_TYPE_TIME:
		return DelRecord((PXACL_RECORD*)&m_pAclHeader->pTime, dwId, &m_pAclHeader->ulTimeCount);
	default:
		return XERR_FILE_INVALID_PARAMETER;
	}

	return XERR_SUCCESS;
}

//
// 查找控管规则
//
PVOID CAclFile::FindAcl(DWORD dwId, int AclType)
{
	switch(AclType)
	{
	case ACL_TYPE_ACL:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pAcl, dwId);
	case ACL_TYPE_DISTRUST_IP:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pDistrustIp, dwId);
	case ACL_TYPE_TRUST_IP:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pTrustIp, dwId);
	case ACL_TYPE_CUSTOM_IP:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pCustomIp, dwId);
	case ACL_TYPE_INTRANET_IP:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pIntranetIp, dwId);
	case ACL_TYPE_WEB:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pWeb, dwId);
	case ACL_TYPE_NNB:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pNnb, dwId);
	case ACL_TYPE_ICMP:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pIcmp, dwId);
	case ACL_TYPE_TIME:
		return FindRecord((PXACL_RECORD)m_pAclHeader->pTime, dwId);
	case ACL_TYPE_ALL_IP:
		return (PVOID)m_pAclHeader->pAllIp;
	default:
		return NULL;
	}
}

//
// 得到错误字符串
//
CString CAclFile::GetErrorString(int iErrorCode)
{
	switch(iErrorCode)
	{
	case XERR_SUCCESS:
		return ERROR_STRING_SUCCESS;
	case XERR_FILE_NOT_FOUND:
		return ERROR_STRING_FILE_NOT_FOUND;
	case XERR_FILE_ALREDAY_EXISTS:
		return ERROR_STRING_FILE_ALREDAY_EXISTS;
	case XERR_FILE_LOCKED:
		return ERROR_STRING_FILE_LOCKED;
	case XERR_FILE_CREATE_FAILURE:
		return ERROR_STRING_FILE_CREATE_FAILURE;
	case XERR_FILE_CAN_NOT_OPEN:
		return ERROR_STRING_FILE_CAN_NOT_OPEN;
	case XERR_FILE_INVALID_SIGNATURE:
		return ERROR_STRING_FILE_INVALID_SIGNATURE;
	case XERR_FILE_READ_ERROR:
		return ERROR_STRING_FILE_READ_ERROR;
	case XERR_FILE_SAVE_ERROR:
		return ERROR_STRING_FILE_SAVE_ERROR;
	case XERR_FILE_ADD_ERROR:
		return ERROR_STRING_FILE_ADD_ERROR;
	case XERR_FILE_GET_STATUS_ERROR:
		return ERROR_STRING_FILE_GET_STATUS_ERROR;
	case XERR_FILE_READ_ONLY:
		return ERROR_STRING_FILE_READ_ONLY;
	case XERR_FILE_WRITER_HEADER_ERROR:
		return ERROR_STRING_FILE_WRITER_HEADER_ERROR;
	case XERR_FILE_RECORD_CAN_NOT_FIND:
		return ERROR_STRING_FILE_RECORD_CAN_NOT_FIND;
	case XERR_FILE_INVALID_PARAMETER:
		return ERROR_STRING_FILE_INVALID_PARAMETER;
	case XERR_FILE_INVALID_VERSION:
		return ERROR_STRING_FILE_INVALID_VERSION;
	default:
		return ERROR_STRING_NOCODE;
	}
}

//
// 将数值IP转化为字符串IP
//
CString CAclFile::DIPToSIP(DWORD* pIP)
{
	if(pIP == NULL)
		return _T("");

	CString	s;
	BYTE	*b = (BYTE*)pIP;
	s.Format(_T("%d.%d.%d.%d"),b[3],b[2],b[1],b[0]);

	return s;
}

/*---------------------------------------------------------------------------------------------
	index from 0 start, for example:
	index:				0 1 2 3 4 5 6 7
	Binary value:		0 0 0 0 0 0 0 0
*/
int	CAclFile::GetBit(BYTE bit, int index, int count)
{
	bit <<= index;
	bit >>= (8 - count);

	return bit;
}

//
// 设置字节中的一位
//
int CAclFile::SetBit(BYTE* bit, int index, BOOL isTrue)
{
	BYTE bOr = 0xFF,bAnd = 0x00;

	bOr <<= index;
	bOr >>= 7;
	bOr <<= (7 - index);
	bAnd = ~bOr;

	if(isTrue)
		*bit = *bit | bOr;
	else
		*bit = *bit & bAnd;

	return 0;
}

//============================================================================================
//static function

//
// 得到应用程序路径
//
CString CAclFile::GetAppPath(BOOL IsDLL, HINSTANCE instance, BOOL IsFullPathName) 
{
	TCHAR sFilename[_MAX_PATH];
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	if(IsDLL)
		GetModuleFileName(instance, sFilename, _MAX_PATH);
	else
		GetModuleFileName(AfxGetInstanceHandle(), sFilename, _MAX_PATH);

	if(IsFullPathName)
		return sFilename;

	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	CString rVal(CString(sDrive) + CString(sDir));
	int nLen = rVal.GetLength();

	if (rVal.GetAt(nLen-1) != _T('\\'))
		rVal += _T("\\");

	return rVal;
}

//
// 从完整路径中分离出路径
//
CString CAclFile::GetPath(TCHAR *sFilename) 
{
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	CString rVal(CString(sDrive) + CString(sDir));
	int nLen = rVal.GetLength();

	if (rVal.GetAt(nLen-1) != _T('\\'))
		rVal += _T("\\");

	return rVal;
}  

//
// 从完整路径中分离出文件名
//
CString CAclFile::GetName(TCHAR *sFilename) 
{
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	CString rVal;
	rVal.Format(_T("%s%s"), sFname, sExt);

	return rVal;
}  


//==================================================================================
// 控管规则操作的历史记录类，保存在界面上对控管规则所做的添加、修改、删除动作，
// 当使用应用按钮时，再真正更新内存并保存到文件。
//


CAclHistory::CAclHistory(int nAclType, CAclFile *pAclFile)
{
	m_nAclType = nAclType;
	m_pAclFile = pAclFile;
	m_nAclLenth = GetAclLenth(nAclType);
}

CAclHistory::~CAclHistory()
{
	RemoveArray();
}

void CAclHistory::RemoveArray()
{
	m_arHistory.RemoveAll();
	m_arForward.RemoveAll();
}

char* CAclHistory::FindAcl(DWORD dwId)
{
	int nCount = GetHistoryCount();
	int nPoint = sizeof(PVOID);
	for(int i = nCount - 1; i >= 0; i--)
	{
		if((m_arHistory[i].bOptType == OPT_TYPE_ADD || m_arHistory[i].bOptType == OPT_TYPE_EDIT)
		  && *(DWORD*)(m_arHistory[i].pRecord + nPoint) == dwId)
			return (char*)m_arHistory[i].pRecord;
	}
	return NULL;
}

int CAclHistory::AddHistory(
	BYTE	bOptType, 
	BYTE	bButtonStatus, 
	char	*pRecord, 
	char	*pRecordOld,
	BYTE	bSet, 
	BYTE	bQueryEx
)
{
	if((bButtonStatus & ACL_BUTTON_APPLY_MASK) != ACL_BUTTON_APPLY_MASK)
	{
		RemoveArray();
		return XERR_SUCCESS;
	}

	if(m_nAclLenth > MAX_RECORD_LENTH_BUFFER) return ERROR_NO_ENOUGH_BUFFER;

	m_AclHistory.bOptType = bOptType;
	m_AclHistory.bButtonStatus = bButtonStatus;

	if(bOptType == OPT_TYPE_EDIT && pRecordOld != NULL)
	{
		memcpy(m_AclHistory.pRecordOld, pRecordOld, m_nAclLenth);
	}

	switch(bOptType)
	{
	case OPT_TYPE_ADD:
	case OPT_TYPE_EDIT:
	case OPT_TYPE_DELETE:
		if(pRecord == NULL) return ERROR_INVALID_RECORD;
		memcpy(m_AclHistory.pRecord, pRecord, m_nAclLenth);
		break;
	case OPT_TYPE_SET:
		m_AclHistory.bSet = bSet;
		break;
	case OPT_TYPE_QUERY_EX:
		m_AclHistory.bQueryEx = bQueryEx;
		break;
	default:
		return ERROR_INVALID_OPTION;
	}
	m_arHistory.Add(m_AclHistory);
	m_arForward.RemoveAll();

	return XERR_SUCCESS;
}

int CAclHistory::Apply()
{
	if(m_pAclFile == NULL) return ERROR_NO_ACL_FILE;

	int nCount = GetHistoryCount();
	for(int i = 0; i < nCount; i++)
	{
		switch(m_arHistory[i].bOptType)
		{
		case OPT_TYPE_ADD:
			m_pAclFile->AddAcl((PVOID)m_arHistory[i].pRecord, m_nAclType);
			break;
		case OPT_TYPE_EDIT:
			m_pAclFile->UpdateAcl((PVOID)m_arHistory[i].pRecord, m_nAclType);
			break;
		case OPT_TYPE_DELETE:
			m_pAclFile->DelAcl(*((DWORD*)m_arHistory[i].pRecord + 1), m_nAclType);
			break;
		case OPT_TYPE_SET:
			SetSet(m_nAclType, m_arHistory[i].bOptType, m_arHistory[i].bSet);
			break;
		case OPT_TYPE_QUERY_EX:
			SetSet(m_nAclType, m_arHistory[i].bOptType, m_arHistory[i].bQueryEx);
			break;
		default:
			return ERROR_INVALID_OPTION;
		}
	}
	RemoveArray();
	return m_pAclFile->SaveAcl();
}

int CAclHistory::Cancel()
{
	RemoveArray();
	return XERR_SUCCESS;
}

int CAclHistory::Back()
{
	return XERR_SUCCESS;
}

int CAclHistory::Forward()
{
	return XERR_SUCCESS;
}

void CAclHistory::SetSet(int nAclType, BYTE bOptType, BYTE bSet)
{
	switch(nAclType)
	{
	case ACL_TYPE_TIME:
	case ACL_TYPE_ALL_IP:
	case ACL_TYPE_INTRANET_IP:
	case ACL_TYPE_DISTRUST_IP:
	case ACL_TYPE_TRUST_IP:
	case ACL_TYPE_CUSTOM_IP:
		return;
	case ACL_TYPE_ACL:
		if(OPT_TYPE_SET == bOptType)
			m_pAclFile->GetHeader()->bAppSet = bSet;
		else if(OPT_TYPE_QUERY_EX == bOptType)
			m_pAclFile->GetHeader()->bAppQueryEx = bSet;
		else
			return;
		return;
	case ACL_TYPE_WEB:
		if(OPT_TYPE_SET == bOptType)
			m_pAclFile->GetHeader()->bWebSet = bSet;
		else if(OPT_TYPE_QUERY_EX == bOptType)
			m_pAclFile->GetHeader()->bWebQueryEx = bSet;
		else
			return;
		return;
	case ACL_TYPE_NNB:
		if(OPT_TYPE_SET == bOptType)
			m_pAclFile->GetHeader()->bNnbSet = bSet;
		else if(OPT_TYPE_QUERY_EX == bOptType)
			m_pAclFile->GetHeader()->bNnbQueryEx = bSet;
		else
			return;
		return;
	case ACL_TYPE_ICMP:
		if(OPT_TYPE_SET == bOptType)
			m_pAclFile->GetHeader()->bIcmpSet = bSet;
		else if(OPT_TYPE_QUERY_EX == bOptType)
			m_pAclFile->GetHeader()->bIcmpQueryEx = bSet;
		else
			return;
		return;
	default:
		return;
	}
}

int CAclHistory::GetAclLenth(int nAclType)
{
	switch(nAclType)
	{
	case ACL_TYPE_TIME:
		return ACL_TIME_LENTH;
	case ACL_TYPE_ALL_IP:
	case ACL_TYPE_INTRANET_IP:
	case ACL_TYPE_DISTRUST_IP:
	case ACL_TYPE_TRUST_IP:
	case ACL_TYPE_CUSTOM_IP:
		return ACL_IP_LENTH;
	case ACL_TYPE_ACL:
		return ACL_ACL_LENTH;
	case ACL_TYPE_WEB:
		return ACL_WEB_LENTH;
	case ACL_TYPE_NNB:
		return ACL_NNB_LENTH;
	case ACL_TYPE_ICMP:
		return ACL_ICMP_LENTH;
	default:
		return 0;
	}
}


#pragma comment( exestr, "B9D3B8FD2A7A686B6E672B")
