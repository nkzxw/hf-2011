//#include <ntddk.h>
//typedef int BOOL, *PBOOL;
#include "Disasm.h"


BOOL
FindBaseAndSize(
	PVOID SomePtr,
	PVOID *BaseAddress, 
	ULONG *ImageSize
	);

ULONG
SymGlobGetSymbolByAddress(
	PVOID Address,
	char* Symbol,
	ULONG *SymLen
	);

ULONG
SymWrGetNearestSymbolByAddress(
	PVOID Address,
	char* Symbol,
	ULONG *SymLen
	);

#define SymGlobGetSymbolByAddress SymWrGetNearestSymbolByAddress

BOOL MmIsAddressValid (PVOID);

//extern PVOID pNtSymbols;
//extern PVOID pNtBase;

VOID _cdecl DbgPrint (char*,...);
#define KdPrint(X) DbgPrint X


USHORT GetPrefixes(PUCHAR cPtr, PInstruction Instr){
    USHORT Prefixes = 0;
    USHORT PrefCount = 0;

    while (PrefCount <= MAX_INSTR_LEN){
        switch (*cPtr){
            case PFX_CS:
                Prefixes = (Prefixes & PFX_SEG_MASK) | OPX_CS;
                break;
            case PFX_SS:
                Prefixes = (Prefixes & PFX_SEG_MASK) | OPX_SS;
                break;
            case PFX_DS:
                Prefixes = (Prefixes & PFX_SEG_MASK) | OPX_DS;
                break;
            case PFX_ES:
                Prefixes = (Prefixes & PFX_SEG_MASK) | OPX_ES;
                break;
            case PFX_FS:
                Prefixes = (Prefixes & PFX_SEG_MASK) | OPX_FS;
                break;
            case PFX_GS:
                Prefixes = (Prefixes & PFX_SEG_MASK) | OPX_GS;
                break;
            case PFX_LOCK:
                Prefixes |= OPX_LOCK;
                break;
            case PFX_REPNZ:
                Prefixes = (Prefixes & PFX_REP_MASK) | OPX_REPNZ;
                break;
            case PFX_REP:
                Prefixes = (Prefixes & PFX_REP_MASK) | OPX_REP;
                break;
            case PFX_66:
                Prefixes |= OPX_66;
                break;
            case PFX_67:
                Prefixes |= OPX_67;
                break;
            default:
                Instr->Preffixes = Prefixes;
                return PrefCount;
        }
        cPtr++;
        PrefCount++;
    }
    return PrefCount;
}

ULONG GetOpcode(PUCHAR cPtr, PInstruction Instr){
    if (*cPtr == PFX_L_CD){
        cPtr++;
        Instr->OpCode = *cPtr;
        Instr->OpcodeExt = 1;
        return 2;
    }
    Instr->OpCode =  *cPtr;
    if ((OpcodeFlags[Instr->OpCode] & OP_WORD) && ((*(cPtr + 1) >> 6) == 3)){
        Instr->OpCode = *(PUSHORT)(cPtr);
//        cPtr++;
//        Instr->OpCode = (Instr->OpCode << 8) | (*cPtr);
        Instr->OpcodeWord = 1;
        return 2;
    }
    return 1;
}

ULONG GetOpcodeFlags(PInstruction Instr){
    if (Instr->OpcodeExt)
        return OpcodeFlagsExt[(UCHAR)(Instr->OpCode)];
    return OpcodeFlags[(UCHAR)(Instr->OpCode)];
}

ULONG GetModRM(PUCHAR cPtr, PInstruction Instr, BOOL Mode16){
    UCHAR ModRM;
    Instr->Mode16Cmd = Mode16;
    Instr->Mode16Oper = Mode16;
    if (Instr->Preffixes & OPX_67)
        Instr->Mode16Cmd = !(Mode16);
    if (Instr->Preffixes & OPX_66)
        Instr->Mode16Oper = !(Mode16);
    if (Instr->ModRM.Present = (((GetOpcodeFlags(Instr) & OP_MODRM) != 0) && (!(Instr->OpcodeWord)))){
        ModRM = *cPtr;
        Instr->ModRM.iMod = ModRM >> 6;
        Instr->ModRM.iReg = (ModRM & 0x38) >> 3;
        Instr->ModRM.iRM = ModRM & 7;
        return 1;
    }
    return 0;
}

ULONG GetSIBAndOffset(PUCHAR cPtr, PInstruction Instr){
    UCHAR SIB, OffsetSize = 0, ret = 0;

    Instr->SIB.Present = ((!(Instr->Mode16Cmd)) && (Instr->ModRM.iRM == 4));
    switch (Instr->ModRM.iMod){
        case 0:
            if ((Instr->Mode16Cmd) && (Instr->ModRM.iRM == 6))
                OffsetSize = 16;
            else
                if ((!(Instr->Mode16Cmd)) && (Instr->ModRM.iRM == 5))
                    OffsetSize = 32;
            break;
        case 1:
            OffsetSize = 8;
            break;
        case 2:
            if (Instr->Mode16Cmd)
                OffsetSize = 16;
            else
                OffsetSize = 32;
            break;
        case 3:
            Instr->SIB.Present = 0;
            break;
    }
    if (Instr->SIB.Present){
        SIB = *cPtr;
        Instr->SIB.Scale = SIB >> 6;
        Instr->SIB.Index = (SIB & 0x38) >> 3;
        Instr->SIB.Base = SIB & 7;
        ret++;
        if ((Instr->SIB.Base == 5) && (Instr->ModRM.iMod >= 0) && (Instr->ModRM.iMod <= 2))
            OffsetSize = 32;
    }
    switch (OffsetSize){
        case 0:
            Instr->Offset = 0;
            break;
        case 8:
            Instr->Offset = *(PUCHAR)(cPtr + ret);
            break;
        case 16:
            Instr->Offset = *(PUSHORT)(cPtr + ret);
            break;
        case 32:
            Instr->Offset = *(PULONG)(cPtr + ret);
            break;
    }
    Instr->OffsetSize = OffsetSize;
    return ret + (OffsetSize / 8);
}

