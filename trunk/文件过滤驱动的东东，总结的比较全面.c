1> IFS   流程图   
	a.生成一个控制设备.当然此前你必须给控制设置指定名称.   
	b.设置Dispatch   Functions.     
	c.设置Fast   Io   Functions.     
	d.编写一个my_fs_notify回调函数，在其中绑定刚激活的FS   CDO.     
	e.使用wdff_reg_notify调用注册这个回调函数。   
	f.编写默认的dispatch   functions.     
	g.处理IRP_MJ_FILE_SYSTEM_CONTROL,在其中监控Volumne的Mount和Dismount.     
	h.下一步自然是绑定Volumne了.   
	(全路径是在   FileObject->FileName.Buffer中得到的.)   

2> 一些必要知识   
	a.几个概念的区别

	1.多数的storage drivers是PNP管理的,存在一个设备节点（DEVNODE），
		It is important to note that file systems and file system filter drivers are not PnP device drivers,
		每个设备节点上维护一个Storage Device Stacks，
		这个就是因为每个存储设备,例如磁盘设备,可能包含一个或者多个逻辑卷(分区或者动态卷),这些卷就是通过这个Storage Device Stack来保存的。
		该设备点的信息就是functional device object (FDO)。
		剩下的就是physical device objects (PDO)代表各个分区。   

	2.通过下面的方式可以得到卷的名称   
		The Mount Manager responds to the arrival of a new storage volume by querying the volume driver for the following information:     
		·The volume's nonpersistent device object name (or target name), located in the Device directory of the system object tree (for example: "\Device\HarddiskVolume1")     
		·The volume's globally unique identifier (GUID), also called the unique volume name     
		·A suggested persistent symbolic link name for the volume, such as a drive letter (for example, "\DosDevices\D:")     

	3.文件系统和卷的区别   
		When a file system is mounted on a storage volume,it creates a file system volume device object (VDO) to represent the volume to the file system.   
		The file system VDO is mounted on the storage device object by means of a shared object called a volume parameterblock (VPB).   
		File System Stacks: File system drivers create two different types of device objects: control device objects (CDO) and volume device objects(VDO).   
		File System Control Device Objects(CDO)
		File System Volume Device Objects(VDO)

