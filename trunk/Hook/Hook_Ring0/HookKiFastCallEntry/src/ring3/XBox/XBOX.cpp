/*++

Module Name:

    XBox.c [ring3 module]

    
Abstract:

    应用层部分，负责跟驱动通信、显示界面等等


Author: 

    Fypher [nmn714@163.com]

    http://hi.baidu.com/nmn714


Time:
    2010/04/16

--*/
#include "XBOX.h"

HANDLE g_hXBox;
HANDLE g_hThread = NULL, g_hProcess = NULL;
HANDLE g_hLogFile = NULL;
DWORD g_dwRet;
TCHAR g_tcsLogFilePath[MAX_PATH];
TCHAR g_tcsCurrenPath[MAX_PATH];

BOOL g_bKKernel, g_bKUser, g_bKSSDT, g_bKSSDTShadow;
//设置图标
inline void SETDLGICONS(HWND hwnd, int idi) {
	SendMessage(hwnd, WM_SETICON, TRUE,  (LPARAM)
		LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		MAKEINTRESOURCE(idi)));
	SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM) 
		LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
		MAKEINTRESOURCE(idi)));
}

//释放资源
BOOL Release(){
	HRSRC res = FindResource(NULL,MAKEINTRESOURCE(IDR_SYS),TEXT("BINARY"));
	if(!res)
		return FALSE;

	HGLOBAL resGlobal = LoadResource(NULL,res);
	if(!resGlobal)
		return FALSE;

	DWORD size=SizeofResource(NULL,res);
	BYTE* ptr=(BYTE*)LockResource(resGlobal);
	if(!ptr)
		return FALSE;

	HANDLE hFile=CreateFile(TEXT("XBox.sys"), GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile==INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dw;
	if(!WriteFile(hFile,ptr,size,&dw,NULL)){
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}

BOOL UnloadDrv(){
	SC_HANDLE      hSCManager;
	SC_HANDLE      hService;
	SERVICE_STATUS ss;
	TCHAR DriverName[]=TEXT("XBox");

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager){
		return FALSE;
	}

	hService = OpenService( hSCManager,DriverName,SERVICE_ALL_ACCESS);
	if( !hService ) {
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	ControlService(hService, SERVICE_CONTROL_STOP, &ss);
	DeleteService(hService);

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return TRUE;
}

BOOL LoadDrv(){
	TCHAR DriverName[]=TEXT("XBox");
	TCHAR DrvFullPathName[MAX_PATH];
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	UnloadDrv();

	if(!Release())
		return FALSE;


	GetFullPathName(TEXT("XBox.sys"), MAX_PATH, DrvFullPathName, NULL);
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!schSCManager)
		return FALSE;

	schService = CreateService( 
		schSCManager,DriverName,DriverName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		DrvFullPathName,
		NULL,NULL,NULL,NULL,NULL
		);

	if (!schService){
		if (GetLastError() == ERROR_SERVICE_EXISTS){
			schService = OpenService(schSCManager,DriverName,SERVICE_ALL_ACCESS);
			if (!schService){
				CloseServiceHandle(schSCManager);
				return FALSE;
			}
		}else{
			CloseServiceHandle(schSCManager);
			return FALSE;
		}
	}

	if (!StartService(schService,0,NULL)){
		if ( !(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING ) ){
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}

VOID LogInfo(HWND hwnd) {
	XBoxData data;
	TCHAR msg[1024];
	int nLength;
	static PTSTR ptstrKernel =      TEXT("From Kernel Mode");
	static PTSTR ptstrUser =        TEXT("From User Mode");
	static PTSTR ptstrSSDT =        TEXT("From SSDT Table");
	static PTSTR ptstrSSDTShadow =  TEXT("From SSDTShadow Table");
	PTSTR ptstrFrom, ptstrTable, *pptstrTable;

	HWND hEditCtrl = GetDlgItem(hwnd, IDC_CONTENT);
	while (1) {
		BOOL bRet = DeviceIoControl(g_hXBox, IOCTL_XBOX_READ,
			NULL, 0, &data, sizeof(data), &g_dwRet, NULL);
		

		if (!bRet) {
			//__asm int 3;
			_stprintf_s(msg, _countof(msg), TEXT("\r\nProcess Terminated!\r\n")
				TEXT("Log File is: %s\r\n\r\n"), g_tcsLogFilePath);
			nLength = Edit_GetTextLength(hEditCtrl); 
			Edit_SetSel(hEditCtrl, nLength, nLength);
			Edit_ReplaceSel(hEditCtrl, msg);
			WriteFile(g_hLogFile, msg, _tcslen(msg) * sizeof(TCHAR), &g_dwRet, NULL);
			return;
		} 
		
		if (g_dwRet == 0) {
			continue;
		}
		if (data.bFromUser == 0) {
			if (g_bKKernel)
				continue;
			ptstrFrom = ptstrKernel;
		}
		else {
			if (g_bKUser)
				continue;	
			ptstrFrom = ptstrUser;
		}
		if (data.bFromSSDT == 0) {
			if (g_bKSSDTShadow || data.argc > _countof(XPSSDTShadow))
				continue;
			ptstrTable = ptstrSSDTShadow;
			pptstrTable = XPSSDTShadow;
		}
		else {
			if (g_bKSSDT || data.argc > _countof(XPSSDT))
				continue;
			ptstrTable = ptstrSSDT;
			pptstrTable = XPSSDT;
		}
		
		
		LARGE_INTEGER liLocalTime;
		TIME_FIELDS TimeFields;
		int nowLength = Edit_GetTextLength(hEditCtrl);

		// 调用信息
		_stprintf_s(msg, _countof(msg), 
			TEXT("PID: %d, TID: %d,	%s, %s\r\n"),
			data.pid, data.tid, ptstrFrom, ptstrTable);
		nLength = Edit_GetTextLength(hEditCtrl); 
		Edit_SetSel(hEditCtrl, nLength, nLength);
		Edit_ReplaceSel(hEditCtrl, msg);
		WriteFile(g_hLogFile, msg, _tcslen(msg) * sizeof(TCHAR), &g_dwRet, NULL);

		// 函数及参数信息
		_stprintf_s(msg, _countof(msg), 
			TEXT("函数名: %s,		参数个数: %d,	功能号: %d\r\n"), 
			pptstrTable[data.sid], data.argc, data.sid);

		for (ULONG i = 0; i < data.argc; ++i) {
			_stprintf_s(msg, _countof(msg), TEXT("%s参数%d: 0x%08x    "), msg, i, data.args[i]);
		}
		_tcscat_s(msg, _countof(msg), TEXT("\r\n"));

		nLength = Edit_GetTextLength(hEditCtrl); 
		Edit_SetSel(hEditCtrl, nLength, nLength);
		Edit_ReplaceSel(hEditCtrl, msg);
		WriteFile(g_hLogFile, msg, _tcslen(msg) * sizeof(TCHAR), &g_dwRet, NULL);

		// 时间信息
		if (RtlSystemTimeToLocalTime && RtlTimeToTimeFields) {
			__asm {
				//int 3;
				push eax
				lea eax, liLocalTime
				push eax
				lea eax, data.time
				push eax
				mov eax, RtlSystemTimeToLocalTime
				call eax
				lea eax, TimeFields
				push eax
				lea eax, liLocalTime
				push eax
				mov eax, RtlTimeToTimeFields
				call eax
				pop eax
			}
			_stprintf_s(msg, _countof(msg), 
				TEXT("调用时间: %02d:%02d:%02d.%03d\r\n\r\n"),
				TimeFields.Hour, TimeFields.Minute, 
				TimeFields.Second, TimeFields.Milliseconds);
			// append text
			nLength = Edit_GetTextLength(hEditCtrl); 
			Edit_SetSel(hEditCtrl, nLength, nLength);
			Edit_ReplaceSel(hEditCtrl, msg);
			WriteFile(g_hLogFile, msg, _tcslen(msg) * sizeof(TCHAR), &g_dwRet, NULL);

		}
		if (nLength <= nowLength)
			SetDlgItemText(hwnd, IDC_CONTENT, TEXT(""));
	}
}

DWORD WINAPI RunProcessThread(PVOID pvParam) {
	HWND hwnd = (HWND)pvParam;
	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi;

	if (Edit_GetTextLength(GetDlgItem(hwnd, IDC_PID)) == 0 &&
		Edit_GetTextLength(GetDlgItem(hwnd, IDC_PATH)) == 0    ) {
		MessageBox(hwnd, TEXT("请指定要监控的exe文件或要附加的进程ID"), TEXT("err"), MB_OK);
		CloseHandle(g_hThread);
		g_hThread = NULL;
		EnableWindow(GetDlgItem(hwnd, IDC_PATH), TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_PID), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_RUN), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_PAUSE), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);
		return 0;
	}
	// 创建日志文件
	TCHAR tcsPath[MAX_PATH];
	GetDlgItemText(hwnd, IDC_PATH, tcsPath, _countof(tcsPath));
	if (_tcslen(tcsPath) == 0) {
		_stprintf_s(tcsPath, _countof(tcsPath),
			TEXT("[PID_%d]"), GetDlgItemInt(hwnd, IDC_PID, NULL, FALSE));
	}
	PTSTR ptstrTmp = _tcsrchr(tcsPath, TEXT('\\'));
	if (ptstrTmp == NULL)
		ptstrTmp = tcsPath;
	else
		ptstrTmp++;
	_stprintf_s(g_tcsLogFilePath, _countof(g_tcsLogFilePath),
		TEXT("%s%s.txt"), g_tcsCurrenPath, ptstrTmp);
	g_hLogFile = CreateFile(g_tcsLogFilePath, GENERIC_WRITE, FILE_SHARE_READ, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (g_hLogFile == INVALID_HANDLE_VALUE) {
		MessageBox(hwnd, TEXT("日志文件创建失败"), TEXT("err"), MB_OK);
		CloseHandle(g_hThread);
		g_hThread = NULL;
		EnableWindow(GetDlgItem(hwnd, IDC_PATH), TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_PID), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_RUN), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_PAUSE), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);
		return 0;
	}

	// 创建进程或附加进程
	ULONG ulPid;
	if (Edit_GetTextLength(GetDlgItem(hwnd, IDC_PID)) != 0) {
		ulPid = GetDlgItemInt(hwnd, IDC_PID, NULL, FALSE);
		g_hProcess = NULL;
	}
	else if (Edit_GetTextLength(GetDlgItem(hwnd, IDC_PATH)) != 0) {
		BOOL bRet = CreateProcess(NULL, tcsPath, NULL, 
			NULL, FALSE, CREATE_SUSPENDED,NULL, NULL, &si, &pi);
		if (!bRet) {
			MessageBox(hwnd, TEXT("创建进程失败"), TEXT("err"), MB_OK);
			CloseHandle(g_hThread);
			g_hThread = NULL;
			EnableWindow(GetDlgItem(hwnd, IDC_PATH), TRUE);
			EnableWindow(GetDlgItem(hwnd,IDC_PID), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_RUN), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_PAUSE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);
			return 0;
		}
		ulPid = pi.dwProcessId;
		g_hProcess = pi.hProcess;
	}

	


	BOOL bRet = DeviceIoControl(g_hXBox, IOCTL_XBOX_SETPID, 
		&ulPid, sizeof(ulPid), NULL, 0, &g_dwRet, NULL);

	if (!bRet) {
		MessageBox(hwnd, TEXT("无法与设备通信"), TEXT("err"), MB_OK);
		if (pi.hProcess) {
			TerminateProcess(pi.hProcess, 0);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		CloseHandle(g_hThread);
		CloseHandle(g_hLogFile);
		g_hThread = g_hLogFile = NULL;
		EnableWindow(GetDlgItem(hwnd,IDC_PATH), TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_PID), TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_BROWSE), TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_RUN), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_PAUSE), FALSE);
		EnableWindow(GetDlgItem(hwnd,IDC_STOP), FALSE);
		return 0;
	}

	TCHAR ptcsMsg[MAX_PATH];
	_stprintf_s(ptcsMsg, _countof(ptcsMsg), TEXT("Now Monitoring Process: %s\r\n")
		TEXT("Log File is: %s\r\n\r\n"), tcsPath, g_tcsLogFilePath);
	
	SetDlgItemText(hwnd, IDC_CONTENT, ptcsMsg);
	WriteFile(g_hLogFile, ptcsMsg, _tcslen(ptcsMsg) * sizeof(TCHAR), &g_dwRet, NULL);
	//UpdateWindow(GetDlgItem(hwnd, IDC_CONTENT));
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	LogInfo(hwnd);

	ulPid = 0xFFFFFFFF;

	DeviceIoControl(g_hXBox, IOCTL_XBOX_SETPID, 
		&ulPid, sizeof(ulPid), NULL, 0, &g_dwRet, NULL);
	if (g_hProcess) {
		TerminateProcess(g_hProcess, 0);
		WaitForSingleObject(g_hProcess, INFINITE);
		CloseHandle(g_hProcess);
	}
	CloseHandle(g_hThread);
	CloseHandle(g_hLogFile);
	g_hProcess = g_hThread = g_hLogFile = NULL;

	EnableWindow(GetDlgItem(hwnd, IDC_PATH), TRUE);
	EnableWindow(GetDlgItem(hwnd,IDC_PID), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_RUN), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_PAUSE), FALSE);
	EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);

	return 0;
}

