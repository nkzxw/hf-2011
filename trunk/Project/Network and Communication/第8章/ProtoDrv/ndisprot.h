/////////////////////////////////////////////
// ndisprot.h�ļ�
// ����������������ݽṹ������ͺ���ԭ��




#ifndef __NDISPROT_H__
#define __NDISPROT_H__

// �����ڲ����ƺͷ�����������
#define DEVICE_NAME L"\\Device\\devNdisProt"
#define LINK_NAME L"\\DosDevices\\slNdisProt"


typedef struct _GLOBAL
{
	PDRIVER_OBJECT pDriverObj;		// ��������ָ��
	NDIS_HANDLE hNdisProtocol;		// Э�������������NdisRegisterProtocol�������ص�

	LIST_ENTRY AdapterList; 		// Ϊ���ǰ󶨵�ÿ���������������豸�����б�
	KSPIN_LOCK GlobalLock;			// Ϊ��ͬ���������ķ���
	PDEVICE_OBJECT pControlDevice;	// ����������Ŀ����豸����ָ��
} GLOBAL;



typedef struct _INTERNAL_REQUEST
{
	PIRP pIrp;				 
	NDIS_REQUEST Request;
} INTERNAL_REQUEST, *PINTERNAL_REQUEST;

// ÿ��������ҲҪ���Լ���˽�б����������OPEN_INSTANCE�ṹ�����˴򿪵�������
typedef struct _OPEN_INSTANCE
{
	// ��̬����
	LIST_ENTRY AdapterListEntry;	// �������ӵ�����NIC�豸���󣬼����ӵ�ȫ��AdapterList�б�
	PDEVICE_OBJECT pDeviceObj;		// ���������豸�����ָ��
	UNICODE_STRING ustrAdapterName;	// ��������������
	UNICODE_STRING ustrLinkName;	// ����������Ӧ����������ķ�����������
	NDIS_HANDLE hAdapter;			// ���������
	
	// ״̬��Ϣ
	BOOLEAN bBound;			// �Ƿ��
	NDIS_STATUS Status;		// ״̬����		
	ULONG nIrpCount;		// ��ǰ�û��ڴ��������ϵ�IRP��������
	
	// ����ؾ��
	NDIS_HANDLE	hPacketPool;

	// �����б�
	LIST_ENTRY RcvList;
	KSPIN_LOCK	RcvSpinLock;
	// �����б�
	LIST_ENTRY	ResetIrpList;
	KSPIN_LOCK ResetQueueLock;
	
	// ͬ���¼�
	NDIS_EVENT BindEvent;
	NDIS_EVENT CleanupEvent;

	NDIS_MEDIUM Medium;		// ��������������
} OPEN_INSTANCE, *POPEN_INSTANCE;

typedef struct _PACKET_RESERVED
{
	LIST_ENTRY ListElement;		// �������������������һ��
	PIRP	pIrp;				// ��¼�˷����Ӧ��δ����IRP����
	PMDL	pMdl;				// ��¼Ϊ�˷�������MDL
}	PACKET_RESERVED, *PPACKET_RESERVED;


#define ETHERNET_HEADER_LENGTH	14
#define RESERVED(_p) ((PACKET_RESERVED*)((_p)->ProtocolReserved))



extern GLOBAL g_data;



VOID 
  ProtocolBindAdapter(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE  BindContext,
    IN PNDIS_STRING  DeviceName,
    IN PVOID  SystemSpecific1,
    IN PVOID  SystemSpecific2
    );
VOID
  ProtocolUnbindAdapter(
      OUT PNDIS_STATUS  Status,
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_HANDLE  UnbindContext
      );


// �Զ��庯��������
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
void DriverUnload(PDRIVER_OBJECT pDriverObj);

NTSTATUS DispatchRead(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS DispatchWrite(PDEVICE_OBJECT pDevObj, PIRP pIrp);

void IoIncrement(OPEN_INSTANCE *pOpen);


void IoDecrement(OPEN_INSTANCE *pOpen);

VOID
ProtocolTransferDataComplete (
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status,
    IN UINT          BytesTransfered
    );


NDIS_STATUS
ProtocolReceive(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID       HeaderBuffer,
    IN UINT        HeaderBufferSize,
    IN PVOID       LookAheadBuffer,
    IN UINT        LookaheadBufferSize,
    IN UINT        PacketSize
    );

int ProtocolReceivePacket(NDIS_HANDLE ProtocolBindingContext, PNDIS_PACKET Packet);

void CancelReadIrp(PDEVICE_OBJECT pDeviceObj);

NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp);

NTSTATUS
GetAdapterList(
    IN  PVOID              Buffer,
    IN  ULONG              Length,
    IN  OUT PULONG         DataLength
    );

VOID
PacketResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );

VOID
  ProtocolUnbindAdapter(
      OUT PNDIS_STATUS  Status,
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_HANDLE  UnbindContext
      );

VOID
  ProtocolOpenAdapterComplete(
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_STATUS  Status,
      IN NDIS_STATUS  OpenErrorStatus
      );
VOID 
  ProtocolCloseAdapterComplete(
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_STATUS  Status
      );

VOID
ReadCancelRoutine (
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

VOID
ProtocolSendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    );

VOID
ProtocolResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );

VOID
ProtocolRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status
    );

VOID
PacketRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status
    );

NTSTATUS
DispatchCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ProtocolReceiveComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    );

VOID
ProtocolStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    );

VOID
ProtocolStatusComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    );

NDIS_STATUS
ProtocolPNPHandler(
    IN    NDIS_HANDLE        ProtocolBindingContext,
    IN    PNET_PNP_EVENT     NetPnPEvent
    );

#endif // __NDISPROT_H__