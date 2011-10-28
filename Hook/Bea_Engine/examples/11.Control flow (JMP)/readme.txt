CONTROL FLOW 2 - Example

In this example, the file "example.exe" disassembles the file "msgbox.exe" which is slightly obfuscated.

First, the disassembly is in normal mode. Then, "example.exe" applies a filter to eliminate superfluous "jmp" instructions and to reorder "good" instructions by using the field MyDisasm.Instruction.AddrValue. The code disassembled is cleared and understandable immediatly.

*******************************************************
Disassemble code in normal mode
*******************************************************
00401000 push 0h
00401002 jmp 40101Dh
00401004 call 40102Ah
00401009 jmp 401024h
0040100B push 40300Ah
00401010 jmp 401019h
00401012 call 401030h
00401017 jmp 401028h
00401019 push 0h
0040101B jmp 401004h
0040101D push 403004h
00401022 jmp 40100Bh
00401024 push 0h
00401026 jmp 401012h
00401028 ret
00401029 int3
0040102A jmp near dword ptr [402008h]
*******************************************************
Disassemble code by following jumps
*******************************************************
00401000 push 0h
0040101D push 403004h
0040100B push 40300Ah
00401019 push 0h
00401004 call 40102Ah
00401024 push 0h
00401012 call 401030h
00401028 ret
00401029 int3
0040102A jmp near dword ptr [402008h]
