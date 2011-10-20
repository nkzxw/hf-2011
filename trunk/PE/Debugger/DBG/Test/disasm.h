#include <iostream>

#ifndef disasm_h
#define disasm_h
typedef struct Decoded{
	
	// Define Decoded instruction struct

    char Assembly[256]; // Menemonics
    char Remarks[256];  // Menemonic addons
    char Opcode[30];    // Opcode Byte forms
    DWORD Address;      // Current address of decoded instruction
    BYTE  OpcodeSize;   // Opcode Size
	BYTE  PrefixSize;   // Size of all prefixes used

} DISASSEMBLY;

void Decode(DISASSEMBLY *Disasm,char *Opcode,DWORD *Index);




#endif