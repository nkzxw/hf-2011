void HideLibrary(HMODULE hModule, LPVOID pCallBackAddr, LPVOID lParam);

typedef struct
{
      HMODULE lpDllBase;
      LPVOID lpNewDllBase;
      PTHREAD_START_ROUTINE pAddress;
      LPVOID lParam;
}UNLOADLIB_CALLBACK, *PUNLOADLIB_CALLBACK;

typedef
LPVOID WINAPI VIRTUALALLOC(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
);

typedef
BOOL WINAPI VIRTUALFREE(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD dwFreeType
);


typedef
BOOL WINAPI HEAPDESTROY(
    HANDLE hHeap
);

typedef
HMODULE WINAPI LOADLIBRARY(
    LPCTSTR lpFileName
);

typedef
HANDLE WINAPI CREATETHREAD(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
);

typedef void *    __cdecl MEMCPY(void *, const void *, size_t);


BOOL incLibraryCount(HMODULE hMe)
{
      //FreeLibrary��ܶ�ϵͳdllҲ��free�������Խ������Ѽ��ص���loadһ�������Ӽ���
    
      HANDLE hModsSnap =    CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

      if(INVALID_HANDLE_VALUE == hModsSnap)
      {
          return FALSE;
      }

      MODULEENTRY32 meModuleEntry;
      meModuleEntry.dwSize = sizeof(MODULEENTRY32);

      if(!Module32First(hModsSnap, &meModuleEntry))
      {
          CloseHandle(hModsSnap);
          return FALSE;
      }
      do
      {
          if(LoadLibrary(meModuleEntry.szModule) == hMe)
              FreeLibrary(hMe);

      } while(Module32Next(hModsSnap, &meModuleEntry));

      CloseHandle(hModsSnap);

      return TRUE;
}

//ö��ָ�����̵������߳�
DWORD WINAPI EnumAndSetThreadState(LPVOID lParam)
{
      HANDLE hThreadSnap = NULL;
      THREADENTRY32 te32;
      memset(&te32,0,sizeof(te32));
      te32.dwSize = sizeof(THREADENTRY32);
      hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);

      DWORD myThreadId = GetCurrentThreadId();
      DWORD pid = GetCurrentProcessId();

      if (Thread32First(hThreadSnap, &te32))
      {
          do
          {
               if (pid == te32.th32OwnerProcessID)
               {
                   if(myThreadId != te32.th32ThreadID)
                   {
                       HANDLE hThread = OpenThread(
                           THREAD_SUSPEND_RESUME,
                           FALSE,
                           te32.th32ThreadID);

                       if(hThread != NULL)
                       {
                           if((int)lParam)
                               ResumeThread(hThread);
                           else
                               SuspendThread(hThread);

                           CloseHandle(hThread);
                       }
                   }
               }
          }
          while (Thread32Next(hThreadSnap,&te32));
      }
      CloseHandle( hThreadSnap );

      return 0;
}

DWORD WINAPI GotoCallBackAddr(LPVOID lParam)
{
      PUNLOADLIB_CALLBACK cbFunc = (PUNLOADLIB_CALLBACK)lParam;

      DWORD dwThreadId;
      HANDLE hThread;

      if(cbFunc->pAddress)
      {
          hThread = CreateThread(
              NULL,
              0,
              cbFunc->pAddress,
              cbFunc->lParam,
              0,
              &dwThreadId);

          if(hThread)
              CloseHandle(hThread);
      }

      //�Ƿ�dll�Ŀ�������Ҫ�ˣ��ͷ�~
      VirtualFree(cbFunc->lpNewDllBase, 0, MEM_DECOMMIT);
      delete cbFunc;

      return 0;
}

