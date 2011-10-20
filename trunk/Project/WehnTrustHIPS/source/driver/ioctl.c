/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

//
// Device names and aliases
//
#define DEVICE_NAME     L"\\Device\\wehntrust"
#define DOS_DEVICE_NAME L"\\DosDevices\\wehntrust"

static NTSTATUS DeviceOpen(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp);
static NTSTATUS DeviceClose(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp);
static NTSTATUS DeviceIoControl(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp);

#pragma code_seg("INIT")

//
// Register the control device that allows user-mode to interact with the
// driver.
//
NTSTATUS DeviceRegister(
		IN PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING NtName;
	UNICODE_STRING DosName;
	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS       Status;

	do
	{
		RtlInitUnicodeString(
				&NtName,
				DEVICE_NAME);

		//
		// Create the device object
		//
		if (!NT_SUCCESS(Status = IoCreateDevice(
				DriverObject,
				0,
				&NtName,
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE,
				&DeviceObject)))
		{
			DebugPrint(("DeviceRegister(): IoCreateDevice failed, %.8x.",
					Status));
			break;
		}

		//
		// Initialize the driver's major function dispatch table
		//
		DriverObject->MajorFunction[IRP_MJ_CREATE]         = DeviceOpen;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControl;
		DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DeviceClose;

		//
		// Create the symbolic link for accessing the device from usermode
		//
		RtlInitUnicodeString(
				&DosName,
				DOS_DEVICE_NAME);

		if (!NT_SUCCESS(Status = IoCreateSymbolicLink(
				&DosName,
				&NtName)))
		{
			DebugPrint(("DeviceRegister(): CreateSymbolicLink failed, %.8x.",
					Status));
			break;
		}

		//
		// Unset the device initializing flag so that we can start using it
		//
		DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

		//
		// Expressly
		//
		Status = STATUS_SUCCESS;

	} while (0);

	//
	// On failure, cleanup the device object if it was allocated
	//
	if (!NT_SUCCESS(Status))
	{
		if (DeviceObject)
			IoDeleteDevice(
					DeviceObject);
	}

	return Status;
}

#pragma code_seg()

