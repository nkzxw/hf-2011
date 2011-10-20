//
// Don't Include W32NDIS.H If Compiling A NDIS Driver
// --------------------------------------------------
// The _NDIS_ preprocessor variable is defined in the beginning of the
// "real" NDIS.H in the various Windows DDK's. The following statement
// causes this header file to be ignored if NDIS.H has laready been
// included.
//
#ifndef _NDIS_

#ifndef _W32NDIS_H
#define _W32NDIS_H

/////////////////////////////////////////////////////////////////////////////
//// SPECIFY STRUCTURE PACKING

#pragma pack(push,4)

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

//
// NtDeviceIoControlFile IoControlCode values for NT NDIS adapter devices.
//

#define _NDIS_CONTROL_CODE(request,method) \
            CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)

#define  IOCTL_NDIS_QUERY_GLOBAL_STATS _NDIS_CONTROL_CODE( 0, METHOD_OUT_DIRECT )
#define  IOCTL_NDIS_QUERY_ALL_STATS    _NDIS_CONTROL_CODE( 1, METHOD_OUT_DIRECT )
#define  IOCTL_NDIS_ADD_DEVICE         _NDIS_CONTROL_CODE( 2, METHOD_BUFFERED )
#define  IOCTL_NDIS_DELETE_DEVICE      _NDIS_CONTROL_CODE( 3, METHOD_BUFFERED )
#define  IOCTL_NDIS_TRANSLATE_NAME     _NDIS_CONTROL_CODE( 4, METHOD_BUFFERED )
#define  IOCTL_NDIS_ADD_TDI_DEVICE     _NDIS_CONTROL_CODE( 5, METHOD_BUFFERED )
#define  IOCTL_NDIS_NOTIFY_PROTOCOL    _NDIS_CONTROL_CODE( 6, METHOD_BUFFERED )
#define  IOCTL_NDIS_GET_LOG_DATA       _NDIS_CONTROL_CODE( 7, METHOD_OUT_DIRECT )

typedef struct _MEMORY_DESCRIPTOR {
    PCHAR Address;
    UINT  Length;
} MEMORY_DESCRIPTOR, *PMEMORY_DESCRIPTOR;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} STRING, *PSTRING;

typedef STRING NDIS_STRING, *PNDIS_STRING;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

typedef ULONG NDIS_STATUS;
typedef NDIS_STATUS *PNDIS_STATUS;

// BUGBUG for compatibility with NT, ask them to remove it from
// Their drivers
typedef NDIS_STATUS NTSTATUS;
#define PDRIVER_OBJECT PVOID
#define PUNICODE_STRING PVOID
#define PDEVICE_OBJECT PVOID

typedef enum _NDIS_REQUEST_TYPE {
    NdisRequestQueryInformation,
    NdisRequestSetInformation,
    NdisRequestQueryStatistics,
    NdisRequestOpen,
    NdisRequestClose,
    NdisRequestSend,
    NdisRequestTransferData,
    NdisRequestReset,
    NdisRequestGeneric1,
    NdisRequestGeneric2,
    NdisRequestGeneric3,
    NdisRequestGeneric4
} NDIS_REQUEST_TYPE, *PNDIS_REQUEST_TYPE;

//
// Object ID
//
typedef ULONG NDIS_OID, *PNDIS_OID;

//
// Medium Ndis Driver is running on
//

typedef enum _NDIS_MEDIUM {
   NdisMedium802_3,
   NdisMedium802_5,
   NdisMediumFddi,
   NdisMediumWan,
   NdisMediumLocalTalk,
   NdisMediumDix,              // defined for convenience, not a real medium
   NdisMediumArcnetRaw,
   NdisMediumArcnet878_2,
   NdisMediumAtm,
   NdisMediumWirelessWan,
   NdisMediumIrda,
   NdisMediumBpc,
   NdisMediumCoWan,
   NdisMediumMax            // Not a real medium, defined as an upper-bound
}
   NDIS_MEDIUM, *PNDIS_MEDIUM;                    

//
// Hardware status codes (OID_GEN_HARDWARE_STATUS).
//

