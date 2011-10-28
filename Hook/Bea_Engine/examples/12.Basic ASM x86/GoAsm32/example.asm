#include BeaEngineGoAsm.inc
Disasm = BeaEngine.lib:Disasm

.data
    
    MyDisasm       _Disasm      <>
    szoutofblock    db           "Security alert. Disasm tries to read unreadable memory",0
    i               db            100
.code

start:

    ; *********************** Init EIP
    mov eax, offset start
    mov [MyDisasm.EIP], eax

    ; *********************** loop for disasm
MakeDisasm:
    push offset MyDisasm
    call Disasm
    cmp eax, OUT_OF_BLOCK
    jne >
        push offset szoutofblock
        call puts
        add esp, 4
        push 0
        call ExitProcess
    :
    cmp eax, UNKNOWN_OPCODE
    jne >
        inc d[MyDisasm.EIP]
        jmp Display
    :
        add [MyDisasm.EIP], eax
Display:        
    push offset MyDisasm.CompleteInstr
    call puts                           
    add esp, 4
    dec b[i]
    jne MakeDisasm
    push 0
    call ExitProcess



