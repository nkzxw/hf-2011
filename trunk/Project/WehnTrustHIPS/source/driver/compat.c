/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"
#include "compat.h"

//
// Common prototypes
//

VOID (NTAPI * KiCheckForKernelApcDelivery)(VOID) = NULL;

// 
// Windows 2000 prototypes
//
VOID AcquireProcessRegionLockW2k(
		IN PPROCESS_OBJECT_W2K ProcessObject);
VOID ReleaseProcessRegionLockW2k(
		IN PPROCESS_OBJECT_W2K ProcessObject);
PMMVAD GetProcessVadRootW2k(
		IN PPROCESS_OBJECT_W2K ProcessObject);

ULONG GetSectionObjectControlAreaFlagsW2k(
		IN PSECTION_OBJECT_W2K SectionObject);
PFILE_OBJECT GetSectionObjectControlAreaFilePointerW2k(
		IN PSECTION_OBJECT_W2K SectionObject);
PVOID GetSectionObjectSegmentBasedAddressW2k(
		IN PSECTION_OBJECT_W2K SectionObject);
VOID SetSectionObjectSegmentBasedAddressW2k(
		IN PSECTION_OBJECT_W2K SectionObject,
		IN PVOID BasedAddress);
ULONG GetSectionObjectControlAreaNumberOfMappedViewsW2k(
		IN PSECTION_OBJECT_W2K SectionObject);
ULONG GetSectionObjectControlAreaNumberOfSectionReferencesW2k(
		IN PSECTION_OBJECT_W2K SectionObject);
PLARGE_INTEGER GetSectionObjectSegmentSizeW2k(
		IN PSECTION_OBJECT_W2K SectionObject);
PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformationW2k(
		IN PSECTION_OBJECT_W2K SectionObject);

//
// Windows XP prototypes
//
VOID AcquireProcessRegionLockWxp(
		IN PPROCESS_OBJECT_WXP ProcessObject);
VOID ReleaseProcessRegionLockWxp(
		IN PPROCESS_OBJECT_WXP ProcessObject);
PMMVAD GetProcessVadRootWxp(
		IN PPROCESS_OBJECT_WXP ProcessObject);

PVOID GetVadStartingVpnWxp(
		IN PMMVAD_WXP Vad);
PVOID GetVadEndingVpnWxp(
		IN PMMVAD_WXP Vad);
PMMVAD GetVadLeftChildWxp(
		IN PMMVAD_WXP Vad);
PMMVAD GetVadRightChildWxp(
		IN PMMVAD_WXP Vad);

ULONG GetSectionObjectControlAreaFlagsWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
PFILE_OBJECT GetSectionObjectControlAreaFilePointerWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
PVOID GetSectionObjectSegmentBasedAddressWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
VOID SetSectionObjectSegmentBasedAddressWxp(
		IN PSECTION_OBJECT_WXP SectionObject,
		IN PVOID BasedAddress);
PVOID GetSectionObjectSegmentBasedAddressWxpPae(
		IN PSECTION_OBJECT_WXP SectionObject);
VOID SetSectionObjectSegmentBasedAddressWxpPae(
		IN PSECTION_OBJECT_WXP SectionObject,
		IN PVOID BasedAddress);
ULONG GetSectionObjectControlAreaNumberOfMappedViewsWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
ULONG GetSectionObjectControlAreaNumberOfSectionReferencesWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
PLARGE_INTEGER GetSectionObjectSegmentSizeWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformationWxp(
		IN PSECTION_OBJECT_WXP SectionObject);
PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformationWxpPae(
		IN PSECTION_OBJECT_WXP SectionObject);

//
// Windows 2003 Server prototypes
//
VOID AcquireProcessRegionLock2k3(
		IN PPROCESS_OBJECT_2K3 ProcessObject);
VOID ReleaseProcessRegionLock2k3(
		IN PPROCESS_OBJECT_2K3 ProcessObject);
PMMVAD GetProcessVadRoot2k3(
		IN PPROCESS_OBJECT_2K3 ProcessObject);