typedef enum _NDIS_HARDWARE_STATUS {
    NdisHardwareStatusReady,
    NdisHardwareStatusInitializing,
    NdisHardwareStatusReset,
    NdisHardwareStatusClosing,
    NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;


//
// Defines the attachment types for FDDI (OID_FDDI_ATTACHMENT_TYPE).
//

typedef enum _NDIS_FDDI_ATTACHMENT_TYPE {
    NdisFddiTypeIsolated = 1,
    NdisFddiTypeLocalA,
    NdisFddiTypeLocalB,
    NdisFddiTypeLocalAB,
    NdisFddiTypeLocalS,
    NdisFddiTypeWrapA,
    NdisFddiTypeWrapB,
    NdisFddiTypeWrapAB,
    NdisFddiTypeWrapS,
    NdisFddiTypeCWrapA,
    NdisFddiTypeCWrapB,
    NdisFddiTypeCWrapS,
    NdisFddiTypeThrough
} NDIS_FDDI_ATTACHMENT_TYPE, *PNDIS_FDDI_ATTACHMENT_TYPE;


//
// Defines the ring management states for FDDI (OID_FDDI_RING_MGT_STATE).
//

typedef enum _NDIS_FDDI_RING_MGT_STATE {
    NdisFddiRingIsolated = 1,
    NdisFddiRingNonOperational,
    NdisFddiRingOperational,
    NdisFddiRingDetect,
    NdisFddiRingNonOperationalDup,
    NdisFddiRingOperationalDup,
    NdisFddiRingDirected,
    NdisFddiRingTrace
} NDIS_FDDI_RING_MGT_STATE, *PNDIS_FDDI_RING_MGT_STATE;


//
// Defines the Lconnection state for FDDI (OID_FDDI_LCONNECTION_STATE).
//

typedef enum _NDIS_FDDI_LCONNECTION_STATE {
    NdisFddiStateOff = 1,
    NdisFddiStateBreak,
    NdisFddiStateTrace,
    NdisFddiStateConnect,
    NdisFddiStateNext,
    NdisFddiStateSignal,
    NdisFddiStateJoin,
    NdisFddiStateVerify,
    NdisFddiStateActive,
    NdisFddiStateMaintenance
} NDIS_FDDI_LCONNECTION_STATE, *PNDIS_FDDI_LCONNECTION_STATE;


//
// Defines the medium subtypes for WAN medium
//

typedef enum _NDIS_WAN_MEDIUM_SUBTYPE {
    NdisWanMediumHub,
    NdisWanMediumX_25,
    NdisWanMediumIsdn,
    NdisWanMediumSerial,
    NdisWanMediumFrameRelay
} NDIS_WAN_MEDIUM_SUBTYPE, *PNDIS_WAN_MEDIUM_SUBTYPE;

//
// Defines the header format for WAN medium
//

typedef enum _NDIS_WAN_HEADER_FORMAT {
    NdisWanHeaderNative,       // src/dest based on subtype, followed by NLPID
    NdisWanHeaderEthernet      // emulation of ethernet header
} NDIS_WAN_HEADER_FORMAT, *PNDIS_WAN_HEADER_FORMAT;

//
// Defines the line quality on an WAN line
//

typedef enum _NDIS_WAN_QUALITY {
    NdisWanRaw,
    NdisWanErrorControl,
    NdisWanReliable
} NDIS_WAN_QUALITY, *PNDIS_WAN_QUALITY;

//
// The structure passed up on a WAN_LINE_UP indication
//

typedef struct _NDIS_WAN_LINE_UP {
    ULONG LinkSpeed;                // 100 bps units
    ULONG MaximumTotalSize;         // suggested max for send packets
    NDIS_WAN_QUALITY Quality;
    USHORT SendWindow;              // suggested by the MAC
    UCHAR Address[1];               // variable length, depends on address type
} NDIS_WAN_LINE_UP, *PNDIS_WAN_LINE_UP;

//
// The structure passed up on a WAN_LINE_DOWN indication
//

typedef struct _NDIS_WAN_LINE_DOWN {
    UCHAR Address[1];               // variable length, depends on address type
} NDIS_WAN_LINE_DOWN, *PNDIS_WAN_LINE_DOWN;

//
// The structure passed up on a WAN_FRAGMENT indication
//

typedef struct _NDIS_WAN_FRAGMENT {
    UCHAR Address[1];               // variable length, depends on address type
} NDIS_WAN_FRAGMENT, *PNDIS_WAN_FRAGMENT;


//
// Defines the state of a token-ring adapter
//

typedef enum _NDIS_802_5_RING_STATE {
    NdisRingStateOpened = 1,
    NdisRingStateClosed,
    NdisRingStateOpening,
    NdisRingStateClosing,
    NdisRingStateOpenFailure,
    NdisRingStateRingFailure
} NDIS_802_5_RING_STATE, *PNDIS_802_5_RING_STATE;


//
// NDIS_STATUS values
//

/* INC */
#define NDIS_STATUS_SUCCESS                 ((NDIS_STATUS)0x00000000L)
#define NDIS_STATUS_PENDING                 ((NDIS_STATUS)0x00000103L)
#define NDIS_STATUS_NOT_RECOGNIZED          ((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED              ((NDIS_STATUS)0x00010002L)
#define NDIS_STATUS_NOT_ACCEPTED            ((NDIS_STATUS)0x00010003L)
#define NDIS_STATUS_MAY_CLOSE               ((NDIS_STATUS)0x00010004L)

#define NDIS_STATUS_ONLINE                  ((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START             ((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END               ((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS             ((NDIS_STATUS)0x40010006L)
#define NDIS_STATUS_CLOSED                  ((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP             ((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN           ((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT            ((NDIS_STATUS)0x4001000AL)

#define NDIS_STATUS_NOT_RESETTABLE          ((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS             ((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS             ((NDIS_STATUS)0x80010004L)

#define NDIS_STATUS_FAILURE                 ((NDIS_STATUS)0xC0000001L)
#define NDIS_STATUS_RESOURCES               ((NDIS_STATUS)0xC000009AL)

#define NDIS_STATUS_CLOSING                 ((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION             ((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS     ((NDIS_STATUS)0xC0010005L)
#define NDIS_STATUS_ADAPTER_NOT_FOUND       ((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED             ((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED           ((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL          ((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS        ((NDIS_STATUS)0xC001000AL)
#define NDIS_STATUS_MULTICAST_NOT_FOUND     ((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED         ((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS       ((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING      ((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_NOT_SUPPORTED           ((NDIS_STATUS)0xC00000BBL)
#define NDIS_STATUS_INVALID_PACKET          ((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL          ((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY       ((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN        ((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING          ((NDIS_STATUS)0xC0010013L)
#define NDIS_STATUS_INVALID_LENGTH          ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA            ((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT        ((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID             ((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED         ((NDIS_STATUS)0xC0010018L)
#define NDIS_STATUS_UNSUPPORTED_MEDIA       ((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE    ((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND          ((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE      ((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED          ((NDIS_STATUS)0xC001001DL)
#define NDIS_STATUS_RESOURCE_CONFLICT       ((NDIS_STATUS)0xC001001EL)
#define NDIS_STATUS_NO_CABLE                ((NDIS_STATUS)0xC001001FL)

#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR   ((NDIS_STATUS)0xC0011000L)

// For NT Compatability
#define STATUS_SUCCESS NDIS_STATUS_SUCCESS
#define STATUS_UNSUCCESSFUL NDIS_STATUS_FAILURE

//
// Ndis Packet Filter Bits
//

#define NDIS_PACKET_TYPE_DIRECTED           0x0001
#define NDIS_PACKET_TYPE_MULTICAST          0x0002
#define NDIS_PACKET_TYPE_ALL_MULTICAST      0x0004
#define NDIS_PACKET_TYPE_BROADCAST          0x0008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING     0x0010
#define NDIS_PACKET_TYPE_PROMISCUOUS        0x0020
#define NDIS_PACKET_TYPE_SMT                0x0040
#define NDIS_PACKET_TYPE_ALL_LOCAL          0x0080   // NDIS 4.X Only. Win98/NT4
#define NDIS_PACKET_TYPE_MAC_FRAME          0x8000
#define NDIS_PACKET_TYPE_FUNCTIONAL         0x4000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL     0x2000
#define NDIS_PACKET_TYPE_GROUP              0x1000

//
// Ndis Token-Ring Ring Status Codes
//

#define NDIS_RING_SIGNAL_LOSS               0x00008000
#define NDIS_RING_HARD_ERROR                0x00004000
#define NDIS_RING_SOFT_ERROR                0x00002000
#define NDIS_RING_TRANSMIT_BEACON           0x00001000
#define NDIS_RING_LOBE_WIRE_FAULT           0x00000800
#define NDIS_RING_AUTO_REMOVAL_ERROR        0x00000400
#define NDIS_RING_REMOVE_RECEIVED           0x00000200
#define NDIS_RING_COUNTER_OVERFLOW          0x00000100
#define NDIS_RING_SINGLE_STATION            0x00000080
#define NDIS_RING_RING_RECOVERY             0x00000040

//
// Ndis protocol option bits (OID_GEN_PROTOCOL_OPTIONS).
//

#define NDIS_PROT_OPTION_ESTIMATED_LENGTH   0x00000001
#define NDIS_PROT_OPTION_NO_LOOPBACK        0x00000002

//
// Ndis MAC option bits (OID_GEN_MAC_OPTIONS).
//

#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA             0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED              0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND              0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK                     0x00000008
//
// Object Identifiers used by NdisRequest Query/Set Information
//

//
// General Objects
//

#define OID_GEN_SUPPORTED_LIST              0x00010101
#define OID_GEN_HARDWARE_STATUS             0x00010102
#define OID_GEN_MEDIA_SUPPORTED             0x00010103
#define OID_GEN_MEDIA_IN_USE                0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD           0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE          0x00010106
#define OID_GEN_LINK_SPEED                  0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE       0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE        0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE         0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE          0x0001010B
#define OID_GEN_VENDOR_ID                   0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION          0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER       0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD           0x0001010F
#define OID_GEN_DRIVER_VERSION              0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE          0x00010111
#define OID_GEN_PROTOCOL_OPTIONS            0x00010112
#define OID_GEN_MAC_OPTIONS                 0x00010113

#define OID_GEN_XMIT_OK                     0x00020101
#define OID_GEN_RCV_OK                      0x00020102
#define OID_GEN_XMIT_ERROR                  0x00020103
#define OID_GEN_RCV_ERROR                   0x00020104
#define OID_GEN_RCV_NO_BUFFER               0x00020105

#define OID_GEN_DIRECTED_BYTES_XMIT         0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT        0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT        0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT       0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT        0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT       0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV          0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV         0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV         0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV        0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV         0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV        0x0002020C

#define OID_GEN_RCV_CRC_ERROR               0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH       0x0002020E


//
// 802.3 Objects (Ethernet)
//

#define OID_802_3_PERMANENT_ADDRESS         0x01010101
#define OID_802_3_CURRENT_ADDRESS           0x01010102
#define OID_802_3_MULTICAST_LIST            0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE         0x01010104

#define OID_802_3_RCV_ERROR_ALIGNMENT       0x01020101
#define OID_802_3_XMIT_ONE_COLLISION        0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS      0x01020103

#define OID_802_3_XMIT_DEFERRED             0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS       0x01020202
#define OID_802_3_RCV_OVERRUN               0x01020203
#define OID_802_3_XMIT_UNDERRUN             0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE    0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST       0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS      0x01020207


//
// 802.5 Objects (Token-Ring)
//

#define OID_802_5_PERMANENT_ADDRESS         0x02010101
#define OID_802_5_CURRENT_ADDRESS           0x02010102
#define OID_802_5_CURRENT_FUNCTIONAL        0x02010103
#define OID_802_5_CURRENT_GROUP             0x02010104
#define OID_802_5_LAST_OPEN_STATUS          0x02010105
#define OID_802_5_CURRENT_RING_STATUS       0x02010106
#define OID_802_5_CURRENT_RING_STATE        0x02010107

#define OID_802_5_LINE_ERRORS               0x02020101
#define OID_802_5_LOST_FRAMES               0x02020102

#define OID_802_5_BURST_ERRORS              0x02020201
#define OID_802_5_AC_ERRORS                 0x02020202
#define OID_802_5_ABORT_DELIMETERS          0x02020203
#define OID_802_5_FRAME_COPIED_ERRORS       0x02020204
#define OID_802_5_FREQUENCY_ERRORS          0x02020205
#define OID_802_5_TOKEN_ERRORS              0x02020206
#define OID_802_5_INTERNAL_ERRORS           0x02020207


//
// FDDI Objects
//

#define OID_FDDI_LONG_PERMANENT_ADDR        0x03010101
#define OID_FDDI_LONG_CURRENT_ADDR          0x03010102
#define OID_FDDI_LONG_MULTICAST_LIST        0x03010103
#define OID_FDDI_LONG_MAX_LIST_SIZE         0x03010104
#define OID_FDDI_SHORT_PERMANENT_ADDR       0x03010105
#define OID_FDDI_SHORT_CURRENT_ADDR         0x03010106
#define OID_FDDI_SHORT_MULTICAST_LIST       0x03010107
#define OID_FDDI_SHORT_MAX_LIST_SIZE        0x03010108

#define OID_FDDI_ATTACHMENT_TYPE               0x03020101
#define OID_FDDI_UPSTREAM_NODE_LONG            0x03020102
#define OID_FDDI_DOWNSTREAM_NODE_LONG          0x03020103
#define OID_FDDI_FRAME_ERRORS                  0x03020104
#define OID_FDDI_FRAMES_LOST                   0x03020105
#define OID_FDDI_RING_MGT_STATE                0x03020106
#define OID_FDDI_LCT_FAILURES                  0x03020107
#define OID_FDDI_LEM_REJECTS                   0x03020108
#define OID_FDDI_LCONNECTION_STATE             0x03020109

#define OID_FDDI_SMT_STATION_ID                0x03030201
#define OID_FDDI_SMT_OP_VERSION_ID             0x03030202
#define OID_FDDI_SMT_HI_VERSION_ID             0x03030203
#define OID_FDDI_SMT_LO_VERSION_ID             0x03030204
#define OID_FDDI_SMT_MANUFACTURER_DATA         0x03030205
#define OID_FDDI_SMT_USER_DATA                 0x03030206
#define OID_FDDI_SMT_MIB_VERSION_ID            0x03030207
#define OID_FDDI_SMT_MAC_CT                    0x03030208
#define OID_FDDI_SMT_NON_MASTER_CT             0x03030209
#define OID_FDDI_SMT_MASTER_CT                 0x0303020A
#define OID_FDDI_SMT_AVAILABLE_PATHS           0x0303020B
#define OID_FDDI_SMT_CONFIG_CAPABILITIES       0x0303020C
#define OID_FDDI_SMT_CONFIG_POLICY             0x0303020D
#define OID_FDDI_SMT_CONNECTION_POLICY         0x0303020E
#define OID_FDDI_SMT_T_NOTIFY                  0x0303020F
#define OID_FDDI_SMT_STAT_RPT_POLICY           0x03030210
#define OID_FDDI_SMT_TRACE_MAX_EXPIRATION      0x03030211
#define OID_FDDI_SMT_PORT_INDEXES              0x03030212
#define OID_FDDI_SMT_MAC_INDEXES               0x03030213
#define OID_FDDI_SMT_BYPASS_PRESENT            0x03030214
#define OID_FDDI_SMT_ECM_STATE                 0x03030215
#define OID_FDDI_SMT_CF_STATE                  0x03030216
#define OID_FDDI_SMT_HOLD_STATE                0x03030217
#define OID_FDDI_SMT_REMOTE_DISCONNECT_FLAG    0x03030218
#define OID_FDDI_SMT_STATION_STATUS            0x03030219
#define OID_FDDI_SMT_PEER_WRAP_FLAG            0x0303021A
#define OID_FDDI_SMT_MSG_TIME_STAMP            0x0303021B
#define OID_FDDI_SMT_TRANSITION_TIME_STAMP     0x0303021C
#define OID_FDDI_SMT_SET_COUNT                 0x0303021D
#define OID_FDDI_SMT_LAST_SET_STATION_ID       0x0303021E
#define OID_FDDI_MAC_FRAME_STATUS_FUNCTIONS    0x0303021F
#define OID_FDDI_MAC_BRIDGE_FUNCTIONS          0x03030220
#define OID_FDDI_MAC_T_MAX_CAPABILITY          0x03030221
#define OID_FDDI_MAC_TVX_CAPABILITY            0x03030222
#define OID_FDDI_MAC_AVAILABLE_PATHS           0x03030223
#define OID_FDDI_MAC_CURRENT_PATH              0x03030224
#define OID_FDDI_MAC_UPSTREAM_NBR              0x03030225
#define OID_FDDI_MAC_DOWNSTREAM_NBR            0x03030226
#define OID_FDDI_MAC_OLD_UPSTREAM_NBR          0x03030227
#define OID_FDDI_MAC_OLD_DOWNSTREAM_NBR        0x03030228
#define OID_FDDI_MAC_DUP_ADDRESS_TEST          0x03030229
#define OID_FDDI_MAC_REQUESTED_PATHS           0x0303022A
#define OID_FDDI_MAC_DOWNSTREAM_PORT_TYPE      0x0303022B
#define OID_FDDI_MAC_INDEX                     0x0303022C
#define OID_FDDI_MAC_SMT_ADDRESS               0x0303022D
#define OID_FDDI_MAC_LONG_GRP_ADDRESS          0x0303022E
#define OID_FDDI_MAC_SHORT_GRP_ADDRESS         0x0303022F
#define OID_FDDI_MAC_T_REQ                     0x03030230
#define OID_FDDI_MAC_T_NEG                     0x03030231
#define OID_FDDI_MAC_T_MAX                     0x03030232
#define OID_FDDI_MAC_TVX_VALUE                 0x03030233
#define OID_FDDI_MAC_T_PRI0                    0x03030234
#define OID_FDDI_MAC_T_PRI1                    0x03030235
#define OID_FDDI_MAC_T_PRI2                    0x03030236
#define OID_FDDI_MAC_T_PRI3                    0x03030237
#define OID_FDDI_MAC_T_PRI4                    0x03030238
#define OID_FDDI_MAC_T_PRI5                    0x03030239
#define OID_FDDI_MAC_T_PRI6                    0x0303023A
#define OID_FDDI_MAC_FRAME_CT                  0x0303023B
#define OID_FDDI_MAC_COPIED_CT                 0x0303023C
#define OID_FDDI_MAC_TRANSMIT_CT               0x0303023D
#define OID_FDDI_MAC_TOKEN_CT                  0x0303023E
#define OID_FDDI_MAC_ERROR_CT                  0x0303023F
#define OID_FDDI_MAC_LOST_CT                   0x03030240
#define OID_FDDI_MAC_TVX_EXPIRED_CT            0x03030241
#define OID_FDDI_MAC_NOT_COPIED_CT             0x03030242
#define OID_FDDI_MAC_LATE_CT                   0x03030243
#define OID_FDDI_MAC_RING_OP_CT                0x03030244
#define OID_FDDI_MAC_FRAME_ERROR_THRESHOLD     0x03030245
#define OID_FDDI_MAC_FRAME_ERROR_RATIO         0x03030246
#define OID_FDDI_MAC_NOT_COPIED_THRESHOLD      0x03030247
#define OID_FDDI_MAC_NOT_COPIED_RATIO          0x03030248
#define OID_FDDI_MAC_RMT_STATE                 0x03030249
#define OID_FDDI_MAC_DA_FLAG                   0x0303024A
#define OID_FDDI_MAC_UNDA_FLAG                 0x0303024B
#define OID_FDDI_MAC_FRAME_ERROR_FLAG          0x0303024C
#define OID_FDDI_MAC_NOT_COPIED_FLAG           0x0303024D
#define OID_FDDI_MAC_MA_UNITDATA_AVAILABLE     0x0303024E
#define OID_FDDI_MAC_HARDWARE_PRESENT          0x0303024F
#define OID_FDDI_MAC_MA_UNITDATA_ENABLE        0x03030250
#define OID_FDDI_PATH_INDEX                    0x03030251
#define OID_FDDI_PATH_RING_LATENCY             0x03030252
#define OID_FDDI_PATH_TRACE_STATUS             0x03030253
#define OID_FDDI_PATH_SBA_PAYLOAD              0x03030254
#define OID_FDDI_PATH_SBA_OVERHEAD             0x03030255
#define OID_FDDI_PATH_CONFIGURATION            0x03030256
#define OID_FDDI_PATH_T_R_MODE                 0x03030257
#define OID_FDDI_PATH_SBA_AVAILABLE            0x03030258
#define OID_FDDI_PATH_TVX_LOWER_BOUND          0x03030259
#define OID_FDDI_PATH_T_MAX_LOWER_BOUND        0x0303025A
#define OID_FDDI_PATH_MAX_T_REQ                0x0303025B
#define OID_FDDI_PORT_MY_TYPE                  0x0303025C
#define OID_FDDI_PORT_NEIGHBOR_TYPE            0x0303025D
#define OID_FDDI_PORT_CONNECTION_POLICIES      0x0303025E
#define OID_FDDI_PORT_MAC_INDICATED            0x0303025F
#define OID_FDDI_PORT_CURRENT_PATH             0x03030260
#define OID_FDDI_PORT_REQUESTED_PATHS          0x03030261
#define OID_FDDI_PORT_MAC_PLACEMENT            0x03030262
#define OID_FDDI_PORT_AVAILABLE_PATHS          0x03030263
#define OID_FDDI_PORT_MAC_LOOP_TIME            0x03030264
#define OID_FDDI_PORT_PMD_CLASS                0x03030265
#define OID_FDDI_PORT_CONNECTION_CAPABILITIES   0x03030266
#define OID_FDDI_PORT_INDEX                    0x03030267
#define OID_FDDI_PORT_MAINT_LS                 0x03030268
#define OID_FDDI_PORT_BS_FLAG                  0x03030269
#define OID_FDDI_PORT_PC_LS                    0x0303026A
#define OID_FDDI_PORT_EB_ERROR_CT              0x0303026B
#define OID_FDDI_PORT_LCT_FAIL_CT              0x0303026C
#define OID_FDDI_PORT_LER_ESTIMATE             0x0303026D
#define OID_FDDI_PORT_LEM_REJECT_CT            0x0303026E
#define OID_FDDI_PORT_LEM_CT                   0x0303026F
#define OID_FDDI_PORT_LER_CUTOFF               0x03030270
#define OID_FDDI_PORT_LER_ALARM                0x03030271
#define OID_FDDI_PORT_CONNNECT_STATE           0x03030272
#define OID_FDDI_PORT_PCM_STATE                0x03030273
#define OID_FDDI_PORT_PC_WITHHOLD              0x03030274
#define OID_FDDI_PORT_LER_FLAG                 0x03030275
#define OID_FDDI_PORT_HARDWARE_PRESENT         0x03030276
#define OID_FDDI_SMT_STATION_ACTION            0x03030277
#define OID_FDDI_PORT_ACTION                   0x03030278
#define OID_FDDI_IF_DESCR                      0x03030279
#define OID_FDDI_IF_TYPE                       0x0303027A
#define OID_FDDI_IF_MTU                        0x0303027B
#define OID_FDDI_IF_SPEED                      0x0303027C
#define OID_FDDI_IF_PHYS_ADDRESS               0x0303027D
#define OID_FDDI_IF_ADMIN_STATUS               0x0303027E
#define OID_FDDI_IF_OPER_STATUS                0x0303027F
#define OID_FDDI_IF_LAST_CHANGE                0x03030280
#define OID_FDDI_IF_IN_OCTETS                  0x03030281
#define OID_FDDI_IF_IN_UCAST_PKTS              0x03030282
#define OID_FDDI_IF_IN_NUCAST_PKTS             0x03030283
#define OID_FDDI_IF_IN_DISCARDS                0x03030284
#define OID_FDDI_IF_IN_ERRORS                  0x03030285
#define OID_FDDI_IF_IN_UNKNOWN_PROTOS          0x03030286
#define OID_FDDI_IF_OUT_OCTETS                 0x03030287
#define OID_FDDI_IF_OUT_UCAST_PKTS             0x03030288
#define OID_FDDI_IF_OUT_NUCAST_PKTS            0x03030289
#define OID_FDDI_IF_OUT_DISCARDS               0x0303028A
#define OID_FDDI_IF_OUT_ERRORS                 0x0303028B
#define OID_FDDI_IF_OUT_QLEN                   0x0303028C
#define OID_FDDI_IF_SPECIFIC                   0x0303028D


//
// WAN objects
//

#define OID_WAN_PERMANENT_ADDRESS           0x04010101
#define OID_WAN_CURRENT_ADDRESS             0x04010102
#define OID_WAN_QUALITY_OF_SERVICE          0x04010103
#define OID_WAN_PROTOCOL_TYPE               0x04010104
#define OID_WAN_MEDIUM_SUBTYPE              0x04010105
#define OID_WAN_HEADER_FORMAT               0x04010106

#define OID_WAN_GET_INFO                       0x04010107
#define OID_WAN_SET_LINK_INFO                  0x04010108
#define OID_WAN_GET_LINK_INFO                  0x04010109

#define OID_WAN_LINE_COUNT                     0x0401010A

#define OID_WAN_GET_BRIDGE_INFO                0x0401020A
#define OID_WAN_SET_BRIDGE_INFO                0x0401020B
#define OID_WAN_GET_COMP_INFO                  0x0401020C
#define OID_WAN_SET_COMP_INFO                  0x0401020D
#define OID_WAN_GET_STATS_INFO                 0x0401020E


//
// LocalTalk objects
//

#define OID_LTALK_CURRENT_NODE_ID           0x05010102

#define OID_LTALK_IN_BROADCASTS             0x05020101
#define OID_LTALK_IN_LENGTH_ERRORS          0x05020102

#define OID_LTALK_OUT_NO_HANDLERS           0x05020201
#define OID_LTALK_COLLISIONS                0x05020202
#define OID_LTALK_DEFERS                    0x05020203
#define OID_LTALK_NO_DATA_ERRORS            0x05020204
#define OID_LTALK_RANDOM_CTS_ERRORS         0x05020205
#define OID_LTALK_FCS_ERRORS                0x05020206


//
// Arcnet objects
//
#define OID_ARCNET_PERMANENT_ADDRESS        0x06010101
#define OID_ARCNET_CURRENT_ADDRESS          0x06010102
#define OID_ARCNET_RECONFIGURATIONS         0x06020201



//
// ATM Connection Oriented Ndis
//
#define OID_ATM_SUPPORTED_VC_RATES              0x08010101
#define OID_ATM_SUPPORTED_QOS                   0x08010102
#define OID_ATM_SUPPORTED_AAL_TYPES             0x08010103
#define OID_ATM_GET_TIME_CAPS                   0x08010104
#define OID_ATM_HW_CURRENT_ADDRESS              0x08010105
#define OID_ATM_GET_NETCARD_TIME                0x08010106
#define OID_ATM_GET_HARDWARE_LINE_STATE         0x08010108
#define OID_ATM_ALIGNMENT_REQUIRED              0x08010109
#define OID_ATM_SIGNALING_VPIVCI                0x0801010A
#define OID_ATM_ASSIGNED_VPI                    0x0801010B
#define OID_ATM_MAX_ACTIVE_VCS                  0x0801010C
#define OID_ATM_MAX_ACTIVE_VCI_BITS             0x0801010D
#define OID_ATM_MAX_ACTIVE_VPI_BITS             0x0801010E
#define OID_ATM_ACQUIRE_ACCESS_NET_RESOURCES    0x0801010F
#define OID_ATM_RELEASE_ACCESS_NET_RESOURCES    0x08010110
#define OID_ATM_ILMI_VPIVCI                     0x08010111
#define OID_ATM_DIGITAL_BROADCAST_VPIVCI        0x08010112


#define OID_ATM_GET_RESERVED_VC_LIST            0x08020101
#define OID_ATM_GET_SPECIFIC_VC_STATS           0x08020102
#define OID_ATM_GET_GENERAL_VC_STATS            0x08020103
#define OID_ATM_NETCARD_LOAD                    0x08020104
#define OID_ATM_SET_REQUIRED_STATISTICS         0x08020105

//
// All WirelessWAN devices must support the following OIDs
//

#define OID_WW_GEN_NETWORK_TYPES_SUPPORTED      0x09010101
#define OID_WW_GEN_NETWORK_TYPE_IN_USE          0x09010102
#define OID_WW_GEN_HEADER_FORMATS_SUPPORTED     0x09010103
#define OID_WW_GEN_HEADER_FORMAT_IN_USE         0x09010104
#define OID_WW_GEN_INDICATION_REQUEST           0x09010105
#define OID_WW_GEN_DEVICE_INFO                  0x09010106
#define OID_WW_GEN_OPERATION_MODE               0x09010107
#define OID_WW_GEN_LOCK_STATUS                  0x09010108
#define OID_WW_GEN_DISABLE_TRANSMITTER          0x09010109
#define OID_WW_GEN_NETWORK_ID                   0x0901010A
#define OID_WW_GEN_PERMANENT_ADDRESS            0x0901010B
#define OID_WW_GEN_CURRENT_ADDRESS              0x0901010C
#define OID_WW_GEN_SUSPEND_DRIVER               0x0901010D
#define OID_WW_GEN_BASESTATION_ID               0x0901010E
#define OID_WW_GEN_CHANNEL_ID                   0x0901010F
#define OID_WW_GEN_ENCRYPTION_SUPPORTED         0x09010110
#define OID_WW_GEN_ENCRYPTION_IN_USE            0x09010111
#define OID_WW_GEN_ENCRYPTION_STATE             0x09010112
#define OID_WW_GEN_CHANNEL_QUALITY              0x09010113
#define OID_WW_GEN_REGISTRATION_STATUS          0x09010114
#define OID_WW_GEN_RADIO_LINK_SPEED             0x09010115
#define OID_WW_GEN_LATENCY                      0x09010116
#define OID_WW_GEN_BATTERY_LEVEL                0x09010117
#define OID_WW_GEN_EXTERNAL_POWER               0x09010118

//
// Network Dependent OIDs - Mobitex:
//

#define OID_WW_MBX_SUBADDR                      0x09050101
#define OID_WW_MBX_FLEXLIST                     0x09050103
#define OID_WW_MBX_GROUPLIST                    0x09050104
#define OID_WW_MBX_TRAFFIC_AREA                 0x09050105
#define OID_WW_MBX_LIVE_DIE                     0x09050106
#define OID_WW_MBX_TEMP_DEFAULTLIST             0x09050107

//
// Network Dependent OIDs - Pinpoint:
//

#define OID_WW_PIN_LOC_AUTHORIZE                0x09090101
#define OID_WW_PIN_LAST_LOCATION                0x09090102
#define OID_WW_PIN_LOC_FIX                      0x09090103

//
// Network Dependent - CDPD:
//

#define OID_WW_CDPD_SPNI                        0x090D0101
#define OID_WW_CDPD_WASI                        0x090D0102
#define OID_WW_CDPD_AREA_COLOR                  0x090D0103
#define OID_WW_CDPD_TX_POWER_LEVEL              0x090D0104
#define OID_WW_CDPD_EID                         0x090D0105
#define OID_WW_CDPD_HEADER_COMPRESSION          0x090D0106
#define OID_WW_CDPD_DATA_COMPRESSION            0x090D0107
#define OID_WW_CDPD_CHANNEL_SELECT              0x090D0108
#define OID_WW_CDPD_CHANNEL_STATE               0x090D0109
#define OID_WW_CDPD_NEI                         0x090D010A
#define OID_WW_CDPD_NEI_STATE                   0x090D010B
#define OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER 0x090D010C
#define OID_WW_CDPD_SLEEP_MODE                  0x090D010D
#define OID_WW_CDPD_CIRCUIT_SWITCHED            0x090D010E
#define   OID_WW_CDPD_TEI                       0x090D010F
#define   OID_WW_CDPD_RSSI                      0x090D0110

//
// Network Dependent - Ardis:
//

#define OID_WW_ARD_SNDCP                        0x09110101
#define OID_WW_ARD_TMLY_MSG                     0x09110102
#define OID_WW_ARD_DATAGRAM                     0x09110103

//
// Network Dependent - DataTac:
//

#define OID_WW_TAC_COMPRESSION                  0x09150101
#define OID_WW_TAC_SET_CONFIG                   0x09150102
#define OID_WW_TAC_GET_STATUS                   0x09150103
#define OID_WW_TAC_USER_HEADER                  0x09150104

//
// Network Dependent - Metricom:
//

#define OID_WW_MET_FUNCTION                     0x09190101

//
// IRDA objects
//
#define OID_IRDA_RECEIVING                      0x0A010100
#define OID_IRDA_TURNAROUND_TIME                0x0A010101
#define OID_IRDA_SUPPORTED_SPEEDS               0x0A010102
#define OID_IRDA_LINK_SPEED                     0x0A010103
#define OID_IRDA_MEDIA_BUSY                     0x0A010104

#define OID_IRDA_EXTRA_RCV_BOFS                 0x0A010200
#define OID_IRDA_RATE_SNIFF                     0x0A010201
#define OID_IRDA_UNICAST_LIST                   0x0A010202
#define OID_IRDA_MAX_UNICAST_LIST_SIZE          0x0A010203

//
// Chicago Implementation Specific objects
//

#define OID_PRIVATE_PROTOCOL_HANDLE             0xff010207

/////////////////////////////////////////////////////////////////////////////
//// REVERT TO DEFAULT STRUCTURE PACKING

#pragma pack(pop)

#endif  // _W32NDIS_H

#endif // _NDIS_

