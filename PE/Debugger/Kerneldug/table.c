#include "internal.h"

/* 
Opcodes Flags:
Bit: 7 6 5 4 3 2 1 0
     ¦ ¦ ¦ ¦ ¦ ¦ ¦ L OP_MODRM     - MOD r/m present
     ¦ ¦ ¦ ¦ ¦ ¦ L-- OP_DATA_I8   - imm8 operand
     ¦ ¦ ¦ ¦ ¦ L---- OP_DATA_I16  - imm 16 operand
     ¦ ¦ ¦ ¦ L------ OP_DATA_I32  - imm 32 operand
     ¦ ¦ ¦ L-------- OP_DATA_PRE66 - operand PRE66
     ¦ ¦ L---------- OP_DATA_PRE67 - operand PRE67
     ¦ L------------ OP_W
     L-------------- OP_D

Opcodes Flags:
Bit: 15 14 13 12 11 10 9 8
      ¦  ¦  ¦  ¦  ¦  ¦ ¦ L- OP_BYTE (OP_S f| base opcodes only)
      ¦  ¦  ¦  ¦  ¦  ¦ L--- OP_SREG2  (in base opcode only) (OP_MMX in extended opcodes)
      ¦  ¦  ¦  ¦  ¦  L----- OP_SREG3  (in extended opcode only!) 
      ¦  ¦  ¦  ¦  L-------- OP_TTTn  - opcode has condition
      ¦  ¦  ¦  L----------- OP_EEE   - opcode has DR, CR | TR registers  (in extended opcodes only!)
      ¦  ¦  L-------------- OP_WORD  - w|d size opcode
      ¦  L----------------- OP_MNEMDIFF - mnemonic different in 16 and 32 bit modes (16 bit opcode lookup from DifTable), f| extended instructions - OP_MNEMEXT
      L-------------------- OP_RD - opcode contain register number
Bits 16-23 - opcode mnemonic |dinal
Opcodes Flags:
Bit: 31 30 29 28 27 26 25 24
      ¦  ¦  ¦  ¦  ¦  ¦ ¦  L modrm used bit
      ¦  ¦  ¦  ¦  ¦  ¦ L--- OP_REV - always reversing operands
      ¦  ¦  ¦  ¦  ¦  L----- OP_ONE - one param in MOD 11
      ¦  ¦  ¦  ¦  L-------- OP_THREE - three params in opcode           
      ¦  ¦  ¦  L------------ OP_SSE - sse instruction
      ¦  ¦  L--------------- OP_SSE_PFX - use rep prefix f| SSE instruction
      ¦  L-----------------
      L--------------------
*/

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

