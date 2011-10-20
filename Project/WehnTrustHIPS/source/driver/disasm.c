
/*
 * libdasm -- simple x86 disassembly library
 * (c) 2004 - 2005  jt / nologin.org
 *
 *
 * TODO:
 * - more documentation
 * - do more code validation
 *
 */

#include "precomp.h"

#if 0
#ifdef BUILTIN_MEMSET
#define memset builtin_memset
void builtin_memset(void *buf, int c, int size)
{
	while (size-- > 0)
		((char *)buf)[size] = c;
}
#endif
#endif

// MODRM byte
#define MASK_MODRM_MOD(x) (((x) & 0xc0) >> 6)
#define MASK_MODRM_REG(x) (((x) & 0x38) >> 3)
#define MASK_MODRM_RM(x)   ((x) & 0x7)

// SIB byte
#define MASK_SIB_SCALE(x) MASK_MODRM_MOD(x)
#define MASK_SIB_INDEX(x) MASK_MODRM_REG(x)
#define MASK_SIB_BASE(x)  MASK_MODRM_RM(x)

// Registers
#define MASK_REG(x) ((x) & 0x000000FF)
#define REG_EAX 0
#define REG_AX REG_EAX
#define REG_AL REG_EAX
#define REG_ES REG_EAX		// Just for reg_table consistence
#define REG_ST0 REG_EAX		// Just for reg_table consistence
#define REG_ECX 1
#define REG_CX REG_ECX
#define REG_CL REG_ECX
#define REG_CS REG_ECX
#define REG_ST1 REG_ECX
#define REG_EDX 2
#define REG_DX REG_EDX
#define REG_DL REG_EDX
#define REG_SS REG_EDX
#define REG_ST2 REG_EDX
#define REG_EBX 3
#define REG_BX REG_EBX
#define REG_BL REG_EBX
#define REG_DS REG_EBX
#define REG_ST3 REG_EBX
#define REG_ESP 4
#define REG_SP REG_ESP
#define REG_AH REG_ESP		// Just for reg_table consistence
#define REG_FS REG_ESP
#define REG_ST4 REG_ESP
#define REG_EBP 5
#define REG_BP REG_EBP
#define REG_CH REG_EBP
#define REG_GS REG_EBP
#define REG_ST5 REG_EBP
#define REG_ESI 6
#define REG_SI REG_ESI
#define REG_DH REG_ESI
#define REG_ST6 REG_ESI
#define REG_EDI 7
#define REG_DI REG_EDI
#define REG_BH REG_EDI
#define REG_ST7 REG_EDI
#define REG_NOP 10


// Name table index
#define REG_GEN_ULONG 0
#define REG_GEN_USHORT  1
#define REG_GEN_UCHAR  2
#define REG_SEGMENT   3
#define REG_DEBUG     4
#define REG_CONTROL   5
#define REG_TEST      6
#define REG_SIMD      7 
#define REG_MMX       8 
#define REG_FPU       9

// Opcode extensions for one -and two-byte opcodes
// XXX: move these to proper instruction structures ASAP!

#if !defined(NOSTR)
// lock/rep prefix name table
const char *rep_table[] = {
	 "lock ", "repne ", "rep "
};

// Register name table
const char *reg_table[10][8] = {
	{ "eax",  "ecx",  "edx",  "ebx",  "esp",  "ebp",  "esi",  "edi"  },
	{ "ax",   "cx",   "dx",   "bx",   "sp",   "bp",   "si",   "di"   },
	{ "al",   "cl",   "dl",   "bl",   "ah",   "ch",   "dh",   "bh"   },
	{ "es",   "cs",   "ss",   "ds",   "fs",   "gs",   "seg6", "seg7" },
	{ "dr0",  "dr1",  "dr2",  "dr3",  "dr4",  "dr5",  "dr6",  "dr7"  },
	{ "cr0",  "cr1",  "cr2",  "cr3",  "cr4",  "cr5",  "cr6",  "cr7"  },
	{ "tr0",  "tr1",  "tr2",  "tr3",  "tr4",  "tr5",  "tr6",  "tr7"  },
	{ "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7" },
	{ "mm0",  "mmx",  "mm2",  "mm3",  "mm4",  "mm5",  "mm6",  "mm7"  },
	{ "st(0)","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)"},
};


const char * ext_name_table[16][8] = {
	{ "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" },          // g1
	{ "rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar" },          // g2
	{ "test", NULL, "not", "neg", "mul", "imul", "div", "idiv" },       // g3
	{ "inc", "dec", NULL, NULL, NULL, NULL, NULL, NULL },               // g4
	{ "inc", "dec", "call", "callf", "jmp", "jmpf", "push", NULL },     // g5
	{ "sldt", "str", "lldt", "ltr", "verr", "verw", NULL, NULL },       // g6
	{ "sgdt", "sidt", "lgdt", "lidt", "smsw", NULL, "lmsw", "invlpg" }, // g7
	{ NULL, NULL, NULL, NULL, "bt", "bts", "btr", "btc" },              // g8
	{ NULL, "cmpxch", NULL, NULL, NULL, NULL, NULL, NULL },             // g9
	{ NULL, NULL, "psrld", NULL, "psrad", NULL, "pslld", NULL },        // ga
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },                 // gb
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },                 // gc
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },                 // gd
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },                 // ge
	{ "fxsave", "fxstor", "ldmxc5r", "stmxc5r", NULL, NULL, NULL, "sfence" }, // gf
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },                 // g0
};
#endif

// Instruction types for extensions
// XXX: move these to proper instruction structures ASAP!

enum Instruction ext_type_table[16][8] = {
	{ // g1
	INSTRUCTION_TYPE_ADD,   INSTRUCTION_TYPE_OR,
	INSTRUCTION_TYPE_ADC,   INSTRUCTION_TYPE_SBB,
	INSTRUCTION_TYPE_AND,   INSTRUCTION_TYPE_SUB,
	INSTRUCTION_TYPE_XOR,   INSTRUCTION_TYPE_CMP,
	},
	{ // g2
	INSTRUCTION_TYPE_ROX,   INSTRUCTION_TYPE_ROX,
	INSTRUCTION_TYPE_ROX,   INSTRUCTION_TYPE_ROX,
	INSTRUCTION_TYPE_SHX,   INSTRUCTION_TYPE_SHX,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_SHX,
	},
	{ // g3
	INSTRUCTION_TYPE_TEST,  INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_NOT,   INSTRUCTION_TYPE_NEG,
	INSTRUCTION_TYPE_MUL,   INSTRUCTION_TYPE_MUL,
	INSTRUCTION_TYPE_DIV,   INSTRUCTION_TYPE_DIV,
	},
	{ // g4
	INSTRUCTION_TYPE_INC,   INSTRUCTION_TYPE_DEC,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // g5
	INSTRUCTION_TYPE_INC,   INSTRUCTION_TYPE_DEC,
	INSTRUCTION_TYPE_CALL,  INSTRUCTION_TYPE_CALL,
	INSTRUCTION_TYPE_JMP,   INSTRUCTION_TYPE_JMP,
	INSTRUCTION_TYPE_PUSH,  INSTRUCTION_TYPE_OTHER,
	},
	{ // g6
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // g7
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // g8
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // g9
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // ga
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // gb
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // gc
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // gd
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // ge
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // gf
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	},
	{ // g0
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	INSTRUCTION_TYPE_OTHER, INSTRUCTION_TYPE_OTHER,
	}
};

