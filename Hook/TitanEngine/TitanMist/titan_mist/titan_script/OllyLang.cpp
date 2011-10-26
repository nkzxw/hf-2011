#include "OllyLang.h"

#include <algorithm>
#include "HelperFunctions.h"
#include "Dialogs.h"
#include "TE_Interface.h"
#include "Opcodes.h"
#include "globals.h"
#include "Debug.h"

using std::find;
using std::min;

#ifdef _WIN64

const OllyLang::register_t OllyLang::registers[] =
{
	{"rax",  UE_RAX, 8, 0}, {"rbx",  UE_RBX, 8, 0}, {"rcx",  UE_RCX, 8, 0},
	{"rdx",  UE_RDX, 8, 0}, {"rsi",  UE_RSI, 8, 0}, {"rdi",  UE_RDI, 8, 0},
	{"rbp",  UE_RBP, 8, 0}, {"rsp",  UE_RSP, 8, 0}, {"rip",  UE_RIP, 8, 0},

	{"r8",   UE_R8,  8, 0}, {"r9",   UE_R9,  8, 0}, {"r10",  UE_R10, 8, 0},
	{"r11",  UE_R11, 8, 0}, {"r12",  UE_R12, 8, 0}, {"r13",  UE_R13, 8, 0},
	{"r14",  UE_R14, 8, 0}, {"r15",  UE_R15, 8, 0},

	{"dr0",  UE_DR0, 8, 0}, {"dr1",  UE_DR1, 8, 0}, {"dr2",  UE_DR2, 8, 0},
	{"dr3",  UE_DR3, 8, 0}, {"dr6",  UE_DR6, 8, 0}, {"dr7",  UE_DR7, 8, 0},

	{"eax",  UE_RAX, 4, 0}, {"ebx",  UE_RBX, 4, 0}, {"ecx",  UE_RCX, 4, 0},
	{"edx",  UE_RDX, 4, 0}, {"esi",  UE_RSI, 4, 0}, {"edi",  UE_RDI, 4, 0},
	{"ebp",  UE_RBP, 4, 0}, {"esp",  UE_RSP, 4, 0},

	{"r8d",  UE_R8,  4, 0}, {"r9d",  UE_R9,  4, 0}, {"r10d", UE_R10, 4, 0},
	{"r11d", UE_R11, 4, 0}, {"r12d", UE_R12, 4, 0}, {"r13d", UE_R13, 4, 0},
	{"r14d", UE_R14, 4, 0}, {"r15d", UE_R15, 4, 0},

	{"ax",   UE_RAX, 2, 0}, {"bx",   UE_RBX, 2, 0}, {"cx",   UE_RCX, 2, 0},
	{"dx",   UE_RDX, 2, 0}, {"si",   UE_RSI, 2, 0}, {"di",   UE_RDI, 2, 0},
	{"bp",   UE_RBP, 2, 0}, {"sp",   UE_RSP, 2, 0},

	{"r8w",  UE_R8,  2, 0}, {"r9w",  UE_R9,  2, 0}, {"r10w", UE_R10, 2, 0},
	{"r11w", UE_R11, 2, 0}, {"r12w", UE_R12, 2, 0}, {"r13w", UE_R13, 2, 0},
	{"r14w", UE_R14, 2, 0}, {"r15w", UE_R15, 2, 0},

	{"ah",   UE_RAX, 1, 1}, {"bh",   UE_RBX, 1, 1}, {"ch",   UE_RCX, 1, 1},
	{"dh",   UE_RDX, 1, 1},

	{"al",   UE_RAX, 1, 0}, {"bl",   UE_RBX, 1, 0}, {"cl",   UE_RCX, 1, 0},
	{"dl",   UE_RDX, 1, 0}, {"sil",  UE_RSI, 1, 0}, {"dil",  UE_RDI, 1, 0},
	{"bpl",  UE_RBP, 1, 0}, {"spl",  UE_RSP, 1, 0},

	{"r8b",  UE_R8,  1, 0}, {"r9b",  UE_R9,  1, 0}, {"r10b", UE_R10, 1, 0},
	{"r11b", UE_R11, 1, 0}, {"r12b", UE_R12, 1, 0}, {"r13b", UE_R13, 1, 0},
	{"r14b", UE_R14, 1, 0}, {"r15b", UE_R15, 1, 0},
};

#else

const OllyLang::register_t OllyLang::registers[] =
{
	{"eax", UE_EAX, 4, 0}, {"ebx", UE_EBX, 4, 0}, {"ecx", UE_ECX, 4, 0},
	{"edx", UE_EDX, 4, 0}, {"esi", UE_ESI, 4, 0}, {"edi", UE_EDI, 4, 0},
	{"ebp", UE_EBP, 4, 0}, {"esp", UE_ESP, 4, 0}, {"eip", UE_EIP, 4, 0},

	{"dr0", UE_DR0, 4, 0}, {"dr1", UE_DR1, 4, 0}, {"dr2", UE_DR2, 4, 0},
	{"dr3", UE_DR3, 4, 0}, {"dr6", UE_DR6, 4, 0}, {"dr7", UE_DR7, 4, 0},

	{"ax",  UE_EAX, 2, 0}, {"bx",  UE_EBX, 2, 0}, {"cx",  UE_ECX, 2, 0},
	{"dx",  UE_EDX, 2, 0}, {"si",  UE_ESI, 2, 0}, {"di",  UE_EDI, 2, 0},
	{"bp",  UE_EBP, 2, 0}, {"sp",  UE_ESP, 2, 0},

	{"ah",  UE_EAX, 1, 1}, {"bh",  UE_EBX, 1, 1}, {"ch",  UE_ECX, 1, 1},
	{"dh",  UE_EDX, 1, 1},

	{"al",  UE_EAX, 1, 0}, {"bl",  UE_EBX, 1, 0}, {"cl",  UE_ECX, 1, 0},
	{"dl",  UE_EDX, 1, 0}
};

#endif

const string OllyLang::fpu_registers[] = { "st(0)", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)" };

const string OllyLang::e_flags[] = { "!cf", "!pf", "!af", "!zf", "!sf", "!df", "!of" };

const OllyLang::constant_t OllyLang::constants[] =
{
	{"true",  1},
	{"false", 0},
	{"null",  0},

	{"ue_access_read",  UE_ACCESS_READ},
	{"ue_access_write", UE_ACCESS_WRITE},
	{"ue_access_all",   UE_ACCESS_ALL},

	{"ue_pe_offset",              UE_PE_OFFSET},
	{"ue_imagebase",              UE_IMAGEBASE},
	{"ue_oep",                    UE_OEP},
	{"ue_sizeofimage",            UE_SIZEOFIMAGE},
	{"ue_sizeofheaders",          UE_SIZEOFHEADERS},
	{"ue_sizeofoptionalheader",   UE_SIZEOFOPTIONALHEADER},
	{"ue_sectionalignment",       UE_SECTIONALIGNMENT},
	{"ue_importtableaddress",     UE_IMPORTTABLEADDRESS},
	{"ue_importtablesize",        UE_IMPORTTABLESIZE},
	{"ue_resourcetableaddress",   UE_RESOURCETABLEADDRESS},
	{"ue_resourcetablesize",      UE_RESOURCETABLESIZE},
	{"ue_exporttableaddress",     UE_EXPORTTABLEADDRESS},
	{"ue_exporttablesize",        UE_EXPORTTABLESIZE},
	{"ue_tlstableaddress",        UE_TLSTABLEADDRESS},
	{"ue_tlstablesize",           UE_TLSTABLESIZE},
	{"ue_relocationtableaddress", UE_RELOCATIONTABLEADDRESS},
	{"ue_relocationtablesize",    UE_RELOCATIONTABLESIZE},
	{"ue_timedatestamp",          UE_TIMEDATESTAMP},
	{"ue_sectionnumber",          UE_SECTIONNUMBER},
	{"ue_checksum",               UE_CHECKSUM},
	{"ue_subsystem",              UE_SUBSYSTEM},
	{"ue_characteristics",        UE_CHARACTERISTICS},
	{"ue_numberofrvaandsizes",    UE_NUMBEROFRVAANDSIZES},
	{"ue_sectionname",            UE_SECTIONNAME},
	{"ue_sectionvirtualoffset",   UE_SECTIONVIRTUALOFFSET},
	{"ue_sectionvirtualsize",     UE_SECTIONVIRTUALSIZE},
	{"ue_sectionrawoffset",       UE_SECTIONRAWOFFSET},
	{"ue_sectionrawsize",         UE_SECTIONRAWSIZE},
	{"ue_sectionflags",           UE_SECTIONFLAGS},

	{"ue_ch_breakpoint",              UE_CH_BREAKPOINT},
	{"ue_ch_singlestep",              UE_CH_SINGLESTEP},
	{"ue_ch_accessviolation",         UE_CH_ACCESSVIOLATION},
	{"ue_ch_illegalinstruction",      UE_CH_ILLEGALINSTRUCTION},
	{"ue_ch_noncontinuableexception", UE_CH_NONCONTINUABLEEXCEPTION},
	{"ue_ch_arrayboundsexception",    UE_CH_ARRAYBOUNDSEXCEPTION},
	{"ue_ch_floatdenormaloperand",    UE_CH_FLOATDENORMALOPERAND},
	{"ue_ch_floatdevidebyzero",       UE_CH_FLOATDEVIDEBYZERO},
	{"ue_ch_integerdevidebyzero",     UE_CH_INTEGERDEVIDEBYZERO},
	{"ue_ch_integeroverflow",         UE_CH_INTEGEROVERFLOW},
	{"ue_ch_privilegedinstruction",   UE_CH_PRIVILEGEDINSTRUCTION},
	{"ue_ch_pageguard",               UE_CH_PAGEGUARD},
	{"ue_ch_everythingelse",          UE_CH_EVERYTHINGELSE},
	{"ue_ch_createthread",            UE_CH_CREATETHREAD},
	{"ue_ch_exitthread",              UE_CH_EXITTHREAD},
	{"ue_ch_createprocess",           UE_CH_CREATEPROCESS},
	{"ue_ch_exitprocess",             UE_CH_EXITPROCESS},
	{"ue_ch_loaddll",                 UE_CH_LOADDLL},
	{"ue_ch_unloaddll",               UE_CH_UNLOADDLL},
	{"ue_ch_outputdebugstring",       UE_CH_OUTPUTDEBUGSTRING},

	{"ue_on_lib_load",   UE_ON_LIB_LOAD},
	{"ue_on_lib_unload", UE_ON_LIB_UNLOAD},
	{"ue_on_lib_all",    UE_ON_LIB_ALL}
};

