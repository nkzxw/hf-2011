#include<ntddk.h>
#include <windef.h> 
#include "processenum.h"

///////////////////////////不同的windows版本下面的偏移值不同

#define  EPROCESS_SIZE       0x0 //EPROCESS结构大小
#define  PEB_OFFSET          0x1
#define  FILE_NAME_OFFSET    0x2
#define  PROCESS_LINK_OFFSET 0x3
#define  PROCESS_ID_OFFSET   0x4
#define  EXIT_TIME_OFFSET    0x5

#define  OBJECT_HEADER_SIZE  0x018
#define  OBJECT_TYPE_OFFSET  0x008
////////////////////////// 

#define PDE_INVALID 2 
#define PTE_INVALID 1 
#define VALID 0 


Processinfo* pProcessPtr = NULL;
int nProcessCount = 0;

ULONG     pebAddress;         //PEB地址的前半部分
PEPROCESS pSystem;            //system进程
ULONG     pObjectTypeProcess; //进程对象类型

ULONG   VALIDpage(ULONG addr) ;  //该函数直接复制自 Ring0下搜索内存枚举隐藏进程
BOOLEAN IsaRealProcess(ULONG i); //该函数复制自 Ring0下搜索内存枚举隐藏进程
VOID    WorkThread(IN PVOID pContext);
//ULONG   GetPebAddress();          //得到PEB地址前半部分
VOID    EnumProcess();            //枚举进程
VOID    ShowProcess(ULONG pEProcess); //显示结果
DWORD	GetPlantformDependentInfo(DWORD	eprocessflag);
#define SYSNAME    "System"
ULONG	ProcessNameOffset = 0; //进程名偏移量
ULONG   GetProcessNameOffset();
BOOLEAN GetProcess(PCHAR Name);

void EnumProcess2();

WCHAR gDeviceName[]=L"\\Device\\safepsenum";
WCHAR gDosDeviceName[]=L"\\??\\safepsenum";

NTSTATUS SafePsEnumCreate(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS SafePsEnumClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);

NTSTATUS
MyDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp );

/////////////////////////////////////////////////////////
VOID    OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	UNICODE_STRING DosDeviceName;
	RtlInitUnicodeString(&DosDeviceName,gDosDeviceName);
	if(DriverObject->DeviceObject)
		IoDeleteDevice(DriverObject->DeviceObject);
	status = IoDeleteSymbolicLink(&DosDeviceName);
	if(status)	
		DbgPrint("IoDeleteSymbolicLink Return %0x\n",status);	

	if (pProcessPtr)
	{
		ExFreePool(pProcessPtr);
	}
}

/////////////////////////////////////////////////////////
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
	HANDLE hThread; 
	UNICODE_STRING DeviceName;
	UNICODE_STRING DosDeviceName;
	PDEVICE_OBJECT pDeviceObject=NULL;
	NTSTATUS Status;
 
	pSystem    = PsGetCurrentProcess();
//	pebAddress = GetPebAddress();
    pebAddress = 0x7FFD0000;       //取了一个通用的低位地址
	ProcessNameOffset = GetProcessNameOffset();
 
	pObjectTypeProcess = *(PULONG)((ULONG)pSystem - OBJECT_HEADER_SIZE +OBJECT_TYPE_OFFSET);  

	DbgPrint("type: %d \n", pObjectTypeProcess);


	DriverObject -> DriverUnload = OnUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE]         = SafePsEnumCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]          = SafePsEnumClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceControl;

	RtlInitUnicodeString(&DeviceName,gDeviceName);
	RtlInitUnicodeString(&DosDeviceName,gDosDeviceName);
	IoCreateDevice(DriverObject,0,&DeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&pDeviceObject);
	pDeviceObject->Flags|=DO_BUFFERED_IO;
	Status = IoCreateSymbolicLink(&DosDeviceName,&DeviceName);
	if(Status)
		DbgPrint("IoCreateSymbolicLink Return %0x\n",Status);
	
	nProcessCount = 0;
	// 测试固定获取最大为1024
	pProcessPtr =  ExAllocatePoolWithTag(NonPagedPool, 1024*sizeof(Processinfo), 'HpcM');

	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////