// 1-byte opcodes
static const INST inst_table1[256] = {
	{ INSTRUCTION_TYPE_ADD,   MNEMONIC_STR(add)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_ADD,   MNEMONIC_STR(add)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_ADD,   MNEMONIC_STR(add)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_ADD,   MNEMONIC_STR(add)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_ADD,   MNEMONIC_STR(add)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_ADD,   MNEMONIC_STR(add)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_ES|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_ES|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OR,    MNEMONIC_STR(or)       AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OR,    MNEMONIC_STR(or)       AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_OR,    MNEMONIC_STR(or)       AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OR,    MNEMONIC_STR(or)       AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OR,    MNEMONIC_STR(or)       AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OR,    MNEMONIC_STR(or)       AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_CS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	// Escape to 2-byte opcode table
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_ADC,   MNEMONIC_STR(adc)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_ADC,   MNEMONIC_STR(adc)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_ADC,   MNEMONIC_STR(adc)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_ADC,   MNEMONIC_STR(adc)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_ADC,   MNEMONIC_STR(adc)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_ADC,   MNEMONIC_STR(adc)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_SS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_SS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_SBB,   MNEMONIC_STR(sbb)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_SBB,   MNEMONIC_STR(sbb)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_SBB,   MNEMONIC_STR(sbb)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_SBB,   MNEMONIC_STR(sbb)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_SBB,   MNEMONIC_STR(sbb)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_SBB,   MNEMONIC_STR(sbb)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_DS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_DS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_AND,   MNEMONIC_STR(and)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_AND,   MNEMONIC_STR(and)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_AND,   MNEMONIC_STR(and)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_AND,   MNEMONIC_STR(and)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_AND,   MNEMONIC_STR(and)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_AND,   MNEMONIC_STR(and)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	// seg ES override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(daa)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_SUB,   MNEMONIC_STR(sub)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_SUB,   MNEMONIC_STR(sub)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_SUB,   MNEMONIC_STR(sub)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_SUB,   MNEMONIC_STR(sub)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_SUB,   MNEMONIC_STR(sub)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_SUB,   MNEMONIC_STR(sub)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	// seg CS override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(das)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_XOR,   MNEMONIC_STR(xor)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_XOR,   MNEMONIC_STR(xor)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_XOR,   MNEMONIC_STR(xor)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_XOR,   MNEMONIC_STR(xor)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_XOR,   MNEMONIC_STR(xor)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XOR,   MNEMONIC_STR(xor)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	// seg SS override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(aaa)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_CMP,   MNEMONIC_STR(cmp)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_CMP,   MNEMONIC_STR(cmp)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_CMP,   MNEMONIC_STR(cmp)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_CMP,   MNEMONIC_STR(cmp)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_CMP,   MNEMONIC_STR(cmp)      AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_CMP,   MNEMONIC_STR(cmp)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	// seg DS override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(aas)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_EAX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_ECX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_EDX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_EBX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_ESP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_EBP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_ESI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INC,   MNEMONIC_STR(inc)      AM_REG|REG_EDI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_EAX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_ECX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_EDX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_EBX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_ESP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_EBP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_ESI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_DEC,   MNEMONIC_STR(dec)      AM_REG|REG_EDI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_EAX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_ECX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_EDX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_EBX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_ESP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_EBP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_ESI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_REG|REG_EDI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_EAX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_ECX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_EDX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_EBX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_ESP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_EBP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_ESI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_REG|REG_EDI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH , MNEMONIC_STR(pusha)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(popa)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bound)    AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(arpl)     AM_E|OT_w,              AM_G|OT_w,            FLAGS_NONE,  1 },
	// seg FS override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	// seg GS override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	// operand size override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	// address size override
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_I|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MUL,   MNEMONIC_STR(imul)      AM_G|OT_v,              AM_E|OT_v,            AM_I|OT_v ,  1 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(push)     AM_I|OT_b|F_s,          FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MUL,   MNEMONIC_STR(imul)      AM_G|OT_v,              AM_E|OT_v,            AM_I|OT_b|F_s,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(insb)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(insv)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(outsb)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(outsv)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jo)       AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jno)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jb)       AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnb)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jz)       AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnz)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jbe)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnbe)     AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(js)       AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jns)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jp)       AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnp)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jl)       AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnl)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jle)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnle)     AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g1)       AM_E|OT_b,              AM_I|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g1)       AM_E|OT_v,              AM_I|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g1)       AM_E|OT_b,              AM_I|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g1)       AM_E|OT_v,              AM_I|OT_b|F_s,        FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_TEST,  MNEMONIC_STR(test)     AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_TEST,  MNEMONIC_STR(test)     AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_G|OT_b,              AM_E|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_E|OT_w,              AM_S|OT_w,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_LEA,   MNEMONIC_STR(lea)      AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_S|OT_w,              AM_E|OT_w,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(pop)      AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(nop)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_ECX|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_EDX|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_EBX|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_ESP|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_EBP|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_ESI|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_XCHG,  MNEMONIC_STR(xchg)     AM_REG|REG_EAX|OT_v,    AM_REG|REG_EDI|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cbw)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cwd)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_CALL,  MNEMONIC_STR(callf)    AM_A|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(wait)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(pushf)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_POP,   MNEMONIC_STR(popf)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(sahf)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lahf)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EAX|OT_b,    AM_O|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EAX|OT_v,    AM_O|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_O|OT_v,              AM_REG|REG_EAX|OT_b,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_O|OT_v,              AM_REG|REG_EAX|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOVS,  MNEMONIC_STR(movsb)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOVS,  MNEMONIC_STR(movsd)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_CMPS,  MNEMONIC_STR(cmpsb)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_CMPS,  MNEMONIC_STR(cmpsd)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_TEST,  MNEMONIC_STR(test)     AM_REG|REG_EAX|OT_b,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_TEST,  MNEMONIC_STR(test)     AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_STOS,  MNEMONIC_STR(stosb)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_STOS,  MNEMONIC_STR(stosd)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_LODS,  MNEMONIC_STR(lodsb)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_LODS,  MNEMONIC_STR(lodsd)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_SCAS,  MNEMONIC_STR(scasb)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_SCAS,  MNEMONIC_STR(scasd)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_AL|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_CL|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_DL|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_BL|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_AH|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_CH|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_DH|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_BH|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EAX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_ECX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EDX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EBX|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_ESP|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EBP|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_ESI|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_REG|REG_EDI|OT_v,    AM_I|OT_v,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g2)       AM_E|OT_b,              AM_I|OT_b,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g2)       AM_E|OT_v,              AM_I|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_RET,   MNEMONIC_STR(retn)     AM_I|OT_w,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_RET,   MNEMONIC_STR(ret)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(les)      AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lds)      AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_E|OT_b,              AM_I|OT_b,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_MOV,   MNEMONIC_STR(mov)      AM_E|OT_v,              AM_I|OT_v,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_PUSH,  MNEMONIC_STR(enter)    AM_I|OT_w,              AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_RET,   MNEMONIC_STR(leave)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_RET,   MNEMONIC_STR(retf)     AM_I|OT_w,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(retf)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INT,   MNEMONIC_STR(int3)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INT,   MNEMONIC_STR(int)      AM_I|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(into)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(iret)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g2)       AM_E|OT_b,              AM_I1|OT_b,           FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g2)       AM_E|OT_v,              AM_I1|OT_b,           FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g2)       AM_E|OT_b,              AM_REG|REG_CL|OT_b,   FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g2)       AM_E|OT_v,              AM_REG|REG_CL|OT_b,   FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(aam)      AM_I|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(aad)      AM_I|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	// XXX: undocumened?
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(salc)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(xlat)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(esc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_LOOP,  MNEMONIC_STR(loopn)    AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_LOOP,  MNEMONIC_STR(loope)    AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_LOOP,  MNEMONIC_STR(loop)     AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jcxz)     AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(in)       AM_REG|REG_AL|OT_b,     AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(in)       AM_REG|REG_EAX|OT_v,    AM_I|OT_b,            FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(out)      AM_I|OT_b,              AM_REG|REG_AL|OT_b,   FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(out)      AM_I|OT_b,              AM_REG|REG_EAX|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_CALL,  MNEMONIC_STR(call)     AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMP,   MNEMONIC_STR(jmp)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMP,   MNEMONIC_STR(jmpf)     AM_A|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMP,   MNEMONIC_STR(jmp)      AM_J|OT_b,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(in)       AM_REG|REG_EAX|OT_b,    AM_REG|REG_EDX|OT_w,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(in)       AM_REG|REG_EAX|OT_v,    AM_REG|REG_EDX|OT_w,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(out)      AM_REG|REG_EDX|OT_w,    AM_REG|REG_EAX|OT_b,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(out)      AM_REG|REG_EDX|OT_w,    AM_REG|REG_EAX|OT_v,  FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(ext)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_INT,   MNEMONIC_STR(int1)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(ext)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(ext)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(hlt)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g3)       AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g3)       AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(clc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(stc)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cli)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(sti)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cld)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(std)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g4)       AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
	// XXX: far call/jmp syntax in 16-bit mode
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g5)       AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
};


