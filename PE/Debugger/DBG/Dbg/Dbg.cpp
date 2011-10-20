
// Dbg.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"

#define MYDLLAPI extern "C" __declspec(dllexport)

#include <tchar.h>
#include "Dbg.h"
#include <list>


DEBUG_EVENT					stDebugEvent;
PROCESS_INFORMATION			stProcessInfo;

//定义与普通断点相关的变量
struct AddressAndValue
{
	DWORD Address;
	BYTE  Value;
};
typedef std::list<AddressAndValue>::const_iterator LInormal;
std::list<AddressAndValue>						   BreakPoints;

//定义与内存断点相关的变量
struct MemAccess
{
	DWORD Address;
	int OldAttrib;
};
typedef std::list<MemAccess>::const_iterator		LImemory;
std::list<MemAccess>					  			MemBreakPoints;

void AfxMessageBox(LPCWSTR Msg)
{
	MessageBox(NULL, Msg, (LPCWSTR)_T("出错"), MB_OK);
}

bool Check(PROCESS_INFORMATION pi)
{
	if(pi.dwProcessId == 0)
	{
		AfxMessageBox(_T("你也许还没有加载进程"));
		return 0;
	}
	else
	{
		return 1;
	}
}

LInormal FindNormal(DWORD Address)
{//寻找普通断点保存在list里面的位置
	for(LInormal i = BreakPoints.begin(); i != BreakPoints.end(); ++i)
	{
		const AddressAndValue& e = *i;
		if( Address == e.Address )
		{
			return i;
		}
	}
	AfxMessageBox(_T("没有找到Address对应的值"));
	return BreakPoints.begin();
}

LImemory FindMem(DWORD Address)
{//寻找内存断点保存在list里面的位置
	for(LImemory i = MemBreakPoints.begin(); i != MemBreakPoints.end(); ++i)
	{
		const MemAccess& e = *i;
		if( Address == e.Address )
		{
			return i;
		}
	}
	AfxMessageBox(_T("没有找到Address对应的值"));
	return MemBreakPoints.begin();
}

int LoadDebuggedProcess(LPCWSTR FilePath)
{//加载被调试进程
	STARTUPINFO         stStartInfo;

	memset ( &stStartInfo   , NULL , sizeof ( STARTUPINFO         ) ) ;
	memset ( &stProcessInfo , NULL , sizeof ( PROCESS_INFORMATION ) ) ;

	if(!CreateProcess(FilePath, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &stStartInfo, &stProcessInfo))
	{
		int err = GetLastError();
		AfxMessageBox(_T("创建进程失败"));
		return err;
	}

	StopOnDebugEvent(CREATE_PROCESS_DEBUG_EVENT); //在进程加载事件时候挂起
	return 0;
}

int StopOnDebugEvent(DWORD dwDebugEventCode)
{//在指定调试事件上返回

	if(!Check(stProcessInfo))
	{//查看是否加载进程
		return 0;
	}

	bool bState = TRUE;

	while(1)
	{
		bState = WaitForDebugEvent(&stDebugEvent, 5000);
		if(bState != TRUE)
		{
			AfxMessageBox(_T("5秒没有调试事件返回,如果是因为程序执行时间太长,请再次执行StopOnDebugEvent"));
			return 0;
		}
		if( dwDebugEventCode == stDebugEvent.dwDebugEventCode )
		{	
			goto next;				//forgive this goto
		}
		ResumeDebuggedThread();
	}
next:
	return 0;
}

int StopOnException()
{//在异常事件上返回
	return StopOnDebugEvent(EXCEPTION_DEBUG_EVENT);
}

int ResumeDebuggedThread()
{//继续被挂起的调试事件
	if(!Check(stProcessInfo))
	{//查看是否加载进程
		return 0;
	}
	if(!ContinueDebugEvent(stDebugEvent.dwProcessId, stDebugEvent.dwThreadId, DBG_CONTINUE))
	{
		int err = GetLastError();
		AfxMessageBox(_T("继续被调试进程失败"));
		return err;
	}return 0;
}

