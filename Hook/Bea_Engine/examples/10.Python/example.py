#================================================
#
#	Python example using BeaEngine3
#	Disassemble obfuscated binary
#
#================================================

from BeaEnginePython import *

Instruction = DISASM()

# ====================================== Open File2Disasm.exe
try:
    FileObject = open('File2Disasm.exe','rb')
except IOError:
    print "can't find specified file"
else:
    FileObject.seek(0,0)
    Buffer= FileObject.read()
    FileObject.close()

    # ====================================== Create a ctypes buffer
    Target = create_string_buffer(Buffer,len(Buffer))

    # ====================================== init structure
    Instruction.EIP = addressof(Target) + 0x400	# Jump on the ".text" section with RawOffset = 0x400
    Instruction.VirtualAddr = 0x401000
    Instruction.Options = Tabulation + NasmSyntax + SuffixedNumeral  + ShowSegmentRegs		#define syntax

    # ====================================== Disasm 20 instructions

    for i in range(20):
        InstrLength = Disasm(addressof(Instruction))
        print hex(Instruction.VirtualAddr),"   ",Instruction.CompleteInstr
        if InstrLength == UNKNOWN_OPCODE:
            Instruction.EIP = Instruction.EIP + 1
            Instruction.VirtualAddr = Instruction.VirtualAddr + 1
        else:
            Instruction.EIP = Instruction.EIP + InstrLength 
            Instruction.VirtualAddr= Instruction.VirtualAddr+ InstrLength 
