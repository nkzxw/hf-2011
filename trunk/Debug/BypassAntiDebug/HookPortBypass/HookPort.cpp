
#include "comhdr.h"

typedef struct _SERVICE_FILTER_INFO_TABLE{ 
	ULONG SSDTCnt; 
	ULONG SavedSSDTServiceAddress[1024];        //���汻Hook��SSDT�����ĵ�ַ 
	ULONG ProxySSDTServiceAddress[1024];        //���汻Hook��SSDT������Ӧ�Ĵ������ĵ�ַ 
	ULONG SavedShadowSSDTServiceAddress[1024];    //���汻Hook��ShadowSSDT�����ĵ�ַ 
	ULONG ProxyShadowSSDTServiceAddress[1024];    //���汻Hook��ShadowSSDT������Ӧ�Ĵ������ĵ�ַ 
	ULONG SwitchTableForSSDT[1024];                //����SSDT Hook����,�����ú����Ƿ�ᱻHook 
	ULONG SwitchTableForShadowSSDT[1024];        //����ShadowSSDT Hook����,�����ú����Ƿ�ᱻHook 
}SERVICE_FILTER_INFO_TABLE,*PSERVICE_FILTER_INFO_TABLE;
PSERVICE_FILTER_INFO_TABLE ServiceFilterInfoTable=NULL;  //��¼ϵͳ����SSDT����hook�����Ϣ


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


//���SSDT��ַ��    
#define SYSTEMSERVICE(_function)  KeSSDT->ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

/*
base+20
push ebp		//55 
call l1				//E8 00 00 00 00
l1:
  pop ebp		//5d

jmp ebp-xxx

jmpstub������ pop�޸�ebp
*/
UCHAR	g_ntjmpcode[]={0x55,0xe8,0x00,0x00,0x00,0x00,0x5d,0x8b,0x6d,0xf6,0xff,0xe5};

ULONG g_hookaddrs=0;//Hook������д�ĵ�ַ

ULONG g_myhookaddrs=0;//Hook������д�ĵ�ַ

BOOLEAN start_monitor=FALSE;
ULONG RetAdd=0;//Hook����ִ�����Ӧ���صĵ�ַ
CHAR  code[5]={0xe9,0,0,0,0};//д��hook��ַ������
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
	//�����߳�
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
	LONG	detla=	(LONG)g_ProxyModuleBase	-	(LONG)g_osBase;		//����ַ�Ĳ�ֵ
	g_Proxy2OsDelta	=	detla;
	LONG	proxyAddr=0;
	//�޸�ssdt
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

	detla=	(LONG)newWin32kBase	-	(LONG)win32kBse;		//����ַ�Ĳ�ֵ	,ע�����һ��win32kԭ���ĵ�ַ�Ⱥ����Ĵ����������Ǹ�ֵ
	kprintf("new ntos  %X, new Win32k.sys  %X, NtDelta %X,  win32k delta %X\n", g_ProxyModuleBase, newWin32kBase, g_Proxy2OsDelta, win32kBse- newWin32kBase);
	g_Proxy2win32Delta	=	detla;

	KeAttachProcess((PRKPROCESS)g_pkprocess);
	//�޸�shadowSSDT
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

//���жϵ�Ŀ���Ƿ�ֹ�����д�ڴ�������̱��жϣ����Լ���������������֮��Ĵ���Ӧ�������
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
	LARGE_INTEGER li;li.QuadPart=-1000000*5;//0.5����һ��
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
		int	iRet	=	g_NtGetContextThread(ThreadHandle, pContext);	//����ԭ����
//	pContext->Dr0	=	0;
//	pContext->Dr1	=	0;
//	pContext->Dr2	=	0;
//	pContext->Dr3	=	0;
//	pContext->Dr6	=	0;
//	DR7INFO	d7	=	*(DR7INFO*)&pContext->Dr7;
//	d7.G0	=	d7.G1	=	d7.G2	=	d7.G3	=0;
//	pContext->Dr7	= *(PULONG)&d7;
//	kprintf("g_NtGetContextThread ����%X\r\n", iRet);?
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
����ֵ:
0��ʹ��ԭ����
1��ʹ����OS
2��ʹ��α�캯��
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
				//��ʵӦ�ü��NtGetContextThread �����Ĳ���Handle��Ӧ��eprocess�ŶԵ�
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
   {	//�����Ŀ�꣨�磺DNF��EXE,zhengtu.dat)���̵ĵ���
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
		//SSDT�еĵ��� 
		if ( (ServiceTable == tmp_ssdt_base) && (ServiceIndex<tmp_ssdt_count) )// �����ж�
		{ 

			if (ServiceFilterInfoTable->SwitchTableForSSDT[ServiceIndex] && ServiceFilterInfoTable->ProxySSDTServiceAddress[ServiceIndex]) 
			{ 
				
				ServiceFilterInfoTable->SavedSSDTServiceAddress[ServiceIndex]=OriginalServiceRoutine;//����ԭʼ����
	
				return ServiceFilterInfoTable->ProxySSDTServiceAddress[ServiceIndex];//���ش������ĵ�ַ��ʵ��hook��Ӧ����
			} 
		} 
	}

	if (KeSSDTShadow)
	{
		tmp_ssdt_shadow_base=(ULONG)KeSSDTShadow->ServiceTableBase;
		tmp_ssdt_shadow_count=KeSSDTShadow->NumberOfServices;

		//ShadowSSDT�еĵ���
		if ( (ServiceTable==tmp_ssdt_shadow_base) && (ServiceIndex <tmp_ssdt_shadow_count)) 
		{ 
			if ( ServiceFilterInfoTable->SwitchTableForShadowSSDT[ServiceIndex] && ServiceFilterInfoTable->ProxyShadowSSDTServiceAddress[ServiceIndex]) 
			{ 
				ServiceFilterInfoTable->SavedShadowSSDTServiceAddress[ServiceIndex]=OriginalServiceRoutine; 
				return ServiceFilterInfoTable->ProxyShadowSSDTServiceAddress[ServiceIndex];  
			}                          
		} 
	}
	return OriginalServiceRoutine; // ���򣬷���ԭʼ������ַ�������κδ���
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
// 			mov ebp, [ebp]	//ԭ����ebp
// 			push eax	//���ص�ַ
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
		sub esp,ecx;//ִ�б��滻�Ĵ���
		shr ecx,2;//ִ�б��滻�Ĵ���
		push RetAdd;//���÷��ص�ַ
		
		ret;//���ص�RetAdd��ָ��ַ
	}//��ʱ��ebx��ֵ��ΪMyKiFastCallEntry_0���صĺ�����ַ,��ebx��ֵ���ǽ������õ�ssdt�����ĵ�ַ�����Ҳ�ʹﵽ��ssdt hook��Ч��
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
		sub esp,ecx;//ִ�б��滻�Ĵ���
		shr ecx,2;//ִ�б��滻�Ĵ���
		push RetAdd;//���÷��ص�ַ

		ret;//���ص�RetAdd��ָ��ַ
	}
}


NTSTATUS MyZwSetEvent(HANDLE EventHandle,PLONG PreviousState)
{
	NTSTATUS status;
	char *tmp_byte_ptr0=NULL,*tmp_byte_ptr1=NULL;  
	char feature_code[]={0x2b,0xe1,0xc1,0xe9,0x02};//������hook����(code[5])���������룬������֤����λ���Ƿ���ȷ
	KIRQL  Irql; 
	

	if (EventHandle!=FAKE_HANDLE||ExGetPreviousMode()==UserMode)   //�ж�һ���ǲ����Լ�����,ͬʱҲ����������Ӧ�ò�ĵ���
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
			//WIN7ϵͳ�У�ƫ�Ʋ���0X17������Ҳ�ڲ�Զ����ǿ������
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
				//��JMP��NT ͷ
				*(PULONG)(code+1)=(ULONG)g_osBase+0x20-g_hookaddrs-5;
			}
			//д����ת��ַ��֮��code�Ļ����Ϊ:jmp [JmpStub��������Ե�ַ]���滻����ԭ����
			//sub esp,ecx;
			//shr ecx,2
			Irql=KeRaiseIrqlToDpcLevel();
			WPOFF();
			//��������ǵ�ʱ��ʱ����nt ģ��ģ�Ϊ�˶��һЩģ���⣬����������;�����Ϸ�������С�
			//��������360

// 			*(PULONG)(code+1)=(ULONG)MyKiFastCallEntryFilter - hookport_long_jmp_offset-hookportBase-5;
			PULONG	ntjmpstub=	(PULONG)(g_osBase+0x20-4);
			*ntjmpstub	=	(ULONG)JmpStubNt;
			//put up the shellcode
			RtlCopyMemory(ntjmpstub+1, g_ntjmpcode, sizeof(g_ntjmpcode));
			
			RtlCopyMemory((VOID*)g_hookaddrs,code,5);//jmp [JmpStub]