PVOID GetVadStartingVpn2k3(
		IN PMMADDRESS_NODE_2K3 Vad);
PVOID GetVadEndingVpn2k3(
		IN PMMADDRESS_NODE_2K3 Vad);
PMMVAD GetVadLeftChild2k3(
		IN PMMADDRESS_NODE_2K3 Vad);
PMMVAD GetVadRightChild2k3(
		IN PMMADDRESS_NODE_2K3 Vad);

ULONG GetSectionObjectControlAreaFlags2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);
PFILE_OBJECT GetSectionObjectControlAreaFilePointer2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);
PVOID GetSectionObjectSegmentBasedAddress2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);
VOID SetSectionObjectSegmentBasedAddress2k3(
		IN PSECTION_OBJECT_2K3 SectionObject,
		IN PVOID BasedAddress);
ULONG GetSectionObjectControlAreaNumberOfMappedViews2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);
ULONG GetSectionObjectControlAreaNumberOfSectionReferences2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);
PLARGE_INTEGER GetSectionObjectSegmentSize2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);
PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformation2k3(
		IN PSECTION_OBJECT_2K3 SectionObject);

////
//
// Exported function pointers for compatibility routines
//
////
VOID (*AcquireProcessRegionLock)(
		IN PPROCESS_OBJECT ProcessObject) = NULL;
VOID (*ReleaseProcessRegionLock)(
		IN PPROCESS_OBJECT ProcessObject) = NULL;
PMMVAD (*GetProcessVadRoot)(
		IN PPROCESS_OBJECT ProcessObject) = NULL;

PVOID (*GetVadStartingVpn)(
		IN PMMVAD Vad) = NULL;
PVOID (*GetVadEndingVpn)(
		IN PMMVAD Vad) = NULL;
PMMVAD (*GetVadLeftChild)(
		IN PMMVAD Vad) = NULL;
PMMVAD (*GetVadRightChild)(
		IN PMMVAD Vad) = NULL;

ULONG (*GetSectionObjectControlAreaFlags)(
		IN PSECTION_OBJECT SectionObject) = NULL;
PFILE_OBJECT (*GetSectionObjectControlAreaFilePointer)(
		IN PSECTION_OBJECT SectionObject) = NULL;
PVOID (*GetSectionObjectSegmentBasedAddress)(
		IN PSECTION_OBJECT SectionObject) = NULL;
VOID (*SetSectionObjectSegmentBasedAddress)(
		IN PSECTION_OBJECT SectionObject,
		IN PVOID BasedAddress) = NULL;
ULONG (*GetSectionObjectControlAreaNumberOfMappedViews)(
		IN PSECTION_OBJECT SectionObject) = NULL;
ULONG (*GetSectionObjectControlAreaNumberOfSectionReferences)(
		IN PSECTION_OBJECT SectionObject) = NULL;
PLARGE_INTEGER (*GetSectionObjectSegmentSize)(
		IN PSECTION_OBJECT SectionObject) = NULL;
PSECTION_IMAGE_INFORMATION (*GetSectionObjectSegmentImageInformation)(
		IN PSECTION_OBJECT SectionObject) = NULL;

#define DefineAcquireProcessRegionLock(x) \
	AcquireProcessRegionLock = (VOID (*)(PPROCESS_OBJECT))x
#define DefineReleaseProcessRegionLock(x) \
	ReleaseProcessRegionLock = (VOID (*)(PPROCESS_OBJECT))x
#define DefineGetProcessVadRoot(x) \
	GetProcessVadRoot = (PMMVAD (*)(PPROCESS_OBJECT))x
#define DefineGetVadStartingVpn(x) \
	GetVadStartingVpn = (PVOID (*)(PMMVAD))x
#define DefineGetVadEndingVpn(x) \
	GetVadEndingVpn = (PMMVAD (*)(PMMVAD))x
#define DefineGetVadLeftChild(x) \
	GetVadLeftChild = (PMMVAD (*)(PMMVAD))x
#define DefineGetVadRightChild(x) \
	GetVadRightChild = (PMMVAD (*)(PMMVAD))x
