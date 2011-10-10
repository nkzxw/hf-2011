;Follow these steps to build the project: 
;1) Go to Project properties, select All Configurations in the Settings For list. 
;2) In "Build Event" / "Pre-Build Event"
;     put "Command Line" to MakeMasmSub.bat
;3)In "Linker" / "Input"
;    add MasmSub.obj to "Additional Dependencies" 


.386
.MODEL flat, C

.CODE

mRotateRegisters PROC

	; rotate registers in another order
	push edx
	mov edx,ecx
	mov ecx,ebx
	mov ebx,eax
	pop eax
	ret

   ret
mRotateRegisters ENDP
END 