1> IFS   ����ͼ   
	a.����һ�������豸.��Ȼ��ǰ��������������ָ������.   
	b.����Dispatch   Functions.     
	c.����Fast   Io   Functions.     
	d.��дһ��my_fs_notify�ص������������а󶨸ռ����FS   CDO.     
	e.ʹ��wdff_reg_notify����ע������ص�������   
	f.��дĬ�ϵ�dispatch   functions.     
	g.����IRP_MJ_FILE_SYSTEM_CONTROL,�����м��Volumne��Mount��Dismount.     
	h.��һ����Ȼ�ǰ�Volumne��.   
	(ȫ·������   FileObject->FileName.Buffer�еõ���.)   

2> һЩ��Ҫ֪ʶ   
	a.�������������

	1.������storage drivers��PNP�����,����һ���豸�ڵ㣨DEVNODE����
		It is important to note that file systems and file system filter drivers are not PnP device drivers,
		ÿ���豸�ڵ���ά��һ��Storage Device Stacks��
		���������Ϊÿ���洢�豸,��������豸,���ܰ���һ�����߶���߼���(�������߶�̬��),��Щ�����ͨ�����Storage Device Stack������ġ�
		���豸�����Ϣ����functional device object (FDO)��
		ʣ�µľ���physical device objects (PDO)�������������   

	2.ͨ������ķ�ʽ���Եõ��������   
		The Mount Manager responds to the arrival of a new storage volume by querying the volume driver for the following information:     
		��The volume's nonpersistent device object name (or target name), located in the Device directory of the system object tree (for example: "\Device\HarddiskVolume1")     
		��The volume's globally unique identifier (GUID), also called the unique volume name     
		��A suggested persistent symbolic link name for the volume, such as a drive letter (for example, "\DosDevices\D:")     

	3.�ļ�ϵͳ�;������   
		When a file system is mounted on a storage volume,it creates a file system volume device object (VDO) to represent the volume to the file system.   
		The file system VDO is mounted on the storage device object by means of a shared object called a volume parameterblock (VPB).   
		File System Stacks: File system drivers create two different types of device objects: control device objects (CDO) and volume device objects(VDO).   
		File System Control Device Objects(CDO)
		File System Volume Device Objects(VDO)

