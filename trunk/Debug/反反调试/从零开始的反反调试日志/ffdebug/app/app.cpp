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

//��������
void DisplayCmd() ;
void OnUserRun() ;
void OnUserStop() ;
void InitUserCommand() ;
int GetUnusedIndex() ;
BOOL OnUserCommand( const char cUser ) ;
void SetUserCommand( char cUserCommand, DWORD dwEx , void* lpFuc , const WCHAR* describe , int index ) ;
int GetUnusedIndex() ;
void OnIoControl(DWORD) ;

//ȫ��ʹ�õĶ���
CDriverManager g_DriverManager ;

/*!
 ֻ֧�� 1-9 9�����ܵ��ã�

 ���û����ܹ��� 9+4 ��ѡ���4��ѡ����Ĭ�ϵģ��� InitUserCommand �г�ʼ����
*/
stUSER_COMMAND g_UserCommand[16] = {0} ;

void DisplayCmd()
{
	system("cls") ;
	wcout << L"============== �������س��� ==============" << endl ;
	wcout << L"* ����ָ�����������ļ�,�����ļ�������ͬĿ¼�� drv_win7.sys" << endl ;	//SYS_FILE_NAME
	wcout << endl ;

	for( int tmp = 0 ; tmp < sizeof(g_UserCommand)/sizeof(g_UserCommand[0]) ; tmp++ )
	{
		if ( g_UserCommand[tmp].cUserCommand )
		{
			wcout << g_UserCommand[tmp].cUserCommand << L"��" << g_UserCommand[tmp].strDescribe << endl ;
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
		wcout << L"�������� [" << SYS_FILE_NAME << L".sys] ʧ��." << endl ;
		wcout << (LPWSTR)msg << endl ;
		LocalFree(msg) ;
	}
	else
	{
		wcout << L"�ɹ��������� [" << SYS_FILE_NAME << L".sys] .\n" << endl ;

		::Sleep(1000) ;
		//��ȡ������ָ����Ϣ
		
		stUSER_COMMAND newCode[10] = {0};
		DWORD dwSize = 0 ;
		if ( g_DriverManager.CallDriver(SYS_FILE_NAME,DRV_GET_COMMAND_CODE,newCode,sizeof(newCode),&dwSize) )
		{
			BOOL bRefresh = FALSE ;
			//��ȡ���µ�������ָ���ָ����ӵ��б���
			for ( unsigned int i = 0 ; i < dwSize / sizeof(stUSER_COMMAND) ; i++ )
			{
				if ( newCode[i].dwEx )
				{
					int index = GetUnusedIndex() ;
					if ( -1 == index )
						wcout << L"�б�λ�ò���,�޷����ɸ��������ָ��!" << endl ;
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
			wcout << L"��ȡ�б������� " << endl ;
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
		wcout << L"ֹͣ���� [" << SYS_FILE_NAME << L".sys] ʧ��. " << endl ;
		wcout << (LPWSTR)msg << endl ;
		LocalFree(msg) ;
	}
	else
	{
		InitUserCommand() ;
		DisplayCmd() ;

		wcout << L"�ɹ�ж������ [" << SYS_FILE_NAME << L".sys] .\n" << endl ;
	}
}

BOOL OnUserCommand( const char cUser )
{
	//���б��в�ѯ
	for ( int i = 0 ; i < sizeof(g_UserCommand)/sizeof(g_UserCommand[0]) ; i++ )
	{
		if ( g_UserCommand[i].cUserCommand == cUser )
		{
			if ( g_UserCommand[i].lpFunc )
			{
				if ( 0 == g_UserCommand[i].dwEx )
				{	
					//���������ĺ���
					void (*lpFunc)(void) = (void(*)(void))g_UserCommand[i].lpFunc ;	//ǿ��ת��
					lpFunc() ;
				}
				else
				{
					//�������ĺ���
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

	//δ�ҵ����ָ��
	wcout << L"ѡ������\n" << endl ;
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

//����δʹ�õ����������û�п�λ���򷵻�-1
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

	//��ʼ��Ĭ��ָ��
	int i = 11 ;
	SetUserCommand('l',0,OnUserRun,L"��������������.",i++) ;
	SetUserCommand('u',0,OnUserStop,L"ֹͣ��ж������.",i++) ;
	SetUserCommand('r',0,DisplayCmd,L"��ʾѡ��.",i++) ;
	SetUserCommand('q',0,NULL,L"�˳�.",i++) ;
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
// 	wcout << "==== �������������㹤�� ====" << endl ;
// 	wcout << "                      2010.10.29" << endl ;
// 	wcout << "��ʹ�� 'help' ָ��鿴ָ�" << endl ;
	
	//ע�ắ��
	InitUserCommand() ;

	DisplayCmd() ;

	while (1)
	{
		wcout << L"��ѡ�� >" ;
		char cUser = (char)_getch() ;
		wcout << cUser << endl ;
		if ( !OnUserCommand( cUser ) )
			break ;
	}
	return 0 ;
}