int ExchangeOneByte(const HANDLE hProcess, const DWORD dwAddress, const BYTE New, BYTE &Old)
{//替代内存里面的数据     Old为替代前的 New 为替代后的
	if(!ReadProcessMemory(hProcess, (LPVOID)dwAddress, &Old, sizeof(BYTE), NULL))
	{
		int err = GetLastError();
		AfxMessageBox(_T("读取内存失败"));
		return err;
	}

	if(!WriteProcessMemory(hProcess, (LPVOID)dwAddress, &New, sizeof(BYTE), NULL))
	{
		int err = GetLastError();
		AfxMessageBox(_T("写入内存失败"));
		return err;
	}

	return 0;
}

int SetNormalBreakPoint(const DWORD dwBreakAddress)
{//普通断点的实现（implement the normal breakpoint）

	if(!Check(stProcessInfo))
	{//查看是否加载进程
		return 0;
	}

	MEMORY_BASIC_INFORMATION MBI;
	BYTE Old;//用于保存被0xCC替代的字节
	PROCESS_INFORMATION *pi = &stProcessInfo;
	AddressAndValue Temp;

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//查询是否有读写权限
		int err = GetLastError();
		AfxMessageBox(_T("也许权限不够"));
		return err;
	}

	if(MBI.Protect == PAGE_EXECUTE_READWRITE)				//forgive me use bad "goto".
		goto pass;				//原谅我用goto

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, PAGE_EXECUTE_READWRITE, &MBI.Protect))
	{//更改成读写权限
		int err = GetLastError();
		AfxMessageBox(_T("请确认你对被调试进程的权限以及被操作内存的访问权限"));
		return err;
	}

pass:
	int err = ExchangeOneByte(pi->hProcess, dwBreakAddress, 0xCC, Old);

	Temp.Value   = Old;
	Temp.Address = dwBreakAddress;
	BreakPoints.push_front(Temp);

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, MBI.Protect, &MBI.Protect))
	{
		AfxMessageBox(_T("恢复读写权限失败"));
	}

	return 0;
}

int DelNormalBreakPoint(DWORD dwBreakAddress)
{//删除普通断点
	MEMORY_BASIC_INFORMATION MBI;
	BYTE Old;//用于保存被0xCC替代的字节
	PROCESS_INFORMATION *pi = &stProcessInfo;

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//查询是否有读写权限
		AfxMessageBox(_T("也许权限不够"));
		return GetLastError();
	}

	if(MBI.Protect == PAGE_EXECUTE_READWRITE)				//forgive me use bad "goto".
		goto pass;

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, PAGE_EXECUTE_READWRITE, &MBI.Protect))
	{//更改成读写权限
		AfxMessageBox(_T("请确认你对被调试进程的权限以及被操作内存的访问权限"));
		return GetLastError();
	}

pass:
	LInormal i = FindNormal(dwBreakAddress);
	const AddressAndValue &Temp = *i;

	int err = ExchangeOneByte(pi->hProcess, dwBreakAddress, Temp.Value, Old);
	BreakPoints.erase( i );

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, MBI.Protect, &MBI.Protect))
	{
		AfxMessageBox(_T("恢复读写权限失败"));
	}

	CONTEXT stCTX;

	stCTX.ContextFlags = CONTEXT_CONTROL;
	if(!GetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		AfxMessageBox(_T("获取线程现场失败"));
		return GetLastError();
	}
	stCTX.Eip = stCTX.Eip - 1;
	if(!SetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		AfxMessageBox(_T("设置线程现场失败"));
		return GetLastError();
	}
	return 0;
}

