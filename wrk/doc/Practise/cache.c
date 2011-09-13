//ROS在内存管理和缓存管理方面都是虚有其表——只有接口是一样的，里面的实现与windows大相径庭，以下资料来源于wrk，internals。
//缓存管理是对于文件数据在内存中的特殊管理，基于内存管理，向文件系统提供服务。
//而在内存管理器将缓存页面真正写入磁盘时，又调用文件系统的接口然后走磁盘设备驱动。

//★.重要结构
//1.Virtual Address Control Block 虚拟地址控制块（核心） 
//描述了系统缓存的一个槽，系统缓存以256k为一个槽（也成为一个view），
//当进行读写操作时，数据才是活动的（Active），当不活动之后映射不会消失，除非缓存槽已经用尽，他们将被撤除。

//CcVacbs是一个数组，记录了每个VACB的地址
typedef struct _VACB {
	PVOID BaseAddress;
	struct _SHARED_CACHE_MAP *SharedCacheMap;        //被映射到一个文件时，它指向文件的共享缓存表
	union {
		LARGE_INTEGER FileOffset;                    //这份视图在文件中的偏移
		USHORT ActiveCount;                          //引用计数
	} Overlay;                                                
	LIST_ENTRY LruList;                              //管理列表，空闲或者可重用的视图链表（CcVacbFreeList,CcVacbLru）
} VACB, *PVACB;       

//2. PRIVATE_CACHE_MAP 存储于FILE_OBJECT之中的私有缓存表 属于文件对象
typedef struct _PRIVATE_CACHE_MAP {
	union {
	CSHORT NodeTypeCode;
	PRIVATE_CACHE_MAP_FLAGS Flags;
	ULONG UlongFlags;
	};
	ULONG ReadAheadMask;
	PFILE_OBJECT FileObject;
	LARGE_INTEGER FileOffset1;				//包含最近两次读的位置
	LARGE_INTEGER BeyondLastByte1;
	LARGE_INTEGER FileOffset2;
	LARGE_INTEGER BeyondLastByte2;
	LARGE_INTEGER ReadAheadOffset[2];
	ULONG ReadAheadLength[2];
	KSPIN_LOCK ReadAheadSpinLock;
	LIST_ENTRY PrivateLinks;
} PRIVATE_CACHE_MAP;

//3. SHARED_CACHE_MAP 针对文件的共享缓存表，属于文件本身
//	 在FileObject->SectionObjectPointer->SharedCacheMap 有保存其指针
typedef struct _SHARED_CACHE_MAP {
	。。。。。。。。。。。。。。。。。
	ULONG OpenCount;				//打开计数
	LARGE_INTEGER FileSize;
	LIST_ENTRY BcbList;				//按照文件偏移递减的BCB链表
	LARGE_INTEGER SectionSize;
	LARGE_INTEGER ValidDataLength;  //有效数据长度
	LARGE_INTEGER ValidDataGoal;
	PVACB InitialVacbs[PREALLOCATED_VACBS];  //预留的VACB索引数组 第一项指向第一个256k 第二项指向第二个 如果其值不为0 则包含了一个VACB 
	PVACB * Vacbs;					//额外的数组，用于大于1M的内存，放于非分页区。大于32M，这个数组将是多级稀疏的。
	PFILE_OBJECT FileObject;
	volatile PVACB ActiveVacb;			//每次读写操作完之后还要记录一个VACB ，下次再操作相同区域时避免调用CcGetVirtualAddress
	。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
	LIST_ENTRY PrivateList;				//私有表的链表
	。。。。。。。。。。。。。。
	LIST_ENTRY SharedCacheMapLinks;		//全局的共享缓存表
	。。。。。。。。。。。。。。
} SHARED_CACHE_MAP;

//4.Buffer Control Block
//缓存控制块，代表着一块被锁定的内存，在调用CcPinFileData时被初次创建，当计数减为零且不Dirty的时候，被销毁。
//作为对VACB的补充而存在
typedef struct _BCB {
	CSHORT NodeTypeCode;   //类型
	BOOLEAN Dirty;		   //尚未写回磁盘
	BOOLEAN Reserved;
	//  Byte FileOffset and and length of entire buffer
	ULONG  ByteLength;          //字节长度
	LARGE_INTEGER FileOffset;	//偏移
	//  Links for BcbList in SharedCacheMap
	LIST_ENTRY BcbLinks;        //除了有一个链表之外，每512K的BCB node就有一个头结点插入，这些头结点组成一个数组，跟在SharedCacheMap中PVACB数组的后面，多级表的时候，除了一层表，每层都有这个头结点数组，用于快查
	LARGE_INTEGER BeyondLastByte;  //缓冲区中最后一个字节的偏移
	………………………………………..
	//根据internals     可恢复文件系统 将数据写入到缓存时，提供一个关联的最高和最低的LSN 代表着最老和最新的日志文件记录
	LARGE_INTEGER OldestLsn;  
	LARGE_INTEGER NewestLsn;
	………………………………………..
	PVACB Vacb;				//虚拟地址控制块
	………………………………………..
	//  锁定计数
	ULONG PinCount;
	PSHARED_CACHE_MAP SharedCacheMap;
	// 基址
	PVOID BaseAddress;
} BCB;

