#include <windows.h>
#include <stdio.h>
#include "BeaEngine.h"

int main(void)
{
	/* ============================= Init datas */
	DISASM MyDisasm;
	int false = 0, true = 1;
	int len, i = 0;
	_Bool Error = false;

	/* ============================= Init the Disasm structure (important !)*/
	(void) memset (&MyDisasm, 0, sizeof(DISASM));

	/* ============================= Init EIP */
	MyDisasm.EIP = (long long) &main;

	/* ============================= Define Architecture */
	MyDisasm.Archi = 64;

	/* ============================= Loop for Disasm */
	while ((!Error) && (i<20)){
		len = Disasm(&MyDisasm);
		if ((len != OUT_OF_BLOCK) && (len != UNKNOWN_OPCODE)) {
			(void) puts(MyDisasm.CompleteInstr);
			MyDisasm.EIP = MyDisasm.EIP + len;
            i++;
		}
		else {
			Error = true;
		}
	};
	return 0;
}
