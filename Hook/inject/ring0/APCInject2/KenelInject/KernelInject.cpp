#include "APCFunc.h"
#include "MoudleFunc.h"
#include <ntddk.h>

#define PAGEDCODE code_seg("PAGE")
#define INITCODE  code_seg("INIT")
#define MAX_PID 65535	//进程ID最大值
//=======================================================================================//
// 这里声明要用到的全局变量																//
//======================================================================================//
PMDL pMdl = NULL;					//内存描述符表
PVOID g_pFunctionAddress = NULL;	//系统moudle导出函数的地址

//=======================================================================================//
// InjectDll遍历进程,得到根据名字找到我们想要注入的进程,得到进程后遍历线程.找受信线程   //
//注意这里ethread我是用XP..在其他操作系统可能一些偏移不同                               //
//这里需要修改！！！！！！！！！！！！！！！
//======================================================================================//

void InjectDll(const char* DllFullPath,LPSTR ProcessName)
{
	//全部定义为ULONG类型
	ULONG pTargetProcess;                //目标进程
	ULONG pTargetThread;                 //目标线程
	ULONG pNotAlertableThread;           //非警告线程 
	ULONG pSystemProcess;                //系统进程
	ULONG pTempThread;                   //临时线程
	ULONG pNextEntry, pListHead, pThNextEntry,pThListHead; 
	ULONG pid;							//进程ID
	PEPROCESS EProcess;
	NTSTATUS status;

	//遍历进程，按pid查找
	for(pid=0; pid<MAX_PID; pid+=4)
	{  
		//根据pid返回进程的PEPROCESS结构的指针
		status = PsLookupProcessByProcessId((HANDLE)pid,&EProcess);
		if((NT_SUCCESS(status)))
		{
			//比较进程名
			if(_stricmp((const char*)PsGetProcessImageFileName(EProcess),ProcessName)==0)
			{	
				//process found!
				pSystemProcess = (ULONG)EProcess;
				pTargetProcess = pSystemProcess; 
				pTargetThread = pNotAlertableThread = 0;
				//ThreadListHead 
				//eprocess:  XP 0x190 win7 0x188 
				//kprocess:  eprocess的一部分，
				//ThreadListHead在偏移0x50 
				pThListHead = pSystemProcess+0x50;	
				pThNextEntry = *(ULONG *)pThListHead;	//next thread 四个字节

				//遍历双向链表ThreadList
				while(pThNextEntry != pThListHead)
				{
					pTempThread =pThNextEntry-0x1b0;  //ETHREAD????
					//Alertable 0x164
					if(*(char *)(pTempThread+0x164)) //警告线程
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
// 一个内核例程，释放InstallUserModeApc中分配的内存										//
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
//APC：异步过程调用例程（Asynchronous Procedure Call）,内核可以向目标线程发一个apc请求
//安装APC,首先要把用户模式执行的代码映射到用户态的空间MmMapLockedPagesSpecifyCache 
//因为loadlibrary所要用到的参数我们不可能直接用内核的数据....
//因为它只能在用户态执行,不能访问内核空间,所以我们要把参数做下处理
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

	PVOID pMappedAddress = NULL;   //用户态代码的映射地址
	ULONG dwSize = 0;              //用户态代码的size


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

	//计算usercode的长度in bytes
	dwSize = (unsigned char*)ApcUserCodeEnd-(unsigned char*)ApcUserCode;
	//为usercode分配MDL
	pMdl = IoAllocateMdl (ApcUserCode, dwSize, FALSE,FALSE,NULL);
	if (!pMdl)
	{
		//DbgPrint("KernelInject -> Failed to allocate MDL");
		ExFreePool (pApc);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	__try
	{
		//探索并锁定pMdl指定的虚拟内存页，函数返回后pMdl指向物理页
		MmProbeAndLockPages (pMdl,KernelMode,IoWriteAccess);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//DbgPrint("KernelInject -> Exception during MmProbeAndLockPages");
		IoFreeMdl (pMdl);
		ExFreePool (pApc);
		return STATUS_UNSUCCESSFUL;
	}

	//进入目标进程的上下文
	KeStackAttachProcess((ULONG *)pTargetProcess,&ApcState);

	//将pMdl指定的物理地址映射到虚拟地址，设置访问模式为用户模式
	//返回映射后的地址
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
	// 这里是加载exe的硬编码code
	//
	//0x14注意后面的ApcUserCode..有改其他函数的话这两个地方都注意下
	//这里是修改usercode的内容
	//清空20个字节以后的内容,即nop，都是作为占位符
	//memset ((unsigned char*)pMappedAddress + 20, 0, 300);
	//将dllpath复制到30个字节以后
	//memcpy ((unsigned char*)pMappedAddress + 30,  DllFullPath,strlen ( DllFullPath)); 

	//改变push 0xabcd的操作数为dllpath
	//data_addr = (ULONG*)((char*)pMappedAddress + 9); //指向0xabcd的位置
	//*data_addr = dwMappedAddress + 30;   //dllpath的地址
	//=================================================================
	
	//
	// 这里是不用硬编码的code
	//
	DWORD dwFunctionAddress = (DWORD)g_pFunctionAddress;
	memset ((unsigned char*)pMappedAddress + 20, 0, 300);//zero everything out except our asm code
	memcpy ((unsigned char*)pMappedAddress + 1, &dwFunctionAddress,4);//WinExec的地址
	memcpy ((unsigned char*)pMappedAddress + 30, DllFullPath,strlen (DllFullPath)); //copy the dll path

	data_addr = (ULONG*)((char*)pMappedAddress + 9); //(originaly 0xabcd) our dll's path 
	*data_addr = dwMappedAddress + 30; 

	//=============================================================
	// 这里是注入dll的硬编码code
	//
	//0x14注意后面的ApcUserCode..有改其他函数的话这两个地方都注意下
	//这里是修改usercode的内容
	//清空20个字节以后的内容,即nop，都是作为占位符
	//memset ((unsigned char*)pMappedAddress + 20, 0, 300);
	//将dllpath复制到30个字节以后
	//memcpy ((unsigned char*)pMappedAddress + 30,  DllFullPath,strlen ( DllFullPath)); 

	//改变push 0xabcd的操作数为dllpath
	//data_addr = (ULONG*)((char*)pMappedAddress + 9); //指向0xabcd的位置
	//*data_addr = dwMappedAddress + 30;   //dllpath的地址
	//=================================================================

	KeUnstackDetachProcess (&ApcState);  
	//初始化APC,插APC
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
	//使线程处于警告状态,注意不同操作系统的ETHREAD
	if(!*(char *)(pTargetThread+0x4a))
	{

		*(char *)(pTargetThread+0x4a) = TRUE;
	}

	return 0;
}

//=======================================================================================//
// 用户态代码																			//
//======================================================================================//
__declspec(naked) void ApcUserCode(PVOID NormalContext, PVOID  SystemArgument1, PVOID SystemArgument2)
{
	__asm 
	{		
		//加载exe硬编码
		//mov eax,0x7C8623AD    //本机的WinExec地址
		//push 1
		//nop
		//push 0xabcd
		//call eax
		//jmp end   
		//加载exe非硬编码
		/*mov eax,0x1234
		push 1
		nop
		push 0xabcd
		call eax
		jmp end  */ 

		//注入dll
		mov eax,0x1234		//5//本机的LoadLibraryA地址
		nop						//1
		nop						//1
		nop						//1
		push 0xabcd				//5
		call eax				//2
		jmp end					//5
		/*
		//
		// 这里是不用硬编码的code
		//
		// loadlibraryW - 0x7C80AE4B
		// LoadLibraryA - 0x7C801D77        
		push 0xabcd     // 5      dllpath
		nop             // 1
		mov eax,0x1234  // 5	  LoadLibaryA 	
		mov eax,[eax]   // 2
		call eax        // 2
		jmp end         // 5

		// 共  20 字节

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
// 这个函数用来计算用户态代码的长度														//
//======================================================================================//
void ApcUserCodeEnd(){}


//=======================================================================================//
// DriverUnload例程																		//
//======================================================================================//
#pragma PAGEDCODE
VOID Unload (IN PDRIVER_OBJECT pDriverObject) 
{	
	//DbgPrint("KernelInject -> Driver Unloaded");
}
//=======================================================================================//
// DriverEntry例程																		//
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
	
		//加载系统模块
		HANDLE hModule = LoadSystemModule( &usDll );
		if( NULL == hModule ){
			//DbgPrint("KernelInject -> load moudle error!");
			break;
		}
		//DbgPrint( "KernelInject -> load moudle at: %x\n", hModule );

		//获取导出函数的映射地址
		//加载exe用WinExec
		//g_pFunctionAddress = GetSystemProcAddress( hModule, "WinExec" );
		//加载dll用LoadLibaryA
		g_pFunctionAddress = GetSystemProcAddress( hModule, "LoadLibraryA" );
		if( NULL == g_pFunctionAddress){
			//DbgPrint( "KernelInject -> get function address error!" );
			break;
		}
		//DbgPrint( "KernelInject -> get function address: %x", g_pFunctionAddress );

		//释放系统模块
		FreeSystemModule( hModule );

	} while( FALSE );

	//加载exe
	//InjectDll("d:\\hello.exe", "explorer.exe");
	//注入dll
	InjectDll("c:\\ecnet10047.dll", "explorer.exe");
	pDriverObject->DriverUnload = Unload; 
	return STATUS_SUCCESS; 
}


//=======================================================================================//
// 加载系统模块，映射到进程空间															//
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

		//打开文件
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

		//创建一个section对象，进程使用section来和其他进程共享它的部分内存空间
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

		//映射section的视图到目标进程的虚拟地址空间
		//view是进程可见的
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
		ZwClose( hFile );	//关闭文件句柄
	}

	if( NULL != hSection )
	{
		ZwClose( hSection );	//关闭section对象句柄
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
// 获取导出函数lpFunctionName的地址														//
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
		//解析dll文件
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
// 释放系统模块																			//
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
		//解除映射
		status = ZwUnmapViewOfSection( NtCurrentProcess(), hModule );


	} while( FALSE );

	return status;

}