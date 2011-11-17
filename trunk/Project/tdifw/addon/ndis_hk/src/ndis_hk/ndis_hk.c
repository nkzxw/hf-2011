// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: ndis_hk.c,v 1.7 2003/07/07 11:41:55 dev Exp $

/** @addtogroup hook_driver Hook Driver
 *@{
 */

/**
 * @file ndis_hk.c
 * NDIS hooking engine!
 */

#include <ntddk.h>

#include "adapters.h"
#include "av.h"
#include "memtrack.h"
#include "ndis_hk.h"
#include "filter.h"
#include "ndis_hk_ioctl.h"
#include "nt.h"
#include "pe.h"
#include "except.h"

/* globals */

/** macro do build entry for hooked function array g_hook_fn */
#define HOOK_FN_ENTRY(name) \
	{#name, NULL, new_##name}

struct hook_fn g_hook_fn[MAX_HOOK_FN] = {
	// see hooked_fn.c
	HOOK_FN_ENTRY(NdisRegisterProtocol),
	HOOK_FN_ENTRY(NdisDeregisterProtocol),
	HOOK_FN_ENTRY(NdisOpenAdapter),
	HOOK_FN_ENTRY(NdisCloseAdapter)
};

NDIS_HANDLE g_buffer_pool = NULL;
NDIS_HANDLE g_packet_pool = NULL;

/** device object for control device */
static PDEVICE_OBJECT g_devcontrol = NULL;

/** ndis_hk kernel-mode interface */
static const struct ndis_hk_interface g_interface = {
	NDIS_HK_INTERFACE_VER,
	get_adapter_list,
	attach_filter,
	ndis_request
};

/* prototypes */

static NTSTATUS		DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
static VOID			OnUnload(IN PDRIVER_OBJECT DriverObject);

static NTSTATUS		hook_ndis(int unhook);

static void			*find_system_dll(const char *name);
static void			*fix_export(char *base, const char *fn, void *new_fn);

static BOOLEAN		replace_value_safe(ULONG *addr, ULONG value);

/**
 * Main driver function
 */
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT theDriverObject,
            IN PUNICODE_STRING theRegistryPath)
{
	NTSTATUS status;
	int i;
	UNICODE_STRING devname;

 	memtrack_init();
	init_adapter_list();
	init_filter();
	
	__try {
		
		// allocate NDIS packet & buffer pools
		NdisAllocatePacketPool(&status, &g_packet_pool, 100, sizeof(struct protocol_reserved));
		if (status != NDIS_STATUS_SUCCESS) {
			KdPrint(("[ndis_hk] DriverEntry: NdisAllocatePacketPool: 0x%x\n", status));
			__leave;
		}
		
		NdisAllocateBufferPool(&status, &g_buffer_pool, 100);
		if (status != NDIS_STATUS_SUCCESS) {
			KdPrint(("[ndis_hk] DriverEntry: NdisAllocateBufferPool: 0x%x\n", status));
			__leave;
		}
		
		status = init_av();
		if (status != STATUS_SUCCESS) {
			KdPrint(("[ndis_hk] DriverEntry: init_av: 0x%x\n", status));
			__leave;
		}
		
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
			theDriverObject->MajorFunction[i] = DeviceDispatch;
		
		// create control device
		RtlInitUnicodeString(&devname, L"\\Device\\ndis_hk");
		
		status = IoCreateDevice(theDriverObject,
			0,
			&devname,
			0,
			0,
			FALSE,
			&g_devcontrol);
		if (status != STATUS_SUCCESS) {
			KdPrint(("[ndis_hk] DriverEntry: IoCreateDevice(control): 0x%x!\n", status));
			__leave;
		}
		
#if DBG
		// register UnLoad procedure
		theDriverObject->DriverUnload = OnUnload;
#endif
		
		status = hook_ndis(FALSE);
		if (status != STATUS_SUCCESS)
			KdPrint(("[ndis_hk] DriverEntry: hook_ndis: 0x%x!\n", status));
		
	} __except((status = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)) {
		KdPrint(("[ndis_hk] DriverEntry: exception 0x%x!\n", status));
	}
	
	if (status != STATUS_SUCCESS)
		OnUnload(theDriverObject);
	
	return status;
}

/**
 * Dispatch function.
 * Works with i/o controls for control device
 */
NTSTATUS
DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	NTSTATUS status;

	// set irp with defaults
	irp->IoStatus.Information = 0;

	if (DeviceObject == g_devcontrol) {
		PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(irp);

		switch (irps->MajorFunction) {
		case IRP_MJ_CREATE:
		case IRP_MJ_CLEANUP:
		case IRP_MJ_CLOSE:
			status = STATUS_SUCCESS;
			break;

		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			if (irps->Parameters.DeviceIoControl.IoControlCode == IOCTL_CMD_GET_KM_IFACE) {
				
				if (irps->Parameters.DeviceIoControl.OutputBufferLength == sizeof(struct ndis_hk_interface *)) {

					status = STATUS_SUCCESS;

					if (irps->Parameters.DeviceIoControl.InputBufferLength == sizeof(ULONG)) {
						// check version
						ULONG version = *(ULONG *)irp->AssociatedIrp.SystemBuffer;
						if (version > NDIS_HK_INTERFACE_VER)
							status = STATUS_NOT_SUPPORTED;
					
					} else if (irps->Parameters.DeviceIoControl.InputBufferLength != 0)
						status = STATUS_INFO_LENGTH_MISMATCH;

					if (status == STATUS_SUCCESS) {
						// return pointer to interface structure
						*(const struct ndis_hk_interface **)(irp->AssociatedIrp.SystemBuffer) = &g_interface;
						irp->IoStatus.Information = sizeof(struct ndis_hk_interface *);
					}

				} else
					status = STATUS_INFO_LENGTH_MISMATCH;

			} else
				status = STATUS_NOT_SUPPORTED;
			
			break;

		default:
			status = STATUS_NOT_SUPPORTED;
		}
	} else
		status = STATUS_NOT_SUPPORTED;

	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