static const INST inst_table2[256] = {
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g6)       AM_E|OT_w,              FLAGS_NONE,           FLAGS_NONE,  1 },
	// XXX: smsw and lmsw in grp 7 use addressing mode E !!!
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g7)       AM_M|OT_w,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lar)      AM_G|OT_v,              AM_E|OT_w,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lsl)      AM_G|OT_v,              AM_E|OT_w,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	// XXX: undocumented?
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(loadall286)FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(clts)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	// XXX: undocumented?
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(loadall)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(invd)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(wbinvd)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(ud2)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movups)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movups)   AM_W|OT_ps,             AM_V|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movlps)   AM_V|OT_v,              AM_W|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movlps)   AM_W|OT_q,              AM_V|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(unpcklps) AM_V|OT_ps,             AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(unpcklps) AM_V|OT_ps,             AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movhps)   AM_V|OT_q,              AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movhps)   AM_W|OT_q,              AM_V|OT_q,            FLAGS_NONE,  1 },
	// XXX: grp 16
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mov)      AM_R|OT_d,              AM_C|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mov)      AM_R|OT_d,              AM_D|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mov)      AM_C|OT_d,              AM_R|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mov)      AM_D|OT_d,              AM_R|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mov)      AM_R|OT_d,              AM_T|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mov)      AM_T|OT_d,              AM_R|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movaps)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movaps)   AM_W|OT_ps,             AM_V|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cvtpi2ps) AM_V|OT_ps,             AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movntps)  AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cvttps2pi)AM_P|OT_q,              AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cvtps2pi) AM_P|OT_v,              AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(ucomiss)  AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(comiss)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(wrmsr)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(rdtsc)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(rdmsr)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(rdpmc)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(sysenter) FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(sysexit)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovo)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovno)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovb)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovae)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmove)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovne)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovbe)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmova)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovs)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovns)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovp)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovnp)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovl)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovge)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovle)   AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmovg)    AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movmskps) AM_E|OT_d,              AM_V|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(sqrtps)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(rsqrtps)  AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(rcpps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(andps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(andnps)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(orps)     AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(xorps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(addps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(mulps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cvtps2pd) AM_V|OT_ps,             AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cvtdq2ps) AM_V|OT_ps,             AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(subps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(minps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(divps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(maxps)    AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpcklbw)AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpcklwd)AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punockldq)AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(packusdw) AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pcmpgtb)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pcmpgtw)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pcmpgtd)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(packsswb) AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpckhbw)AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpckhbd)AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpckhdq)AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(packssdw) AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpcklqdq)AM_V|OT_q,             AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(punpckhqd)AM_V|OT_q,              AM_W|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movd)     AM_P|OT_d,              AM_E|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movq)     AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pshufw)   AM_P|OT_q,              AM_Q|OT_q,            AM_I|OT_b,   1 },
	// XXX: check groups 12-14
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pshimw)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pshimd)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pshimq)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pcmpeqb)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pcmpeqw)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pcmpeqd)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(emms)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(haddpd)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(hsubpd)   AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movd)     AM_E|OT_d,              AM_P|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movq)     AM_Q|OT_q,              AM_P|OT_q,            FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jo)       AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jno)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jb)       AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnb)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jz)       AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnz)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jbe)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnbe)     AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(js)       AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jns)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jp)       AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnp)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jl)       AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnl)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jle)      AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_JMPC,  MNEMONIC_STR(jnle)     AM_J|OT_v,              FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(seto)     AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setno)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setb)     AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setnb)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setz)     AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setnz)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setbe)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setnbe)   AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(sets)     AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setns)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setp)     AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setnp)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setl)     AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setnl)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setle)    AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(setnle)   AM_E|OT_b,              FLAGS_NONE,           FLAGS_NONE,  1 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(push)     AM_REG|REG_FS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pop)      AM_REG|REG_FS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cpuid)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bt)       AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(shld)     AM_E|OT_v,              AM_G|OT_v,            AM_I|OT_b,   1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(shld)     AM_E|OT_v,              AM_G|OT_v,   AM_REG|REG_ECX|OT_b,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	// XXX: ibts: undocumented? 
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(push)     AM_REG|REG_GS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
	{ INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pop)      AM_REG|REG_GS|F_r,      FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(rsm)      FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bts)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(shrd)     AM_E|OT_v,              AM_G|OT_v,            AM_I|OT_b,   1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(shrd)     AM_E|OT_v,              AM_G|OT_v,  AM_REG|REG_ECX|OT_b,   1 },
	// XXX: check addressing mode, Intel manual is a little bit confusing...
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(grp15)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_MUL, MNEMONIC_STR(imul)       AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmpxchg)  AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmpxchg)  AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lss)      AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(btr)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lfs)      AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lgs)      AM_G|OT_v,              AM_M|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movzx)    AM_G|OT_v,              AM_E|OT_b,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movzx)    AM_G|OT_v,              AM_E|OT_w,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
	// XXX: group 10 / invalid opcode?
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g8)       AM_E|OT_v,              AM_I|OT_b,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(btc)      AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bsf)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bsr)      AM_G|OT_v,              AM_E|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movsx)    AM_G|OT_v,              AM_E|OT_b,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movsx)    AM_G|OT_v,              AM_E|OT_w,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(xadd)     AM_E|OT_b,              AM_G|OT_b,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(xadd)     AM_E|OT_v,              AM_G|OT_v,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cmpps)    AM_V|OT_ps,             AM_W|OT_ps,           AM_I|OT_b,   1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movnti)   AM_M|OT_d,              AM_G|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pinsrw)   AM_P|OT_q,              AM_E|OT_d,            AM_I|OT_b,   1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pextrv)   AM_G|OT_v,              AM_P|OT_q,            AM_I|OT_b,   1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(shufps)   AM_V|OT_ps,             AM_W|OT_ps,           AM_I|OT_b,   1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(g9)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_EAX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_ECX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_EDX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_EBX|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_ESP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_EBP|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_ESI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(bswap)    AM_REG|REG_EDI|OT_v,    FLAGS_NONE,           FLAGS_NONE,  0 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(addsubps) AM_V|OT_ps,             AM_W|OT_ps,           FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psrlw)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psrld)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psrlq)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddq)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmullw)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movq)     AM_W|OT_q,              AM_V|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmovmskb) AM_G|OT_v,              AM_P|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubusb)  AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubusw)  AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pminub)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pand)     AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddusb)  AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddusw)  AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmaxsw)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pandn)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pavgb)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psraw)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psrad)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pavgw)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmulhuw)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmulhw)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(cvtpd2dq) AM_V|OT_q,              AM_W|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(movntq)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubsb)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubsw)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pminsw)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(por)      AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddsb)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddsw)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmaxsw)   AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pxor)     AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(lddqu)    AM_V|OT_q,              AM_M|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psllw)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pslld)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psllq)    AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmuludq)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(pmaddwd)  AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psadbw)   AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
	// XXX: check operand types
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(maskmovq) AM_P|OT_q,              AM_Q|OT_d,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubb)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubw)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubd)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(psubq)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddb)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddw)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(paddd)    AM_P|OT_q,              AM_Q|OT_q,            FLAGS_NONE,  1 },
        { INSTRUCTION_TYPE_OTHER, MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 },
};

