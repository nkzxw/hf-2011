;********************************************************************
;	created:	23:9:2008   02:20
;	file:		trap.asm
;	author:		tiamo
;	purpose:	interrupt handler
;********************************************************************

Trap0:
Trap1:
Trap2:
Trap3:
Trap4:
Trap5:
Trap6:
Trap7:
Trap8:
Trap9:
TrapA:
TrapB:
TrapC:
TrapD:
TrapE:
TrapF:
							;
							; clear dr6
							;
								xor			eax,eax
								mov			dr6,eax

							;
							; set ds,ss
							;
								mov			ax,SuDataSelector
								mov			ds,ax
								mov			ss,ax
								mov			sp,EXPORT_STACK

							;
							; show error string
							;
								push		.ExceptionErrorMsg
								call		puts
								add			sp,2

							;
							; switch to real mode
							;
								call		RealMode

							;
							; dead loop
							;
								jmp			$
.ExceptionErrorMsg				db			"*****EXCEPTION*****",0