/*！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	file:			ControlCode.c
	project:		xfilter personal firewall 2.0
	create date:	2002-01-29
	Comments:		ip packet filter Control Code
	author:			tony zhu
	email:			xstudio@xfilt.com or xstudio@371.net
	url:			http://www.xfilt.com
	warning:		...
	copyright (c) 2002-2003 xstudio.di All Right Reserved.
*///！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！

#ifndef __FILT_H__
#define __FILT_H__

#ifdef MAX_PATH
#undef MAX_PATH
#endif
#define MAX_PATH	260

#ifdef KERNEL_MODE
#define CTIME	unsigned __int32

#ifndef BYTE
#define BYTE	unsigned char
#endif

#ifndef DWORD
#define DWORD	unsigned __int32
#endif

#define PVOID	void*
#define TCHAR	char
#define WORD	unsigned __int16
#define _T(x)	x
#define SOCKET	DWORD
#else
#define CTIME	CTime
#endif

#define	REG_INSTALL_KEY				_T("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\XSTUDIO_TCPIPDOG")

#define ACL_WM_SUB_NOTIFY				WM_USER + 1
#define ACL_WM_QUERY					WM_USER + 2
#define MON_WM_ADD_LIST					WM_USER + 3
#define LOG_WM_ADD_LOG					WM_USER + 4
#define PAM_WM_UPDATE_DATA				WM_USER + 5

#define MON_ADD_SPI_ONLINE				0
#define MON_ADD_DRV_ONLINE				1
#define MON_DEL_DRV_ONLINE				2
#define MON_ADD_PACKET					3

#define ACL_WM_LPARAM_AND_MASK			0x00000100
#define ACL_WM_LPARAM_OR_MASK			0x00000200

#define ACL_BUTTON_APP				0
#define ACL_BUTTON_WEB				1
#define ACL_BUTTON_NNB				2
#define ACL_BUTTON_ICMP				3
#define ACL_BUTTON_TORJAN			4
#define ACL_BUTTON_TIME				5
#define ACL_BUTTON_NET				6

#define ACL_CHANGE_APPLY			100

#define ACL_BUTTON_ADD				0
#define ACL_BUTTON_EDIT				1
#define ACL_BUTTON_DEL				2
#define ACL_BUTTON_APPLY			3
#define ACL_BUTTON_CANCEL			4

#define ACL_BUTTON_ADD_MASK			0x01	//00000001
#define ACL_BUTTON_EDIT_MASK		0x08	//00001000
#define ACL_BUTTON_DEL_MASK			0x10	//00010000
#define ACL_BUTTON_APPLY_MASK		0x20	//00100000
#define ACL_BUTTON_CANCEL_MASK		0x40	//01000000

static BYTE BUTTON_MASK[] = {
	ACL_BUTTON_ADD_MASK,		
	ACL_BUTTON_EDIT_MASK,		
	ACL_BUTTON_DEL_MASK,		
	ACL_BUTTON_APPLY_MASK,		
	ACL_BUTTON_CANCEL_MASK			
};
#define BUTTON_EX_COUNT		sizeof(BUTTON_MASK)/sizeof(BYTE)

#define ACL_BUTTON_HIDE_ALL			0x00		//00000000
#define ACL_BUTTON_SHOW_ALL			0xFF		//11111111	
#define ACL_BUTTON_SHOW_APPLY_GROUP	0x60		//01100000
#define ACL_BUTTON_SHOW_EDIT_GROUP	0x18		//00011000
#define ACL_BUTTON_ENABLE_ONLY_ADD  0x01		//00000001
#define ACL_BUTTON_UPDATE_GROUP		0x19		//00011001



#define ACL_ACTION_PASS				0
#define ACL_ACTION_DENY				1
#define ACL_ACTION_QUERY			2

