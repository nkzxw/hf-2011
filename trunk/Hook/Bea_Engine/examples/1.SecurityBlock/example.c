#include <windows.h>
#include <stdio.h>
#include "..\HEADERS\BeaEngine.h"

int main(void)
{
	/* ============================= Init datas */
	DISASM MyDisasm;
	int false = 0, true = 1;
	int len;
	_Bool Error = false;
	int EndCodeSection = 0x401020;

	/* ============================= Init the Disasm structure (important !)*/
	(void) memset (&MyDisasm, 0, sizeof(DISASM));

	/* ============================= Init EIP */
	MyDisasm.EIP = 0x401000;

	/* ============================= Loop for Disasm */
	while (!Error){
		/* ============================= Fix SecurityBlock */
		MyDisasm.SecurityBlock = EndCodeSection - MyDisasm.EIP;

		len = Disasm(&MyDisasm);
		if (len == OUT_OF_BLOCK) {
			(void) printf("disasm engine is not allowed to read more memory \n");
			Error = true;
		}
		else if (len == UNKNOWN_OPCODE) {
			(void) printf("unknown opcode");
			Error = true;
		}
		else {
			(void) puts(MyDisasm.CompleteInstr);
			MyDisasm.EIP = MyDisasm.EIP + len;
			if (MyDisasm.EIP >= EndCodeSection) {
				(void) printf("End of buffer reached ! \n");
				Error = true;
			}
		}
	};
	return 0;
}
