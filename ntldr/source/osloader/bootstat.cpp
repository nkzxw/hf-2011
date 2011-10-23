//********************************************************************
//	created:	18:8:2008   18:57
//	file:		bootstat.cpp
//	author:		tiamo
//	purpose:	boot status
//********************************************************************

#include "stdafx.h"

//
// lock status data
//
ARC_STATUS BlLockBootStatusData(__in_opt ULONG DeviceId,__in PCHAR DevicePath,__in PCHAR LoadPath,__out PULONG FileId)
{
	//
	// open device if needed
	//
	ULONG RealDeviceId									= DeviceId;
	ARC_STATUS Status									= ESUCCESS;
	if(!RealDeviceId)
		Status											= ArcOpen(DevicePath,ArcOpenReadWrite,&RealDeviceId);

	if(Status != ESUCCESS)
		return Status;

	//
	// build file name
	//
	CHAR FileName[100];
	strcpy(FileName,LoadPath);
	strcat(FileName,"\\bootstat.dat");

	//
	// open the file
	//
	Status												= BlOpen(RealDeviceId,FileName,ArcOpenReadWrite,FileId);

	//
	// if we opened the device,close it
	//
	if(!DeviceId)
		ArcCacheClose(RealDeviceId);

	return Status;
}

//
// unlock boot status
//
ARC_STATUS BlUnlockBootStatusData(__in ULONG FileId)
{
	return BlClose(FileId);
}

//
// get set boot status data
//
ARC_STATUS BlGetSetBootStatusData(__in ULONG FileId,__in BOOLEAN GetData,__in ULONG DataType,__inout PVOID Buffer,__in ULONG BufferLength,__out_opt PULONG ActualLength)
{
	static ULONG LengthFromType[6]						= {4,4,1,1,1,1};
	static ULONG OffsetFromType[6]						= {0,4,8,9,10,11};

	//
	// seek to beginning
	//
	LARGE_INTEGER SeekOffset;
	SeekOffset.QuadPart									= 0;
	ARC_STATUS Status									= ArcSeek(FileId,&SeekOffset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	//
	// read first ULONG
	//
	ULONG Value											= 0;
	ULONG Count											= 0;
	Status												= ArcRead(FileId,&Value,sizeof(Value),&Count);
	if(Status != ESUCCESS)
		return Status;

	//
	// check data type is supported
	//
	if(DataType >= ARRAYSIZE(LengthFromType))
		return EINVAL;

	//
	// check file version
	//	HACK HACK HACK what's return value?
	//
	if(LengthFromType[DataType] + OffsetFromType[DataType] > Value)
		return STATUS_REVISION_MISMATCH;

	//
	// check buffer length
	//
	if(BufferLength < LengthFromType[DataType])
		return EINVAL;

	//
	// seek to the position
	//
	SeekOffset.QuadPart									= OffsetFromType[DataType];
	Status												= ArcSeek(FileId,&SeekOffset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	//
	// get or set the data
	//
	if(GetData)
		Status											= ArcRead(FileId,Buffer,LengthFromType[DataType],&Count);
	else
		Status											= ArcWrite(FileId,Buffer,LengthFromType[DataType],&Count);

	if(Status != ESUCCESS)
		return Status;

	if(ActualLength)
		*ActualLength									= Count;

	return ESUCCESS;
}

//
// get last boot status
//
ULONG BlGetLastBootStatus(__in ULONG FileId,__out PULONG BootStatus)
{
	*BootStatus											= 0;

	//
	// read data type = 4 (offset = 0x0a,length = 0x01)
	//
	UCHAR Value4										= 0;
	if(BlGetSetBootStatusData(FileId,TRUE,4,&Value4,sizeof(Value4),0) != ESUCCESS)
		return 0xffffffff;

	//
	// read data type = 5 (offset = 0x0b,length = 0x01)
	//
	UCHAR Value5										= 0;
	if(BlGetSetBootStatusData(FileId,TRUE,5,&Value5,sizeof(Value5),0) != ESUCCESS)
		return 0xffffffff;

	//
	// read data type = 2 (offset = 0x08,length = 0x01)
	//
	UCHAR Value2										= 0;
	if(BlGetSetBootStatusData(FileId,TRUE,2,&Value2,sizeof(Value2),0) != ESUCCESS)
		return 0xffffffff;

	*BootStatus											= 0;
	if(Value5)
		return 0xffffffff;

	//
	// index = 0, <-> SAFEBOOT:MINIMAL SOS BOOTLOG NOGUIBOOT
	// index = 6, <-> LAST KNOWN GOOD
	//
	ULONG AdvancedBootIndex								= 0;
	*BootStatus											= 2;
	if(Value4)
		*BootStatus										= 3;
	else
		AdvancedBootIndex								= 6;

	if(!Value2)
		return 0xffffffff;

	return AdvancedBootIndex;
}

//
// write boot status flags
//
ARC_STATUS BlWriteBootStatusFlags(__in_opt ULONG DeviceId,__in PCHAR LoadPath,__in UCHAR Value4,__in UCHAR Value5)
{
	//
	// open the file
	//
	ULONG FileId										= 0;
	ARC_STATUS Status									= BlLockBootStatusData(DeviceId,0,LoadPath,&FileId);
	if(Status != ESUCCESS)
		return Status;

	//
	// write value4
	//
	BlGetSetBootStatusData(FileId,FALSE,4,&Value4,sizeof(Value4),0);

	//
	// write value5
	//
	BlGetSetBootStatusData(FileId,FALSE,5,&Value5,sizeof(Value5),0);

	//
	// unlock it
	//
	return BlUnlockBootStatusData(FileId);
}