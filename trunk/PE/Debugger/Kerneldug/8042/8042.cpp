//
// 8042 contoller driver for NGdbg debugger
//
// (C) Great, 2006-2008
//

#include <ntifs.h>

//
// IDT entry and IDTR reg
//

#pragma pack(push, 1)
struct IDTEntry
{
    USHORT OffsetLow;
    USHORT Selector;
    UCHAR ReservedByte;
    UCHAR Type : 3;
    UCHAR D : 1;
    UCHAR UnusedBits2 : 1;
    UCHAR DPL : 2;
    UCHAR Present : 1;
    USHORT OffsetHigh;
};
#pragma pack(pop)

#pragma pack(push, 2)
struct IDTR
{
    USHORT Limit;
    IDTEntry* Table;
};
#pragma pack(pop)


//
// Routines to work with IDT
//

IDTR Idtr;

PVOID
GetVector(
  IN UCHAR Interrupt
  )
/**
	Set IDT vector Interrupt to point to Handler
*/
{
    ULONG OldHandler;

	if (Idtr.Table == NULL)
		__asm sidt fword ptr [Idtr]

	OldHandler = Idtr.Table[Interrupt].OffsetLow  | ( Idtr.Table[Interrupt].OffsetHigh << 16 );

	if (Idtr.Table[Interrupt].Present)
		return (PVOID) OldHandler;
	else
		return NULL;
}

//
// Routines to work with APIC
//

ULONG GetAPICValue (PVOID pAPIC, ULONG xOffset)
{
	__asm
	{
		mov ecx, [pAPIC]
		mov eax, [xOffset]

		pushfd
		cli

		mov [ecx], eax
		mov eax, [ecx+10h]

		popfd
	}
}

ULONG GetIOAPICIntVector (ULONG Vector)
{
	PHYSICAL_ADDRESS PhysAPIC = {0xFEC00000, 0};
	PVOID pAPIC;
	ULONG IOAPICVector;

	pAPIC = MmMapIoSpace (PhysAPIC, 0x100, MmNonCached);
	IOAPICVector = GetAPICValue (pAPIC, (Vector*2 + 0x10)) & 0xFF;

	MmUnmapIoSpace (pAPIC, 0x100);
	return IOAPICVector;
}

#include "8042.h"

// Was previously defined in NTDDK,
// we don't need that definition
#undef DEVICE_TYPE

typedef enum _DEVICE_TYPE {
	ControllerDevice,
	KeyboardDevice,
	MouseDevice,
	UndefinedDevice
} DEVICE_TYPE, *PDEVICE_TYPE;

typedef enum _PORT_TYPE {
	DataPort = 0,
	CommandPort
} PORT_TYPE, *PPORT_TYPE;


/******************************************************************\
 *            Common routines to work with 8042 controller        *
\******************************************************************/

#define KBD_DBG_LEVEL 1

// Show only errors
#if KBD_DBG_LEVEL == 1
#define I8xPrint1(X) KdPrint(X)	// Error
#define I8xPrint2(X) // Warning
#define I8xPrint3(X) // Notice

// Show errors and warnings
#elif KBD_DBG_LEVEL == 2
#define I8xPrint1(X) KdPrint(X)	// Error
#define I8xPrint2(X) KdPrint(X) // Warning
#define I8xPrint3(X) // Notice

// Show all
#elif KBD_DBG_LEVEL == 3
#define I8xPrint1(X) KdPrint(X)	// Error
#define I8xPrint2(X) KdPrint(X) // Warning
#define I8xPrint3(X) KdPrint(X) // Notice

// Show nothing
#else
#define I8xPrint1(X)
#define I8xPrint2(X)
#define I8xPrint3(X)
#endif

VOID
I8xDrainOutputBuffer(
	)
/**
	Drain i8042 output buffer
*/
{
	UCHAR byte;

	//
	// Wail till the input buffer is processed by keboard
	// then go and read the data from keyboard.
	// Don't wait longer, than 1 second if case hardware is/
	// broken. This fix is necessary for some DEC hardware so
	// that the keyboard doesn't lock up.
	//

	for (ULONG i=0; i<2000; i++)
	{
		if (!(I8X_GET_STATUS_BYTE() & I8042_INPUT_BUFFER_FULL))
			break;
		KeStallExecutionProcessor (500);
	}

	while (I8X_GET_STATUS_BYTE() & I8042_OUTPUT_BUFFER_FULL)
	{
		//
		// Eat the output buffer byte
		//

		byte = I8X_GET_DATA_BYTE();
	}
}