#define DefineGetSectionObjectControlAreaFlags(x) \
	GetSectionObjectControlAreaFlags = (ULONG (*)(PSECTION_OBJECT))x
#define DefineGetSectionObjectControlAreaFilePointer(x) \
	GetSectionObjectControlAreaFilePointer = (PFILE_OBJECT (*)(PSECTION_OBJECT))x
#define DefineGetSectionObjectSegmentBasedAddress(x) \
	GetSectionObjectSegmentBasedAddress = (PVOID (*)(PSECTION_OBJECT))x
#define DefineSetSectionObjectSegmentBasedAddress(x) \
	SetSectionObjectSegmentBasedAddress = (VOID (*)(PSECTION_OBJECT, PVOID))x
#define DefineGetSectionObjectControlAreaNumberOfMappedViews(x) \
	GetSectionObjectControlAreaNumberOfMappedViews = (ULONG (*)(PSECTION_OBJECT))x
#define DefineGetSectionObjectControlAreaNumberOfSectionReferences(x) \
	GetSectionObjectControlAreaNumberOfSectionReferences = (ULONG (*)(PSECTION_OBJECT))x
#define DefineGetSectionObjectSegmentSize(x) \
	GetSectionObjectSegmentSize = (PLARGE_INTEGER (*)(PSECTION_OBJECT))x
#define DefineGetSectionObjectSegmentImageInformation(x) \
	GetSectionObjectSegmentImageInformation = (PSECTION_IMAGE_INFORMATION (*)(PSECTION_OBJECT))x

//
// Other global data
//

ULONG COMPAT_OBJ_KERNEL_HANDLE = OBJ_KERNEL_HANDLE;

//
// Whether or not we've switched to PAE
//
static BOOLEAN SwitchedToPae = FALSE;

#pragma code_seg("INIT")

