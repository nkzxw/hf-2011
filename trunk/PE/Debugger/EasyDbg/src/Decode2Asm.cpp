// Decode2Asm.cpp: implementation of the CDecode2Asm class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Disasm.h"
#include "Decode2Asm.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void
__stdcall
Decode2Asm(IN PBYTE pCodeEntry,
           OUT char* strAsmCode,
           OUT UINT* pnCodeSize,
           UINT nAddress)
{
  DISASSEMBLY Disasm; // Creates an Disasm Struct
  // Pointer to linear address
  char *Linear = (char *)pCodeEntry;// Points to the address of array to decode.
  // Index of opcoded to decode
  DWORD       Index = 0; 
  Disasm.Address = nAddress; // Common Entry Point (usually default..)
  FlushDecoded(&Disasm);     // ��շ����ṹ��
  
  // Decode instruction
  Decode(&Disasm,
    Linear,
    &Index);
  
  strcpy(strAsmCode, Disasm.Assembly);

  if(strstr((char *)Disasm.Opcode, ":"))
  {
	  Disasm.OpcodeSize++;
 	  char ch =' ';
 	  strncat(strAsmCode,&ch,sizeof(char));
  }

  strcat(strAsmCode,Disasm.Remarks);
  *pnCodeSize = Disasm.OpcodeSize;
  
  // Clear all information
  FlushDecoded(&Disasm);
  
  return;
}

void
__stdcall
Decode2AsmOpcode(IN PBYTE pCodeEntry,   // ��Ҫ����ָ���ַ
           OUT char* strAsmCode,        // �õ������ָ����Ϣ
           OUT char* strOpcode,         // ������������Ϣ
           OUT UINT* pnCodeSize,        // ����ָ���
           UINT nAddress)
{
   DISASSEMBLY Disasm;                  // ���巴�����Ϣ�ṹ��
   char *Linear = (char *)pCodeEntry;   // ���������ַ
   DWORD       Index = 0;               // ��ʼ�������볤��
   Disasm.Address = nAddress;           // ���ý���ָ���ַ
   FlushDecoded(&Disasm);               // ��շ����ṹ��

   // ����ָ��
   Decode(&Disasm,Linear,&Index);

   // ȡ��������ķ������Ϣ
   strcpy(strAsmCode, Disasm.Assembly);

   if(strstr((char *)Disasm.Opcode, ":"))
   {
     Disasm.OpcodeSize++;
     char ch =' ';
     strncat(strAsmCode,&ch,sizeof(char));
  }
   // ȡ���������ָ����Ϣ
   strcpy(strOpcode, Disasm.Opcode);

   // ȡ��ָ��˵����Ϣ
   strcat(strAsmCode,Disasm.Remarks);

   // ����ָ���
   *pnCodeSize = Disasm.OpcodeSize;
   FlushDecoded(&Disasm);               // ��շ����ṹ��
}