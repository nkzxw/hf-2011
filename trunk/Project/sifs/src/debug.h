#ifndef __FILE_FILTER_DEBUG_H__
#define __FILE_FILTER_DEBUG_H__

/*************************************************************************
    Debug tracing information
*************************************************************************/

//
//  Definitions to display log messages.  The registry DWORD entry:
//  "hklm\system\CurrentControlSet\Services\Swapbuffers\DebugFlags" defines
//  the default state of these logging flags
//

#define LOGFL_ERRORS		0x00000001  // if set, display error messages
#define LOGFL_VOLCTX		0x00000002  // if set, display VOLUME_CTX operation info
#define LOGFL_STRCTX		0x00000004  // if set, display STREAM_CTX operation info
#define LOGFL_CREATE		0x00000008  // if set, display CREATE operation info
#define LOGFL_READ			0x00000010  // if set, display READ operation info
#define LOGFL_WRITE			0x00000020  // if set, display WRITE operation info
#define LOGFL_CLOSE			0x00000040
#define LOGFL_TASK			0x00000080
#define LOGFL_REMOTE		0x00000100
#define LOGFL_TASK_RULE	0x00000200
#define LOGFL_VOLUME_RULE	0x00000400
#define LOGFL_DIRCTRL		0x00000800


#define LOG_PRINT( _logFlag, _string )                              \
    (FlagOn(g_FileFltContext.LoggingFlags,(_logFlag)) ?                              \
        DbgPrint _string  :                                         \
        ((int)0))

#endif /* __FILE_FILTER_DEBUG_H__ */