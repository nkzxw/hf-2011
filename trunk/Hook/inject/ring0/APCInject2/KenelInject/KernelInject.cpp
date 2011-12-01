#include "APCFunc.h"
#include "MoudleFunc.h"
#include <ntddk.h>

#define PAGEDCODE code_seg("PAGE")
#define INITCODE  code_seg("INIT")
#define MAX_PID 65535	//����ID���ֵ
//=======================================================================================//
// ��������Ҫ�õ���ȫ�ֱ���																//
//======================================================================================//
PMDL pMdl = NULL;					//�ڴ���������
PVOID g_pFunctionAddress = NULL;	//ϵͳmoudle���������ĵ�ַ

//=======================================================================================//
// InjectDll��������,�õ����������ҵ�������Ҫע��Ľ���,�õ����̺�����߳�.�������߳�   //
//ע������ethread������XP..����������ϵͳ����һЩƫ�Ʋ�ͬ                               //
//������Ҫ�޸ģ�����������������������������
//======================================================================================//

void InjectDll(const char* DllFullPath,LPSTR ProcessName)
{
	//ȫ������ΪULONG����
	ULONG pTargetProcess;                //Ŀ�����
	ULONG pTargetThread;                 //Ŀ���߳�
	ULONG pNotAlertableThread;           //�Ǿ����߳� 
	ULONG pSystemProcess;                //ϵͳ����
	ULONG pTempThread;                   //��ʱ�߳�
	ULONG pNextEntry, pListHead, pThNextEntry,pThListHead; 
	ULONG pid;							//����ID
	PEPROCESS EProcess;
	NTSTATUS status;

	//�������̣���pid����
	for(pid=0; pid<MAX_PID; pid+=4)
	{  
		//����pid���ؽ��̵�PEPROCESS�ṹ��ָ��
		status = PsLookupProcessByProcessId((HANDLE)pid,&EProcess);
		if((NT_SUCCESS(status)))
		{
			//�ȽϽ�����
			if(_stricmp((const char*)PsGetProcessImageFileName(EProcess),ProcessName)==0)
			{	
				//process found!
				pSystemProcess = (ULONG)EProcess;
				pTargetProcess = pSystemProcess; 
				pTargetThread = pNotAlertableThread = 0;
				//ThreadListHead 
				//eprocess:  XP 0x190 win7 0x188 
				//kprocess:  eprocess��һ���֣�
				//ThreadListHead��ƫ��0x50 
				pThListHead = pSystemProcess+0x50;	
				pThNextEntry = *(ULONG *)pThListHead;	//next thread �ĸ��ֽ�

				//����˫������ThreadList
				while(pThNextEntry != pThListHead)
				{
					pTempThread =pThNextEntry-0x1b0;  //ETHREAD????
					//Alertable 0x164
					if(*(char *)(pTempThread+0x164)) //�����߳�
					{
						pTargetThread =pTempThread;
						//DbgPrint("KernelInject -> Found alertable thread");
						break;
					}
					else
					{
						pNotAlertableThread =pTempThread;
					}

					pThNextEntry = *(ULONG *)pThNextEntry; 
				}
				break;	
			}

		}
	}

	if(!pTargetProcess){
		//DbgPrint("KernelInject -> Couldn't find explorer.exe!");
		return ;
	}

	if(!pTargetThread)
		pTargetThread = pNotAlertableThread;


	if(pTargetThread)
	{
		//DbgPrint("KernelImject -> Target thread: 0x%p",pTargetThread);
		InstallUserModeApc(DllFullPath,pTargetThread,pTargetProcess);
	}
	else
		//DbgPrint("KernelInject -> No thread found!")
		;
}

