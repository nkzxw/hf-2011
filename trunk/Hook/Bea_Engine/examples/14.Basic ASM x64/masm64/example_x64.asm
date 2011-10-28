include ..\..\HEADERS\BeaEngineMasm.inc

extrn puts:PROC
extrn ExitProcess: PROC


.data

    MyDisasm        _Disasm         <>
    szoutofblock    BYTE            "Security alert. Disasm tries to read unreadable memory",0
    i               DWORD           100


.code

main proc

    ; *********************** Init EIP
    mov rax, main
    mov MyDisasm.EIP, rax

    ; *********************** Init Architecture
    mov MyDisasm.Archi, 64

    ; *********************** loop for disasm
    MakeDisasm:
        mov rcx, offset MyDisasm
        call Disasm
        cmp eax, OUT_OF_BLOCK
        jne @F
            mov rcx, offset szoutofblock
            sub rsp, 20h
            call puts
            add rsp, 20h
            mov rcx, 0
            call ExitProcess
        @@:
        cmp eax, UNKNOWN_OPCODE
        jne @F
            inc MyDisasm.EIP
            jmp Display
        @@:
            add MyDisasm.EIP, rax
    Display:
        mov rcx, offset MyDisasm.CompleteInstr
        sub rsp, 20h
        call puts                           
        add rsp, 20h
        dec i
        jne MakeDisasm
    
    mov rcx, 0
    call ExitProcess
main endp

end

