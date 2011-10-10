; ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
    .486                       ; create 32 bit code
    .model flat, stdcall       ; 32 bit memory model
    option casemap :none       ; case sensitive
 
    include windows.inc
    include masm32.inc
    include kernel32.inc
	include user32.inc

    includelib masm32.lib
    includelib kernel32.lib
	includelib user32.lib
    
;--------------------------------------------------------------------------------
; For RADasm breakpoint only
include ..\..\inc\radbg.inc 
    
    
; ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл    
    
    
    ; ---------------------
    ; literal string MACRO
    ; ---------------------
      literal MACRO quoted_text:VARARG
        LOCAL local_text
        .data
          local_text db quoted_text,0
        .code
        EXITM <local_text>
      ENDM
    ; --------------------------------
    ; string address in INVOKE format
    ; --------------------------------
      SADD MACRO quoted_text:VARARG
        EXITM <ADDR literal(quoted_text)>
      ENDM    
    
    
; ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
rotate PROTO

; ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

.code

; ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

start:
pushad
pushfd

;put stupid value in flags
mov eax,02d7H
push eax
popfd

mov eax, 10H
mov ebx, 20H
mov ecx, 30H
mov edx, 40H
call rotate

;save flags for futur checking
pushfd

;check registers values
cmp eax,20H
jne label_error

cmp ebx,30H
jne label_error

cmp ecx,40H
jne label_error

cmp edx,10H
jne label_error

;check flags value

pop eax
cmp eax,02d7H
je label_success

label_error:
invoke MessageBox,0,SADD("Bad registers values"),SADD("Error"),MB_OK OR MB_ICONERROR
jmp label_end

label_success:
invoke MessageBox,0,SADD("Good registers values"),SADD("Information"),MB_OK OR MB_ICONINFORMATION

label_end:

popfd
popad
invoke ExitProcess,0


; ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

rotate proc
	
	; rotate registers 
	push eax
	mov eax,ebx
	mov ebx,ecx
	mov ecx,edx
	pop edx
	ret

rotate endp

end start