ULONG GetImmVal(PUCHAR cPtr, PInstruction Instr){
    ULONG Flags, EFl, ret;
    BOOL pf66;
    UCHAR Add, r;

    ret = 0;
    Flags = GetOpcodeFlags(Instr);
    if (Flags & 0x0F000000)
        for (r = 0; r <= ModrmUsedExtendedLen; r++){
            EFl = ModrmUsedExtendedFlags[r];
            if ((((EFl & 0xFF00) >> 8) == Instr->OpCode) && (((EFl & 0x70000) >> 18) == Instr->ModRM.iReg))
                Flags |= (EFl & 0xFF);
        }
    if (Flags & OP_DATA_I8)
        ret++;
    if (Flags & OP_DATA_I16)
        ret += 2;
    if (Flags & OP_DATA_I32)
        ret += 4;
    if ((Instr->OpCode >= 0xA0) && (Instr->OpCode <= 0xA3))
        pf66 = Instr->Mode16Cmd;
    else
        pf66 = Instr->Mode16Oper;
    if (pf66)
        Add = 2;
    else 
        Add = 4;
    if (Flags & (OP_DATA_PRE66 | OP_DATA_PRE67))
        ret += Add;
    switch (ret){
        case 1:
            Instr->ImmVal = *(PUCHAR)(cPtr);
            break;
        case 2:
            Instr->ImmVal = *(PUSHORT)(cPtr);
            break;
        case 3: 
        case 4:
            Instr->ImmVal = *(PULONG)(cPtr);
            break;
        case 6:
            Instr->ImmVal = *(PULONG)(cPtr);
            Instr->ImmExtVal = *(PUSHORT)(cPtr + 4);
            break;
    }
    Instr->ImmSize = (UCHAR)(ret << 3);
    return ret;
}

ULONG _stdcall InstrDecode(PVOID cPtr, PInstruction Instr, BOOL Mode16){
    PUCHAR dPtr;
    ULONG ret;

    dPtr = (PUCHAR)cPtr;
    Instr->InstrAddr = (ULONG)((__int64)cPtr);
    dPtr += GetPrefixes(dPtr, Instr);
    dPtr += GetOpcode(dPtr, Instr);
    dPtr += GetModRM(dPtr, Instr, Mode16);
    if (Instr->ModRM.Present)
        dPtr += GetSIBAndOffset(dPtr, Instr);
    ret = (ULONG)((__int64)dPtr) - (ULONG)((__int64)cPtr) + GetImmVal(dPtr, Instr);
    if (ret > MAX_INSTR_LEN)
        return ret;
    Instr->InstrLen = ret;
    return ret;
}

UCHAR GetCondition(UCHAR Opcode){
    return (((Opcode & 0x0F) << 1) | 0x01);
}

UCHAR SearchArray(PUSHORT Data, UCHAR Code, ULONG Len){
    UCHAR r;

    for (r = 0; r <= Len; r++)
        if ((UCHAR)(Data[r]) == Code)
            return (UCHAR)(Data[r] >> 8);
    return 0xDA;
}

USHORT GetCommandOrdinal(PInstruction Instr, ULONG Flags){
    BOOL Mode16Cmd;
    USHORT ret;
    UCHAR TwoByte, r;

    ret = 0xDA;
    if (Flags == OP_NONE)
        return ret;
    Mode16Cmd = Instr->Mode16Cmd | Instr->Mode16Oper;
    if (Instr->OpcodeWord){
        TwoByte = Instr->OpCode >> 8;
        switch ((UCHAR)(Instr->OpCode)){
            case 0xD8:
                ret = SearchArray(OpcodeWordD8, TwoByte, OpcodeWordD8Len);
                break;
            case 0xD9:
                ret = SearchArray(OpcodeWordD9, TwoByte, OpcodeWordD9Len);
                break;
            case 0xDA:
                ret = SearchArray(OpcodeWordDA, TwoByte, OpcodeWordDALen);
                break;
            case 0xDB:
                ret = SearchArray(OpcodeWordDB, TwoByte, OpcodeWordDBLen);
                break;
            case 0xDC:
                ret = SearchArray(OpcodeWordDC, TwoByte, OpcodeWordDCLen);
                break;
            case 0xDD:
                ret = SearchArray(OpcodeWordDD, TwoByte, OpcodeWordDDLen);
                break;
            case 0xDE:
                ret = SearchArray(OpcodeWordDE, TwoByte, OpcodeWordDELen);
                break;
            case 0xDF:
                if (TwoByte == 0xE0)
                    ret = 0xDD;
                else
                    ret = 0xDA;
                break;
        }
    } else
        if (!(Flags & OP_MODRM_USED)){
            ret = (Flags & 0x00FF0000) >> 16;
            if ((Mode16Cmd) && (!Instr->OpcodeExt) && (Flags & OP_MNEMDIFF)){
                for (r = 0; r <= MnemDiffTableLen; r++)
                    if (Instr->OpCode == (MnemDiffTable[r] << 8))
                        ret = (UCHAR)(MnemDiffTable[r]);
            } else
                if ((Instr->OpCode) && (Flags & OP_MNEMDIFF))
                    ret |= 0x100;
        } else
            if (Instr->OpcodeExt)
                switch (Instr->OpCode){
                    case 0xBA:
                        ret = ModrmUsedMnems13[Instr->ModRM.iReg];
                        break;
                    case 0x00:
                        ret = ModrmUsedMnems14[Instr->ModRM.iReg];
                        break;
                    case 0x01:
                        ret = ModrmUsedMnems15[Instr->ModRM.iReg];
                        break;
                    case 0x71:
                        ret = MmxModrmUsedMnems1[Instr->ModRM.iReg];
                        break;
                    case 0x72:
                        ret = MmxModrmUsedMnems2[Instr->ModRM.iReg];
                        break;
                    case 0x73:
                        ret = MmxModrmUsedMnems3[Instr->ModRM.iReg];
                        break;
                    case 0xC7:
                        ret = MmxModrmUsedMnems4[Instr->ModRM.iReg];
                        break;
                    case 0xAE:
                        ret = MmxModrmUsedMnems5[Instr->ModRM.iReg];
                        break;
                    case 0x18:
                        ret = MmxModrmUsedMnems6[Instr->ModRM.iReg];
                        break;
                }
            else
                switch (Instr->OpCode){
                    case 0x80:
                    case 0x81:
                    case 0x82:
                    case 0x83:
                        ret = ModrmUsedMnems1[Instr->ModRM.iReg];
                        break;
                    case 0xD8:
                    case 0xDC:
                        ret = ModrmUsedMnems2[Instr->ModRM.iReg];
                        break;
                    case 0xDA:
                    case 0xDE:
                        ret = ModrmUsedMnems3[Instr->ModRM.iReg];
                        break;
                    case 0xF6:
                    case 0xF7:
                        ret = ModrmUsedMnems4[Instr->ModRM.iReg];
                        break;
                    case 0xD0:
                    case 0xD1:
                    case 0xD2:
                    case 0xD3:
                    case 0xC0:
                    case 0xC1:
                        ret = ModrmUsedMnems5[Instr->ModRM.iReg];
                        break;
                    case 0xC6:
                    case 0xC7:
                        ret = ModrmUsedMnems6[Instr->ModRM.iReg];
                        break;
                    case 0xFF:
                    case 0xFE:
                        ret = ModrmUsedMnems7[Instr->ModRM.iReg];
                        break;
                    case 0xDF:
                        ret = ModrmUsedMnems8[Instr->ModRM.iReg];
                        break;
                    case 0xDB:
                        ret = ModrmUsedMnems9[Instr->ModRM.iReg];
                        break;
                    case 0xD9:
                        ret = ModrmUsedMnems10[Instr->ModRM.iReg];
                        break;
                    case 0xDD:
                        ret = ModrmUsedMnems11[Instr->ModRM.iReg];
                        break;
                    case 0x8F:
                        ret = ModrmUsedMnems12[Instr->ModRM.iReg];
                        break;
                }
    if ((Instr->OpcodeExt) && (Flags & OP_SSE_PFX) && (Instr->Preffixes & OPX_REP)){
        Instr->Preffixes &= PFX_REP_MASK;
        Instr->RepPfx = 1;
        ret++;
    }
    if ((ret == 0xCA) && (Instr->Mode16Cmd))
        return 0xC9;
    return ret;
}