//
// Initializes the compatibility functions for the operating system being
// executed on
//
NTSTATUS CompatInitialize()
{
	UNICODE_STRING RoutineName;
	UNICODE_STRING ServicePack;
	NTSTATUS       Status = STATUS_SUCCESS;
	WCHAR          ServicePackBuffer[256];
	ULONG          Major = 0, Minor = 0, BuildNumber = 0;
	BOOLEAN        PromoteToXp = FALSE;

	//
	// Initialize the buffer that will hold service pack information
	//
	ServicePack.Buffer        = ServicePackBuffer;
	ServicePack.Length        = sizeof(ServicePackBuffer);
	ServicePack.MaximumLength = sizeof(ServicePackBuffer);

	RtlZeroMemory(
			ServicePackBuffer,
			sizeof(ServicePackBuffer));

	//
	// Get the machine's operating system major and minor version numbers
	//
	PsGetVersion(
			&Major,
			&Minor,
			NULL,
			&ServicePack);

	RtlInitUnicodeString(&RoutineName, L"KiCheckForKernelApcDelivery");
	(PVOID)KiCheckForKernelApcDelivery = MmGetSystemRoutineAddress(&RoutineName);

	switch (Major)
	{
		case 5:
			//
			// Windows 2000 Specific
			//
			if (Minor == 0)
			{
				if ((RtleFindStringInUnicodeString(
						&ServicePack,
						L"Service Pack 4")) ||
				    (RtleFindStringInUnicodeString(
						&ServicePack,
						L"Service Pack 3")) ||
				    (RtleFindStringInUnicodeString(
						&ServicePack,
						L"Service Pack 2")))
				{
					DebugPrint(("CompatInitialize(): Windows 2000 SP2 or higher found."));

					PromoteToXp = TRUE;
				}
				else
				{
					DefineAcquireProcessRegionLock(
							AcquireProcessRegionLockW2k);
					DefineReleaseProcessRegionLock(
							ReleaseProcessRegionLockW2k);
					DefineGetProcessVadRoot(
							GetProcessVadRootW2k);
					DefineGetSectionObjectControlAreaFlags(
							GetSectionObjectControlAreaFlagsW2k);
					DefineGetSectionObjectControlAreaFilePointer(
							GetSectionObjectControlAreaFilePointerW2k);
					DefineGetSectionObjectSegmentBasedAddress(
							GetSectionObjectSegmentBasedAddressW2k);
					DefineSetSectionObjectSegmentBasedAddress(
							SetSectionObjectSegmentBasedAddressW2k);
					DefineGetSectionObjectControlAreaNumberOfMappedViews(
							GetSectionObjectControlAreaNumberOfMappedViewsW2k);
					DefineGetSectionObjectControlAreaNumberOfSectionReferences(
							GetSectionObjectControlAreaNumberOfSectionReferencesW2k);
					DefineGetSectionObjectSegmentSize(
							GetSectionObjectSegmentSizeW2k);
					DefineGetSectionObjectSegmentImageInformation(
							GetSectionObjectSegmentImageInformationW2k);
				}
			}

			//
			// Windows XP Specific
			//
			if ((PromoteToXp) ||
			    (Minor == 1))
			{
				DefineAcquireProcessRegionLock(
						AcquireProcessRegionLockWxp);
				DefineReleaseProcessRegionLock(
						ReleaseProcessRegionLockWxp);
				DefineGetProcessVadRoot(
						GetProcessVadRootWxp);
				DefineGetSectionObjectControlAreaFlags(
						GetSectionObjectControlAreaFlagsWxp);
				DefineGetSectionObjectControlAreaFilePointer(
						GetSectionObjectControlAreaFilePointerWxp);
				DefineGetSectionObjectSegmentBasedAddress(
						GetSectionObjectSegmentBasedAddressWxp);
				DefineSetSectionObjectSegmentBasedAddress(
						SetSectionObjectSegmentBasedAddressWxp);
				DefineGetSectionObjectControlAreaNumberOfMappedViews(
						GetSectionObjectControlAreaNumberOfMappedViewsWxp);
				DefineGetSectionObjectControlAreaNumberOfSectionReferences(
						GetSectionObjectControlAreaNumberOfSectionReferencesWxp);
				DefineGetSectionObjectSegmentSize(
						GetSectionObjectSegmentSizeWxp);
				DefineGetSectionObjectSegmentImageInformation(
						GetSectionObjectSegmentImageInformationWxp);
			}

			//
			// Windows 2000 & XP Shared
			//
			if ((Minor == 0) || (Minor == 1))
			{
				DefineGetVadStartingVpn(
						GetVadStartingVpnWxp);
				DefineGetVadEndingVpn(
						GetVadEndingVpnWxp);
				DefineGetVadLeftChild(
						GetVadLeftChildWxp);
				DefineGetVadRightChild(
						GetVadRightChildWxp);


				DebugPrint(("CompatInitialize(): Initialized as Windows %s (\"%wZ\", %d.%d).",
						(Minor == 0) ? "2000" : "XP",
						&ServicePack, Major, Minor));
			}
			//
			// Windows 2003 Server
			//
			else if (Minor == 2)
			{
				DefineAcquireProcessRegionLock(
						AcquireProcessRegionLock2k3);
				DefineReleaseProcessRegionLock(
						ReleaseProcessRegionLock2k3);
				DefineGetProcessVadRoot(
						GetProcessVadRoot2k3);
				DefineGetVadStartingVpn(
						GetVadStartingVpn2k3);
				DefineGetVadEndingVpn(
						GetVadEndingVpn2k3);
				DefineGetVadLeftChild(
						GetVadLeftChild2k3);
				DefineGetVadRightChild(
						GetVadRightChild2k3);
				DefineGetSectionObjectControlAreaFlags(
						GetSectionObjectControlAreaFlags2k3);
				DefineGetSectionObjectControlAreaFilePointer(
						GetSectionObjectControlAreaFilePointer2k3);
				DefineGetSectionObjectSegmentBasedAddress(
						GetSectionObjectSegmentBasedAddress2k3);
				DefineSetSectionObjectSegmentBasedAddress(
						SetSectionObjectSegmentBasedAddress2k3);
				DefineGetSectionObjectControlAreaNumberOfMappedViews(
						GetSectionObjectControlAreaNumberOfMappedViews2k3);
				DefineGetSectionObjectControlAreaNumberOfSectionReferences(
						GetSectionObjectControlAreaNumberOfSectionReferences2k3);
				DefineGetSectionObjectSegmentSize(
						GetSectionObjectSegmentSize2k3);
				DefineGetSectionObjectSegmentImageInformation(
						GetSectionObjectSegmentImageInformation2k3);

				DebugPrint(("CompatInitialize(): Initialized as Windows 2003 Server."));
			}
			break;

		case 4:
			//
			// Windows NT 4.0 specific
			//

			COMPAT_OBJ_KERNEL_HANDLE = 0;

			//
			// XXX
			// Fallthrough intended.  Lots of work to do here if we really support NT4 eventually.
			//

		default:
			DebugPrint(("CompatInitialize(): Unsupported operating system: Major %lu Minor %lu.",
					Major, 
					Minor));
			Status = STATUS_NOT_SUPPORTED;
			break;
	}

	return Status;
}

