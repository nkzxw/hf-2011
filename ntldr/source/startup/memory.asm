;********************************************************************
;	created:	23:9:2008   02:15
;	file:		memory.asm
;	author:		tiamo
;	purpose:	memory descriptor list
;********************************************************************

;
; get E820 frame
;
Int15E820:
							;
							; make stack frame
							;
								push		ebp
								mov			bp,sp

							;
							; get frame struct address
							;
								mov			bp,[bp + 6]

							;
							; save registers
							;
								push		es
								push		edi
								push		esi
								push		ebx

							;
							; setup input registers,and call bios int 15
							;
								push		ss
								pop			es
								mov			ebx,[bp + E820FRAME.Key]
								mov			ecx,[bp + E820FRAME.DescSize]
								lea			di, [bp + E820FRAME.BaseAddrLow]
								mov			eax,0e820h
								mov			edx,'PAMS'
								int			15h

							;
							; update with result
							;
								mov			[bp + E820FRAME.Key],ebx
								mov			[bp + E820FRAME.DescSize],ecx

							;
							; if carry ecx = -1,else 0
							;
								sbb			ecx,ecx
								sub			eax,'PAMS'
								or			ecx,eax

							;
							; non-zero indicates error
							;
								mov			[bp + E820FRAME.ErrorFlag],ecx

							;
							; restore registers
							;
								pop			ebx
								pop			esi
								pop			edi
								pop			es
								pop			ebp
								retn
;
; build memory map
;
ConstructMemoryDescriptors:
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								sub			sp,E820FRAME_size
								and			sp,0fffch

							;
							; save registers
							;
								push		esi
								push		edi
								push		es
								push		bx

							;
							; init the first entry in the list to zero
							;
								les			bx,[MemoryDescriptorList]
							es	mov			dword [bx + MEMORY_DESCRIPTOR.BlockSize],0
							es	mov			dword [bx + MEMORY_DESCRIPTOR.BlockBase],0

							;
							; check e820 is supported
							;
								mov			dword [esp + E820FRAME.Key],0
								mov			dword [esp + E820FRAME.DescSize],E820FRAME_DESC_SIZE
								lea			ax,[esp]
								push		ax
								call		Int15E820
								add			sp,2
								cmp			dword [esp + E820FRAME.ErrorFlag],0
								jnz			.IntE820Failed1
								cmp			dword [esp + E820FRAME.DescSize],E820FRAME_DESC_SIZE
								jb			.IntE820Failed1

							;
							; get memory map from bios
							;
								mov			dword [esp + E820FRAME.Key],0

.GetE820Frame:
								mov			dword [esp + E820FRAME.DescSize],E820FRAME_DESC_SIZE
								lea			ax,[esp]
								push		ax
								call		Int15E820
								add			sp,2

								cmp			dword [esp + E820FRAME.ErrorFlag],0
								jnz			.IntE820Failed
								cmp			dword [esp + E820FRAME.DescSize],E820FRAME_DESC_SIZE
								jb			.IntE820Failed
								jmp			.CheckReturnedDescriptor

.IntE820Failed1:
								jmp			.IntE820Failed

.CheckReturnedDescriptor:
							;
							; if the upper 32 bits of base address is non-zero,then this range is entirely above 4GB,ignore it
							;
								cmp			dword [esp + E820FRAME.BaseAddrHigh],0
								jnz			.CheckFinished

							;
							; get start address and end address
							;
								mov			esi,[esp + E820FRAME.BaseAddrLow]
								mov			edi,[esp + E820FRAME.SizeLow]
								add			edi,esi
								dec			edi

							;
							; address wrapped ?
							;
								cmp			edi,esi
								jnb			.CheckInsertThisOne
								mov			edi,0ffffffffh

.CheckInsertThisOne:
							;
							; check memory type
							;
								cmp			dword [esp + E820FRAME.MemoryType],1
								jnz			.CheckFinished

							;
							; insert it into list
							;
								sub			edi,esi
								inc			edi
								push		edi
								push		esi
								call		InsertDescriptor
								add			sp,8

.CheckFinished:
								cmp			dword [esp + E820FRAME.Key],0
								jnz			.GetE820Frame
								mov			ax,1
								jmp			.Return

.IntE820Failed:
								xor			ax,ax

.Return:
							;
							; restore registers,and return
							;
								pop			bx
								pop			es
								pop			edi
								pop			esi
								mov			sp,bp
								pop			bp
								retn

;
; insert descriptor into list
;
InsertDescriptor:
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp

							;
							; save registers
							;
								push		esi
								push		bx
								push		es

							;
							; get start(eax),size(ecx) and end(edx)
							;
								mov			eax,[ebp + 4]
								mov			ecx,[ebp + 8]
								mov			edx,ecx
								add			edx,eax

							;
							; get memory list
							;
								les			bx,[MemoryDescriptorList]

.CheckEndOfList:
							;
							; check this is the last one
							;
							es	cmp			dword [bx + MEMORY_DESCRIPTOR.BlockSize],0
								je			.InsertAtEnd

							;
							; check the new one is contiguous with the current one
							;
							es	cmp			[bx + MEMORY_DESCRIPTOR.BlockBase],edx
								jnz			.CheckContiguous2

							;
							; merge them
							;
							es	mov			[bx + MEMORY_DESCRIPTOR.BlockBase],eax
							es	add			[bx + MEMORY_DESCRIPTOR.BlockSize],ecx
								jmp			.Return

.CheckContiguous2:
							;
							; check the new one is contiguous with the current one
							;
							es	mov			esi,[bx + MEMORY_DESCRIPTOR.BlockBase]
							es	add			esi,[bx + MEMORY_DESCRIPTOR.BlockSize]
								cmp			esi,eax
								jnz			.CheckNextOne

.CheckNextOne:
								add			bx,MEMORY_DESCRIPTOR_size
								jmp			.CheckEndOfList

							;
							; append this one to the end of list
							;
.InsertAtEnd:
							es	mov			[bx + MEMORY_DESCRIPTOR.BlockBase],eax
							es	mov			[bx + MEMORY_DESCRIPTOR.BlockSize],ecx

							;
							; create a new end-of-list
							;
								add			bx,MEMORY_DESCRIPTOR_size
							es	mov			dword [bx + MEMORY_DESCRIPTOR.BlockBase],0
							es	mov			dword [bx + MEMORY_DESCRIPTOR.BlockSize],0
.Return:
								pop			es
								pop			bx
								pop			esi
								pop			bp
								retn

;
; move memory
;
MoveMemory:
								push		bp
								mov			bp,sp
								push		ds
								push		es
								push		esi
								push		edi

								mov			ecx,[bp + 0ch]
								mov			esi,[bp + 8]
								mov			edi,[bp + 4]
								shr			ecx,2
								mov			ax,KeDataSelector
								mov			ds,ax
								mov			es,ax
								cld
							a32	rep			movsd
								mov			ecx,[bp + 0ch]
								and			ecx,3
							a32	rep			movsb

								pop			edi
								pop			esi
								pop			es
								pop			ds
								pop			bp
								retn
;
; zero memory
;
ZeroMemory:
								push		bp
								mov			bp,sp
								push		ds
								push		es
								push		edi

								mov			ecx,[bp + 8]
								mov			edi,[bp + 4]
								shr			ecx,2
								mov			ax,KeDataSelector
								mov			ds,ax
								mov			es,ax
								xor			eax,eax
								cld
							a32	rep			stosd
								mov			ecx,[bp + 8]
								and			ecx,3
							a32	rep			stosb

								pop			edi
								pop			es
								pop			ds
								pop			bp
								retn