#include  "commhdr.h"

//////////////////////////////////////////////////////////////////////////
BOOLEAN	g_bPatchKiDispatch=FALSE;
UCHAR	g_PatchKiDispatchOldCode[0xf];
PUCHAR	g_PatchAddress;
PVOID	g_DebugPort=NULL;
ULONG	g_ProcessNameOffset;
PUCHAR	g_TargetProName=(PUCHAR)"dnf.exe";
	
//////////////////////////////////////////////////////////////////////////
void GetProcessNameOffset() {	
    PEPROCESS curproc;
    int i;
	curproc = PsGetCurrentProcess();
    for(i=0;i<3*PAGE_SIZE;i++) {
        if(strncmp("System",(char*)curproc+i,6)==0) {
            g_ProcessNameOffset = i;
			break;
		}
    }
}

//////////////////////////////////////////////////////////////////////////

void CreateProcessNotifyRoutine(IN HANDLE ParentId, IN HANDLE ProcessId, IN BOOLEAN Create)
{
	PEPROCESS currentEP = NULL;

	if(Create)
	{
			PsLookupProcessByProcessId(ProcessId,&currentEP);
		if(currentEP==NULL)
			return;
			kprintf("CreateProcess %s\r\n", (char*)currentEP+g_ProcessNameOffset );
		if (_strnicmp((char*)currentEP+g_ProcessNameOffset,(char*)g_TargetProName,strlen((char*)g_TargetProName))==0)
		{
			kprintf("Target Identified pid=%d, EP=%X\r\n", ProcessId, currentEP);
			g_TargetEP	=	(ULONG)currentEP;
		}
		ObDereferenceObject(currentEP);
	}
}
//////////////////////////////////////////////////////////////////////////

