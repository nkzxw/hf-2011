#include BeaEngineGoAsm.inc
Disasm = BeaEngine64.lib:Disasm

.data
    
    MyDisasm       _Disasm      <>
    szoutofblock    db           "Security alert. Disasm tries to read unreadable memory",0
    i               db            100
.code

start:

    ; *********************** Init EIP
    mov rax, offset start
    mov q [MyDisasm.EIP], rax

    ; *********************** Init Architecture
    mov d [MyDisasm.Archi], 64

    ; *********************** loop for disasm
MakeDisasm:
    mov rcx, offset MyDisasm
    call Disasm
    cmp rax, OUT_OF_BLOCK
    jne >
        mov rcx, offset szoutofblock
        sub rsp, 8
        call puts
        add rsp, 8
        mov rcx, 0
        call ExitProcess
    :
    cmp rax, UNKNOWN_OPCODE
    jne >
        inc q[MyDisasm.EIP]
        jmp Display
    :
        add q[MyDisasm.EIP], rax
Display:        
    mov rcx, offset MyDisasm.CompleteInstr
    sub rsp, 8
    call puts                           
    add rsp, 8
    dec b[i]
    jne MakeDisasm
    mov rcx, 0
    call ExitProcess



