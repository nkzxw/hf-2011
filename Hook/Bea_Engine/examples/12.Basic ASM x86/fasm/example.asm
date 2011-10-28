format MS COFF

; ************************************** Define "prototype"

extrn '_puts@4' as puts:dword
extrn '_Disasm@4' as Disasm:dword
extrn '_ExitProcess@4' as ExitProcess:dword

; ************************************** includes
include '\fasm\INCLUDE\win32ax.inc'     ; <--- extended headers to enable macroinstruction .if .elseif .end
include '..\..\HEADERS\BeaEngineFasm.inc'


section '.data' data readable writeable


    MyDisasm       _Disasm       <>
    i               db            100
    szoutofblock    db            "Security alert. Disasm tries to read unreadable memory",0


section '.text' code readable executable

 public start

 start:

    ; *********************** Init EIP
    mov eax, start
    mov dword [MyDisasm.EIP], eax
   
    ; *********************** loop for disasm
    MakeDisasm:
        push MyDisasm
        call Disasm
        .if eax = OUT_OF_BLOCK
            push szoutofblock
            call puts
            add esp, 4
            push 0
            call ExitProcess
        .elseif eax = UNKNOWN_OPCODE
            inc dword [MyDisasm.EIP]
        .else
            add dword [MyDisasm.EIP], eax
        .endif
        push MyDisasm.CompleteInstr
        call puts                           
        add esp, 4
        dec byte [i]
        jne MakeDisasm
    push 0
    call ExitProcess