DWORD WINAPI UnLoadLibrary(LPVOID lParam)
{
      //__asm INT 3
      __printf("UnLoadLibrary Entry.\r\n");

      BYTE HeapDestroy_HookCode_bak[4];
      BYTE HeapDestroy_HookCode[4] = "\xC2\x04\x00";//RETN 0004
      MODULEINFO modinfo;
      DWORD oldProtect;

      PUNLOADLIB_CALLBACK cbFunc = (PUNLOADLIB_CALLBACK)lParam;

      HMODULE hDllInstance = cbFunc->lpDllBase;
      char dllpath_bak[MAX_PATH];

      GetModuleFileName(hDllInstance, dllpath_bak, sizeof(dllpath_bak));
      GetModuleInformation(GetCurrentProcess(), hDllInstance, &modinfo, sizeof(MODULEINFO));

      //������dll(�����Լ�)���Ӽ���,��ֹFreeLibrary��ʱ����Щdll��ϵͳж�ص�
      incLibraryCount(hDllInstance);

      //������������������̣߳��㶨���ٻָ�
      EnumAndSetThreadState((LPVOID)FALSE);

      //FreeLibrary֮��ԭ�����api��ַ���ڴ�Ҳ�ᱻ�ͷţ�
      //����FreeLibrary֮����Щ�����������ڻ�ûfree���ؼ�API������
      VIRTUALALLOC *_VirtualAlloc = (VIRTUALALLOC*)
          GetProcAddress(GetModuleHandle("kernel32.dll"), "VirtualAlloc");
      LOADLIBRARY    *_LoadLibrary    = (LOADLIBRARY*)
          GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
      CREATETHREAD *_CreateThread = (CREATETHREAD*)
          GetProcAddress(GetModuleHandle("kernel32.dll"), "CreateThread");
      MEMCPY         *_memcpy         = (MEMCPY*)
          GetProcAddress(GetModuleHandle("ntdll.dll"), "memcpy");

      //����ܹؼ�����������Ҫ���ã��� FreeLibrary ʱϵͳ����ã���Ҫhook��,
      //���ܸ�ϵͳ�ƻ����heap������֮���dllò���ܹ�����
      //��ȴ������new��malloc�����ڴ�, VirtualAlloc���Դ���֮��
      //�������д�ö�����ǻ������ģ�����һЩ���벻�ø�,��list<T>��push�ڲ���new

      HEAPDESTROY *_HeapDestroy    = (HEAPDESTROY*)
          GetProcAddress(GetModuleHandle("kernel32.dll"), "HeapDestroy");

      VirtualProtect(_HeapDestroy, 3, PAGE_EXECUTE_READWRITE, &oldProtect);

      //�޸ĵ�һ��ָ��Ϊֱ�ӷ���
      _memcpy(HeapDestroy_HookCode_bak, _HeapDestroy, 3);
      _memcpy(_HeapDestroy, HeapDestroy_HookCode, 3);


      //Sleep(100);
      //���ڵ�������~~~^_^!
      FreeLibrary(hDllInstance);//�ͷ�

      //�޸���hook�ĺ���
      _memcpy(_HeapDestroy, HeapDestroy_HookCode_bak, 3);
      //_memcpy(_RtlFreeHeap, RtlFreeHeap_HookCode_bak, 3);

      //��ԭ����dll��ַ����ͬ����С���ڴ棬����֮ǰ���Ƿ�dll������ԭ��ȥ
      if(_VirtualAlloc(hDllInstance,
          modinfo.SizeOfImage,
          MEM_COMMIT|MEM_RESERVE,
          PAGE_EXECUTE_READWRITE) == NULL
          )
      {
          //ʧ�ܣ�����ԭ��dll, ��������ʽ����
          //ע�⣬������dllmain�е���HideLibrary��LoadLibrary������dllmain�ٴα�����,������ѭ����
          HMODULE hDll = _LoadLibrary(dllpath_bak);

          //���¼���ص�������hDll��ַ�ռ�ĵ�ַ
          cbFunc->pAddress = (LPTHREAD_START_ROUTINE)
              ((DWORD)cbFunc->pAddress - (DWORD)hDllInstance + (DWORD)hDll);

          LPTHREAD_START_ROUTINE pFunc1 = (LPTHREAD_START_ROUTINE)
              ((DWORD)EnumAndSetThreadState - (DWORD)hDllInstance + (DWORD)hDll);

          //�ָ���������߳�
          _CreateThread(0, 0, pFunc1, (LPVOID)TRUE, 0, 0);

          //���ûص�����
          if(cbFunc->pAddress)
              _CreateThread(0, 0, cbFunc->pAddress, cbFunc->lParam, 0, 0);

          return 0;
      }

      _memcpy(hDllInstance, cbFunc->lpNewDllBase, modinfo.SizeOfImage);

      //�ָ���������߳�
      EnumAndSetThreadState((LPVOID)TRUE);

      //����ԭdll��ַ�ռ��GotoCallBackAddr���������ͷ����VirtualAlloc�����ָ��
      _CreateThread(0, 0, GotoCallBackAddr, cbFunc, 0, 0);

      return 0;
}

DWORD WINAPI HideLibrary02(LPVOID lParam)
{
      //__asm INT 3
      __printf("HideLibrary02 Entry.\r\n");

      PUNLOADLIB_CALLBACK cbFunc = (PUNLOADLIB_CALLBACK)lParam;

      MODULEINFO modinfo;

      GetModuleInformation(GetCurrentProcess(), cbFunc->lpDllBase, &modinfo, sizeof(MODULEINFO));

      //����һ��͵�ǰdllͬ����С���ڴ�
      cbFunc->lpNewDllBase = VirtualAlloc(NULL, modinfo.SizeOfImage, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);

      if(cbFunc->lpNewDllBase == NULL)
          return FALSE;

      //����ǰdll���ݿ����������������ݵ���������ڴ棬
      memcpy(cbFunc->lpNewDllBase, modinfo.lpBaseOfDll, modinfo.SizeOfImage);

      //������copy��UnLoadLibrary�ĵ�ַ,�������̵߳��õ�ִַ��
      void *pNewUnLoadLibrary = LPVOID((DWORD)cbFunc->lpNewDllBase + (DWORD)UnLoadLibrary - (DWORD)modinfo.lpBaseOfDll);

      DWORD ThreadId;
      HANDLE hThread = CreateThread(0,0,
          (LPTHREAD_START_ROUTINE)pNewUnLoadLibrary, (LPVOID)cbFunc, CREATE_SUSPENDED, &ThreadId);

      if(hThread == NULL)
      {
          VirtualFree(cbFunc->lpNewDllBase, 0, MEM_DECOMMIT);
          delete cbFunc;

          return FALSE;
      }

      ResumeThread(hThread);
      CloseHandle(hThread);

      return TRUE;
}


void HideLibrary(HMODULE hModule, LPVOID pCallBackAddr, LPVOID lParam)
{
      __printf("HideLibrary Entry.\r\n");

      PUNLOADLIB_CALLBACK lparam = new UNLOADLIB_CALLBACK;

      lparam->lpDllBase      = hModule;
      lparam->lpNewDllBase = NULL;
      lparam->pAddress       = (PTHREAD_START_ROUTINE)pCallBackAddr;
      lparam->lParam         = lParam;

      HANDLE hThread = CreateThread(0,0,
          HideLibrary02, (LPVOID)lparam, 0, NULL);

      if(hThread == NULL)
      {
          __printf("CreateThread HideLibrary02 Failed.\r\n");

          delete lparam;
          return;
      }

      CloseHandle(hThread);

      return;
}

