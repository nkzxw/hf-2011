#pragma once




/* Sample of use

try
{
    CExceptionHardware::RegisterTry();

    // your code goes here
}
catch( CExceptionHardware e ) // catch hardware exceptions
{

MessageBox(0,e.ExceptionText,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

// optional to translate the current exception to an upper try/catch block
[RaiseException(e.pExceptionInformation->ExceptionRecord->ExceptionCode,
                e.pExceptionInformation->ExceptionRecord->ExceptionFlags,
                e.pExceptionInformation->ExceptionRecord->NumberParameters,
                e.pExceptionInformation->ExceptionRecord->ExceptionInformation);]
}

[catch (...) // optional only to catch software exception [exception launched by user code by "throw 1" or throw "My Error Text"]
{

MessageBox(0,_T("Software Exception occurs"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

// optional to translate the current exception to an upper try/catch block
[throw;] 
}]

*/


#include <Windows.h>
#include <eh.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CExceptionHardware
{
private:
    static void __cdecl se_translator_function(unsigned int u, EXCEPTION_POINTERS* pException);
    void GetExceptionText();
    void GetExceptionText(TCHAR* ExceptionString);
public:
    TCHAR ExceptionText[MAX_PATH];
    unsigned int ExceptionCode; // GetExceptionCode() value
    EXCEPTION_POINTERS* pExceptionInformation; // GetExceptionInformation() value
    
    CExceptionHardware(unsigned int u, EXCEPTION_POINTERS* pException);
    ~CExceptionHardware(void);
    void static RegisterTry();
};
