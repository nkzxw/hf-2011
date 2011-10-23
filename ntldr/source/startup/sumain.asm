;********************************************************************
;	created:	23:9:2008   01:46
;	file:		sumain.asm
;	author:		tiamo
;	purpose:	Main
;********************************************************************

;
; main function for startup module
;
SuMain:
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp

							;
							; clear screen
							;
								call		ClearScreen

							;
							; turn motor off
							;
								call		TurnMotorOff

							;
							; build memory descriptor list
							;
								call		ConstructMemoryDescriptors

							;
							; failed to build memory descriptor list?
							;
								cmp			ax,1
								jz			.SearchLowMemoryStart
								jmp			.BuildMemoryListFailed

.SearchLowMemoryStart:
							;
							; check low memory
							;
								les			bx,[MemoryDescriptorList]
.SearchLowMemory:
							;
							; try to find a descriptor start from 0
							;
							es	cmp			dword [bx + MEMORY_DESCRIPTOR.BlockBase],0
								jz			.FoundLowMemory
							es	cmp			dword [bx + MEMORY_DESCRIPTOR.BlockSize],0
								jz			.FoundLowMemory
								add			bx,MEMORY_DESCRIPTOR_size
								jmp			.SearchLowMemory

.FoundLowMemory:
							;
							; size of low memory must be greater than 512K
							;
							es	cmp			dword [bx + MEMORY_DESCRIPTOR.BlockBase],0
								jnz			.GetOsLoaderInfo
							es	cmp			dword [bx + MEMORY_DESCRIPTOR.BlockSize],512 * 1024
								jnb			.GetOsLoaderInfo
								jmp			.LowMemoryError

.GetOsLoaderInfo:
							;
							; get optional header
							;
								mov			si,EmbededOsloaderFileStart
								mov			ax,[si + 3ch]
								add			si,ax
								add			si,18h

							;
							; get image base(eax),image size(edx)
							;
								mov			eax,[si + 1ch]
								mov			edx,[si + 38h]

.RestartSearchOsLoaderMemory:
								les			bx,[MemoryDescriptorList]
.SearchOsLoaderMemory:
							es	cmp			dword [bx + MEMORY_DESCRIPTOR.BlockSize],0
								jz			.CheckLeftImageSize

							es	cmp			[bx + MEMORY_DESCRIPTOR.BlockBase],eax
								ja			.TryNextDescriptor

							es	mov			ecx,[bx + MEMORY_DESCRIPTOR.BlockSize]
							es	add			ecx,[bx + MEMORY_DESCRIPTOR.BlockBase]
								cmp			ecx,eax
								jbe			.TryNextDescriptor

								mov			ebx,ecx
								sub			ecx,eax
								cmp			ecx,edx
								jb			.CalcLeftImageSize
								xor			edx,edx
								jmp			.CheckLeftImageSize

.CalcLeftImageSize:
								sub			edx,ecx
								mov			eax,ebx
								jmp			.RestartSearchOsLoaderMemory

.TryNextDescriptor:
								add			bx,MEMORY_DESCRIPTOR_size
								jmp			.SearchOsLoaderMemory

.CheckLeftImageSize:
								cmp			edx,0
								ja			.OsLoaderMemoryFailed

							;
							; enable the A20 line for protect mode
							;
								call		EnableA20

							;
							; relocate x86 structures,(GDT, IDT, page directory,level page table).
							;
								call		Relocatex86Structures

							;
							; enable protect modes
							;
								push		0
								call		EnableProtectPaging
								add			sp,2

							;
							; relocate loader sections and build page table entries
							;
								call		RelocateLoaderSections

							;
							; transfer control to the OS loader
							;
								push		eax
								call		TransferToLoader
								add			sp,4

							;
							; return
							;
								pop			bp
								retn
.OsLoaderMemoryFailed:
								push		.OsLoaderMemoryFailedMsg
								call		puts
								add			sp,2
								jmp			$

.OsLoaderMemoryFailedMsg:		db			"Osloader memory failed!",0

