
#include <windows.h>

extern int GetPid();

int PrintGs(void *pid)
{
  printf("gs value = %ld\n",(long)pid);
  return 0;
}

int main(int argc, char* argv[])
{
  DWORD Pid;
  DWORD PidFromGs;

  Pid = GetCurrentProcessId();
  printf("pid=%d\n",Pid);
  PidFromGs = GetPid();
  printf("Pid from gs:[64]=%ld\n",(long)PidFromGs);
  return 0;
}
c���Ա������
cl /c 1.c

2.asm������Դ�����£�
include listing.inc

INCLUDELIB LIBCMT
INCLUDELIB OLDNAMES

PUBLIC  GetPid
EXTRN  PrintGs:PROC

_TEXT  SEGMENT
GetPid  PROC
  mov  rcx,gs
  sub  rsp,8
  call   PrintGs
  add  rsp,8
  mov  eax,gs:[64]
  ret  0
GetPid  ENDP
_TEXT  ENDS
END

���������
ml64 /c 2.asm

�������
link 1.obj 2.obj

ִ�н������Ԥ�ڵ�һ����ͨ��APIȡ���Ľ��̺ţ��ʹ�TEB���ݽṹ�ϵ�ȡ���Ľ��̺ţ���һ���ġ�
pid=2884
gs value = 43
Pid from gs:[64]=2884