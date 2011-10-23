;********************************************************************
;	created:	23:9:2008   01:44
;	file:		data.asm
;	author:		tiamo
;	purpose:	global data
;********************************************************************

								align			16
DataStart:
Beginx86Relocation:
GDT:
							;
							; selector 00h - null selector
							;
								dd			0,0

							;
							; selector 08h - KeCodeSelector,kernel code segment: flat 4gig limit
							;
								dw			0ffffh,0
								db			0,9ah,0cfh,0

							;
							; selector 10h - KeDataSelector,kerenl data segment: flat 4gig limit
							;
								dw			0ffffh,0
								db			0,92h,0cfh,0

							;
							; selector 18h - UsCodeSelector,user code segment: flat 2gig limit
							;
								dw			0ffffh,0
								db			0,0fah,0cfh,0

							;
							; selector 20h - UsDataSelector,user data segment: flat 2gig limit
							;
								dw			0ffffh,0
								db			0,0f2h,0cfh,0

							;
							; selector 28h - TSS_Selector,kerenl TSS
							;
								dw			EndTssKernel - TssKernel - 1,TssKernel
								db			2,89h,0,0

							;
							; selector 30h - PCR_Selector,it will be edited by osloader's BlSetupForNt routine
							;
								dw			1,0
								db			0,92h,0c0h,0

							;
							; selector 38h - TEP_Selector,thread enviroment
							;
								dw			0fffh,0
								db			0,0f3h,40h,0

							;
							; selector 40h - BDA_Selector,bios data area
							;
								dw			0ffffh,400h
								db			0,0f2h,0,0

							;
							; selector 48h - LdtDescriptor
							;
								dd			0,0

							;
							; selector 50h - DblFltTskSelector,double fault TSS
							;
								dw			EndTssDblFault32 - TssDblFault32 - 1,TssDblFault32
								db			2,89h,0,0

							;
							; selector 58h - SuCodeSelector,startup module code segment
							;
								dw			0ffffh,0
								db			2,9ah,0,0

							;
							; selector 60h - SuDataSelector,startup module data segment
							;
								dw			0ffffh,0
								db			2,92h,0,0

							;
							; selector 68h - VideoSelector,video display buffer
							;
								dw			3fffh,8000h
								db			0bh,92h,0,0

							;
							; selector 70h - GDT_AliasSelector,GDT alias selector
							;
								dw			EndGDT - GDT - 1,7000h
								db			0ffh,92h,0,0ffh

							;
							; debug selectors
							;
								dw			0ffffh,0
								db			40h,9ah,0,80h
								dw			0ffffh,0
								db			40h,92h,0,80h
								dw			0,0
								db			0,92h,0,0
								dw			0,0
								db			0,0,0,0

							;
							; empty
							;
								times		1024 + GDT - $			db		0
EndGDT:
IDT:
								dw			Trap0,KeCodeSelector,8f00h,0
								dw			Trap1,SuCodeSelector,8f00h,0
								dw			Trap2,SuCodeSelector,8f00h,0
								dw			Trap3,SuCodeSelector,8f00h,0
								dw			Trap4,SuCodeSelector,8f00h,0
								dw			Trap5,SuCodeSelector,8f00h,0
								dw			Trap6,SuCodeSelector,8f00h,0
								dw			Trap7,SuCodeSelector,8f00h,0
								dw			Trap8,SuCodeSelector,8f00h,0
								dw			Trap9,SuCodeSelector,8f00h,0
								dw			TrapA,SuCodeSelector,8f00h,0
								dw			TrapB,SuCodeSelector,8f00h,0
								dw			TrapC,SuCodeSelector,8f00h,0
								dw			TrapD,SuCodeSelector,8f00h,0
								dw			TrapE,SuCodeSelector,8f00h,0
								dw			TrapF,SuCodeSelector,8f00h,0
								times		2048 + IDT - $			db		0
EndIDT:
Endx86Relocation:

;
; double fault tss
;
								align		16
TssDblFault:
								dw			0
								dw			DblFaultStack
								dw			SuDataSelector
								dd			0
								dd			0
								dw			Trap8
								dw			0
								dw			0
								dw			0
								dw			0
								dw			0
								dw			DblFaultStack
								dw			0
								dw			0
								dw			0
								dw			SuDataSelector
								dw			SuCodeSelector
								dw			SuDataSelector
								dw			SuDataSelector
								dw			0
								dw			0