VOID (*I8xMouseCallback)(UCHAR) = NULL;

VOID
I8xSetupMouseCallack(
	VOID (*MouseCallback)(UCHAR)
	)

/**
	Setup mouse callback
*/
{
	I8xMouseCallback = MouseCallback;
}


NTSTATUS
I8xGetBytePolled(
	IN CCHAR DeviceType,
	OUT PUCHAR Byte,
	IN BOOLEAN AllowMouseCallbacks
	)
/**
	Get byte from i8042 data port (keyboard or mouse)
	in polling mode.
*/
{
	UCHAR response;
	ULONG i;
	UCHAR mask = I8042_OUTPUT_BUFFER_FULL;
	UCHAR auxmask = I8042_AUXOUT_BUFFER_FULL;

	if (DeviceType == KeyboardDevice)
	{
		I8xPrint3(("I8x-" __FUNCTION__ ": enter (PS/2 keyboard)\n"));
	}
	else if (DeviceType == MouseDevice)
	{
		I8xPrint3(("I8x-" __FUNCTION__ ": enter (PS/2 mouse)\n"));
		
		// Swap main & aux devices
		auxmask = I8042_OUTPUT_BUFFER_FULL;
		mask = I8042_AUXOUT_BUFFER_FULL;
	}
	else 
	{
		I8xPrint3(("I8x-" __FUNCTION__ ": enter (8042 controller)\n"));
	}

	for (i=0;
		 (i < I8042_POLLING_ITERATIONS &&
		  ((response = I8X_GET_STATUS_BYTE() & BUFFER_FULL) != mask));
		 i++)
	{
		//
		// Output buffer of our device is not full.
		//
		
		if (response & auxmask)
		{
			//
			// There is something in the output buffer, but it
			// is not from the device we want to get byte from.
			// Eat the byte and try again
			//

			*Byte = I8X_GET_DATA_BYTE ();
			//I8xPrint2(("I8x-" __FUNCTION__ ": ate %X\n", *Byte));
			KdPrint(("I8x-" __FUNCTION__ ": ate %X\n", *Byte));

			if (DeviceType != MouseDevice)
			{
				//
				// Caller expects byte from 8042 controller or from 
				//  keyboard device, but this byte is from mouse device.
				//
				// Call mouse process routine if need
				//

				if (I8xMouseCallback && AllowMouseCallbacks)
				{
					//
					// Notice: Mouse callback routine can recursively call I8xGetBytePolled.
					//

					I8xMouseCallback (*Byte);
				}
			}
		}
		else
		{
			I8xPrint3(("I8x-" __FUNCTION__ ": stalling\n"));
			KeStallExecutionProcessor (I8042_STALL_MICROSECONDS);
		}
	}
	
	if (i >= I8042_POLLING_ITERATIONS)
	{
		I8xPrint2(("I8x-" __FUNCTION__ ": timed out\n"));
		return STATUS_IO_TIMEOUT;
	}

	// Grab the byte from the hardware
	*Byte = I8X_GET_DATA_BYTE();
	I8xPrint3(("I8x-" __FUNCTION__ ": exit with byte %X\n", *Byte));

	return STATUS_SUCCESS;
}

NTSTATUS
I8xPutBytePolled(
	IN CCHAR PortType,
	IN CCHAR DeviceType,
	IN BOOLEAN WaitForAck,
	IN CCHAR Byte
	)