BOOLEAN	IdentiyTarget(
	PPROTECT_INFO pProInfo
	)
{
		PEPROCESS currentEP = NULL;
		if (pProInfo->PidOrName)
		{
				PsLookupProcessByProcessId((HANDLE)pProInfo->PID,&currentEP);
				if(currentEP==NULL)
				{
						if (pProInfo->PID==0)
						{
								g_TargetEP	=	NULL;	//0就清空
								g_DebugPort	=	NULL;
								return TRUE;
						}						
						return FALSE;
				}
				ObDereferenceObject(currentEP);
				g_TargetEP	=	(ULONG)currentEP;
				//kprintf("Target Identified pid=%d, EP=%X, name=%s\r\n", pProInfo->PID, currentEP,  (char*)currentEP+g_ProcessNameOffset);
				return TRUE;
		}	//end  		if (pProInfo->PidOrName)
		else
		{
		//	这个是按进程名字来区分的
				UNICODE_STRING	unistr1;
				ANSI_STRING		ans1;
				RtlInitAnsiString(&ans1, pProInfo->szNameBuffer);
				RtlAnsiStringToUnicodeString(&unistr1, &ans1,1);
				if (GetGargetEPbyName(&unistr1))
				{
						kprintf("GetGargetEPbyName success , EP=%X\r\n",g_TargetEP);
				}
				else
				{
						kprintf("GetGargetEPbyName fail , EP=%X\r\n",g_TargetEP);
				}
				RtlFreeUnicodeString(&unistr1);
		}
		
		return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//这是用在一般调试API判断端口是否为空中用到的
//
PVOID	myReturnDebugPort(EPROCESS* EP)
{
		if ((ULONG)EP==g_TargetEP)
		{
				if (g_DebugPort==0)
				{
					kprintf("fuck ,something unexpected occured\r\n");
				//	DbgBreakPoint();
				}
				return g_DebugPort;
		}
		return EP->DebugPort;
		
}

//////////////////////////////////////////////////////////////////////////
//这个是用在硬编译搜索的点patch的
ULONG CheckDebugPort()
{
	ULONG	CurrenEP	=	(ULONG)PsGetCurrentProcess();
	if (g_TargetEP==0)	//	未开启保护，正常返回
	{
		return	*(PULONG)(CurrenEP+0xbc);	
	}
	
	
	if (g_TargetEP	==	CurrenEP)
	{
		return 1;
	}
	return 0;
	
}
//////////////////////////////////////////////////////////////////////////
NTSTATUS	WithdrawWhenUnload()
{
		kprintf("Enter WithdrawWhenUnload\r\n");
		g_bPatchKiDispatch	=	FALSE;
		WPOFF();
		//recover 1
		RtlCopyMemory(g_PatchAddress, g_PatchKiDispatchOldCode, sizeof(g_PatchKiDispatchOldCode));

		//recover2
		LIST_ENTRY*	pHead=&g_HookInfoListHead;
		while(!IsListEmpty(pHead))
		{
				PHOOKINFO	ptmp	=(PHOOKINFO)	RemoveHeadList((LIST_ENTRY*)pHead);
				kprintf("recovering %s\r\n", ptmp->szFunName);
			
				RtlCopyMemory((PUCHAR)ptmp->OriAddress, (PUCHAR) ptmp->szOldCode, ptmp->OldCodeSize);
				kfree(ptmp);
		}

		WPON();
		
		kprintf("Leave WithdrawWhenUnload()\r\n");
		return 1;
	
}
//////////////////////////////////////////////////////////////////////////

///
			//s -b 804d8000 806ce100 64 A1 24 01 00 00 8B 40  44 39 B8 BC 00 00 00
		//patch 掉
// 		804fd740 64a124010000    mov     eax,dword ptr fs:[00000124h]
// 804fd746 8b4044          mov     eax,dword ptr [eax+44h]
// 804fd749 39b8bc000000    cmp     dword ptr [eax+0BCh],edi
//修改为call xxx
// cmp eax,0
//3d00000000      cmp     eax,0
// 804fd740  0124a164 408b0000 bcb83944 74000000
// 804fd750  8d016a13 fffd1885 e85650ff 0016233e
NTSTATUS	PatchDebugPortCheckInKiDispatch(PUCHAR osNewBaseAddress)
{
		PUCHAR	OsModuleBaseAddress;
		ULONG	OsModuleSize;
// 		if (g_bPatchKiDispatch)
// 		{
// 				kprintf("alread PatchDebugPortCheckInKiDispatch\r\n");
// 				return STATUS_SUCCESS;
// 		}
		OsModuleBaseAddress	=	(PUCHAR)GetOsMoudleInfo(&OsModuleSize);
		if (!MmIsAddressValid(OsModuleBaseAddress))
		{
				kprintf("Can not get ModuleBase,when patch Kidispatch \r\n");
				return	STATUS_UNSUCCESSFUL;
		}
		if (osNewBaseAddress)
		{
				//如果传下来的不是0，说明使用了重新加载大法
				OsModuleBaseAddress	=	osNewBaseAddress;
		}
		ULONG	HD1	=	0x0124a164;
		ULONG	HD2	=	0x408b0000;
		ULONG	HD3	=	0xbcb83944;

		PULONG	target=	(PULONG)(OsModuleBaseAddress+0x5000);//	随便加个数
		BOOLEAN	bFound=FALSE;
		for (; (ULONG)target<ULONG(OsModuleBaseAddress+OsModuleSize-12); target=(PULONG)((ULONG)target+1))
		{
				__try
				{
						if (!MmIsAddressValid(target))
						{
								continue;
						}
						if (target[0]==HD1&&target[1]==HD2&&target[2]==HD3)
						{
							bFound	=	TRUE;
							break;
						}
				}
				__except(1)
				{

				}

		}
		if (!bFound)
		{
				kprintf("Search HardCode fail ,when patch Kidispatch , maybe already patch %X\r\n", OsModuleBaseAddress);
				return	STATUS_UNSUCCESSFUL;
		}

		UCHAR	PatchStub[0xf]={0xe8,0x90,0x90,        0x90,0x90,0x90,           0x90,0x90,0x90,           0x90,0x90,0x90,           0x90,0x90,0x90};
		g_PatchAddress	=	(PUCHAR)target;
		RtlCopyMemory(g_PatchKiDispatchOldCode, g_PatchAddress, 0xf);	
	ULONG	itmp=0;
	#define CALLLEN	5
	//patch 成如下形式
		//call CheckDebugPort
		// cmp eax,0
		//nop 
		//....

		// DO the patch
		WPOFF();
		itmp	=	(LONG)CheckDebugPort-(LONG)target-CALLLEN;
		*(PULONG)&PatchStub[1]=	itmp;
		*(PULONG)&PatchStub[5]=	0x0000003d ;
		PatchStub[9]=0x0;
		RtlCopyMemory(g_PatchAddress, PatchStub, 0xf);	
		WPON();
		g_bPatchKiDispatch	=TRUE;

		return STATUS_SUCCESS;

}


//////////////////////////////////////////////////////////////////////////

//获取系统当前使用的module名字，并返回base address,和size of module 
//返回1表示给的参数长度不够

ULONG GetOsMoudleInfo(PULONG	pModuleSize, char* ModuleName, ULONG uLen )
{
    PSYSTEM_INFO_DRIVERS	pInfoBuff = NULL;
    ULONG					dwBuffSize;
    BOOLEAN					bNotenough;
    ULONG                   ModuleBase = NULL;

	
    dwBuffSize  = 0x8000;	// Exactly as it is in NTDLL.DLL
    bNotenough  = TRUE;

    if (ModuleName)
    {
		RtlZeroMemory(ModuleName, uLen);
    }
    while( bNotenough )
    {
        if ( dwBuffSize > 0x8000 * 20 ) // Too much, hopeless :(
            return NULL;
        
        if ( pInfoBuff = (PSYSTEM_INFO_DRIVERS)kmalloc ( dwBuffSize ) )
        {
            if ( STATUS_INFO_LENGTH_MISMATCH == ZwQuerySystemInformation( (ULONG)0x0b, pInfoBuff, dwBuffSize, NULL ) ) 
            {
                dwBuffSize += 0x8000;
                kfree ( pInfoBuff );
            } 
            else 
                bNotenough = FALSE;
        }
        else        
            return NULL;
    }

	
	ULONG   len = (ULONG)strlen ( pInfoBuff->Drivers[0].PathName );
	if (uLen>=len)
	{
		strncpy(ModuleName, pInfoBuff->Drivers[0].PathName, len);
		ModuleName[len]='\0';
		ModuleBase = (ULONG)pInfoBuff->Drivers[0].BaseAddress;
		*pModuleSize	=	pInfoBuff->Drivers[0].Size;
	}
	else
	{
		ModuleBase = (ULONG)pInfoBuff->Drivers[0].BaseAddress;
		*pModuleSize	=	pInfoBuff->Drivers[0].Size;
	}
    if ( pInfoBuff )
        kfree(pInfoBuff);
	return ModuleBase;
}

ULONG GetMoudleBase(char* ModuleName )
{
    PSYSTEM_INFO_DRIVERS	pInfoBuff = NULL;
    ULONG					dwBuffSize;
    BOOLEAN					bNotenough;
    ULONG                   ModuleBase = NULL;
	 ULONG                   ModuleNameLength = (ULONG)strlen ( ModuleName );
	
    dwBuffSize  = 0x8000;	// Exactly as it is in NTDLL.DLL
    bNotenough  = TRUE;
	
    while( bNotenough )
    {
        if ( dwBuffSize > 0x8000 * 20 ) // Too much, hopeless :(
            return NULL;
        
        if ( pInfoBuff = (PSYSTEM_INFO_DRIVERS)kmalloc ( dwBuffSize ) )
        {
            if ( STATUS_INFO_LENGTH_MISMATCH == ZwQuerySystemInformation( (ULONG)0x0b, pInfoBuff, dwBuffSize, NULL ) ) 
            {
                dwBuffSize += 0x8000;
                kfree ( pInfoBuff );
            } 
            else 
                bNotenough = FALSE;
        }
        else        
            return NULL;
    }
	
	for ( ULONG i = 0; i < pInfoBuff->NumberOfDrivers; ++i ) 
    {
        ULONG   len = (ULONG)strlen ( pInfoBuff->Drivers[i].PathName );
        
        if (0== _strnicmp(pInfoBuff->Drivers[i].PathName + len - ModuleNameLength, ModuleName, ModuleNameLength ) )
        {
            ModuleBase = (ULONG)pInfoBuff->Drivers[i].BaseAddress;
            break; 
        }
    }	

    if ( pInfoBuff )
        kfree(pInfoBuff);
	return ModuleBase;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

/* EOF */
//获得进程完整路径
BOOLEAN GetGargetEPbyName(UNICODE_STRING	*unitmi)
{

	NTSTATUS status;
	HANDLE hProcess=NULL;
	CLIENT_ID clientid;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG returnedLength; 
	ULONG bufferLength=0;    
	PVOID buffer=NULL;
	PSYSTEM_PROCESS_INFORMATION	ppi;
	
	BOOLEAN	Bret=FALSE;
	PEPROCESS	pEP;
		//获得大小
	status = ZwQuerySystemInformation(SystemProcessInformation, 0, 0, &returnedLength);

	
	if (status!=STATUS_INFO_LENGTH_MISMATCH)
	{
        return FALSE;
	}
	
	bufferLength = returnedLength +0x100;  
	
	buffer=ExAllocatePoolWithTag(PagedPool,returnedLength,'ipgD');
	
	if (buffer==NULL)
	{
		return FALSE;        
	}
	
	
	//获得文件镜像名
	status=ZwQuerySystemInformation(SystemProcessInformation,buffer,returnedLength,&returnedLength);
	
	if (!NT_SUCCESS(status)) 
	{
		kprintf("ZwQuerySystemInformation return bad \r\n");
		goto Exit;

	}
	
	ppi	=	(PSYSTEM_PROCESS_INFORMATION)buffer;


	if (RtlEqualUnicodeString(unitmi, &ppi->ImageName,FALSE))
	{
		kprintf("name %wZ\r\n", &ppi->ImageName);
		PsLookupProcessByProcessId(ppi->UniqueProcessId, &pEP);
		ObDereferenceObject(pEP);
		g_TargetEP	=	(ULONG)pEP;
		Bret	=	true;
		goto Exit;
	}

	while (ppi->NextEntryOffset)
	{
		ppi	=	(PSYSTEM_PROCESS_INFORMATION)(ppi->NextEntryOffset+(ULONG)ppi);
		if (RtlEqualUnicodeString(unitmi, &ppi->ImageName,FALSE))
		{
			PsLookupProcessByProcessId(ppi->UniqueProcessId, &pEP);
			ObDereferenceObject(pEP);
			g_TargetEP	=	(ULONG)pEP;
				Bret	=	true;
				goto Exit;
		}

	}
Exit:	
	ExFreePool(buffer);	
	return Bret;
}