/*
��������360��HOOK
			ULONG	hookport_ret_hook_offset	=	0x00009c4c;
			g_360jmp_back	=	0x00009c66+hookportBase;
			PULONG	push_patch=	(PULONG)(hookport_ret_hook_offset + hookportBase);
			//put up the shellcode
			UCHAR self_jmp[]	=	{0x68,0x00,0x00,0x00,0x00};
			*(PULONG)(self_jmp+1)	=(ULONG)	NakedJmpFor360;

			RtlCopyMemory(push_patch, self_jmp, 5);
*/
			WPON();

			//�������HOOK �Լ���proxymodule,��ΪAPI����������õ�������ֻ��ͷͨ�������jmpstub ����
			{
				g_myhookaddrs	=	g_hookaddrs+g_Proxy2OsDelta;
				if (GetWindowsVersion()==Windows_7||GetWindowsVersion()==Windows_Vista)
				{
					*(PULONG)(code+1)=(ULONG)JmpStubWin7-g_myhookaddrs-5;
				}
				else
				{
							//��JMP��NT ͷ
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
		//	start_monitor=TRUE; //ȡ�����
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
//��ȡϵͳ��ǰʹ�õ�module���֣�������base address
//����1��ʾ���Ĳ������Ȳ���
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
fengyue0.sys,Ҫpatch����Щ����������⼸��λ��ͦ�̶���
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
	//�ͷ��ڴ棬��ʱ��free�ˡ������к���pendding�����档���ؾͱ�����
//	ExFreePoolWithTag(ServiceFilterInfoTable, 'GTRZ');
	g_UnLoad	=	TRUE;
	KeDelayExecutionThread(KernelMode,FALSE,&inteval);	//�п����к����������棬ж�ص��ڴ�ͻ����
	kprintf("Leave UnHookPort()");
	return 0;
	////����к�������ִ�е����ǵĸ����У���ʱ���˳�����BSOD�����Բ�Ӧ���ͷŸ������ڴ棬�����Ű�
}

////////////////////////////////////////////////////////////////////////////////

VOID ThreadPatchSOD(PVOID ctx)
{
		LARGE_INTEGER li;li.QuadPart=-1000000*5*4;//0.5*4=2����һ��
		LARGE_INTEGER li2;li2.QuadPart=-1000000*5*8;//0.5*6=3��
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
				//���ּ��ؽ�����,�ȴ�8�룬��strongod������ȡ������Щ��ַ��patch
					KeDelayExecutionThread(KernelMode,FALSE,&li2);
					g_SodLoad	=	FALSE;
					WPOFF();
					pPatchAddress	=	(PULONG)(g_uStrongOdDriverBase+FENGYUE_IAT_OFFSET);
					ULONG	itmp4=0;
					for (int idx=0; idx< sizeof(g_IATHookInfo2NewOS)/sizeof(g_IATHookInfo2NewOS[0]) ; idx++)
					{
						
						//ֻ����SOD��fengyue.sys������patch
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
	RtlFreeUnicodeString(&ustrtmp1);	//Ҫ�ͷ�

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
				//�ȴ��ɹ�HOOK��
				kprintf("�ȴ��ɹ�HOOK�ϲŷ��ظ�r3");
				KeDelayExecutionThread(KernelMode,FALSE,&inteval);
	}
	CKl_HookModule1	ck1;
	//	ck1.GetBase("\\WINDOWS\\system32\\win32k.sys");
	ULONG win32kBse	=	(ULONG)ck1.GetBase("win32k.sys");
	KeAttachProcess((PRKPROCESS)g_pkprocess);
	KIRQL Irql	=	KeRaiseIrqlToDpcLevel();
	WPOFF();
	//ת��win32�ĵ������
	char *pOsFileName	=	strrchr(ModuleName,'\\')+1;
	for (int idx=0; idx< sizeof(g_IATHookInfo2NewOS)/sizeof(g_IATHookInfo2NewOS[0]) ; idx++)
	{

		//SOD��fengyue.sys������patchҪ����������loadimage�ص���
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

  //�õ�DOSͷ

  PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hMod ; 

  //���DOSͷ��Ч
  if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
  {
    return ; 
  }

  //�õ�NTͷ

  PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((ULONG)hMod + pDosHeader->e_lfanew);

  //���NTͷ��Ч
  if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
  {
    return ; 
  }

  //������������Ŀ¼�Ƿ����
  if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0 ||
    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0 )
  {
    return ; 
  }
  //�õ����������ָ��

  PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG)hMod + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

  PIMAGE_THUNK_DATA ThunkData ; 

  //���ÿ��������
  while(ImportDescriptor->FirstThunk)
  {
    //�����������Ƿ�Ϊntdll.dll

    char* dllname = (char*)((ULONG)hMod + ImportDescriptor->Name);
    
    //������ǣ���������һ������
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
        //����ǣ���ô��¼ԭʼ������ַ
        //HOOK���ǵĺ�����ַ
        //
        ULONG myaddr2Detla = (ULONG)pfNewFunAddress;
        ULONG btw ; 
        PULONG lpAddr = (ULONG *)((ULONG)hMod + (DWORD)ImportDescriptor->FirstThunk) +(no-1);
		pfnOriAddressLocation	=	(ULONG)lpAddr;	//IAT��λ��
        pfnOriAddress= *lpAddr ; //�����IATλ�õ�ԭʼֵ
		*lpAddr	=	myaddr2Detla+*lpAddr;	//�滻
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

	astr2.Buffer[astr2.Length]=0;//�ض�
	kprintf("Image Identify %s\r\n", astr2.Buffer);
	 g_uStrongOdDriverBase	=	(ULONG)ImageInfo->ImageBase;
	g_SodLoad	=	TRUE;
	RtlFreeUnicodeString(&str1);
	RtlFreeAnsiString(&astr2);

	return ;


}