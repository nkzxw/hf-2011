/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: Provide a small sample of procmon driver and ProcMonInterface class
//
//          this sample will watch process creation and end during 20 seconds
//-----------------------------------------------------------------------------


#include <windows.h>
#include <stdio.h>
#include "../../ProcMonInterface.h"

void ProcessStartCallBack(HANDLE hParentId,HANDLE hProcessId)
{
    printf("Creation of process: ParentID %X, ProcessID %X\r\n",hParentId,hProcessId);
}

void ProcessStopCallBack(HANDLE hParentId,HANDLE hProcessId)
{
    printf("End of process: ParentID %X, ProcessID %X\r\n",hParentId,hProcessId);
}

int main()
{
    CProcMonInterface ProcMonInterface;

    // set call back
    ProcMonInterface.SetProcessStartCallBack(ProcessStartCallBack);
    ProcMonInterface.SetProcessStopCallBack(ProcessStopCallBack);

    // start driver
    ProcMonInterface.StartDriver();

    // start monitoring
    ProcMonInterface.StartMonitoring();


Sleep(20000);

    // stop monitoring
    ProcMonInterface.StopMonitoring();

    // stop driver
    ProcMonInterface.StopDriver();

    return 0;
}
