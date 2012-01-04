.586
.model flat, stdcall
option casemap:none

;; ----------------------------------------
;; header file and lib file
;; ----------------------------------------
include \masm32\include\windows.inc

include \masm32\include\masm32.inc
include \masm32\include\msvcrt.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc


includelib \masm32\lib\masm32.lib
includelib \masm32\lib\msvcrt.lib
includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\user32.lib

;; ----------------------------------------
;; value definition
;; ----------------------------------------
SystemNotImplemented8               equ 30
SystemNotImplemented9               equ 31
SystemKernelDebuggerInformation     equ 35
ThreadHideFromDebugger              equ 17
ProcessDebugPort                    equ 7
ObjectAllTypeInformation            equ 3
STATUS_SUCCESS						equ 0
;; ----------------------------------------
;; structure definition
;; ----------------------------------------
SYSTEM_KERNEL_DEBUGGER_INFORMATION struct
    DebuggerEnabled                 dd 0
    DebuggerNotPresent              dd 0
SYSTEM_KERNEL_DEBUGGER_INFORMATION ends

PROCESS_DEBUG_PORT_INFO struct
    DebugPort                       dd 0
PROCESS_DEBUG_PORT_INFO ends

PROCESS_DEBUG_FLAGS_INFO struct
    DebugFlags                      dd 0
PROCESS_DEBUG_FLAGS_INFO ends

PROCESS_DEBUG_OBJECTHANDLE_INFO struct
    ObjectHandle                    dd 0
PROCESS_DEBUG_OBJECTHANDLE_INFO ends

WIN32APIBASE struct
    ;; kernel32.dll
    xLoadLibraryA                    dd 0
    xGetProcAddress                  dd 0
    xGetModuleFileNameA              dd 0
    xGetCurrentProcess               dd 0
    xGetCurrentProcessId             dd 0
    xGetCurrentThread                dd 0
    xGetCurrentThreadId              dd 0
    xGetWindowsDirectoryA            dd 0
    xGetProcessHeap                  dd 0
    xGetLastError                    dd 0
    xGetVersion                      dd 0
    xGetVersionExA                   dd 0
    xGetFileSize                     dd 0
    xGetTickCount                    dd 0
    xGetProcessId					 dd 0
    xCreateToolhelp32Snapshot        dd 0
    xCreateFileA                     dd 0
    xCloseHandle                     dd 0
    xCheckRemoteDebuggerPresent      dd 0
    xProcess32First                  dd 0
    xProcess32Next                   dd 0
    xProcess32NextW                  dd 0
    xModule32First                   dd 0
    xModule32Next                    dd 0
    xOpenProcess                     dd 0
    xOutputDebugStringA              dd 0
    xFindWindowA                     dd 0    
    xVirtualAlloc                    dd 0
    xVirtualFree                     dd 0
    xVirtualProtect                  dd 0
	xHeapAlloc                       dd 0
    xHeapFree                        dd 0
    xUnhandledExceptionFilter        dd 0
    xReadFile                        dd 0
    xThread32First				     dd 0  
    xThread32Next				     dd 0 
    xTerminateProcess				 dd 0
    xSuspendThread                   dd 0
    xSleep                           dd 0
    xSetUnhandledExceptionFilter     dd 0
    xIsDebuggerPresent               dd 0
    xExitProcess					 dd 0    
    xlstrcpyA                        dd 0
    xlstrcatA                        dd 0
    xlstrcmpA                        dd 0
    xlstrcmpiA                       dd 0
    xlstrlenA                        dd 0
    ;; user32.dll
    xBlockInput                      dd 0
    xCreateDesktopA                  dd 0
    xCloseDesktop                    dd 0        
    xGetThreadDesktop                dd 0
    xSetThreadDesktop                dd 0
    xSwitchDesktop                   dd 0
    xOpenInputDesktop                dd 0
    ;; ntdll.dll
    xZwQueryInformationProcess       dd 0
    xZwQuerySystemInformation        dd 0
    xZwSetInformationThread          dd 0
    xNtQueryObject                   dd 0
    ;; Mmtimer.dll
    xtimeGetTime                     dd 0
    ;; Coredll.dll
    xQueryPerformanceCounter         dd 0
    ;; dll handles
    hKernel32Dll					 dd 0
    hUser32Dll                       dd 0
    hNtdllDll					 	 dd 0
    hWinmmDll					 	 dd 0
    hCoredllDll						 dd 0
WIN32APIBASE ends

FUNCADDRTBL struct
    xGetFileNameFromPath             dd 0
    xCRC32                           dd 0
    xGetThreadHandle			     dd 0
FUNCADDRTBL ends

ANTIDEBUGGERARGUMENTS struct
	szFileName          					db MAX_PATH dup (0)
	dwFuncCounts							dd 0
	Use_FD_IsDebuggerPresent				dd 0
	Use_FD_PEB_BeingDebuggedFlag			dd 0
	Use_FD_PEB_NtGlobalFlags				dd 0
	Use_FD_Heap_HeapFlags					dd 0
	Use_FD_Heap_ForceFlags					dd 0
	Use_FD_Heap_Magic						dd 0	
	Use_FD_CheckRemoteDebuggerPresent		dd 0
	Use_FD_NtQueryInfoProc_DbgPort			dd 0	
	Use_FD_NtQueryInfoProc_DbgObjHandle		dd 0
	Use_FD_NtQueryInfoProc_DbgFlags			dd 0
	Use_FD_NtQueryInfoProc_SysKrlDbgInfo	dd 0
	Use_FD_SeDebugPrivilege					dd 0
	Use_FD_Parent_Process					dd 0
	Use_FD_DebugObject_NtQueryObject		dd 0
	Use_FD_Find_Debugger_Window				dd 0
	Use_FD_Find_Debugger_Process			dd 0
	Use_FD_Find_Device_Driver				dd 0
	Use_FD_Exception_Int3					dd 0
	Use_FD_Exception_Popf					dd 0
	Use_FD_OutputDebugString				dd 0
		
	Use_FS_OD_Exception_GuardPages			dd 0
	Use_FS_SI_UnhandledExceptionFilter		dd 0	
	Use_FS_ODP_Process32NextW				dd 0
	Use_FS_ODP_OutputDebugStringA			dd 0	
	Use_FS_ODP_OpenProcess					dd 0
	Use_FV_VMWare_VMX						dd 0
	Use_FV_VPC_Exception					dd 0
	
	Use_FB_HWBP_Exception					dd 0
	Use_FB_SWBP_Memory_CRC					dd 0
	
	Use_FT_PushSS_PopSS						dd 0
	Use_FT_RDTSC							dd 0
	Use_FT_GetTickCount						dd 0
	Use_FT_timeGetTime						dd 0
	Use_FT_SharedUserData_TickCount			dd 0	
	Use_FT_INT1_IceBreakpoint				dd 0
	
	Use_FP_Check_FileSize					dd 0
	Use_FP_Check_FileHashValue_CRC			dd 0
	
	Use_AD_SwitchDesktop					dd 0
	Use_AD_BlockInput						dd 0
	Use_AD_ZwSetInformationThread			dd 0
	Use_AD_INT_2e							dd 0
	
	Use_AS_OD_OutputDebugString				dd 0
	
	Use_AB_HWP_CLR_Exception				dd 0	
ANTIDEBUGGERARGUMENTS ends
;; ----------------------------------------
;; const definition
;; ----------------------------------------
.const
g_szErr                     db "错误",0
g_szDone                    db "文件加密成功!",0
g_szDoneCap                 db "^_^",0
g_szOpenFileFailed          db "打不开文件",0
g_szGetFileSizeFailed       db "获取文件大小失败",0
g_szCreateMapFailed         db "创建文件映射失败",0
g_szMapFileFailed           db "映射文件到内存失败",0
g_szInvalidPE               db "无效的PE文件",0

g_szTitle					db "AntiDebugger",0dh,0ah,0
g_szUsage					db "Usage:AntiDebugger [options] file",0dh,0ah,0
g_szOptions					db "[options]",0dh,0ah
							db "1    FD_IsDebuggerPresent",0dh,0ah
							db "2    FD_PEB_BeingDebuggedFlag",0dh,0ah
							db "3    FD_PEB_NtGlobalFlags",0dh,0ah
							db "4    FD_Heap_HeapFlags",0dh,0ah
							db "5    FD_Heap_ForceFlags",0dh,0ah
							db "6    FD_Heap_Magic",0dh,0ah
							db "7    FD_CheckRemoteDebuggerPresent",0dh,0ah
							db "8    FD_NtQueryInfoProc_DbgPort",0dh,0ah
							db "9    FD_NtQueryInfoProc_DbgObjHandle",0dh,0ah
							db "10   FD_NtQueryInfoProc_DbgFlags",0dh,0ah
							db "11   FD_NtQueryInfoProc_SysKrlDbgInfo",0dh,0ah
							db "12   FD_SeDebugPrivilege",0dh,0ah
							db "13   FD_Parent_Process",0dh,0ah
							db "14   FD_DebugObject_NtQueryObject",0dh,0ah
							db "15   FD_Find_Debugger_Window",0dh,0ah
							db "16   FD_Find_Debugger_Process",0dh,0ah
							db "17   FD_Find_Device_Driver",0dh,0ah
							db "18   FD_Exception_Int3",0dh,0ah
							db "19   FD_Exception_Popf",0dh,0ah
							db "20   FD_OutputDebugString",0dh,0ah
							db "21   FS_OD_Exception_GuardPages",0dh,0ah
							db "22   FS_SI_UnhandledExceptionFilter",0dh,0ah
							db "23   FS_ODP_Process32NextW",0dh,0ah
							db "24   FS_ODP_OutputDebugStringA",0dh,0ah
							db "25   FS_ODP_OpenProcess",0dh,0ah
							db "26   FV_VMWare_VMX",0dh,0ah
							db "27   FV_VPC_Exception",0dh,0ah
							db "28   FB_HWBP_Exception",0dh,0ah
							db "29   FB_SWBP_Memory_CRC",0dh,0ah
							db "30   FT_PushSS_PopSS",0dh,0ah
							db "31   FT_RDTSC",0dh,0ah
							db "32   FT_GetTickCount",0dh,0ah
							db "33   FT_timeGetTime",0dh,0ah
							db "34   FT_SharedUserData_TickCount",0dh,0ah
							db "35   FT_INT1_IceBreakpoint",0dh,0ah
							db "36   FP_Check_FileSize",0dh,0ah
							db "37   FP_Check_FileHashValue_CRC",0dh,0ah
							db "38   AD_SwitchDesktop",0dh,0ah
							db "39   AD_BlockInput",0dh,0ah
							db "40   AD_ZwSetInformationThread",0dh,0ah
							db "41   AD_INT_2e",0dh,0ah
							db "42   AS_OD_OutputDebugString",0dh,0ah
							db "43   AB_HWP_CLR_Exception",0dh,0ah							
							db 0

g_szAuthor					db "Author:玩命",0dh,0ah,0

g_szOutFormat               db "%s",0
g_szOutOrdFormat            db "%d",0
g_szOutLine                 db "--------------------",0

g_szSectionName				db ".anti",0
;; ----------------------------------------
;; data segment
;; ----------------------------------------
.data
g_bError                    db 0
g_dwAntiDebugSize           dd 0
g_AntiDebuggerArguments     ANTIDEBUGGERARGUMENTS <>
;; ----------------------------------------
;; code segment
;; ----------------------------------------
.code
;; ----------------------------------------
;; function statement
;; ----------------------------------------
PEAlign proto dwTarNum : DWORD, dwAlignTo : DWORD
RVA2Offset proto Base : DWORD, dwITRVA : DWORD
CountAppendSize proto pMem : LPVOID, dwAppendSize : DWORD
AddSection proto pMem : LPVOID, pSectionName : LPVOID, dwSectionSize : DWORD
AntiDebugger_HandleArguments proto pAntiDebuggerCmdLine : LPSTR, pAntiDebuggerArguments : LPVOID
Help proto
AntiDebugger proto
Start:
	
	invoke GetCommandLine
	invoke AntiDebugger_HandleArguments, eax, offset g_AntiDebuggerArguments
	invoke AntiDebugger
    invoke ExitProcess, 1
    
AntiDebugger proc
    LOCAL hFile : HANDLE
    LOCAL hMap : HANDLE
    LOCAL pMem : LPVOID
    LOCAL dwAppendSize : DWORD    
    LOCAL dwNTHeaderAddr : DWORD
    LOCAL dwImageBase : DWORD
    LOCAL dwOrigEntryAddr : DWORD
    LOCAL dwAntiSectionFAddr : DWORD
    LOCAL dwAntiSectionVAddr : DWORD
    LOCAL dwArgumentAddr : DWORD
    LOCAL dwAntiFuncsLength : DWORD

	lea edi, g_AntiDebuggerArguments
	assume edi : ptr ANTIDEBUGGERARGUMENTS
    
    ;; init data
    xor eax, eax
    mov g_bError, al
    mov eax, offset End_AntiDebuggerModules - offset AntiDebuggerModules
   	add eax, offset End_AntiDebuggerSection - offset AntiDebuggerSection
    mov g_dwAntiDebugSize, eax
    
    ;; get the append size
    lea eax, [edi].szFileName
    invoke CountAppendSize, eax, g_dwAntiDebugSize
    .IF eax == 0
        jmp GetFileSizeFailed
    .ENDIF
    mov dwAppendSize, eax
    
    ;; open file
    lea eax, [edi].szFileName
	invoke CreateFile, eax,
	                   GENERIC_WRITE + GENERIC_READ,
	                   FILE_SHARE_WRITE + FILE_SHARE_READ,
	                   NULL,
	                   OPEN_EXISTING,
	                   FILE_ATTRIBUTE_NORMAL,
	                   0
    .IF eax == INVALID_HANDLE_VALUE
        jmp OpenFileFailed
    .ENDIF
    mov hFile, eax
