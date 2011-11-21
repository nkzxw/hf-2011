//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/23
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//		This module used to manage the ACL memory. First time, 
//		allocate full size of ACL file, and after, dynamic allocate 
//		page when add ACL's record.
//
//
//

extern DWORD m_dwPacketMonitorThreadId;
extern BOOL m_bIsMonitor;
extern BOOL m_bInMonitor;

extern DWORD			m_SessionCount;
extern PSESSION			m_pSession;
extern PACKET_BUFFER_POINT  m_BufferPoint;
extern DIRECTION_POINT		m_DirectionPoint;

BOOL WINAPI StartMonitor();
BOOL WINAPI StopMonitor();
DWORD WINAPI MonitorPacket(LPVOID pVoid);




extern DWORD m_dwUpdateNnbThreadId;
extern BOOL m_bIsUpdateNnb;
extern BOOL m_bInUpdateNnb;

BOOL WINAPI StartUpdateNnb();
BOOL WINAPI StopUpdateNnb();
DWORD WINAPI UpdateNnbThread(LPVOID pVoid);

BOOL WINAPI EnumerateFunc(LPNETRESOURCE lpnr, CListBox* pList, DWORD dwDisplayType);
BOOL WINAPI GetAddressByHost(LPTSTR sHost, BYTE* pIpBuffer);
void UpdateNnb(LPTSTR pHost);

