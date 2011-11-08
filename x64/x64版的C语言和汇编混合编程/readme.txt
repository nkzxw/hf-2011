
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
c语言编译命令：
cl /c 1.c

2.asm汇编程序源码如下：
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

汇编编译命令：
ml64 /c 2.asm

链接命令：
link 1.obj 2.obj

执行结果，和预期的一样，通过API取到的进程号，和从TEB数据结构上的取到的进程号，是一样的。
pid=2884
gs value = 43
Pid from gs:[64]=2884