/*++

Copyright (c) 2009  Macrosoft Corporation

Module Name:

    Pnppower.c.c

Abstract:

    This module implements the File Cleanup routine for Ntfs called by the
    dispatch driver.

Author:

    WangBin      weolar         31-6-2009

Revision History:

--*/

#include "NtfsProc.h"

NTSTATUS
NtfsFsdPnp (
			IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
			IN PIRP Irp
				 );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsFsdPnp)
#endif

NTSTATUS
NtfsFsdPnp (
			IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
			IN PIRP Irp
				 )
/*++

  Routine Description:
  
	This routine we always return STATUS_SUCCESS for the moment. Later on we will complete it.
	
	Arguments:
	  
	VolumeDeviceObject - Volume's DeviceObject to checkpoint until done
		
	Irp - Irp from last DeviceObject
		  
	Return Value:
			
		STATUS_SUCCESS

							 
--*/
{
	UNREFERENCED_PARAMETER( VolumeDeviceObject );
    ASSERT_IRP( Irp );

	return STATUS_SUCCESS;
}

// 
// NTSTATUS
// NtfsFsdPnp (
// 			IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
// 			IN PIRP Irp
// 				 )
// {
// 	unsigned __int8 v3; // al@1
// 	char v4; // ST04_1@4
// 	char v5; // ST08_1@4
// 	int v6; // esi@5
// 	int v8; // eax@5
// 	int v9; // eax@9
// 	int v10; // edi@9
// 	PVOID Entry; // [sp+38h] [bp-1Ch]@1
// 	char v12; // [sp+Ch] [bp-48h]@5
// 	int v13; // [sp+34h] [bp-20h]@5
// 	CPPEH_RECORD ms_exc; // [sp+3Ch] [bp-18h]@5
// 	int v15; // [sp+2Ch] [bp-28h]@9
// 	
// 	Entry = 0;
// 	KeEnterCriticalRegion();
// 	v3 = *(_BYTE *)(*(_DWORD *)(a3 + 96) + 1);
// 	if ( v3 > 0u && (v3 <= 3u || v3 == 23) )
// 	{
// 		v5 = 0;
// 		v4 = 0;
// 	}
// 	else
// 	{
// 		v5 = 1;
// 		v4 = 1;
// 	}
// 	v8 = NtfsInitializeTopLevelIrp(&v12, v4, v5);
// 	v6 = v8;
// 	v13 = v8;
// 	ms_exc.disabled = 0;
// 	if ( !Entry )
// 	{
// 		NtfsInitializeIrpContext(a3, 1, &Entry);
// 		if ( !*(_DWORD *)(v6 + 24) )
// 		{
// 			*(_DWORD *)(v6 + 4) = 0x5346544Eu;
// 			*(_DWORD *)(v6 + 24) = Entry;
// 			*((_BYTE *)Entry + 10) |= 0x10u;
// 			IoSetTopLevelIrp(v6);
// 		}
// 		*((_DWORD *)Entry + 10) = *(_DWORD *)(v6 + 24);
// 	}
// 	v9 = NtfsCommonPnp(a1, Entry, (IRP **)&a3);
// 	v10 = v9;
// 	v15 = v9;
// 	ms_exc.disabled = -1;
// 	KeLeaveCriticalRegion();
// 	return v10;
// }
