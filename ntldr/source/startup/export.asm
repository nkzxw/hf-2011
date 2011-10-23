;********************************************************************
;	created:	23:9:2008   02:10
;	file:		export.asm
;	author:		tiamo
;	purpose:	export services
;********************************************************************

EXPORT_PROLOGUE	RebootProcessor,0
							;
							; set reboot byte
							;
								mov			ax,40h
								mov			ds,ax
								mov			word [72h],1234h

							;
							; send reboot cmd to 8042
							;
								mov			al,0feh
								out			64h,al
								jmp			$
EXPORT_EPILOGUE

EXPORT_PROLOGUE	DiskIOSystem,28
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2

							;
							; eax = buffer
							;
								mov			eax,[bp + 18h]
								mov			bx,ax
								and			bx,0fh
								shr			eax,4
								mov			es,ax

							;
							; cx = head,sector
							;
								mov			cx,[bp + 0ch]
								xchg		ch,cl
								shl			cl,6
								add			cl,[bp + 10h]

							;
							; ah = func ,al = count
							; dh = track,dl = drive number
							;
								mov			ah,[bp]
								mov			al,[bp + 14h]
								mov			dh,[bp + 8]
								mov			dl,[bp + 4]
								cmp			dl,1
								jnz			.CallInt13
								cmp			ah,4
								jg			.CallInt13
								cmp			ah,0
								jz			.ChangeDiskVector
								cmp			ah,2
								jl			.CallInt13

.ChangeDiskVector:
								push		es
								push		bx
								push		di
								push		0
								pop			es
								mov			di,RomDiskBasePointer
							es	mov			bx,[78h]
								mov			[di],bx
							es	mov			bx,[7ah]
								mov			[di + 2],bx
								mov			bx,DiskBaseTable
							es	mov			[78h],bx
								mov			bx,ds
							es	mov			[7ah],bx
								pop			di
								pop			bx
								pop			es
								int			13h
								push		es
								push		bx
								push		di
								push		0
								pop			es
								mov			di,RomDiskBasePointer
								mov			bx,[di]
							es	mov			[78h],bx
								mov			bx,[di + 2]
							es	mov			[7ah],bx
								pop			di
								pop			bx
								pop			es
								jb			.Finish
								xor			eax,eax
								jmp			.Finish
.CallInt13:
								int			13h
								jb			.Finish
								xor			eax,eax
.Finish:
								and			eax,0ffffh
								pop			bp
							;
							; write result to ecx
							;
								shl			ecx,16
								mov			cx,dx
EXPORT_EPILOGUE

EXPORT_PROLOGUE	GetKey,0
							;
							; check buffer
							;
								mov			ax,100h
								int			16h
								jnz			.ReadKeyboard
								xor			eax,eax
								jmp			.Return

.ReadKeyboard:
							;
							; read keyboard
							;
								mov			ax,0
								int			16h

							;
							; clear high word
							;
								and			eax,0ffffh
.Return:
EXPORT_EPILOGUE

EXPORT_PROLOGUE	GetCounter,0
							;
							; get time of date
							;
								mov			ah,0
								int			1ah

							;
							; mov result from cx:dx to eax
							;
								mov			ax,cx
								shl			eax,16
								mov			ax,dx
EXPORT_EPILOGUE

EXPORT_PROLOGUE	Reboot,4
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2

							;
							; get reboot type
							;
								mov			edx,[bp]

							;
							; clear 3000:0000 - 5000:0000
							;
								xor			eax,eax
								mov			bx,3000h
								mov			es,bx
								mov			di,ax
								mov			cx,4000h
								cld
								rep			stosd
								mov			cx,4000h
								mov			es,cx
								rep			stosd

							;
							; disable A20
							;
								call		DisableA20

							;
							; set video mode
							;
								push		dx
								mov			ax,3
								int			10h
								pop			dx

							;
							; set registers
							;
								mov			ax,0
								mov			ds,ax
								mov			es,ax
								mov			fs,ax
								mov			gs,ax
								mov			ax,1eh
								mov			ss,ax
								mov			esp,100h
								mov			ebp,0
								mov			edi,0
								mov			esi,0
								test		dx,0ffffh
								jz			.JumpMBR
								mov			ax,BOOT_SEGMENT
								mov			ds,ax
								mov			ax,HPFS_BOOT2_SEGMENT
								mov			es,ax
								cli
								xor			ax,ax
								mov			ss,ax
								mov			sp,BOOT_CODE_OFFSET
								push		HPFS_BOOT2_SEGMENT
								push		HPFS_BOOT2_OFFSET
								retf

.JumpMBR:
								push		0
								push		BOOT_CODE_OFFSET
								mov			dx,80h
								retf
EXPORT_EPILOGUE

EXPORT_PROLOGUE	DetectHardware,24
							;
							; parameters have already been pushed on the stack,just setup return address
							;
								push		cs
								push		.ReturnFromDetect
								push		NT_DETECT_SEGMENT
								push		0
								retf
