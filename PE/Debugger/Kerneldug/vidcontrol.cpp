#include <ntdll.h>
#include <stdio.h>

#define ExAllocatePool(x,y) malloc(y)
#define ExFreePool(x) free(x)


PVOID 
GetSystemInformation (
	SYSTEM_INFORMATION_CLASS InfoClass
	)
/**
	Get system information by class.
	ZwQuerySystemInformation wrapper
*/
{
	NTSTATUS Status;
	PVOID Buffer;
	ULONG Size = PAGE_SIZE;

	do
	{
		Buffer = ExAllocatePool (PagedPool, Size);

		Status = ZwQuerySystemInformation ( InfoClass,
											Buffer,
											Size,
											&Size );

		if (Status == STATUS_INFO_LENGTH_MISMATCH)
			ExFreePool (Buffer);

	}
	while (Status == STATUS_INFO_LENGTH_MISMATCH);

	if (!NT_SUCCESS(Status))
	{
		ExFreePool (Buffer);
		return NULL;
	}

	return Buffer;
}


PVOID
GetModule(
	PCHAR Name
	)
{
	PSYSTEM_MODULE_INFORMATION Modules = (PSYSTEM_MODULE_INFORMATION) GetSystemInformation (
		SystemModuleInformation);

	for (ULONG i=0; i<Modules->ModulesCount; i++)
	{
		if (((ULONG)Modules->Modules[i].ImageBaseAddress & 0xF0000000) == 0xB0000000)
		{
			//
			// Session-space modules
			//

			char *mod = (char*)Modules->Modules[i].Name;
			char *relmod = strrchr (mod, '\\');
			if (relmod)
				relmod++;
			else
				relmod = mod;

			printf("%08x  %s\n", Modules->Modules[i].ImageBaseAddress, relmod);

			if (!lstrcmpi(relmod, Name))
			{
				PVOID ret = Modules->Modules[i].ImageBaseAddress;
				ExFreePool (Modules);
				return ret;
			}
		}
	}

	ExFreePool (Modules);
	return NULL;
}




typedef struct  _DRVFN  /* drvfn */
{
    ULONG   iFunc;
    PVOID     pfn;
} DRVFN, *PDRVFN;

