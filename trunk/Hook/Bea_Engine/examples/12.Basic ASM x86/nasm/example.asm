extern _puts@4				; define external symbols
extern _ExitProcess@4
extern _Disasm@4
global start
%include "..\..\HEADERS\BeaEngineNasm.inc"

section .data use32

    i       db 100
    
    MyDisasm:
            istruc _Disasm
            iend    

section .text use32

start: 

    ; ***************************** Init EIP
    mov eax, start
    mov [MyDisasm+EIP], eax

    ; ***************************** just for fun : init VirtualAddr with weird address :)
    mov eax, 0xbea2008
    movd mm0, eax
    movq [MyDisasm+VirtualAddr], mm0
    
    ; ***************************** loop for disasm
MakeDisasm:
    push MyDisasm
    call _Disasm@4
    cmp eax, UNKNOWN_OPCODE
    je IncreaseEIP
        add [MyDisasm+EIP], eax
        jmp DisplayInstruction
    
    IncreaseEIP:
        inc dword [MyDisasm+EIP]
    
    DisplayInstruction:
        push MyDisasm+CompleteInstr
        call _puts@4
        add esp, 4
        dec byte [i]
        jne MakeDisasm
    push 0
    call _ExitProcess@4

