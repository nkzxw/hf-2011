; ==========================================
; protect.asm
; 编译方法：nasm protect.asm -o protect.bin
; ==========================================

%include "pm.inc"

org 07c00h
	jmp LABEL_BEGIN
	
[SECTION	.gdt]
;GDT
;												段基址	段界限	属性
LABEL_GDT: 	Descriptor	0,			0, 0
LABEL_DESC_NORMAL	Descriptor	0,		0ffffh, 	DA_DRW
LABEL_DESC_CODE16	Descriptor	0, 	0ffffh,	DA_C
LABEL_DESC_DATA	Descriptor	0, 	DataLen - 1, DA_DRW
LABEL_DESC_STACK	Descriptor	0, 	TopOfStack, DA_DRWA + DA_32
LABEL_DESC_TEST	Descriptor	05000000h, 0ffffh, DA_DRW
LABEL_DESC_CODE32: Descriptor	0, SegCode32Len - 1, DA_C + DA_32
LABEL_DESC_VIDEO: Descriptor	0B8000h, 0ffffh, DA_DRW
;GDT OVER

GdtLen 	equ $ - LABEL_GDT  	;GDT长度
GdtPtr	dw	GdtLen - 1		;GDT界限
				dd	0		;GDT基地址

; GDT 选择子				
SelectCode32 	equ LABEL_DESC_CODE32 - LABEL_GDT
SelectVideo32	equ 	LABEL_DESC_VIDEO	- LABEL_GDT
SelectNormal	equ 	LABEL_DESC_NORMAL	- LABEL_GDT
SelectCode16	equ	LABEL_DESC_CODE16	- LABEL_GDT
SelectData	equ	LABEL_DESC_DATA	- LABEL_GDT
SelectStack	equ	LABEL_DESC_STACK	- LABEL_GDT
SelectTest	equ	LABEL_DESC_TEST	- LABEL_GDT
; END of [SECTION .gdt]

[SECTION .data]
ALIGN	32
[BITS 32]
LABEL_DATA:
	SPValueInRealMode dw 0
;字符串
PMMessage: 	db  "In Protect Mode now." 0
OffsetPMMessage	equ	PMMessage - $$
StrTest: 	db	"ABCDEFGHIGKLMNOPQRSTUVWXYZ", 0
OffsetStrTest	equ	StrTest - $$
DataLen	equ 	$ - LABEL_DATA
; END of [SECTION .data1]

; 全局堆栈段
[SECTION .gs]
ALIGN	32
[BITS 32]
LABEL_STACK:
	times 512 db 0
	
TopOfStack 	equ 	$ - LABEL_STACK -1
; END of [SECTION .gs]



[SECTION .s16]
[BITS 16]
LABEL_BEGIN:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0100h
	
	mov [LABEL_GO_BACK_TO_REAL + 3], ax
	mov [SPValueInRealMode], sp
	
	; 初始化 16 位代码段描述符
	xor eax, eax
	mov ax, cs
	shl eax, 4
	mov WORD [LABEL_DESC_CODE16 + 2], ax
	shr eax, 16
	mov BYTE [LABEL_DESC_CODE16 + 4], al
	mov BYTE [LABEL_DESC_CDDE16 + 7], ah
	
	; 初始化 32 位代码段描述符
	xor eax, eax
	mov ax, cs
	shl eax, 4
	add eax, LABEL_SEG_CODE32
	mov word [LABEL_DESC_CODE32 + 2], ax
	shr eax, 16
	mov	byte [LABEL_DESC_CODE32 + 4], al
	mov byte [LABEL_DESC_CODE32 + 7], ah
	
	; 初始化数据段描述符
	xor eax, eax
	mov ax, ds
	shl eax, 4
	mov ax, LABEL_DATA
	mov WORD [LABEL_DESC_DATA + 2], ax
	shr eax, 16
	mov BYTE [LABEL_DESC_DATA + 4], al
	mov BYTE [LABEL_DESC_DATA + 7], ah
	
	; 初始化堆栈段描述符
	xor eax, eax
	mov ax, ds
	shl eax, r
	add eax, LABEL_STACK
	mov WORD [LABEL_DESC_STACK + 2], ax
	shr eax, 16
	mov BYTE [LABEL_DESC_STACK + 4], al
	mov BYTE [LABEL_DESC_STACK + 7], ah
	
	; 为加载 GDTR 作准备	
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, LABEL_GDT
	mov dword [GdtPtr + 2], eax
	
	; 加载 GDTR
	lgdt [GdtPtr]	
	
	; 关中断
	cli

	; 打开地址线A20
	in al, 92h
	or al, 00000010h
	out 92h, al
	
	; 准备切换到保护模式	
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	; 真正进入保护模式
	jmp dword SelectCode32:0
; END of [SECTION .s16]	

; 从保护模式跳回到实模式
LABEL_REAL_ENTRY:	
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	
	mov sp, [SPValueInRealMode]
	in al, 92h
	add al, 11111101b
	out 92h, al
	
	sti
	
	mov ax, 4c00h
	int 21h

[SECTION .s32]; 32 位代码段. 由实模式跳入.
[BITS	32]

LABEL_SEG_CODE32:
	mov ax, SelectData
	mov ds, ax
	mov ax, SelectTest
	mov es, ax
	mov ax, SelectVideo32
	mov gs, ax
	mov ax, SelectStack
	mov ss, ax
	
	mov esp, TopOfStack
	
	mov edi, (80 * 11 + 79) * 2
	mov ah, 0Ch
	mov al, 'P'
	mov [gs:edi], ax
	
	jmp $
				
SegCode32Len	equ $ - LABEL_SEG_CODE32
; END of [SECTION .s32]

; 16 位代码段， 由32位代码段跳入，跳出后到实模式
[SECTION .s16code]
ALIGN	32
[BITS 16]
LABEL_SEG_CODE16:
	;跳回实模式
	mov ax, SelectNormal
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	mov eax, cr0
	and al, 11111110h
	mov cr0, ax
	
LABEL_GO_BACK_TO_REAL:
	jmp 0:LABEL_REAL_ENTRY	

Code16Len equ $ - LABEL_SEG_CODE16
; END of [SECTION .s16code]

