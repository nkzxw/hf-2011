///
/// @file         tdifw_smpl.c
/// @author    crazy_chu
/// @date       2009-4-11
/// @brief      ʹ��tdi_fw.lib��һ�����ӡ� 
/// 
/// ��������
/// ������Ϊʾ�����롣δ���꾡���ԣ�����֤�ɿ��ԡ����߶�
/// �κ���ʹ�ô˴��뵼�µ�ֱ�Ӻͼ����ʧ�������Ρ�
/// 

#include "..\tdi_fw\tdi_fw_lib.h"

NTSTATUS
tdifw_driver_entry(
			IN PDRIVER_OBJECT theDriverObject,
            IN PUNICODE_STRING theRegistryPath)
{
    // ֱ�ӷ��سɹ����ɡ�
    return STATUS_SUCCESS;
}

VOID
tdifw_driver_unload(
			IN PDRIVER_OBJECT DriverObject)
{
    // û����Դ��Ҫ�ͷš�
    return;
}

NTSTATUS tdifw_user_device_dispatch(
	IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
    // �������κ����󵽴��������û��ע����Զ����豸��
    return STATUS_UNSUCCESSFUL;
}

u_short
tdifw_ntohs (u_short netshort)
{
	u_short result = 0;
	((char *)&result)[0] = ((char *)&netshort)[1];
	((char *)&result)[1] = ((char *)&netshort)[0];
	return result;
}


int tdifw_filter(struct flt_request *request)
{
    if(request->proto == IPPROTO_TCP)
    {
        struct sockaddr_in* from = (struct sockaddr_in*)&request->addr.from;
        struct sockaddr_in* to = (struct sockaddr_in*)&request->addr.to;

        // Ȼ���ӡЭ������
        DbgPrint("tdifw_smpl: protocol type = TCP\r\n");
        // ��ӡ��ǰ���̵�PID��
        DbgPrint("tdifw_smpl: currect process = %d\r\n",request->pid);

        // ��ӡ�¼�����
        switch(request->type)
        {
        case TYPE_CONNECT:
            DbgPrint("tdifw_smpl: event: CONNECT\r\n");
            break;
        case TYPE_DATAGRAM:
            DbgPrint("tdifw_smpl: event: DATAGRAM\r\n");
            break;
        case TYPE_CONNECT_ERROR:
            DbgPrint("tdifw_smpl: event: CONNECT ERROR\r\n");
            break;
        case TYPE_LISTEN:
            DbgPrint("tdifw_smpl: event: LISTEN\r\n");
            break;
        case TYPE_NOT_LISTEN:
            DbgPrint("tdifw_smpl: event: NOT LISTEN\r\n");
            break;
        case TYPE_CONNECT_CANCELED:
            DbgPrint("tdifw_smpl: event: CONNECT CANCELED\r\n");
            break;
        case TYPE_CONNECT_RESET:
            DbgPrint("tdifw_smpl: event: CONNECT RESET\r\n");
            break;
        case TYPE_CONNECT_TIMEOUT:
            DbgPrint("tdifw_smpl: event: CONNECT TIMEOUT\r\n");
            break;
        case TYPE_CONNECT_UNREACH:
            DbgPrint("tdifw_smpl: event: CONNECT UNREACH\r\n");
            break;
        default:
            break;
        }
  

        // �����TCP�����Ǵ�ӡ��������ݡ�����������ԴIP��ַ
        // Ŀ��IP��ַ���ȵȡ����Ƕ�������Э��Ͳ���ӡ�ˡ�
        DbgPrint("tdifw_smpl: direction = %d\r\n",request->direction);
        DbgPrint("tdifw_smpl: src port = %d\r\n",tdifw_ntohs(from->sin_port));
        DbgPrint("tdifw_smpl: src ip = %d.%d.%d.%d\r\n",
            from->sin_addr.S_un.S_un_b.s_b1,
            from->sin_addr.S_un.S_un_b.s_b2,
            from->sin_addr.S_un.S_un_b.s_b3,
            from->sin_addr.S_un.S_un_b.s_b4);
        DbgPrint("tdifw_smpl: dst port = %d\r\n",tdifw_ntohs(to->sin_port));
        DbgPrint("tdifw_smpl: dst ip = %d.%d.%d.%d\r\n",
            to->sin_addr.S_un.S_un_b.s_b1,
            to->sin_addr.S_un.S_un_b.s_b2,
            to->sin_addr.S_un.S_un_b.s_b3,
            to->sin_addr.S_un.S_un_b.s_b4);
    }

    return FILTER_ALLOW;
}