.ReturnFromDetect:
EXPORT_EPILOGUE

EXPORT_PROLOGUE	HardwareCursor,8
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2

							;
							; get x,y
							;
								mov			eax,[bp + 4]
								mov			edx,[bp]
								cmp			edx,80000000h
								jnz			.SetupXY
								mov			ebx,eax
								shr			ebx,16
								jmp			.VideoInterrupt
.SetupXY:
								mov			dh,al
								mov			ah,2
								mov			bh,0
.VideoInterrupt:
								int			10h
								pop			bp
EXPORT_EPILOGUE

EXPORT_PROLOGUE	GetDateTime,8
								push		bp
								mov			bp,sp
								add			bp,2

								;
								; get the time
								;
								mov			ah,2
								int			1ah

							;
							; convert BIOS time format into our format and place in caller's dword
							;	bits 0-5 are the second
							;	bits 6-11 are the minute
							;	bits 12-16 are the hour
							;
								xor			eax,eax
								mov			al,dh
								BCD_TO_BIN
								movzx		edx,ax
								mov			al,cl
								BCD_TO_BIN
								shl			ax,6
								or			dx,ax
								mov			al,ch
								BCD_TO_BIN
								shl			eax,12
								or			edx,eax
								mov			eax,[bp + 4]
								mov			bx,ax
								and			bx,0fh
								shr			eax,4
								mov			es,ax
							es	mov			[bx],edx

								;
								; get the date
								;
								mov			ah,4
								int			1ah

							;
							; convert BIOS date format into our format and place in caller's dword
							;	bits 0-4  are the day
							;	bits 5-8  are the month
							;	bits 9-31 are the year
							;
								xor			eax,eax
								mov			al,dl
								BCD_TO_BIN
								mov			bl,dh
								movzx		edx,ax
								mov			al,bl
								BCD_TO_BIN
								shl			ax,5
								or			dx,ax
								mov			al,cl
								BCD_TO_BIN
								mov			cl,al
								mov			al,ch
								BCD_TO_BIN
								mov			ah,100
								mul			ah
								xor			ch,ch
								add			ax,cx
								shl			eax,9
								or			edx,eax
								mov			eax,[bp]
								mov			bx,ax
								and			bx,0fh
								shr			eax,4
								mov			es,ax
							es	mov			[bx],edx

								pop			bp
EXPORT_EPILOGUE

EXPORT_PROLOGUE	ComPort,12
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2

							;
							; setup registers and call bios
							;
								mov			ah,[bp + 4]
								mov			al,[bp + 8]
								mov			dx,[bp]
								int			14h
								pop			bp
EXPORT_EPILOGUE

EXPORT_PROLOGUE	GetStallCount,0
								cli
								push		di
								push		si
								push		ds
								mov			ax,0
								mov			ds,ax

							;
							; save previous vector
							;
								mov			di,01ch * 4
								mov			cx,[di]
								mov			dx,[di+2]

							;
							; insert our vector
							;
								mov			ax,.GscISR
								mov			[di], ax
								push		cs
								pop			ax
								mov			[di+2], ax

								mov			eax,0
								mov			ebx,0
								mov			si,sp
								sub			si,6
							cs	mov			[.savesp],si
							cs	mov			word [.newip],.GscLoop2
								sti

							;
							; wait for first tick.
							;
.GscLoop1:
								cmp			ebx,0
								je			.GscLoop1

							;
							; start counting
							;	we spin in this loop until the ISR fires.
							;	the ISR will munge the return address on the stack to blow us out of the loop and into GscLoop3
							;
.GscLoop2:
							cs	mov			word [.newip],.GscLoop4

.GscLoop3:
								add			eax,1
								jnz			.GscLoop3

.GscLoop4:
							;
							; stop counting,replace old vector
							;
								cli
								mov			[di],cx
								mov			[di+2],dx
								sti

								pop			ds
								pop			si
								pop			di
								jmp			.GscDone

								align		4
.newip							dw			0
.savesp							dw			0

.GscISR:
							;
							; blow out of loop
							;
								push		bp
								push		ax
							cs	mov			bp,[.savesp]
							cs	mov			ax,[.newip]
							ss	mov			[bp],ax
								pop			ax
								pop			bp

.GscISRdone:
								iret

.GscDone:
							;
							; (dx:ax) = dividend,(cx) = divisor
							;
								mov			edx, eax
								shr			edx,16
								mov			cx,0d6a6h
								div			cx

							;
							; round loopcount up (prevent 0)
							;
								and			eax,0ffffh
								inc			eax
EXPORT_EPILOGUE

EXPORT_PROLOGUE	InitializeDisplayForNt,0
							;
							; load rom
							;
								mov			ax,1112h
								mov			bx,0
								int			10h
EXPORT_EPILOGUE

