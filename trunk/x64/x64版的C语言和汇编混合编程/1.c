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