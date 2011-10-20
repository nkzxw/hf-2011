/*
Ultimap implements the recording of all the branches in the target process
Requires dbvm for process selection
*/

#include "ntifs.h"
#include "ultimap.h"
#include "vmxhelper.h"
#include "DBKFunc.h"
#include <windef.h>


JUMPBACK perfmonJumpBackLocation;



#ifdef AMD64
PAPIC APIC_BASE=(PAPIC)0xfffffffffffe0000;
#else
PAPIC APIC_BASE=(PAPIC)0xfffe0000;
#endif

BOOL SaveToFile; //If set it will save the results to a file instead of sending a message to the usermode app that is watching the data
HANDLE FileHandle;

int MaxDataBlocks=1;

KSEMAPHORE DataBlockSemaphore; //governs how many events can be active at a time
FAST_MUTEX DataBlockMutex; //when a thread passes the semaphore this is used to pick a DataBlock


typedef struct
{
	BOOL Available;
	PBTS Data;
	int DataSize;
	KEVENT DataReady;    
} _DataBlock;

_DataBlock *DataBlock;
PVOID *DataReadyPointerList;

int perfmon_interrupt_centry(void);

void ultimap_flushBuffers_all(UINT_PTR param)
{
	DbgPrint("Calling perfmon_interrupt_centry() manually\n");
	perfmon_interrupt_centry();
	enableInterrupts(); //the handler disables it on exit so re-enable it
}

void ultimap_flushBuffers(void)
{
	//call this when the buffer of the current cpu needs to be flushed and handled
	int i;
	int count;

	DbgPrint("ultimap_flushBuffers\n");

	//what it does:
	//for each cpu emulate a "buffer filled" event.
	//the handler then copies all the current data to a temporary buffer and signals the worker thread to deal with it. If there is no available worker thread it waits
	forEachCpuPassive(ultimap_flushBuffers_all,0);

	DbgPrint("ultimap_flushBuffers_all has returned\n");
	//it returned and all worker thread are currently working on this data (it only returns when it has send a worker to work)

	//now wait for all workers to finish
	//do this by aquiring all semaphore slots and waiting for them to return again
	forEachCpuPassive(ultimap_flushBuffers_all,0);
	DbgPrint("ultimap_flushBuffers_all has returned a second time\n"); //this means that the previous blocks have been dealt with

}


NTSTATUS ultimap_continue(PULTIMAPDATAEVENT data)
/*
Called from usermode to signal that the data has been handled
*/
{
	DbgPrint("ultimap_continue\n");
	MmUnmapLockedPages((PVOID)data->Address, (PMDL)data->Mdl);
	IoFreeMdl((PMDL)data->Mdl);

	ExFreePool((PVOID)data->KernelAddress); //this memory is not needed anymore


	if (DataBlock)
		DataBlock[data->Block].Available=TRUE;


	KeReleaseSemaphore(&DataBlockSemaphore, 1, 1, FALSE); //Let the next block go through
	DbgPrint("Released semaphore\n");
	return STATUS_SUCCESS;	
}

