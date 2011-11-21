/*
 * TDI HOOK DEMO by boywhp@126.com
 * Thanks to tdifw1.4.4
 */

#include <ntddk.h>
#include <TdiKrnl.h>
#include "tdihook.h"
#include "key_object.h"

PDEVICE_OBJECT g_tcpfltobj = NULL;
PDEVICE_OBJECT g_udpfltobj = NULL;
DRIVER_OBJECT g_old_DriverObject;
PVOID g_hash_pool = NULL;

struct tdi_obj{
        PFILE_OBJECT fileobj, associate_obj;
        UCHAR   local_addr[TA_ADDRESS_MAX];
};

unsigned long ntohl (unsigned long netlong)
{
        unsigned long result = 0;
        ((char *)&result)[0] = ((char *)&netlong)[3];
        ((char *)&result)[1] = ((char *)&netlong)[2];
        ((char *)&result)[2] = ((char *)&netlong)[1];
        ((char *)&result)[3] = ((char *)&netlong)[0];
        return result;
}

unsigned short ntohs (unsigned short netshort)
{
        unsigned short result = 0;
        ((char *)&result)[0] = ((char *)&netshort)[1];
        ((char *)&result)[1] = ((char *)&netshort)[0];
        return result;
}

NTSTATUS tdi_skip_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
        TDI_SKIP_CTX *ctx = (TDI_SKIP_CTX *)Context;
        NTSTATUS status;
        PIO_STACK_LOCATION irps;
        
        if (Irp->IoStatus.Status != STATUS_SUCCESS)
                KdPrint(("[tdi_fw] tdi_skip_complete: status 0x%x\n", Irp->IoStatus.Status));
        
        // restore IRP for using in our completion
        
        Irp->CurrentLocation--;
        Irp->Tail.Overlay.CurrentStackLocation--;
        
        irps = IoGetCurrentIrpStackLocation(Irp);
        
        DeviceObject = irps->DeviceObject;
        
        if (ctx->new_cr != NULL) {
                // restore fileobject (it's NULL)
                irps->FileObject = ctx->fileobj;
                // set new device object in irps
                irps->DeviceObject = ctx->new_devobj;
                
                // call new completion 
                status = ctx->new_cr(ctx->new_devobj, Irp, ctx->new_context);
                
        } else
                status = STATUS_SUCCESS;
        
        /* patch IRP back */
        
        // restore routine and context (and even control!)
        irps->CompletionRoutine = ctx->old_cr;
        irps->Context = ctx->old_context;
        irps->Control = ctx->old_control;
        
        // restore device object
        irps->DeviceObject = DeviceObject;
        
        Irp->CurrentLocation++;
        Irp->Tail.Overlay.CurrentStackLocation++;
        
        if (ctx->old_cr != NULL) {
                
                if (status != STATUS_MORE_PROCESSING_REQUIRED) {
                        // call old completion (see the old control)
                        BOOLEAN b_call = FALSE;
                        
                        if (Irp->Cancel) {
                                // cancel
                                if (ctx->old_control & SL_INVOKE_ON_CANCEL)
                                        b_call = TRUE;
                        } else {
                                if (Irp->IoStatus.Status >= STATUS_SUCCESS) {
                                        // success
                                        if (ctx->old_control & SL_INVOKE_ON_SUCCESS)
                                                b_call = TRUE;
                                } else {
                                        // error
                                        if (ctx->old_control & SL_INVOKE_ON_ERROR)
                                                b_call = TRUE;
                                }
                        }
                        
                        if (b_call)
                                status = ctx->old_cr(DeviceObject, Irp, ctx->old_context);
                        
                } else {
                        
                        /*
                         * patch IRP to set IoManager to call completion next time
                         */
                        
                        // restore Control
                        irps->Control = ctx->old_control;
                }
        }
        
        free(ctx);
        
        return status;
}

/*
 * Completion routines must call this function at the end of their execution
 */
NTSTATUS tdi_generic_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
        KdPrint(("[tdi_fw] tdi_generic_complete: STATUS = 0x%x\n", Irp->IoStatus.Status));
        
        if (Irp->PendingReturned) {
                KdPrint(("[tdi_fw] tdi_generic_complete: PENDING\n"));
                IoMarkIrpPending(Irp);
        }
        
        return STATUS_SUCCESS;
}

