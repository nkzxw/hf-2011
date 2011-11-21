//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//
// 2001-12-24�����ص��������ƣ���Ϊ�첽�ص�������������ʱ���ܲ�����������
// �Ӷ������޷�����֪�������������ݺʹ�С���������ûص��������������ʱ���á�
//
#include "stdafx.h"
#include "overlapped.h"

COverlapped::COverlapped()
{
	InitializeCriticalSection(&m_CriticalSection);
	m_OverlappedRecorder.SetSize(0);
}

//
// ����һ���ص�������¼
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
// �����ص�������¼
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
// ɾ��һ���ص�������¼
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
