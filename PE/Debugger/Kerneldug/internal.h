#ifndef _INTERNAL_H_INCLUDED
#define _INTERNAL_H_INCLUDED

#ifndef _NTDEF_
#define VOID void

typedef char * PCHAR;
typedef int BOOL, *PBOOL;
typedef void* PVOID;
typedef unsigned int ULONG, *PULONG;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned char UCHAR, *PUCHAR;
#endif

#define F_PTR_WORD    0x000 << 1
#define F_PTR_DWORD   0x080 << 1
#define F_PTR_QWORD   0x100 << 1
#define F_PTR_REAL4   0x180 << 1
#define F_PTR_REAL8   0x200 << 1
#define F_PTR_REAL10  0x280 << 1
#define F_PTR_TBYTE   0x300 << 1

#define OP_NONE        0x00000000
#define OP_MODRM       0x00000001
#define OP_DATA_I8     0x00000002
#define OP_DATA_I16    0x00000004
#define OP_DATA_I32    0x00000008
#define OP_DATA_PRE66  0x00000010
#define OP_DATA_PRE67  0x00000020
#define OP_W           0x00000040
#define OP_D           0x00000080
#define OP_S           0x00000100
#define OP_BYTE        0x00000100
#define OP_SREG2       0x00000200
#define OP_MMX         0x00000200
#define OP_SREG3       0x00000400
#define OP_TTTn        0x00000800
#define OP_EEE         0x00001000
#define OP_WORD        0x00002000
#define OP_MNEMDIFF    0x00004000
#define OP_MNEMEXT     0x00004000
#define OP_RD          0x00008000
#define OP_MODRM_USED  0x01000000
#define OP_REV         0x02000000
#define OP_ONE         0x04000000
#define OP_THREE       0x08000000
#define OP_SSE         0x10000000
#define OP_SSE_PFX     0x20000000

#define ModrmUsedExtendedLen 1
#define MnemDiffTableLen 11
#define OpcodeWordD9Len 0x2B
#define OpcodeWordDBLen 33
#define OpcodeWordDELen 0x30
#define OpcodeWordDDLen 39
#define OpcodeWordDALen 32
#define OpcodeWordD8Len 63
#define OpcodeWordDCLen 62

extern ULONG OpcodeFlags[0x100];
extern ULONG OpcodeFlagsExt[0x100];
extern ULONG ModrmUsedExtendedFlags[2];
extern USHORT MnemDiffTable[12];
extern USHORT OpcodeWordD9[0x2C];
extern USHORT OpcodeWordDB[34];
extern USHORT OpcodeWordDE[0x31];
extern USHORT OpcodeWordDD[40];
extern USHORT OpcodeWordDA[33];
extern USHORT OpcodeWordD8[64];
extern USHORT OpcodeWordDC[63];
extern UCHAR ModrmUsedMnems1[8];
extern UCHAR ModrmUsedMnems2[8];
extern UCHAR ModrmUsedMnems3[8];
extern UCHAR ModrmUsedMnems4[8];
extern UCHAR ModrmUsedMnems5[8];
extern UCHAR ModrmUsedMnems6[8];
extern UCHAR ModrmUsedMnems7[8];
extern UCHAR ModrmUsedMnems8[8];
extern UCHAR ModrmUsedMnems9[8];
extern UCHAR ModrmUsedMnems10[8];
extern UCHAR ModrmUsedMnems11[8];
extern UCHAR ModrmUsedMnems12[8];
extern UCHAR ModrmUsedMnems13[8];
extern UCHAR ModrmUsedMnems14[8];
extern UCHAR ModrmUsedMnems15[8];
extern USHORT MmxModrmUsedMnems1[8];
extern USHORT MmxModrmUsedMnems2[8];
extern USHORT MmxModrmUsedMnems3[8];
extern USHORT MmxModrmUsedMnems4[8];
extern USHORT MmxModrmUsedMnems5[8];
extern USHORT MmxModrmUsedMnems6[8];

#define OP_USEAL     0x01 << 8
#define OP_IMMREV    0x02 << 8
#define OP_PARAMDX   0x04 << 8
#define OP_USECL     0x08 << 8
#define OP_REVERS    0x10 << 8
#define OP_USE1      0x20 << 8

#define OpcodeSpecFlagsLen 40

extern USHORT OpcodeSpecFlags[41];

#define OP_REGUSED   0x01 << 16
#define OP_REVERSED  0x02 << 16
#define OP_TWOPARAM  0x04 << 16

#define OpcodeFpuFlagsLen 38
#define FpuMemFmtLen 49

extern ULONG OpcodeFpuFlags[39];
extern USHORT FpuMemFmt[50];

#define RelInstrSize 7
extern UCHAR RelInstrTable[8];
extern char MnemArr[0x15E][12];

// Only for Disasm.c

#define PFX_CS     0x2E  
#define PFX_SS     0x36
#define PFX_DS     0x3E
#define PFX_ES     0x26
#define PFX_FS     0x64
#define PFX_GS     0x65
#define PFX_LOCK   0xF0
#define PFX_REPNZ  0xF2
#define PFX_REP    0xF3
#define PFX_66     0x66 // переопределение размера операндов
#define PFX_67     0x67 // переопределение размера адреса
#define PFX_L_CD   0x0F // 2 байта на опкод

#define MAX_INSTR_LEN 15
#define PFX_SEG_MASK  0xFF03
#define PFX_REP_MASK  0xFCFF

//typedef UCHAR[8] TModrmUsedTable, *PModrmUsedTable;
//typedef USHORT[8] TMmxModrmUsedTable, *PMmxModrmUsedTable;

#endif