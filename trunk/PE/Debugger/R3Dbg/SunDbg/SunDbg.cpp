// SunDbg.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DoException.h"

//������
int main(int argc, char* argv[])
{
    //ѡ��Ҫ�򿪵�EXE�ļ�
	char            szFileName[MAX_PATH] = "";	
	OPENFILENAME    ofn;
	
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize  = sizeof(OPENFILENAME);
	ofn.lpstrFile	 = szFileName;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrFilter  = "Exe Files(*.exe)\0*.exe\0All Files(*.*)\0*.*\0\0";
	ofn.nFilterIndex = 1;
	if( GetOpenFileName(&ofn) == FALSE)
	{
        return 0;
	}

    //���������Խ���
    char                szDirectoryBuf[MAXBYTE] = {0};
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION pInfo;

    GetStartupInfo(&StartupInfo);
    BOOL isCreateSucess =  CreateProcess(szFileName, NULL, NULL, NULL, TRUE,
                           DEBUG_PROCESS || DEBUG_ONLY_THIS_PROCESS, NULL,
                           NULL, &StartupInfo, &pInfo);
    if (isCreateSucess == FALSE)
    {
        printf("Create debug process failed!\r\n");
        return 1;
    }

    //��ʾһ�°���
    CDoException::ShowHelp(NULL);

    //�������ѭ��
    BOOL        isContinue = TRUE;
    DEBUG_EVENT stuDbgEvent = {0};
    DWORD       dwContinueStatus;
    BOOL        bRet;

    while (isContinue)
    {
        dwContinueStatus = DBG_CONTINUE;
        bRet = WaitForDebugEvent(&stuDbgEvent, INFINITE);
        if (!bRet)
        {
            printf("WaitForDebugEvent error!");
            return 1;
        }

        CDoException::m_stuDbgEvent = stuDbgEvent;
        
        switch (stuDbgEvent.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:
            //�����쳣
            bRet = CDoException::DoException();
            if (bRet == FALSE)
            {
                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
            } 
            break;
        case CREATE_THREAD_DEBUG_EVENT:
            //�����߳�
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            //��������
            //��OEP������һ���Զϵ�
            {
                CDoException::m_ProcessId = stuDbgEvent.dwProcessId;
                CDoException::m_hProcess = stuDbgEvent.u.CreateProcessInfo.hProcess;
                CDoException::m_lpOepAddr = stuDbgEvent.u.CreateProcessInfo.lpStartAddress;
                stuCommand stuCmd = {0};
                wsprintf(stuCmd.chParam1, "%p", 
                         stuDbgEvent.u.CreateProcessInfo.lpStartAddress);
                wsprintf(stuCmd.chParam2, "%s", "once");//һ���Զϵ�
                CDoException::SetOrdPoint(&stuCmd);
                bRet = CloseHandle(stuDbgEvent.u.CreateProcessInfo.hThread);
                if (!bRet)
                {
                    printf("CloseHandle error!");
                    return 1;
                }
            }
            break;
        case EXIT_THREAD_DEBUG_EVENT:
            //�˳��߳�
            break;
        case EXIT_PROCESS_DEBUG_EVENT:
            //�˳�����
            isContinue = FALSE;
            break;
        case LOAD_DLL_DEBUG_EVENT:
            //����DLL
            break;
        case UNLOAD_DLL_DEBUG_EVENT:
            //ж��DLL
            break;
        case OUTPUT_DEBUG_STRING_EVENT:
            break;
        }
        
        bRet = ContinueDebugEvent(stuDbgEvent.dwProcessId, 
            stuDbgEvent.dwThreadId, dwContinueStatus);
        
        if (!bRet)
        {
            printf("ContinueDebugEvent error!");
            return 1;
        }
    }

    //�Ƿ񱣴�ű��ļ�
    if ((MessageBox(NULL, "Need export all user input command to script file?",
        "", MB_YESNO) == IDYES)
       )
    {
        CDoException::ExportScript(NULL);
    }

    //�ͷ�������Դ
    CDoException::ReleaseResource();

    system("pause");
    return 0;
}