// Just a lame hack to provide additional arguments to group 3 MNEMONIC_STR(test"
static const INST inst_table3[2] = {
	{ INSTRUCTION_TYPE_TEST,  MNEMONIC_STR(test)     AM_E|OT_b,              AM_I|OT_b,            FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_TEST,  MNEMONIC_STR(test)     AM_E|OT_v,              AM_I|OT_v,            FLAGS_NONE,  1 }, 
};

// FPU instruction tables

/*
 * Tables are composed in two parts:
 *
 * - 1st part (index 0-7) are identified by the reg field of MODRM byte
 *   if the MODRM is < 0xc0. reg field can be used directly as an index to table.
 *
 * - 2nd part (8 - 0x47) are identified by the MODRM byte itself. In that case,
 *   the index can be calculated by MNEMONIC_STR(index = MODRM - 0xb8"
 *
 */
static const INST inst_table_fpu_d8[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadds)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmuls)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcoms)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomps)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubs)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrs)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivs)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrs)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcom)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
};
static const INST inst_table_fpu_d9[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldenv)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldcw)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstenv)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstcw)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld)      AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxch)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fnop)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fchs)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fabs)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ftst)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxam)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fld1)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldl2t)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldl2e)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldpi)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldlg2)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldln2)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldz)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(f2xm1)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fyl2x)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fptan)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fpatan)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fxtract)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fprem1)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdecstp)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fincstp)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fprem)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fyl2xp1)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsqrt)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsincos)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(frndint)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fscale)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsin)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcos)     FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
};
static const INST inst_table_fpu_da[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fiaddl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fimull)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ficoml)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ficompl)  AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fisubl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fisubrl)  AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fidivl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fidivrl)  AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovb)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmove)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovbe)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovu)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucompp)  FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
};

// XXX: fsetpm??
static const INST inst_table_fpu_db[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fildl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fistl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fistpl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldl)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstpl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnb)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovne)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnbe) AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcmovnu)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fclex)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(finit)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomi)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomi)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
};
static const INST inst_table_fpu_dc[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmull)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcoml)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcompl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrl)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fadd)     AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmul)     AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubr)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsub)     AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivr)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdiv)     AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
};
static const INST inst_table_fpu_dd[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fldl)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstl)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstpl)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(frstor)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsave)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstsw)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST0|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST1|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST2|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST3|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST4|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST5|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST6|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffree)    AM_REG|REG_ST7|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST0|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST1|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST2|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST3|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST4|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST5|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST6|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fst)      AM_REG|REG_ST7|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST0|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST1|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST2|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST3|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST4|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST5|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST6|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstp)     AM_REG|REG_ST7|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucom)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST0|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST1|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST2|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST3|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST4|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST5|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST6|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomp)   AM_REG|REG_ST7|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
};
static const INST inst_table_fpu_de[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fiadd)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fimul)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ficom)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ficomp)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fisub)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fisubr)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fidiv)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fidivr)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(faddp)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fmulp)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcompp)   FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubrp)   AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fsubp)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivrp)   AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST1|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST2|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST3|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST4|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST5|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST6|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fdivp)    AM_REG|REG_ST7|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
};

static const INST inst_table_fpu_df[72] = {
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fild)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	// fisttp: IA-32 2004
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fisttp)   AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fist)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fistp)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fbld)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fild)     AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fbstp)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fistp)    AM_E|OT_v,              FLAGS_NONE,           FLAGS_NONE,  1 }, 
	// ffreep undocumented!!
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST0|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST1|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST2|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST3|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST4|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST5|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST6|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(ffreep)   AM_REG|REG_ST7|F_f,     FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fstsw)    FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fucomip)  AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST0|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST1|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST2|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST3|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST4|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST5|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST6|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(fcomip)   AM_REG|REG_ST0|F_f,     AM_REG|REG_ST7|F_f,   FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
	{ INSTRUCTION_TYPE_FPU,   MNEMONIC_STR(NULL)       FLAGS_NONE,             FLAGS_NONE,           FLAGS_NONE,  0 }, 
};

// Table of FPU instruction tables

/*
 * These tables are accessed by the following way:
 *
 * INST *fpuinst = inst_table4[opcode - 0xd8][index];
 * where index is determined by the MODRM byte.
 *
 */
static const INST * inst_table4[8] = {
	inst_table_fpu_d8,
	inst_table_fpu_d9,
	inst_table_fpu_da,
	inst_table_fpu_db,
	inst_table_fpu_dc,
	inst_table_fpu_dd,
	inst_table_fpu_de,
	inst_table_fpu_df,
};

// for 2-byte opcodes

int get_real_instruction2(UCHAR *addr, int *flags) {
	switch (*addr) {

		// opcode extensions for 2-byte opcodes
		case 0x00:
			// Clear extension
			*flags &= 0xFFFFFF00;
			*flags |= EXT_G6;
			break;
		case 0x01:
			*flags &= 0xFFFFFF00;
			*flags |= EXT_G7;
			break;
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
			*flags &= 0xFFFFFF00;
			*flags |= EXT_GA;
			break;
		case 0xae:
			*flags &= 0xFFFFFF00;
			*flags |= EXT_GF;
			break;
		case 0xba:
			*flags &= 0xFFFFFF00;
			*flags |= EXT_G8;
			break;
		case 0xc7:
			*flags &= 0xFFFFFF00;
			*flags |= EXT_G9;
			break;
		default:
			break;
	}
	return 0;
}

// Parse instruction flags, get opcode index