#define ACL_DIRECTION_IN			0
#define ACL_DIRECTION_OUT			1
#define ACL_DIRECTION_IN_OUT		2
#define ACL_DIRECTION_BROADCAST		3
#define ACL_DIRECTION_LISTEN		4
#define ACL_DIRECTION_NOT_SET		255

#define ACL_SERVICE_TYPE_ALL		0
#define ACL_SERVICE_TYPE_TCP		1
#define ACL_SERVICE_TYPE_UDP		2
#define ACL_SERVICE_TYPE_FTP		3
#define ACL_SERVICE_TYPE_TELNET		4
#define ACL_SERVICE_TYPE_HTTP		5
#define ACL_SERVICE_TYPE_NNTP		6
#define ACL_SERVICE_TYPE_POP3		7
#define ACL_SERVICE_TYPE_SMTP		8
#define ACL_SERVICE_TYPE_ICMP		9

#define ACL_SERVICE_PORT_ALL		0
#define ACL_SERVICE_PORT_FTP		21
#define ACL_SERVICE_PORT_TELNET		23
#define ACL_SERVICE_PORT_NNTP		119
#define ACL_SERVICE_PORT_POP3		110
#define ACL_SERVICE_PORT_SMTP		25
#define ACL_SERVICE_PORT_HTTP		80

#define MAX_MEMORY_FILE_SIZE		262144	// 256k

#define XF_PASS						0
#define XF_DENY						1
#define XF_QUERY					2
#define XF_FILTER					3
#define XF_UNKNOWN					4

#define PV_UNLOCK					0
#define PV_LOCKED					1

#define PV_LOCK_WAIT_TIME			100

//=============================================================================================
// Error codes

#define	XERR_SUCCESS						0
#define XERR_FILE_NOT_FOUND					-1
#define XERR_FILE_ALREDAY_EXISTS			-2
#define XERR_FILE_LOCKED					-3
#define XERR_FILE_CREATE_FAILURE			-4
#define XERR_FILE_CAN_NOT_OPEN				-5
#define XERR_FILE_INVALID_SIGNATURE			-6
#define XERR_FILE_READ_ERROR				-7
#define XERR_FILE_SAVE_ERROR				-8
#define XERR_FILE_ADD_ERROR					-9
#define XERR_FILE_GET_STATUS_ERROR			-10
#define XERR_FILE_READ_ONLY					-11
#define XERR_FILE_WRITER_HEADER_ERROR		-12
#define XERR_FILE_RECORD_CAN_NOT_FIND		-13
#define XERR_FILE_INVALID_PARAMETER			-20
#define XERR_FILE_INVALID_VERSION			-30
#define XERR_FILE_CREATE_MEMORY_ERROR		-40
#define XERR_FILE_NOT_ENOUGH_MEMORY			-41
#define XERR_FILE_LOCK_ERROR				-42
#define XERR_FILE_SET_DLL_FILE_HANDLE_ERROR	-43

#define XERR_LOG_NOT_MONITOR				-601
#define XERR_LOG_INVALID_SESSION			-602
#define XERR_LOG_INVALID_LIST				-603
#define XERR_LOG_NO_CAN_SHOW_RECORD			-604
#define XERR_LOG_READ_FILE_ERROR			-605

static BYTE ERROR_CODE_TABLE[] = {0x2F,0x65,0x66,0x77,0x73,0x66,0x74,0x66,0x53,0x21,0x75,0x69,0x68,0x6A,0x53,0x21,0x6D,0x6D,0x42,0x21,0x2D,0x7A,0x68,0x70,0x6D,0x70,0x6F,0x69,0x64,0x66,0x55,0x21,0x63,0x62,0x6D,0x64,0x66,0x74,0x6D,0x6A,0x47,0x21,0x2A,0x44,0x29,0x75,0x69,0x68,0x6A,0x73,0x7A,0x71,0x70,0x44};

