// GalaxyApMGR.cpp: implementation of the GalaxyApMGR class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "commhdr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GalaxyApMGR::GalaxyApMGR()
{

		ZeroMemory(szReloadOsDriverName,sizeof(szReloadOsDriverName));

		ZeroMemory(szExceptionDriverName, sizeof(szExceptionDriverName));
		
		ZeroMemory(&m_protect_info, sizeof(m_protect_info));
	m_bObtainedSyms	=	false;

	m_bSetupReloadOS	=	false;
	m_bSetupKidisp		=	false;
}

GalaxyApMGR::~GalaxyApMGR()
{


}
//////////////////////////////////////////////////////////////////////////
//初始化，获取符号 。。
bool	GalaxyApMGR::Initialize(bool binitAgain)
{
		//还是这样好。
	//否则在卸载然后再加载的时候就会出现问题
	//因为在第一次下发地址的时候，如果已经reload OS,就会累加改变了原来的内核地址，
	//这样如果每次卸载再加载，就会导致不断的累加
		binitAgain=1;
		//没load过
		if (!m_bObtainedSyms)
		{
				m_OsBaseAddress	=	getSymbols();
				if (m_OsBaseAddress)
				{
								m_bObtainedSyms	=	true;
				}
			return m_OsBaseAddress?m_OsBaseAddress:0; 
		}
		//已经load 过，不过要求重新load
		if (binitAgain)
		{
				m_OsBaseAddress	=	getSymbols();
	
		}
		return m_OsBaseAddress?m_OsBaseAddress:0; 
}

//////////////////////////////////////////////////////////////////////////

bool GalaxyApMGR::FixExceptionDriverSymAddress()
{
	
	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		myprint("createfile fail %d\r\n", GetLastError());
		return 0 ;	
	}
	DWORD datasize;
	HX_DYNC_FUNCTION *pdata;
	get_dync_funs(&pdata, datasize);

			if (m_bSetupReloadOS)	//如果启用了新内核，则要修复符号的新地址,其中一个符号不能修正
			{
					for(int i=0;i<datasize/sizeof(HX_DYNC_FUNCTION);i++)
					{
							if (stricmp(pdata[i].FunName, "DbgkDebugObjectType")==0)
							{
								myprint("DbgkDebugObjectType %x \r\n", pdata[i].FunAddr);
								continue;
							}
							pdata[i].FunAddr	=	pdata[i].FunAddr+m_uNewOsBaseDelt;

					}
			}
	printf(" send size %x", datasize);
	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_Set,
		pdata,
		datasize,
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		myprint("DeviceIoControl  IOCTL_TEST_KIDISPAT_Set fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control IOCTL_TEST_KIDISPAT_Set success \r\n");
	return 1;
}
//////////////////////////////////////////////////////////////////////////

bool GalaxyApMGR::HookDIspatchException()
{
	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}
		DWORD datasize;
	HX_DYNC_FUNCTION *pdata;
	get_dync_funs_hook(&pdata, datasize);
	//这个是专门把老的os的的实现跳转到自己的实现中的

	printf(" send size %x", datasize);
	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_InlineHook,
		pdata,
		datasize,
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		myprint("DeviceIoControl   IOCTL_TEST_KIDISPAT_InlineHook fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  IOCTL_TEST_KIDISPAT_InlineHook success \r\n");
	return 1;
}
//////////////////////////////////////////////////////////////////////////

bool GalaxyApMGR::SetNewOsAddress()
{
	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}

	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_NewOsAddress,
		&m_uNewOsBase,
		sizeof(m_uNewOsBase),
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl IOCTL_TEST_KIDISPAT_NewOsAddress fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  IOCTL_TEST_KIDISPAT_NewOsAddress success , NewAddress is %X\r\n",m_uNewOsBase);
	return 1;
}
//////////////////////////////////////////////////////////////////////////