/*
 * Dispatch routines call this function to complete their processing.
 * They _MUST_ call this function anyway.
 */
NTSTATUS tdi_dispatch_complete(PDEVICE_OBJECT devobj, PIRP irp, int filter, 
                               PIO_COMPLETION_ROUTINE cr, PVOID context)
{
        PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(irp);
        NTSTATUS status;
        
        if (filter == FILTER_DENY) {
                if (irp->IoStatus.Status == STATUS_SUCCESS) {
                        // change status
                        status = irp->IoStatus.Status = STATUS_ACCESS_DENIED;
                } else {
                        // set IRP status unchanged
                        status = irp->IoStatus.Status;
                }
                
                IoCompleteRequest (irp, IO_NO_INCREMENT);                
        } else if (filter == FILTER_ALLOW) {
                if (cr != NULL) {                        
                        // save old completion routine and context
                        TDI_SKIP_CTX *ctx = (TDI_SKIP_CTX *)malloc_np(sizeof(*ctx));
                        if (ctx == NULL) {
                                KdPrint(("[tdi_fw] tdi_send_irp_to_old_driver: malloc_np\n"));
                                
                                status = irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                                IoCompleteRequest(irp, IO_NO_INCREMENT);
                                
                                return status;
                        }
                        
                        ctx->old_cr = irps->CompletionRoutine;
                        ctx->old_context = irps->Context;
                        ctx->new_cr = cr;
                        ctx->new_context = context;
                        ctx->fileobj = irps->FileObject;
                        ctx->new_devobj = devobj;
                        
                        ctx->old_control = irps->Control;

                        irps->Context = ctx;
                        irps->CompletionRoutine = (PIO_COMPLETION_ROUTINE) tdi_skip_complete;
                        irps->Control=SL_INVOKE_ON_ERROR|SL_INVOKE_ON_SUCCESS|SL_INVOKE_ON_CANCEL;
                        //IoSetCompletionRoutine(irp, tdi_skip_complete, ctx, TRUE, TRUE, TRUE);
                }
                /* call original driver */                
                status = g_old_DriverObject.MajorFunction[irps->MajorFunction](devobj, irp);
        } else {	/* FILTER_UNKNOWN */
                status = irp->IoStatus.Status = STATUS_SUCCESS;	// ???
                IoCompleteRequest (irp, IO_NO_INCREMENT);
        }
        
        return status;
}

NTSTATUS get_device_object(wchar_t *name, PDEVICE_OBJECT *devobj)
{
        UNICODE_STRING str;
        NTSTATUS status;
        PFILE_OBJECT fileobj;
        
        RtlInitUnicodeString(&str, name);
        
        status = IoGetDeviceObjectPointer(&str, FILE_ALL_ACCESS, &fileobj, devobj);
        if (status == STATUS_SUCCESS)
                ObDereferenceObject(fileobj);
        
        return status;
}

int tdi_connect(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
	PTDI_REQUEST_KERNEL_CONNECT param = (PTDI_REQUEST_KERNEL_CONNECT)(&irps->Parameters);
	TA_ADDRESS *remote_addr = ((TRANSPORT_ADDRESS *)(param->RequestConnectionInformation->RemoteAddress))->Address;
        TA_ADDRESS *local_addr = NULL;
        struct tdi_obj* obj_item = NULL;

        obj_item = get_object_hash_pool(g_hash_pool, (unsigned long)irps->FileObject);

        if (obj_item){
                local_addr = (TA_ADDRESS *)obj_item->local_addr;

                /* 监控TCP外连参数 ---在这里处理 */
                KdPrint(("[tdi_fw] pid:%d tdi_connect: connobj 0x%x, %x:%u -> %x:%u\n",
                        PsGetCurrentProcessId(),
                        irps->FileObject,                        
                        ntohl(((TDI_ADDRESS_IP *)(local_addr->Address))->in_addr),
		        ntohs(((TDI_ADDRESS_IP *)(local_addr->Address))->sin_port),
                        ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr),
		        ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port)));

                del_object_hash_pool(g_hash_pool, (unsigned long)irps->FileObject);
        }

	return FILTER_ALLOW;
}

