.686p
.model flat, stdcall
option casemap:none

include \masm32\include\w2k\ntstatus.inc
include \masm32\include\w2k\ntddk.inc
include \masm32\include\w2k\ntoskrnl.inc
includelib \masm32\lib\w2k\ntoskrnl.lib
include ..\..\HEADERS\BeaEngineMasm.inc
includelib ..\..\LIBRARY\BeaEngine.lib

.data

    len             DWORD 0
    CountInstr      DWORD 20
    align 10h
    MonDisasm       _Disasm <>
    align 10h
    StopDisasm      BYTE    "Stop disassembling",0Dh, 0Ah, 0
    align 10h
    FormatNumber    BYTE    "%.8X %s",0Dh,0Ah,0
    
.code
; ***********************************************
;
;             Routine to disassemble
;
; ***********************************************
RoutineToDisasm proc
	cli
      sti
      hlt
      iret
      lidt fword ptr [ebx]
      lgdt fword ptr [ecx+edi*4+401000h]
      sidt fword ptr [eax]
	mov eax,cr0
	or ax,1
	mov cr0,eax		
	mov ax,10h		
	mov ds,ax
	mov fs,ax
	mov gs,ax
	mov es,ax
	mov ss,ax
	mov esp,9F000h	
    popad
RoutineToDisasm endp

; ***********************************************
;
;               Main loop 
;
; ***********************************************

DriverEntry proc pDriverObject:PDRIVER_OBJECT, pusRegistryPath:PUNICODE_STRING
    ; ************************* clear _Disasm structure
    push edi
    mov ecx, SIZEOF _Disasm
    xor eax, eax
    mov edi, offset MonDisasm
    rep stosb
    pop edi
    ; ************************* Init disasm structure
    push offset RoutineToDisasm
    pop dword ptr [MonDisasm.EIP]
    push offset RoutineToDisasm
    pop dword ptr [MonDisasm.VirtualAddr]
DisasmInstruction:
    mov MonDisasm.SecurityBlock, 0
    push offset MonDisasm
    call Disasm
    .if (eax != OUT_OF_BLOCK) && (eax != UNKNOWN_OPCODE)
        mov len, eax
        ; ************************ Display address
        push offset MonDisasm.CompleteInstr
        push offset MonDisasm.VirtualAddr
        push offset FormatNumber
        call DbgPrint
        ; ************************ Next instruction
        mov eax, len
        add dword ptr [MonDisasm.EIP], eax
        add dword ptr [MonDisasm.VirtualAddr], eax
        dec CountInstr
        jne DisasmInstruction
    .endif
    push offset StopDisasm
    call DbgPrint
    mov eax, STATUS_DEVICE_CONFIGURATION_ERROR
    ret
DriverEntry endp

end DriverEntry
