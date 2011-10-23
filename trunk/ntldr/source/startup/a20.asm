;********************************************************************
;	created:	23:9:2008   02:05
;	file:		a20.asm
;	author:		tiamo
;	purpose:	a20 line
;********************************************************************

EnableA20:
							;
							; check error status
							;
								mov			di,Empty8042Error
								cmp			byte [di],1
								jz			.Empty8042Error

							;
							; ensure 8042 input buffer empty
							;
								call		Empty8042
								jnz			.Empty8042Error

							;
							; send 8042 command
							;
								mov			al,0d1h
								out			64h,al

							;
							; wait for 8042 to accept cmd
							;
								call		Empty8042
								jnz			.Empty8042Error

							;
							; output data
							;
								mov			al,0dfh
								out			60h,al
								call		Empty8042

							;
							; pulse output port
							;
								mov			al,0ffh
								out			64h,al
								call		Empty8042

.Empty8042Error:
								ret

;
; disable A20
;
DisableA20:
							;
							; check error status
							;
								mov			di,Empty8042Error
								cmp			byte [di],1
								jz			.Empty8042Error

							;
							; empty 8042 buffer
							;
								call		Empty8042
								jnz			.Empty8042Error

							;
							; send cmd to 8042
							;
								mov			al,0d1h
								out			64h,al

							;
							; wait 8042
							;
								call		Empty8042
								jnz			.Empty8042Error

							;
							; send data
							;
								mov			al,0ddh
								out			60h,al

							;
							; wait 8042
							;
								call		Empty8042

							;
							; pulse output port
							;
								mov			al,0ffh
								out			64h,al
								call		Empty8042

.Empty8042Error:
								ret

;
; wait for 8042
;
Empty8042:
							;
							; setu loop counter
							;
								sub			cx,cx

.EmptyLoop:
							;
							; read status
							;
								in			al,64h

							;
							; delay
							;
								jmp			.Next1

.Next1:
								jmp			.Next2

.Next2:
								jmp			.Next3

.Next3:
								jmp			.Next4

.Next4:
							;
							; test buffer full
							;
								and			al,2
								loopnz		.EmptyLoop
								cmp			cx,0
								jnz			.Return
								mov			di,Empty8042Error
								mov			byte [di],1
								ret

.Return:
								add			al,2
								ret

Empty8042Error:					db			0