unsigned long GenerateDr7(int One = 0, int Two = 0, int Three = 0, int Four = 0, int Five = 0, int LowestBit = 0)
{//临时写的计算Dr7的值的函数，One Two …… 代表对应在dr7里面的位，LowestBit是最后一位的值
	unsigned long ResultOne = 1, ResultTwo = 1, ResultThree = 1, ResultFour = 1, ResultFive = 1;
	if(One == 0)
	{
		ResultOne = 0;
	}
	else{
		for(int i = 0;i < One;i++)
		{
			ResultOne = 2*ResultOne;
		}
	}

	if(Two == 0)
	{
		ResultTwo = 0;
	}
	else{
		for(int i = 0;i < Two;i++)
		{
			ResultTwo = 2*ResultTwo;
		}
	}

	if(Three == 0)
	{
		ResultThree = 0;
	}
	else{
		for(int i = 0;i < Three;i++)
		{
			ResultThree = 2*ResultThree;
		}
	}

	if(Four == 0)
	{
		ResultFour = 0;
	}
	else{
		for(int i = 0;i < Four;i++)
		{
			ResultFour = 2*ResultFour;
		}
	}

	if(Five == 0)
	{
		ResultFive = 0;
	}
	else{
		for(int i = 0;i < Five;i++)
		{
			ResultFive = 2*ResultFive;
		}
	}

	return ResultOne + ResultTwo + ResultThree + ResultFour + ResultFive + LowestBit + 1024;
}

