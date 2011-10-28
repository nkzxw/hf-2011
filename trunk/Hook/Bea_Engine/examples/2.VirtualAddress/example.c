/*
BeaEngine - Disasm
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "..\HEADERS\BeaEngine.h"
/* ============================= Init datas */
DISASM MyDisasm;
int len;
_Bool Error = 0;
void *pBuffer;
int  (*pSourceCode) (void); 	/* function pointer */

/* ================================================================================= */
/*																									*/
/*						Disassemble code in the specified buffer using the correct VA					*/
/*																									*/
/*==================================================================================*/

void DisassembleCode(char *StartCodeSection, char *EndCodeSection, int (*Virtual_Address)(void))
{

	/* ============================= Init the Disasm structure (important !)*/
	(void) memset (&MyDisasm, 0, sizeof(DISASM));

	/* ============================= Init EIP */
	MyDisasm.EIP = (long long) StartCodeSection;
	/* ============================= Init VirtualAddr */
	MyDisasm.VirtualAddr = (long long) Virtual_Address;

	/* ============================= set IA-32 architecture */
	MyDisasm.Archi = 0;
	/* ============================= Loop for Disasm */
	while (!Error){
		/* ============================= Fix SecurityBlock */
		MyDisasm.SecurityBlock = (int) EndCodeSection - MyDisasm.EIP;

		len = Disasm(&MyDisasm);
		if (len == OUT_OF_BLOCK) {
			(void) printf("disasm engine is not allowed to read more memory \n");
			Error = 1;
		}
		else if (len == UNKNOWN_OPCODE) {
			(void) printf("unknown opcode \n");
			Error = 1;
		}
		else {
            (void) printf("%.8X %s\n",(int) MyDisasm.VirtualAddr, (char*) &MyDisasm.CompleteInstr);
			MyDisasm.EIP = MyDisasm.EIP + len;
			MyDisasm.VirtualAddr = MyDisasm.VirtualAddr + len;
			if (MyDisasm.EIP >= (int) EndCodeSection) {
				(void) printf("End of buffer reached ! \n");
				Error = 1;
			}
		}
	};
	return;
}

/* ================================================================================= */
/*																									*/
/*												MAIN												*/
/*																									*/
/*==================================================================================*/
int main(void)
{
	pSourceCode =  &main;

	pBuffer = malloc(100);
	/* ============================= Let's NOP the buffer */
	(void) memset (pBuffer, 0x90, 100);

	/* ============================= Copy 100 bytes in it */
	(void) memcpy (pBuffer,(void*)(int) pSourceCode, 100);

	/* ============================= Disassemble code located in that buffer */
	DisassembleCode (pBuffer, (char*) pBuffer + 100, pSourceCode);
	return 0;
}
