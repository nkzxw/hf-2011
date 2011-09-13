//ROS���ڴ����ͻ�������涼�����������ֻ�нӿ���һ���ģ������ʵ����windows���ྶͥ������������Դ��wrk��internals��
//��������Ƕ����ļ��������ڴ��е�������������ڴ�������ļ�ϵͳ�ṩ����
//�����ڴ������������ҳ������д�����ʱ���ֵ����ļ�ϵͳ�Ľӿ�Ȼ���ߴ����豸������

//��.��Ҫ�ṹ
//1.Virtual Address Control Block �����ַ���ƿ飨���ģ� 
//������ϵͳ�����һ���ۣ�ϵͳ������256kΪһ���ۣ�Ҳ��Ϊһ��view����
//�����ж�д����ʱ�����ݲ��ǻ�ģ�Active���������֮��ӳ�䲻����ʧ�����ǻ�����Ѿ��þ������ǽ���������

//CcVacbs��һ�����飬��¼��ÿ��VACB�ĵ�ַ
typedef struct _VACB {
	PVOID BaseAddress;
	struct _SHARED_CACHE_MAP *SharedCacheMap;        //��ӳ�䵽һ���ļ�ʱ����ָ���ļ��Ĺ������
	union {
		LARGE_INTEGER FileOffset;                    //�����ͼ���ļ��е�ƫ��
		USHORT ActiveCount;                          //���ü���
	} Overlay;                                                
	LIST_ENTRY LruList;                              //�����б����л��߿����õ���ͼ����CcVacbFreeList,CcVacbLru��
} VACB, *PVACB;       

//2. PRIVATE_CACHE_MAP �洢��FILE_OBJECT֮�е�˽�л���� �����ļ�����
typedef struct _PRIVATE_CACHE_MAP {
	union {
	CSHORT NodeTypeCode;
	PRIVATE_CACHE_MAP_FLAGS Flags;
	ULONG UlongFlags;
	};
	ULONG ReadAheadMask;
	PFILE_OBJECT FileObject;
	LARGE_INTEGER FileOffset1;				//����������ζ���λ��
	LARGE_INTEGER BeyondLastByte1;
	LARGE_INTEGER FileOffset2;
	LARGE_INTEGER BeyondLastByte2;
	LARGE_INTEGER ReadAheadOffset[2];
	ULONG ReadAheadLength[2];
	KSPIN_LOCK ReadAheadSpinLock;
	LIST_ENTRY PrivateLinks;
} PRIVATE_CACHE_MAP;

//3. SHARED_CACHE_MAP ����ļ��Ĺ�����������ļ�����
//	 ��FileObject->SectionObjectPointer->SharedCacheMap �б�����ָ��
typedef struct _SHARED_CACHE_MAP {
	����������������������������������
	ULONG OpenCount;				//�򿪼���
	LARGE_INTEGER FileSize;
	LIST_ENTRY BcbList;				//�����ļ�ƫ�Ƶݼ���BCB����
	LARGE_INTEGER SectionSize;
	LARGE_INTEGER ValidDataLength;  //��Ч���ݳ���
	LARGE_INTEGER ValidDataGoal;
	PVACB InitialVacbs[PREALLOCATED_VACBS];  //Ԥ����VACB�������� ��һ��ָ���һ��256k �ڶ���ָ��ڶ��� �����ֵ��Ϊ0 �������һ��VACB 
	PVACB * Vacbs;					//��������飬���ڴ���1M���ڴ棬���ڷǷ�ҳ��������32M��������齫�Ƕ༶ϡ��ġ�
	PFILE_OBJECT FileObject;
	volatile PVACB ActiveVacb;			//ÿ�ζ�д������֮��Ҫ��¼һ��VACB ���´��ٲ�����ͬ����ʱ�������CcGetVirtualAddress
	������������������������������������������������������������������
	LIST_ENTRY PrivateList;				//˽�б������
	����������������������������
	LIST_ENTRY SharedCacheMapLinks;		//ȫ�ֵĹ������
	����������������������������
} SHARED_CACHE_MAP;

//4.Buffer Control Block
//������ƿ飬������һ�鱻�������ڴ棬�ڵ���CcPinFileDataʱ�����δ�������������Ϊ���Ҳ�Dirty��ʱ�򣬱����١�
//��Ϊ��VACB�Ĳ��������
typedef struct _BCB {
	CSHORT NodeTypeCode;   //����
	BOOLEAN Dirty;		   //��δд�ش���
	BOOLEAN Reserved;
	//  Byte FileOffset and and length of entire buffer
	ULONG  ByteLength;          //�ֽڳ���
	LARGE_INTEGER FileOffset;	//ƫ��
	//  Links for BcbList in SharedCacheMap
	LIST_ENTRY BcbLinks;        //������һ������֮�⣬ÿ512K��BCB node����һ��ͷ�����룬��Щͷ������һ�����飬����SharedCacheMap��PVACB����ĺ��棬�༶���ʱ�򣬳���һ���ÿ�㶼�����ͷ������飬���ڿ��
	LARGE_INTEGER BeyondLastByte;  //�����������һ���ֽڵ�ƫ��
	������������������������������..
	//����internals     �ɻָ��ļ�ϵͳ ������д�뵽����ʱ���ṩһ����������ߺ���͵�LSN ���������Ϻ����µ���־�ļ���¼
	LARGE_INTEGER OldestLsn;  
	LARGE_INTEGER NewestLsn;
	������������������������������..
	PVACB Vacb;				//�����ַ���ƿ�
	������������������������������..
	//  ��������
	ULONG PinCount;
	PSHARED_CACHE_MAP SharedCacheMap;
	// ��ַ
	PVOID BaseAddress;
} BCB;

