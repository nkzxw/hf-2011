/*
BeaEngine - Disasm
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "..\HEADERS\BeaEngine.h"
/* ============================= Init datas */
DISASM MyDisasm[100];
int len;
long i,FileSize;
_Bool Error = 0;
void *pBuffer;
int  (*pSourceCode) (void); 	/* function pointer */
FILE *FileHandle;


/* ================================================================================= */
/*																									*/
/*						Disassemble code in the specified buffer using the correct VA					*/
/*																									*/
/*==================================================================================*/

void DisassembleCode(char *StartCodeSection, char *EndCodeSection, int (Virtual_Address))
{
	Error = 0;
	MyDisasm[0].EIP = (int) StartCodeSection;
	MyDisasm[0].VirtualAddr = (__int64) Virtual_Address;
	for (i=0;i <100;i++)  {
		MyDisasm[i].Archi = 0;
	}
	i = 0;
	/* ============================= Loop for Disasm */
	while (( !Error) && (i<99)){
		MyDisasm[i].SecurityBlock = (int) EndCodeSection - MyDisasm[i].EIP;
		len = Disasm(&MyDisasm[i]);
		if ((len != OUT_OF_BLOCK) && (len != UNKNOWN_OPCODE)) {
			i++;
			MyDisasm[i].EIP = MyDisasm[i-1].EIP+len;
			MyDisasm[i].VirtualAddr = MyDisasm[i-1].VirtualAddr+len;
            if (MyDisasm[i].EIP >= (long) EndCodeSection)  {
				Error = 1;
            }
		}
		else {
			Error = 1;
		}
	}
	return;
}

/* =================================================================================*/
/*																					*/
/*						Data-flow analysis by scanning MyDisasm[i] table			*/
/*																					*/
/*==================================================================================*/

void AnalyzeCode(void)
{
	for (i=0;i <100;i++) {
		if ((!strcmp(MyDisasm[i-1].Instruction.Mnemonic,"add ")) && (!strcmp(MyDisasm[i].Instruction.Mnemonic,"sub ")))  {
			if (((MyDisasm[i-1].Argument1.ArgType) == (MyDisasm[i].Argument1.ArgType)) && ((MyDisasm[i-1].Argument2.ArgType ) == (MyDisasm[i].Argument2.ArgType ))) {
				(void) printf("\n pattern found in line %d \n\n",i-1);
				(void) printf("%.8X %s\n",(int) MyDisasm[i-1].VirtualAddr, &MyDisasm[i-1].CompleteInstr);
				(void) printf("%.8X %s\n",(int) MyDisasm[i].VirtualAddr, &MyDisasm[i].CompleteInstr);
			}
		}
	}
}



/* ================================================================================= */
/*																									*/
/*												MAIN												*/
/*																									*/
/*==================================================================================*/
int main(void)
{

	FileHandle = fopen("junkcode.bin", "rb");
	(void)fseek(FileHandle,0,SEEK_END);
	FileSize = ftell(FileHandle);
	(void)rewind(FileHandle);
	pBuffer = malloc(FileSize);
	(void)fread(pBuffer,1,FileSize, FileHandle);
	(void)fclose(FileHandle);

	/* ============================= Init the Disasm structure (important !)*/
	for (i=0;i<100;i++) {
		(void) memset (&MyDisasm[i], 0, sizeof(DISASM));
	}


	(void) printf("******************************************************* \n");
	(void) printf("Looking for obfuscation in junkcode.bin - simple pattern :\nadd X, Y\nsub X, Y\n");
	(void) printf("******************************************************* \n");

	/* ============================= Disassemble code located in that buffer */

	DisassembleCode (pBuffer, (char*) pBuffer + 0x600, 0x0);
	AnalyzeCode();


	return 0;
}