int get_real_instruction(UCHAR *addr, int *index, int *flags) {
	switch (*addr) {

		// 2-byte opcode
		case 0x0f:
			*index += 1;
			*flags |= EXT_T2;
			break;

		// Prefix group 2
		case 0x2e:
			*index += 1;
			// Clear previous flags from same group (undefined effect)
			*flags &= 0xFF00FFFF;
			*flags |= PREFIX_CS_OVERRIDE;
			get_real_instruction(addr + 1, index, flags);
			break;
		case 0x36:
			*index += 1;
			*flags &= 0xFF00FFFF;
			*flags |= PREFIX_SS_OVERRIDE;
			get_real_instruction(addr + 1, index, flags);
			break;
		case 0x3e:
			*index += 1;
			*flags &= 0xFF00FFFF;
			*flags |= PREFIX_DS_OVERRIDE;
			get_real_instruction(addr + 1, index, flags);
			break;
		case 0x26:
			*index += 1;
			*flags &= 0xFF00FFFF;
			*flags |= PREFIX_ES_OVERRIDE;
			get_real_instruction(addr + 1, index, flags);
			break;
		case 0x64:
			*index += 1;
			*flags &= 0xFF00FFFF;
			*flags |= PREFIX_FS_OVERRIDE;
			get_real_instruction(addr + 1, index, flags);
			break;
		case 0x65:
			*index += 1;
			*flags &= 0xFF00FFFF;
			*flags |= PREFIX_GS_OVERRIDE;
			get_real_instruction(addr + 1, index, flags);
			break;
		// Prefix group 3
		case 0x66:
			// Do not clear flags from the same group!!!!
			*index += 1;
			*flags |= PREFIX_OPERAND_SIZE_OVERRIDE;
			get_real_instruction(addr + 1, index, flags); 
			break;
		// Prefix group 4
		case 0x67:
			// Do not clear flags from the same group!!!!
			*index += 1;
			*flags |=  PREFIX_ADDR_SIZE_OVERRIDE;
			get_real_instruction(addr + 1, index, flags); 
			break;

		// Extension group 1
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
			*flags |=  EXT_G1;
			break;

		// Extension group 2
		case 0xc0:
		case 0xc1:
		case 0xd0:
		case 0xd1:
		case 0xd2:
		case 0xd3:
			*flags |=  EXT_G2;
			break;

		// Escape to co-processor
		case 0xd8:
		case 0xd9:
		case 0xda:
		case 0xdb:
		case 0xdc:
		case 0xdd:
		case 0xde:
		case 0xdf:
			*index += 1;
			*flags |=  EXT_CP;
			break;

		// Prefix group 1
		case 0xf0:
			*index += 1;
			*flags &= 0x00FFFFFF;
			*flags |=  PREFIX_LOCK;
			get_real_instruction(addr + 1, index, flags); 
			break;
		case 0xf2:
			*index += 1;
			*flags &= 0x00FFFFFF;
			*flags |=  PREFIX_REPNE;
			get_real_instruction(addr + 1, index, flags); 
			break;
		case 0xf3:
			*index += 1;
			*flags &= 0x00FFFFFF;
			*flags |=  PREFIX_REP;
			get_real_instruction(addr + 1, index, flags); 
			break;

		// Extension group 3
		case 0xf6:
		case 0xf7:
			*flags |=  EXT_G3;
			break;

		// Extension group 4
		case 0xfe:
			*flags |=  EXT_G4;
			break;

		// Extension group 5
		case 0xff:
			*flags |=  EXT_G5;
			break;
		default:
			break;
	}
	return 0;
}

// Parse operand and fill OPERAND structure

int get_operand(PINST inst, int oflags, PINSTRUCTION instruction,
	POPERAND op, UCHAR *data, int offset, enum Mode mode, int iflags) {
	UCHAR *addr = data + offset;
	int index = 0, sib = 0, scale = 0;
	int reg      = REG_NOP;
	int basereg  = REG_NOP;
	int indexreg = REG_NOP;
	int dispbytes = 0;
	enum Mode pmode;

	// Is this valid operand?
	if (oflags == FLAGS_NONE) {
		op->type = OPERAND_TYPE_NONE;
		return 1;
	}
	// Copy flags
	op->flags = oflags;

	// Set operand registers
	op->reg      = REG_NOP;
	op->basereg  = REG_NOP;
	op->indexreg = REG_NOP;

	// Offsets
	op->dispoffset = 0;
	op->immoffset  = 0;

	// Parse modrm and sib
	if (inst->modrm) {
		// 32-bit mode
		if (((mode == MODE_32) && (MASK_PREFIX_ADDR(iflags) == 0)) ||
		    ((mode == MODE_16) && (MASK_PREFIX_ADDR(iflags) == 1)))
			pmode = MODE_32;
		else 
			pmode = MODE_16;

		// Update length only once!
		if (!instruction->length) {
			instruction->modrm = *addr;
			instruction->length += 1;
		}
		// Register
		reg =  MASK_MODRM_REG(*addr);

		// Displacement bytes
		// SIB can also specify additional displacement, see below
		if (MASK_MODRM_MOD(*addr) == 0) {
			if ((pmode == MODE_32) && (MASK_MODRM_RM(*addr) == REG_EBP))
				dispbytes = 4;
			if ((pmode == MODE_16) && (MASK_MODRM_RM(*addr) == REG_ESI))
				dispbytes = 2;
		} else if (MASK_MODRM_MOD(*addr) == 1) {
			dispbytes = 1;

		} else if (MASK_MODRM_MOD(*addr) == 2) {
			dispbytes = (pmode == MODE_32) ? 4 : 2; 
		}
		// Base and index registers

		// 32-bit mode
		if (pmode == MODE_32) {
			if ((MASK_MODRM_RM(*addr) == REG_ESP) && 
					(MASK_MODRM_MOD(*addr) != 3)) {
				sib = 1;
				instruction->sib = *(addr + 1);

				// Update length only once!
				if (instruction->length == 1) {
					instruction->sib = *(addr + 1);
					instruction->length += 1;
				}
				basereg  = MASK_SIB_BASE( *(addr + 1));
				indexreg = MASK_SIB_INDEX(*(addr + 1));
				scale    = MASK_SIB_SCALE(*(addr + 1)) * 2;
				// Fix scale *8
				if (scale == 6)
					scale += 2;

				// Special case where base=ebp and MOD = 0
				if ((basereg == REG_EBP) && !MASK_MODRM_MOD(*addr)) {
					basereg = REG_NOP;
						dispbytes = 4;
				}
				if (indexreg == REG_ESP)
					indexreg = REG_NOP;
			} else {
				if (!MASK_MODRM_MOD(*addr) && (MASK_MODRM_RM(*addr) == REG_EBP))
					basereg = REG_NOP;
				else
					basereg = MASK_MODRM_RM(*addr);
			}
		// 16-bit
		} else {
			switch (MASK_MODRM_RM(*addr)) {
				case 0:
					basereg  = REG_EBX;
					indexreg = REG_ESI;
					break;
				case 1:
					basereg  = REG_EBX;
					indexreg = REG_EDI;
					break;
				case 2:
					basereg  = REG_EBP;
					indexreg = REG_ESI;
					break;
				case 3:
					basereg  = REG_EBP;
					indexreg = REG_EDI;
					break;
				case 4:
					basereg  = REG_ESI;
					indexreg = REG_NOP;
					break;
				case 5:
					basereg  = REG_EDI;
					indexreg = REG_NOP;
					break;
				case 6:
					if (!MASK_MODRM_MOD(*addr))
						basereg = REG_NOP;
					else
						basereg = REG_EBP;
					indexreg = REG_NOP;
					break;
				case 7:
					basereg  = REG_EBX;
					indexreg = REG_NOP;
					break;
			}
			if (MASK_MODRM_MOD(*addr) == 3) {
				basereg  = MASK_MODRM_RM(*addr);
				indexreg = REG_NOP;
			}
		}
	}
	// Operand addressing mode -specific parsing
	switch (MASK_AM(oflags)) {

		// Register encoded in instruction
		case AM_REG:
			op->type = OPERAND_TYPE_REGISTER;
			op->reg  = MASK_REG(oflags);
			break;

		// Register/memory encoded in MODRM
		case AM_R:
			if (MASK_MODRM_MOD(*addr) != 3)
				return 0;
		case AM_M:
			if (MASK_MODRM_MOD(*addr) == 3)
				return 0;
		case AM_Q:
		case AM_W:
		case AM_E:
			op->type = OPERAND_TYPE_MEMORY;
			op->dispbytes          = dispbytes;
			instruction->dispbytes = dispbytes;
			op->basereg            = basereg;
			op->indexreg           = indexreg;
			op->scale              = scale;

			index = (sib) ? 1 : 0;
			if (dispbytes)
				op->dispoffset = index + 1 + offset;
			switch (dispbytes) {
				case 0:
					break;
				case 1:
					op->displacement = *(UCHAR *)(addr + 1 + index);
					// Always sign-extend
					if (op->displacement >= 0x80)
						op->displacement |= 0xffffff00;
					break;
				case 2:
					op->displacement = *(USHORT *)(addr + 1 + index);
					// Malformed opcode
					if (op->displacement < 0x80)
						return 0;
					break;
				case 4:
					op->displacement = *(ULONG *)(addr + 1 + index);
					// XXX: problems with [index*scale + disp] addressing
					//if (op->displacement < 0x80)
					//	return 0;
					break;
			}

			// MODRM defines register
			if ((basereg != REG_NOP) && (MASK_MODRM_MOD(*addr) == 3)) { 
				op->type = OPERAND_TYPE_REGISTER;
				op->reg  = basereg;
			}
			break;

		// Immediate byte 1 encoded in instruction
		case AM_I1:
			op->type = OPERAND_TYPE_IMMEDIATE;
			op->immbytes  = 1;
			op->immediate = 1;
			break;
		// Immediate value
		case AM_J:
			op->type = OPERAND_TYPE_IMMEDIATE;
			// Always sign-extend
			oflags |= F_s;
		case AM_I:
			op->type = OPERAND_TYPE_IMMEDIATE;
			index  = (inst->modrm) ? 1 : 0;
			index += (sib) ? 1 : 0;
			index += instruction->immbytes;
			index += instruction->dispbytes;
			op->immoffset = index + offset;

			// 32-bit mode
			if (((mode == MODE_32) && (MASK_PREFIX_OPERAND(iflags) == 0)) ||
		    	    ((mode == MODE_16) && (MASK_PREFIX_OPERAND(iflags) == 1)))
				mode = MODE_32;
			else 
				mode = MODE_16;

			switch (MASK_OT(oflags)) {
				case OT_b:
					op->immbytes  = 1;
					op->immediate = *(UCHAR *)(addr + index);
					if ((op->immediate >= 0x80) &&
						(MASK_FLAGS(oflags) == F_s))
						op->immediate |= 0xffffff00;
					break;
				case OT_v:
					op->immbytes  = (mode == MODE_32) ? 4 : 2;
					op->immediate = (mode == MODE_32) ? 
						*(ULONG *)(addr + index) :
						*(USHORT *) (addr + index);
					break;
				case OT_w:
					op->immbytes  = 2;
					op->immediate = *(USHORT *)(addr + index);
					break;
			}
			instruction->immbytes += op->immbytes;
			break;

		// 32-bit or 48-bit address
		case AM_A:
			op->type = OPERAND_TYPE_IMMEDIATE;
			// 32-bit mode
			if (((mode == MODE_32) && (MASK_PREFIX_OPERAND(iflags) == 0)) ||
		    	    ((mode == MODE_16) && (MASK_PREFIX_OPERAND(iflags) == 1)))
				mode = MODE_32;
			else 
				mode = MODE_16;

			op->dispbytes    = (mode == MODE_32) ? 6 : 4;
			op->displacement = (mode == MODE_32) ?
				*(ULONG *)addr : *(USHORT *) addr;
			op->section = *(USHORT *)(addr + op->dispbytes - 2);

			instruction->dispbytes    = op->dispbytes;
			instruction->sectionbytes = 2;
			break;

		// Plain displacement without MODRM/SIB
		case AM_O:
			op->type = OPERAND_TYPE_MEMORY;
			switch (MASK_OT(oflags)) {
				case OT_b:
					op->dispbytes    = 1;
					op->displacement = *(UCHAR *)addr;
					break;
				case OT_v:
					op->dispbytes    = (mode == MODE_32) ? 4 : 2;
					op->displacement = (mode == MODE_32) ?
						*(ULONG *)addr :
						*(USHORT *) addr;
					break;
			}
			op->dispoffset = offset;
			instruction->dispbytes = op->dispbytes;
			break;

		// General-purpose register encoded in MODRM
		case AM_G:
			op->type = OPERAND_TYPE_REGISTER;
			op->reg  = reg;
			break;

		// control register encoded in MODRM
		case AM_C:
		// debug register encoded in MODRM
		case AM_D:
		// Segment register encoded in MODRM
		case AM_S:
		// TEST register encoded in MODRM
		case AM_T:
		// MMX register encoded in MODRM
		case AM_P:
		// SIMD register encoded in MODRM
		case AM_V:
			op->type = OPERAND_TYPE_REGISTER;
			op->reg  = MASK_MODRM_REG(instruction->modrm);
			break;
	}
	return 1;
}


