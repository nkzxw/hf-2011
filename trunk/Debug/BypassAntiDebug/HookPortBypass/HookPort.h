
#ifndef ___HOOKPORT___
#define ___HOOKPORT___

typedef	struct __IATHOOKINFO__
{
//	LIST_ENTRY		Next;
	char *ModuleNameSrc;
	char *ModuleNameDst;
	char *pFunName;
	ULONG pfTargetFunAddressLocation;
	ULONG pfTargetFunAddressValue;
	ULONG pfNewFunAddressDelta;
	ULONG	HookType;
}IATHOOKINFO, *PIATHOOKINFO;


NTSTATUS	InitHookPort();
NTSTATUS	UnHookPort();
void GetProcessNameOffset() ;
ULONG GetOsMoudleName(char* ModuleName, ULONG uLen);
VOID WPOFF();
VOID WPON() ;
void IATHookWin32(ULONG hMod ,  char *ModuleNameDst, char *pFunName, ULONG pfNewFunAddress, ULONG &pfnOriAddressLocation, ULONG &pfnOriAddress);
VOID LoadImageNotify(
					 __in_opt PUNICODE_STRING  FullImageName,
					 __in HANDLE  ProcessId,
					 __in PIMAGE_INFO  ImageInfo
					 );
#endif