OllyLang::OllyLang()
{
	// Init command array
	commands["add"] = &OllyLang::DoADD;
	commands["ai"] = &OllyLang::DoAI;
	commands["alloc"] = &OllyLang::DoALLOC;
	commands["an"] = &OllyLang::DoAN;
	commands["and"] = &OllyLang::DoAND;
	commands["ao"] = &OllyLang::DoAO;
	commands["ask"] = &OllyLang::DoASK;
	commands["asm"] = &OllyLang::DoASM;	
	commands["asmtxt"] = &OllyLang::DoASMTXT;	
	commands["atoi"] = &OllyLang::DoATOI;
	commands["backup"] = &OllyLang::DoOPENDUMP;
	commands["bc"] = &OllyLang::DoBC;
	commands["bd"] = &OllyLang::DoBD;
	commands["beginsearch"] = &OllyLang::DoBEGINSEARCH;
	commands["bp"] = &OllyLang::DoBP;
	commands["bpcnd"] = &OllyLang::DoBPCND;
	commands["bpd"] = &OllyLang::DoBPD;
	commands["bpgoto"] = &OllyLang::DoBPGOTO;
	commands["bphwcall"] = &OllyLang::DoBPHWCA;
	commands["bphwc"] = &OllyLang::DoBPHWC;
	commands["bphws"] = &OllyLang::DoBPHWS;
	commands["bpl"] = &OllyLang::DoBPL;
	commands["bplcnd"] = &OllyLang::DoBPLCND;
	commands["bpmc"] = &OllyLang::DoBPMC;
	commands["bprm"] = &OllyLang::DoBPRM;
	commands["bpwm"] = &OllyLang::DoBPWM;
	commands["bpx"] = &OllyLang::DoBPX;
	commands["buf"] = &OllyLang::DoBUF;
	commands["call"] = &OllyLang::DoCALL;
	commands["close"] = &OllyLang::DoCLOSE;
	commands["cmp"] = &OllyLang::DoCMP;
	commands["cmt"] = &OllyLang::DoCMT;
	commands["cob"] = &OllyLang::DoCOB;
	commands["coe"] = &OllyLang::DoCOE;
	commands["dbh"] = &OllyLang::DoDBH;
	commands["dbs"] = &OllyLang::DoDBS;
	commands["dec"] = &OllyLang::DoDEC;
	commands["div"] = &OllyLang::DoDIV;
	commands["dm"] = &OllyLang::DoDM;
	commands["dma"] = &OllyLang::DoDMA;
	commands["dpe"] = &OllyLang::DoDPE;
	commands["ende"] = &OllyLang::DoENDE;
	commands["endsearch"] = &OllyLang::DoENDSEARCH;
	commands["erun"] = &OllyLang::DoERUN;
	commands["esti"] = &OllyLang::DoESTI;
	commands["esto"] = &OllyLang::DoERUN;
	commands["estep"] = &OllyLang::DoESTEP;
	commands["eob"] = &OllyLang::DoEOB;
	commands["eoe"] = &OllyLang::DoEOE;
	commands["eval"] = &OllyLang::DoEVAL;
	commands["exec"] = &OllyLang::DoEXEC;
	commands["fill"] = &OllyLang::DoFILL;
	commands["find"] = &OllyLang::DoFIND;
	commands["findcalls"] = &OllyLang::DoFINDCALLS;
	commands["findcmd"] = &OllyLang::DoFINDCMD;
	commands["findcmds"] = &OllyLang::DoFINDCMD;
	commands["findop"] = &OllyLang::DoFINDOP;
	commands["findmem"] = &OllyLang::DoFINDMEM;
	commands["free"] = &OllyLang::DoFREE;
	commands["gapi"] = &OllyLang::DoGAPI;
	commands["gbpm"] = &OllyLang::DoGBPM;
	commands["gbpr"] = &OllyLang::DoGBPR;
	commands["gci"] = &OllyLang::DoGCI;
	commands["gcmt"] = &OllyLang::DoGCMT;
	commands["gfo"] = &OllyLang::DoGFO;
	commands["glbl"] = &OllyLang::DoGLBL;
	commands["gmemi"] = &OllyLang::DoGMEMI;
	commands["gmexp"] = &OllyLang::DoGMEXP;
	commands["gma"] = &OllyLang::DoGMA;
	commands["gmi"] = &OllyLang::DoGMI;
	commands["gmimp"] = &OllyLang::DoGMIMP;
	commands["gn"] = &OllyLang::DoGN;
	commands["go"] = &OllyLang::DoGO;
	commands["gopi"] = &OllyLang::DoGOPI;	
	commands["gpa"] = &OllyLang::DoGPA;
	commands["gpi"] = &OllyLang::DoGPI;
	commands["gpp"] = &OllyLang::DoGPP;
	commands["gro"] = &OllyLang::DoGRO;
	commands["gref"] = &OllyLang::DoGREF;
	commands["gsl"] = &OllyLang::DoGSL;
	commands["gstr"] = &OllyLang::DoGSTR;
	commands["handle"] = &OllyLang::DoHANDLE;
	commands["history"] = &OllyLang::DoHISTORY;
	commands["inc"] = &OllyLang::DoINC;
	commands["itoa"] = &OllyLang::DoITOA;

	commands["jmp"] = &OllyLang::DoJMP;

	commands["ja"]   = &OllyLang::DoJA;
	commands["jg"]   = &OllyLang::DoJA;
	commands["jnbe"] = &OllyLang::DoJA;
	commands["jnle"] = &OllyLang::DoJA;

	commands["jae"] = &OllyLang::DoJAE;
	commands["jge"] = &OllyLang::DoJAE;
	commands["jnb"] = &OllyLang::DoJAE;
	commands["jnl"] = &OllyLang::DoJAE;

	commands["jnae"] = &OllyLang::DoJB;
	commands["jnge"] = &OllyLang::DoJB;
	commands["jb"]   = &OllyLang::DoJB;
	commands["jl"]   = &OllyLang::DoJB;

	commands["jna"] = &OllyLang::DoJBE;
	commands["jng"] = &OllyLang::DoJBE;
	commands["jbe"] = &OllyLang::DoJBE;
	commands["jle"] = &OllyLang::DoJBE;

	commands["je"] = &OllyLang::DoJE;
	commands["jz"] = &OllyLang::DoJE;
	
	commands["jne"] = &OllyLang::DoJNE;
	commands["jnz"] = &OllyLang::DoJNE;

	commands["key"] = &OllyLang::DoKEY;
	commands["lbl"] = &OllyLang::DoLBL;
	commands["lc"] = &OllyLang::DoLC;
	commands["lclr"] = &OllyLang::DoLCLR;
	commands["len"] = &OllyLang::DoLEN;
	commands["loadlib"] = &OllyLang::DoLOADLIB;
	commands["lm"] = &OllyLang::DoLM;
	commands["log"] = &OllyLang::DoLOG;
	commands["logbuf"] = &OllyLang::DoLOGBUF;
	commands["memcpy"] = &OllyLang::DoMEMCPY;
	commands["mov"] = &OllyLang::DoMOV;
	commands["msg"] = &OllyLang::DoMSG;
	commands["msgyn"] = &OllyLang::DoMSGYN;
	commands["mul"] = &OllyLang::DoMUL;
	commands["names"] = &OllyLang::DoNAMES;	
	commands["neg"] = &OllyLang::DoNEG;
	commands["not"] = &OllyLang::DoNOT;
	commands["or"] = &OllyLang::DoOR;
	commands["olly"] = &OllyLang::DoOLLY;
	commands["opcode"] = &OllyLang::DoOPCODE;
	commands["opendump"] = &OllyLang::DoOPENDUMP;
	commands["opentrace"] = &OllyLang::DoOPENTRACE;
	commands["pause"] = &OllyLang::DoPAUSE;
	commands["pop"] = &OllyLang::DoPOP;
	commands["popa"] = &OllyLang::DoPOPA;
	commands["preop"] = &OllyLang::DoPREOP;
	commands["push"] = &OllyLang::DoPUSH;
	commands["pusha"] = &OllyLang::DoPUSHA;
	commands["rbp"] = &OllyLang::DoRBP;
	commands["readstr"] = &OllyLang::DoREADSTR;
	commands["refresh"] = &OllyLang::DoREFRESH;
	commands["ref"] = &OllyLang::DoREF;
	commands["repl"] = &OllyLang::DoREPL;
	commands["reset"] = &OllyLang::DoRESET;
	commands["ret"] = &OllyLang::DoRET;
	commands["rev"] = &OllyLang::DoREV;
	commands["rol"] = &OllyLang::DoROL;
	commands["ror"] = &OllyLang::DoROR;
	commands["rtr"] = &OllyLang::DoRTR;
	commands["rtu"] = &OllyLang::DoRTU;
	commands["run"] = &OllyLang::DoRUN;
	commands["sbp"] = &OllyLang::DoSBP;
	commands["scmp"] = &OllyLang::DoSCMP;
	commands["scmpi"] = &OllyLang::DoSCMPI;
	commands["setoption"] = &OllyLang::DoSETOPTION;
	commands["shl"] = &OllyLang::DoSHL;
	commands["shr"] = &OllyLang::DoSHR;
	commands["step"] = &OllyLang::DoSTO;
	commands["sti"] = &OllyLang::DoSTI;
	commands["sto"] = &OllyLang::DoSTO;
	commands["str"] = &OllyLang::DoSTR;
	commands["sub"] = &OllyLang::DoSUB;
	commands["tc"] = &OllyLang::DoTC;
	commands["test"] = &OllyLang::DoTEST;
	commands["ti"] = &OllyLang::DoTI;
	commands["tick"] = &OllyLang::DoTICK;
	commands["ticnd"] = &OllyLang::DoTICND;
	commands["to"] = &OllyLang::DoTO;
	commands["tocnd"] = &OllyLang::DoTOCND;
	commands["ubp"] = &OllyLang::DoBP;
	commands["unicode"] = &OllyLang::DoUNICODE;
	commands["var"] = &OllyLang::DoVAR;
	commands["xor"] = &OllyLang::DoXOR;
	commands["xchg"] = &OllyLang::DoXCHG;
	commands["wrt"] = &OllyLang::DoWRT;
	commands["wrta"] = &OllyLang::DoWRTA;

	commands["error"] = &OllyLang::DoError;
	commands["dnf"] = &OllyLang::DoDumpAndFix;
	commands["stopdebug"] = &OllyLang::DoStopDebug;
	commands["dumpprocess"] = &OllyLang::DoDumpProcess;
	commands["dumpregions"] = &OllyLang::DoDumpRegions;
	commands["dumpmodule"] = &OllyLang::DoDumpModule;
	commands["pastepeheader"] = &OllyLang::DoPastePEHeader;
	commands["extractoverlay"] = &OllyLang::DoExtractOverlay;
	commands["addoverlay"] = &OllyLang::DoAddOverlay;
	commands["copyoverlay"] = &OllyLang::DoCopyOverlay;
	commands["removeoverlay"] = &OllyLang::DoRemoveOverlay;
	commands["resortfilesections"] = &OllyLang::DoResortFileSections;
	commands["makeallsectionsrwe"] = &OllyLang::DoMakeAllSectionsRWE;
	commands["addnewsection"] = &OllyLang::DoAddNewSection;
	commands["resizelastsection"] = &OllyLang::DoResizeLastSection;
	commands["getpe32data"] = &OllyLang::DoGetPE32Data;
	commands["setpe32data"] = &OllyLang::DoSetPE32Data;
	commands["getpe32sectionnumberfromva"] = &OllyLang::DoGetPE32SectionNumberFromVA;
	commands["convertvatofileoffset"] = &OllyLang::DoConvertVAtoFileOffset;
	commands["convertfileoffsettova"] = &OllyLang::DoConvertFileOffsetToVA;
	commands["isfiledll"] = &OllyLang::DoIsFileDLL;
	commands["realignpe"] = &OllyLang::DoRealignPE;
	commands["relocatercleanup"] = &OllyLang::DoRelocaterCleanup;
	commands["relocaterinit"] = &OllyLang::DoRelocaterInit;
	commands["relocateraddnewrelocation"] = &OllyLang::DoRelocaterAddNewRelocation;
	commands["relocaterestimatedsize"] = &OllyLang::DoRelocaterEstimatedSize;
	commands["relocaterexportrelocation"] = &OllyLang::DoRelocaterExportRelocation;
	commands["relocaterexportrelocationex"] = &OllyLang::DoRelocaterExportRelocationEx;
	commands["relocatermakesnapshot"] = &OllyLang::DoRelocaterMakeSnapshot;
	commands["relocatercomparetwosnapshots"] = &OllyLang::DoRelocaterCompareTwoSnapshots;
	commands["relocaterchangefilebase"] = &OllyLang::DoRelocaterChangeFileBase;
	commands["threaderpausethread"] = &OllyLang::DoThreaderPauseThread;
	commands["threaderresumethread"] = &OllyLang::DoThreaderResumeThread;
	commands["threaderterminatethread"] = &OllyLang::DoThreaderTerminateThread;
	commands["threaderpauseallthreads"] = &OllyLang::DoThreaderPauseAllThreads;
	commands["threaderresumeallthreads"] = &OllyLang::DoThreaderResumeAllThreads;
	commands["getdebuggeddllbaseaddress"] = &OllyLang::DoGetDebuggedDLLBaseAddress;
	commands["getdebuggedfilebaseaddress"] = &OllyLang::DoGetDebuggedFileBaseAddress;
	commands["getjumpdestination"] = &OllyLang::DoGetJumpDestination;
	commands["isjumpgoingtoexecute"] = &OllyLang::DoIsJumpGoingToExecute;
	commands["getpeblocation"] = &OllyLang::DoGetPEBLocation;
	commands["detachdebuggerex"] = &OllyLang::DoDetachDebuggerEx;
	commands["setcustomhandler"] = &OllyLang::DoSetCustomHandler;
	commands["importercleanup"] = &OllyLang::DoImporterCleanup;
	commands["importersetimagebase"] = &OllyLang::DoImporterSetImageBase;
	commands["importerinit"] = &OllyLang::DoImporterInit;
	commands["importeraddnewdll"] = &OllyLang::DoImporterAddNewDll;
	commands["importeraddnewapi"] = &OllyLang::DoImporterAddNewAPI;
	commands["importeraddnewordinalapi"] = &OllyLang::DoImporterAddNewOrdinalAPI;
	commands["importergetaddeddllcount"] = &OllyLang::DoImporterGetAddedDllCount;
	commands["importergetaddedapicount"] = &OllyLang::DoImporterGetAddedAPICount;
	commands["importermoveiat"] = &OllyLang::DoImporterMoveIAT;
	commands["importerrelocatewritelocation"] = &OllyLang::DoImporterRelocateWriteLocation;
	commands["importerexportiat"] = &OllyLang::DoImporterExportIAT;
	commands["importerestimatedsize"] = &OllyLang::DoImporterEstimatedSize;
	commands["importerexportiatex"] = &OllyLang::DoImporterExportIATEx;
	commands["importergetnearestapiaddress"] = &OllyLang::DoImporterGetNearestAPIAddress;
	commands["importerautosearchiat"] = &OllyLang::DoImporterAutoSearchIAT;
	commands["importerautosearchiatex"] = &OllyLang::DoImporterAutoSearchIATEx;
	commands["importerautofixiatex"] = &OllyLang::DoImporterAutoFixIATEx;
	commands["importerautofixiat"] = &OllyLang::DoImporterAutoFixIAT;
	commands["tracerlevel1"] = &OllyLang::DoTracerLevel1;
	commands["hashtracerlevel1"] = &OllyLang::DoHashTracerLevel1;
	commands["tracerdetectredirection"] = &OllyLang::DoTracerDetectRedirection;
	commands["tracerfixknownredirection"] = &OllyLang::DoTracerFixKnownRedirection;
	commands["tracerfixredirectionviaimprecplugin"] = &OllyLang::DoTracerFixRedirectionViaImpRecPlugin;
	commands["exportercleanup"] = &OllyLang::DoExporterCleanup;
	commands["exportersetimagebase"] = &OllyLang::DoExporterSetImageBase;
	commands["exporterinit"] = &OllyLang::DoExporterInit;
	commands["exporteraddnewexport"] = &OllyLang::DoExporterAddNewExport;
	commands["exporteraddnewordinalexport"] = &OllyLang::DoExporterAddNewOrdinalExport;
	commands["exportergetaddedexportcount"] = &OllyLang::DoExporterGetAddedExportCount;
	commands["exporterestimatedsize"] = &OllyLang::DoExporterEstimatedSize;
	commands["exporterbuildexporttable"] = &OllyLang::DoExporterBuildExportTable;
	commands["exporterbuildexporttableex"] = &OllyLang::DoExporterBuildExportTableEx;
	commands["librariansetbreakpoint"] = &OllyLang::DoLibrarianSetBreakPoint;
	commands["librarianremovebreakpoint"] = &OllyLang::DoLibrarianRemoveBreakPoint;
	commands["tlsremovecallback"] = &OllyLang::DoTLSRemoveCallback;
	commands["tlsremovetable"] = &OllyLang::DoTLSRemoveTable;
	commands["tlsbackupdata"] = &OllyLang::DoTLSBackupData;
	commands["tlsrestoredata"] = &OllyLang::DoTLSRestoreData;
	commands["handlerishandleopen"] = &OllyLang::DoHandlerIsHandleOpen;
	commands["handlercloseremotehandle"] = &OllyLang::DoHandlerCloseRemoteHandle;
	commands["staticfileload"] = &OllyLang::DoStaticFileLoad;
	commands["staticfileunload"] = &OllyLang::DoStaticFileUnload;

	CustomHandlerCallbacks[UE_CH_BREAKPOINT] = &OllyLang::CHC_BREAKPOINT;
	CustomHandlerCallbacks[UE_CH_SINGLESTEP] = &OllyLang::CHC_SINGLESTEP;
	CustomHandlerCallbacks[UE_CH_ACCESSVIOLATION] = &OllyLang::CHC_ACCESSVIOLATION;
	CustomHandlerCallbacks[UE_CH_ILLEGALINSTRUCTION] = &OllyLang::CHC_ILLEGALINSTRUCTION;
	CustomHandlerCallbacks[UE_CH_NONCONTINUABLEEXCEPTION] = &OllyLang::CHC_NONCONTINUABLEEXCEPTION;
	CustomHandlerCallbacks[UE_CH_ARRAYBOUNDSEXCEPTION] = &OllyLang::CHC_ARRAYBOUNDSEXCEPTION;
	CustomHandlerCallbacks[UE_CH_FLOATDENORMALOPERAND] = &OllyLang::CHC_FLOATDENORMALOPERAND;
	CustomHandlerCallbacks[UE_CH_FLOATDEVIDEBYZERO] = &OllyLang::CHC_FLOATDEVIDEBYZERO;
	CustomHandlerCallbacks[UE_CH_INTEGERDEVIDEBYZERO] = &OllyLang::CHC_INTEGERDEVIDEBYZERO;
	CustomHandlerCallbacks[UE_CH_INTEGEROVERFLOW] = &OllyLang::CHC_INTEGEROVERFLOW;
	CustomHandlerCallbacks[UE_CH_PRIVILEGEDINSTRUCTION] = &OllyLang::CHC_PRIVILEGEDINSTRUCTION;
	CustomHandlerCallbacks[UE_CH_PAGEGUARD] = &OllyLang::CHC_PAGEGUARD;
	CustomHandlerCallbacks[UE_CH_EVERYTHINGELSE] = &OllyLang::CHC_EVERYTHINGELSE;
	CustomHandlerCallbacks[UE_CH_CREATETHREAD] = &OllyLang::CHC_CREATETHREAD;
	CustomHandlerCallbacks[UE_CH_EXITTHREAD] = &OllyLang::CHC_EXITTHREAD;
	CustomHandlerCallbacks[UE_CH_CREATEPROCESS] = &OllyLang::CHC_CREATEPROCESS;
	CustomHandlerCallbacks[UE_CH_EXITPROCESS] = &OllyLang::CHC_EXITPROCESS;
	CustomHandlerCallbacks[UE_CH_LOADDLL] = &OllyLang::CHC_LOADDLL;
	CustomHandlerCallbacks[UE_CH_UNLOADDLL] = &OllyLang::CHC_UNLOADDLL;
	CustomHandlerCallbacks[UE_CH_OUTPUTDEBUGSTRING] = &OllyLang::CHC_OUTPUTDEBUGSTRING;

	LibraryBreakpointCallbacks[UE_ON_LIB_LOAD] = &OllyLang::LBPC_LOAD;
	LibraryBreakpointCallbacks[UE_ON_LIB_UNLOAD] = &OllyLang::LBPC_UNLOAD;
	LibraryBreakpointCallbacks[UE_ON_LIB_ALL] = &OllyLang::LBPC_ALL;

	/*
	variables["$RESULT"] = 0;

	script_running = false;

	script_pos_next = 0;
	EOB_row = EOE_row = -1;
	zf = cf = 0;
	log_commands = false;
	search_buffer = NULL;

	back_to_debugloop = false;
	require_addonaction = false;

	saved_bp = 0;
	alloc_bp = 0;
	//softbp_t = NULL;

	//for(int i = 0; i < 4; i++)
	//	hwbp_t[i].addr = 0;
	*/
}

