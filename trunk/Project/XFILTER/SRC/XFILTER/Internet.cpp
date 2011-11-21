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

#include "stdafx.h"
#include "xfilter.h"		// in the header #include "internet.h"
#include "Register.h"

const TCHAR szHeaders[] =
	_T("Accept: text/*\r\nUser-Agent: XFILTER\r\n");

BOOL m_bIsDownloading = FALSE;

CHttpRequest::CHttpRequest(CWnd* pParent)
{
	m_pUrlRequest[0]		= '\0';
	m_DownloadThread		= NULL;
	m_UploadThread			= NULL;
	m_IsUploaded			= TRUE;
	lCommandId				= NULL;
	m_IsConnecting			= FALSE;

	m_pParent				= pParent;
	m_IsSyn					= TRUE;
	m_bIsClose				= FALSE;
	m_bIsEdit				= FALSE;

	memset(&m_UserInfo, 0, sizeof(m_UserInfo));
}

CHttpRequest::~CHttpRequest()
{
	Close();
}

void CHttpRequest::SetVersion(DWORD dwVersion, BYTE bMajor, BYTE bMinor)
{
	m_dwVersion = dwVersion;
	m_bMajor = bMajor;
	m_bMinor = bMinor;
	m_sVersion.Format("%u.%u.%u", dwVersion, bMajor, bMinor);
}

int CHttpRequest::ConnectUrl(TCHAR *sUrl, TCHAR *sReturn, long *lVersion, int *Count)
{
	ODS(_T("XFILTER.EXE: GetFromUrl Begin... \n"));
	
	if(sUrl == NULL)
		return XERR_INVALID_PARAMETER;

	CString			strServerName;
	CString			strObject;
	INTERNET_PORT	nPort;
	DWORD			dwServiceType;

	if (!AfxParseURL(sUrl, dwServiceType, strServerName, strObject, nPort) ||
		dwServiceType != INTERNET_SERVICE_HTTP)
	{
		ODS(_T("XFILTER.EXE: Internet Invalid Url ..."));
		return XERR_INTERNET_URL_ERROR;
	}

	CInternetSession	session(GUI_APP_CLASS_NAME);
	CHttpConnection		*pServer	= NULL;
	CHttpFile			*pFile		= NULL;
	int					iRet		= XERR_SUCCESS;

	m_IsConnecting = TRUE;

	try
	{
		pServer = session.GetHttpConnection(strServerName, nPort);
		pFile	= pServer->OpenRequest(CHttpConnection::HTTP_VERB_GET, strObject);

		pFile->AddRequestHeaders(szHeaders);
		pFile->SendRequest();

		DWORD	dwRet;
		pFile->QueryInfoStatusCode(dwRet);

		if (dwRet >= 400 && dwRet <= 499)
		{
			ODS(_T("XFILTER.EXE: Internet Request Error ..."));
			iRet = XERR_INTERNET_REQUEST_ERROR;
		}
		else if(dwRet >= 500 && dwRet <= 599)
		{
			ODS(_T("XFILTER.EXE: Internet Server Error ..."));
			iRet = XERR_INTERNET_SERVER_ERROR;
		}
		else if(sReturn != NULL)
		{
			pFile->ReadString(sReturn, MAX_NET_COMMAND_LENTH - 1);
			ODS(sReturn);

			CString tmpStr	= sReturn;
			long lVer		= atol(tmpStr.Left(MAX_NET_COMMAND_VERSION_LENTH));

			if(lVer > *lVersion)
			{
				*lVersion = lVer;
				int		i = 1;

				while (i < MAX_NET_COMMAND
					&& pFile->ReadString((sReturn + MAX_NET_COMMAND_LENTH * i), MAX_NET_COMMAND_LENTH - 1))
				{
					ODS(sReturn + i * MAX_NET_COMMAND_LENTH);
					i ++;
				}
				*Count = i;
			}
			else
			{
				*Count = 1;
			}
		}
		else
		{
			CString sRet;
			pFile->ReadString(sRet);
			if(sRet.GetAt(0) != '1')
				iRet = XERR_INTERNET_REG_ERROR;

			ODS2(_T("XFILTER.EXE: Internet User Register Return Value "),sRet);
		}

		pFile->Close();
		pServer->Close();
	}
	catch(CInternetException* pEx)
	{
		pEx->Delete();
		iRet = XERR_INTERNET_CONNECT_ERROR;
		ODS(_T("XFILTER.EXE: GetFromUrl XERR_INTERNET_CONNECT_ERROR... "));
	}

	if (pFile != NULL)
		delete pFile;
	if (pServer != NULL)
		delete pServer;
	session.Close();

	m_IsConnecting = FALSE;

	ODS(_T("XFILTER.EXE: GetFromUrl End... "));
	return iRet;
}