bool GalaxyApMGR::HookNew2My()
{
	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}
		DWORD datasize;
	HX_DYNC_FUNCTION *pdata;
	get_dync_New2my(&pdata, datasize);

			if (m_bSetupReloadOS)	//如果启用了新内核，在HOOK的时候，就在老的函数开头inline hook JMP到新内核中
			{
					for(int i=0;i<datasize/sizeof(HX_DYNC_FUNCTION);i++)
					{
							pdata[i].delta	=m_uNewOsBaseDelt;
					}
			}

	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_NEW2MY,
		pdata,
		datasize,
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl IOCTL_TEST_KIDISPAT_NEW2MY fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  IOCTL_TEST_KIDISPAT_NEW2MY success , \r\n");
	return 1;
}
//////////////////////////////////////////////////////////////////////////
bool GalaxyApMGR::HookOldFuns()
{
			HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}

		DWORD datasize;
	HX_DYNC_FUNCTION *pdata;
	get_dync_funs_hook_old(&pdata, datasize);

			if (m_bSetupReloadOS)	//如果启用了新内核，在HOOK的时候，就在老的函数开头inline hook JMP到新内核中
			{
					for(int i=0;i<datasize/sizeof(HX_DYNC_FUNCTION);i++)
					{
							pdata[i].delta	=m_uNewOsBaseDelt;
					}
			}
	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_HOOK_OLD,
		pdata,
		datasize,
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl IOCTL_TEST_KIDISPAT_HOOK_OLD fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  IOCTL_TEST_KIDISPAT_HOOK_OLD success \r\n");
	return 1;

}
//////////////////////////////////////////////////////////////////////////

bool GalaxyApMGR::SetProtectInfo()
{
	
	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}

	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_PROTECT_INFO,
		&m_protect_info,
		sizeof(m_protect_info),
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl IOCTL_TEST_KIDISPAT_PROTECT_INFO fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  IOCTL_TEST_KIDISPAT_PROTECT_INFO success , NewAddress is %X\r\n",m_protect_info.PidOrName);
	return 1;
}

//////////////////////////////////////////////////////////////////////////

bool	GalaxyApMGR::SetupMyKidispatchexcepion(char *pDriverName)
{
		bool	bret	=	false;
		do 
		{
			strcpy(szExceptionDriverName, pDriverName);
			char	szfilesysPath[1024];
			ZeroMemory(szfilesysPath, sizeof(szfilesysPath));
			GetCurrentDirectory(1022, szfilesysPath);
			strcat(szfilesysPath, "\\");
			strcat(szfilesysPath, pDriverName);
			strcat(szfilesysPath, ".sys");
			bret	=	LoadNTDriver(pDriverName, szfilesysPath);
			if (!bret)
			{
				myprint("LoadDriver %s fail\r\n", szfilesysPath);
				break;
			}
			//设置新地址，这是为了让驱动在硬编译搜索 kidispatchexception 的时候使用的
			if (m_bSetupReloadOS)
			{
				SetNewOsAddress();
			}
			bret	=	FixExceptionDriverSymAddress();
			if (!bret)
			{
				myprint("FixExceptionDriverSymAddress fail\r\n");
				break;
			}
			bret	=	HookDIspatchException();
			if (!bret)
			{
				myprint("HookDIspatchException fail\r\n");
				break;
			}
			bret	=	HookOldFuns();
			if (!bret)
			{
				myprint("HookOldFuns fail\r\n");
				break;
			}
			bret	=	HookNew2My();
			if (!bret)
			{
				myprint("HookNew2My fail\r\n");
				break;
			}
			bret	=	SendReplace();
			if (!bret)
			{
				myprint("SendReplace fail\r\n");
				break;
			}

			myprint("发送Dispatch Hook 返回成功\r\n");
			break;

		} while (0);
		m_bSetupKidisp	=	bret;
		return bret;
}
//////////////////////////////////////////////////////////////////////////
bool	GalaxyApMGR::unHookKidisp()
{

			HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}

	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_UNHOOK,
		&m_uNewOsBase,
		sizeof(m_uNewOsBase),
		NULL,
		0,
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl IOCTL_TEST_KIDISPAT_UNHOOK fail %d\r\n", GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  IOCTL_TEST_KIDISPAT_UNHOOK success \r\n");
	return 1;
		
}