OllyLang::~OllyLang()
{
	if(search_buffer)
		DoENDSEARCH(NULL, 0);
	FreeBpMem();
	freeMemBlocks();
}

void OllyLang::OllyScript::parse_insert(const vector<string>& toInsert, const wstring& currentdir)
{
uint curline = 1;
bool in_comment = false, in_asm = false;

	loaded = true;

	for(size_t i = 0; i < toInsert.size(); i++, curline++)
	{
		string scriptline = trim(toInsert[i]);
		bool nextline = false;
		size_t curpos = 0; // for skipping string literals

		while(!nextline)
		{
			// Handle comments and string literals
			size_t linecmt = string::npos, spancmt = string::npos, strdel = string::npos;

			if(curpos < scriptline.length())
			if(in_comment)
			{
				spancmt = 0;
			}
			else
			{
				size_t min = string::npos, tmp;

				tmp = scriptline.find("//", curpos);
				if(tmp < min)
					min = linecmt = tmp;
				tmp = scriptline.find(';', curpos);
				if(tmp < min)
					min = linecmt = tmp;
				tmp = scriptline.find("/*", curpos);
				if(tmp < min)
					min = spancmt = tmp;
				tmp = scriptline.find('\"', curpos);
				if(tmp < min)
					min = strdel = tmp;

				curpos = min;

				if(linecmt != min)
					linecmt = string::npos;
				if(spancmt != min)
					spancmt = string::npos;
				if(strdel != min)
					strdel = string::npos;
			}

			if(strdel != string::npos)
			{
				curpos = scriptline.find('\"', strdel+1); // find end of string
				if(curpos != string::npos)
					curpos++;
			}
			else if(linecmt != string::npos) 
			{
				scriptline.resize(linecmt);
			}
			else if(spancmt != string::npos)
			{
				size_t start = in_comment ? spancmt : spancmt+2;
				size_t end = scriptline.find("*/", start);
				in_comment = (end == string::npos);
				if(in_comment)
					scriptline.resize(spancmt);
				else
					scriptline.erase(spancmt, end-spancmt+2);
			}
			else
			{
				scriptline = trim(scriptline);
				size_t len = scriptline.length();

				if(len)
				{
					string lcline = tolower(scriptline);

					// Check for label
					if(!in_asm && len > 1 && scriptline[len-1] == ':')
					{
						scriptline.resize(len-1);
						labels[trim(scriptline)] = lines.size();
					}
					// Check for #inc and include file if it exists
					else if(0 == lcline.find("#inc"))
					{
						if(len > 5 && strchr(whitespaces, lcline[4]))
						{
							string args = trim(scriptline.substr(5));
							string filename;
							if(args.size() > 2 && *args.begin() == '\"' && *args.rbegin() == '\"')
							{
								wstring dir;
								wstring filename = ascii2unicode(pathfixup(args.substr(1, args.size()-2), false));
								if(!isfullpath(filename))
								{
									filename = currentdir + filename;
									dir = currentdir;
								}
								else
									dir = folderfrompath(filename);

								parse_insert(getlines_file(filename.c_str()), dir);
							}
							else MsgError("Bad #inc directive!");
						}
						else MsgError("Bad #inc directive!");
					}
					// Logging
					else if(!in_asm && lcline == "#log")
					{
						log = true;
					}
					// Add line
					else
					{
						scriptline_t cur = { 0 };

						if(in_asm && lcline == "ende")
							in_asm = false;

						cur.line = scriptline;
						cur.linenum = curline;
						cur.is_command = !in_asm;

						if(!in_asm && lcline == "exec")
							in_asm = true;

						size_t pos = scriptline.find_first_of(whitespaces);
						if(pos != string::npos)
						{
							cur.command = tolower(scriptline.substr(0, pos));
							split(cur.args, scriptline.substr(pos+1), ',');
						}
						else
						{
							cur.command = tolower(scriptline);
						}

						lines.push_back(cur);
					}
				}
				nextline = true;
			}
		}
	}
}

