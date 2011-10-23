;********************************************************************
;	created:	23:9:2008   02:03
;	file:		common.inc
;	author:		tiamo
;	purpose:	constant and macro
;********************************************************************

FAT_START_OFFSET				equ			3
LINUX_HEAD_OFFSET				equ			1f1h
LINUX_START_OFFSET				equ			200h
MOVE_EXT_MEMORY_LEN				equ			4000h

BOOT_SEGMENT					equ			07c0h
BOOT_CODE_OFFSET				equ			7c00h
HPFS_BOOT2_SEGMENT				equ			0d00h
HPFS_BOOT2_OFFSET				equ			256h
ROOT_DIR_SEGMENT				equ			1000h
SU_SEGMENT						equ			2000h
SU_FLAT_ADDRESS					equ			20000h
SYSTEM_STRUCTS_SEGMENT			equ			1700h
SYSTEM_STRUCTS_BASE_PA			equ			17000h
MEMORY_MAP_SEGMENT				equ			7000h
MEMORY_MAP_FLAT_ADDRESS			equ			70000h
LOADER_STACK					equ			061ffch
EXPORT_STACK					equ			DataStart + 7ffeh
NT_DETECT_SEGMENT				equ			1000h

NULL_Selector					equ			0h
KeCodeSelector					equ			8h
KeDataSelector					equ			10h
UsCodeSelector					equ			18h
UsDataSelector					equ			20h
TSS_Selector					equ			28h
PCR_Selector					equ			30h
TEP_Selector					equ			38h
BDA_Selector					equ			40h
KeLdtSelector					equ			48h
DblFltTskSelector				equ			50h
SuCodeSelector					equ			58h
SuDataSelector					equ			60h
VideoSelector					equ			68h
GDT_AliasSelector				equ			70h
DbCodeSelector					equ			78h
DbDataSelector					equ			80h
DebugUseSelector				equ			88h
ReservedSelector				equ			90h

struc E820FRAME
.ErrorFlag						resd		1
.Key							resd		1
.DescSize						resd		1
.BaseAddrLow					resd		1
.BaseAddrHigh					resd		1
.SizeLow						resd		1
.SizeHigh						resd		1
.MemoryType						resd		1
endstruc

E820FRAME_DESC_SIZE				equ			E820FRAME_size - E820FRAME.BaseAddrLow

struc MEMORY_DESCRIPTOR
.BlockBase						resd		1
.BlockSize						resd		1
endstruc

%macro EXPORT_PROLOGUE			1-2			0
%1:
								[bits 32]
								push		dword SuCodeSelector
								push		dword %%Jmp16Bits
								retf
%%Jmp16Bits:
								[bits 16]
								push		ebp
								push		ebx
								push		esi
								push		edi
								mov			ebx,esp
								mov			ax,SuDataSelector
								mov			ds,ax
								mov			ss,ax
								mov			sp,EXPORT_STACK
								push		ebx

								%assign		EXPORT_PARAMS_LENGTH	%2
%if %2 <> 0
								mov			ecx,(%2) >> 1
								mov			esi,ebx
								add			esi,20
								push		KeDataSelector
								pop			ds
								push		ss
								pop			es
								sub			sp,%2
								xor			edi,edi
								mov			di,sp

%%LoopCopy:
								mov			ax,[esi]
							es	mov			[edi],ax
								add			esi,2
								add			edi,2
								loop		%%LoopCopy
								push		es
								pop			ds
%endif
								call		RealMode
%endmacro

%macro EXPORT_EPILOGUE			0
							%if EXPORT_PARAMS_LENGTH <> 0
								add			sp,EXPORT_PARAMS_LENGTH
							%endif
								push		eax
								push		1
								call		EnableProtectPaging
								add			sp,2
								pop			eax
								pop			ebx
								mov			dx,KeDataSelector
								mov			ds,dx
								mov			es,dx
								mov			ss,dx
								mov			esp,ebx
								pop			edi
								pop			esi
								pop			ebx
								pop			ebp
								pop			edx
								push		dword KeCodeSelector
								push		edx
							o32	retf
								%assign		EXPORT_PARAMS_LENGTH	0
%endmacro

%macro BCD_TO_BIN 0
								xor			ah,ah
								rol			ax,4
								ror			al,4
								aad
%endmacro