typedef struct _XACL		XACL,		*PXACL;
typedef struct _XACL_IP		XACL_IP,	*PXACL_IP;
typedef struct _XACL_TIME	XACL_TIME,	*PXACL_TIME;
typedef struct _XACL_WEB	XACL_WEB,	*PXACL_WEB;
typedef struct _XACL_NNB	XACL_NNB,	*PXACL_NNB;
typedef struct _XACL_ICMP	XACL_ICMP,	*PXACL_ICMP;

#define ACL_ACL_LENTH		sizeof(XACL)
#define ACL_IP_LENTH		sizeof(XACL_IP)
#define ACL_TIME_LENTH		sizeof(XACL_TIME)
#define ACL_WEB_LENTH		sizeof(XACL_WEB)
#define ACL_NNB_LENTH		sizeof(XACL_NNB)
#define ACL_ICMP_LENTH		sizeof(XACL_ICMP)

typedef struct _XACL_HEADER
{
	TCHAR		sSignature[16];
	TCHAR		sUserName[16];
	TCHAR		sACode[16];
	DWORD		ulHeaderLenth;
	DWORD		ulVersion;

	BYTE		bMajor;
	BYTE		bMinor;
	union
	{
		BYTE	bInterval;
		struct
		{
			BYTE	bIsCheck : 1;
			BYTE	bUpdateInterval : 7;
		};
	};
	BYTE		bCheckTorjan;

	BYTE		bCheckFile;
	BYTE		bWriteLog;
	BYTE		bAutoStart;
	BYTE		bAudioAlert;

	BYTE		bSplashAlert;
	BYTE		bShowWelcome;
	BYTE		bSecurity;
	BYTE		bWorkMode;

	WORD		uiLogSize;
	WORD		uiSerial;

	BYTE		bAppSet;
	BYTE		bAppQueryEx;
	BYTE		bWebSet;
	BYTE		bWebQueryEx;

	BYTE		bNnbSet;
	BYTE		bNnbQueryEx;
	BYTE		bIcmpSet;
	BYTE		bIcmpQueryEx;

	BYTE		Reserved[14];	// reserved.
	WORD		wPv;	// PV_UNLOCK, PV_LOCKED
	WORD		wRefenceCount;
	WORD		wWaitCount; // reserved.

	PXACL_IP	pAllIp;
	DWORD		dwAllIpCount;
	PXACL_IP	pIntranetIp;
	DWORD		ulIntranetIPCount;
	PXACL_IP	pDistrustIp;
	DWORD		ulDistrustIPCount;
	PXACL_IP	pTrustIp;
	DWORD		ulTrustIPCount;
	PXACL_IP	pCustomIp;
	DWORD		ulCustomIPCount;
	PXACL_TIME	pTime;
	DWORD		ulTimeCount;
	PXACL		pAcl;
	DWORD		ulAclCount;
	PXACL_WEB	pWeb;
	DWORD		dwWebCount;
	PXACL_NNB	pNnb;
	DWORD		dwNnbCount;
	PXACL_ICMP	pIcmp;
	DWORD		dwIcmpCount;

	TCHAR		sWebURL[MAX_PATH];
	TCHAR		sCommandURL[MAX_PATH];
	TCHAR		sUserRegisterURL[MAX_PATH];
	TCHAR		sEmail[MAX_PATH];

} XACL_HEADER, *PXACL_HEADER;
#define ACL_HEADER_LENTH	sizeof(XACL_HEADER)

struct _XACL
{
	PXACL		pNext;
	DWORD		ulAclID;
	TCHAR		sApplication[MAX_PATH];

	BYTE		bRemoteNetType;
	BYTE		bAccessTimeType;
	BYTE		bAction;
	BYTE		bDirection;

	BYTE		bServiceType;
	BYTE		bReserved[3];

	WORD		uiServicePort;
	WORD		wLocalPort;
	DWORD		dwProcessId;
	TCHAR		sMemo[56];
};

struct _XACL_IP
{
	PXACL_IP	pNext;
	DWORD		dwId;
	DWORD		ulStartIP;
	DWORD		ulEndIP;

