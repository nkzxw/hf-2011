////////////////////////////////////////////////
// ProcDrv.h�ļ�


#define IOCTL_NTPROCDRV_GET_PROCINFO    CTL_CODE(FILE_DEVICE_UNKNOWN, \
			0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)



//	�û����ں˽����Ļ�������ʽ������ṹ���û����򷵻ؽ�����Ϣ
typedef struct _CallbackInfo
{
    HANDLE  hParentId;
    HANDLE  hProcessId;
    BOOLEAN bCreate;
}CALLBACK_INFO, *PCALLBACK_INFO;