

#ifndef ___COMHDR___
#define ___COMHDR___

#include <ntifs.h>
#include <wdm.h>
#include <ntddk.h>
#include "Loader.h"
#include "stdlib.h"
#include "stdio.h"
#include <stdarg.h>
#include "myDriverLoader.h"
#include "HookPort.h"
#include "HookPortBypass.h"
#include "myDriverLoader.h"

#include "comstruct.h"

////////////////////////////////////////////////////////////////////////////////

#define Windows_2K	2000
#define Windows_XP	2001
#define Windows_2k3	2003
#define Windows_Vista	2004
#define Windows_7	2005

#define OsModuleName	("ntkrnlpa.exe")






inline void kprintf(char *fmt,...)
{
	char mybuffer[2048]={0};
	va_list	val;
	va_start(val, fmt);
	int icount=_vsnprintf(mybuffer,2048,fmt, val);
	if (icount>2048)
	{
		DbgPrint("warning...buffer overflow....\r\n");
				va_end(val);
		return ;
	}
	DbgPrint("[REKNERNEL-HookPort]:");
	DbgPrint(mybuffer);
	DbgPrint("\r\n");
	va_end(val);

}
extern ULONG	g_osBase;
extern ULONG	g_ProxyModuleBase;
extern ULONG	g_Base2Relocation;
extern	char		g_StrongOdSyS[512];
//Windows XP SP2£º

typedef struct _KPROCESS                     // 29 elements, 0x6C bytes (sizeof) 
{                                                                                
	/*0x000*/     struct _DISPATCHER_HEADER Header;        // 6 elements, 0x10 bytes (sizeof)  
	/*0x010*/     struct _LIST_ENTRY ProfileListHead;      // 2 elements, 0x8 bytes (sizeof)   
	/*0x018*/     ULONG32      DirectoryTableBase[2];                                          
	/*0x020*/     struct _LIST_ENTRY LdtDescriptor;         // 3 elements, 0x8 bytes (sizeof)   
	/*0x028*/     struct _LIST_ENTRY Int21Descriptor;       // 4 elements, 0x8 bytes (sizeof)   
	/*0x030*/     UINT16       IopmOffset;                                                     
	/*0x032*/     UINT8        Iopl;                                                           
	/*0x033*/     UINT8        Unused;                                                         
	/*0x034*/     ULONG32      ActiveProcessors;                                               
	/*0x038*/     ULONG32      KernelTime;                                                     
	/*0x03C*/     ULONG32      UserTime;                                                       
	/*0x040*/     struct _LIST_ENTRY ReadyListHead;        // 2 elements, 0x8 bytes (sizeof)   
	/*0x048*/     struct _SINGLE_LIST_ENTRY SwapListEntry; // 1 elements, 0x4 bytes (sizeof)   
	/*0x04C*/     VOID*        VdmTrapcHandler;                                                
	/*0x050*/     struct _LIST_ENTRY ThreadListHead;       // 2 elements, 0x8 bytes (sizeof)   
	/*0x058*/     ULONG32      ProcessLock;                                                    
	/*0x05C*/     ULONG32      Affinity;                                                       
	/*0x060*/     UINT16       StackCount;                                                     
	/*0x062*/     CHAR         BasePriority;                                                   
	/*0x063*/     CHAR         ThreadQuantum;                                                  
	/*0x064*/     UINT8        AutoAlignment;                                                  
	/*0x065*/     UINT8        State;                                                          
	/*0x066*/     UINT8        ThreadSeed;                                                     
	/*0x067*/     UINT8        DisableBoost;                                                   
	/*0x068*/     UINT8        PowerState;                                                     
	/*0x069*/     UINT8        DisableQuantum;                                                 
	/*0x06A*/     UINT8        IdealNode;                                                      
	union                                    // 2 elements, 0x1 bytes (sizeof)   
	{                                                                            
	//	/*0x06B*/         struct _KEXECUTE_OPTIONS Flags;      // 7 elements, 0x1 bytes (sizeof)   
		/*0x06B*/         UINT8        ExecuteOptions;                                             
	};                                                                           
}KPROCESS, *PKPROCESS; 


#endif

