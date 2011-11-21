#ifndef _TDI_HOOK_H_
#define _TDI_HOOK_H_

#define MEM_TAG		'1VRD'
#define malloc_np(size)	ExAllocatePoolWithTag(NonPagedPool, (size), MEM_TAG)
#define free(ptr)		ExFreePool(ptr)

// maximum length of TDI_ADDRESS_TYPE_*
#define TDI_ADDRESS_MAX_LENGTH	TDI_ADDRESS_LENGTH_OSI_TSAP
#define TA_ADDRESS_MAX			(sizeof(TA_ADDRESS) - 1 + TDI_ADDRESS_MAX_LENGTH)
#define TDI_ADDRESS_INFO_MAX	(sizeof(TDI_ADDRESS_INFO) - 1 + TDI_ADDRESS_MAX_LENGTH)

NTSTATUS
TdiMapUserRequest(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN PIO_STACK_LOCATION  IrpSp
  );


NTKERNELAPI
NTSTATUS
ObReferenceObjectByName	(
                         IN PUNICODE_STRING	ObjectName,
                         IN ULONG			Attributes,
                         IN PACCESS_STATE	PassedAccessState OPTIONAL,
                         IN ACCESS_MASK		DesiredAccess OPTIONAL,
                         IN POBJECT_TYPE		ObjectType OPTIONAL,
                         IN KPROCESSOR_MODE	AccessMode,
                         IN OUT PVOID		ParseContext OPTIONAL,
                         OUT	PVOID			*Object
                         );

extern POBJECT_TYPE	IoDriverObjectType;

/* filter result */
enum {
        FILTER_ALLOW = 1,
                FILTER_DENY,
                FILTER_PACKET_LOG,
                FILTER_PACKET_BAD,
                FILTER_DISCONNECT
};

/* context for tdi_skip_complete */
typedef struct {
        PIO_COMPLETION_ROUTINE	old_cr;			/* old (original) completion routine */
        PVOID					old_context;	/* old (original) parameter for old_cr */
        PIO_COMPLETION_ROUTINE	new_cr;			/* new (replaced) completion routine */
        PVOID					new_context;	/* new (replaced) parameter for new_cr */
        PFILE_OBJECT			fileobj;		/* FileObject from IO_STACK_LOCATION */
        PDEVICE_OBJECT			new_devobj;		/* filter device object */
        UCHAR					old_control;	/* old (original) irps->Control */
} TDI_SKIP_CTX;

struct completion {
        PIO_COMPLETION_ROUTINE	routine;
        PVOID					context;
};

// context for tdi_create_addrobj_complete2
typedef struct {
        TDI_ADDRESS_INFO	*tai;		/* address info -- result of TDI_QUERY_ADDRESS_INFO */
        PFILE_OBJECT		fileobj;	/* FileObject from IO_STACK_LOCATION */
} TDI_CREATE_ADDROBJ2_CTX;

unsigned long ntohl(unsigned long data);
unsigned short ntohs(unsigned short data);

#endif