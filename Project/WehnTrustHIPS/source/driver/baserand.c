/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

VOID DriverUnload(PDRIVER_OBJECT Driver);

//
// Global pointer to the system process object
//
PPROCESS_OBJECT SystemProcess = NULL;

#pragma code_seg("INIT")

//
// The driver's entry point
//
NTSTATUS DriverEntry(PDRIVER_OBJECT Driver, PUNICODE_STRING Path)
{
	PIMAGE_SET ImageSet = NULL;
	NTSTATUS   Status = STATUS_SUCCESS;

	//
	// Save the pointer to the system process for future reference 
	// in the exclusion of drivers from being randomized
	//
	SystemProcess = IoGetCurrentProcess();

	do
	{
#if DBG
		//
		// Set the unload routine for the driver for testing purposes.  An unload
		// routine is only enabled for debug builds.  Production builds cannot
		// have their image unloaded as it is dangerous.
		//
		Driver->DriverUnload = DriverUnload;
#endif

		//
		// First thing's first, check to see if the driver is being started in
		// safe mode.  If it is, bail out.
		//
		if (*InitSafeBootMode > 0)
		{
			DebugPrint(("DriverEntry(): Disabling the driver due to safe mode boot."));
			
			Status = STATUS_NOT_SAFE_MODE_DRIVER;
			break;
		}

		//
		// Initialize the random number generator
		//
		RngInitialize();

		//
		// Initialize the driver's executive subsystem which is responsible for
		// managing exemptions, start/stop, and other such administrative tasks.
		//
		InitializeExecutive();

		//
		// Initialize the underlying image sets
		//
		if (!NT_SUCCESS(Status = ImageSetsInitialize()))
		{
			DebugPrint(("DriverEntry(): ERROR: ImageSetsInitialize failed, %.8x.",
					Status));
			break;
		}

		//
		// Initialize image and application exemptions
		//
		if (!NT_SUCCESS(Status = InitializeExemptions()))
		{
			DebugPrint(("DriverEntry(): WARNING: InitializeExemptions failed, %.8x.",
					Status));
		}

		//
		// Initialize the compatibility subsystem.  This is generic interface is
		// used to interact with undocumented structures and functions that differ
		// between various versions of Windows.
		//
		if (!NT_SUCCESS(Status = CompatInitialize()))
		{
			DebugPrint(("DriverEntry(): ERROR: CompatInitialize failed, %.8x.",
					Status));
			break;
		}

		//
		// Try to register the device object.  Do not fail execution if we are
		// unable to, though.
		//
		if (!NT_SUCCESS(Status = DeviceRegister(
				Driver)))
		{
			DebugPrint(("DriverEntry(): WARNING: Failed to register device object, %.8x.",
					Status));
		}

#ifdef MULTIPLE_IMAGE_SETS
		//
		// Initialize & Refresh the list of images that are specified in the 
		// registry, and thus by virtue validating their checksums and building 
		// their jump tables accordingly
		//
		if (!NT_SUCCESS(Status = InitializeImageTable()))
		{
			DebugPrint(("DriverEntry(): ERROR: InitializeImageTable failed, %.8x.", 
					Status));
			break;
		}
#endif

		//
		// Start the image set mapping expiration monitor
		//
		if (!NT_SUCCESS(Status = StartMappingExpirationMonitor()))
		{
			DebugPrint(("DriverEntry(): WARNING: StartMappingExpirationMonitor failed, %.8x.", 
					Status));
		}

		//
		// If stack/heap randomization is enabled, initialize the process
		// subsystem
		//
		if (!NT_SUCCESS(Status = InitializeProcess()))
		{
			DebugPrint(("DriverEntry(): ERROR: Failed to initialize the process subsystem, %.8x.",
					Status));
			break;
		}

#ifndef DISABLE_PEBTEB_RANDOMIZATION
		//
		// Randomize the highest user address such that the PEB/TEB are
		// 'randomized'.
		//
		if (!NT_SUCCESS(Status = RandomizeHighestUserAddress()))
		{
			DebugPrint(("DriverEntry(): WARNING: RandomizeHighestUserAddress failed, %.8x.",
					Status));
		}
#endif

		//
		// Install the system service hooks
		//
		if (!NT_SUCCESS(Status = InstallHooks()))
		{
			DebugPrint(("DriverEntry(): ERROR: InstallHooks failed, %.8x.", Status));
			break;
		}

	} while (0);

	//
	// If we failed to initialize, disable all randomization subsystems
	//
	if (!NT_SUCCESS(Status))
		EnableRandomizationSubsystems(
				0,
				FALSE);

	return Status;
}

#pragma code_seg()

#if DBG
//
// This is mainly here for debugging purposes.  It is not enabled in the
// production driver.
//
// Called when the driver is being unloaded
//
VOID DriverUnload(PDRIVER_OBJECT Driver)
{
	NTSTATUS Status;

	do
	{
		//
		// Uninstall hooks
		//
		if (!NT_SUCCESS(Status = UninstallHooks()))
		{
			DebugPrint(("DriverUnload(): UninstallHooks failed, %.8x", 
					Status));
		}

		//
		// Stop the mapping expiration monitor
		//
		if (!NT_SUCCESS(Status = StopMappingExpirationMonitor()))
		{
			DebugPrint(("DriverUnload(): StopMappingExpirationMonitor failed, %.8x.",
					Status));
		}

		//
		// Deinitialize the process subsystem
		//
		if (!NT_SUCCESS(Status = DeinitializeProcess()))
		{
			DebugPrint(("DriverUnload(): DeinitializeProcess failed, %.8x.",
					Status));
		}

	} while (0);
}
#endif