/* this completion routine gets address and port from reply to TDI_QUERY_ADDRESS_INFO */
NTSTATUS tdi_create_addrobj_complete2(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
        NTSTATUS status;
        TDI_CREATE_ADDROBJ2_CTX *ctx = (TDI_CREATE_ADDROBJ2_CTX *)Context;
        TA_ADDRESS *addr = ctx->tai->Address.Address;
        struct tdi_obj* obj_item = NULL;
        
        KdPrint(("[tdi_fw] tdi_create_addrobj_complete2: address: %x:%u:0x%x\n", 
                ntohl(((TDI_ADDRESS_IP *)(addr->Address))->in_addr),
                ntohs(((TDI_ADDRESS_IP *)(addr->Address))->sin_port),
                ctx->fileobj));
        
        obj_item = (struct tdi_obj*)malloc_np(sizeof(*obj_item));

        memcpy(obj_item->local_addr, addr, addr->AddressLength);
        obj_item->associate_obj = NULL;
        obj_item->fileobj = ctx->fileobj;
        
        if (get_object_hash_pool(g_hash_pool, (unsigned long)ctx->fileobj))
                del_object_hash_pool(g_hash_pool, (unsigned long)ctx->fileobj);

        add_object_hash_pool(g_hash_pool, (unsigned long)ctx->fileobj, obj_item);

        status = STATUS_SUCCESS;
        
        // cleanup MDL to avoid unlocking pages from NonPaged pool
        if (Irp->MdlAddress != NULL) {
                IoFreeMdl(Irp->MdlAddress);
                Irp->MdlAddress = NULL;
        }
        
        free(ctx->tai);
        free(ctx);
        
        // success anyway
        return STATUS_SUCCESS;
}

NTSTATUS tdi_create_addrobj_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
        NTSTATUS status;
        PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(Irp);
        PIRP query_irp = (PIRP)Context;
        PDEVICE_OBJECT devobj;
        TDI_CREATE_ADDROBJ2_CTX *ctx = NULL;
        PMDL mdl = NULL;
        
        if (Irp->IoStatus.Status != STATUS_SUCCESS) {
                KdPrint(("[tdi_fw] tdi_create_addrobj_complete: status 0x%x\n", Irp->IoStatus.Status));
                
                status = Irp->IoStatus.Status;
                goto done;
        }
        
        // query addrobj address:port
        
        ctx = (TDI_CREATE_ADDROBJ2_CTX *)malloc_np(sizeof(TDI_CREATE_ADDROBJ2_CTX));
        if (ctx == NULL) {
                KdPrint(("[tdi_fw] tdi_create_addrobj_complete: malloc_np\n"));
                
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
        }

        ctx->fileobj = irps->FileObject;
        
        ctx->tai = (TDI_ADDRESS_INFO *)malloc_np(TDI_ADDRESS_INFO_MAX);
        if (ctx->tai == NULL) {
                KdPrint(("[tdi_fw] tdi_create_addrobj_complete: malloc_np!\n"));
                
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
        }
        
        mdl = IoAllocateMdl(ctx->tai, TDI_ADDRESS_INFO_MAX, FALSE, FALSE, NULL);
        if (mdl == NULL) {
                KdPrint(("[tdi_fw] tdi_create_addrobj_complete: IoAllocateMdl!\n"));
                
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
        }
        MmBuildMdlForNonPagedPool(mdl);
        
        devobj = DeviceObject;
        if (devobj == NULL) {
                KdPrint(("[tdi_fw] tdi_create_addrobj_complete: get_original_devobj!\n"));
                
                status = STATUS_INVALID_PARAMETER;
                goto done;
        }
        
        TdiBuildQueryInformation(query_irp, devobj, irps->FileObject,
                tdi_create_addrobj_complete2, ctx,
                TDI_QUERY_ADDRESS_INFO, mdl);
        
        status = IoCallDriver(devobj, query_irp);
        query_irp = NULL;
        mdl = NULL;
        ctx = NULL;
        
        if (status != STATUS_SUCCESS) {
                KdPrint(("[tdi_fw] tdi_create_addrobj_complete: IoCallDriver: 0x%x\n", status));
                goto done;
        }
        
        status = STATUS_SUCCESS;
        
