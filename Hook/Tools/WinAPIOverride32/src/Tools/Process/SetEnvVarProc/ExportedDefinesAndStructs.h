#include <windows.h> 

#define SetEnvVarProc_SET_ENVIRONMENT_VARIABLES_EXPORTED_FUNC_NAME "_SetEnvironmentVariables@8"
typedef struct tagEnvVarDefinitionW 
{
    WCHAR* Name;
    WCHAR* Value;
}ENV_VAR_DEFINITIONW,*PENV_VAR_DEFINITIONW;

typedef BOOL (__stdcall *pfSetEnvironmentVariables)(ENV_VAR_DEFINITIONW* EnvVarArray,DWORD NbEnvVar);

