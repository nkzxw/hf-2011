#pragma once

#if !defined(_WIN32_WINNT) || _WIN32_WINNT!=0x5000
#define _WIN32_WINNT 0x5000
#endif

#include <Windows.h>
#include <WinIoCtl.h>

typedef struct { 
    ULONG Type;  //4B  'ELIF','XDNI','DAAB','ELOH','DKHC'  
    USHORT UsaOffset;  //2B �������к�����ƫ�ƣ�У��ֵ��ַ
    USHORT UsaCount;  //1+n 1ΪУ��ֵ���� nΪ���滻ֵ����  fixup
    USN Usn; //8B ÿ�μ�¼���޸� USN���仯
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER; 

typedef struct { //sizeof(FILE_RECORD_HEADER)==48
    NTFS_RECORD_HEADER Ntfs;  //16B Ntfs.Type����'ELIF'
    USHORT SequenceNumber; //File Reference Number�ĸ�16λ  i�ǵ�48λ 0~total_file_count-1
    //���ڼ�¼���ļ����¼���ظ�ʹ�õĴ���
    USHORT LinkCount; //��¼Ӳ���ӵ���Ŀ��ֻ�����ڻ����ļ���¼��
    USHORT AttributeOffset; //��һ�����Ե�ƫ��
    USHORT Flags;       // 0x0001 = InUse, 0x0002 = Directory 
    ULONG BytesInUse;  //��¼ͷ�����Ե��ܳ��ȣ����ļ���¼��ʵ�ʳ����ļ�,����¼�ڴ�����ʵ��ռ�õ��ֽڿռ䡣
    ULONG BytesAllocated; //�ܹ��������¼�ĳ���
    ULONGLONG BaseFileRecord; //�����ļ���¼�е��ļ������ţ�
    //���ڻ����ļ���¼����ֵΪ0�������Ϊ0������һ�����ļ�����ļ������ţ�
    //ָ�������Ļ����ļ���¼�е��ļ���¼�ţ�
    //�ڻ����ļ���¼�а�������չ�ļ���¼����Ϣ���洢�ڡ������б�ATTRIBUTE_LIST�������С�
    USHORT NextAttributeNumber; //��һ����ID
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER; 


//�ļ���¼�а����Էǽ�����Ϊ��ʱ���������ͬ�����ԣ����У�
//���Ե�ֵ��Ϊ �ֽ���
typedef enum {
    //����ֻ�����浵���ļ����ԣ�
    //ʱ������ļ�����ʱ�����һ���޸�ʱ��
    //����Ŀ¼ָ����ļ���Ӳ���Ӽ���hard link count��
    AttributeStandardInformation = 0x10, //Resident_Attributes ��פ����

    //?????????????????????????????????
    //��һ���ļ�Ҫ����MFT�ļ���¼ʱ ���и�����
    //�����б��������ɸ��ļ�����Щ���ԣ��Լ�ÿ���������ڵ�MFT�ļ���¼���ļ�����
    //?????????????????????????????????
    AttributeAttributeList = 0x20,//��������ֵ���ܻ������������Ƿ�פ������

    //�ļ������Կ����ж����
    //1.���ļ����Զ�Ϊ����ļ���(�Ա�MS-DOS��16λ�������)
    //2.�����ļ�����Ӳ����ʱ
    AttributeFileName = 0x30, //��פ

    //һ���ļ���Ŀ¼��64�ֽڱ�ʶ�������е�16�ֽڶ��ڸþ���˵��Ψһ��
    //����-���ٷ��񽫶���ID�������ǿ�ݷ�ʽ��OLE����Դ�ļ���
    //NTFS�ṩ����Ӧ��API����Ϊ�ļ���Ŀ¼����ͨ�������ID��������ͨ�����ļ�����
    AttributeObjectId = 0x40, //��פ

    //Ϊ��NTFS��ǰ�汾����������
    //���о�����ͬ��ȫ���������ļ���Ŀ¼����ͬ���İ�ȫ����
    //��ǰ�汾��NTFS��˽�еİ�ȫ��������Ϣ��ÿ���ļ���Ŀ¼�洢��һ��
    AttributeSecurityDescriptor = 0x50,//������$SecureԪ�����ļ���

    //�����˸þ�İ汾��label��Ϣ
    AttributeVolumeName = 0x60, //��������$VolumeԪ�����ļ���
    AttributeVolumeInformation = 0x70,//��������$VolumeԪ�����ļ���

    //�ļ����ݣ�һ���ļ�����һ��δ�������������ԣ������ж�����������������
    //��һ���ļ������ж����������Ŀ¼û��Ĭ�ϵ��������ԣ������ж����ѡ����������������
    AttributeData = 0x80,//��������ֵ���ܻ������������Ƿ�פ������

    //������������ʵ�ִ�Ŀ¼���ļ��������λͼ����
    AttributeIndexRoot = 0x90,//��פ
    AttributeIndexAllocation = 0xA0,
    AttributeBitmap = 0xB0,

    //�洢��һ���ļ����ؽ��������ݣ�NTFS�Ľ���(junction)�͹��ص����������
    AttributeReparsePoint = 0xC0,

    //��������Ϊ��չ���ԣ����Ѳ��ٱ�����ʹ�ã�֮�����ṩ��Ϊ��OS/2���򱣳�������
    AttributeEAInformation = 0xD0,
    AttributeEA = 0xE0,

    AttributePropertySet = 0xF0,
    AttributeLoggedUtilityStream = 0x100,
    AttributeEnd=0xFFFFFFFF
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct { 
    ATTRIBUTE_TYPE AttributeType; 
    ULONG Length; //�����Գ��ȣ���������ֵ��
    BOOLEAN Nonresident; //�����Բ��� פ�� ����ô��
    UCHAR NameLength; //�����������Ƴ���
    USHORT NameOffset;//������ƫ��
    USHORT Flags; // 0x0001 ѹ�� 0x4000 ���� 0x8000ϡ���ļ� 
    USHORT AttributeNumber; 
} ATTRIBUTE, *PATTRIBUTE; 

typedef struct { 
    ATTRIBUTE Attribute; 
    ULONG ValueLength; //����ֵ����
    USHORT ValueOffset; //����ֵƫ��
    USHORT Flags; // ������־ 0x0001 = Indexed 
} RESIDENT_ATTRIBUTE, *PRESIDENT_ATTRIBUTE; 

typedef struct {
    ATTRIBUTE Attribute; 
    ULONGLONG LowVcn;
    ULONGLONG HighVcn;
    USHORT RunArrayOffset;
    UCHAR CompressionUnit;
    UCHAR AlignmentOrReserved[5];
    ULONGLONG AllocatedSize;
    ULONGLONG DataSize;
    ULONGLONG InitializedSize;
    ULONGLONG CompressedSize;    // Only when compressed
} NONRESIDENT_ATTRIBUTE, *PNONRESIDENT_ATTRIBUTE;

typedef struct { //�ļ������Ե�ֵ����
    ULONGLONG DirectoryFileReferenceNumber; //��Ŀ¼��FRN 
    ULONGLONG CreationTime; 
    ULONGLONG ChangeTime; 
    ULONGLONG LastWriteTime; // ���һ��MFT����ʱ��
    ULONGLONG LastAccessTime; 
    ULONGLONG AllocatedSize; // δ��
    ULONGLONG DataSize; // ż�����ļ���СGetFileSize��ͬ
    ULONG FileAttributes; 
    ULONG AlignmentOrReserved; 
    UCHAR NameLength; 
    UCHAR NameType; //POSIX 0x0  WIN32 0x01  DOS 0x02  WIN32&DOS 0x3
    WCHAR Name[1]; 
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE; 

typedef struct {
    ATTRIBUTE_TYPE AttributeType;   //��������
    USHORT Length;                  //����¼����
    UCHAR NameLength;               //����������
    UCHAR NameOffset;               //������ƫ��
    ULONGLONG LowVcn;               //��ʼVCN
    ULONGLONG FileReferenceNumber;  //���Ե��ļ��ο���
    USHORT AttributeNumber;         //��ʶ
    WCHAR Name[1];
} ATTRIBUTE_LIST, *PATTRIBUTE_LIST; 

#pragma pack(push,1)
typedef struct { //512B
    UCHAR Jump[3];//����3���ֽ�
    UCHAR Format[8]; //��N��'T' 'F' 'S' 0x20 0x20 0x20 0x20
    USHORT BytesPerSector;//ÿ�����ж����ֽ� һ��Ϊ512B 0x200
    UCHAR SectorsPerCluster;//ÿ���ж��ٸ�����
    USHORT BootSectors;//
    UCHAR Mbz1;//����0
    USHORT Mbz2;//����0
    USHORT Reserved1;//����0
    UCHAR MediaType;//������������Ӳ��Ϊ0xf8
    USHORT Mbz3;//��Ϊ0
    USHORT SectorsPerTrack;//ÿ����������һ��Ϊ0x3f
    USHORT NumberOfHeads;//��ͷ��
    ULONG PartitionOffset;//�÷����ı��ˣ����÷���ǰ������������ һ��Ϊ�ŵ�������0x3f 63��
    ULONG Reserved2[2];
    ULONGLONG TotalSectors;//�÷�����������
    ULONGLONG MftStartLcn;//MFT�����ʼ�غ�LCN
    ULONGLONG Mft2StartLcn;//MFT���ݱ����ʼ�غ�LCN
    ULONG ClustersPerFileRecord;//ÿ��MFT��¼����������  ��¼���ֽڲ�һ��Ϊ��ClustersPerFileRecord*SectorsPerCluster*BytesPerSector
    ULONG ClustersPerIndexBlock;//ÿ��������Ĵ���
    ULONGLONG VolumeSerialNumber;//�����к�
    UCHAR Code[0x1AE];
    USHORT BootSignature;
} BOOT_BLOCK, *PBOOT_BLOCK;
#pragma pack(pop)


void Helper_GetBasicInformation(
                                DWORD iDri//�̺�
                                ,DWORD dwFrn
                                ,DWORDLONG *pOutTimeCreate
                                ,DWORDLONG *pOutTimeAccess
                                ,DWORDLONG *pOutTimeChange
                                ,DWORDLONG *pOutSize
                                ,DWORD *pOutAttr
                                );