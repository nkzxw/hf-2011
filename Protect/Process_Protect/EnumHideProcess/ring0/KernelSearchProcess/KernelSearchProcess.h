
#define  VALID_PAGE 1
#define  INVALID_PAGE 0
#define  PDEINVALID 2
#define  PTEINVALID 3


PVOID PsGetProcessPeb( PEPROCESS pEProcess );
void UnLoad( PDRIVER_OBJECT pDriverObject );
void DisplayInfo();
ULONG IsValidAddr( ULONG uAddr );
BOOLEAN IsRealProcess( ULONG pAddr );
