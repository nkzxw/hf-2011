// Gravedigger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Gravedigger.h"
#include "Tlhelp32.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;
HANDLE hProcess;
CString appPath;
CString des;

using namespace std;

DWORD WINAPI ShowInfo(LPVOID lpParameter)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);	//��ȡ����̨���
	while (TRUE)
	{
		if(GetIsPase())
		{
			system("cls");
			printf("%s\r\n", GetAsmStr());					//��ӡ�������Ϣ

			COORD pos;										//��ӡ�Ĵ�����Ϣ
			pos.X=64;
			pos.Y=0;
			SetConsoleCursorPosition(hOut,pos);
			BYTE *szReg = (BYTE *)GetRegisterStr();
			while(*szReg != '\0')
			{
				if(*szReg == '|')
				{
					pos.Y++;
					SetConsoleCursorPosition(hOut,pos);
					(int)szReg++;
				}
				printf("%c", *szReg);
				(int)szReg++;
			}


			pos.X=0;										//��ӡ�ϵ���Ϣ
			pos.Y=12;
			SetConsoleCursorPosition(hOut,pos);
			BYTE *szBreak = (BYTE *)GetBreakStr();
			int nHang = 1;
			while(*szBreak != '\0')
			{
				if(*szBreak == '|')
				{
					pos.Y++;
					SetConsoleCursorPosition(hOut,pos);
					(int)szBreak++;
					nHang++;
					
					if(nHang == 6)
					{
						pos.X=24;
						pos.Y=13;
						SetConsoleCursorPosition(hOut,pos);
					}
				}
				printf("%c", *szBreak);
				(int)szBreak++;
			}

			pos.X=51;										//��ӡջ��Ϣ
			pos.Y=13;
			SetConsoleCursorPosition(hOut,pos);
			BYTE *szStack = (BYTE *)GetStackStr();
			while(*szStack != '\0')
			{
				if(*szStack == '|')
				{
					pos.Y++;
					SetConsoleCursorPosition(hOut,pos);
					(int)szStack++;
				}
				printf("%c", *szStack);
				(int)szStack++;
			}


			pos.X=0;										//��ӡ�ڴ���Ϣ
			pos.Y=17;
			SetConsoleCursorPosition(hOut,pos);
			BYTE *szMem = (BYTE *)GetMemStr();
			while(*szMem != '\0')
			{
				if(*szMem == '|')
				{
					pos.Y++;
					SetConsoleCursorPosition(hOut,pos);
					(int)szMem++;
				}
				printf("%c", *szMem);
				(int)szMem++;
			}
			

			pos.X=0;										//��ӡ������Ϣ
			pos.Y=23;
			SetConsoleCursorPosition(hOut,pos);
			for (int i =0; i<40; i++)
			{
				printf("��");
			}

			pos.X=40;										//��ӡ������Ϣ
			pos.Y=24;
			SetConsoleCursorPosition(hOut,pos);
			printf(GetErr());
			pos.X=0;										//�ظ����λ��
			pos.Y=24;
			SetConsoleCursorPosition(hOut,pos);

			pos.X=72;										//��ӡ״̬��Ϣ
			pos.Y=24;
			SetConsoleCursorPosition(hOut,pos);
			printf("��ͣ");

			pos.X=0;										//��ӡ״̬��Ϣ
			pos.Y=24;
			SetConsoleCursorPosition(hOut,pos);
			SetIsPase(FALSE);
		}
		Sleep(1);
	}

}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);	//��ȡ����̨���
		SMALL_RECT rc = {0,0, 80-1, 26-1};				// ���ô���λ�úʹ�С
		SetConsoleWindowInfo(hOut,true ,&rc);
		
		COORD pos;
		pos.X=28;										//��ӡ����
		pos.Y=11;
		SetConsoleCursorPosition(hOut,pos);
		printf("��Ĺ�� ���԰� Ver0.0.2--By:Wall-E\r\n");

		pos.X=0;										//���ձ����Գ���·��
		pos.Y=24;
		SetConsoleCursorPosition(hOut,pos);
		printf("ѡ�񱻵��Գ���(ȫ·��+��չ��):");

		char a[255];
		Sleep(1000);
		DWORD dwThread = 0;
		HANDLE hDebugThread = CreateThread(NULL,
											0,
											(LPTHREAD_START_ROUTINE)ShowInfo,
											NULL,
											0,
											&dwThread);
		while (TRUE)
		{
			_flushall();
			gets(a);
			SetOpCode(a);
			OpCommend();
		}
	}

	return nRetCode;
}

