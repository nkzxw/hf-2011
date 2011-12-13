extrn MessageBoxA: proc
extrn ExitProcess: proc

include wap32.inc

	.386
.	model flat,stdcall

	.data
		db 0
	.code

Start:
	mov eax,[esp] ;   //ȡKernel32���ص�ַ
	and ax,0f000h
	mov esi,eax ;    //�õ�Kernel.PELoader����λ��(����ȷ)

FindKernel32:
	sub esi,1000h
	cmp W [esi], $1$ZM$1$ ;//����EXE�ļ�ͷ
	jnz short FindKernel32

GetPeHeader:
	movzx edi,W [esi.PEHeaderOffset]
	add edi,esi
	cmp W [edi],$1$EP$1$ ;//ȷ���Ƿ�PE�ļ�ͷ
	jnz short FindKernel32

;//////////////////////////////////////////////////���񣺲���GetProcAddress������ַ
GetPeExportTable:
	mov ebp,[edi.fhExportsRVA]
	add ebp,esi ;//�õ����������
GetExportNameList:
	mov ebx,[ebp.etExportNameList] ;//�õ������������
	add ebx,esi
	xor eax,eax ;//������ż���
	mov edx,esi ;//�ݴ�Kernel32ģ����
FindApiStr:
	add ebx,04
	inc eax ;//���Ӻ�������
	mov edi,[ebx]
	add edi,edx ;//�õ�һ��Api�������ַ���
	call PushStrGetProcAddress
	db $1$GetProcAddress$1$,0
	PushStrGetProcAddress:
	pop esi ;//�õ�Api�����ַ���
	xor ecx,ecx
	mov cl,15 ;//GetProcAddress����С
	cld
	rep cmpsb
	jnz short FindApiStr
	mov esi,edx
	mov ebx,[ebp.etExportOrdlList]
	add ebx,esi ; //ȡ������ŵ�ַ�б�
	movzx ecx,W [ebx+eax*2]
	mov ebx,[ebp.etExportAddrList]
	add ebx,esi ; //�õ�Kernel32������ַ�б�
	mov ebx,[ebx+ecx*4]
	add ebx,esi ; //����GetProcAddress������ַ

	;���ڣ� esi=Kernel32.dll hModule ebx=GetProcAddress
	int 3;
	ret
end Start