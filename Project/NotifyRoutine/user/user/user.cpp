// user.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "define.h"

int main()
{
    HANDLE        hDevice;     
    bool        status; 
    HANDLE        m_hCommEvent;
    ULONG        dwReturn;
    char        outbuf[255];
    CHECKLIST    CheckList;

    hDevice = NULL;
    m_hCommEvent = NULL;
    hDevice = CreateFile( "\\\\.\\MyEvent",
                    GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                    NULL,
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, 
                    NULL);
    if(hDevice == INVALID_HANDLE_VALUE)
    {
        printf("createfile wrong\n");
        getchar();
        return 0;
    }

    m_hCommEvent = CreateEvent(NULL,
                                  false,
                                  false,
                                  NULL);
    printf("hEvent:%08x\n", m_hCommEvent);

    status =DeviceIoControl(hDevice,
                IOCTL_PASSEVENT,
                &m_hCommEvent,
                sizeof(m_hCommEvent),
                NULL,
                0,
                &dwReturn,
                NULL); 
    if( !status)
    {
        printf("IO wrong+%d\n", GetLastError());
        getchar();
        return 0;
    }

    CheckList.ONLYSHOWREMOTETHREAD=TRUE;
    CheckList.SHOWTHREAD=TRUE;
    CheckList.SHOWTERMINATETHREAD=FALSE;
    CheckList.SHOWTERMINATEPROCESS=FALSE;
    status =DeviceIoControl(hDevice,
                IOCTL_PASSEVSTRUCT,
                &CheckList,
                sizeof(CheckList),
                NULL,
                0,
                &dwReturn,
                NULL); 
    if( !status)
    {
        printf("IO wrong+%d\n", GetLastError());
        getchar();
        return 0;
    }

    printf("      [Process Name]    [PID]    [TID]    [Parent Process Name]    [PID]    [TID]\n");
    while(1)
    {
        ResetEvent(m_hCommEvent);
        WaitForSingleObject(m_hCommEvent, INFINITE);
        status =DeviceIoControl(hDevice,
                    IOCTL_PASSBUF,
                    NULL,
                    0,
                    &outbuf,
                    sizeof(outbuf),
                    &dwReturn,
                    NULL); 
        if( !status)
        {
            printf("IO wrong+%d\n", GetLastError());
            getchar();
            return 0;
        }
        printf("%s", outbuf);
    }

    status =DeviceIoControl(hDevice,
                IOCTL_UNPASSEVENT,
                NULL,
                0,
                NULL,
                0,
                &dwReturn,
                NULL); 
    if( !status)
    {
        printf("UNPASSEVENT wrong+%d\n", GetLastError());
        getchar();
        return 0;
    }

    status = CloseHandle( hDevice );
    status = CloseHandle(m_hCommEvent);
    getchar();
    return 0;
}