void CHttpRequest::Close()
{
	if(lCommandId != NULL)
	{
		delete lCommandId;
		lCommandId = NULL;
	}
/*
	if(m_UploadThread != NULL)
	{
		TerminateThread(m_UploadThread, 0);
		m_UploadThread = NULL;
	}

	if(m_DownloadThread != NULL)
	{
		TerminateThread(m_DownloadThread, 0);
		m_DownloadThread = NULL;
	}
*/
	ODS(_T("XFILTER.EXE: Internet Close delete[] lCommandId and exit thread..."));
}

BOOL CHttpRequest::IsConnected()
{
	ODS(_T("XFILTER.EXE: Internet Check Connected..."));

    RASCONN			lpRasConn;
	RASCONNSTATUS	rasStatus;     
	DWORD			cbBuf = 0;     
	DWORD			cConn = 0;     
	DWORD			dwRet = 0; 
	cbBuf				= sizeof(RASCONN);
	lpRasConn.dwSize	= sizeof(RASCONN );
	dwRet = RasEnumConnections(&lpRasConn, &cbBuf, &cConn );
	if ( dwRet != 0 )   
		return FALSE;
	else
	{
		rasStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(lpRasConn.hrasconn,&rasStatus);
		if (rasStatus.rasconnstate==RASCS_Connected)
			return TRUE;
		else
			return FALSE;
	}
	return TRUE;
}

BOOL CHttpRequest::IsRegistered()
{
	//
	// 2002/12/20 modify for 2.1.0
	//
	//m_Install.ReadReg(REG_INFO_ITEM, (BYTE*)&m_UserInfo, sizeof(XUSER_INFO));
	m_Install.ReadReg(REG_INFO_ITEM, (BYTE*)&m_UserInfo, sizeof(XUSER_INFO), HKEY_LOCAL_MACHINE, "Software\\Filseclab");
	if(m_UserInfo.iStatus == REG_STATUS_REGISTERED)
		return TRUE;
	return FALSE;
}

int CHttpRequest::Register(BOOL IsSyn)
{
//	DWORD dwFlags;
//	if(!InternetGetConnectedState(&dwFlags, 0))
//		return XERR_CURRENT_NOT_ONLINE;

	m_Install.ReadReg(REG_INFO_ITEM, (BYTE*)&m_UserInfo, sizeof(XUSER_INFO));

	m_IsSyn = IsSyn;
	UserReg();

	return XERR_SUCCESS;
}

BOOL CHttpRequest::UserReg()
{
	CRegister dlgReg(this);

	int iRet = dlgReg.DoModal();
	if(iRet == IDCANCEL)
		return FALSE;
	return TRUE;
}

BOOL CHttpRequest::PreUpload(BOOL IsSyn)
{
	SetRegisterUrl();

	if(IsSyn)
	{
		if(ConnectUrl(m_pUrlRequest) == XERR_SUCCESS)
		{
			m_UserInfo.iStatus = REG_STATUS_REGISTERED;
			if(m_Install.SaveReg(REG_INFO_ITEM, (BYTE*)&m_UserInfo, sizeof(XUSER_INFO)))
				return TRUE;
			else 
				return FALSE;
		}
		else
			return FALSE;
	}
	else
	{
		DWORD	dwThreadId;
		m_UploadThread = ::CreateThread(NULL, 0, UploadUserInfo, this, 0, &dwThreadId);
	}

	return TRUE;
}

