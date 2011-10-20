
// Dbg.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#include "stdafx.h"

#define MYDLLAPI extern "C" __declspec(dllexport)

#include <tchar.h>
#include "Dbg.h"
#include <list>


DEBUG_EVENT					stDebugEvent;
PROCESS_INFORMATION			stProcessInfo;

//��������ͨ�ϵ���صı���
struct AddressAndValue
{
	DWORD Address;
	BYTE  Value;
};
typedef std::list<AddressAndValue>::const_iterator LInormal;
std::list<AddressAndValue>						   BreakPoints;

//�������ڴ�ϵ���صı���
struct MemAccess
{
	DWORD Address;
	int OldAttrib;
};
typedef std::list<MemAccess>::const_iterator		LImemory;
std::list<MemAccess>					  			MemBreakPoints;

void AfxMessageBox(LPCWSTR Msg)
{
	MessageBox(NULL, Msg, (LPCWSTR)_T("����"), MB_OK);
}

bool Check(PROCESS_INFORMATION pi)
{
	if(pi.dwProcessId == 0)
	{
		AfxMessageBox(_T("��Ҳ��û�м��ؽ���"));
		return 0;
	}
	else
	{
		return 1;
	}
}

LInormal FindNormal(DWORD Address)
{//Ѱ����ͨ�ϵ㱣����list�����λ��
	for(LInormal i = BreakPoints.begin(); i != BreakPoints.end(); ++i)
	{
		const AddressAndValue& e = *i;
		if( Address == e.Address )
		{
			return i;
		}
	}
	AfxMessageBox(_T("û���ҵ�Address��Ӧ��ֵ"));
	return BreakPoints.begin();
}

LImemory FindMem(DWORD Address)
{//Ѱ���ڴ�ϵ㱣����list�����λ��
	for(LImemory i = MemBreakPoints.begin(); i != MemBreakPoints.end(); ++i)
	{
		const MemAccess& e = *i;
		if( Address == e.Address )
		{
			return i;
		}
	}
	AfxMessageBox(_T("û���ҵ�Address��Ӧ��ֵ"));
	return MemBreakPoints.begin();
}

int LoadDebuggedProcess(LPCWSTR FilePath)
{//���ر����Խ���
	STARTUPINFO         stStartInfo;

	memset ( &stStartInfo   , NULL , sizeof ( STARTUPINFO         ) ) ;
	memset ( &stProcessInfo , NULL , sizeof ( PROCESS_INFORMATION ) ) ;

	if(!CreateProcess(FilePath, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &stStartInfo, &stProcessInfo))
	{
		int err = GetLastError();
		AfxMessageBox(_T("��������ʧ��"));
		return err;
	}

	StopOnDebugEvent(CREATE_PROCESS_DEBUG_EVENT); //�ڽ��̼����¼�ʱ�����
	return 0;
}