typedef struct  tagDRVENABLEDATA
{
    ULONG   iDriverVersion;
    ULONG   c;
    DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;


typedef struct MODULE32
{
	char ModuleName[256];
	PVOID ImageBase;
	ULONG ImageSize;
} *PMODULE32;

extern MODULE32 LoadedModules[];
extern ULONG LoadedModCount;

PMODULE32 LdrLookupModByPointer (PVOID Pointer)
{
	for (ULONG i=0; i<LoadedModCount; i++)
	{
		if ( (ULONG)Pointer >= (ULONG)LoadedModules[i].ImageBase &&
			 (ULONG)Pointer <  (ULONG)LoadedModules[i].ImageBase + LoadedModules[i].ImageSize )
		{
			return &LoadedModules[i];
		}
	}
	return NULL;
}

void *SysLoad (char*);
void *LdrPrepareCallLoadedImage (void*);

#define INDEX_DrvCopyBits                       19L
ULONG DrvCopyBits_RVA;

ULONG DrvCopyBits;
PULONG pTblDrvCopyBits;

void GetDrvCopyBits()
{
	void* ptr = SysLoad ("igxprd32.dll");

	BOOL (APIENTRY *DrvEnableDriver)(
		ULONG          iEngineVersion,
		ULONG          cj,
		DRVENABLEDATA *pded
		);
	
	*(PVOID*)&DrvEnableDriver = LdrPrepareCallLoadedImage( (HINSTANCE)ptr );

	DRVENABLEDATA ded = {0};
	BOOL b = DrvEnableDriver (0x00030101, sizeof(ded), &ded);


	printf("DrvEnableDriver returned %d\n", b);

	if (!b)
		Sleep(-1);

	printf("DISPLAY DRIVER LOADED\n");
	printf("===============================================================\n"
		   "Dumping entries                   \n"
		   "Index		Entry		Relative	Module    \n"
		   "===============================================================\n");

	for (ULONG i=0; i<ded.c; i++)
	{
		char *modname = "";

		PMODULE32 mod = LdrLookupModByPointer (ded.pdrvfn[i].pfn);
		if (mod)
		{
			modname = strrchr (mod->ModuleName, '\\');
			if (modname)
				modname ++;
			else
				modname = "";
		}

		printf("%2d		%08x	%08x	%s\n", 
			ded.pdrvfn[i].iFunc, 
			ded.pdrvfn[i].pfn,
			mod ? ((ULONG)ded.pdrvfn[i].pfn - (ULONG)mod->ImageBase) : 0,
			modname);

		if (ded.pdrvfn[i].iFunc == INDEX_DrvCopyBits)
		{
			DrvCopyBits_RVA = ((ULONG)ded.pdrvfn[i].pfn - (ULONG)mod->ImageBase);

			PVOID KernelModule = GetModule (modname);
			if (KernelModule)
			{
				DrvCopyBits = (ULONG)KernelModule + DrvCopyBits_RVA;
				pTblDrvCopyBits = (PULONG)((ULONG)KernelModule - (ULONG)mod->ImageBase + (ULONG)&ded.pdrvfn[i].pfn);
				printf("DrvCopyBits %X pp %X\n", DrvCopyBits, pTblDrvCopyBits);
				return;
			}
		}
	}
}

static bool AdjustSingleTokenPrivilege(HANDLE TokenHandle, LPCTSTR lpName, DWORD dwAttributes)
{
   TOKEN_PRIVILEGES tp;
   tp.PrivilegeCount = 1;

   tp.Privileges[0].Attributes = dwAttributes;

   if (!LookupPrivilegeValue(NULL, lpName, &(tp.Privileges[0].Luid)))
      return false;

   if (!AdjustTokenPrivileges(TokenHandle, FALSE, &tp, 0, NULL, NULL))
      return false;

   return true;
}

bool EnableLoadDriverPrivilege()
{
   DWORD dwPID = GetCurrentProcessId();

   HANDLE hProcess = NULL;
   if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwPID)) == INVALID_HANDLE_VALUE)
      return false;

   HANDLE hToken = NULL;
   if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
      return false;

   if (!AdjustSingleTokenPrivilege(hToken, SE_SECURITY_NAME, SE_PRIVILEGE_ENABLED) ||
      !AdjustSingleTokenPrivilege(hToken, SE_LOAD_DRIVER_NAME, SE_PRIVILEGE_ENABLED))
      return false;

   CloseHandle(hToken);
   CloseHandle(hProcess);

   return true;
} 



int main()
{
	GetDrvCopyBits ();

	if (!DrvCopyBits)
		return 0;

	if(!EnableLoadDriverPrivilege())
		return printf("Could not enable SeLoadDriverPrivilege\n");


	char DriverPath [MAX_PATH];

	GetSystemDirectory (DriverPath, MAX_PATH-1);
	lstrcat (DriverPath, "\\drivers\\ngvid.sys");

	if(!CopyFile ("ngvid.sys", DriverPath, FALSE))
		return printf("Cannot copy driver file\n");

	SC_HANDLE hManager = OpenSCManager (0, 0, SC_MANAGER_CONNECT|SC_MANAGER_CREATE_SERVICE);
	if (!hManager)
		return printf("Cannot open SC manager\n");

	SC_HANDLE hService = CreateService (hManager, "ngvid2", "ngvid2", SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, DriverPath, 0, 0, 0, 0, 0);

	CloseServiceHandle (hService);
	CloseServiceHandle (hManager);

	HKEY hk;
	if(RegCreateKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\ngvid2\\Parameters", 0, 0, 
		0, KEY_SET_VALUE, 0, &hk, 0))
		return 0;

	if (RegSetValueEx (hk, "DrvCopyBits", 0, REG_DWORD, (PBYTE)&DrvCopyBits, 4))
		return 0;

	if (RegSetValueEx (hk, "pTblDrvCopyBits", 0, REG_DWORD, (PBYTE)&pTblDrvCopyBits, 4))
		return 0;

	RegCloseKey (hk);
	

	UNICODE_STRING Path;
	NTSTATUS st;

	RtlInitUnicodeString (&Path, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\ngvid2");

	st = ZwLoadDriver (&Path);

	if (!NT_SUCCESS(st) && st != STATUS_IMAGE_ALREADY_LOADED)
		return printf("ZwLoadDriver failed with status %X\n", st);


	return 0;
}

 // */