//////////////////////////////////////////////////////////////////////////
bool	GalaxyApMGR::unSetupMyKidispatchexcepion()
{
		bool	bret	=false;
		if (!m_bSetupKidisp)
		{
				myprint("还没安装MyKidispatchexcepion\r\n");
				return 1;
		}
		bret	=	unHookKidisp();
		if (!bret)
		{
				return bret;

		}
		UnloadNTDriver(szExceptionDriverName);
		if (bret)
		{
				m_bSetupKidisp	=	false;	//已经卸载了，则置位为FALSE
		}

		return bret ;
}
//////////////////////////////////////////////////////////////////////////
//安装
bool	GalaxyApMGR::SetupReloadOS(char *pDriverName)
{
		bool	bret	=	false;
		do 
		{
			strcpy(szReloadOsDriverName, pDriverName);
			char	szfilesysPath[1024];
			ZeroMemory(szfilesysPath, sizeof(szfilesysPath));
			GetCurrentDirectory(1022, szfilesysPath);
			strcat(szfilesysPath, "\\");
			strcat(szfilesysPath, pDriverName);
			strcat(szfilesysPath, ".sys");
			bret	=	LoadNTDriver(pDriverName, szfilesysPath);	//如果驱动已经正在运行，也会返回成功的
			if (!bret)
			{
				myprint("LoadDriver %s fail\r\n", szfilesysPath);
				break;
			}
			if (SendHookPort(IOCTL_HOOKPORTBYPASS_HOOKPORT))
			{
					bret	=	m_bSetupReloadOS	=	true;
					m_uNewOsBaseDelt	=	m_uNewOsBase	-	m_OsBaseAddress;
			}
			else
			{
					myprint("SendHookPort() fail\r\n");
			}

		}
		while(0);
		
		return	bret;
		
}
//////////////////////////////////////////////////////////////////////////
//会把驱动也卸载掉的。
bool		GalaxyApMGR::UnSetupReloadOS()
{
		if (!SendHookPort(IOCTL_HOOKPORTBYPASS_UNHOOKPORT))
		{
				myprint("卸载 hookport 失败\r\n");
				return false;
		}
		m_bSetupReloadOS	=	false;
		UnloadNTDriver(szReloadOsDriverName);
		return 1;
		
}

bool	GalaxyApMGR::SendHookPort(DWORD	IOCTL_CODE)
{

	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		HOOKPORTBYPASS_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}
	
	if (!DeviceIoControl(
		Device,
		IOCTL_CODE,
		szStrongOD,
		sizeof(szStrongOD),
		&m_uNewOsBase,		//返回了newaddress
		sizeof(m_uNewOsBase),
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl %d fail %d\r\n", IOCTL_CODE, GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  %d success RetBuffer:%X ,RetSize:%x\r\n",IOCTL_CODE,m_uNewOsBase, BytesReturned);
	return 1;
		
}

//////////////////////////////////////////////////////////////////////////


bool	GalaxyApMGR::SendReplace()
	{
	
	HANDLE  Device;
	DWORD   BytesReturned;
	
	
	Device = CreateFile(
		TEST_KIDISPAT_WIN32_DEVICE_NAME_A,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
		);
	
	if (Device == INVALID_HANDLE_VALUE)
	{
		printf("createfile fail %d\r\n", GetLastError());
		return 0;	
	}
		char szBuffer[512];
		ZeroMemory(szBuffer, sizeof(szBuffer));
		*(PULONG)szBuffer	=	m_uNewOsBaseDelt;
	strcpy(&szBuffer[4], szStrongOD);

	if (!DeviceIoControl(
		Device,
		IOCTL_TEST_KIDISPAT_HOOK_REPLACE,
		szBuffer,
		sizeof(szBuffer),
		&m_uNewOsBase,		//返回了newaddress
		sizeof(m_uNewOsBase),
		&BytesReturned,
		NULL
		))
	{
		printf("DeviceIoControl %d fail %d\r\n", IOCTL_TEST_KIDISPAT_HOOK_REPLACE, GetLastError());
		CloseHandle(Device);
		return 0;
	}
	CloseHandle(Device);
	myprint(" send io control  %d success RetSize:%x\r\n",IOCTL_TEST_KIDISPAT_HOOK_REPLACE, BytesReturned);
	return 1;
}