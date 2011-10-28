
#include <windows.h>
#include "HookApi.h"

CHOOKAPI	HookItem ;

// ����MessageBoxA����ԭ��
typedef int (WINAPI* PFNMessageBoxA)( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType ) ;

// �Զ����MessageBoxA����
// ʵ�ֶ�ԭʼMessageBoxA�����롢��������ļ�أ�������ȡ������
int WINAPI NEW_MessageBoxA( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType )
{
	// ����HOOK
	HookItem.UnHook () ;

	// �˴����Թ۲�/�޸ĵ��ò�������������ȡ������ֱ�ӷ��ء�
	// ����

	// ȡ��ԭ������ַ
	PFNMessageBoxA pfnMessageBoxA = (PFNMessageBoxA)HookItem.pOldFunEntry ;

	// ����ԭ�������޸��������
	int ret = pfnMessageBoxA ( hWnd, "����HOOK�������̵���Ϣ��", "[����]", uType ) ;

	// �˴����Բ鿴/�޸ĵ���ԭ�����ķ���ֵ
	// ����

	// ����HOOK
	HookItem.ReHook () ;

	return ret ;
}

int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// ԭʼAPI
	MessageBoxA ( 0, "������Ϣ��", "����", 0 ) ;

	// HOOK API
	HookItem.Hook ( "USER32.dll", "MessageBoxA", (FARPROC)NEW_MessageBoxA ) ;
	
	// ����API������
	MessageBoxA ( 0, "������Ϣ��", "����", 0 ) ;

	// ����HOOK
	HookItem.UnHook () ;
	return 0 ;
}