/**
	Put byte into 8042 data or command port
	in polling mode and wait for ack if need.
*/
{
	NTSTATUS Status;
	UCHAR response;
	ULONG j;

	I8xPrint3(("I8X-" __FUNCTION__ ": enter, Port %X Dev %X Ack %X Byte %X\n",
		PortType,
		DeviceType,
		WaitForAck,
		Byte
		));

	if (DeviceType == MouseDevice)
	{
		//
		// Recursive call to tell controller
		// that we are going to work with
		// auxililary device (PS/2 mouse)
		//

		I8xPutBytePolled (
			CommandPort,
			UndefinedDevice,
			FALSE,
			(UCHAR) I8042_WRITE_TO_AUX_DEVICE
			);

	}

	for (j = 0; j < I8042_RESEND_ITERATIONS; j++)
	{
		ULONG i;

		//
		// Make sure that input-buffer-full-bit is clear
		//

		for (i=0; 
			 (i < I8042_POLLING_ITERATIONS 
			   && (I8X_GET_STATUS_BYTE() & I8042_INPUT_BUFFER_FULL)); 
			 i++)
		{
			I8xPrint3(("I8X-" __FUNCTION__ ": stalling\n"));
			KeStallExecutionProcessor (I8042_STALL_MICROSECONDS);
		}

		if (i >= I8042_POLLING_ITERATIONS)
		{
			I8xPrint1(("I8X-" __FUNCTION__ ": timed out\n"));
			Status = STATUS_IO_TIMEOUT;
			break;
		}

		// Drain i8042 output buffer
		I8xDrainOutputBuffer ();

		// Send the byte to appropriate register
		if (PortType == CommandPort)
		{
			I8X_WRITE_COMMAND_BYTE (Byte);
		}
		else
		{
			I8X_WRITE_DATA_BYTE (Byte);
		}

		// If we don't need to wait for ACK back from controller,
		// set the status and break out of the for loop.
		if (WaitForAck == FALSE)
		{
			Status = STATUS_SUCCESS;
			break;
		}

		//
		// Wait for ACK back from the controller.
		// If we get an ACK, the operation was successful
		// If we get a RESEND, break out to the for loop and
		//  try again.
		// Ignore anything other that ACK or RESEND
		//

		I8xPrint3(("I8x-" __FUNCTION__ ": waiting for ACK\n"));
		//I8xPrint1(("I8x-" __FUNCTION__ ": waiting for ACK\n"));

		BOOLEAN keepTrying = FALSE;

		while ((Status = I8xGetBytePolled(
							DeviceType,
							&response,
							FALSE)) == STATUS_SUCCESS)
		{
			if (response == I8042RS_ACKNOWLEDGE)
			{
				I8xPrint3(("I8x-" __FUNCTION__ ": got ACK\n"));
				//I8xPrint1(("I8x-" __FUNCTION__ ": got ACK\n"));
				break;
			}
			else if (response == I8042RS_RESEND)
			{
				I8xPrint2(("I8x-" __FUNCTION__ ": got RESEND\n"));
				//I8xPrint1(("I8x-" __FUNCTION__ ": got RESEND\n"));
								
				//
				// Indicate again if we are working with mouse
				//

				if (DeviceType == MouseDevice)
				{
					I8xPutBytePolled(
						CommandPort,
						UndefinedDevice,
						FALSE,
						(UCHAR) I8042_WRITE_TO_AUX_DEVICE
						);
				}

				keepTrying = TRUE;
				break;

			} // resend
		} // while

		if (!keepTrying)
			break;
	}

	//
	// Check to see if the number of allowed retries was exceeded.
	//

	if (j >= I8042_RESEND_ITERATIONS)
	{
		I8xPrint1(("I8x-" __FUNCTION__ ": exceeded number of retries\n"));
		Status = STATUS_IO_TIMEOUT;
	}

	I8xPrint3(("I8x-" __FUNCTION__": exit\n"));

	return Status;
}

VOID
I8xGetByteAsynchronous(
	IN CCHAR DeviceType,
	OUT PUCHAR Byte
	)
/**
	This routine reads a data byte from
	the controller of keyboard or mouse asynchronously.
*/
{
	UCHAR response;
	UCHAR mask = I8042_OUTPUT_BUFFER_FULL;
	ULONG i;

	if (DeviceType == KeyboardDevice)
	{
		I8xPrint3(("I8x-" __FUNCTION__ ": enter (PS/2 keyboard)\n"));
	}
	else if (DeviceType == MouseDevice)
	{
		I8xPrint3(("I8x-" __FUNCTION__ ": enter (PS/2 mouse)\n"));
		mask |= I8042_AUXOUT_BUFFER_FULL;
	}
	else 
	{
		I8xPrint3(("I8x-" __FUNCTION__ ": enter (8042 controller)\n"));
	}

	for (i=0;
		 i < I8042_POLLING_ITERATIONS && 
		  (response = I8X_GET_STATUS_BYTE() & mask) != mask;
		 i++)
	{
		if (response & I8042_OUTPUT_BUFFER_FULL)
		{
			//
			// There is something in the i8042 output buffer, but it
			// is not from the device we want to get a byte from.
			// Eat the byte and try again.
			//

			*Byte = I8X_GET_DATA_BYTE();
			I8xPrint2(("I8x-" __FUNCTION__ ": ate %X\n", *Byte));
		}
		else
		{
			// Try again

			I8xPrint3(("I8x-" __FUNCTION__ ": wait for correct status\n"));
		}
	}

	if (i >= I8042_POLLING_ITERATIONS)
	{
		I8xPrint1(("I8x-" __FUNCTION__ ": timed out\n"));
		ASSERT (FALSE);
		return;
	}

	// Grab byte from the hardware
	*Byte = I8X_GET_DATA_BYTE();
	I8xPrint3(("I8x-" __FUNCTION__ ": exit with byte %X\n", *Byte));
}