3>�������(��ϸ�ο�Sfilter)   
	1��
	DriverEntry   ()   
	{   
		status = IoRegisterFsRegistrationChange(DriverObject,SfFsNotification);   
	}

	SfFsNotification(IN PDEVICE_OBJECT DeviceObject, //ԭʼ�ļ�ϵͳ   
					IN BOOLEAN  FsActive)   
	{     
		SfAttachToFileSystemDevice(DeviceObject, &name);   
	}   
	SfAttachToFileSystemDevice(               
				IN   PDEVICE_OBJECT   DeviceObject,//ԭʼ�ļ�ϵͳ   
				IN   PUNICODE_STRING   DeviceName)   
	{   
		status = IoCreateDevice(...   
						&newDeviceObject   );   
						status   =   SfAttachDeviceToDeviceStack(   newDeviceObject,     
						DeviceObject,     
						&devExt->AttachedToDeviceObject);//ԭʼ�ı���
	}   

	2 �ļ����ͷ�Ϊ��     
		(((_type)   ==   FILE_DEVICE_DISK_FILE_SYSTEM)   ||   \   �����ļ�ϵͳ   
		((_type)   ==   FILE_DEVICE_CD_ROM_FILE_SYSTEM)   ||   \CDROM�ļ�ϵͳ   
		((_type)   ==   FILE_DEVICE_NETWORK_FILE_SYSTEM   �����ļ�ϵͳ   
  
	3�����������е�����   
		status = IoCreateDevice(DriverObject,   
                          0,                               //has   no   device   extension   
                          &nameString,   
                          FILE_DEVICE_DISK_FILE_SYSTEM,   
                          FILE_DEVICE_SECURE_OPEN,   
                          FALSE,   
                          &gSFilterControlDeviceObject   );   
		��ô������豸�����IRP�����ڸ�����(DriverObject)�н��յ�������������豸�����ʱ�򣬿���ͨ���ж��豸�������������ĸ������ġ�   
		ͬʱ���轨��������һ���豸�������ϵͳ���豸ATTACH�ˣ���ô����ԭ��ϵͳ�豸�����IRPҲ��������������н��յ�������������ķַ������о��ܵõ���ЩIRP��   
		for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++){
			DriverObject->MajorFunction   =   SfPassThrough;   
			DriverObject->MajorFunction[IRP_MJ_CREATE]   =   SfCreate;   
		}

	4�������ļ�ϵͳ����������������һ�������豸��CDO����������ƶ����������ⲿ��Ӧ�ý���ͨѶ�������������ġ�   
	   ��һ���豸�Ǳ�����ļ�ϵͳMount��Volume��
	   һ��FS�����ж��Volume,Ҳ����һ����û�У�����һ�£��������C:,D:,E:,F:�ĸ�������C:,D:ΪNTFS,E:,F:ΪFat32.��ôC:,D:����Fat������Volume�豸����.)�ļ�ϵͳ���������ÿ��Volume������һ��DeviceObjec��IoCreateDevice����   
  
	5����ΪIRP���ϲ�Ӧ�õ������ں˲�ģ�����ֻҪ���ǽ��������豸ATTACH��ԭʼ���豸�ϣ���λ�ڰ󶨵��ļ�����STACK���ϱߣ�������������IRP�������ȱ������½������豸���󲶻񡣵��Ƕ�������ԭʼ�豸�Լ�����������ֻ��ʹ������CALLBACK�ķ����ˡ�   

	6���������������Ҫ�󶨵��ļ�ϵͳ�������ϱߣ��ļ�ϵͳ���˴���������IRP֮�⣬��Ҫ������ν��FastIo.FastIo��Cache   Manager������������һ��û��irp��������ΪFAST   IO������CACHE�ģ����������BASE   FILE   SYSTEMֱ�Ӵ򽻵������Ƕ����ϲ���˵Ҳ���ļ�ϵͳ�ķ��ʣ�����ҲҪ���á���������ȷ���һ���ռ䣬   
		PFAST_IO_DISPATCH   fastIoDispatch   =   ExAllocatePoolWithTag()   
		��FAST   IO�ĺ�����������ṹ��fastIoDispatch->FastIoRead   =   SfFastIoRead;   
		Ȼ��DriverObject->FastIoDispatch   =   fastIoDispatch;   

	7��irp�Ǵ��豸ջ�Ķ��˿�ʼ�������·��͡�DevVolumue��ʾ����ʵ��Ҫ���˵�Volume�豸������ֻҪ������豸ջ�Ķ����ٰ�һ���豸���Ƿ��͸�Volume��������Ȼ���ȷ������ǵ��豸������IoAttachDeviceToDeviceStack��ע��Դ�豸δ��ֱ�Ӱ���Ŀ���豸�ϡ���Ӧ����Ŀ���豸���豸ջ�Ķ��ˡ������硰C:������豸�����Ѿ�֪����������Ϊ��C:��,���ѵõ��豸�����õ��豸�����ֲ��ѵõ��豸����ʱ������IoCreateDevice()����һ��Device   Object,Ȼ�����IoAttachDeviceToDeviceStack�󶨣����з�����C:����irp,�ͱ�Ȼ�ȷ��͸����ǵ�����������Ҳ���Բ������ж��ļ��Ĳ�����!   
	
	8�����ϵķ����Ǿ�̬�ģ���������봦��̬��Volume,����ȫ�������������������������и��ߵ�Ҫ�󡣵����һ��U�̲���usb�ڣ�һ����J:��֮���Volume��̬������ʱ��������ȻҪ��������¼���������һ��Device��������   
	һ���µĴ洢ý�ʱ�ϵͳ���ֲ����ļ�ϵͳ������һ��Volume�Ĺ��̳�ΪMounting.����̿�ʼ��ʱ��FS��CDO���õ�һ��IRP,��Major   Function   CodeΪIRP_MJ_FILE_SYSTEM_CONTROL��Minor   Function   CodeΪIRP_MN_MOUNT�����仰˵����������Ѿ�������һ���豸���ļ�ϵͳ��CDO����ô���ǾͿ��Եõ�������IRP,������֪��һ���µ�Volume����Mount.��ʱ�����ǿ���ִ���ϱ���˵�Ĳ�����   ������������ĺ��壺   
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]   =   SfFsControl;   
	NTSTATUS   
	SfFsControl (IN   PDEVICE_OBJECT   DeviceObject,   
				 IN  PIRP   Irp)   
	{   
		switch   (irpSp->MinorFunction)   {   
		case   IRP_MN_MOUNT_VOLUME:   
			  return   SfFsControlMountVolume(   DeviceObject,   Irp   );   
		case   IRP_MN_LOAD_FILE_SYSTEM:   
			  return   SfFsControlLoadFileSystem(   DeviceObject,   Irp   );   
		case   IRP_MN_USER_FS_REQUEST:   
	}

	//����USB�̵ȷ������£�   
	NTSTATUS   
	SfFsControlMountVolume   (   
		  IN   PDEVICE_OBJECT   DeviceObject,   
		  IN   PIRP   Irp)   
	{   
		//����Ҫ����because this VPB may be changed by the underlying file system.   
		storageStackDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;   

		//�жϲ���Ӱ���豸   
		status = SfIsShadowCopyVolume   (storageStackDeviceObject,&isShadowCopyVolume);   

		//�����µ�FILTER�豸����׼�����ӵ�MOUNT���豸��   
		//   Since   the   device   object   we   are   going   to   attach   to   has   not   yet   been   
		//   created   (it   is   created   by   the   base   file   system)   we   are   going   to   use   
		//   the   type   of   the   file   system   control   device   object.   We   are   assuming   
		//   that   the   file   system   control   device   object   will   have   the   same   type   
		//   as   the   volume   device   objects   associated   with   it.   
		status = IoCreateDevice(gSFilterDriverObject,   
								  sizeof(   SFILTER_DEVICE_EXTENSION   ),   
								  NULL,   
								  DeviceObject->DeviceType,   
								  0,   
								  FALSE,   
								  &newDeviceObject   );   
	   #if WINVER   >=   0x0501   
          //�����¼�   
          KeInitializeEvent(&waitEvent,   
                          NotificationEvent,   
                          FALSE   );   

		  IoCopyCurrentIrpStackLocationToNext (Irp);   

		  //����������̣�   
          IoSetCompletionRoutine(Irp,   
                                  SfFsControlCompletion,   
                                  &waitEvent,       //context   parameter   
                                  TRUE,   
                                  TRUE,   
                                  TRUE);

          //��IRP���͵�������豸������   
		  status = IoCallDriver(   devExt->NLExtHeader.AttachedToDeviceObject,   Irp   );   
          
		  //�ȴ����MOUNT�Ĺ���   
		  status = KeWaitForSingleObject(&waitEvent,   
								  Executive,   
								  KernelMode,   
								  FALSE,   
								  NULL); 

          //ִ�а����ǵ����豸ATTACH��MOUNT���豸�ϵĹ���   
		  status = SfFsControlMountVolumeComplete(DeviceObject,Irp,newDeviceObject);   
	  #else   
		  //��WINXP�ķ�ʽ��ͨ��WorkItem��ʵ��   
		  ExInitializeWorkItem ();
		  status = IoCallDriver(devExt->NLExtHeader.AttachedToDeviceObject, Irp);   
	  #endif   
	}   
    
	//��ɺ����������������ô�Ĵ����أ�   
	NTSTATUS   
	SfFsControlMountVolumeComplete   (   
		  IN   PDEVICE_OBJECT   DeviceObject,   
		  IN   PIRP   Irp,   
		  IN   PDEVICE_OBJECT   NewDeviceObject   
	)   
  {   
	//���ȴ�newDevExt���ҵ������ԭ��MOUNT�����VPB����Ϊ��MOUNT�Ĺ����п��ܱ��ı䡣   
	newDevExt = NewDeviceObject->DeviceExtension;   
	
	vpb = newDevExt->NLExtHeader.StorageStackDeviceObject->Vpb;   
	
	//���MOUIN�ɹ�����ִ��ATTACH   
	status = SfAttachToMountedDevice(vpb->DeviceObject,NewDeviceObject   );   
	
	//Ȼ��õ���MOUNT�豸��DOS����LGetDosDeviceName(   NewDeviceObject,&newDevExt->NLExtHeader   );   
  } 
  Ϊʲô��������ķ����ء����ǵ���������USB�����ʱ���ʱ�������Volume��������û�б������BASE FILE SYSTEM ����MOUNT����Ϊһ���豸��
  ��������Ҫ����һ��EVENT�¼���Ȼ��IoSetCompletionRoutine����EVENT���룬Ȼ����ð�IRP�·�������� IoCallDriver��
  �����������BASE FILE SYSTEM�Ը�Volume MOUNT�ɹ���ɺ󣬽���Ȼ��������ǰ�����õ�SfFsControlCompletion���������������ڸ���ɺ����н�����EVENT �źŻ������������KeWaitForSingleObject����֪��EVENT�Ѿ��ź�״̬�ˣ���֪���Ѿ������MOUNT�Ĺ������������ܵ��ú����SfFsControlMountVolumeComplete�����������ڸú�������ɾ����ATTACH������   
  
  9������IRP�´������������   
		���������������̣�IoSetCompletionRoutine����ֱ�Ӱ�IRP�·����豸STACK���棬��ô��������������   
		IoSkipCurrentIrpStackLocation(Irp);   
		IoCallDriver(DeviceObject, Irp);   
		
		The IoSkipCurrentIrpStackLocation macro modifies the system's IO_STACK_LOCATION array pointer, 
		so that when the current driver calls the next-lower driver, that driver receives the same IO_STACK_LOCATION structure that the current driver received.   
		When sending an IRP to the next-lower driver, your driver can call IoSkipCurrentIrpStackLocation if you do not intend to provide an IoCompletion routine (the   address   of   which   is   stored   in   the   driver's  IO_STACK_LOCATION structure).   
		If you call IoSkipCurrentIrpStackLocation before calling IoCallDriver,  the next-lower driver receives the same IO_STACK_LOCATION that your driver received.     

		���Ҫ����������̣���ô����Ҫ������   
		IoCopyCurrentIrpStackLocationToNext (Irp);   
		IoSetCompletionRoutine(Irp,
							SfFsControlCompletion,   
							&waitEvent,       //context   parameter   
							TRUE,   
							TRUE,   
							TRUE   );   
		IoCallDriver(devExt->NLExtHeader.AttachedToDeviceObject,Irp);   

		The IoCopyCurrentIrpStackLocationToNext routine copies the IRP stack parameters from the current I/O stack location to the stack location of the next-lower driver and allows the current driver to set an I/O completion routine.   
		A driver calls IoCopyCurrentIrpStackLocationToNext to copy the IRP parameters from its stack location to the nex-lower driver��s stack location.     
		After  calling this routine, a driver typically sets an I/O completion routine with IoSetCompletionRoutine before passing the IRP to the next-lower driver with IoCallDriver.   Drivers   that   pass   on   their   IRP   parameters   but   do   not   set   an   I/O   completion   routine   should   call   IoSkipCurrentIrpStackLocation   instead   of   this   routine.   
	10�������豸���غ�ö��ϵͳ���豸   
		�����֪��ϵͳ������Щ�ļ�ϵͳ�����о���Ӧ����ʲôʱ������ǵĿ����豸��   
		��ʹ��IoRegisterFsRegistrationChange()��ʹ�����������������ע��һ���ص���������ϵͳ�����κ��ļ�ϵͳ����������Ǳ�ע����ʱ��ע����Ļص������ͻᱻ���á�   
		status   =   IoRegisterFsRegistrationChange(   DriverObject,   SfFsNotification   );   

		//������ĺ����н�ִ�о���������Ĳ�����   
		VOID   SfFsNotification   (   
			  IN   PDEVICE_OBJECT   DeviceObject,   
			  IN   BOOLEAN   FsActive   
		)   
		{   
		  if   (FsActive)   
			  SfAttachToFileSystemDevice(   DeviceObject,   devName   );   
		  else   
			  SfDetachFromFileSystemDevice(   DeviceObject   );   
		}   

		NTSTATUS   
		SfAttachToFileSystemDevice   (   
				  IN   PDEVICE_OBJECT   DeviceObject,   
				  IN   PNAME_CONTROL   DeviceName)   
	  {   
 
		  //���Ƿ��������豸�ļ�ϵͳ��CDO���豸�������±ߵļ��ֿ���   DISK   CDROM   NETWORK 
		  if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)     
		      return   STATUS_SUCCESS;   

		  /*��һ���������Ҵ��������ļ�ϵͳʶ�������ļ�ϵͳʶ�������ļ�ϵͳ������һ����С������Ϊ�˱���û��ʹ�õ����ļ�ϵͳ����ռ���ں��ڴ棬windowsϵͳ��������Щ���������������Ը��ļ�ϵͳ������Ӧ���ļ�ϵͳʶ���������µ�����洢ý�����ϵͳ��io�����������εĳ��Ը����ļ�ϵͳ�������С�ʶ�𡱡�ʶ��ɹ������̼����������ļ�ϵͳ��������Ӧ���ļ�ϵͳʶ������ж�ص�����������˵���ļ�ϵͳʶ�����Ŀ����豸����������һ���ļ�ϵͳ�����豸�������ǲ����������     
			�ֱ�ķ�����ͨ�����������֡������ļ�ϵͳʶ������������������֣�ע����DriverObject������DeviceObject!����Ϊ��\FileSystem\Fs_Rec��. */   
    
		 if   (RtlCompareUnicodeString(   &fsName->Name,&fsrecName,   TRUE   )   ==   0);   
		
		 //�����µ��豸   
		 status = IoCreateDevice( gSFilterDriverObject,   
                                      sizeof(   SFILTER_DEVICE_EXTENSION   ),   
                                      NULL,   
                                      DeviceObject->DeviceType,   
                                      0,   
                                      FALSE,   
                                      &newDeviceObject   );   
		�����豸��Ϊ����ϵͳ������������豸��ԭ�����豸ûʲô�������������һЩ���豸�ı�־λ�������󶨵��豸��ͬ��   
		if (FlagOn(   DeviceObject->Flags,   DO_BUFFERED_IO   ))   
        {   
			SetFlag(   newDeviceObject->Flags,   DO_BUFFERED_IO   );   
		}   
    
		//����ATTACH�Ĺ���   
		status   =   SfAttachDeviceToDeviceStack(newDeviceObject,   
                                                  DeviceObject,   
                                                  &devExt->NLExtHeader.AttachedToDeviceObject   );   
    
		//ͬʱ��WINXP��ϵͳ�£�ö�ٳ����е��豸��������û��ATTACH��ʱ�򣬾ͽ���ATTACH   
		#if   WINVER   >=   0x0501   
			status   =   SfEnumerateFileSystemVolumes(   DeviceObject   );   
		#endif   
    
		//ö���豸�ĺ���Ҳ�ǣ�ö�ٳ������豸   
		SfEnumerateFileSystemVolumes()   
		{   
			EnumerateDeviceObjectList)(FSDeviceObject->DriverObject,   
								  devList,   
								  (numDevices   *   sizeof(PDEVICE_OBJECT)),   
								  &numDevices);     
			for (i=0;   i< numDevices;   i++)     
			{   
			  if(SfIsAttachedToDevice(devList, NULL))     
				leave;   
			  status = IoCreateDevice(gSFilterDriverObject,   
									  sizeof(   SFILTER_DEVICE_EXTENSION   ),   
									  NULL,   
									  devList->DeviceType,   
									  0,   
									  FALSE,   
									  &newDeviceObject);
			  status = SfAttachToMountedDevice(devList,newDeviceObject);   
		}   

		11.//IRP��·��   
		status   =   IoCreateDevice(   gSFilterDriverObject,   
										  sizeof(   SFILTER_DEVICE_EXTENSION   ),   
										  NULL,   
										  DeviceObject->DeviceType,   
										  0,   
										  FALSE,   
										  &newDeviceObject   );     

		PSFILTER_DEVICE_EXTENSION   devExt   =   newDeviceObject->DeviceExtension;   
  
		//���ص��豸���󱣴���   �½������豸�������չ������   
		status = SfAttachDeviceToDeviceStack(newDeviceObject,   
                                          DeviceObject,   
                                          &devExt->NLExtHeader.AttachedToDeviceObject   );   
		//��Ϊ���ǽ��������豸�Ѿ��󶨵��ļ�ϵͳ�����豸��ȥ�ˡ�windows�����ļ�ϵͳ�����󷢸����ǵ����������������ǡ���Ĵ������ǵ�ϵͳ�ľͻ��������Ϊ�������豸��ʱ���豸����ṹ��չ��NONEPAGE�������ܱ����������ڼ䡣   
		//��Ϊ������������IoCreateDevice������ATTACH�˵���Ŀ���豸�����TOP������ԭ����Щ��ԭʼ�豸��IRP���ᵽ�����ǵ������У��������������˴�������̣������ᵽ�����ǵĺ��������������ǵ��豸�еĺ����У���ô�õ�ԭ����ATTACH���豸�����أ���������ԭ���������豸�����е���չ����������ķ�����   
		NTSTATUS   
		SfCreate   (   
				  IN   PDEVICE_OBJECT   DeviceObject,   
				  IN   PIRP   Irp   
		)   
		{   
			return   IoCallDriver( ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->NLExtHeader.AttachedToDeviceObject, Irp);   
		}   
  

		ע�ͣ�����MAX��FUNCTION������   
		IRP_MN_MOUNT_VOLUME   
		IRP_MN_LOAD_FILESYS   
		�����������ֻ��һ�����ͣ���һ���ļ�ʶ�����������ģ����������������ļ�ϵͳ��ʱ�򣬻����һ��������irp��   

	12���ļ�ϵͳ���豸����Ĺ�ϵ������   
		�����Ѿ���notify�����а����ļ�ϵͳ�����Ŀ��ƶ���
		���ļ�ϵͳ�õ�ʵ�ʵĽ��ʵ�ʱ�򣬻������µ��豸����,�����豸��Ϊ��(Volume),�������豸����file_sys�е�mount�����ɵģ�����Ҳ��unmount��ע�����ġ�
		���ǲ��������Ĳ���֮��,�ͱ����������ǵ��豸���󣬰��������ġ�����,���ܰ󶨶�������ϵ��ļ��Ĳ�����     
		VPB��Volume parameter block.һ�����ݽṹ.������Ҫ�����ǰ�ʵ�ʴ洢ý���豸������ļ�ϵͳ�ϵľ��豸������ϵ����.   
		Ϊʲô������IoRegisterFsRegistrationChange�����н���һЩ������Ҫ��VOLUME���д����أ������ǲ���ͬ�����飬ǰ���ǵ��ļ�ϵͳ��ע���ʱ�����������ǵ�����洢�豸���ļ�ϵͳMOUNT��ΪVOLUME��ʱ��   
	
	13��IRQL�Ϳ�ԽIRQL������   
	  //ʵ�ʵ�Ӧ��Ӧ����������:���е�dispatch   functions�������ϲ㷢����irp�����µĵ���,����Ӧ�ö���Passive   Level,����������Ե��þ������ϵͳ����.����������OnReceive,Ӳ�̶�д���,���ض����µ���ɺ���,���п�����Dispatch��.ע�ⶼ���п���,�����Ǿ�����.����һ���п���,���Ǿ�Ӧ�ð����ǿ���.     
	  //   Since   the   device   object   we   are   going   to   attach   to   has   not   yet   been   
	  //   created   (it   is   created   by   the   base   file   system)   we   are   going   to   use   
	  //   the   type   of   the   file   system   control   device   object.   We   are   assuming   
	  //   that   the   file   system   control   device   object   will   have   the   same   type   
	  //   as   the   volume   device   objects   associated   with   it.   
	  ���������ǣ����ϲ㷢��MOUNT��IRP�����ʱ����ʵ���ǽ�ҪATTACH���豸����û�н��������ڵõ��������Ӧ�Ŀ����豸����CDO��control   device   object�������ʱ�����Ǽ���CDO��volume   device   objects������ͬ�����͡�   
	  �������ǵķ�����������һ��������̣������MOUNT��ʱ�򣬸����̽������ã����������ATTACH�Ĺ�������������ɺ����е���   IoAttachDeviceToDeviceStack����Volume.   
	  �������ǵ����������������DISPATCH_LEVEL�ϵģ���IoAttachDeviceToDeviceStack   must   be   running   at   IRQL   <=   DISPATCH_LEVEL.   ʵ����ǰ��˵����IoAttachDeviceToDeviceStackSafe,������ÿ�����Dispatch   level����.����������ý���������Xp���ϵ�ϵͳ��.   
	  ��Խ�жϼ���������м��ַ���.��һ�����Լ�����һ��ϵͳ�߳�����ɴ���.ϵͳ�߳̽���֤��Passive   Level������.��һ�ַ������ǰ��Լ����������Windows�������߳�,���ʹ����������õ�ִ��.����������Ƚ�С,����ʵ�еڶ��ַ���.��ϵͳ��˵�Ƚ�ʡ��,�Գ���Ա��˵���������鷳.   

	14://   ��ַ����Ч��   
	  �����������ڴ���IRP_MJ_READ��Ӧ��SFREAD����������     
	  1��IRP����һ��FileObjectָ��.�������ָ��һ���ļ�����.����Եõ��ļ����������,���������û���̷����ļ�ȫ·��.�����ͨ��FILEMON�ķ�����   
	  2���̷���λ��?��Ϊ�Ѿ�֪����Volume,ǰ���Ѿ�˵���̷�������Volume�ķ���������������Ҳ���Ǻܴ�����⡣   
	  3�����ļ���ƫ������irpsp->Parameters.Read.ByteOffset;     
	  4������Ķ��������������أ�   
	  Depending   on   whether   the   underlying   device   driver   sets   up   the   target   device   object's   Flags   with   DO_BUFFERED_IO   or   with   DO_DIRECT_IO,   data   is   transferred   into   one   of   the   following:     
	  ��The   buffer   at   Irp->AssociatedIrp.SystemBuffer   if   the   driver   uses   buffered   I/O     
	  ��The   buffer   described   by   the   MDL   at   Irp->MdlAddress   if   the   underlying   device   driver   uses   direct   I/O   (DMA   or   PIO)     
	  Volume�豸����DO_BUFFERED���������û��,����DO_DIRECT_IO��ʾ����Ӧ�÷��ص�   
	  Irp->MdlAddress��ָ���MDL��ָ����ڴ�.���ޱ�ǵ������,�������ݶ���,�뷵�ص�     
	  Irp->UseBuffer�м���.Irp->UseBuffer��һ��ֻ�ڵ�ǰ�߳������Ĳ���Ч�ĵ�ַ.�����ǰ�����õ�������̣���ԭ�����̲߳�����һ���������еģ�����������������еõ��ĸõ�ַ�ǲ���ȷ�ġ�Ҫôֻ�ܴ�Irp->MdlAddress�еõ����ݣ������Ҫ�ص���ǰ�߳������ģ���ô��ʹ��ǰ��ķ�����ͨ���ȴ�EVENT�ķ�����   

	15: 
	  ���MOUNT�Ĺ�������͵��ǵ���һ���ļ������߼��������ʱ�򱻴�����
	  The volume mount process is typically triggered by a request to open a file on a logical volume (that is, a partition or dynamic volume) as follows:     
	  
	  һ���û�Ӧ�õ���CREATEFILE����һ���ļ��������ں�ģʽ�������������ZwCreateFile��   
	  1.A user application calls CreateFile to open a file Or a kernel-mode driver calls ZwCreateFile or IoCreateFileSpecifyDeviceObjectHint.     

	  I/O�����������ĸ��߼����������Ŀ�꣬���Ҽ���豸���󣬲鿴�Ƿ����Ѿ���MOUNT�����VPB_MOUNTED��ʾ�����ã���֤�����ļ�ϵͳ�����ˡ�   
	  2.The I/O Manager determines which logical volume is the   target   of   the   request   and   checks   its   device   object   to   see   whether   it   is   mounted.   If   the   VPB_MOUNTED   flag   is   set,   the   volume   has   been   mounted   by   a   file   system.     

	  ������Դ�ϵͳ������û�б��ļ�ϵͳMOUNT��I/O����������һ����MOUNT������(IRP_MJ_FILE_SYSTEM_CONTROL,   IRP_MN_MOUNT_VOLUME)��ӵ�иþ���ļ�ϵͳ��   
	  �������е������ļ�ϵͳ���Ǳ�����صģ���ʹϵͳ�������������ģ���������ļ�ϵͳû�б����أ���ôI/O���������;�MOUNT�������ļ�ϵͳ������(FsRec)�ϣ�����Ϊ�ļ�ϵͳ�����boot   sector   
	  3.If   the   volume   has   not   been   mounted   by   a   file   system   since   system   boot   (that   is,   the   VPB_MOUNTED   flag   is   not   set),   the   I/O   Manager   sends   a   volume   mount   (IRP_MJ_FILE_SYSTEM_CONTROL,   IRP_MN_MOUNT_VOLUME)   request   to   each   file   system   that   might   claim   the   volume.     
	  Not   all   built-in   file   systems   are   necessarily   loaded  