//=============================================================================================
/*
	Internet.h
	Internet Transfer operator.

	Project	: XFILTER 1.0
	Author	: Tony Zhu
	Create Date	: 2001/08/26
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================
#ifndef INTERNET_H
#define INTERNET_H

#include <ras.h>
#include <afxinet.h>

#define WM_NET_MESSAGE					WM_USER + 6

#define PRODUCT_ID						_T("80215")
#define REGISTER_URL					_T("http://test/user_register_2_0_1.asp")
#define NET_COMMAND_URL					_T("http://test/Command_2_0_6.txt")

//=============================================================================================
// Max Values

#define MAX_NET_COMMAND					20
#define MAX_NET_COMMAND_LENTH			512
#define MAX_NET_COMMAND_VERSION_LENTH	10
#define MAX_NET_COMMAND_COMMAND_LENTH	1
#define MAX_NET_MESSAGE_LENTH			MAX_NET_COMMAND_LENTH - MAX_NET_COMMAND_VERSION_LENTH - 2

#define NET_COMMAND_CHANGE_WEB_STATION_URL		1
#define NET_COMMAND_CHANGE_NET_COMMAND_URL		2
#define NET_COMMAND_CHANGE_USER_REGISTER_URL	3
#define NET_COMMAND_CHANGE_EMAIL_ADDRESS		4
#define NET_COMMAND_CHANGE_UPDATE_INTERVAL_DAYS	5
#define NET_COMMAND_CHANGE_POST_MESSAGE			6

#define NET_COMMAND_NO_NEW_VERSION				0x00
#define NET_COMMAND_HAVE_MESSAGE				0x01
#define NET_COMMAND_HAVE_NEW_VERSION			0x02
#define NET_UPLOAD_SUCCESS						0x03


//=============================================================================================
// User Register

#define REG_STATUS_NO_REGISTER		0
#define REG_STATUS_REGISTERED		1
#define REG_STATUS_REGISTERING		2
#define REG_STATUS_INFO_CHANGED		3

#define XERR_SUCCESS						0
#define XERR_INVALID_PARAMETER				-101
#define XERR_INTERNET_URL_ERROR				-701
#define XERR_INTERNET_CONNECT_ERROR			-702
#define XERR_INTERNET_REQUEST_ERROR			-703
#define XERR_INTERNET_SERVER_ERROR			-704
#define XERR_INTERNET_REG_ERROR				-705
#define XERR_CURRENT_NOT_ONLINE				-706
#define XERR_REGISTER_DLG_CANCEL			-707
#define XERR_ALREDAY_REGISTERED				-708
#define XERR_UPDATE_INTERVAL_INVALID		-709
#define XERR_STATUS_PENDING					-710

typedef struct _XUSER_INFO
{
	TCHAR		sEmail[51];
	TCHAR		sUserName[21];
	TCHAR		sPassword[21];
	TCHAR		sQQ[13];
	TCHAR		sICQ[13];
	TCHAR		sName[21];
	BYTE		bIdType;
	TCHAR		sId[21];
	int			iStatus;
	BYTE		bGender;
	BYTE		bMetier;
	TCHAR		sDuty[21];
	BYTE		bDegree;
	BYTE		bSalary;
	CTime		tBirthday;
	WORD 		wCountry;
	WORD 		wCity;
	TCHAR		sAddress[51];
	TCHAR		sZip[11];
	TCHAR		sInc[51];

	TCHAR		sOffenToWeb[51];
	BYTE		bCheck[14];
	BYTE		bCombo[9];
	TCHAR		sCity[21];
	TCHAR		Recommender[51];
} XUSER_INFO, *PXUSER_INFO;

typedef struct _XNET_COMMAND_HEADER
{
	long		lVersion;
	long		lCount;
	CTime		tCheckTime;
} XNET_COMMAND_HEADER, *PXNET_COMMAND_HEADER;

class CHttpRequest
{
public:
	CHttpRequest(CWnd* pParent = NULL);
	virtual ~CHttpRequest();

	void SetParent(CWnd* pParent){m_pParent = pParent;}
	void SetVersion(DWORD dwVersion, BYTE bMajor = 0, BYTE bMinor = 0);
	BOOL IsRegistered();
	int Register(BOOL IsSyn = TRUE);
	int DownloadNetCommand(BYTE bUpdateInterval, BOOL IsSyn = TRUE);
	void SetIsEdit(BYTE bIsEdit){m_bIsEdit = bIsEdit;}
	BYTE GetIsEdit(){return m_bIsEdit;}

	BOOL StopDownload();

public:
	int	 ConnectUrl(TCHAR *sUrl, TCHAR *sReturn = NULL, long *lVersion = 0, int *Count = NULL);
	BOOL IsConnected();
	void Close();
	void SetRegisterUrl();
	BOOL PreUpload(BOOL IsSyn);
	BOOL UserReg();

public:
	BYTE				m_bIsEdit;
	BYTE				m_bIsClose;
	XUSER_INFO			m_UserInfo;
	TCHAR				m_pUrlRequest[4096];
	XNET_COMMAND_HEADER	m_CommandHeader;
	long				*lCommandId;
	HANDLE				m_DownloadThread;
	HANDLE				m_UploadThread;
	BOOL				m_IsUploaded;
	BOOL				m_IsConnecting;
	CXInstall			m_Install;
	TCHAR				m_sMessage[MAX_NET_COMMAND][MAX_NET_MESSAGE_LENTH];
	CWnd*				m_pParent;
	BOOL				m_IsSyn;
	CString				m_sVersion;
	char				m_sNewVersion[16];

private: 
	DWORD				m_dwVersion;
	BYTE				m_bMajor;
	BYTE				m_bMinor;
};

DWORD WINAPI UploadUserInfo(LPVOID pVoid);
DWORD WINAPI DownloadCommandFile(LPVOID pVoid);

#endif