//���ù�ϵ
//Ӧ�ó���ͨ��API����һ��IO������ 
//�� �ļ�ϵͳ�������𴴽��ļ������Լ���صĹ���ṹ��
//�� �����ѱ�ǲ�������ļ� ֱ���ߴ洢���� / �����ļ������CcCopyRead��CcCopyWrite��
//�� ���������MmFlushSection��ʱ������ҳ��������ڴ����������IoPageRead�����ļ�ϵͳ��ֱ���ߴ洢�豸������

CcInitializeCacheMap(
	__in PFILE_OBJECT FileObject,     //�ļ�����
	__in PCC_FILE_SIZES FileSizes,    //��С
	__in BOOLEAN PinAccess,			  //�Ƿ�ʹ����
	__in PCACHE_MANAGER_CALLBACKS Callbacks,        //�ص�Ԥ�����ӳ�д�Ļص�֪ͨ
	__in PVOID LazyWriteContext                     //�ص�������
)

//�ļ�ϵͳ���ã���һ���ļ����л���ʱ���á�
//��������ڴ�������VACB���飬˽�л����ȵȽṹ�Ľ���
CcGetVirtualAddress (
	IN PSHARED_CACHE_MAP SharedCacheMap,
	IN LARGE_INTEGER FileOffset,
	OUT PVACB *Vacb,
	IN OUT PULONG ReceivedLength
)

//����VACB����Ҫ���������ڸ������ļ�ƫ�ƣ��������vacb�����ء�
//���û�У���Ҫ��ȡ����VACB����CcGetVacbMiss�ҵ�һ������vacb���ڻ���������һ��256k���ڴ棬�����ڴ������Ϊ����������ṩ�Ľӿ� MmMapViewInSystemCache ���ڴ��������д�����Ӧ��ԭ��PTE������CcCopyRead�Ѿ����Զ�ȡ�ļ��������ˣ������ڴ������ڴ����������ⲿ���ڴ滻�롣
CcCopyRead (
	__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset,
	__in ULONG Length,
	__in BOOLEAN Wait,
	__out_bcount(Length) PVOID Buffer,
	__out PIO_STATUS_BLOCK IoStatus
)

//��ϵͳ���������û�������֮�俽�����ݣ�����ActiveVacb��SHARED_CACHE_MAP�еĽṹ����
//�����ڵ���CcGetVirtualAddress��Ȼ��RtlCopyMemory
CcCopyWrite (
	__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset,
	__in ULONG Length,
	__in BOOLEAN Wait,
	__in_bcount(Length) PVOID Buffer
)

//�����д���л���ӳ��Ĳ��֣�ͨ���ڴ濽��ȥд��
//���û��ӳ�䣬�����CcMapAndCopy�ȴ���һ��ӳ����ͼ�����ڴ濽����ȥ��ͨ��MmFlushSectionˢ�ش洢�豸�����߱����ҳ�棬ͨ��Mm����Cc����ҳ�滻����
CcMapData (
	__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset,
	__in ULONG Length,
	__in ULONG Flags,
	__out PVOID *Bcb,
	__deref_out_bcount(Length) PVOID *Buffer
)

//���ļ�ϵͳ���ã���һ���ļ��л��ָ���ļ�ƫ�ƿ�ʼ�����ָ����С��ƫ�ơ�����CcMapDataCommon
//ͨ������������ϵͳ��Ԫ�ļ����޸ġ���Щ�ļ��������ڴ��е�
//ֱ�ӽ�CcGetVirtualAddress���ص�Buffer���ظ�������
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

//����һ���ڴ棬�ڴ�֮ǰ���ƫ�ƿ�����BCB�� ������BCBȴ��ӳ�䡢Ҳ�����Ѿ�ӳ��

//CcPinRead
//��һ�����������ڴ���

//CcMdlRead
//DMA��ʽ��ֱ�ӷ��������ڴ棬���ܸ���������
//CcGetVirtualAddress�ĵ��ĵ�ַ���������������ڻ����е�ӳ�䣬����MDL ���ء�

//�����������Ԥ��
//�û����л��������ʱ��Copy/FastCopy/Mdl���������CcScheduleReadAhead ����Ԥ��λ�úͳ��ȣ�����Ԥ������ ����ȫ�������ɸ����߳̽��ж�ȡ
//
//������������ӳ�д
//�����ڵ���CcCopyReadָ��Ҫ�ȴ�(�����������õ��������)��д�������첽��
//CcScheduleLazyWriteScan ����һ��dpc ��ʱɨ��LazyWriteScan CcLazyWriteScan��ѡ��һ����ҳ�����д�������а�����ҳ��Ļ����CcDirtySharedCacheMapList ֮�С�

//���յ���CcFlushCacheȥ���д����

//Tips    
//����������������ļ�Ϊ���󣬶���NTFS�����ļ��ж����������CC�������ж�������