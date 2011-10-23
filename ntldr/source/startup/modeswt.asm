;********************************************************************
;	created:	23:9:2008   03:07
;	file:		modeswt.asm
;	author:		tiamo
;	purpose:	mode switch
;********************************************************************

;
; switch to protect mode
;
EnableProtectPaging:
							;
							; clear eflags
							;
								push		dword 0
								popfd

							;
							; enable paging?
							;
								mov			bx,sp
								mov			dx,[bx + 2]

							;
							; clear es,gs
							;
								xor			ax,ax
								mov			gs,ax
								mov			es,ax

							;
							; setup fs
							;
								push		PCR_Selector
								pop			fs

							;
							; load gdt idt
							;
								cli
								lgdt		[GDTregister]
								lidt		[IDTregister]

							;
							; enable protect mode
							;
								mov			eax,cr0
								or			dx,dx
								jz			.EnableProtectMode
								or			eax,80000000h

.EnableProtectMode:
								or			eax,1
								mov			cr0,eax

								align		4
								jmp			.FlushPrefetchQueue

.FlushPrefetchQueue:
							;
							; load cs
							;
								push		SuCodeSelector
								push		.RestartWithSuCodeSelector
								retf

.RestartWithSuCodeSelector:
							;
							; load ds,ss
							;
								mov			ax,SuDataSelector
								mov			ds,ax
								mov			ss,ax

							;
							; load zero ldt
							;
								xor			bx,bx
								lldt		bx

							;
							; load ts and return
							;
								or			dx,dx
								jnz			.Return
								mov			bx,TSS_Selector
								ltr			bx

.Return:
								retn

;
; switch to real mode
;
RealMode:
							;
							; save gdt,idt
							;
								sgdt		[GDTregister]
								sidt		[IDTregister]

							;
							; setup 16bits selectors
							;
								mov			ax,SuDataSelector
								mov			es,ax
								mov			fs,ax
								mov			gs,ax

							;
							; go
							;
								mov			eax,cr0
								and			eax,7ffffffeh
								mov			cr0,eax

							;
							; flush prefetch queue
							;
								align		4
								jmp			short .FlushTLB
								nop
								nop
								nop
.FlushTLB:
							;
							; DO NOT modify cr3
							;
								mov			eax,cr3
								nop
								nop
								nop
								nop
								mov			cr3,eax

							;
							; far jmp,reload cs
							;
								jmp			SU_SEGMENT:.Next

.Next:
							;
							; setup realmode segments
							;
								mov			ax,SU_SEGMENT
								mov			ds,ax
								mov			ss,ax

							;
							; load zero idt(realmode interrupt vector)
							;
								lidt		[IDTregisterZero]
								sti
								retn