//窗口消息回调函数
INT_PTR WINAPI Dlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static OPENFILENAME ofn = { sizeof(OPENFILENAME) };
	TCHAR tcsFilePath[MAX_PATH];
	TCHAR msg[MAX_PATH];
	int nLength;
	switch (uMsg) {
		case WM_INITDIALOG:
			SETDLGICONS(hwnd,IDI_XBOX);
			CheckDlgButton(hwnd, IDC_KKERNEL, TRUE);
			CheckDlgButton(hwnd, IDC_KSHADOW, TRUE);
			Edit_LimitText(GetDlgItem(hwnd, IDC_CONTENT), 0);
			return TRUE;
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_BROWSE) {
				GetDlgItemText(hwnd, IDC_PATH, tcsFilePath, _countof(tcsFilePath));
				ofn.hwndOwner = hwnd;
				ofn.lpstrFile = tcsFilePath;
				ofn.lpstrFile[0] = 0;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = TEXT("浏览啊浏览~");
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
				if (GetOpenFileName(&ofn))
					SetDlgItemText(hwnd, IDC_PATH, tcsFilePath);
			}
			else if (LOWORD(wParam) == IDC_RUN) {
				g_bKKernel = IsDlgButtonChecked(hwnd, IDC_KKERNEL);
				g_bKUser = IsDlgButtonChecked(hwnd, IDC_KUSER);
				g_bKSSDT = IsDlgButtonChecked(hwnd, IDC_KSSDT);
				g_bKSSDTShadow = IsDlgButtonChecked(hwnd, IDC_KSHADOW);

				EnableWindow(GetDlgItem(hwnd, IDC_PATH), FALSE);
				EnableWindow(GetDlgItem(hwnd,IDC_PID), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_RUN), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_KUSER), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PAUSE), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_STOP), TRUE);

				if (g_hThread == NULL)
					g_hThread = BEGINTHREADEX(NULL, 0, RunProcessThread, (PVOID)hwnd, 0, NULL);
				else
					ResumeThread(g_hThread);
				
				
			}
			else if (LOWORD(wParam) == IDC_PAUSE) {
				if (g_hThread == NULL)
					break;
				SuspendThread(g_hThread);
				
				EnableWindow(GetDlgItem(hwnd,IDC_RUN), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_PAUSE), FALSE);
			}
			else if (LOWORD(wParam) == IDC_STOP) {
				

				if (g_hThread) {
					TerminateThread(g_hThread, 0);
					WaitForSingleObject(g_hThread, INFINITE);
					CloseHandle(g_hThread);
					g_hThread = NULL;
				}
				if (g_hProcess) {
					TerminateProcess(g_hProcess, 0);
					WaitForSingleObject(g_hProcess, INFINITE);
					CloseHandle(g_hProcess);
					g_hProcess = NULL;
				}

				ULONG ulPid = 0xFFFFFFFF;
				DeviceIoControl(g_hXBox, IOCTL_XBOX_SETPID, 
					&ulPid, sizeof(ulPid), NULL, 0, &g_dwRet, NULL);

				_stprintf_s(msg, _countof(msg), TEXT("\r\nMonitor Terminated!\r\n")
					TEXT("Log File is: %s\r\n\r\n"), g_tcsLogFilePath);
				nLength = Edit_GetTextLength(GetDlgItem(hwnd, IDC_CONTENT)); 
				Edit_SetSel(GetDlgItem(hwnd, IDC_CONTENT), nLength, nLength);
				Edit_ReplaceSel(GetDlgItem(hwnd, IDC_CONTENT), msg);
				
				if (g_hLogFile != NULL) {
					WriteFile(g_hLogFile, msg, _tcslen(msg) * sizeof(TCHAR), &g_dwRet, NULL);
					CloseHandle(g_hLogFile);
					g_hLogFile = NULL;
				}

				EnableWindow(GetDlgItem(hwnd, IDC_PATH), TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_PID), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_RUN), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KKERNEL), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KUSER), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KSSDT), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_KSHADOW), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PAUSE), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);
			}
			else if (LOWORD(wParam) == IDC_PATH) {
				if (Edit_GetTextLength((HWND)lParam) > 0) {
					SetDlgItemText(hwnd, IDC_PID, TEXT(""));
				}
			}
			else if (LOWORD(wParam) == IDC_PID) {
				if (Edit_GetTextLength((HWND)lParam) > 0) {
					SetDlgItemText(hwnd, IDC_PATH, TEXT(""));
				}
			}
			break;
		case WM_CLOSE:
			if (IsWindowEnabled(GetDlgItem(hwnd, IDC_STOP))) {
				FORWARD_WM_COMMAND(hwnd, IDC_STOP, GetDlgItem(hwnd, IDOK), BN_CLICKED, 
					SendMessage);
			}
			EndDialog(hwnd,0);
			break;
	}
	return FALSE;
}

