#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <stdio.h>
#include <Shlobj.h>
#pragma comment(lib,"Shell32.lib")

#include "../../../File/StdFileOperations.h"
#include "../../../String/AnsiUnicodeConvert.h"
#include "../../MailSlot/MailSlotClient.h"
#include "../../MailSlot/MailSlotServer.h"
#include "../../../Privilege/Privilege.h"

#define CModulesInfos_ALLOW_INTERNAL_ACCESS
#include "../../ModulesInfos/ModulesInfos.h"
#include "../../PEB_TEB/PEB_TEB.h"

#include "../x86CrossCompatibilityInterProcessCom.h"

class CmdLineOptions
{
public:
	TCHAR MailSlotsBaseName[MAX_PATH];

	CmdLineOptions()
	{
		*this->MailSlotsBaseName=0;
	}
	BOOL Check()
	{
		return (*this->MailSlotsBaseName!=0);
	}
};

// return : TRUE on SUCCESS, FALSE on error
BOOL ParseCommandLine(IN OUT CmdLineOptions* pCmdLineOptions)
{
	BOOL bRes=TRUE;
	WCHAR pszError[MAX_PATH];
	LPWSTR* lppwargv;
	int argc=0;
	int cnt;

	// use CommandLineToArgvW
	// Notice : CommandLineToArgvW translate 'option="a";"b"' into 'option=a;b'
	lppwargv=CommandLineToArgvW(
		GetCommandLineW(),
		&argc
		);
	// if no params
	if (argc<=1)
	{
		LocalFree(lppwargv);
		return FALSE;
	}
#ifdef _DEBUG
    ::MessageBox(NULL,_T("Message Box For Debug"),_T("x86 CrossCompatibility"),MB_OK|MB_ICONINFORMATION);
#endif
	// for each param
	for(cnt=1;cnt<argc;cnt++)// cnt[0] is app name
	{
		if (wcsnicmp(lppwargv[cnt],L"MailSlotsBaseName",wcslen(L"MailSlotsBaseName"))==0)
		{
			WCHAR* pwc;
			int iScanfRes=0;
			pwc=wcschr(lppwargv[cnt],'=');
			if (pwc)
			{
				// point after "="
				pwc++;
                CAnsiUnicodeConvert::UnicodeToTchar(pwc,pCmdLineOptions->MailSlotsBaseName,MAX_PATH);
			}
			if ((pwc==NULL)||(*pCmdLineOptions->MailSlotsBaseName==0))
			{
				_snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
				OutputDebugStringW(pszError);
				bRes=FALSE;
				break;
			}
		} 
 
		else
		{
			_snwprintf(pszError,MAX_PATH,L"Unknown command line option %s",lppwargv[cnt]);
			OutputDebugStringW(pszError);
			bRes=FALSE;
			break;
		}
	}
	LocalFree(lppwargv);    
	return pCmdLineOptions->Check();
}



#define USED_TYPE UINT64

typedef LONG (WINAPI *ptrNtQueryInformationProcess) (
    IN HANDLE ProcessHandle,
    IN SIZE_T ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

// cause ms developer are so stupid that they don't give access to Module32FirstA in unicode version
typedef BOOL (WINAPI *ptrModule32FirstA)(
    HANDLE hSnapshot,
    LPMODULEENTRY32 lpme
    );

typedef BOOL (WINAPI *ptrModule32NextA)(
    HANDLE hSnapshot,
    LPMODULEENTRY32 lpme
    );

void MailSlotCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData);
ptrNtQueryInformationProcess pNtQueryInformationProcess = 0;

ptrModule32FirstA pModule32FirstA = 0;
ptrModule32NextA pModule32NextA = 0;

