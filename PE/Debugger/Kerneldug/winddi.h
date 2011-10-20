#ifndef _WINDDI_H_
#define _WINDDI_H_

extern "C"
{

//win32k
VOID 
NTAPI
EngDebugBreak(
    VOID
    );

VOID 
NTAPI
EngDebugPrint(
	PSTR,
	PSTR,
	PVOID
	);

HANDLE 
NTAPI
EngLoadImage(
    PWSTR pwszDriver
    );

PVOID 
NTAPI
EngFindImageProcAddress(
    HANDLE hModule,
	PSTR lpProcName
    );

VOID 
NTAPI
EngUnloadImage(
    HANDLE hModule
    );

#define DDI_DRIVER_VERSION_NT5_01_SP1 0x00030101
}

#endif