void CHttpRequest::SetRegisterUrl()
{
	sprintf(m_pUrlRequest, _T("%s?sEmail=%s&sName=%s&bGender=%u&sQQ=%s&sInc=%s&sBirthday=%s&bDegree=%u&bMetier=%u&sDuty=%s&sZip=%s&bSalary=%u&sAddress=%s&iProductId=%s&sPassword=%s&sOffenToWeb=%s&bCheck0=%u&bCheck1=%u&bCheck2=%u&bCheck3=%u&bCheck4=%u&bCheck5=%u&bCheck6=%u&bCheck7=%u&bCheck8=%u&bCheck9=%u&bCheck10=%u&bCheck11=%u&bCheck12=%u&bCheck13=%u&bCombo0=%u&bCombo1=%u&bCombo2=%u&bCombo3=%u&bCombo4=%u&bCombo5=%u&bCombo6=%u&bCombo7=%u&bCombo8=%u&sCity=%s&Recommender=%s")
		, REGISTER_URL
		, m_UserInfo.sEmail
		, m_UserInfo.sName
		, m_UserInfo.bGender 
		, m_UserInfo.sQQ 
		, m_UserInfo.sInc
		, m_UserInfo.tBirthday.Format("%Y-%m-%d")
		, m_UserInfo.bDegree 
		, m_UserInfo.bMetier
		, m_UserInfo.sDuty 
		, m_UserInfo.sZip  
		, m_UserInfo.bSalary 
		, m_UserInfo.sAddress
		, PRODUCT_ID
		, m_UserInfo.sPassword 
		, m_UserInfo.sOffenToWeb
		, m_UserInfo.bCheck[0]
		, m_UserInfo.bCheck[1]
		, m_UserInfo.bCheck[2]
		, m_UserInfo.bCheck[3]
		, m_UserInfo.bCheck[4]
		, m_UserInfo.bCheck[5]
		, m_UserInfo.bCheck[6]
		, m_UserInfo.bCheck[7]
		, m_UserInfo.bCheck[8]
		, m_UserInfo.bCheck[9]
		, m_UserInfo.bCheck[10]
		, m_UserInfo.bCheck[11]
		, m_UserInfo.bCheck[12]
		, m_UserInfo.bCheck[13]
		, m_UserInfo.bCombo[0]
		, m_UserInfo.bCombo[1]
		, m_UserInfo.bCombo[2]
		, m_UserInfo.bCombo[3]
		, m_UserInfo.bCombo[4]
		, m_UserInfo.bCombo[5]
		, m_UserInfo.bCombo[6]
		, m_UserInfo.bCombo[7]
		, m_UserInfo.bCombo[8]
		, m_UserInfo.sCity
		, m_UserInfo.Recommender
		);

	ODS(m_pUrlRequest);
	//OutputDebugString(m_pUrlRequest);
}

int CHttpRequest::DownloadNetCommand(BYTE bUpdateInterval, BOOL IsSyn)
{
	int iRet = XERR_UPDATE_INTERVAL_INVALID;
//	DWORD dwFlags;
//	if(!InternetGetConnectedState(&dwFlags, 0))
//		return XERR_CURRENT_NOT_ONLINE;

	if(bUpdateInterval >= 100)
		return XERR_UPDATE_INTERVAL_INVALID;

	memset(&m_CommandHeader, 0, sizeof(XNET_COMMAND_HEADER));
	m_Install.ReadReg(REG_NET_COMMAND_HEADER_ITEM
		, (BYTE*)&m_CommandHeader, sizeof(XNET_COMMAND_HEADER));

	//
	// 2002/12/20 Modify for v2.1.0
	//
	CTimeSpan ts = CTime::GetCurrentTime() - m_CommandHeader.tCheckTime;
	int IntervalDays = ts.GetDays();
	if( bUpdateInterval == 0 || m_CommandHeader.tCheckTime == 0
		|| IntervalDays >= bUpdateInterval) 
	{
		if(m_CommandHeader.tCheckTime != 0)
		{
			CString String, Message, Caption;
			Caption.LoadString(IDS_CAPTION);
			if(bUpdateInterval != 0)
			{
				String.LoadString(IDS_UPDATE_MESSAGE);
				Message.Format(String, IntervalDays);
				int iAsk = ::MessageBox(NULL, Message, Caption, MB_YESNO | MB_ICONQUESTION);
				if(iAsk == IDNO)
					return XERR_REGISTER_DLG_CANCEL;
			}
			CShell m_Shell;
			if(m_Shell.RunProgram(PROGRAM_UPDATE) != SHELL_ERROR_SUCCESS)
			{
				Message.LoadString(IDS_ERROR_CANT_RUN_FILUP);
				::MessageBox(NULL, Message, Caption, MB_OK | MB_ICONWARNING);
				return XERR_REGISTER_DLG_CANCEL;
			}
		}
		m_CommandHeader.tCheckTime = CTime::GetCurrentTime();
		m_Install.SaveReg(REG_NET_COMMAND_HEADER_ITEM, (BYTE*)&m_CommandHeader, sizeof(XNET_COMMAND_HEADER));
		iRet = XERR_SUCCESS;

		//
		// 2002/12/20 remove for v2.1.0
		//
		/*
		sprintf(m_pUrlRequest, NET_COMMAND_URL);

		if(IsSyn)
		{
			iRet = DownloadCommandFile(this);
		}
		else
		{
			DWORD dwThreadId;
			m_DownloadThread = ::CreateThread(NULL
				, 0, DownloadCommandFile, this, 0, &dwThreadId);
			m_CommandHeader.tCheckTime = CTime::GetCurrentTime();
			m_Install.SaveReg(REG_NET_COMMAND_HEADER_ITEM, 
				(BYTE*)&m_CommandHeader, sizeof(XNET_COMMAND_HEADER));
			iRet = XERR_STATUS_PENDING;
		}
		//*/
	}

	return iRet;
}