int StopOnDebugEvent(DWORD dwDebugEventCode)
{//��ָ�������¼��Ϸ���

	if(!Check(stProcessInfo))
	{//�鿴�Ƿ���ؽ���
		return 0;
	}

	bool bState = TRUE;

	while(1)
	{
		bState = WaitForDebugEvent(&stDebugEvent, 5000);
		if(bState != TRUE)
		{
			AfxMessageBox(_T("5��û�е����¼�����,�������Ϊ����ִ��ʱ��̫��,���ٴ�ִ��StopOnDebugEvent"));
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
{//���쳣�¼��Ϸ���
	return StopOnDebugEvent(EXCEPTION_DEBUG_EVENT);
}

int ResumeDebuggedThread()
{//����������ĵ����¼�
	if(!Check(stProcessInfo))
	{//�鿴�Ƿ���ؽ���
		return 0;
	}
	if(!ContinueDebugEvent(stDebugEvent.dwProcessId, stDebugEvent.dwThreadId, DBG_CONTINUE))
	{
		int err = GetLastError();
		AfxMessageBox(_T("���������Խ���ʧ��"));
		return err;
	}return 0;
}

int ExchangeOneByte(const HANDLE hProcess, const DWORD dwAddress, const BYTE New, BYTE &Old)
{//����ڴ����������     OldΪ���ǰ�� New Ϊ������
	if(!ReadProcessMemory(hProcess, (LPVOID)dwAddress, &Old, sizeof(BYTE), NULL))
	{
		int err = GetLastError();
		AfxMessageBox(_T("��ȡ�ڴ�ʧ��"));
		return err;
	}

	if(!WriteProcessMemory(hProcess, (LPVOID)dwAddress, &New, sizeof(BYTE), NULL))
	{
		int err = GetLastError();
		AfxMessageBox(_T("д���ڴ�ʧ��"));
		return err;
	}

	return 0;
}

int SetNormalBreakPoint(const DWORD dwBreakAddress)
{//��ͨ�ϵ��ʵ�֣�implement the normal breakpoint��

	if(!Check(stProcessInfo))
	{//�鿴�Ƿ���ؽ���
		return 0;
	}

	MEMORY_BASIC_INFORMATION MBI;
	BYTE Old;//���ڱ��汻0xCC������ֽ�
	PROCESS_INFORMATION *pi = &stProcessInfo;
	AddressAndValue Temp;

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//��ѯ�Ƿ��ж�дȨ��
		int err = GetLastError();
		AfxMessageBox(_T("Ҳ��Ȩ�޲���"));
		return err;
	}

	if(MBI.Protect == PAGE_EXECUTE_READWRITE)				//forgive me use bad "goto".
		goto pass;				//ԭ������goto

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, PAGE_EXECUTE_READWRITE, &MBI.Protect))
	{//���ĳɶ�дȨ��
		int err = GetLastError();
		AfxMessageBox(_T("��ȷ����Ա����Խ��̵�Ȩ���Լ��������ڴ�ķ���Ȩ��"));
		return err;
	}

pass:
	int err = ExchangeOneByte(pi->hProcess, dwBreakAddress, 0xCC, Old);

	Temp.Value   = Old;
	Temp.Address = dwBreakAddress;
	BreakPoints.push_front(Temp);

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, MBI.Protect, &MBI.Protect))
	{
		AfxMessageBox(_T("�ָ���дȨ��ʧ��"));
	}

	return 0;
}

int DelNormalBreakPoint(DWORD dwBreakAddress)
{//ɾ����ͨ�ϵ�
	MEMORY_BASIC_INFORMATION MBI;
	BYTE Old;//���ڱ��汻0xCC������ֽ�
	PROCESS_INFORMATION *pi = &stProcessInfo;

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//��ѯ�Ƿ��ж�дȨ��
		AfxMessageBox(_T("Ҳ��Ȩ�޲���"));
		return GetLastError();
	}

	if(MBI.Protect == PAGE_EXECUTE_READWRITE)				//forgive me use bad "goto".
		goto pass;

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, PAGE_EXECUTE_READWRITE, &MBI.Protect))
	{//���ĳɶ�дȨ��
		AfxMessageBox(_T("��ȷ����Ա����Խ��̵�Ȩ���Լ��������ڴ�ķ���Ȩ��"));
		return GetLastError();
	}

pass:
	LInormal i = FindNormal(dwBreakAddress);
	const AddressAndValue &Temp = *i;

	int err = ExchangeOneByte(pi->hProcess, dwBreakAddress, Temp.Value, Old);
	BreakPoints.erase( i );

	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, MBI.Protect, &MBI.Protect))
	{
		AfxMessageBox(_T("�ָ���дȨ��ʧ��"));
	}

	CONTEXT stCTX;

	stCTX.ContextFlags = CONTEXT_CONTROL;
	if(!GetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		AfxMessageBox(_T("��ȡ�߳��ֳ�ʧ��"));
		return GetLastError();
	}
	stCTX.Eip = stCTX.Eip - 1;
	if(!SetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		AfxMessageBox(_T("�����߳��ֳ�ʧ��"));
		return GetLastError();
	}
	return 0;
}

