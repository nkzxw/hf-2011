#ifndef __HOOK_LIB_INTERFACE_2008_07_28__
#define __HOOK_LIB_INTERFACE_2008_07_28__

//////////////////////////////////////////////////////////////////////////
// ��ӭʹ�ñ�����
// �������BUG������ʲô������߽����뷢���ʼ��� safejmp@163.com 
//////////////////////////////////////////////////////////////////////////

#define	HOOK_LOAD		1	// ��ʾ����dll
#define	HOOK_FREE		2	// ��ʾж��dll

//////////////////////////////////////////////////////////////////////////
// �ӿں���
//////////////////////////////////////////////////////////////////////////

// ���ں��豸
HANDLE WINAPI OpenDevice();

// �ر��ں��豸
void WINAPI CloseDevice(HANDLE f_hDev);

// ����HOOK�����½���������ע�� f_pModulePath ָ��DLLģ��
BOOL WINAPI StartHook(HANDLE f_hDev, PWCHAR f_pModulePath);

// ֹͣHOOK�½���ע�����
BOOL WINAPI StopHook(HANDLE f_hDev);

// ���ݽ���PIDע��/ж�� f_pModulePath DLLģ�鵽ָ���Ľ���
BOOL WINAPI InjectProcessByID(HANDLE f_hDev, ULONG f_Pid, PWCHAR f_pModulePath, ULONG f_Flag);

#endif // __HOOK_LIB_INTERFACE_2008_07_28__