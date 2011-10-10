#include <windows.h>
#include "../../_Common_Files/GenericFakeAPI.h"

#include "HideMe.h"

// You just need to edit this file to add new fake api 
// WARNING YOUR FAKE API MUST HAVE THE SAME PARAMETERS AND CALLING CONVENTION AS THE REAL ONE,
//                  ELSE YOU WILL GET STACK ERRORS

///////////////////////////////////////////////////////////////////////////////
// fake API prototype MUST HAVE THE SAME PARAMETERS 
// for the same calling convention see MSDN : 
// "Using a Microsoft modifier such as __cdecl on a data declaration is an outdated practice"
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI mProcess32NextA(HANDLE hSnapshot,LPPROCESSENTRY32A lppe);
BOOL WINAPI mProcess32NextW(HANDLE hSnapshot,LPPROCESSENTRY32W lppe);

BOOL WINAPI mModule32NextA(HANDLE hSnapshot,LPMODULEENTRY32A lpme);
BOOL WINAPI mModule32NextW(HANDLE hSnapshot,LPMODULEENTRY32W lpme);

BOOL WINAPI mEnumProcesses(DWORD* lpidProcess,DWORD cb,DWORD* cbNeeded);
BOOL WINAPI mEnumProcessModules(HANDLE hProcess,HMODULE* lphModule,DWORD cb,LPDWORD lpcbNeeded);

NTSTATUS WINAPI mNtQuerySystemInformation(
  SYSTEM_INFORMATION_CLASS SystemInformationClass,
  PVOID SystemInformation,
  ULONG SystemInformationLength,
  PULONG ReturnLength
);

BOOL WINAPI mEnumWindows(IN WNDENUMPROC lpEnumFunc,IN LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T("Kernel32.dll"),_T("Process32Next"),(FARPROC)mProcess32NextA,StackSizeOf(HANDLE)+StackSizeOf(LPPROCESSENTRY32A),0},
    {_T("Kernel32.dll"),_T("Process32NextW"),(FARPROC)mProcess32NextW,StackSizeOf(HANDLE)+StackSizeOf(LPPROCESSENTRY32W),0},
    {_T("Kernel32.dll"),_T("Module32Next"),(FARPROC)mModule32NextA,StackSizeOf(HANDLE)+StackSizeOf(LPMODULEENTRY32A),0},
    {_T("Kernel32.dll"),_T("Module32NextW"),(FARPROC)mModule32NextW,StackSizeOf(HANDLE)+StackSizeOf(LPMODULEENTRY32W),0},
    {_T("ntdll.dll"),_T("NtQuerySystemInformation"),(FARPROC)mNtQuerySystemInformation,StackSizeOf(SYSTEM_INFORMATION_CLASS)+StackSizeOf(PVOID)+StackSizeOf(ULONG)+StackSizeOf(PULONG),0},

	{_T("Psapi.dll"),_T("EnumProcesses"),(FARPROC)mEnumProcesses,StackSizeOf(DWORD*)+StackSizeOf(DWORD)+StackSizeOf(DWORD*),0},
	{_T("Psapi.dll"),_T("EnumProcessModules"),(FARPROC)mEnumProcessModules,StackSizeOf(HANDLE)+StackSizeOf(HMODULE*)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD),0},

	{_T("user32.dll"),_T("EnumWindows"),(FARPROC)mEnumWindows,StackSizeOf(WNDENUMPROC)+StackSizeOf(LPARAM),0},


    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// Before API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////// NEW API DEFINITION //////////////////////////////
/////////////////////// You don't need to export these functions //////////////
///////////////////////////////////////////////////////////////////////////////

// this stuff just hide process and loaded dll of apioverride,
// this don't hides named event 
// --> don't think WinAPIOverride32 is fully hidden : it's only a proof of concept

// as we are never first process or first module, it's unused to fake Process32First 
// or Module32First



BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, PVOID pvReserved)
{
	UNREFERENCED_PARAMETER(hInstDLL);
    UNREFERENCED_PARAMETER(pvReserved);
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
                // get func address
                HMODULE hKernel32=GetModuleHandle(_T("kernel32.dll"));
                pProcess32NextA=(pfProcess32NextA)GetProcAddress(hKernel32,"Process32Next");
                pProcess32NextW=(pfProcess32NextW)GetProcAddress(hKernel32,"Process32NextW");
                pModule32NextA=(pfModule32NextA)GetProcAddress(hKernel32,"Module32Next");
                pModule32NextW=(pfModule32NextW)GetProcAddress(hKernel32,"Module32NextW");
                pNtQuerySystemInformation=(pfNtQuerySystemInformation)GetProcAddress(GetModuleHandle(_T("ntdll.dll")),"NtQuerySystemInformation");

                dwCurrentProcessId=GetCurrentProcessId();

                // find explorer pid
                PROCESSENTRY32 pe32 = {0};
                HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
                if (hSnap != INVALID_HANDLE_VALUE) 
                {
                
                    // Fill the size of the structure before using it. 
                    pe32.dwSize = sizeof(PROCESSENTRY32); 
                    
                    // Walk the process list of the system
                    if (Process32First(hSnap, &pe32))
                    {
                        do 
                        {
                            // don't use system processes
                            if ((pe32.th32ProcessID==0)
                                ||(pe32.th32ProcessID==4))
                                continue;
                            if (_tcsicmp(pe32.szExeFile,_T("explorer.exe"))==0)
                            {
                                dwExplorerID=pe32.th32ProcessID;
                                break;
                            }
                        } 
                        while (Process32Next(hSnap, &pe32)); 
                    }

                    // clean up the snapshot object. 
                    CloseHandle (hSnap); 
                }

                // create CProcessAndThreadID object
                ProcessAndThreadID=new CProcessAndThreadID();
            }
            break;
        case DLL_PROCESS_DETACH:
            if (ProcessAndThreadID)
            {
                delete ProcessAndThreadID;
                ProcessAndThreadID=NULL;
            }
            break;
    }

    return TRUE;
}

// fake Process32NextA
BOOL WINAPI mProcess32NextA(HANDLE hSnapshot,LPPROCESSENTRY32A lppe)
{
    BOOL bRet;
    do
    {
        bRet=pProcess32NextA(hSnapshot,lppe);
        if (!bRet)
            break;
        // if process is spied process
        if (lppe->th32ProcessID==dwCurrentProcessId)
            // make as it was launched by the explorer 
            // if we launch it with "attach at application startup" of winapioverride 
            // or if is launched from a debugger, it has the launcher pid instead of explorer pid
            lppe->th32ParentProcessID=dwExplorerID;
    }
    while (stricmp(lppe->szExeFile,"WinAPIOverride32.exe")==0);// hide WinAPIOverride32.exe application
                                                               // by returning the next Process32Next result
    return bRet;
}

// fake Process32NextW
BOOL WINAPI mProcess32NextW(HANDLE hSnapshot,LPPROCESSENTRY32W lppe)
{
    BOOL bRet;
    do
    {
        bRet=pProcess32NextW(hSnapshot,lppe);
        if (!bRet)
            break;
        // if process is spyed process
        if (lppe->th32ProcessID==dwCurrentProcessId)
            // make as it was launched by the explorer 
            // if we launch it with "attach at application statup" of winapioverride 
            // or if is launched from a debugger, it has the launcher pid instead of explorer pid
            lppe->th32ParentProcessID=dwExplorerID;
    }
    while (wcsicmp(lppe->szExeFile,L"WinAPIOverride32.exe")==0);// hide WinAPIOverride32.exe application
                                                                // by returning the next Process32Next result

    return bRet;
}

// fake Module32NextA
BOOL WINAPI mModule32NextA(HANDLE hSnapshot,LPMODULEENTRY32A lpme)
{
    BOOL bRet;
    char* psz;
    // hide dll APIOverride and HideMe from the module list of spyed process id
    do
    {
        bRet=pModule32NextA(hSnapshot,lpme);
        if (!bRet)
            break;
        psz=strrchr(lpme->szExePath,'\\');
        if (psz)
            psz++;
        else
            psz=lpme->szExePath;
    }
    while (
            ((stricmp(psz,"APIOverride.dll")==0)
             ||(stricmp(psz,"HideMe.dll")==0)
             )
            &&(lpme->th32ProcessID==dwCurrentProcessId)
          );
    return bRet;
}