//
// IRP_MJ_CREATE device handler
//
static NTSTATUS DeviceOpen(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
	Irp->IoStatus.Status      = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(
			Irp,
			IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// IRP_MJ_CLOSE device handler
//
static NTSTATUS DeviceClose(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
	Irp->IoStatus.Status      = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(
			Irp,
			IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// Handles IOCTL requests to the device for obtaining things such as statistics
// and for stopping/starting randomization, among other things.
//
static NTSTATUS DeviceIoControl(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	NTSTATUS           Status = STATUS_SUCCESS;
	ULONG              ExemptionMethodAdd = 1;
	PVOID              InBuffer;
	PVOID              OutBuffer;
	ULONG              InBufferLength;
	ULONG              OutBufferLength;
	ULONG              Ioctl;

	//
	// Initialize information to zero
	//
	Irp->IoStatus.Information = 0;

	Stack = IoGetCurrentIrpStackLocation(Irp);

	//
	// If the major function for the calling IRP is not DeviceControl, simply
	// return status success.
	//
	if (Stack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
	{
		Irp->IoStatus.Status      = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
	
	//
	// Grab shorter pointers to work with
	//
	InBuffer        = Irp->AssociatedIrp.SystemBuffer;
	InBufferLength  = Stack->Parameters.DeviceIoControl.InputBufferLength;
	OutBuffer       = Irp->UserBuffer;
	OutBufferLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	Ioctl           = Stack->Parameters.DeviceIoControl.IoControlCode;

	//
	// Handle the individual I/O controls
	//
	switch (Ioctl)
	{
		//
		// Starts randomization subsystems from the supplied mask
		//
		case IOCTL_WEHNTRUST_START:
			if (InBufferLength < sizeof(ULONG))
				Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				DebugPrint(("DeviceIoControl(IOCTL_WEHNTRUST_START): Enabling randomization subsystems %.8x.",
						*(PULONG)InBuffer));

				EnableRandomizationSubsystems(
						*(PULONG)InBuffer,
						TRUE);
			}
			break;
		//
		// Stops randomization subsystems from the supplied mask
		//
		case IOCTL_WEHNTRUST_STOP:
			if (InBufferLength < sizeof(ULONG))
				Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				DebugPrint(("DeviceIoControl(IOCTL_WEHNTRUST_STOP): Disabling randomization subsystems %.8x.",
						*(PULONG)InBuffer));

				DisableRandomizationSubsystems(
						*(PULONG)InBuffer);
			}
			break;
		//
		// Query the driver for certain statistics
		//
		case IOCTL_WEHNTRUST_GET_STATISTICS:
			{
				WEHNTRUST_STATISTICS LocalStatistics;
				//
				// Ask for internal statistics to our locally managed statistic
				// buffer
				//
				if (!NT_SUCCESS(Status = GetExecutiveStatistics(
						&LocalStatistics)))
				{
					DebugPrint(("DeviceIoControl(IOCTL_WEHNTRUST_GET_STATISTICS): GetExecutiveStatistics failed, %.8x.",
							Status));
					break;
				}

				//
				// Do we have enough room to at least check the structure version
				// information?
				//
				if (OutBufferLength < sizeof(ULONG))
				{
					Status = STATUS_BUFFER_TOO_SMALL;
					break;
				}

				//
				// Get the version of the output structure
				//
				switch (GetStructureVersion(OutBuffer))
				{
					case 0x0001:
						if ((GetStructureSize(OutBuffer) != sizeof(WEHNTRUST_STATISTICS_V1)) ||
						    (OutBufferLength != sizeof(WEHNTRUST_STATISTICS_V1)))
							Status = STATUS_BUFFER_TOO_SMALL;
						else
						{
							PWEHNTRUST_STATISTICS_V1 V1 = (PWEHNTRUST_STATISTICS_V1)OutBuffer;

							V1->Enabled                        = LocalStatistics.Enabled;
							V1->NumberOfImageSets              = LocalStatistics.NumberOfImageSets;
							V1->NumberOfRandomizedDlls         = LocalStatistics.NumberOfRandomizedDlls;
							V1->NumberOfRandomizationsExempted = LocalStatistics.NumberOfRandomizationsExempted;
							V1->NumberOfRandomizedAllocations  = LocalStatistics.NumberOfRandomizedAllocations;
						}
						break;
					default:
						DebugPrint(("DeviceIoControl(IOCTL_WEHNTRUST_GET_STATISTICS): Invalid structure version, %.4x.",
								GetStructureVersion(OutBuffer)));

						Status = STATUS_INVALID_PARAMETER;
						break;
				}
			}
			break;
		//
		// Add or remove an application exemption
		//
		case IOCTL_WEHNTRUST_ADD_EXEMPTION:
		case IOCTL_WEHNTRUST_REMOVE_EXEMPTION:
		case IOCTL_WEHNTRUST_FLUSH_EXEMPTIONS:
			// 
			// If the buffer is at least four bytes in size...
			//
			if (InBufferLength > sizeof(ULONG))
			{
				UNICODE_STRING  ExemptionPath;
				EXEMPTION_SCOPE ExemptionScope;
				EXEMPTION_TYPE  ExemptionType;
				BOOLEAN         Valid = FALSE;
				ULONG           Flags;

				RtlZeroMemory(
						&ExemptionPath,
						sizeof(ExemptionPath));

				switch (GetStructureVersion(InBuffer))
				{
					case 0x0001:
						//
						// Make sure the size of the supplied buffer is valid
						//
						if ((GetStructureSize(InBuffer) < sizeof(WEHNTRUST_EXEMPTION_INFO_V1)) ||
						    (InBufferLength < sizeof(WEHNTRUST_EXEMPTION_INFO_V1)))
							Status = STATUS_INVALID_PARAMETER;
						else
						{
							PWEHNTRUST_EXEMPTION_INFO_V1 V1 = (PWEHNTRUST_EXEMPTION_INFO_V1)InBuffer;

							//
							// Naughty Naughty.  ExemptionPathSize is a USHORT so no
							// overflow check is needed.
							//
							if (V1->ExemptionPathSize + sizeof(WEHNTRUST_EXEMPTION_INFO_V1) > InBufferLength)
							{
								Status = STATUS_BUFFER_OVERFLOW;
								break;
							}

							//
							// If we were supplied with an exemption path...
							//
							if (V1->ExemptionPathSize > 0)
							{
								ExemptionPath.Buffer        = (PWSTR)V1->ExemptionPath;
								ExemptionPath.Length        = (USHORT)V1->ExemptionPathSize;
								ExemptionPath.MaximumLength = (USHORT)V1->ExemptionPathSize;
					
								RtleTruncateUnicodeTerminator(
										&ExemptionPath);
							}
							else
							{
								RtlInitUnicodeString(
										&ExemptionPath,
										L"");
							}

							ExemptionType  = V1->Type;
							ExemptionScope = V1->Scope;
							Flags          = V1->Flags;
							Valid          = TRUE;
						}
						break;
					default:
						DebugPrint(("DeviceIoControl(IOCTL_WEHNTRUST_XXX_EXEMPTION): Invalid structure version, %.4x.",
								GetStructureVersion(InBuffer)));

						Status = STATUS_INVALID_PARAMETER;
						break;
				}

				//
				// If everything is kosher, add, remove, or flush the exemption
				//
				if (Valid)
				{
					switch (Ioctl)
					{
						case IOCTL_WEHNTRUST_ADD_EXEMPTION:
							if (!NT_SUCCESS(Status = AddExemption(
									ExemptionType,
									ExemptionScope,
									Flags,
									&ExemptionPath,
									TRUE)))
							{
								DebugPrint(("DeviceIoControl(): AddExemption(%d, %d, %d, %wZ) failed, %.8x.",
										ExemptionType,
										ExemptionScope,
										Flags,
										&ExemptionPath,
										Status));
							}
							break;
						case IOCTL_WEHNTRUST_REMOVE_EXEMPTION:
							if (!NT_SUCCESS(Status = RemoveExemption(
									ExemptionType,
									ExemptionScope,
									Flags,
									&ExemptionPath,
									TRUE)))
							{
								DebugPrint(("DeviceIoControl(): RemoveExemption(%d, %d, %d, %wZ) failed, %.8x.",
										ExemptionType,
										ExemptionScope,
										Flags,
										&ExemptionPath,
										Status));
							}
							break;
						case IOCTL_WEHNTRUST_FLUSH_EXEMPTIONS:
							if (!NT_SUCCESS(Status = FlushExemptions(
									ExemptionType,
									ExemptionScope,
									TRUE)))
							{
								DebugPrint(("DeviceIoControl(): FlushExemption(%d, %d) failed, %.8x.",
										ExemptionType,
										ExemptionScope,
										Status));
							}
							break;
						default:
							break;
					}
				}
			}
			break;
		default:
			DebugPrint(("DeviceIoControl(): Unknown control code: %.8x.",
					Ioctl));
			Status = STATUS_INVALID_PARAMETER;
			break;
	}

	Irp->IoStatus.Status = Status;
		
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}
