//
// ReadGp
// Copyright (C) 2002 Mark Russinovich
// Sysinternals - www.sysinternals.com						
//
// Reads the gp register.
//
	.file	"readgp.s"
	.radix	D
	.section	.text,	"ax", "progbits"
	.align 32
	.section	.pdata,	"a", "progbits"
	.align 4
	.section	.xdata,	"a", "progbits"
	.align 8
	.section	.data,	"wa", "progbits"
	.align 16
	.section	.rdata,	"a", "progbits"
	.align 16
	.section	.bss,	"wa", "nobits"
	.align 16
	.section	.debug$S,	"ax", "progbits"
	.align 16
	.section	.tls$,	"was", "progbits"
	.align 16
	.section	.sdata,	"was", "progbits"
	.align 16
	.section	.sbss,	"was", "nobits"
	.align 16
	.section	.srdata,	"as", "progbits"
	.align 16
	.section	.rdata,	"a", "progbits"
	.align 16
	.type	ReadGpRegister#	,@function 
    .global ReadGpRegister#
	.section	.xdata
$T106:	data2	03H
	data2	00H
	data4	02H
	string	"\x05"		//R1:prologue size 5
	string	"\xe8\x01"	//P7:preds_when time 1
	string	"\xb1\xa1"	//P3:pred_gr 33
	string	"\xe0\x02\x01"	//P7:mem_stack_f time 2 size 1
	string	"\x30"		//R1:body size size 16
	string	"\x81"		//B1:label_state 1
	string	"\xc0\x02"	//B2:ecount 0 time 2
	string	"\x00\x00\x00\x00" //padding
	.section	.pdata
$T108:	data4	@imagerel($L107#)
	data4	@imagerel($L107#+112)
	data4	@imagerel($T106#)
	.section	.text

	.proc	ReadGpRegister#
	.align 32
ReadGpRegister:	
$L107:
 {   .mii  
	alloc	r2=1, 1, 0, 0				   
	mov	r33=pr
	adds	sp=-16, sp;;				    
 }
 {   .mmi 
	adds	sp=16, sp
	mov	r8=gp	
	nop.i	 0;;
 }
 {   .mib 
    nop.m  0
	mov	pr=r33, -65473
	br.ret.sptk.few b0
 }
	.endp	ReadGpRegister#