// fake Module32NextW
BOOL WINAPI mModule32NextW(HANDLE hSnapshot,LPMODULEENTRY32W lpme)
{
    BOOL bRet;
    wchar_t* psz;
    // hide dll APIOverride and HideMe from the module list of spyed process id
    do
    {
        bRet=pModule32NextW(hSnapshot,lpme);
        if (!bRet)
            break;
        psz=wcsrchr(lpme->szExePath,'\\');
        if (psz)
            psz++;
        else
            psz=lpme->szExePath;
    }
    while (
            ((wcsicmp(psz,L"APIOverride.dll")==0)
             ||(wcsicmp(psz,L"HideMe.dll")==0)
            )
            &&(lpme->th32ProcessID==dwCurrentProcessId)
          );
    return bRet;
}

// fake EnumProcesses
BOOL WINAPI mEnumProcesses(DWORD* lpidProcess,DWORD cb,DWORD* cbNeeded)
{
    DWORD ArraySize=1024;
    DWORD* ArrayIdProcess=new DWORD[ArraySize];
    DWORD dwRealNeeded=0;
    BOOL bRet;
    DWORD cnt;
    TCHAR ProcessName[MAX_PATH];
    TCHAR* ShortProcessName;
    DWORD ProcessID;
    DWORD cntReturnedArray;

    MODULEENTRY32 me32 = {0}; 
    HANDLE hSnap;
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 

    // we don't now how many WinApiOverride32 are runnings
    bRet=EnumProcesses(ArrayIdProcess,ArraySize*sizeof(DWORD),&dwRealNeeded);
    if (dwRealNeeded>ArraySize*sizeof(DWORD))
    {
        delete[] ArrayIdProcess;
        ArrayIdProcess=new DWORD[dwRealNeeded/sizeof(DWORD)];
        bRet=EnumProcesses(ArrayIdProcess,dwRealNeeded,&dwRealNeeded);
    }

    if (!bRet)
    {
        delete[] ArrayIdProcess;
        return bRet;
    }


    cntReturnedArray=0;
    for (cnt=0;(cnt<(dwRealNeeded/sizeof(DWORD)))&&(cntReturnedArray<(cb/sizeof(DWORD)));cnt++)
    {
        ProcessID=ArrayIdProcess[cnt];

        // can use either Module32First or EnumProcessModules+GetModuleBaseName
        hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,ProcessID);
        if (hSnap == INVALID_HANDLE_VALUE) 
            continue; 
     
        // get first module entry
        if (!Module32First(hSnap, &me32))
        {
            CloseHandle(hSnap);
            continue;
        }
        _tcscpy(ProcessName,me32.szExePath);
        // clean up the snapshot object. 
        CloseHandle (hSnap); 

        // get only file name
        ShortProcessName=_tcsrchr(ProcessName,'\\');
        if (ShortProcessName)
            ShortProcessName++;
        else
            ShortProcessName=ProcessName;

        // if ShortProcessName != "WinAPIOverride32.exe"
        if (_tcsicmp(ShortProcessName,_T("WinAPIOverride32.exe")))
        {
            // store data into array
            lpidProcess[cntReturnedArray]=ProcessID;
            cntReturnedArray++;
        }
    }
    if (cbNeeded!=NULL)
        *cbNeeded=cntReturnedArray*sizeof(DWORD);
    delete[] ArrayIdProcess;
    return TRUE;
}


