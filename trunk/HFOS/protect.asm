; ==========================================
; protect.asm
; ���뷽����nasm protect.asm -o protect.bin
; ==========================================

%include "pm.inc"

org 07c00h
	jmp LABEL_BEGIN
	
[SECTION	.gdt]
;GDT
;												�λ�ַ	�ν���	����
LABEL_GDT: 	Descriptor	0,			0, 0
LABEL_DESC_NORMAL	Descriptor	0,		0ffffh, 	DA_DRW
LABEL_DESC_CODE16	Descriptor	0, 	0ffffh,	DA_C
LABEL_DESC_DATA	Descriptor	0, 	DataLen - 1, DA_DRW
LABEL_DESC_STACK	Descriptor	0, 	TopOfStack, DA_DRWA + DA_32
LABEL_DESC_TEST	Descriptor	05000000h, 0ffffh, DA_DRW
LABEL_DESC_CODE32: Descriptor	0, SegCode32Len - 1, DA_C + DA_32
LABEL_DESC_VIDEO: Descriptor	0B8000h, 0ffffh, DA_DRW
;GDT OVER

GdtLen 	equ $ - LABEL_GDT  	;GDT����
GdtPtr	dw	GdtLen - 1		;GDT����
				dd	0		;GDT����ַ

; GDT ѡ����				
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
;�ַ���
PMMessage: 	db  "In Protect Mode now." 0
OffsetPMMessage	equ	PMMessage - $$
StrTest: 	db	"ABCDEFGHIGKLMNOPQRSTUVWXYZ", 0
OffsetStrTest	equ	StrTest - $$
DataLen	equ 	$ - LABEL_DATA
; END of [SECTION .data1]

; ȫ�ֶ�ջ��
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
	
	; ��ʼ�� 16 λ�����������
	xor eax, eax
	mov ax, cs
	shl eax, 4
	mov WORD [LABEL_DESC_CODE16 + 2], ax
	shr eax, 16
	mov BYTE [LABEL_DESC_CODE16 + 4], al
	mov BYTE [LABEL_DESC_CDDE16 + 7], ah
	
	; ��ʼ�� 32 λ�����������
	xor eax, eax
	mov ax, cs
	shl eax, 4
	add eax, LABEL_SEG_CODE32
	mov word [LABEL_DESC_CODE32 + 2], ax
	shr eax, 16
	mov	byte [LABEL_DESC_CODE32 + 4], al
	mov byte [LABEL_DESC_CODE32 + 7], ah
	
	; ��ʼ�����ݶ�������
	xor eax, eax
	mov ax, ds
	shl eax, 4
	mov ax, LABEL_DATA
	mov WORD [LABEL_DESC_DATA + 2], ax
	shr eax, 16
	mov BYTE [LABEL_DESC_DATA + 4], al
	mov BYTE [LABEL_DESC_DATA + 7], ah
	
	; ��ʼ����ջ��������
	xor eax, eax
	mov ax, ds
	shl eax, r
	add eax, LABEL_STACK
	mov WORD [LABEL_DESC_STACK + 2], ax
	shr eax, 16
	mov BYTE [LABEL_DESC_STACK + 4], al
	mov BYTE [LABEL_DESC_STACK + 7], ah
	
	; Ϊ���� GDTR ��׼��	
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, LABEL_GDT
	mov dword [GdtPtr + 2], eax
	
	; ���� GDTR
	lgdt [GdtPtr]	
	
	; ���ж�
	cli

	; �򿪵�ַ��A20
	in al, 92h
	or al, 00000010h
	out 92h, al
	
	; ׼���л�������ģʽ	
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	; �������뱣��ģʽ
	jmp dword SelectCode32:0
; END of [SECTION .s16]	

; �ӱ���ģʽ���ص�ʵģʽ
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

[SECTION .s32]; 32 λ�����. ��ʵģʽ����.
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

; 16 λ����Σ� ��32λ��������룬������ʵģʽ
[SECTION .s16code]
ALIGN	32
[BITS 16]
LABEL_SEG_CODE16:
	;����ʵģʽ
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

