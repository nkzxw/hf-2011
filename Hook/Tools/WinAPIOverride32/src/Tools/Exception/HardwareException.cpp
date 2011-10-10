#include "HardwareException.h"

// to catch ALL exceptions (even memory access) assume to have the /EHa option 
// VS2003 "project" / "properties" / "C/C++" / "Command Line" / "Additional options" / "/EHa"
// VS2005 "project" / "properties" / "C/C++" / "Code Generation" / "Enable C++ exceptions"  --> "Yes With SEH Exceptions (/EHa)"
#pragma message (__FILE__ " Information : to catch ALL exceptions (even memory access) assume to have the /EHa option enable for project\r\n")


/* Sample of use

try
{
    CExceptionHardware::RegisterTry();

    // your code goes here
}
catch( CExceptionHardware e ) // catch hardware exceptions
{
    MessageBox(0,e.ExceptionText,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}

[catch (...) // optional only to catch software exception [exception launched by user code by "throw 1" or throw "My Error Text"]
{
    MessageBox(0,_T("Software Exception occurs"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    [throw;] // optional only to rethrow the current exception for an upper try/catch block
}]

*/

CExceptionHardware::CExceptionHardware(unsigned int u, EXCEPTION_POINTERS* pException)
{
    this->ExceptionCode=u;
    this->pExceptionInformation=pException;
    this->GetExceptionText();
}

CExceptionHardware::~CExceptionHardware(void)
{
}
// se_translator_function: exception callback
void __cdecl CExceptionHardware::se_translator_function(unsigned int u, EXCEPTION_POINTERS* pException)
{
    throw CExceptionHardware(u,pException);
}
// set CExceptionHardware for current try block
void CExceptionHardware::RegisterTry()
{
    _set_se_translator(CExceptionHardware::se_translator_function);
}

void CExceptionHardware::GetExceptionText(TCHAR* ExceptionString)
{
    _sntprintf(this->ExceptionText, MAX_PATH, 
        _T("%s at address 0x%p."),
        ExceptionString,
        this->pExceptionInformation->ExceptionRecord->ExceptionAddress);
}

void CExceptionHardware::GetExceptionText()
{
    switch(this->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION: 
        {
            TCHAR psz[MAX_PATH];
            _tcscpy(psz,_T("ACCESS_VIOLATION"));
            if (this->pExceptionInformation->ExceptionRecord->NumberParameters>=2)
            {
                if (this->pExceptionInformation->ExceptionRecord->ExceptionInformation[0]==0)
                    _stprintf(psz,_T("ACCESS_VIOLATION reading location 0x%p"),this->pExceptionInformation->ExceptionRecord->ExceptionInformation[1]);
                else
                    _stprintf(psz,_T("ACCESS_VIOLATION writing location 0x%p"),this->pExceptionInformation->ExceptionRecord->ExceptionInformation[1]);
            }
            this->GetExceptionText(psz);
        }

        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        this->GetExceptionText(_T("DATATYPE_MISALIGNMENT"));
        break;
    case EXCEPTION_BREAKPOINT:
        this->GetExceptionText(_T("BREAKPOINT"));
        break;
    case EXCEPTION_SINGLE_STEP:
        this->GetExceptionText(_T("SINGLE_STEP"));
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        this->GetExceptionText(_T("ARRAY_BOUNDS_EXCEEDED"));
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        this->GetExceptionText(_T("FLT_DENORMAL_OPERAND"));
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        this->GetExceptionText(_T("FLT_DIVIDE_BY_ZERO"));
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        this->GetExceptionText(_T("FLT_INEXACT_RESULT"));
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        this->GetExceptionText(_T("FLT_INVALID_OPERATION"));
        break;
    case EXCEPTION_FLT_OVERFLOW:
        this->GetExceptionText(_T("FLT_OVERFLOW"));
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        this->GetExceptionText(_T("FLT_STACK_CHECK"));
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        this->GetExceptionText(_T("FLT_UNDERFLOW"));
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        this->GetExceptionText(_T("INT_DIVIDE_BY_ZERO"));
        break;
    case EXCEPTION_INT_OVERFLOW:
        this->GetExceptionText(_T("INT_OVERFLOW"));
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        this->GetExceptionText(_T("PRIV_INSTRUCTION"));
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        this->GetExceptionText(_T("IN_PAGE_ERROR"));
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        this->GetExceptionText(_T("ILLEGAL_INSTRUCTION"));
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        this->GetExceptionText(_T("NONCONTINUABLE_EXCEPTION"));
        break;
    case EXCEPTION_STACK_OVERFLOW:
        this->GetExceptionText(_T("STACK_OVERFLOW"));
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        this->GetExceptionText(_T("INVALID_DISPOSITION"));
        break;
    case EXCEPTION_GUARD_PAGE:
        this->GetExceptionText(_T("GUARD_PAGE"));
        break;
    case EXCEPTION_INVALID_HANDLE:
        this->GetExceptionText(_T("INVALID_HANDLE"));
        break;
    default:
        _sntprintf(this->ExceptionText, MAX_PATH,
                  _T("Unknown exception at address 0x%p."),
                  this->pExceptionInformation->ExceptionRecord->ExceptionAddress);
        break;
    }
}