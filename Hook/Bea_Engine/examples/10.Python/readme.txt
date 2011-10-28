Python EXAMPLE

A small example of how to use BeaEngine3 with Python. Thanks to ctypes library, BeaEngine.dll can be used easily.

Here is the source code from File2Disasm.exe read with the debugger OllyDbg 1.1 :

00401000  6A 00            PUSH       00H
00401002  EB 19            JMP        0040101DH
00401004  E8 21000000      CALL       0040102AH                     ; \MessageBoxA
00401009  EB 19            JMP        00401024H
0040100B  68 0A304000      PUSH       0040300AH                     ;  ASCII "text"
00401010  EB 07            JMP        00401019H
00401012  E8 19000000      CALL       00401030H                     ; \ExitProcess
00401017  EB 0F            JMP        00401028H
00401019  6A 00            PUSH       00H
0040101B  EB E7            JMP        00401004H
0040101D  68 04304000      PUSH       00403004H                     ;  ASCII "title"
00401022  EB E7            JMP        0040100BH
00401024  6A 00            PUSH       00H
00401026  EB EA            JMP        00401012H
00401028  C3               RET
00401029  CC               INT3
0040102A  FF25 08204000    JMP        DWORD PTR [00402008H]         ;  user32.MessageBoxA
00401030  FF25 00204000    JMP        DWORD PTR [00402000H]         ;  kernel32.ExitProcess



Here is the same source code read thanks to the small Python program :


0x401000L     push       00h
0x401002L     jmp        0040101Dh
0x401004L     call       0040102Ah
0x401009L     jmp        00401024h
0x40100bL     push       0040300Ah
0x401010L     jmp        00401019h
0x401012L     call       00401030h
0x401017L     jmp        00401028h
0x401019L     push       00h
0x40101bL     jmp        00401004h
0x40101dL     push       00403004h
0x401022L     jmp        0040100Bh
0x401024L     push       00h
0x401026L     jmp        00401012h
0x401028L     ret
0x401029L     int3
0x40102aL     jmp        dword [ds:00402008h]
0x401030L     jmp        dword [ds:00402000h]
