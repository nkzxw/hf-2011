基本流程: 
  1. 读MBR,区别处fat32 / ntfs .到主分区去读BPB,得到 每簇扇区数, 该主分区的总簇数等信息,保存于全局变量; ( sysnap借鉴了机器狗的代码(AtapiReadWriteDisk) ) 
  2. 读注册表获取system / currentuser / software 这3个Hive对应的文件路径,保存于全局变量中 
  3. (分3次) 获取以上Hive文件对应的deviceObject,填充IRP, 控制码XX, 得到该文件对应的所有簇,转换成物理扇区号,保存到全局数组中. 
  4. IAT hook atapi.sys 的分发函数IoStartPacket; 替换了disk.sys的IRP_MJ_WRITE 分发历程. 进行过滤处理 
  
fake函数流程: 
  1. fake_diskDispatch_writefunc: 
    分析IRP包中的irpStack->Parameters.Write.ByteOffset, 若当前物理磁盘上的偏移在以上要保护的hive文件内的,IofCompleteRequest拒绝之. 
    
  2. fake_atapi_IoStartPacket_writefunc: 
    分析IRP包中的srb.QueueSortKey,若是写请求(SRB_FLAGS_DATA_OUT),且当前物理磁盘上的偏移在以上要保护的hive文件内的,填充srb中的CDB块(Command Descriptor Block). 
    重定向为往磁盘的第3个扇区内写数据. 
    
Over...    
  总体来说,有点儿大题小做~ 