VOID GetInOpcodeRegister(PInstruction Instr, PDisCommand Command, ULONG Flags){
    ULONG AddFlag;

    if (Flags & OP_RD){
        AddFlag = 0;
        switch (Instr->ImmSize){
            case 0: 
                if (Instr->Mode16Oper)
                    AddFlag = PRS_WORD;
                else
                    AddFlag = PRS_DWORD;
                break;
            case 8: 
                AddFlag = PRS_BYTE;
                break;
            case 16: 
                AddFlag = PRS_WORD;
                break;
            case 32:
                AddFlag = PRS_DWORD;
                break;
        }
        Command->Params[Command->ParamsNum].Value = (Instr->OpCode & 7) | AddFlag;
        Command->Params[Command->ParamsNum].Flags = PRT_REG | AddFlag;
        if (Instr->OpCode != 0x90)
            Command->ParamsNum++;
    } else
        if ((!(Instr->OpcodeExt)) && (Flags & OP_SREG2)){
            Command->Params[Command->ParamsNum].Flags = PRT_REG;
            Command->Params[Command->ParamsNum].Value = ((Instr->OpCode & 0x18) >> 3) | RGT_SEG;
            Command->ParamsNum++;
        } else
            if ((Flags & OP_SREG3) && (!(Instr->ModRM.Present))){
                Command->Params[Command->ParamsNum].Flags = PRT_REG;
                Command->Params[Command->ParamsNum].Value = ((Instr->OpCode & 0x38) >> 3) | RGT_SEG;
                Command->ParamsNum++;
            }
}

BOOL DecodeModRM(PInstruction Instr, PDisCommand Command, BOOL Reserved, ULONG Flags, ULONG DefSize){
    UCHAR Prm1, Prm2, Prt1, Prt2;
    BOOL Param1Present, Param2Present, ret = 0;

    Prm1 = Instr->ModRM.iReg | DefSize;
    if (Instr->OpcodeExt)
        switch (Instr->OpCode){
            case 0x2C:
            case 0x2D:
                if (Instr->RepPfx)
                    Prm1 = Instr->ModRM.iReg | PRS_DWORD;
                else
                    Prm1 = Instr->ModRM.iReg | PRS_QWORD;
                break;
            case 0x50:
            case 0xC5:
            case 0xD7:
                Prm1 = Instr->ModRM.iReg | PRS_DWORD;
                break;
        }
    Prm2 = Instr->ModRM.iRM;
    Prt1 = PRT_REG;
    Prt2 = 0;
    Param1Present = 0;
    Param2Present = 0;
    if (Flags & OP_EEE){
        Param1Present = 1;
        Prm2 = Prm2 | RGT_DWORD;
        switch (Instr->OpCode){
            case 0x20:
            case 0x22:
                Prm1 = (Prm1 & 7) | RGT_CR;
                break;
            case 0x21:
            case 0x23:
                Prm1 = (Prm1 & 7) | RGT_DR;
                break;
            case 0x24:
            case 0x26:
                Prm1 = (Prm1 & 7) | RGT_TR;
                break;
        }
        if (Instr->ModRM.iMod < 3){
            Command->CmdOrdinal = 0xDA;
            return ret;
        }
    }
    switch (Instr->ModRM.iMod){
        case 0:
            Param1Present = (Instr->ImmSize == 0);
            if (Instr->Mode16Cmd){
                if (Prm2 != 6){
                    Prt2 = PRT_MODRM | DefSize;
                    Param2Present = 1;
                } else
                    Param1Present = 0;
            } else
                if (!(Instr->SIB.Present))
                    if (Prm2 != 5) {
                        if ((Instr->OpCode == 1) && (Instr->OpcodeExt))
                            Prt2 = PRT_MODRM | RGT_WORD;
                        else
                            Prt2 = PRT_MODRM | DefSize;
                        Prm2 = Prm2 | MODRM_32;
                        Param2Present = 1;
                    }
            break;
        case 1:
        case 2:
            if (!(Instr->SIB.Present)){
                if (!(Instr->Mode16Cmd))
                    Prm2 = Prm2 | MODRM_32;
                Prt1 = PRT_REG;
                Prt2 = PRT_MODRM | DefSize;
                Param1Present = (Instr->ImmSize == 0);
                Param2Present = 1;
            } else {
                Prt1 = PRT_REG;
                Prm1 = Prm1 | DefSize;
                Param1Present = (Instr->ImmSize == 0);
            }
            break;
        case 3:
            Prt2 = PRT_REG;
            Param2Present = 1;
            if (!(Flags & OP_EEE))
                Prm2 = Prm2 | DefSize;
            if (Instr->OpcodeExt)
                switch (Instr->OpCode){
                    case 0x2A:
                        if (Instr->RepPfx)
                            Prm2 = (Prm2 & 0x187) | PRS_DWORD;
                        else
                            Prm2 = (Prm2 & 0x187) | PRS_QWORD;
                        break;
                    case 0xC4:
                        Prm2 = (Prm2 & 0x187) | PRS_DWORD;
                        break;
                }
            Param1Present = (Instr->ImmSize == 0);
            break;
    }
    if (Flags & OP_SREG3){
        Param1Present = 1;
        Prm1 = (Prm1 & 7) | RGT_SEG;
        if (Prt2 == PRT_REG)
            Prm2 = (Prm2 & 7) | DefSize;
        if ((!Reserved) && (Prt2 == PRT_REG))
            Prm2 = (Prm2 & 7) | RGT_WORD;
        Prt2 = (Prt2 & 7) | PRS_WORD;
    }
    if (Flags & OP_ONE)
        Param1Present = 0;
    if ((Flags & OP_THREE) && (Prm1 != Prm2))
        Param1Present = 1;
    if (Command->CmdOrdinal == 0x15D)
        Param2Present = 0;
    Command->Params[Command->ParamsNum].Flags = Prt1;
    Command->Params[Command->ParamsNum].Value = Prm1;
    if (Param1Present)
        Command->ParamsNum++;
    Command->Params[Command->ParamsNum].Flags = Prt2;
    Command->Params[Command->ParamsNum].Value = Prm2;
    if (Param2Present)
        Command->ParamsNum++;
    return 1;
}

USHORT GetSpecOpcodeFlags(UCHAR Opcode){
    UCHAR r;

    for (r = 0; r <= OpcodeSpecFlagsLen; r++)
        if ((OpcodeSpecFlags[r] & 0xFF) == Opcode)
            return OpcodeSpecFlags[r];
    return 0;
}

