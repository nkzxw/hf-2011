
#include <windows.h>

// ����API�ҽ���ṹ
typedef struct _HOOK_ITEM {
	DWORD	dwAddr ;			// IAT�����ڵ�ַ
	DWORD	dwOldValue ;		// IAT���ԭʼ������ַ
	DWORD	dwNewValue ;		// IAT����º�����ַ
} HOOK_ITEM, *PHOOK_ITEM ;
HOOK_ITEM	HookItem = {0} ;	// ����IAT����ڱ���MessageBoxA��IAT����Ϣ

// ����MessageBoxA����ԭ��
typedef int (WINAPI* PFNMessageBoxA)( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType ) ;

// �����ض���API��ʵ�ֺ���
BOOL WINAPI RedirectApi ( PCHAR pDllName, PCHAR pFunName, DWORD dwNewProc, PHOOK_ITEM pItem ) ;

// �Զ����MessageBoxA����
// ʵ�ֶ�ԭʼMessageBoxA�����롢��������ļ�أ�������ȡ������
int WINAPI NEW_MessageBoxA( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType )
{
	// �˴����Թ۲�/�޸ĵ��ò�������������ȡ������ֱ�ӷ��ء�
	// ����

	// ȡ��ԭ������ַ
	PFNMessageBoxA pfnMessageBoxA = (PFNMessageBoxA)HookItem.dwOldValue ;

	// ���������Ϣ��
	// �������ֱ�ӵ���MessageBoxA���ͽ�������ѭ��
	pfnMessageBoxA ( hWnd, "����API�ض�����̵���Ϣ��", "����", 0 ) ;

	// ����ԭ����
	int ret = pfnMessageBoxA ( hWnd, lpText, lpCaption, uType ) ;

	// �˴����Բ鿴/�޸ĵ���ԭ�����ķ���ֵ
	// ����

	return ret ;
}

int WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// �ض���API
	if ( !RedirectApi ( "USER32.dll", "MessageBoxA", (DWORD)NEW_MessageBoxA, &HookItem ) )
		OutputDebugStringA ( "RedirectApi failed!" ) ;
	else
		OutputDebugStringA ( "RedirectApi success!" ) ;
	
	MessageBoxA ( 0, "������Ϣ��", "����", 0 ) ;
	return 0 ;
}

// ʵ���ض���API
// ����pDllName:Ŀ��API���ڵ�DLL����
// ����pFunName:Ŀ��API����
// ����dwNewProc:�Զ���ĺ�����ַ
// ����pItem:���ڱ���IAT����Ϣ
BOOL WINAPI RedirectApi ( PCHAR pDllName, PCHAR pFunName, DWORD dwNewProc, PHOOK_ITEM pItem )
{
	// �������Ƿ�Ϸ�
	if ( pDllName == NULL || pFunName == NULL || !dwNewProc || !pItem )
		return FALSE ;

	// ���Ŀ��ģ���Ƿ����
	char	szTempDllName[256] = {0} ;
	DWORD	dwBaseImage = (DWORD)GetModuleHandle(NULL) ;
	if ( dwBaseImage == 0 )
		return FALSE ;

	// ȡ��PE�ļ�ͷ��Ϣָ��
	PIMAGE_DOS_HEADER			pDosHeader	= (PIMAGE_DOS_HEADER)dwBaseImage ;
	PIMAGE_NT_HEADERS			pNtHeader	= (PIMAGE_NT_HEADERS)(dwBaseImage + (pDosHeader->e_lfanew)) ;
	PIMAGE_OPTIONAL_HEADER32	pOptionalHeader = &(pNtHeader->OptionalHeader) ;
	PIMAGE_SECTION_HEADER		pSectionHeader	= (PIMAGE_SECTION_HEADER)((DWORD)pNtHeader + 0x18 + pNtHeader->FileHeader.SizeOfOptionalHeader ) ;

	// ���������
	PIMAGE_THUNK_DATA pThunk, pIAT ;
	PIMAGE_IMPORT_DESCRIPTOR pIID = (PIMAGE_IMPORT_DESCRIPTOR)(dwBaseImage+pOptionalHeader->DataDirectory[1].VirtualAddress ) ;
	while ( pIID->FirstThunk )
	{
		// ����Ƿ�Ŀ��ģ��
		if ( strcmp ( (PCHAR)(dwBaseImage+pIID->Name), pDllName ) )
		{
			pIID++ ;
			continue ;
		}

		pIAT = (PIMAGE_THUNK_DATA)( dwBaseImage + pIID->FirstThunk ) ;
		if ( pIID->OriginalFirstThunk )
			pThunk = (PIMAGE_THUNK_DATA)( dwBaseImage + pIID->OriginalFirstThunk ) ;
		else
			pThunk = pIAT ;

		// ����IAT
		DWORD	dwThunkValue = 0 ;
		while ( ( dwThunkValue = *((DWORD*)pThunk) ) != 0 )
		{
			if ( ( dwThunkValue & IMAGE_ORDINAL_FLAG32 ) == 0 )
			{
				// ����Ƿ�Ŀ�꺯��
				if ( strcmp ( (PCHAR)(dwBaseImage+dwThunkValue+2), pFunName ) == 0 )
				{
					// ��亯���ض�����Ϣ
					pItem->dwAddr		= (DWORD)pIAT ;
					pItem->dwOldValue	= *((DWORD*)pIAT) ;
					pItem->dwNewValue = dwNewProc;

					// �޸�IAT��
					DWORD dwOldProtect = 0 ;
					VirtualProtect ( pIAT, 4, PAGE_READWRITE, &dwOldProtect ) ;
					*((DWORD*)pIAT) = dwNewProc ;
					VirtualProtect ( pIAT, 4, PAGE_READWRITE, &dwOldProtect ) ;
					return TRUE ;
				}
			}

			pThunk ++ ;
			pIAT ++ ;
		}

		pIID ++ ;
	}

	return FALSE ;
}