NTSTATUS SafePsEnumCreate(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{	
 	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////
NTSTATUS SafePsEnumClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
 	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////
/*
ULONG  GetPebAddress()
{
	ULONG Address;
	PEPROCESS pEProcess;

	//由于system进程的peb总是零 我们只有到其他进程去找了
	pEProcess = (PEPROCESS)((ULONG)((PLIST_ENTRY)((ULONG)pSystem + PROCESS_LINK_OFFSET))->Flink - PROCESS_LINK_OFFSET);
	Address   = *(PULONG)((ULONG)pEProcess + PEB_OFFSET);

	return (Address & 0xFFFF0000);  
}
*/
///////////////////////////////////////////////////////
VOID EnumProcess()
{
	ULONG  uSystemAddress = (ULONG)pSystem;
	ULONG  i;
	ULONG  Address;
	ULONG  ret;

	nProcessCount = 0;
 
	for(i = 0x80000000; i < uSystemAddress; i += 4) {//system进程的EPROCESS地址就是最大值了
		
		ret = VALIDpage(i); 

		if (ret == VALID) { 
		 
			Address = *(PULONG)i;
			if (( Address & 0xFFFF0000) == 0x7FFD0000) {//每个进程的PEB地址都是在差不多的地方，地址前半部分是相同的       
				
				if(IsaRealProcess(i)) { 
				
					ShowProcess(i - GetPlantformDependentInfo(PEB_OFFSET)); 

					i += GetPlantformDependentInfo(EPROCESS_SIZE);                
				} 
			} 
		}
		else if(ret == PTE_INVALID) { 
			
			i -=4; 
			i += 0x1000;//4k 
		}
		else { 

		  i-=4; 
		  i+= 0x400000;//4mb 
		} 
	}

 	ShowProcess(uSystemAddress);//system的PEB总是零 上面的方法是枚举不到的 不过我们用PsGetCurrentProcess就能得到了
}

/////////////////////////////////////////////////////////
VOID    ShowProcess(ULONG pEProcess)
{
	PLARGE_INTEGER ExitTime;
	ULONG PID;
	PUCHAR pFileName;
	
	ExitTime = (PLARGE_INTEGER)(pEProcess + GetPlantformDependentInfo(EXIT_TIME_OFFSET));  

	if(ExitTime->QuadPart != 0) //已经结束的进程的ExitTime为非零	
		return ;

	PID = *(PULONG)(pEProcess + GetPlantformDependentInfo(PROCESS_ID_OFFSET));
	pFileName = (PUCHAR)(pEProcess + GetPlantformDependentInfo(FILE_NAME_OFFSET));

 	pProcessPtr[nProcessCount].pEProcess = pEProcess;
	pProcessPtr[nProcessCount].PId = PID;
	strcpy(pProcessPtr[nProcessCount].Name, pFileName);	 

	nProcessCount++;
}

/////////////////////////////////////////////////////////////
ULONG VALIDpage(ULONG addr) 
{ 
	ULONG pte; 
	ULONG pde; 

	pde = 0xc0300000 + (addr>>22)*4; 
	if((*(PULONG)pde & 0x1) != 0){ 
	//large page 
	if((*(PULONG)pde & 0x80) != 0){ 
		return VALID; 
	} 
	pte = 0xc0000000 + (addr>>12)*4; 
	if((*(PULONG)pte & 0x1) != 0){ 
		return VALID; 
	}else{ 
		return PTE_INVALID; 
	} 
	} 
	return PDE_INVALID; 
} 

////////////////////////////////////////////////////////////////
BOOLEAN IsaRealProcess(ULONG i) 
{ 
	NTSTATUS STATUS; 
	PUNICODE_STRING pUnicode; 
	UNICODE_STRING Process; 
	ULONG pObjectType; 
	ULONG ObjectTypeAddress; 

	if (VALIDpage(i- GetPlantformDependentInfo(PEB_OFFSET)) != VALID){ 
		return FALSE; 
	} 

	ObjectTypeAddress = i - GetPlantformDependentInfo(PEB_OFFSET) - OBJECT_HEADER_SIZE + OBJECT_TYPE_OFFSET ;

	if (VALIDpage(ObjectTypeAddress) == VALID){ 
		pObjectType = *(PULONG)ObjectTypeAddress; 
	}else{ 
		return FALSE; 
	} 

	if (pObjectTypeProcess == pObjectType){ //确定ObjectType是Process类型
		return TRUE; 
	} 
	return FALSE; 
} 
////////////////////////////////////////////////////////////////////
#define		DWORD	unsigned long

NTSTATUS
MyDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{

    PIO_STACK_LOCATION  io_stack;
    NTSTATUS            status;

	Irp->IoStatus.Information = 0;
    io_stack = IoGetCurrentIrpStackLocation(Irp);

	if (io_stack->MajorFunction==IRP_MJ_DEVICE_CONTROL) 
	{
		switch (io_stack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_GETPROCESSPTR:
			{
			 	EnumProcess();
			//	EnumProcess2();
				RtlCopyMemory((unsigned char*)Irp->UserBuffer, (unsigned char*)&nProcessCount, sizeof(DWORD));
				RtlCopyMemory((unsigned char*)Irp->UserBuffer+sizeof(DWORD), (unsigned char*)pProcessPtr, nProcessCount*sizeof(Processinfo));
				Irp->IoStatus.Information = nProcessCount*sizeof(Processinfo)+sizeof(DWORD);
			}
		}
	}
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//---------------------------------------------------------------------- 
// 
// GetProcessNameOffset 
// 
// In an effort to remain version-independent, rather than using a 
// hard-coded into the KPEB (Kernel Process Environment Block), we 
// scan the KPEB looking for the name, which should match that 
// of the GUI process 
// 
//---------------------------------------------------------------------- 
ULONG GetProcessNameOffset() 
{ 
    PEPROCESS       curproc; 
    int             i; 
    DbgPrint(("GetProcessNameOffset\n")); 
    curproc = PsGetCurrentProcess(); 

    // 
    // Scan for 12KB, hopping the KPEB never grows that big! 
    // 
    for( i = 0; i < 3*PAGE_SIZE; i++ ) { 

		if( !strncmp( SYSNAME, (PCHAR) curproc + i, strlen(SYSNAME) )) { 

			DbgPrint("%d\n", i);
			return i; 
		} 
	} 
	// 
	// Name not found - oh, well 
	// 
	DbgPrint("0\n");
	return 0; 
} 

//---------------------------------------------------------------------- 
// 
// GetProcess 
// 
// Uses undocumented data structure offsets to obtain the name of the 
// currently executing process. 
// 
//---------------------------------------------------------------------- 
BOOLEAN GetProcess( PCHAR Name ) 
{ 
	PEPROCESS curproc; 
	char *nameptr; 
	ULONG i; 

	// 
	// We only try and get the name if we located the name offset 
	// 
	if( ProcessNameOffset ) { 

		curproc = PsGetCurrentProcess(); 
		nameptr = (PCHAR) curproc + ProcessNameOffset; 
		strncpy( Name, nameptr, 16 ); 
		return TRUE;
	} else { 
		strcpy( Name, "???"); 
		return FALSE;
	}
}


////////////////////////////////////////////////////////////////////////
// EnumProcess2
#define BASE_PROCESS_PEB_OFFSET					0x01B0
#define BASE_PEB_PROCESS_PARAMETER_OFFSET		0x0010
#define BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME	0x003C
#define W2003_BASE_PROCESS_PEB_OFFSET			0x0190
#define W2003_BASE_PROCESS_PEB_OFFSET_SP1		0x01A0
#define VISTA_BASE_PROCESS_PEB_OFFSET			0x0188
 

void EnumProcess2()
{
   ULONG OsMajorVersion;
   ULONG OsMinorVersion ;
   DWORD dwAddress;
   PCWSTR Temp=NULL;
   ULONG uSystemAddress = (ULONG) pSystem;
   DWORD i;

   if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
		return  ;
   }
 
   PsGetVersion( &OsMajorVersion,
	   &OsMinorVersion,
	   NULL,
	   NULL );

   for(i = 0x80000000; i < uSystemAddress; i += 4) {//system进程的EPROCESS地址就是最大值了

	   try {

		   ULONG PID = 0;

		   if ( *(DWORD*)(i+GetPlantformDependentInfo(PROCESS_ID_OFFSET)) == PID)
			   continue;

		   if (!IsaRealProcess(i))
			   continue;

		   dwAddress = i;
 
		   if(dwAddress == 0 || dwAddress == 0xFFFFFFFF) {
			  return  ;
		   }

		   //目前只支持Win 2000/xp/2003/VISTA 
		   if (OsMajorVersion < 5 || OsMinorVersion > 2 ) {
			   return  ;
		   }

		   //取得PEB，不同平台的位置是不同的。
		   //
		   //2000   0X0500         XP 0X0501
		   //
		   if( OsMajorVersion == 5 && OsMinorVersion < 2) {
   
			   dwAddress += BASE_PROCESS_PEB_OFFSET;
		   }
		   //
		   //2003   0X0502 
		   //
		   if (OsMajorVersion == 5 && OsMinorVersion ==2) {
 			   dwAddress += W2003_BASE_PROCESS_PEB_OFFSET;
		   }
		   //
		   //VISTA   0X0600 
		   //
		   if (OsMajorVersion == 6 && OsMinorVersion ==0) {
 			   dwAddress += VISTA_BASE_PROCESS_PEB_OFFSET;
		   }

		   if ((dwAddress = *(DWORD*)dwAddress) == 0) {
			   continue;
		   }

		   //
		   // 通过peb取得RTL_USER_PROCESS_PARAMETERS
		   //
		   dwAddress += BASE_PEB_PROCESS_PARAMETER_OFFSET;
		   if((dwAddress = *(DWORD*)dwAddress) == 0) {
			   continue;
		   }
		   //
		   // 在RTL_USER_PROCESS_PARAMETERS->ImagePathName保存了路径，偏移为38,
		   //
		   dwAddress += BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME;
		   if ((dwAddress = *(DWORD*)dwAddress) == 0) {
			   continue;
		   }
	   // [10/14/2006]
		   Temp=(PCWSTR)dwAddress;
		   if (wcslen(Temp)>4) {
				if (Temp[0]==L'\\'&&
				   Temp[1]==L'?'&&
				   Temp[2]==L'?'&&
				   Temp[3]==L'\\') {
				   dwAddress+=8;
			   }
			   if (Temp[0]==L'\\'&&
				   Temp[1]==L'\\'&&
				   Temp[2]==L'?'&&
				   Temp[3]==L'\\') {
				   dwAddress+=8;
			   }
			   DbgPrint("%ws\n", dwAddress);
			   i = dwAddress;
		   }
	   }
	   except (EXCEPTION_EXECUTE_HANDLER) {
			try {
				if(OsMajorVersion == 5 && OsMinorVersion ==2) {
					dwAddress = (DWORD)PsGetCurrentProcess();
					dwAddress += W2003_BASE_PROCESS_PEB_OFFSET_SP1;
					if((dwAddress = *(DWORD*)dwAddress) == 0) {
					   continue;
					}

					//
					// 通过peb取得RTL_USER_PROCESS_PARAMETERS
					//
					dwAddress += BASE_PEB_PROCESS_PARAMETER_OFFSET;
					if((dwAddress = *(DWORD*)dwAddress) == 0) {
					   continue;
					}
					//
					// 在RTL_USER_PROCESS_PARAMETERS->ImagePathName保存了路径，偏移为38,
					//
					dwAddress += BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME;
					if((dwAddress = *(DWORD*)dwAddress) == 0) {
					   continue;
					}
					// [10/14/2006]
					Temp=(PCWSTR)dwAddress;
					if (wcslen(Temp)>4) {
						if (Temp[0]==L'\\'&&
						   Temp[1]==L'?'&&
						   Temp[2]==L'?'&&
						   Temp[3]==L'\\') {
						   dwAddress+=8;
					   }
					   if (Temp[0]==L'\\'&&
						   Temp[1]==L'\\'&&
						   Temp[2]==L'?'&&
						   Temp[3]==L'\\') {
						   dwAddress+=8;
					   }
					}
					DbgPrint("%ws\n", dwAddress);
					i = dwAddress;
				}
			}
			except (EXCEPTION_EXECUTE_HANDLER) {
			}			  
	   }
   }
}

DWORD	GetPlantformDependentInfo(DWORD	eprocessflag)
{
	DWORD current_build;
	DWORD ans = 0;

	PsGetVersion(NULL, NULL, &current_build, NULL); 

	switch(eprocessflag){
		case EPROCESS_SIZE:
			if (current_build ==  2195) //2000
			{
				ans = 0x1FC;
			}
			if (current_build ==  2600) //XP
			{
				ans = 0x25C;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x270;    
			}
			break;
		case PEB_OFFSET:
			if (current_build ==  2195) //2000
			{

				ans = 0x09c;
			}
			if (current_build ==  2600) //XP
			{
				ans = 0x1b0;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x1a0; 
			}
			break;
		case FILE_NAME_OFFSET:
			if (current_build ==  2195) //2000
			{

				ans = 0x09c;      
			}
			if (current_build ==  2600) //XP
			{
				ans = 0x174;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x164; 
			}
			break;
		case PROCESS_LINK_OFFSET:
			if (current_build ==  2195) //2000
			{

				ans = 0x09c;      
			}
			if (current_build ==  2600) //XP
			{
				ans = 0x088;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x098; 
			}
			break;
		case PROCESS_ID_OFFSET:
			if (current_build ==  2195) //2000
			{

				ans = 0x09c;      
			}
			if (current_build ==  2600) //XP
			{
				ans = 0x084;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x094; 
			}
			break;
		case EXIT_TIME_OFFSET:
			if (current_build ==  2195) //2000
			{

				ans = 0x09c;      
			}
			if (current_build ==  2600) //XP
			{
				ans = 0x078;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x088; 
			}
			break;
		default:
			break;
	}
	return ans;
}