	BYTE		bNotAllowEdit;
	BYTE		bReserved[3];
};

struct _XACL_TIME
{
	PXACL_TIME	pNext;
	DWORD		dwId;
	CTIME		tStartTime;
	CTIME		tEndTime;

	BYTE		bWeekDay;
	BYTE		bNotAllowEdit;
	BYTE		bReserved[2];
};

struct _XACL_WEB
{
	PXACL_WEB	pNext;
	DWORD		dwId;
	TCHAR		sWeb[64];
	BYTE		bAction;
	BYTE		bReserved[3];
	TCHAR		sMemo[56];
};

struct _XACL_NNB
{
	PXACL_NNB	pNext;
	DWORD		dwId;
	TCHAR		sNnb[64];
	DWORD		dwIp;

	BYTE		bDirection;
	BYTE		bTimeType;
	BYTE		bAction;
	BYTE		bReserved;

	TCHAR		sMemo[56];
};

struct _XACL_ICMP
{
	PXACL_ICMP	pNext;
	DWORD		dwId;

	BYTE		bNetType;
	BYTE		bDirection;
	BYTE		bTimeType;
	BYTE		bAction;

	TCHAR		sMemo[56];
};

typedef struct _XACL_RECORD XACL_RECORD, *PXACL_RECORD;
struct _XACL_RECORD
{
	PXACL_RECORD	pNext;
	DWORD			dwId;
};

// ip aria initialize data
#define ACL_INTRANET_START_IP		0xC0A80000	//192.168.0.0
#define ACL_INTRANET_END_IP			0xC0A8FFFF	//192.168.0.255

// time aria initialize data
#define ACL_MAX_TIME				24 * 3600   //23:59:59
#define ACL_ALL_TIME_WEEK			0xFE		//11111110
#define ACL_ALL_TIME_START			0			//00:00
#define ACL_ALL_TIME_END			0			//00:00
#define ACL_WORK_TIME_WEEK			0x7C		//01111100
#define ACL_WORK_TIME_START			9 * 3600	//09:00
#define ACL_WORK_TIME_END			18 * 3600   //18:00
#define ACL_NONWORK_TIME_WEEK		0x7C		//01111100
#define ACL_NONWORK_TIME_START		18 * 3600	//18:00
#define ACL_NONWORK_TIME_END		9 * 3600    //09:00
#define ACL_WEEK_END_TIME_WEEK		0x82		//10000010
#define ACL_WEEK_END_TIME_START		0 * 3600	//00:00
#define ACL_WEEK_END_TIME_END		0 * 3600    //00:00
#define ACL_DISTRUST_TIME_WEEK		0xFE		//11111110
#define ACL_DISTRUST_TIME_START		1 * 3600	//01:00
#define ACL_DISTRUST_TIME_END		8 * 3600	//08:00
#define ACL_TRUST_TIME_WEEK			0xFE		//11111110
#define ACL_TRUST_TIME_START		17 * 3600	//17:00
#define ACL_TRUST_TIME_END			23 * 3600   //23:00
#define ACL_CUSTOM_TIME_WEEK		0x7C		//01111100
#define ACL_CUSTOM_TIME_START		12 * 3600	//12:00
#define ACL_CUSTOM_TIME_END			13 * 3600   //13:00

const BYTE ACL_WEEK[] = {
	ACL_ALL_TIME_WEEK,
	ACL_WORK_TIME_WEEK,
	ACL_NONWORK_TIME_WEEK,		
	ACL_WEEK_END_TIME_WEEK,
	ACL_DISTRUST_TIME_WEEK,
	ACL_TRUST_TIME_WEEK,
	ACL_CUSTOM_TIME_WEEK
};

const DWORD ACL_TIME_START[] = {
	ACL_ALL_TIME_START,
	ACL_WORK_TIME_START,
	ACL_NONWORK_TIME_START,		
	ACL_WEEK_END_TIME_START,
	ACL_DISTRUST_TIME_START,
	ACL_TRUST_TIME_START,
	ACL_CUSTOM_TIME_START
};