;    ;; get file size
;    invoke GetFileSize, eax, NULL
;    .IF eax == 0
;        jmp GetFileSizeFailed        
;    .ENDIF
    mov eax, dwAppendSize
    xchg eax, ecx
    ;; create memory map
    xor ebx, ebx
    invoke CreateFileMapping, hFile, ebx, PAGE_READWRITE, ebx, ecx, ebx
    .IF eax == 0
        invoke CloseHandle, hFile
        jmp CreateMapFailed                
    .ENDIF
    mov hMap, eax
    ;; map file to memory
    xor ebx, ebx
    invoke MapViewOfFile, hMap, 
                          FILE_MAP_WRITE + FILE_MAP_READ + FILE_MAP_COPY,
                          ebx, ebx, ebx
    .IF eax == 0
        invoke CloseHandle, hMap
        invoke CloseHandle, hFile
        jmp MapFileFailed        
    .ENDIF                          
    mov pMem, eax                               
    ;; check it's PE file or not ?
    xchg eax, esi
    assume esi : ptr IMAGE_DOS_HEADER
    .IF [esi].e_magic != 'ZM'
        invoke UnmapViewOfFile, pMem
        invoke CloseHandle, hMap
        invoke CloseHandle, hFile
        jmp InvalidPE        
    .ENDIF    	 
    add esi, [esi].e_lfanew
    assume esi : ptr IMAGE_NT_HEADERS   
    .IF word ptr [esi].Signature != 'EP'
        invoke UnmapViewOfFile, pMem
        invoke CloseHandle, hMap
        invoke CloseHandle, hFile
        jmp InvalidPE        
    .ENDIF
    ;; store basic PE orig information
    mov dwNTHeaderAddr, esi
    assume esi : ptr IMAGE_NT_HEADERS
    mov eax, dword ptr [esi].OptionalHeader.ImageBase
    mov dwImageBase, eax
    add eax, dword ptr [esi].OptionalHeader.AddressOfEntryPoint
    mov dwOrigEntryAddr, eax
    
    ;; add anti-debugger section
    lea eax, g_szSectionName
    invoke AddSection, pMem, eax, g_dwAntiDebugSize
    mov edx, eax
    assume edx : ptr IMAGE_SECTION_HEADER
    mov eax, dword ptr [edx].VirtualAddress
    add eax, dword ptr [esi].OptionalHeader.ImageBase
	mov dwAntiSectionVAddr, eax
    ;; modify EntryPoint
    push dword ptr [edx].VirtualAddress
    pop dword ptr [esi].OptionalHeader.AddressOfEntryPoint
    
    ;; write anti-debugger to new section
    mov edi, dword ptr [edx].PointerToRawData
    add edi, pMem
    mov dwAntiSectionFAddr, edi
    
    push edi
    push esi
    ;; moving...
    mov esi, offset AntiDebuggerSection
    mov ecx, offset End_AntiDebuggerSection - offset AntiDebuggerSection
    cld
    rep movsb
    ;; AntiModules clear 0
    xor eax, eax
    mov ecx, offset End_AntiDebuggerModules - offset AntiDebuggerModules
    rep stosb
    
    pop esi
    pop edi
    
    ;; set orig entry address
    mov ebx, edi
    add ebx, offset AntiDebugger_OrigEntryPoint - offset AntiDebuggerSection
    push dwOrigEntryAddr
    pop dword ptr [ebx]
    
    ;; judge the user select funcs
    ;; set the invoke function loop count
 	lea edx, g_AntiDebuggerArguments
	assume edx : ptr ANTIDEBUGGERARGUMENTS
	mov ecx, dword ptr [edx].dwFuncCounts    
    mov eax, edi
    add eax, offset LoopInvokeAntiFunctions_AntiDebuggerSection - offset AntiDebuggerSection
    inc eax
    mov dword ptr [eax], ecx
    mov eax, edi		; edi -> AntiDebuggerSection FVA
    add eax, offset AntiDebugger_AntiFuncs - offset AntiDebuggerSection
    mov ebx, offset AntiDebuggerModules
    add edi, offset AntiDebuggerModules - offset AntiDebuggerSection
	
	;; eax = Anti Function List
	;; ebx = AntiDebuggerModules in orig file
	;; ecx = Anti Function Length | Anti Func Counts
	;; edx = Arguments Struct
	;; esi = Anti Function Start
	;; edi = Anti FuncTion Start in target
	cld
	.while ecx != 0
		push ecx
		.if dword ptr [edx].Use_FD_IsDebuggerPresent == 1
			mov esi, ebx
			add esi, offset FD_IsDebuggerPresent - offset AntiDebuggerModules
			mov ecx, offset End_FD_IsDebuggerPresent - offset FD_IsDebuggerPresent
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_IsDebuggerPresent, 0
		.elseif dword ptr [edx].Use_FD_PEB_BeingDebuggedFlag == 1
			mov esi, ebx
			add esi, offset FD_PEB_BeingDebuggedFlag - offset AntiDebuggerModules
			mov ecx, offset End_FD_PEB_BeingDebuggedFlag - offset FD_PEB_BeingDebuggedFlag
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_PEB_BeingDebuggedFlag, 0
		.elseif dword ptr [edx].Use_FD_PEB_NtGlobalFlags == 1
			mov esi, ebx
			add esi, offset FD_PEB_NtGlobalFlags - offset AntiDebuggerModules
			mov ecx, offset End_FD_PEB_NtGlobalFlags - offset FD_PEB_NtGlobalFlags
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_PEB_NtGlobalFlags, 0
		.elseif dword ptr [edx].Use_FD_Heap_HeapFlags == 1
			mov esi, ebx
			add esi, offset FD_Heap_HeapFlags - offset AntiDebuggerModules
			mov ecx, offset End_FD_Heap_HeapFlags - offset FD_Heap_HeapFlags
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Heap_HeapFlags, 0	
		.elseif dword ptr [edx].Use_FD_Heap_ForceFlags == 1
			mov esi, ebx
			add esi, offset FD_Heap_ForceFlags - offset AntiDebuggerModules
			mov ecx, offset End_FD_Heap_ForceFlags - offset FD_Heap_ForceFlags
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Heap_ForceFlags, 0	
		.elseif dword ptr [edx].Use_FD_Heap_Magic == 1
			mov esi, ebx
			add esi, offset FD_Heap_Magic - offset AntiDebuggerModules
			mov ecx, offset End_FD_Heap_Magic - offset FD_Heap_Magic
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Heap_Magic, 0	
		.elseif dword ptr [edx].Use_FD_CheckRemoteDebuggerPresent == 1
			mov esi, ebx
			add esi, offset FD_CheckRemoteDebuggerPresent - offset AntiDebuggerModules
			mov ecx, offset End_FD_CheckRemoteDebuggerPresent - offset FD_CheckRemoteDebuggerPresent
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_CheckRemoteDebuggerPresent, 0	    
		.elseif dword ptr [edx].Use_FD_NtQueryInfoProc_DbgPort == 1
			mov esi, ebx
			add esi, offset FD_NtQueryInfoProc_DbgPort - offset AntiDebuggerModules
			mov ecx, offset End_FD_NtQueryInfoProc_DbgPort - offset FD_NtQueryInfoProc_DbgPort
			rep movsb
			mov dword ptr [edx].Use_FD_NtQueryInfoProc_DbgPort, 0	
		.elseif dword ptr [edx].Use_FD_NtQueryInfoProc_DbgObjHandle == 1
			mov esi, ebx
			add esi, offset FD_NtQueryInfoProc_DbgObjHandle - offset AntiDebuggerModules
			mov ecx, offset End_FD_NtQueryInfoProc_DbgObjHandle - offset FD_NtQueryInfoProc_DbgObjHandle
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_NtQueryInfoProc_DbgObjHandle, 0
		.elseif dword ptr [edx].Use_FD_NtQueryInfoProc_DbgFlags == 1
			mov esi, ebx
			add esi, offset FD_NtQueryInfoProc_DbgFlags - offset AntiDebuggerModules
			mov ecx, offset End_FD_NtQueryInfoProc_DbgFlags - offset FD_NtQueryInfoProc_DbgFlags
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_NtQueryInfoProc_DbgFlags, 0	
		.elseif dword ptr [edx].Use_FD_NtQueryInfoProc_SysKrlDbgInfo == 1
			mov esi, ebx
			add esi, offset FD_NtQueryInfoProc_SysKrlDbgInfo - offset AntiDebuggerModules
			mov ecx, offset End_FD_NtQueryInfoProc_SysKrlDbgInfo - offset FD_NtQueryInfoProc_SysKrlDbgInfo
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_NtQueryInfoProc_SysKrlDbgInfo, 0		
		.elseif dword ptr [edx].Use_FD_SeDebugPrivilege == 1
			mov esi, ebx
			add esi, offset FD_SeDebugPrivilege - offset AntiDebuggerModules
			mov ecx, offset End_FD_SeDebugPrivilege - offset FD_SeDebugPrivilege
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_SeDebugPrivilege, 0	
		.elseif dword ptr [edx].Use_FD_Parent_Process == 1
			mov esi, ebx
			add esi, offset FD_Parent_Process - offset AntiDebuggerModules
			mov ecx, offset End_FD_Parent_Process - offset FD_Parent_Process
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Parent_Process, 0	    
		.elseif dword ptr [edx].Use_FD_DebugObject_NtQueryObject == 1
			mov esi, ebx
			add esi, offset FD_DebugObject_NtQueryObject - offset AntiDebuggerModules
			mov ecx, offset End_FD_DebugObject_NtQueryObject - offset FD_DebugObject_NtQueryObject
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_DebugObject_NtQueryObject, 0
		.elseif dword ptr [edx].Use_FD_Find_Debugger_Window == 1
			mov esi, ebx
			add esi, offset FD_Find_Debugger_Window - offset AntiDebuggerModules
			mov ecx, offset End_FD_Find_Debugger_Window - offset FD_Find_Debugger_Window
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Find_Debugger_Window, 0	
		.elseif dword ptr [edx].Use_FD_Find_Debugger_Process == 1
			mov esi, ebx
			add esi, offset FD_Find_Debugger_Process - offset AntiDebuggerModules
			mov ecx, offset End_FD_Find_Debugger_Process - offset FD_Find_Debugger_Process
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Find_Debugger_Process, 0
		.elseif dword ptr [edx].Use_FD_Find_Device_Driver == 1
			mov esi, ebx
			add esi, offset FD_Find_Device_Driver - offset AntiDebuggerModules
			mov ecx, offset End_FD_Find_Device_Driver - offset FD_Find_Device_Driver
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Find_Device_Driver, 0
		.elseif dword ptr [edx].Use_FD_Exception_Int3 == 1
			mov esi, ebx
			add esi, offset FD_Exception_Int3 - offset AntiDebuggerModules
			mov ecx, offset End_FD_Exception_Int3 - offset FD_Exception_Int3
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Exception_Int3, 0
		.elseif dword ptr [edx].Use_FD_Exception_Popf == 1
			mov esi, ebx
			add esi, offset FD_Exception_Popf - offset AntiDebuggerModules
			mov ecx, offset End_FD_Exception_Popf - offset FD_Exception_Popf
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_Exception_Popf, 0	    
		.elseif dword ptr [edx].Use_FD_OutputDebugString == 1
			mov esi, ebx
			add esi, offset FD_OutputDebugString - offset AntiDebuggerModules
			mov ecx, offset End_FD_OutputDebugString - offset FD_OutputDebugString
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FD_OutputDebugString, 0
		.elseif dword ptr [edx].Use_FS_OD_Exception_GuardPages == 1
			mov esi, ebx
			add esi, offset FS_OD_Exception_GuardPages - offset AntiDebuggerModules
			mov ecx, offset End_FS_OD_Exception_GuardPages - offset FS_OD_Exception_GuardPages
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FS_OD_Exception_GuardPages, 0
		.elseif dword ptr [edx].Use_FS_SI_UnhandledExceptionFilter == 1
			mov esi, ebx
			add esi, offset FS_SI_UnhandledExceptionFilter - offset AntiDebuggerModules
			mov ecx, offset End_FS_SI_UnhandledExceptionFilter - offset FS_SI_UnhandledExceptionFilter
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FS_SI_UnhandledExceptionFilter, 0
		.elseif dword ptr [edx].Use_FS_ODP_Process32NextW == 1
			mov esi, ebx
			add esi, offset FS_ODP_Process32NextW - offset AntiDebuggerModules
			mov ecx, offset End_FS_ODP_Process32NextW - offset FS_ODP_Process32NextW
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FS_ODP_Process32NextW, 0
		.elseif dword ptr [edx].Use_FS_ODP_OutputDebugStringA == 1
			mov esi, ebx
			add esi, offset FS_ODP_OutputDebugStringA - offset AntiDebuggerModules
			mov ecx, offset End_FS_ODP_OutputDebugStringA - offset FS_ODP_OutputDebugStringA
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FS_ODP_OutputDebugStringA, 0
		.elseif dword ptr [edx].Use_FS_ODP_OpenProcess == 1
			mov esi, ebx
			add esi, offset FS_ODP_OpenProcess - offset AntiDebuggerModules
			mov ecx, offset End_FS_ODP_OpenProcess - offset FS_ODP_OpenProcess
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FS_ODP_OpenProcess, 0
		.elseif dword ptr [edx].Use_FV_VMWare_VMX == 1
			mov esi, ebx
			add esi, offset FV_VMWare_VMX - offset AntiDebuggerModules
			mov ecx, offset End_FV_VMWare_VMX - offset FV_VMWare_VMX
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FV_VMWare_VMX, 0
		.elseif dword ptr [edx].Use_FV_VPC_Exception == 1
			mov esi, ebx
			add esi, offset FV_VPC_Exception - offset AntiDebuggerModules
			mov ecx, offset End_FV_VPC_Exception - offset FV_VPC_Exception
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FV_VPC_Exception, 0
		.elseif dword ptr [edx].Use_FB_HWBP_Exception == 1
			mov esi, ebx
			add esi, offset FB_HWBP_Exception - offset AntiDebuggerModules
			mov ecx, offset End_FB_HWBP_Exception - offset FB_HWBP_Exception
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FB_HWBP_Exception, 0
		.elseif dword ptr [edx].Use_FB_SWBP_Memory_CRC == 1
			mov esi, ebx
			add esi, offset FB_SWBP_Memory_CRC - offset AntiDebuggerModules
			mov ecx, offset End_FB_SWBP_Memory_CRC - offset FB_SWBP_Memory_CRC
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FB_SWBP_Memory_CRC, 0
		.elseif dword ptr [edx].Use_FT_PushSS_PopSS == 1
			mov esi, ebx
			add esi, offset FT_PushSS_PopSS - offset AntiDebuggerModules
			mov ecx, offset End_FT_PushSS_PopSS - offset FT_PushSS_PopSS
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FT_PushSS_PopSS, 0
		.elseif dword ptr [edx].Use_FT_RDTSC == 1
			mov esi, ebx
			add esi, offset FT_RDTSC - offset AntiDebuggerModules
			mov ecx, offset End_FT_RDTSC - offset FT_RDTSC
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FT_RDTSC, 0
		.elseif dword ptr [edx].Use_FT_GetTickCount == 1
			mov esi, ebx
			add esi, offset FT_GetTickCount - offset AntiDebuggerModules
			mov ecx, offset End_FT_GetTickCount - offset FT_GetTickCount
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FT_GetTickCount, 0
		.elseif dword ptr [edx].Use_FT_timeGetTime == 1
			mov esi, ebx
			add esi, offset FT_timeGetTime - offset AntiDebuggerModules
			mov ecx, offset End_FT_timeGetTime - offset FT_timeGetTime
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FT_timeGetTime, 0
		.elseif dword ptr [edx].Use_FT_SharedUserData_TickCount == 1
			mov esi, ebx
			add esi, offset FT_SharedUserData_TickCount - offset AntiDebuggerModules
			mov ecx, offset End_FT_SharedUserData_TickCount - offset FT_SharedUserData_TickCount
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FT_SharedUserData_TickCount, 0
		.elseif dword ptr [edx].Use_FT_INT1_IceBreakpoint == 1
			mov esi, ebx
			add esi, offset FT_INT1_IceBreakpoint - offset AntiDebuggerModules
			mov ecx, offset End_FT_INT1_IceBreakpoint - offset FT_INT1_IceBreakpoint
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FT_INT1_IceBreakpoint, 0
		.elseif dword ptr [edx].Use_FP_Check_FileSize == 1
			mov esi, ebx
			add esi, offset FP_Check_FileSize - offset AntiDebuggerModules
			mov ecx, offset End_FP_Check_FileSize - offset FP_Check_FileSize
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FP_Check_FileSize, 0
		.elseif dword ptr [edx].Use_FP_Check_FileHashValue_CRC == 1
			mov esi, ebx
			add esi, offset FP_Check_FileHashValue_CRC - offset AntiDebuggerModules
			mov ecx, offset End_FP_Check_FileHashValue_CRC - offset FP_Check_FileHashValue_CRC
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_FP_Check_FileHashValue_CRC, 0
		.elseif dword ptr [edx].Use_AD_SwitchDesktop == 1
			mov esi, ebx
			add esi, offset AD_SwitchDesktop - offset AntiDebuggerModules
			mov ecx, offset End_AD_SwitchDesktop - offset AD_SwitchDesktop
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_AD_SwitchDesktop, 0	
		.elseif dword ptr [edx].Use_AD_BlockInput == 1
			mov esi, ebx
			add esi, offset AD_BlockInput - offset AntiDebuggerModules
			mov ecx, offset End_AD_BlockInput - offset AD_BlockInput
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_AD_BlockInput, 0
		.elseif dword ptr [edx].Use_AD_ZwSetInformationThread == 1
			mov esi, ebx
			add esi, offset AD_ZwSetInformationThread - offset AntiDebuggerModules
			mov ecx, offset End_AD_ZwSetInformationThread - offset AD_ZwSetInformationThread
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_AD_ZwSetInformationThread, 0
		.elseif dword ptr [edx].Use_AD_INT_2e == 1
			mov esi, ebx
			add esi, offset AD_INT_2e - offset AntiDebuggerModules
			mov ecx, offset End_AD_INT_2e - offset AD_INT_2e
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_AD_INT_2e, 0
		.elseif dword ptr [edx].Use_AS_OD_OutputDebugString == 1
			mov esi, ebx
			add esi, offset AS_OD_OutputDebugString - offset AntiDebuggerModules
			mov ecx, offset End_AS_OD_OutputDebugString - offset AS_OD_OutputDebugString
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_AS_OD_OutputDebugString, 0
		.elseif dword ptr [edx].Use_AB_HWP_CLR_Exception == 1
			mov esi, ebx
			add esi, offset AB_HWP_CLR_Exception - offset AntiDebuggerModules
			mov ecx, offset End_AB_HWP_CLR_Exception - offset AB_HWP_CLR_Exception
			mov dwAntiFuncsLength, ecx
			rep movsb
			mov dword ptr [edx].Use_AB_HWP_CLR_Exception, 0
		.endif
		;; set address to the list
		push ebx
		mov ebx, edi
		sub ebx, dwAntiFuncsLength
		sub ebx, dwAntiSectionFAddr
		add ebx, dwAntiSectionVAddr
		mov dword ptr [eax], ebx
		add eax, 04h
		pop ebx
		pop ecx
		dec ecx
	.endw
	
	;; set the CRC value
	
LogicShellExit:
    ;; close handle & write it
    invoke UnmapViewOfFile, pMem
    invoke CloseHandle, hMap
    invoke CloseHandle, hFile
    .IF g_bError == 0
        ;; show success message  
        ;;invoke PrintLine, offset g_szOutFormat, offset g_szDone
    .ENDIF
    assume eax : nothing
    assume edx : nothing
    assume esi : nothing
    assume edi : nothing
    ret
    
;; ----- Show error message ----- 
OpenFileFailed:
    lea eax, g_szOpenFileFailed
    jmp ShowErr
GetFileSizeFailed:
    lea eax, g_szGetFileSizeFailed
    jmp ShowErr    
CreateMapFailed:
    lea eax, g_szCreateMapFailed
    jmp ShowErr
MapFileFailed:
    lea eax, g_szMapFileFailed
    jmp ShowErr        
InvalidPE:
    lea eax, g_szInvalidPE
    jmp ShowErr   
ShowErr:
    invoke crt_printf, offset g_szOutFormat, eax
    mov al, 1
    mov g_bError, al
    jmp LogicShellExit

;; ----- Anti Debugger -----
AntiDebuggerSection:
AntiDebuggerSection_StackSize		equ 04h
AntiDebuggerSection_Eip				equ -04h
	push ebp
	mov ebp, esp
	sub esp, AntiDebuggerSection_StackSize
	;; get Win32 API address
	call Get_AntiDebuggerSection_Eip
	Get_AntiDebuggerSection_Eip:
	pop eax
	mov dword ptr [ebp+AntiDebuggerSection_Eip], eax
	add eax, offset AntiDebugger_Win32ApiBase - offset Get_AntiDebuggerSection_Eip
	push eax
	call GetApiAddress
	;; set Assist function list
	mov edi, dword ptr [ebp+AntiDebuggerSection_Eip]
	add edi, offset AntiDebugger_FuncTbl - offset Get_AntiDebuggerSection_Eip
	assume edi : ptr FUNCADDRTBL
	
	mov ebx, dword ptr [ebp+AntiDebuggerSection_Eip]
	
	mov eax, ebx
	add eax, offset GetFileNameFromPath - offset Get_AntiDebuggerSection_Eip
	mov dword ptr [edi].xGetFileNameFromPath, eax
	
	mov eax, ebx
	add eax, offset CRC32 - offset Get_AntiDebuggerSection_Eip
	mov dword ptr [edi].xCRC32, eax	
	
	mov eax, ebx
	add eax, offset GetThreadHandle - offset Get_AntiDebuggerSection_Eip
	mov dword ptr [edi].xGetThreadHandle, eax
	
	mov eax, ebx
	add eax, offset AntiDebugger_Win32ApiBase - offset Get_AntiDebuggerSection_Eip
	mov edx, eax
	
	mov eax,  ebx
	add eax, offset AntiDebugger_AntiFuncs - offset Get_AntiDebuggerSection_Eip
	mov esi, eax
	;; loop invoke the select function	
LoopInvokeAntiFunctions_AntiDebuggerSection:
	mov ecx, 00000000h
	cld
Start_LoopInvokeAntiFunctions_AntiDebuggerSection:
	push edi	; FuncAddrTbl
	push edx	; Win32ApiBase
	lodsd
	call eax	; Anti Function
	add esp, 08h
	test eax, eax
	jnz Found_Debugger
	loop Start_LoopInvokeAntiFunctions_AntiDebuggerSection
Exit_AntiDebuggerSection:
	;; goto the orig code section
	mov eax, dword ptr [ebp+AntiDebuggerSection_Eip]
	add eax, offset AntiDebugger_OrigEntryPoint - offset Get_AntiDebuggerSection_Eip
	;; clear the stack
	mov esp, ebp
	pop ebp
	jmp dword ptr [eax]
