/*************           ��8������       ***************/
/*************           ö�ٽ���        ***************/
#include <ntddk.h>
#include <windef.h>
#include "ListProc.h"


WCHAR gDeviceName[]=L"\\Device\\proclistdrv";
WCHAR gDosDeviceName[]=L"\\??\\proclistdrv";

//_OBJECT_HEADER�ṹ�Լ����ڸýṹʵ�ִӶ����壩ָ���ö���ͷ��ָ��ĺ�
typedef struct _OBJECT_HEADER {
    union {
        struct {
            LONG PointerCount;
            LONG HandleCount;
        };
        LIST_ENTRY Entry;
    };
    POBJECT_TYPE Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;

    union {
        //POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER(obj)         CONTAINING_RECORD( (obj), OBJECT_HEADER, Body )
//ϵͳƫ��������ϵͳ���죬��ʹ��Windbg��ѯ��
#define TYPE 0X08               //_OBJECT_HEADER�е�ƫ��
#define NEXTFREETABLEENTRY 0X04 //_HANDLE_TABLE_ENTRY�е�ƫ��
#define OFFSET_EPROCESS_IMAGENAME 0x0
#define OFFSET_EPROCESS_PID       0x1
#define OFFSET_EPROCESS_FLAGS     0x2
ProcessInfo *head,*p;

//��������
NTSTATUS DriverEntry(IN PDRIVER_OBJECT		DriverObject,IN PUNICODE_STRING		RegistryPath);

// ����Ӧ�ò����֧��
NTSTATUS
ProcCreateClose (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

// ��������Ӧ�ò�Ŀ���
NTSTATUS
ProcDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);
VOID Unload(IN PDRIVER_OBJECT		DriverObject);
ULONG GetCidAddr();
ULONG GetProcessType();
VOID IsValidProcess();
VOID RecordInfo(ULONG i);
DWORD	GetPlantformDependentInfo(DWORD	eprocessflag);
//DriverEntry��������
NTSTATUS DriverEntry(IN PDRIVER_OBJECT		DriverObject,IN PUNICODE_STRING		RegistryPath)
{
	UNICODE_STRING DeviceName;
	UNICODE_STRING DosDeviceName;
	NTSTATUS Status;
	PDEVICE_OBJECT pDeviceObject=NULL;

	DriverObject->DriverUnload=Unload;

  	DriverObject->MajorFunction[IRP_MJ_CREATE]         = ProcCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]          = ProcCreateClose;	 
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcDeviceControl;

	RtlInitUnicodeString(&DeviceName,gDeviceName);
	RtlInitUnicodeString(&DosDeviceName,gDosDeviceName);
	IoCreateDevice(DriverObject,0,&DeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&pDeviceObject);
	pDeviceObject->Flags|=DO_BUFFERED_IO;
	Status = IoCreateSymbolicLink(&DosDeviceName,&DeviceName);
	if(Status)
		DbgPrint("IoCreateSymbolicLink Return %0x\n",Status);

	IsValidProcess(); 
	return TRUE;
}

//Unload��������
VOID Unload(IN PDRIVER_OBJECT		DriverObject)
{
	DbgPrint("unload!");
}

//ͨ������PsLookupProcessByProcessId��������ȡPspCidTable�ĵ�ַ
ULONG GetCidAddr()
{
	PUCHAR addr;
	PUCHAR p;
	UNICODE_STRING          pslookup;	
	ULONG cid;
	
	RtlInitUnicodeString (&pslookup,
		L"PsLookupProcessByProcessId");	
		addr = (PUCHAR) MmGetSystemRoutineAddress(&pslookup);//MmGetSystemRoutineAddress����ͨ����������ú�����ַ
		for (p=addr;p<addr+PAGE_SIZE;p++)
		{
			if((*(PUSHORT)p==0x35ff)&&(*(p+6)==0xe8))
			{
				cid=*(PULONG)(p+2);
				return cid;
				break;
			}
		}
		return 0;
}

//ͨ����ǰ���̻�ȡ���̶��������ָ��
ULONG GetProcessType()
{
	ULONG eproc;
	ULONG type;
	ULONG total;
	eproc=(ULONG)PsGetCurrentProcess();//PsGetCurrentProcess��ȡ��ǰ����̵ĵ�ַ��ʵ���Ͼ��Ƕ����壩ָ��
	eproc=(ULONG)OBJECT_TO_OBJECT_HEADER(eproc);
	type=*(PULONG)(eproc+TYPE);
	return type;
}

//�ж��Ƿ��ǽ��̶��������¼�����������
VOID IsValidProcess()
{
	ULONG PspCidTable;
	ULONG TableCode;
	ULONG table1,table2;
	ULONG object,objectheader;
	ULONG NextFreeTableEntry;
	ULONG processtype,type;
	ULONG flags;
	ULONG i;
	PspCidTable=GetCidAddr();
	processtype=GetProcessType();
	if(PspCidTable==0)
	{
		return ;
	}
	else
	{
		//TableCode�������λ��XP�о����˾����Ĳ���
		TableCode=*(PULONG)(*(PULONG)PspCidTable);
		if((TableCode&0x3)==0x0)
		{
			table1=TableCode;
			table2=0x0;
		}
		if((TableCode&0x3)==0x1)
		{
			TableCode=TableCode&0xfffffffc;
			table1=*(PULONG)TableCode;
			table2=*(PULONG)(TableCode+0x4);
		}
		//��cid��0x0��0x4e1c���б���
		for(i=0x0;i<0x4e1c;i++)
		{
			if(i<=0x800)
			{	
				if(MmIsAddressValid((PULONG)(table1+i*2)))
				{
					object=*(PULONG)(table1+i*2);
					if(MmIsAddressValid((PULONG)(table1+i*2+NEXTFREETABLEENTRY)))
					{
						NextFreeTableEntry=*(PULONG)(table1+i*2+NEXTFREETABLEENTRY);
				    	if(NextFreeTableEntry==0x0)//������_HANDLE_TABLE_ENTRY��NextFreeTableEntryΪ0x0
						{
					    	object=((object | 0x80000000)& 0xfffffff8);//ת��Ϊ�����壩ָ��
		    		    	objectheader=(ULONG)OBJECT_TO_OBJECT_HEADER(object);//��ȡ����ͷ��ָ��
			    	    	if(MmIsAddressValid((PULONG)(objectheader+TYPE)))
							{
					        	type=*(PULONG)(objectheader+TYPE);
				            	if(type==processtype)
								{
						    		flags=*(PULONG)((ULONG)object+GetPlantformDependentInfo(OFFSET_EPROCESS_FLAGS));
						    		if((flags&0xc)!=0xc)
						    		RecordInfo(object);//flags��ʾ����û���˳�
								}
							}
						}
					}
				}
			}
			else
			{
				if(table2!=0)
				{
					if(MmIsAddressValid((PULONG)(table2+(i-0x800)*2)))
					{
						object=*(PULONG)(table2+(i-0x800)*2);
						if(MmIsAddressValid((PULONG)((table2+(i-0x800)*2)+NEXTFREETABLEENTRY)))
						{
							NextFreeTableEntry=*(PULONG)((table2+(i-0x800)*2)+NEXTFREETABLEENTRY);
					    	if(NextFreeTableEntry==0x0)
							{
					    		object=((object | 0x80000000)& 0xfffffff8);
					        	objectheader=(ULONG)OBJECT_TO_OBJECT_HEADER(object);
					        	if(MmIsAddressValid((PULONG)(objectheader+TYPE)))
								{
					            	type=*(PULONG)(objectheader+TYPE);
				                	if(type==processtype)
									{	
							    		flags=*(PULONG)((ULONG)object+GetPlantformDependentInfo(OFFSET_EPROCESS_FLAGS));
							    	    if((flags&0xc)!=0xc)
							        	RecordInfo(object);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

//ʹ�õ��������¼������Ϣ
VOID RecordInfo(ULONG i)
{
	ProcessInfo *r;
	if(head==NULL)
	{
		if((head=(ProcessInfo *)ExAllocatePool(NonPagedPool,sizeof(ProcessInfo)))==NULL)
		{
			return;
		}
		head->addr=0x0;
	}
	if (head->addr==0x0)
	{
		head->addr=i;
		p=head;
	}
	else
	{
		if((r=(ProcessInfo *)ExAllocatePool(NonPagedPool,sizeof(ProcessInfo)))==NULL)
		{
			return;
		}
		p->next=r;
		r->addr=i;
		r->next=NULL;
		p=r;
	}
}
///////////////////////////////////////////////////////////////////////////////
// ProcCreateClose
// ����Ӧ�ò����
//
NTSTATUS 
ProcCreateClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{	
	DbgPrint(" Create or Close ok...\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// ProcDeviceControl
// Ӧ�ó������λ��
//
NTSTATUS
ProcDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{   
	PIO_STACK_LOCATION  io_stack;
    NTSTATUS            status;

    io_stack = IoGetCurrentIrpStackLocation(Irp);

 	if (io_stack->MajorFunction==IRP_MJ_DEVICE_CONTROL) 
	{
		switch (io_stack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_GETPROC_LIST:
			{
				ProcessInfo* pi = (ProcessInfo*) Irp->AssociatedIrp.SystemBuffer;
				ProcessInfo* q;  
				int j = 0;
				//��ȡ���̵�ID�ͽ�����
				for (p=head;p;p=p->next)
				{
					p->pid=*(int *)(p->addr+GetPlantformDependentInfo(OFFSET_EPROCESS_PID));
    				strcpy(p->name,(UCHAR *)(p->addr+GetPlantformDependentInfo(OFFSET_EPROCESS_IMAGENAME)));
				}
				for(p=head;p;p=p->next)
				{
					ProcessInfo* pnext;
					pi->addr = p->addr;
					RtlCopyMemory(pi->name, p->name, 16);
					pi->pid = p->pid;				
					if(p->next)
						pnext = (++pi);
					else 
						pnext = NULL;
					pi->next = pnext;
					pi = pnext;
				}
				//�ͷ�����
				p=head;
				q=p->next;
				while(q!=NULL)
				{
					ExFreePool(p);
					p=q;
					q=p->next;
				}
				ExFreePool(p);
			}
			return STATUS_SUCCESS;
		}
	}
	return STATUS_SUCCESS;
}
DWORD	GetPlantformDependentInfo(DWORD	eprocessflag)
{
	DWORD current_build;
	DWORD ans = 0;

	PsGetVersion(NULL, NULL, &current_build, NULL); 

	switch(eprocessflag){
		case OFFSET_EPROCESS_IMAGENAME:
			if (current_build ==  2195) //2000
			{

				ans = 0x1FC;
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
		case OFFSET_EPROCESS_PID:
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
				ans = 0x94; 
			}
			break;
		case OFFSET_EPROCESS_FLAGS:
//			if (current_build ==  2195) //2000
//			{
//2000��ò��û�������־,��������û�ҵ�
//				ans = 0x09c;      
//			}
			if (current_build ==  2600) //XP
			{
				ans = 0x248;

			}
			if (current_build ==  3790) //2003
			{
				ans = 0x240; 
			}
			break;
		default:
			break;
	}

	return ans;
}