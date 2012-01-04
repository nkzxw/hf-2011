
#include "comhdr.h"

typedef struct _SERVICE_FILTER_INFO_TABLE{ 
	ULONG SSDTCnt; 
	ULONG SavedSSDTServiceAddress[1024];        //保存被Hook的SSDT函数的地址 
	ULONG ProxySSDTServiceAddress[1024];        //保存被Hook的SSDT函数对应的代理函数的地址 
	ULONG SavedShadowSSDTServiceAddress[1024];    //保存被Hook的ShadowSSDT函数的地址 
	ULONG ProxyShadowSSDTServiceAddress[1024];    //保存被Hook的ShadowSSDT函数对应的代理函数的地址 
	ULONG SwitchTableForSSDT[1024];                //保存SSDT Hook开关,决定该函数是否会被Hook 
	ULONG SwitchTableForShadowSSDT[1024];        //保存ShadowSSDT Hook开关,决定该函数是否会被Hook 
}SERVICE_FILTER_INFO_TABLE,*PSERVICE_FILTER_INFO_TABLE;
PSERVICE_FILTER_INFO_TABLE ServiceFilterInfoTable=NULL;  //记录系统所有SSDT函数hook相关信息


#pragma pack(1) //SSDT Table    
typedef struct ServiceDescriptorEntry 
{    
	PULONG ServiceTableBase;    
	PULONG ServiceCounterTableBase; 
	ULONG NumberOfServices;    
	PULONG ParamTableBase;    
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t; 
#pragma pack() 
#define FAKE_HANDLE ((HANDLE)0x10100101)
PServiceDescriptorTableEntry_t KeSSDT;
PServiceDescriptorTableEntry_t KeSSDTShadow=NULL;
ULONG	g_ProxyModuleBase=0;
ULONG	g_Proxy2OsDelta=0;
LONG	g_Proxy2win32Delta=0;
ULONG	g_osBase	=	0;
ULONG	g_Base2Relocation=0;
HANDLE hThread=NULL;
ULONG	g_ProcessNameOffset;

char		g_StrongOdSyS[512];
BOOLEAN		g_SodLoad	=	FALSE;
BOOLEAN		g_UnLoad		=	FALSE;
ULONG			g_uStrongOdDriverBase=0;
#define		FENGYUE_IAT_OFFSET	(0x0000903c)


//////////////////////////////////////////////////////////////////////////
void GetProcessNameOffset() 
{	
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

extern "C"
__declspec(dllimport) ServiceDescriptorTableEntry_t KeServiceDescriptorTable;


//获得SSDT基址宏    
#define SYSTEMSERVICE(_function)  KeSSDT->ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

/*
base+20
push ebp		//55 
call l1				//E8 00 00 00 00
l1:
  pop ebp		//5d

jmp ebp-xxx

jmpstub里面再 pop修复ebp
*/
UCHAR	g_ntjmpcode[]={0x55,0xe8,0x00,0x00,0x00,0x00,0x5d,0x8b,0x6d,0xf6,0xff,0xe5};

ULONG g_hookaddrs=0;//Hook代码填写的地址

ULONG g_myhookaddrs=0;//Hook代码填写的地址

BOOLEAN start_monitor=FALSE;
ULONG RetAdd=0;//Hook函数执行完后应返回的地址
CHAR  code[5]={0xe9,0,0,0,0};//写入hook地址的内容
HANDLE h_write_code=NULL;


typedef NTSTATUS (*g_RealZwSetEvent2)(HANDLE EventHandle,PLONG PreviousState );
g_RealZwSetEvent2 g_RealZwSetEvent;
//NTSTATUS (*g_RealZwSetEvent)(HANDLE EventHandle,PLONG PreviousState );
VOID UnHookZwSetEvent();
VOID HookZwEvent();
VOID HookKiFc_By_ZwSetEvent();
extern "C"
__declspec(dllimport) _stdcall KeAddSystemServiceTable(PVOID, PVOID, PVOID, PVOID, PVOID);

PServiceDescriptorTableEntry_t GetKeServiceDescriptorTableShadow(VOID)   
{   
	PServiceDescriptorTableEntry_t ShadowTable = NULL;   
	ULONG   ServiceTableAddress = 0;   
	PUCHAR  cPtr = NULL;    

	for (cPtr = (PUCHAR)KeAddSystemServiceTable;cPtr < (PUCHAR)KeAddSystemServiceTable + PAGE_SIZE;cPtr += 1 )   
	{   
		if (!MmIsAddressValid(cPtr))  continue;   

		ServiceTableAddress = *(PULONG)cPtr;   
		if (!MmIsAddressValid((PVOID)ServiceTableAddress)) continue;   

		if (memcmp((PVOID)ServiceTableAddress, (PVOID)&KeServiceDescriptorTable, 16) == 0)   
		{   
			if ((PVOID)ServiceTableAddress == &KeServiceDescriptorTable) continue;   
			ShadowTable = (PServiceDescriptorTableEntry_t)ServiceTableAddress;   
			ShadowTable ++;   
			return ShadowTable;   
		}   
	}   
	return NULL;   
}

PKPROCESS	g_pkprocess=NULL;
VOID GetSSDTAndShadowAddress(PVOID ctx)
{
	//遍历线程
	ULONG i;
	NTSTATUS status;
	ULONG thread;
	KeSSDT=&KeServiceDescriptorTable;
	ULONG	shadowEP;
Tag:
	for( i=8; i<32768; i++)
	{
		status = PsLookupThreadByThreadId( (HANDLE)i,(PETHREAD*)&thread);
		if(NT_SUCCESS(status))
		{
			if( ((PServiceDescriptorTableEntry_t)(*(PULONG)(thread+0x0e0)))->ServiceTableBase != KeServiceDescriptorTable.ServiceTableBase)
			{
				KeSSDTShadow=(PServiceDescriptorTableEntry_t)(*(PULONG)(thread+0x0e0));
			}

			if (KeSSDTShadow)
			{
				break;
			}
			ObDereferenceObject((PVOID)thread);
		}
	}
	if (!KeSSDTShadow)
	{
		KeSSDTShadow=GetKeServiceDescriptorTableShadow();
	}
	if (!KeSSDTShadow)
	{
		LARGE_INTEGER timeout={-10000*50};
		KeDelayExecutionThread(KernelMode,FALSE,&timeout);
		i=8;
		goto Tag;
	}

	
	long	icountservices	=	KeServiceDescriptorTable.NumberOfServices;
	int k=0;
	LONG	detla=	(LONG)g_ProxyModuleBase	-	(LONG)g_osBase;		//基地址的差值
	g_Proxy2OsDelta	=	detla;
	LONG	proxyAddr=0;
	//修复ssdt
	for (k=0; k<icountservices; k++)
	{
		proxyAddr	=	(LONG)KeServiceDescriptorTable.ServiceTableBase[k]+detla;
		ServiceFilterInfoTable->ProxySSDTServiceAddress[k]	=	proxyAddr;
		ServiceFilterInfoTable->SwitchTableForSSDT[k]=1;
	
	}
	
	CKl_HookModule1	ck1;
//	ck1.GetBase("\\WINDOWS\\system32\\win32k.sys");
	ULONG win32kBse	=	(ULONG)ck1.GetBase("win32k.sys");
	if (win32kBse==0)
	{
		kprintf(" GetBase win32k.sys; fail\r\n");
		return ;
	}
	g_Base2Relocation	=	win32kBse;
	ULONG	newWin32kBase	=	myloader("mywin32k", L"\\WINDOWS\\system32\\win32k.sys", NULL, 1, NULL);
	if (newWin32kBase==0)
	{
		kprintf( "my load win32k fail \r\n");
	}

	detla=	(LONG)newWin32kBase	-	(LONG)win32kBse;		//基地址的差值	,注意这里，一般win32k原来的地址比后来的大，所以这里是负值
	kprintf("new ntos  %X, new Win32k.sys  %X, NtDelta %X,  win32k delta %X\n", g_ProxyModuleBase, newWin32kBase, g_Proxy2OsDelta, win32kBse- newWin32kBase);
	g_Proxy2win32Delta	=	detla;

	KeAttachProcess((PRKPROCESS)g_pkprocess);
	//修复shadowSSDT
	LONG	addrTmp=0;
	icountservices	=	KeSSDTShadow->NumberOfServices;
	for (k=0; k<icountservices; k++)
	{
		
		addrTmp	=	(LONG)	KeSSDTShadow->ServiceTableBase[k];
		proxyAddr	=	addrTmp+	detla;

		ServiceFilterInfoTable->ProxyShadowSSDTServiceAddress[k]	=	(ULONG)proxyAddr;

		ServiceFilterInfoTable->SwitchTableForShadowSSDT[k]=1;
	
	}
	KeDetachProcess();
	HookKiFc_By_ZwSetEvent();
	kprintf("SSDT:0x%p,SSDTShadow:0x%p\n",KeSSDT,KeSSDTShadow);
//	ZwClose(hThread);
//	PsTerminateSystemThread(STATUS_SUCCESS);
	return ;
}

//关中断的目的是防止后面的写内存操作过程被中断，所以夹在下面两个函数之间的代码应尽量简短
VOID WPOFF()
{                   \
	__asm
	{
		cli
		push eax    
		mov  eax, cr0 
		and  eax, 0FFFEFFFFh 
		mov  cr0, eax 
		pop  eax 
	}    

}

VOID WPON() 
{                
	__asm
	{
		push eax 
		mov  eax, cr0 
		or   eax, NOT 0FFFEFFFFh
		mov  cr0, eax 
		pop  eax 
		sti 
	}	    

}
////////////////////////////////////////////////////////////////////////////////

ULONG  GetWindowsVersion()
{
	
	ULONG	dwMajorVersion;
	ULONG	dwMinorVersion;
	PsGetVersion(&dwMajorVersion, &dwMinorVersion, NULL, NULL);
	if (dwMajorVersion == 5 && dwMinorVersion == 0) 
	{
		
		kprintf("Window 2K \n");
		return Windows_2K;
		
	} else if (dwMajorVersion == 5 && dwMinorVersion == 1) {
		kprintf("Window XP \n");
		return Windows_XP;
	} else if (dwMajorVersion == 5 && dwMinorVersion == 2) {
        kprintf("Window 2003 \n");
		return Windows_2k3;	
	} else if (dwMajorVersion == 6 && dwMinorVersion == 0) 
	{
		kprintf("Window Vista \n");
		return Windows_Vista;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 1) {
		kprintf("Window 7 \n");
		return Windows_7;
	}
	
	return NULL;
}
////////////////////////////////////////////////////////////////////////////////
VOID WriteCode(PVOID ctx)
{
	LARGE_INTEGER li;li.QuadPart=-1000000*5;//0.5秒检测一次
	kprintf("Enter " __FUNCTION__ "");
	while(1)
	{
		if (g_hookaddrs && start_monitor)
		{
			if (memcmp((PVOID)g_hookaddrs,code,5)!=0)
			{
				kprintf("Rehook Address:%X",g_hookaddrs);
				WPOFF();
				RtlCopyMemory((PVOID)g_hookaddrs,code,5);
				WPON();
			}
		}
		KeDelayExecutionThread(KernelMode,FALSE,&li);
	}
	kprintf("Leave " __FUNCTION__ "");
}

PVOID ReplaceAPI(PVOID TargetAPI,PVOID NewAPI)
{
	PVOID OrignalAPI=NULL;
	KIRQL OldIrql=0;
	OldIrql=KeRaiseIrqlToDpcLevel();
    WPOFF();
	OrignalAPI=(PVOID)SYSTEMSERVICE(TargetAPI);
	SYSTEMSERVICE(TargetAPI)=(int)NewAPI;
	WPON();
	KeLowerIrql(OldIrql);
	return OrignalAPI;
}

//////////////////////////////////////////////////////////////////////////
typedef ULONG	(__stdcall  *PNtGetContextThread)(HANDLE          ThreadHandle, PCONTEXT    pContext);
#define		INDEX_NtSetContextThread	(213)
#define		INDEX_NtGetContextThread	(85)
PNtGetContextThread	g_NtGetContextThread;

/*__declspec(naked)*/ int	__stdcall  myNtGetContextThread(HANDLE          ThreadHandle, PCONTEXT    pContext)
{
		PNtGetContextThread	p	=	(PNtGetContextThread)ServiceFilterInfoTable->SavedSSDTServiceAddress[INDEX_NtGetContextThread];
		int	iRet	=	g_NtGetContextThread(ThreadHandle, pContext);	//调用原来的
//	pContext->Dr0	=	0;
//	pContext->Dr1	=	0;
//	pContext->Dr2	=	0;
//	pContext->Dr3	=	0;
//	pContext->Dr6	=	0;
//	DR7INFO	d7	=	*(DR7INFO*)&pContext->Dr7;
//	d7.G0	=	d7.G1	=	d7.G2	=	d7.G3	=0;
//	pContext->Dr7	= *(PULONG)&d7;
//	kprintf("g_NtGetContextThread 返回%X\r\n", iRet);?
		return iRet ;
}
//////////////////////////////////////////////////////////////////////////
__declspec(naked) void	__stdcall  DUMMY()
{
	_asm
	{
		push [esp+8]
		push [esp+4]
		call  g_NtGetContextThread
		add esp,8
		ret 
	}
}
//////////////////////////////////////////////////////////////////////////
__declspec(naked) ULONG	__stdcall  par2()
{
	_asm
	{
		mov eax, 0x0	//return Stuaus_success
			retn 8
	}
}
__declspec(naked) ULONG	__stdcall  par3()
{
	_asm
	{
		mov eax, 0x0	//return Stuaus_success
			retn 0xc
	}
}
__declspec(naked) ULONG	__stdcall  par4()
{
	_asm
	{
		mov eax, 0x0	//return Stuaus_success
		retn 0x10
	}
}

//////////////////////////////////////////////////////////////////////////
PUCHAR	g_TargetProName=(PUCHAR)"dnf.exe";
PUCHAR	g_TargetProName2=(PUCHAR)"zhengtu2.dat";
PUCHAR	g_TargetProName3=(PUCHAR)"zhengtu2.dat";
char*	g_BypassProName[]={"DragonNest.exe", "iagent.exe", "dnf.exe", "crossfire.exe",  "zhengtu2.dat"/*, "HProtect.exe"*/, "TenSafe.exe"};


/*
返回值:
0。使用原来的
1。使用新OS
2。使用伪造函数
*/
int 	bypass(ULONG ServiceIndex, ULONG OriginalServiceRoutine)
{

	char *currentEP	=	(char*)PsGetCurrentProcess();
	for (int k=0; k<sizeof(g_BypassProName)/sizeof(g_BypassProName[0]); k++)
	{
		if (currentEP==0)
		{
			continue;
		}
		if (_strnicmp((char*)currentEP+g_ProcessNameOffset,(char*)g_BypassProName[k],strlen((char*)g_BypassProName[k]))==0)
		{
			//
			if (ServiceIndex==INDEX_NtGetContextThread )
			{
				//其实应该检测NtGetContextThread 函数的参数Handle对应的eprocess才对的
				//		fake 
				//return 2;

				return -1;
			}
			// original 
			return 0;
		}
	}
	// new os
	return 1;

}
//////////////////////////////////////////////////////////////////////////
ULONG __stdcall MyKiFastCallEntryFilter(ULONG ServiceIndex, ULONG OriginalServiceRoutine, ULONG ServiceTable) 
{ 

	ULONG tmp_ssdt_base=0,tmp_ssdt_shadow_base=0;
	ULONG tmp_ssdt_count=0,tmp_ssdt_shadow_count=0;
	int iRet = bypass(ServiceIndex, OriginalServiceRoutine);
	if (iRet==0)
	{
		return OriginalServiceRoutine;
	}
   if (iRet==-1)
   {	//这个是目标（如：DNF。EXE,zhengtu.dat)进程的调用
	   g_NtGetContextThread	=	(PNtGetContextThread)OriginalServiceRoutine;
	   return (ULONG)myNtGetContextThread;
   }
	if(iRet>=2)
	{
		if (iRet==2)
		{
				return (ULONG)par2;
		}
		if (iRet==3)
		{
				return (ULONG)par3;
		}
	}
	// iret ==1
	if (KeSSDT )
	{
		tmp_ssdt_base=(ULONG)KeSSDT->ServiceTableBase;
		tmp_ssdt_count=KeSSDT->NumberOfServices;
		//SSDT中的调用 
		if ( (ServiceTable == tmp_ssdt_base) && (ServiceIndex<tmp_ssdt_count) )// 参数判断
		{ 

			if (ServiceFilterInfoTable->SwitchTableForSSDT[ServiceIndex] && ServiceFilterInfoTable->ProxySSDTServiceAddress[ServiceIndex]) 
			{ 
				
				ServiceFilterInfoTable->SavedSSDTServiceAddress[ServiceIndex]=OriginalServiceRoutine;//保存原始例程
	
				return ServiceFilterInfoTable->ProxySSDTServiceAddress[ServiceIndex];//返回代理函数的地址，实现hook相应函数
			} 
		} 
	}

	if (KeSSDTShadow)
	{
		tmp_ssdt_shadow_base=(ULONG)KeSSDTShadow->ServiceTableBase;
		tmp_ssdt_shadow_count=KeSSDTShadow->NumberOfServices;

		//ShadowSSDT中的调用
		if ( (ServiceTable==tmp_ssdt_shadow_base) && (ServiceIndex <tmp_ssdt_shadow_count)) 
		{ 
			if ( ServiceFilterInfoTable->SwitchTableForShadowSSDT[ServiceIndex] && ServiceFilterInfoTable->ProxyShadowSSDTServiceAddress[ServiceIndex]) 
			{ 
				ServiceFilterInfoTable->SavedShadowSSDTServiceAddress[ServiceIndex]=OriginalServiceRoutine; 
				return ServiceFilterInfoTable->ProxyShadowSSDTServiceAddress[ServiceIndex];  
			}                          
		} 
	}
	return OriginalServiceRoutine; // 否则，返回原始函数地址，不做任何处理
} 

//////////////////////////////////////////////////////////////////////////
ULONG	g_360jmp_back;
__declspec(naked) void	__stdcall  NakedJmpFor360()
{
	
	_asm
	{

// 			sub esp,0xc
// 			mov eax, [ebp+4]
// 			push [ebp+0x10]
// 			push [ebp+0xc]
// 			push [ebp+8]
// 			mov ebp, [ebp]	//原本的ebp
// 			push eax	//返回地址
			 push g_360jmp_back
	
			jmp MyKiFastCallEntryFilter
			
	}
}
//////////////////////////////////////////////////////////////////////////

__declspec(naked) JmpStubNt()
{
	__asm
	{
			pop ebp	//we push ebp when setup a thunk at nt header,so...have to pop ebp,to fix stack
		mov edi,edi;
		pushfd;
		pushad;
		push edi;
		push ebx;
		push eax;
		call MyKiFastCallEntryFilter;
		mov dword ptr[esp+10h],eax;
		popad;
		popfd;
		sub esp,ecx;//执行被替换的代码
		shr ecx,2;//执行被替换的代码
		push RetAdd;//设置返回地址
		
		ret;//返回到RetAdd所指地址
	}//此时，ebx的值即为MyKiFastCallEntry_0返回的函数地址,而ebx的值又是将被调用的ssdt函数的地址，因此也就达到了ssdt hook的效果
}


__declspec(naked) JmpStubWin7()
{
	__asm
	{

		mov edi,edi;
		pushfd;
		pushad;
		push edi;
		push edx;
		push eax;
		call MyKiFastCallEntryFilter;
		mov dword ptr[esp+14h],eax;	
		popad;
		popfd;
		sub esp,ecx;//执行被替换的代码
		shr ecx,2;//执行被替换的代码
		push RetAdd;//设置返回地址

		ret;//返回到RetAdd所指地址
	}
}


NTSTATUS MyZwSetEvent(HANDLE EventHandle,PLONG PreviousState)
{
	NTSTATUS status;
	char *tmp_byte_ptr0=NULL,*tmp_byte_ptr1=NULL;  
	char feature_code[]={0x2b,0xe1,0xc1,0xe9,0x02};//被填入hook代码(code[5])处的特征码，用于验证所找位置是否正确
	KIRQL  Irql; 
	

	if (EventHandle!=FAKE_HANDLE||ExGetPreviousMode()==UserMode)   //判断一下是不是自己调用,同时也不处理来自应用层的调用
	{ 
		if (g_RealZwSetEvent!=NULL)
		{
			status= g_RealZwSetEvent(EventHandle,PreviousState);
		}else
		{
			status=STATUS_ACCESS_DENIED;
		}
		 
	}else
	{
		
		UnHookZwSetEvent();
		__asm
		{
			mov eax,[ebp+4]
			mov g_hookaddrs,eax
		}
		/*while(MmIsAddressValid((PVOID)g_hookaddrs))
		{
		}*/
//		g_hookaddrs-=0x17;
		int itrytimes=0x30;
		BOOLEAN	bFound	=	FALSE;
		while(itrytimes-->0)
		{
			//WIN7系统中，偏移不是0X17，不过也在不远处，强行搜索
			if(!MmIsAddressValid((PVOID)g_hookaddrs))
				break;
			if (*(PULONG)g_hookaddrs==0xe9c1e12b)
			{
				bFound	=	TRUE;
				break;
			}
			g_hookaddrs--;
			
		}
		if (!bFound)
		{
						kprintf("Can not find the HookPort Address\r\n");
						status=STATUS_ACCESS_DENIED;
						return status;
		}
		RetAdd=g_hookaddrs+5;
		kprintf("OSbase %x, g_hookaddrs %x", g_osBase, g_hookaddrs);
		CKl_HookModule1	ck1;
			ULONG hookportBase	=	(ULONG)ck1.GetBase("hookport.sys");
		if (hookportBase==0)
		{
			kprintf("Get hookport.sys  !\r\n");
			g_hookaddrs=0;
			status=STATUS_ACCESS_DENIED;
		}
		if(!MmIsAddressValid((PVOID)RetAdd))
		{
			kprintf("Get Error g_hookaddrs!");
			g_hookaddrs=0;
			status=STATUS_ACCESS_DENIED;
		}else if(memcmp((PVOID)g_hookaddrs,feature_code,5)==0 || *(PUCHAR)g_hookaddrs==0xE9)
		{
			kprintf("RetAdd is %08X\n",RetAdd);
			kprintf("g_hookaddrs is %08X",g_hookaddrs);
			if (GetWindowsVersion()==Windows_7||GetWindowsVersion()==Windows_Vista)
			{
				*(PULONG)(code+1)=(ULONG)JmpStubWin7-g_hookaddrs-5;
			}
			else
			{
				//先JMP到NT 头
				*(PULONG)(code+1)=(ULONG)g_osBase+0x20-g_hookaddrs-5;
			}
			//写入跳转地址，之后code的汇编码为:jmp [JmpStub函数的相对地址]，替换掉了原来的
			//sub esp,ecx;
			//shr ecx,2
			Irql=KeRaiseIrqlToDpcLevel();
			WPOFF();
			//下面这块是当时临时跳到nt 模块的，为了躲避一些模块检测，不过发现征途这个游戏保护不行。
			//打算跳到360

// 			*(PULONG)(code+1)=(ULONG)MyKiFastCallEntryFilter - hookport_long_jmp_offset-hookportBase-5;
			PULONG	ntjmpstub=	(PULONG)(g_osBase+0x20-4);
			*ntjmpstub	=	(ULONG)JmpStubNt;
			//put up the shellcode
			RtlCopyMemory(ntjmpstub+1, g_ntjmpcode, sizeof(g_ntjmpcode));
			
			RtlCopyMemory((VOID*)g_hookaddrs,code,5);//jmp [JmpStub]


/*
这是利用360的HOOK
			ULONG	hookport_ret_hook_offset	=	0x00009c4c;
			g_360jmp_back	=	0x00009c66+hookportBase;
			PULONG	push_patch=	(PULONG)(hookport_ret_hook_offset + hookportBase);
			//put up the shellcode
			UCHAR self_jmp[]	=	{0x68,0x00,0x00,0x00,0x00};
			*(PULONG)(self_jmp+1)	=(ULONG)	NakedJmpFor360;

			RtlCopyMemory(push_patch, self_jmp, 5);
*/
			WPON();

			//这里加上HOOK 自己的proxymodule,因为API发现深入调用到里面后，又会回头通过自身的jmpstub 调用
			{
				g_myhookaddrs	=	g_hookaddrs+g_Proxy2OsDelta;
				if (GetWindowsVersion()==Windows_7||GetWindowsVersion()==Windows_Vista)
				{
					*(PULONG)(code+1)=(ULONG)JmpStubWin7-g_myhookaddrs-5;
				}
				else
				{
							//先JMP到NT 头
					*(PULONG)(code+1)=(ULONG)g_osBase+0x20-g_myhookaddrs-5;
				}
				WPOFF();
			PULONG	ntjmpstub=	(PULONG)(g_osBase+0x20-4);
			*ntjmpstub	=	(ULONG)JmpStubNt;
			//put up the shellcode
				RtlCopyMemory(ntjmpstub+1, g_ntjmpcode, sizeof(g_ntjmpcode));
				RtlCopyMemory((VOID*)g_myhookaddrs,code,5);//jmp [JmpStub]
				WPON();
			}
		KeLowerIrql(Irql);
		//	start_monitor=TRUE; //取消监控
		//	PsCreateSystemThread(&h_write_code,THREAD_ALL_ACCESS,NULL,NULL,NULL,WriteCode,NULL);
			status=STATUS_SUCCESS;
		}else
		{
			g_hookaddrs=0;
			RetAdd=0;
			kprintf(("Get Hook Address Error!\n"));
			status=STATUS_ACCESS_DENIED;
		}
 
	}
	return status;
}

VOID UnHookZwSetEvent()
{
	ReplaceAPI(ZwSetEvent,g_RealZwSetEvent);
	g_RealZwSetEvent=NULL;
}

VOID HookZwEvent()
{
	g_RealZwSetEvent=(g_RealZwSetEvent2)ReplaceAPI(ZwSetEvent,MyZwSetEvent);
	while(1)
	{
		if (ZwSetEvent(FAKE_HANDLE,NULL)>=0)
		{
			break;
		}else
		{
			continue;;
		}
	}
}


VOID HookKiFc_By_ZwSetEvent()
{
	HookZwEvent();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//获取系统当前使用的module名字，并返回base address
//返回1表示给的参数长度不够
ULONG GetOsMoudleName(char* ModuleName, ULONG uLen)
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
	}
	else
	{
		ModuleBase	=	1;
	}
    if ( pInfoBuff )
        kfree(pInfoBuff);
	return ModuleBase;
}
//////////////////////////////////////////////////////////////////////////
#define DECL_DYNCFUN_HOOK_IAT(x,y,z) \
{#x,0,#y,0,0,0,z}


IATHOOKINFO g_IATHookInfo2NewOS[]=
{
		DECL_DYNCFUN_HOOK_IAT(win32k.sys, KeAttachProcess,0),
		DECL_DYNCFUN_HOOK_IAT(win32k.sys, MmMapViewOfSection,0),
		DECL_DYNCFUN_HOOK_IAT(win32k.sys, MmUnmapViewOfSection,0),

		DECL_DYNCFUN_HOOK_IAT(fengyue, KeUnstackDetachProcess,1),
		DECL_DYNCFUN_HOOK_IAT(fengyue, KeStackAttachProcess,1),
		DECL_DYNCFUN_HOOK_IAT(fengyue, RtlFreeUnicodeString,1),
		DECL_DYNCFUN_HOOK_IAT(fengyue, ZwClose,1),
		DECL_DYNCFUN_HOOK_IAT(fengyue, ZwUnmapViewOfSection,1),
		DECL_DYNCFUN_HOOK_IAT(fengyue, ZwMapViewOfSection,1),

};
/*
fengyue0.sys,要patch下这些导入表。发现这几个位置挺固定的
lkd> dps 00009040+0xB08E3000-10 l10
b08ec030  8050a44e nt!MmUnmapLockedPages
b08ec034  804f05ac nt!IoFreeMdl
b08ec038  8054c2e0 nt!ExFreePoolWithTag
b08ec03c  804f96e4 nt!KeUnstackDetachProcess
b08ec040  804f9c32 nt!KeStackAttachProcess
b08ec044  805e2aa4 nt!RtlFreeUnicodeString
b08ec048  80500f30 nt!ZwClose
b08ec04c  80502218 nt!ZwUnmapViewOfSection
b08ec050  805015ac nt!ZwMapViewOfSection
*/
//////////////////////////////////////////////////////////////////////////

NTSTATUS	UnHookPort()
{
	LARGE_INTEGER inteval;
	KIRQL  Irql; 
	kprintf("Enter UnHookPort()");
	inteval.QuadPart = -20000000;
	if (g_hookaddrs)
	{
		KeAttachProcess((PRKPROCESS)g_pkprocess);
		Irql	=	KeRaiseIrqlToDpcLevel();
		WPOFF();
		for (int idx=0; idx< sizeof(g_IATHookInfo2NewOS)/sizeof(g_IATHookInfo2NewOS[0]) ; idx++)
		{

			if(g_IATHookInfo2NewOS[idx].pfTargetFunAddressLocation!=0&&g_IATHookInfo2NewOS[idx].HookType==0)
			{
				kprintf("UnHook g_IATHookInfo2NewOS %s at %X\r\n", g_IATHookInfo2NewOS[idx].pFunName, g_IATHookInfo2NewOS[idx].pfTargetFunAddressLocation);
				*(PULONG)g_IATHookInfo2NewOS[idx].pfTargetFunAddressLocation	=	g_IATHookInfo2NewOS[idx].pfTargetFunAddressValue;
			}
		}
		
		memcpy((PUCHAR)g_hookaddrs, "\x2b\xe1\xc1\xe9\x02", 5);
		WPON();
		KeLowerIrql(Irql);
		KeDetachProcess();
	}
	//释放内存，暂时不free了。怕是有函数pendding在里面。返回就崩溃了
//	ExFreePoolWithTag(ServiceFilterInfoTable, 'GTRZ');
	g_UnLoad	=	TRUE;
	KeDelayExecutionThread(KernelMode,FALSE,&inteval);	//有可能有函数还在里面，卸载掉内存就会出错。
	kprintf("Leave UnHookPort()");
	return 0;
	////如果有函数正在执行到我们的副本中，这时候退出，会BSOD，所以不应该释放副本的内存，就留着吧
}

////////////////////////////////////////////////////////////////////////////////

VOID ThreadPatchSOD(PVOID ctx)
{
		LARGE_INTEGER li;li.QuadPart=-1000000*5*4;//0.5*4=2秒检测一次
		LARGE_INTEGER li2;li2.QuadPart=-1000000*5*8;//0.5*6=3秒
		ULONG	item2=0;
		PULONG	pPatchAddress=0;
		while(1)
		{
			if (g_UnLoad	==TRUE)
			{
					kprintf("ThreadPatchSOD Exit\r\n");
					PsTerminateSystemThread(STATUS_SUCCESS);
					return ;
			}
			if (g_SodLoad)
			{
				//发现加载进来了,等待8秒，让strongod的驱动取得了那些地址再patch
					KeDelayExecutionThread(KernelMode,FALSE,&li2);
					g_SodLoad	=	FALSE;
					WPOFF();
					pPatchAddress	=	(PULONG)(g_uStrongOdDriverBase+FENGYUE_IAT_OFFSET);
					ULONG	itmp4=0;
					for (int idx=0; idx< sizeof(g_IATHookInfo2NewOS)/sizeof(g_IATHookInfo2NewOS[0]) ; idx++)
					{
						
						//只处理SOD的fengyue.sys驱动的patch
						if (g_IATHookInfo2NewOS[idx].HookType==0)
						{
							continue;
						}

							itmp4	=	*pPatchAddress;
							*pPatchAddress	=	g_Proxy2OsDelta+itmp4;
							kprintf("Replacing strongOD's  %s at %X\r\n", g_IATHookInfo2NewOS[idx].pFunName, pPatchAddress);
							pPatchAddress	=	pPatchAddress+1;
					}
					WPON();
					

			}
			else
			{
					KeDelayExecutionThread(KernelMode,FALSE,&li);
			}
		}
	
}
//////////////////////////////////////////////////////////////////////////


NTSTATUS	InitHookPort()
{
	
	kprintf(("Enter " __FUNCTION__ "\n"));
	char ModuleName[1024];
	g_osBase	=	GetOsMoudleName(ModuleName, 1024);
	kprintf("Current Os Module :%s",ModuleName);
	if (g_osBase<2)
	{
		return 1;
	}
	ANSI_STRING	ansitmp;
	UNICODE_STRING	ustrtmp1;

	RtlInitAnsiString(&ansitmp, ModuleName);
	RtlAnsiStringToUnicodeString(&ustrtmp1, &ansitmp, 1);
	wchar_t wname[1024]={0};
	
	RtlMoveMemory(wname, ustrtmp1.Buffer, ustrtmp1.Length);
	wname[ustrtmp1.Length/2]=L'\0';
	g_Base2Relocation	=	g_osBase;
	ULONG	loadbase = myloader("myntos", wname, NULL,1, NULL);
	RtlFreeUnicodeString(&ustrtmp1);	//要释放

	g_ProxyModuleBase	=	loadbase;
	if (loadbase <MmUserProbeAddress)
	{
		kprintf(" loadder fail");
		return STATUS_ACCESS_DENIED;
	}

	//KeSSDTShadow=(PServiceDescriptorTableEntry_t)GetSSDTShadowAddress();
	ServiceFilterInfoTable=(PSERVICE_FILTER_INFO_TABLE)ExAllocatePoolWithTag(NonPagedPool,sizeof(SERVICE_FILTER_INFO_TABLE),'GTRZ');
	if (!ServiceFilterInfoTable)
	{
		return STATUS_ACCESS_DENIED;
	}
	RtlZeroMemory(ServiceFilterInfoTable,sizeof(SERVICE_FILTER_INFO_TABLE));
	//HookKiFc_By_ZwSetEvent();

	g_pkprocess	=	PsGetCurrentProcess();
	PsCreateSystemThread(&hThread,THREAD_ALL_ACCESS,NULL,NULL,NULL,(PKSTART_ROUTINE)GetSSDTAndShadowAddress,NULL);
	LARGE_INTEGER inteval;
	inteval.QuadPart = -20000000;
	while (!g_hookaddrs)
	{
				//等待成功HOOK上
				kprintf("等待成功HOOK上才返回给r3");
				KeDelayExecutionThread(KernelMode,FALSE,&inteval);
	}
	CKl_HookModule1	ck1;
	//	ck1.GetBase("\\WINDOWS\\system32\\win32k.sys");
	ULONG win32kBse	=	(ULONG)ck1.GetBase("win32k.sys");
	KeAttachProcess((PRKPROCESS)g_pkprocess);
	KIRQL Irql	=	KeRaiseIrqlToDpcLevel();
	WPOFF();
	//转换win32的导入表函数
	char *pOsFileName	=	strrchr(ModuleName,'\\')+1;
	for (int idx=0; idx< sizeof(g_IATHookInfo2NewOS)/sizeof(g_IATHookInfo2NewOS[0]) ; idx++)
	{

		//SOD的fengyue.sys驱动的patch要跳过，放在loadimage回调中
		if (g_IATHookInfo2NewOS[idx].HookType==1)
		{
				continue;
		}
		IATHookWin32(win32kBse, pOsFileName, g_IATHookInfo2NewOS[idx].pFunName, g_Proxy2OsDelta, 
			g_IATHookInfo2NewOS[idx].pfTargetFunAddressLocation,g_IATHookInfo2NewOS[idx].pfTargetFunAddressValue );
		if (g_IATHookInfo2NewOS[idx].pfTargetFunAddressLocation==0)
		{
			kprintf("IATHookWin32  %s fail\r\n", g_IATHookInfo2NewOS[idx].pFunName);
		}
		else
		{
				kprintf("IATHookWin32  %s  at %X\r\n", g_IATHookInfo2NewOS[idx].pFunName, g_IATHookInfo2NewOS[idx].pfTargetFunAddressLocation);
		}

	}
	WPON();
	KeLowerIrql(Irql);
	KeDetachProcess();
	g_UnLoad	=FALSE;
	PsCreateSystemThread(&hThread,THREAD_ALL_ACCESS,NULL,NULL,NULL,(PKSTART_ROUTINE)ThreadPatchSOD,NULL);
//	kprintf("set proxy ssdt done ,base delta=%x,proxy base= %x, os base=%x, count :%d", detla, g_ProxyModuleBase, g_osBase,icountservices);

	kprintf(("Leave " __FUNCTION__ "\n"));
	return STATUS_SUCCESS;
}


void IATHookWin32(ULONG hMod ,  char *ModuleNameDst, char *pFunName, ULONG pfNewFunAddress, ULONG &pfnOriAddressLocation, ULONG &pfnOriAddress)
{


	pfnOriAddressLocation=0;
	pfnOriAddress	=	0;
  if (hMod == 0 )
  {
    return ;
  }

  //得到DOS头

  PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hMod ; 

  //如果DOS头无效
  if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
  {
    return ; 
  }

  //得到NT头

  PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((ULONG)hMod + pDosHeader->e_lfanew);

  //如果NT头无效
  if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
  {
    return ; 
  }

  //检查输入表数据目录是否存在
  if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0 ||
    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0 )
  {
    return ; 
  }
  //得到输入表描述指针

  PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG)hMod + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

  PIMAGE_THUNK_DATA ThunkData ; 

  //检查每个输入项
  while(ImportDescriptor->FirstThunk)
  {
    //检查输入表项是否为ntdll.dll

    char* dllname = (char*)((ULONG)hMod + ImportDescriptor->Name);
    
    //如果不是，则跳到下一个处理
 //   if (_stricmp(dllname , ModuleNameDst) !=0)
    if (_strnicmp(dllname , "nt",2) !=0)
    {
      ImportDescriptor ++ ; 
      continue;
    }
    
    ThunkData = (PIMAGE_THUNK_DATA)((ULONG)hMod + (ULONG) ImportDescriptor->OriginalFirstThunk);

    int no = 1;
    while(ThunkData->u1.Function)
    {

      char* functionname = (char*)((ULONG)0 + (ULONG)ThunkData->u1.AddressOfData + 2);
      if (_stricmp(functionname , pFunName) == 0 )
      {
        //
        //如果是，那么记录原始函数地址
        //HOOK我们的函数地址
        //
        ULONG myaddr2Detla = (ULONG)pfNewFunAddress;
        ULONG btw ; 
        PULONG lpAddr = (ULONG *)((ULONG)hMod + (DWORD)ImportDescriptor->FirstThunk) +(no-1);
		pfnOriAddressLocation	=	(ULONG)lpAddr;	//IAT的位置
        pfnOriAddress= *lpAddr ; //这个是IAT位置的原始值
		*lpAddr	=	myaddr2Detla+*lpAddr;	//替换
 //       WriteProcessMemory(GetCurrentProcess() , lpAddr , &myaddr , sizeof(ULONG), &btw );
        return ; 

      }

      no++;
      ThunkData ++;
    }
    ImportDescriptor ++;
  }
  return ; 
}



//////////////////////////////////////////////////////////////////////////
VOID LoadImageNotify(
					 __in_opt PUNICODE_STRING  FullImageName,
					 __in HANDLE  ProcessId,
					 __in PIMAGE_INFO  ImageInfo
					 )
{
	
	UNICODE_STRING	str1;
	ANSI_STRING			astr1,astr2;
	RtlInitAnsiString(&astr1, g_StrongOdSyS);
	RtlAnsiStringToUnicodeString(&str1, &astr1, 1);
	RtlUnicodeStringToAnsiString(&astr2, FullImageName,1);
	WCHAR	*pImageName	=wcsrchr(FullImageName->Buffer, L'\\')+1;
	if (_wcsnicmp(pImageName, str1.Buffer, str1.Length)!=0)
	{
		RtlFreeUnicodeString(&str1);
		RtlFreeAnsiString(&astr2);
		return ;
	}

	astr2.Buffer[astr2.Length]=0;//截断
	kprintf("Image Identify %s\r\n", astr2.Buffer);
	 g_uStrongOdDriverBase	=	(ULONG)ImageInfo->ImageBase;
	g_SodLoad	=	TRUE;
	RtlFreeUnicodeString(&str1);
	RtlFreeAnsiString(&astr2);

	return ;


}