3>代码分析(详细参考Sfilter)   
	1：
	DriverEntry   ()   
	{   
		status = IoRegisterFsRegistrationChange(DriverObject,SfFsNotification);   
	}

	SfFsNotification(IN PDEVICE_OBJECT DeviceObject, //原始文件系统   
					IN BOOLEAN  FsActive)   
	{     
		SfAttachToFileSystemDevice(DeviceObject, &name);   
	}   
	SfAttachToFileSystemDevice(               
				IN   PDEVICE_OBJECT   DeviceObject,//原始文件系统   
				IN   PUNICODE_STRING   DeviceName)   
	{   
		status = IoCreateDevice(...   
						&newDeviceObject   );   
						status   =   SfAttachDeviceToDeviceStack(   newDeviceObject,     
						DeviceObject,     
						&devExt->AttachedToDeviceObject);//原始的保存
	}   

	2 文件类型分为：     
		(((_type)   ==   FILE_DEVICE_DISK_FILE_SYSTEM)   ||   \   磁盘文件系统   
		((_type)   ==   FILE_DEVICE_CD_ROM_FILE_SYSTEM)   ||   \CDROM文件系统   
		((_type)   ==   FILE_DEVICE_NETWORK_FILE_SYSTEM   网络文件系统   
  
	3：当在驱动中调用了   
		status = IoCreateDevice(DriverObject,   
                          0,                               //has   no   device   extension   
                          &nameString,   
                          FILE_DEVICE_DISK_FILE_SYSTEM,   
                          FILE_DEVICE_SECURE_OPEN,   
                          FALSE,   
                          &gSFilterControlDeviceObject   );   
		那么发向该设备对象的IRP将能在该驱动(DriverObject)中接收到，当建立多个设备对象的时候，可以通过判断设备对象来进行是哪个发出的。   
		同时假设建立的其中一个设备和另外的系统的设备ATTACH了，那么发向原来系统设备对象的IRP也就能在这个驱动中接收到。例如在下面的分发例程中就能得到这些IRP。   
		for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++){
			DriverObject->MajorFunction   =   SfPassThrough;   
			DriverObject->MajorFunction[IRP_MJ_CREATE]   =   SfCreate;   
		}

	4：首先文件系统驱动本身往往生成一个控制设备（CDO），这个控制对象用来和外部的应用进行通讯，来配置驱动的。   
	   另一种设备是被这个文件系统Mount的Volume。
	   一个FS可能有多个Volume,也可能一个都没有（解释一下，如果你有C:,D:,E:,F:四个分区。C:,D:为NTFS,E:,F:为Fat32.那么C:,D:则是Fat的两个Volume设备对象.)文件系统驱动是针对每个Volume来生成一个DeviceObjec（IoCreateDevice）。   
  
	5：因为IRP是上层应用到下面内核层的，所以只要我们建立的新设备ATTACH到原始的设备上，将位于绑定的文件对象STACK的上边，这样上面来的IRP将能首先被我们新建立的设备对象捕获。但是对于下面原始设备自己发出的请求，只能使用设置CALLBACK的方法了。   

	6：由于你的驱动将要绑定到文件系统驱动的上边，文件系统除了处理正常的IRP之外，还要处理所谓的FastIo.FastIo是Cache   Manager调用所引发的一种没有irp的请求。因为FAST   IO是用于CACHE的，不和下面的BASE   FILE   SYSTEM直接打交道，但是对于上层来说也是文件系统的访问，所以也要设置。具体就是先分配一个空间，   
		PFAST_IO_DISPATCH   fastIoDispatch   =   ExAllocatePoolWithTag()   
		把FAST   IO的函数赋给这个结构，fastIoDispatch->FastIoRead   =   SfFastIoRead;   
		然后DriverObject->FastIoDispatch   =   fastIoDispatch;   

	7：irp是从设备栈的顶端开始，逐步向下发送。DevVolumue表示我们实际要过滤的Volume设备，我们只要在这个设备栈的顶端再绑定一个设备，那发送给Volume的请求，自然会先发给我们的设备来处理。IoAttachDeviceToDeviceStack（注意源设备未必直接绑定在目标设备上。它应绑定在目标设备的设备栈的顶端。）比如“C:”这个设备，我已经知道符号连接为“C:”,不难得到设备名。得到设备名后，又不难得到设备。这时候我们IoCreateDevice()生成一个Device   Object,然后调用IoAttachDeviceToDeviceStack绑定，所有发给“C:”的irp,就必然先发送给我们的驱动，我们也可以捕获所有对文件的操作了!   
	
	8：以上的方法是静态的，，如果不想处理动态的Volume,你完全可以这样做。但是我们这里有更高的要求。当你把一个U盘插入usb口，一个“J:”之类的Volume动态诞生的时候，我们依然要捕获这个事件，并生成一个Device来绑定它。   
	一个新的存储媒质被系统发现并在文件系统中生成一个Volume的过程称为Mounting.其过程开始的时候，FS的CDO将得到一个IRP,其Major   Function   Code为IRP_MJ_FILE_SYSTEM_CONTROL，Minor   Function   Code为IRP_MN_MOUNT。换句话说，如果我们已经生成了一个设备绑定文件系统的CDO，那么我们就可以得到这样的IRP,在其中知道一个新的Volume正在Mount.这时候我们可以执行上边所说的操作。   这就是下面代码的含义：   
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

	//对于USB盘等分析如下：   
	NTSTATUS   
	SfFsControlMountVolume   (   
		  IN   PDEVICE_OBJECT   DeviceObject,   
		  IN   PIRP   Irp)   
	{   
		//首先要保存because this VPB may be changed by the underlying file system.   
		storageStackDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;   

		//判断不是影子设备   
		status = SfIsShadowCopyVolume   (storageStackDeviceObject,&isShadowCopyVolume);   

		//建立新的FILTER设备对象，准备附加到MOUNT的设备上   
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
          //设置事件   
          KeInitializeEvent(&waitEvent,   
                          NotificationEvent,   
                          FALSE   );   

		  IoCopyCurrentIrpStackLocationToNext (Irp);   

		  //设置完成例程，   
          IoSetCompletionRoutine(Irp,   
                                  SfFsControlCompletion,   
                                  &waitEvent,       //context   parameter   
                                  TRUE,   
                                  TRUE,   
                                  TRUE);

          //把IRP传送到下面的设备对象中   
		  status = IoCallDriver(   devExt->NLExtHeader.AttachedToDeviceObject,   Irp   );   
          
		  //等待完成MOUNT的工作   
		  status = KeWaitForSingleObject(&waitEvent,   
								  Executive,   
								  KernelMode,   
								  FALSE,   
								  NULL); 

          //执行把我们的新设备ATTACH到MOUNT的设备上的功能   
		  status = SfFsControlMountVolumeComplete(DeviceObject,Irp,newDeviceObject);   
	  #else   
		  //非WINXP的方式，通过WorkItem来实现   
		  ExInitializeWorkItem ();
		  status = IoCallDriver(devExt->NLExtHeader.AttachedToDeviceObject, Irp);   
	  #endif   
	}   
    
	//完成后，这个函数中做了怎么的处理呢？   
	NTSTATUS   
	SfFsControlMountVolumeComplete   (   
		  IN   PDEVICE_OBJECT   DeviceObject,   
		  IN   PIRP   Irp,   
		  IN   PDEVICE_OBJECT   NewDeviceObject   
	)   
  {   
	//首先从newDevExt中找到保存的原来MOUNT对象的VPB，因为在MOUNT的过程中可能被改变。   
	newDevExt = NewDeviceObject->DeviceExtension;   
	
	vpb = newDevExt->NLExtHeader.StorageStackDeviceObject->Vpb;   
	
	//如果MOUIN成功，就执行ATTACH   
	status = SfAttachToMountedDevice(vpb->DeviceObject,NewDeviceObject   );   
	
	//然后得到新MOUNT设备的DOS名称LGetDosDeviceName(   NewDeviceObject,&newDevExt->NLExtHeader   );   
  } 
  为什么采用上面的方法呢。就是当发现例如USB插入的时间的时候，这个卷（Volume）还可能没有被下面的BASE FILE SYSTEM 进行MOUNT而成为一个设备，
  所以首先要设置一个EVENT事件，然后IoSetCompletionRoutine，把EVENT传入，然后调用把IRP下发到下面的 IoCallDriver，
  这样当下面的BASE FILE SYSTEM对该Volume MOUNT成功完成后，将自然调用我们前面设置的SfFsControlCompletion（）函数，这样在该完成函数中仅仅把EVENT 信号化，这样后面的KeWaitForSingleObject就能知道EVENT已经信号状态了，就知道已经完成了MOUNT的工作，这样就能调用后面的SfFsControlMountVolumeComplete（）函数，在该函数中完成具体的ATTACH工作。   
  
  9：关于IRP下传的问题的讨论   
		如果不设置完成例程（IoSetCompletionRoutine），直接把IRP下发到设备STACK下面，那么仅仅就是两步：   
		IoSkipCurrentIrpStackLocation(Irp);   
		IoCallDriver(DeviceObject, Irp);   
		
		The IoSkipCurrentIrpStackLocation macro modifies the system's IO_STACK_LOCATION array pointer, 
		so that when the current driver calls the next-lower driver, that driver receives the same IO_STACK_LOCATION structure that the current driver received.   
		When sending an IRP to the next-lower driver, your driver can call IoSkipCurrentIrpStackLocation if you do not intend to provide an IoCompletion routine (the   address   of   which   is   stored   in   the   driver's  IO_STACK_LOCATION structure).   
		If you call IoSkipCurrentIrpStackLocation before calling IoCallDriver,  the next-lower driver receives the same IO_STACK_LOCATION that your driver received.     

		如果要设置完成例程，那么就需要三步：   
		IoCopyCurrentIrpStackLocationToNext (Irp);   
		IoSetCompletionRoutine(Irp,
							SfFsControlCompletion,   
							&waitEvent,       //context   parameter   
							TRUE,   
							TRUE,   
							TRUE   );   
		IoCallDriver(devExt->NLExtHeader.AttachedToDeviceObject,Irp);   

		The IoCopyCurrentIrpStackLocationToNext routine copies the IRP stack parameters from the current I/O stack location to the stack location of the next-lower driver and allows the current driver to set an I/O completion routine.   
		A driver calls IoCopyCurrentIrpStackLocationToNext to copy the IRP parameters from its stack location to the nex-lower driver’s stack location.     
		After  calling this routine, a driver typically sets an I/O completion routine with IoSetCompletionRoutine before passing the IRP to the next-lower driver with IoCallDriver.   Drivers   that   pass   on   their   IRP   parameters   but   do   not   set   an   I/O   completion   routine   should   call   IoSkipCurrentIrpStackLocation   instead   of   this   routine.   
	10：发现设备加载和枚举系统的设备   
		如果想知道系统中有那些文件系统，还有就是应该在什么时候绑定它们的控制设备。   
		将使用IoRegisterFsRegistrationChange()，使用这个函数函数调用注册一个回调函数。当系统中有任何文件系统被激活或者是被注销的时候，注册过的回调函数就会被调用。   
		status   =   IoRegisterFsRegistrationChange(   DriverObject,   SfFsNotification   );   

		//在下面的函数中将执行具体的真正的操作。   
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
 
		  //看是否是三种设备文件系统的CDO的设备类型有下边的几种可能   DISK   CDROM   NETWORK 
		  if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)     
		      return   STATUS_SUCCESS;   

		  /*下一个问题是我打算跳过文件系统识别器。文件系统识别器是文件系统驱动的一个很小的替身。为了避免没有使用到的文件系统驱动占据内核内存，windows系统不加载这些大驱动，而代替以该文件系统驱动对应的文件系统识别器。当新的物理存储媒介进入系统，io管理器会依次的尝试各种文件系统对它进行“识别”。识别成功，立刻加载真正的文件系统驱动，对应的文件系统识别器则被卸载掉。对我们来说，文件系统识别器的控制设备看起来就像一个文件系统控制设备。但我们不打算绑定它。     
			分辨的方法是通过驱动的名字。凡是文件系统识别器的驱动对象的名字（注意是DriverObject而不是DeviceObject!）都为“\FileSystem\Fs_Rec”. */   
    
		 if   (RtlCompareUnicodeString(   &fsName->Name,&fsrecName,   TRUE   )   ==   0);   
		
		 //建立新的设备   
		 status = IoCreateDevice( gSFilterDriverObject,   
                                      sizeof(   SFILTER_DEVICE_EXTENSION   ),   
                                      NULL,   
                                      DeviceObject->DeviceType,   
                                      0,   
                                      FALSE,   
                                      &newDeviceObject   );   
		生成设备后，为了让系统看起来，你的设备和原来的设备没什么区别，你必须设置一些该设备的标志位与你所绑定的设备相同。   
		if (FlagOn(   DeviceObject->Flags,   DO_BUFFERED_IO   ))   
        {   
			SetFlag(   newDeviceObject->Flags,   DO_BUFFERED_IO   );   
		}   
    
		//进行ATTACH的工作   
		status   =   SfAttachDeviceToDeviceStack(newDeviceObject,   
                                                  DeviceObject,   
                                                  &devExt->NLExtHeader.AttachedToDeviceObject   );   
    
		//同时在WINXP的系统下，枚举出所有的设备，当发现没有ATTACH的时候，就进行ATTACH   
		#if   WINVER   >=   0x0501   
			status   =   SfEnumerateFileSystemVolumes(   DeviceObject   );   
		#endif   
    
		//枚举设备的函数也是，枚举出各个设备   
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

		11.//IRP的路径   
		status   =   IoCreateDevice(   gSFilterDriverObject,   
										  sizeof(   SFILTER_DEVICE_EXTENSION   ),   
										  NULL,   
										  DeviceObject->DeviceType,   
										  0,   
										  FALSE,   
										  &newDeviceObject   );     

		PSFILTER_DEVICE_EXTENSION   devExt   =   newDeviceObject->DeviceExtension;   
  
		//返回的设备对象保存在   新建立的设备对象的扩展数据中   
		status = SfAttachDeviceToDeviceStack(newDeviceObject,   
                                          DeviceObject,   
                                          &devExt->NLExtHeader.AttachedToDeviceObject   );   
		//因为我们建立的新设备已经绑定到文件系统控制设备上去了。windows发给文件系统的请求发给我们的驱动。如果不能做恰当的处理，我们的系统的就会崩溃。因为建立新设备的时候，设备对象结构扩展是NONEPAGE，所以能保存在整个期间。   
		//因为我们在驱动中IoCreateDevice（）并ATTACH了到了目标设备对象的TOP，所以原来那些到原始设备的IRP都会到达我们的驱动中，并且我们设置了处理的例程，这样会到达我们的函数。但是在我们的设备中的函数中，怎么得到原来被ATTACH的设备对象呢，就是我们原来保存在设备对象中的扩展。就是下面的方法：   
		NTSTATUS   
		SfCreate   (   
				  IN   PDEVICE_OBJECT   DeviceObject,   
				  IN   PIRP   Irp   
		)   
		{   
			return   IoCallDriver( ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->NLExtHeader.AttachedToDeviceObject, Irp);   
		}   
  

		注释：几个MAX—FUNCTION的区别   
		IRP_MN_MOUNT_VOLUME   
		IRP_MN_LOAD_FILESYS   
		这个功能码我只做一点点解释：当一个文件识别器（见上文）决定加载真正的文件系统的时候，会产生一个这样的irp。   

	12：文件系统和设备、卷的关系和区别   
		我们已经在notify函数中绑定了文件系统驱动的控制对象。
		当文件系统得到实际的介质的时候，会生成新的设备对象,这种设备称为卷(Volume),而这种设备是在file_sys中的mount中生成的，而且也是unmount中注销掉的。
		我们捕获这样的操作之后,就必须生成我们的设备对象，绑定在这样的“卷”上,才能绑定对这个卷上的文件的操作。     
		VPB是Volume parameter block.一个数据结构.它的主要作用是把实际存储媒介设备对象和文件系统上的卷设备对象联系起来.   
		为什么我们在IoRegisterFsRegistrationChange（）中进行一些处理，还要对VOLUME进行处理呢，两者是不相同的事情，前者是当文件系统被注册的时候发生，或者是当物理存储设备被文件系统MOUNT成为VOLUME的时候。   
	
	13：IRQL和跨越IRQL的限制   
	  //实际的应用应该是这样的:所有的dispatch   functions由于是上层发来的irp而导致的调用,所以应该都是Passive   Level,在其中你可以调用绝大多数系统调用.而如网卡的OnReceive,硬盘读写完毕,返回而导致的完成函数,都有可能在Dispatch级.注意都是有可能,而不是绝对是.但是一旦有可能,我们就应该按就是考虑.     
	  //   Since   the   device   object   we   are   going   to   attach   to   has   not   yet   been   
	  //   created   (it   is   created   by   the   base   file   system)   we   are   going   to   use   
	  //   the   type   of   the   file   system   control   device   object.   We   are   assuming   
	  //   that   the   file   system   control   device   object   will   have   the   same   type   
	  //   as   the   volume   device   objects   associated   with   it.   
	  上面的理解是，当上层发出MOUNT的IRP请求的时候，其实我们将要ATTACH的设备对象还没有建立，现在得到的是其对应的控制设备对象CDO（control   device   object），这个时候我们假设CDO和volume   device   objects具有相同的类型。   
	  所以我们的方法就是设置一个完成例程，当完成MOUNT的时候，该例程将被调用，在这里完成ATTACH的工作。决定在完成函数中调用   IoAttachDeviceToDeviceStack来绑定Volume.   
	  但是我们的完成例程是运行在DISPATCH_LEVEL上的，而IoAttachDeviceToDeviceStack   must   be   running   at   IRQL   <=   DISPATCH_LEVEL.   实际上前边说过有IoAttachDeviceToDeviceStackSafe,这个调用可以在Dispatch   level进行.无奈这个调用仅仅出现在Xp以上的系统中.   
	  超越中断级别的限制有几种方法.第一种是自己生成一个系统线程来完成此事.系统线程将保证在Passive   Level中运行.另一种方法就是把自己的任务插入Windows工作者线程,这会使你的任务迟早得到执行.如果你的任务比较小,可以实行第二种方法.对系统来说比较省事,对程序员来说则反正都是麻烦.   

	14://   地址的有效性   
	  假设我们现在处理IRP_MJ_READ对应的SFREAD（）函数。     
	  1：IRP下有一个FileObject指针.这个东西指向一个文件对象.你可以得到文件对象的名字,这个名字是没有盘符的文件全路径.这可以通过FILEMON的方法。   
	  2：盘符如何获得?因为已经知道了Volume,前边已经说过盘符不过是Volume的符号连接名，所以也不是很大的问题。   
	  3：读文件的偏移量：irpsp->Parameters.Read.ByteOffset;     
	  4：具体的读的数据在那里呢？   
	  Depending   on   whether   the   underlying   device   driver   sets   up   the   target   device   object's   Flags   with   DO_BUFFERED_IO   or   with   DO_DIRECT_IO,   data   is   transferred   into   one   of   the   following:     
	  ·The   buffer   at   Irp->AssociatedIrp.SystemBuffer   if   the   driver   uses   buffered   I/O     
	  ·The   buffer   described   by   the   MDL   at   Irp->MdlAddress   if   the   underlying   device   driver   uses   direct   I/O   (DMA   or   PIO)     
	  Volume设备出现DO_BUFFERED的情况几乎没有,所以DO_DIRECT_IO表示数据应该返回到   
	  Irp->MdlAddress所指向的MDL所指向的内存.在无标记的情况下,表明数据读好,请返回到     
	  Irp->UseBuffer中即可.Irp->UseBuffer是一个只在当前线程上下文才有效的地址.如果在前面设置的完成例程，和原来的线程不是在一个上下文中的，所以在完成例程序中得到的该地址是不正确的。要么只能从Irp->MdlAddress中得到数据，如果想要回到当前线程上下文，那么就使用前面的方法，通过等待EVENT的方法。   

	15: 
	  卷的MOUNT的过程最典型的是当打开一个文件或者逻辑卷的请求时候被触发。
	  The volume mount process is typically triggered by a request to open a file on a logical volume (that is, a partition or dynamic volume) as follows:     
	  
	  一个用户应用调用CREATEFILE来打开一个文件，或者内核模式的驱动程序调用ZwCreateFile。   
	  1.A user application calls CreateFile to open a file Or a kernel-mode driver calls ZwCreateFile or IoCreateFileSpecifyDeviceObjectHint.     

	  I/O管理器决定哪个逻辑卷是请求的目标，并且检查设备对象，查看是否它已经被MOUNT。如果VPB_MOUNTED表示被设置，则证明卷被文件系统加载了。   
	  2.The I/O Manager determines which logical volume is the   target   of   the   request   and   checks   its   device   object   to   see   whether   it   is   mounted.   If   the   VPB_MOUNTED   flag   is   set,   the   volume   has   been   mounted   by   a   file   system.     

	  如果卷自从系统启动后没有被文件系统MOUNT，I/O管理器发送一个卷MOUNT的请求(IRP_MJ_FILE_SYSTEM_CONTROL,   IRP_MN_MOUNT_VOLUME)到拥有该卷的文件系统。   
	  不是所有的内置文件系统都是必须加载的，即使系统启动或是正常的，如果内置文件系统没有被加载，那么I/O管理器发送卷MOUNT的请求到文件系统发现器(FsRec)上，它会为文件系统检查卷的boot   sector   
	  3.If   the   volume   has   not   been   mounted   by   a   file   system   since   system   boot   (that   is,   the   VPB_MOUNTED   flag   is   not   set),   the   I/O   Manager   sends   a   volume   mount   (IRP_MJ_FILE_SYSTEM_CONTROL,   IRP_MN_MOUNT_VOLUME)   request   to   each   file   system   that   might   claim   the   volume.     
	  Not   all   built-in   file   systems   are   necessarily   loaded  