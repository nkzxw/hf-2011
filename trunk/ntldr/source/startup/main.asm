;********************************************************************
;	created:	22:9:2008   22:56
;	file:		main.asm
;	author:		tiamo
;	purpose:	main
;********************************************************************

%include "common.inc"

;
; 16 bits code section
;
[bits 16]
[section .text]
								org			0
;
; if we have booted from M$'s ntfs,cdfs,hpfs boot code,we are here
;
FileStart:
NtfsStart:
								jmp			RealMain

;
; if we have booted from M$'s fat boot code,we are here
;
%if $ - NtfsStart > FAT_START_OFFSET
	%error fat start must be at offset 3
%endif
								times		(FAT_START_OFFSET + NtfsStart - $)	db		90
FatStart:
							;
							; setup ds,es,ss,sp
							;
								mov			ax,SU_SEGMENT
								mov			ds,ax
								mov			es,ax
								mov			ss,ax
								mov			sp,SuStack

							;
							; output unsupported string
							;
								mov			si,.FatNotSupportedMsg
								call		OutputMessage

							;
							; dead loop
							;
								jmp			$
								
.FatNotSupportedMsg				db			"this code can't be run from a fat drive",0

OutputMessage:
								push		si
								xor			bx,bx
								mov			ah,0eh
								mov			al,[si]
								int			10h
								pop			si
								inc			si
								cmp			al,0
								jnz			OutputMessage
								ret

;
; copy cmd line here
;
CommandLineFromGrub:			times		256		db		0
CommandLineFromGrubEnd:
BootDrive:						dw			80h

;
; linux header at 1f1h
;
%if	$ - NtfsStart > LINUX_HEAD_OFFSET
	%error linux header must be at offset 1f1
%endif

								times		LINUX_HEAD_OFFSET - $ + NtfsStart	db		0

SetupSects						db			((RealMain - LinuxStart + 511) & 0fe00h) >> 9
								dw			0
								dw			0
								dw			0
								dw			0
								dw			0
								dw			0
								dw			0aa55h

%if	$ - NtfsStart <> LINUX_START_OFFSET
	%error linux start offset error
%endif

;
; if we have booted from grub as a linux kernel,we are here
;
LinuxStart:
								jmp			short LinuxMoveSelf

								db			"HdrS"
								dw			0203h
								dd			0
								dd			1000h
								db			0
								db			1
								dw			0
Code32Start						dd			100000h
								dd			0
								dd			0
								dd			0
								dw			0
								dw			0
CommandLine						dd			0
								dd			0ffffffffh
;
; move the whole program to SU_SEG:0
;
; grub will give us:
;	CS	= loaded segment + 20h
;	DS	= loaded segment
;	ES	= DS
;	SS	= CS
;	SP	= 9000h
;
LinuxMoveSelf:
							;
							; save dx
							;

								mov			[BootDrive],dx

							;
							; copy cmd line
							;
								mov			esi,[CommandLine]
								cmp			esi,0
								jz			.CheckLoadSegment2000

								mov			ax,ds
								mov			es,ax
								movzx		edi,ax
								shl			edi,4
								add			edi,CommandLineFromGrub
								mov			ecx,CommandLineFromGrubEnd - CommandLineFromGrub
								call		MoveExtendedMemory

.CheckLoadSegment2000:
							;
							; if we are loaded at 2000:0000,then the real mode code is already here
							;
								mov			ax,ds
								cmp			ax,SU_SEGMENT
								jz			.MoveProtectModeCode

.MoveRealModeCode:
							;
							; move real mode sectors first,its size will be ([SetupSects] + 1) * 512
							;
								mov			cx,[SetupSects]
								inc			cx
								shl			cx,7

							;
							; setup src,dst selector
							;
								mov			ax,SU_SEGMENT
								mov			es,ax
								xor			si,si
								mov			di,si

							;
							; move it
							;
								cld
								rep			movsd

							;
							; jmp to SU_SEGMENT
							;
								mov			ds,ax
								mov			ax,BOOT_SEGMENT
								mov			ss,ax
								mov			sp,0
								jmp			SU_SEGMENT:.MoveProtectModeCode