.BuildMemoryListFailed:
								push		.BuildMemoryListFailedMsg
								call		puts
								add			sp,2
								jmp			$

.BuildMemoryListFailedMsg:		db			"unable to get memory map from bios,e802 failed!",0

.LowMemoryError:
								push		.LowMemoryErrorMsg
								call		puts
								add			sp,2
								jmp			$

.LowMemoryErrorMsg:				db			"Low Memory Error!",0

;
; turn motor off
;
TurnMotorOff:
								mov				al,0ch
								mov				dx,3f2h
								out				dx,al
								retn

;
; copy x86 structure to SYSTEM_STRUCTS_BASE_PA
;
Relocatex86Structures:
							;
							; save registers
							;
								push		si
								push		di
								push		es

							;
							; ds:si point to src
							;
								mov			si,Beginx86Relocation

							;
							; es:di point to dst
							;
								mov			ax,SYSTEM_STRUCTS_SEGMENT
								mov			es,ax
								xor			di,di

							;
							; cx = byte counts
							;
								mov			cx,Endx86Relocation - Beginx86Relocation
								cmp			cx,0
								jz			.Return
.CopyCheck:
								mov			al,[si]
							es	mov			[di],al
								inc			si
								inc			di
								dec			cx
								jnz			.CopyCheck

.Return:
							;
							; restore registers,and return
							;
								pop			es
								pop			di
								pop			si
								retn

;
; transfer control to osloader
;
TransferToLoader:
							;
							; get entry point
							;
								mov			ebx,[esp + 2]

							;
							; setup loader's stack
							;
								mov			cx,KeDataSelector
								mov			ss,cx
								mov			esp,LOADER_STACK

							;
							; setup ds,es
							;
								mov			ds,cx
								mov			es,cx

							;
							; push boot record
							;
								push		dword BootRecord + SU_FLAT_ADDRESS

							;
							; push dummy return address
							;
								push		dword 10101010h

							;
							; jmp to loader
							;
								push		dword KeCodeSelector
								push		ebx
							o32	retf

;
; relocate osloader
;
RelocateLoaderSections:
							;
							; save registers
							;
								push		ebp
								push		esi
								push		edi
								push		ebx

							;
							; get dos headers and check MZ
							;
								mov			si,EmbededOsloaderFileStart
								cmp			word [si],'MZ'
								jz			.GetAndCheckPEHeader
								jmp			.InvalidOsloaderFileData

.GetAndCheckPEHeader:
							;
							; e_lfanew is at offset 3c,check PE
							;
								mov			ax,[si + 3ch]
								add			si,ax
								cmp			dword [si],'PE'
								jz			.CheckExeImage
								jmp			.InvalidOsloaderFileData

.CheckExeImage:
							;
							; check Characteristics with IMAGE_FILE_EXECUTABLE_IMAGE bit set
							;
								test		byte [si + 16h],2
								jnz			.CheckI386Machine
								jmp			.InvalidOsloaderFileData

.CheckI386Machine:
							;
							; check Machine with IMAGE_FILE_MACHINE_I386
							;
								cmp			word [si + 4],14ch
								jz			.GetSectionHeader
								jmp			.InvalidOsloaderFileData

.GetSectionHeader:
							;
							; get optional header(edi),we need image base(ebx) and entry point
							;
								mov			edi,esi
								add			edi,18h
								mov			ebx,[edi + 1ch]

							;
							; save image base
							;
								mov			[BootRecord.OsLoaderBase],ebx

							;
							; set command line ptr
							;
								mov			eax,CommandLineFromGrub
								cmp			byte [eax],0
								jz			.CopyFileHeader

								mov			dword [EmbededOsloaderFileStart + 1ch],SU_FLAT_ADDRESS + CommandLineFromGrub
								mov			dword [EmbededOsloaderFileStart + 20h],CommandLineFromGrubEnd - CommandLineFromGrub - 1

.CopyFileHeader:
							;
							; copy file header
							;
								mov			eax,[edi + 3ch]
								push		eax
								push		dword EmbededOsloaderFileStart + SU_FLAT_ADDRESS
								push		ebx
								call		MoveMemory
								add			esp,12

							;
							; get exports dir and save it
							;
								mov			eax,[edi + 60h]
								add			eax,ebx
								mov			[BootRecord.OsLoaderExports],eax

							;
							; get entry point(eax)
							;
								mov			ebp,[edi + 10h]
								add			ebp,ebx

							;
							; get sections count and point esi to the first section header
							;
								movzx		ecx,word [si + 6]
								mov			si,[si + 14h]
								add			esi,edi
								mov			edi,ecx

							;
							; currently
							;	ebx	= image base
							;	edi	= section count
							;	esi = first section

								cmp			edi,0
								jnz			.ProcessSection
								jmp			.Return

.ProcessSection:
							;
							; get dst(edx)
							;
								mov			edx,ebx
								add			edx,[esi + 0ch]

							;
							; get src(eax)
							;
								mov			eax,[esi + 14h]

							;
							; get virtual size(ebx),raw size(ecx)
							;
								mov			ebx,[esi + 8]
								mov			ecx,[esi + 10h]

							;
							; if virtual size is 0,set it to raw size
							;
								cmp			ebx,0
								jnz			.FixRawSize
								mov			ebx,ecx

.FixRawSize:
							;
							; if raw data is empty,force raw size to zero
							;
								cmp			eax,0
								jnz			.FixRawSize2
								xor			ecx,ecx
								jmp			.UpdateStartEnd
.FixRawSize2:
							;
							; if raw size is bigger than virtual size,set it to virtual size
							;
								cmp			ecx,ebx
								jb			.UpdateStartEnd
								mov			ecx,ebx

.UpdateStartEnd:
							;
							;	ebx = virtual size
							;	ecx	= raw size
							;	esi = section header
							;	edi	= left section count
							;	eax	= file data offset
							;	edx	= dst address
							;
								cmp			[BootRecord.OsLoaderStart],edx
								jbe			.UpdateStartEnd2
								mov			[BootRecord.OsLoaderStart],edx

.UpdateStartEnd2:
								cmp			[BootRecord.OsLoaderEnd],edx
								jae			.CopyFileData
								mov			[BootRecord.OsLoaderEnd],edx
								add			[BootRecord.OsLoaderEnd],ebx

.CopyFileData:
								add			eax,EmbededOsloaderFileStart + SU_FLAT_ADDRESS
								cmp			ecx,0
								jz			.ZeroUninitializedData

								push		ecx
								push		eax
								push		edx
								call		MoveMemory
								pop			edx
								pop			eax
								pop			ecx

.ZeroUninitializedData:
							;
							; exchange ebx,edx
							;	ebx	= dst
							;	edx = virtual size
							;
								xchg		ebx,edx

								cmp			ecx,edx
								jnb			.CheckResourceSection

								mov			eax,ebx
								add			eax,ecx
								sub			edx,ecx
								push		edx
								push		eax
								call		ZeroMemory
								add			sp,8

.CheckResourceSection:
								cmp			dword [esi],'.rsr'
								jnz			.ProcessNextSection
								cmp			byte [esi + 4],'c'
								jnz			.ProcessNextSection

								mov			[BootRecord.ResourceDirectory],ebx
								mov			eax,[esi + 0ch]
								mov			[BootRecord.ResourceOffset],eax

.ProcessNextSection:
								add			esi,28h
								mov			ebx,[BootRecord.OsLoaderBase]
								dec			edi
								jz			.Return
								jmp			.ProcessSection

.InvalidOsloaderFileData:
								push		.InvalidOsloaderFileDataMsg
								call		puts
								add			sp,2
								jmp			$

.Return:
								mov			eax,ebp
								pop			ebx
								pop			edi
								pop			esi
								pop			ebp
								retn

.InvalidOsloaderFileDataMsg		db			"NTLDR is corrupt.  The system cannot boot.",0