#pragma code_seg()

//
// This routine checks to see if we should switch to the PAE release of Windows
// XP based on the location of the BasedAddress attribute in the SEGMENT
// structure.  This is a bit of a hack, but that's what happens when you're
// dealing with internal data structures that change between different versions
// of Windows...
//
VOID CheckSwitchToPae(
		IN PSECTION_OBJECT_WXP SectionObject,
		IN PVOID ExpectedBaseAddress)
{
	//
	// Have we tried to switch to PAE yet?
	//
	if (!SwitchedToPae)
	{
		PSEGMENT_WXP_XPSP2_PAE Pae = (PSEGMENT_WXP_XPSP2_PAE)SectionObject->Segment;
		PSEGMENT_WXP           Nor = (PSEGMENT_WXP)SectionObject->Segment;

		//
		// If the PAE BasedAddress is equal to the supplied base address, then we
		// should switch to PAE.
		//
		if (Pae->BasedAddress == ExpectedBaseAddress)
		{
			DebugPrint(("CheckSwitchToPae(): Switching to XPSP2 PAE."));

			DefineGetSectionObjectSegmentBasedAddress(
					GetSectionObjectSegmentBasedAddressWxpPae);
			DefineSetSectionObjectSegmentBasedAddress(
					SetSectionObjectSegmentBasedAddressWxpPae);
				DefineGetSectionObjectSegmentImageInformation(
						GetSectionObjectSegmentImageInformationWxpPae);

			SwitchedToPae = TRUE;
		}
	}
}

////
// 
// Windows 2000
//
////

VOID AcquireProcessRegionLockW2k(
		IN PPROCESS_OBJECT_W2K ProcessObject)
{
	ExAcquireFastMutex(
			&ProcessObject->AddressCreationLock);
	ExAcquireFastMutex(
			&ProcessObject->WorkingSetLock);
}

VOID ReleaseProcessRegionLockW2k(
		IN PPROCESS_OBJECT_W2K ProcessObject)
{
	ExReleaseFastMutex(
			&ProcessObject->WorkingSetLock);
	ExReleaseFastMutex(
			&ProcessObject->AddressCreationLock);
}

PMMVAD GetProcessVadRootW2k(
		IN PPROCESS_OBJECT_W2K ProcessObject)
{
	return ProcessObject->VadRoot;
}

ULONG GetSectionObjectControlAreaFlagsW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return SectionObject->Segment->ControlArea->Flags;
}

PFILE_OBJECT GetSectionObjectControlAreaFilePointerW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return SectionObject->Segment->ControlArea->FilePointer;
}

PVOID GetSectionObjectSegmentBasedAddressW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return SectionObject->Segment->BasedAddress;
}

VOID SetSectionObjectSegmentBasedAddressW2k(
		IN PSECTION_OBJECT_W2K SectionObject,
		IN PVOID BasedAddress)
{
	SectionObject->Segment->BasedAddress = BasedAddress;
}

ULONG GetSectionObjectControlAreaNumberOfMappedViewsW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return SectionObject->Segment->ControlArea->NumberOfMappedViews;
}