uint OllyLang::OllyScript::next_command(uint from)
{
	while(from < lines.size() && !lines[from].is_command)
	{
		from++;
	}
	return from;
}

/*
bool OllyLang::OllyScript::load_file(const char* file, const char* dir)
{
	clear();

	char cdir[MAX_PATH];
	GetCurrentDirectory(_countof(cdir), cdir);
	string curdir = pathfixup(cdir, true);
	string sdir;

	path = pathfixup(file, false);
	if(!isfullpath(path))
	{
		path = curdir + path;
	}
	if(!dir)
		sdir = folderfrompath(path);
	else
		sdir = dir;

	vector<string> unparsedScript = getlines_file(path.c_str());
	parse_insert(unparsedScript, sdir);

	return true;
}
*/

bool OllyLang::OllyScript::load_file(const wchar_t* file, const wchar_t* dir)
{
	clear();

	wchar_t cdir[MAX_PATH];
	GetCurrentDirectoryW(_countof(cdir), cdir);
	wstring curdir = pathfixup(cdir, true);
	wstring sdir;

	path = pathfixup(file, false);
	if(!isfullpath(path))
	{
		path = curdir + path;
	}
	if(!dir)
		sdir = folderfrompath(path);
	else
		sdir = dir;

	vector<string> unparsedScript = getlines_file(path.c_str());
	parse_insert(unparsedScript, sdir);

	TSErrorExit = false;
	return true;
}

