#include <ntddk.h>
#include "KernelSearchProcess.h"

extern POBJECT_TYPE *PsProcessType;


void UnLoad( PDRIVER_OBJECT pDriverObject );



/*
typedef ULONG   DWORD ;  
 
#define EPROCESS_SIZE       1  
#define PEB_OFFSET          2  
#define FILE_NAME_OFFSET        3  
#define PROCESS_LINK_OFFSET     4  
#define PROCESS_ID_OFFSET       5  
#define EXIT_TIME_OFFSET        6  
 
#define OBJECT_HEADER_SIZE      0x018 // 对象头大小  
#define OBJECT_TYPE_OFFSET      0x008 // 类型在对象头结构中的偏移      
  
typedef enum
{   
    VALID_PAGE = 0,   
    INVALID_PTE,   
    INVALID_PDE    
}PAGE_STATUS;   
  
typedef struct _PROCESS_INFO {   
    DWORD   EProcess ;   
    DWORD   dwProcessId ;   
    DWORD   dwPeb ;   
    PUCHAR  pImageFileName ;   
} PROCESS_INFO, *PPROCESS_INFO ;   
  
  
DWORD GetPlantformDependentInfo ( DWORD dwFlag )   
{    
    DWORD current_build;    
    DWORD ans = 0;    
  
    PsGetVersion(NULL, NULL, &current_build, NULL);    
  
    switch ( dwFlag )   
    {    
    case EPROCESS_SIZE:    
        if (current_build == 2195) ans = 0 ;        // 2000，当前不支持2000，下同   
        if (current_build == 2600) ans = 0x25C;     // xp   
        if (current_build == 3790) ans = 0x270;     // 2003   
        break;    
    case PEB_OFFSET:    
        if (current_build == 2195)  ans = 0;    
        if (current_build == 2600)  ans = 0x1b0;    
        if (current_build == 3790)  ans = 0x1a0;   
        break;    
    case FILE_NAME_OFFSET:    
        if (current_build == 2195)  ans = 0;    
        if (current_build == 2600)  ans = 0x174;    
        if (current_build == 3790)  ans = 0x164;   
        break;    
    case PROCESS_LINK_OFFSET:    
        if (current_build == 2195)  ans = 0;    
        if (current_build == 2600)  ans = 0x088;    
        if (current_build == 3790)  ans = 0x098;   
        break;    
    case PROCESS_ID_OFFSET:    
        if (current_build == 2195)  ans = 0;    
        if (current_build == 2600)  ans = 0x084;    
        if (current_build == 3790)  ans = 0x094;   
        break;    
    case EXIT_TIME_OFFSET:    
        if (current_build == 2195)  ans = 0;    
        if (current_build == 2600)  ans = 0x078;    
        if (current_build == 3790)  ans = 0x088;   
        break;    
    }    
    return ans;    
}    
  
PAGE_STATUS CheckPageValid ( DWORD addr )   
{      
    ULONG pte;    
    ULONG pde;   
	_asm
		mov 
  
    pde = 0xc0300000 + (addr>>22)*4;    
    if( (*(PULONG)pde & 0x1) != 0)   
    {    
        //large page    
        if( (*(PULONG)pde & 0x80) != 0)   
            return VALID_PAGE;    
  
        pte = 0xc0000000 + ( addr >> 12 ) * 4 ;    
  
        return ( (*(PULONG)pte & 0x1) != 0 ) ? VALID_PAGE :INVALID_PTE ;   
    }    
    return INVALID_PDE ;   
}   
  
VOID ShowProcessInfo ( DWORD EProcess )   
{   
    PROCESS_INFO    ProcessInfo = {0} ;   
  
    DWORD   dwPidOffset     = GetPlantformDependentInfo ( PROCESS_ID_OFFSET ) ;   
    DWORD   dwPNameOffset   = GetPlantformDependentInfo ( FILE_NAME_OFFSET ) ;   
    DWORD   dwPLinkOffset   = GetPlantformDependentInfo ( PROCESS_LINK_OFFSET ) ;   
    DWORD   dwPebOffset     = GetPlantformDependentInfo ( PEB_OFFSET ) ;   
  
    ProcessInfo.EProcess        = EProcess ;   
    ProcessInfo.dwProcessId     = *(DWORD*)( EProcess + dwPidOffset ) ;   
    ProcessInfo.pImageFileName  = (PUCHAR)( EProcess + dwPNameOffset ) ;   
    ProcessInfo.dwPeb           = *(DWORD*)( EProcess + dwPebOffset ) ;   
  
    DbgPrint ("Pid=%8d EProcess=0x%08X Peb=0x%08X %s\n", \
		ProcessInfo.dwProcessId, ProcessInfo.EProcess, ProcessInfo.dwPeb, ProcessInfo.pImageFileName ) ;   
}   
  
///////////////////////////////////////////////////////////////////////////////   
//  枚举进程——搜索PEB   
///////////////////////////////////////////////////////////////////////////////   
VOID EnumProcessList ()   
{   
    DWORD       SysEProcess, CurEProcess ;   
    ULONG       i;    
   // ULONG       Address;    
    PAGE_STATUS PageStatus ;   
    DWORD       dwObjTypeAddr ;   
    DWORD       dwObjProcessType ;   
  
    DWORD   dwPebOffset     = GetPlantformDependentInfo ( PEB_OFFSET ) ;   
    DWORD   dwEProcessSize  = GetPlantformDependentInfo ( EPROCESS_SIZE ) ;   
  
    __try {   
  
        SysEProcess = (DWORD)PsGetCurrentProcess () ;   
        dwObjProcessType = *((DWORD*)(SysEProcess - OBJECT_HEADER_SIZE + OBJECT_TYPE_OFFSET ) ) ;   
  
        //system的PEB总是零,无法用下面的方法搜索到   
        ShowProcessInfo ( SysEProcess );   
  
        // 所有的EPROCESS中，System进程处于一个比较大的位置，   
        // 在测试时只发现winlogon的EPROCESS大于SysEProcess，   
        // 所以只需要在SysEProcess中加少量修正即可（这里为8M）   
        for( i = 0x80000000; i < SysEProcess + 0x800000; i += 4 ) {    
            PageStatus = CheckPageValid ( i ) ;   
            switch ( PageStatus )   
            {   
            case VALID_PAGE:   
                {   
                    // 每个进程的PEB地址的都差不多，高字部分固定为0x7FFD   
                    if ( ( *(PULONG)i & 0xFFFF0000 ) != 0x7FFD0000 )   
                        break ;   
  
                    // 到这里就表明：地址i很有可能就是EProcess结构中的Peb的指针   
                    // 下面就开始确认。   
  
                    // 检测EPROCESS地址是否合法   
                    if ( CheckPageValid(CurEProcess=i-dwPebOffset) != VALID_PAGE )   
                        break ;   
  
                    // 检测进程对象头中类型地址是否合法   
                    if ( CheckPageValid(dwObjTypeAddr=CurEProcess-OBJECT_HEADER_SIZE+OBJECT_TYPE_OFFSET) != VALID_PAGE )   
                        break ;   
  
                    // 检测是否进程对象类型   
                    if ( *(DWORD*)dwObjTypeAddr == dwObjProcessType )   
                    {   
                        ShowProcessInfo ( CurEProcess ) ;   
                        i += dwEProcessSize ;   
                    }   
                }   
                break ;   
            case INVALID_PTE:   i = i - 4 + 0x1000;     break ;     // 4KB   
            case INVALID_PDE:   i = i - 4 + 0x400000;   break ;     // 4MB   
            }   
        }   
    } __except ( 1 ) {   
        DbgPrint ( "exception" ) ;   
    }   
}  

*/

NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS; 

	 DisplayInfo();
	 //EnumProcessList ();

	pDriverObject->DriverUnload = UnLoad; 

	return Status;
}


void UnLoad( PDRIVER_OBJECT pDriverObject )
{
	DbgPrint("Driver Unload..\n");
}


void DisplayInfo()
{
	ULONG uStartAddr = 0x80000000;
	PEPROCESS pCurrent = PsGetCurrentProcess();

//	ULONG uPetAddr = (ULONG)PsGetProcessPeb( pCurrent );
	DbgPrint("PId:%d\tPath:%s\n", *(PULONG)((ULONG)pCurrent+0x084), (PUCHAR)((ULONG)pCurrent+0x174));

	for(; uStartAddr < (ULONG)pCurrent+0x800000; uStartAddr += 4 )
	{
		ULONG uRet = IsValidAddr( uStartAddr );
		if( uRet == VALID_PAGE )
		{

			if( ( *(PULONG)uStartAddr & 0xffff0000) == 0x7ffd0000 )
			{
				if( IsRealProcess(uStartAddr - 0x1b0) )
				{
					PLARGE_INTEGER pExitTime = (PLARGE_INTEGER)(uStartAddr-0x1b0+0x078);
					if( pExitTime->QuadPart == 0 )
					{
						DbgPrint("PId:%d\tPath:%s\n", *(PULONG)(uStartAddr-0x1b0+0x084), (PUCHAR)(uStartAddr-0x1b0+0x174));
					}
					
					uStartAddr -= 4;
					uStartAddr += 0x25c;
				}
			}
		}
		else if( uRet == PDEINVALID )
		{
			uStartAddr -= 4;
			uStartAddr += 0x400000;
		}
		else
		{
			uStartAddr -= 4;
			uStartAddr += 0x1000;
		}
	
	}
}

ULONG IsValidAddr( ULONG uAddr )
{
	ULONG uInfo;
	ULONG uCr4;
	ULONG uPdeAddr;
	ULONG uPteAddr;
	_asm
	{
		cli
		push eax

		_emit 0x0F
		_emit 0x20
		_emit 0xE0//mov eax,cr4

		mov [uCr4], eax
		pop eax
	}

	_asm sti

	uInfo = uCr4 & 0x20;
	if( uInfo != 0 )
	{
		uPdeAddr = (uAddr>>21)*8+0xC0600000;
	}
	else
		uPdeAddr = (uAddr>>22)*4+0xc0300000;
	if( (*(PULONG)uPdeAddr & 0x1) != 0 )
	{
		if( (*(PULONG)uPdeAddr & 0x80) != 0 )
		{
			return VALID_PAGE;
		}
		else
		{
			if( uInfo != 0 )
			{
				uPteAddr = (uAddr>>12)*8+0xc0000000;
			}
			else
			{
				uPteAddr = (uAddr>>12)*4+0xc0000000;
			}

			if( (*(PULONG)uPteAddr & 0x1) != 0 )
				return VALID_PAGE;
			else
				return PTEINVALID;
		}
	}
	else
		return PDEINVALID;

}

BOOLEAN IsRealProcess( ULONG pAddr )
 {
	 ULONG uType = 0;
	 ULONG pObjectTypeAddr = 0;
	 if( IsValidAddr(pAddr) != VALID_PAGE )
		 return FALSE;
	 pObjectTypeAddr = pAddr-0x18+0x8;
	 if( IsValidAddr( pObjectTypeAddr ) != VALID_PAGE )
		 return FALSE;
	 else
	 {
		 ULONG pObjectType = *(PULONG)pObjectTypeAddr;
		 if( pObjectType == *(PULONG)PsProcessType )
			 return TRUE;
	 }
	 return FALSE;

 }
 