/*
 * Unload procedure
 * Driver can't be unloaded due to security reasons.
 * This function only for memory leak testing
 */
VOID
OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	// unhook NDIS
	hook_ndis(TRUE);

	free_av();
	free_adapter_list();

	memtrack_free();
}

/**
 * Hook or unhook NDIS functions
 * @param	unhook				if (!unhook) hook; else unhook;
 * @retval	STATUS_SUCCESS		no error
 */
NTSTATUS
hook_ndis(int unhook)
{
	void *ndis_sys;
	int i;

	// 1. find ndis.sys
	ndis_sys = find_system_dll("NDIS.sys");
	if (ndis_sys == NULL) {
		KdPrint(("[ndis_hk] hook_ndis: find_system_dll!\n"));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	// 2. (un)hook all of the functions
	for (i = 0; i < MAX_HOOK_FN; i++) {
		if (!unhook) {
			void *old_fn = fix_export((char *)ndis_sys, g_hook_fn[i].name, g_hook_fn[i].new_fn);

			if (old_fn == NULL) {
				KdPrint(("[ndis_hk] hook_ndis: fix_export!\n"));

				// replace them back!
				hook_ndis(TRUE);

				return STATUS_OBJECT_NAME_NOT_FOUND;
			}
			
			KdPrint(("[ndis_hk] hook_ndis: %s: old: 0x%x new: 0x%x\n", 
				g_hook_fn[i].name,
				old_fn,
				g_hook_fn[i].new_fn));

			g_hook_fn[i].old_fn = old_fn;
		
		} else {
			if (g_hook_fn[i].old_fn != NULL)
				fix_export((char *)ndis_sys, g_hook_fn[i].name, g_hook_fn[i].old_fn);
		}
	}

	return STATUS_SUCCESS;
}

/**
 * Find base address of system module.
 * Used to find base address of ndis.sys
 * @param	name	name of system module
 * @return			base address of system module
 * @retval	NULL	system module not found
 */
void *
find_system_dll(const char *name)
{
	ULONG i, n, *q;
	PSYSTEM_MODULE_INFORMATION p;
	void *base;

	ZwQuerySystemInformation(SystemModuleInformation, &n, 0, &n);
	q = (ULONG *)ExAllocatePool(PagedPool, n);
	ZwQuerySystemInformation(SystemModuleInformation, q, n * sizeof (*q), 0);
	
	p = (PSYSTEM_MODULE_INFORMATION)(q + 1);
	base = NULL;
	for (i = 0; i < *q; i++) {
		if (_stricmp(p[i].ImageName + p[i].ModuleNameOffset, name) == 0) {
			base = p[i].Base;
			KdPrint(("[ndis_hk] find_system_dll: %s; base = 0x%x; size = 0x%x\n", name, base, p[i].Size));
			break;
		}
	}
		
	ExFreePool(q);
	return base;
}

/**
 * Fix export table in module
 * @param	base	base address of module
 * @param	fn		name of function
 * @param	new_fn	new address of function
 * @return			old address of function
 */
void *
fix_export(char *base, const char *fn, void *new_fn)
{
	PIMAGE_DOS_HEADER dos_hdr;
	PIMAGE_NT_HEADERS nt_hdr;
	PIMAGE_EXPORT_DIRECTORY export_dir;
	ULONG *fn_name, *fn_addr, i;

	dos_hdr = (PIMAGE_DOS_HEADER)base;

	if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	nt_hdr = (PIMAGE_NT_HEADERS)(base + dos_hdr->e_lfanew);

	export_dir = (PIMAGE_EXPORT_DIRECTORY)(base + nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	fn_name = (ULONG *)(base + export_dir->AddressOfNames);
	fn_addr = (ULONG *)(base + export_dir->AddressOfFunctions);

	for (i = 0; i < export_dir->NumberOfNames; i++, fn_name++, fn_addr++) {
		if (strcmp(fn, base + *fn_name) == 0) {
			void *old_addr = base + *fn_addr;

			// replace value safe
			replace_value_safe(fn_addr, (char *)new_fn - base);

			return old_addr;
		}
	}

	return NULL;
}

/**
 * Replace ULONG value even if memory page has read only attributes
 * @param	addr	address of ULONG
 * @param	value	ULONG value
 * @retval	TRUE	no errors
 */
BOOLEAN
replace_value_safe(ULONG *addr, ULONG value)
{
	MDL *mdl;
	ULONG *virt_addr;

	mdl = IoAllocateMdl(addr, sizeof(value), FALSE, FALSE, NULL);
	if (mdl == NULL)
		return FALSE;

	__try {
	
		MmProbeAndLockPages(mdl, KernelMode, IoModifyAccess);

	} __except(EXCEPTION_EXECUTE_HANDLER) {
		KdPrint(("[ndis_hk] replace_value_safe: MmProbeAndLockPages!\n"));
		return FALSE;
	}

	virt_addr = (ULONG *)MmGetSystemAddressForMdl(mdl);

	*(ULONG *)virt_addr = value;

	MmUnlockPages(mdl);
	IoFreeMdl(mdl);
	return TRUE;
}
/*@}*/