VOID DecodeSpecParams(PInstruction Instr, PDisCommand Command, USHORT DefSize, PBOOL ImmRevers, PBOOL NotImm){
    UCHAR Prt1 = 0, Prt2 = 0;
    ULONG SpecFlags, Prm1 = 0, Prm2 = 0, RegTmp;
    BOOL Reversed = 0, Param1Present = 0, Param2Present = 0;

    SpecFlags = GetSpecOpcodeFlags((UCHAR)Instr->OpCode);
    if (SpecFlags & OP_USEAL){
        Prt1 = PRT_REG;
        Param1Present = 1;
        if (DefSize == PRS_BYTE)
            Prm1 = PRR_AL;
        else
            if (Instr->Mode16Oper)
                Prm1 = PRR_AX;
            else
                Prm1 = PRR_EAX;
    }
    if (SpecFlags & OP_PARAMDX){
        Prt2 = PRT_REG;
        Param2Present = 1;
        Prm2 = PRR_DX;
    }
    if (SpecFlags & (OP_USECL | OP_USE1)){
        if (SpecFlags & OP_USECL){
            Prt1 = PRT_REG;
            Prm1 = PRR_CL;
        } else {
            Prt1 = PRT_IMM | PRS_BYTE;
            Prm1 = 1;
        }
        Param1Present = 1;
    }
    if ((Instr->OpCode >= 0xA0) && (Instr->OpCode <= 0xA3)){
        Prt1 = PRT_REG;
        if ((Instr->OpCode == 0xA0) || (Instr->OpCode == 0xA2))
            Prm1 = PRR_AL;
        else
            if (Instr->Mode16Oper)
                Prm1 = PRR_AX;
            else
                Prm1 = PRR_EAX;
        Param2Present = 1;
        Prt2 = PRT_DSP;
        Prm2 = Instr->ImmVal;
        *NotImm = 1;
        if ((Instr->OpCode == 0xA2) || (Instr->OpCode == 0xA3))
            Reversed = 1;
    }
    if (SpecFlags & OP_REVERS)
        if ((Instr->ImmSize) && (!Reversed))
            *ImmRevers = 1;
        else {
            RegTmp = Prm1;
            Prm1 = Prm2;
            Prm2 = RegTmp;
            RegTmp = Prt1;
            Prt1 = Prt2;
            Prt2 = (UCHAR)RegTmp;
        }
    if (SpecFlags & OP_IMMREV)
        *ImmRevers = 1;
    Command->Params[Command->ParamsNum].Flags = Prt1;
    Command->Params[Command->ParamsNum].Value = Prm1;
    Command->Params[Command->ParamsNum].EFlags = DefSize;
    if (Param1Present)
        Command->ParamsNum++;
    Command->Params[Command->ParamsNum].Flags = Prt2;
    Command->Params[Command->ParamsNum].Value = Prm2;
    Command->Params[Command->ParamsNum].EFlags = DefSize;
    if (Param2Present)
        Command->ParamsNum++;
}

USHORT swap(USHORT b){
    return (((UCHAR)b << 8) | (b >> 8));
}

ULONG GetFpuOpcodeFlags(PInstruction Instr){
    UCHAR r;

    for (r = 0; r <= OpcodeFpuFlagsLen; r++)
        if ((USHORT)(OpcodeFpuFlags[r]) == (swap(Instr->OpCode) & 0xFFF9))
            return (OpcodeFpuFlags[r] & 0xFFFF0000);
    return 0;
}

VOID DecodeFpuParams(PInstruction Instr, PDisCommand Command, ULONG Flags){
    ULONG FFlags;
    UCHAR r;

    if (Flags & OP_SSE)
        return ;
    FFlags = GetFpuOpcodeFlags(Instr);
    if (Instr->OpcodeWord){
        if (FFlags & OP_REGUSED){
            if (FFlags & OP_TWOPARAM){
                Command->Params[Command->ParamsNum].Flags = PRT_REG;
                Command->Params[Command->ParamsNum].Value = PRR_ST0 | RGT_FPU;
                Command->ParamsNum++;
            }
            Command->Params[Command->ParamsNum].Flags = PRT_REG;
            Command->Params[Command->ParamsNum].Value = ((Instr->OpCode >> 8) & 7) | RGT_FPU;
            Command->ParamsNum++;
            if (FFlags & OP_REVERSED){
                Command->Params[Command->ParamsNum].Flags = PRT_REG;
                Command->Params[Command->ParamsNum].Value = PRR_ST0 | RGT_FPU;
                Command->ParamsNum++;
            }
        }
        if (Instr->OpCode == 0xE0DF){
            Command->Params[Command->ParamsNum].Flags = PRT_REG;
            Command->Params[Command->ParamsNum].Value = PRR_AX;
            Command->ParamsNum++;
        }
    } else
        for (r = 0; r <= FpuMemFmtLen; r++)
            if ((Instr->OpCode == (FpuMemFmt[r] & 0xFF)) && (Instr->ModRM.iReg == (FpuMemFmt[r] >> 11))){
                Command->CmdFlags = Command->CmdFlags | INSTR_FPU | (FpuMemFmt[r] & 0x700);
                return ;
            }
}

VOID DecodeOffsetParam(PInstruction Instr, PDisCommand Command, ULONG Flags, ULONG DefSize){
    UCHAR Prt1, Prt2;
    
    Prt1 = PRT_DSP;
    Prt2 = 0;
    switch (Instr->OffsetSize){
        case 8:
            Prt1 |= PRS_BYTE;
            break;
        case 16:
            Prt1 |= PRS_WORD;
            break;
        case 32:
            Prt1 |= PRS_DWORD;
            break;
    }
    if (!(Flags & OP_SREG3))
        Prt2 = Prt2 | DefSize;
    else
        Prt2 = Prt2 | PRS_WORD;
    if ((Instr->OpcodeExt) && (!Instr->OpCode))
        Prt2 = (Prt2 & 7) | PRS_WORD;
    Command->Params[Command->ParamsNum].Flags = Prt1;
    Command->Params[Command->ParamsNum].EFlags = Prt2;
    Command->Params[Command->ParamsNum].Value = Instr->Offset;
    Command->ParamsNum++;
}

VOID DecodeImmParam(PInstruction Instr, PDisCommand Command, ULONG Flags, ULONG DefSize){
    ULONG Val = Instr->ImmVal, AddVal;
    UCHAR Prt1 = PRT_IMM;

    switch (Instr->ImmSize){
        case 8:
            if ((Instr->OpcodeExt) || (!(Flags & OP_S)))
                Prt1 = Prt1 | DefSize;
            else {
                Prt1 = Prt1 | DefSize;
                if (DefSize == PRS_WORD)
                    AddVal = 0xFF00;
                else
                    AddVal = (ULONG)(-0x100);
                Val = Val | (AddVal * (Val >> 7));
            }
            break;
        case 16:
            Prt1 = Prt1 | PRS_WORD;
            break;
        case 32:
            Prt1 = Prt1 | PRS_DWORD;
            break;
        case 48:
            Prt1 = Prt1 | PRS_FWORD;
            break;
    }
    Command->Params[Command->ParamsNum].Flags = Prt1;
    Command->Params[Command->ParamsNum].EFlags = Instr->ImmExtVal;
    Command->Params[Command->ParamsNum].Value = Val;
    Command->ParamsNum++;
}

