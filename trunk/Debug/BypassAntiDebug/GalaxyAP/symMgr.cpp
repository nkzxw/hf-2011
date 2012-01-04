#include "StdAfx.h"
#include "commhdr.h"

char osname[1024];

NTSTATUS  (WINAPI *PsResumeThread)(IN ULONG,OUT ULONG PreviousSuspendCount OPTIONAL);
NTSTATUS  (WINAPI *PsTerminateProcess)(IN ULONG Process,IN ULONG Status);
ULONG (WINAPI *PsGetNextProcess) (IN ULONG Process);
PETHREAD  (WINAPI *PsGetNextProcessThread) (IN ULONG Process,IN ULONG Thread);
NTSTATUS  (WINAPI *PsSuspendThread) (IN ULONG Thread,OUT ULONG PreviousSuspendCount );

NTSTATUS  (WINAPI *MmGetFileNameForAddress) (IN PVOID ProcessVa,OUT ULONG FileName);
NTSTATUS  (WINAPI *ObDuplicateObject) (IN ULONG SourceProcess,IN HANDLE SourceHandle,IN ULONG TargetProcess OPTIONAL,OUT PHANDLE TargetHandle OPTIONAL,IN ACCESS_MASK DesiredAccess,IN ULONG HandleAttributes,IN ULONG Options,IN KPROCESSOR_MODE PreviousMode );

NTSTATUS (WINAPI *LpcRequestWaitReplyPortEx) (IN PVOID PortAddress,IN PPORT_MESSAGE RequestMessage,OUT PPORT_MESSAGE ReplyMessage);
VOID      (WINAPI *PsQuitNextProcessThread) (IN PETHREAD Thread);
VOID      (WINAPI *PsCallImageNotifyRoutines)(IN PUNICODE_STRING FullImageName,IN HANDLE ProcessId, IN ULONG ImageInfo );


NTSTATUS  (WINAPI * MmCopyVirtualMemory)(IN ULONG FromProcess,IN CONST VOID *FromAddress,IN ULONG ToProcess,OUT PVOID ToAddress,IN SIZE_T BufferSize,IN KPROCESSOR_MODE PreviousMode,OUT PSIZE_T NumberOfBytesCopied);

VOID      (WINAPI *KeFreezeAllThreads) (VOID);
VOID      (WINAPI *KeThawAllThreads) (VOID);
VOID      (FASTCALL  *KiReadyThread)(IN PKTHREAD Thread);
VOID      (WINAPI * KiSetSwapEvent)();
BOOLEAN   (WINAPI *KiSwapProcess) (IN PKPROCESS NewProcess,IN PKPROCESS OldProcess);
int       (FASTCALL *KiSwapThread) ();
NTSTATUS  (FASTCALL *MiMakeProtectionMask)(unsigned int a1);
NTSTATUS  (WINAPI*MiProtectVirtualMemory) (IN ULONG Process,IN PVOID *BaseAddress,IN ULONG RegionSize,IN ULONG NewProtectWin32,IN ULONG LastProtect);
NTSTATUS  (WINAPI *PspCreateThread)(OUT PHANDLE ThreadHandle,IN ACCESS_MASK DesiredAccess,IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,IN HANDLE ProcessHandle,IN ULONG ProcessPointer,OUT PCLIENT_ID ClientId OPTIONAL,IN PCONTEXT ThreadContext OPTIONAL,IN ULONG InitialTeb OPTIONAL,IN BOOLEAN CreateSuspended,IN ULONG StartRoutine OPTIONAL,IN PVOID StartContext);
void      (WINAPI *IopDeallocateApc)(PVOID P, int a2, int a3, int a4, int a5);

VOID (FASTCALL *HalRequestSoftwareInterrupt) (ULONG RequestIrql);
VOID (FASTCALL *KiUnlockDispatcherDatabase)(ULONG irql);

VOID  (WINAPI *KeContextFromKframes)(IN ULONG TrapFrame,ULONG ExceptionFrame,PCONTEXT ContextFrame);
VOID (WINAPI *KeContextToKframes) ( ULONG TrapFrame,ULONG ExceptionFrame,IN PCONTEXT ContextFrame,IN ULONG ContextFlags,IN KPROCESSOR_MODE PreviousMode);
BOOLEAN (WINAPI *KiCheckForAtlThunk) (IN ULONG ExceptionRecord,IN PCONTEXT Context);
BOOLEAN (WINAPI *RtlDispatchException) (IN ULONG ExceptionRecord,IN PCONTEXT ContextRecord);