Found_Debugger:
	push 0
	assume edx : ptr WIN32APIBASE
	call dword ptr [edx].xExitProcess
;; ----- Private Data -----
AntiDebugger_Win32ApiBase		WIN32APIBASE <0>
AntiDebugger_FuncTbl			FUNCADDRTBL <0>
AntiDebugger_OrigEntryPoint     dd 0
	
;; ----- Assist Func -----
GetFileNameFromPath:
GetFileNameFromPath_Arg_szSource    equ 08h
    push ebp
    mov ebp, esp
    
    push edi
    push esi
    
    ;; count the length
    mov esi, dword ptr [ebp+GetFileNameFromPath_Arg_szSource]
    mov edi, esi
    mov al, NULL
    cld
    repnz scasb
    dec edi
    mov al, '\'
    std
    repnz scasb
    add edi, 02h
    mov eax, edi
    
    pop esi
    pop edi
    
    mov esp, ebp
    pop ebp
    retn 04h
End_GetFileNameFromPath:

CRC32:
CRC32_Arg_ptr       equ 08h
CRC32_Arg_Size      equ 0ch
CRC32_StackSize     equ 0400h
CRC32_crcTable      equ -0400h
    push ebp
    mov ebp, esp
    sub esp, CRC32_StackSize
    
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    ;; dynamic create CRC32 table
    xor ecx, ecx
    lea esi, [ebp+CRC32_crcTable]
    CreateCRC32Tbl_Loop1:
    cmp ecx, 0100h
    jz EndCreateCRC32Tbl_Loop1
    mov edx, ecx
    push ecx
    mov ecx, 08h
    CreateCRC32Tbl_Loop2:
    test edx, 01h
    jz CreateCRC32Tbl_Loop2_Tmp1
    shr edx, 01h
    xor edx, 0EDB88320h
    jmp CreateCRC32Tbl_Loop2_Tmp2
    CreateCRC32Tbl_Loop2_Tmp1:
    shr edx, 01h
    CreateCRC32Tbl_Loop2_Tmp2:
    dec ecx
    jnz CreateCRC32Tbl_Loop2
    pop ecx
    mov dword ptr [esi+ecx*04h], edx
    inc ecx
 	jmp CreateCRC32Tbl_Loop1
    EndCreateCRC32Tbl_Loop1:
    
    ;; calcu CRC32 value
    mov edx, 0FFFFFFFFh
    mov ecx, dword ptr [ebp+CRC32_Arg_Size]
    mov edi, dword ptr [ebp+CRC32_Arg_ptr]
    MakeCRC32Tbl_Loop:
    mov al, byte ptr [edi]
    movzx eax, al
    xor eax, edx
    and eax, 0FFh
    mov eax, dword ptr [esi+eax*04h]
    shr edx, 08h
    and edx, 00FFFFFFh
    xor edx, eax
    inc edi
    dec ecx
    jnz MakeCRC32Tbl_Loop
    xor edx, 0FFFFFFFFh
    mov eax, edx
Exit_EndCRC32:   
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    
    mov esp, ebp
    pop ebp    
    retn 08h
End_CRC32:


GetThreadHandle:
GetThreadHandle_Arg_Win32Api		equ 08h
GetThreadHandle_Arg_hProcess		equ 0ch
GetThreadHandle_StackSize			equ 10h + sizeof THREADENTRY32
GetThreadHandle_dwPid				equ -04h
GetThreadHandle_hThreadSnap			equ -08h
GetThreadHandle_dwThreadID			equ -0ch
GetThreadHandle_entry				equ -(10h+sizeof THREADENTRY32)
	push ebp
	mov ebp, esp
	sub esp, GetThreadHandle_StackSize
	
	push ebx
	push ecx
	push edx
	push edi
	
	;; clear the stack
	lea edi, [ebp+GetThreadHandle_entry]
	mov ecx, GetThreadHandle_StackSize
	xor eax, eax
	cld
	rep stosb
	
	mov ebx, dword ptr [ebp+GetThreadHandle_Arg_Win32Api]
	assume ebx : ptr WIN32APIBASE
	
	push dword ptr [ebp+GetThreadHandle_Arg_hProcess]
	call dword ptr [ebx].xGetProcessId
	mov dword ptr [ebp+GetThreadHandle_dwPid], eax
	push eax
	push TH32CS_SNAPTHREAD
	call dword ptr [ebx].xCreateToolhelp32Snapshot
	mov dword ptr [ebp+GetThreadHandle_hThreadSnap], eax
	
	lea edx, [ebp+GetThreadHandle_entry]
	assume edx : ptr THREADENTRY32
	push sizeof THREADENTRY32
	pop dword ptr [edx].dwSize
	
	push edx
	push dword ptr [ebp+GetThreadHandle_hThreadSnap]
	call dword ptr [ebx].xThread32First
	test eax, eax
	jz Exit_GetThreadHandle
	mov eax, dword ptr [edx].th32OwnerProcessID
	sub eax, dword ptr [ebp+GetThreadHandle_dwPid]
	jnz GetThreadHandle_Tmp1
	push dword ptr [edx].th32ThreadID
	pop dword ptr [ebp+GetThreadHandle_dwPid]
	GetThreadHandle_Tmp1:
	push edx
	push dword ptr [ebp+GetThreadHandle_hThreadSnap]
	call dword ptr [ebx].xThread32Next
	test eax, eax
	jz Exit_GetThreadHandle
	mov eax, dword ptr [edx].th32OwnerProcessID
	sub eax, dword ptr [ebp+GetThreadHandle_dwPid]
	jnz GetThreadHandle_Tmp1
	push dword ptr [edx].th32ThreadID
	pop dword ptr [ebp+GetThreadHandle_dwThreadID]
Exit_GetThreadHandle:
	
	push dword ptr [ebp+GetThreadHandle_hThreadSnap]
	call dword ptr [ebx].xCloseHandle
	push dword ptr [ebp+GetThreadHandle_dwThreadID]
	push FALSE
	push THREAD_ALL_ACCESS
	call dword ptr [ebx].xOpenProcess
	
	pop edi	
	pop edx
	pop ecx
	pop ebx
	assume ebx : nothing
	assume edx : nothing
	mov esp, ebp
	pop ebp
	retn 08h
End_GetThreadHandle:

GetApiAddress:
GetApiAddress_Arg_Win32ApiBasePoint		equ 08h
	push ebp
	mov ebp, esp
	
	push ebx
	push ecx
	push edx
	push edi
	push esi
	
	call GetApiKeyAddress
	
	mov edi, dword ptr [ebp+GetApiAddress_Arg_Win32ApiBasePoint]
	assume edi : ptr WIN32APIBASE
	
	mov dword ptr [edi].hKernel32Dll, esi
	mov dword ptr [edi].xGetProcAddress, ebx
	
	call strGetApiAddress_LoadLibraryA
		db "LoadLibraryA",0
	strGetApiAddress_LoadLibraryA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xLoadLibraryA, eax

	call strGetApiAddress_GetModuleFileNameA
		db "GetModuleFileNameA",0
	strGetApiAddress_GetModuleFileNameA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetModuleFileNameA, eax
	
	call strGetApiAddress_GetCurrentProcess
		db "GetCurrentProcess",0
	strGetApiAddress_GetCurrentProcess:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetCurrentProcess, eax
	
	call strGetApiAddress_GetCurrentProcessId
		db "GetCurrentProcessId",0
	strGetApiAddress_GetCurrentProcessId:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetCurrentProcessId, eax
	
	call strGetApiAddress_GetCurrentThread
		db "GetCurrentThread",0
	strGetApiAddress_GetCurrentThread:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetCurrentThread, eax		
	
	call strGetApiAddress_GetCurrentThreadId
		db "GetCurrentThreadId",0
	strGetApiAddress_GetCurrentThreadId:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetCurrentThreadId, eax
	
	call strGetApiAddress_GetWindowsDirectoryA
		db "GetWindowsDirectoryA",0
	strGetApiAddress_GetWindowsDirectoryA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetWindowsDirectoryA, eax
	
	call strGetApiAddress_GetProcessHeap
		db "GetProcessHeap",0
	strGetApiAddress_GetProcessHeap:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetProcessHeap, eax
	
	call strGetApiAddress_GetLastError
		db "GetLastError",0
	strGetApiAddress_GetLastError:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetLastError, eax
	
	call strGetApiAddress_GetVersion
		db "GetVersion",0
	strGetApiAddress_GetVersion:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetVersion, eax
	
	call strGetApiAddress_GetVersionExA
		db "GetVersionExA",0
	strGetApiAddress_GetVersionExA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetVersionExA, eax
	
	call strGetApiAddress_GetFileSize
		db "GetFileSize",0
	strGetApiAddress_GetFileSize:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetFileSize, eax
	
	call strGetApiAddress_GetTickCount
		db "GetTickCount",0
	strGetApiAddress_GetTickCount:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetTickCount, eax
	
	call strGetApiAddress_GetProcessId
		db "GetProcessId",0
	strGetApiAddress_GetProcessId:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetProcessId, eax

	call strGetApiAddress_CreateToolhelp32Snapshot
		db "CreateToolhelp32Snapshot",0
	strGetApiAddress_CreateToolhelp32Snapshot:
	call InvokeGetProcAddress
	mov dword ptr [edi].xCreateToolhelp32Snapshot, eax
	
	call strGetApiAddress_CreateFileA
		db "CreateFileA",0
	strGetApiAddress_CreateFileA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xCreateFileA, eax
	
	call strGetApiAddress_CloseHandle
		db "CloseHandle",0
	strGetApiAddress_CloseHandle:
	call InvokeGetProcAddress
	mov dword ptr [edi].xCloseHandle, eax
	
	call strGetApiAddress_CheckRemoteDebuggerPresent
		db "CheckRemoteDebuggerPresent",0
	strGetApiAddress_CheckRemoteDebuggerPresent:
	call InvokeGetProcAddress
	mov dword ptr [edi].xCheckRemoteDebuggerPresent, eax		
	
	call strGetApiAddress_Process32First
		db "Process32First",0
	strGetApiAddress_Process32First:
	call InvokeGetProcAddress
	mov dword ptr [edi].xProcess32First, eax
	
	call strGetApiAddress_Process32Next
		db "Process32Next",0
	strGetApiAddress_Process32Next:
	call InvokeGetProcAddress
	mov dword ptr [edi].xProcess32Next, eax
	
	call strGetApiAddress_Process32NextW
		db "Process32NextW",0
	strGetApiAddress_Process32NextW:
	call InvokeGetProcAddress
	mov dword ptr [edi].xProcess32NextW, eax
	
	call strGetApiAddress_Module32First
		db "Module32First",0
	strGetApiAddress_Module32First:
	call InvokeGetProcAddress
	mov dword ptr [edi].xModule32First, eax
	
	call strGetApiAddress_Module32Next
		db "Module32Next",0
	strGetApiAddress_Module32Next:
	call InvokeGetProcAddress
	mov dword ptr [edi].xModule32Next, eax
	
	call strGetApiAddress_OpenProcess
		db "OpenProcess",0
	strGetApiAddress_OpenProcess:
	call InvokeGetProcAddress
	mov dword ptr [edi].xOpenProcess, eax
	
	call strGetApiAddress_OutputDebugStringA
		db "OutputDebugStringA",0
	strGetApiAddress_OutputDebugStringA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xOutputDebugStringA, eax
	
	call strGetApiAddress_VirtualAlloc
		db "VirtualAlloc",0
	strGetApiAddress_VirtualAlloc:
	call InvokeGetProcAddress
	mov dword ptr [edi].xVirtualAlloc, eax
	
	call strGetApiAddress_VirtualFree
		db "VirtualFree",0
	strGetApiAddress_VirtualFree:
	call InvokeGetProcAddress
	mov dword ptr [edi].xVirtualFree, eax

	call strGetApiAddress_VirtualProtect
		db "VirtualProtect",0
	strGetApiAddress_VirtualProtect:
	call InvokeGetProcAddress
	mov dword ptr [edi].xVirtualProtect, eax
	
	call strGetApiAddress_HeapAlloc
		db "HeapAlloc",0
	strGetApiAddress_HeapAlloc:
	call InvokeGetProcAddress
	mov dword ptr [edi].xHeapAlloc, eax
	
	call strGetApiAddress_HeapFree
		db "HeapFree",0
	strGetApiAddress_HeapFree:
	call InvokeGetProcAddress
	mov dword ptr [edi].xHeapFree, eax		
	
	call strGetApiAddress_UnhandledExceptionFilter
		db "UnhandledExceptionFilter",0
	strGetApiAddress_UnhandledExceptionFilter:
	call InvokeGetProcAddress
	mov dword ptr [edi].xUnhandledExceptionFilter, eax
	
	call strGetApiAddress_ReadFile
		db "ReadFile",0
	strGetApiAddress_ReadFile:
	call InvokeGetProcAddress
	mov dword ptr [edi].xReadFile, eax
	
	call strGetApiAddress_Thread32First
		db "Thread32First",0
	strGetApiAddress_Thread32First:
	call InvokeGetProcAddress
	mov dword ptr [edi].xThread32First, eax		
	
	call strGetApiAddress_Thread32Next
		db "Thread32Next",0
	strGetApiAddress_Thread32Next:
	call InvokeGetProcAddress
	mov dword ptr [edi].xThread32Next, eax
	
	call strGetApiAddress_TerminateProcess
		db "TerminateProcess",0
	strGetApiAddress_TerminateProcess:
	call InvokeGetProcAddress
	mov dword ptr [edi].xTerminateProcess, eax
	
	call strGetApiAddress_SuspendThread
		db "SuspendThread",0
	strGetApiAddress_SuspendThread:
	call InvokeGetProcAddress
	mov dword ptr [edi].xSuspendThread, eax
	
	call strGetApiAddress_Sleep
		db "Sleep",0
	strGetApiAddress_Sleep:
	call InvokeGetProcAddress
	mov dword ptr [edi].xSleep, eax
	
	call strGetApiAddress_SetUnhandledExceptionFilter
		db "SetUnhandledExceptionFilter",0
	strGetApiAddress_SetUnhandledExceptionFilter:
	call InvokeGetProcAddress
	mov dword ptr [edi].xSetUnhandledExceptionFilter, eax
	
	call strGetApiAddress_IsDebuggerPresent
		db "IsDebuggerPresent",0
	strGetApiAddress_IsDebuggerPresent:
	call InvokeGetProcAddress
	mov dword ptr [edi].xIsDebuggerPresent, eax
	
	call strGetApiAddress_QueryPerformanceCounter
		db "QueryPerformanceCounter",0
	strGetApiAddress_QueryPerformanceCounter:
	call InvokeGetProcAddress
	mov dword ptr [edi].xQueryPerformanceCounter, eax
	
	call strGetApiAddress_ExitProcess
		db "ExitProcess",0
	strGetApiAddress_ExitProcess:
	call InvokeGetProcAddress
	mov dword ptr [edi].xExitProcess, eax	
	
	call strGetApiAddress_lstrcpyA
		db "lstrcpyA",0
	strGetApiAddress_lstrcpyA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xlstrcpyA, eax
	
	call strGetApiAddress_lstrcatA
		db "lstrcatA",0
	strGetApiAddress_lstrcatA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xlstrcatA, eax	
	
	call strGetApiAddress_lstrcmpA
		db "lstrcmpA",0
	strGetApiAddress_lstrcmpA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xlstrcmpA, eax
	
	call strGetApiAddress_lstrcmpiA
		db "lstrcmpiA",0
	strGetApiAddress_lstrcmpiA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xlstrcmpiA, eax
	
	call strGetApiAddress_lstrlenA
		db "lstrlenA",0
	strGetApiAddress_lstrlenA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xlstrlenA, eax
	
	;; user32.dll function
	call strGetApiAddress_User32Dll
		db "user32.dll",0
	strGetApiAddress_User32Dll:
	call dword ptr [edi].xLoadLibraryA
	mov dword ptr [edi].hUser32Dll, eax
	mov esi, eax
	
	call strGetApiAddress_BlockInput
		db "BlockInput",0
	strGetApiAddress_BlockInput:
	call InvokeGetProcAddress
	mov dword ptr [edi].xBlockInput, eax
	
	call strGetApiAddress_CreateDesktopA
		db "CreateDesktopA",0
	strGetApiAddress_CreateDesktopA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xCreateDesktopA, eax
	
	call strGetApiAddress_CloseDesktop
		db "CloseDesktop",0
	strGetApiAddress_CloseDesktop:
	call InvokeGetProcAddress
	mov dword ptr [edi].xCloseDesktop, eax
	
	call strGetApiAddress_GetThreadDesktop
		db "GetThreadDesktop",0
	strGetApiAddress_GetThreadDesktop:
	call InvokeGetProcAddress
	mov dword ptr [edi].xGetThreadDesktop, eax
	
	call strGetApiAddress_SetThreadDesktop
		db "SetThreadDesktop",0
	strGetApiAddress_SetThreadDesktop:
	call InvokeGetProcAddress
	mov dword ptr [edi].xSetThreadDesktop, eax
		
	call strGetApiAddress_SwitchDesktop
		db "SwitchDesktop",0
	strGetApiAddress_SwitchDesktop:
	call InvokeGetProcAddress
	mov dword ptr [edi].xSwitchDesktop, eax	
	
	call strGetApiAddress_OpenInputDesktop
		db "OpenInputDesktop",0
	strGetApiAddress_OpenInputDesktop:
	call InvokeGetProcAddress
	mov dword ptr [edi].xOpenInputDesktop, eax
	
	call strGetApiAddress_FindWindowA
		db "FindWindowA",0
	strGetApiAddress_FindWindowA:
	call InvokeGetProcAddress
	mov dword ptr [edi].xFindWindowA, eax	
	
	;; ntdll.dll function
	call strGetApiAddress_NtdllDll
		db "ntdll.dll",0
	strGetApiAddress_NtdllDll:
	call dword ptr [edi].xLoadLibraryA
	mov dword ptr [edi].hNtdllDll, eax
	mov esi, eax
	
	call strGetApiAddress_ZwQueryInformationProcess
		db "ZwQueryInformationProcess",0
	strGetApiAddress_ZwQueryInformationProcess:
	call InvokeGetProcAddress
	mov dword ptr [edi].xZwQueryInformationProcess, eax
	
	call strGetApiAddress_ZwQuerySystemInformation
		db "ZwQuerySystemInformation",0
	strGetApiAddress_ZwQuerySystemInformation:
	call InvokeGetProcAddress
	mov dword ptr [edi].xZwQuerySystemInformation, eax
	
	call strGetApiAddress_ZwSetInformationThread
		db "ZwSetInformationThread",0
	strGetApiAddress_ZwSetInformationThread:
	call InvokeGetProcAddress
	mov dword ptr [edi].xZwSetInformationThread, eax
	
	call strGetApiAddress_NtQueryObject
		db "NtQueryObject",0
	strGetApiAddress_NtQueryObject:
	call InvokeGetProcAddress
	mov dword ptr [edi].xNtQueryObject, eax	
	
	;; Winmm.dll function
	call strGetApiAddress_WinmmDll
		db "winmm.dll",0
	strGetApiAddress_WinmmDll:
	call dword ptr [edi].xLoadLibraryA
	mov dword ptr [edi].hWinmmDll, eax
	mov esi, eax
	
	call strGetApiAddress_timeGetTime
		db "timeGetTime",0
	strGetApiAddress_timeGetTime:
	call InvokeGetProcAddress
	mov dword ptr [edi].xtimeGetTime, eax
	
	pop esi
	pop edi
	pop edx
	pop ecx
	pop ebx
	assume edx : nothing
	mov esp, ebp
	pop ebp
	retn 04h