HANDLE hEvtStop =0;
CMailSlotClient* pMailSlotReply=0;

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
	)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);


    pNtQueryInformationProcess=(ptrNtQueryInformationProcess)GetProcAddress(GetModuleHandle(_T("Ntdll.dll")),"NtQueryInformationProcess");
    pModule32FirstA = (ptrModule32FirstA)GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"Module32First");
    pModule32NextA = (ptrModule32NextA)GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"Module32Next");

    // gives the debug privileges
    CPrivilege Privilege(FALSE);
    Privilege.SetPrivilege(SE_DEBUG_NAME,TRUE);

    // parse command lines
	CmdLineOptions Options;
	if (!ParseCommandLine(&Options))
		return -1;

    // create mailslots
    TCHAR MailSlotName[MAX_PATH];

    _sntprintf(MailSlotName,_countof(MailSlotName),_T("\\\\.\\mailslot\\%sReply"),Options.MailSlotsBaseName);
    pMailSlotReply = new CMailSlotClient( MailSlotName) ;
    pMailSlotReply->Open();

    _sntprintf(MailSlotName,_countof(MailSlotName),_T("\\\\.\\mailslot\\%sQuery"),Options.MailSlotsBaseName);
    CMailSlotServer* pMailSlotQuery = new CMailSlotServer( MailSlotName,MailSlotCallback,NULL );
    pMailSlotQuery->Start(FALSE);

    hEvtStop = ::CreateEvent(0,0,0,0);


    HANDLE hSubProcessReady = ::OpenEvent(EVENT_MODIFY_STATE,FALSE,Options.MailSlotsBaseName);
    ::SetEvent(hSubProcessReady);
    ::CloseHandle(hSubProcessReady);

    ::WaitForSingleObject(hEvtStop,INFINITE);
    ::CloseHandle(hEvtStop);

    delete pMailSlotQuery;
    delete pMailSlotReply;

    return 0;
}

void MailSlotCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData)
{
    if (dwDataSize<sizeof(x86CrossCompatibility::BASE_HEADER))
    {
#ifdef _DEBUG
        ::DebugBreak();
#endif
        return;
    }

    x86CrossCompatibility::BASE_HEADER* pHeader;
    pHeader = (x86CrossCompatibility::BASE_HEADER*)pData;
    switch(pHeader->CommandId)
    {
    case x86CrossCompatibility::COMMAND_ID_Stopx86CrossCompatibilityProcess:
        ::SetEvent(hEvtStop);
        break;
    case x86CrossCompatibility::COMMAND_ID_CreateToolhelp32Snapshot:
        {
            x86CrossCompatibility::CreateToolhelp32SnapshotQuery* pQuery;
            x86CrossCompatibility::CreateToolhelp32SnapshotReply Reply;
            pQuery = (x86CrossCompatibility::CreateToolhelp32SnapshotQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.Handle = (__int64) ::CreateToolhelp32Snapshot(pQuery->dwFlags,pQuery->th32ProcessID);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
                break;
        }
    case x86CrossCompatibility::COMMAND_ID_Module32FirstA:
        {
            x86CrossCompatibility::Module32FirstQuery* pQuery;
            x86CrossCompatibility::Module32FirstAReply Reply;
            pQuery = (x86CrossCompatibility::Module32FirstQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.me.dwSize = sizeof(Reply.me);
            Reply.RetValue = pModule32FirstA((HANDLE)pQuery->hSnapshot,(LPMODULEENTRY32)&Reply.me);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_Module32FirstW:
        {
            x86CrossCompatibility::Module32FirstQuery* pQuery;
            x86CrossCompatibility::Module32FirstWReply Reply;
            pQuery = (x86CrossCompatibility::Module32FirstQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.me.dwSize = sizeof(Reply.me);
            Reply.RetValue = ::Module32FirstW((HANDLE)pQuery->hSnapshot,(LPMODULEENTRY32)&Reply.me);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_Module32NextA:
        {
            x86CrossCompatibility::Module32NextQuery* pQuery;
            x86CrossCompatibility::Module32NextAReply Reply;
            pQuery = (x86CrossCompatibility::Module32NextQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.me.dwSize = sizeof(Reply.me);
            Reply.RetValue = pModule32NextA((HANDLE)pQuery->hSnapshot,(LPMODULEENTRY32)&Reply.me);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_Module32NextW:
        {
            x86CrossCompatibility::Module32NextQuery* pQuery;
            x86CrossCompatibility::Module32NextWReply Reply;
            pQuery = (x86CrossCompatibility::Module32NextQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.me.dwSize = sizeof(Reply.me);
            Reply.RetValue = ::Module32NextW((HANDLE)pQuery->hSnapshot,(LPMODULEENTRY32)&Reply.me);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_CloseHandle:
        {
            x86CrossCompatibility::CloseHandleQuery* pQuery;
            x86CrossCompatibility::CloseHandleReply Reply;
            pQuery = (x86CrossCompatibility::CloseHandleQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.RetValue = ::CloseHandle((HANDLE)pQuery->Handle);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_VirtualProtectEx:
        {
            x86CrossCompatibility::VirtualProtectExQuery* pQuery;
            x86CrossCompatibility::VirtualProtectExReply Reply;
            pQuery = (x86CrossCompatibility::VirtualProtectExQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.RetValue = ::VirtualProtectEx((HANDLE)pQuery->hProcess,(LPVOID)pQuery->lpAddress,pQuery->dwSize,pQuery->flNewProtect,&Reply.OldProtect);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_VirtualQueryEx:
        {
            x86CrossCompatibility::VirtualQueryExQuery* pQuery;
            x86CrossCompatibility::VirtualQueryExReply Reply;
            pQuery = (x86CrossCompatibility::VirtualQueryExQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.RetValue = ::VirtualQueryEx((HANDLE)pQuery->hProcess,(LPVOID)pQuery->lpAddress,(PMEMORY_BASIC_INFORMATION)&Reply.lpBuffer,sizeof(Reply.lpBuffer));
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_ReadProcessMemory:
        {
            x86CrossCompatibility::ReadProcessMemoryQuery* pQuery;
            x86CrossCompatibility::ReadProcessMemoryReply Reply;
            pQuery = (x86CrossCompatibility::ReadProcessMemoryQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.NumberOfBytesRead = 0;
            PBYTE Buffer = new BYTE[sizeof(Reply)+pQuery->nSize];
            if (Buffer)
            {
                Reply.RetValue = ::ReadProcessMemory((HANDLE)pQuery->hProcess,(LPVOID)pQuery->lpBaseAddress,&Buffer[sizeof(Reply)],pQuery->nSize,(SIZE_T*)&Reply.NumberOfBytesRead);
                memcpy(Buffer,&Reply,sizeof(Reply));
                pMailSlotReply->Write(Buffer,sizeof(Reply)+Reply.NumberOfBytesRead);
                delete [] Buffer;
            }
            else
            {
                Reply.RetValue=0;
                pMailSlotReply->Write(Buffer,sizeof(Reply));
            }
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_WriteProcessMemory:
        {
            x86CrossCompatibility::WriteProcessMemoryQuery* pQuery;
            x86CrossCompatibility::WriteProcessMemoryReply Reply;
            pQuery = (x86CrossCompatibility::WriteProcessMemoryQuery*)pData;
            PBYTE Buffer = (PBYTE)pQuery+sizeof(x86CrossCompatibility::WriteProcessMemoryQuery);
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.RetValue = ::WriteProcessMemory((HANDLE)pQuery->hProcess,(LPVOID)pQuery->lpBaseAddress,Buffer,pQuery->nSize,(SIZE_T*)&Reply.NumberOfBytesWritten);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }
    case x86CrossCompatibility::COMMAND_ID_OpenProcess:
        {
            x86CrossCompatibility::OpenProcessQuery* pQuery;
            x86CrossCompatibility::OpenProcessReply Reply;
            pQuery = (x86CrossCompatibility::OpenProcessQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.RetValue = (__int64)::OpenProcess(pQuery->dwDesiredAccess,pQuery->bInheritHandle,pQuery->dwProcessId);
            pMailSlotReply->Write(&Reply,sizeof(Reply));
            break;
        }

    case x86CrossCompatibility::COMMAND_ID_NtQueryInformationProcess:
        {
            x86CrossCompatibility::NtQueryInformationProcessQuery* pQuery;
            x86CrossCompatibility::NtQueryInformationProcessReply Reply;
            pQuery = (x86CrossCompatibility::NtQueryInformationProcessQuery*)pData;
            Reply.Header.CommandId = pQuery->Header.CommandId;
            Reply.ReturnLength = 0;
            PBYTE Buffer = new BYTE[sizeof(Reply)+pQuery->ProcessInformationLength];
            if (Buffer)
            {
                Reply.RetValue = pNtQueryInformationProcess((HANDLE)pQuery->ProcessHandle,pQuery->ProcessInformationClass,&Buffer[sizeof(Reply)],pQuery->ProcessInformationLength,&Reply.ReturnLength);
                memcpy(Buffer,&Reply,sizeof(Reply));
                pMailSlotReply->Write(Buffer,sizeof(Reply)+Reply.ReturnLength);
                delete [] Buffer;
            }
            else
            {
                Reply.RetValue=0;
                pMailSlotReply->Write(Buffer,sizeof(Reply));
            }
            break;
        }
    }
}