int SetHardwareBreakPoint(DWORD dwBreakAddress, int Register, int type)
{/*
 100  全局 1字节 执行断点      1

 101  全局 1字节 写入数据断点  16 1

 103  全局 1字节 读写断点	  17 16 1

 111  全局 2字节 写入数据断点  18 16 1

 113  全局 2字节 读写断点	  18 17 16 1

 131  全局 4字节 写入数据断点  19 18 16 1

 133  全局 4字节 读写断点	  19 18 17 16 1

 上面的1 赋予 LowestBit参数 其他不变

 000  局部 1字节 执行断点	  

 001  局部 1字节 写入数据断点  

 003  局部 1字节 读写断点      

 011  局部 2字节 写入数据断点

 013  局部 2字节 读写断点

 031  局部 4字节 写入数据断点

 033  局部 4字节 读写断点*/

	if(!Check(stProcessInfo))
	{//查看是否加载进程
		return 0;
	}

	CONTEXT stCTX;
	stCTX.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	if(!GetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		int err = GetLastError();
		AfxMessageBox(_T("获取线程现场失败"));
		return err;
	}


	if(Register == 0)
	{
		stCTX.Dr0 = dwBreakAddress;
		switch(type)
		{
		case 100:stCTX.Dr7 = GenerateDr7(1);break;
		case 101:stCTX.Dr7 = GenerateDr7(16, 1);break;
		case 103:stCTX.Dr7 = GenerateDr7(17, 16 ,1);break;
		case 111:stCTX.Dr7 = GenerateDr7(18, 16 ,1);break;
		case 113:stCTX.Dr7 = GenerateDr7(18, 17, 16 ,1);break;
		case 131:stCTX.Dr7 = GenerateDr7(19, 18 , 16, 1);break;
		case 133:stCTX.Dr7 = GenerateDr7(19, 18, 17, 16 ,1);break;
		case 000:stCTX.Dr7 = GenerateDr7(0, 0, 0, 0, 0, 1);break;
		case 001:stCTX.Dr7 = GenerateDr7(16, 1)				- 1;break;
		case 003:stCTX.Dr7 = GenerateDr7(17, 16 ,1)		    - 1;break;
		case 011:stCTX.Dr7 = GenerateDr7(18, 16 ,1)			- 1;break;
		case 013:stCTX.Dr7 = GenerateDr7(18, 17, 16 ,1)     - 1;break;
		case 031:stCTX.Dr7 = GenerateDr7(19, 18 , 16, 1)    - 1;break;
		case 033:stCTX.Dr7 = GenerateDr7(19, 18, 17, 16 ,1) - 1;break;
		}
	}
	else if(Register == 1)
	{
		stCTX.Dr1 = dwBreakAddress;

		switch(type)
		{
		case 100:stCTX.Dr7 = GenerateDr7(3);break;
		case 101:stCTX.Dr7 = GenerateDr7(20, 3);break;
		case 103:stCTX.Dr7 = GenerateDr7(21, 20 ,3);break;
		case 111:stCTX.Dr7 = GenerateDr7(22, 20 ,3);break;
		case 113:stCTX.Dr7 = GenerateDr7(22, 21, 20 ,3);break;
		case 131:stCTX.Dr7 = GenerateDr7(23, 22, 20, 3);break;
		case 133:stCTX.Dr7 = GenerateDr7(23, 22, 21, 20 ,3);break;
		case 000:stCTX.Dr7 = GenerateDr7(2);break;
		case 001:stCTX.Dr7 = GenerateDr7(20, 2);break;
		case 003:stCTX.Dr7 = GenerateDr7(21, 20 ,2);break;
		case 011:stCTX.Dr7 = GenerateDr7(22, 20 ,2);break;
		case 013:stCTX.Dr7 = GenerateDr7(22, 21, 20 ,2);break;
		case 031:stCTX.Dr7 = GenerateDr7(23, 22, 20, 2);break;
		case 033:stCTX.Dr7 = GenerateDr7(23, 22, 21, 20 ,2);break;
		}
	}
	else if(Register == 2)
	{
		stCTX.Dr2 = dwBreakAddress;

		switch(type)
		{
		case 100:stCTX.Dr7 = GenerateDr7(5);break;
		case 101:stCTX.Dr7 = GenerateDr7(24, 5);break;
		case 103:stCTX.Dr7 = GenerateDr7(25, 24 ,5);break;
		case 111:stCTX.Dr7 = GenerateDr7(26, 24 ,5);break;
		case 113:stCTX.Dr7 = GenerateDr7(26, 25, 24, 5);break;
		case 131:stCTX.Dr7 = GenerateDr7(27, 26, 24, 5);break;
		case 133:stCTX.Dr7 = GenerateDr7(27, 26, 25, 24, 5);break;
		case 000:stCTX.Dr7 = GenerateDr7(4);break;
		case 001:stCTX.Dr7 = GenerateDr7(24, 4);break;
		case 003:stCTX.Dr7 = GenerateDr7(25, 24 ,4);break;
		case 011:stCTX.Dr7 = GenerateDr7(26, 24 ,4);break;
		case 013:stCTX.Dr7 = GenerateDr7(26, 25, 24, 4);break;
		case 031:stCTX.Dr7 = GenerateDr7(27, 26, 24, 4);break;
		case 033:stCTX.Dr7 = GenerateDr7(27, 26, 25, 24, 4);break;
		}
	}
	else if(Register == 3)
	{
		stCTX.Dr3 = dwBreakAddress;

		switch(type)
		{
		case 100:stCTX.Dr7 = GenerateDr7(7);break;
		case 101:stCTX.Dr7 = GenerateDr7(28, 7);break;
		case 103:stCTX.Dr7 = GenerateDr7(29, 28 ,7);break;
		case 111:stCTX.Dr7 = GenerateDr7(30, 28 ,7);break;
		case 113:stCTX.Dr7 = GenerateDr7(30, 29, 28, 7);break;
		case 131:stCTX.Dr7 = GenerateDr7(31, 30, 28, 7);break;
		case 133:stCTX.Dr7 = GenerateDr7(31, 30, 29, 27, 7);break;
		case 000:stCTX.Dr7 = GenerateDr7(6);break;
		case 001:stCTX.Dr7 = GenerateDr7(28, 6);break;
		case 003:stCTX.Dr7 = GenerateDr7(29, 28 ,6);break;
		case 011:stCTX.Dr7 = GenerateDr7(30, 28 ,6);break;
		case 013:stCTX.Dr7 = GenerateDr7(30, 29, 28, 6);break;
		case 031:stCTX.Dr7 = GenerateDr7(31, 30, 28, 6);break;
		case 033:stCTX.Dr7 = GenerateDr7(31, 30, 29, 27, 6);break;
		}
	}
	else
	{
		AfxMessageBox(_T("你输入的参数错误"));
		return 0;
	}

	if(!SetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		int err = GetLastError();
		AfxMessageBox(_T("设置线程现场失败"));
		return err;
	}
	return 0;
}