InvokeGetProcAddress:
InvokeGetProcAddress_Arg_Str	equ 04h
;; esi -> dll handle
	push dword ptr [esp+InvokeGetProcAddress_Arg_Str]
	push esi
	call ebx
	retn 04h
	
GetApiKeyAddress:
GetApiKeyAddress_StackSize       equ 08h
GetApiKeyAddress_hKernel32Dll    equ 04h
GetApiKeyAddress_dwExportsRVA    equ 08h
;; ebx -> GetProcAddress
;; esi -> Kernel32.dll                           
    push ebp
    mov ebp, esp
    sub esp, GetApiKeyAddress_StackSize       
    ;; find Kernel32.Dll handle
	cld
	xor esi, esi
	assume fs:nothing
	lods dword ptr fs:[esi]
    Tmp1_GetKeyApiAddress:
	inc eax
	jz Tmp2_GetKeyApiAddress
	dec eax
	xchg esi, eax
	lodsd
	jmp short Tmp1_GetKeyApiAddress
    Tmp2_GetKeyApiAddress:
	lodsd
	xchg esi, eax
    Tmp3_GetKeyApiAddress:
	dec esi
	xor si, si
	mov eax, dword ptr [esi]
	add ax, 0A5B3h
	jnz Tmp3_GetKeyApiAddress
	assume esi : ptr IMAGE_DOS_HEADER
	mov edi, [esi].e_lfanew
	mov eax, dword ptr [esi+edi]
	add eax, 0FFFFBAB0h
	jnz Tmp3_GetKeyApiAddress
	mov dword ptr [ebp-GetApiKeyAddress_hKernel32Dll], esi           
    ;; find GetProcAddress Address
    GetPeExportTable:
    mov edi, esi
    add edi, [esi].e_lfanew
    assume edi : ptr IMAGE_NT_HEADERS
    mov eax, [edi].OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT * sizeof IMAGE_DATA_DIRECTORY].VirtualAddress
    add eax, esi
    mov dword ptr [ebp-GetApiKeyAddress_dwExportsRVA], eax
    GetExportNameList:
    assume eax : ptr IMAGE_EXPORT_DIRECTORY
    mov ebx, [eax].AddressOfNames
    add ebx, esi
    xor eax, eax
    mov edx, esi
    LoopFindApiStr:
    add ebx, 04h
    inc eax
    mov edi, [ebx]
    add edi, edx
    call PushStrGetrocAddress
        db 'GetProcAddress',0
    PushStrGetrocAddress:
    pop esi
    xor ecx, ecx
    mov cl, 0fh
    cld
    repz cmpsb
    jnz short LoopFindApiStr
    mov esi, edx
    mov ecx, eax 
    mov eax, dword ptr [ebp-GetApiKeyAddress_dwExportsRVA]
    mov ebx, [eax].AddressOfFunctions
    add ebx, esi
    mov ebx, [ebx+ecx*4]
    add ebx, esi            
    assume eax : nothing
    assume esi : nothing
    assume edi : nothing  
    mov esp, ebp
    pop ebp          
    retn 0h
End_GetApiAddress:

;; ----- Anti Functions List -----
AntiDebugger_AntiFuncs	db 200h dup (0)
	
End_AntiDebuggerSection:

;; ----- AntiDebuggerModules -----
AntiDebuggerModules:
;; find debugger
FD_IsDebuggerPresent:
FD_IsDebugger_Arg_Win32Api      equ 04h
    mov eax, dword ptr [esp+FD_IsDebugger_Arg_Win32Api]
    assume eax : ptr WIN32APIBASE
    call dword ptr [eax].xIsDebuggerPresent
Exit_FD_IsDebuggerPresent:
	assume eax : nothing
    ;retn 04h
    retn 0
End_FD_IsDebuggerPresent:


FD_PEB_BeingDebuggedFlag:
    assume fs : nothing
    mov eax, fs:[30h]   ; eax = TEB.ProcessEnvironmentBlock
    inc eax
    inc eax
    mov eax, dword ptr [eax]
    and eax, 000000FFh  ; al = PEB.BeingDebugged
    test eax, eax
    jnz Found_FD_PEB_BeingDebuggedFlag
Exit_PEB_BeingDebuggedFlag:
    retn 0    
Found_FD_PEB_BeingDebuggedFlag:
    mov eax, 1
    jmp  Exit_PEB_BeingDebuggedFlag
End_FD_PEB_BeingDebuggedFlag:


FD_PEB_NtGlobalFlags:
    assume fs : nothing
    mov eax, fs:[30h]
    mov eax, dword ptr [eax+68h]
    and eax, 070h
    test eax, eax
    jnz Found_FD_PEB_NtGlobalFlags
Exit_FD_PEB_NtGlobalFlags:
    retn 0
Found_FD_PEB_NtGlobalFlags:
    mov eax, 1
    jmp  Exit_FD_PEB_NtGlobalFlags
End_FD_PEB_NtGlobalFlags:


FD_Heap_ForceFlags:
    assume fs : nothing
    mov eax, fs:[30h]
    mov eax, dword ptr [eax+18h]    ; PEB.ProcessHeap
    mov eax, dword ptr [eax+10h]    ; PEB.ProcessHeap.Flags
    test eax, eax
    jnz Found_FD_Heap_ForceFlags
Exit_FD_Heap_ForceFlag:
    retn 0
Found_FD_Heap_ForceFlags:
    mov eax, 1
    jmp Exit_FD_Heap_ForceFlag           
End_FD_Heap_ForceFlags:


FD_Heap_HeapFlags:
    assume fs : nothing
    mov eax, fs:[30h]
    mov eax, dword ptr [eax+18h]    ; PEB.ProcessHeap
    mov eax, dword ptr [eax+0ch]    ; PEB.ProcessHeap.ForceFlags
    cmp eax, 2
    jnz Found_FD_Heap_HeapFlags
    xor eax, eax
Exit_FD_Heap_HeapFlags:
    retn 0  
Found_FD_Heap_HeapFlags:
    mov eax, 1
    jmp Exit_FD_Heap_HeapFlags  
End_FD_Heap_HeapFlags:


FD_Heap_Magic:
FD_Heap_Magic_Arg_Win32Api		equ 04h
	mov eax, dword ptr [esp+FD_Heap_Magic_Arg_Win32Api]
	
	push ebx
	push ecx
	push edx
	push esi
	push edi
	
	mov ebx, eax
	assume ebx : ptr WIN32APIBASE
	
	push 100h
	push NULL
	call dword ptr [ebx].xGetProcessHeap
	mov edi, eax	; HeapHandle
	push eax
	call dword ptr [ebx].xHeapAlloc
	mov esi, eax	; HeapMem
	xor ecx, ecx
	mov edx, 100h
	cld
	FD_Heap_Magic_Loop:
	lodsd
	cmp eax, 0ABABABABh
	jnz FD_Heap_Magic_Tmp1
	inc ecx
	FD_Heap_Magic_Tmp1:
	cmp eax, 0BAADF00Dh
	jnz FD_Heap_Magic_Tmp2
	inc ecx		
	FD_Heap_Magic_Tmp2:
	cmp eax, 0FEEEFEEEh
	jnz FD_Heap_Magic_Tmp3
	inc ecx
	FD_Heap_Magic_Tmp3:
	sub edx, 04h
	jnz FD_Heap_Magic_Loop
	push ecx	
	;; free heap
	push esi
	push HEAP_NO_SERIALIZE
	push edi
	call dword ptr [ebx].xHeapFree
	pop ecx
	;; judge count
	cmp ecx, 10h
	jae Found_FD_Heap_Magic
	xor eax, eax
Exit_FD_Heap_Magic:	
	pop edi	
	pop esi
	pop edx
	pop ecx
	pop ebx
	assume ebx : nothing
	;retn 04h
	retn 0
Found_FD_Heap_Magic:
	mov eax, 1
	jmp Exit_FD_Heap_Magic
End_FD_Heap_Magic:


FD_CheckRemoteDebuggerPresent:
FD_CheckRemoteDebuggerPresent_Arg_Win32Api    equ 04h
    mov eax, dword ptr [esp+FD_CheckRemoteDebuggerPresent_Arg_Win32Api]
    assume eax : ptr WIN32APIBASE
    push esp
    push esp
    call dword ptr [eax].xGetCurrentProcess
    push eax
    call dword ptr [eax].xCheckRemoteDebuggerPresent
    pop esp
    assume eax : nothing
    retn 04h;
    retn 0
End_FD_CheckRemoteDebuggerPresent:


