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
// 安装类 XInstall.cpp
//
//

#include "stdafx.h"
#include "XInstall.h"
#include <ws2spi.h>

//
// 判断是否安装了Winsock 2
//
BOOL CXInstall::IsWinsock2()
{
	WORD	wVersionRequested	= MAKEWORD(2, 0);
	WSADATA wsaData;
	
	if(WSAStartup(wVersionRequested, &wsaData) != 0)
		return FALSE;
 
	if (LOBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		return FALSE; 
	}

	return TRUE;
}

//
// 判断是否已经安装过XFILTER
//
BOOL CXInstall::IsInstalled(TCHAR *sPathName)
{
	TCHAR tsPathName[MAX_PATH];

	if( ReadReg(REG_INSTALL_PATH_ITEM, 
				(BYTE*)tsPathName, 
				MAX_PATH, 
				HKEY_LOCAL_MACHINE, 
				REG_INSTALL_KEY, REG_SZ
				)
	)
	{
		if(sPathName != NULL)
			_tcscpy(sPathName, tsPathName);
		return TRUE;
	}

	return FALSE;
}

//
// 判断安装的版本与当前版本是否相符合
//
BOOL CXInstall::IsRightVersion()
{
	char tsVersion[16];
	if( ReadReg(REG_INSTALL_VERSION_ITEM, 
				(BYTE*)tsVersion, 
				MAX_PATH, 
				HKEY_LOCAL_MACHINE, 
				REG_INSTALL_KEY, REG_SZ
				)
	)
	{
		char sVersion[16];
		sprintf(sVersion, "%u.%u.%u", ACL_HEADER_VERSION, ACL_HEADER_MAJOR, ACL_HEADER_MINOR);
		if(strcmp(tsVersion, sVersion) != 0)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

//
// 安装XFITLER.DLL
//
int CXInstall::InstallProvider(TCHAR *sPathName)
{
	if(IsInstalled())
	{
		if(!IsRightVersion())
			RemoveProvider();
		else
			return XERR_PROVIDER_ALREADY_INSTALL;
	}

	_tcscpy(m_sPathName, sPathName);

	int iRet;
	if((iRet = EnumHookKey()) != XERR_SUCCESS)
		return iRet;

	if(!SaveReg(
			REG_INSTALL_PATH_ITEM, 
			(BYTE*)sPathName, 
			_tcslen(sPathName),
			HKEY_LOCAL_MACHINE, 
			REG_INSTALL_KEY, 
			REG_SZ
			)
		)
		return XERR_PROVIDER_SAVE_PATH_FAILED;

	sprintf(sPathName, "%u.%u.%u", ACL_HEADER_VERSION, ACL_HEADER_MAJOR, ACL_HEADER_MINOR);
	if(!SaveReg(
			REG_INSTALL_VERSION_ITEM, 
			(BYTE*)sPathName, 
			_tcslen(sPathName),
			HKEY_LOCAL_MACHINE, 
			REG_INSTALL_KEY, 
			REG_SZ
			)
		)
		return XERR_PROVIDER_SAVE_PATH_FAILED;

	return XERR_SUCCESS;
}

//
// 卸载XFITLER.DLL
//
BOOL CXInstall::RemoveProvider()
{
	int iRet = XERR_SUCCESS;

	if(!IsInstalled())
		return XERR_PROVIDER_NOT_INSTALL;

	if(iRet = EnumHookKey(TRUE) != XERR_SUCCESS)
		return iRet;

	if(!DeleteReg())
		return XERR_PROVIDER_REG_DELETE_FAILED;

	return XERR_SUCCESS;
}

//
// 2002/06/10 add
// 检查SPI配置的注册表是否被修改，如果被修改，重新修改过来
// Windows XP 下 SPI 系统配置有时会自动恢复，所以在每次启动
// 和关闭时用这个函数校验一下。
//
BOOL CXInstall::UpdateInstall()
{
	//
	// 2002/08/21 add
	//
	if(GetWindowsVersion() != WINDOWS_VERSION_XP)
		return FALSE;

	if(!IsInstalled())
		return FALSE;
	if(EnumHookKey(FALSE, TRUE) != XERR_SUCCESS)
		return FALSE;
	return TRUE;
	
}

//=============================================================================================
// private install function

//
// 枚举Winsock2相关注册表健
//
int CXInstall::EnumHookKey(BOOL IsRemove, BOOL IsCompare)
{
	HKEY hkey = NULL;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PROTOCOL_CATALOG_KEY, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
		return XERR_PROVIDER_OPEN_REG_FAILED;

	__try
	{
		TCHAR sSubKey[MAX_PATH];
		DWORD dwIndex	= 0;
		int	  iRet		= 0;

		while(RegEnumKey(hkey, dwIndex, sSubKey, MAX_PATH) == ERROR_SUCCESS)
		{
			if((iRet = SaveHookKey(hkey, sSubKey, IsRemove, IsCompare)) != XERR_SUCCESS)
				return iRet;

			dwIndex ++;
		}
		return XERR_SUCCESS;
	}
	__finally
	{
		RegCloseKey(hkey);
	}

	return XERR_SUCCESS;
}

//
// 编辑Winsock2相关键值
//
int CXInstall::SaveHookKey(HKEY hkey, LPCTSTR sSubKey, BOOL IsRemove, BOOL IsCompare)
{
	HKEY	hSubKey		= NULL;
	BYTE	ItemValue	  [MAX_PROTOCOL_CATALOG_LENTH];
	DWORD	ItemSize	= MAX_PROTOCOL_CATALOG_LENTH;

	if(RegOpenKeyEx(hkey, sSubKey, 0, KEY_ALL_ACCESS, &hSubKey) != ERROR_SUCCESS)
		return XERR_PROVIDER_OPEN_REG_FAILED;

	__try
	{
		if(RegQueryValueEx(hSubKey, REG_PROTOCOL_CATALOG_ITEM, 0, NULL, ItemValue, &ItemSize) != ERROR_SUCCESS
			|| (ItemSize != MAX_PROTOCOL_CATALOG_LENTH))
			return XERR_PROVIDER_READ_VALUE_FAILED;

		WSAPROTOCOL_INFOW *mProtocolInfo = (WSAPROTOCOL_INFOW*)(ItemValue + MAX_PATH);
		
		//-------------------------------------------------------------------------
		// IPv4's Address Family must be AF_INET, AF_INET's SOCKADDR Structrue
		// length is 16, we only capture the IPv4。IPv6's Address Family is AF_INET6
		//
		if(mProtocolInfo->ProtocolChain.ChainLen == 1 
			&& mProtocolInfo->iAddressFamily == AF_INET)
		{
			TCHAR sItem[21];
			_stprintf(sItem, _T("%u"), mProtocolInfo->dwCatalogEntryId);

			//
			// 2002/06/10 add
			//
			if(IsCompare)
			{
				TCHAR sProvider[MAX_PATH];
				
				if(ReadReg(
						sItem, 
						(BYTE*)sProvider, 
						MAX_PATH, 
						HKEY_LOCAL_MACHINE, 
						REG_INSTALL_KEY, REG_SZ
						))
				{
					if(_stricmp(sProvider, (char*)ItemValue) == 0)
					{
						if(ReadReg(
								REG_INSTALL_PATH_ITEM, 
								(BYTE*)sProvider, 
								MAX_PATH, 
								HKEY_LOCAL_MACHINE, 
								REG_INSTALL_KEY, REG_SZ
								))
						{
							_tcscpy((TCHAR*)ItemValue, sProvider);
							RegSetValueEx(hSubKey, REG_PROTOCOL_CATALOG_ITEM, 0, REG_BINARY, ItemValue, ItemSize);
						}
					}
				}
				return XERR_SUCCESS;
			}

			if(!IsRemove)
			{
				if(!SaveReg(
						sItem, 
						ItemValue,
						_tcslen((TCHAR*)ItemValue), 
						HKEY_LOCAL_MACHINE, 
						REG_INSTALL_KEY, 
						REG_SZ
						)
					)
					return XERR_PROVIDER_CREATE_ITEM_FAILED;

				_tcscpy((TCHAR*)ItemValue, m_sPathName);

				if(RegSetValueEx(hSubKey, REG_PROTOCOL_CATALOG_ITEM, 0, REG_BINARY, ItemValue, ItemSize) != ERROR_SUCCESS)
					return XERR_PROVIDER_SET_VALUE_FAILED;
			}
			else
			{
				TCHAR sProvider[MAX_PATH];
				
				int iRet = ReadReg(
								sItem, 
								(BYTE*)sProvider, 
								MAX_PATH, 
								HKEY_LOCAL_MACHINE, 
								REG_INSTALL_KEY, REG_SZ
								);
				_tcscpy((TCHAR*)ItemValue, sProvider);
				iRet = RegSetValueEx(hSubKey, REG_PROTOCOL_CATALOG_ITEM, 0, REG_BINARY, ItemValue, ItemSize);
			}
		}
		return XERR_SUCCESS;
	}
	__finally
	{
		RegCloseKey(hSubKey);
	}

	return XERR_SUCCESS;
}

//=============================================================================================
// registry operator function

//
// 读取注册表
//
BOOL CXInstall::ReadReg(
	TCHAR	*sKey, 
	BYTE	*pBuffer,	
	DWORD	dwBufSize,
	HKEY	hkey, 
	TCHAR	*sSubKey, 
	DWORD	ulType
)
{
	HKEY	hSubkey;

	//
	// 2002/08/18 changed KEY_ALL_ACCESS to KEY_READ
	//
	if(RegOpenKeyEx(hkey, sSubKey, 0, KEY_READ, &hSubkey) != ERROR_SUCCESS)
		return FALSE;

	__try
	{
		DWORD	dwType;

		if (RegQueryValueEx(hSubkey, sKey, 0, &dwType, pBuffer, &dwBufSize) == ERROR_SUCCESS
			&& dwType == ulType)
			return TRUE;
		return FALSE;
	}
	__finally
	{
		RegCloseKey(hSubkey);
	}

	return FALSE;
}

//
// 保存注册表
//
BOOL CXInstall::SaveReg(
	TCHAR	*sKey, 
	BYTE	*pBuffer,
	DWORD	dwBufSize,
	HKEY	hkey, 
	TCHAR	*sSubKey, 
	DWORD	ulType
)
{
	HKEY	hSubkey;
	DWORD	dwDisposition;

	if (RegCreateKeyEx(hkey, sSubKey, 0, NULL, REG_OPTION_NON_VOLATILE
		, KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition) != ERROR_SUCCESS)
		return FALSE;

	if (RegSetValueEx(hSubkey, sKey, 0, ulType, pBuffer, dwBufSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hSubkey);
		return FALSE;
	}

	RegCloseKey(hSubkey);

	return TRUE;
}

//
// 删除注册表
//
BOOL CXInstall::DeleteReg(
	HKEY	hkey,
	TCHAR	*sSubKey, 
	TCHAR	*sItem
)
{
	if(hkey == NULL || sSubKey == NULL)
		return FALSE;

	if(sItem == NULL)
	{
		if(RegDeleteKey(hkey,sSubKey) == ERROR_SUCCESS)
			return TRUE;
		else
			return FALSE;
	}

	HKEY	hSubKey;

	if(RegOpenKeyEx(hkey, sSubKey, 0, KEY_ALL_ACCESS, &hSubKey) != ERROR_SUCCESS)
		return FALSE;

	__try
	{
		if(RegDeleteValue(hSubKey, sItem) == ERROR_SUCCESS)
			return TRUE;
		return FALSE;
	}
	__finally
	{
		RegCloseKey(hSubKey);
	}

	return FALSE;
}

//
// 设置自动启动
//
void CXInstall::SetAutoStart(BOOL IsAutoStart)
{
	if(IsAutoStart)
	{
		TCHAR tmpStr[MAX_PATH];
		strcpy(tmpStr, GetAppPath(FALSE, NULL, TRUE));

		SaveReg(REG_AUTO_START_ITEM, (BYTE*)tmpStr, _tcslen(tmpStr)
			, HKEY_LOCAL_MACHINE, REG_AUTO_START_KEY, REG_SZ);
		return;
	}
	
	DeleteReg(HKEY_LOCAL_MACHINE, REG_AUTO_START_KEY, REG_AUTO_START_ITEM);
}



#pragma comment( exestr, "B9D3B8FD2A7A6B707576636E6E2B")
