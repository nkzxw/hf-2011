
#pragma once

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

enum tagUserMessagesTypes // same values as COMMonitoring file generator
{
    USER_MESSAGE_INFORMATION=0,
    USER_MESSAGE_WARNING,
    USER_MESSAGE_ERROR
};

typedef void (*tagUserMessageInformationCallBack)(TCHAR* Message,
                                                  tagUserMessagesTypes MessageType,
                                                  LPVOID UserParam);