// fake EnumProcessModules
BOOL WINAPI mEnumProcessModules(HANDLE hProcess,HMODULE* lphModule,DWORD cb,LPDWORD lpcbNeeded)
{

    // if it's not the spied process, there's no one of our module in it --> just return 
    // real EnumProcessModules func
    if (ProcessAndThreadID->GetProcessId(hProcess)!=dwCurrentProcessId)
    {
        return EnumProcessModules(hProcess,lphModule,cb,lpcbNeeded);
    }

    HMODULE* ArrayhModule=new HMODULE[cb/sizeof(HMODULE)+2];
    DWORD dwRealNeeded=0;
    BOOL bRet;
    DWORD cnt;
    DWORD cntReturnedArray;
    TCHAR pszModName[MAX_PATH];
    TCHAR* psz;

    // enum process modules
    bRet=EnumProcessModules(hProcess,ArrayhModule,cb+2*sizeof(HMODULE),&dwRealNeeded);
    if (lpcbNeeded!=NULL)
        *lpcbNeeded=dwRealNeeded-2*sizeof(HMODULE);

    if (!bRet)
    {
        delete[] ArrayhModule;
        return bRet;
    }
    cntReturnedArray=0;
    // remove modules we want to hide
    for (cnt=0;(cnt<dwRealNeeded/sizeof(HMODULE))&&(cntReturnedArray<cb/sizeof(HMODULE));cnt++)
    {           
        // try to get name of the module
        if (GetModuleFileNameEx( hProcess, ArrayhModule[cnt], pszModName,MAX_PATH))
        {
            psz=_tcsrchr(pszModName,'\\');
            if (psz)
                psz++;
            else
                psz=pszModName;

            if (  (_tcsicmp(psz,_T("APIOverride.dll"))==0)
                ||(_tcsicmp(psz,_T("HideMe.dll"))==0)
             )
            {
                // if module match names, don't add it
                continue;
            }
        }
        // module name is ok add it
        lphModule[cntReturnedArray]=ArrayhModule[cnt];
        cntReturnedArray++;
    }


    delete[] ArrayhModule;
    return TRUE;
}

