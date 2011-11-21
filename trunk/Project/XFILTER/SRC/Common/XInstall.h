//=============================================================================================
/*
	XInstall.h
	Install XFILTER Winscok 2 base service provider hook

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/28
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================
#ifndef XINSTALL_H
#define XINSTALL_H

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
#define REG_INSTALL_VERSION_ITEM	_T("Version")
#define REG_INFO_ITEM				_T("RegInfo")
#define REG_NET_COMMAND_HEADER_ITEM	_T("NetCommandHeader")
#define REG_NET_COMMAND_ITEM		_T("NetCommand")

#define XERR_PROVIDER_NOT_INSTALL			-801
#define XERR_PROVIDER_ALREADY_INSTALL		-802
#define XERR_PROVIDER_OPEN_REG_FAILED		-803
#define XERR_PROVIDER_SAVE_PATH_FAILED		-804
#define XERR_PROVIDER_READ_VALUE_FAILED		-805
#define XERR_PROVIDER_CREATE_ITEM_FAILED	-806
#define XERR_PROVIDER_SET_VALUE_FAILED		-807
#define XERR_PROVIDER_REG_DELETE_FAILED		-808

#define MAX_PROTOCOL_CATALOG_LENTH		sizeof(WSAPROTOCOL_INFOW) + MAX_PATH

class CXInstall
{
private:
	int EnumHookKey(BOOL IsRemove = FALSE, BOOL IsCompare = FALSE);
	int SaveHookKey(HKEY hkey, LPCTSTR sSubKey, BOOL IsRemove = FALSE, BOOL IsCompare = FALSE);

public:
	BOOL	IsWinsock2();
	BOOL	IsRightVersion();
	BOOL	IsInstalled(TCHAR *sPathName = NULL);
	int		InstallProvider(TCHAR *sPathName);
	int		RemoveProvider();
	void	SetAutoStart(BOOL IsAutoStart);
	BOOL	UpdateInstall();

public:
	BOOL ReadReg(
		TCHAR	*sKey,
		BYTE	*pBuffer,
		DWORD	dwBufSize, 
		HKEY	hkey	= HKEY_LOCAL_MACHINE, 
		TCHAR	*sSubKey = REG_INSTALL_KEY,
		DWORD	ulType	= REG_BINARY
		);

	BOOL SaveReg(
		TCHAR	*sKey, 
		BYTE	*pBuffer, 
		DWORD	dwBufSize, 
		HKEY	hkey	= HKEY_LOCAL_MACHINE, 
		TCHAR	*sSubKey = REG_INSTALL_KEY,	
		DWORD	ulType	= REG_BINARY
		);

	BOOL DeleteReg(
		HKEY	hkey	= HKEY_LOCAL_MACHINE,
		TCHAR	*sSubKey = REG_INSTALL_KEY, 
		TCHAR	*sItem	= NULL
		);

public:
	TCHAR m_sPathName[MAX_PATH];
};

#endif