NTSTATUS ultimap_waitForData(ULONG timeout, PULTIMAPDATAEVENT data)
/*
Called from usermode to wait for data
*/
{
	NTSTATUS r;
	LARGE_INTEGER wait;

	PKWAIT_BLOCK waitblock;

	if (DataBlock)
	{		
		waitblock=ExAllocatePool(NonPagedPool, MaxDataBlocks*sizeof(KWAIT_BLOCK));


		wait.QuadPart=-10000LL * timeout;

		//Wait for the events in the list
		//If an event is triggered find out which one is triggered, then map that block into the usermode space and return the address and block
		//That block will be needed to continue

		if (timeout==0xffffffff) //infinite wait
			r=KeWaitForMultipleObjects(MaxDataBlocks, DataReadyPointerList, WaitAny, UserRequest, UserMode, TRUE, NULL, waitblock);
		else
			r=KeWaitForMultipleObjects(MaxDataBlocks, DataReadyPointerList, WaitAny, UserRequest, UserMode, TRUE, &wait, waitblock);

		ExFreePool(waitblock);

		data->Block=r-STATUS_WAIT_0;

		if ((data->Block <= MaxDataBlocks) && (DataBlock))
		{
			//Map this block to usermode
			data->KernelAddress=(UINT64)DataBlock[data->Block].Data;
			(PMDL)data->Mdl=IoAllocateMdl(DataBlock[data->Block].Data, DataBlock[data->Block].DataSize, FALSE, FALSE, NULL);

			MmBuildMdlForNonPagedPool((PMDL)data->Mdl);

			data->Address=(UINT_PTR)MmMapLockedPagesSpecifyCache((PMDL)data->Mdl, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
			data->Size=DataBlock[data->Block].DataSize;

			return STATUS_SUCCESS;	
		}
		else
			return STATUS_UNSUCCESSFUL;
		
	}
	else
		return STATUS_UNSUCCESSFUL;


	
}

int perfmon_interrupt_centry(void)
{
	KIRQL old;
	
	void *temp;
	int causedbyme=(DS_AREA[cpunr()]->BTS_IndexBaseAddress>=DS_AREA[cpunr()]->BTS_InterruptThresholdAddress);
	UINT_PTR blocksize;

	if (causedbyme)
	{
		//undo the system flags that got set by this interrupt
		APIC_BASE->LVT_Performance_Monitor.a=APIC_BASE->LVT_Performance_Monitor.a & 0xff;
		APIC_BASE->EOI.a=0;
	}

	blocksize=DS_AREA[cpunr()]->BTS_IndexBaseAddress-DS_AREA[cpunr()]->BTS_BufferBaseAddress;
	
	{	
		old=KeRaiseIrqlToDpcLevel();
		enableInterrupts();

		DbgPrint("Entry cpunr=%d\n", cpunr());
		DbgPrint("Entry threadid=%d\n", PsGetCurrentThreadId());
		

		temp=ExAllocatePool(NonPagedPool, blocksize);
		if (temp)
		{
			RtlCopyMemory(temp, (PVOID *)DS_AREA[cpunr()]->BTS_BufferBaseAddress, blocksize);

			DbgPrint("temp=%p\n", temp);


			DS_AREA[cpunr()]->BTS_IndexBaseAddress=DS_AREA[cpunr()]->BTS_BufferBaseAddress; //don't reset on alloc error	
		}
		else
		{
			DbgPrint("ExAllocatePool has failed\n");
			KeLowerIrql(old);
			disableInterrupts();
			return causedbyme;
		}

		


		
		KeLowerIrql(old);
		//should be passive mode, taskswitches and cpu switches will happen now (When this returns, I may not be on the same interrupt as I was when I started)


		if (SaveToFile)
		{
			IO_STATUS_BLOCK iosb;
			NTSTATUS r;

			//Instead of sending the data to a usermode app it was chosen to store the data to a file for later usage
			DbgPrint("Writing buffer to disk\n");			
			r=ZwWriteFile(FileHandle, NULL, NULL, NULL, &iosb,  temp, (ULONG)blocksize, NULL, NULL); 
			DbgPrint("Done Writing. Result=%x\n", r);			
		}
		else
		{
			DbgPrint("Waiting till there is a block free\n");
			//When all workers are busy do not continue
			if ((DataBlock) && (KeWaitForSingleObject(&DataBlockSemaphore, Executive, KernelMode, FALSE, NULL) == STATUS_SUCCESS))
			{
				int currentblock;
				int i;

				//Enter a critical section and choose a block
				DbgPrint("Acquired semaphore. Now picking a usable datablock\n");

				
				ExAcquireFastMutex(&DataBlockMutex);
				DbgPrint("Acquired mutex. Looking for a Datablock that can be used\n");

				if (DataBlock)
				{
					currentblock=-1;
					for (i=0; i< MaxDataBlocks; i++)
					{
						if (DataBlock[i].Available) //look for a block that is set as available
						{
							currentblock=i;
							DataBlock[currentblock].Available=FALSE; //not available anymore
							break;
						}
					}
				}
				else currentblock=-1;

				ExReleaseFastMutex(&DataBlockMutex);
				DbgPrint("Released mutex\n");

				if (currentblock>=0) 
				{				
					
						

					DbgPrint("Using datablock %d\n", currentblock);
					DataBlock[currentblock].Data=temp;
					DataBlock[currentblock].DataSize=(int)blocksize;
					
					DbgPrint("Calling KeSetEvent/KeWaitForSingleObject\n");
					KeSetEvent(&DataBlock[currentblock].DataReady, 1, FALSE); //Trigger a worker thread to start working

					
				}				


			}
			
		}


		


		//and return to the caller process
		disableInterrupts();
		return causedbyme;

	}

	return causedbyme;
}


#ifdef AMD64
extern void perfmon_interrupt();
#else
_declspec( naked ) void perfmon_interrupt( void )
{
	__asm{
		push ebp
		mov ebp,esp

		//store state
		pushad
		xor eax,eax
		mov ax,ds
		push eax

		mov ax,es
		push eax

		mov ax,fs
		push eax

		mov ax,gs
		push eax

		mov ax,0x23 //0x10 should work too, but even windows itself is using 0x23
		mov ds,ax
		mov es,ax
		mov gs,ax
		mov ax,0x30
		mov fs,ax

		call perfmon_interrupt_centry
		cmp eax,1	//set flag


		//restore state
		pop gs
		pop fs
		pop es
		pop ds
		popad

		pop ebp		

		je skip_original_perfmon

		jmp far [perfmonJumpBackLocation]

skip_original_perfmon:
		add esp,4 //undo errorcode push
		iretd
	}
}
#endif


VOID ultimap_disable_dpc(IN struct _KDPC *Dpc, IN PVOID DeferredContext, IN PVOID SystemArgumen1, IN PVOID SystemArgument2)
{
	int i;
	DbgPrint("ultimap_disable()\n");
	i=vmx_ultimap_disable();

	if (DS_AREA[cpunr()])
	{
		ExFreePool(DS_AREA[cpunr()]);
		DS_AREA[cpunr()]=NULL;
	}
}

void ultimap_disable(void)
{
	if (DataBlock)
	{
		int i;
		_DataBlock *tempDataBlock=DataBlock;

		forEachCpu(ultimap_disable_dpc, NULL, NULL, NULL);

		if (SaveToFile && FileHandle) 
		{		
			ZwClose(FileHandle);
			FileHandle=NULL;
		}

		//all logging should have stopped now
		DataBlock=NULL;

		
		//Trigger all events waking up each thread that was waiting for the events
		for (i=0; i<MaxDataBlocks; i++)
			KeSetEvent(&tempDataBlock[i].DataReady,0, FALSE);
		
		ExFreePool(tempDataBlock);
	
		if (DataReadyPointerList)
		{
			ExFreePool(DataReadyPointerList);
			DataReadyPointerList=NULL;		
		}

		

	}
}


VOID ultimap_setup_dpc(IN struct _KDPC *Dpc, IN PVOID  DeferredContext, IN PVOID  SystemArgument1, IN PVOID  SystemArgument2)
/*
initializes ultimap. If the DS_AREA_SIZE is bigger than 0 it will allocate the required region (the usual option, but if not used it could be a LBR only thing)
Call this for each processor
*/
{
	struct
	{
		UINT64 cr3;
		UINT64 dbgctl_msr;
		int DS_AREA_SIZE;
	} *params;

	params=DeferredContext;

	DS_AREA_SIZE=params->DS_AREA_SIZE;
	

	DbgPrint("ultimap(%llx, %llx, %d)", params->cr3, params->dbgctl_msr, params->DS_AREA_SIZE);
	DS_AREA[cpunr()]=NULL;

	if (params->DS_AREA_SIZE)
	{
		DS_AREA[cpunr()]=ExAllocatePool(NonPagedPool, params->DS_AREA_SIZE);

		DbgPrint("DS_AREA[%d]=%p", cpunr(), DS_AREA[cpunr()]);

		//Initialize the DS_AREA 

		DS_AREA[cpunr()]->BTS_BufferBaseAddress=(QWORD)DS_AREA[cpunr()]+sizeof(DS_AREA_MANAGEMENT);
        DS_AREA[cpunr()]->BTS_BufferBaseAddress+=sizeof(BTS);

        DS_AREA[cpunr()]->BTS_BufferBaseAddress-=DS_AREA[cpunr()]->BTS_BufferBaseAddress % sizeof(BTS);

        DS_AREA[cpunr()]->BTS_IndexBaseAddress=DS_AREA[cpunr()]->BTS_BufferBaseAddress;
        DS_AREA[cpunr()]->BTS_AbsoluteMaxAddress=(QWORD)DS_AREA[cpunr()]+params->DS_AREA_SIZE-sizeof(BTS);
        DS_AREA[cpunr()]->BTS_AbsoluteMaxAddress-=DS_AREA[cpunr()]->BTS_AbsoluteMaxAddress % sizeof(BTS);
        DS_AREA[cpunr()]->BTS_AbsoluteMaxAddress++;

        DS_AREA[cpunr()]->BTS_InterruptThresholdAddress=(DS_AREA[cpunr()]->BTS_AbsoluteMaxAddress-1) - 4*sizeof(BTS);
	}
	

	if (params->dbgctl_msr & (1 << 8))
	{
		//hook the perfmon interrupt. First get the interrupt assigned (usually 0xfe, but let's be sure and read it from the apic)		

		int perfmonIVT=(APIC_BASE->LVT_Performance_Monitor.a) & 0xff;

		APIC_BASE->LVT_Performance_Monitor.a=perfmonIVT; //clear mask flag if it was set

	

		if (inthook_HookInterrupt((unsigned char)perfmonIVT, getCS(), (ULONG_PTR)perfmon_interrupt, &perfmonJumpBackLocation))
			DbgPrint("Interrupt hooked\n");
		else
			DbgPrint("Failed to hook interrupt\n");

	}

	//and finally activate the mapping
	vmx_ultimap((UINT_PTR)params->cr3, params->dbgctl_msr, DS_AREA[cpunr()]);
}


NTSTATUS ultimap(UINT64 cr3, UINT64 dbgctl_msr, int DS_AREA_SIZE, BOOL savetofile, WCHAR *filename, int handlerCount)
{
	struct
	{
		UINT64 cr3;
		UINT64 dbgctl_msr;
		int DS_AREA_SIZE;
	} params;
	int i;

	if (handlerCount>64)
		return STATUS_UNSUCCESSFUL;



	//create file
	SaveToFile=savetofile;

	if (SaveToFile)
	{
		UNICODE_STRING usFile;	
		OBJECT_ATTRIBUTES oaFile;
		IO_STATUS_BLOCK iosb;
		NTSTATUS r;

		RtlInitUnicodeString(&usFile, filename);
		InitializeObjectAttributes(&oaFile,&usFile, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL,NULL);

		DbgPrint("Creating file %S", usFile.Buffer);

		FileHandle=0;
		r=ZwCreateFile(&FileHandle,SYNCHRONIZE|FILE_READ_DATA|FILE_APPEND_DATA | GENERIC_ALL,&oaFile,&iosb,0,FILE_ATTRIBUTE_NORMAL,0,FILE_SUPERSEDE, FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT,NULL,0);
		DbgPrint("ZwCreateFile=%x\n", r);


	}

	MaxDataBlocks=handlerCount;
	KeInitializeSemaphore(&DataBlockSemaphore, MaxDataBlocks, MaxDataBlocks);
	ExInitializeFastMutex(&DataBlockMutex);

	//Datablock inits

	DataBlock=ExAllocatePool(NonPagedPool, sizeof(_DataBlock) * MaxDataBlocks);
	DataReadyPointerList=ExAllocatePool(NonPagedPool, sizeof(PVOID) * MaxDataBlocks);

	if ((DataBlock) && (DataReadyPointerList))
	{


		for (i=0; i< MaxDataBlocks; i++)
		{
			//DataBlock[i]->
			DataBlock[i].Data=NULL;

			KeInitializeEvent(&DataBlock[i].DataReady, SynchronizationEvent , FALSE);

			DataBlock[i].Available=TRUE;

			DataReadyPointerList[i]=&DataBlock[i].DataReady;
		}


		params.cr3=cr3;
		params.dbgctl_msr=dbgctl_msr;
		params.DS_AREA_SIZE=DS_AREA_SIZE;

		forEachCpu(ultimap_setup_dpc, &params, NULL, NULL);
		return STATUS_SUCCESS;
	}
	else
	{
		DbgPrint("Failure allocating DataBlock and DataReadyPointerList\n");
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	
}