BOOLEAN (WINAPI *KdIsThisAKdTrap) (IN ULONG ExceptionRecord,IN PCONTEXT ContextRecord,IN KPROCESSOR_MODE PreviousMode);
VOID (WINAPI *KiSegSsToTrapFrame ) (IN ULONG TrapFrame,IN ULONG SegSs);
VOID (WINAPI *KiEspToTrapFrame)(IN ULONG TrapFrame,IN ULONG Esp);
ULONG (WINAPI *KiCopyInformation) (IN OUT ULONG ExceptionRecord1,IN ULONG ExceptionRecord2);
BOOLEAN
(WINAPI *KiDebugRoutine) (
						  IN ULONG TrapFrame,
						  IN ULONG ExceptionFrame,
						  IN ULONG ExceptionRecord,
						  IN PCONTEXT ContextRecord,
						  IN KPROCESSOR_MODE PreviousMode,
						  IN BOOLEAN SecondChance
						  );

VOID (WINAPI *ObpDeleteObjectType )(IN  PVOID   Object);
VOID (WINAPI *DbgkForwardException )(IN  PVOID   Object);
VOID (WINAPI *NtCreateDebugObject )(IN  PVOID   Object);
VOID (WINAPI *KiDispatchException )(IN  PVOID   Object);
VOID (WINAPI *ExRaiseException )(IN  PVOID   Object);
VOID (WINAPI *DbgkCreateThread )(IN  PVOID   Object);
VOID (WINAPI *DbgkExitThread )(IN  PVOID   Object);
VOID (WINAPI *DbgkExitProcess )(IN  PVOID   Object);
VOID (WINAPI *DbgkpMarkProcessPeb )(IN  PVOID   Object);
VOID (WINAPI *DbgkMapViewOfSection )(IN  PVOID   Object);
VOID (WINAPI *KeStackAttachProcess )(IN  PVOID   Object);
VOID (WINAPI *NtWaitForDebugEvent )(IN  PVOID   Object);
VOID (WINAPI *NtDebugContinue )(IN  PVOID   Object);
VOID (WINAPI *KeAttachProcess )(IN  PVOID   Object);
VOID (WINAPI *NtDebugActiveProcess )(IN  PVOID   Object);
VOID (WINAPI *DUMMYFUCK )(IN  PVOID   Object);
VOID (WINAPI *KiThreadStartup )(IN  PVOID   Object);
VOID (WINAPI *DbgkpSetProcessDebugObject )(IN  PVOID   Object);


NTSTATUS  (WINAPI *MmGetFileNameForSection) (IN PVOID SectionObject,OUT POBJECT_NAME_INFORMATION *FileNameInfo);

PVOID KeUserExceptionDispatcher;
PUCHAR KeI386XMMIPresent;
BOOLEAN PsImageNotifyEnabled;
PULONG KeFeatureBits;
const UNICODE_STRING PsNtDllPathName;
PVOID PsSystemDllBase;
ULONG DbgkpProcessDebugPortMutex;
ULONG DbgkDebugObjectType;

HX_DYNC_FUNCTION *g_dync_funs;
ULONG						g_FunCount=0;
ULONG						g_CountOfFunFound=0;

//////////////////////////////////////////////////////////////////////////