done:
        // cleanup
        if (mdl != NULL)
                IoFreeMdl(mdl);
        
        if (ctx != NULL) {
                if (ctx->tai != NULL)
                        free(ctx->tai);
                free(ctx);
        }
        
        if (query_irp != NULL)
                IoCompleteRequest(query_irp, IO_NO_INCREMENT);
        
        Irp->IoStatus.Status = status;
        
        return tdi_generic_complete(DeviceObject, Irp, Context);
}

int tdi_associate_address(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
        HANDLE addr_handle = ((TDI_REQUEST_KERNEL_ASSOCIATE *)(&irps->Parameters))->AddressHandle;
        PFILE_OBJECT addrobj = NULL;
        NTSTATUS status;
        int result = FILTER_DENY;
        struct tdi_obj *tmp_obj, *obj_item;
        
        KdPrint(("[tdi_fw] tdi_associate_address: devobj 0x%x; connobj 0x%x\n",
                irps->DeviceObject, irps->FileObject));
        
        status = ObReferenceObjectByHandle(addr_handle, GENERIC_READ, NULL, KernelMode, &addrobj, NULL);
        if (status != STATUS_SUCCESS) {
                KdPrint(("[tdi_fw] tdi_associate_address: ObReferenceObjectByHandle: 0x%x\n", status));
                goto done;
        }
        
        KdPrint(("[tdi_fw] tdi_associate_address: connobj = 0x%x ---> addrobj = 0x%x\n",
                irps->FileObject, addrobj));
        
        tmp_obj = get_object_hash_pool(g_hash_pool, (unsigned long)addrobj);
        if (tmp_obj){
                obj_item = (struct tdi_obj *)malloc_np(sizeof(*obj_item));
                
                memcpy(obj_item->local_addr, tmp_obj->local_addr, TA_ADDRESS_MAX);
                obj_item->associate_obj = addrobj;
                obj_item->fileobj = irps->FileObject;

                if (get_object_hash_pool(g_hash_pool, (unsigned long)obj_item->fileobj))
                        del_object_hash_pool(g_hash_pool, (unsigned long)obj_item->fileobj);

                add_object_hash_pool(g_hash_pool, (unsigned long)obj_item->fileobj, obj_item);

                del_object_hash_pool(g_hash_pool, (unsigned long)addrobj);
        }
        result = FILTER_ALLOW;
done:
        if (addrobj != NULL)
                ObDereferenceObject(addrobj);
        
        return result;
}

int tdi_create(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
	FILE_FULL_EA_INFORMATION *ea = (FILE_FULL_EA_INFORMATION *)irp->AssociatedIrp.SystemBuffer;

	if (ea != NULL) {
		PDEVICE_OBJECT devobj;
		
		devobj = irps->DeviceObject;
		if (devobj == NULL) {
			KdPrint(("[tdi_fw] tdi_create: unknown device object 0x%x!\n", irps->DeviceObject));
			return FILTER_DENY;
		}
		// NOTE: for RawIp you can extract protocol number from irps->FileObject->FileName

		if (ea->EaNameLength == TDI_TRANSPORT_ADDRESS_LENGTH &&
			memcmp(ea->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH) == 0) {

			PIRP query_irp;

			/*
			 * This is creation of address object
			 */

			KdPrint(("[tdi_fw] tdi_create: devobj 0x%x; addrobj 0x%x\n",
				irps->DeviceObject,
				irps->FileObject));

			// while we're on PASSIVE_LEVEL build control IRP for completion
			query_irp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION,
				devobj, irps->FileObject, NULL, NULL);
			if (query_irp == NULL) {
				KdPrint(("[tdi_fw] tdi_create: TdiBuildInternalDeviceControlIrp\n"));
				return FILTER_DENY;
			}

			/* set IRP completion & context for completion */

			completion->routine = tdi_create_addrobj_complete;
			completion->context = query_irp;

                } else if (ea->EaNameLength == TDI_CONNECTION_CONTEXT_LENGTH &&
                        memcmp(ea->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) == 0) {
                        
                        /*
                         * This is creation of connection object
                         */
                        
                        CONNECTION_CONTEXT conn_ctx = *(CONNECTION_CONTEXT *)
                                (ea->EaName + ea->EaNameLength + 1);
                        
                        KdPrint(("[tdi_fw] tdi_create: devobj 0x%x; connobj 0x%x; conn_ctx 0x%x\n",
                                irps->DeviceObject,
                                irps->FileObject,
                                conn_ctx));
                }
	
	}

	return FILTER_ALLOW;
}