//=======================================================================================//
// һ���ں����̣��ͷ�InstallUserModeApc�з�����ڴ�										//
//======================================================================================//
void ApcKernelRoutine( IN struct _KAPC *Apc, 
					  IN OUT PKNORMAL_ROUTINE *NormalRoutine, 
					  IN OUT PVOID *NormalContext, 
					  IN OUT PVOID *SystemArgument1, 
					  IN OUT PVOID *SystemArgument2 ) 
{

	if (Apc)
		ExFreePool(Apc);
	if(pMdl)
	{
		MmUnlockPages(pMdl);
		IoFreeMdl (pMdl);
		pMdl = NULL;
	}
	//DbgPrint("KernelInject -> ApcKernelRoutine called. Memory freed.");
}
//=======================================================================================//
//APC���첽���̵������̣�Asynchronous Procedure Call��,�ں˿�����Ŀ���̷߳�һ��apc����
//��װAPC,����Ҫ���û�ģʽִ�еĴ���ӳ�䵽�û�̬�Ŀռ�MmMapLockedPagesSpecifyCache 
//��Ϊloadlibrary��Ҫ�õ��Ĳ������ǲ�����ֱ�����ں˵�����....
//��Ϊ��ֻ�����û�ִ̬��,���ܷ����ں˿ռ�,��������Ҫ�Ѳ������´���
// memset ((unsigned char*)pMappedAddress + 0x14, 0, 300);
// memcpy ((unsigned char*)pMappedAddress + 0x14,  DllFullPath,strlen ( DllFullPath));
// data_addr = (ULONG*)((char*)pMappedAddress+0x9); 
// *data_addr = dwMappedAddress+0x14; 
//======================================================================================//
NTSTATUS 
InstallUserModeApc(const char* DllFullPath, ULONG pTargetThread, ULONG pTargetProcess)
{
	PRKAPC pApc = NULL; 
	KAPC_STATE ApcState; 

	PVOID pMappedAddress = NULL;   //�û�̬�����ӳ���ַ
	ULONG dwSize = 0;              //�û�̬�����size


	ULONG *data_addr=0; 
	ULONG dwMappedAddress = 0; 
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (!pTargetThread || !pTargetProcess)
		return STATUS_UNSUCCESSFUL;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////	
	pApc = (PRKAPC)ExAllocatePool (NonPagedPool,sizeof (KAPC)); 
	if (!pApc)
	{
		//DbgPrint("KernelInject -> Failed to allocate memory for the APC structure");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//����usercode�ĳ���in bytes
	dwSize = (unsigned char*)ApcUserCodeEnd-(unsigned char*)ApcUserCode;
	//Ϊusercode����MDL
	pMdl = IoAllocateMdl (ApcUserCode, dwSize, FALSE,FALSE,NULL);
	if (!pMdl)
	{
		//DbgPrint("KernelInject -> Failed to allocate MDL");
		ExFreePool (pApc);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	__try
	{
		//̽��������pMdlָ���������ڴ�ҳ���������غ�pMdlָ������ҳ
		MmProbeAndLockPages (pMdl,KernelMode,IoWriteAccess);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//DbgPrint("KernelInject -> Exception during MmProbeAndLockPages");
		IoFreeMdl (pMdl);
		ExFreePool (pApc);
		return STATUS_UNSUCCESSFUL;
	}

	//����Ŀ����̵�������
	KeStackAttachProcess((ULONG *)pTargetProcess,&ApcState);

	//��pMdlָ���������ַӳ�䵽�����ַ�����÷���ģʽΪ�û�ģʽ
	//����ӳ���ĵ�ַ
	pMappedAddress = MmMapLockedPagesSpecifyCache (pMdl,UserMode,MmCached,NULL,FALSE,NormalPagePriority);

	if (!pMappedAddress)
	{
		//DbgPrint("KernelInject -> Cannot map address");
		KeUnstackDetachProcess (&ApcState);
		IoFreeMdl (pMdl);
		ExFreePool (pApc);

		return STATUS_UNSUCCESSFUL;
	}
	else 
		//DbgPrint("KernelInject -> UserMode memory at address: 0x%p",pMappedAddress);

	dwMappedAddress = (ULONG)pMappedAddress;
	//=============================================================
	// �����Ǽ���exe��Ӳ����code
	//
	//0x14ע������ApcUserCode..�и����������Ļ��������ط���ע����
	//�������޸�usercode������
	//���20���ֽ��Ժ������,��nop��������Ϊռλ��
	//memset ((unsigned char*)pMappedAddress + 20, 0, 300);
	//��dllpath���Ƶ�30���ֽ��Ժ�
	//memcpy ((unsigned char*)pMappedAddress + 30,  DllFullPath,strlen ( DllFullPath)); 

	//�ı�push 0xabcd�Ĳ�����Ϊdllpath
	//data_addr = (ULONG*)((char*)pMappedAddress + 9); //ָ��0xabcd��λ��
	//*data_addr = dwMappedAddress + 30;   //dllpath�ĵ�ַ
	//=================================================================
	
	//
	// �����ǲ���Ӳ�����code
	//
	DWORD dwFunctionAddress = (DWORD)g_pFunctionAddress;
	memset ((unsigned char*)pMappedAddress + 20, 0, 300);//zero everything out except our asm code
	memcpy ((unsigned char*)pMappedAddress + 1, &dwFunctionAddress,4);//WinExec�ĵ�ַ
	memcpy ((unsigned char*)pMappedAddress + 30, DllFullPath,strlen (DllFullPath)); //copy the dll path

	data_addr = (ULONG*)((char*)pMappedAddress + 9); //(originaly 0xabcd) our dll's path 
	*data_addr = dwMappedAddress + 30; 

	//=============================================================
	// ������ע��dll��Ӳ����code
	//
	//0x14ע������ApcUserCode..�и����������Ļ��������ط���ע����
	//�������޸�usercode������
	//���20���ֽ��Ժ������,��nop��������Ϊռλ��
	//memset ((unsigned char*)pMappedAddress + 20, 0, 300);
	//��dllpath���Ƶ�30���ֽ��Ժ�
	//memcpy ((unsigned char*)pMappedAddress + 30,  DllFullPath,strlen ( DllFullPath)); 

	//�ı�push 0xabcd�Ĳ�����Ϊdllpath
	//data_addr = (ULONG*)((char*)pMappedAddress + 9); //ָ��0xabcd��λ��
	//*data_addr = dwMappedAddress + 30;   //dllpath�ĵ�ַ
	//=================================================================

	KeUnstackDetachProcess (&ApcState);  
	//��ʼ��APC,��APC
	KeInitializeApc(pApc,
		(PETHREAD)pTargetThread,
		OriginalApcEnvironment,
		&ApcKernelRoutine,
		NULL,
		(PKNORMAL_ROUTINE)pMappedAddress,
		UserMode,
		(PVOID) NULL);
	if (!KeInsertQueueApc(pApc,0,NULL,0))
	{
		//DbgPrint("KernelInject -> Failed to insert APC");
		MmUnlockPages(pMdl);
		IoFreeMdl (pMdl);
		ExFreePool (pApc);
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		//DbgPrint("KernelInject -> APC delivered");
	}
	//ʹ�̴߳��ھ���״̬,ע�ⲻͬ����ϵͳ��ETHREAD
	if(!*(char *)(pTargetThread+0x4a))
	{

		*(char *)(pTargetThread+0x4a) = TRUE;
	}

	return 0;
}

//=======================================================================================//
// �û�̬����																			//
//======================================================================================//
__declspec(naked) void ApcUserCode(PVOID NormalContext, PVOID  SystemArgument1, PVOID SystemArgument2)
{
	__asm 
	{		
		//����exeӲ����
		//mov eax,0x7C8623AD    //������WinExec��ַ
		//push 1
		//nop
		//push 0xabcd
		//call eax
		//jmp end   
		//����exe��Ӳ����
		/*mov eax,0x1234
		push 1
		nop
		push 0xabcd
		call eax
		jmp end  */ 

		//ע��dll
		mov eax,0x1234		//5//������LoadLibraryA��ַ
		nop						//1
		nop						//1
		nop						//1
		push 0xabcd				//5
		call eax				//2
		jmp end					//5
		/*
		//
		// �����ǲ���Ӳ�����code
		//
		// loadlibraryW - 0x7C80AE4B
		// LoadLibraryA - 0x7C801D77        
		push 0xabcd     // 5      dllpath
		nop             // 1
		mov eax,0x1234  // 5	  LoadLibaryA 	
		mov eax,[eax]   // 2
		call eax        // 2
		jmp end         // 5

		// ��  20 �ֽ�

		*/
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
	end:
		nop
		ret 0x0c
	}

}
//=======================================================================================//
// ����������������û�̬����ĳ���														//
//======================================================================================//
void ApcUserCodeEnd(){}


//=======================================================================================//
// DriverUnload����																		//
//======================================================================================//
#pragma PAGEDCODE
VOID Unload (IN PDRIVER_OBJECT pDriverObject) 
{	
	//DbgPrint("KernelInject -> Driver Unloaded");
}
//=======================================================================================//
// DriverEntry����																		//
//======================================================================================//
#pragma INITCODE
extern "C" NTSTATUS DriverEntry (
			IN PDRIVER_OBJECT pDriverObject,
			IN PUNICODE_STRING pRegistryPath	) 
{
	//DbgPrint("KernelInject -> Driver Loaded");
	
	do 
	{
		UNICODE_STRING usDll;
		RtlInitUnicodeString( &usDll, L"\\SystemRoot\\system32\\kernel32.dll" );
	
		//����ϵͳģ��
		HANDLE hModule = LoadSystemModule( &usDll );
		if( NULL == hModule ){
			//DbgPrint("KernelInject -> load moudle error!");
			break;
		}
		//DbgPrint( "KernelInject -> load moudle at: %x\n", hModule );

		//��ȡ����������ӳ���ַ
		//����exe��WinExec
		//g_pFunctionAddress = GetSystemProcAddress( hModule, "WinExec" );
		//����dll��LoadLibaryA
		g_pFunctionAddress = GetSystemProcAddress( hModule, "LoadLibraryA" );
		if( NULL == g_pFunctionAddress){
			//DbgPrint( "KernelInject -> get function address error!" );
			break;
		}
		//DbgPrint( "KernelInject -> get function address: %x", g_pFunctionAddress );

		//�ͷ�ϵͳģ��
		FreeSystemModule( hModule );

	} while( FALSE );

	//����exe
	//InjectDll("d:\\hello.exe", "explorer.exe");
	//ע��dll
	InjectDll("c:\\ecnet10047.dll", "explorer.exe");
	pDriverObject->DriverUnload = Unload; 
	return STATUS_SUCCESS; 
}


//=======================================================================================//
// ����ϵͳģ�飬ӳ�䵽���̿ռ�															//
//======================================================================================//
HANDLE LoadSystemModule(IN PUNICODE_STRING pusModule)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE hFile = NULL;
	HANDLE hModule = NULL;
	HANDLE hSection = NULL;

	do 
	{
		IO_STATUS_BLOCK isb = { 0 };
		OBJECT_ATTRIBUTES oa = { 0 };
		InitializeObjectAttributes( &oa, 
			pusModule,
			OBJ_CASE_INSENSITIVE, 
			NULL, 
			NULL );

		//���ļ�
		status = ZwOpenFile( &hFile, 
			FILE_EXECUTE | SYNCHRONIZE, 
			&oa,
			&isb,
			FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT );
		if( !NT_SUCCESS(status) ){
			//DbgPrint("KernelInject -> open moudle file error!");
			break;
		}

		//����һ��section���󣬽���ʹ��section�����������̹������Ĳ����ڴ�ռ�
		oa.ObjectName = NULL;
		status = ZwCreateSection( &hSection,
			SECTION_ALL_ACCESS,
			&oa,
			0,
			PAGE_EXECUTE,
			SEC_IMAGE,
			hFile );
		if( !NT_SUCCESS(status) ){
			//DbgPrint("KernelInject -> create new section error!");
			break;
		}

		//ӳ��section����ͼ��Ŀ����̵������ַ�ռ�
		//view�ǽ��̿ɼ���
		DWORD dwSize = 0;
		status = ZwMapViewOfSection( hSection,
			NtCurrentProcess(),
			&hModule,
			0,
			1000,
			0,
			&dwSize,
			(SECTION_INHERIT) 1,
			MEM_TOP_DOWN,
			PAGE_READWRITE );


	} while( FALSE );

	if( NULL != hFile )
	{
		ZwClose( hFile );	//�ر��ļ����
	}

	if( NULL != hSection )
	{
		ZwClose( hSection );	//�ر�section������
	}

	if( !NT_SUCCESS(status) )
	{
		if( NULL != hModule ){
			//DbgPrint("KernelInject -> map moudle dll error!");
			ZwUnmapViewOfSection( NtCurrentProcess(), hModule );
			hModule = NULL;
		}
	}

	return hModule;
}

//=======================================================================================//
// ��ȡ��������lpFunctionName�ĵ�ַ														//
//======================================================================================//
PVOID 
GetSystemProcAddress(
					 IN	HANDLE		hModule,
					 IN	LPSTR		lpFunctionName
					 )
{
	PVOID pFunctionAddress = NULL;

	do 
	{
		if( ( NULL == hModule ) ||
			( NULL == lpFunctionName ) )
		{
			break;
		}
		//����dll�ļ�
		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) hModule;
		PIMAGE_OPTIONAL_HEADER pOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ( (DWORD) hModule + pDosHeader->e_lfanew + 24 );
		PIMAGE_EXPORT_DIRECTORY pExportTable = (PIMAGE_EXPORT_DIRECTORY)( (DWORD) hModule + pOptionalHeader->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress );

		PDWORD pAddressOfFunctions = (PDWORD) ( (DWORD) hModule + pExportTable->AddressOfFunctions );
		PDWORD pAddressOfNames = (PDWORD) ( (DWORD) hModule + pExportTable->AddressOfNames );
		PWORD pAddressOfNameOrdinals = (PWORD) ( (DWORD) hModule + pExportTable->AddressOfNameOrdinals );
		DWORD dwBase = pExportTable->Base;

		ANSI_STRING asFunctionName;
		RtlInitAnsiString( &asFunctionName, lpFunctionName );

		LPSTR lpExportFunctionName = NULL;
		ANSI_STRING asExportFunctionName;

		BOOL bStatus = FALSE;
		DWORD dwOrdinal = 0;
		for( DWORD dwIndex = 0; dwIndex < pExportTable->NumberOfFunctions; dwIndex ++ )
		{
			lpExportFunctionName = (LPSTR) ( (DWORD) hModule + pAddressOfNames[dwIndex] );
			RtlInitString( &asExportFunctionName, lpExportFunctionName );
			dwOrdinal = pAddressOfNameOrdinals[dwIndex] + dwBase - 1;
			pFunctionAddress = (PVOID) ( (DWORD) hModule + pAddressOfFunctions[dwOrdinal] );

			if( 0 == RtlCompareString(&asExportFunctionName, &asFunctionName, TRUE) )
			{
				bStatus = TRUE;
				break;
			}
		}

		if( !bStatus )
		{
			pFunctionAddress = NULL;
		}


	} while( FALSE );

	return pFunctionAddress;
}

//=======================================================================================//
// �ͷ�ϵͳģ��																			//
//======================================================================================//
NTSTATUS
FreeSystemModule(
				 IN	HANDLE	hModule
				 )
{
	NTSTATUS status = STATUS_INVALID_HANDLE;

	do 
	{
		if( NULL == hModule )
		{
			break;
		}
		//���ӳ��
		status = ZwUnmapViewOfSection( NtCurrentProcess(), hModule );


	} while( FALSE );

	return status;

}