VOID DecodeCmdSpecParams(PInstruction Instr, PDisCommand Command, PBOOL NotImm){
    if (Instr->OpcodeExt)
        switch (Instr->OpCode){
            case 0x00:
                if (!(Instr->OffsetSize))
                    Command->Params[0].Value = (Command->Params[0].Value & 0x0F) | RGT_WORD;
                Command->Params[0].Flags = (Command->Params[0].Flags & 7) | PRS_WORD;
                break;
            case 0xA5:
            case 0xAD:
                Command->Params[Command->ParamsNum].Flags = PRT_REG;
                Command->Params[Command->ParamsNum].Value = PRR_CL;
                Command->ParamsNum++;
                break;
        }
    else
        if (Instr->OpCode == 0xC8){
            Command->Params[Command->ParamsNum].Flags = PRT_IMM;
            Command->Params[Command->ParamsNum].Value = Instr->ImmVal & 0xFFFF;
            Command->ParamsNum++;
            Command->Params[Command->ParamsNum].Flags = PRT_IMM;
            Command->Params[Command->ParamsNum].Value = (Instr->ImmVal & 0xFF0000) >> 16;
            Command->ParamsNum++;
            *NotImm = 1;
        }
}

BOOL DecodeSibParam(PInstruction Instr, PDisCommand Command, ULONG Flags, ULONG DefSize){
    ULONG Prm1;
    UCHAR Prt1;
    BOOL res = 1;

    if ((!(Instr->SIB.Scale)) && (Instr->SIB.Index == 4) && (Instr->SIB.Base == 4)){
        Prt1 = PRT_MODRM;
        Prm1 = PRR_ESP | MODRM_32;
    } else {
        Prt1 = PRT_SIB;
        Prm1 = Instr->SIB.Base | RGT_DWORD;
        if ((Instr->SIB.Base == 5) && (!(Instr->ModRM.iMod)))
            Prm1 = RGT_NONE;
        if (Instr->SIB.Index == 4){
            Prm1 = Prm1 | (RGT_NONE << 8);
            if (Instr->SIB.Scale)
                res = 0;
        } else
            Prm1 = Prm1 | ((Instr->SIB.Index | RGT_DWORD) << 8);
        Prm1 = Prm1 | (Instr->SIB.Scale << 16);
    }
    Prt1 = Prt1 | DefSize;
    if ((Instr->OpcodeExt) && (!(Instr->OpCode)))
        Prt1 = (Prt1 & 7) | PRS_WORD;
    if (Flags & OP_SREG3)
        Prt1 = (Prt1 & 7) | PRS_WORD;
    Command->Params[Command->ParamsNum].Flags = Prt1;
    Command->Params[Command->ParamsNum].Value = Prm1;
    Command->ParamsNum++;
    return res;
}

BOOL CleanDisasmStruct(PDisCommand Command, BOOL MmxInstr, BOOL Mode16){
    UCHAR r, CurrParam = 0;
    TParam NewParams[4];
    ULONG pType, pValue, RegSize = 0, AddrSize = 0;
    BOOL AddrPresent = 0, RegPresent = 0, MemOverride;

    if (Command->ParamsNum){
        for (r = 0; r < Command->ParamsNum; r++){
            pType = Command->Params[r].Flags & 7;
            pValue = Command->Params[r].Value;
            if ((pType == PRT_MODRM) || (pType == PRT_SIB)){
                AddrPresent = 1;
                AddrSize = Command->Params[r].Flags & 0x78;
            }
            if (pType == PRT_REG){
                RegPresent = 1;
                if (pValue <= 0x17)
                    RegSize = pValue & 0x38;
                else
                    if ((pValue >= 0x20) && (pValue <= 0x27))
                        RegSize = PRS_WORD;
                    else
                        if (pValue <= 0x2F)
                            RegSize = PRS_DWORD;
            }
            if ((pType == PRT_REG) && ((pValue == PRR_CR1) || (pValue == PRR_CR5) || (pValue == PRR_CR6)
                || (pValue == PRR_CR7) || (pValue == PRR_TR0) || (pValue == PRR_TR1) || (pValue == PRR_TR2)
                || (pValue == PRR_INV1 || (pValue == PRR_INV2)))){
                Command->CmdOrdinal = 0xDA;
                return 0;
            }
        }
        for (r = 0; r < Command->ParamsNum; r++)
            if ((!AddrPresent) || ((Command->Params[r].Flags & 7) != PRT_DSP) || (Command->Params[r].Value)){
                NewParams[CurrParam].Flags = Command->Params[r].Flags;
                NewParams[CurrParam].EFlags = Command->Params[r].EFlags;
                NewParams[CurrParam].Value = Command->Params[r].Value;
                CurrParam++;
            }
    }
    if (CurrParam > 1){
        for (r = 0; r < CurrParam; r++){
            if (((NewParams[r].Flags & 7) == PRT_DSP) && (!AddrPresent)){
                AddrPresent = 1;
                AddrSize = NewParams[r].EFlags & 0x38;
            }
            Command->Params[r].Flags = NewParams[r].Flags;
            Command->Params[r].EFlags = NewParams[r].EFlags;
            Command->Params[r].Value = NewParams[r].Value;
        }
        Command->ParamsNum = CurrParam;
    }
    if (AddrPresent)
        if ((Command->CmdOrdinal == 0x132) || (Command->CmdOrdinal == 0x134))
            Command->CmdOrdinal++;
    MemOverride = (RegPresent & AddrPresent & (AddrSize != RegSize));
    if ((!RegPresent) && (AddrPresent))
        switch (AddrSize){
            case PRS_BYTE:
                MemOverride = 1;
                break;
            case PRS_WORD:
                MemOverride = (Mode16 == 0);
                break;
            case PRS_DWORD:
                MemOverride = (Mode16 == 1);
                break;
        }
    if ((Command->CmdOrdinal == 0x74) || (MmxInstr))
        MemOverride = 0;
    if (MemOverride)
        Command->CmdFlags = Command->CmdFlags | MEM_OVERRIDE;
    return 1;
}

BOOL IsRelative(USHORT Ordinal){
    UCHAR r;

    for (r = 0; r <= RelInstrSize; r++)
        if (RelInstrTable[r] == Ordinal)
            return 1;
    return 0;
}

