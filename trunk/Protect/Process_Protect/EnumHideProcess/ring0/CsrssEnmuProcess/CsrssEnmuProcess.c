#include <ntddk.h>
#include "CsrssEnumProcess.h"

#define MAX_ENT_CNT (0x1000/0x8)
#define MAX_ADD_CNT (0x1000/0x4)
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
	PEPROCESS pEProcess = NULL;

	NTSTATUS Status = STATUS_SUCCESS;
	PHANDLE_TABLE pObjectTable = NULL;
	PHANDLE_TABLE_ENTRY table1, *table2, **table3;
	ULONG level;
	ULONG pRealHandleTable;
	ULONG uHandleCount;

	pEProcess = GetCsrssObject();

	pObjectTable = (PHANDLE_TABLE)(*(PULONG)((ULONG)pEProcess + 0xc4));
	level = pObjectTable->TableCode & 3;
	pRealHandleTable = pObjectTable->TableCode & ~3;
	uHandleCount = pObjectTable->NextHandleNeedingPool;
	switch( level )
	{
	case 0:
		{
			ULONG index = 0;
			table1 = (PHANDLE_TABLE_ENTRY)pRealHandleTable;
			for( index = 0; index < MAX_ENT_CNT; index++ )
			{
				ULONG pObject = (ULONG)(table1[index].Object) & ~7;
				if( MmIsAddressValid( (PULONG)(pObject+0x8) ) )
				{
					POBJECT_TYPE pType = (POBJECT_TYPE)(*(PULONG)(pObject+0x8));
					if( pType == *PsProcessType )
					{
						PEPROCESS pAddr = (PEPROCESS)(pObject+0x18);
						DbgPrint("PId:%d\tPath:%s\n", *(PULONG)((ULONG)pAddr+0x84), (PUCHAR)((ULONG)pAddr+0x174) );
					}
				}
			}
			break;

		}
	case 1:
		{
			ULONG index = 0;
			table2 = (PHANDLE_TABLE_ENTRY*)pRealHandleTable;
			for( index = 0; index < uHandleCount/(4*MAX_ENT_CNT); index++ )
			{
				ULONG i = 0;
				table1 = table2[index];
				if( table1 == NULL )
					break;
				for( i = 0; i < MAX_ENT_CNT; i++ )
				{
					ULONG pObject = (ULONG)(table1[i].Object) & ~7;
					if( MmIsAddressValid( (PULONG)(pObject+0x8) ) )
					{
						POBJECT_TYPE pType = (POBJECT_TYPE)(*(PULONG)(pObject+0x8));
						if( pType == *PsProcessType )
						{
							ULONG pAddr = (ULONG)(pObject+0x18);
							DbgPrint("PId:%d\tPath:%s\n", *(PULONG)(pAddr+0x84),  (PUCHAR)(pAddr+0x174) );
						}
					}

				}
				
			}
			break;
		}
	case 2:
		{
			ULONG index = 0;
			table3 = (PHANDLE_TABLE_ENTRY**)pRealHandleTable;
			for( index = 0; index < uHandleCount/(4*MAX_ENT_CNT*MAX_ADD_CNT); index++ )
			{
				ULONG i = 0;
				table2 = (PHANDLE_TABLE_ENTRY*)((ULONG)table3[index] & ~0x3);
				if( table2 == NULL )
					break;
				for( i = 0; i < MAX_ADD_CNT; i++ )
				{
					ULONG j = 0;
					table1 = table2[i];
					if( table1 == NULL )
						break;
					for( j = 0; j < MAX_ENT_CNT; j++ )
					{
						ULONG pObject = (ULONG)(table1[j].Object) & ~7;
						if( MmIsAddressValid( (PULONG)(pObject+0x8) ) )
						{
							POBJECT_TYPE pType = (POBJECT_TYPE)(*(PULONG)(pObject+0x8));
							if( pType == *PsProcessType )
							{
								ULONG pAddr = (ULONG)(pObject+0x18);
								DbgPrint("PId:%d\tPath:%s\n", *(PULONG)(pAddr+0x84),  (PUCHAR)(pAddr+0x174) );
							}
						}
					
					}
				}
			
			}
			break;
		}
	}

}

void UnLoad( PDRIVER_OBJECT pDriverObject )
{
	DbgPrint("Driver Unload..");
}

PEPROCESS GetCsrssObject()
{
	PEPROCESS pCurrent = PsGetCurrentProcess();
	PEPROCESS pTemp = pCurrent;
	do 
	{
		if( strcmp( (PUCHAR)((ULONG)pTemp+0x174), "csrss.exe") == 0 )
			break;
		pTemp = (PEPROCESS)(*(PULONG)((ULONG)pTemp + 0x88) - 0x88);
	} while (pCurrent != pTemp);
	return pTemp;
}