// Print operand string

#if !defined NOSTR
int get_operand_string(INSTRUCTION *inst, OPERAND *op,
	enum Format format, ULONG offset, char *string, int length) {
	
	enum Mode mode;
	int regtype = 0;

	//memset(string, 0, length);

	if (op->type == OPERAND_TYPE_REGISTER) {
		// 32-bit mode
		if (((inst->mode == MODE_32) && (MASK_PREFIX_OPERAND(inst->flags) == 0)) ||
		    ((inst->mode == MODE_16) && (MASK_PREFIX_OPERAND(inst->flags) == 1)))
			mode = MODE_32;
		else 
			mode = MODE_16;

		if (format == FORMAT_ATT)
			snprintf(string + strlen(string), length - strlen(string), "%%");
	
		// Determine register type
		switch (MASK_AM(op->flags)) {
			case AM_REG:
				if (MASK_FLAGS(op->flags) == F_r)
					regtype = REG_SEGMENT;
				else if (MASK_FLAGS(op->flags) == F_f)
					regtype = REG_FPU;
				else
					regtype = REG_GEN_ULONG;
				break;
			case AM_E:
			case AM_G:
			case AM_R:
				regtype = REG_GEN_ULONG;
				break;
			// control register encoded in MODRM
			case AM_C:
				regtype = REG_CONTROL;
				break;
			// debug register encoded in MODRM
			case AM_D:
				regtype = REG_DEBUG;
				break;
			// Segment register encoded in MODRM
			case AM_S:
				regtype = REG_SEGMENT;
				break;
			// TEST register encoded in MODRM
			case AM_T:
				regtype = REG_TEST;
				break;
			// MMX register encoded in MODRM
			case AM_P:
			case AM_Q:
				regtype = REG_MMX;
				break;
			// SIMD register encoded in MODRM
			case AM_V:
			case AM_W:
				regtype = REG_SIMD;
				break;
		}
		if (regtype == REG_GEN_ULONG) {
			 switch (MASK_OT(op->flags)) {
				case OT_b:
					snprintf(string + strlen(string), length - strlen(string),
						"%s", reg_table[REG_GEN_UCHAR][op->reg]);
                                        break;
				case OT_v:
					snprintf(string + strlen(string), length - strlen(string),
						"%s", (mode == MODE_32) ?
						reg_table[REG_GEN_ULONG][op->reg] :
						reg_table[REG_GEN_USHORT][op->reg]);
                                        break;
				case OT_w:
					snprintf(string + strlen(string), length - strlen(string),
						"%s", reg_table[REG_GEN_USHORT][op->reg]);
					break;
			}
		} else
			snprintf(string + strlen(string), length - strlen(string),
				"%s", reg_table[regtype][op->reg]);

	} else if (op->type == OPERAND_TYPE_MEMORY) {
		// 32-bit mode
		if (((inst->mode == MODE_32) && (MASK_PREFIX_ADDR(inst->flags) == 0)) ||
		    ((inst->mode == MODE_16) && (MASK_PREFIX_ADDR(inst->flags) == 1)))
			mode = MODE_32;
		else 
			mode = MODE_16;

		// Segment register prefix (only in memory operands)
		if (MASK_PREFIX_G2(inst->flags)) {
			if (format == FORMAT_ATT)
				snprintf(string + strlen(string),
					length - strlen(string), "%%");
			snprintf(string + strlen(string), length - strlen(string),
				"%s:", reg_table[REG_SEGMENT][(MASK_PREFIX_G2(inst->flags)) - 1]);
		}
		// Displacement in ATT
		if (op->dispbytes && (format == FORMAT_ATT))
			snprintf(string + strlen(string), length - strlen(string),
				"0x%x", op->displacement); 

		// Open memory addressing brackets
		snprintf(string + strlen(string), length - strlen(string),
			"%s", (format == FORMAT_ATT) ? "(" : "["); 

		// Base register
		if (op->basereg != REG_NOP) {
			snprintf(string + strlen(string), length - strlen(string),
				"%s%s", (format == FORMAT_ATT) ? "%" : "", 
				(mode == MODE_32) ?
				reg_table[REG_GEN_ULONG][op->basereg] :
				reg_table[REG_GEN_USHORT][op->basereg]);
		}
		// Index register
		if (op->indexreg != REG_NOP) {
			if (op->basereg != REG_NOP)
				snprintf(string + strlen(string), length - strlen(string),
					"%s%s", (format == FORMAT_ATT) ? ",%" : "+", 
					(mode == MODE_32) ?
					reg_table[REG_GEN_ULONG][op->indexreg] :
					reg_table[REG_GEN_USHORT][op->indexreg]); 
			else
				snprintf(string + strlen(string), length - strlen(string),
					"%s%s", (format == FORMAT_ATT) ? "%" : "",
					(mode == MODE_32) ?
					reg_table[REG_GEN_ULONG][op->indexreg] :
					reg_table[REG_GEN_USHORT][op->indexreg]); 
			switch (op->scale) {
				case 2:
					snprintf(string + strlen(string), length - strlen(string),
						"%s", (format == FORMAT_ATT) ?
						",2" : "*2"); 
					break;
				case 4:
					snprintf(string + strlen(string), length - strlen(string),
						"%s", (format == FORMAT_ATT) ?
						",4" : "*4"); 
					break;
				case 8:
					snprintf(string + strlen(string), length - strlen(string),
						"%s", (format == FORMAT_ATT) ?
						",8" : "*8"); 
					break;
			}
		}
		// Displacement
		if (inst->dispbytes && (format != FORMAT_ATT)) {
			// Looks nicer with -disp..
			if ((op->dispbytes == 1) && (op->displacement >= 0x80)) {
				snprintf(string + strlen(string), length - strlen(string),
					"-0x%x", ~op->displacement + 1);
			} else {
				if ((op->basereg != REG_NOP) || (op->indexreg != REG_NOP))
					snprintf(string + strlen(string), length - strlen(string),
						"+0x%x", op->displacement);
				else
					snprintf(string + strlen(string), length - strlen(string),
						"0x%x", op->displacement);
			}
		}
		// Close memory addressing brackets
		snprintf(string + strlen(string), length - strlen(string),
				"%s", (format == FORMAT_ATT) ? ")" : "]"); 

	} else if (op->type == OPERAND_TYPE_IMMEDIATE) {
		// 32-bit mode
		if (((inst->mode == MODE_32) && (MASK_PREFIX_OPERAND(inst->flags) == 0)) ||
		    ((inst->mode == MODE_16) && (MASK_PREFIX_OPERAND(inst->flags) == 1)))
			mode = MODE_32;
		else 
			mode = MODE_16;

		switch (MASK_AM(op->flags)) {
			case AM_J:
				snprintf(string + strlen(string), length - strlen(string),
					"0x%x", op->immediate + inst->length + offset);
				break;
			case AM_I1:
			case AM_I:
				if (format == FORMAT_ATT)
					snprintf(string + strlen(string), length - strlen(string), "$");
				snprintf(string + strlen(string), length - strlen(string),
					"0x%x", op->immediate);
				break;
			// 32-bit or 48-bit address
			case AM_A:
				snprintf(string + strlen(string), length - strlen(string),
					"%s0x%x:%s0x%x",
					(format == FORMAT_ATT) ? "$" : "",
					op->section, 
					(format == FORMAT_ATT) ? "$" : "",
					op->displacement);
				break;
		}

	} else
		return 0;

	return 1;
}