EndTssDblFault:
TssDblFault32:
								dd			0
								dd			DblFaultStack
								dd			SuDataSelector
								dd			0
								dd			0
								dd			0
								dd			0
								dd			99000h
								dd			Trap8
								dd			0
								dd			0
								dd			0
								dd			0
								dd			0
								dd			DblFaultStack
								dd			0
								dd			0
								dd			0
								dd			SuDataSelector
								dd			SuCodeSelector
								dd			SuDataSelector
								dd			SuDataSelector
								dd			0
								dd			0
								dd			0
								dd			0
								dd			0
								dd			0
EndTssDblFault32:

;
; double fault handler task stack
;
								align		4
DblFaultStackEnd:
								times		50					dw		0
DblFaultStack:

;
; startup module stack
;
								align		4
SuStackEnd:
								times		800h				db		0
SuStack:

;
; kerenl tss
;
								align		16
TssKernel:
								times		60					dw		0
EndTssKernel:

;
; gdtr
;
								align		4
GDTregister:
								dw			EndGDT - GDT - 1
								dd			SYSTEM_STRUCTS_BASE_PA + GDT - Beginx86Relocation

;
; idtr
;
								align		4
IDTregister:
								dw			EndIDT - IDT - 1
								dd			SYSTEM_STRUCTS_BASE_PA + IDT - Beginx86Relocation

;
; zero idtr
;
IDTregisterZero:
								dw			0ffffh
								dd			0

;
; disk base table
;
DiskBaseTable:
.SpecifyBytes					dw			0
.WaitTime						db			0
.SectorLength					db			0
.LastSector						db			0
.SecGapLength					db			0
.DataTransfer					db			0
.TrackGapLength					db			0
.DataValue						db			0
.HeadSettle						db			0
.StartupTime					db			0
;
; saved rom disk base vector
;
RomDiskBasePointer				dd			0

;
; edds packet
;
								align		16
EddsAddressPacket:
.PacketSize						db			16
.Reserved						db			0
.BlockCount						dw			0
.XferBufferOffset				dw			0
.XferBufferSegment				dw			0
.LBALow							dd			0
.LBAHigh						dd			0

;
; fs context
;
								align		4
FsContext						dd			0

;
; memory descriptor list
;
								align		4
MemoryDescriptorList:
.Offset							dw			0
.Segment						dw			MEMORY_MAP_SEGMENT

;
; export table
;
								align		4
ExportEntryTable:
								dd			SU_FLAT_ADDRESS + RebootProcessor
								dd			SU_FLAT_ADDRESS + DiskIOSystem
								dd			SU_FLAT_ADDRESS + GetKey
								dd			SU_FLAT_ADDRESS + GetCounter
								dd			SU_FLAT_ADDRESS + Reboot
								dd			SU_FLAT_ADDRESS + DetectHardware
								dd			SU_FLAT_ADDRESS + HardwareCursor
								dd			SU_FLAT_ADDRESS + GetDateTime
								dd			SU_FLAT_ADDRESS + ComPort
								dd			SU_FLAT_ADDRESS + GetStallCount
								dd			SU_FLAT_ADDRESS + InitializeDisplayForNt
								dd			SU_FLAT_ADDRESS + GetMemoryDescriptor
								dd			SU_FLAT_ADDRESS + ExtendedDiskIOSystem
								dd			SU_FLAT_ADDRESS + GetElToritoStatus
								dd			SU_FLAT_ADDRESS + CheckInt13ExtensionSupported
								dd			SU_FLAT_ADDRESS + PxeService
								dd			SU_FLAT_ADDRESS + InitializeAPMBios
								dd			0,0

;
; boot record
;
								align		4
BootRecord:
.FSContextPointer				dd			SU_FLAT_ADDRESS + FsContext
.ExternalServicesTable			dd			SU_FLAT_ADDRESS + ExportEntryTable
.MemoryDescriptorList			dd			MEMORY_MAP_FLAT_ADDRESS
.MachineType					dd			0
.OsLoaderStart					dd			0ffffffffh
.OsLoaderEnd					dd			0
.ResourceDirectory				dd			0
.ResourceOffset					dd			0
.OsLoaderBase					dd			0
.OsLoaderExports				dd			0
.BootFlags						dd			0
.NtDetectStart					dd			0
.NtDetectEnd					dd			0
.SdiAddress						dd			0

								align		16
EmbededOsloaderFileStart:
								incbin		OSLOADER_EXE
EmbededOsloaderFileEnd:
