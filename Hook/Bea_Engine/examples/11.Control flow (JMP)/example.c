/*
BeaEngine - Disasm
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "..\HEADERS\BeaEngine.h"
/* ============================= Init datas */
DISASM MyDisasm;
int len,i,FileSize;
_Bool Error = 0;
unsigned char *pBuffer;
int  (*pSourceCode) (void); 	/* function pointer */
FILE *FileHandle;

int RVA2OFFSET(int Address, unsigned char *pBuff);

/* =================================================================================*/
/*																					*/
/*				Disassemble code in the specified buffer using the correct VA		*/
/*																					*/
/*==================================================================================*/

void DisassembleCode(unsigned char *StartCodeSection, unsigned char *EndCodeSection, int (Virtual_Address))
{
	Error = 0;
	MyDisasm.EIP = (long long) StartCodeSection;
	MyDisasm.VirtualAddr = (long long) Virtual_Address;
	MyDisasm.Archi = 0;

	/* ============================= Loop for Disasm */
	while (!Error){
		MyDisasm.SecurityBlock = (int) EndCodeSection - MyDisasm.EIP;
		len = Disasm(&MyDisasm);
		if ((len != OUT_OF_BLOCK) && (len != UNKNOWN_OPCODE)) {
            (void) printf("%.8X %s\n",(int) MyDisasm.VirtualAddr, &MyDisasm.CompleteInstr);
			MyDisasm.EIP = MyDisasm.EIP+len;
            MyDisasm.VirtualAddr = MyDisasm.VirtualAddr+len;
			if (MyDisasm.EIP >= (long) EndCodeSection)  {
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
/*						eliminate JUMPS  and reorder instructions  					*/
/*																					*/
/*==================================================================================*/

void DisassembleCodeFilter(unsigned char *StartCodeSection, unsigned char *EndCodeSection, int (Virtual_Address))
{
	Error = 0;
	MyDisasm.EIP = (long long) StartCodeSection;
	MyDisasm.VirtualAddr = (long long) Virtual_Address;
	MyDisasm.Archi = 0;

	/* ============================= Loop for Disasm */
	while (!Error){
		MyDisasm.SecurityBlock = (int) EndCodeSection - MyDisasm.EIP;
		len = Disasm(&MyDisasm);
		if ((len != OUT_OF_BLOCK) && (len != UNKNOWN_OPCODE)) {
			if ((MyDisasm.Instruction.BranchType == JmpType) && (MyDisasm.Instruction.AddrValue != 0)) {
			    MyDisasm.EIP = RVA2OFFSET((int) MyDisasm.Instruction.AddrValue - 0x400000,pBuffer);
			    MyDisasm.VirtualAddr = MyDisasm.Instruction.AddrValue;
			}
			else {
                (void) printf("%.8X %s\n",(int) MyDisasm.VirtualAddr, &MyDisasm.CompleteInstr);
			    MyDisasm.EIP = MyDisasm.EIP+len;
                MyDisasm.VirtualAddr = MyDisasm.VirtualAddr+len;
			}
            if (MyDisasm.EIP >= (long) EndCodeSection)  {
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
/*	        Convert Relative Virtual Address to offset in the file                  */
/*          works fine even in naughty binaries										*/
/*			BeatriX manufacture :)													*/
/*																					*/
/*==================================================================================*/

int RVA2OFFSET(int RVA, unsigned char *pBuff)
{
int RawSize, VirtualBorneInf, RawBorneInf, SectionHeader;
int OffsetNtHeaders,OffsetSectionHeaders, NumberOfSections, SizeOfOptionalHeaders, VirtualAddress;

    OffsetNtHeaders = (int) *((int*) (pBuff + 0x3c));
    NumberOfSections = (int) *((unsigned short*) (pBuff + OffsetNtHeaders + 6));
    SizeOfOptionalHeaders = (int) *((unsigned short*) (pBuff + OffsetNtHeaders + 0x14));
    OffsetSectionHeaders = OffsetNtHeaders + SizeOfOptionalHeaders + 0x18;
    VirtualBorneInf = 0;
    RawBorneInf = 0;
    VirtualAddress = 0;
    SectionHeader = 0;
    while (VirtualAddress <= RVA) {
        if (VirtualAddress != 0) {
            VirtualBorneInf = VirtualAddress;
            RawSize = (int) *((unsigned int*) (pBuff + OffsetSectionHeaders + 0x10));
            RawBorneInf = (int) *((unsigned int*) (pBuff + OffsetSectionHeaders + 0x14));
        }
        VirtualAddress = (int) *((unsigned int*) (pBuff + OffsetSectionHeaders + SectionHeader*0x28 + 0x0C));
        SectionHeader ++;
    }
    if ((RVA-VirtualBorneInf)>RawSize) return -1;
    RawBorneInf = RawBorneInf >> 8;
    if (RawBorneInf & 1) RawBorneInf--;
    RawBorneInf = RawBorneInf << 8;
    return RVA - VirtualBorneInf + RawBorneInf + (int) pBuff;
}
/* =================================================================================*/
/*																					*/
/*												MAIN								*/
/*																					*/
/*==================================================================================*/
int main(void)
{
	/* ============================= Open "msgbox.exe" and store it in memory */
	FileHandle = fopen("msgbox.exe", "rb");
	(void)fseek(FileHandle,0,SEEK_END);
	FileSize = ftell(FileHandle);
	(void)rewind(FileHandle);
	pBuffer = malloc(FileSize);

	(void)fread(pBuffer,1,FileSize, FileHandle);
	(void)fclose(FileHandle);
	/* ============================= Init the Disasm structure */
	(void) memset (&MyDisasm, 0, sizeof(DISASM));

	(void) printf("******************************************************* \n");
	(void) printf("Disassemble code in normal mode\n");
	(void) printf("******************************************************* \n");

	DisassembleCode ((unsigned char*) pBuffer + 0x400, (unsigned char*) pBuffer + 0x430, 0x401000);


	(void) printf("******************************************************* \n");
	(void) printf("Disassemble code by following jumps\n");
	(void) printf("******************************************************* \n");


	DisassembleCodeFilter ((unsigned char*) pBuffer + 0x400, (unsigned char*) pBuffer + 0x430, 0x401000);
	return 0;
}
