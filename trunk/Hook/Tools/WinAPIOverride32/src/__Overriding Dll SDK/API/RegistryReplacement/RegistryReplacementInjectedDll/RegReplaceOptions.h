#pragma once

#include <windows.h>

#define REGISTRY_REPLACEMENT_OPTIONS_VERSION 1
#define REGISTRY_REPLACEMENT_OPTIONS_FLAG_DON_T_SAVE_REGISTRY 0x1


typedef enum tagFilteringType // specify enum values as they are stored in configuration file
{
    FilteringType_ONLY_BASE_MODULE=0,
    FilteringType_INCLUDE_ONLY_SPECIFIED=1,
    FilteringType_INCLUDE_ALL_BUT_SPECIFIED=2
}REGISTRY_REPLACEMENT_FILTERING_TYPE;

typedef struct tagRegistryConfigurationOptionsA
{
    DWORD Version;
    DWORD Flags;
    CHAR EmulatedRegistryConfigFileAbsolutePath[MAX_PATH];
    REGISTRY_REPLACEMENT_FILTERING_TYPE FilteringType;
    CHAR FilteringTypeFileAbsolutePath[MAX_PATH];
    CHAR OutputFileWhenSpyModeEnabledAbsolutePath[MAX_PATH];
}REGISTRY_REPLACEMENT_OPTIONSA;

typedef struct tagRegistryConfigurationOptionsW
{
    DWORD Version;
    DWORD Flags;
    WCHAR EmulatedRegistryConfigFileAbsolutePath[MAX_PATH];
    REGISTRY_REPLACEMENT_FILTERING_TYPE FilteringType;
    WCHAR FilteringTypeFileAbsolutePath[MAX_PATH];
    WCHAR OutputFileWhenSpyModeEnabledAbsolutePath[MAX_PATH];
}REGISTRY_REPLACEMENT_OPTIONSW;


#if (defined(UNICODE)||defined(_UNICODE))
    typedef REGISTRY_REPLACEMENT_OPTIONSW REGISTRY_REPLACEMENT_OPTIONS;
#else
    typedef REGISTRY_REPLACEMENT_OPTIONSA REGISTRY_REPLACEMENT_OPTIONS;
#endif