FD_NtQueryInfoProc_DbgPort:
FD_NtQueryInfoProc_DbgPort_Arg_Win32Api    equ 08h
FD_NtQueryInfoProc_DbgPort_StackSize    equ sizeof PROCESS_DEBUG_PORT_INFO
FD_NtQueryInfoProc_DbgPort_ProcessInfo  equ -(FD_NtQueryInfoProc_DbgPort_StackSize)
    push ebp
    mov ebp, esp
    sub esp, FD_NtQueryInfoProc_DbgPort_StackSize
    
    push ebx
    
    mov ebx, dword ptr [ebp+FD_NtQueryInfoProc_DbgPort_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    push NULL
    push sizeof PROCESS_DEBUG_PORT_INFO
    lea eax, [ebp+FD_NtQueryInfoProc_DbgPort_ProcessInfo]
    push eax
    push ProcessDebugPort
    call dword ptr [ebx].xGetCurrentProcess
    push eax
    call dword ptr [ebx].xZwQueryInformationProcess
    test eax, eax
    jnz FD_NtQueryInfoProc_DbgPort_Tmp1
    lea eax, [ebp+FD_NtQueryInfoProc_DbgPort_ProcessInfo]
    assume eax : ptr PROCESS_DEBUG_PORT_INFO
    mov eax, dword ptr [eax].DebugPort
    test eax, eax
    jnz Found_FD_NtQueryInfoProc_DbgPort
FD_NtQueryInfoProc_DbgPort_Tmp1:
	xor eax, eax
Exit_FD_NtQueryInfoProc_DbgPort:
    assume eax : nothing
    assume ebx : nothing
    
    pop ebx
    
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
Found_FD_NtQueryInfoProc_DbgPort:
    mov eax, 1
    jmp Exit_FD_NtQueryInfoProc_DbgPort         
End_FD_NtQueryInfoProc_DbgPort:


FD_NtQueryInfoProc_DbgObjHandle:
FD_NtQueryInfoProc_DbgObjHandle_Arg_Win32Api    equ 08h
FD_NtQueryInfoProc_DbgObjHandle_StackSize    	equ sizeof PROCESS_DEBUG_OBJECTHANDLE_INFO
FD_NtQueryInfoProc_DbgObjHandle_ProcessInfo 	equ -(FD_NtQueryInfoProc_DbgObjHandle_StackSize)
    push ebp
    mov ebp, esp
    sub esp, FD_NtQueryInfoProc_DbgObjHandle_StackSize
    
    push ebx
    mov ebx, dword ptr [ebp+FD_NtQueryInfoProc_DbgObjHandle_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    push NULL
    push sizeof PROCESS_DEBUG_OBJECTHANDLE_INFO
    lea eax, [ebp+FD_NtQueryInfoProc_DbgObjHandle_ProcessInfo]
    push eax    
    push SystemNotImplemented8
    call dword ptr [ebx].xGetCurrentProcess
    push eax
    call dword ptr [ebx].xZwQueryInformationProcess
    test eax, eax
    jnz FD_NtQueryInfoProc_DbgObjHandle_Tmp1
    lea eax, [ebp+FD_NtQueryInfoProc_DbgObjHandle_ProcessInfo]
    assume eax : ptr PROCESS_DEBUG_OBJECTHANDLE_INFO
    mov eax, dword ptr [eax].ObjectHandle
    test eax, eax
    jnz Found_FD_NtQueryInfoProc_DbgObjHandle
FD_NtQueryInfoProc_DbgObjHandle_Tmp1:
	xor eax, eax
Exit_FD_NtQueryInfoProc_DbgObjHandle:

    assume eax : nothing
    assume ebx : nothing
    
    pop ebx
    
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
Found_FD_NtQueryInfoProc_DbgObjHandle:
    mov eax, 1
    jmp Exit_FD_NtQueryInfoProc_DbgObjHandle
End_FD_NtQueryInfoProc_DbgObjHandle:


FD_NtQueryInfoProc_DbgFlags:
FD_NtQueryInfoProc_DbgFlags_Arg_Win32Api    equ 08h
FD_NtQueryInfoProc_DbgFlags_StackSize    equ sizeof PROCESS_DEBUG_FLAGS_INFO
FD_NtQueryInfoProc_DbgFlags_ProcessInfo  equ -(FD_NtQueryInfoProc_DbgFlags_StackSize)
    push ebp
    mov ebp, esp
    sub esp, FD_NtQueryInfoProc_DbgFlags_StackSize
    
    push ebx
    
    mov ebx, dword ptr [ebp+FD_NtQueryInfoProc_DbgFlags_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    push NULL
    push sizeof PROCESS_DEBUG_FLAGS_INFO
    lea eax, [ebp+FD_NtQueryInfoProc_DbgFlags_ProcessInfo]
    push eax    
    push SystemNotImplemented9
    call dword ptr [ebx].xGetCurrentProcess
    push eax
    call dword ptr [ebx].xZwQueryInformationProcess
    test eax, eax
    jnz FD_NtQueryInfoProc_DbgFlags_Tmp1
    lea eax, [ebp+FD_NtQueryInfoProc_DbgFlags_ProcessInfo]
    assume eax : ptr PROCESS_DEBUG_FLAGS_INFO
    mov eax, dword ptr [eax].DebugFlags
    test eax, eax
    jz Found_FD_NtQueryInfoProc_DbgFlags
FD_NtQueryInfoProc_DbgFlags_Tmp1: 
    xor eax, eax
Exit_FD_NtQueryInfoProc_DbgFlags:
    assume eax : nothing
    assume ebx : nothing
    
    pop ebx
    
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
Found_FD_NtQueryInfoProc_DbgFlags:
    mov eax, 1
    jmp Exit_FD_NtQueryInfoProc_DbgFlags
End_FD_NtQueryInfoProc_DbgFlags:


FD_NtQueryInfoProc_SysKrlDbgInfo:
FD_NtQueryInfoProc_SysKrlDbgInfo_Arg_Win32Api    equ 08h
FD_NtQueryInfoProc_SysKrlDbgInfo_StackSize      equ sizeof PROCESS_DEBUG_FLAGS_INFO
FD_NtQueryInfoProc_SysKrlDbgInfo_Info           equ -(sizeof PROCESS_DEBUG_FLAGS_INFO)
    push ebp
    mov ebp, esp
    sub esp, FD_NtQueryInfoProc_SysKrlDbgInfo_StackSize
    
    push ebx
    
    mov ebx, dword ptr [ebp+FD_NtQueryInfoProc_SysKrlDbgInfo_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    push NULL
    push sizeof PROCESS_DEBUG_FLAGS_INFO
    lea eax, [ebp+FD_NtQueryInfoProc_SysKrlDbgInfo_Info]
    push eax    
    push SystemKernelDebuggerInformation
    call dword ptr [ebx].xGetCurrentProcess
    push eax
    call dword ptr [ebx].xZwQuerySystemInformation
    test eax, eax
    jnz FD_NtQueryInfoProc_SysKrlDbgInfo_Tmp1
    lea eax, [ebp+FD_NtQueryInfoProc_SysKrlDbgInfo_Info]
    assume eax : ptr PROCESS_DEBUG_FLAGS_INFO
    mov eax, dword ptr [eax].DebugFlags
    test eax, eax
    jz Found_FD_NtQueryInfoProc_SysKrlDbgInfo
FD_NtQueryInfoProc_SysKrlDbgInfo_Tmp1:    
    xor eax, eax
Exit_FD_NtQueryInfoProc_SysKrlDbgInfo:
    assume eax : nothing
    assume ebx : nothing
    
    pop ebx
    
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
Found_FD_NtQueryInfoProc_SysKrlDbgInfo:
    mov eax, 1
    jmp Exit_FD_NtQueryInfoProc_SysKrlDbgInfo
End_FD_NtQueryInfoProc_SysKrlDbgInfo:

FD_SeDebugPrivilege:
FD_SeDebugPrivilege_Arg_Win32Api    equ 08h
FD_SeDebugPrivilege_StackSize       equ 10h + sizeof PROCESSENTRY32
FD_SeDebugPrivilege_hProcessSnap    equ -04h
FD_SeDebugPrivilege_PID_csrss       equ -08h
FD_SeDebugPrivilege_FingFlag        equ -0ch
FD_SeDebugPrivilege_pe32            equ -(10h+sizeof PROCESSENTRY32)
    push ebp
    mov ebp, esp
    sub esp, FD_SeDebugPrivilege_StackSize
    
    push ebx
    push ecx
    push edi
    
    ;; clear stack
    lea edi, [ebp-FD_SeDebugPrivilege_StackSize]
    mov ecx, FD_SeDebugPrivilege_StackSize
    xor eax, eax
    cld
    rep stosb
    
    mov ebx, dword ptr [ebp+FD_SeDebugPrivilege_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    lea edi, [ebp+FD_SeDebugPrivilege_pe32]
    assume edi : ptr PROCESSENTRY32
                     
    push 0
    push TH32CS_SNAPPROCESS
    call dword ptr [ebx].xCreateToolhelp32Snapshot
    cmp eax, INVALID_HANDLE_VALUE
    jz NotFound_FD_SeDebugPrivilege
    mov dword ptr [ebp+FD_SeDebugPrivilege_hProcessSnap], eax
    push sizeof PROCESSENTRY32
    pop dword ptr [edi].dwSize
    
    push edi
    push dword ptr [ebp+FD_SeDebugPrivilege_hProcessSnap]
    call dword ptr [ebx].xProcess32First
    test eax, eax
    jnz FD_SeDebugPrivilege_Loop
    push dword ptr [ebp+FD_SeDebugPrivilege_hProcessSnap]
    call dword ptr [ebx].xCloseHandle
    jmp NotFound_FD_SeDebugPrivilege
    
    FD_SeDebugPrivilege_Loop:
    call FD_SeDebugPrivilege_Str
        db 'CSRSS.EXE',0
    FD_SeDebugPrivilege_Str:
    lea eax, [edi].szExeFile
    push eax    
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jnz FD_SeDebugPrivilege_Tmp2
    push dword ptr [edi].th32ProcessID
    pop dword ptr [ebp+FD_SeDebugPrivilege_PID_csrss]
    push TRUE
    pop dword ptr [ebp+FD_SeDebugPrivilege_FingFlag]
    FD_SeDebugPrivilege_Tmp2:
    mov eax, dword ptr [ebp+FD_SeDebugPrivilege_FingFlag]
    test eax, eax
    jnz FD_SeDebugPrivilege_Tmp3
    push edi
    push dword ptr [ebp+FD_SeDebugPrivilege_hProcessSnap]
    call dword ptr [ebx].xProcess32Next
    test eax, eax
    jnz FD_SeDebugPrivilege_Loop
    
    FD_SeDebugPrivilege_Tmp3:
    mov eax, dword ptr [ebp+FD_SeDebugPrivilege_FingFlag]
    test eax, eax
    jz FD_SeDebugPrivilege_Tmp4
    push dword ptr [ebp+FD_SeDebugPrivilege_PID_csrss]
    push FALSE
    push PROCESS_QUERY_INFORMATION
    call dword ptr [ebx].xOpenProcess
    test eax, eax
    jz FD_SeDebugPrivilege_Tmp4
    push dword ptr [ebp+FD_SeDebugPrivilege_hProcessSnap]
    call dword ptr [ebx].xCloseHandle
    jmp Found_FD_SeDebugPrivilege
    FD_SeDebugPrivilege_Tmp4:
    push dword ptr [ebp+FD_SeDebugPrivilege_hProcessSnap]
    call dword ptr [ebx].xCloseHandle
    jmp NotFound_FD_SeDebugPrivilege
Exit_FD_SeDebugPrivilege:
	pop edi
	pop ecx
	pop ebx
    assume ebx : nothing
    assume edi : nothing
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
NotFound_FD_SeDebugPrivilege:
    xor eax, eax
    jmp Exit_FD_SeDebugPrivilege
Found_FD_SeDebugPrivilege:
    mov eax, 1
    jmp Exit_FD_SeDebugPrivilege
End_FD_SeDebugPrivilege:

FD_Parent_Process:
FD_Parent_Process_Arg_Win32Api      equ 08h
FD_Parent_Process_StackSize         equ MAX_PATH + sizeof PROCESSENTRY32 + sizeof MODULEENTRY32 + 20h
FD_Parent_Process_hParnet           equ -04h
FD_Parent_Process_PIDExplorer       equ -08h
FD_Parent_Process_PIDParent         equ -0ch
FD_Parent_Process_PIDChild          equ -10h
FD_Parent_Process_hSnapshot         equ -14h
FD_Parent_Process_pe32              equ -(20h + PROCESSENTRY32)
FD_Parent_Process_me32              equ -(20h + PROCESSENTRY32 + MODULEENTRY32)
FD_Parent_Process_lpszSystemInfo    equ -(20h + PROCESSENTRY32 + MODULEENTRY32 + MAX_PATH)
    push ebp
    mov ebp, esp
    sub esp, FD_Parent_Process_StackSize
    
    push ebx
    push ecx
    push edi
    push esi
    
    ;; clear the stack
	lea edi, [ebp-FD_Parent_Process_StackSize]
    xor eax, eax
    mov ecx, FD_Parent_Process_StackSize
    cld
    rep stosb
    
    mov ebx, dword ptr [ebp+FD_Parent_Process_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    lea eax, [ebp+FD_Parent_Process_pe32]
    assume eax : ptr PROCESSENTRY32
    push sizeof PROCESSENTRY32
    pop dword ptr [eax].dwSize
    
    call dword ptr [ebx].xGetCurrentProcessId
    mov dword ptr [ebp+FD_Parent_Process_PIDChild], eax
    
    push 0
    push TH32CS_SNAPPROCESS
    call dword ptr [ebx].xCreateToolhelp32Snapshot
    mov dword ptr [ebp+FD_Parent_Process_hSnapshot], eax
    
    lea eax, [ebp+FD_Parent_Process_pe32]
    push eax
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xProcess32First
    test eax, eax
    jz FD_Parent_Process_Tmp1
    FD_Parent_Process_Loop1:
    lea eax, [ebp+FD_Parent_Process_pe32]
    push eax
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xProcess32Next
    test eax, eax
    jz FD_Parent_Process_Tmp2
    call FD_Parent_Process_Str1
        db "EXPLORER.EXE",0
    FD_Parent_Process_Str1:
    lea eax, [ebp+FD_Parent_Process_pe32]
    lea eax, [eax].szExeFile    
    push eax
    call dword ptr [ebx].xlstrcmpiA
    jnz FD_Parent_Process_Tmp3
    mov eax, dword ptr [ebp+FD_Parent_Process_PIDExplorer]
    test eax, eax
    jnz FD_Parent_Process_Tmp3
    lea eax, [ebp+FD_Parent_Process_pe32]
    assume eax : ptr PROCESSENTRY32
    push dword ptr [eax].th32ProcessID
    pop dword ptr [ebp+FD_Parent_Process_PIDExplorer]
    FD_Parent_Process_Tmp3:
    lea eax, [ebp+FD_Parent_Process_pe32]
    mov eax, dword ptr [eax].th32ProcessID
    sub eax, dword ptr [ebp+FD_Parent_Process_PIDChild]
    jnz FD_Parent_Process_Tmp4
    lea eax, [ebp+FD_Parent_Process_pe32]
    push dword ptr [eax].th32ParentProcessID
    pop dword ptr [ebp+FD_Parent_Process_PIDParent]
    FD_Parent_Process_Tmp4:
    jmp FD_Parent_Process_Loop1
    FD_Parent_Process_Tmp1:
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xCloseHandle
    jmp NotFound_FD_Parent_Process
    FD_Parent_Process_Tmp2:
    mov eax, dword ptr [ebp+FD_Parent_Process_PIDExplorer]
    sub eax, dword ptr [ebp+FD_Parent_Process_PIDParent]
    jz FD_Parent_Process_Tmp5
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xCloseHandle
    jmp Found_FD_Parent_Process
    FD_Parent_Process_Tmp5:
    lea eax, [ebp+FD_Parent_Process_me32]
    assume eax : ptr MODULEENTRY32
    push sizeof MODULEENTRY32
    pop dword ptr [eax].dwSize  
    push dword ptr [ebp+FD_Parent_Process_PIDExplorer]
    push TH32CS_SNAPMODULE
    call dword ptr [ebx].xCreateToolhelp32Snapshot
    mov dword ptr [ebp+FD_Parent_Process_hSnapshot], eax
    lea eax, [ebp+FD_Parent_Process_me32]
    push eax
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xModule32First
    test eax, eax
    jz FD_Parent_Process_Tmp6
    FD_Parent_Process_Loop2:
    lea eax, [ebp+FD_Parent_Process_me32]
    mov eax, dword ptr [eax].th32ProcessID
    sub eax, dword ptr [ebp+FD_Parent_Process_PIDExplorer]
    jnz FD_Parent_Process_Tmp7
    push MAX_PATH
    lea eax, [ebp+FD_Parent_Process_lpszSystemInfo]
    push eax
    call dword ptr [ebx].xGetWindowsDirectoryA
    call FD_Parent_Process_Str2
        db '\',0
    FD_Parent_Process_Str2:
    lea eax, [ebp+FD_Parent_Process_lpszSystemInfo]
    push eax
    call dword ptr [ebx].xlstrcatA
    call FD_Parent_Process_Str3
        db "EXPLORER.EXE",0
    FD_Parent_Process_Str3:
    lea eax, [ebp+FD_Parent_Process_lpszSystemInfo]
    push eax
    call dword ptr [ebx].xlstrcatA
    lea eax, [ebp+FD_Parent_Process_lpszSystemInfo]
    push eax
    lea eax, [ebp+FD_Parent_Process_me32]
    lea eax, [eax].szExePath
    push eax
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz FD_Parent_Process_Tmp6
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xCloseHandle
    jmp Found_FD_Parent_Process
    FD_Parent_Process_Tmp7:
    lea eax, [ebp+FD_Parent_Process_me32]
    push eax
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xModule32Next
    test eax, eax
    jnz FD_Parent_Process_Loop2
    FD_Parent_Process_Tmp6:
    push dword ptr [ebp+FD_Parent_Process_hSnapshot]
    call dword ptr [ebx].xCloseHandle
    jmp NotFound_FD_Parent_Process
Exit_FD_Parent_Process:
    pop esi
    pop edi
    pop ecx
    pop ebx
	assume eax : nothing
    assume ebx : nothing           
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
NotFound_FD_Parent_Process:
    xor eax, eax
    jmp Exit_FD_Parent_Process
Found_FD_Parent_Process:
    mov eax, 1
    jmp Exit_FD_Parent_Process    
End_FD_Parent_Process:


FD_DebugObject_NtQueryObject:
FD_DebugObject_NtQueryObject_Arg_Win32Api		equ 04h
	mov eax, dword ptr [esp+FD_DebugObject_NtQueryObject_Arg_Win32Api]
		
	push ebx
	push ecx
	push edx
	push edi
	push esi
    
    mov ebx, eax
    assume ebx : ptr WIN32APIBASE
    push edx	; alloc the stack
    
    push esp	; ReturnLength
    push 0
    push 0
    push ObjectAllTypeInformation
    push 0
    call dword ptr [ebx].xNtQueryObject
    pop ecx
	;; make a tmp stack
	push ebp
	mov ebp, esp
	sub esp, ecx
	mov esi, esp
    ;; ObjectInformationLength
    push 0
    push ecx
    push esi
    push ObjectAllTypeInformation
    push 0
    call dword ptr [ebx].xNtQueryObject
    cld
    ;; NumberOfObjectsTypes
    lodsd
    xchg ecx, eax ; ecx = NumberOfObjectsTypes
    FD_DebugObject_NtQueryObject_Loop:    
    ;; load string lengths
    lodsd
    movzx edx, ax
    ;; pointer to TypeName
    lodsd
    xchg esi, eax
    ;; sizeof(L"DebugObject")
    ;; avoids superstrings
    ;; like "DebugObjective"
    cmp edx, 16h
    jnz FD_DebugObject_NtQueryObject_Tmp2
    xchg ecx, edx
    FD_DebugObject_NtQueryObject_Tmp1:
    call FD_DebugObject_NtQueryObject_UnicodeStr1
    	dw 'D','e','b','u','g'
    	dw 'O','b','j','e','c','t'
    FD_DebugObject_NtQueryObject_UnicodeStr1:
    pop edi
    repe cmpsb
    xchg ecx, edx
    jnz FD_DebugObject_NtQueryObject_Tmp2
    ;; TotalNumberOfObjects
    cmp dword ptr [eax], edx
    jnz Found_FD_DebugObject_NtQueryObject
    ;; point to trailing  null
    FD_DebugObject_NtQueryObject_Tmp2:
    add esi, edx
    ;; round down to dword
    and esi, -4
    ;; skip trailing null
    ;; and any alignment bytes
    lodsd
    loop FD_DebugObject_NtQueryObject_Loop
    xor eax, eax
Exit_FD_DebugObject_NtQueryObject:
	;; clear the tmp stack
	mov esp, ebp
	pop ebp
	
	pop esi
	pop edi
	pop edx
	pop ecx
	pop ebx
	assume ebx : nothing    
    ;retn 04h
    retn 0
Found_FD_DebugObject_NtQueryObject:    
	mov eax, 1
	jmp Exit_FD_DebugObject_NtQueryObject
End_FD_DebugObject_NtQueryObject:


FD_Find_Debugger_Window:
FD_Find_Debugger_Window_Arg_WinApi32   equ 08h
    push ebp
    mov ebp, esp
    
    push ebx
    
    mov ebx, dword ptr [ebp+FD_Find_Debugger_Window_Arg_WinApi32]
    assume ebx : ptr WIN32APIBASE
    
    push NULL
    call FD_Find_Debugger_Window_Str1
        db "1212121",0
    FD_Find_Debugger_Window_Str1:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window
    
    push NULL
    call FD_Find_Debugger_Window_Str2
        db "icu_dbg",0
    FD_Find_Debugger_Window_Str2:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window    
    
    push NULL
    call FD_Find_Debugger_Window_Str3
        db "pe--diy",0
    FD_Find_Debugger_Window_Str3:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window     
    
    push NULL
    call FD_Find_Debugger_Window_Str5
        db "ollydbg",0
    FD_Find_Debugger_Window_Str5:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window 

    push NULL
    call FD_Find_Debugger_Window_Str6
        db "odbydyk",0
    FD_Find_Debugger_Window_Str6:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window

    push NULL
    call FD_Find_Debugger_Window_Str7
        db "WinDbgFrameClass",0
    FD_Find_Debugger_Window_Str7:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window

    push NULL
    call FD_Find_Debugger_Window_Str8
        db "TDeDeMainForm",0
    FD_Find_Debugger_Window_Str8:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window

    push NULL
    call FD_Find_Debugger_Window_Str9
        db "TIdaWindow",0
    FD_Find_Debugger_Window_Str9:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window

    push NULL
    call FD_Find_Debugger_Window_StrA
        db "TESTDBG",0
    FD_Find_Debugger_Window_StrA:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window

    push NULL
    call FD_Find_Debugger_Window_StrB
        db "kk1",0
    FD_Find_Debugger_Window_StrB:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window
    
    push NULL
    call FD_Find_Debugger_Window_StrC
        db "Eew75",0
    FD_Find_Debugger_Window_StrC:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window  
    
    push NULL
    call FD_Find_Debugger_Window_StrD
        db "Shadow",0
    FD_Find_Debugger_Window_StrD:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window       
   
    push NULL
    call FD_Find_Debugger_Window_StrE
        db "PEiD v0.94",0
    FD_Find_Debugger_Window_StrE:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window    
   
    push NULL
    call FD_Find_Debugger_Window_StrF
        db "Registry Monitor - Sysinternals: www.sysinternals.com",0
    FD_Find_Debugger_Window_StrF:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window    
 
    push NULL
    call FD_Find_Debugger_Window_Str10
        db "File Monitor - Sysinternals: www.sysinternals.com",0
    FD_Find_Debugger_Window_Str10:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window    

    push NULL
    call FD_Find_Debugger_Window_Str11
        db "Import REConstructor v1.6 FINAL (C) 2001-2003 MackT/uCF",0
    FD_Find_Debugger_Window_Str11:
    call dword ptr [ebx].xFindWindowA
    test eax, eax
    jnz Found_FD_Find_Debugger_Window
    jmp NotFound_Found_FD_Find_Debugger_Window
Exit_FD_Find_Debugger_Window:
    pop ebx
    assume ebx : nothing
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
NotFound_Found_FD_Find_Debugger_Window:
    xor eax, eax
    jmp Exit_FD_Find_Debugger_Window
Found_FD_Find_Debugger_Window:
    mov eax, 1
    jmp Exit_FD_Find_Debugger_Window    
End_FD_Find_Debugger_Window:


FD_Find_Debugger_Process:
FD_Find_Debugger_Process_Arg_Win32Api   equ 08h
FD_Find_Debugger_Process_Arg_FuncAddr   equ 0ch
FD_Find_Debugger_Process_StackSize      equ 08h + sizeof PROCESSENTRY32
FD_Find_Debugger_Process_hSnapshot      equ -04h
FD_Find_Debugger_Process_hParnet        equ -08h
FD_Find_Debugger_Process_pe32           equ -(08 + sizeof PROCESSENTRY32)

    push ebp
    mov ebp, esp
    sub esp, FD_Find_Debugger_Process_StackSize
    
    push esi
    push edx
    push ebx
    mov ebx, dword ptr [ebp+FD_Find_Debugger_Process_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    lea esi, [ebp+FD_Find_Debugger_Process_pe32]
    assume esi : ptr PROCESSENTRY32
    push sizeof PROCESSENTRY32
    pop dword ptr [esi].dwSize
    push 0
    push TH32CS_SNAPPROCESS
    call dword ptr [ebx].xCreateToolhelp32Snapshot
    mov dword ptr [ebp+FD_Find_Debugger_Process_hSnapshot], eax
    lea eax, [ebp+FD_Find_Debugger_Process_pe32]
    push eax
    push dword ptr [ebp+FD_Find_Debugger_Process_hSnapshot]
    call dword ptr [ebx].xProcess32First
    test eax, eax
    jz NotFound_FD_Find_Debugger_Process
    FD_Find_Debugger_Process_Loop:
    lea eax, [esi].szExeFile
    push eax
    mov eax, dword ptr [ebp+FD_Find_Debugger_Process_Arg_FuncAddr]
    assume eax : ptr FUNCADDRTBL
    call dword ptr [eax].xGetFileNameFromPath
    assume eax : nothing
    mov edx, eax
    call FD_Find_Debugger_Process_Str1
        db "OLLYICE.EXE",0
    FD_Find_Debugger_Process_Str1:
    push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process
    
    call FD_Find_Debugger_Process_Str2
        db "IDAG.EXE",0
    FD_Find_Debugger_Process_Str2:
    push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process 
    
    call FD_Find_Debugger_Process_Str3
        db "OLLYDBG.EXE",0
    FD_Find_Debugger_Process_Str3:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process    
    
    call FD_Find_Debugger_Process_Str4
        db "PEID.EXE",0
    FD_Find_Debugger_Process_Str4:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process    
    
    call FD_Find_Debugger_Process_Str5
        db "SOFTICE.EXE",0
    FD_Find_Debugger_Process_Str5:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process

    call FD_Find_Debugger_Process_Str6
        db "LORDPE.EXE",0
    FD_Find_Debugger_Process_Str6:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process

    call FD_Find_Debugger_Process_Str7
        db "IMPORTREC.EXE",0
    FD_Find_Debugger_Process_Str7:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process
    
    call FD_Find_Debugger_Process_Str8
        db "W32DSM89.EXE",0
    FD_Find_Debugger_Process_Str8:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process    
    
    call FD_Find_Debugger_Process_Str9
        db "WINDBG.EXE",0
    FD_Find_Debugger_Process_Str9:
	push edx
    call dword ptr [ebx].xlstrcmpiA
    test eax, eax
    jz Found_FD_Find_Debugger_Process
    lea eax, [ebp+FD_Find_Debugger_Process_pe32]
    push eax
    push dword ptr [ebp+FD_Find_Debugger_Process_hSnapshot]
    call dword ptr [ebx].xProcess32Next
    test eax, eax
    jnz FD_Find_Debugger_Process_Loop
    jmp NotFound_FD_Find_Debugger_Process
Exit_FD_Find_Debugger_Process:
    pop ebx
    pop edx
    pop esi
    assume ebx : nothing
    assume esi : nothing
    mov esp, ebp
    pop ebp 
    ;retn 08h
    retn 0
NotFound_FD_Find_Debugger_Process:
    xor eax, eax
    jmp Exit_FD_Find_Debugger_Process
Found_FD_Find_Debugger_Process:
    mov eax, 1
    jmp Exit_FD_Find_Debugger_Process
End_FD_Find_Debugger_Process:


FD_Find_Device_Driver:
FD_Find_Device_Driver_Arg_Win32Api      equ 08h

    push ebp
    mov ebp, esp
    
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    mov ebx, dword ptr [ebp+FD_Find_Device_Driver_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    ;; check softice on unknow system
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str1
        db "\\.\SIWVID",0
    FD_Find_Device_Driver_Str1:
    call dword ptr [ebx].xCreateFileA
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver
    
    ;; check softice 4.05 on win2k
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str2
        db "\\.\NTICE",0
    FD_Find_Device_Driver_Str2:
    call dword ptr [ebx].xCreateFileA
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver
    
    ;; check softice on win9x
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str3
        db "\\.\SICE",0
    FD_Find_Device_Driver_Str3:
    call dword ptr [ebx].xCreateFileA
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver    
    
    ;; check softice on win9x
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str4
        db "\\.\SIWDEBUG",0
    FD_Find_Device_Driver_Str4:
    call dword ptr [ebx].xCreateFileA
    push eax
    call dword ptr [ebx].xGetLastError
    test al, 032h
    pop eax
    jz Found_FD_Find_Device_Driver
    
    ;; check regmon on win9x
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str5
        db "\\.\REGVXD",0
    FD_Find_Device_Driver_Str5:
    call dword ptr [ebx].xCreateFileA    
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver
    
    ;; check RegMON
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str6
        db "\\.\FILEM",0
    FD_Find_Device_Driver_Str6:
    call dword ptr [ebx].xCreateFileA      
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver
    
    ;; check TRW
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str7
        db "\\.\TRW",0
    FD_Find_Device_Driver_Str7:
    call dword ptr [ebx].xCreateFileA      
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver       
    
    ;; check softice extender
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ + FILE_SHARE_WRITE
    push GENERIC_READ + GENERIC_WRITE
    call FD_Find_Device_Driver_Str8
        db "\\.\ICEEXT",0
    FD_Find_Device_Driver_Str8:
    call dword ptr [ebx].xCreateFileA      
    cmp eax, INVALID_HANDLE_VALUE
    jnz Found_FD_Find_Device_Driver 
    jmp NotFound_FD_Find_Device_Driver
    
Exit_FD_Find_Device_Driver:    
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    assume ebx : nothing    
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
NotFound_FD_Find_Device_Driver:
    xor eax, eax
    jmp Exit_FD_Find_Device_Driver
Found_FD_Find_Device_Driver:
    push eax
    assume ebx : ptr WIN32APIBASE
    call dword ptr [ebx].xCloseHandle
    assume ebx : nothing
    mov eax, 1
    jmp Exit_FD_Find_Device_Driver
End_FD_Find_Device_Driver:


FD_Exception_Int3:
    call Get_FD_Exception_Int3_Eip
    Get_FD_Exception_Int3_Eip:
    pop eax
    add eax, offset FD_Exception_Int3_Exception - offset Get_FD_Exception_Int3_Eip
    ;; setup exception
    assume fs : nothing
    push eax
    push dword ptr fs : [0]
    mov dword ptr fs : [0], esp
    ;; reset eax
    xor eax, eax
    int 03h
    ;; unsetup exception
    pop dword ptr fs : [0]
    add esp, 04h
    
    ;; check the flag
    test eax, eax
    jz Found_FD_Exception_Int3
    jmp NotFound_FD_Exception_Int3
FD_Exception_Int3_Exception:
    mov eax, dword ptr [esp+0ch]
    ;; eax = ContextRecord
    assume eax : ptr CONTEXT
    mov dword ptr [eax].regEax, 0FFFFFFFFh
    inc dword ptr [eax].regEip
    xor eax, eax
    assume eax : nothing
    retn
Exit_FD_Exception_Int3:
    retn 0h
NotFound_FD_Exception_Int3:
    xor eax, eax
    jmp Exit_FD_Exception_Int3
Found_FD_Exception_Int3:
    mov eax, 1
    jmp Exit_FD_Exception_Int3        
End_FD_Exception_Int3:

FD_Exception_Popf:
    push ebx

    call Get_FD_Exception_Popf_Eip
    Get_FD_Exception_Popf_Eip:
    pop eax
    add eax, offset FD_Exception_Popf_Exception - offset Get_FD_Exception_Popf_Eip
    assume fs : nothing    
    push eax
    push dword ptr fs:[0]
    mov dword ptr fs:[0], esp
    ;; reset eax
    xor eax, eax
    pushf
    mov dword ptr [esp], 0100h
    popf
    nop
FD_Exception_Popf_NextEip:
    pop dword ptr fs:[0]
    add esp, 04h
    
    ;; check the flag
    test eax, eax
    jz Found_FD_Exception_Popf
    jmp NotFound_FD_Exception_Popf
    
FD_Exception_Popf_Exception:
    mov eax, dword ptr [esp+0ch]
    assume eax : ptr CONTEXT
    mov dword ptr [eax].regEax, 0FFFFFFFFh
    call Get_FD_Exception_Popf_Exception_Eip2
    Get_FD_Exception_Popf_Exception_Eip2:
    pop ebx
    sub ebx, offset Get_FD_Exception_Popf_Exception_Eip2 - offset FD_Exception_Popf_NextEip
    mov dword ptr [eax].regEip, ebx
    xor eax, eax
    retn
    assume eax : nothing
Exit_FD_Exception_Popf:
    pop ebx
    retn 0h
NotFound_FD_Exception_Popf:
    xor eax, eax
    jmp Exit_FD_Exception_Popf
Found_FD_Exception_Popf:
    mov eax, 1
    jmp Exit_FD_Exception_Popf
End_FD_Exception_Popf:


FD_OutputDebugString:
FD_OutputDebugString_Arg_Win32Api       equ 04h
    
    mov eax, dword ptr [esp+FD_OutputDebugString_Arg_Win32Api]
    push ebx
    mov ebx, eax
    assume ebx : ptr WIN32APIBASE
    call FD_OutputDebugString_Str
        db 0
    FD_OutputDebugString_Str:
    call dword ptr [ebx].xOutputDebugStringA
    call dword ptr [ebx].xGetLastError 
    test eax, eax   
    jnz NotFound_FD_OutputDebugString
    inc eax
Exit_FD_OutputDebugString:    
    pop ebx
	assume ebx : nothing
    ;retn 04h
    retn 0
NotFound_FD_OutputDebugString:
	pop ebx
    xor eax, eax
	jmp Exit_FD_OutputDebugString
End_FD_OutputDebugString:


;; find specic debugger
FS_OD_Exception_GuardPages:
FS_OD_Exception_GuardPages_Arg_Win32Api     equ 04h

    mov eax, dword ptr [esp+FS_OD_Exception_GuardPages_Arg_Win32Api]
    
    push edi    ; pAllocatedMem
    push ebx
    
    mov ebx, eax
    assume ebx : ptr WIN32APIBASE
     
    push PAGE_READWRITE
    push MEM_COMMIT
    push 01000h
    push NULL
    call dword ptr [ebx].xVirtualAlloc
    test eax, eax
    jz NotFound_FS_OD_Exception_GuardPages
    mov edi, eax
    ;; store a RENT on the allocated memory
    mov byte ptr [edi], 0c3h
    push edi		; alloc stack
    
    push esp
    push PAGE_EXECUTE_READ + PAGE_GUARD
    push 01000h
    push edi
    call dword ptr [ebx].xVirtualProtect
    ;; set eax to 0
    xor eax, eax
    ;; trigger a STATUS_GUARD_PAGE_VIOLATION exception
    ;; setup expcetion
    call Get_FS_OD_Exception_GuardPages_Eip
    Get_FS_OD_Exception_GuardPages_Eip:
    pop eax
    add eax, offset FS_OD_Exception_GuardPages_Exception - offset Get_FS_OD_Exception_GuardPages_Eip
    assume fs : nothing
    push eax
    push dword ptr fs:[0]
    mov dword ptr fs:[0], esp
    call edi
FS_OD_Exception_GuardPages_Continue:
    pop edi
    ;; unsetup exception
    pop dword ptr fs:[0]
    add esp, 04h
    test eax, eax
    jz Found_FS_OD_Exception_GuardPages
    jmp NotFound_FS_OD_Exception_GuardPages
FS_OD_Exception_GuardPages_Exception:
    mov eax, dword ptr [esp+0ch]
    assume eax : ptr CONTEXT
    mov dword ptr [eax].regEax, 0FFFFFFFFh
    call Get_FS_OD_Exception_GuardPages_Exception_Eip
    Get_FS_OD_Exception_GuardPages_Exception_Eip:
    pop ebx
    sub ebx, offset Get_FS_OD_Exception_GuardPages_Exception_Eip - offset FS_OD_Exception_GuardPages_Continue  
    mov dword ptr [eax].regEip, ebx
    assume eax : nothing
    xor eax, eax
    retn
    
Exit_FS_OD_Exception_GuardPages:
    pop ebx
    pop edi
    assume ebx : nothing
    ;retn 04h
    retn 0
    
NotFound_FS_OD_Exception_GuardPages:
	assume ebx : ptr WIN32APIBASE
    push MEM_DECOMMIT
	push 01000h
	push edi
	call dword ptr [ebx].xVirtualFree
	assume ebx : nothing
    xor eax, eax
    jmp Exit_FS_OD_Exception_GuardPages
Found_FS_OD_Exception_GuardPages:
	assume ebx : ptr WIN32APIBASE
    push MEM_DECOMMIT
	push 01000h
	push edi
	call dword ptr [ebx].xVirtualFree
	assume ebx : nothing
    mov eax, 1
    jmp Exit_FS_OD_Exception_GuardPages
End_FS_OD_Exception_GuardPages:


FS_SI_UnhandledExceptionFilter:
FS_SI_UnhandledExceptionFilter_Arg_Win32Api     equ 04h
    mov eax, dword ptr [esp+FS_SI_UnhandledExceptionFilter_Arg_Win32Api]
    assume eax : ptr WIN32APIBASE
    mov eax, dword ptr [eax].xUnhandledExceptionFilter
    mov al, byte ptr [eax]
    sub al, 0cch
    jz Found_FS_SI_UnhandledExceptionFilter
    xor eax, eax
Exit_FS_SI_UnhandledExceptionFilter:
    assume eax : nothing
    ;retn 04h
    retn 0
Found_FS_SI_UnhandledExceptionFilter:
    mov eax, 1
    jmp Exit_FS_SI_UnhandledExceptionFilter    
End_FS_SI_UnhandledExceptionFilter:

;; find OD plugin
FS_ODP_Process32NextW:
FS_ODP_Process32NextW_Arg_Win32Api          equ 04h
    mov eax, dword ptr [esp+FS_ODP_Process32NextW_Arg_Win32Api]
    assume eax : ptr WIN32APIBASE
    mov eax, dword ptr [eax].xProcess32NextW
    mov ax, word ptr [eax]
    sub ax, 0FF88h
    jnz Found_FS_ODP_Process32NextW
    xor eax, eax
Exit_FS_ODP_Process32NextW:
    assume eax : nothing 
    ;retn 04h
    retn 0
Found_FS_ODP_Process32NextW:
    mov eax, 1
    jmp Exit_FS_ODP_Process32NextW
End_FS_ODP_Process32NextW:


FS_ODP_OutputDebugStringA:
FS_ODP_OutputDebugStringA_Arg_Win32Api      equ 04h
    mov eax, dword ptr [ebp+FS_ODP_OutputDebugStringA_Arg_Win32Api]
    assume eax : ptr WIN32APIBASE
    mov eax, dword ptr [eax].xOutputDebugStringA
    mov ax, word ptr [eax]
    sub ax, 03468h
    jnz Found_FS_ODP_OutputDebugStringA
    xor eax, eax
Exit_FS_ODP_OutputDebugStringA:
    assume eax : nothing
    ;retn 04h
    retn 0
Found_FS_ODP_OutputDebugStringA:
    mov eax, 1
    jmp Exit_FS_ODP_OutputDebugStringA            
End_FS_ODP_OutputDebugStringA:

FS_ODP_OpenProcess:
FS_ODP_OpenProcess_Arg_Win32Api     equ 04h
    mov eax, dword ptr [ebp+FS_ODP_OpenProcess_Arg_Win32Api]
    assume eax : ptr WIN32APIBASE
    mov eax, dword ptr [eax].xOpenProcess
    mov al, byte ptr [eax+06h]
    ;; Hide Debugger Plugin of OD is present
    sub al, 0eah
    jnz Found_FS_ODP_OpenProcess
    xor eax, eax
Exit_FS_ODP_OpenProcess:
    assume eax : nothing
    ;retn 04h
    retn 0  
Found_FS_ODP_OpenProcess:
    mov eax, 1
    jmp Exit_FS_ODP_OpenProcess      
End_FS_ODP_OpenProcess:


;; find breakpoint
FB_HWBP_Exception:
    call Get_FB_HWBP_Exception_Eip
    Get_FB_HWBP_Exception_Eip:
    pop eax
    add eax, offset FB_HWBP_Exception_Exception - offset FB_HWBP_Exception
    assume fs : nothing
    push eax
    push fs:[0]
    mov dword ptr fs:[0], esp
    ;; reset eax
    xor eax, eax
    int 01h
    pop dword ptr fs:[0]
    add esp, 04h
    test eax, eax
    jnz Found_FB_HWBP_Exception
    jmp NotFound_FB_HWBP_Exception
FB_HWBP_Exception_Exception:
    mov eax, dword ptr [esp+0ch]
    assume eax : ptr CONTEXT
    ;; check if debug Registers Context.Dr0-Dr3 is not zero
    cmp dword ptr [eax].iDr0, 0
    jnz FB_HWBP_Exception_HardwareBp_Found
    cmp dword ptr [eax].iDr1, 0
    jnz FB_HWBP_Exception_HardwareBp_Found  
    cmp dword ptr [eax].iDr2, 0
    jnz FB_HWBP_Exception_HardwareBp_Found
    cmp dword ptr [eax].iDr3, 0
    jnz FB_HWBP_Exception_HardwareBp_Found
    jmp FB_HWBP_Exception_Exception_Ret     
FB_HWBP_Exception_HardwareBp_Found:
    ;; set Context.Eax to signal breakpoint found
    mov dword ptr [eax].regEax, 0FFFFFFFFh
FB_HWBP_Exception_Exception_Ret:
    ;; set Context.Eip upon return
    inc dword ptr [eax].regEip
    inc dword ptr [eax].regEip
    xor eax, eax
    retn
Exit_FB_HWBP_Exception:
    retn 0h
NotFound_FB_HWBP_Exception:
    xor eax, eax
    jmp Exit_FB_HWBP_Exception
Found_FB_HWBP_Exception:
    mov eax, 1
    jmp Exit_FB_HWBP_Exception                
End_FB_HWBP_Exception:


FB_SWBP_Memory_CRC:
FB_SWBP_Memory_CRC_Arg_Win32Api         equ 08h
FB_SWBP_Memory_CRC_Arg_FuncAddr         equ 0ch
FB_SWBP_Memory_CRC_StackSize            equ 10h+MAX_PATH
FB_SWBP_Memory_CRC_fileSize             equ -04h
FB_SWBP_Memory_CRC_NumberOfBytesRW      equ -08h
FB_SWBP_Memory_CRC_pBuffer              equ -0ch
FB_SWBP_Memory_CRC_szFileName           equ -(10h+MAX_PATH)
    push ebp
    mov ebp, esp
    sub esp, FB_SWBP_Memory_CRC_StackSize
    
    push esi
    push edi
    push ecx
    push edx
    push ebx
    
    
    ;; clear the stack
    lea edi, [ebp+FB_SWBP_Memory_CRC_szFileName]
    mov ecx, FB_SWBP_Memory_CRC_StackSize
    xor eax, eax
    cld
    rep stosb
	
    
    mov ebx, dword ptr [ebp+FB_SWBP_Memory_CRC_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    push MAX_PATH
    lea eax, [ebp+FB_SWBP_Memory_CRC_szFileName]
    push eax
    push NULL
    call dword ptr [ebx].xGetModuleFileNameA
    ;; open file
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ
    push GENERIC_READ
    lea eax, [ebp+FB_SWBP_Memory_CRC_szFileName]
    push eax
    call dword ptr [ebx].xCreateFileA
    cmp eax, INVALID_HANDLE_VALUE
    jz Error_FB_SWBP_Memory_CRC
    mov edi, eax
    push NULL
    push eax
    call dword ptr [ebx].xGetFileSize
;    cmp eax, INVALID_FILE_SIZE
;    jz Error_FB_SWBP_Memory_CRC    
    mov dword ptr [ebp+FB_SWBP_Memory_CRC_fileSize], eax
    push PAGE_READWRITE
    push MEM_COMMIT
    push eax
    push NULL
    call dword ptr [ebx].xVirtualAlloc
    mov dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer], eax
    push NULL
    lea eax, [ebp+FB_SWBP_Memory_CRC_NumberOfBytesRW]
    push eax
    push dword ptr [ebp+FB_SWBP_Memory_CRC_fileSize]
    push dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    push edi
    call dword ptr [ebx].xReadFile
    push edi
    call dword ptr [ebx].xCloseHandle
        
    mov esi, dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    add esi, dword ptr [esi+3ch]
    assume esi : ptr IMAGE_NT_HEADERS
    mov edx, dword ptr [esi].OptionalHeader.ImageBase
    mov eax, dword ptr [esi].OptionalHeader.AddressOfEntryPoint
    add eax, edx
    mov cx, word ptr [esi].FileHeader.NumberOfSections
    movzx ecx, cx
    mov edi, esi
    add edi, sizeof IMAGE_NT_HEADERS
    assume edi : ptr IMAGE_SECTION_HEADER
    ;; find the entry section
    ;; eax = AddressOfEntryPoint
    ;; ebx = Each Section Virtual Address
    ;; ecx = NumberOfSections    
    ;; edx = ImageBase
    ;; edi = Section Table Point
    FB_SWBP_Memory_CRC_Loop:
    mov ebx, dword ptr [edi].VirtualAddress
    add ebx, edx
    cmp ebx, eax
    jz FB_SWBP_Memory_CRC_Loop_End
    add edi, sizeof IMAGE_SECTION_HEADER
    dec ecx
    jnz FB_SWBP_Memory_CRC_Loop
    jmp Error_FB_SWBP_Memory_CRC
    FB_SWBP_Memory_CRC_Loop_End:
    mov edi, dword ptr [edi].Misc.VirtualSize
    mov esi, eax
    ;; alloc the memory
    push MEM_DECOMMIT
    push dword ptr [ebp+FB_SWBP_Memory_CRC_fileSize]
    push dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    mov ebx, dword ptr [ebp+FB_SWBP_Memory_CRC_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    call dword ptr [ebx].xVirtualFree
    mov edx, dword ptr [ebp+FB_SWBP_Memory_CRC_Arg_FuncAddr]
    assume edx : ptr FUNCADDRTBL
    push edi
    push esi
    call dword ptr [edx].xCRC32
    call Get_FB_SWBP_Memory_CRC_Eip
    Get_FB_SWBP_Memory_CRC_Eip:
    pop ecx
    add ecx, offset FB_SWBP_Memory_CRC_Orig_CRC - offset Get_FB_SWBP_Memory_CRC_Eip
    cmp eax, ecx
    jz NotFound_FB_SWBP_Memory_CRC
    jmp Found_FB_SWBP_Memory_CRC
Exit_FB_SWBP_Memory_CRC:
    pop ebx
    pop edx
    pop ecx
    pop edi
    pop esi
    assume ebx : nothing
    assume edx : nothing
    assume edi : nothing
    assume esi : nothing
    mov esp, ebp
    pop ebp
    ;retn 08h
    retn 0
Error_FB_SWBP_Memory_CRC:
    mov eax, dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    test eax, eax
    jz Error_FB_SWBP_Memory_CRC_Continue
    push MEM_DECOMMIT
    push dword ptr [ebp+FB_SWBP_Memory_CRC_fileSize]
    push dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    mov ebx, dword ptr [ebp+FB_SWBP_Memory_CRC_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE    
    call dword ptr [ebx].xVirtualFree
    assume ebx : nothing
    Error_FB_SWBP_Memory_CRC_Continue:
    xor eax, eax
    jmp Exit_FB_SWBP_Memory_CRC
Found_FB_SWBP_Memory_CRC:
    push MEM_DECOMMIT
    push dword ptr [ebp+FB_SWBP_Memory_CRC_fileSize]
    push dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    mov ebx, dword ptr [ebp+FB_SWBP_Memory_CRC_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE    
    call dword ptr [ebx].xVirtualFree
    assume ebx : nothing
	mov eax, 1
	jmp Exit_FB_SWBP_Memory_CRC
NotFound_FB_SWBP_Memory_CRC:
    push MEM_DECOMMIT
    push dword ptr [ebp+FB_SWBP_Memory_CRC_fileSize]
    push dword ptr [ebp+FB_SWBP_Memory_CRC_pBuffer]
    mov ebx, dword ptr [ebp+FB_SWBP_Memory_CRC_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE    
    call dword ptr [ebx].xVirtualFree
    assume ebx : nothing
	xor eax, eax
	jmp Exit_FB_SWBP_Memory_CRC	    
FB_SWBP_Memory_CRC_Orig_CRC:
	dd 0   
End_FB_SWBP_Memory_CRC:


;; find single step or trace
FT_PushSS_PopSS:
    ;; release version
    push ebp
    mov ebp, esp
    push ss
    pop ss
    pushf
    pop eax
    and eax, 00000100h
    jnz Found_FT_PushSS_PopSS
Exit_FT_PushSS_PopSS:
    xor eax, eax
    mov esp, ebp
    pop ebp
    retn 0h    
Found_FT_PushSS_PopSS:
    mov eax, 1
    jmp Exit_FT_PushSS_PopSS   
End_FT_PushSS_PopSS:


FT_RDTSC:
	push ecx
  	rdtsc
    xchg ecx, eax
    rdtsc
    sub eax, ecx
    cmp eax, 500h
    jnbe Found_FT_RDTSC
    xor eax, eax
Exit_FT_RDTSC:
	pop ecx
	retn 0h
Found_FT_RDTSC:
	mov eax, 1
	jmp Exit_FT_RDTSC
End_FT_RDTSC:


FT_GetTickCount:
FT_GetTickCount_Arg_Win32Api		equ 04h
	mov eax, dword ptr [esp+FT_GetTickCount_Arg_Win32Api]
	push ebx
	push ecx
	mov ebx, eax
	assume ebx : ptr WIN32APIBASE
	call dword ptr [ebx].xGetTickCount
	xchg ecx, eax
	call dword ptr [ebx].xGetTickCount
	sub eax, ebx
	cmp eax, 1
	jnb Found_FT_GetTickCount
	xor eax, eax
Exit_FT_GetTickCount:
	pop ecx
	pop ebx
	assume ebx : nothing
	;retn 04h
	retn 0	
Found_FT_GetTickCount:
	mov eax, 1
	jmp Exit_FT_GetTickCount	
End_FT_GetTickCount:

FT_timeGetTime:
FT_timeGetTime_Arg_Win32Api			equ 04h
	mov eax, dword ptr [esp+FT_timeGetTime_Arg_Win32Api]
	push ebx
	push ecx
	mov ebx, eax
	assume ebx : ptr WIN32APIBASE
	call dword ptr [ebx].xtimeGetTime
	xchg ecx, eax
	call dword ptr [ebx].xtimeGetTime
	sub eax, ebx
	cmp eax, 10h
	jnb Found_FT_timeGetTime
	xor eax, eax
Exit_FT_timeGetTime:
	pop ecx
	pop ebx
	assume ebx : nothing
	;retn 04h
	retn 0	
Found_FT_timeGetTime:
	mov eax, 1
	jmp Exit_FT_timeGetTime
End_FT_timeGetTime:

FT_SharedUserData_TickCount:
	push ecx
    push edx    
    mov edx, 07FFE0000h
    mov eax, dword ptr [edx]
    imul dword ptr [edx+04h]
    shrd eax, edx, 018h
    xchg eax, ecx
    mov edx, 07FFE0000h
    mov eax, dword ptr [edx]
    imul dword ptr [edx+04h]
    shrd eax, edx, 018h
    sub eax, ecx
   	cmp eax, 500h
   	jnbe Found_FT_SharedUserData_TickCount
   	xor eax, eax
Exit_FT_SharedUserData_TickCount:
    pop edx
    pop ecx
    retn 0h
Found_FT_SharedUserData_TickCount:
	mov eax, 1
	jmp Exit_FT_SharedUserData_TickCount    
End_FT_SharedUserData_TickCount:

FT_INT1_IceBreakpoint:
    push ebp
    mov ebp, esp
    call Get_FT_INT1_IceBreakpoint_Eip
    Get_FT_INT1_IceBreakpoint_Eip:
    pop eax
    add eax, offset FT_INT1_IceBreakpoint_Exception - offset Get_FT_INT1_IceBreakpoint_Eip
    push eax
    assume fs : nothing
    push dword ptr fs:[0]
    mov dword ptr fs:[0], esp
    ;; reset eax
    xor eax, eax
    db 0CDh, 01h	; int 1
    pop dword ptr fs:[0]
    add esp, 04h
    test eax, eax
    jz Found_FT_INT1_IceBreakpoint
    jmp NotFound_FT_INT1_IceBreakpoint
FT_INT1_IceBreakpoint_Exception:
    ;mov eax, dword ptr [esp+04h]
    ;cmp dword ptr [eax], 80000004h
    mov eax, dword ptr [esp+0ch]
    assume eax : ptr CONTEXT 
    mov dword ptr [eax].regEax, 1
    inc dword ptr [eax].regEip
    xor eax, eax
    retn
Exit_FT_INT1_IceBreakpoint:
    mov esp, ebp
    pop ebp
    retn 0h
NotFound_FT_INT1_IceBreakpoint:
    xor eax, eax
    jmp Exit_FT_INT1_IceBreakpoint
Found_FT_INT1_IceBreakpoint:
    mov eax, 1
    jmp Exit_FT_INT1_IceBreakpoint
End_FT_INT1_IceBreakpoint:

;; find VME
FV_VPC_Exception: 

    push ebx
    push ecx
   
    call Get_FV_VPC_Exception_Eip
    Get_FV_VPC_Exception_Eip:
    pop ecx
    add ecx, offset FV_VPC_Exception_Exception - offset Get_FV_VPC_Exception_Eip
    assume fs : nothing        
    push ecx
    push dword ptr fs:[0]
    mov dword ptr fs:[0], esp
    
    ;; Flag
    xor ebx, ebx
    ;; VPC function number
    mov eax, 1
    
    ;; call VPC
    db 0Fh,3Fh,07h,0Bh
    
    pop dword ptr fs:[0]
    add esp, 04h    
    test ebx, ebx
    setz al
    
    pop ecx
    pop ebx    
    retn 0h
    
FV_VPC_Exception_Exception:    
    mov ecx, dword ptr [esp+0ch]
    assume ecx : ptr CONTEXT
    ;; ebx = -1 : not running
    ;; ebx = 0 : running
    mov dword ptr [ecx].regEbx, -1
    ;; skip past the call to VPC
    add dword ptr [ecx].regEip, 04h
    xor eax, eax
    retn
End_FV_VPC_Exception:


FV_VMWare_VMX:
	
    call IsInsideVMWare
	retn 0h
IsInsideVMWare:

	push ebx
	push ecx
	push edx

	assume fs : nothing
	call Get_IsInsideVMWare_Eip
	Get_IsInsideVMWare_Eip:
	pop eax
	add eax, offset IsInsideVMWare_Exception - offset Get_IsInsideVMWare_Eip
	push eax
	push dword ptr fs:[0]
	mov dword ptr fs:[0], esp
	
    mov    eax, 'VMXh'
    mov    ebx, 0 		; any value but MAGIC VALUE
    mov    ecx, 10		; get VMWare version
    mov    edx, 'VX' 	; port number
    in     eax, dx 		; read port
                   		; on return EAX returns the VERSION
    cmp    ebx, 'VMXh' 	; is it a reply from VMWare?
    setz   al 			; set return value
    movzx eax, al

	pop dword ptr fs:[0]
	add esp, 04h
	
	pop edx
	pop ecx
	pop ebx
	
	retn 0h
IsInsideVMWare_Exception:
	mov eax, dword ptr [esp+0ch]
	assume eax : ptr CONTEXT
	inc dword ptr [eax].regEip
	retn       
End_IsInsideVMWare:
    
End_FV_VMWare_VMX:


FP_Check_FileSize:
FP_Check_FileSize_Arg_Win32Api      equ 08h
FP_Check_FileSize_StackSize         equ MAX_PATH
FP_Check_FileSize_szPath            equ -MAX_PATH
    push ebp
    mov ebp, esp
    sub esp, FP_Check_FileSize_StackSize
    
    push ebx
    push ecx
    push edx
    mov ebx, dword ptr [ebp+FP_Check_FileSize_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    push MAX_PATH
    lea eax, [ebp+FP_Check_FileSize_szPath]
    push eax
    push NULL
    call dword ptr [ebx].xGetModuleFileNameA
    test eax, eax
    jz NotFound_FP_Check_FileSize
    
    ;; create file
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_ALWAYS
    push NULL
    push FILE_SHARE_READ
    push GENERIC_READ
    lea eax, [ebp+FP_Check_FileSize_szPath]
    push eax
    call dword ptr [ebx].xCreateFileA
    cmp eax, INVALID_HANDLE_VALUE
    jz NotFound_FP_Check_FileSize
    mov ecx, eax
    
    push NULL
    push eax
    call dword ptr [ebx].xGetFileSize
    mov edx, eax
    
    push ecx
    call dword ptr [ebx].xCloseHandle
    
    call Get_FP_Check_FileSize_Eip
    Get_FP_Check_FileSize_Eip:
    pop eax
    add eax, offset FP_Check_FileSize_Orig_Size - offset Get_FP_Check_FileSize_Eip
    cmp edx, eax
    jnz Found_FP_Check_FileSize
    jmp NotFound_FP_Check_FileSize

Exit_FP_Check_FileSize:
    pop edx
    pop ecx
    pop ebx
    assume ebx : nothing
    
    mov esp, ebp
    pop ebp
    ;retn 04h
    retn 0
NotFound_FP_Check_FileSize:
    xor eax, eax
    jmp Exit_FP_Check_FileSize
Found_FP_Check_FileSize:
    mov eax, 1
    jmp Exit_FP_Check_FileSize
FP_Check_FileSize_Orig_Size:
	dd 0   
End_FP_Check_FileSize:


FP_Check_FileHashValue_CRC:
FP_Check_FileHashValue_CRC_Arg_Win32Api         equ 08h
FP_Check_FileHashValue_CRC_Arg_FuncAddr			equ 0ch
FP_Check_FileHashValue_CRC_StackSize            equ 20h+MAX_PATH
FP_Check_FileHashValue_CRC_fileSize             equ -04h
FP_Check_FileHashValue_CRC_NumberOfBytesRW      equ -08h
FP_Check_FileHashValue_CRC_CRCVALUE_current     equ -0ch
FP_Check_FileHashValue_CRC_pBuffer              equ -10h
FP_Check_FileHashValue_CRC_hFile                equ -14h
FP_Check_FileHashValue_CRC_szFileName           equ -(20h+MAX_PATH)
    
    push ebp
    mov ebp, esp
    sub esp, FP_Check_FileHashValue_CRC_StackSize
    
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    ;; clear the stack
    lea edi, [ebp-FP_Check_FileHashValue_CRC_StackSize]
    mov ecx, FP_Check_FileHashValue_CRC_StackSize
    xor eax, eax
    cld
    rep stosb
    
    mov ebx, dword ptr [ebp+FP_Check_FileHashValue_CRC_Arg_Win32Api]
    assume ebx : ptr WIN32APIBASE
    
    push MAX_PATH
    lea eax, [ebp+FP_Check_FileHashValue_CRC_szFileName]
    push eax
    push NULL
    call dword ptr [ebx].xGetModuleFileNameA
    
    push NULL
    push FILE_ATTRIBUTE_NORMAL
    push OPEN_EXISTING
    push NULL
    push FILE_SHARE_READ
    push GENERIC_READ
    lea eax, [ebp+FP_Check_FileHashValue_CRC_szFileName]
    push eax
    call dword ptr [ebx].xCreateFileA
    cmp eax, INVALID_HANDLE_VALUE
    jz NotFound_FP_Check_FileHashValue_CRC
    mov dword ptr [ebp+FP_Check_FileHashValue_CRC_hFile], eax
    
    push NULL
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_hFile]
    call dword ptr [ebx].xGetFileSize
    ;cmp eax, INVALID_FILE_SIZE
    ;jz NotFound_FP_Check_FileHashValue_CRC
    mov dword ptr [ebp+FP_Check_FileHashValue_CRC_fileSize], eax
    
    push PAGE_READWRITE
    push MEM_COMMIT
    push eax
    push NULL
    call dword ptr [ebx].xVirtualAlloc
    mov dword ptr [ebp+FP_Check_FileHashValue_CRC_pBuffer], eax
    
    ;; read file
    push NULL
    lea eax, [ebp+FP_Check_FileHashValue_CRC_NumberOfBytesRW]
    push eax
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_fileSize]
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_pBuffer]
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_hFile]
    call dword ptr [ebx].xReadFile
    
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_hFile]
    call dword ptr [ebx].xCloseHandle
    
    ;; get CRC value
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_fileSize]
    push dword ptr [ebp+FP_Check_FileHashValue_CRC_pBuffer]
    mov eax, dword ptr [ebp+FP_Check_FileHashValue_CRC_Arg_FuncAddr]
    assume eax : ptr FUNCADDRTBL
    call dword ptr [eax].xCRC32
    mov esi, eax
    
	;; release the memory
	push MEM_RELEASE
	push dword ptr [ebp+FP_Check_FileHashValue_CRC_fileSize]
	push dword ptr [ebp+FP_Check_FileHashValue_CRC_pBuffer]
	call dword ptr [ebx].xVirtualFree    
    
    call Get_FP_Check_FileHashValue_CRC_Eip
    Get_FP_Check_FileHashValue_CRC_Eip:
    pop edi
    add edi, offset FP_Check_FileHashValue_Orig_CRC - offset Get_FP_Check_FileHashValue_CRC_Eip
    sub edi, esi
    jz NotFound_FP_Check_FileHashValue_CRC
    jmp Found_FP_Check_FileHashValue_CRC
    
Exit_FP_Check_FileHashValue_CRC:
	pop edi
	pop esi
	pop edx
	pop ecx
    pop ebx
    assume eax : nothing
    assume ebx : nothing
    mov esp, ebp
    pop ebp
    ;retn 08h
    retn 0
NotFound_FP_Check_FileHashValue_CRC:
    xor eax, eax
    jmp  Exit_FP_Check_FileHashValue_CRC
Found_FP_Check_FileHashValue_CRC:
    mov eax, 1
    jmp  Exit_FP_Check_FileHashValue_CRC
FP_Check_FileHashValue_Orig_CRC:
	dd 0   
End_FP_Check_FileHashValue_CRC:

;; attack debugger
AD_SwitchDesktop:
AD_SwitchDesktop_Arg_Win32Api			equ 04h
	mov eax, dword ptr [esp+AD_SwitchDesktop_Arg_Win32Api]
	
	push ebx
	push ecx
	push edx
	push edi
	push esi
	
	mov ebx, eax
	assume ebx : ptr WIN32APIBASE
	
	call dword ptr [ebx].xGetCurrentThreadId
	push eax
	call dword ptr [ebx].xGetThreadDesktop
	mov ecx, eax
	
	push DESKTOP_SWITCHDESKTOP
	push FALSE
	push 0
	call dword ptr [ebx].xOpenInputDesktop
	mov edx, eax
	
	push NULL
	push GENERIC_ALL
	push 0
	push NULL
	push NULL
	call AD_SwitchDesktop_Str
		db "MyDesktop",0
	AD_SwitchDesktop_Str:
	call dword ptr [ebx].xCreateDesktopA
	mov esi, eax
	
	push esi
	call dword ptr [ebx].xSetThreadDesktop
	
	push esi
	call dword ptr [ebx].xSwitchDesktop
	
	push 5000
	call dword ptr [ebx].xSleep
	
	push edx
	call dword ptr [ebx].xSwitchDesktop
	
	push ecx
	call dword ptr [ebx].xSetThreadDesktop
	
	push esi
	call dword ptr [ebx].xCloseDesktop
	
	xor eax, eax
	
	pop esi
	pop edi
	pop edx
	pop ecx
	pop ebx
	assume ebx : nothing
	;retn 04h
	retn 0
End_AD_SwitchDesktop:


AD_BlockInput:
AD_BlockInput_Arg_Win32Api		equ 04h
	mov eax, dword ptr [esp+AD_BlockInput_Arg_Win32Api]
	
	push ebx
	mov ebx, eax
	assume ebx : ptr WIN32APIBASE
	
	push TRUE
	call dword ptr [ebx].xBlockInput
	test eax, eax
	jz Exit_AD_BlockInput
	push 5000
	call dword ptr [ebx].xSleep
	
	push FALSE
	call dword ptr [ebx].xBlockInput
Exit_AD_BlockInput:
	xor eax, eax
	pop ebx
	assume ebx : nothing
	;retn 04h
	retn 0
End_AD_BlockInput:


AD_ZwSetInformationThread:
AD_ZwSetInformationThread_Arg_Win32Api		equ 04h
	mov eax, dword ptr [esp+AD_ZwSetInformationThread_Arg_Win32Api]
	push ebx
	mov ebx, eax
	assume ebx : ptr WIN32APIBASE
	
	push 0
	push NULL
	push ThreadHideFromDebugger
	call dword ptr [ebx].xGetCurrentThread
	push eax
	call dword ptr [ebx].xZwSetInformationThread
	cmp eax, STATUS_SUCCESS
Exit_AD_ZwSetInformationThread:
	pop ebx
	assume ebx : nothing
	;retn 04h
	retn 0
End_AD_ZwSetInformationThread:


AD_INT_2e:
AD_INT_2e_Arg_Win32Api			equ 08h
AD_INT_2e_StackSize				equ 10h + sizeof OSVERSIONINFOEX
AD_INT_2e_bOsVersionInfoEx      equ -04h
AD_INT_2e_osvi                  equ -(10h+sizeof OSVERSIONINFOEX)
	push ebp
	mov ebp, esp
	sub esp, AD_INT_2e_StackSize
	
	push ebx
	push ecx
	push edx
	push edi
	push esi
	
	mov ebx, dword ptr [ebp+AD_INT_2e_Arg_Win32Api]
	assume ebx : ptr WIN32APIBASE
	
	lea edx, [ebp+AD_INT_2e_osvi]
	assume edx : ptr OSVERSIONINFOEX
	
	;; clear the stack
	lea edi, [ebp-AD_INT_2e_StackSize]
	mov ecx, AD_INT_2e_StackSize
	xor eax, eax
	cld
	rep stosb
	
	push sizeof OSVERSIONINFOEX
	pop dword ptr [edx].dwOSVersionInfoSize
	
	call dword ptr [ebx].xGetCurrentThread
	mov esi, eax
	
	lea edx, [ebp+AD_INT_2e_osvi]
	push edx
	call dword ptr [ebx].xGetVersionExA
	test eax, eax
	jz Exit_AD_INT_2e
	;; Test for the Windows NT product family
	lea edx, [ebp+AD_INT_2e_osvi]	
	cmp dword ptr [edx].dwPlatformId, VER_PLATFORM_WIN32_NT
	jnz Exit_AD_INT_2e
	push 0
	push 0
	push ThreadHideFromDebugger
	push esi
	mov eax, 0EEh		; NtSetInformationThread
	mov edx, esp
	int 2eh
	add esp, 10h
	
	push 0
	push 0
	push ThreadHideFromDebugger
	push esi
	mov eax, 0E5h		; NtSetInformationThread
	mov edx, esp
	int 2eh
	add esp, 10h
	;jmp Found_AD_INT_2e
	;AD_INT_2e_Tmp1:
	;; Test for the Windows 95 product family
	;cmp dword ptr [edx].dwPlatformId, VER_PLATFORM_WIN32_WINDOWS
	;jnz AD_INT_2e_Tmp2
	;AD_INT_2e_Tmp2:
	;; Microsoft Win32s
Exit_AD_INT_2e:
	xor eax, eax
	pop esi
	pop edi
	pop edx
	pop ecx
	pop ebx
	assume ebx : nothing
	assume edx : nothing
	mov esp, ebp
	pop ebp
	;retn 04h
	retn 0
End_AD_INT_2e:


;; attack specific debugger
AS_OD_OutputDebugString:
AS_OD_OutputDebugString_Arg_Win32Api		equ 04h
	mov eax, dword ptr [ebp+AS_OD_OutputDebugString_Arg_Win32Api]
	assume eax : ptr WIN32APIBASE
	call AS_OD_OutputDebugString_Str
		db "%s%s",0
	AS_OD_OutputDebugString_Str:
	call dword ptr [eax].xOutputDebugStringA
	xor eax, eax
	assume eax : nothing
	;retn 04h
	retn 0		
End_AS_OD_OutputDebugString:


;; attack breakpoint
AB_HWP_CLR_Exception:
	call Get_AB_HWP_CLR_Exception_Eip
	Get_AB_HWP_CLR_Exception_Eip:
	pop eax
	add eax, offset AB_HWP_CLR_Exception_Exception - offset Get_AB_HWP_CLR_Exception_Eip
	push eax
	assume fs : nothing
	push dword ptr fs:[0]
	mov dword ptr fs:[0], esp
	xor eax, eax
	;; throw an exception
	div eax
	;; restore exception handler
	pop dword ptr fs:[0]
	add esp, 04h
	jmp Exit_AB_HWP_CLR_Exception
AB_HWP_CLR_Exception_Exception:
	mov eax, dword ptr [esp+0ch]
	assume eax : ptr CONTEXT
	;; clr debug registers context Dr0-Dr3
	mov dword ptr [eax].iDr0, 0
	mov dword ptr [eax].iDr1, 0
	mov dword ptr [eax].iDr2, 0
	mov dword ptr [eax].iDr3, 0
	mov dword ptr [eax].iDr6, 0
	mov dword ptr [eax].iDr7, 0
	;; set eip upon return
	add dword ptr [eax].regEip, 02h
	xor eax, eax
	retn
Exit_AB_HWP_CLR_Exception:
	xor eax, eax
	assume eax : nothing
	retn 0h
End_AB_HWP_CLR_Exception:

End_AntiDebuggerModules:


AntiDebugger endp

CountAppendSize proc uses ebx ecx edx esi edi szFname : LPSTR, dwAppendSize : DWORD
;; count the append size
;; success:eax = file size
;; failed:eax = 0
    LOCAL hFile : HANDLE
    LOCAL hMap : HANDLE
    LOCAL pMem : LPVOID
    LOCAL dwFileSize : DWORD
    LOCAL dwResult : DWORD

    ;; open file
	invoke CreateFile, szFname,\
	                   GENERIC_WRITE + GENERIC_READ,\
	                   FILE_SHARE_WRITE + FILE_SHARE_READ,\
	                   NULL,\
	                   OPEN_EXISTING,\
	                   FILE_ATTRIBUTE_NORMAL,\
	                   0
    .IF eax == INVALID_HANDLE_VALUE
        xor eax, eax
        ret              
    .ENDIF	              
    mov hFile, eax
    ;; get file size
    invoke GetFileSize, hFile, NULL
    inc eax
    .IF eax == 0
        invoke CloseHandle, hFile
        ret
    .ENDIF
    dec eax
    mov dwFileSize, eax
    ;; plus shell size
    xchg eax, ecx
    ;; create memory map
    xor ebx, ebx
    invoke CreateFileMapping, hFile, ebx, PAGE_READONLY, ebx, ecx, ebx
    .IF eax == 0
        invoke CloseHandle, hFile
        ret                
    .ENDIF
    mov hMap, eax
    ;; map file to memory
    xor ebx, ebx
    invoke MapViewOfFile, hMap, 
                          FILE_MAP_READ,
                          ebx, ebx, ebx
    .IF eax == 0
        invoke CloseHandle, hMap
        invoke CloseHandle, hFile
        ret
    .ENDIF 
    mov pMem, eax    
    mov ebx, eax
    add ebx, dword ptr [ebx+3ch]
    assume ebx : ptr IMAGE_NT_HEADERS
    mov edx, dword ptr [ebx].OptionalHeader.FileAlignment   
    mov ecx, dwAppendSize
    add ecx, dwFileSize
    invoke PEAlign, ecx, edx
    mov dwResult, eax 
    ;; close handle & write it
    invoke UnmapViewOfFile, pMem
    invoke CloseHandle, hMap
    invoke CloseHandle, hFile
    mov eax, dwResult
    assume ebx : nothing
    ret

CountAppendSize endp

PEAlign proc uses ecx edx, dwTarNum : DWORD, dwAlignTo : DWORD
;; returns aligned value    
    ;; Algorithms:
    ;; $1 = dwTarNum / dwAlignTo
    ;; if remain != 0
    ;; $r = $1 + 1 * dwAlignTo
    ;; return $r
    mov ecx, dwAlignTo
    mov eax, dwTarNum
    xor edx, edx
    div ecx
    cmp edx, 0
    jz AlreadyAligned
    inc eax
AlreadyAligned:
    mul ecx      
    ret

PEAlign endp

RVA2Offset proc uses edi esi edx ecx, pFileMap : DWORD, RVA : DWORD
; RVA2Offset proc uses ebx ecx edx, Base : DWORD, dwITRVA : DWORD
; calulates the Offset from a RVA
; Base    - base of the memory file
; dwITRVA - the RVA to calculate
; returns 0 if an error occurred else the calculated Offset will be returned
    mov esi, pFileMap 
    assume esi : ptr IMAGE_DOS_HEADER 
    add esi, [esi].e_lfanew 
    assume esi : ptr IMAGE_NT_HEADERS 
    mov edi, RVA ; edi == RVA 
    mov edx, esi
    add edx, sizeof IMAGE_NT_HEADERS 
    mov cx, [esi].FileHeader.NumberOfSections
    movzx ecx, cx 
    assume edx : ptr IMAGE_SECTION_HEADER 
    .WHILE ecx > 0 ; check all sections 
        .IF edi >= [edx].VirtualAddress 
            mov eax, [edx].VirtualAddress 
            add eax, [edx].SizeOfRawData 
            .IF edi < eax ; The address is in this section
                mov eax, [edx].VirtualAddress 
                sub edi, eax
                mov eax, [edx].PointerToRawData 
                add eax, edi ; eax == file offset 
                ret
            .ENDIF 
        .ENDIF
        add edx, sizeof IMAGE_SECTION_HEADER 
        dec ecx 
    .endw 
    assume edx : nothing 
    assume esi : nothing 
    ; mov eax, edi
    xor eax, eax  
RVA2OffsetExit:    
    ret

RVA2Offset endp

AddSection proc uses ebx ecx edx esi edi, pMem : LPVOID,
                                          pSectionName : LPVOID,        
                                          dwSectionSize : DWORD
;; add a new section
;; success: eax =  new section table file offset
;; failed: eax = 0
    LOCAL dwNTHeader : LPVOID
    LOCAL dwLastSecTbl : LPVOID    
    LOCAL dwFileAlig : DWORD
    LOCAL dwSecAlig : DWORD
    
    ;; move to section table
    mov esi, pMem
    ;ssume esi : ptr IMAGE_DOS_HEADER
    ;add esi, dword ptr [esi].e_lfanew
    add esi, dword ptr [esi+3ch]
    mov dwNTHeader, esi  
    assume esi : ptr IMAGE_NT_HEADERS
    ;; update the number of section
    mov cx, word ptr [esi].FileHeader.NumberOfSections
    movzx ecx, cx
    inc word ptr [esi].FileHeader.NumberOfSections
    push dword ptr [esi].OptionalHeader.FileAlignment
    pop dwFileAlig
    push dword ptr [esi].OptionalHeader.SectionAlignment
    pop dwSecAlig        
    ;; move esi point to section table
    add esi, sizeof IMAGE_NT_HEADERS      
    ;; store the last section table
    mov eax, sizeof IMAGE_SECTION_HEADER
    mov ebx, ecx
    imul ebx
    add esi, eax                            ; esi = the end of orig last section fva
    push esi
    sub esi, sizeof IMAGE_SECTION_HEADER    ; esi = the orig last section fva
    mov dwLastSecTbl, esi
    pop esi
    ;; set new section table
    assume esi : ptr IMAGE_SECTION_HEADER
    ;; set section name
    push esi
    lea edi, [esi].Name1
    mov esi, pSectionName
    test esi, esi
    jz EndCopySectionNameLoop
CopySectionNameLoop:   
    lodsb
    test al, al
    jz EndCopySectionNameLoop
    stosb
    jmp CopySectionNameLoop
EndCopySectionNameLoop:
    pop esi  
    ;; set section characteristics
    push 0E00000E0h
    pop dword ptr [esi].Characteristics
    ;; set section virtualsize
    push dwSectionSize
    pop dword ptr [esi].Misc.VirtualSize
    ;; set section sizeofrawdata
    invoke PEAlign, dwSectionSize, dwFileAlig
    mov dword ptr [esi].SizeOfRawData, eax
    ;; set section virtualaddress
    mov eax, dwLastSecTbl                            ; eax = orig last section table fva
    assume eax : ptr IMAGE_SECTION_HEADER
    mov ecx, dword ptr [eax].VirtualAddress
    add ecx, dword ptr [eax].Misc.VirtualSize        ; ecx = new section rva
    mov ebx, dword ptr [eax].PointerToRawData
    add ebx, dword ptr [eax].SizeOfRawData           ; ebx = new section fva
    invoke PEAlign, ecx, dwSecAlig
    mov dword ptr [esi].VirtualAddress, eax
    ;; set section pointertorawdata
    invoke PEAlign, ebx, dwFileAlig
    mov dword ptr [esi].PointerToRawData, eax
    ;; update the sizeofimage
    mov eax, dword ptr [esi].VirtualAddress
    add eax, dword ptr [esi].Misc.VirtualSize
    invoke PEAlign, eax, dwSecAlig
    mov edx, dwNTHeader
    assume edx : ptr IMAGE_NT_HEADERS
    mov dword ptr [edx].OptionalHeader.SizeOfImage, eax
    push dword ptr [esi].PointerToRawData
    pop edi
    add edi, pMem                                   ; edi = new IID fva
    ;; clear the new Section
    mov ecx, dwSectionSize
    xor eax, eax
    cld
    rep stosb
    
    mov eax, esi  
    assume esi : nothing
    assume eax : nothing
    assume edx : nothing    
    
    ret
    
AddSection endp

Help proc
	
	invoke crt_printf, offset g_szOutFormat, offset g_szTitle
	invoke crt_printf, offset g_szOutFormat, offset g_szUsage
	invoke crt_printf, offset g_szOutFormat, offset g_szOptions
	invoke crt_printf, offset g_szOutFormat, offset g_szAuthor
	ret
Help endp

AntiDebugger_HandleArguments proc uses ebx ecx edx esi edi pAntiDebuggerCmdLine : LPSTR, pAntiDebuggerArguments : LPVOID
	LOCAL szCommandLine[MAX_PATH] : BYTE
	
	;; clear the stack
	lea edi, szCommandLine
	mov ecx, MAX_PATH
	cld
	rep stosb
	
	invoke lstrcpy, addr szCommandLine, pAntiDebuggerCmdLine
	mov esi, eax
	
	mov edi, pAntiDebuggerArguments
	assume edi : ptr ANTIDEBUGGERARGUMENTS
	
	;; count the argc and replace the bank to NULL
	xor ecx, ecx
	.while byte ptr [esi] != NULL
		mov al, byte ptr [esi]
		.if al == 20h
			mov byte ptr [esi], NULL
			inc ecx
		.endif
		inc esi
	.endw
	inc ecx
	mov eax, ecx
	;; set funcs count
	sub eax, 02h
	mov dword ptr [edi].dwFuncCounts, eax
	
	;; judge the arguments count
	lea esi, szCommandLine
	.if ecx < 2
		invoke Help
		jmp Exit_AntiDebugger_HandleArguments
	.elseif ecx == 2
		;; copy file name
		invoke lstrlen, esi
		add esi, eax
		inc esi
		lea eax, [edi].szFileName
		invoke lstrcpy, eax, esi
		jmp Exit_AntiDebugger_HandleArguments
	.endif
	
	mov ebx, 04h
	lea esi, szCommandLine
	
	push ecx
	invoke lstrlen, esi
	pop ecx
	
	add esi, eax
	inc esi
	dec ecx
	lea edi, [edi].Use_FD_IsDebuggerPresent
	;; get the arguments
	.while ecx != 1
	
		push ecx
		invoke crt_atoi, esi
		pop ecx
		
		dec eax
		imul ebx
		add eax, edi
		mov dword ptr [eax], 1
		
		;; count the current argument
		push ecx
		invoke lstrlen, esi
		pop ecx
		
		add esi, eax
		inc esi
		
		dec ecx		
	.endw
	
	;; copy file path
	mov edi, pAntiDebuggerArguments
	lea edi, [edi].szFileName
	invoke lstrcpy, edi, esi
	
Exit_AntiDebugger_HandleArguments:
	assume edi : nothing
	ret
AntiDebugger_HandleArguments endp

end Start



