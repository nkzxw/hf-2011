// Test.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "Test.h"
#include <commdlg.h>
#include <iostream>
#include "../Dbg/Dbg.h"

#include "disasm.h"
#pragma comment(lib,"Decode.lib")

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
DWORD AddressOfBreak;							// 保存断点地址
DWORD StartAddress;								// 保存反汇编的开始地址
DWORD Size;										// 保存反汇编的长度

// 此代码模块中包含的函数的前向声明:
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

	// TODO: 在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEST));

	// 主消息循环:
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
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
//    仅当希望
//    此代码与添加到 Windows 95 中的“RegisterClassEx”
//    函数之前的 Win32 系统兼容时，才需要此函数及其用法。调用此函数十分重要，
//    这样应用程序就可以获得关联的
//    “格式正确的”小图标。
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
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	HWND hEdit;

	hInst = hInstance; // 将实例句柄存储在全局变量中

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
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	OPENFILENAME stOFN;
	char stFilePath[100];	//保存文件路径
	char szBuf[1000];		//保存内存读取来的数据

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
		// 分析菜单选择:
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
					MessageBox(hWnd, _T("也许你还没有加载进程"), _T("出错"), MB_OK);
					break;
				}
				char sz[10];				//保存地址
				DWORD OutAddress = NULL;	//保存输出地址

				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DlgProc2);
				if(StartAddress == 0)
					break;				//判断是否成功加载进程
				if(!ReadProcessMemory(stProcessInfo.hProcess, (LPCVOID)StartAddress, szBuf, Size, NULL))
				{
					MessageBox(NULL, _T("读取内存失败，你确认你的权限"), _T("出错"), MB_OK);
				}
				char *Linear=szBuf;		//Pointer to linear address
				DWORD Index=0;			// Index of opcoded to decode
				DISASSEMBLY dis;
				std::string out;		//输出字符串
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
		// TODO: 在此添加任意绘图代码...
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
//断点输入对话框的消息处理函数
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

//反汇编对话框的消息处理
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


// “关于”框的消息处理程序。
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
