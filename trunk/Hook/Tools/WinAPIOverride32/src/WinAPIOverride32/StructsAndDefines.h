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

#pragma once

#include "../tools/Process/APIOverride/APIOverride.h"

/////////////////////////////////////////
// Defines
/////////////////////////////////////////
#define MENU_SAVE_ALL                       _T("Save All")
#define MENU_INVERT_SELECTION               _T("Invert Selection")
#define MENU_REMOVE_SELECTED_ITEMS          _T("Remove Selected Logs \tDel")
#define MENU_CLEAR                          _T("Clear Logs")
#define MENU_GET_NONREBASED_VIRTUAL_ADDRESS _T("Get non rebased virtual address")
#define MENU_SHOW_GETLASTERROR_MESSAGE      _T("Show GetLastError error message")
#define MENU_SHOW_RETURN_VALUE_ERROR_MESSAGE _T("Show returned value error message")
#define MENU_ONLINE_MSDN_HELP               _T("Online MSDN Help \tCtrl+W")
#define MENU_GOOGLE_FUNC_NAME               _T("Google Function Name \tCtrl+G")
#define MENU_COPY_FUNC_NAME                 _T("Copy Function Name \tCtrl+C")
#define MSDN_ONLINE_HELP_LINK               _T("http://www.google.com/search?hl=en&q=%s+site%%3Amsdn.microsoft.com&btnI=I%%27m+Feeling+Lucky")
#define MSDN_ONLINE_HELP_DEFAULT_LINK       _T("http://msdn.microsoft.com")
#define GOOGLE_ONLINE_DEFAULT_LINK          _T("http://www.google.com")
#define GOOGLE_ONLINE_LINK                  _T("http://www.google.com/search?hl=en&q=%s")
#define MENU_GOTO_NEXT_FAILURE              _T("Go To Next Failure \tCtrl+N")
#define MENU_GOTO_PREVIOUS_FAILURE          _T("Go To Previous Failure \tCtrl+P")

#define MENU_DISPLAY_DATA_IN_HEX_WINDOW     _T("Show Hex Data")

#define HELP_FILE                           _T("winapioverride.chm")
#define MONITORING_FILE_BUILDER_APP_NAME    _T("MonitoringFileBuilder.exe")


/////////////////////////////////////////
// Structs
/////////////////////////////////////////
typedef struct tagAnalysisThreads
{
    DWORD ThreadId;
    CLinkListSimple* LogList;// contains ordered list of LOG_LIST_ENTRY*
}ANALYSIS_THREADS,*PANALYSIS_THREADS;

typedef struct tagAnalysisProcesses
{
    DWORD ProcessId;
    CLinkList* ThreadList;// list of ANALYSIS_THREADS
}ANALYSIS_PROCESSES,*PANALYSIS_PROCESSES;