ULONG GetSectionObjectControlAreaNumberOfSectionReferencesW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return SectionObject->Segment->ControlArea->NumberOfSectionReferences;
}

PLARGE_INTEGER GetSectionObjectSegmentSizeW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return &SectionObject->Segment->SizeOfSegment;
}

PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformationW2k(
		IN PSECTION_OBJECT_W2K SectionObject)
{
	return SectionObject->Segment->u2.ImageInformation;
}

////
//
// Windows XP
//
////

VOID AcquireProcessRegionLockWxp(
		IN PPROCESS_OBJECT_WXP ProcessObject)
{
	ExAcquireFastMutex(
			&ProcessObject->AddressCreationLock);
	ExAcquireFastMutex(
			&ProcessObject->WorkingSetLock);
}

VOID ReleaseProcessRegionLockWxp(
		IN PPROCESS_OBJECT_WXP ProcessObject)
{
	ExReleaseFastMutex(
			&ProcessObject->WorkingSetLock);
	ExReleaseFastMutex(
			&ProcessObject->AddressCreationLock);
}

PMMVAD GetProcessVadRootWxp(
		IN PPROCESS_OBJECT_WXP ProcessObject)
{
	return ProcessObject->VadRoot;
}

PVOID GetVadStartingVpnWxp(
		IN PMMVAD_WXP Vad)
{
	return Vad->StartingVpn;
}

PVOID GetVadEndingVpnWxp(
		IN PMMVAD_WXP Vad)
{
	return Vad->EndingVpn;
}

PMMVAD GetVadLeftChildWxp(
		IN PMMVAD_WXP Vad)
{
	return Vad->LeftChild;
}

PMMVAD GetVadRightChildWxp(
		IN PMMVAD_WXP Vad)
{
	return Vad->RightChild;
}

ULONG GetSectionObjectControlAreaFlagsWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return SectionObject->Segment->ControlArea->Flags;
}

PFILE_OBJECT GetSectionObjectControlAreaFilePointerWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return SectionObject->Segment->ControlArea->FilePointer;
}

PVOID GetSectionObjectSegmentBasedAddressWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return SectionObject->Segment->BasedAddress;
}

VOID SetSectionObjectSegmentBasedAddressWxp(
		IN PSECTION_OBJECT_WXP SectionObject,
		IN PVOID BasedAddress)
{
	SectionObject->Segment->BasedAddress = BasedAddress;
}

PVOID GetSectionObjectSegmentBasedAddressWxpPae(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return ((PSEGMENT_WXP_XPSP2_PAE)SectionObject->Segment)->BasedAddress;
}

VOID SetSectionObjectSegmentBasedAddressWxpPae(
		IN PSECTION_OBJECT_WXP SectionObject,
		IN PVOID BasedAddress)
{
	((PSEGMENT_WXP_XPSP2_PAE)SectionObject->Segment)->BasedAddress = BasedAddress;
}

ULONG GetSectionObjectControlAreaNumberOfMappedViewsWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return SectionObject->Segment->ControlArea->NumberOfMappedViews;
}

ULONG GetSectionObjectControlAreaNumberOfSectionReferencesWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return SectionObject->Segment->ControlArea->NumberOfSectionReferences;
}

PLARGE_INTEGER GetSectionObjectSegmentSizeWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return &SectionObject->Segment->SizeOfSegment;
}

PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformationWxp(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return SectionObject->Segment->u2.ImageInformation;
}

PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformationWxpPae(
		IN PSECTION_OBJECT_WXP SectionObject)
{
	return ((PSEGMENT_WXP_XPSP2_PAE)SectionObject->Segment)->u2.ImageInformation;
}

////
//
// Windows 2003 Server
//
////

VOID AcquireGuardedMutex(
		PKGUARDED_MUTEX_2K3 GuardedMutex)
{
	PKTHREAD_2K3 CurrentThread;
	
	CurrentThread = (PKTHREAD_2K3)KeGetCurrentThread();

	CurrentThread->SpecialApcDisable--;

	if (InterlockedExchangeAdd(
			&GuardedMutex->Count,
			-1) != 1)
	{
		DebugPrint(("AcquireGuardedMutex(%p): Waiting for event...",
				GuardedMutex));

		GuardedMutex->Contention++;

		KeWaitForSingleObject(
				&GuardedMutex->Event,
				WR_MUTEX, // WrMutex (new in 2003)
				KernelMode,
				FALSE,
				NULL);
	}
		
	GuardedMutex->Owner = (PKTHREAD)CurrentThread;
}