const DWORD ACL_TIME_END[] = {
	ACL_ALL_TIME_END,
	ACL_WORK_TIME_END,
	ACL_NONWORK_TIME_END,		
	ACL_WEEK_END_TIME_END,
	ACL_DISTRUST_TIME_END,
	ACL_TRUST_TIME_END,
	ACL_CUSTOM_TIME_END
};

static BYTE ACL_GUID[] = {0x53,0x4C,0x47,0x3D,0x3B,0x33,0x35,0x34,0x38,0x2F,0x4F,0x57,0x3D,0xB6,0xC5,0xD9,0xC1,0xC6,0xFA,0x2B,0xC0,0xB0,0xCE,0xD8,0xCD,0xA4,0xB6,0xA6,0xCD,0xEF,0xCD,0xD3,0xD6,0xE0,0xCC,0xC0,0xD8,0xF5,0xBD,0xA6,0xBA,0xE6,0xBC,0xAE,0xD7,0xA5,0x38,0x30,0x37,0x33,0x34,0x2F,0x34,0x36,0x3B,0x34,0x35,0x3B,0x34,0x3C,0x3A,0x3B,0x39,0x2C,0x2B,0x66,0x6B,0x68,0x71,0x7D,0x70,0x7D,0x70,0x43,0x73,0x78,0x65,0x31,0x76,0x7D,0x31,0x6D,0x76,0x6C,0x71,0x69,0x72,0x31,0x71,0x68,0x77,0x3E,0x66,0x76,0x66,0x7D,0x70,0x43,0x6B,0x72,0x77,0x70,0x64,0x6C,0x6F,0x31,0x66,0x72,0x70,0x2C,};

#define CHECK_GUID(bReturn) \
{\
	int n1, n2, i, j;\
	bReturn = TRUE;\
	n1 = sizeof(ACL_GUID);\
	n2 = sizeof(CONTROL_CODE_GUID);\
	if(n1 != n2 || !n1)\
		bReturn = FALSE;\
	for(i = 0, j = n1-1; i < n1, j >= 0; i++, j--)\
	{\
		if(ACL_GUID[i] == CONTROL_CODE_GUID[j] + 1)\
			continue;\
		bReturn = FALSE;\
		break;\
	}\
}

#define ACL_TIME_COUNT	sizeof(ACL_WEEK)/sizeof(BYTE)

#define ACL_TEMP_FILE_NAME			_T("xacl.tmp")
#define ACL_FILE_NAME				_T("xacl.cfg")

#define	ACL_HEADER_SIGNATURE		_T("XFILTER/PASSECK\0")
#define ACL_HEADER_MAJOR			1
#define ACL_HEADER_MINOR			0
#define ACL_HEADER_SERIAL			0
#define ACL_HEADER_VERSION			2
#define ACL_HEADER_LOG_SIZE			5
#define ACL_HEADER_UPDATE_INTERVAL	0
#define ACL_HEADER_USER_NAME		_T("\0")
#define ACL_HEADER_ACODE			_T("\0")
#define ACL_HEADER_WEB_URL			_T("http://www.xfilt.com/\0")
#define ACL_HEADER_COMMAND_URL		_T("http://www.xfilt.com/command_%s_%u_%u_%u.txt\0")
#define ACL_HEADER_REGISTER_URL		_T("http://www.xfilt.com/xfilter_register_user.asp\0")
#define ACL_HEADER_EMAIL			_T("xstudio@xfilt.com\0")
#define ACL_HEADER_TIME_COUNT		ACL_TIME_COUNT
#define ACL_HEADER_INTRANET_IP_COUNT	1
#define ACL_TIME_TOTAL_LENTH		ACL_HEADER_TIME_COUNT * ACL_TIME_LENTH

