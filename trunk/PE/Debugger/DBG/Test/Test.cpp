// Test.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Test.h"
#include <commdlg.h>
#include <iostream>
#include "../Dbg/Dbg.h"

#include "disasm.h"
#pragma comment(lib,"Decode.lib")

#define MAX_LOADSTRING 100

// ȫ�ֱ���:
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������
DWORD AddressOfBreak;							// ����ϵ��ַ
DWORD StartAddress;								// ���淴���Ŀ�ʼ��ַ
DWORD Size;										// ���淴���ĳ���

// �˴���ģ���а����ĺ�����ǰ������:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc1(HWND hdlg,UINT message,WPARAM  wParam,	LPARAM  lParam);
BOOL CALLBACK DlgProc2(HWND hdlg,UINT message,WPARAM  wParam,	LPARAM  lParam);

int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEST));

	// ����Ϣѭ��:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
//  ע��:
//
//    ����ϣ��
//    �˴�������ӵ� Windows 95 �еġ�RegisterClassEx��
//    ����֮ǰ�� Win32 ϵͳ����ʱ������Ҫ�˺��������÷������ô˺���ʮ����Ҫ��
//    ����Ӧ�ó���Ϳ��Ի�ù�����
//    ����ʽ��ȷ�ġ�Сͼ�ꡣ
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEST));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��:
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	HWND hEdit;

	hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	hEdit = CreateWindow(_T("EDIT"), _T(""), WS_CHILD|WS_MAXIMIZE|WS_VISIBLE|ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��: ���������ڵ���Ϣ��
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	OPENFILENAME stOFN;
	char stFilePath[100];	//�����ļ�·��
	char szBuf[1000];		//�����ڴ��ȡ��������

	memset(&stOFN, 0, sizeof(stOFN));
	memset(stFilePath, 0, 100);

	//initial stOFN
	stOFN.lStructSize      =sizeof(stOFN);
	stOFN.nMaxFile         =MAX_PATH;
	stOFN.lpstrFile        =(LPWSTR)stFilePath;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// �����˵�ѡ��:
		switch (wmId)
		{
		case IDM_OPEN:
			{
				GetOpenFileName(&stOFN);
				LoadDebuggedProcess(stOFN.lpstrFile);

			}
			break;
		case IDM_START:
			{
				static int i = 0;
				if(i == 0)
				{
					ResumeDebuggedThread();
					StopOnException();
					
					ResumeDebuggedThread();
					StopOnException();
					i++;
				}
				else 
				{
					ResumeDebuggedThread();
					StopOnException();
				}
			}
			//DestroyWindow(hWnd);
			break;
		case ID_NORMAL:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc1);
				if(AddressOfBreak != 0)
					SetNormalBreakPoint(AddressOfBreak);
			}
			break;
		case ID_HARDWARE:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc1);
				if(AddressOfBreak != 0)
					SetHardwareBreakPoint(AddressOfBreak, 2, 000);
			}
			break;
		case ID_DELNORMAL:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc1);
				if(AddressOfBreak != 0)
					DelNormalBreakPoint(AddressOfBreak);
			}
			break;
		case ID_DELHARDWARE:
			{
				DelHardwareBreakPoint(2);
			}
			break;
		case ID_MEMORY:
			{				
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc1);
				if(AddressOfBreak != 0)
					SetMemoryBreakPoint(AddressOfBreak, 3);
			}break;
		case ID_DELMEM:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc1);
				if(AddressOfBreak != 0)
					DelMemoryBreakPoint(AddressOfBreak);
			}break;
		case ID_GO:
			{
				StopOnException();			
			}
			break;
		case ID_DISASM:
			{
				if( stProcessInfo.hProcess == 0)
				{
					MessageBox(hWnd, _T("Ҳ���㻹û�м��ؽ���"), _T("����"), MB_OK);
					break;
				}
				char sz[10];				//�����ַ
				DWORD OutAddress = NULL;	//���������ַ

				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DlgProc2);
				if(StartAddress == 0)
					break;				//�ж��Ƿ�ɹ����ؽ���
				if(!ReadProcessMemory(stProcessInfo.hProcess, (LPCVOID)StartAddress, szBuf, Size, NULL))
				{
					MessageBox(NULL, _T("��ȡ�ڴ�ʧ�ܣ���ȷ�����Ȩ��"), _T("����"), MB_OK);
				}
				char *Linear=szBuf;		//Pointer to linear address
				DWORD Index=0;			// Index of opcoded to decode
				DISASSEMBLY dis;
				std::string out;		//����ַ���
				while(Index < Size)
				{
					int i = 0;
					memset(&dis,0,sizeof(DISASSEMBLY));
					dis.Address = Index;
					Decode(&dis,Linear,&Index);
					OutAddress = dis.Address + StartAddress;
					sprintf(sz,"%0x",OutAddress);
					OutAddress--;
					out += sz;	
					out += "   ";
					out += dis.Assembly;
					out += "\r\n";
					Index++;
					i--;
				}
				SetWindowTextA(GetWindow(hWnd, GW_CHILD), out.c_str());
			}

			break;
		case ID_MEM:
			{	/*
				std::string out;
				char sz[10];
				int j;

				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DlgProc2);
				ReadProcessMemory(stProcessInfo.hProcess, (LPCVOID)StartAddress, szBuf, Size, NULL);
				for(int i = 0; i < Size; i ++)
				{
				StartAddress = StartAddress + i;
				sprintf(sz,"%0x",StartAddress);
				out += sz;
				out += " ";	
				j = szBuf[i];
				sprintf(sz,"%0x",j);
				out += sz;
				out += "\r\n";
				}
				SetWindowTextA(GetWindow(hWnd, GW_CHILD), out.c_str());
				*/
			}
			break;
		case ID_SHOW:
			{
				std::string out;
				char sz[10];
				DWORD Address;

				for(int i = 1; Address = GetNormalBreakPoints( i ); i++)		
				{
					sprintf(sz, "%0x", Address);
					out += sz;
					out += "\r\n";
				}
				SetWindowTextA(GetWindow(hWnd, GW_CHILD), out.c_str());
			}
			break;
		case ID_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: �ڴ���������ͼ����...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
//�ϵ�����Ի������Ϣ������
BOOL CALLBACK DlgProc1(HWND hdlg,UINT message,WPARAM  wParam,	LPARAM  lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_OK:
			{
				char szStr[20];
				GetDlgItemTextA(hdlg, IDC_EDIT1, szStr, 20);
				AddressOfBreak = atoi(szStr);
				EndDialog(hdlg, 0);
			}
			break;
		case ID_CLOSE:
			EndDialog(hdlg, 0);
			break;
		}

	}
	return 0;
}

//�����Ի������Ϣ����
BOOL CALLBACK DlgProc2(HWND hdlg,UINT message,WPARAM  wParam,	LPARAM  lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_OK:
			{
				char szStr[20];
				GetDlgItemTextA(hdlg, IDC_EDIT1, szStr, 20);
				StartAddress = atoi(szStr);
				GetDlgItemTextA(hdlg, IDC_EDIT2, szStr, 20);
				Size = atoi(szStr);
				EndDialog(hdlg, 0);
			}
			break;
		case ID_CLOSE:
			EndDialog(hdlg, 0);
			break;
		}

	}
	return 0;
}


// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
