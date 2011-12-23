// app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

#include <conio.h>
#include "DriverManager.h"

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include <winioctl.h> 
#include "../my_common.h"

using namespace std ;

//函数声明
void DisplayCmd() ;
void OnUserRun() ;
void OnUserStop() ;
void InitUserCommand() ;
int GetUnusedIndex() ;
BOOL OnUserCommand( const char cUser ) ;
void SetUserCommand( char cUserCommand, DWORD dwEx , void* lpFuc , const WCHAR* describe , int index ) ;
int GetUnusedIndex() ;
void OnIoControl(DWORD) ;

//全局使用的对象
CDriverManager g_DriverManager ;

/*!
 只支持 1-9 9个功能调用！

 给用户的总共有 9+4 个选项，后4个选项是默认的，在 InitUserCommand 中初始化。
*/
stUSER_COMMAND g_UserCommand[16] = {0} ;

void DisplayCmd()
{
	system("cls") ;
	wcout << L"============== 驱动加载程序 ==============" << endl ;
	wcout << L"* 不可指定驱动程序文件,驱动文件必须是同目录的 drv_win7.sys" << endl ;	//SYS_FILE_NAME
	wcout << endl ;

	for( int tmp = 0 ; tmp < sizeof(g_UserCommand)/sizeof(g_UserCommand[0]) ; tmp++ )
	{
		if ( g_UserCommand[tmp].cUserCommand )
		{
			wcout << g_UserCommand[tmp].cUserCommand << L"、" << g_UserCommand[tmp].strDescribe << endl ;
		}
	}
	wcout << endl ;
}


void OnUserRun()
{
	if ( !g_DriverManager.StartDriver(SYS_FILE_NAME) )
	{
		LPVOID msg ;
		FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM 
			, NULL
			, ::GetLastError()
			, 0 
			, (LPWSTR)&msg
			, 0 
			, NULL ) ;
		wcout << L"启动驱动 [" << SYS_FILE_NAME << L".sys] 失败." << endl ;
		wcout << (LPWSTR)msg << endl ;
		LocalFree(msg) ;
	}
	else
	{
		wcout << L"成功启动驱动 [" << SYS_FILE_NAME << L".sys] .\n" << endl ;

		::Sleep(1000) ;
		//获取驱动的指令信息
		
		stUSER_COMMAND newCode[10] = {0};
		DWORD dwSize = 0 ;
		if ( g_DriverManager.CallDriver(SYS_FILE_NAME,DRV_GET_COMMAND_CODE,newCode,sizeof(newCode),&dwSize) )
		{
			BOOL bRefresh = FALSE ;
			//获取到新的驱动层指令，将指令添加到列表中
			for ( unsigned int i = 0 ; i < dwSize / sizeof(stUSER_COMMAND) ; i++ )
			{
				if ( newCode[i].dwEx )
				{
					int index = GetUnusedIndex() ;
					if ( -1 == index )
						wcout << L"列表位置不够,无法容纳更多的驱动指令!" << endl ;
					else
					{
						bRefresh = TRUE ;
						newCode[i].cUserCommand = (char)(index + '0') ;
						newCode[i].lpFunc = OnIoControl ;
						SetUserCommand(newCode[i].cUserCommand,newCode[i].dwEx,newCode[i].lpFunc,newCode[i].strDescribe,index) ;
						wcout << L"ceshi " << newCode[i].dwEx << L" " << newCode[i].strDescribe << endl ;
					}
				}
			}

			if ( bRefresh )
				DisplayCmd() ;
		}
		else
		{
			LPVOID msg ;
			FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM 
				, NULL
				, ::GetLastError()
				, 0 
				, (LPWSTR)&msg
				, 0 
				, NULL ) ;
			wcout << L"获取列表发生错误 " << endl ;
			wcout << (LPWSTR)msg << endl ;
			LocalFree(msg) ;
		}
	}
}

void OnUserStop()
{
	if ( !g_DriverManager.StopDriver(SYS_FILE_NAME) )
	{
		LPVOID msg ;
		FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM 
			, NULL
			, ::GetLastError()
			, 0 
			, (LPWSTR)&msg
			, 0 
			, NULL ) ;
		wcout << L"停止驱动 [" << SYS_FILE_NAME << L".sys] 失败. " << endl ;
		wcout << (LPWSTR)msg << endl ;
		LocalFree(msg) ;
	}
	else
	{
		InitUserCommand() ;
		DisplayCmd() ;

		wcout << L"成功卸载驱动 [" << SYS_FILE_NAME << L".sys] .\n" << endl ;
	}
}