EXPORT_PROLOGUE	GetMemoryDescriptor,4
							;
							; int15 E820
							;
								push		bp
								mov			bp,sp
								add			bp,2
								mov			eax,[bp]
								mov			bp,ax
								and			bp,0fh
								shr			eax,4
								mov			es,ax
							es	mov			ebx,[bp + E820FRAME.Key]
							es	mov			ecx,[bp + E820FRAME.DescSize]
								lea			 di,[bp + E820FRAME.BaseAddrLow]
								mov			eax,0e820h
								mov			edx,'PAMS'
								int			15h
							es	mov			[bp + E820FRAME.Key],ebx
							es	mov			[bp + E820FRAME.DescSize],ecx
								sbb			ecx,ecx
								sub			eax,'PAMS'
								or			ecx,eax
							es	mov			[bp + E820FRAME.ErrorFlag],ecx
								pop			bp
EXPORT_EPILOGUE

EXPORT_PROLOGUE	ExtendedDiskIOSystem,24
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2
								push		si
								push		bx

								mov			si,EddsAddressPacket
								mov			word [EddsAddressPacket.PacketSize],16
								mov			ax,[bp + 0ch]
								mov			[EddsAddressPacket.BlockCount],ax
								mov			eax,[bp + 10h]
								mov			bx,ax
								and			bx,0fh
								mov			[EddsAddressPacket.XferBufferOffset],bx
								shr			eax,4
								mov			[EddsAddressPacket.XferBufferSegment],ax
								mov			eax,[bp + 4]
								mov			[EddsAddressPacket.LBALow],eax
								mov			eax,[bp + 8]
								mov			[EddsAddressPacket.LBAHigh],eax
								mov			ah,[bp + 14h]
								xor			al,al
								mov			dl,[bp]
								int			13h
								jb			.Error
								xor			eax,eax
.Error:
								and			eax,0ffffh
								pop			bx
								pop			si
								pop			bp
								shl			ecx,16
								mov			cx,dx
EXPORT_EPILOGUE

EXPORT_PROLOGUE	GetElToritoStatus,8
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2

							;
							; save registers
							;
								push		dx
								push		bx
								push		ds
								push		si

							;
							; get buffer
							;
								mov			eax,[bp]
								mov			bx,ax
								and			bx,0fh
								mov			si,bx
								shr			eax,4
								mov			ds,ax

							;
							; get drive number
							;
								mov			dl,[bp + 4]

							;
							; int13 ext call bios
							;
								mov			ax,4b01h
								int			13h
								jb			.Error
								xor			eax,eax

.Error:
								and			eax,0ffffh
								pop			si
								pop			ds
								pop			bx
								pop			dx
								pop			bp
EXPORT_EPILOGUE

EXPORT_PROLOGUE	CheckInt13ExtensionSupported,8
							;
							; make stack frame
							;
								push		bp
								mov			bp,sp
								add			bp,2
								push		dx
								push		bx
								push		ds
								push		si

							;
							; install check
							;
								mov			ah,41h
								mov			bx,55aah
								mov			dl,[bp + 4]
								int			13h
								jb			.Error
								cmp			bx,0aa55h
								jnz			.Error
								test		cl,1
								jz			.Error

							;
							; check LBA mode supported
							;
								mov			eax,[bp]
								mov			bx,ax
								and			bx,0fh
								mov			si,bx
								shr			eax,4
								mov			ds,ax
								mov			word [si],1ah
								mov			dl,[bp + 4]
								mov			ah,48h
								int			13h
								jb			.Error
								mov			al,1
								jnb			.OK
.Error:
								xor			al,al
.OK:
								movzx		eax,al
								pop			si
								pop			ds
								pop			bx
								pop			dx
								pop			bp
EXPORT_EPILOGUE

EXPORT_PROLOGUE	PxeService,8
EXPORT_EPILOGUE

EXPORT_PROLOGUE	InitializeAPMBios,0
							;
							; AMP Install check
							;
								mov			eax,5300h
								mov			ebx,0
								int			15h
								jb			.Error
								cmp			bx,'MP'
								jnz			.Error
								cmp			ah,1
								jle			.CheckMinor
								mov			ah,1
.CheckMinor:
								cmp			al,2
								jle			.ConnectRealMode
								mov			al,2

.ConnectRealMode:
								push		ax
								mov			ax,5301h
								mov			bx,0
								int			15h
								pop			ax
								jb			.Error

							;
							; driver version
							;
								push		ax
								mov			cx,ax
								mov			bx,0
								mov			ax,530eh
								int			15h
								pop			ax
								jb			.Error
								mov			ax,5300h
								mov			bx,0
								int			15h
								jb			.Error

							;
							; disconnect
							;
								push		cx
								push		ax
								mov			ax,5304h
								mov			bx,0
								int			15h
								pop			ax
								pop			cx
								jb			.Error
								cmp			cx,1
								jz			.Error

							;
							; connect 16-bit protmode interface
							;
								push		ax
								push		cx
								mov			ax,5302h
								mov			bx,0
								int			15h
								pop			cx
								pop			ax
								jb			.Error

								mov			cx,ax
								mov			ax,530eh
								mov			bx,0
								int			15h
.Error:
EXPORT_EPILOGUE