bool OllyLang::OllyScript::load_buff(const char* buff, size_t size, const wchar_t* dir)
{
	clear();

	wchar_t cdir[MAX_PATH];
	GetCurrentDirectoryW(_countof(cdir), cdir);
	wstring curdir = pathfixup(cdir, true);
	wstring sdir;

	path = L"";
	if(!dir)
	{
		sdir = curdir;
	}
	else
		sdir = dir;

	vector<string> unparsedScript = getlines_buff(buff, size);
	parse_insert(unparsedScript, sdir);

	TSErrorExit = false;
	return true;
}

bool OllyLang::OllyScript::is_label(const string& s)
{
	return (labels.find(s) != labels.end());
}

void OllyLang::InitGlobalVariables(){

	// Global variables
	variables["$INPUTFILE"] = TE_GetTargetPath();

	string name = TE_GetOutputPath();
	if(name.length() == NULL){
		string ext;
		size_t offs;
		name = TE_GetTargetPath();
		if((offs = name.rfind('.')) != string::npos){
			ext = name.substr(offs);
			ext.insert(0, ".unpacked");
			name.erase(offs);
			name.append(ext);
		}
	}
	variables["$OUTPUTFILE"] = name;
}

void OllyLang::Reset()
{
	freeMemBlocks();
	variables.clear();
	bpjumps.clear();
	calls.clear();

	variables["$OS_VERSION"] = rul2decstr(OS_VERSION_HI) + '.' + rul2decstr(OS_VERSION_LO);
	variables["$TE_VERSION"] = rul2decstr(TE_VERSION_HI) + '.' + rul2decstr(TE_VERSION_LO);
	variables["$TS_VERSION"] = rul2decstr(TS_VERSION_HI) + '.' + rul2decstr(TS_VERSION_LO);
	variables["$VERSION"] = variables["$OS_VERSION"];
	variables["$WIN_VERSION"] = GetVersion();

#ifdef _WIN64
	variables["$PLATFORM"] = "x86-64";
#else
	variables["$PLATFORM"] = "x86-32";
#endif

	EOB_row = EOE_row = -1;

	zf = cf = 0;
	search_buffer = NULL;

	saved_bp = 0;
	alloc_bp = 0;

	script_running = false;

	script_pos_next = 0;
	tickcount_startup = 0;
	break_memaddr = 0;
	break_reason = 0;

	pmemforexec = 0;
	membpaddr = 0;
	membpsize = 0;

	reg_backup.loaded = false;

	variables["$RESULT"] = 0;

	callbacks.clear();
	debuggee_running = false;
	require_addonaction = false;

	run_till_return = false;
	return_to_usercode = false;

	tExportsCache.clear();
	exportsCacheAddr = 0;
	tImportsCache.clear();
	importsCacheAddr = 0;
}

bool OllyLang::Run()
{
	script_running = true;
	Step();
	return true;
}

bool OllyLang::Pause()
{
	script_running = false;
	return true;
}

bool OllyLang::Step()
{
	back_to_debugloop = false;
	ignore_exceptions = false;
	stepcount = 0;

	while(!back_to_debugloop && script.isloaded() && script_running)
	{		
		if(!tickcount_startup)
			tickcount_startup = MyTickCount();

		script_pos = script.next_command(script_pos_next);

		// Check if script out of bounds
		if(script_pos >= script.lines.size())
			return false;

		OllyScript::scriptline_t& line = script.lines[script_pos];

		script_pos_next = script_pos + 1;

		// Log line of code if  enabled
		if(script.log)
		{
			string logstr = "--> " + line.line;
			TE_Log(logstr.c_str(), TS_LOG_COMMAND);
		}

		bool result = false;

		// Find command and execute it
		PFCOMMAND cmd = line.commandptr;
		if(!cmd)
		{
			map<string, PFCOMMAND>::const_iterator it = commands.find(line.command);
			if(it != commands.end())
			{
				line.commandptr = cmd = it->second;
			}
		}

		if(cmd)
		{
			size_t size = line.args.size();
			const string* args = size ? &line.args[0] : 0;
			result = (this->*cmd)(args, size); // Call command
		}
		else errorstr = "Unknown command: " + line.command;

		if(callbacks.size() && back_to_debugloop)
		{
			result = false;
			errorstr = "Unallowed command during callback: " + line.command;
		}

		// Error in processing, show error and die
		if(!result)
		{
			Pause();
			string message = "Error on line " + rul2decstr(line.linenum) + ": " + line.line + "\r\n" + errorstr;
			MsgError(message);
			errorstr = "";
			return false;
		}
	}
	return true;
}

//Executed after some commands to clean memory or to get something after ollydbg processing
bool OllyLang::ProcessAddonAction()
{
bool restore_registers = false;

	rulong ip = Debugger::GetContextData(UE_CIP);

	for(size_t i = 0; i < tMemBlocks.size();)
	{
		if(tMemBlocks[i].free_at_ip == ip)
		{
			TE_FreeMemory(tMemBlocks[i].address, tMemBlocks[i].size);
			if(tMemBlocks[i].result_register)
				variables["$RESULT"] = (rulong)Debugger::GetContextData(tMemBlocks[i].reg_to_return);
			if(tMemBlocks[i].restore_registers)
				restore_registers = true;
			require_addonaction = false;
			tMemBlocks.erase(tMemBlocks.begin() + i);
		}
		else i++;
	}

	if(restore_registers)
		RestoreRegisters(true);

	return true;
}

void OllyLang::OnBreakpoint(eBreakpointType reason)
{
	if(bInternalBP) //dont process temporary bp (exec/ende/go)
	{
		bInternalBP = false;
	}
	else
	{
		break_reason = reason;

		if(EOB_row > -1)
		{
			script_pos_next = EOB_row;
		}
		else
		{
			rulong ip = Debugger::GetContextData(UE_CIP);
			map<rulong, uint>::const_iterator it = bpjumps.find(ip);
			if(it != bpjumps.end())
			{
				script_pos_next = it->second;
			}
		}
	}

	StepChecked();
}

void OllyLang::OnException()
{
	if(EOE_row > -1)
	{
		script_pos_next = EOE_row;
	}
	else if(ignore_exceptions)
	{
		return;
	}
	StepChecked();
}

size_t OllyLang::GetStringOperatorPos(const string& ops)
{
string cache = ops;
size_t b = 0, e = 0, p = 0;

	//hide [pointer operations]
	while((p = cache.find('[', p)) != string::npos && (e = cache.find(']', p)) != string::npos)
	{
		cache.erase(p, e-p+1);
	}
	e = p = 0;

	//Search for operator(s) outside "quotes" 
	while((b = cache.find('\"', b)) != string::npos)
	{
		//Check Before
		if((p = cache.find('+'), e) != string::npos && p < b)
		{
			return p;
		}

		if((e = cache.find('\"', b+1)) != string::npos)
		{
			//Check between
			if((p = cache.find('+', e+1)) != string::npos && p < cache.find('\"', e+1))
			{
				return p;
			}
			b = e;
		}
		b++;
	}

	//Check after
	return cache.find('+', e);
}

size_t OllyLang::GetRulongOperatorPos(const string& ops)
{
string operators = "+-*/&|^<>";
size_t b = 0, e = 0, p;

	// []]
	// [[]
	// [[]]

	//Search for operator(s) outside [pointers]
	while((b = ops.find('[', b)) != string::npos)
	{
		//Check Before
		if((p = ops.find_first_of(operators), e) != string::npos && p < b)
		{
			return p;
		}

		if((e = ops.find(']', b+1)) != string::npos)
		{
			//Check between
			if((p = ops.find_first_of(operators, e+1)) != string::npos && p < ops.find('[', e+1))
			{
				return p;
			}
			b = e;
		}
		b++;
	}

	//Check after
	return ops.find_first_of(operators, e);
}