int DelHardwareBreakPoint(int Register)
{
	CONTEXT stCTX;

	stCTX.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	if(!GetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		int err = GetLastError();
		AfxMessageBox(_T("获取线程现场失败"));
		return err;
	}
switch(Register)
{
case 0:stCTX.Dr0 = 0;break;
case 1:stCTX.Dr1 = 0;break;
case 2:stCTX.Dr2 = 0;break;
case 3:stCTX.Dr3 = 0;break;
}
	if(!SetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		int err = GetLastError();
		AfxMessageBox(_T("设置线程现场失败"));
		return err;
	}
	return 0;
}

int SetMemoryBreakPoint(DWORD dwBreakAddress, int type)
{//设置内存断点

	if(!Check(stProcessInfo))
	{//查看是否加载进程
		return 0;
	}

	int Attrib;
	MemAccess Temp;
	int First;		//内存属性的第一个  比如  First|Attrib 作为参数传递
	MEMORY_BASIC_INFORMATION MBI;
	PROCESS_INFORMATION *pi = &stProcessInfo;

	switch(type)
	{
	case 1:Attrib = PAGE_READONLY;break;
	case 2:Attrib = PAGE_NOACCESS;break;
	case 3:Attrib = PAGE_GUARD	 ;break;
	case 4:Attrib = PAGE_READONLY;break;
	}

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//查询权限
		int err = GetLastError();
		AfxMessageBox(_T("也许权限不够"));
		return err;
	}

	First = MBI.Protect;
	if(MBI.Protect == Attrib)
	{
		AfxMessageBox(_T("当前内存即是发生相应异常的状态,不需要设置"));
		goto pass;
	}
	if( Attrib == PAGE_GUARD )
	{
		if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, First|Attrib, &MBI.Protect))
		{
			int err = GetLastError();
			AfxMessageBox(_T("更改内存属性失败"));
			return err;
		}
	}
	else{
		if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, Attrib, &MBI.Protect))
		{
			int err = GetLastError();
			AfxMessageBox(_T("更改内存属性失败"));
			return err;
		}
	}

	Temp.OldAttrib   = MBI.Protect;
	Temp.Address	 = dwBreakAddress;
	MemBreakPoints.push_front(Temp);

pass:
	return 0;
}

int DelMemoryBreakPoint(DWORD dwBreakAddress)
{//删除内存断点
	MEMORY_BASIC_INFORMATION MBI;
	PROCESS_INFORMATION *pi = &stProcessInfo;
	MemAccess Temp;

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//查询权限
		int err = GetLastError();
		AfxMessageBox(_T("也许权限不够"));
		return err;
	}

	LImemory i = FindMem(dwBreakAddress);
	Temp = *i;
	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, Temp.OldAttrib, &MBI.Protect))
	{
		int err = GetLastError();
		AfxMessageBox(_T("更改内存属性失败"));
		return err;
	}
	return 0;
}

DWORD GetNormalBreakPoints(int i)
{//查找int3断点

	if(BreakPoints.empty())
	{
		AfxMessageBox(_T("当前没有普通断点"));
		return 0;
	}
	LInormal LI = BreakPoints.begin();
	for(int j = 1; j != i; ++j)
	{
		LI++;
		if( LI == BreakPoints.end())
			return 0;
	}

	const AddressAndValue &Temp = *LI;
	return Temp.Address;

}