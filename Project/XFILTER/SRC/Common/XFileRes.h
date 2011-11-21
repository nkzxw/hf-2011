//=============================================================================================
/*
	XFileRes.h
	The Xfilter Resource

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/04
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================

#ifndef XFILERES_H
#define XFILERES_H

//=============================================================================================
// Max Values

#define MAX_ACL							100
#define MAX_IP_ARIA						10
#define MAX_SESSION_BUFFER				100
#define MAX_QUERY_SESSION				20
#define MAX_NET_COMMAND					20
#define MAX_NET_COMMAND_LENTH			512
#define MAX_NET_COMMAND_VERSION_LENTH	10
#define MAX_NET_COMMAND_COMMAND_LENTH	1
#define MAX_PROTOCOL_CATALOG_LENTH		sizeof(WSAPROTOCOL_INFOW) + MAX_PATH
#define MAX_NET_MESSAGE_LENTH			MAX_NET_COMMAND_LENTH - MAX_NET_COMMAND_VERSION_LENTH - 2

//=============================================================================================
// Net Command

#define NET_COMMAND_CHANGE_WEB_STATION_URL		1
#define NET_COMMAND_CHANGE_NET_COMMAND_URL		2
#define NET_COMMAND_CHANGE_USER_REGISTER_URL	3
#define NET_COMMAND_CHANGE_EMAIL_ADDRESS		4
#define NET_COMMAND_CHANGE_UPDATE_INTERVAL_DAYS	5
#define NET_COMMAND_CHANGE_POST_MESSAGE			6

//=============================================================================================
// User Register

#define REG_STATUS_NO_REGISTER		0
#define REG_STATUS_REGISTERED		1
#define REG_STATUS_REGISTERING		2
#define REG_STATUS_INFO_CHANGED		3

#define	REG_AUTO_START_KEY			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")
#define REG_AUTO_START_ITEM			_T("XFILTER")
#define	REG_PROTOCOL_CATALOG_KEY	_T("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\Protocol_Catalog9\\Catalog_Entries")
#define REG_PROTOCOL_CATALOG_ITEM	_T("PackedCatalogItem")
#define	REG_INSTALL_KEY				_T("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\XSTUDIO_TCPIPDOG")
#define REG_INSTALL_PATH_ITEM		_T("PathName")
#define REG_INFO_ITEM				_T("RegInfo")
#define REG_NET_COMMAND_HEADER_ITEM	_T("NetCommandHeader")
#define REG_NET_COMMAND_ITEM		_T("NetCommand")

//=============================================================================================
// Log file

#define LOG_FILE_NAME				_T("xlog.dat")

#define LOG_QUERY_PAGE_SIZE			500

//=============================================================================================
// Message

#define	WM_ICON_NOTIFY				WM_USER + 10
#define WM_SESSION_NOTIFY			WM_USER + 11
#define WM_QUERY_ACL_NOTIFY			WM_USER + 12
#define WM_NET_MESSAGE				WM_USER + 13

//=============================================================================================
// ACL file

#define XFILTER_SERVICE_DLL_NAME	_T("XFILTER.DLL")
#define XFILTER_HELP_FILE_NAME		_T("XFILTER.CHM")
#define XFILTER_PRODUCT_ID			0xA003

#define ACL_TEMP_FILE_NAME			_T("xacl.tmp")
#define ACL_FILE_NAME				_T("xacl.cfg")
#define ACL_ACL_LENTH				sizeof(XACL)
#define ACL_TIME_LENTH				sizeof(XACL_TIME)
#define ACL_IP_LENTH				sizeof(XACL_IP)
#define ACL_HEADER_LENTH			sizeof(XACL_HEADER)
#define	ACL_HEADER_SIGNATURE		_T("XFILTER/XSTUDIO\0")
#define ACL_HEADER_MAJOR			0
#define ACL_HEADER_MINOR			2
#define ACL_HEADER_SERIAL			0xA003
#define ACL_HEADER_VERSION			1
#define ACL_HEADER_SET				0xF6	//11110110
#define ACL_HEADER_LOG_SIZE			5
#define ACL_HEADER_UPDATE_INTERVAL	0
#define ACL_HEADER_USER_NAME		_T("\0")
#define ACL_HEADER_ACODE			_T("\0")
#define ACL_HEADER_WEB_URL			_T("http://www.xfilt.com/\0")
#define ACL_HEADER_COMMAND_URL		_T("http://www.xfilt.com/command_%s_%u_%u_%u.txt\0")
#define ACL_HEADER_REGISTER_URL		_T("http://www.xfilt.com/xfilter_register_user.asp\0")
#define ACL_HEADER_EMAIL			_T("xstudio@xfilt.com\0")
#define ACL_HEADER_TIME_COUNT		6
#define ACL_HEADER_INTRANET_IP_COUNT	1
#define ACL_HEADER_INITIALIZE		0
#define ACL_TIME_TOTAL_LENTH		ACL_HEADER_TIME_COUNT * ACL_TIME_LENTH

// ip aria initialize data
#define ACL_INTRANET_START_IP		0xC0A80A00	//192.168.10.0
#define ACL_INTRANET_END_IP			0xC0A80AFF	//192.168.10.255

// time aria initialize data
#define ACL_MAX_TIME				24 * 3600   //23:59:59
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
#define XERR_GET_FILE_STATUS_ERROR			-10
#define XERR_FILE_READ_ONLY					-11
#define XERR_FILE_WRITER_HEADER_ERROR		-12
#define XERR_FILE_RECORD_CAN_NOT_FIND		-13

#define XERR_INVALID_PARAMETER				-101
#define XERR_ACCESS_INVALID_PROCESS			-201
#define XERR_CREATE_FILE_MAPPING_ERROR		-301
#define XERR_SESSION_ALREDAY_EXISTS			-401
#define XERR_SESSION_NOT_EXISTS				-402
#define XERR_PROTOCOL_NO_DATA				-501
#define XERR_LOG_NOT_MONITOR				-601
#define XERR_LOG_INVALID_SESSION			-602
#define XERR_LOG_INVALID_LIST				-603
#define XERR_LOG_NO_CAN_SHOW_RECORD			-604
#define XERR_LOG_READ_FILE_ERROR			-605
#define XERR_INTERNET_URL_ERROR				-701
#define XERR_INTERNET_CONNECT_ERROR			-702
#define XERR_INTERNET_REQUEST_ERROR			-703
#define XERR_INTERNET_SERVER_ERROR			-704
#define XERR_INTERNET_REG_ERROR				-705
#define XERR_PROVIDER_NOT_INSTALL			-801
#define XERR_PROVIDER_ALREADY_INSTALL		-802
#define XERR_PROVIDER_OPEN_REG_FAILED		-803
#define XERR_PROVIDER_SAVE_PATH_FAILED		-804
#define XERR_PROVIDER_READ_VALUE_FAILED		-805
#define XERR_PROVIDER_CREATE_ITEM_FAILED	-806
#define XERR_PROVIDER_SET_VALUE_FAILED		-807
#define XERR_PROVIDER_REG_DELETE_FAILED		-808

//=============================================================================================
// Io Control codes

#define IO_CONTROL_SET_WORK_MODE			0
#define IO_CONTROL_SET_ACL					1
#define IO_CONTROL_GET_ACL_CHANGE_COUNT		2
#define IO_CONTROL_SET_GUI_INSTANCE			3
#define IO_CONTROL_GET_SESSION				4
#define IO_CONTROL_GET_QUERY_SESSION		5
#define IO_CONTROL_SET_QUERY_SESSION		6
#define IO_CONTROL_GET_WORK_MODE			7
#define IO_CONTROL_SET_MEMORY_FILE			8
#define INIT_ACL_CHANGE_COUNT				1000

//=============================================================================================
// Check Access info codes

#define XF_INVALID_PROCESS			_T("")

#define XF_PASS						0
#define XF_DENY						1
#define XF_QUERY					2
#define XF_FILTER					3
#define XF_UNKNOWN					4

#define XF_PASS_ALL					0
#define XF_QUERY_ALL				1
#define XF_DENY_ALL					2

//=============================================================================================
// other acl const

#define ACL_TYPE_ACL				0
#define ACL_TYPE_INTRANET_IP		1
#define ACL_TYPE_DISTRUST_IP		2
#define ACL_TYPE_TRUST_IP			3
#define ACL_TYPE_CUSTOM_IP			4

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

#define ACL_ACTION_PASS				0
#define ACL_ACTION_DENY				1
#define ACL_ACTION_QUERY			2

#define ACL_DIRECTION_IN			0
#define ACL_DIRECTION_OUT			1
#define ACL_DIRECTION_IN_OUT		2
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

#define ACL_SERVICE_PORT_ALL		0
#define ACL_SERVICE_PORT_FTP		21
#define ACL_SERVICE_PORT_TELNET		23
#define ACL_SERVICE_PORT_NNTP		119
#define ACL_SERVICE_PORT_POP3		110
#define ACL_SERVICE_PORT_SMTP		25
#define ACL_SERVICE_PORT_HTTP		80

#endif