size_t OllyLang::GetFloatOperatorPos(const string& ops)
{
string operators = "+-*/";
size_t b = 0, e = 0, p;

	//Search for operator(s) outside [pointers]
	while((b = ops.find('[', b)) != string::npos)
	{
		//Check Before
		if((p = ops.find_first_of(operators), e) != string::npos && p < b)
		{
			return p;
		}

		if((e = ops.find(']', b+1)) != string::npos)
		{
			//Check between
			if((p = ops.find_first_of(operators, e+1)) != string::npos && p < ops.find('[', e+1))
			{
				return p;
			}
			b = e;
		}
		b++;
	}

	//Check after
	return ops.find_first_of(operators, e);
}

/*
bool OllyLang::ParseOperands(const string* args, string* results, size_t count, bool preferstr)
{
	for(size_t i = 0; i < count; i++) 
	{
		results[i] = args[i];

		continue;

		if(preferstr || args[i].find('\"') != string::npos)
		{
			if(!ParseString(args[i], results[i]))
			{
				if(!ParseRulong(args[i], results[i]))
				{
					ParseFloat(args[i], results[i]);
				}
			}
		}
		else
		{
			if(!ParseRulong(args[i], results[i]))
			{
				if(!ParseFloat(args[i], results[i]))
				{
					ParseString(args[i], results[i]);
				}
			}
		}
	}
	return true;
}
*/

bool OllyLang::ParseString(const string& arg, string& result)
{
size_t start = 0, offs;
char oper = '+';
var val = "";
string curval;

	if((offs = GetStringOperatorPos(arg)) != string::npos)
	{
		do
		{
			string token = trim(arg.substr(start, offs));

			if(!GetString(token, curval))
				return false;

			switch(oper)
			{
			case '+': val += curval; break;
			}

			if(offs == string::npos)
				break;

			oper = arg[start+offs];

			start += offs + 1;
			offs = GetRulongOperatorPos(arg.substr(start));
		}
		while(start < arg.size());

		if(!val.isbuf)
			result = '\"' + val.str + '\"';
		else
			result = val.str;
		return true;
	}

	return false;
}

bool OllyLang::ParseRulong(const string& arg, string& result)
{
size_t start = 0, offs;
char oper = '+';
rulong val = 0, curval;

	if((offs = GetRulongOperatorPos(arg)) != string::npos)
	{
		do
		{
			if(!start && !offs) // allow leading +/-
			{
				curval = 0;
			}
			else
			{
				string token = trim(arg.substr(start, offs));

				if(!GetRulong(token, curval))
					return false;
			}

			if(oper == '/' && curval == 0)
			{
				errorstr = "Division by 0";
				return false;
			}

			switch(oper)
			{
			case '+': val +=  curval; break;
			case '-': val -=  curval; break;
			case '*': val *=  curval; break;
			case '/': val /=  curval; break;
			case '&': val &=  curval; break;
			case '|': val |=  curval; break;
			case '^': val ^=  curval; break;
			case '>': val >>= curval; break;
			case '<': val <<= curval; break;
			}

			if(offs == string::npos)
				break;

			oper = arg[start+offs];

			start += offs + 1;
			offs = GetRulongOperatorPos(arg.substr(start));
		}
		while(start < arg.size());

		result = rul2hexstr(val);
		return true;
	}

	return false;
}

bool OllyLang::ParseFloat(const string& arg, string& result)
{
size_t start = 0, offs;
char oper = '+';
double val = 0, curval;

	if((offs = GetFloatOperatorPos(arg)) != string::npos)
	{
		do
		{
			if(!start && !offs) // allow leading +/-
			{
				curval = 0.0;
			}
			else
			{
				string token = trim(arg.substr(start, offs));

				if(!GetFloat(token, curval))
				{
					//Convert integer to float (but not for first operand)
					rulong dw;
					if(start && GetRulong(token, dw))
						curval = dw;
					else
						return false;
				}
			}

			if(oper == '/' && curval == 0.0)
			{
				errorstr = "Division by 0";
				return false;
			}

			switch(oper)
			{
			case '+': val +=  curval; break;
			case '-': val -=  curval; break;
			case '*': val *=  curval; break;
			case '/': val /=  curval; break;
			}

			if(offs == string::npos)
				break;

			oper = arg[start+offs];

			start += offs + 1;
			offs = GetFloatOperatorPos(arg.substr(start));
		}
		while(start < arg.size());

		result = dbl2str(val);

		// Remove trailing zeroes (keep 1 digit after '.')
		size_t p;
		if((p = result.find('.')) != string::npos)
		{
			size_t psize = result.length()-p;

			do psize--;
			while(psize > 1 && result[p+psize] == '0');

			result.resize(p+psize+1);
		}
		return true;
	}

	return false;
}

bool OllyLang::GetAnyValue(const string& op, string &value, bool hex8forExec)
{
rulong dw;

	if(is_variable(op))
	{
		const var& v = variables[op];
		if(v.type == var::STR)
		{
			value = v.str;
			return true;
		}
		else if(v.type == var::DW)
		{
			if(hex8forExec) //For Assemble Command (EXEC/ENDE) ie. "0DEADBEEF"
				value = '0' + toupper(rul2hexstr(v.dw));
			else 
				value = toupper(rul2hexstr(v.dw));
			return true;
		}
	}
	else if(is_float(op))
	{
		value = op;
		return true;
	}
	else if(is_hex(op))
	{
		if(hex8forExec)
			value = '0' + op;
		else
			value = op;
		return true;
	}
	else if(is_dec(op))
	{
		value = toupper(rul2hexstr(decstr2rul(op.substr(0, op.length()-1))));
		return true;
	}
	else if(is_string(op))
	{
		value = UnquoteString(op, '"');
		return true;
	}
	else if(is_bytestring(op)) 
	{
		value = op;
		return true;
	}
	else if(is_memory(op))
	{
		return GetString(op, value);
	}
	else if(GetRulong(op, dw))
	{
		if(hex8forExec)
			value = '0' + toupper(rul2hexstr(dw));
		else 
			value = toupper(rul2hexstr(dw));
		return true;
	}
	return false;
}

bool OllyLang::GetString(const string& op, string &value, size_t size)
{
	if(is_variable(op))
	{
		if(variables[op].type == var::STR)
		{
			if(size && size < variables[op].size)
			{
				var tmp = variables[op];
				tmp.resize(size);
				value = tmp.str;
			}
			else
			{
				value = variables[op].str;
			}
			return true;
			/*
			// It's a string var, return value
			if(size && size < v.size)
				value = v.to_string().substr(0, size);
			else
				value = v.str;
			return true;
			*/
		}
	}
	else if(is_string(op))
	{
		value = UnquoteString(op, '"');

		if(size && size < value.length())
			value.resize(size);
		return true; 
	}
	else if(is_bytestring(op))
	{
		if(size && (size*2) < (op.length()-2))
			value = op.substr(0, (size*2)+1) + '#';
		else
			value = op;
		return true;
	}
	else if(is_memory(op))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong src;
		if(GetRulong(tmp, src))
		{
			ASSERT(src != 0);

			value = "";
			if(size)
			{
				byte* buffer;

				try
				{
					buffer = new byte[size];
				}
				catch(std::bad_alloc)
				{
					return false;	
				}

				if(TE_ReadMemory(src, size, buffer))
				{
					value = '#' + bytes2hexstr(buffer, size) + '#';
					return true;
				}
				delete[] buffer;

				/*
				char* buffer;

				try
				{
					buffer = new char[size+1];
				}
				catch(std::bad_alloc)
				{
					return false;	
				}

				if(TE_ReadMemory(src, size, buffer))
				{
					buffer[size] = '\0';
					value = buffer;
					if(value.length() != size)
					{
						var v = value;
						value = '#' + v.to_bytes() + '#';
					}
					delete[] buffer;
					return true;
				}
				delete[] buffer;
				*/
			}
			else
			{
				char buffer[STRING_READSIZE] = {0};
				if(TE_ReadMemory(src, sizeof(buffer), buffer))
				{
					buffer[_countof(buffer)-1] = '\0';
					value = buffer;
					return true;
				}
			}
		}
	}
	else
	{
		string parsed;
		return (ParseString(op, parsed) && GetString(parsed, value, size));
	}
	return false;
}
/*
bool OllyLang::GetStringLiteral(const string& op, string &value)
{
	if(is_variable(op))
	{
		const var& v = variables[op];
		if(v.type == var::STR && !v.isbuf)
		{
			value = v.str;
			return true;
		}
	}
	else if(is_string(op))
	{
		value = UnquoteString(op, '"');
		return true; 
	}
	else
	{
		string parsed;
		return (ParseString(op, parsed) && GetStringLiteral(parsed, value));
	}
	return false;
}

bool OllyLang::GetBytestring(const string& op, string &value, size_t size)
{
	if(is_variable(op))
	{
		const var& v = variables[op];
		if(v.type == var::STR && v.isbuf)
		{
			if(size && size < v.size)
			{
				var tmp = v;
				tmp.resize(size);
				value = tmp.str;
			}
			else
				value = v.str;
			return true;
		}
	}
	else if(is_bytestring(op))
	{
		if(size && (size*2) < (op.length()-2))
			value = op.substr(0, (size*2)+1) + '#';
		else
			value = op;
		return true;
	}
	else if(is_memory(op))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong src;
		if(GetRulong(tmp, src))
		{
			ASSERT(src != 0);

			if(size)
			{
				byte* buffer;

				try
				{
					buffer = new byte[size];
				}
				catch(std::bad_alloc)
				{
					return false;	
				}

				if(TE_ReadMemory(src, size, buffer))
				{
					value = '#' + bytes2hexstr(buffer, size) + '#';
					delete[] buffer;
					return true;
				}
				delete[] buffer;
			}
		}
	}
	else
	{
		string parsed;
		return (ParseString(op, parsed) && GetBytestring(parsed, value, size));
	}
	return false;
}
*/

