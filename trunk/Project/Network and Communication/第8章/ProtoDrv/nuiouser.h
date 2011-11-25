///////////////////////////////////////////////
// nuiouser.h�ļ�
// ����ProtoDrv������Ҫ�ĳ��������ͣ��û�����������




#ifndef __NUIOUSER_H__
#define __NUIOUSER_H__


#define  MAX_LINK_NAME_LENGTH   124

// ���úͻ�ȡ������OID��Ϣ����Ľṹ
typedef struct _PROTOCOL_OID_DATA	
{

    ULONG           Oid;
    ULONG           Length;
    UCHAR           Data[1];

} PROTOCOL_OID_DATA, *PPROTOCOL_OID_DATA;


#define FILE_DEVICE_PROTOCOL      0x8000

// 4��IOCTL�Ĺ��ֱܷ��ǣ�������������OID��Ϣ����ȡ��������OID��Ϣ��������������ö�ٰ󶨵�������
#define IOCTL_PROTOCOL_SET_OID      CTL_CODE(FILE_DEVICE_PROTOCOL, \
										0 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_QUERY_OID    CTL_CODE(FILE_DEVICE_PROTOCOL, \
										1 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_RESET        CTL_CODE(FILE_DEVICE_PROTOCOL, \
										2 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENUM_ADAPTERS         CTL_CODE(FILE_DEVICE_PROTOCOL, \
										3 , METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // __NUIOUSER_H__