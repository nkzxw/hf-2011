#include "fileflt.h"
#include "task_i.h"

#define NT_TASK_NAME_MAX_LEN 16
/*-------------------------------------------------------------------------------*/

static ULONG g_SystemProcNameOffset = 0;

/*-------------------------------------------------------------------------------*/

int 
TaskGetNameById(
	__in HANDLE TaskId, 
	__out PCHAR TaskName
	)
{
	int ret = -1;

	if(TaskName != NULL){
		
		PCHAR procL = NULL;
					
		if(STATUS_SUCCESS == PsLookupProcessByProcessId(TaskId, (PEPROCESS *)&procL)) {
			
			RtlCopyMemory(TaskName, (procL + g_SystemProcNameOffset), NT_TASK_NAME_MAX_LEN);
			TaskName[NT_TASK_NAME_MAX_LEN] = 0;		

			FsLowerString(TaskName, NT_TASK_NAME_MAX_LEN);
			
			ObDereferenceObject((PEPROCESS)procL);

			ret = 0;
		}
	}

	return ret;
}

int 
TaskCheckTaskByName(
	__in PCHAR Source, 
	__in PCHAR Dest
	)
{
	int ret = -1;

	if((Source != NULL)
		&& (Dest != NULL)){

		LONG  srcLen = strlen(Source);

		if((srcLen == strlen(Dest))
			&& (_strnicmp(Source, Dest, srcLen) == 0) ){

			ret = 0;
		}
	}

	return ret;
}

/*-------------------------------------------------------------------------------*/

static FLT_TASK_STATE
TaskCheckTrustTask(
	__in HANDLE Pid
	)
{
	FLT_TASK_STATE rc = FLT_TASK_STATE_UNKNOWN;
	CHAR task_name[TASK_NAME_MAX_LEN];

	if(Pid == (HANDLE)4) {

		rc = FLT_TASK_STATE_SYSTEM;
		
	}else if(TaskGetNameById(Pid, task_name) == 0) {

		if(TaskCheckTaskByName(task_name, "notepad.exe") == 0){

			rc = FLT_TASK_STATE_TRUST_HOOK;
		}else if(TaskCheckTaskByName(task_name, "explorer.exe") == 0){

			rc = FLT_TASK_STATE_EXPLORE_HOOK;
		}else if(TaskCheckTaskByName(task_name, "uedit32.exe") == 0){

                    rc = FLT_TASK_STATE_TRUST_HOOK;
		}else if(TaskCheckTaskByName(task_name, "insight3.exe") == 0) {

                    rc = FLT_TASK_STATE_TRUST_HOOK;
		}
	}

	return rc;
}

VOID
TaskGetState(
	__inout HANDLE Pid,
	__inout PFLT_TASK_STATE TaskState
	)
{
	if(1) {
		FLT_TASK_STATE retValue = TaskCheckTrustTask(Pid);

		if(retValue != FLT_TASK_STATE_UNKNOWN) {

			*TaskState = retValue;
			return;
		}		
	}	
}

/*-------------------------------------------------------------------------------*/

int
TaskInit(
	VOID
	)
{
	int rc = -1;
	
	g_SystemProcNameOffset = FsGetTaskNameOffset();

	rc = 0;
	
	return rc;
}

void
TaskExit(
	VOID
	)
{
}