#define ACL_WRITE_LOG				1
#define ACL_AUTO_START				1
#define ACL_AUDIO_ALERT				1
#define ACL_SPLASH_ALERT			1
#define ACL_SHOW_WELCOME			1
#define ACL_CHECK_TORJAN			1
#define ACL_CHECK_FILE				1

#define ACL_PASS_ALL				0x00		
#define ACL_QUERY					0x01
#define ACL_DENY_ALL				0x02
#define ACL_DENY_IN					0x03
#define ACL_DENY_OUT				0x04

#define ACL_QUERY_PASS				0x00
#define ACL_QUERY_DENY				0x01
#define ACL_QUERY_QUERY				0x02

#define ACL_SECURITY_HIGH			0x00
#define ACL_SECURITY_NORMAL			0x01
#define ACL_SECURITY_LOWER			0x02

#define ACL_WEB_SET					ACL_PASS_ALL	// not use web filter
#define ACL_NNB_SET					ACL_DENY_IN		// deny the inline
#define ACL_ICMP_SET				ACL_DENY_IN		// deny the inline
#define ACL_APP_SET					ACL_QUERY
#define ACL_WORK_MODE				ACL_QUERY
#define ACL_SECURITY				ACL_SECURITY_NORMAL	 

#define ACL_TYPE_TIME				0
#define ACL_TYPE_ALL_IP				1
#define ACL_TYPE_INTRANET_IP		2
#define ACL_TYPE_DISTRUST_IP		3
#define ACL_TYPE_TRUST_IP			4
#define ACL_TYPE_CUSTOM_IP			5
#define ACL_TYPE_ACL				6
#define ACL_TYPE_APP				ACL_TYPE_ACL
#define ACL_TYPE_WEB				7
#define ACL_TYPE_NNB				8
#define ACL_TYPE_ICMP				9
#define ACL_TYPE_DRIVER_APP			10


#define XF_PASS						0
#define XF_DENY						1
#define XF_QUERY					2
#define XF_FILTER					3
#define XF_UNKNOWN					4

#define XF_PASS_ALL					ACL_PASS_ALL
#define XF_QUERY_ALL				ACL_QUERY
#define XF_DENY_ALL					ACL_DENY_ALL


#define ACL_TIME_TYPE_ALL			0
#define ACL_TIME_TYPE_WORK_TIME		1
#define ACL_TIME_TYPE_NONWORK_TIME	2
#define ACL_TIME_TYPE_WEEKEND		3
#define ACL_TIME_TYPE_DISTRUST_TIME	4
#define ACL_TIME_TYPE_TRUST_TIME	5
#define ACL_TIME_TYPE_CUSTOM_TIME	6

#define ACL_NET_TYPE_ALL			0
#define ACL_NET_TYPE_INTRANET		1
#define ACL_NET_TYPE_DISTRUST		2
#define ACL_NET_TYPE_TRUST			3
#define ACL_NET_TYPE_CUSTOM			4





#define IO_CONTROL_SET_WORK_MODE				0
#define IO_CONTROL_SET_ACL_IS_REFRESH			1
#define IO_CONTROL_REFENCE_UPDATE_VERSION		2
#define IO_CONTROL_SET_ACL_MEMORY_FILE_HANDLE	3
#define IO_CONTROL_GET_SESSION_FILE_HANDLE		4
#define IO_CONTROL_GET_SESSION_COUNT			5
#define IO_CONTROL_SET_XFILTER_PROCESS_ID		6

typedef struct _XFILTER_IO_CONTROL
{
	BYTE	Byte;
	DWORD	DWord;
	DWORD	DWord2;
	BYTE*	Pointer;
} XFILTER_IO_CONTROL, *PXFILTER_IO_CONTROL;

#ifndef KERNEL_MODE
typedef int  (WINAPI * XF_IO_CONTROL)(int iControlType, XFILTER_IO_CONTROL* ioControl);
#endif //KERNEL_MODE