bool OllyLang::GetBool(const string& op, bool &value)
{
rulong temp;

	if(GetRulong(op, temp))
	{
		value = temp != 0;
		return true;
	}
	return false;
}

bool OllyLang::GetRulong(const string& op, rulong& value)
{
	if(is_register(op))
	{
		const register_t* reg = find_register(op);
		value = Debugger::GetContextData(reg->id);
		value = resize(value >> (reg->offset * 8), reg->size);
		return true;
	}
	else if(is_flag(op))
	{
		eflags_t flags;
        flags.dw = Debugger::GetContextData(UE_EFLAGS);
		switch(op[1])
		{
		case 'a': value = flags.bits.AF; break;
		case 'c': value = flags.bits.CF; break;
		case 'd': value = flags.bits.DF; break;
		case 'o': value = flags.bits.OF; break;
		case 'p': value = flags.bits.PF; break;
		case 's': value = flags.bits.SF; break;
		case 'z': value = flags.bits.ZF; break;
		}
		return true;
	}
	else if(is_variable(op))
	{
		if(variables[op].type == var::DW)
		{
			value = variables[op].dw;
			return true;
		}
	}
	else if(is_constant(op))
	{
		value = find_constant(op)->value;
		return true;
	}
	else if(is_hex(op))
	{
		value = hexstr2rul(op);
		return true;
	}
	else if(is_dec(op))
	{
		value = decstr2rul(op.substr(0, op.length()-1));
		return true;
	}
	else if(IsQuotedString(op, '[', ']'))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong src;
		if(GetRulong(tmp, src))
		{
			ASSERT(src != 0);
			return TE_ReadMemory(src, sizeof(value), &value);
		}
	}
	else
	{
		string parsed;
		return (ParseRulong(op, parsed) && GetRulong(parsed, value));
	}
	return false;
}

bool OllyLang::GetFloat(const string& op, double &value)
{
	if(is_float(op))
	{
		value = str2dbl(op);
		return true;
	}
	else if(is_floatreg(op))
	{
		int index = op[3] - '0';
		double reg;
		#ifdef _WIN64
			XMM_SAVE_AREA32 fltctx;
			//reg = (double)fltctx.FloatRegisters[index];
		#else
			FLOATING_SAVE_AREA fltctx;
			reg = ((double*)&fltctx.RegisterArea[0])[index];
		#endif
		if(Debugger::GetContextFPUDataEx(TE_GetCurrentThreadHandle(), &fltctx))
		{
			value = reg;
			return true;
		}
	}
	else if(is_variable(op))
	{
		if(variables[op].type == var::FLT)
		{
			value = variables[op].flt;
			return true;
		}
	}
	else if(is_memory(op))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong src;
		if(GetRulong(tmp, src))
		{
			ASSERT(src != 0);
			return TE_ReadMemory(src, sizeof(value), &value);
		}
	}
	else
	{
		string parsed;
		return (ParseFloat(op, parsed) && GetFloat(parsed, value));
	}
	return false;
}

bool OllyLang::SetRulong(const string& op, const rulong& value, size_t size)
{
	if(size > sizeof(value))
		size = sizeof(value);

	if(is_variable(op))
	{
		variables[op] = value;
		variables[op].resize(size);
		return true;
	}
	else if(is_register(op))
	{
		const register_t* reg = find_register(op);
		rulong tmp = resize(value, min(size, (size_t)reg->size));
		if(reg->size < sizeof(rulong))
		{
			rulong oldval, newval;
			oldval = Debugger::GetContextData(reg->id);
			oldval &= ~(((1 << (reg->size * 8)) - 1) << (reg->offset * 8));
			newval = resize(value, reg->size) << (reg->offset * 8);
			tmp = oldval | newval;
		}
		return Debugger::SetContextData(reg->id, tmp);
	}
	else if(is_flag(op))
	{
		bool flagval = value != 0;

		eflags_t flags;
		flags.dw = Debugger::GetContextData(UE_EFLAGS);

		switch(op[1])
		{
		case 'a': flags.bits.AF = flagval; break;
		case 'c': flags.bits.CF = flagval; break;
		case 'd': flags.bits.DF = flagval; break;
		case 'o': flags.bits.OF = flagval; break;
		case 'p': flags.bits.PF = flagval; break;
		case 's': flags.bits.SF = flagval; break;
		case 'z': flags.bits.ZF = flagval; break;
		}

		return Debugger::SetContextData(UE_EFLAGS, flags.dw);
	}
	else if(is_memory(op))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong target;
		if(GetRulong(tmp, target))
		{
			ASSERT(target != 0);
			return TE_WriteMemory(target, size, &value);
		}
	}

	return false;
}

bool OllyLang::SetFloat(const string& op, const double& value)
{
	if(is_variable(op))
	{
		variables[op] = value;
		return true;
	}
	else if(is_floatreg(op))
	{
		int index = op[3] - '0';
		double* preg;
		#ifdef _WIN64
			XMM_SAVE_AREA32 fltctx;
			preg = (double*)&fltctx.FloatRegisters + index;
		#else
			FLOATING_SAVE_AREA fltctx;
			preg = (double*)&fltctx.RegisterArea[0] + index;
		#endif
		if(Debugger::GetContextFPUDataEx(TE_GetCurrentThreadHandle(), &fltctx))
		{
			*preg = value;
			return Debugger::SetContextFPUDataEx(TE_GetCurrentThreadHandle(), &fltctx);
		}
	}
	else if(is_memory(op))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong target;
		if(GetRulong(tmp, target))
		{
			ASSERT(target != 0);

			return TE_WriteMemory(target, sizeof(value), &value);
		}
	}

	return false;
}

bool OllyLang::SetString(const string& op, const string& value, size_t size)
{
	if(is_variable(op))
	{
		variables[op] = value;
		if(size && size < variables[op].size)
			variables[op].resize(size);
		return true;
	}
	else if(is_memory(op))
	{
		string tmp = UnquoteString(op, '[', ']');

		rulong target;
		if(GetRulong(tmp, target))
		{
			ASSERT(target != 0);
			return TE_WriteMemory(target, min(size, value.size()), &value);
		}
	}
	return false;
}

bool OllyLang::SetBool(const string& op, const bool& value)
{
	return SetRulong(op, value, sizeof(value));
}

const OllyLang::register_t* OllyLang::find_register(const string& name)
{
string lower = tolower(name);

	for(int i = 0; i < _countof(registers); i++)
	{
		if(registers[i].name == lower)
			return &registers[i];
	}
	return 0;
}

const OllyLang::constant_t* OllyLang::find_constant(const string& name)
{
string lower = tolower(name);

	for(int i = 0; i < _countof(constants); i++)
	{
		if(constants[i].name == lower)
			return &constants[i];
	}
	return 0;
}

bool OllyLang::is_register(const string& s)
{
	return (find_register(s) != 0);
}

bool OllyLang::is_floatreg(const string& s)
{
	return (fpu_registers+_countof(fpu_registers) != find(fpu_registers, fpu_registers+_countof(fpu_registers), tolower(s)));
}

bool OllyLang::is_flag(const string& s)
{
	return (e_flags+_countof(e_flags) != find(e_flags, e_flags+_countof(e_flags), tolower(s)));
}

bool OllyLang::is_variable(const string& s)
{
	return (variables.find(s) != variables.end());
}

bool OllyLang::is_constant(const string& s)
{
	return (find_constant(s) != 0);
}

bool OllyLang::is_valid_variable_name(const string& s)
{
	return (s.length() && isalpha(s[0]) && !is_register(s) && !is_floatreg(s) && !is_constant(s));
}

bool OllyLang::is_writable(const string&s)
{
	return (is_variable(s) || is_memory(s) || is_register(s) || is_flag(s) || is_floatreg(s));
}

