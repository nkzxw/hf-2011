/***************************************************************
模块：Dbg.h
版权：Tweek

Module name: Dbg.h
Copyright (c) 2010 Tweek

Notice:	it was implemented by Tweek when it works nice
		if not, I don't know who wrote it.

***************************************************************/

#ifdef MYDLLAPI

#else 
#define MYDLLAPI extern "C" __declspec(dllimport)
#endif

//被调试进程的相关信息
MYDLLAPI DEBUG_EVENT					stDebugEvent;
MYDLLAPI PROCESS_INFORMATION			stProcessInfo;

//普通断点
//dwBreakAddress是断点地址，pi是指向被调试进程的进程信息的指针
//返回值为0，正常  不为0 即是对应错误码
//功能:在相应的地址，用0xCC替代
MYDLLAPI int SetNormalBreakPoint(DWORD dwBreakAddress);

//删除普通断点
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int DelNormalBreakPoint(DWORD dwBreakAddress);

//获取所有普通断点的地址
//返回的是第i个断点的地址，通过循环可以列出所有int3断点
//返回值即是对应断点地址
MYDLLAPI DWORD GetNormalBreakPoints(int i);

//内存断点
//dwBreakAddress是断点地址
//功能:在相应地址修改操作权限
/*********
type取值：
1：写入断点
2：访问断点  
3：一次性访问断点断点 （设置这种断点后，不需要Del，系统自动在异常后清除）
4：执行断点
********/
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int SetMemoryBreakPoint(DWORD dwBreakAddress, int type);

//删除内存断点
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int DelMemoryBreakPoint(DWORD dwBreakAddress);

//硬件断点
/**********
Register:取值
0:Dr0
1:Dr1
2:Dr2
3:Dr3
type：取值
100  全局 1字节 执行断点

101  全局 1字节 写入数据断点

103  全局 1字节 读写断点

111  全局 2字节 写入数据断点

113  全局 2字节 读写断点

131  全局 4字节 写入数据断点

133  全局 4字节 读写断点

000  局部 1字节 执行断点

001  局部 1字节 写入数据断点

003  局部 1字节 读写断点

011  局部 2字节 写入数据断点

013  局部 2字节 读写断点

031  局部 4字节 写入数据断点

033  局部 4字节 读写断点

I/0端口断点由于我的cpu不支持
还有 8字节的情况，我假设我的cpu 不支持
这两种没有设置
LE GE我cpu不支持，没有设置,GD调试器保护也没有设置
******************************/
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int SetHardwareBreakPoint(DWORD dwBreakAddress,int Register, int type);

//删除硬件断点
/**********
Register:取值
0:Dr0
1:Dr1
2:Dr2
3:Dr3
*************/
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int DelHardwareBreakPoint(int Register);

//加载被调试进程 FilePath 文件路径
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int LoadDebuggedProcess(LPCWSTR FilePath);

//在指定调试事件挂起被调试进程
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int StopOnDebugEvent(DWORD dwDebugEventCode);

//在异常时挂起进程
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int StopOnException();

//继续被挂起的进程
//返回值为0，正常  不为0 即是对应错误码
MYDLLAPI int ResumeDebuggedThread();