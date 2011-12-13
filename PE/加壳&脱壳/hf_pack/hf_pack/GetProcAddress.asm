extrn MessageBoxA: proc
extrn ExitProcess: proc

include wap32.inc

	.386
.	model flat,stdcall

	.data
		db 0
	.code

Start:
	mov eax,[esp] ;   //取Kernel32返回地址
	and ax,0f000h
	mov esi,eax ;    //得到Kernel.PELoader代码位置(不精确)

FindKernel32:
	sub esi,1000h
	cmp W [esi], $1$ZM$1$ ;//搜索EXE文件头
	jnz short FindKernel32

GetPeHeader:
	movzx edi,W [esi.PEHeaderOffset]
	add edi,esi
	cmp W [edi],$1$EP$1$ ;//确认是否PE文件头
	jnz short FindKernel32

;//////////////////////////////////////////////////任务：查找GetProcAddress函数地址
GetPeExportTable:
	mov ebp,[edi.fhExportsRVA]
	add ebp,esi ;//得到输出函数表
GetExportNameList:
	mov ebx,[ebp.etExportNameList] ;//得到输出函数名表
	add ebx,esi
	xor eax,eax ;//函数序号计数
	mov edx,esi ;//暂存Kernel32模块句柄
FindApiStr:
	add ebx,04
	inc eax ;//增加函数计数
	mov edi,[ebx]
	add edi,edx ;//得到一个Api函数名字符串
	call PushStrGetProcAddress
	db $1$GetProcAddress$1$,0
	PushStrGetProcAddress:
	pop esi ;//得到Api名字字符串
	xor ecx,ecx
	mov cl,15 ;//GetProcAddress串大小
	cld
	rep cmpsb
	jnz short FindApiStr
	mov esi,edx
	mov ebx,[ebp.etExportOrdlList]
	add ebx,esi ; //取函数序号地址列表
	movzx ecx,W [ebx+eax*2]
	mov ebx,[ebp.etExportAddrList]
	add ebx,esi ; //得到Kernel32函数地址列表
	mov ebx,[ebx+ecx*4]
	add ebx,esi ; //计算GetProcAddress函数地址

	;现在： esi=Kernel32.dll hModule ebx=GetProcAddress
	int 3;
	ret
end Start