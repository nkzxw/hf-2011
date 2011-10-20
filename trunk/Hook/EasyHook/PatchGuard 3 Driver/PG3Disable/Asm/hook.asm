;Released under MIT License
;
;Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.
;
;Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
;associated documentation files (the "Software"), to deal in the Software without restriction, 
;including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
;and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
;subject to the following conditions:
;
;The above copyright notice and this permission notice shall be included in all copies or substantial 
;portions of the Software.
;
;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
;LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
;WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;
;Visit http://www.codeplex.com/easyhook for more information.


.CODE


public VistaSp0_ExpWorkerThread_Fix
public VistaSp1_ExpWorkerThread_Fix

extrn VistaAll_ExpWorkerThreadInterceptor : PROC

VistaSp0_ExpWorkerThread_Fix PROC

	mov			rcx, rdi
	mov			rdx, r12
	mov			r8, rsp
	jmp			VistaAll_ExpWorkerThreadInterceptor

VistaSp0_ExpWorkerThread_Fix ENDP


VistaSp1_ExpWorkerThread_Fix PROC

	mov			rcx, rbx
	mov			rdx, rdi
	mov			r8, rsp
	jmp			VistaAll_ExpWorkerThreadInterceptor

VistaSp1_ExpWorkerThread_Fix ENDP

extrn KeBugCheck_Hook : PROC

EnableInterrupts PROC
	sti
	ret
EnableInterrupts ENDP

RtlCaptureContext_Hook PROC

	; call high level handler without messing up the context structure...
	push		rcx
	push		rdx
	push		r8
	push		r9
	push		r10
	push		r11
	mov			rcx, qword ptr[rsp + 128]
	mov			rdx, qword ptr[rsp + 7 * 8]
	sub			rsp, 32
	call		KeBugCheck_Hook
	mov			qword ptr [rsp], rax
	add			rsp, 32
	pop			r11
	pop			r10
	pop			r9
	pop			r8
	pop			rdx
	pop			rcx
	pop			rax
	
	; recover destroyed bytes of RtlCaptureContext
	pushfq
	mov			word ptr [rcx+38h],cs
	mov			word ptr [rcx+3Ah],ds
	mov			word ptr [rcx+3Ch],es
	mov			word ptr [rcx+42h],ss
	
	; jump behind destroyed bytes... (RetVal of RtlCaptureContext_HookEx)
	jmp			qword ptr[rsp - 32 - 8 * 7 + 8]

RtlCaptureContext_Hook ENDP

END