
//��������ȡ�����б�
#define DRV_GET_COMMAND_CODE	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define DESCRIBE_MAXLEN 128
#define SYS_FILE_NAME		L"drv_win7"
#define SYS_SYMLINK_NAME	"\\DosDevices\\drv_win7"
#define SYS_DEVICE_NAME		"\\Device\\drv_win7"

//�û�ָ��ṹ
typedef struct _USER_COMMAND 
{
	char cUserCommand ;		//�û�ָ�� 1-9,l,u,r,q
	void* lpFunc ;			//������ַ
	DWORD dwEx ;			//����ֵ���������򽻵���ʱ����Ϊ uCode ʹ��
	WCHAR strDescribe[DESCRIBE_MAXLEN] ;	//ָ�����������128λ
} stUSER_COMMAND, *LPSTUSER_COMMAND;
