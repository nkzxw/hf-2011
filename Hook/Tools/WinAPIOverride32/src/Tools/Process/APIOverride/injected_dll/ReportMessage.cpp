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
// Object: manages the report of messages to main application
//-----------------------------------------------------------------------------

#include "reportmessage.h"

extern CMailSlotClient* pMailSlotClt;
extern LARGE_INTEGER PerformanceFrequency;
extern LARGE_INTEGER ReferenceCounter;
extern LARGE_INTEGER ReferenceTime;
// extern HANDLE ApiOverrideHeap;

//-----------------------------------------------------------------------------
// Name: ReportMessage
// Object: check if LoadFunctions has been successfully called
// Parameters :
//      in: tagReportMessageType ReportMessageType : type of message : info, warning or error
//          TCHAR* pszMsg : string containing message to be displayed
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL __stdcall CReportMessage::ReportMessage(tagReportMessageType ReportMessageType,TCHAR* pszMsg)
{
    // message struct :
    // DWORD : CMD_REPORT_MESSAGE 
    // DWORD : tagReportMessageType
    // FILETIME : Current Time
    // DWORD : string length including \0 in bytes count
    // string including \0
    BOOL bResult;
    PBYTE Buffer;
    DWORD dw;
    DWORD MessageSize;
    DWORD StringLength;
    int Index;
    LARGE_INTEGER TickCount;
    FILETIME FileTime;

    // do not use the following but the same algo used in ApiHandler to get same time base
    //// get UTC time
    //GetSystemTimeAsFileTime(&FileTime);
    //// convert UTC time to local filetime
    //FileTimeToLocalFileTime(&FileTime,&FileTime);

    // until GetSystemTimeAsFileTime returned value is updated every ms

    // get execution time (try to call it as soon as possible for log ordering)
    QueryPerformanceCounter(&TickCount);
    // compute number of 100ns (PerformanceFrequency is in count per second)
    TickCount.QuadPart=((TickCount.QuadPart-ReferenceCounter.QuadPart)*1000*1000*10)/PerformanceFrequency.QuadPart;
    TickCount.QuadPart+=ReferenceTime.QuadPart;
    FileTime.dwHighDateTime=(DWORD)TickCount.HighPart;
    FileTime.dwLowDateTime=(DWORD)TickCount.LowPart;



    StringLength=(DWORD)(_tcslen(pszMsg)+1)*sizeof(TCHAR);
    MessageSize=3*sizeof(DWORD)+StringLength+sizeof(FILETIME);

    // allocate buffer to send full data throw mail slot
    // Buffer=(PBYTE)HeapAlloc(ApiOverrideHeap,0,MessageSize);
    Buffer=(PBYTE)_alloca(MessageSize);
    if (!Buffer)
        return FALSE;
    
    // cast buffer to DWORD array for easily storing first DWORD value
    Index=0;
    dw=CMD_REPORT_MESSAGE;
    memcpy(Buffer,&dw,sizeof(DWORD));
    Index+=sizeof(DWORD);

    dw=ReportMessageType;
    memcpy(&Buffer[Index],&dw,sizeof(DWORD));
    Index+=sizeof(DWORD);

    memcpy(&Buffer[Index],&FileTime,sizeof(FILETIME));
    Index+=sizeof(FILETIME);

    memcpy(&Buffer[Index],&StringLength,sizeof(DWORD));
    Index+=sizeof(DWORD);

    // copy message content
    memcpy(&Buffer[Index],pszMsg,StringLength);
    
    // transmit struct to remote process
    bResult=pMailSlotClt->Write(Buffer, MessageSize);

    // free allocated memory
    // HeapFree(ApiOverrideHeap,0,Buffer);

    return bResult;
}