BOOL CHttpRequest::StopDownload()
{
	while(m_bIsDownloading)
		Sleep(50);
	return TRUE;
}

DWORD WINAPI DownloadCommandFile(LPVOID pVoid)
{
	CString		tmpStr;
	long		tmpLong;
	int			i;
	int			tmpCount	= 0;

	m_bIsDownloading = TRUE;

	DWORD dwReturnCode = 0;
	CHttpRequest *pDownloadCommand = (CHttpRequest *)pVoid;
	CXInstall m_Install;

	if(pDownloadCommand->m_pUrlRequest[0] == 0)
	{
		dwReturnCode = XERR_INVALID_PARAMETER;
		goto XF_EXIT;
	}
	
	TCHAR	sCommand[MAX_NET_COMMAND][MAX_NET_COMMAND_LENTH];

	if(pDownloadCommand->m_CommandHeader.lCount > 0)
	{
		pDownloadCommand->lCommandId = new long[pDownloadCommand->m_CommandHeader.lCount];
		m_Install.ReadReg(REG_NET_COMMAND_ITEM
			, (BYTE*)pDownloadCommand->lCommandId, sizeof(long) * pDownloadCommand->m_CommandHeader.lCount);
	}

	if(pDownloadCommand->ConnectUrl(pDownloadCommand->m_pUrlRequest
		, (TCHAR*)sCommand, &pDownloadCommand->m_CommandHeader.lVersion,&tmpCount) != XERR_SUCCESS)
	{
		dwReturnCode = XERR_INTERNET_CONNECT_ERROR;
		goto XF_EXIT;
	}

	if(tmpCount < 1)
	{
		dwReturnCode = XERR_INTERNET_CONNECT_ERROR;
		goto XF_EXIT;
	}

	if(tmpCount == 1)
		goto ONLY_VERSION;

	for(i = 1; i < tmpCount; i++)
	{
		tmpStr	= sCommand[i];
		tmpLong = atol(tmpStr.Left(MAX_NET_COMMAND_VERSION_LENTH));

		int j = 0;
		for(j; j < pDownloadCommand->m_CommandHeader.lCount; j++)
		{
			if(tmpLong == pDownloadCommand->lCommandId [j])
				break;
		}

		if(j < pDownloadCommand->m_CommandHeader.lCount)
			continue;

		long *pLong = NULL;

		if(pDownloadCommand->m_CommandHeader.lCount > 0)
		{
			pLong = new long[pDownloadCommand->m_CommandHeader.lCount];
			memcpy(pLong, pDownloadCommand->lCommandId, sizeof(long) * pDownloadCommand->m_CommandHeader.lCount);
			delete[](pDownloadCommand->lCommandId);
		}

		pDownloadCommand->lCommandId = new long[pDownloadCommand->m_CommandHeader.lCount + 1];

		if(pDownloadCommand->m_CommandHeader.lCount > 0)
		{
			memcpy(pDownloadCommand->lCommandId, pLong, sizeof(long) * pDownloadCommand->m_CommandHeader.lCount);
			delete[](pLong);
		}

		pDownloadCommand->lCommandId[pDownloadCommand->m_CommandHeader.lCount] = tmpLong;
		pDownloadCommand->m_CommandHeader.lCount ++;

		int iCommand	= atoi(tmpStr.Mid(MAX_NET_COMMAND_VERSION_LENTH + 1
							, MAX_NET_COMMAND_COMMAND_LENTH));
		CString sMessage= tmpStr.Mid(MAX_NET_COMMAND_VERSION_LENTH 
							+ MAX_NET_COMMAND_COMMAND_LENTH + 2);

		sMessage.Replace(13, '\0');
		sMessage.Replace(10, '\0');

		if(sMessage.GetAt(0) == '\0')
			break;
		
		switch(iCommand)
		{
		case NET_COMMAND_CHANGE_POST_MESSAGE:
			_tcscpy(pDownloadCommand->m_sMessage[i], sMessage);
			if(!pDownloadCommand->m_bIsClose)
				pDownloadCommand->m_pParent->SendMessage(WM_NET_MESSAGE, i, NULL);
			dwReturnCode = NET_COMMAND_HAVE_MESSAGE;
			break;

		default:
			break;
		}
	}

	// Syn not save time
	//pDownloadCommand->m_CommandHeader.tCheckTime = CTime::GetCurrentTime();

	m_Install.SaveReg(REG_NET_COMMAND_HEADER_ITEM, 
		(BYTE*)&pDownloadCommand->m_CommandHeader, sizeof(XNET_COMMAND_HEADER));

	m_Install.SaveReg(REG_NET_COMMAND_ITEM, (BYTE*)pDownloadCommand->lCommandId
		, sizeof(long) * pDownloadCommand->m_CommandHeader.lCount);

	if(pDownloadCommand->lCommandId != NULL)
	{
		delete pDownloadCommand->lCommandId;
		pDownloadCommand->lCommandId = NULL;
	}

ONLY_VERSION:
	{
		LPTSTR pNewVersion = sCommand[0] + MAX_NET_COMMAND_VERSION_LENTH + 1;
		int iLength = pDownloadCommand->m_sVersion.GetLength();
		if(strnicmp((LPCTSTR)pDownloadCommand->m_sVersion, pNewVersion, iLength) < 0)
		{
			memcpy(pDownloadCommand->m_sNewVersion, pNewVersion, iLength);
			pDownloadCommand->m_sNewVersion[iLength] = 0;
			dwReturnCode = NET_COMMAND_HAVE_NEW_VERSION;
		}
	}

XF_EXIT:
	if(!pDownloadCommand->m_bIsClose)
		pDownloadCommand->m_pParent->SendMessage(WM_NET_MESSAGE
			, MAX_NET_COMMAND, dwReturnCode);

	m_bIsDownloading = FALSE;

	return dwReturnCode;
}

