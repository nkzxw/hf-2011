#include <windows.h>
#include <tchar.h>

#define APPLICATION_NAME _T("C:\\WINDOWS\\NOTEPAD.EXE")

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow
                   )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(lpCmdLine);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    for (SIZE_T Cnt = 0; Cnt < 3 ; Cnt++)
    {
        // Start the child process. 
        if( !CreateProcess( NULL, // No module name (use command line). 
                            APPLICATION_NAME, // Command line. 
                            NULL,             // Process handle not inheritable. 
                            NULL,             // Thread handle not inheritable. 
                            FALSE,            // Set handle inheritance to FALSE. 
                            0,                // No creation flags. 
                            NULL,             // Use parent's environment block. 
                            NULL,             // Use parent's starting directory. 
                            &si,              // Pointer to STARTUPINFO structure.
                            &pi )             // Pointer to PROCESS_INFORMATION structure.
          ) 
        {
            break;
        }
        // Close process and thread handles. 
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

}