string OllyLang::ResolveVarsForExec(const string& in, bool hex8forExec)
{
	string out, varname;
	const string ti = trim(in);
	bool in_var = false;

	for(size_t i = 0; i < ti.length(); i++)
	{
		if(ti[i] == '{')
		{
			in_var = true;
		}
		else if(ti[i] == '}')
		{
			in_var = false;
			GetAnyValue(varname, varname, hex8forExec);
			out += varname;
			varname = "";
		}
		else
		{
			if(in_var)
				varname += ti[i];
			else
				out += ti[i];
		}
	}				
	return out;
}

//Add zero char before dw values, ex: 0DEADBEEF (to be assembled) usefull if first char is letter
string OllyLang::FormatAsmDwords(const string& asmLine)
{
	// Create command and arguments
	string command, arg, args;
	string cSep = "";
	size_t pos;

	pos = asmLine.find_first_of(whitespaces);

	if(pos == string::npos)
		return asmLine; //no args

	command = asmLine.substr(0, pos) + ' ';
	args = asmLine.substr(pos+1);

	while((pos = args.find_first_of("+,[")) != string::npos)
	{
		arg = trim(args.substr(0, pos));
		ForLastArg:
		if(cSep == "[")
		{
			if(arg.size() && arg[arg.size()-1] == ']')
			{
				if(is_hex(arg.substr(0, arg.size()-1)) && isalpha(arg[0]))
				{
					arg.insert(0, 1, '0');
				}
			}
		}
		else
		{
			if(is_hex(arg) && isalpha(arg[0]))
			{
				arg.insert(0, 1, '0');
			}
		}

		command += cSep + arg;

		if(args != "")
		{
			cSep = string(1, args[pos]);
			args.erase(0, pos+1);
		}
	}

	args = trim(args);
	if(args != "")
	{ 
		arg = args;
		args = "";
		goto ForLastArg;
	}

	return trim(command);
}

bool OllyLang::callCommand(PFCOMMAND command, int count, ...)
{
vector<string> args;
va_list list;

	va_start(list, count);

	for(int i = 0; i < count; i++)
	{
		args.push_back(string(va_arg(list, const char*)));
	}

	va_end(list);

	return (this->*command)(&args[0], args.size());
}

void OllyLang::regBlockToFree(const t_dbgmemblock& block)
{
	tMemBlocks.push_back(block);
}

void OllyLang::regBlockToFree(rulong address, size_t size, bool autoclean)
{
t_dbgmemblock block = {0};

	block.address = address;
	block.size = size;	
	block.autoclean = autoclean; 
	block.script_pos = script_pos;
	block.restore_registers = false;

	regBlockToFree(block);
}

bool OllyLang::unregMemBlock(rulong address)
{
	for(size_t i = 0; i < tMemBlocks.size(); i++)
	{
		if(tMemBlocks[i].address == address)
		{
			tMemBlocks.erase(tMemBlocks.begin() + i);
			return true;
		}
	}
	return false;
}

bool OllyLang::freeMemBlocks()
{
	for(size_t i = 0; i < tMemBlocks.size(); i++)
	{
		if(tMemBlocks[i].autoclean)
			TE_FreeMemory(tMemBlocks[i].address, tMemBlocks[i].size);
	}
	tMemBlocks.clear();
	return true;
}

bool OllyLang::SaveRegisters(bool stackToo)
{
	for(size_t i = 0; i < _countof(registers); i++)
	{
		if(registers[i].size == sizeof(rulong))
		{
			eContextData reg = registers[i].id;
			if(stackToo || (reg != UE_ESP && reg != UE_RSP && reg != UE_EBP && reg != UE_RBP))
			{
				reg_backup.regs[i] = Debugger::GetContextData(reg);
			}
		}
	}
	reg_backup.eflags = Debugger::GetContextData(UE_EFLAGS);

	reg_backup.threadid = TE_GetCurrentThreadId();
	reg_backup.script_pos = script_pos;
	reg_backup.loaded = true;
	return true;
}

bool OllyLang::RestoreRegisters(bool stackToo)
{
	if(!reg_backup.loaded)
		return false;		

	if(TE_GetCurrentThreadId() != reg_backup.threadid)
		return false;

	for(size_t i = 0; i < _countof(registers); i++)
	{
		if(registers[i].size == sizeof(rulong))
		{
			eContextData reg = registers[i].id;
			if(stackToo || (reg != UE_ESP && reg != UE_RSP && reg != UE_EBP && reg != UE_RBP))
			{
				Debugger::SetContextData(reg, reg_backup.regs[i]);
			}
		}
	}

	Debugger::SetContextData(UE_EFLAGS, reg_backup.eflags);

	return true;
}

bool OllyLang::AllocSwbpMem(uint tmpSizet)
{	
	/*
	if(!tmpSizet)
	{
		FreeBpMem();
	}
	else if(!softbp_t || tmpSizet > alloc_bp)
	{
		try
		{
			if(softbp_t)
				delete[] softbp_t;
			softbp_t = new t_bpoint[tmpSizet]; // new tmt_bpointpSizet* ???
			alloc_bp = tmpSizet;
		}
		catch(...)
		{
			return false;
		}
	}*/
	return true;
}

void OllyLang::FreeBpMem()
{
	/*
	if(softbp_t)
	{
		delete[] softbp_t;
		softbp_t = NULL;
	}*/
	saved_bp = 0;
	alloc_bp = 0;
}

void OllyLang::StepIntoCallback()
{
	switch(Instance().stepcount)
	{
		default: // continue stepping, count > 0
			Instance().stepcount--;
		case -1: // endless stepping, only enter script command loop on BP/exception
			Debugger::StepInto(&StepIntoCallback);
			break;
		case 0: // stop stepping, enter script command loop
			Instance().StepChecked();
			break;
	}
}

void OllyLang::StepOverCallback()
{
	switch(Instance().stepcount)
	{
		default:
			Instance().stepcount--;
		case -1:
			if(Instance().return_to_usercode)
			{
				if(true/*is_this_user_code(EIP)*/)
				{
					Instance().return_to_usercode = false;
					Instance().stepcount = 0;
					Instance().StepChecked();
					break;
				}
			}
			else if(Instance().run_till_return)
			{
				string cmd = DisassembleEx(Debugger::GetContextData(UE_CIP));
				if(cmd.find("RETN") == 0)
				{
					Instance().run_till_return = false;
					Instance().stepcount = 0;
					Instance().StepChecked();
					break;
				}
			}

			{
				/*
				only step over calls and string operations
				StepOver effectively sets a BP after the current instruction
				that's not gonna do us any good for jumps
				so we'll stepinto except for a few exceptions
				*/
				string cmd = DisassembleEx(Debugger::GetContextData(UE_CIP));
				if(cmd.find("CALL") == 0 || cmd.find("REP") == 0)
					Debugger::StepOver(&StepOverCallback);
				else
					Debugger::StepInto(&StepIntoCallback);
				break;
			}
		case 0:
			Instance().StepChecked();
			break;
	}
}

bool OllyLang::StepChecked()
{
	if(!debuggee_running)
	{
		debuggee_running = true;
		Step();
	}
	return true;
}

bool OllyLang::StepCallback(uint pos, bool returns_value, var::etype return_type, var* result)
{
	callback_t callback;
	callback.call = calls.size();
	callback.returns_value = returns_value;
	callback.return_type = return_type;

	callbacks.push_back(callback);

	calls.push_back(script_pos+1);
	script_pos_next = pos;

	bool ret = Step();
	if(ret && returns_value && result)
		*result = callback_return;

	return ret;
}

void OllyLang::CHC_TRAMPOLINE(const void* ExceptionData, eCustomException ExceptionId)
{
	map<eCustomException, string>::const_iterator it = Instance().CustomHandlerLabels.find(ExceptionId);
	if(it != Instance().CustomHandlerLabels.end())
	{
		//variables["$TE_ARG_1"] = (rulong)ExceptionData;
		Instance().DoCALL(&it->second, 1);
	}
}

void* OllyLang::Callback_AutoFixIATEx(void* fIATPointer)
{
var ret;

	uint label = Instance().script.labels[Instance().Label_AutoFixIATEx];
	Instance().variables["$TE_ARG_1"] = (rulong)fIATPointer;
	if(Instance().StepCallback(label, true, var::DW, &ret))
		return (void*)ret.dw;
	else
		return 0;
}

void OllyLang::LBPC_TRAMPOLINE(const LOAD_DLL_DEBUG_INFO* SpecialDBG, eLibraryEvent bpxType)
{
	const Librarian::LIBRARY_ITEM_DATA* Lib = Librarian::GetLibraryInfoEx(SpecialDBG->lpBaseOfDll);
	if(Lib)
	{
		const map<string, string>& labels = LibraryBreakpointLabels[bpxType];
		map<string, string>::const_iterator it = labels.find(Lib->szLibraryPath);
		if(it != labels.end())
		{
			Instance().DoCALL(&it->second, 1);
		}
	}
}
