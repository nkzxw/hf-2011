
NTSTATUS PsLookupThreadByThreadId( ULONG Id, PETHREAD *pEThread );
void UnLoad( PDRIVER_OBJECT pDriverobject );
void DisplayInfo();
ULONG GetSwapContextAddr();

void ShowProcess( PETHREAD pOldThread, PETHREAD pNewThread );