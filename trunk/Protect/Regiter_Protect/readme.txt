��������: 
  1. ��MBR,����fat32 / ntfs .��������ȥ��BPB,�õ� ÿ��������, �����������ܴ�������Ϣ,������ȫ�ֱ���; ( sysnap����˻������Ĵ���(AtapiReadWriteDisk) ) 
  2. ��ע����ȡsystem / currentuser / software ��3��Hive��Ӧ���ļ�·��,������ȫ�ֱ����� 
  3. (��3��) ��ȡ����Hive�ļ���Ӧ��deviceObject,���IRP, ������XX, �õ����ļ���Ӧ�����д�,ת��������������,���浽ȫ��������. 
  4. IAT hook atapi.sys �ķַ�����IoStartPacket; �滻��disk.sys��IRP_MJ_WRITE �ַ�����. ���й��˴��� 
  
fake��������: 
  1. fake_diskDispatch_writefunc: 
    ����IRP���е�irpStack->Parameters.Write.ByteOffset, ����ǰ��������ϵ�ƫ��������Ҫ������hive�ļ��ڵ�,IofCompleteRequest�ܾ�֮. 
    
  2. fake_atapi_IoStartPacket_writefunc: 
    ����IRP���е�srb.QueueSortKey,����д����(SRB_FLAGS_DATA_OUT),�ҵ�ǰ��������ϵ�ƫ��������Ҫ������hive�ļ��ڵ�,���srb�е�CDB��(Command Descriptor Block). 
    �ض���Ϊ�����̵ĵ�3��������д����. 
    
Over...    
  ������˵,�е������С��~ 