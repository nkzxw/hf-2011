Dynamic patterns - EXAMPLE

In this example, we open the target file "junkcode.bin" and we look for the dynamic pattern :

add X, Y
sub X, Y

X and Y can be registers, constants, memory. 

Here is the source code included in the junkcode.bin file :


00000000    0FC8          BSWAP EAX
00000002    50            PUSH EAX
00000003    58            POP EAX
00000004    03C6          ADD EAX,ESI
00000006    2BC6          SUB EAX,ESI
00000008    90            NOP
00000009    83EC 04       SUB ESP,4
0000000C    5B            POP EBX
0000000D    03FB          ADD EDI,EBX
0000000F    2BFB          SUB EDI,EBX
00000011    90            NOP
00000012    0FC8          BSWAP EAX
00000014    B9 0F000000   MOV ECX,0F
00000019    2BCF          SUB ECX,EDI
0000001B    47            INC EDI
0000001C    4F            DEC EDI
0000001D    83C4 04       ADD ESP,4
00000020    83EC 04       SUB ESP,4
00000023    90            NOP
00000024    90            NOP
00000025    90            NOP
00000026    03CD          ADD ECX,EBP
00000028    2BCD          SUB ECX,EBP
0000002A    83C0 0C       ADD EAX,0C
0000002D    83E8 0C       SUB EAX,0C
00000030    0FC1C0        XADD EAX,EAX

Here is the result after scanning the previous code :


*******************************************************
Looking for obfuscation - simple pattern :
add X, Y
sub X, Y
*******************************************************

pattern found in line 3

00401004 add eax, esi
00401006 sub eax, esi

pattern found in line 8

0040100D add edi, ebx
0040100F sub edi, ebx

pattern found in line 16

0040101D add esp, 4h
00401020 sub esp, 4h

pattern found in line 21

00401026 add ecx, ebp
00401028 sub ecx, ebp

pattern found in line 23

0040102A add eax, 0Ch
0040102D sub eax, 0Ch

