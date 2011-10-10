/*
Example of tls callback declaration

#pragma comment(linker, "/INCLUDE:__tls_used")

void __stdcall OnTlsCallback(void * /*instance*/,
                             DWORD reason,
                             void * /*reserved*/)
{
    if ( reason == DLL_PROCESS_ATTACH )
    {
        MessageBox(NULL, _T("OnTlsCallback is called before entry point"), _T("Information"), MB_OK|MB_ICONINFORMATION);
        // ExitProcess(0);
    }
}

#pragma data_seg(".CRT$XLB")
PIMAGE_TLS_CALLBACK pThreadCallback = OnTlsCallback;
#pragma data_seg()

*/