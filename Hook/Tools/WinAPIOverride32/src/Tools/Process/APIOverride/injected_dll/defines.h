/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originaly based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: common defines
//-----------------------------------------------------------------------------

#pragma once
#include <windows.h>
#pragma intrinsic(memcmp,memcpy,memset)

#define NB_APIOVERRIDE_WORKER_THREADS 2
#define MAX_CONFIG_FILE_LINE_SIZE 2048

#define API_MONITORING_FILE_LOADER_FUNCTION_PARAMETER _T("PVOID pFunction")
#define FIELDS_SEPARATOR            '|' // separator for api function options in monitoring files, MUST BE A SINGLE CHAR
#define FIELDS_SEPARATOR_STRING     _T("|")// separator for api function options in monitoring files, MUST BE A SINGLE CHAR
#define PARAMETER_OPTION_SEPARATOR  ':' // separator for parameter options in monitoring files, MUST BE A SINGLE CHAR
///////////////////////////////////////////////////////
////////////// param direction const //////////////////
///////////////////////////////////////////////////////
#define PARAM_DIR_IN_NAME       _T("IN")
#define PARAM_DIR_OUT_NAME      _T("OUT")
#define PARAM_DIR_INOUT_NAME    _T("INOUT")
#define PARAM_DIR_INNORET_NAME  _T("INNORET")
#define PARAM_DIR_NONE_NAME     _T("NONE")

////////////// Optional keywords //////////////////

// functions options
#define OPTION_BREAK_BEFORE_CALL            _T("BreakBeforeCall")
#define OPTION_BREAK_AFTER_CALL             _T("BreakAfterCall")
#define OPTION_BREAK_BEFORE_AND_AFTER_CALL  _T("BreakBeforeAndAfterCall")
#define OPTION_BREAK_AFTER_CALL_IF_NULL_RESULT _T("BreakAfterCallIfNullRet")
#define OPTION_BREAK_AFTER_CALL_IF_NOT_NULL_RESULT _T("BreakAfterCallIfNotNullRet")
#define OPTION_BREAK_BREAK_ON_FAILURE       _T("BreakOnFailure")
#define OPTION_BREAK_BREAK_ON_SUCCESS       _T("BreakOnSuccess")
#define OPTION_BREAK_LOG_INPUT_AFTER        _T("LogInputAfterBreak")
#define OPTION_BREAK_LOG_OUTPUT_AFTER       _T("LogOutputAfterBreak")


#define OPTION_LOG_IF_NULL_RESULT           _T("LogIfNullRet")
#define OPTION_LOG_IF_NOT_NULL_RESULT       _T("LogIfNotNullRet")
#define OPTION_LOG_ON_FAILURE               _T("LogOnFailure")
#define OPTION_LOG_ON_SUCCESS               _T("LogOnSuccess")

#define OPTION_FAILURE_IF_NULL_RET          _T("FailureIfNullRet")
#define OPTION_FAILURE_IF_NOT_NULL_RET      _T("FailureIfNotNullRet")
#define OPTION_FAILURE_IF_RET_VALUE         _T("FailureIfRetValue=")
#define OPTION_FAILURE_IF_NOT_RET_VALUE     _T("FailureIfRetValue!=")
#define OPTION_FAILURE_IF_NEGATIVE_RET_VALUE            _T("FailureIfNegativeRet")
#define OPTION_FAILURE_IF_POSITIVE_RET_VALUE            _T("FailureIfPositiveRet")
#define OPTION_FAILURE_IF_NULL_FLOATING_RET             _T("FailureIfNullFloatingRet")
#define OPTION_FAILURE_IF_NOT_NULL_FLOATING_RET         _T("FailureIfNotNullFloatingRet")
#define OPTION_FAILURE_IF_FLOATINGRET_VALUE             _T("FailureIfFloatingRetValue=")
#define OPTION_FAILURE_IF_NOT_FLOATING_RET_VALUE        _T("FailureIfFloatingRetValue!=")
#define OPTION_FAILURE_IF_FLOATING_NEGATIVE_RET_VALUE   _T("FailureIfFloatingNegativeRet")
#define OPTION_FAILURE_IF_FLOATING_POSITIVE_RET_VALUE   _T("FailureIfFloatingPositiveRet")

#define OPTION_FAILURE_IF_SIGNED_RET_LESS     _T("FailureIfSignedRet<")
#define OPTION_FAILURE_IF_SIGNED_RET_UPPER    _T("FailureIfSignedRet>")
#define OPTION_FAILURE_IF_UNSIGNED_RET_LESS   _T("FailureIfUnsignedRet<")
#define OPTION_FAILURE_IF_UNSIGNED_RET_UPPER  _T("FailureIfUnsignedRet>")
#define OPTION_FAILURE_IF_FLOATING_RET_LESS   _T("FailureIfFloatingRet<")
#define OPTION_FAILURE_IF_FLOATING_RET_UPPER  _T("FailureIfFloatingRet>")

#define OPTION_FAILURE_IF_LAST_ERROR_VALUE_VALUE      _T("FailureIfLastErrorValue=")
#define OPTION_FAILURE_IF_NOT_LAST_ERROR_VALUE_VALUE  _T("FailureIfLastErrorValue!=")
#define OPTION_FAILURE_IF_LAST_ERROR_VALUE_LESS       _T("FailureIfLastErrorValue<")
#define OPTION_FAILURE_IF_LAST_ERROR_VALUE_UPPER      _T("FailureIfLastErrorValue>")

#define OPTION_BLOCKING_CALL                  _T("BlockingCall")
#define OPTION_DISPLAY_NAME                   _T("DisplayName=")
#define OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE _T("FirstBytesCanExecuteAnywhere=")
#define OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE _T("FirstBytesCanExecuteAnywhere")
#define OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE_SIZE _T("FirstBytesCanExecuteAnywhereWithRelativeAddressChange=")
#define OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE _T("FirstBytesCanExecuteAnywhereWithRelativeAddressChange")
#define OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE _T("FirstBytesCantExecuteAnywhere")
#define OPTION_DONT_CHECK_MODULES_FILTERS _T("DontCheckModulesFilters")

// parameters options
#define OPTION_DATA_SIZE                    _T("DataSize=")
#define OPTION_POINTED_DATA_SIZE            _T("PointedDataSize=")
#define OPTION_POINTED_DATA_SIZE_DEFINED_BY_ANOTHER_ARG _T("Arg")
// parameters conditional logging
#define OPTION_LOG_VALUE                    _T("LogValue=")
#define OPTION_LOG_BUFFER_VALUE             _T("LogBufferValue=") // for struct, double, ... parameters
#define OPTION_LOG_POINTED_VALUE            _T("LogPointedValue=")// hexa byte array
// parameters conditional breaking
#define OPTION_BREAK_VALUE                  _T("BreakValue=")
#define OPTION_BREAK_BUFFER_VALUE           _T("BreakBufferValue=") // for struct, double, ... parameters
#define OPTION_BREAK_POINTED_VALUE          _T("BreakPointedValue=")// hexa byte array
// define name
#define OPTION_DEFINE_VALUES                _T("Define=")


#define ESP_SECURITY_SIZE 40 // this number is the number of args that can be unfortunately forget in config file
// --> esp will be changed of ESP_SECURITY_SIZE*4 bits on 32bits platforms
#define MAX_PARAM         40 // nb max of Api parameters

#define MAX_APIOVERRIDE_MODULESLIMITS 200 // allow to check 199 faking dll --> should be enough

#define UNHOOK_MAX_WAIT_TIME 1000 // max time (in ms) for executing hook, used as max wait time for unhooking
#define UNHOOK_SECURITY_WAIT_TIME_BEFORE_MEMORY_FREEING 50 // time to wait (in ms) to be sur APIHandler has finished after SetEvent(pAPIInfo->evtEndOfHook) call

#define OPCODE_REPLACEMENT_SIZE (1+sizeof(PBYTE))
#define MAX_OPCODE_REPLACEMENT_SIZE 12 // max size in bytes of overwritten bytes (struct align for OPCODE_REPLACEMENT_SIZE)
#define MAX_OPCODES_EXECUTEDATANOTHERPLACE_SIZE 64 // OpcodesExecutedAtAnotherPlace static buffer size
#define REENTER_FUNCTION_FLOW_OPCODE_SIZE (1+sizeof(PBYTE))// size in byte of opcode to jump from OpcodesExecutedAtAnotherPlace/OpcodesExecutedAtAnotherPlaceExtended to next function original byte
#define FIRST_OPCODES_MAX_SIZE  (MAX_OPCODES_EXECUTEDATANOTHERPLACE_SIZE - REENTER_FUNCTION_FLOW_OPCODE_SIZE) // max size in bytes of movables bytes to fit OpcodesExecutedAtAnotherPlace

#define MAX_SECOND_HOOK_SIZE    32  // max size in bytes of second part of the hook

// monitoring parameter direction def
enum tagParamDirection
{
    PARAM_DIR_IN=0,
    PARAM_DIR_OUT,
    PARAM_DIR_INOUT,
    PARAM_DIR_IN_NO_RETURN,
    PARAM_DIR_NONE // for only modify an api without logging it
};
#define DEFAULT_PARAMETER_DIRECTION_SPYING PARAM_DIR_IN

enum tagFakingDllArray
{
    FAKING_DLL_ARRAY_FAKING=0,
    FAKING_DLL_ARRAY_PRE_HOOK,
    FAKING_DLL_ARRAY_POST_HOOK
};