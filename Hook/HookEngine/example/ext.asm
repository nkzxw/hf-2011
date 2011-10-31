		.code
;
; Points the stack pointer at the supplied argument and returns to the caller.
;
	public AdjustStackCallPointer
	AdjustStackCallPointer PROC
		mov rsp, rcx
		xchg r8, rcx
		jmp rdx
	AdjustStackCallPointer ENDP
	END