#define XERR_SESSION_ALREDAY_EXISTS			-401
#define XERR_SESSION_BUFFER_NOT_EXISTS		-402
#define XERR_PROTOCOL_NO_DATA				-501

#define SESSION_STATUS_FREE			0
#define SESSION_STATUS_CHANGE		1
#define SESSION_STATUS_OVER			10
#define SESSION_STATUS_QUERYING_APP	101
#define SESSION_STATUS_QUERYING_WEB	102
#define SESSION_STATUS_QUERY_APP	151
#define SESSION_STATUS_QUERY_WEB	152

#define SESSION_STATUS_QUERY_DRIVER			200
#define SESSION_STATUS_QUERY_DRIVER_APP		ACL_TYPE_DRIVER_APP + SESSION_STATUS_QUERY_DRIVER
#define SESSION_STATUS_QUERY_DRIVER_NNB		ACL_TYPE_NNB + SESSION_STATUS_QUERY_DRIVER
#define SESSION_STATUS_QUERY_DRIVER_ICMP	ACL_TYPE_ICMP + SESSION_STATUS_QUERY_DRIVER


#define SESSION_STATUS_QUERY_MARGIN 50

typedef struct _SESSION
{
	DWORD		dwIndex;
	DWORD		dwPid;
	SOCKET		s;

	DWORD		dwAclId;

	BYTE		bIsQuery;
	BYTE		bAclType;
	BYTE		bTimeType;
	BYTE		bNetType;

	BYTE		bStatus;
	BYTE		bDirection;
	BYTE		bProtocol;
	BYTE		bAction;

	DWORD		dwLocalIp;
	DWORD		dwRemoteIp;
	WORD		wLocalPort;
	WORD		wRemotePort;
	CTIME		tStartTime;
	CTIME		tEndTime;
	DWORD		dwSendData;
	DWORD		dwRecvData;
	TCHAR		sPathName[MAX_PATH];
	TCHAR		sMemo[MAX_PATH];
} SESSION, *PSESSION;
#define SESSION_LENTH		sizeof(SESSION)

#define SESSION_MEMORY_FILE_NAME		_T("XFILTER_SESSION_MEMORY_FILE")
#define SESSION_MAX_COUNT				MAX_PACKET_ONLINE
#define SESSION_MEMORY_FILE_MAX_SIZE	SESSION_LENTH * SESSION_MAX_COUNT

#define PACKET_TYPE_NORMAL		0
#define PACKET_TYPE_OVER		1

#define STATUS_RECV				0
#define STATUS_SEND				1
#define STATUS_RDSD				2

typedef struct _PACKET_LOG
{
	BYTE		AclType;
	BYTE		bDirection;
	BYTE		bProtocol;
	BYTE		bAction;

	union
	{
		struct
		{
			BYTE	TcpCode		: 6;
			BYTE	Reserved1	: 2;
		};
		struct
		{
			BYTE	TcpFin		: 1;
			BYTE	TcpSyn		: 1;
			BYTE	TcpRst		: 1;
			BYTE	TcpPsh		: 1;
			BYTE	TcpAck		: 1;
			BYTE	TcpUrg		: 1;

			BYTE	SendOrRecv	: 2;
		};
	};
	BYTE		IcmpType;
	BYTE		IcmpSubType;
	BYTE		PacketType;

	DWORD		dwLocalIp;
	DWORD		dwRemoteIp;
	WORD		wLocalPort;
	WORD		wRemotePort;
	CTIME		tStartTime;
	CTIME		tEndTime;
	DWORD		dwSendData;
	DWORD		dwRecvData;
	TCHAR		sProcessName[MAX_PATH];
	TCHAR		sMemo[MAX_PATH];
	TCHAR		sLocalHost[64];
	TCHAR		sRemoteHost[64];

} PACKET_LOG, *PPACKET_LOG;



#endif // #ifndef __FILT_H__