VOID GetJumpFlags(PInstruction Instr, PDisCommand Command){
    if (IsRelative(Command->CmdOrdinal)){
        Command->CmdFlags = Command->CmdFlags | CMD_JUMP;
        if (!Instr->OpcodeExt){
            if ((Instr->OpCode == 0xEA) || (Instr->OpCode == 0x9A))
                Command->CmdFlags = Command->CmdFlags | JMP_ABS | JMP_FAR;
            if ((Instr->OpCode == 0xFF) && ((Instr->ModRM.iReg == 3) || (Instr->ModRM.iReg == 5)))
                Command->CmdFlags = Command->CmdFlags | JMP_FAR;
        }
    }
}

BOOL _stdcall InstrDasm(PInstruction Instr, PDisCommand Command, BOOL Mode16){
    ULONG Flags, Prm1;
    UCHAR Prt1;
    USHORT DefSize;
    BOOL ImmRevers = 0, NotImm = 0, ByteParam, FpuCode, Reversed, MmxInstr;

    Flags = GetOpcodeFlags(Instr);
    Command->CmdOrdinal = GetCommandOrdinal(Instr, Flags);
    Command->CmdAddr = Instr->InstrAddr;
    Command->CmdLength = Instr->InstrLen;
    Command->Cmd16BitOperand = Instr->Mode16Oper;
    if (Command->CmdOrdinal == 0xDA)
        return 0;
    Command->Preffixes = Instr->Preffixes;
    if (Flags & OP_TTTn)
        Command->CmdFlags = GetCondition((UCHAR)Instr->OpCode);
    ByteParam = ((Flags & OP_W) != 0) && ((Instr->OpCode & 1) == 0);
    if ((Instr->OpcodeExt) && (Flags & OP_BYTE))
        ByteParam = 1;
    FpuCode = (Instr->InstrLen > 1) && ((Instr->OpCode & 0xF8) == 0xD8) && (!Instr->OpcodeExt);
    Reversed = ((Flags & OP_D) != 0) && ((Instr->OpCode & 2) == 0);
    MmxInstr = (Instr->OpcodeExt) && ((Flags & OP_MMX) != 0);
    if (ByteParam)
        DefSize = PRS_BYTE;
    else
        if (Instr->Mode16Oper)
            DefSize = PRS_WORD;
        else
            DefSize = PRS_DWORD;
    if (MmxInstr)
        DefSize = PRS_QWORD;
    if ((Instr->OpcodeExt) && (Flags & OP_SSE))
        DefSize = PRS_DQWORD;
    if ((MmxInstr) && ((Instr->OpCode == 0x6E) || (Instr->OpCode == 0x7E)))
        MmxInstr = 0;
    if (!FpuCode)
        GetInOpcodeRegister(Instr, Command, Flags);
    if (FpuCode)
        DecodeFpuParams(Instr, Command, Flags);
    if (Instr->ModRM.Present)
        if (!DecodeModRM(Instr, Command, Reversed, Flags, DefSize))
            return 0;
    if (Instr->SIB.Present)
        if (!DecodeSibParam(Instr, Command, Flags, DefSize))
            return 0;
    if (Instr->OffsetSize)
        DecodeOffsetParam(Instr, Command, Flags, DefSize);
    if (!((Instr->OpcodeExt) || (FpuCode)))
        DecodeSpecParams(Instr, Command, DefSize, &ImmRevers, &NotImm);
    if (!Instr->OpcodeWord)
        DecodeCmdSpecParams(Instr, Command, &NotImm);
    if ((Instr->ImmSize) && (!NotImm))
        DecodeImmParam(Instr, Command, Flags, DefSize);
    if ((Instr->OpcodeExt) && ((Instr->OpCode == 0xBE) || (Instr->OpCode == 0xB6))){
        Command->Params[0].Value = (Command->Params[0].Value & 7) | RGT_DWORD;
        if ((Command->Params[1].Flags & 7) == PRT_REG)
            Command->Params[1].Value = (Command->Params[1].Value & 0x0F) | RGT_BYTE;
        Command->Params[1].EFlags = (Command->Params[1].EFlags & 7) | PRS_BYTE;
        Command->Params[1].Flags = (Command->Params[1].Flags & 7) | PRS_BYTE;
    }
    if ((Instr->OpcodeExt) && ((Instr->OpCode == 0xB7) || (Instr->OpCode == 0xBF))){
        Command->Params[0].Value = (Command->Params[0].Value & 7) | RGT_DWORD;
        if ((Command->Params[1].Flags & 7) == PRT_REG)
            Command->Params[1].Value = (Command->Params[1].Value & 0x0F) | RGT_WORD;
        Command->Params[1].EFlags = (Command->Params[1].EFlags & 7) | PRS_WORD;
        Command->Params[1].Flags = (Command->Params[1].Flags & 7) | PRS_WORD;
    }
    if ((!Instr->OpcodeExt) && ((Instr->OpCode == 0xA0) || (Instr->OpCode == 0xA1)))
        Reversed = 0;
    if ((Reversed) || (Flags & OP_REV) || (ImmRevers)){
        Prt1 = Command->Params[0].Flags;
        Prm1 = Command->Params[0].Value;
        Command->Params[0].Flags = Command->Params[1].Flags;
        Command->Params[0].Value = Command->Params[1].Value;
        Command->Params[1].Flags = Prt1;
        Command->Params[1].Value = Prm1;
        Prt1 = (UCHAR)Command->Params[1].EFlags;
        Command->Params[1].EFlags = Command->Params[0].EFlags;
        Command->Params[0].EFlags = Prt1;
    }
    GetJumpFlags(Instr, Command);
    if (Flags & OP_SSE)
        MmxInstr = 1;
    if (!CleanDisasmStruct(Command, MmxInstr, Mode16))
        return 0;
    return 1;
}

char Regs[56][6] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "cr0", "", "cr2", "cr3", "cr4", "", "", "",
    "es", "cs", "ss", "ds", "fs", "gs", "", "",
    "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7",
    "st(0)", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"
};

char ExtRegs[24][5] = {
    "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7",
    "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7",
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
};

PCHAR GetRegName(ULONG Reg){
    PCHAR ret;

    if (Reg & 0x40)
        ret = (ExtRegs[Reg & 0x3F]);
    else
        ret = (Regs[Reg & 0x3F]);
    if (Reg == RGT_NONE)
        return 0;
    return ret;
}

char CondTable[16][4] = {
    "o", "no", "b", "nb", "z", "nz", "be", "nbe",
    "s", "ns", "pe", "np", "l", "nl", "le", "nle"
};

PCHAR GetConditionStr(PDisCommand Cmd){
    if (Cmd->CmdFlags & 1)
        return CondTable[(Cmd->CmdFlags & 0x1E) >> 1];
    else
        return 0;
}

char PtrType[7][11] = {
    "word ptr", "dword ptr", "qword ptr",
    "real4 ptr", "real8 ptr", "real10 ptr", "tbyte ptr"
};

PCHAR GetFpuWidthStr(USHORT Flags){
    return PtrType[(Flags & 0x700) >> 8];    
}

