#ifndef __HOOK_LIB_INTERFACE_2008_07_28__
#define __HOOK_LIB_INTERFACE_2008_07_28__

//////////////////////////////////////////////////////////////////////////
// 欢迎使用本例程
// 如果发现BUG或者用什么意见或者建议请发送邮件到 safejmp@163.com 
//////////////////////////////////////////////////////////////////////////

#define	HOOK_LOAD		1	// 表示加载dll
#define	HOOK_FREE		2	// 表示卸载dll

//////////////////////////////////////////////////////////////////////////
// 接口函数
//////////////////////////////////////////////////////////////////////////

// 打开内核设备
HANDLE WINAPI OpenDevice();

// 关闭内核设备
void WINAPI CloseDevice(HANDLE f_hDev);

// 启动HOOK监视新进程启动并注入 f_pModulePath 指定DLL模块
BOOL WINAPI StartHook(HANDLE f_hDev, PWCHAR f_pModulePath);

// 停止HOOK新进程注入监视
BOOL WINAPI StopHook(HANDLE f_hDev);

// 根据进程PID注入/卸载 f_pModulePath DLL模块到指定的进程
BOOL WINAPI InjectProcessByID(HANDLE f_hDev, ULONG f_Pid, PWCHAR f_pModulePath, ULONG f_Flag);

#endif // __HOOK_LIB_INTERFACE_2008_07_28__