#endif

//
// Disassemble until the minimum size specified is satisified
//
int DisassembleUntil(
	IN PUCHAR Address,
	IN ULONG MinimumSize)
{
	INSTRUCTION Instruction;
	ULONG CurrentLength;
	ULONG TotalLength = 0;

	while (TotalLength < MinimumSize)
	{
		CurrentLength = get_instruction(&Instruction, Address, MODE_32);

		if (!CurrentLength)
		{
			TotalLength = 0;
			break;
		}

		Address     += CurrentLength;
		TotalLength += CurrentLength;
	}

	return TotalLength;
}

// Fetch instruction

int get_instruction(PINSTRUCTION inst, UCHAR *addr, enum Mode mode) {
	PINST ptr;
	int index = 0;
	int flags = 0;
	const char *ext = NULL;

	RtlZeroMemory(inst, sizeof(INSTRUCTION));

	// Parse flags, skip prefixes etc.
	get_real_instruction(addr, &index, &flags);

	// Select instruction table 

	// co-processor escape
	if (MASK_EXT(flags) == EXT_CP) {
		if (*(addr + index) < 0xc0) {
			// MODRM byte adds the additional byte
			index--;
			inst->opcode = MASK_MODRM_REG(*(addr + index + 1));
 			ptr = (PINST)&inst_table4[*(addr + index) - 0xd8]
				[MASK_MODRM_REG(*(addr + index + 1))];
		} else {
			inst->opcode = *(addr + index) - 0xd8;
 			ptr = (PINST)&inst_table4[*(addr + index - 1) - 0xd8]
				[*(addr + index) - 0xb8];
		}
	// 2-byte opcode
	} else if (MASK_EXT(flags) == EXT_T2) {
		inst->opcode = *(addr + index);
		ptr = (PINST)&inst_table2[inst->opcode];
		get_real_instruction2(addr + index, &flags);

	// extension group 3 "test" (<-- stupid hack)
	} else if ((MASK_EXT(flags) == EXT_G3) && !MASK_MODRM_REG(*(addr + index + 1))) {
		inst->opcode = *(addr + index);
		ptr = (PINST)&inst_table3[inst->opcode - 0xf6];

	// finally, the default 1-byte opcode table
	} else {
		inst->opcode = *(addr + index);
		ptr = (PINST)&inst_table1[inst->opcode];
	}

	// Illegal instruction
#if !defined(NOSTR)
        if (!ptr->mnemonic) return 0;
#endif

	// Instruction table
	inst->ptr = ptr;

	// Copy instruction type
	inst->type = ptr->type;

	// Index points now to first byte after prefixes/escapes
	index++;

	// Opcode extensions
	if (MASK_EXT(flags) && (MASK_EXT(flags) != EXT_T2) &&
			(MASK_EXT(flags) != EXT_CP)) {
		inst->extindex = MASK_MODRM_REG(*(addr + index));
#if !defined(NOSTR)
		ext = ext_name_table[(MASK_EXT(flags)) - 1][inst->extindex];
		if (ext == NULL)
			return 0;
#endif
		inst->type = ext_type_table[(MASK_EXT(flags)) - 1][inst->extindex];
	} 

	// Parse operands
	if (!get_operand(ptr, ptr->flags1, inst, &inst->op1, addr, index,
			mode, flags))
		return 0;
	if (!get_operand(ptr, ptr->flags2, inst, &inst->op2, addr, index,
			mode, flags))
		return 0;
	if (!get_operand(ptr, ptr->flags3, inst, &inst->op3, addr, index,
			mode, flags))
		return 0;

	// Add modrm/sib, displacement and immediate bytes in size
	inst->length += index + inst->immbytes + inst->dispbytes;

	// Copy addressing mode
	inst->mode = mode;

	// Copy instruction flags
	inst->flags = flags;

	return inst->length;
}