VOID __stdcall ToHex(ULONG Value, ULONG Digits, PCHAR OutBuff){
    __asm{
        mov eax, Value
        mov edx, Digits
        mov ecx, OutBuff
        push esi
        push edi
        push ecx
        mov esi, esp
        sub esp, 32
        push ecx
        push edx
        push esi
        D1:
        xor edx, edx
        mov ecx, 16
        div ecx
        dec esi
        add dl, '0'
        cmp dl, '0' + 10
        jb D2
        add dl, ('A' - '0') - 10
        D2:
        mov [esi], dl
        or eax, eax
        jne D1
        pop ecx
        pop edx
        sub ecx, esi
        sub edx, ecx
        jbe D5
        add ecx, edx
        mov al, '0'
        sub esi, edx
        jmp z
        zloop:
        mov [esi + edx], al
        z:
        dec edx
        jnz zloop
        mov [esi], al
        D5:
        pop edi
        rep movsb
        xor eax, eax
        mov [edi], al
        add esp, 32
        pop ecx
        pop edi
        pop esi
    }
}


VOID GetHexDump(ULONG Addr, ULONG Len, PCHAR OutBuff){
    PUCHAR cPtr = (PUCHAR)((__int64)Addr);
    char Tmp[6];

    OutBuff[0] = 0;
    while ((ULONG)((__int64)cPtr) < Addr + Len){
        ToHex(*cPtr, 2, Tmp);
        strcat(OutBuff, Tmp);
        cPtr++;
    }
}

VOID GetPrePfxStr(USHORT Prefixes, PCHAR OutBuff){
    OutBuff[0] = 0;
    if (Prefixes & OPX_LOCK)
        strcpy(OutBuff, "lock ");
    if (Prefixes & OPX_REP)
        strcat(OutBuff, "rep ");
    if (Prefixes & OPX_REPNZ)
        strcat(OutBuff, "repnz ");
}

char SegStrs[6][4] = {
    "cs:", "ds:", "es:", "ss:", "fs:", "gs:"
};

PCHAR GetSegmentPfxStr(USHORT Prefixes){
    switch (Prefixes & 0xFC){
        case OPX_CS:
            return SegStrs[0];
        case OPX_DS:
            return SegStrs[1];
        case OPX_ES:
            return SegStrs[2];
        case OPX_SS:
            return SegStrs[3];
        case OPX_FS:
            return SegStrs[4];
        case OPX_GS:
            return SegStrs[5];
    }
    return 0;
}

VOID GetSibStr(TParam Param, PCHAR OutBuff){
    ULONG Index = (Param.Value & 0xFF00) >> 8, Base = Param.Value & 0xFF;
    UCHAR Scale = (Param.Value & 0x30000) >> 16;
    char Tmp[6];

    strcpy(OutBuff, "[");
    if (Base != RGT_NONE){
        strcat(OutBuff, GetRegName(Base));
        strcat(OutBuff, "+");
    }
    strcat(OutBuff, GetRegName(Index));
    if (Scale){
        strcat(OutBuff, "*");
        ToHex(2 << (Scale - 1), 1, Tmp);
        strcat(OutBuff, Tmp);
    }
}

VOID GetSignedHexStr(ULONG Value, ULONG Flags, PCHAR OutBuff, PULONG Val, PBOOL Sign){
    ULONG Size;
    char Tmp[11];

    Size = 2 << ((Flags & 0x38) >> 3);
    *Sign = 0;
    *Val = 0;
    switch (Size){
        case 2:
            *Sign = ((Value & 0x80) == 0);
            *Val = Value & 0x7F;
            if (!(*Sign))
                *Val = 0x80 - *Val;
            break;
        case 4:
            *Sign = ((Value & 0x8000) == 0);
            *Val = Value & 0x7FFF;
            if (!(*Sign))
                *Val = 0x8000 - *Val;
            break;
        case 8:
            *Sign = ((Value & 0x80000000) == 0);
            *Val = Value & 0x7FFFFFFF;
            if (!(*Sign))
                *Val = 0x80000000 - *Val;
            break;
            break;
    }
    if (*Sign)
        strcpy(OutBuff, "+");
    else
        strcpy(OutBuff, "-");
    ToHex(*Val, Size, Tmp);
    strcat(OutBuff, Tmp);
}

char ModRm16[8][6] = {
    "bx+si", "bx+di", "bp+si", "bp+di",
    "si","di","bp","bx"
};

VOID AlignText(PCHAR Buff, UCHAR Aligment){
    ULONG r, Len = (ULONG)strlen(Buff);

    if (Len < Aligment){
        for (r = 0; r <= Aligment - Len; r++)
            Buff[Len + r] = ' ';
        Buff[Aligment] = 0;
    }
}

