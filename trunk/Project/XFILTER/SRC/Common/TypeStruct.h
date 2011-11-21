
#ifndef TYPESTRUCT_H
#define TYPESTRUCT_H

#include "..\common\XFileRes.h"


//=============================================================================================
// ACL file

typedef struct _XACL_HEADER
{
	TCHAR		sSignature[16];
	DWORD		ulHeaderLenth;
	BYTE		bMajor;
	BYTE		bMinor;
	UINT		uiSerial;
	DWORD		ulVersion;
	BYTE		bSet;
	UINT		uiLogSize;
	BYTE		bUpdateInterval;
	TCHAR		sUserName[16];
	TCHAR		sACode[16];
	TCHAR		sWebURL[MAX_PATH];
	TCHAR		sCommandURL[MAX_PATH];
	TCHAR		sUserRegisterURL[MAX_PATH];
	TCHAR		sEmail[MAX_PATH];
	DWORD		ulAclOffset;
	DWORD		ulAclCount;
	DWORD		ulIntranetIPOffset;
	DWORD		ulIntranetIPCount;
	DWORD		ulDistrustIPOffset;
	DWORD		ulDistrustIPCount;
	DWORD		ulTrustIPOffset;
	DWORD		ulTrustIPCount;
	DWORD		ulCustomIPOffset;
	DWORD		ulCustomIPCount;
	DWORD		ulTimeOffset;
	DWORD		ulTimeCount;
} XACL_HEADER, *PXACL_HEADER;

typedef struct _XACL
{
	DWORD		ulAclID;
	TCHAR		sApplication[MAX_PATH];
	BYTE		bRemoteNetType;
	BYTE		bAccessTimeType;
	BYTE		bAction;
	BYTE		bDirection;
	BYTE		bServiceType;
	UINT		uiServicePort;
	TCHAR		sMemo[51];
} XACL, *PXACL;

typedef struct _XACL_IP
{
	DWORD		ulStartIP;
	DWORD		ulEndIP;
} XACL_IP, *PXACL_IP;

typedef struct _XACL_TIME
{
	BYTE		bWeekDay;
	CTime		tStartTime;
	CTime		tEndTime;
} XACL_TIME, *PXACL_TIME;

typedef struct _XACL_FILE
{
	XACL_HEADER		mAclHeader;
	XACL_IP			mAclIntranetIP;
	XACL_TIME		mAclTime		[ACL_HEADER_TIME_COUNT];
	XACL_IP			mpAclDistrustIP	[MAX_IP_ARIA];
	XACL_IP			mpAclTrustIP	[MAX_IP_ARIA];
	XACL_IP			mpAclCustomIP	[MAX_IP_ARIA];
	XACL			mpAcl			[MAX_ACL];
} XACL_FILE, *PXACL_FILE;

//=============================================================================================
// the packet struct of TCPIPDOG Capture 

typedef struct _SESSION
{
	BYTE		bStatus;	// 0, free; 1, using, 2, write log and after set free
	BYTE		bType;		// 0, normal; 1, listen
	SOCKET		s;
	DWORD		ulRemoteIP;
	BYTE		bDirection;
	BYTE		bProtocol;
	UINT		uiPort;
	BYTE		bAction;
	UINT		uiLocalPort;
	CTime		tStartTime;
	CTime		tEndTime;
	DWORD		ulLocalIP;
	DWORD		ulSendData;
	DWORD		ulRecvData;
	TCHAR		sPathName[MAX_PATH];
	TCHAR		sMemo[MAX_PATH];
} SESSION, *PSESSION;

#define SESSION_LENTH		sizeof(SESSION)

//=============================================================================================
// the query session 

typedef struct _QUERY_SESSION
{
	BYTE		status;				//0: no use, 1: using
	TCHAR		sPathName[MAX_PATH];
	int			ReturnCode;
} QUERY_SESSION, *PQUERY_SESSION;

//=============================================================================================
// XFITLER Io Control struct

typedef BOOL (WINAPI * ADD_ACL_QUERY)(TCHAR* sPathName, BOOL EnableComboApplication = FALSE);


#endif