unsigned long GenerateDr7(int One = 0, int Two = 0, int Three = 0, int Four = 0, int Five = 0, int LowestBit = 0)
{//��ʱд�ļ���Dr7��ֵ�ĺ�����One Two ���� �����Ӧ��dr7�����λ��LowestBit�����һλ��ֵ
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
 100  ȫ�� 1�ֽ� ִ�жϵ�      1

 101  ȫ�� 1�ֽ� д�����ݶϵ�  16 1

 103  ȫ�� 1�ֽ� ��д�ϵ�	  17 16 1

 111  ȫ�� 2�ֽ� д�����ݶϵ�  18 16 1

 113  ȫ�� 2�ֽ� ��д�ϵ�	  18 17 16 1

 131  ȫ�� 4�ֽ� д�����ݶϵ�  19 18 16 1

 133  ȫ�� 4�ֽ� ��д�ϵ�	  19 18 17 16 1

 �����1 ���� LowestBit���� ��������

 000  �ֲ� 1�ֽ� ִ�жϵ�	  

 001  �ֲ� 1�ֽ� д�����ݶϵ�  

 003  �ֲ� 1�ֽ� ��д�ϵ�      

 011  �ֲ� 2�ֽ� д�����ݶϵ�

 013  �ֲ� 2�ֽ� ��д�ϵ�

 031  �ֲ� 4�ֽ� д�����ݶϵ�

 033  �ֲ� 4�ֽ� ��д�ϵ�*/

	if(!Check(stProcessInfo))
	{//�鿴�Ƿ���ؽ���
		return 0;
	}

	CONTEXT stCTX;
	stCTX.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	if(!GetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		int err = GetLastError();
		AfxMessageBox(_T("��ȡ�߳��ֳ�ʧ��"));
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
		AfxMessageBox(_T("������Ĳ�������"));
		return 0;
	}

	if(!SetThreadContext(stProcessInfo.hThread, &stCTX))
	{
		int err = GetLastError();
		AfxMessageBox(_T("�����߳��ֳ�ʧ��"));
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
		AfxMessageBox(_T("��ȡ�߳��ֳ�ʧ��"));
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
		AfxMessageBox(_T("�����߳��ֳ�ʧ��"));
		return err;
	}
	return 0;
}

int SetMemoryBreakPoint(DWORD dwBreakAddress, int type)
{//�����ڴ�ϵ�

	if(!Check(stProcessInfo))
	{//�鿴�Ƿ���ؽ���
		return 0;
	}

	int Attrib;
	MemAccess Temp;
	int First;		//�ڴ����Եĵ�һ��  ����  First|Attrib ��Ϊ��������
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
	{//��ѯȨ��
		int err = GetLastError();
		AfxMessageBox(_T("Ҳ��Ȩ�޲���"));
		return err;
	}

	First = MBI.Protect;
	if(MBI.Protect == Attrib)
	{
		AfxMessageBox(_T("��ǰ�ڴ漴�Ƿ�����Ӧ�쳣��״̬,����Ҫ����"));
		goto pass;
	}
	if( Attrib == PAGE_GUARD )
	{
		if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, First|Attrib, &MBI.Protect))
		{
			int err = GetLastError();
			AfxMessageBox(_T("�����ڴ�����ʧ��"));
			return err;
		}
	}
	else{
		if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, Attrib, &MBI.Protect))
		{
			int err = GetLastError();
			AfxMessageBox(_T("�����ڴ�����ʧ��"));
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
{//ɾ���ڴ�ϵ�
	MEMORY_BASIC_INFORMATION MBI;
	PROCESS_INFORMATION *pi = &stProcessInfo;
	MemAccess Temp;

	if(!VirtualQueryEx(pi->hProcess, (LPVOID)dwBreakAddress, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
	{//��ѯȨ��
		int err = GetLastError();
		AfxMessageBox(_T("Ҳ��Ȩ�޲���"));
		return err;
	}

	LImemory i = FindMem(dwBreakAddress);
	Temp = *i;
	if(!VirtualProtectEx(pi->hProcess, (LPVOID)dwBreakAddress, 4, Temp.OldAttrib, &MBI.Protect))
	{
		int err = GetLastError();
		AfxMessageBox(_T("�����ڴ�����ʧ��"));
		return err;
	}
	return 0;
}

DWORD GetNormalBreakPoints(int i)
{//����int3�ϵ�

	if(BreakPoints.empty())
	{
		AfxMessageBox(_T("��ǰû����ͨ�ϵ�"));
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