VOID ReleaseGuardedMutex(
		PKGUARDED_MUTEX_2K3 GuardedMutex)
{
	PKTHREAD_2K3 CurrentThread;

	CurrentThread = (PKTHREAD_2K3)KeGetCurrentThread();

	GuardedMutex->Owner = NULL;

	if (InterlockedExchangeAdd(
			&GuardedMutex->Count,
			1) < 0)
	{
		DebugPrint(("ReleaseGuardedMutex(%p): Setting event.",
				GuardedMutex));

		KeSetEventBoostPriority(
				&GuardedMutex->Event,
				0);
	}

	CurrentThread->SpecialApcDisable++;

	if(!CurrentThread->SpecialApcDisable && CurrentThread->ApcState.ApcListHead[KernelMode].Flink != &CurrentThread->ApcState.ApcListHead[KernelMode])
	{
		ASSERT(KiCheckForKernelApcDelivery);

		KiCheckForKernelApcDelivery();
	}
}

VOID AcquireProcessRegionLock2k3(
		IN PPROCESS_OBJECT_2K3 ProcessObject)
{
	AcquireGuardedMutex(
			&ProcessObject->AddressCreationLock);
	AcquireGuardedMutex(
			&ProcessObject->WorkingSetMutex);
}

VOID ReleaseProcessRegionLock2k3(
		IN PPROCESS_OBJECT_2K3 ProcessObject)
{
	ReleaseGuardedMutex(
			&ProcessObject->WorkingSetMutex);
	ReleaseGuardedMutex(
			&ProcessObject->AddressCreationLock);
}

PMMVAD GetProcessVadRoot2k3(
		IN PPROCESS_OBJECT_2K3 ProcessObject)
{
	return &ProcessObject->VadRoot;
}

PVOID GetVadStartingVpn2k3(
		IN PMMADDRESS_NODE_2K3 Vad)
{
	return Vad->StartingVpn;
}

PVOID GetVadEndingVpn2k3(
		IN PMMADDRESS_NODE_2K3 Vad)
{
	return Vad->EndingVpn;
}

PMMVAD GetVadLeftChild2k3(
		IN PMMADDRESS_NODE_2K3 Vad)
{
	return Vad->LeftChild;
}

PMMVAD GetVadRightChild2k3(
		IN PMMADDRESS_NODE_2K3 Vad)
{
	return Vad->RightChild;
}

ULONG GetSectionObjectControlAreaFlags2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return SectionObject->Segment->ControlArea->Flags;
}

PFILE_OBJECT GetSectionObjectControlAreaFilePointer2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return SectionObject->Segment->ControlArea->FilePointer;
}

PVOID GetSectionObjectSegmentBasedAddress2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return SectionObject->Segment->BasedAddress;
}

VOID SetSectionObjectSegmentBasedAddress2k3(
		IN PSECTION_OBJECT_2K3 SectionObject,
		IN PVOID BasedAddress)
{
	SectionObject->Segment->BasedAddress = BasedAddress;
}

ULONG GetSectionObjectControlAreaNumberOfMappedViews2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return SectionObject->Segment->ControlArea->NumberOfMappedViews;
}

ULONG GetSectionObjectControlAreaNumberOfSectionReferences2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return SectionObject->Segment->ControlArea->NumberOfSectionReferences;
}

PLARGE_INTEGER GetSectionObjectSegmentSize2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return &SectionObject->Segment->SizeOfSegment;
}

PSECTION_IMAGE_INFORMATION GetSectionObjectSegmentImageInformation2k3(
		IN PSECTION_OBJECT_2K3 SectionObject)
{
	return SectionObject->Segment->u2.ImageInformation;
}