/******************************************************************\
 *            Common routines to work with keyboard               *
\******************************************************************/

// Keyboard interrupt vector in IDT
//  that corresponds to IRQ1
ULONG OldKbd;


#define PROFILE 0

VOID (*ProcessScanCode) (UCHAR);

BOOLEAN
KeyboardISR(
	)
/**
	Keyboard IRQ1 interrupt service routine.
	We should only MONITOR incoming scancodes, but not delete them.
*/
{
	UCHAR ScanCode;
	BOOLEAN Return = TRUE;

#if PROFILE
	LARGE_INTEGER TickCountEnter, TickCountExit;
#endif

	I8xPrint3(("Kbd-" __FUNCTION__ ": enter\n"));

#if PROFILE
	KeQueryTickCount (&TickCountEnter);
#endif

	//
	// Check that is really our interrupt.
	// Perform this check for i8042prt.sys, because
	// if we don't check this and read byte, i8042.prt
	// ISR will fail to perform this check.
	//

	if ((I8X_GET_STATUS_BYTE() 
		 & (I8042_OUTPUT_BUFFER_FULL|I8042_AUXOUT_BUFFER_FULL))
		 != I8042_OUTPUT_BUFFER_FULL)
	{
		ULONG i;

		I8xPrint3(("Kbd-" __FUNCTION__ ": aux buffer full\n"));

		//
		// Stall and then try ahain.
		//

		for (i=0; i < I8042_POLLSTATUS_ITERATIONS; i++)
		{
			KeStallExecutionProcessor (1);
			if ((I8X_GET_STATUS_BYTE() 
				 & (I8042_OUTPUT_BUFFER_FULL|I8042_AUXOUT_BUFFER_FULL))
				 == I8042_OUTPUT_BUFFER_FULL)
			{
				break;
			}
		}

		if ((I8X_GET_STATUS_BYTE() 
			 & (I8042_OUTPUT_BUFFER_FULL|I8042_AUXOUT_BUFFER_FULL))
			 != I8042_OUTPUT_BUFFER_FULL)
		{
			//
			// Not our interrupt
			//

			I8xPrint3(("Kbd-" __FUNCTION__ ": not our interrupt\n"));

			return FALSE;

//			Return = FALSE;
//			goto _request_resend_and_return;
		}
	}

	//
	// The interrupt is valid.
	// Read the byte from the i8042 data port.
	//

	I8xGetByteAsynchronous (KeyboardDevice, &ScanCode);

	if (ScanCode == I8042RS_RESEND ||
		ScanCode == I8042RS_ACKNOWLEDGE)
	{
		if (ScanCode == I8042RS_RESEND)
		{
			I8xPrint1(("Kbd-" __FUNCTION__": got RESEND\n"));
		}
		else
		{
			I8xPrint1(("Kbd-" __FUNCTION__": got ACK\n"));
		}
		
		Return = TRUE;

		goto _request_resend_and_return;
	}

	ProcessScanCode (ScanCode);

	Return = TRUE;

_request_resend_and_return:

#if PROFILE
	KeQueryTickCount (&TickCountExit);

	LARGE_INTEGER IsrExecutionTime;
	IsrExecutionTime.QuadPart = 
		((TickCountExit.QuadPart - TickCountEnter.QuadPart) * KeQueryTimeIncrement ()) / 10;

	KdPrint(("ISR took %d microseconds to execute\n", IsrExecutionTime.QuadPart));
#endif

	//
	// Write scan-code back to i8042 controller.
	// Controller will write it to output port
	//
	
	// Temporarily disable keyboard & mouse
	I8xPutBytePolled (CommandPort,
		ControllerDevice,
		FALSE,
		(UCHAR) I8042_DISABLE_KEYBOARD
		);
	I8xPutBytePolled (CommandPort,
		ControllerDevice,
		FALSE,
		(UCHAR) I8042_DISABLE_MOUSE
		);

	I8xPutBytePolled (CommandPort, 
		ControllerDevice, 
		FALSE, 
		(UCHAR)I8042_WRITE_OUTPUT_REGISTER);

	I8xPutBytePolled (DataPort, 
		ControllerDevice, 
		FALSE, 
		ScanCode);

	// Enable keyboard & mouse
	I8xPutBytePolled (CommandPort,
		ControllerDevice,
		FALSE,
		(UCHAR) I8042_ENABLE_KEYBOARD
		);
	I8xPutBytePolled (CommandPort,
		ControllerDevice,
		FALSE,
		(UCHAR) I8042_ENABLE_MOUSE
		);

	return Return;
}