//调用关系
//应用程序通过API发出一个IO的请求 
//→ 文件系统驱动负责创建文件对象，以及相关的管理结构。
//→ 对于已标记不缓存的文件 直接走存储驱动 / 缓存文件则调用CcCopyRead、CcCopyWrite等
//→ 缓存管理器MmFlushSection的时候会产生页面错误，由内存管理器调用IoPageRead经由文件系统而直接走存储设备驱动。

CcInitializeCacheMap(
	__in PFILE_OBJECT FileObject,     //文件对象
	__in PCC_FILE_SIZES FileSizes,    //大小
	__in BOOLEAN PinAccess,			  //是否使用锁
	__in PCACHE_MANAGER_CALLBACKS Callbacks,        //回调预读和延迟写的回调通知
	__in PVOID LazyWriteContext                     //回调参数？
)

//文件系统调用，对一个文件进行缓存时调用。
//共享缓存表，内存区对象，VACB数组，私有缓存表等等结构的建立
CcGetVirtualAddress (
	IN PSHARED_CACHE_MAP SharedCacheMap,
	IN LARGE_INTEGER FileOffset,
	OUT PVACB *Vacb,
	IN OUT PULONG ReceivedLength
)

//管理VACB的主要函数，对于给定的文件偏移，如果已有vacb，返回。
//如果没有，就要读取放入VACB。由CcGetVacbMiss找到一个空闲vacb，在缓存区申请一个256k的内存，调用内存管理器为缓存管理器提供的接口 MmMapViewInSystemCache 在内存区对象中创建相应的原型PTE。至此CcCopyRead已经可以读取文件的数据了，发生内存错误后内存管理器会把这部分内存换入。
CcCopyRead (
	__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset,
	__in ULONG Length,
	__in BOOLEAN Wait,
	__out_bcount(Length) PVOID Buffer,
	__out PIO_STATUS_BLOCK IoStatus
)

//在系统缓冲区和用户缓冲区之间拷贝数据，先找ActiveVacb（SHARED_CACHE_MAP中的结构），
//不行在调用CcGetVirtualAddress，然后RtlCopyMemory
CcCopyWrite (
	__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset,
	__in ULONG Length,
	__in BOOLEAN Wait,
	__in_bcount(Length) PVOID Buffer
)

//如果是写入有缓存映射的部分，通过内存拷贝去写。
//如果没有映射，则调用CcMapAndCopy先创建一个映射视图，将内存拷贝进去再通过MmFlushSection刷回存储设备，或者标记脏页面，通过Mm或者Cc将脏页面换出。
CcMapData (
	__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset,
	__in ULONG Length,
	__in ULONG Flags,
	__out PVOID *Bcb,
	__deref_out_bcount(Length) PVOID *Buffer
)

//被文件系统调用，从一个文件中获得指定文件偏移开始，获得指定大小的偏移。调用CcMapDataCommon
//通常被用来进行系统中元文件的修改。这些文件是锁在内存中的
//直接将CcGetVirtualAddress返回的Buffer返回给调用者
CcPinFileData (
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN ReadOnly,
	IN BOOLEAN WriteOnly,
	IN ULONG Flags,
	OUT PBCB *Bcb,
	OUT PVOID *BaseAddress,
	OUT PLARGE_INTEGER BeyondLastByte
)

//锁定一块内存，在此之前这块偏移可能无BCB、 可能有BCB却无映射、也可能已经映射

//CcPinRead
//将一段数据锁在内存中

//CcMdlRead
//DMA方式，直接访问物理内存，性能更加提升。
//CcGetVirtualAddress的到的地址，解除掉这段数据在缓存中的映射，建立MDL 返回。

//缓存管理器的预读
//用户进行缓存读操作时（Copy/FastCopy/Mdl）将会调用CcScheduleReadAhead 计算预读位置和长度，构造预读队列 挂入全局链表由辅助线程进行读取
//
//缓存管理器的延迟写
//除非在调用CcCopyRead指明要等待(或者其他调用的类似情况)，写出都是异步的
//CcScheduleLazyWriteScan 设置一个dpc 定时扫描LazyWriteScan CcLazyWriteScan会选择一部分页面进行写出。所有包含脏页面的缓存表CcDirtySharedCacheMapList 之中。

//最终调用CcFlushCache去完成写操作

//Tips    
//缓存管理器是以流文件为对象，对于NTFS这类文件有多个数据流，CC对流进行独立缓存