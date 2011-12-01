#include <ntddk.h>

#define MAX_ENT_CNT (0x1000/8)
#define MAX_ADD_CNT (0x1000/4)

#include "PspCidTable3.h"

extern POBJECT_TYPE *PsProcessType;

NTSTATUS DriverEntry( PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS;
	pDriverObject->DriverUnload = UnLoad;

	DisplayInfo();
	return Status;
}

void DisplayInfo()
{
	ULONG pCidTableAddr = 0;
	PHANDLE_TABLE pCidHandleTable = NULL;
	PHANDLE_TABLE_ENTRY pTable1, *pTable2, **pTable3;
	ULONG pRealHandleTable = 0;
	ULONG level = 0;
	ULONG uMax_Handle = 0;

	pCidTableAddr = GetCidTableAddr();
	pCidHandleTable = (PHANDLE_TABLE)(*(PULONG)pCidTableAddr);
	level = (pCidHandleTable->TableCode) & 0x3;
	pRealHandleTable = (pCidHandleTable->TableCode) & ~0x3;
	uMax_Handle = pCidHandleTable->NextHandleNeedingPool;

	switch( level )
	{
	case 0:
		{
			ULONG index = 0;
			pTable1 = (PHANDLE_TABLE_ENTRY)(pRealHandleTable);
			for( index = 0; index < MAX_ENT_CNT; index++ )
			{
				if( pTable1[index].Object != NULL )
				{
					ULONG pObject = (ULONG)(pTable1[index].Object) & ~7 ;
					if( MmIsAddressValid((PULONG)(pObject - 0x10)) )
					{
						POBJECT_TYPE pType = (POBJECT_TYPE)(*(PULONG)(pObject - 0x10));
						if( pType == *PsProcessType )
						{
							DbgPrint("PId:%d\tPath:%s\n", index*4, PsGetProcessImageFileName((PEPROCESS)pObject) );
						}
					}
					
				}
			}
			break;
		}
	case 1:
		{
			ULONG index = 0;
			pTable2 = (PHANDLE_TABLE_ENTRY*)(pRealHandleTable);
			for( index = 0; index < uMax_Handle/(4*MAX_ENT_CNT); index++ )
			{
				pTable1 = pTable2[index];
				if( pTable1 == NULL )
					break;
				else
				{
					ULONG i = 0;
					for( i = 0; i < MAX_ENT_CNT; i++ )
					{
						if( pTable1[i].Object != NULL )
						{
							ULONG pObject = (ULONG)(pTable1[i].Object) & ~7;
							if( MmIsAddressValid( (PULONG)(pObject-0x10) ) )
							{
								POBJECT_TYPE pType = (POBJECT_TYPE)(*(PULONG)(pObject-0x10));
								if( pType == *PsProcessType )
								{
									DbgPrint("PId:%d\tPath:%s\n", index*MAX_ENT_CNT*4+i*4, PsGetProcessImageFileName((PEPROCESS)pObject) );
								}
							}
							
						}
					}
				}
			}
			break;
		}
	case 2:
		{
			ULONG index = 0;
			pTable3 = (PHANDLE_TABLE_ENTRY**)(pRealHandleTable);
			for( index = 0; index < uMax_Handle/(MAX_ADD_CNT*MAX_ENT_CNT*4); index++ )
			{
				ULONG i = 0;
				pTable2 = (PHANDLE_TABLE_ENTRY*)((ULONG)pTable3[index] & ~0x3);
				if( pTable2 == NULL )
					break;
				for( i = 0; i < MAX_ADD_CNT; i++ )
				{
					
					pTable1 = pTable2[i];
					if( pTable1 == NULL )
						break;
					else
					{
						ULONG j = 0;
						for( j = 0; j < MAX_ENT_CNT; j++ )
						{
							if( pTable1[j].Object != NULL )
							{
								ULONG pObject = (ULONG)(pTable1[j].Object) & ~7;
								if( MmIsAddressValid( (PULONG)(pObject-0x10) ) )
								{
									POBJECT_TYPE pType = (POBJECT_TYPE)(*(PULONG)(pObject-0x10));
									if( pType == *PsProcessType )
									{
										DbgPrint("PId:%d\tPath:%s\n", index*MAX_ADD_CNT*MAX_ENT_CNT*4+i*MAX_ENT_CNT*4+j*4,\
											PsGetProcessImageFileName((PEPROCESS)pObject) );
									}
								}
							}
							

						}

					}
				}

			}

		}
		break;
	}


}

ULONG GetCidTableAddr()
{
	ULONG pCidHandleAddr = 0;
	_asm
	{
		mov eax,fs:[0x34]
		mov eax,[eax+0x80]
		//mov eax,[eax]
		mov pCidHandleAddr,eax
	}
	return pCidHandleAddr;
}

void UnLoad( PDRIVER_OBJECT pDriverObject )
{
	DbgPrint("Driver Unload..");
}