DWORD WINAPI UploadUserInfo(LPVOID pVoid)
{
	DWORD dwReturnCode = 0;
	CXInstall m_Install;
	BOOL	IsUploaded	= FALSE;
	BOOL	IsSaved		= FALSE;
	int		iCount		= 0;

	CHttpRequest *pRegisterRequest = (CHttpRequest *)pVoid;
	if(pRegisterRequest->m_pUrlRequest[0] == '\0')
		goto XF_EXIT;

	while(!IsUploaded || !IsSaved)
	{
		if(++iCount > 3)
			break;

		if(!IsUploaded && pRegisterRequest->ConnectUrl(
			pRegisterRequest->m_pUrlRequest
			) == XERR_SUCCESS)
		{
			pRegisterRequest->m_UserInfo.iStatus = REG_STATUS_REGISTERED;
			IsUploaded = TRUE;
		}

		if(m_Install.SaveReg(REG_INFO_ITEM, (BYTE*)&pRegisterRequest->m_UserInfo, sizeof(XUSER_INFO)))
		{
			dwReturnCode = NET_UPLOAD_SUCCESS;
			IsSaved = TRUE;
			break;
		}
		
		break;	// not retry
		Sleep(180000);
	}

XF_EXIT:
	if(!pRegisterRequest->m_bIsClose)
		pRegisterRequest->m_pParent->SendMessage(WM_NET_MESSAGE
			, MAX_NET_COMMAND + 1, dwReturnCode);

	return dwReturnCode;
}


#pragma comment( exestr, "B9D3B8FD2A6B707667747067762B")