// fake NtQuerySystemInformation
NTSTATUS WINAPI mNtQuerySystemInformation(
  SYSTEM_INFORMATION_CLASS SystemInformationClass,
  PVOID SystemInformation,
  ULONG SystemInformationLength,
  PULONG ReturnLength
)
{
    // if SystemInformationClass is not SystemProcessInformation, just return real values
    if (SystemInformationClass!=SystemProcessInformation)
        return pNtQuerySystemInformation(SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength);

    NTSTATUS Ret=STATUS_SUCCESS;
    ULONG ulReturnLength=0;
    SYSTEM_PROCESSES* ArrayProcessInfo=(SYSTEM_PROCESSES*)new BYTE[SystemInformationLength];
    SYSTEM_PROCESSES* pProcessInfo;
	SYSTEM_PROCESSES* pPreviousProcessInfo=NULL;
	DWORD dwLastItemSize;
	DWORD dwSizeDelta;
	DWORD dwFullSizeRequiered;

    Ret=pNtQuerySystemInformation(SystemInformationClass,ArrayProcessInfo,SystemInformationLength,&ulReturnLength);
    if (SystemInformationLength<ulReturnLength)
    {
        delete[] ArrayProcessInfo;
        ArrayProcessInfo=(SYSTEM_PROCESSES*)new BYTE[ulReturnLength];
        Ret=pNtQuerySystemInformation(SystemInformationClass,ArrayProcessInfo,ulReturnLength,&ulReturnLength);
    }
    if (Ret!=STATUS_SUCCESS)
    {
        delete[] ArrayProcessInfo;
        return Ret;
    }

    // for each process
    pProcessInfo=ArrayProcessInfo;
    dwFullSizeRequiered=0;
	dwSizeDelta=0;

    while(pProcessInfo->NextEntryDelta!=0)
    {

        if (pProcessInfo->ProcessName.Length>0)
        {
            // check process name
            if (wcsicmp(pProcessInfo->ProcessName.Buffer,L"WinapiOverride32.exe")==0)
            {
				dwSizeDelta+=pProcessInfo->NextEntryDelta;// we have assume that pProcessInfo->NextEntryDelta is !=0
                pProcessInfo=(SYSTEM_PROCESSES*)((PBYTE(pProcessInfo))+pProcessInfo->NextEntryDelta);
                continue;
            }
        }

		// check for enought space in output buffer
        if (dwFullSizeRequiered+pProcessInfo->NextEntryDelta>SystemInformationLength)
            Ret=STATUS_INFO_LENGTH_MISMATCH;// continue to retreive full requiered length

        // until there's space inside returned buffer
        if (Ret!=STATUS_INFO_LENGTH_MISMATCH)
            // copy content of data
            memcpy(&(((PBYTE)SystemInformation)[dwFullSizeRequiered]),pProcessInfo,pProcessInfo->NextEntryDelta);

		// increase dwFullSizeRequiered
        dwFullSizeRequiered+=pProcessInfo->NextEntryDelta;


		// store ProcessInfo pointer
		pPreviousProcessInfo=pProcessInfo;

        // get next result
        pProcessInfo=(SYSTEM_PROCESSES*)((PBYTE(pProcessInfo))+pProcessInfo->NextEntryDelta);
    }


	//  last item
    if (pProcessInfo->ProcessName.Length>0)
    {
        // check process name
        if (wcsicmp(pProcessInfo->ProcessName.Buffer,L"WinapiOverride32.exe")==0)
		{
			if (Ret!=STATUS_INFO_LENGTH_MISMATCH)
			{
				// set last next entry delta to 0
				if (pPreviousProcessInfo)
				{
					pProcessInfo=(SYSTEM_PROCESSES*)(&((PBYTE)SystemInformation)[dwFullSizeRequiered-pPreviousProcessInfo->NextEntryDelta]);
					pProcessInfo->NextEntryDelta=0;
				}
			}
			goto end;
		}
	}

	// if process name don't matchs --> copy it
	dwLastItemSize=ulReturnLength-dwFullSizeRequiered-dwSizeDelta;
	// check for enought space in output buffer
    if (dwFullSizeRequiered+dwLastItemSize>SystemInformationLength)
        Ret=STATUS_INFO_LENGTH_MISMATCH;// continue to retreive full requiered length

    // if we get enougth space
    if (Ret!=STATUS_INFO_LENGTH_MISMATCH)
        // copy content of data
        memcpy(&(((PBYTE)SystemInformation)[dwFullSizeRequiered]),pProcessInfo,dwLastItemSize);

	// increase dwFullSizeRequiered
	dwFullSizeRequiered+=dwLastItemSize;


end:
    delete[] ArrayProcessInfo;
    if (ReturnLength!=NULL)
        *ReturnLength=dwFullSizeRequiered;
    
    return Ret;
}




// The following is for window task manager
// It list windows, so just hide WinAPIOverride32 windows from the enum callback
typedef struct tagENUM_FUNC_PARAM
{
	WNDENUMPROC lpEnumFunc;
	LPARAM lParam;
}ENUM_FUNC_PARAM,*PENUM_FUNC_PARAM;

BOOL CALLBACK mEnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	wchar_t psz[MAX_PATH];
	ENUM_FUNC_PARAM param=*((PENUM_FUNC_PARAM)lParam);
	HWND ParenthWnd;
	
	ParenthWnd=GetParent(hwnd);

	// if no more parent window
	if (ParenthWnd==NULL)
	{
		// check window name
		*psz=0;
		InternalGetWindowText(hwnd,psz,MAX_PATH);

		// if it's an instance of WinAPIOverride32, don't call the real callback
		if (wcsnicmp(psz,L"WinAPIOverride32",16)==0)
			return TRUE;
	}

	// call the real callback
	return param.lpEnumFunc(hwnd,param.lParam);
}

BOOL WINAPI mEnumWindows(IN WNDENUMPROC lpEnumFunc,IN LPARAM lParam)
{
	ENUM_FUNC_PARAM param;

	if(IsBadCodePtr((FARPROC)lpEnumFunc))
		return FALSE;

	// store lpEnumFunc and lParam
	param.lpEnumFunc=lpEnumFunc;
	param.lParam=lParam;
	// as call backs are called before EnumWindows returns, 
	// our locally defined param will still be valid in callbacks
	return EnumWindows(mEnumWindowsProc,(LPARAM)&param);
}

// we could continue with WinStationEnumerateProcesses ....