NTSTATUS TdiHookDeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	PIO_STACK_LOCATION irps;
	NTSTATUS status;
        int result;
        struct completion completion = {0};

	// sanity check
	if (irp == NULL)
		return STATUS_SUCCESS;
	
	irps = IoGetCurrentIrpStackLocation(irp);

	if (DeviceObject == g_tcpfltobj || DeviceObject == g_udpfltobj){
                switch (irps->MajorFunction) {
                case IRP_MJ_CREATE:
                        result = tdi_create(irp, irps, &completion);
                        status = tdi_dispatch_complete(DeviceObject, irp, result,
				completion.routine, completion.context);
                        break;
                case IRP_MJ_DEVICE_CONTROL:                        
                        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
                                /*
                                * try to convert it to IRP_MJ_INTERNAL_DEVICE_CONTROL
                                * (works on PASSIVE_LEVEL only!)
                                */
                                status = TdiMapUserRequest(DeviceObject, irp, irps);
                                
                        } else
                                status = STATUS_NOT_IMPLEMENTED; // set fake status
                        
                        if (status != STATUS_SUCCESS) {
                                void *buf = (irps->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER) ?
                                        irps->Parameters.DeviceIoControl.Type3InputBuffer : NULL;
                                
                                // send IRP to original driver
                                status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW, NULL, NULL);
                                
                                break;
                        }                        
                        // don't break! go to internal device control!
                case IRP_MJ_INTERNAL_DEVICE_CONTROL: 
                        /* 根据irps->MinorFunction类型进行处理 */
                        if (irps->MinorFunction == TDI_CONNECT){
                                tdi_connect(irp, irps, &completion);
                        }else if(irps->MinorFunction == TDI_ASSOCIATE_ADDRESS){
                                tdi_associate_address(irp, irps, &completion);
                        }

                        status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW,
						completion.routine, completion.context);
                        break;
                default:
                        // passthrough IRP
                        status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW, NULL, NULL);
                }

        } else {
		// call original handler
		status = g_old_DriverObject.MajorFunction[irps->MajorFunction](
			DeviceObject, irp);
        }
        return status;
}

NTSTATUS hook_tcpip(DRIVER_OBJECT *old_DriverObject, BOOLEAN b_hook)
{
        UNICODE_STRING drv_name;
        NTSTATUS status;
        PDRIVER_OBJECT new_DriverObject;
        int i;
        
        status = get_device_object(L"\\Device\\Tcp", &g_tcpfltobj);
        status = get_device_object(L"\\Device\\Udp", &g_udpfltobj);

        RtlInitUnicodeString(&drv_name, L"\\Driver\\Tcpip");
        
        status = ObReferenceObjectByName(&drv_name, OBJ_CASE_INSENSITIVE, NULL, 0,
                IoDriverObjectType, KernelMode, NULL, &new_DriverObject);
        if (status != STATUS_SUCCESS) {
                KdPrint(("[tdi_fw] hook_driver: ObReferenceObjectByName\n"));
                return status;
        }
        
        if (b_hook)
                KdPrint(("[tdi_fw] hook_tcpip %x %x %x...\n",
                        g_tcpfltobj, 
                        g_udpfltobj,
                        new_DriverObject));
        else
                KdPrint(("[tdi_fw] unhook_tcpip...\n"));

        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
                if (b_hook) {
                        old_DriverObject->MajorFunction[i] = new_DriverObject->MajorFunction[i];
                        new_DriverObject->MajorFunction[i] = TdiHookDeviceDispatch;
                } else
                        new_DriverObject->MajorFunction[i] = old_DriverObject->MajorFunction[i];
        }
        
        return STATUS_SUCCESS;	
}

int init_tdi_hook()
{
        NTSTATUS status = STATUS_SUCCESS;

        g_hash_pool = alloc_object_hash_pool(256, 256);
        status = hook_tcpip(&g_old_DriverObject, TRUE);
        if (status != STATUS_SUCCESS)
                return -1;

        return 0;
}

void clearup_tdi_hook()
{
        hook_tcpip(&g_old_DriverObject, FALSE);
        free_object_hash_pool(g_hash_pool);
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
        clearup_tdi_hook();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = DriverUnload;	

    init_tdi_hook();

    return(STATUS_SUCCESS);
}