BOOL OnUserCommand( const char cUser )
{
	//在列表中查询
	for ( int i = 0 ; i < sizeof(g_UserCommand)/sizeof(g_UserCommand[0]) ; i++ )
	{
		if ( g_UserCommand[i].cUserCommand == cUser )
		{
			if ( g_UserCommand[i].lpFunc )
			{
				if ( 0 == g_UserCommand[i].dwEx )
				{	
					//不带参数的函数
					void (*lpFunc)(void) = (void(*)(void))g_UserCommand[i].lpFunc ;	//强制转换
					lpFunc() ;
				}
				else
				{
					//带参数的函数
					void (*lpFunc)(DWORD) = (void(*)(DWORD))g_UserCommand[i].lpFunc ;
					lpFunc(g_UserCommand[i].dwEx) ;
				}
				//g_UserCommand[i].lpFunc() ;
				return TRUE ;
			}
			else
				return FALSE ;
		}
	}

	//未找到相关指令
	wcout << L"选项有误\n" << endl ;
	return TRUE ;
}

void SetUserCommand( char cUserCommand, DWORD dwEx , void* lpFuc , const WCHAR* describe , int index )
{
	if ( index < 0 || index > sizeof(g_UserCommand)/sizeof(g_UserCommand[0]) )
		return ;

	g_UserCommand[index].cUserCommand = cUserCommand ;
	g_UserCommand[index].dwEx = dwEx ;
	g_UserCommand[index].lpFunc = lpFuc ;
	wcscpy_s(g_UserCommand[index].strDescribe,describe) ;
}

//返回未使用的索引，如果没有空位，则返回-1
int GetUnusedIndex()
{
	for ( int i = 0 ; i < sizeof(g_UserCommand)/sizeof(g_UserCommand[0]) ; i++ )
	{
		if ( g_UserCommand[i].cUserCommand == 0 )
			return i ;
	}
	return -1 ;
}

void InitUserCommand()
{
	::ZeroMemory(g_UserCommand,sizeof(g_UserCommand)) ;

	//初始化默认指令
	int i = 11 ;
	SetUserCommand('l',0,OnUserRun,L"加载且运行驱动.",i++) ;
	SetUserCommand('u',0,OnUserStop,L"停止且卸载驱动.",i++) ;
	SetUserCommand('r',0,DisplayCmd,L"显示选项.",i++) ;
	SetUserCommand('q',0,NULL,L"退出.",i++) ;
}

void OnIoControl(DWORD dwCode)
{
	WCHAR msg[256] = {0} ;
	DWORD dwSize = 0 ;
	if ( g_DriverManager.CallDriver(SYS_FILE_NAME,dwCode,msg,sizeof(msg),&dwSize) )
	{
	}
	else
	{
		wcout << L"IoControlCode[0x" << hex << dwCode << L"] Return False." << endl ;
		DWORD dwRet = ::GetLastError() ;
		if ( 0 != dwRet )
		{
			LPVOID msg ;
			FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM 
				, NULL
				, dwRet
				, 0 
				, (LPWSTR)&msg
				, 0 
				, NULL ) ;
			wcout << (LPWSTR)msg << endl ;
			LocalFree(msg) ;
		}
		wcout << endl ;
	}
	if ( dwSize > 0 )
		wcout << L"->" << msg << L"\n" << endl ;
	else
		wcout << endl ;
}

int _tmain(int argc, _TCHAR* argv[])
{
	wcout.imbue(locale(locale(),"",LC_CTYPE));
// 	wcout << "==== 反反调试驱动层工具 ====" << endl ;
// 	wcout << "                      2010.10.29" << endl ;
// 	wcout << "请使用 'help' 指令查看指令集" << endl ;
	
	//注册函数
	InitUserCommand() ;

	DisplayCmd() ;

	while (1)
	{
		wcout << L"请选择 >" ;
		char cUser = (char)_getch() ;
		wcout << cUser << endl ;
		if ( !OnUserCommand( cUser ) )
			break ;
	}
	return 0 ;
}

