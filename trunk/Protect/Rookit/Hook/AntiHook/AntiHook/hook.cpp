// RedirectApi.cpp : ����Ӧ�ó������ڵ㡣
//

#include <windows.h>
#include "..\\detour\\detours.h"


static int (WINAPI* OLD_MessageBoxA)( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType ) = MessageBoxA;

int WINAPI NEW_MessageBoxA( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType )
{
	// �˴����Թ۲�/�޸ĵ��ò�������������ȡ������ֱ�ӷ��ء�
	// ����

	// �޸��������������ԭ����
	int ret = OLD_MessageBoxA ( hWnd, "��������ѱ��޸�", "[����]", uType ) ;

	// �˴����Բ鿴/�޸ĵ���ԭ�����ķ���ֵ
	// ����

	return ret ;
}

VOID Hook ()
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	
	// �������������ε���DetourAttach������Hook�������
	DetourAttach(&(PVOID&)OLD_MessageBoxA, NEW_MessageBoxA ) ;

	DetourTransactionCommit();
}

VOID UnHook ()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// �������������ε���DetourDetach�����������������Hook
	DetourDetach(&(PVOID&)OLD_MessageBoxA, NEW_MessageBoxA ) ;

	DetourTransactionCommit();
}

int WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	MessageBoxA ( 0, "������Ϣ��", "����", 0 ) ;
	Hook () ;
	MessageBoxA ( 0, "������Ϣ��", "����", 0 ) ;
	UnHook () ;
	return 0 ;
}