ULONG OpcodeFlags[0x100] = {
  OP_MODRM | (0x62 << 16) | OP_W | OP_D,                    // 00
  OP_MODRM | (0x62 << 16) | OP_W | OP_D,                    // 01
  OP_MODRM | (0x62 << 16) | OP_W | OP_D,                    // 02
  OP_MODRM | (0x62 << 16) | OP_W | OP_D,                    // 03
  OP_DATA_I8 | (0x62 << 16) | OP_W,                         // 04
  OP_DATA_PRE66 | (0x62 << 16) | OP_W,                      // 05
  (0xD8 << 16) | OP_SREG2,                                  // 06
  (0xD9 << 16) | OP_SREG2,                                  // 07
  OP_MODRM | (0x6F << 16) | OP_W | OP_D,                    // 08
  OP_MODRM | (0x6F << 16) | OP_W | OP_D,                    // 09
  OP_MODRM | (0x6F << 16) | OP_W | OP_D,                    // 0A
  OP_MODRM | (0x6F << 16) | OP_W | OP_D,                    // 0B
  OP_DATA_I8 | (0x6F << 16) | OP_W,                         // 0C
  OP_DATA_PRE66 | (0x6F << 16) | OP_W,                      // 0D
  (0xD8 << 16) | OP_SREG2,                                  // 0E
  OP_NONE,                                                  // 0F
  OP_MODRM | (0x61 << 16) | OP_W | OP_D,                    // 10
  OP_MODRM | (0x61 << 16) | OP_W | OP_D,                    // 11
  OP_MODRM | (0x61 << 16) | OP_W | OP_D,                    // 12
  OP_MODRM | (0x61 << 16) | OP_W | OP_D,                    // 13
  OP_DATA_I8 | (0x61 << 16) | OP_W,                         // 14
  OP_DATA_PRE66 | (0x61 << 16) | OP_W,                      // 15
  (0xD8 << 16) | OP_SREG2,                                  // 16
  (0xD9 << 16) | OP_SREG2,                                  // 17
  OP_MODRM | (0x70 << 16) | OP_W | OP_D,                    // 18
  OP_MODRM | (0x70 << 16) | OP_W | OP_D,                    // 19
  OP_MODRM | (0x70 << 16) | OP_W | OP_D,                    // 1A
  OP_MODRM | (0x70 << 16) | OP_W | OP_D,                    // 1B
  OP_DATA_I8 | (0x70 << 16) | OP_W,                         // 1C
  OP_DATA_PRE66 | (0x70 << 16) | OP_W,                      // 1D
  (0xD8 << 16) | OP_SREG2,                                  // 1E
  (0xD9 << 16) | OP_SREG2,                                  // 1F
  OP_MODRM | (0x63 << 16) | OP_W | OP_D,                    // 20
  OP_MODRM | (0x63 << 16) | OP_W | OP_D,                    // 21
  OP_MODRM | (0x63 << 16) | OP_W | OP_D,                    // 22
  OP_MODRM | (0x63 << 16) | OP_W | OP_D,                    // 23
  OP_DATA_I8 | (0x63 << 16) | OP_W,                         // 24
  OP_DATA_PRE66 | (0x63 << 16) | OP_W,                      // 25
  OP_NONE,                                                  // 26
  (0x10 << 16),                                             // 27
  OP_MODRM | (0x71 << 16) | OP_W | OP_D,                    // 28
  OP_MODRM | (0x71 << 16) | OP_W | OP_D,                    // 29
  OP_MODRM | (0x71 << 16) | OP_W | OP_D,                    // 2A
  OP_MODRM | (0x71 << 16) | OP_W | OP_D,                    // 2B
  OP_DATA_I8 | (0x71 << 16) | OP_W,                         // 2C
  OP_DATA_PRE66 | (0x71 << 16) | OP_W,                      // 2D
  OP_NONE,                                                  // 2E
  (0x11 << 16),                                             // 2F
  OP_MODRM | (0x72 << 16) | OP_W | OP_D,                    // 30
  OP_MODRM | (0x72 << 16) | OP_W | OP_D,                    // 31
  OP_MODRM | (0x72 << 16) | OP_W | OP_D,                    // 32
  OP_MODRM | (0x72 << 16) | OP_W | OP_D,                    // 33
  OP_DATA_I8 | (0x72 << 16) | OP_W,                         // 34
  OP_DATA_PRE66 | (0x72 << 16) | OP_W,                      // 35
  OP_NONE,                                                  // 36
  (0x00 << 16) | OP_D,                                      // 37
  OP_MODRM | (0x6C << 16) | OP_W | OP_D,                    // 38
  OP_MODRM | (0x6C << 16) | OP_W | OP_D,                    // 39
  OP_MODRM | (0x6C << 16) | OP_W | OP_D,                    // 3A
  OP_MODRM | (0x6C << 16) | OP_W | OP_D,                    // 3B
  OP_DATA_I8 | (0x6C << 16) | OP_W,                         // 3C
  OP_DATA_PRE66 | (0x6C << 16) | OP_W,                      // 3D
  OP_NONE,                                                  // 3E
  (0x03 << 16),                                             // 3F
  (0x9C << 16) | OP_RD,                                     // 40
  (0x9C << 16) | OP_RD,                                     // 41
  (0x9C << 16) | OP_RD,                                     // 42
  (0x9C << 16) | OP_RD,                                     // 43
  (0x9C << 16) | OP_RD,                                     // 44
  (0x9C << 16) | OP_RD,                                     // 45
  (0x9C << 16) | OP_RD,                                     // 46
  (0x9C << 16) | OP_RD,                                     // 47
  (0x9B << 16) | OP_RD,                                     // 48
  (0x9B << 16) | OP_RD,                                     // 49
  (0x9B << 16) | OP_RD,                                     // 4A
  (0x9B << 16) | OP_RD,                                     // 4B
  (0x9B << 16) | OP_RD,                                     // 4C
  (0x9B << 16) | OP_RD,                                     // 4D
  (0x9B << 16) | OP_RD,                                     // 4E
  (0x9B << 16) | OP_RD,                                     // 4F
  (0xD8 << 16) | OP_RD,                                     // 50
  (0xD8 << 16) | OP_RD,                                     // 51
  (0xD8 << 16) | OP_RD,                                     // 52
  (0xD8 << 16) | OP_RD,                                     // 53
  (0xD8 << 16) | OP_RD,                                     // 54
  (0xD8 << 16) | OP_RD,                                     // 55
  (0xD8 << 16) | OP_RD,                                     // 56
  (0xD8 << 16) | OP_RD,                                     // 57
  (0xD9 << 16) | OP_RD,                                     // 58
  (0xD9 << 16) | OP_RD,                                     // 59
  (0xD9 << 16) | OP_RD,                                     // 5A
  (0xD9 << 16) | OP_RD,                                     // 5B
  (0xD9 << 16) | OP_RD,                                     // 5C
  (0xD9 << 16) | OP_RD,                                     // 5D
  (0xD9 << 16) | OP_RD,                                     // 5E
  (0xD9 << 16) | OP_RD,                                     // 5F
  (0x49 << 16) | OP_MNEMDIFF,                               // 60
  (0x45 << 16) | OP_MNEMDIFF,                               // 61
  OP_MODRM | (0x65 << 16),                                  // 62
  OP_MODRM | (0x64 << 16),                                  // 63
  OP_NONE,                                                  // 64
  OP_NONE,                                                  // 65
  OP_NONE,                                                  // 66
  OP_NONE,                                                  // 67
  OP_DATA_PRE66 | (0xD8 << 16) | OP_S,                      // 68
  OP_MODRM | OP_DATA_PRE66 | (0x88 << 16) | OP_THREE,       // 69
  OP_DATA_I8 | (0xD8 << 16) | OP_S,                         // 6A
  OP_MODRM | OP_DATA_I8 | (0x88 << 16) | OP_THREE,          // 6B
  (0x31 << 16) | OP_W,                                      // 6C
  (0x33 << 16) | OP_W | OP_MNEMDIFF,                        // 6D
  (0x41 << 16) | OP_W,                                      // 6E
  (0x43 << 16) | OP_W | OP_MNEMDIFF,                        // 6F
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 70
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 71
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 72
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 73
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 74
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 75
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 76
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 77
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 78
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 79
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 7A
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 7B
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 7C
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 7D
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 7E
  OP_DATA_I8 | OP_TTTn | (0xD6 << 16),                      // 7F
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_W | OP_S,      // 80
  OP_MODRM | OP_DATA_PRE66 | OP_MODRM_USED | OP_W | OP_S,   // 81
  OP_MODRM | OP_DATA_I8 | (0x63 << 16) | OP_W | OP_S,       // 82
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_W | OP_S,      // 83
  OP_MODRM | (0x8A << 16) | OP_W,                           // 84
  OP_MODRM | (0x8A << 16) | OP_W,                           // 85
  OP_MODRM | (0x95 << 16) | OP_W,                           // 86
  OP_MODRM | (0x95 << 16) | OP_W,                           // 87
  OP_MODRM | (0x96 << 16) | OP_W | OP_D,                    // 88
  OP_MODRM | (0x96 << 16) | OP_W | OP_D,                    // 89
  OP_MODRM | (0x96 << 16) | OP_W | OP_D,                    // 8A
  OP_MODRM | (0x96 << 16) | OP_W | OP_D,                    // 8B
  OP_MODRM | (0x96 << 16) | OP_SREG3 | OP_D,                // 8C
  OP_MODRM | (0x74 << 16),                                  // 8D
  OP_MODRM | (0x96 << 16) | OP_SREG3 | OP_D,                // 8E
  OP_MODRM | OP_MODRM_USED | OP_ONE,                        // 8F
  (0x40 << 16) | OP_RD,                                     // 90
  (0x95 << 16) | OP_RD,                                     // 91
  (0x95 << 16) | OP_RD,                                     // 92
  (0x95 << 16) | OP_RD,                                     // 93
  (0x95 << 16) | OP_RD,                                     // 94
  (0x95 << 16) | OP_RD,                                     // 95
  (0x95 << 16) | OP_RD,                                     // 96
  (0x95 << 16) | OP_RD,                                     // 97
  (0x04 << 16) | OP_MNEMDIFF,                               // 98
  (0x0F << 16) | OP_MNEMDIFF,                               // 99
  OP_DATA_I32 | OP_DATA_I16 | (0x9A << 16),                 // 9A
  (0x58 << 16),                                             // 9B
  (0x4B << 16),                                             // 9C
  (0x47 << 16),                                             // 9D
  (0x4E << 16),                                             // 9E
  (0x38 << 16),                                             // 9F
  OP_DATA_PRE67 | (0x96 << 16) | OP_W | OP_D,               // A0
  OP_DATA_PRE67 | (0x96 << 16) | OP_W | OP_D,               // A1
  OP_DATA_PRE67 | (0x96 << 16) | OP_W | OP_D,               // A2
  OP_DATA_PRE67 | (0x96 << 16) | OP_W | OP_D,               // A3
  (0x3D << 16) | OP_W,                                      // A4
  (0x3F << 16) | OP_MNEMDIFF | OP_W,                        // A5
  (0x0B << 16) | OP_W,                                      // A6
  (0x0D << 16) | OP_MNEMDIFF | OP_W,                        // A7
  OP_DATA_I8 | (0x8A << 16) | OP_W,                         // A8
  OP_DATA_PRE66 | (0x8A << 16) | OP_W,                      // A9
  (0x55 << 16) | OP_W,                                      // AA
  (0x57 << 16) | OP_MNEMDIFF | OP_W,                        // AB
  (0x3A << 16) | OP_W,                                      // AC
  (0x3C << 16) | OP_MNEMDIFF | OP_W,                        // AD
  (0x4F << 16) | OP_W,                                      // AE
  (0x51 << 16) | OP_MNEMDIFF | OP_W,                        // AF
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B0
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B1
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B2
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B3
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B4
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B5
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B6
  OP_DATA_I8 | (0x96 << 16) | OP_RD,                        // B7
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // B8
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // B9
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // BA
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // BB
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // BC
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // BD
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // BE
  OP_DATA_PRE66 | (0x96 << 16) | OP_RD,                     // BF
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_W,             // C0
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_W,             // C1
  OP_DATA_I16 | (0xBA << 16),                               // C2
  (0x4C << 16),                                             // C3
  OP_MODRM | (0x84 << 16),                                  // C4
  OP_MODRM | (0x83 << 16),                                  // C5
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_W,             // C6
  OP_MODRM | OP_DATA_PRE66 | OP_MODRM_USED | OP_W,          // C7
  OP_DATA_I8 | OP_DATA_I16 | (0x6E << 16),                  // C8
  (0x39 << 16),                                             // C9
  OP_DATA_I16 | (0x4D << 16),                               // CA
  (0x4D << 16),                                             // CB
  (0xC2 << 16),                                             // CC
  OP_DATA_I8 | (0xC1 << 16),                                // CD
  (0x34 << 16),                                             // CE
  (0x37 << 16) | OP_MNEMDIFF,                               // CF
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // D0
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // D1
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // D2
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // D3
  OP_DATA_I8 | (0x02 << 16),                                // D4
  OP_DATA_I8 | (0x01 << 16),                                // D5
  (0xB7 << 16),                                             // D6
  (0x5A << 16),                                             // D7
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // D8
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // D9
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // DA
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // DB
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // DC
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // DD
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // DE
  OP_MODRM | OP_WORD | OP_MODRM_USED | OP_ONE,              // DF
  OP_DATA_I8 | (0xC5 << 16),                                // E0
  OP_DATA_I8 | (0xC4 << 16),                                // E1
  OP_DATA_I8 | (0xC3 << 16),                                // E2
  OP_DATA_I8 | (0xCA << 16),                                // E3
  OP_DATA_I8 | (0x75 << 16) | OP_W,                         // E4
  OP_DATA_I8 | (0x75 << 16) | OP_W,                         // E5
  OP_DATA_I8 | (0x76 << 16) | OP_W,                         // E6
  OP_DATA_I8 | (0x76 << 16) | OP_W,                         // E7
  OP_DATA_PRE66 | (0x9A << 16),                             // E8
  OP_DATA_PRE66 | (0xD5 << 16),                             // E9
  OP_DATA_I16 | OP_DATA_I32 | (0xD5 << 16),                 // EA
  OP_DATA_I8 | (0xD5 << 16),                                // EB
  (0x75 << 16) | OP_W,                                      // EC
  (0x75 << 16) | OP_W,                                      // ED
  (0x76 << 16) | OP_W,                                      // EE
  (0x76 << 16) | OP_W,                                      // EF
  OP_NONE,                                                  // F0
  (0xDE << 16),                                             // F1
  OP_NONE,                                                  // F2
  OP_NONE,                                                  // F3
  (0x30 << 16),                                             // F4
  (0x0A << 16),                                             // F5
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // F6
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // F7
  (0x06 << 16),                                             // F8
  (0x52 << 16),                                             // F9
  (0x08 << 16),                                             // FA
  (0x54 << 16),                                             // FB
  (0x07 << 16),                                             // FC
  (0x53 << 16),                                             // FD
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE,                 // FE
  OP_MODRM | OP_MODRM_USED | OP_W | OP_ONE                  // FF
};

