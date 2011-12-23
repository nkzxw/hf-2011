
//从驱动获取服务列表
#define DRV_GET_COMMAND_CODE	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define DESCRIBE_MAXLEN 128
#define SYS_FILE_NAME		L"drv_win7"
#define SYS_SYMLINK_NAME	"\\DosDevices\\drv_win7"
#define SYS_DEVICE_NAME		"\\Device\\drv_win7"

//用户指令结构
typedef struct _USER_COMMAND 
{
	char cUserCommand ;		//用户指令 1-9,l,u,r,q
	void* lpFunc ;			//函数地址
	DWORD dwEx ;			//附加值，与驱动打交道的时候，作为 uCode 使用
	WCHAR strDescribe[DESCRIBE_MAXLEN] ;	//指令描述，最大128位
} stUSER_COMMAND, *LPSTUSER_COMMAND;