#define DECL_DYNCFUN(x) \
{#x,(DWORD)&x,0,0}

#define DECL_DYNCFUN_HOOK(x) \
{#x,(DWORD)&x,0,1}

#define DECL_DYNCFUN_HOOK_Old2New(x) \
{#x,0,0,1}

//这是hook新的os的hook,涉及到DebugPort判断的，转到自己实现中
HX_DYNC_FUNCTION dync_New2myIMPL[]={
	//			DECL_DYNCFUN_HOOK(NtDebugActiveProcess),
				DECL_DYNCFUN_HOOK(DbgkpSetProcessDebugObject),
				DECL_DYNCFUN_HOOK(NtCreateDebugObject),
				DECL_DYNCFUN_HOOK(NtWaitForDebugEvent),
				DECL_DYNCFUN_HOOK(NtDebugContinue),
				DECL_DYNCFUN_HOOK(DbgkCreateThread),
				DECL_DYNCFUN_HOOK(DbgkExitThread),
				DECL_DYNCFUN_HOOK(DbgkExitProcess),
				DECL_DYNCFUN_HOOK(DbgkMapViewOfSection),
				DECL_DYNCFUN_HOOK(DbgkForwardException),
};

//////////////////////////////////////////////////////////////////////////
//这是为了HOOK原来的ntos，转到新的os中

HX_DYNC_FUNCTION dync_Old2New[]={
//				DECL_DYNCFUN_HOOK_Old2New(DUMMYFUCK),
				DECL_DYNCFUN_HOOK_Old2New(PspUserThreadStartup),
				DECL_DYNCFUN_HOOK_Old2New(PspSystemThreadStartup),
				DECL_DYNCFUN_HOOK_Old2New(ObCloseHandle),
				DECL_DYNCFUN_HOOK_Old2New(PspProcessDelete),
				DECL_DYNCFUN_HOOK_Old2New(pIofCallDriver),
				DECL_DYNCFUN_HOOK_Old2New(KiTrap03),
								DECL_DYNCFUN_HOOK_Old2New(ObpCreateHandle),
								
//				DECL_DYNCFUN_HOOK_Old2New(KeStackAttachProcess),
//				DECL_DYNCFUN_HOOK_Old2New(KeAttachProcess),
		};


//这个需要另外HOOK的函数 ，old os 中的，把这些不经过hookport的转到自己的实现中
HX_DYNC_FUNCTION dync_funs_hook[]={
				DECL_DYNCFUN_HOOK(DbgkCreateThread),
				DECL_DYNCFUN_HOOK(DbgkExitThread),
				DECL_DYNCFUN_HOOK(DbgkExitProcess),
				DECL_DYNCFUN_HOOK(DbgkMapViewOfSection),
				DECL_DYNCFUN_HOOK(DbgkpMarkProcessPeb),

//		DECL_DYNCFUN_HOOK(NtCreateDebugObject),
				DECL_DYNCFUN_HOOK(DbgkForwardException),

		};
HX_DYNC_FUNCTION dync_funs[]={

	DECL_DYNCFUN(KeUserExceptionDispatcher),
	DECL_DYNCFUN(KeI386XMMIPresent),
	DECL_DYNCFUN(PsImageNotifyEnabled),
	DECL_DYNCFUN(PsNtDllPathName),
	DECL_DYNCFUN(KeFeatureBits),
	DECL_DYNCFUN(PsSystemDllBase),
	DECL_DYNCFUN(PsGetNextProcess),
	DECL_DYNCFUN(PsGetNextProcessThread),
	DECL_DYNCFUN(PsQuitNextProcessThread),
	DECL_DYNCFUN(PsResumeThread),
	DECL_DYNCFUN(PsTerminateProcess),
	DECL_DYNCFUN(PsSuspendThread),	
	DECL_DYNCFUN(PsCallImageNotifyRoutines),
	DECL_DYNCFUN(ObDuplicateObject),
	DECL_DYNCFUN(MmGetFileNameForAddress),
	DECL_DYNCFUN(MmGetFileNameForSection),
	DECL_DYNCFUN(KeFreezeAllThreads),
	DECL_DYNCFUN(KeThawAllThreads),	
	DECL_DYNCFUN(KiReadyThread),	
	DECL_DYNCFUN(KiSwapProcess),
	DECL_DYNCFUN(KiSwapThread),	
	DECL_DYNCFUN(KiSetSwapEvent),
	DECL_DYNCFUN(LpcRequestWaitReplyPortEx),
	DECL_DYNCFUN(KiUnlockDispatcherDatabase),
	DECL_DYNCFUN(MiMakeProtectionMask),
	DECL_DYNCFUN(MiProtectVirtualMemory),
	DECL_DYNCFUN(MmCopyVirtualMemory),
	DECL_DYNCFUN(PspCreateThread),
	DECL_DYNCFUN(IopDeallocateApc),
	DECL_DYNCFUN(KeContextFromKframes),
	DECL_DYNCFUN(KeContextToKframes),
	DECL_DYNCFUN(KiCheckForAtlThunk),
	DECL_DYNCFUN(RtlDispatchException),
	DECL_DYNCFUN(KdIsThisAKdTrap),
	DECL_DYNCFUN(KiSegSsToTrapFrame),
	DECL_DYNCFUN(KiEspToTrapFrame),
	DECL_DYNCFUN(KiCopyInformation),
// 	DECL_DYNCFUN(DbgkpProcessDebugPortMutex),
//要注意这个变量，全局变量。如果用个reload Kernel大法的话，这变量的地址不能加上reload后的偏移
 	DECL_DYNCFUN(DbgkDebugObjectType),
};
//////////////////////////////////////////////////////////////////////////
void get_dync_funs_hook(HX_DYNC_FUNCTION **pdync_funs_hook, DWORD &Size)
{
		*pdync_funs_hook=dync_funs_hook;
		Size	=	 sizeof(dync_funs_hook);

}
//////////////////////////////////////////////////////////////////////////
void get_dync_New2my(HX_DYNC_FUNCTION **pdync_funs_hook, DWORD &Size)
{
		*pdync_funs_hook=dync_New2myIMPL;
		Size	=	 sizeof(dync_New2myIMPL);

}

void get_dync_funs(HX_DYNC_FUNCTION **pdync_funs, DWORD &Size)
{
			*pdync_funs =dync_funs;
			Size	=	 sizeof(dync_funs);
}

void get_dync_funs_hook_old(HX_DYNC_FUNCTION **pdync_funs, DWORD &Size)
{
			*pdync_funs =dync_Old2New;
			Size	=	 sizeof(dync_Old2New);
}
//////////////////////////////////////////////////////////////////////////
ULONG	getNtBase()
{
	DWORD	dwsize;
	DWORD	dwSizeReturn;
	PUCHAR	pBuffer	=	NULL;
	
	PSYSTEM_MODULE_INFORMATION	pSmi=NULL;
	PSYSTEM_MODULE	psm=NULL;
	NTSTATUS	ntStatus=STATUS_UNSUCCESSFUL;
	ntStatus = NtQuerySystemInformation(SystemModuleInformation, pSmi, 0, &dwSizeReturn);
	
	if (ntStatus!=STATUS_INFO_LENGTH_MISMATCH)
	{
		
		MessageBoxA(NULL,"fuck1",NULL,NULL);
		return 0;
	}
	dwsize	=	dwSizeReturn*2;
	pSmi	=	(PSYSTEM_MODULE_INFORMATION)new char[dwsize];
	if (pSmi==NULL)
	{
		MessageBoxA(NULL,"fuck2",NULL,NULL);
		return 0;
	}
	
	ntStatus = NtQuerySystemInformation(SystemModuleInformation, pSmi,dwsize, &dwSizeReturn);
	
	if (ntStatus!=STATUS_SUCCESS)
	{
		MessageBoxA(NULL,"fuck3",NULL,NULL);
		return 0;
	}

	DWORD	dwcount	=	pSmi->uCount;
	psm	=	pSmi->aSM;
	DWORD	ntbase	=	psm->Base;
	strcpy(osname, psm->ImageName);
	delete pSmi;
	
	return ntbase;
}
//////////////////////////////////////////////////////////////////////////


BOOL CALLBACK SymEnumSymbolsProc(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext )
{
	static int inum=1;
   	char funcname[100]={NULL};
	DWORD addr;
	char a[200]={NULL};
	DWORD	totalcount	=	sizeof(dync_funs)/sizeof(HX_DYNC_FUNCTION)+sizeof(dync_Old2New)/sizeof(HX_DYNC_FUNCTION)+
sizeof(dync_New2myIMPL)/sizeof(HX_DYNC_FUNCTION)+sizeof(dync_funs_hook)/sizeof(HX_DYNC_FUNCTION);

		if( pSymInfo != 0 ) 
		{
			
			strcpy(funcname,pSymInfo->Name);
			addr=pSymInfo->Address;	
			//SET
			for(int i=0;i<sizeof(dync_funs)/sizeof(HX_DYNC_FUNCTION);i++)
			{
			   if(strcmp(dync_funs[i].FunName,funcname)==0)
			    {

				g_CountOfFunFound++;
				 dync_funs[i].FunAddr	=	addr;
				 sprintf(a,"%d,名字 %s,地址 %x %d\n",g_CountOfFunFound,funcname,addr,totalcount);
				OutputDebugStringA(a);
				break;	
			    }
			}
			//hook
			int i;
			for( i=0;i<sizeof(dync_Old2New)/sizeof(HX_DYNC_FUNCTION);i++)
			{
			  if(strcmp(dync_Old2New[i].FunName,funcname)==0)
			    {

					g_CountOfFunFound++;
					 dync_Old2New[i].FunAddr	=	addr;
					 	 sprintf(a,"%d,名字 %s,=======================地址 %x \n",g_CountOfFunFound,funcname,addr);
					 	OutputDebugStringA(a);
					break;	
			    }
					
			}

			for( i=0;i<sizeof(dync_New2myIMPL)/sizeof(HX_DYNC_FUNCTION);i++)
			{
			  if(strcmp(dync_New2myIMPL[i].FunName,funcname)==0)
			    {

					g_CountOfFunFound++;
					 dync_New2myIMPL[i].FunAddr	=	addr;
					 sprintf(a,"%d,名字 %s,=======================地址 %x \n",g_CountOfFunFound,funcname,addr);
					 	OutputDebugStringA(a);
					break;	
			    }
					
			}

			for( i=0;i<sizeof(dync_funs_hook)/sizeof(HX_DYNC_FUNCTION);i++)
			{
			   if(strcmp(dync_funs_hook[i].FunName,funcname)==0)
			    {

					g_CountOfFunFound++;
				 dync_funs_hook[i].FunAddr	=	addr;
				 sprintf(a,"%d,名字 %s,地址 %x \n",g_CountOfFunFound,funcname,addr);

				OutputDebugStringA(a);
				break;	
			    }
			}


	     }
	return TRUE;
}


#include <Shlwapi.h>
#pragma comment(lib,"shlwapi")
//return osbase if retrive symbols successfully, otherwise return 0

ULONG	getSymbols()
{
	CString FilePathName;
	PCSTR path;
	DWORD error;
	DWORD64 dw64Base;
	DWORD dwFileSize;
	g_CountOfFunFound	=	0;
	dw64Base = getNtBase();
	if (dw64Base==0)
	{
		::MessageBox(NULL,"Get base 0",NULL,NULL);
		return 0 ;
	}
	char	szSystemDir[1024]={0};
	GetSystemDirectory(szSystemDir, sizeof(szSystemDir));
	FilePathName	=	szSystemDir;
	FilePathName	=	FilePathName+"\\"+PathFindFileName(osname);
	
	path=FilePathName;
	//记得要带上symsrv.dll和dbghelp.dll
	char *pSymPath	=	"srv*C:\\winddk\\symbolsl*http://msdl.microsoft.com/download/symbols";
	myprint("retriving symbols for %s,symbols store in %s\r\n",path, pSymPath);
	 SymCleanup(GetCurrentProcess());
	if (SymInitialize(GetCurrentProcess(), pSymPath, TRUE))
	{
		// SymInitialize returned success
	}
	else
	{
		// SymInitialize failed
		error = GetLastError();
		myprint("SymInitialize returned error : %d\n", error);
		return 0;
	}
	
	DWORD err=0;
	// get the file size
	HANDLE hFile = CreateFile( FilePathName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
	if( INVALID_HANDLE_VALUE == hFile )
	{
		err	=	GetLastError();
		::MessageBox(NULL,"out",NULL,NULL);
		return false;
	}
	if( INVALID_FILE_SIZE == ( dwFileSize = GetFileSize(hFile, NULL)) )
	{
		
	}
	CloseHandle(hFile);
	
	
	DWORD64 dw64ModAddress=SymLoadModule64( GetCurrentProcess(),NULL, (char*)path,NULL,dw64Base,dwFileSize);
	
	if(dw64ModAddress==0)
	{
		error = GetLastError();
		myprint("SymLoadModule64 returned error : %d\n", error);
		return 0;
		
	}

	if(!SymEnumSymbols( GetCurrentProcess(),
		dw64ModAddress,
		NULL, // Null point out that list all symbols
		SymEnumSymbolsProc,
		NULL))
	{
		//_tprintf( _T("Failed when SymEnumSymbols(): %d \n"), GetLastError() );
		return 0;
	}
	myprint("g_CountOfFunFound %d\r\n", g_CountOfFunFound);
	bool bRetSym	=	(g_CountOfFunFound>sizeof(dync_funs)/sizeof(dync_funs[0]));
	return bRetSym?dw64Base:0;

}