
#ifdef __cplusplus
extern "C"
{
#endif

#include <ntifs.h>
#include <string.h>
#include "FileFilter.h"

#ifdef __cplusplus
};
#endif



#ifdef __cplusplus
extern "C"
{
#endif
NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegEntry)
{

	return FileFilterInit(pDrvObj);

}
#ifdef __cplusplus
};
#endif