VOID _stdcall MakeMnemonic(PCHAR OutBuffer, PDisCommand Command, PMnemonicOptions Options){
    UCHAR r;
    ULONG pType, pSize, pValue, OffsetSize, Len, Val, EffectAddr;
    BOOL Sign, AddrPresent, MemOverride;
    char PtrStr[21], OffsetStr[21], Tmp[21], SegPfx[21];
    PCHAR pTmp;
	BOOL SymOk = 0;
	char sym[64];
	ULONG len;

    OutBuffer[0] = 0;
    if (Options->AlternativeAddres)
        EffectAddr = Options->AlternativeAddres;
    else
        EffectAddr = Command->CmdAddr;
    if (Options->AddAddresPart){
        ToHex(EffectAddr, 8, Tmp);
        strcat(OutBuffer, Tmp);
        strcat(OutBuffer, ": ");
    }
    if (Options->AddHexDump){
        GetHexDump(Command->CmdAddr, Command->CmdLength, Tmp);
        strcat(OutBuffer, Tmp);
    }
    if (Options->MnemonicAlign)
        AlignText(OutBuffer, Options->MnemonicAlign);
    GetPrePfxStr(Command->Preffixes, Tmp);
    strcat(OutBuffer, Tmp);
    strcat(OutBuffer, MnemArr[Command->CmdOrdinal]);
    if (Command->CmdOrdinal == 0xDA)
        return ;
    OffsetStr[0] = 0;
    PtrStr[0] = 0;
    SegPfx[0] = 0;
    AddrPresent = 0;
    OffsetSize = 0;
    MemOverride = 1;//((Command->CmdFlags & MEM_OVERRIDE) != 0);

    pTmp = GetConditionStr(Command);
    if (pTmp)
        strcat(OutBuffer, pTmp);
    strcat(OutBuffer, " ");
    pTmp = GetSegmentPfxStr(Command->Preffixes);
    if (pTmp)
        strcpy(SegPfx, pTmp);
    if (Command-> CmdFlags & JMP_FAR)
        strcat(OutBuffer, "far ");
    if (Command->ParamsNum)
        for (r = 0; r < Command->ParamsNum; r++)
            switch (Command->Params[r].Flags & 7){
                case PRT_DSP:
                    OffsetSize = 2 << ((Command->Params[r].Flags & 0x38) >> 3);
                    if (Command->Params[r].Value){
                        GetSignedHexStr(Command->Params[r].Value, Command->Params[r].Flags, Tmp, &Val, 
                            &Sign);
                        strcat(OffsetStr, Tmp);
                        strcat(OffsetStr, "h");
                    }
                    break;
                case PRT_MODRM:
                case PRT_SIB:
                    AddrPresent = 1;
                    break;
            }
    if (Command->CmdFlags & INSTR_FPU)
        strcpy(PtrStr, GetFpuWidthStr(Command->CmdFlags));
    if (Command->ParamsNum)
        for (r = 0; r < Command->ParamsNum; r++){
            pType = Command->Params[r].Flags & 0x7;
            pSize = Command->Params[r].Flags & 0x78;
            if (pType == PRT_DSP)
                pSize = Command->Params[r].EFlags & 0x38;
            pValue = Command->Params[r].Value;
            if (((pType == PRT_MODRM) || (pType == PRT_SIB) || (pType == PRT_DSP)) && 
                (!(Command->CmdFlags & INSTR_FPU))){
                    switch (pSize){
                        case PRS_BYTE:
                            strcpy(PtrStr, "byte ptr");
                            break;
                        case PRS_WORD:
                            strcpy(PtrStr, "word ptr");
                            break;
                        case PRS_DWORD:
                        case PRS_QWORD:
                            strcpy(PtrStr, "dword ptr");
                            break;
                    }
                }
            strcat(PtrStr, " ");
            if ((!MemOverride) && (!(Command->CmdFlags & INSTR_FPU)))
                PtrStr[0] = 0;
            switch (pType){
                case PRT_REG:
                    strcat(OutBuffer, GetRegName(pValue));
                    break;
                case PRT_MODRM:
                    strcat(OutBuffer, PtrStr);
                    strcat(OutBuffer, SegPfx);
                    strcat(OutBuffer, "[");
                    if (pValue & MODRM_32)
                        strcat(OutBuffer, GetRegName((pValue & 7) | RGT_DWORD));
                    else
                        strcat(OutBuffer, ModRm16[pValue & 7]);
                    strcat(OutBuffer, OffsetStr);
                    strcat(OutBuffer, "]");
                    break;
                case PRT_SIB:
                    strcat(OutBuffer, PtrStr);
                    strcat(OutBuffer, SegPfx);
                    GetSibStr(Command->Params[r], Tmp);
                    strcat(OutBuffer, Tmp);
                    strcat(OutBuffer, OffsetStr);
                    strcat(OutBuffer, "]");
                    break;
                case PRT_DSP:
                    if (!AddrPresent)
					{
                        strcat(OutBuffer, PtrStr);
                        strcat(OutBuffer, SegPfx);
                        strcat(OutBuffer, "[");

						sym[0] = 'W'; sym[1] = 0;
						len = sizeof(sym);
						if (SymGlobGetSymbolByAddress ((PVOID)pValue, sym, &len) == 0)
						{
							char *excl;

							KdPrint(("got sym for dsp: %s\n", sym));
							strcat (OutBuffer, sym);

							excl = strchr (sym, '!');
							if (excl)
								excl ++;
							else
								excl = sym;

							if (_stricmp(excl, "_imp_") == 0)
							{
								BOOL PrintHex = 1;

								if (MmIsAddressValid((PVOID)pValue))
								{
									PVOID ptr = *(PVOID*)pValue;

									len = sizeof(sym);
									if (SymGlobGetSymbolByAddress (ptr, sym, &len) == 0)
									{
										KdPrint(("Found imp symbol for %X [%X] '%s'\n", pValue, ptr, sym));
	
										strcat (OutBuffer, sym);

										PrintHex = 0;
									}
									else
									{
										KdPrint(("Could not find imp symbol for %X [%X]\n", pValue, ptr));
									}
								}

								// some import
								if (PrintHex)
								{
									ToHex (pValue, OffsetSize, Tmp);
									strcat(OutBuffer, Tmp);
								}
							}

							strcat (OutBuffer, "]");
						}
						else
						{
							ToHex(pValue, OffsetSize, Tmp);
							strcat(OutBuffer, Tmp);
							strcat(OutBuffer, "h]");
						}
					}
                    break;
                case PRT_IMM:
                    if ((Command->Params[r].Flags & 0x38) == PRS_FWORD)
					{
                        ToHex(Command->Params[r].EFlags, 4, Tmp);
                        strcat(OutBuffer, Tmp);
                        strcat(OutBuffer, ":");

						sym[0] = 'Z'; sym[1] = 0;
						len = sizeof(sym);
						if (SymGlobGetSymbolByAddress ((PVOID)pValue, sym, &len) == 0)
						{
							KdPrint(("got sym for imm, fword: %s\n", sym));
							strcpy (Tmp, sym);
						}
						else
						{
							ToHex(pValue, 8, Tmp);
							strcat(Tmp, "h");
						}
                    }
					else
					{
                        if (Command->CmdFlags & CMD_JUMP)
						{
                            GetSignedHexStr(pValue + Command->CmdLength, Command->Params[r].Flags, Tmp, 
                                &Val, &Sign);
                            if (Options->RealtiveOffsets)
							{
                                strcat(OutBuffer, "$");
							}
                            else
							{
                                if (Sign)
                                    Val = EffectAddr + Val;
                                else
                                    Val = EffectAddr - Val;
                                if (Command->Cmd16BitOperand)
                                    Val = Val & 0xFFFF;

								sym[0] = 'Y'; sym[1] = 0;
								len = sizeof(sym);
								if (SymGlobGetSymbolByAddress ((PVOID)Val, sym, &len) == 0)
								{
									KdPrint(("got sym for imm, jump: %s\n", sym));
									strcpy (Tmp, sym);
								}
								else
								{
									ToHex(Val, 2 << ((Command->Params[r].Flags & 0x38) >> 3), Tmp);
									strcat(Tmp, "h");
								}
                            }
                        } 
						else
						{
							ULONG Bits = 2 << ((Command->Params[r].Flags & 0x38) >> 3);

							sym[0] = 'X'; sym[1] = 0;
							len = sizeof(sym);
							if (Bits == 32 &&
								SymGlobGetSymbolByAddress ((PVOID)pValue, sym, &len) == 0)
							{
								KdPrint(("got sym for imm32: %s\n", sym));
								strcpy (Tmp, sym);
							}
							else
							{
								ToHex(pValue, Bits, Tmp);
								strcat(Tmp, "h");
							}
						}
					}
                    
                    strcat(OutBuffer, Tmp);
                    break;
            }
            if ((pType != PRT_DSP) || (!AddrPresent))
                strcat(OutBuffer, ", ");
        }
    Len = (ULONG)strlen(OutBuffer) - 2;
    if (OutBuffer[Len] == ',')
        OutBuffer[Len] = 0;
}