.386
.MODEL flat,stdcall
option casemap:none
.mmx

include \masm32\include\kernel32.inc
include \masm32\include\windows.inc
includelib \masm32\lib\kernel32.lib
include ..\..\HEADERS\BeaEngineMasm.inc

puts PROTO:DWORD

.data

    MyDisasm        _Disasm         <>
    szoutofblock    BYTE            "Security alert. Disasm tries to read unreadable memory",0
    Info1           BYTE            "********** Decode the start routine in 32 bits configuration *********",0
    Info2           BYTE            "********** Decode the same routine in 64 bits configuration *********",0
    i               DWORD           0
    
.code

start:

; *****************************************************
;
;           decode instructions (x86)
;
; *****************************************************

    push offset Info1
    call puts                           
    add esp, 4

    mov i, 100
    
    ; *********************** Init EIP 
    mov eax, start
    mov dword ptr [MyDisasm.EIP], eax

    ; *********************** Define 32 bits architecture
    mov MyDisasm.Archi, 0

    ; *********************** Just for fun : init VirtualAddr with funky value :)
    mov eax, 0bea2009h
    movd mm0, eax
    movq MyDisasm.VirtualAddr, mm0

    ; *********************** loop for disasm
    MakeDisasmx86:
        push offset MyDisasm
        call Disasm
        .if (eax == OUT_OF_BLOCK)
            push offset szoutofblock
            call puts
            add esp, 4
            push 0
            call ExitProcess
        .elseif (eax == UNKNOWN_OPCODE)
            inc dword ptr [MyDisasm.EIP]
        .else
            add dword ptr [MyDisasm.EIP], eax
        .endif
        push offset MyDisasm.CompleteInstr
        call puts                           
        add esp, 4
        dec i
        jne MakeDisasmx86

; *****************************************************
;
;           decode instructions (x64)
;
; *****************************************************
    push offset Info2
    call puts                           
    add esp, 4
    
    mov i, 100

    ; *********************** Init EIP 
    mov eax, start
    mov dword ptr [MyDisasm.EIP], eax

    ; *********************** Define 64 bits architecture
    mov MyDisasm.Archi, 64

    ; *********************** Just for fun : init VirtualAddr with funky value :)
    mov eax, 0bea2009h
    movd mm0, eax
    movq MyDisasm.VirtualAddr, mm0

    ; *********************** loop for disasm
    MakeDisasmx64:
        push offset MyDisasm
        call Disasm
        .if (eax == OUT_OF_BLOCK)
            push offset szoutofblock
            call puts
            add esp, 4
            push 0
            call ExitProcess
        .elseif (eax == UNKNOWN_OPCODE)
            inc dword ptr [MyDisasm.EIP]
        .else
            add dword ptr [MyDisasm.EIP], eax
        .endif
        push offset MyDisasm.CompleteInstr
        call puts                           
        add esp, 4
        dec i
        jne MakeDisasmx64

    push 0
    call ExitProcess
End start                         



