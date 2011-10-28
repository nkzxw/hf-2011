#include <ntddk.h>
#include "..\..\HEADERS\BeaEngine.h"


/* ============================= Init datas */
DISASM MyDisasm;
int len;
_Bool Error = 0;
void *pBuffer;
int (*pSourceCode) (void); 	/* function pointer */

// ===================================================================================
//
//                              Disassemble Routine
//
// ===================================================================================
void DisassembleCode(char *StartCodeSection, char *EndCodeSection, int (*Virtual_Address)(void))
{
	/* ============================= Init the Disasm structure (important !)*/
	(void) memset (&MyDisasm, 0, sizeof(DISASM));
	/* ============================= Init EIP */
	MyDisasm.EIP = (int) StartCodeSection;
	/* ============================= Init VirtualAddr */
	MyDisasm.VirtualAddr = (__int64)(int) Virtual_Address;
	/* ============================= set IA-32 architecture */
	MyDisasm.Archi = 0;
	/* ============================= Loop for Disasm */
	while (!Error){
		/* ============================= Fix SecurityBlock */
		MyDisasm.SecurityBlock = (int) EndCodeSection - MyDisasm.EIP;
		len = Disasm(&MyDisasm);
		if (len == OUT_OF_BLOCK) {
			DbgPrint("disasm engine is not allowed to read more memory \n");
			Error = 1;
		}
		else if (len == UNKNOWN_OPCODE) {
			DbgPrint("unknown opcode");
			Error = 1;
		}
		else {
		    //asm("int $3");
		    DbgPrint("%.8X %s\n", (int) MyDisasm.VirtualAddr, &MyDisasm.CompleteInstr);
			MyDisasm.EIP = MyDisasm.EIP + len;
			MyDisasm.VirtualAddr = MyDisasm.VirtualAddr + len;
			if (MyDisasm.EIP >= (long) EndCodeSection) {
				DbgPrint("End of buffer reached ! \n");
				Error = 1;
			}
		}
	};
	return;
}
// ===================================================================================
//
//                              Driver Unloading - not used here
//
// ===================================================================================
void __attribute__((stdcall)) DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    DbgPrint("Driver unloading\n");
}
// ===================================================================================
//
//                              Driver Entry
//
// ===================================================================================
NTSTATUS __attribute__((stdcall)) DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = DriverUnload;
	pSourceCode =  (void*) &DriverEntry;
	pBuffer = (void*)ExAllocatePool(PagedPool, 100);
	if (pBuffer != 0) {
        DbgPrint("Allocate memory at %.8X \n",pBuffer);
        /* ============================= Let's NOP the buffer */
        (void) memset (pBuffer, 0x90, 100);
        /* ============================= Copy 100 bytes in it */
        (void) memcpy (pBuffer,(void*)(int) pSourceCode, 100);
        /* ============================= Disassemble code located in that buffer */
        DisassembleCode (pBuffer, (char*) pBuffer + 100, pSourceCode);
        ExFreePool(pBuffer);
	}
    else {
        DbgPrint("can't allocate memory \n");
    }
    return STATUS_DEVICE_CONFIGURATION_ERROR;

}