ULONG OpcodeFlagsExt[0x100] = {
  OP_MODRM | OP_MODRM_USED | OP_ONE,                        // 00
  OP_MODRM | OP_MODRM_USED | OP_ONE,                        // 01
  OP_MODRM | (0x73 << 16),                                  // 02
  OP_MODRM | (0x89 << 16),                                  // 03
  OP_NONE,                                                  // 04
  OP_NONE,                                                  // 05
  (0x09 << 16),                                             // 06
  OP_NONE,                                                  // 07
  (0x35 << 16),                                             // 08
  (0x59 << 16),                                             // 09
  OP_NONE,                                                  // 0A
  (0x5F << 16),                                             // 0B
  OP_NONE,                                                  // 0C
  OP_NONE,                                                  // 0D
  OP_NONE,                                                  // 0E
  OP_NONE,                                                  // 0F
  OP_MODRM | (0x37 << 16) | OP_SSE 
    | OP_MNEMEXT | OP_SSE_PFX,                              // 10
  OP_MODRM | (0x37 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX | OP_REV,                         // 11
  OP_MODRM | (0x32 << 16) | OP_SSE | OP_MNEMEXT,            // 12
  OP_NONE,                                                  // 13
  OP_MODRM | (0x48 << 16) | OP_SSE | OP_MNEMEXT,            // 14
  OP_MODRM | (0x47 << 16) | OP_SSE | OP_MNEMEXT,            // 15
  OP_MODRM | (0x34 << 16) | OP_SSE | OP_MNEMEXT,            // 16
  OP_MODRM | (0x35 << 16) | OP_SSE | OP_MNEMEXT | OP_REV,   // 17
  OP_MODRM | OP_MODRM_USED | OP_ONE,                        // 18
  OP_NONE,                                                  // 19
  OP_NONE,                                                  // 1A
  OP_NONE,                                                  // 1B
  OP_NONE,                                                  // 1C
  OP_NONE,                                                  // 1D
  OP_NONE,                                                  // 1E
  OP_NONE,                                                  // 1F
  OP_MODRM | (0x96 << 16) | OP_EEE | OP_D,                  // 20
  OP_MODRM | (0x96 << 16) | OP_EEE | OP_D,                  // 21
  OP_MODRM | (0x96 << 16) | OP_EEE | OP_D,                  // 22
  OP_MODRM | (0x96 << 16) | OP_EEE | OP_D,                  // 23
  OP_MODRM | (0x96 << 16) | OP_EEE,                         // 24
  OP_NONE,                                                  // 25
  OP_MODRM | (0x96 << 16) | OP_EEE,                         // 26
  OP_NONE,                                                  // 27
  OP_MODRM | (0x31 << 16) | OP_SSE | OP_MNEMEXT,            // 28
  OP_MODRM | (0x31 << 16) | OP_SSE | OP_MNEMEXT | OP_REV,   // 29
  OP_MODRM | (0x24 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 2A
  OP_MODRM | (0x4B << 16) | OP_SSE | OP_MNEMEXT | OP_REV,   // 2B
  OP_MODRM | (0x28 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 2C
  OP_MODRM | (0x26 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 2D
  OP_MODRM | (0x46 << 16) | OP_SSE | OP_MNEMEXT,            // 2E
  OP_MODRM | (0x23 << 16) | OP_SSE | OP_MNEMEXT,            // 2F
  (0x60 << 16),                                             // 30
  (0x5E << 16),                                             // 31
  (0x5C << 16),                                             // 32
  (0x5D << 16),                                             // 33
  (0xDF << 16),                                             // 34
  (0xE0 << 16),                                             // 35
  OP_NONE,                                                  // 36
  OP_NONE,                                                  // 37
  OP_NONE,                                                  // 38
  OP_NONE,                                                  // 39
  OP_NONE,                                                  // 3A
  OP_NONE,                                                  // 3B
  OP_NONE,                                                  // 3C
  OP_NONE,                                                  // 3D
  OP_NONE,                                                  // 3E
  OP_NONE,                                                  // 3F
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 40
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 41
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 42
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 43
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 44
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 45
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 46
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 47
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 48
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 49
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 4A
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 4B
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 4C
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 4D
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 4E
  OP_MODRM | (0xE9 << 16) | OP_TTTn,                        // 4F
  OP_MODRM | (0x36 << 16) | OP_SSE | OP_MNEMEXT,            // 50
  OP_MODRM | (0x41 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 51
  OP_MODRM | (0x3E << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 52
  OP_MODRM | (0x3C << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 53
  OP_MODRM | (0x1F << 16) | OP_SSE | OP_MNEMEXT,            // 54
  OP_MODRM | (0x20 << 16) | OP_SSE | OP_MNEMEXT,            // 55
  OP_MODRM | (0x3B << 16) | OP_SSE | OP_MNEMEXT,            // 56
  OP_MODRM | (0x49 << 16) | OP_SSE | OP_MNEMEXT,            // 57
  OP_MODRM | (0x1D << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 58
  OP_MODRM | (0x39 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 59
  OP_NONE,                                                  // 5A
  OP_NONE,                                                  // 5B
  OP_MODRM | (0x44 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 5C
  OP_MODRM | (0x2F << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 5D
  OP_MODRM | (0x2A << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 5E
  OP_MODRM | (0x2D << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX,                                  // 5F
  OP_MODRM | (0x15 << 16) | OP_MMX | OP_MNEMEXT,            // 60
  OP_MODRM | (0x16 << 16) | OP_MMX | OP_MNEMEXT,            // 61
  OP_MODRM | (0x17 << 16) | OP_MMX | OP_MNEMEXT,            // 62
  OP_MODRM | (0xED << 16) | OP_MMX,                         // 63
  OP_MODRM | (0xFC << 16) | OP_MMX,                         // 64
  OP_MODRM | (0xFD << 16) | OP_MMX,                         // 65
  OP_MODRM | (0xFE << 16) | OP_MMX,                         // 66
  OP_MODRM | (0xEF << 16) | OP_MMX,                         // 67
  OP_MODRM | (0x12 << 16) | OP_MMX | OP_MNEMEXT,            // 68
  OP_MODRM | (0x13 << 16) | OP_MMX | OP_MNEMEXT,            // 69
  OP_MODRM | (0x14 << 16) | OP_MMX | OP_MNEMEXT,            // 6A
  OP_MODRM | (0xEE << 16) | OP_MMX,                         // 6B
  OP_NONE,                                                  // 6C
  OP_NONE,                                                  // 6D
  OP_MODRM | (0x19 << 16) | OP_MMX | OP_MNEMEXT,            // 6E
  OP_MODRM | (0xEC << 16) | OP_MMX,                         // 6F
  OP_MODRM | OP_DATA_I8 | (0x58 << 16) |
  OP_MMX | OP_MNEMEXT | OP_THREE,                           // 70
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_MMX,           // 71
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_MMX,           // 72
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED | OP_MMX,           // 73
  OP_MODRM | (0xF9 << 16) | OP_MMX,                         // 74
  OP_MODRM | (0xFA << 16) | OP_MMX,                         // 75
  OP_MODRM | (0xFB << 16) | OP_MMX,                         // 76
  (0xEA << 16),                                             // 77
  OP_NONE,                                                  // 78
  OP_NONE,                                                  // 79
  OP_NONE,                                                  // 7A
  OP_NONE,                                                  // 7B
  OP_NONE,                                                  // 7C
  OP_NONE,                                                  // 7D
  OP_MODRM | (0x19 << 16) | OP_MMX | OP_MNEMEXT | OP_REV,   // 7E
  OP_MODRM | (0xEC << 16) | OP_MMX | OP_REV,                // 7F
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 80
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 81
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 82
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 83
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 84
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 85
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 86
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 87
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 88
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 89
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 8A
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 8B
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 8C
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 8D
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 8E
  OP_DATA_PRE66 | OP_TTTn | (0xD6 << 16),                   // 8F
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 90
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 91
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 92
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 93
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 94
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 95
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 96
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 97
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 98
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 99
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 9A
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 9B
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 9C
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 9D
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 9E
  OP_MODRM | OP_TTTn | (0xD7 << 16) | OP_ONE | OP_BYTE,     // 9F
  (0xD8 << 16) | OP_SREG3,                                  // A0
  (0xD9 << 16) | OP_SREG3,                                  // A1
  (0x5B << 16),                                             // A2
  OP_MODRM | (0x68 << 16) | OP_REV,                         // A3
  OP_MODRM | OP_DATA_I8 | (0x97 << 16) | OP_THREE | OP_REV, // A4
  OP_MODRM | (0x97 << 16) | OP_REV,                         // A5
  OP_NONE,                                                  // A6
  OP_NONE,                                                  // A7
  (0xD8 << 16) | OP_SREG3,                                  // A8
  (0xD9 << 16) | OP_SREG3,                                  // A9
  (0xEB << 16),                                             // AA
  OP_MODRM | (0x6B << 16) | OP_REV,                         // AB
  OP_MODRM | OP_DATA_I8 | (0x98 << 16) | OP_THREE | OP_REV, // AC
  OP_MODRM | (0x98 << 16) | OP_REV,                         // AD
  OP_MODRM | OP_MODRM_USED | OP_ONE,                        // AE
  OP_MODRM | (0x88 << 16),                                  // AF
  OP_MODRM | (0x6D << 16) | OP_W | OP_REV,                  // B0
  OP_MODRM | (0x6D << 16) | OP_W | OP_REV,                  // B1
  OP_MODRM | (0x85 << 16),                                  // B2
  OP_MODRM | (0x6A << 16) | OP_REV,                         // B3
  OP_MODRM | (0x87 << 16),                                  // B4
  OP_MODRM | (0x86 << 16),                                  // B5
  OP_MODRM | (0x94 << 16),                                  // B6
  OP_MODRM | (0x94 << 16),                                  // B7
  OP_NONE,                                                  // B8
  OP_NONE,                                                  // B9
  OP_MODRM | OP_DATA_I8 | OP_MODRM_USED,                    // BA
  OP_MODRM | (0x69 << 16) | OP_REV,                         // BB
  OP_MODRM | (0x66 << 16),                                  // BC
  OP_MODRM | (0x67 << 16),                                  // BD
  OP_MODRM | (0x93 << 16),                                  // BE
  OP_MODRM | (0x93 << 16),                                  // BF
  OP_MODRM | (0x92 << 16) | OP_W | OP_REV,                  // C0
  OP_MODRM | (0x92 << 16) | OP_W | OP_REV,                  // C1
  OP_MODRM | (0x21 << 16) | OP_SSE |
  OP_MNEMEXT | OP_SSE_PFX | OP_DATA_I8 | OP_THREE,          // C2
  OP_NONE,                                                  // C3
  OP_MODRM | OP_DATA_I8 | (0x50 << 16) |
  OP_MMX | OP_MNEMEXT | OP_THREE,                           // C4
  OP_MODRM | OP_DATA_I8 | (0x4F << 16) |
  OP_MMX | OP_MNEMEXT | OP_THREE,                           // C5
  OP_MODRM | (0x40 << 16) | OP_SSE |
  OP_MNEMEXT | OP_DATA_I8 | OP_THREE,                       // C6
  OP_MODRM | OP_MODRM_USED | OP_MMX | OP_ONE,               // C7
  (0x99 << 16) | OP_RD,                                     // C8
  (0x99 << 16) | OP_RD,                                     // C9
  (0x99 << 16) | OP_RD,                                     // CA
  (0x99 << 16) | OP_RD,                                     // CB
  (0x99 << 16) | OP_RD,                                     // CC
  (0x99 << 16) | OP_RD,                                     // CD
  (0x99 << 16) | OP_RD,                                     // CE
  (0x99 << 16) | OP_RD,                                     // CF
  OP_NONE,                                                  // D0
  OP_MODRM | (0x08 << 16) | OP_MMX | OP_MNEMEXT,            // D1
  OP_MODRM | (0x09 << 16) | OP_MMX | OP_MNEMEXT,            // D2
  OP_MODRM | (0x0A << 16) | OP_MMX | OP_MNEMEXT,            // D3
  OP_NONE,                                                  // D4
  OP_MODRM | (0x01 << 16) | OP_MMX | OP_MNEMEXT,            // D5
  OP_NONE,                                                  // D6
  OP_MODRM | (0x55 << 16) | OP_MMX | OP_MNEMEXT,            // D7
  OP_MODRM | (0x10 << 16) | OP_MMX | OP_MNEMEXT,            // D8
  OP_MODRM | (0x11 << 16) | OP_MMX | OP_MNEMEXT,            // D9
  OP_MODRM | (0x54 << 16) | OP_MMX | OP_MNEMEXT,            // DA
  OP_MODRM | (0xF7 << 16) | OP_MMX,                         // DB
  OP_MODRM | (0xF5 << 16) | OP_MMX,                         // DC
  OP_MODRM | (0xF6 << 16) | OP_MMX,                         // DD
  OP_MODRM | (0x52 << 16) | OP_MMX | OP_MNEMEXT,            // DE
  OP_MODRM | (0xF8 << 16) | OP_MMX,                         // DF
  OP_MODRM | (0x4D << 16) | OP_MMX | OP_MNEMEXT,            // E0
  OP_MODRM | (0x06 << 16) | OP_MMX | OP_MNEMEXT,            // E1
  OP_MODRM | (0x07 << 16) | OP_MMX | OP_MNEMEXT,            // E2
  OP_MODRM | (0x4E << 16) | OP_MMX | OP_MNEMEXT,            // E3
  OP_MODRM | (0x56 << 16) | OP_MMX | OP_MNEMEXT,            // E4
  OP_MODRM | (0x00 << 16) | OP_MMX | OP_MNEMEXT,            // E5
  OP_NONE,                                                  // E6
  OP_MODRM | (0x4C << 16) | OP_MMX | OP_MNEMEXT | OP_REV,   // E7
  OP_MODRM | (0x0E << 16) | OP_MMX | OP_MNEMEXT,            // E8
  OP_MODRM | (0x0F << 16) | OP_MMX | OP_MNEMEXT,            // E9
  OP_MODRM | (0x53 << 16) | OP_MMX | OP_MNEMEXT,            // EA
  OP_MODRM | (0x02 << 16) | OP_MMX | OP_MNEMEXT,            // EB
  OP_MODRM | (0xF3 << 16) | OP_MMX,                         // EC
  OP_MODRM | (0xF4 << 16) | OP_MMX,                         // ED
  OP_MODRM | (0x51 << 16) | OP_MMX | OP_MNEMEXT,            // EE
  OP_MODRM | (0x18 << 16) | OP_MMX | OP_MNEMEXT,            // EF
  OP_NONE,                                                  // F0
  OP_MODRM | (0x03 << 16) | OP_MMX | OP_MNEMEXT,            // F1
  OP_MODRM | (0x04 << 16) | OP_MMX | OP_MNEMEXT,            // F2
  OP_MODRM | (0x05 << 16) | OP_MMX | OP_MNEMEXT,            // F3
  OP_NONE,                                                  // F4
  OP_MODRM | (0xFF << 16) | OP_MMX,                         // F5
  OP_MODRM | (0x57 << 16) | OP_MMX | OP_MNEMEXT,            // F6
  OP_MODRM | (0x4A << 16) | OP_MMX | OP_MNEMEXT,            // F7
  OP_MODRM | (0x0B << 16) | OP_MMX | OP_MNEMEXT,            // F8
  OP_MODRM | (0x0C << 16) | OP_MMX | OP_MNEMEXT,            // F9
  OP_MODRM | (0x0D << 16) | OP_MMX | OP_MNEMEXT,            // FA
  OP_NONE,                                                  // FB
  OP_MODRM | (0xF0 << 16) | OP_MMX,                         // FC
  OP_MODRM | (0xF1 << 16) | OP_MMX,                         // FD
  OP_MODRM | (0xF2 << 16) | OP_MMX,                         // FE
  OP_NONE                                                   // FF
};

 /*
   Modrm Used instructions Extended Flags:
   Bits 8..15 - opcode, 16..18 - REG, 0..7 - flags
 */
ULONG ModrmUsedExtendedFlags[2] = {
  (0xF6 << 8) | (0x0 << 18) | OP_DATA_I8,
  (0xF7 << 8) | (0x0 << 18) | OP_DATA_I32
};

/*
   Bits 0..7 - mnemonic ordinal, 8..15 - opcode.
*/
USHORT MnemDiffTable[12] = {
   (0x60 << 8) | 0x48,
   (0x61 << 8) | 0x44,
   (0x98 << 8) | 0x05,
   (0x99 << 8) | 0x0E,
   (0xA5 << 8) | 0x3E,
   (0xA7 << 8) | 0x0C,
   (0xAB << 8) | 0x56,
   (0xAD << 8) | 0x3B,
   (0xAF << 8) | 0x50,
   (0xCF << 8) | 0x36,
   (0x6F << 8) | 0x42,
   (0x6D << 8) | 0x32
};


/*
 word opcodes table format:
 Bits 0..7 - Opcode two byte
 Bits 8..15 - Opcode ordinal (or_con = idx << 8)
*/
USHORT OpcodeWordD9[0x2C] = {
  0xF0 | (0x12 << 8),
  0xE1 | (0x13 << 8),
  0xE0 | (0x14 << 8),
  0xFF | (0x16 << 8),
  0xF6 | (0x17 << 8),
  0xF7 | (0x18 << 8),
  0xE8 | (0x1A << 8),
  0xE9 | (0x1B << 8),
  0xEA | (0x1C << 8),
  0xEB | (0x1D << 8),
  0xEC | (0x1E << 8),
  0xED | (0x1F << 8),
  0xEE | (0x20 << 8),
  0xD0 | (0x21 << 8),
  0xF3 | (0x22 << 8),
  0xF8 | (0x23 << 8),
  0xF5 | (0x24 << 8),
  0xF2 | (0x25 << 8),
  0xFC | (0x26 << 8),
  0xFD | (0x27 << 8),
  0xFE | (0x28 << 8),
  0xFB | (0x29 << 8),
  0xFA | (0x2A << 8),
  0xE4 | (0x2B << 8),
  0xE5 | (0x2C << 8),
  0xF4 | (0x2D << 8),
  0xF1 | (0x2E << 8),
  0xF9 | (0x2F << 8),
  0xC8 | (0xAD << 8),
  0xC9 | (0xAD << 8),
  0xCA | (0xAD << 8),
  0xCB | (0xAD << 8),
  0xCC | (0xAD << 8),
  0xCD | (0xAD << 8),
  0xCE | (0xAD << 8),
  0xCF | (0xAD << 8),
  0xC0 | (0xB1 << 8),
  0xC1 | (0xB1 << 8),
  0xC2 | (0xB1 << 8),
  0xC3 | (0xB1 << 8),
  0xC4 | (0xB1 << 8),
  0xC5 | (0xB1 << 8),
  0xC6 | (0xB1 << 8),
  0xC7 | (0xB1 << 8)
};

USHORT OpcodeWordDB[34] = {
  0xE2 | (0x15 << 8),
  0xE3 | (0x19 << 8),
  0xC0 | (0xE5 << 8),
  0xC1 | (0xE5 << 8),
  0xC2 | (0xE5 << 8),
  0xC3 | (0xE5 << 8),
  0xC4 | (0xE5 << 8),
  0xC5 | (0xE5 << 8),
  0xC6 | (0xE5 << 8),
  0xC7 | (0xE5 << 8),
  0xC8 | (0xE6 << 8),
  0xC9 | (0xE6 << 8),
  0xCA | (0xE6 << 8),
  0xCB | (0xE6 << 8),
  0xCC | (0xE6 << 8),
  0xCD | (0xE6 << 8),
  0xCE | (0xE6 << 8),
  0xCF | (0xE6 << 8),
  0xD0 | (0xE7 << 8),
  0xD1 | (0xE7 << 8),
  0xD2 | (0xE7 << 8),
  0xD3 | (0xE7 << 8),
  0xD4 | (0xE7 << 8),
  0xD5 | (0xE7 << 8),
  0xD6 | (0xE7 << 8),
  0xD7 | (0xE7 << 8),
  0xD8 | (0xE8 << 8),
  0xD9 | (0xE8 << 8),
  0xDA | (0xE8 << 8),
  0xDB | (0xE8 << 8),
  0xDC | (0xE8 << 8),
  0xDD | (0xE8 << 8),
  0xDE | (0xE8 << 8),
  0xDF | (0xE8 << 8)
};

USHORT OpcodeWordDE[0x31] = {
  0xC0 | (0x78 << 8),
  0xC1 | (0x78 << 8),
  0xC2 | (0x78 << 8),
  0xC3 | (0x78 << 8),
  0xC4 | (0x78 << 8),
  0xC5 | (0x78 << 8),
  0xC6 | (0x78 << 8),
  0xC7 | (0x78 << 8),
  0xC8 | (0x7A << 8),
  0xC9 | (0x7A << 8),
  0xCA | (0x7A << 8),
  0xCB | (0x7A << 8),
  0xCC | (0x7A << 8),
  0xCD | (0x7A << 8),
  0xCE | (0x7A << 8),
  0xCF | (0x7A << 8),
  0xF8 | (0x7C << 8),
  0xF9 | (0x7C << 8),
  0xFA | (0x7C << 8),
  0xFB | (0x7C << 8),
  0xFC | (0x7C << 8),
  0xFD | (0x7C << 8),
  0xFE | (0x7C << 8),
  0xFF | (0x7C << 8),
  0xF0 | (0x7E << 8),
  0xF1 | (0x7E << 8),
  0xF2 | (0x7E << 8),
  0xF3 | (0x7E << 8),
  0xF4 | (0x7E << 8),
  0xF5 | (0x7E << 8),
  0xF6 | (0x7E << 8),
  0xF7 | (0x7E << 8),
  0xE8 | (0x80 << 8),
  0xE9 | (0x80 << 8),
  0xEA | (0x80 << 8),
  0xEB | (0x80 << 8),
  0xEC | (0x80 << 8),
  0xED | (0x80 << 8),
  0xEE | (0x80 << 8),
  0xEF | (0x80 << 8),
  0xE0 | (0x82 << 8),
  0xE1 | (0x82 << 8),
  0xE2 | (0x82 << 8),
  0xE3 | (0x82 << 8),
  0xE4 | (0x82 << 8),
  0xE5 | (0x82 << 8),
  0xE6 | (0x82 << 8),
  0xE7 | (0x82 << 8),
  0xD9 | (0xA4 << 8)
};

USHORT OpcodeWordDD[40] = {
  0xE0 | (0xA5 << 8),
  0xE1 | (0xA5 << 8),
  0xE2 | (0xA5 << 8),
  0xE3 | (0xA5 << 8),
  0xE4 | (0xA5 << 8),
  0xE5 | (0xA5 << 8),
  0xE6 | (0xA5 << 8),
  0xE7 | (0xA5 << 8),
  0xE8 | (0xA6 << 8),
  0xE9 | (0xA6 << 8),
  0xEA | (0xA6 << 8),
  0xEB | (0xA6 << 8),
  0xEC | (0xA6 << 8),
  0xED | (0xA6 << 8),
  0xEE | (0xA6 << 8),
  0xEF | (0xA6 << 8),
  0xC0 | (0xAC << 8),
  0xC1 | (0xAC << 8),
  0xC2 | (0xAC << 8),
  0xC3 | (0xAC << 8),
  0xC4 | (0xAC << 8),
  0xC5 | (0xAC << 8),
  0xC6 | (0xAC << 8),
  0xC7 | (0xAC << 8),
  0xD0 | (0xB2 << 8),
  0xD1 | (0xB2 << 8),
  0xD2 | (0xB2 << 8),
  0xD3 | (0xB2 << 8),
  0xD4 | (0xB2 << 8),
  0xD5 | (0xB2 << 8),
  0xD6 | (0xB2 << 8),
  0xD7 | (0xB2 << 8),
  0xD8 | (0xB3 << 8),
  0xD9 | (0xB3 << 8),
  0xDA | (0xB3 << 8),
  0xDB | (0xB3 << 8),
  0xDC | (0xB3 << 8),
  0xDD | (0xB3 << 8),
  0xDE | (0xB3 << 8),
  0xDF | (0xB3 << 8)
};

USHORT OpcodeWordDA[33] = {
  0xE9 | (0xA7 << 8),
  0xC0 | (0xE1 << 8),
  0xC1 | (0xE1 << 8),
  0xC2 | (0xE1 << 8),
  0xC3 | (0xE1 << 8),
  0xC4 | (0xE1 << 8),
  0xC5 | (0xE1 << 8),
  0xC6 | (0xE1 << 8),
  0xC7 | (0xE1 << 8),
  0xC8 | (0xE2 << 8),
  0xC9 | (0xE2 << 8),
  0xCA | (0xE2 << 8),
  0xCB | (0xE2 << 8),
  0xCC | (0xE2 << 8),
  0xCD | (0xE2 << 8),
  0xCE | (0xE2 << 8),
  0xCF | (0xE2 << 8),
  0xD0 | (0xE3 << 8),
  0xD1 | (0xE3 << 8),
  0xD2 | (0xE3 << 8),
  0xD3 | (0xE3 << 8),
  0xD4 | (0xE3 << 8),
  0xD5 | (0xE3 << 8),
  0xD6 | (0xE3 << 8),
  0xD7 | (0xE3 << 8),
  0xD8 | (0xE4 << 8),
  0xD9 | (0xE4 << 8),
  0xDA | (0xE4 << 8),
  0xDB | (0xE4 << 8),
  0xDC | (0xE4 << 8),
  0xDD | (0xE4 << 8),
  0xDE | (0xE4 << 8),
  0xDF | (0xE4 << 8)
};

USHORT OpcodeWordD8[64] = {
  0xD0 | (0xA2 << 8),
  0xD1 | (0xA2 << 8),
  0xD2 | (0xA2 << 8),
  0xD3 | (0xA2 << 8),
  0xD4 | (0xA2 << 8),
  0xD5 | (0xA2 << 8),
  0xD6 | (0xA2 << 8),
  0xD7 | (0xA2 << 8),
  0xD8 | (0xA3 << 8),
  0xD9 | (0xA3 << 8),
  0xDA | (0xA3 << 8),
  0xDB | (0xA3 << 8),
  0xDC | (0xA3 << 8),
  0xDD | (0xA3 << 8),
  0xDE | (0xA3 << 8),
  0xDF | (0xA3 << 8),
  0xF0 | (0x7B << 8),
  0xF1 | (0x7B << 8),
  0xF2 | (0x7B << 8),
  0xF3 | (0x7B << 8),
  0xF4 | (0x7B << 8),
  0xF5 | (0x7B << 8),
  0xF6 | (0x7B << 8),
  0xF7 | (0x7B << 8),
  0xF8 | (0x7D << 8),
  0xF9 | (0x7D << 8),
  0xFA | (0x7D << 8),
  0xFB | (0x7D << 8),
  0xFC | (0x7D << 8),
  0xFD | (0x7D << 8),
  0xFE | (0x7D << 8),
  0xFF | (0x7D << 8),
  0xE0 | (0x7F << 8),
  0xE1 | (0x7F << 8),
  0xE2 | (0x7F << 8),
  0xE3 | (0x7F << 8),
  0xE4 | (0x7F << 8),
  0xE5 | (0x7F << 8),
  0xE6 | (0x7F << 8),
  0xE7 | (0x7F << 8),
  0xE8 | (0x81 << 8),
  0xE9 | (0x81 << 8),
  0xEA | (0x81 << 8),
  0xEB | (0x81 << 8),
  0xEC | (0x81 << 8),
  0xED | (0x81 << 8),
  0xEE | (0x81 << 8),
  0xEF | (0x81 << 8),
  0xC0 | (0x77 << 8),
  0xC1 | (0x77 << 8),
  0xC2 | (0x77 << 8),
  0xC3 | (0x77 << 8),
  0xC4 | (0x77 << 8),
  0xC5 | (0x77 << 8),
  0xC6 | (0x77 << 8),
  0xC7 | (0x77 << 8),
  0xC8 | (0x79 << 8),
  0xC9 | (0x79 << 8),
  0xCA | (0x79 << 8),
  0xCB | (0x79 << 8),
  0xCC | (0x79 << 8),
  0xCD | (0x79 << 8),
  0xCE | (0x79 << 8),
  0xCF | (0x79 << 8)
};

USHORT OpcodeWordDC[63] = {
  0xC0 | (0x77 << 8),
  0xC1 | (0x77 << 8),
  0xC2 | (0x77 << 8),
  0xC3 | (0x77 << 8),
  0xC4 | (0x77 << 8),
  0xC5 | (0x77 << 8),
  0xC6 | (0x77 << 8),
  0xC7 | (0x77 << 8),
  0xF8 | (0x7B << 8),
  0xF9 | (0x7B << 8),
  0xFA | (0x7B << 8),
  0xFB | (0x7B << 8),
  0xFC | (0x7B << 8),
  0xFD | (0x7B << 8),
  0xFE | (0x7B << 8),
  0xFF | (0x7B << 8),
  0xF0 | (0x7D << 8),
  0xF1 | (0x7D << 8),
  0xF2 | (0x7D << 8),
  0xF3 | (0x7D << 8),
  0xF4 | (0x7D << 8),
  0xF5 | (0x7D << 8),
  0xF6 | (0x7D << 8),
  0xF7 | (0x7D << 8),
  0xC8 | (0x79 << 8),
  0xC9 | (0x79 << 8),
  0xCA | (0x79 << 8),
  0xCB | (0x79 << 8),
  0xCC | (0x79 << 8),
  0xCD | (0x79 << 8),
  0xCE | (0x79 << 8),
  0xCF | (0x79 << 8),
  0xE8 | (0x7F << 8),
  0xE9 | (0x7F << 8),
  0xEA | (0x7F << 8),
  0xEB | (0x7F << 8),
  0xEC | (0x7F << 8),
  0xED | (0x7F << 8),
  0xEF | (0x7F << 8),
  0xE0 | (0x81 << 8),
  0xE1 | (0x81 << 8),
  0xE2 | (0x81 << 8),
  0xE3 | (0x81 << 8),
  0xE4 | (0x81 << 8),
  0xE5 | (0x81 << 8),
  0xE6 | (0x81 << 8),
  0xE7 | (0x81 << 8),
  0xD0 | (0xA2 << 8),
  0xD1 | (0xA2 << 8),
  0xD2 | (0xA2 << 8),
  0xD3 | (0xA2 << 8),
  0xD4 | (0xA2 << 8),
  0xD5 | (0xA2 << 8),
  0xD6 | (0xA2 << 8),
  0xD7 | (0xA2 << 8),
  0xD8 | (0xA3 << 8),
  0xD9 | (0xA3 << 8),
  0xDA | (0xA3 << 8),
  0xDB | (0xA3 << 8),
  0xDC | (0xA3 << 8),
  0xDD | (0xA3 << 8),
  0xDE | (0xA3 << 8),
  0xDF | (0xA3 << 8)
};

UCHAR ModrmUsedMnems1[8] = {
   0x62, // add
   0x6F, // or
   0x61, // adc
   0x70, // sbb
   0x63, // and
   0x71, // sub
   0x72, // xor
   0x6C  // cmp
};

UCHAR ModrmUsedMnems2[8] = {
   0x77, // fadd
   0x79, // fmul
   0xA2, // fcom
   0xA3, // fcomp
   0x7F, // fsub
   0x81, // fsubr
   0x7B, // fdiv
   0x7D  // fdivr  
};

UCHAR ModrmUsedMnems3[8] = {
   0x9F, // fiadd
   0xBD, // fimul
   0xA8, // ficom
   0xA9, // ficomp
   0xBB, // fisub
   0xBC, // fisubr
   0xAA, // fidiv
   0xAB  // fidivr
};

UCHAR ModrmUsedMnems4[8] = {
   0x8A, // test
   0xDA, // invalid
   0xC0, // not
   0xBF, // neg
   0xBE, // mul
   0x88, // imul
   0x9D, // div
   0x9E  // idiv
};

UCHAR ModrmUsedMnems5[8] = {
   0x8D, // rol
   0x8E, // r|
   0x8B, // rcl
   0x8C, // rcr
   0x90, // <<
   0x91, // shr
   0xDA, // invalid
   0x8F  // sar
};

UCHAR ModrmUsedMnems6[8] = {
   0x96, // mov
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA  // invalid  
};

UCHAR ModrmUsedMnems7[8] = {
   0x9C, // inc
   0x9B, // dec
   0x9A, // call
   0x9A, // call
   0xD5, // jmp
   0xD5, // jmp
   0xD8, // push  
   0xDA  // invalid
};

UCHAR ModrmUsedMnems8[8] = {
   0xAE, // fild
   0xDA, // invalid
   0xAF, // fist
   0xB0, // fistp
   0xA0, // fbld
   0xAE, // fild
   0xA1, // fbstp
   0xB0  // fistp    
};

UCHAR ModrmUsedMnems9[8] = {
   0xAE, // fild
   0xDA, // invalid
   0xAF, // fist
   0xB0, // fistp
   0xDA, // invalid
   0xB1, // fld
   0xDA, // invalid
   0xB3  // fstp
};

UCHAR ModrmUsedMnems10[8] = {
   0xB1, // fld      
   0xDA, // invalid
   0xB2, // fst
   0xB3, // fstp
   0xB6, // fldenv
   0xB4, // fldcw
   0xDB, // fnstenv
   0xDC  // fnstcw
};
 
UCHAR ModrmUsedMnems11[8] = {
   0xB1, // fld
   0xDA, // invalid
   0xB2, // fst
   0xB3, // fstp
   0xB9, // frst|
   0xDA, // invalid
   0xB8, // fsave
   0xDD  // fnstsw
};

UCHAR ModrmUsedMnems12[8] = {
   0xD9, // pop
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA  // invalid
};

UCHAR ModrmUsedMnems13[8] = {
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0xDA, // invalid
   0x68, // bt
   0x6B, // bts
   0x6A, // btr
   0x69  // btc     
};

UCHAR ModrmUsedMnems14[8] = {
   0xD1, // sldt
   0xC7, // str
   0xCD, // lldt
   0xC6, // ltr
   0xD3, // verr
   0xD4, // verw
   0xDA, // invalid
   0xDA  // invalid
};

UCHAR ModrmUsedMnems15[8] = {
   0xCF, // sgdt
   0xD0, // sidt
   0xCB, // lgdt
   0xCC, // lidt
   0xD2, // smsw
   0xDA, // invalid
   0xCE, // lmsw
   0xC8  // invlpg  
};

USHORT MmxModrmUsedMnems1[8] = {
   0xDA,  // invalid
   0xDA,  // invalid
   0x108, // PSRLW
   0xDA,  // invalid
   0x106, // PSRAW
   0xDA,  // invalid
   0x103, // PSLLW
   0xDA   // invalid
};

USHORT MmxModrmUsedMnems2[8] = {
   0xDA,  // invalid
   0xDA,  // invalid
   0x109, // PSRLD
   0xDA,  // invalid
   0x107, // PSRAD
   0xDA,  // invalid
   0x104, // PSLLD
   0xDA   // invalid
};

USHORT MmxModrmUsedMnems3[8] = {
   0xDA,  // invalid
   0xDA,  // invalid
   0x10A, // PSRLQ
   0xDA,  // invalid
   0xDA,  // invalid
   0xDA,  // invalid
   0x105, // PSLLQ
   0xDA   // invalid
};

USHORT MmxModrmUsedMnems4[8] = {
   0xDA,   // invalid
   0x11A,  // cmpxchg8b
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA    // invalid
};

USHORT MmxModrmUsedMnems5[8] = {
   0x11B,  // fxrst|
   0x11C,  // fxsave
   0x12C,  // ldmxcsr
   0x143,  // stmxcsr
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA,   // invalid
   0x15D   // sfence
};

USHORT MmxModrmUsedMnems6[8] = {
   0x15C,  // prefetchnta
   0x159,  // prefetcht0
   0x15A,  // prefetcht1
   0x15B,  // prefetcht2
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA,   // invalid
   0xDA    // invalid
};


/*
Base opcodes special flags
Bit: 7 6 5 4 3 2 1 0
     ¦ ¦ ¦ ¦ ¦ ¦ ¦ L OP_USEAL - ocode use AL register
     ¦ ¦ ¦ ¦ ¦ ¦ L-- OP_IMMREV - revers imm and param
     ¦ ¦ ¦ ¦ ¦ L---- OP_PARAMDX - two parametr - DX
     ¦ ¦ ¦ ¦ L------ OP_USECL - ocode use CL register
     ¦ ¦ ¦ L-------- OP_REVERS - reversed params
     ¦ ¦ L---------- OP_USE1 - param 1 used
     ¦ L------------ 
     L-------------- 
*/

USHORT OpcodeSpecFlags[41] = {
  (0x14) | OP_USEAL,
  (0x15) | OP_USEAL,
  (0x04) | OP_USEAL,
  (0x05) | OP_USEAL,
  (0x24) | OP_USEAL,
  (0x25) | OP_USEAL,
  (0x3C) | OP_USEAL,
  (0x3D) | OP_USEAL,
  (0xE4) | OP_USEAL,
  (0xE5) | OP_USEAL,
  (0xEC) | OP_USEAL | OP_PARAMDX,
  (0xED) | OP_USEAL | OP_PARAMDX,
  (0xA0) | OP_USEAL,
  (0xA1) | OP_USEAL,
  (0xA2) | OP_USEAL | OP_REVERS,
  (0xA3) | OP_USEAL | OP_REVERS,
  (0xEE) | OP_USEAL | OP_PARAMDX | OP_REVERS,
  (0xEF) | OP_USEAL | OP_PARAMDX | OP_REVERS,
  (0xD2) | OP_USECL,
  (0xD3) | OP_USECL,
  (0x1C) | OP_USEAL,
  (0x1D) | OP_USEAL,
  (0xA8) | OP_USEAL,
  (0xA9) | OP_USEAL,
  (0x91) | OP_USEAL,
  (0x92) | OP_USEAL,
  (0x93) | OP_USEAL,
  (0x94) | OP_USEAL,
  (0x95) | OP_USEAL,
  (0x96) | OP_USEAL,
  (0x97) | OP_USEAL,
  (0x34) | OP_USEAL,
  (0x35) | OP_USEAL,
  (0xE6) | OP_USEAL | OP_IMMREV,
  (0xE7) | OP_USEAL | OP_IMMREV,
  (0xD0) | OP_USE1,
  (0xD1) | OP_USE1,
  (0x0C) | OP_USEAL,
  (0x0D) | OP_USEAL,
  (0x2D) | OP_USEAL,
  (0x2C) | OP_USEAL
};

/*
W|d Opcodes Flags: (high byte)
Bit: 7 6 5 4 3 2 1 0
     ¦ ¦ ¦ ¦ ¦ ¦ ¦ L OP_REGUSED - instruction opcode used ST(i) registers, if it bit set on non FPU instruction, then not revers use
     ¦ ¦ ¦ ¦ ¦ ¦ L-- OP_REVERSED - ST(0) ir right part in opcode 
     ¦ ¦ ¦ ¦ ¦ L---- OP_TWOPARAM - instruction has two parametrs
     ¦ ¦ ¦ ¦ L------ 
     ¦ ¦ ¦ L--------
     ¦ ¦ L----------
     ¦ L------------
     L--------------
Bits 0..7 - full instruction opcode (w|d) (opcode << 16)
*/
ULONG OpcodeFpuFlags[39] = {
  (0xD8C0) | OP_REGUSED | OP_TWOPARAM,
  (0xDCC0) | OP_REGUSED | OP_REVERSED,
  (0xDEC0) | OP_REGUSED | OP_REVERSED,
  (0xDAC0) | OP_REGUSED | OP_TWOPARAM,
  (0xDAC8) | OP_REGUSED | OP_TWOPARAM,
  (0xDAD0) | OP_REGUSED | OP_TWOPARAM,
  (0xDAD8) | OP_REGUSED | OP_TWOPARAM,
  (0xDBC0) | OP_REGUSED | OP_TWOPARAM,
  (0xDBC8) | OP_REGUSED | OP_TWOPARAM,
  (0xDBD0) | OP_REGUSED | OP_TWOPARAM,
  (0xDBD8) | OP_REGUSED | OP_TWOPARAM,
  (0xD8D0) | OP_REGUSED,
  (0xDEC8) | OP_REGUSED | OP_REVERSED,
  (0xD8D8) | OP_REGUSED,
  (0xDBF0) | OP_REGUSED | OP_TWOPARAM,
  (0xDFF0) | OP_REGUSED | OP_TWOPARAM,
  (0xDBE8) | OP_REGUSED | OP_TWOPARAM,
  (0xDFE8) | OP_REGUSED | OP_TWOPARAM,
  (0xD8F0) | OP_REGUSED | OP_TWOPARAM,
  (0xDCF8) | OP_REGUSED | OP_REVERSED,
  (0xDEF8) | OP_REGUSED | OP_REVERSED,
  (0xD8F8) | OP_REGUSED | OP_TWOPARAM,
  (0xDCF0) | OP_REGUSED | OP_REVERSED,
  (0xDEF0) | OP_REGUSED | OP_REVERSED,
  (0xDDC0) | OP_REGUSED,
  (0xD9C0) | OP_REGUSED,
  (0xD8C8) | OP_REGUSED | OP_TWOPARAM,
  (0xDCC8) | OP_REGUSED | OP_REVERSED,
  (0xDDD0) | OP_REGUSED,
  (0xDDD8) | OP_REGUSED,
  (0xD8E0) | OP_REGUSED | OP_TWOPARAM,
  (0xDCE8) | OP_REGUSED | OP_REVERSED,
  (0xDEE8) | OP_REGUSED | OP_REVERSED,
  (0xD8E8) | OP_REGUSED | OP_TWOPARAM,
  (0xDCE0) | OP_REGUSED | OP_REVERSED,
  (0xDEE0) | OP_REGUSED | OP_REVERSED,
  (0xDDE0) | OP_REGUSED,
  (0xDDE8) | OP_REGUSED,
  (0xD9C8) | OP_REGUSED
};
 
/*
  FPU commands mem|y f|mat table.
  Bits 0..7  - Opcode
  Bits 8..10  - Ptr type
  Bits 11-13 - REG value
*/                       
USHORT FpuMemFmt[50] = {
  0xD8 | (0 << 11) | F_PTR_REAL4,
  0xD8 | (1 << 11) | F_PTR_REAL4,
  0xD8 | (2 << 11) | F_PTR_REAL4,
  0xD8 | (3 << 11) | F_PTR_REAL4,
  0xD8 | (4 << 11) | F_PTR_REAL4,
  0xD8 | (5 << 11) | F_PTR_REAL4,
  0xD8 | (6 << 11) | F_PTR_REAL4,
  0xD8 | (7 << 11) | F_PTR_REAL4,
  0xDC | (0 << 11) | F_PTR_REAL8,
  0xDC | (1 << 11) | F_PTR_REAL8,
  0xDC | (2 << 11) | F_PTR_REAL8,
  0xDC | (3 << 11) | F_PTR_REAL8,
  0xDC | (4 << 11) | F_PTR_REAL8,
  0xDC | (5 << 11) | F_PTR_REAL8,
  0xDC | (6 << 11) | F_PTR_REAL8,
  0xDC | (7 << 11) | F_PTR_REAL8,
  0xDA | (0 << 11) | F_PTR_DWORD,
  0xDA | (1 << 11) | F_PTR_DWORD,
  0xDA | (2 << 11) | F_PTR_DWORD,
  0xDA | (3 << 11) | F_PTR_DWORD,
  0xDA | (4 << 11) | F_PTR_DWORD,
  0xDA | (5 << 11) | F_PTR_DWORD,
  0xDA | (6 << 11) | F_PTR_DWORD,
  0xDA | (7 << 11) | F_PTR_DWORD,
  0xDE | (0 << 11) | F_PTR_WORD,
  0xDE | (1 << 11) | F_PTR_WORD,
  0xDE | (2 << 11) | F_PTR_WORD,
  0xDE | (3 << 11) | F_PTR_WORD,
  0xDE | (4 << 11) | F_PTR_WORD,
  0xDE | (5 << 11) | F_PTR_WORD,
  0xDE | (6 << 11) | F_PTR_WORD,
  0xDE | (7 << 11) | F_PTR_WORD,
  0xDF | (4 << 11) | F_PTR_TBYTE,
  0xDF | (6 << 11) | F_PTR_TBYTE,
  0xDF | (0 << 11) | F_PTR_WORD,
  0xDF | (5 << 11) | F_PTR_QWORD,
  0xDF | (2 << 11) | F_PTR_WORD,
  0xDF | (3 << 11) | F_PTR_WORD,
  0xDF | (7 << 11) | F_PTR_QWORD,
  0xDB | (0 << 11) | F_PTR_DWORD,
  0xDB | (2 << 11) | F_PTR_DWORD,
  0xDB | (3 << 11) | F_PTR_DWORD,
  0xDB | (5 << 11) | F_PTR_REAL10,
  0xDB | (7 << 11) | F_PTR_REAL10,
  0xD9 | (0 << 11) | F_PTR_REAL4,
  0xD9 | (2 << 11) | F_PTR_REAL4,
  0xD9 | (3 << 11) | F_PTR_REAL4,
  0xDD | (0 << 11) | F_PTR_REAL8,
  0xDD | (2 << 11) | F_PTR_REAL8,
  0xDD | (3 << 11) | F_PTR_REAL8
};

UCHAR RelInstrTable[8] = {
  0xD5,  // jmp
  0xD6,  // Jcc
  0x9A,  // call
  0xC9,  // jcxz
  0xCA,  // jecxz
  0xC3,  // loop
  0xC4,  // loopz
  0xC5   // loopnz
};

char MnemArr[0x15E][12] = {
  "aaa",         // 00
  "aad",         // 01
  "aam",         // 02
  "aas",         // 03
  "cwde",        // 04
  "cbw",         // 05
  "clc",         // 06
  "cld",         // 07
  "cli",         // 08
  "clts",        // 09
  "cmc",         // 0A
  "cmpsb",       // 0B
  "cmpsw",       // 0C
  "cmpsd",       // 0D
  "cwd",         // 0E
  "cdq",         // 0F
  "daa",         // 10
  "das",         // 11
  "f2xm1",       // 12
  "fabs",        // 13
  "fchs",        // 14
  "fclex",       // 15
  "fcos",        // 16
  "fdecstp",     // 17
  "fincstp",     // 18
  "finit",       // 19
  "fld1",        // 1A
  "fldl2t",      // 1B
  "fldl2e",      // 1C
  "fldpi",       // 1D
  "fldlg2",      // 1E
  "fldln2",      // 1F
  "fldz",        // 20
  "fnop",        // 21
  "fpatan",      // 22
  "fprem",       // 23
  "fprem1",      // 24
  "fptan",       // 25
  "frndint",     // 26
  "fscale",      // 27
  "fsin",        // 28
  "fsincos",     // 29
  "fsqrt",       // 2A
  "ftst",        // 2B
  "fxam",        // 2C
  "fxtract",     // 2D
  "fyl2x",       // 2E
  "fyl2xp1",     // 2F
  "hlt",         // 30
  "insb",        // 31
  "insw",        // 32
  "insd",        // 33
  "into",        // 34
  "invd",        // 35
  "iret",        // 36
  "iretd",       // 37
  "lahf",        // 38
  "leave",       // 39
  "lodsb",       // 3A
  "lodsw",       // 3B
  "lodsd",       // 3C
  "movsb",       // 3D
  "movsw",       // 3E
  "movsd",       // 3F
  "nop",         // 40
  "outsb",       // 41
  "outsw",       // 42
  "outsd",       // 43
  "popa",        // 44
  "popad",       // 45
  "popf",        // 46
  "popfd",       // 47
  "pusha",       // 48
  "pushad",      // 49
  "pushf",       // 4A
  "pushfd",      // 4B
  "ret",         // 4C
  "retf",        // 4D
  "sahf",        // 4E
  "scasb",       // 4F
  "scasw",       // 50
  "scasd",       // 51
  "stc",         // 52
  "std",         // 53
  "sti",         // 54
  "stosb",       // 55
  "stosw",       // 56
  "stosd",       // 57
  "wait",        // 58
  "wbinvd",      // 59
  "xlat",        // 5A
  "cpuid",       // 5B
  "rdmsr",       // 5C
  "rdpmc",       // 5D
  "rdtsc",       // 5E
  "ud2",         // 5F
  "wrmsr",       // 60
  "adc",         // 61
  "add",         // 62
  "and",         // 63
  "arpl",        // 64
  "bound",       // 65
  "bsf",         // 66
  "bsr",         // 67
  "bt",          // 68
  "btc",         // 69
  "btr",         // 6A
  "bts",         // 6B
  "cmp",         // 6C
  "cmpxchg",     // 6D
  "enter",       // 6E
  "or",          // 6F
  "sbb",         // 70
  "sub",         // 71
  "xor",         // 72
  "lar",         // 73
  "lea",         // 74
  "in",          // 75
  "out",         // 76
  "fadd",        // 77
  "faddp",       // 78
  "fmul",        // 79
  "fmulp",       // 7A
  "fdiv",        // 7B
  "fdivp",       // 7C
  "fdivr",       // 7D
  "fdivrp",      // 7E
  "fsub",        // 7F
  "fsubp",       // 80
  "fsubr",       // 81
  "fsubrp",      // 82
  "lds",         // 83
  "les",         // 84
  "lss",         // 85
  "lgs",         // 86
  "lfs",         // 87
  "imul",        // 88
  "lsl",         // 89
  "test",        // 8A
  "rcl",         // 8B
  "rcr",         // 8C
  "rol",         // 8D
  "ror",         // 8E
  "sar",         // 8F
  "shl",         // 90
  "shr",         // 91
  "xadd",        // 92
  "movsx",       // 93
  "movzx",       // 94
  "xchg",        // 95
  "mov",         // 96
  "shld",        // 97
  "shrd",        // 98
  "bswap",       // 99
  "call",        // 9A
  "dec",         // 9B
  "inc",         // 9C
  "div",         // 9D
  "idiv",        // 9E
  "fiadd",       // 9F
  "fbld",        // A0
  "fbstp",       // A1
  "fcom",        // A2
  "fcomp",       // A3
  "fcompp",      // A4
  "fucom",       // A5
  "fucomp",      // A6
  "fucompp",     // A7
  "ficom",       // A8
  "ficomp",      // A9
  "fidiv",       // AA
  "fidivr",      // AB
  "ffree",       // AC
  "fxch",        // AD
  "fild",        // AE
  "fist",        // AF
  "fistp",       // B0
  "fld",         // B1
  "fst",         // B2
  "fstp",        // B3
  "fldcw",       // B4
  "fstcw",       // B5
  "fldenv",      // B6
  "setalc",      // B7
  "fsave",       // B8
  "frstor",      // B9
  "retn",        // BA
  "fisub",       // BB
  "fisubr",      // BC
  "fimul",       // BD
  "mul",         // BE
  "neg",         // BF
  "not",         // C0
  "int",         // C1
  "int 3",       // C2
  "loop",        // C3
  "loopz",       // C4
  "loopnz",      // C5
  "ltr",         // C6
  "str",         // C7
  "invlpg",      // C8
  "jcxz",        // C9
  "jecxz",       // CA
  "lgdt",        // CB
  "lidt",        // CC
  "lldt",        // CD
  "lmsw",        // CE
  "sgdt",        // CF
  "sidt",        // D0
  "sldt",        // D1
  "smsw",        // D2
  "verr",        // D3
  "verw",        // D4
  "jmp",         // D5
  "j",           // D6
  "set",         // D7
  "push",        // D8
  "pop",         // D9
  "invalid",     // DA
  "fstenv",      // DB
  "fnstcw",      // DC
  "fstsw",       // DD
  "icebp",       // DE
  "sysenter",    // DF
  "sysexit",     // E0
  "fcmovb",      // E1
  "fcmove",      // E2
  "fcmovbe",     // E3
  "fcmovu",      // E4
  "fcmovnb",     // E5
  "fcmovne",     // E6
  "fcmovnbe",    // E7
  "fcmovnu",     // E8
  "cmov",        // E9
  "emms",        // EA
  "rsm",         // EB
  "movq",        // EC
  "packsswb",    // ED
  "paskssdw",    // EE
  "packuswb",    // EF
  "paddb",       // F0
  "paddw",       // F1
  "paddd",       // F2
  "paddsb",      // F3
  "paddsw",      // F4
  "paddusb",     // F5
  "paddusw",     // F6
  "pand",        // F7
  "pandn",       // F8
  "pcmpeqb",     // F9
  "pcmpeqw",     // FA
  "pcmpeqd",     // FB
  "pcmpgtb",     // FC
  "pcmpgtw",     // FD
  "pcmpgtd",     // FE
  "pmaddwd",     // FF
  "pmulhw",      // 100
  "pmullw",      // 101
  "por",         // 102
  "psllw",       // 103
  "pslld",       // 104
  "psllq",       // 105
  "psraw",       // 106
  "psrad",       // 107
  "psrlw",       // 108
  "psrld",       // 109
  "psrlq",       // 10A
  "psubb",       // 10B
  "psubw",       // 10C
  "psubd",       // 10D
  "psubsb",      // 10E
  "psubsw",      // 10F
  "psubusb",     // 110
  "psubusw",     // 111
  "punpckhbw",   // 112
  "punpckhwd",   // 113
  "punpckhdq",   // 114
  "punpcklbw",   // 115
  "punpcklwd",   // 116
  "punpckldq",   // 117
  "pxor",        // 118
  "movd",        // 119
  "cmpxchg8b",   // 11A
  "fxsave",      // 11B
  "fxrstor",     // 11C
  "addps",       // 11D
  "addss",       // 11E
  "andps",       // 11F
  "andnps",      // 120
  "cmpps",       // 121
  "cmpss",       // 122
  "comiss",      // 123
  "cvtpi2ps",    // 124
  "cvtsi2ss",    // 125
  "cvtps2pi",    // 126
  "cvtss2si",    // 127
  "cvttps2pi",   // 128
  "cvtss2si",    // 129
  "divps",       // 12A
  "divss",       // 12B
  "ldmxcsr",     // 12C
  "maxps",       // 12D
  "maxss",       // 12E
  "minps",       // 12F
  "minss",       // 130
  "movaps",      // 131
  "movhlps",     // 132
  "movlps",      // 133
  "movlhps",     // 134
  "movhps",      // 135
  "movmskps",    // 136
  "movups",      // 137
  "movss",       // 138
  "mulps",       // 139
  "mulss",       // 13A
  "orps",        // 13B
  "rcpps",       // 13C
  "rcpss",       // 13D
  "rsqrtps",     // 13E
  "rsqrtss",     // 13F
  "shufps",      // 140
  "sqrtps",      // 141
  "sqrtss",      // 142
  "stmxcsr",     // 143
  "subps",       // 144
  "subss",       // 145
  "ucomiss",     // 146
  "unpckhps",    // 147
  "unpcklps",    // 148
  "xorps",       // 149
  "maskmovq",    // 14A
  "movntps",     // 14B
  "movntq",      // 14C
  "pavgb",       // 14D
  "pavgw",       // 14E
  "pextrw",      // 14F
  "pinsrw",      // 150
  "pmaxsw",      // 151
  "pmaxub",      // 152
  "pminsw",      // 153
  "pminub",      // 154
  "pmovmskb",    // 155
  "pmulhuw",     // 156
  "psadbw",      // 157
  "pshufw",      // 158
  "prefetcht0",  // 159
  "prefetcht1",  // 15A
  "prefetcht2",  // 15B
  "prefetchnta", // 15C
  "sfence"       // 15D
 };