VOID InitFuncionts() {
	HMODULE hMod = GetModuleHandle(TEXT("ntdll.dll"));
	if (hMod) {
		RtlSystemTimeToLocalTime = (ULONG)GetProcAddress(hMod, "RtlSystemTimeToLocalTime");
		RtlTimeToTimeFields = (ULONG)GetProcAddress(hMod, "RtlTimeToTimeFields");
	}
}

BOOL Is360Present() {
	HANDLE h360 = CreateMutex(NULL, FALSE, TEXT("Q360MonMutex"));
	BOOL bRet = (GetLastError() == ERROR_ALREADY_EXISTS);
	if (h360)
		CloseHandle(h360);
	return bRet;
}


int WINAPI _tWinMain(HINSTANCE hinstExe, HINSTANCE, PTSTR pszCmdLine, int) {
	InitFuncionts();

	// 判断实例是否唯一
	HWND hwnd = FindWindow(TEXT("#32770"),TEXT("XBox 1.0"));
	if(IsWindow(hwnd)){
		SetForegroundWindow(hwnd);
		return 0;
	}
	
	// 检测 360
	if (Is360Present()) {
		int nRet = MessageBox(
			NULL,
			TEXT("本产品在运行过程中，会使“360安全卫士”的主动防御暂时失效。"),
			TEXT("提示"),
			MB_OKCANCEL | MB_ICONINFORMATION);
		if (nRet != IDOK)
			return 0;
	}
	
	// 释放资源
	if(!Release()) {
		MessageBox(NULL,TEXT("释放文件失败"),TEXT("error"),MB_OK);
		return 0;
	}
	
	// 加载驱动
	if(!LoadDrv()) {
		MessageBox(NULL,
			TEXT("加载驱动失败，可能原因：\r\n")
			TEXT("1.权限不够；\r\n")
			TEXT("2.系统不支持MSR；\r\n")
			TEXT("3.被主动防御拦截；\r\n"),
			TEXT("error"),MB_OK);
		UnloadDrv();
		return 0;
	}
	
	g_hXBox = CreateFile( TEXT("\\\\.\\XBox"), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(g_hXBox == INVALID_HANDLE_VALUE) {
		MessageBox(NULL, TEXT("打开设备失败"), TEXT("error"), MB_OK);
		return 0;
	}
	
	GetModuleFileName(NULL, g_tcsCurrenPath, _countof(g_tcsCurrenPath));
	PTSTR ptstrTmp = _tcsrchr(g_tcsCurrenPath, TEXT('\\'));
	ptstrTmp[1] = 0;

	//显示主界面
	DialogBox(hinstExe, MAKEINTRESOURCE(IDD_XBOX), NULL, Dlg_Proc);
	
	CloseHandle(g_hXBox);
	// 卸载驱动
	UnloadDrv();

	return 0;
}