BOOLEAN (*OldISR)(PKINTERRUPT,PVOID);

BOOLEAN
  InterruptService(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext
    )
{
	if (!KeyboardISR())
		return FALSE;
	return OldISR(Interrupt, ServiceContext);
}

PVOID 
IoHookInterrupt (
	ULONG Vector, 
	PVOID NewRoutine)
{
	PKINTERRUPT Interrupt;
	PVOID Handler;

	Handler = GetVector ((UCHAR)Vector);
	if (Handler == NULL)
		return NULL;

	Interrupt = CONTAINING_RECORD (Handler, _KINTERRUPT, DispatchCode);
	if (Interrupt->Type != 22)
		return NULL;

	Handler = Interrupt->ServiceRoutine;
	*(PVOID*)&Interrupt->ServiceRoutine = NewRoutine;
	return Handler;
}

VOID
I8042HookKeyboardIsr(
	VOID (*pProcessScanCode)(UCHAR)
	)
{
	ProcessScanCode = pProcessScanCode;
	OldKbd = GetIOAPICIntVector (1);
	*(PVOID*)&OldISR = IoHookInterrupt ( (UCHAR)OldKbd, InterruptService);
}


#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntdd8042.h>

NTSTATUS
I8042HookKeyboard(
	PI8042_KEYBOARD_ISR IsrRoutine
	)
	
/*++

Routine Description

	Hook keyboard by calling i8042prt with ioctl code IOCTL_INTERNAL_I8042_HOOK_KEYBOARD

Arguments

	IsrRoutine

		New hook routine

Return value

	NTSTATUS of the operation

--*/

{
	UNICODE_STRING DeviceName;
	PDEVICE_OBJECT DeviceObject;
	PFILE_OBJECT FileObject;
	NTSTATUS Status;

	RtlInitUnicodeString (&DeviceName, L"\\Device\\KeyboardClass0");

	Status = IoGetDeviceObjectPointer (&DeviceName,
		FILE_READ_ATTRIBUTES,
		&FileObject,
		&DeviceObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("IoGetDeviceObjectPointer failed st %X\n", Status));
		return Status;
	}

	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	KEVENT Event;

	KeInitializeEvent (&Event, SynchronizationEvent, FALSE);

	INTERNAL_I8042_HOOK_KEYBOARD hookkbd = {0};
	hookkbd.IsrRoutine = IsrRoutine;
	
	Irp = IoBuildDeviceIoControlRequest (
		IOCTL_INTERNAL_I8042_HOOK_KEYBOARD,
		DeviceObject,
		&hookkbd,
		sizeof(hookkbd),
		NULL,
		0,
		TRUE,
		&Event,
		&IoStatus );

	Status = IoCallDriver (DeviceObject, Irp);

	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);
	}

	if (NT_SUCCESS(Status))
		Status = IoStatus.Status;

	ObDereferenceObject (FileObject);

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("IOCTL_INTERNAL_HOOK_KEYBOARD failed with status %X\n", Status));
		return Status;
	}

	KdPrint(("IOCTL_INTERNAL_HOOK_KEYBOARD ok\n"));
	return STATUS_SUCCESS;
}

// Driver entry point
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("[~] DriverEntry()\n"));

	return STATUS_UNSUCCESSFUL;

}
