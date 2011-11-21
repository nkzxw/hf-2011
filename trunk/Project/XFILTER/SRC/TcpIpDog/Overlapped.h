#ifndef OVERLAPPED_H
#define OVERLAPPED_H

#include <afxtempl.h>

typedef struct _OVERLAPPED_RECORDER
{
	SOCKET			s;
	LPWSABUF		lpBuffers;
	DWORD			dwBufferCount;
	LPDWORD			lpNumberOfBytesRecvd;
	LPDWORD			lpFlags;
	LPWSAOVERLAPPED lpOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;
	struct sockaddr FAR * lpFrom;
	LPINT			lpFromlen;
	int				FunctionType; //0: WSPRecv; 1:WSPRecvFrom
} OVERLAPPED_RECORDER, *POVERLAPPED_RECORDER;

class COverlapped
{
public:
	COverlapped();

	int FindOverlapped(LPWSAOVERLAPPED lpOverlapped);
	BOOL DeleteOverlapped(int iIndex);
	BOOL AddOverlapped(
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
		);
public:
	CArray<OVERLAPPED_RECORDER, OVERLAPPED_RECORDER> m_OverlappedRecorder;
private:
	CRITICAL_SECTION m_CriticalSection;
};

#endif //OVERLAPPED_H