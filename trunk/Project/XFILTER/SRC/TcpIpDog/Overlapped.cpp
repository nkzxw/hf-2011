//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//
// 2001-12-24增加重叠操作控制，因为异步重叠操作接收数据时可能不能立即返回
// 从而导致无法立即知道接收数据内容和大小，所以设置回调函数，接收完成时调用。
//
#include "stdafx.h"
#include "overlapped.h"

COverlapped::COverlapped()
{
	InitializeCriticalSection(&m_CriticalSection);
	m_OverlappedRecorder.SetSize(0);
}

//
// 增加一条重叠操作记录
//
BOOL COverlapped::AddOverlapped(
	SOCKET			s,
	LPWSABUF		lpBuffers,
	DWORD			dwBufferCount,
	LPDWORD			lpNumberOfBytesRecvd,
	LPDWORD			lpFlags,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	struct sockaddr FAR * lpFrom,
	LPINT			lpFromlen,
	int				FunctionType //0: WSPRecv; 1:WSPRecvFrom
)
{
	EnterCriticalSection(&m_CriticalSection);
	__try
	{
		OVERLAPPED_RECORDER OverlappedRecorder;
		OverlappedRecorder.s					= s;
		OverlappedRecorder.lpBuffers			= lpBuffers;
		OverlappedRecorder.dwBufferCount		= dwBufferCount;
		OverlappedRecorder.lpNumberOfBytesRecvd = lpNumberOfBytesRecvd;
		OverlappedRecorder.lpFlags				= lpFlags;
		OverlappedRecorder.lpOverlapped			= lpOverlapped;
		OverlappedRecorder.lpCompletionRoutine	= lpCompletionRoutine;
		OverlappedRecorder.lpFrom				= lpFrom;
		OverlappedRecorder.lpFromlen			= lpFromlen;
		OverlappedRecorder.FunctionType			= FunctionType;

		int iIndex = FindOverlapped(lpOverlapped);
		if(iIndex < 0)
		{
			if(m_OverlappedRecorder.Add(OverlappedRecorder) < 0)
				return FALSE;
		}
		else
		{
			m_OverlappedRecorder[iIndex] = OverlappedRecorder;
		}
		return TRUE;
	}
	__finally
	{
		LeaveCriticalSection(&m_CriticalSection);
	}
	return TRUE;
}

//
// 查找重叠操作记录
//
int COverlapped::FindOverlapped(LPWSAOVERLAPPED lpOverlapped)
{
	int i = 0, j = m_OverlappedRecorder.GetSize();
	for(i = 0; i < j; i++)
	{
		if(m_OverlappedRecorder[i].lpOverlapped == lpOverlapped)
			return i;
	}
	return -1;
}

//
// 删除一条重叠操作记录
//
BOOL COverlapped::DeleteOverlapped(int iIndex)
{
	try
	{
		EnterCriticalSection(&m_CriticalSection);
		m_OverlappedRecorder.RemoveAt(iIndex);
		LeaveCriticalSection(&m_CriticalSection);
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}
#pragma comment( exestr, "B9D3B8FD2A717867746E63727267662B")