.MoveProtectModeCode:
							;
							; then,move the rest high loaded data
							;
								mov			ax,[SetupSects]
								inc			ax
								shl			ax,9
								movzx		eax,ax

							;
							; left bytes will be FileEnd - FileStart - ([SetupSects] + 1) * 512
							;
								mov			ecx,FileEnd - FileStart
								sub			ecx,eax

							;
							; src will be [Code32Start]
							;
								mov			esi,[Code32Start]

							;
							; dst is (SU_SEGMENT << 4) + ([SetupSects] + 1) * 512
							;
								mov			edi,SU_FLAT_ADDRESS
								add			edi,eax

							;
							; es:si = gdt 
							;
								push		ds
								pop			es

							;
							; move it by using int15,87 bios call
							;
								call		MoveExtendedMemory

							;
							; set boot drive
							;
								mov			dx,[BootDrive]

							;
							; jmp to SU_SEGMENT:real_main
							;
								mov			ax,BOOT_SEGMENT
								mov			ds,ax
								mov			ax,ROOT_DIR_SEGMENT
								mov			es,ax
								xor			si,si
								xor			di,di
								mov			dx,0180h
								mov			ss,si
								mov			sp,BOOT_SEGMENT << 4
								jmp			SU_SEGMENT:RealMain.KeepDH

;
; move extended memory
;
MoveExtendedMemory:
							;
							; round up to 2
							;
								inc			ecx
								and			cl,0feh

							;
							; check direction
							;
								cmp			edi,esi
								ja			.MoveDown
								jb			.LoopMove
								ret

.MoveDown:
								add			esi,ecx
								add			edi,ecx

.LoopMove:
							;
							; save count
							;
								push		ecx

							;
							; check move length
							;
								cmp			ecx,MOVE_EXT_MEMORY_LEN
								jbe			.CheckMoveDirection
								mov			ecx,MOVE_EXT_MEMORY_LEN

.CheckMoveDirection:
								push		ecx
								cmp			edi,esi
								jb			.SetupGdt

								sub			esi,ecx
								sub			edi,ecx

.SetupGdt:
							;
							; save pointers
							;
								push		esi
								push		edi

							;
							; setup src
							;
								mov			eax,esi
								shr			eax,16
								mov			[.SrcLow],si
								mov			[.SrcMid],al
								mov			[.SrcHi],ah

							;
							; setup dst
							;
								mov			eax,edi
								shr			eax,16
								mov			[.DstLow],di
								mov			[.DstMid],al
								mov			[.DstHi],ah

							;
							; ecx = length is WORD
							;
								shr			ecx,1

							;
							; int15,87 move memory
							;
								mov			ah,87h
								mov			si,.Gdt
								int			15h

							;
							; restore pointers
							;
								pop			edi
								pop			esi

							;
							; restore current copy length
							;
								pop			eax

							;
							; restore total length
							;
								pop			ecx
								jnc			.TryNextBlock
								mov			si,.MoveExtMemoryFailedMsg
								call		OutputMessage
								jmp			$

.TryNextBlock:
								cmp			edi,esi
								ja			.CheckLeftLength
								add			esi,eax
								add			edi,eax

.CheckLeftLength:
								sub			ecx,eax
								jnz			.LoopMove

							;
							; return to caller
							;
								ret

.MoveExtMemoryFailedMsg			db			"int15,87 failed",0

.Gdt							dd			0,0,0,0

								dw			0ffffh
.SrcLow							dw			0
.SrcMid							db			0
								db			93h,0
.SrcHi							db			0

								dw			0ffffh
.DstLow							dw			0
.DstMid							db			0
								db			93h,0
.DstHi							db			0

								dd			0,0,0,0

								times		512 - (($ - FileStart) & 511) db 0
%if $ - FileStart <> 1024
	%error linux startup code is too big
%endif

;
; main function
;
; currently
;	CS	= 2000h		startup module code segment
;	IP	= 0000h		startup module code offset
;	DS	= 07c0h		bios parameter block segment
;	SI	= 0000h		bios parameter block offset
;	ES	= 1000h		root diretory segment
;	di	= 0000h		root directory offset
;	SS	= 0000h		stack segment
;	SP	= 7c00h		stack pointer
;
RealMain:
							;
							; clear dh
							;
								xor			dh,dh

.KeepDH:
							;
							; save boot drive
							;
							cs	mov			[FsContext],dx

							;
							; setup su stack
							;
								mov			ax,cs
								mov			ss,ax
								mov			sp,SuStack

							;
							; setup ds,es
							;
								mov			ds,ax
								mov			es,ax


							;
							; zero high 16bits of ebp and esp
							;
								xor			bp,bp
								movzx		ebp,bp
								movzx		esp,sp

							;
							; call to SuMain
							;
								call		SuMain
								jmp			$
;
; include other files
;
%include "sumain.asm"
%include "a20.asm"
%include "display.asm"
%include "memory.asm"
%include "trap.asm"
%include "modeswt.asm"
%include "export.asm"
%include "data.asm"

FileEnd: