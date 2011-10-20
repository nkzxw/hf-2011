#ifndef _DISASM_H_INCLUDED
#define _DISASM_H_INCLUDED

#include "internal.h"
#include <string.h>

/*
  Code Analization and Disassembling Tool (CADT)
  Coded By Ms-Rem
*/

#pragma pack (push, 1)

typedef struct _TModRM{
    BOOL  Present;
    UCHAR iMod;
    UCHAR iReg;
    UCHAR iRM;
} TModRM, *PModRM;

typedef struct _TSIB{
    BOOL   Present;
    UCHAR  Scale;
    UCHAR  Index;
    UCHAR  Base;
} TSIB, *PSIB;

typedef struct _TInstruction{
    USHORT  Preffixes;
    BOOL    RepPfx; 
    USHORT  OpCode;
    BOOL    OpcodeWord;
    BOOL    OpcodeExt;
    TModRM  ModRM;
    TSIB    SIB;
    ULONG   Offset;
    UCHAR   OffsetSize;
    ULONG   ImmVal;
    USHORT  ImmExtVal;
    UCHAR   ImmSize;
    BOOL    Mode16Cmd;
    BOOL    Mode16Oper;
    ULONG   InstrAddr;
    UCHAR   InstrLen;
} TInstruction, *PInstruction;

typedef struct _TParam{
    UCHAR  Flags;
    USHORT EFlags;
    ULONG  Value;
} TParam, *PParam;

typedef struct _TDisCommand{
    USHORT  Preffixes;
    USHORT  CmdOrdinal;
    USHORT  CmdFlags;
    UCHAR   ParamsNum;
    BOOL    Cmd16BitOperand;
    TParam  Params[4];
    ULONG   CmdAddr;
    UCHAR   CmdLength;
} TDisCommand, *PDisCommand;
 

typedef struct _TMnemonicOptions{
    BOOL    RealtiveOffsets;
    BOOL    AddAddresPart;
    BOOL    AddHexDump;
    ULONG   AlternativeAddres;
    UCHAR   MnemonicAlign;
} TMnemonicOptions, *PMnemonicOptions;

#pragma pack (pop)

#define  PRT_IMM     0 // immediate param value
#define  PRT_DSP     1 // displacement (offset) value
#define  PRT_REG     2 // register param
#define  PRT_SIB     3 // param is SIB value and sib flags
#define  PRT_MODRM   4 // param defined is MOD r/m
#define  MODRM_32    8 // 32bit MOD r/m type

#define  PRS_BYTE    0x00
#define  PRS_WORD    0x08
#define  PRS_DWORD   0x10
#define  PRS_QWORD   0x48
#define  PRS_DQWORD  0x50
#define  PRS_FWORD   0x18
#define  PRS_TBYTE   0x20
#define  PRS_REAL4   0x28
#define  PRS_REAL8   0x30
#define  PRS_REAL10  0x38

#define  RGT_BYTE    PRS_BYTE
#define  RGT_WORD    PRS_WORD
#define  RGT_DWORD   PRS_DWORD
#define  RGT_SEG     0x20
#define  RGT_CR      0x18
#define  RGT_DR      0x28
#define  RGT_TR      0x40
#define  RGT_FPU     0x30
#define  RGT_NONE    0x38
#define  RGT_MMX     0x48
#define  RGT_XMM     0x50

#define  MEM_OVERRIDE  0x0040
#define  INSTR_FPU     0x0080
#define  CMD_JUMP      0x0800
#define  JMP_FAR       0x1000
#define  JMP_ABS       0x2000 

#define  PRR_AL     0x00 | RGT_BYTE
#define  PRR_CL     0x01 | RGT_BYTE
#define  PRR_DL     0x02 | RGT_BYTE
#define  PRR_BL     0x03 | RGT_BYTE
#define  PRR_AH     0x04 | RGT_BYTE
#define  PRR_CH     0x05 | RGT_BYTE
#define  PRR_DH     0x06 | RGT_BYTE
#define  PRR_BH     0x07 | RGT_BYTE
#define  PRR_AX     0x00 | RGT_WORD
#define  PRR_CX     0x01 | RGT_WORD
#define  PRR_DX     0x02 | RGT_WORD
#define  PRR_BX     0x03 | RGT_WORD
#define  PRR_SP     0x04 | RGT_WORD
#define  PRR_BP     0x05 | RGT_WORD
#define  PRR_SI     0x06 | RGT_WORD
#define  PRR_DI     0x07 | RGT_WORD
#define  PRR_EAX    0x00 | RGT_DWORD
#define  PRR_ECX    0x01 | RGT_DWORD
#define  PRR_EDX    0x02 | RGT_DWORD
#define  PRR_EBX    0x03 | RGT_DWORD
#define  PRR_ESP    0x04 | RGT_DWORD
#define  PRR_EBP    0x05 | RGT_DWORD
#define  PRR_ESI    0x06 | RGT_DWORD
#define  PRR_EDI    0x07 | RGT_DWORD
#define  PRR_ES     0x00 | RGT_SEG
#define  PRR_CS     0x01 | RGT_SEG
#define  PRR_SS     0x02 | RGT_SEG
#define  PRR_DS     0x03 | RGT_SEG
#define  PRR_FS     0x04 | RGT_SEG
#define  PRR_GS     0x05 | RGT_SEG
#define  PRR_INV1   (0x06 | RGT_SEG) // invalid segment register
#define  PRR_INV2   (0x06 | RGT_SEG) // invalid segment register
#define  PRR_CR0    0x00 | RGT_CR
#define  PRR_CR1    (0x01 | RGT_CR) // invalid register
#define  PRR_CR2    0x02 | RGT_CR
#define  PRR_CR3    0x03 | RGT_CR
#define  PRR_CR4    0x04 | RGT_CR
#define  PRR_CR5    (0x05 | RGT_CR) // invalid register
#define  PRR_CR6    (0x06 | RGT_CR) // invalid register
#define  PRR_CR7    (0x07 | RGT_CR) // invalid register
#define  PRR_DR0    0x00 | RGT_DR
#define  PRR_DR1    0x01 | RGT_DR
#define  PRR_DR2    0x02 | RGT_DR
#define  PRR_DR3    0x03 | RGT_DR
#define  PRR_DR6    0x06 | RGT_DR
#define  PRR_DR7    0x07 | RGT_DR
#define  PRR_ST0    0x00 | RGT_FPU
#define  PRR_ST1    0x01 | RGT_FPU
#define  PRR_ST2    0x02 | RGT_FPU
#define  PRR_ST3    0x03 | RGT_FPU
#define  PRR_ST4    0x04 | RGT_FPU
#define  PRR_ST5    0x05 | RGT_FPU
#define  PRR_ST6    0x06 | RGT_FPU
#define  PRR_ST7    0x07 | RGT_FPU
#define  PRR_TR0    (0x00 | RGT_TR) // invalid register
#define  PRR_TR1    (0x01 | RGT_TR) // invalid register
#define  PRR_TR2    (0x00 | RGT_TR) // invalid register
#define  PRR_TR3    0x00 | RGT_TR
#define  PRR_TR4    0x00 | RGT_TR
#define  PRR_TR5    0x00 | RGT_TR
#define  PRR_TR6    0x00 | RGT_TR
#define  PRR_TR7    0x00 | RGT_TR
#define  PRR_MM0    0x00 | RGT_MMX
#define  PRR_MM1    0x01 | RGT_MMX
#define  PRR_MM2    0x02 | RGT_MMX
#define  PRR_MM3    0x03 | RGT_MMX
#define  PRR_MM4    0x04 | RGT_MMX
#define  PRR_MM5    0x05 | RGT_MMX
#define  PRR_MM6    0x06 | RGT_MMX
#define  PRR_MM7    0x07 | RGT_MMX
#define  PRR_XMM0   0x00 | RGT_XMM
#define  PRR_XMM1   0x01 | RGT_XMM
#define  PRR_XMM2   0x02 | RGT_XMM
#define  PRR_XMM3   0x03 | RGT_XMM
#define  PRR_XMM4   0x04 | RGT_XMM
#define  PRR_XMM5   0x05 | RGT_XMM
#define  PRR_XMM6   0x06 | RGT_XMM
#define  PRR_XMM7   0x07 | RGT_XMM

#define  OPX_66      0x0001
#define  OPX_67      0x0002
#define  OPX_CS      0x0004
#define  OPX_DS      0x0008
#define  OPX_ES      0x0010
#define  OPX_SS      0x0020
#define  OPX_FS      0x0040
#define  OPX_GS      0x0080
#define  OPX_REP     0x0100
#define  OPX_REPNZ   0x0200
#define  OPX_LOCK    0x0400

#define  F_PTR_WORD     0x000 << 1
#define  F_PTR_DWORD    0x080 << 1
#define  F_PTR_QWORD    0x100 << 1
#define  F_PTR_REAL4    0x180 << 1
#define  F_PTR_REAL8    0x200 << 1
#define  F_PTR_REAL10   0x280 << 1
#define  F_PTR_TBYTE    0x300 << 1

#define  CON_O     0x00
#define  CON_NO    0x01
#define  CON_B     0x02
#define  CON_NB    0x03
#define  CON_Z     0x04
#define  CON_NZ    0x05
#define  CON_BE    0x06
#define  CON_NBE   0x07
#define  CON_S     0x08
#define  CON_NS    0x09
#define  CON_PE    0x0A
#define  CON_NP    0x0B
#define  CON_L     0x0C
#define  CON_NL    0x0D
#define  CON_LE    0x0E
#define  CON_NLE   0x0F

ULONG _stdcall InstrDecode(PVOID cPtr, PInstruction Instr, BOOL Mode16);
BOOL _stdcall InstrDasm(PInstruction Instr, PDisCommand Command, BOOL Mode16);
VOID _stdcall MakeMnemonic(PCHAR OutBuffer, PDisCommand Command, PMnemonicOptions Options);

#endif