// Print instruction mnemonic

#if !defined NOSTR
int get_mnemonic_string(INSTRUCTION *inst, enum Format format, char *string, int length) {
	const char *ext;

	//memset(string, 0, length);

	// Segment override
	if (MASK_PREFIX_G2(inst->flags) &&
		(inst->op1.type != OPERAND_TYPE_MEMORY) &&
		(inst->op2.type != OPERAND_TYPE_MEMORY))
		snprintf(string + strlen(string), length - strlen(string),
			"%s ", reg_table[REG_SEGMENT][(MASK_PREFIX_G2(inst->flags)) - 1]);

	// Rep, lock etc.
	if (MASK_PREFIX_G1(inst->flags))
		snprintf(string + strlen(string), length - strlen(string),
			"%s", rep_table[(MASK_PREFIX_G1(inst->flags)) - 1]);

	// Opcode extensions
	if (MASK_EXT(inst->flags) && (MASK_EXT(inst->flags) != EXT_T2) &&
                        (MASK_EXT(inst->flags) != EXT_CP)) {
		ext = ext_name_table[(MASK_EXT(inst->flags)) - 1][inst->extindex];
		snprintf(string + strlen(string), length - strlen(string),
			"%s", ext);
	} else {
		snprintf(string + strlen(string), length - strlen(string),
			"%s", inst->ptr->mnemonic);
	}

	// memory operation size in immediate to memory operations
	// XXX: also, register -> memory operations when size is different
	if (inst->ptr->modrm && (MASK_MODRM_MOD(inst->modrm) != 3) &&
		(MASK_AM(inst->op2.flags) == AM_I)) {

		switch (MASK_OT(inst->op1.flags)) {
			case OT_b:
				snprintf(string + strlen(string), length - strlen(string),
					"%s", (format == FORMAT_ATT) ?
					"b" : " byte");
				break;
			case OT_w:
				snprintf(string + strlen(string), length - strlen(string),
					"%s", (format == FORMAT_ATT) ?
					"w" : " word");
				break;
			case OT_d:
				snprintf(string + strlen(string), length - strlen(string),
					"%s", (format == FORMAT_ATT) ?
					"l" : " dword");
				break;
			case OT_v:
				if (((inst->mode == MODE_32) && (MASK_PREFIX_OPERAND(inst->flags) == 0)) ||
				    ((inst->mode == MODE_16) && (MASK_PREFIX_OPERAND(inst->flags) == 1)))
					snprintf(string + strlen(string), length - strlen(string),
						"%s", (format == FORMAT_ATT) ?
						"l" : " dword");
				else
					snprintf(string + strlen(string), length - strlen(string),
						"%s", (format == FORMAT_ATT) ?
						"w" : " word");
				break;
		}
	}
	return 1;
}

// Print operands

int get_operands_string(INSTRUCTION *inst, enum Format format, ULONG offset,
	char *string, int length) {

	if (format == FORMAT_ATT) {
		if (inst->op3.type != OPERAND_TYPE_NONE) {
			get_operand_string(inst, &inst->op3, format, offset,
				string + strlen(string), length - strlen(string));
			snprintf(string + strlen(string), length - strlen(string), ",");
		}
		if (inst->op2.type != OPERAND_TYPE_NONE) {
			get_operand_string(inst, &inst->op2, format, offset,
				string + strlen(string), length - strlen(string));
			snprintf(string + strlen(string), length - strlen(string), ",");
		}
		if (inst->op1.type != OPERAND_TYPE_NONE)
			get_operand_string(inst, &inst->op1, format, offset,
				string + strlen(string), length - strlen(string));
	} else if (format == FORMAT_INTEL) {
		if (inst->op1.type != OPERAND_TYPE_NONE)
			get_operand_string(inst, &inst->op1, format, offset,
				string + strlen(string), length - strlen(string));
		if (inst->op2.type != OPERAND_TYPE_NONE) {
			snprintf(string + strlen(string), length - strlen(string), ",");
			get_operand_string(inst, &inst->op2, format, offset,
				string + strlen(string), length - strlen(string));
		}
		if (inst->op3.type != OPERAND_TYPE_NONE) {
			snprintf(string + strlen(string), length - strlen(string), ",");
			get_operand_string(inst, &inst->op3, format, offset,
				string + strlen(string), length - strlen(string));
		}
	} else
		return 0;

	return 1;
}

// Print instruction mnemonic, prefixes and operands

int get_instruction_string(INSTRUCTION *inst, enum Format format, ULONG offset,
		char *string, int length) {

	// Print the actual instruction string with possible prefixes etc.
	get_mnemonic_string(inst, format, string, length);

	snprintf(string + strlen(string), length - strlen(string), " ");
	
	// Print operands
	if (!get_operands_string(inst, format, offset,
		string + strlen(string), length - strlen(string)))
		return 0;

	return 1;
}

#endif

// Helper functions

int get_register_type(POPERAND op) {
	
	if (op->type != OPERAND_TYPE_REGISTER)
		return 0;
	switch (MASK_AM(op->flags)) {
		case AM_REG:
			if (MASK_FLAGS(op->flags) == F_r)
				return REGISTER_TYPE_SEGMENT;
			else if (MASK_FLAGS(op->flags) == F_f)
				return REGISTER_TYPE_FPU;
			else
				return REGISTER_TYPE_GEN;
		case AM_E:
		case AM_G:
		case AM_R:
				return REGISTER_TYPE_GEN;
		case AM_C:
				return REGISTER_TYPE_CONTROL;
		case AM_D:
				return REGISTER_TYPE_DEBUG;
		case AM_S:
				return REGISTER_TYPE_SEGMENT;
		case AM_T:
				return REGISTER_TYPE_TEST;
		case AM_P:
		case AM_Q:
				return REGISTER_TYPE_MMX;
		case AM_V:
		case AM_W:
				return REGISTER_TYPE_SIMD;
		default:
				break;
	}
	return 0;
}

int get_operand_type(POPERAND op) {
	return op->type;
}

int get_operand_register(POPERAND op) {
	return op->reg;
}

int get_operand_basereg(POPERAND op) {
	return op->basereg;
}

int get_operand_indexreg(POPERAND op) {
	return op->indexreg;
}

int get_operand_scale(POPERAND op) {
	return op->scale;
}

int get_operand_immediate(POPERAND op, ULONG *imm) {
	if (op->immbytes) {
		*imm = op->immediate;
		return 1;
	} else
		return 0;
}

int get_operand_displacement(POPERAND op, ULONG *disp) {
	if (op->dispbytes) {
		*disp = op->displacement;
		return 1;
	} else
		return 0;
}

// XXX: note that source and destination are not always literal

POPERAND get_source_operand(PINSTRUCTION inst) {
	if (inst->op2.type != OPERAND_TYPE_NONE)
		return &inst->op2;
	else
		return NULL;
}
POPERAND get_destination_operand(PINSTRUCTION inst) {
	if (inst->op1.type != OPERAND_TYPE_NONE)
		return &inst->op1;
	else
		return NULL;
}



