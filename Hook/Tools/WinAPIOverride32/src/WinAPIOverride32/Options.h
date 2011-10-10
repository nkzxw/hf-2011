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
// Object: manages winapioverride options
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../Tools/LinkList/LinkList.h"
#include "../tools/Process/APIOverride/ApiOverride.h"// to know number of columns of monitoring listview
#include "../tools/Process/APIOverride/InterProcessCommunication.h" // for tagFirstBytesAutoAnalysis

#define COPTION_OPTION_FILENAME _T("winapioverride.ini")
#define COM_OBJECT_CREATION_HOOKED_FUNCTIONS_DEFAULT_CONFIG_FILENAME _T("COM_ObjectCreationHookedFunctions.txt")
#define COM_OBJECT_HOOKED_CLSID_FILENAME                             _T("COM_HookedCLSID.txt")
#define COM_OBJECT_NOT_HOOKED_CLSID_FILENAME                         _T("COM_NotHookedCLSID.txt")
#define COPTION_OPTION_MODULE_EXCLUSION_FILENAME                     _T("NotHookedModuleList.txt")
#define COPTION_OPTION_MODULE_INCLUSION_FILENAME                     _T("HookedOnlyModuleList.txt")

#define COPTIONS_SECTION_NAME_VISIBLE_COLUMNS       _T("VISIBLE_COLUMNS")
#define COPTIONS_SECTION_NAME_COLUMNS_INDEX         _T("COLUMNS_ORDER")
#define COPTIONS_SECTION_NAME_COLUMNS_SIZE          _T("COLUMNS_SIZE")
#define COPTIONS_SECTION_NAME_USER_INTERFACE        _T("USER_INTERFACE")
#define COPTIONS_KEY_NAME_START_WAY                 _T("START_WAY")
#define COPTIONS_KEY_NAME_STARTUP_APP_PID           _T("STARTUP_APP_PID")
#define COPTIONS_KEY_NAME_STARTUP_APP_PATH          _T("STARTUP_APP_PATH")
#define COPTIONS_KEY_NAME_STARTUP_APP_CMD           _T("STARTUP_APP_CMD")
#define COPTIONS_KEY_NAME_STARTUP_APP_ONLY_AFTER    _T("STARTUP_APP_ONLY_AFTER")
#define COPTIONS_KEY_NAME_STARTUP_APP_ONLY_AFTER_MS _T("STARTUP_APP_ONLY_AFTER_MS")
#define COPTIONS_KEY_NAME_STOP_AND_KILL             _T("STOP_AND_KILL")
#define COPTIONS_KEY_NAME_STOP_AND_KILL_MS          _T("STOP_AND_KILL_MS")
#define COPTIONS_KEY_NAME_ALL_PROCESSES_ONLY_AFTER  _T("ALL_ONLY_AFTER")
#define COPTIONS_KEY_NAME_ALL_PROCESSES_ONLY_AFTER_MS _T("ALL_ONLY_AFTER_MS")
#define COPTIONS_KEY_NAME_ONLY_BASE_MODULE          _T("ONLY_BASE_MODULE")
#define COPTIONS_KEY_NAME_NOT_LOGGED_MODULES_FILE   _T("NOT_LOGGED_MODULES_FILE")
#define COPTIONS_KEY_NAME_LOGGED_ONLY_MODULES_FILE   _T("LOGGED_ONLY_MODULES_FILE")
#define COPTIONS_KEY_NAME_USE_NOT_LOGGED_MODULES_FILE   _T("USE_NOT_LOGGED_MODULES_FILE")
#define COPTIONS_KEY_NAME_USE_FILTER_MODULES_FILE_LIST     _T("USE_FILTER_MODULES_FILE_LIST")
#define COPTIONS_KEY_NAME_FILTERS_APPLY_TO_MONITORING_FILES _T("FILTERS_APPLY_TO_MONITORING_FILES")
#define COPTIONS_KEY_NAME_FILTERS_APPLY_TO_OVERRIDING_DLL _T("FILTERS_APPLY_TO_OVERRIDING_DLL")
#define COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_INCLUSION _T("FILTERS_ALL_PROCESSES_INCLUSION")
#define COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_EXCLUSION _T("FILTERS_ALL_PROCESSES_EXCLUSION")
#define COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_PARENT_PID _T("FILTERS_ALL_PROCESSES_PARENT_PID")
#define COPTIONS_KEY_NAME_EXPORT_ONLY_VISIBLE       _T("EXPORT_ONLY_VISIBLE")
#define COPTIONS_KEY_NAME_EXPORT_ONLY_SELECTED      _T("EXPORT_ONLY_SELECTED")
#define COPTIONS_KEY_NAME_EXPORT_FULL_PARAMETERS_CONTENT _T("EXPORT_FULL_PARAMETERS_CONTENT")
#define COPTIONS_SECTION_NAME_MONITORING_FILES      _T("MONITORING_FILES")
#define COPTIONS_SECTION_NAME_OVERRIDING_DLL        _T("OVERRIDING_DLL")
#define COPTIONS_KEY_NAME_COUNT                     _T("COUNT")
#define COPTIONS_SECTION_NAME_OPTIONS               _T("OPTIONS")
#define COPTIONS_KEY_NAME_AUTOANALYSIS              _T("AUTOANALYSIS")
#define COPTIONS_KEY_NAME_LOG_CALL_STACK            _T("LOG_CALL_STACK")
#define COPTIONS_KEY_NAME_CALL_STACK_PARAMETERS_SIZE _T("CALL_STACK_PARAMETERS_SIZE")
#define COPTIONS_KEY_NAME_MONITORING_FILE_DEBUG_MODE _T("MONITORING_FILE_DEBUG_MODE")
#define COPTIONS_KEY_NAME_BREAK_DONT_BREAK_APIOVERRIDE_THREADS _T("BREAK_DONT_BREAK_APIOVERRIDE_THREADS")
#define COPTIONS_KEY_NAME_ALLOW_TLS_CALLBACK_HOOKS  _T("ALLOW_TLS_CALLBACK_HOOKS")
#define COPTIONS_KEY_NAME_EXE_IAT_HOOKING           _T("EXE_IAT_HOOKING")

// COM
#define COPTIONS_SECTION_NAME_COM_OPTIONS                       _T("COM_OPTIONS")
#define COPTIONS_KEY_NAME_COM_AUTO_HOOKING_ENABLED              _T("COM_AUTO_HOOKING_ENABLED")
#define COPTIONS_KEY_NAME_REPORT_HOOKED_COM_OBJECT              _T("REPORT_HOOKED_COM_OBJECTS")
#define COPTIONS_KEY_NAME_IDISPATCH_AUTO_MONITORING             _T("IDISPATCH_AUTO_MONITORING")
#define COPTIONS_KEY_NAME_QUERY_METHOD_TO_HOOK                  _T("QUERY_METHOD_TO_HOOK")
#define COPTIONS_KEY_NAME_REPORT_CLSID_NOT_SUPPORTING_IDISPATCH _T("REPORT_CLSID_NOT_SUPPORTING_IDISPATCH")
#define COPTIONS_KEY_NAME_REPORT_IID_HAVING_NO_MONITORING_FILE_ASSOCIATED       _T("REPORT_IID_HAVING_NO_MONITORING_FILE_ASSOCIATED")
#define COPTIONS_KEY_NAME_REPORT_USE_NAME_INSTEAD_ID            _T("REPORT_USE_NAME_INSTEAD_ID")
#define COPTIONS_KEY_NAME_CONFIG_FILE_COM_OBJECT_CREATION       _T("CONFIG_FILE_COM_OBJECT_CREATION")
#define COPTIONS_KEY_NAME_CLSID_FILTER_TYPE                     _T("CLSID_FILTER_TYPE")
#define COPTIONS_KEY_NAME_USE_CLSID_FILTER                      _T("USE_CLSID_FILTER")
#define COPTIONS_KEY_NAME_HOOKED_CLSID_FILENAME                 _T("HOOKED_CLSID_FILENAME")
#define COPTIONS_KEY_NAME_NOT_HOOKED_CLSID_FILENAME             _T("NOT_HOOKED_CLSID_FILENAME")

// NET
#define COPTIONS_SECTION_NAME_NET_OPTIONS                       _T("NET_OPTIONS")
#define COPTIONS_KEY_NAME_NET_PROFILING_ENABLED                 _T("NET_PROFILING_ENABLED")
#define COPTIONS_KEY_NAME_NET_AUTO_HOOKING_ENABLED              _T("NET_AUTO_HOOKING_ENABLED")
#define COPTIONS_KEY_NAME_NET_DISABLE_OPTIMIZATION              _T("NET_DISABLE_OPTIMIZATION")
#define COPTIONS_KEY_NAME_NET_MONITOR_EXCEPTIONS                _T("NET_MONITOR_EXCEPTIONS")
#define COPTIONS_KEY_NAME_NET_ENABLE_FRAMEWORK_MONITORING       _T("NET_ENABLE_FRAMEWORK_MONITORING")

#include "IWinApiOverrideOptions.h"

class COptions: public IWinApiOverrideOptions // only for plugins support
{
private:
    void CommonConstructor();
public:
    HOOK_COM_OPTIONS ComOptions;
    HOOK_NET_OPTIONS NetOptions;
// TODO !!!
    CLinkList* MonitoringFilesList;
    CLinkList* OverridingDllList;
// TODO !!!
    typedef struct tagColumnInfos // keep struct order
    {
        SSIZE_T iOrder;
        BOOL bVisible;
        SIZE_T Size;
    }COLUMN_INFOS;
    COLUMN_INFOS ListviewLogsColumnsInfos[CApiOverride::NbColumns];

    COptions();
    COptions(TCHAR* FileName);
    ~COptions(void);

    BOOL Load();
    BOOL Save();
    virtual BOOL Load(TCHAR* OptionsFileName);
    virtual BOOL Save(TCHAR* OptionsFileName);

    // avoid to publish pointer of struct and array to interface
    // else user may want to play with pointer value and as ComOptions and NetOptions are directly used,
    // after modifying pointer value, changes done by user inside plug in will be with no effects
    virtual HOOK_COM_OPTIONS* GetComOptionsPointer()
    {
        return &this->ComOptions;
    }
    virtual HOOK_NET_OPTIONS* GetNetOptionsPointer()
    {
        return &this->NetOptions;
    }
};
