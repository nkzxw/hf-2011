/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originaly based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: kernel of monitoring and overriding
//-----------------------------------------------------------------------------

#include "APIOverrideKernel.h"
#include "APIMonitoringFileLoader.h"
#include "FakeAPILoader.h"
#include "COM_Manager.h"
#include "NET_Manager.h"
#include "LogAPI.h"
#include "BreakUserInterface.h"
#include "ReportMessage.h"
#include "DynamicLoadedFuncs.h"
#include "../../../LinkList/LinkList.h"
#include "../../../LinkList/SingleThreaded/LinkListSingleThreaded.h"
#include "../../../CleanCloseHandle/CleanCloseHandle.h"
#include "../HookAvailabilityCheck/HookAvailabilityCheck.h"
#include "../../../Exception/HardwareException.h"
#include "../../../Disasm/CallInstructionSizeFromRetAddress/CallInstructionSizeFromRetAddress.h"
#include "../../../String/AnsiUnicodeConvert.h"

typedef struct tagNetStringHeader
{
    PBYTE VTBL;
#ifdef _WIN64
    to check
#endif 
    DWORD MaxSize; // check if SIZE_T in 64 bits (with \0)
    DWORD Length;  // check if SIZE_T in 64 bits (without \0)
}NET_STRING_HEADER;

#pragma intrinsic (memcpy,memset,memcmp) // required for memcpy

// compile with: /Oi /MT /D"_X86_"
// To declare an interlocked function for use as an intrinsic,
// First, the function must be declared with the leading underscore.
// Second, the new function must appear in a #pragma intrinsic statement.
// For convenience, the intrinsic versions of the functions may be
// #define'd to appear in the source code without the leading underscore.
// Declare all of the supported interlocked intrinsic functions:
extern "C"
{
    LONG  __cdecl _InterlockedIncrement(LONG volatile *Addend);
    LONG  __cdecl _InterlockedDecrement(LONG volatile *Addend);
}
#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement
#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

// use of intrinsic function instead of ApiSubstitution for interlocked
// extern "C" LONG __stdcall InterlockedIncrementApiSubstitution(volatile LONG *lpAddend);
// extern "C" LONG __stdcall InterlockedDecrementApiSubstitution(volatile LONG *lpAddend);
extern "C" LPVOID __stdcall TlsGetValueApiSubstitution(DWORD dwTlsIndex);
extern "C" BOOL __stdcall TlsSetValueApiSubstitution(DWORD dwTlsIndex, LPVOID lpTlsValue);

extern "C" void __stdcall SetLastErrorApiSubstitution(DWORD dwErrCode);
extern "C" DWORD __stdcall GetLastErrorApiSubstitution();

typedef struct tagHookThreadNetExceptionInfos 
{
    BOOL NetExceptionBeingProcessing;
    CLinkListItem* pNetExceptionLastExceptionHandlerItem;
}HOOK_THREAD_NET_EXCEPTION_INFOS,*PHOOK_THREAD_NET_EXCEPTION_INFOS;

typedef struct tagTlsDataSimpleHookCriticalCallInfo
{
    PBYTE OriginalReturnAddress;
    API_INFO* pApiInfos;
}TLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFO,*PTLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFO;

#define MAX_API_DEPTH_USED_IN_LIST_CRITICAL_PART 10 // max depth of monitored subfunctions calls done by api used in ListCriticalPart
typedef struct tagTlsDataSimpleHookCriticalCallInfos
{
    TLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFO HookCriticalInfos[MAX_API_DEPTH_USED_IN_LIST_CRITICAL_PART];
    DWORD CurrentIndex;
}TLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFOS,*PTLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFOS;

// CApiOverrideTlsData : class containing all informations needed for a single thread
// CApiOverrideTlsData object are stored in tls
class CApiOverrideTlsData
{
public:
    BOOL InsidePrePost;// TRUE if inside APIHandler or APIAfterCallHandler for a pointer hook api or if first bytes can execute anywhere (avoids infinite loop)
    CLinkListSingleThreaded* pLinkListTlsData;// contains pointer on linked list of currently hooked function informations for the thread
    CLinkListSingleThreaded* pLinkListTlsDataPotentialUncatchException;// contains pointer on linked list of currently hooked function informations for the thread
    HOOK_THREAD_NET_EXCEPTION_INFOS* pNetExceptionInfos;
    DWORD ThreadId;

    TLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFOS SimpleHookCriticalCallInfos;

    CApiOverrideTlsData()
    {
        this->InsidePrePost=FALSE;
        this->pNetExceptionInfos=NULL;
        this->pLinkListTlsData=NULL;
        this->pLinkListTlsDataPotentialUncatchException=NULL;
        this->ThreadId=::GetCurrentThreadId();// mov eax fs:[18] 
                                              // mov eax, [eax+24] 
                                              // ret
        memset(&SimpleHookCriticalCallInfos,0,sizeof(SimpleHookCriticalCallInfos));
    }

    ~CApiOverrideTlsData()
    {
        if (this->pNetExceptionInfos)
        {
            HOOK_THREAD_NET_EXCEPTION_INFOS* pTmpNetExceptionInfos;
            pTmpNetExceptionInfos=this->pNetExceptionInfos;
            this->pNetExceptionInfos=NULL;
            delete pTmpNetExceptionInfos;
        }
        if (this->pLinkListTlsData)
        {
            CLinkListSingleThreaded* pTmpLinkListTlsData;
            pTmpLinkListTlsData=this->pLinkListTlsData;
            this->pLinkListTlsData=NULL;
            delete pTmpLinkListTlsData;
        }
        if (this->pLinkListTlsDataPotentialUncatchException)
        {
            CLinkListSingleThreaded* pTmpLinkListTlsData;
            pTmpLinkListTlsData=this->pLinkListTlsDataPotentialUncatchException;
            this->pLinkListTlsDataPotentialUncatchException=NULL;
            delete pTmpLinkListTlsData;
        }
    }
};


// MAX_SIMPLE_HOOK_CRITICAL_CALL_INFO : for all threads and for all subfunctions calls in critical part;
// Array is used for functions using first opcodes replacement, a single used API can be in array only once
// As critical part use less than 10 API, and size is enough for more than 100 threads all being in critical section in the same time
// --> size must never be reached
#define MAX_SIMPLE_HOOK_CRITICAL_CALL_INFO 1024 
typedef struct tagSimpleHookCriticalCallInfo
{
    PBYTE OriginalReturnAddress;
    API_INFO* pApiInfos;
    BOOL bIsUsed;
    DWORD PreviousIndex;
}SIMPLE_HOOK_CRITICAL_CALL_INFO,*PSIMPLE_HOOK_CRITICAL_CALL_INFO;

#pragma pack(push)
#pragma pack (1)
typedef union _FPU_STATUS_REGISTER // Floating point unit status register struct definition for quick access
{
    struct _Registers
    {
        WORD ExceptionInvalidOperation:1,        
            ExceptionDenormalized:1,
            ExceptionZeroDevide:1,
            ExceptionOverflow:1,
            ExceptionUnderflow:1,
            ExceptionPrecision:1,
            ExceptionStackFault:1,
            ExceptionFlag:1,
            ConditionCode0:1,
            ConditionCode1:1,
            ConditionCode2:1,
            TopOfStack:3,
            ConditionCode3:1,
            Busy:1;
    }Registers;

    WORD wRegisters;
}FPU_STATUS_REGISTER;
#pragma pack (pop)

#define FPU_BUFFER_SIZE 108 // FSAVE and FRSTOR instructions which save/restore the FPU context to a 94 or 108 byte memory space depending on the CPU operating mode (16 or 32 bit)
void RestoreFloatingRegistersAndUnmaskFloatingExceptions(PBYTE FPUContent);
void SaveFloatingRegistersAndMaskFloatingExceptions(PBYTE FPUContent);
BOOL IsSt0Empty();
void FreeLastSimpleHookCriticalCallInfo();
SIMPLE_HOOK_CRITICAL_CALL_INFO* GetLastSimpleHookCriticalCallInfo();
SIMPLE_HOOK_CRITICAL_CALL_INFO* GetFreeSimpleHookCriticalCallInfo();
void APIHandler();
void APIAfterCallHandler();
void APINotMonitoredGlobalAfterCallHandler();
void APINotMonitoredTlsAfterCallHandler();
void __fastcall ParseAPIParameters(API_INFO* const pAPIInfo, REGISTERS* const pRegistersBeforeCall, LOG_INFOS* const pLogInfo, PBYTE const ParamStackAddress);
void Break(PAPI_INFO pAPIInfo,LOG_INFOS* pLogInfo,PBYTE StackParamtersPointer,PREGISTERS pRegisters,double* pDoubleResult,PBYTE ReturnAddress,PBYTE EbpAtAPIHandler,BOOL BeforeCall);
void BadParameterNumber(PAPI_INFO pAPIInfo,DWORD dwCurrentParamSize,DWORD dwRealParamSize);

BOOL __fastcall CheckParamLogFilters(API_INFO* const pAPIInfo, LOG_INFOS* const pLogInfo);
BOOL __fastcall CheckParamBreakFilters(API_INFO* const pAPIInfo, LOG_INFOS* const pLogInfo);
BOOL __fastcall CheckParamFilters(API_INFO* const pAPIInfo, LOG_INFOS* const pLogInfo,BOOL const bLogFilters);
BOOL __fastcall DoFunctionFail(API_INFO* const pAPIInfo,PBYTE const Return,double const FloatingReturn,DWORD const dwLastErrorCode);

void ReportBadHookChainBadCallingConvention(API_INFO *pAPIInfo,PBYTE PrePostHookCallBack,BOOL bPreHook);

void LogExceptionInformations(API_HANDLER_TLS_DATA* pTlsData,_EXCEPTION_RECORD *ExceptionRecord,_CONTEXT *ContextRecord);
BOOL HookExceptionHandler(PBYTE NewExceptionHandler,PBYTE* pOriginalExceptionHandler,PBYTE* pOriginalExceptionHandlerAddress);
BOOL UnhookExceptionHandler(PBYTE OriginalExceptionHandler,PBYTE OriginalExceptionHandlerAddress);
void FORCEINLINE __fastcall RestoreHook(CLinkListSingleThreaded* const pLinkListTlsData,API_HANDLER_TLS_DATA* const pTlsData,API_INFO* const pAPIInfo,DWORD const LastError);
void FORCEINLINE __fastcall CheckParameterNumbers(API_INFO* const pAPIInfo,DWORD const StackChangedSize);
void LogExceptionAndRestoreHook(CLinkListSingleThreaded* pLinkListTlsData);


DWORD APIOverrideInternalModulesLimitsIndex=0;
APIOVERRIDE_INTERNAL_MODULELIMITS APIOverrideInternalModulesLimits[MAX_APIOVERRIDE_MODULESLIMITS]={0};
SIMPLE_HOOK_CRITICAL_CALL_INFO SimpleHookCriticalCallInfo[MAX_SIMPLE_HOOK_CRITICAL_CALL_INFO]={0};

extern BOOL bMonitoring;
extern BOOL bFaking;
extern BOOL bFiltersApplyToMonitoring;
extern BOOL bFiltersApplyToFaking;
extern CLinkList* pLinkListAPIInfos;
extern CLinkList* pLinkListAPIInfosToBeFree;
extern CLinkList* pLinkListLoadedDll;
extern CCOM_Manager* pComManager;
extern CNET_Manager* pNetManager;
extern CLogAPI* pLogAPI;
extern DWORD dwSystemPageSize;
extern CModulesFilters* pModulesFilters;
extern HANDLE hevtAllAPIUnhookedDllFreeAll;
extern HANDLE hevtFreeAPIInfo;
extern HANDLE hevtWaitForUnlocker;
extern HANDLE hevtUnload;
extern HANDLE hevtSimpleHookCriticalCallUnlockedEvent;
extern DWORD dwCurrentProcessID;
extern BOOL FreeingThreadGracefullyClosed;
extern double dFloatingNotSet;
extern tagFirstBytesAutoAnalysis FirstBytesAutoAnalysis;
extern BOOL bDebugMonitoringFile;
extern LARGE_INTEGER PerformanceFrequency;
extern LARGE_INTEGER ReferenceCounter;
extern LARGE_INTEGER ReferenceTime;
extern HANDLE ApiOverrideLogHeap;
extern HANDLE ApiOverrideHeap;
extern HANDLE ApiOverrideKernelHeap;
extern DWORD ApiOverrideTlsIndex;
extern DWORD ApiOverrideListCriticalPartTlsIndex;
extern DWORD ApiOverrideSimpleHookCriticalCallTlsIndex;
extern CLinkListSimple* pLinkListAllThreadTlsData;
extern BOOL ApiOverrideDllDetaching;

#define asm_memcpy(Dest,Source,ln) __asm     \
{                                            \
    __asm push esi                           \
    __asm push edi                           \
    __asm push ecx                           \
    __asm cld                                \
    __asm mov esi, [Source]                  \
    __asm mov edi, [Dest]                    \
    __asm mov ecx, [ln]                      \
                                             \
    __asm shr ecx, 2                         \
    __asm rep movsd                          \
                                             \
    __asm mov ecx, [ln]                      \
    __asm and ecx, 3                         \
    __asm rep movsb                          \
    __asm pop ecx                            \
    __asm pop edi                            \
    __asm pop esi                            \
}


struct EXCEPTION_REGISTRATION
{
    EXCEPTION_REGISTRATION *prev;
    DWORD handler;
};
typedef EXCEPTION_DISPOSITION (__cdecl *pfExceptionHandler)(
                                                            _EXCEPTION_RECORD *ExcRecord,
                                                            void * EstablisherFrame, 
                                                            _CONTEXT *ContextRecord,
                                                            void * DispatcherContext);

//-----------------------------------------------------------------------------
// Name: ApiOverrideExceptionHandler
// Object: exception handler : called on exception before original handler, for hooked exception handler
//         allow to do specific operations (memory freeing, logging, ...) when an exception occurs inside a hooked function
// Parameters :
//     in : 
// Return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" EXCEPTION_DISPOSITION __cdecl ApiOverrideExceptionHandler(
                                        _EXCEPTION_RECORD *ExcRecord,
                                        void * EstablisherFrame, 
                                        _CONTEXT *ContextRecord,
                                        void * DispatcherContext)
{
    PBYTE OriginalExceptionHandler;
    API_HANDLER_TLS_DATA* pTlsData = NULL;

    // get hook associated with exception
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return ExceptionContinueSearch;
    if (!pApiOverrideTlsData->pLinkListTlsData)
        return ExceptionContinueSearch;

    while (pApiOverrideTlsData->pLinkListTlsData->Tail)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)(pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);
        // has soon there's a successful exception hooking,
        // we are sure that items where removed from the list on exceptions
        if (!pTlsData->bExceptionHookError)
            break;

        // else, as we are in an exception, that means an exception occurred for items

        // 5.5.1 : store information in case of uncatched exception catched by debugger
        // do it in reverse order, so first hook is in pLinkListTlsDataPotentialUncatchException Head
        // (in case of uncatched exception, for the last call of this handler
        // pLinkListTlsDataPotentialUncatchException content will be the same that 
        // pLinkListTlsData before exception occurred)
        pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->InsertItem(pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->Head,pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);

        // log function information, restore hook, free stack duplication and pop pLinkListTlsData informations (remove hook infos from list)
        LogExceptionAndRestoreHook(pApiOverrideTlsData->pLinkListTlsData);
    }
    // must be after "while (pLinkListTlsData->Tail)" because "LogExceptionAndRestoreHook(pLinkListTlsData);" acts on pLinkListTlsData content
    if (!pApiOverrideTlsData->pLinkListTlsData->Tail)
        return ExceptionContinueSearch;

    // store OriginalExceptionHandler in local var to allow pTlsData memory freeing
    OriginalExceptionHandler=pTlsData->OriginalExceptionHandler;

    //////////////////////
    // 1) do our own actions (should be done first as we don't now what original handler is going to do)
    //////////////////////

    if (pTlsData->pAPIInfo->HookType==HOOK_TYPE_NET)
    {
        CLinkListItem* pItem;
        BOOL UpperExceptionHandlerFound=FALSE;

        // as .Net exception handling come after all c++ exception, all our hooked exception handler are going to be called
        // do work only for first c++ exception filter

        HOOK_THREAD_NET_EXCEPTION_INFOS* pNetExceptionInfos=pApiOverrideTlsData->pNetExceptionInfos;
        if (!pNetExceptionInfos)
        {
            pNetExceptionInfos=new HOOK_THREAD_NET_EXCEPTION_INFOS;
            pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem=NULL;
            pNetExceptionInfos->NetExceptionBeingProcessing=FALSE;
            pApiOverrideTlsData->pNetExceptionInfos = pNetExceptionInfos;
        }
        if (!pNetExceptionInfos->NetExceptionBeingProcessing)
        {
            // log exception content
            LogExceptionInformations(pTlsData,ExcRecord,ContextRecord);

            // restore return address (needed by .Net for a successful stack walk)
            for (pItem=pApiOverrideTlsData->pLinkListTlsData->Tail;pItem;pItem=pItem->PreviousItem)
            {
                pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
                // restore original return address
                *((PBYTE*)pTlsData->AddressOfOriginalReturnAddress)=pTlsData->OriginalReturnAddress;
                
                // find first upper exception handler that is not ApiOverrideExceptionHandler
                // to know which handler call at the end of current function
                if (!UpperExceptionHandlerFound)
                {
                    if (pTlsData->OriginalExceptionHandler!=(PBYTE)ApiOverrideExceptionHandler)
                    {
                        OriginalExceptionHandler=pTlsData->OriginalExceptionHandler;
                        UpperExceptionHandlerFound=TRUE;
                        pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem=pItem;
                    }
                }

                // restore original exception handlers
                UnhookExceptionHandler(pTlsData->OriginalExceptionHandler,pTlsData->OriginalExceptionHandlerAddress);
            }
            pNetExceptionInfos->NetExceptionBeingProcessing=TRUE;
        }
        else// a net exception is already being processing
        {
            if (OriginalExceptionHandler==(PBYTE)ApiOverrideExceptionHandler)
            {
                // restore first exception handler different than ApiOverrideExceptionHandler

                // get the last item having restored an exception handler different than ApiOverrideExceptionHandler
                if (!pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem)
                      pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem=pApiOverrideTlsData->pLinkListTlsData->Tail;

                // parse the list of remaining exceptions handlers
                for (pItem=pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem->PreviousItem;pItem;pItem=pItem->PreviousItem)
                {
                    pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
                    if (*((PBYTE*)pTlsData->OriginalExceptionHandlerAddress)!=(PBYTE)ApiOverrideExceptionHandler)
                    {
                        // get original exception handler
                        OriginalExceptionHandler=*((PBYTE*)pTlsData->OriginalExceptionHandlerAddress);
                        // save item having restored exception handler
                        pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem=pItem;
                        break;
                    }
                }
                // if previous exception handler still not found, avoid a stack overflow
                if (OriginalExceptionHandler==(PBYTE)ApiOverrideExceptionHandler)// should not occur
                    return ExceptionContinueSearch;
            }
        }
    }
    else
    {
        // restore exception handler, so our handler won't be called for unwinding (very important has log tls data are going to be removed)
        UnhookExceptionHandler(pTlsData->OriginalExceptionHandler,pTlsData->OriginalExceptionHandlerAddress);

        // log exception content
        LogExceptionInformations(pTlsData,ExcRecord,ContextRecord);

        // 5.5.1 : store information in case of uncatched exception catched by debugger
        // do it in reverse order, so first hook is in pLinkListTlsDataPotentialUncatchException Head
        // (in case of uncatched exception, for the last call of this handler
        // pLinkListTlsDataPotentialUncatchException content will be the same that 
        // pLinkListTlsData before exception occurred)
        pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->InsertItem(pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->Head,pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);

        // log function information, restore hook, free stack duplication and pop pLinkListTlsData informations (remove hook infos from list)
        LogExceptionAndRestoreHook(pApiOverrideTlsData->pLinkListTlsData);
    }


    //////////////////////
    // 2) call original exception handler (warning pTlsData may be destroyed, so use our local OriginalExceptionHandler)
    //////////////////////
    if (IsBadCodePtr((FARPROC(OriginalExceptionHandler))))
        return ExceptionContinueSearch;

// possible improvement : avoid to be detected by exception handler using naked ?
    return ((pfExceptionHandler)OriginalExceptionHandler)(ExcRecord,EstablisherFrame,ContextRecord,DispatcherContext);
}

//-----------------------------------------------------------------------------
// Name: HookExceptionHandler
// Object: install hook on first exception handler
// Parameters :
//     in : PBYTE NewExceptionHandler : exception handler to install
//     out : PBYTE* pOriginalExceptionHandler : old exception handler
//           PBYTE* pOriginalExceptionHandlerAddress : old exception handler address
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL HookExceptionHandler(PBYTE NewExceptionHandler,PBYTE* pOriginalExceptionHandler,PBYTE* pOriginalExceptionHandlerAddress)
{
    EXCEPTION_REGISTRATION* pExceptionRegistration;

    // get pointer to current EXCEPTION_REGISTRATION
    _asm
    {
        mov EAX, FS:[0]
        mov [pExceptionRegistration], EAX
    }
    // check if exception handler is set and can be hooked
    if (IsBadReadPtr((PVOID)pExceptionRegistration,sizeof(EXCEPTION_REGISTRATION)))
    {
        *pOriginalExceptionHandlerAddress = NULL;
        *pOriginalExceptionHandler = NULL;

        TCHAR sz[MAX_PATH];
        _sntprintf(sz,MAX_PATH,
            _T("Exceptions can't be hooked for thread 0x%.8X for exception regitration at address 0x%p"),
            GetCurrentThreadId(),
            pExceptionRegistration
            );
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,sz);
        return FALSE;
    }

    // get exception handler address
    *pOriginalExceptionHandlerAddress=(PBYTE)&pExceptionRegistration->handler;
    // get original exception handler
    *pOriginalExceptionHandler=(PBYTE)pExceptionRegistration->handler;
    // install new exception handler
    pExceptionRegistration->handler=(DWORD)NewExceptionHandler;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UnhookExceptionHandler
// Object: remove first exception handler hook
// Parameters :
//     in : PBYTE OriginalExceptionHandler : original exception handler 
//          PBYTE OriginalExceptionHandlerAddress : original exception handler address
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL UnhookExceptionHandler(PBYTE OriginalExceptionHandler,PBYTE OriginalExceptionHandlerAddress)
{
    if (OriginalExceptionHandlerAddress == NULL)
        return FALSE;

    *((PBYTE*)OriginalExceptionHandlerAddress)=OriginalExceptionHandler;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CreateTlsData
// Object: create CApiOverrideTlsData stored in tls
// Parameters :
//     in : 
// Return : created CApiOverrideTlsData object
//-----------------------------------------------------------------------------
CApiOverrideTlsData* CreateTlsData()
{
    DWORD LastError = GetLastErrorApiSubstitution();

    // from now no thread call must be hooked
    // no function must be called before TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)TRUE); to avoid infinite loop
    TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)TRUE);

    CApiOverrideTlsData* pApiOverrideTlsData = new CApiOverrideTlsData();

    // as link list is in tls (so in a single thread), we can use CLinkListSingleThreaded instead of CLinkList to earn some time
    CLinkListSingleThreaded* pLinkListTlsData=new CLinkListSingleThreaded(sizeof(API_HANDLER_TLS_DATA));
    pLinkListTlsData->SetHeap(ApiOverrideKernelHeap);
    pApiOverrideTlsData->pLinkListTlsData = pLinkListTlsData;

    CLinkListSingleThreaded* pLinkListTlsDataPotentialUncatchException=new CLinkListSingleThreaded(sizeof(API_HANDLER_TLS_DATA));
    pLinkListTlsDataPotentialUncatchException->SetHeap(ApiOverrideKernelHeap);
    pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException = pLinkListTlsDataPotentialUncatchException;

    TlsSetValueApiSubstitution(ApiOverrideTlsIndex,pApiOverrideTlsData);

    pLinkListAllThreadTlsData->AddItem(pApiOverrideTlsData);

    // from now thread call can be hooked
    // no function must be called after TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)FALSE); to avoid infinite loop
    TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,FALSE);

    SetLastErrorApiSubstitution(LastError);

    return pApiOverrideTlsData;
}


//-----------------------------------------------------------------------------
// Name: DestroyTlsData
// Object: destroy data contained in tls MUST BE CALLED ONLY BY THREAD DETACH
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void DestroyTlsData()
{
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return;

    ///////////////////////////////////////////
    // from now no call must be hooked for current thread
    ///////////////////////////////////////////
    
    TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)TRUE);
    pLinkListAllThreadTlsData->Lock();

    // remove list from all thread list
    pLinkListAllThreadTlsData->RemoveItemFromItemData(pApiOverrideTlsData,TRUE);

    pLinkListAllThreadTlsData->Unlock();
    TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)FALSE);

    // clear tls value before destroying memory
    TlsSetValueApiSubstitution(ApiOverrideTlsIndex,NULL);
    delete pApiOverrideTlsData;
}

//-----------------------------------------------------------------------------
// Name: NetExceptionCatcherEnterCallBack
// Object: callback for ICorProfilerCallback::ExceptionCatcherEnter
//         catch code is being entered : restore return address of hooked calling function (functions unaffected by exception) to APIAfterCallHandler
//         (remember we have to restore original return address at the begin of exception for successful .net exception handler walk)
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void __stdcall NetExceptionCatcherEnterCallBack()
{
    // reset thread net exception global vars
    HOOK_THREAD_NET_EXCEPTION_INFOS* pNetExceptionInfos;
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return;
    pNetExceptionInfos=pApiOverrideTlsData->pNetExceptionInfos;
    if (pNetExceptionInfos==NULL)
        return;
    pNetExceptionInfos->NetExceptionBeingProcessing=FALSE;
    pNetExceptionInfos->pNetExceptionLastExceptionHandlerItem=NULL;

    //////////////////////////////////////////////
    // ret address restoration of unaffected stack
    // & exception Handler restoration
    //////////////////////////////////////////////
    CLinkListItem* pItem;
    API_HANDLER_TLS_DATA* pTlsData;
    if (pApiOverrideTlsData->pLinkListTlsData)
    {
        for (pItem=pApiOverrideTlsData->pLinkListTlsData->Head;pItem;pItem=pItem->NextItem)
        {
            pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
            if (!pNetManager->AreEnterLeaveSpied())
                *((PBYTE*)pTlsData->AddressOfOriginalReturnAddress)=(PBYTE)APIAfterCallHandler;
            *((PBYTE*)pTlsData->OriginalExceptionHandlerAddress)=(PBYTE)ApiOverrideExceptionHandler;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: NetTlsRestoreHookAddress
// Object: change return address of all hooked function to APIAfterCallHandler
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void NetTlsRestoreAllHookAddress()
{
    CLinkListItem* pItem;
    API_HANDLER_TLS_DATA* pTlsData;
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (pApiOverrideTlsData)
    {
        if (pApiOverrideTlsData->pLinkListTlsData)
        {
            for (pItem=pApiOverrideTlsData->pLinkListTlsData->Head;pItem;pItem=pItem->NextItem)
            {
                pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
                *((PBYTE*)pTlsData->AddressOfOriginalReturnAddress)=(PBYTE)APIAfterCallHandler;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: NetTlsRestoreHookAddress
// Object: change return address of hooked function to APIAfterCallHandler
//         (in .net when enter leave function are spied, function is called with changing return address
//          and return address is changed inside the leave hook : when function has been called but we are not returned to caller)
//         called by Net profiler LeaveStub (see ICorProfilerInfo::SetEnterLeaveFunctionHooks)
// Parameters :
//     in : API_INFO* pAPIInfo : api info associated to function
// Return : 
//-----------------------------------------------------------------------------
void __stdcall NetTlsRestoreHookAddress(API_INFO* pAPIInfo)
{
    if (!pAPIInfo)
        return;

    CLinkListItem* pItem;
    API_HANDLER_TLS_DATA* pTlsData;
    // get last registered function (last function 
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return;
    if (!pApiOverrideTlsData->pLinkListTlsData)
        return;
    pItem=pApiOverrideTlsData->pLinkListTlsData->Tail;
    if (!pItem)
        return;

    // assume there is no list error
    pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
    if (pTlsData->pAPIInfo!=pAPIInfo)
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())
            DebugBreak();
#endif
        return;
    }

    // set return address to our after call handler
    *((PBYTE*)pTlsData->AddressOfOriginalReturnAddress)=(PBYTE)APIAfterCallHandler;
}


//-----------------------------------------------------------------------------
// Name: NetExceptionSearchFunctionLeaveCallBack
// Object: callback for ICorProfilerCallback::ExceptionSearchFunctionLeave
// Parameters :
//     in : 
//          BOOL ExceptionCatchedInsideHookedFunction : TRUE if exception is not propagated outside the hook
//                                                      that means current .net function catchs the exception
//          API_INFO* pApiInfo : hook info associated to .net function
// Return : 
//-----------------------------------------------------------------------------
void __stdcall NetExceptionSearchFunctionLeaveCallBack(BOOL ExceptionCatchedInsideFunction,API_INFO* pApiInfo)
{
    if (!pApiInfo) // if function is not hooked
        return;
    if (!ExceptionCatchedInsideFunction)// if exception not catched inside function
    {
        CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
        if (pApiOverrideTlsData)
        {
            if (pApiOverrideTlsData->pLinkListTlsData)
            {
                // API_HANDLER_TLS_DATA* pTlsData=(API_HANDLER_TLS_DATA*)pLinkListTlsData->Tail->ItemData;
                // exception handler is already removed
                // -> no need to call UnhookExceptionHandler(pTlsData->OriginalExceptionHandler,pTlsData->OriginalExceptionHandlerAddress);

                // 5.5.1 : store information in case of uncatched exception catched by debugger
                // do it in reverse order, so first hook is in pLinkListTlsDataPotentialUncatchException Head
                // (in case of uncatched exception, for the last call of this handler
                // pLinkListTlsDataPotentialUncatchException content will be the same that 
                // pLinkListTlsData before exception occurred)
                pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->InsertItem(pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->Head,pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);

                // log function information, restore hook, free stack duplication and pop pLinkListTlsData informations (remove hook infos from list)
                LogExceptionAndRestoreHook(pApiOverrideTlsData->pLinkListTlsData);
            }
        }
    }
    return;
}

//-----------------------------------------------------------------------------
// Name: SendLogInsideHookedFunctionCall
// Object:  send log of function before api call is finished
//          this can occurs in 2 cases :
//              - exceptions
//              - unhooking query when api call is not finished (ex blocking api like message box)
// Parameters :
//     in : CLinkList* pLinkListTlsData
//          API_HANDLER_TLS_DATA* pTlsData
//          BOOL Failure : TRUE if function fail (exception)
//          BOOL ForceLog : TRUE if force log (like monitoring file debug mode)
//          DWORD ThreadId
// Return : 
//-----------------------------------------------------------------------------
void SendLogInsideHookedFunctionCall(CLinkListSingleThreaded* pLinkListTlsData,API_HANDLER_TLS_DATA* pTlsData, BOOL Failure, BOOL ForceLog, DWORD ThreadId)
{
    // if function must monitor parameters
    if (pTlsData->pAPIInfo->MonitoringParamCount>0)
    {
        // if input parameters havn't been parsed
        if (!pTlsData->LogInfoIn.ParmeterParsed)
        {
            // parse parameters

            PBYTE pParametersPointer;
            // if a copy of stack has been done
            if (pTlsData->pParametersPointer)
                pParametersPointer=pTlsData->pParametersPointer;
            else
                // during exception report stack should not be destroyed, so parameters should be still available
                pParametersPointer=(PBYTE)pTlsData->OriginalRegisters.esp;

            pTlsData->LogInfoIn.NbParameters=pTlsData->pAPIInfo->MonitoringParamCount;
            ParseAPIParameters(pTlsData->pAPIInfo, &pTlsData->OriginalRegisters, &pTlsData->LogInfoIn,pParametersPointer);
        }
    }

    // if function should be logged
    if ( (bMonitoring && pTlsData->pAPIInfo->pMonitoringFileInfos)
         || ForceLog
       )
    {
        // send log
        CLogAPI::AddLogEntry(pTlsData->pAPIInfo, &pTlsData->LogInfoIn,(PBYTE)-1,dFloatingNotSet,Failure,
            pTlsData->OriginalReturnAddress ,
            PARAM_DIR_TYPE_IN_NO_RETURN,pTlsData->szCallingModuleName,pTlsData->RelativeAddressFromCallingModule,
            &pTlsData->LogOriginalRegisters,&pTlsData->LogOriginalRegisters,
            (PBYTE)pTlsData->OriginalRegisters.ebp,ThreadId,pLinkListTlsData);

        // free allocated memory by ParseAPIParameters
        if (pTlsData->LogInfoIn.ParmeterParsed)
        {
            DWORD Cnt;
            for (Cnt = 0; Cnt < pTlsData->pAPIInfo->MonitoringParamCount; Cnt++)
            {
                // if memory has been allocated
                if (pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue)
                {
                    HeapFree(ApiOverrideLogHeap, 0,pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue);
                    pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue=NULL;
                }
            }
            pTlsData->LogInfoIn.ParmeterParsed=FALSE;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: LogExceptionAndRestoreHook
// Object:  log exception that occurred inside hooked api / function call and restore hook
// Parameters :
//     in : CLinkList* pLinkListTlsData
// Return : 
//-----------------------------------------------------------------------------
void LogExceptionAndRestoreHook(CLinkListSingleThreaded* pLinkListTlsData)
{
    API_HANDLER_TLS_DATA* pTlsData=(API_HANDLER_TLS_DATA*)(pLinkListTlsData->Tail->ItemData);

    SendLogInsideHookedFunctionCall(pLinkListTlsData,pTlsData,TRUE,FALSE,GetCurrentThreadId());

    // restore hook, and pop pLinkListTlsData informations (remove hook infos from list)
    RestoreHook(pLinkListTlsData,pTlsData,pTlsData->pAPIInfo,0);
}


//-----------------------------------------------------------------------------
// Name: CheckForExceptionInsideNotHookedExceptionsHandlers
// Object:  
//          check for exception inside hooked functions, which exception handler can't be hooked
//          if an exception is not hooked, and exception occurs, 
//          this can make a difference between our list and reality 
//          (our list may contain information of functions exited by the exception)
//          this function tries to check and restore concordance
//          Notice : this should appear only in extreme cases
// Parameters :
//     in : BOOL bBeforeCall : TRUE if this function is called before api call (in pre part), 
//                             FALSE if this function is called after api call (in post part), 
//          PBYTE FS0Value : 
//          PBYTE StackPointer :
// Return : 
//-----------------------------------------------------------------------------
void CheckForExceptionInsideNotHookedExceptionsHandlers(BOOL bBeforeCall,PBYTE FS0Value,PBYTE StackPointer)
{
    BOOL bExceptionOccured;
    API_HANDLER_TLS_DATA* pTlsData;

    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return;
    if (!pApiOverrideTlsData->pLinkListTlsData)
        return;

    if (bBeforeCall) 
    {
        if (pApiOverrideTlsData->pLinkListTlsData->GetItemsCount()==0)
            return;
    }
    else// if after call checking
    {
        // if there's only one item, we are sure there is no failure 
        // whatever the value of pTlsData->bExceptionHookError 
        if (pApiOverrideTlsData->pLinkListTlsData->GetItemsCount()==1)
            return;
    }

    DWORD Cnt;
    API_INFO* pAPIInfo;
    DWORD MaxParamStackSize;

    while (pApiOverrideTlsData->pLinkListTlsData->Tail)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)(pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);
        // has soon there's a successful exception hooking,
        // we are sure that items where removed from the list on exceptions
        if (!pTlsData->bExceptionHookError)
            return;

        // else, we have to do an fs:0 / esp checking
        bExceptionOccured = FALSE;

        //////////////////
        // fs:0 checking
        //////////////////
        if (bBeforeCall)
        {
            // current fs:0 must be less or equal to previously logged fs:0
            if (FS0Value > (PBYTE)pTlsData->FS0Value ) // if current fs:0 is greater than the logged one, that mean an exception has occurred
                bExceptionOccured = TRUE;
            else if (FS0Value == (PBYTE)pTlsData->FS0Value) // it can be equal but in this case current esp MUST be lesser than the logged one
            {
                if (StackPointer > (PBYTE)(pTlsData->LogOriginalRegisters.esp) )
                {
                    bExceptionOccured = TRUE;
                }
            }
            // assume caller fs:0 belongs to exceptions record list
        }
        //else // after call 
        //{
        //    // Theory : if fs:0 is different, that means an exception has occurred
        //    // if (FS0Value != (PBYTE)pTlsData->FS0Value)
        //    //      bExceptionOccured = TRUE;
        //    // Reality : some function install exception handler without removing them --> assume old fs:0 belongs to exceptions record list
        //}

        if (!bExceptionOccured) // fs:0 checking common for before call and after call
        {
            if ( FS0Value != pTlsData->FS0Value )
            {
                EXCEPTION_REGISTRATION* pExceptionRegistration;
                pExceptionRegistration = (EXCEPTION_REGISTRATION*)FS0Value;

                // if current exception registration is accessible (avoid to default bExceptionOccured to TRUE in this case)
                if (!IsBadReadPtr(pExceptionRegistration, sizeof(EXCEPTION_REGISTRATION)))
                {
                    // loop through exceptions registrations list to find our logged exception registration
                    for(bExceptionOccured = FALSE;;pExceptionRegistration = pExceptionRegistration->prev)
                    {
                        if (IsBadReadPtr(pExceptionRegistration, sizeof(EXCEPTION_REGISTRATION)))
                        {
                            if (pExceptionRegistration > (EXCEPTION_REGISTRATION*)pTlsData->FS0Value)
                            {
                                bExceptionOccured = TRUE;
                            }
                            break;
                        }
                        if ( pExceptionRegistration->prev == (EXCEPTION_REGISTRATION*)pTlsData->FS0Value )
                        {
                            break;
                        }
                    }
                }
            }
        }

        //////////////////
        // esp checking
        //////////////////
        if (!bExceptionOccured)
        {
            if (bBeforeCall)
            {
                // current stack pointer must be less or equal to log stack pointer
                if (StackPointer > (PBYTE)pTlsData->LogOriginalRegisters.esp)
                    bExceptionOccured = TRUE;
            }
            else // after call 
            {
                MaxParamStackSize=0;
                pAPIInfo = pTlsData->pAPIInfo;
                for (Cnt=0;Cnt<pAPIInfo->MonitoringParamCount;Cnt++)
                {
                    MaxParamStackSize+=__max(pAPIInfo->ParamList[Cnt].dwSizeOfData,sizeof(PBYTE));
                }

                // stack pointer should be between (log stack pointer + max stack args size) and (log stack pointer)
                if (StackPointer > (PBYTE)(pTlsData->LogOriginalRegisters.esp + MaxParamStackSize) ) // log stack pointer + max stack args size
                {
                    bExceptionOccured = TRUE;
                }
                else
                {
                    if (StackPointer < (PBYTE)(pTlsData->LogOriginalRegisters.esp) ) // log stack pointer
                    {
                        bExceptionOccured = TRUE;
                    }
                }
            }
        }

        if (bExceptionOccured)
        {
            // 5.5.1 : store information in case of uncatched exception catched by debugger
            // do it in reverse order, so first hook is in pLinkListTlsDataPotentialUncatchException Head
            // (in case of uncatched exception, for the last call of this handler
            // pLinkListTlsDataPotentialUncatchException content will be the same that 
            // pLinkListTlsData before exception occurred)
            pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->InsertItem(pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->Head,pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);

            // log function information, restore hook, and pop pLinkListTlsData informations 
            // remove hook infos from pLinkListTlsData list
            // pApiOverrideTlsData->pLinkListTlsData->RemoveItem(pApiOverrideTlsData->pLinkListTlsData->Tail); // done by LogExceptionAndRestoreHook
            LogExceptionAndRestoreHook(pApiOverrideTlsData->pLinkListTlsData);
        }
        else
        {
            break;
        }
    }
}


//-----------------------------------------------------------------------------
// Name: LogExceptionInformations
// Object:  log exception that occurred inside api / function call
// Parameters :
//     in : API_HANDLER_TLS_DATA* pTlsData
//          _EXCEPTION_RECORD *ExceptionRecord
//          _CONTEXT *ContextRecord
// Return : 
//-----------------------------------------------------------------------------
#define SOFTWARE_EXCEPTION 0xE06D7363 // c++ software exception code
#define MANAGED_EXCEPTION  0xE0434F4D // managed exception code
void LogExceptionInformations(API_HANDLER_TLS_DATA* pTlsData,_EXCEPTION_RECORD *ExceptionRecord,_CONTEXT *ContextRecord)
{
    TCHAR szExceptionType[50];
    TCHAR szException[4*MAX_PATH];

    switch(ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION: 
        {
            _tcscpy(szExceptionType,_T("ACCESS_VIOLATION"));
            if (ExceptionRecord->NumberParameters>=2)
            {
                if (ExceptionRecord->ExceptionInformation[0]==0)
                    _stprintf(szExceptionType,_T("ACCESS_VIOLATION reading location 0x%p"),ExceptionRecord->ExceptionInformation[1]);
                else
                    _stprintf(szExceptionType,_T("ACCESS_VIOLATION writing location 0x%p"),ExceptionRecord->ExceptionInformation[1]);
            }
        }
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        _tcscpy(szExceptionType,_T("DATATYPE_MISALIGNMENT"));
        break;
    case EXCEPTION_BREAKPOINT:
        _tcscpy(szExceptionType,_T("BREAKPOINT"));
        break;
    case EXCEPTION_SINGLE_STEP:
        _tcscpy(szExceptionType,_T("SINGLE_STEP"));
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        _tcscpy(szExceptionType,_T("ARRAY_BOUNDS_EXCEEDED"));
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        _tcscpy(szExceptionType,_T("FLT_DENORMAL_OPERAND"));
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        _tcscpy(szExceptionType,_T("FLT_DIVIDE_BY_ZERO"));
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        _tcscpy(szExceptionType,_T("FLT_INEXACT_RESULT"));
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        _tcscpy(szExceptionType,_T("FLT_INVALID_OPERATION"));
        break;
    case EXCEPTION_FLT_OVERFLOW:
        _tcscpy(szExceptionType,_T("FLT_OVERFLOW"));
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        _tcscpy(szExceptionType,_T("FLT_STACK_CHECK"));
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        _tcscpy(szExceptionType,_T("FLT_UNDERFLOW"));
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        _tcscpy(szExceptionType,_T("INT_DIVIDE_BY_ZERO"));
        break;
    case EXCEPTION_INT_OVERFLOW:
        _tcscpy(szExceptionType,_T("INT_OVERFLOW"));
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        _tcscpy(szExceptionType,_T("PRIV_INSTRUCTION"));
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        _tcscpy(szExceptionType,_T("IN_PAGE_ERROR"));
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        _tcscpy(szExceptionType,_T("ILLEGAL_INSTRUCTION"));
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        _tcscpy(szExceptionType,_T("NONCONTINUABLE_EXCEPTION"));
        break;
    case EXCEPTION_STACK_OVERFLOW:
        _tcscpy(szExceptionType,_T("STACK_OVERFLOW"));
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        _tcscpy(szExceptionType,_T("INVALID_DISPOSITION"));
        break;
    case EXCEPTION_GUARD_PAGE:
        _tcscpy(szExceptionType,_T("GUARD_PAGE"));
        break;
    case EXCEPTION_INVALID_HANDLE:
        _tcscpy(szExceptionType,_T("INVALID_HANDLE"));
        break;
    case SOFTWARE_EXCEPTION:
        _tcscpy(szExceptionType,_T("SOFTWARE EXCEPTION"));
        break;
    case MANAGED_EXCEPTION:
        _tcscpy(szExceptionType,_T("MANAGED EXCEPTION"));
        break;
    default:
        _tcscpy(szExceptionType,_T("UNKNOWN EXCEPTION"));
        break;
    }
    // forge string like "exception msg || registers | ProcessId | ThreadId"
    // 5.4 changes : first "|" delimiter has been replaced by "||" delimiter because for internal function names, 
    // pTlsData->pAPIInfo->szModuleName contains a "|" which makes decofing fail
    if (pTlsData)
    {
        _sntprintf(szException,4*MAX_PATH,
                    _T("%s at address 0x%p thrown by %s (%s)")
                    _T("||EAX=0x%.8x, EBX=0x%.8x, ECX=0x%.8x, EDX=0x%.8x, ESI=0x%.8x, EDI=0x%.8x, EFL=0x%.8x, ESP=0x%.8x, EBP=0x%.8x")
                    _T("|0x%.8X")
                    _T("|0x%.8X"),
                    szExceptionType,
                    ExceptionRecord->ExceptionAddress,
                    pTlsData->pAPIInfo->szAPIName,
                    pTlsData->pAPIInfo->szModuleName,
                    ContextRecord->Eax,
                    ContextRecord->Ebx,
                    ContextRecord->Ecx,
                    ContextRecord->Edx,
                    ContextRecord->Esi,
                    ContextRecord->Edi,
                    ContextRecord->EFlags,
                    ContextRecord->Esp,
                    ContextRecord->Ebp,
                    GetCurrentProcessId(),
                    GetCurrentThreadId()
                    );
    }
    else
    {
        _sntprintf(szException,4*MAX_PATH,
                    _T("%s at address 0x%p")
                    _T("||EAX=0x%.8x, EBX=0x%.8x, ECX=0x%.8x, EDX=0x%.8x, ESI=0x%.8x, EDI=0x%.8x, EFL=0x%.8x, ESP=0x%.8x, EBP=0x%.8x")
                    _T("|0x%.8X")
                    _T("|0x%.8X"),
                    szExceptionType,
                    ExceptionRecord->ExceptionAddress,
                    ContextRecord->Eax,
                    ContextRecord->Ebx,
                    ContextRecord->Ecx,
                    ContextRecord->Edx,
                    ContextRecord->Esi,
                    ContextRecord->Edi,
                    ContextRecord->EFlags,
                    ContextRecord->Esp,
                    ContextRecord->Ebp,
                    GetCurrentProcessId(),
                    GetCurrentThreadId()
                    );
    }
    CReportMessage::ReportMessage(REPORT_MESSAGE_EXCEPTION,szException);
}

//////////////////////////////////////////////////////////////////////
// APIHandler :
// function call at API start
//
// WARNING if you don't filter APIOverride.dll call, call order of function used
//         inside APIHandler and subroutine can be false and stack report will be wrong
//         by the way if you hook ReadFile and WriteFile and do a call to ReadFile, as APIHandler call WriteFile
//         the log of WriteFile will be send before the one of ReadFile
//
//////////////////////////////////////////////////////////////////////
__declspec(naked) void APIHandler()
{
    API_INFO* pAPIInfo;
    PVOID pSource;
    PVOID pDest;
    DWORD dwSize;
    HANDLE ThreadHandle;
    int ThreadPriority;
    PDWORD pParametersPointer;
    PBYTE ReturnAddr;
    FARPROC FunctionAddress;

    REGISTERS OriginalRegisters;
    BOOL bLogInputParametersWithoutReturn;
    BOOL bFakeCurrentCall;
    BOOL bLogCall;
    DWORD LastErrorCode;
    PBYTE CurrentBasePointer;
    PDWORD OriginalEspWithReturnAddress;

    CLinkListItem* pItem;
    CLinkListSingleThreaded* pLinkListTlsData;
    API_HANDLER_TLS_DATA* pTlsData;
    BYTE FPUContent[FPU_BUFFER_SIZE];
    PBYTE FS0Value;
    CApiOverrideTlsData* pApiOverrideTlsData;
    BOOL bCallOriginalFunctionOnly;
    BOOL bCallingModuleMatchDefinedFilters;
    // try to don't call functions before restoring original opcode of current pAPIInfo (avoid infinite hook reentering)

    //////////////////////////////////////////////////////////////////////
    // get informations from the stack
    //////////////////////////////////////////////////////////////////////
    //Each function call creates a new stack frame with the following layout, note that high memory is at the top of the list: 
    // Function parameters 
    // Function return address 
 
    // call order was
    //      1) push API params
    //      2) call API --> push return address of hooked func
    //      3) code modification of first bytes of API
    //          jmp buffered hook (a buffered hook is needed to push pAPIInfo --> different for each hook)
    //      4) buffered hook
    //          push pAPIInfo
    //          jmp APIHandler
    //      5) APIHandler
    //          a) pushfd
    //          b) push other registers (notice you can push all data you want that a generated c function code will destroy)
    //          c) save frame (ebp)
    __asm
    {
        // push flags
        pushfd

        // push registers
        push        eax  
        push        ebx  
        push        ecx  
        push        edx  
        push        esi  
        push        edi  
        push        es   
        push        fs
        push        gs

        // standard frame saving
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE

        // get registers pushed by second hook
        Mov EAX, 0

        Mov AX, [EBP + 4]
        Mov [OriginalRegisters.gs], EAX

        Mov AX, [EBP + 8]
        Mov [OriginalRegisters.fs], EAX

        Mov AX, [EBP + 12]
        Mov [OriginalRegisters.es], EAX

        Mov EAX, [EBP + 16]
        Mov [OriginalRegisters.edi], EAX

        Mov EAX, [EBP + 20]
        Mov [OriginalRegisters.esi], EAX

        Mov EAX, [EBP + 24]
        Mov [OriginalRegisters.edx], EAX

        Mov EAX, [EBP + 28]
        Mov [OriginalRegisters.ecx], EAX

        Mov EAX, [EBP + 32]
        Mov [OriginalRegisters.ebx], EAX

        Mov EAX, [EBP + 36]
        Mov [OriginalRegisters.eax], EAX


        Mov EAX, [EBP + 40]
        Mov [OriginalRegisters.efl], EAX

        Mov EAX, [EBP + 44]
        Mov [pAPIInfo], EAX

        Mov EAX, [EBP + 48] // <-- return address of hooked func
        Mov [ReturnAddr], EAX

        Lea EAX, [EBP + 48]
        Mov [OriginalEspWithReturnAddress], EAX // OriginalEspWithReturnAddress CONTAINS ESP of the callee (ESP WITH THE PUSHED RETURN VALUE)

        Lea EAX, [EBP + 52]
        Mov [OriginalRegisters.esp], EAX // "OriginalRegisters.esp" CONTAINS ESP of the caller (ESP WITHOUT THE PUSHED RETURN VALUE)

        Lea EAX, [EBP + 52]
        Mov [pParametersPointer], EAX // <-- get api parameters

        mov [CurrentBasePointer],ebp // get current ebp
        
        // save current ebp content (caller's ebp)
        Mov EAX, [EBP]
        Mov [OriginalRegisters.ebp],EAX

        // allow stack retrieval later (ebp=*ebp ret addr=*(ebp+4))
        lea eax,[OriginalRegisters.ebp]
        mov [ebp],eax

        // restore real ebp+4 value (return address) to avoid call stack retrieval holes
        mov eax,[ReturnAddr]
        mov [ebp+4],eax
        
        mov EAX, FS:[0]
        mov [FS0Value], EAX
    }
    // do the first important first : setting lock to avoid unhooking
    // and set thread priority to the max

    // must be before hook removal
    // pAPIInfo->UseCount++;
    InterlockedIncrement(&pAPIInfo->UseCount);// intrinsic function 

    SaveFloatingRegistersAndMaskFloatingExceptions(FPUContent);// pure asm

    // avoid to loose last error code
    LastErrorCode=GetLastErrorApiSubstitution(); // pure asm

    pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);// pure asm func (no api call)

    bCallOriginalFunctionOnly = FALSE;
    /////////////////////////////////////////////
    // if (pAPIInfo->FirstBytesCanExecuteAnywhereSize || pAPIInfo->bFunctionPointer)
    // hook is let, we don't write original code back, 
    // but we have to check for potential infinite loop
    ////////////////////////////////////////////
    if (pAPIInfo->FirstBytesCanExecuteAnywhereSize
        || pAPIInfo->bFunctionPointer
        )
    {
        /////////////////////////////////////////////
        // avoid infinite loop
        ////////////////////////////////////////////
        if (pApiOverrideTlsData)
        {
            if (pApiOverrideTlsData->InsidePrePost)
            {
                bCallOriginalFunctionOnly = TRUE;
            }
            if (TlsGetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex))
            {
                bCallOriginalFunctionOnly = TRUE;
            }
        }
        else 
        {
            if (TlsGetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex))
            {
                bCallOriginalFunctionOnly = TRUE;
            }
            else
            {
                pApiOverrideTlsData = CreateTlsData();
            }
        }

        
        if (!bCallOriginalFunctionOnly)
        {
            //    // if function is already inside hook 1 time
            //    if (pAPIInfo->UseCount>1)
            //    {
            // Disabled since 5.2 : time consuming and pApiOverrideTlsData->InsidePrePost information is enough
            //        // if call comes from current dll or from a faking dll, call only the original function 
            //        // to avoid infinite loop
            //        // (else loop can appear for each function used by this APIHandler and all its subroutine)
            //        if (IsAPIOverrideInternalCall(ReturnAddr,CurrentBasePointer))
            //            bCallOriginalFunctionOnly = TRUE;
            //    }

            // if api is override, the overriding call may wants to call the original api,
            // and in this case our lock pApiOverrideTlsData->InsidePrePost is not set
            // --> we have to do more check (a better IsAPIOverrideInternalCall like)
            if (pAPIInfo->FakeAPIAddress)
            {
                // if this api is called more than once
                // Notice : UseCount is not in TLS so it just give us an information to speed up a little (the others call can be done in another thread)
                if (pAPIInfo->UseCount>1)
                {
                    for (pItem=pApiOverrideTlsData->pLinkListTlsData->Tail;pItem;pItem=pItem->PreviousItem)
                    {
                        pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
                        if (pTlsData->pAPIInfo == pAPIInfo)
                        {
                            // if faking is applied to previous occurrence (else it can be a simple recursive call)
                            if (pTlsData->bFakingApplied)
                            {
                                // call original function to avoid infinite loop
                                bCallOriginalFunctionOnly = TRUE;
                                break;
                            }
                        }
                    }
                }
            }
        }


        if (bCallOriginalFunctionOnly)
        {

            // WE MUST NOT USE OTHER API/FUNCTIONS (else we can re-enter this loop)

            // we have to call original function only (without spying or faking) to avoid infinite hook loop
            if (pAPIInfo->bFunctionPointer)
                // original opcode contains the real function address
                FunctionAddress=(FARPROC)(*((PBYTE*)pAPIInfo->Opcodes));
            else
                FunctionAddress=(FARPROC)&pAPIInfo->OpcodesExecutedAtAnotherPlace;

            // decrease use counter, incremented at the begin of the hook
            // pAPIInfo->UseCount--;
            InterlockedDecrement(&pAPIInfo->UseCount);
            if (pAPIInfo->UseCount<0)// can occurred for uncatched exception with flow restored by debugger
                pAPIInfo->UseCount=0;

            RestoreFloatingRegistersAndUnmaskFloatingExceptions(FPUContent);

            __asm
            {
                ////////////////////////////////
                // restore registers like they were at the begin of hook
                ////////////////////////////////
                mov eax, [OriginalRegisters.eax]
                mov ebx, [OriginalRegisters.ebx]
                mov ecx, [OriginalRegisters.ecx]
                mov edx, [OriginalRegisters.edx]
                mov esi, [OriginalRegisters.esi]
                mov edi, [OriginalRegisters.edi]
                push [OriginalRegisters.efl]
                popfd

                // restore stack
                mov esp, [OriginalEspWithReturnAddress]

                // push original function address on stack
                push FunctionAddress

                // restore base pointer at least
                mov ebp, [OriginalRegisters.ebp] // from now you can't access your function local var

                // call original API (as we push it's address on stack)
                ret
            }
        }

        /////////////////////////////////////////////
        // end of avoid infinite loop
        ////////////////////////////////////////////

        pApiOverrideTlsData->InsidePrePost = TRUE;

        // Reset end of hook event
        ResetEvent(pAPIInfo->evtEndOfHook);
    }
    else // (!pAPIInfo->FirstBytesCanExecuteAnywhereSize) && (!pAPIInfo->bFunctionPointer)
    {
        //////////////////////////////////////
        // restore original opcode
        //////////////////////////////////////

        // put flag to know we are restoring original opcode (in case of dump)
        pAPIInfo->bOriginalOpcodes=TRUE;
        // copy original stored instructions to execute original API func after the hook
        //memcpy(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize);
        pDest=(PVOID)pAPIInfo->APIAddress;
        pSource=(PVOID)pAPIInfo->Opcodes;
        dwSize=pAPIInfo->OpcodeReplacementSize;
        asm_memcpy(pDest, pSource, dwSize);

        // don't use pApiOverrideTlsData->InsidePrePost
        if (pApiOverrideTlsData)
        {
            if (TlsGetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex))
            {
                bCallOriginalFunctionOnly = TRUE;
            }
        }
        else
        {
            if (TlsGetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex))
            {
                bCallOriginalFunctionOnly = TRUE;
            }
            else
            {
                pApiOverrideTlsData = CreateTlsData();
            }
        }

        if (bCallOriginalFunctionOnly)
        {
            if (!pApiOverrideTlsData)
            {
                // change return address to our handler address
                *((PBYTE*)OriginalEspWithReturnAddress)=(PBYTE)APINotMonitoredGlobalAfterCallHandler;
                // we can't use pApiOverrideTlsData as tls storage as pApiOverrideTlsData is may not created
                // so we have to use static data storage
                SIMPLE_HOOK_CRITICAL_CALL_INFO* pSimpleHookCriticalCallInfo = GetFreeSimpleHookCriticalCallInfo();

                if (pSimpleHookCriticalCallInfo)
                {
                    pSimpleHookCriticalCallInfo->pApiInfos = pAPIInfo;
                    pSimpleHookCriticalCallInfo->OriginalReturnAddress = ReturnAddr;
                }
                else
                {
                    *((PBYTE*)OriginalEspWithReturnAddress)=(PBYTE)ReturnAddr;
                    InterlockedDecrement(&pAPIInfo->UseCount);
                    if (pAPIInfo->UseCount<0)// can occurred for uncatched exception with flow restored by debugger
                        pAPIInfo->UseCount=0;
                    // hook won't be restored
                }
            }
            else
            {
                // we can use TLS data (better than using global)

                // change return address to our handler address
                *((PBYTE*)OriginalEspWithReturnAddress)=(PBYTE)APINotMonitoredTlsAfterCallHandler;

                // use simple array instead of link list to avoid all api calls

                if (pApiOverrideTlsData->SimpleHookCriticalCallInfos.CurrentIndex==MAX_API_DEPTH_USED_IN_LIST_CRITICAL_PART)
                {
#ifdef _DEBUG
                    if (IsDebuggerPresent())// avoid to crash application if no debugger
                        ::DebugBreak();// MAX_API_DEPTH_USED_IN_LIST_CRITICAL_PART must be incremented
#endif

                    *((PBYTE*)OriginalEspWithReturnAddress)=(PBYTE)ReturnAddr;
                    InterlockedDecrement(&pAPIInfo->UseCount);
                    if (pAPIInfo->UseCount<0)// can occurred for uncatched exception with flow restored by debugger
                        pAPIInfo->UseCount=0;
                    // hook won't be restored
                }
                else
                {
                    TLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFO* pSimpleHookCriticalCallInfo=&pApiOverrideTlsData->SimpleHookCriticalCallInfos.HookCriticalInfos[pApiOverrideTlsData->SimpleHookCriticalCallInfos.CurrentIndex];
                    pSimpleHookCriticalCallInfo->pApiInfos=pAPIInfo;
                    pSimpleHookCriticalCallInfo->OriginalReturnAddress=ReturnAddr;
                    pApiOverrideTlsData->SimpleHookCriticalCallInfos.CurrentIndex++;
                }
            }

            RestoreFloatingRegistersAndUnmaskFloatingExceptions(FPUContent);
            FunctionAddress=pAPIInfo->APIAddress;
            __asm
            {
                ////////////////////////////////
                // restore registers like they were at the begin of hook
                ////////////////////////////////
                mov eax, [OriginalRegisters.eax]
                mov ebx, [OriginalRegisters.ebx]
                mov ecx, [OriginalRegisters.ecx]
                mov edx, [OriginalRegisters.edx]
                mov esi, [OriginalRegisters.esi]
                mov edi, [OriginalRegisters.edi]
                push [OriginalRegisters.efl]
                popfd

                // restore stack
                mov esp, [OriginalEspWithReturnAddress]

                // push original function address on stack
                push FunctionAddress

                // restore base pointer at least
                mov ebp, [OriginalRegisters.ebp] // from now you can't access your function local var

                // call original API (as we push it's address on stack)
                ret
            }
        }

        // Reset end of hook event
        ResetEvent(pAPIInfo->evtEndOfHook);

        // start boosting thread to try to avoid lost of other threads hook (when original opcodes are restored)
        // we do it after removing hook to avoid troubles hooking following funcs
        ThreadHandle=GetCurrentThread();
        ThreadPriority=GetThreadPriority(ThreadHandle);
#ifndef _DEBUG
        SetThreadPriority(ThreadHandle,THREAD_PRIORITY_TIME_CRITICAL);
#endif
    }

    bLogCall=(bMonitoring||bDebugMonitoringFile) && pAPIInfo->pMonitoringFileInfos;

    //////////////////////////////////////////////////
    // allocate and fill thread local storage data
    //////////////////////////////////////////////////

    // check for exception inside hooked functions, which exception handlers can't be hooked
    CheckForExceptionInsideNotHookedExceptionsHandlers(TRUE,FS0Value,(PBYTE)OriginalRegisters.esp);


    // all functions called by this thread and used inside item adding to list musn't be hooked to avoid infinite loop
    TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)TRUE);

    pLinkListTlsData=pApiOverrideTlsData->pLinkListTlsData;
    // clear potentially unhooked items
    while (pLinkListTlsData->Tail)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)(pLinkListTlsData->Tail->ItemData);
        if (!pTlsData->UnhookedDuringCall)
            break;
        pLinkListTlsData->RemoveItem(pLinkListTlsData->Tail);
    }
    pItem=pLinkListTlsData->AddItem();

    // item is added, we can hook following function
    TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,FALSE);

    pTlsData=(API_HANDLER_TLS_DATA*)(pItem->ItemData);
    pTlsData->FS0Value=FS0Value;
    // try to hook exception handler
    pTlsData->bExceptionHookError = !HookExceptionHandler((PBYTE)ApiOverrideExceptionHandler,
                                                           &pTlsData->OriginalExceptionHandler,
                                                           &pTlsData->OriginalExceptionHandlerAddress
                                                           );

    pTlsData->pAPIInfo=pAPIInfo;
    memcpy(&pTlsData->OriginalRegisters,&OriginalRegisters,sizeof(REGISTERS));
    pTlsData->OriginalReturnAddress=ReturnAddr;
    pTlsData->AddressOfOriginalReturnAddress=(PBYTE)OriginalEspWithReturnAddress;

    pTlsData->OriginalThreadPriority=ThreadPriority;
    //////////////////////////////////////////////////
    // End of allocate and fill thread local storage data
    //////////////////////////////////////////////////

    if(bLogCall) // try to speed up a little
    {
        // until GetSystemTimeAsFileTime returned value is updated every ms
        // get execution time (try to call it as soon as possible for log ordering)
        QueryPerformanceCounter(&pTlsData->TickCountBeforeCall);
        // compute number of 100ns (PerformanceFrequency is in count per second)
        pTlsData->TickCountBeforeCall.QuadPart=((pTlsData->TickCountBeforeCall.QuadPart-ReferenceCounter.QuadPart)*1000*1000*10)/PerformanceFrequency.QuadPart;
        pTlsData->TickCountBeforeCall.QuadPart+=ReferenceTime.QuadPart;
        pTlsData->LogInfoIn.CallTime.dwHighDateTime=(DWORD)pTlsData->TickCountBeforeCall.HighPart;
        pTlsData->LogInfoIn.CallTime.dwLowDateTime=(DWORD)pTlsData->TickCountBeforeCall.LowPart;
    }

    /////////////////////////////////////////////////////////////////////
    // initialize var
    /////////////////////////////////////////////////////////////////////
    pTlsData->BlockingCallThread=NULL;
    pTlsData->bLogInputParameters=FALSE;
    pTlsData->bMatchMonitoringFilters = FALSE;
    pTlsData->bMatchFakingFilters = FALSE;

    if(   bLogCall
       || (bFaking && pAPIInfo->FakeAPIAddress)// if faking enabled and a faking function is defined for the hooked function
       || pAPIInfo->PreApiCallChain // if a pre call chain is defined
       || pAPIInfo->PostApiCallChain // if a post call chain is defined
       ) 
    {
        bCallingModuleMatchDefinedFilters = FALSE;

        // check if we have to log item
        if(pModulesFilters->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(
            (PBYTE)ReturnAddr,
            &pTlsData->CallingModuleHandle,
            pTlsData->szCallingModuleName,
            &pTlsData->RelativeAddressFromCallingModule,
            &bCallingModuleMatchDefinedFilters,
            TRUE,
            !(bFiltersApplyToMonitoring && bFiltersApplyToFaking) // if !bFiltersApplyToMonitoring or !bFiltersApplyToFaking, we need to get function name and relative address for more processing and to get logging informations
                || bDebugMonitoringFile
            )
            )
        {
            pTlsData->RelativeAddressFromCallingModule-=CCallInstructionSizeFromReturnAddress::GetCallInstructionSizeFromReturnAddress((PBYTE)ReturnAddr);
        }
        else
        {
            pTlsData->bMatchFakingFilters=FALSE; // to dangerous for faking to put it to TRUE
            pTlsData->bMatchMonitoringFilters=TRUE;
            *pTlsData->szCallingModuleName=0;
            pTlsData->RelativeAddressFromCallingModule=0;
        }

	    pTlsData->bMatchMonitoringFilters=bCallingModuleMatchDefinedFilters || pAPIInfo->DontCheckModulesFilters || bDebugMonitoringFile;
        pTlsData->bMatchFakingFilters= bCallingModuleMatchDefinedFilters || pAPIInfo->DontCheckModulesFiltersForFaking;
    }

    //////////////////////////////////////////////////////////////////////
    // call pre api call callbacks
    //////////////////////////////////////////////////////////////////////
    if (pAPIInfo->PreApiCallChain)
    {
        BOOL PrePostApiHookCallContinueChain;
        PRE_POST_API_CALL_HOOK_INFOS HookInfos;
        PRE_POST_API_CALL_CHAIN_DATA* pCallChainData;
        DWORD CurrentEsp;
        HookInfos.Rbp=(PBYTE)OriginalRegisters.ebp;
        HookInfos.OverridingModulesFiltersSuccessfullyChecked=pTlsData->bMatchFakingFilters;
        HookInfos.ReturnAddress=ReturnAddr;
        HookInfos.CallingModuleHandle=pTlsData->CallingModuleHandle;

        __asm
        {
            mov [CurrentEsp],esp
        }

        pAPIInfo->PreApiCallChain->Lock(TRUE);
        for (pItem=pAPIInfo->PreApiCallChain->Head;pItem;pItem=pItem->NextItem)
        {
            pCallChainData=(PRE_POST_API_CALL_CHAIN_DATA*)pItem->ItemData;
            // call callback
            if (IsBadCodePtr((FARPROC)pCallChainData->CallBack))
            {
#ifdef _DEBUG
                if (IsDebuggerPresent())// avoid to crash application if no debugger
                    DebugBreak();
#endif
                continue;
            }

            // call user callback
            PrePostApiHookCallContinueChain=((pfPreApiCallCallBack)pCallChainData->CallBack)((PBYTE)pParametersPointer,&OriginalRegisters,&HookInfos,pCallChainData->UserParam);

            __asm
            {
                // compare esp to an ebp based value
                cmp esp,[CurrentEsp]
                // if esp is ok go to PreApiCallChainStackSuccessFullyChecked
                je PreApiCallChainStackSuccessFullyChecked
                // else
                // restore esp
                mov esp,[CurrentEsp]
            }
            ReportBadHookChainBadCallingConvention(pAPIInfo,pCallChainData->CallBack,TRUE);
#ifdef _DEBUG
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
#endif
PreApiCallChainStackSuccessFullyChecked:
            if (!PrePostApiHookCallContinueChain)
                break;
        }
        pAPIInfo->PreApiCallChain->Unlock();
    }
    //////////////////////////////////////////////////////////////////////
    // End of call pre api call callbacks
    //////////////////////////////////////////////////////////////////////

    if(bLogCall) // try to speed up a little
    {
        memcpy(&pTlsData->LogOriginalRegisters,&OriginalRegisters,sizeof(REGISTERS));

        //////////////////////////////////////////////////////////////////////
        // log input parameters if required
        //////////////////////////////////////////////////////////////////////
        bLogInputParametersWithoutReturn=((pAPIInfo->ParamDirectionType==PARAM_DIR_IN_NO_RETURN)
                                        ||(pAPIInfo->ParamDirectionType==PARAM_DIR_INOUT));
        pTlsData->bLogInputParameters=(pAPIInfo->ParamDirectionType==PARAM_DIR_IN);


        // if we log parameter in
        if ( (pAPIInfo->pMonitoringFileInfos)
             &&( ((pTlsData->bLogInputParameters||bLogInputParametersWithoutReturn)
                 && (!(bFiltersApplyToMonitoring && !pTlsData->bMatchMonitoringFilters))
                 )
                ||(pAPIInfo->LogBreakWay.BreakBeforeCall && pTlsData->bMatchMonitoringFilters)
                || bDebugMonitoringFile
                )
            )
        {
            
            // store number of parameter before call, because it can be recompute according to calling convention
            // so we need to save number of parameters stored in 
            pTlsData->LogInfoIn.NbParameters=pAPIInfo->MonitoringParamCount;
            ParseAPIParameters(pAPIInfo, &OriginalRegisters, &pTlsData->LogInfoIn,(PBYTE)pParametersPointer);

            // call break dialog if needed
            if (pAPIInfo->LogBreakWay.BreakBeforeCall && pTlsData->bMatchMonitoringFilters)
            {
                // check parameters break filters (as func is quite time consuming, check only if all other conditions are ok)
                if (CheckParamBreakFilters(pAPIInfo, &pTlsData->LogInfoIn))
                {
                    double DoubleResult;
                    // show BreakUserInterface Dialog
                    Break(pAPIInfo,&pTlsData->LogInfoIn,(PBYTE)pParametersPointer,&OriginalRegisters,&DoubleResult,ReturnAddr,(PBYTE)OriginalRegisters.ebp,TRUE);

                    if (pAPIInfo->LogBreakWay.BreakLogInputAfter)
                    {
                        memcpy(&pTlsData->LogOriginalRegisters,&OriginalRegisters,sizeof(REGISTERS));
                        ParseAPIParameters(pAPIInfo, &OriginalRegisters, &pTlsData->LogInfoIn,(PBYTE)pParametersPointer);
                    }
                }
            }

            if (bLogInputParametersWithoutReturn || bDebugMonitoringFile)
            {
                // check parameter log filters
                if (CheckParamLogFilters(pAPIInfo, &pTlsData->LogInfoIn))
                    CLogAPI::AddLogEntry(pAPIInfo, &pTlsData->LogInfoIn, 0,0.0,FALSE,
                                        ReturnAddr,
                                        PARAM_DIR_TYPE_IN_NO_RETURN,
                                        pTlsData->szCallingModuleName,pTlsData->RelativeAddressFromCallingModule,
                                        &pTlsData->LogOriginalRegisters,&pTlsData->LogOriginalRegisters,
                                        (PBYTE)pTlsData->LogOriginalRegisters.ebp,GetCurrentThreadId(),pLinkListTlsData);
            }
        }
    }

    // save stack parameters only now (after before call Break may change stack content, to allow changes to appear in output logging and breaking)
    if (pAPIInfo->PostApiCallChain                         // if post api call chain as been defined
        || (pAPIInfo->ParamDirectionType==PARAM_DIR_INOUT) // out parameter logging needed
        || (pAPIInfo->ParamDirectionType==PARAM_DIR_OUT)   // out parameter logging needed
        || bDebugMonitoringFile
        || (pAPIInfo->LogBreakWay.BreakAfterCall)          // after call break dialog may be required
        || (pAPIInfo->LogBreakWay.BreakAfterCallIfNullResult)
        || (pAPIInfo->LogBreakWay.BreakAfterCallIfNotNullResult)
        || (pAPIInfo->LogBreakWay.BreakOnFailure)
        || (pAPIInfo->LogBreakWay.BreakOnSuccess)
        )
    {

        // pAPIInfo->ParamCount gives a supposed size (size given by the config file)
        // in case of error in your config file you can crash your esp --> adding security size
        DWORD ESPSecuritySize=ESP_SECURITY_SIZE* REGISTER_BYTE_SIZE;

        // ok sometimes stack is not big enough to use full ESP_SECURITY_SIZE
        // so we have to adjust dwESPSecuritySize by checking available space
        while ((IsBadReadPtr(pParametersPointer,pAPIInfo->StackSize+ESPSecuritySize))&&ESPSecuritySize!=0)
            ESPSecuritySize/=2;

        pTlsData->pParametersPointer=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,pAPIInfo->StackSize+ESPSecuritySize);
        memcpy(pTlsData->pParametersPointer,pParametersPointer,pAPIInfo->StackSize+ESPSecuritySize);
    }

    //////////////////////////////////////////////////////////////////////
    // call API or api override
    //////////////////////////////////////////////////////////////////////
    bFakeCurrentCall=(bFaking  // if faking is enabled
                                       && ( 
                                                ( !(bFiltersApplyToFaking && !pTlsData->bMatchFakingFilters) ) // if filters applies to faking and filters match
                                                 || (pAPIInfo->DontCheckModulesFiltersForFaking) // the function doesn't use modules filters
                                                )
                                       );
    pTlsData->bFakingApplied = bFakeCurrentCall;

    // if an api replacement has been configured execute it instead of real api
    if (pAPIInfo->FakeAPIAddress && bFakeCurrentCall)
        FunctionAddress=pAPIInfo->FakeAPIAddress;
    else if (pAPIInfo->FirstBytesCanExecuteAnywhereSize)
        FunctionAddress=(FARPROC)&pAPIInfo->OpcodesExecutedAtAnotherPlace;
    else if (pAPIInfo->bFunctionPointer)
        // original opcode contains the real function address
        FunctionAddress=(FARPROC)(*((PBYTE*)pAPIInfo->Opcodes));
    else
        FunctionAddress=pAPIInfo->APIAddress;

    // if blocking call
    // blocking call is necessary only if not bFirstBytesCanExecuteAnyWhere
    if (pAPIInfo->BlockingCall 
        && (!pAPIInfo->FirstBytesCanExecuteAnywhereSize)
        && (!pAPIInfo->bFunctionPointer)
        )
    {
        pTlsData->BlockingCallArg.evtThreadStop=CreateEvent(NULL,FALSE,FALSE,NULL);
        pTlsData->BlockingCallArg.pApiInfo=pAPIInfo;
        // passing pTlsData->BlockingCallArg as param, we have to assume that thread returns before end of APIHandler
        pTlsData->BlockingCallThread=CreateThread(NULL,0,BlockingCallThreadProc,&pTlsData->BlockingCallArg,0,NULL);
    }

    // critical part no other thread should have restore hook before the call is made
    // --> check use count before restoring hook
    if (pAPIInfo->HookType==HOOK_TYPE_NET)
    {
        // if enter leave are spied by profiler
        if (pNetManager->AreEnterLeaveSpied())
        {
            // 1) let leave call back set return address to avoid security troubles
            // 2) avoid to quite lock computer with highest thread priority
            if (   (!pAPIInfo->FirstBytesCanExecuteAnywhereSize)
                && (!pAPIInfo->bFunctionPointer)
                )
            {
                SetThreadPriority(ThreadHandle,ThreadPriority);
            }
        }
        else
        {
            // change return address to our handler address
            *((PBYTE*)OriginalEspWithReturnAddress)=(PBYTE)APIAfterCallHandler;
        }
    }
    else // standard monitoring
    {
        // change return address to our handler address
        *((PBYTE*)OriginalEspWithReturnAddress)=(PBYTE)APIAfterCallHandler;
    }

    // pTlsData->TickCountBeforeCall=GetTickCount(); // for those QueryPerformanceCounter dosen't work
    QueryPerformanceCounter(&pTlsData->TickCountBeforeCall);

    RestoreFloatingRegistersAndUnmaskFloatingExceptions(FPUContent);

    // restore last error
    SetLastErrorApiSubstitution(LastErrorCode); // pure asm

    if (pAPIInfo->FirstBytesCanExecuteAnywhereSize
        || pAPIInfo->bFunctionPointer
        )
    {
        pApiOverrideTlsData->InsidePrePost = FALSE;
    }

    __asm 
    {
        ////////////////////////////////
        // no need to save local registers as function will exist
        ////////////////////////////////

        ////////////////////////////////
        // restore registers like they were at the begin of hook
        ////////////////////////////////
        mov esp, [OriginalEspWithReturnAddress]
        mov eax, [OriginalRegisters.eax]
        mov ebx, [OriginalRegisters.ebx]
        mov ecx, [OriginalRegisters.ecx]
        mov edx, [OriginalRegisters.edx]
        mov esi, [OriginalRegisters.esi]
        mov edi, [OriginalRegisters.edi]
        push     [OriginalRegisters.efl]
        popfd

        ////////////////////////////////
        // call real or faked API
        ////////////////////////////////

        // as we want to restore ebp instead of doing "call FunctionAddress"
        // we have to do
        push FunctionAddress // push FunctionAddress address of function we want to call
        mov ebp,[OriginalRegisters.ebp]  // FROM NOW WE CAN ACCESS LOCAL VARS
        ret                  // as FunctionAddress is on stack it means call FunctionAddress
    }
}

//-----------------------------------------------------------------------------
// Name: APIAfterCallHandler
// Object:  function called after api call in standard cases
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) void APIAfterCallHandler()
{
    API_HANDLER_TLS_DATA* pTlsData;
    API_INFO* pAPIInfo;
    PBYTE OriginalReturnAddress;
    WORD wFPUControlRegister;
    WORD wFPUControlRegisterWithoutExceptions;
    DWORD LastError;
    DWORD Cnt;
    double DoubleResult;
    double OriginalDoubleResult;
    REGISTERS AfterCallRegisters;
    LOG_INFOS LogInfoOut;
    REGISTERS LogAfterCallRegisters;
    BOOL bFunctionFail;
    BOOL bLogOuputParameters;
    PBYTE ReturnValue;
    BOOL bBreak;
    double DoubleLogResult;
    LARGE_INTEGER TickCountAfterCall;
    BOOL bEmptyFloatStack;
    BOOL bFloatingReturnHasChanged;
    BOOL bLogCall;
    BYTE FPUContent[FPU_BUFFER_SIZE];
    PBYTE FS0Value;
    CApiOverrideTlsData* pApiOverrideTlsData;

    //////////////
    // prolog
    //////////////
    __asm
    {
        push eax // only to reserve space on stack for return address
        // push flags
        pushfd

        // push registers
        push        eax  
        push        ebx  
        push        ecx  
        push        edx  
        push        esi  
        push        edi  

        // save frame
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE

        Mov EAX, [EBP]
        Mov [AfterCallRegisters.ebp], EAX

        Mov EAX, [EBP + 4]
        Mov [AfterCallRegisters.edi], EAX

        Mov EAX, [EBP + 8]
        Mov [AfterCallRegisters.esi], EAX

        Mov EAX, [EBP + 12]
        Mov [AfterCallRegisters.edx], EAX

        Mov EAX, [EBP + 16]
        Mov [AfterCallRegisters.ecx], EAX

        Mov EAX, [EBP + 20]
        Mov [AfterCallRegisters.ebx], EAX

        Mov EAX, [EBP + 24]
        Mov [AfterCallRegisters.eax], EAX

        Mov EAX, [EBP + 28]
        Mov [AfterCallRegisters.efl], EAX

        push gs
        pop [AfterCallRegisters.gs]
        push fs
        pop [AfterCallRegisters.fs]
        push es
        pop [AfterCallRegisters.es]

        lea eax, [ebp+36]
        Mov [AfterCallRegisters.esp], EAX

        mov EAX, FS:[0]
        mov [FS0Value], EAX
    }

    SaveFloatingRegistersAndMaskFloatingExceptions(FPUContent);// asm only
    bEmptyFloatStack = IsSt0Empty(); // asm call only
    if (!bEmptyFloatStack)
    {
        __asm
        {
            // store st(0) value in OriginalDoubleResult and DoubleResult
            fst [OriginalDoubleResult]
            fst [DoubleResult]
            fclex // clear exception if any
        }
    }

    LastError=GetLastErrorApiSubstitution(); // must be called before first called API

    /////////////////
    // Function body
    /////////////////
    pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);// pure asm function no api call
#ifdef _DEBUG
    if (!pApiOverrideTlsData)// process will surely crash
    {
        __asm int 3 // better for stack retrieval
        // DebugBreak();
    }
#endif

    // 5.5.1 : case of uncatched exception, with program flow restored by debugger
    // uncatched exception can be found because all our items of pApiOverrideTlsData->pLinkListTlsData
    // have been removed (as all exceptions handler were called to try to find one matching
    if (!pApiOverrideTlsData->pLinkListTlsData->Tail)
    {
        // copy content of pLinkListTlsDataPotentialUncatchException to pLinkListTlsData
        CLinkListItem* pItem;
        for (pItem=pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->Head;
             pItem;
             pItem=pItem->NextItem)
        {
            pApiOverrideTlsData->pLinkListTlsData->AddItem(pItem->ItemData);
        }
    }
    pAPIInfo = ((API_HANDLER_TLS_DATA*)pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData)->pAPIInfo;

    // 5.5.1 : clear pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException because
    // there 2 cases
    // - an uncatched exception occurred and previous code has put content inside pApiOverrideTlsData->pLinkListTlsData
    // - no exception occurred or a catched exception occurred, and in this case pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException content is useless
    pApiOverrideTlsData->pLinkListTlsDataPotentialUncatchException->RemoveAllItems();

#ifdef _DEBUG
    if (pApiOverrideTlsData->InsidePrePost)
    {
        if (pAPIInfo->FirstBytesCanExecuteAnywhereSize
            || pAPIInfo->bFunctionPointer
            )
        {
            __asm int 3 // better for stack retrieval
            // DebugBreak();
        }
    }
#endif
    
    if (pAPIInfo->FirstBytesCanExecuteAnywhereSize
        || pAPIInfo->bFunctionPointer
        )
    {
        pApiOverrideTlsData->InsidePrePost = TRUE;
    }

    // check for exception inside hooked functions, which exception handlers can't be hooked
    CheckForExceptionInsideNotHookedExceptionsHandlers(FALSE,FS0Value,(PBYTE)AfterCallRegisters.esp);

    // clear potentially unhooked items
    while (pApiOverrideTlsData->pLinkListTlsData->Tail)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)(pApiOverrideTlsData->pLinkListTlsData->Tail->ItemData);
        if (!pTlsData->UnhookedDuringCall)
            break;
        pApiOverrideTlsData->pLinkListTlsData->RemoveItem(pApiOverrideTlsData->pLinkListTlsData->Tail);
    }
    // put execution completed flag has soon as possible
    pTlsData->ExecutionCompleted=TRUE;

    ReturnValue=(PBYTE)AfterCallRegisters.eax;
    memset((PVOID)&LogInfoOut,0,sizeof(LOG_INFOS));


    // remove exception handler hook
    UnhookExceptionHandler(pTlsData->OriginalExceptionHandler,pTlsData->OriginalExceptionHandlerAddress);

    // get original return address
    OriginalReturnAddress=pTlsData->OriginalReturnAddress;

    // get api info struct pointer
    pAPIInfo=pTlsData->pAPIInfo;

    bLogCall=(bMonitoring||bDebugMonitoringFile) && pAPIInfo->pMonitoringFileInfos;
    if(bLogCall)
    {
        if (bEmptyFloatStack)
            DoubleResult=dFloatingNotSet;

        pTlsData->LogInfoIn.dwLastErrorCode=LastError;
        LogInfoOut.dwLastErrorCode=LastError;

        // get tick count after call, only few us have been taken by register saving
        QueryPerformanceCounter(&TickCountAfterCall);
        // compute number of us (PerformanceFrequency is in count per second)
        TickCountAfterCall.QuadPart=((TickCountAfterCall.QuadPart-pTlsData->TickCountBeforeCall.QuadPart)*1000*1000)/PerformanceFrequency.QuadPart;
        pTlsData->LogInfoIn.dwCallDuration = TickCountAfterCall.LowPart;
        LogInfoOut.dwCallDuration=pTlsData->LogInfoIn.dwCallDuration;
        LogInfoOut.CallTime=pTlsData->LogInfoIn.CallTime;
    }

    // if Stack Changed By Callee
    if (pTlsData->OriginalRegisters.esp!=AfterCallRegisters.esp)
    {
        DWORD StackChangedSize=AfterCallRegisters.esp-pTlsData->OriginalRegisters.esp;
        CheckParameterNumbers(pAPIInfo,StackChangedSize);
    }

    // allow break only if we are currently monitoring
    if (bLogCall)// try to speed up a little if we are not monitoring
    {
        // check if we have to log output
        bLogOuputParameters=(pAPIInfo->ParamDirectionType==PARAM_DIR_OUT)||(pAPIInfo->ParamDirectionType==PARAM_DIR_INOUT);

        // update bLogOuputParameters and pTlsData->bLogInputParameters
        // depending result
        if (pAPIInfo->LogBreakWay.LogIfNullResult)
        {
            pTlsData->bLogInputParameters=pTlsData->bLogInputParameters&&(ReturnValue==0);
            bLogOuputParameters=bLogOuputParameters&&(ReturnValue==0);
        }
        if (pAPIInfo->LogBreakWay.LogIfNotNullResult)
        {
            pTlsData->bLogInputParameters=pTlsData->bLogInputParameters&&(ReturnValue!=0);
            bLogOuputParameters=bLogOuputParameters&&(ReturnValue!=0);
        }

        bBreak=pAPIInfo->LogBreakWay.BreakAfterCall
            ||(pAPIInfo->LogBreakWay.BreakAfterCallIfNullResult && (ReturnValue==0))
            ||(pAPIInfo->LogBreakWay.BreakAfterCallIfNotNullResult && (ReturnValue!=0));

        // check function failure
        bFunctionFail=DoFunctionFail(pAPIInfo,ReturnValue,DoubleResult,LastError);

        // adjust bLog and bBreak depending function failure
        if (pAPIInfo->LogBreakWay.LogOnFailure)
        {
            pTlsData->bLogInputParameters=pTlsData->bLogInputParameters && bFunctionFail;
            bLogOuputParameters=bLogOuputParameters && bFunctionFail;
        }
        if (pAPIInfo->LogBreakWay.LogOnSuccess)
        {
            pTlsData->bLogInputParameters=pTlsData->bLogInputParameters && !bFunctionFail;
            bLogOuputParameters=bLogOuputParameters && !bFunctionFail;
        }
        if (pAPIInfo->LogBreakWay.BreakOnFailure)
            bBreak=bFunctionFail;
        if (pAPIInfo->LogBreakWay.BreakOnSuccess)
            bBreak=!bFunctionFail;

        //////////////////////////////////////////////////////////////////////
        // log output parameters if required (do the same as input parameters)
        //////////////////////////////////////////////////////////////////////
        memcpy(&LogAfterCallRegisters,&AfterCallRegisters,sizeof(REGISTERS));
        DoubleLogResult=DoubleResult;

        if ( (pAPIInfo->pMonitoringFileInfos)
              && (
                    bLogOuputParameters && (!(bFiltersApplyToMonitoring && !pTlsData->bMatchMonitoringFilters))// if filters are (not activated) or (activated and match)
                    || (bBreak && pTlsData->bMatchMonitoringFilters)
                    || bDebugMonitoringFile
                 )
            )
        {
            PBYTE pParametersPointer;
            // if a copy of stack has been done and not removed due to exception
            if (pTlsData->pParametersPointer)
                pParametersPointer=pTlsData->pParametersPointer;
            else
                // during exception report stack should not be destroyed, so parameters should be still available
                pParametersPointer=(PBYTE)pTlsData->OriginalRegisters.esp;

            // we get original parameters and parse them again because only pointed content as been changed,
            ParseAPIParameters(pAPIInfo, &pTlsData->OriginalRegisters, &LogInfoOut,pParametersPointer);// parse parameters (calling this way, params are used without being consumed)
            if (pTlsData->bMatchMonitoringFilters && bBreak)
            {
                // check parameters break filters (as func is quite time consuming, check only if all other conditions are ok)
                if (CheckParamBreakFilters(pAPIInfo, &LogInfoOut))
                {
                    Break(pAPIInfo,&LogInfoOut,pParametersPointer,&AfterCallRegisters,&DoubleResult,pTlsData->OriginalReturnAddress,(PBYTE)pTlsData->OriginalRegisters.ebp,FALSE);
                    if (pAPIInfo->LogBreakWay.BreakLogOutputAfter)
                    {
                        // param are still on stack
                        // re parse params in case they were modified during break
                        ParseAPIParameters(pAPIInfo, &pTlsData->OriginalRegisters, &LogInfoOut,pParametersPointer);

                        memcpy(&LogAfterCallRegisters,&AfterCallRegisters,sizeof(REGISTERS));
                        DoubleLogResult=DoubleResult;

                        // update returned value if eax register as been changed
                        ReturnValue=(PBYTE)AfterCallRegisters.eax;
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////
        // send logging information to monitoring application
        //////////////////////////////////////////////////////////////////////
        if ((pAPIInfo->pMonitoringFileInfos)// if a monitoring informations are defined
            && (!(bFiltersApplyToMonitoring && !pTlsData->bMatchMonitoringFilters)|| bDebugMonitoringFile) // if filters are (not activated) or (activated and match)
            )
        {
            if (pTlsData->bLogInputParameters // if we log input parameters
                && (!bDebugMonitoringFile) // if bDebugMonitoringFile, we already have logged the InNoRet
                )
            {
                // check parameters log filters (as func is quite time consuming, check only if all other conditions are ok)
                if (CheckParamLogFilters(pAPIInfo, &pTlsData->LogInfoIn))
                    CLogAPI::AddLogEntry(pAPIInfo, &pTlsData->LogInfoIn, ReturnValue,DoubleLogResult,bFunctionFail,
                                        OriginalReturnAddress,
                                        PARAM_DIR_TYPE_IN, pTlsData->szCallingModuleName,pTlsData->RelativeAddressFromCallingModule,
                                        &pTlsData->LogOriginalRegisters,&LogAfterCallRegisters,
                                        (PBYTE)pTlsData->OriginalRegisters.ebp,GetCurrentThreadId(),pApiOverrideTlsData->pLinkListTlsData);
            }
            else if (bLogOuputParameters // if we log output parameters
                     || bDebugMonitoringFile // or we are in debug mode
                    )
            {
                LogInfoOut.NbParameters=pAPIInfo->MonitoringParamCount;
                // check parameters log filters (as func is quite time consuming, check only if all other conditions are ok)
                if (bDebugMonitoringFile || CheckParamLogFilters(pAPIInfo, &pTlsData->LogInfoIn))
                    CLogAPI::AddLogEntry(pAPIInfo, &LogInfoOut, ReturnValue,DoubleLogResult,bFunctionFail,
                                        OriginalReturnAddress,
                                        PARAM_DIR_TYPE_OUT,pTlsData->szCallingModuleName,pTlsData->RelativeAddressFromCallingModule,
                                        &pTlsData->LogOriginalRegisters,&LogAfterCallRegisters,
                                        (PBYTE)pTlsData->OriginalRegisters.ebp,GetCurrentThreadId(),pApiOverrideTlsData->pLinkListTlsData);
            }
        }
        // free allocated memory by ParseAPIParameters
        //   MAKE SURE THE FOLLOWING CODE IS USED ONLY IF YOU HAVE INTIALIZE LOGINFOIN AND LOGINFOOUT WITH
        //           memset((PVOID)&LogInfoIn,0,sizeof(LOG_INFOS));
        //           memset((PVOID)&LogInfoOut,0,sizeof(LOG_INFOS));
        if(pTlsData->LogInfoIn.ParmeterParsed)
        {
            for (Cnt = 0; Cnt < pAPIInfo->MonitoringParamCount; Cnt++)
            {
                // if memory has been allocated
                if (pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue)
                {
                    HeapFree(ApiOverrideLogHeap, 0,pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue);
                    // reset pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue in case of exception before exception handler removal
                    pTlsData->LogInfoIn.ParamLogList[Cnt].pbValue=NULL;
                }
            }
            pTlsData->LogInfoIn.ParmeterParsed=FALSE;
        }
        if(LogInfoOut.ParmeterParsed)
        {
            for (Cnt = 0; Cnt < pAPIInfo->MonitoringParamCount; Cnt++)
            {
                // if memory has been allocated
                if (LogInfoOut.ParamLogList[Cnt].pbValue)
                    HeapFree(ApiOverrideLogHeap, 0,LogInfoOut.ParamLogList[Cnt].pbValue);
            }
            LogInfoOut.ParmeterParsed=FALSE;
        }
    }

    //////////////////////////////////////////////////////////////////////
    // call post api call callbacks
    //////////////////////////////////////////////////////////////////////
    if (pAPIInfo->PostApiCallChain)
    {
        DWORD CurrentEsp;
        CLinkListItem* pItem;
        PRE_POST_API_CALL_CHAIN_DATA* pCallChainData;
        PRE_POST_API_CALL_HOOK_INFOS HookInfos;
        BOOL PrePostApiHookCallContinueChain;
        HookInfos.Rbp=(PBYTE)AfterCallRegisters.ebp;
        HookInfos.OverridingModulesFiltersSuccessfullyChecked=pTlsData->bMatchFakingFilters;
        HookInfos.ReturnAddress=OriginalReturnAddress;
        HookInfos.CallingModuleHandle=pTlsData->CallingModuleHandle;

        __asm
        {
            mov [CurrentEsp],esp
        }

        PBYTE pParametersPointer;
        // if a copy of stack has been done and not removed due to exception
        if (pTlsData->pParametersPointer)
            pParametersPointer=pTlsData->pParametersPointer;
        else
            // during exception report stack should not be destroyed, so parameters should be still available
            pParametersPointer=(PBYTE)pTlsData->OriginalRegisters.esp;

        pAPIInfo->PostApiCallChain->Lock(TRUE);
        for (pItem=pAPIInfo->PostApiCallChain->Head;pItem;pItem=pItem->NextItem)
        {
            pCallChainData=(PRE_POST_API_CALL_CHAIN_DATA*)pItem->ItemData;
            // call callback
            if (IsBadCodePtr((FARPROC)pCallChainData->CallBack))
            {
#ifdef _DEBUG
                if (IsDebuggerPresent())// avoid to crash application if no debugger
                    DebugBreak();
#endif
                continue;
            }

            // gives pParametersPointer to allow user to see param changes
            PrePostApiHookCallContinueChain=((pfPostApiCallCallBack)pCallChainData->CallBack)((PBYTE)pParametersPointer,&AfterCallRegisters,&HookInfos,pCallChainData->UserParam);

            __asm
            {
                // compare esp to an ebp based value
                cmp esp,[CurrentEsp]
                // if esp is ok go to PostApiCallChainStackSuccessFullyChecked
                je PostApiCallChainStackSuccessFullyChecked
                // else
                // restore esp
                mov esp,[CurrentEsp]
            }
            ReportBadHookChainBadCallingConvention(pAPIInfo,pCallChainData->CallBack,FALSE);
#ifdef _DEBUG
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
#endif
PostApiCallChainStackSuccessFullyChecked:
            if (!PrePostApiHookCallContinueChain)
                break;// break if callback query it
        }
        pAPIInfo->PostApiCallChain->Unlock();
    }
    //////////////////////////////////////////////////////////////////////
    // end of call post api call callbacks
    //////////////////////////////////////////////////////////////////////

    // restore hook, free stack duplication and pop pLinkListTlsData informations (remove hook infos from list)
    RestoreHook(pApiOverrideTlsData->pLinkListTlsData,pTlsData,pAPIInfo,LastError);

    // AVOID TO CALL FUNC FROM HERE TO AVOID INFINITE LOOP if it's hooked

    bFloatingReturnHasChanged = FALSE;
    if (!bEmptyFloatStack)
        bFloatingReturnHasChanged = (DoubleResult != OriginalDoubleResult);

    RestoreFloatingRegistersAndUnmaskFloatingExceptions(FPUContent);

    if (bFloatingReturnHasChanged)
    {
        /////////////////////////////////////
        // we need to change st(0)
        /////////////////////////////////////

        // 1) mask floating point exception
        __asm
        {
            // get current floating exception flags
            fstcw [wFPUControlRegister]
        }

        // add floating exception masks --> no floating exception will occur
        wFPUControlRegisterWithoutExceptions = wFPUControlRegister | 0x7F; 

        __asm
        {
            // apply new floating exception masks (no floating exception will occur)
            fldcw [wFPUControlRegisterWithoutExceptions]
        

        // 2) pop st(0) in OriginalDoubleResult (trash memory) and push DoubleResult (new floating return value)
            fstp qword ptr [OriginalDoubleResult] // use OriginalDoubleResult as trash memory (only to dequeue stack)
            fld qword ptr [DoubleResult]

        // 3) reset floating point exception
            fclex

        // 4) unmask floating point exception
            fldcw [wFPUControlRegister]
        }
    }

    if (pAPIInfo->FirstBytesCanExecuteAnywhereSize
        || pAPIInfo->bFunctionPointer
        )
    {
        pApiOverrideTlsData->InsidePrePost = FALSE;
    }

    //////////////
    // epilog
    //////////////
    __asm
    {
        // return address registration
        mov eax,[OriginalReturnAddress]
        mov [ebp+32],eax // +4 for ret addr +7*4 for pushfd and other register pushing

        // restore frame
        mov esp, ebp
        pop ebp

        // restore registers
        pop edi
        pop esi 
        pop edx 
        pop ecx
        pop ebx 
        pop eax  
        popfd

        // go back to original return address
        ret
    }
}

//-----------------------------------------------------------------------------
// Name: APINotMonitoredTlsAfterCallHandler
// Object:  function called after api call in specific cases :
//          for api called when tls linked list is in ListCriticalPart
//          for api hooked with (!pAPIInfo->FirstBytesCanExecuteAnywhereSize) && (!pAPIInfo->bFunctionPointer) and 
//          if tls doesn't exist (CApiOverrideTlsData object not created in ApiOverrideTlsIndex)
//          it's goal : just restore hook and return to original address
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) void APINotMonitoredGlobalAfterCallHandler()
{
    PVOID pSource;
    PVOID pDest;
    DWORD dwSize;
    API_INFO* pAPIInfo;
    PBYTE OriginalReturnAddress;
    SIMPLE_HOOK_CRITICAL_CALL_INFO* pSimpleHookCriticalCallInfo;

    __asm
    {
        push eax // only to reserve space on stack for return address
        // push flags
        pushfd

        // push registers
        push        eax  
        push        ebx  
        push        ecx  
        push        edx  
        push        esi  
        push        edi  

        // save frame
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE
    }

    // functions must be called before hook opcodes restoration

    // use static global array (remember tls data is still not created !!)
    pSimpleHookCriticalCallInfo = GetLastSimpleHookCriticalCallInfo();
    pAPIInfo = pSimpleHookCriticalCallInfo->pApiInfos;
    OriginalReturnAddress = pSimpleHookCriticalCallInfo->OriginalReturnAddress;
    FreeLastSimpleHookCriticalCallInfo();

    // flag to know we are restoring hook (in case of dump)
    pAPIInfo->bOriginalOpcodes=FALSE;

    //memcpy(pAPIInfo->APIAddress, pAPIInfo->pbHookCodes, pAPIInfo->OpcodeReplacementSize);
    pDest=pAPIInfo->APIAddress;
    pSource=pAPIInfo->pbHookCodes;
    dwSize=pAPIInfo->OpcodeReplacementSize;
    asm_memcpy(pDest, pSource, dwSize);

    // pAPIInfo->UseCount--;
    InterlockedDecrement(&pAPIInfo->UseCount);
    if (pAPIInfo->UseCount<0)// can occurred for uncatched exception with flow restored by debugger
        pAPIInfo->UseCount=0;

    //////////////
    // epilog
    //////////////
    __asm
    {
        // return address registration
        mov eax,[OriginalReturnAddress]
        mov [ebp+32],eax // +4 for ret addr +7*4 for pushfd and other register pushing

        // restore frame
        mov esp, ebp
        pop ebp

        // restore registers
        pop edi
        pop esi 
        pop edx 
        pop ecx
        pop ebx 
        pop eax  
        popfd

        // go back to original return address
        ret
    }
}

//-----------------------------------------------------------------------------
// Name: APINotMonitoredTlsAfterCallHandler
// Object:  function called after api call in specific cases :
//          for api called when tls linked list is in ListCriticalPart
//          for api hooked with (!pAPIInfo->FirstBytesCanExecuteAnywhereSize) && (!pAPIInfo->bFunctionPointer) and 
//          if tls data exists (CApiOverrideTlsData object created in ApiOverrideTlsIndex)
//          it's goal : just restore hook and return to original address
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) void APINotMonitoredTlsAfterCallHandler()
{
    PVOID pSource;
    PVOID pDest;
    DWORD dwSize;
    API_INFO* pAPIInfo;
    PBYTE OriginalReturnAddress;
    CApiOverrideTlsData* pApiOverrideTlsData;
    TLS_DATA_SIMPLE_HOOK_CRITICAL_CALL_INFO* pSimpleHookCriticalCallInfo;

    __asm
    {
        push eax // only to reserve space on stack for return address
        // push flags
        pushfd

        // push registers
        push        eax  
        push        ebx  
        push        ecx  
        push        edx  
        push        esi  
        push        edi  

        // save frame
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE
    }

    // get tls data
    pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
#ifdef _DEBUG
    if (pApiOverrideTlsData->SimpleHookCriticalCallInfos.CurrentIndex==0)
    {
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
    }
#endif

    // use tls data fixed size array instead of linked list
    pApiOverrideTlsData->SimpleHookCriticalCallInfos.CurrentIndex--;
    pSimpleHookCriticalCallInfo=&pApiOverrideTlsData->SimpleHookCriticalCallInfos.HookCriticalInfos[pApiOverrideTlsData->SimpleHookCriticalCallInfos.CurrentIndex];

    pAPIInfo = pSimpleHookCriticalCallInfo->pApiInfos;
    OriginalReturnAddress = pSimpleHookCriticalCallInfo->OriginalReturnAddress;

    // flag to know we are restoring hook (in case of dump)
    pAPIInfo->bOriginalOpcodes=FALSE;

    //memcpy(pAPIInfo->APIAddress, pAPIInfo->pbHookCodes, pAPIInfo->OpcodeReplacementSize);
    pDest=pAPIInfo->APIAddress;
    pSource=pAPIInfo->pbHookCodes;
    dwSize=pAPIInfo->OpcodeReplacementSize;
    asm_memcpy(pDest, pSource, dwSize);

    // pAPIInfo->UseCount--;
    InterlockedDecrement(&pAPIInfo->UseCount);
    if (pAPIInfo->UseCount<0)// can occurred for uncatched exception with flow restored by debugger
        pAPIInfo->UseCount=0;

    //////////////
    // epilog
    //////////////
    __asm
    {
        // return address registration
        mov eax,[OriginalReturnAddress]
        mov [ebp+32],eax // +4 for ret addr +7*4 for pushfd and other register pushing

        // restore frame
        mov esp, ebp
        pop ebp

        // restore registers
        pop edi
        pop esi 
        pop edx 
        pop ecx
        pop ebx 
        pop eax  
        popfd

        // go back to original return address
        ret
    }
}


//-----------------------------------------------------------------------------
// Name: GetLastSimpleHookCriticalCallInfo
// Object:  get last used hook infos 
//          [ used only when tls data CApiOverrideTlsData object is not created in ApiOverrideTlsIndex
//           and we are in a critical section so we must only call original api ]
// Parameters :
//     in : 
// Return : SIMPLE_HOOK_CRITICAL_CALL_INFO* : pointer to struct where informations are stored
//-----------------------------------------------------------------------------
SIMPLE_HOOK_CRITICAL_CALL_INFO* GetLastSimpleHookCriticalCallInfo()
{
    // CApiOverrideTlsData object is not created in ApiOverrideTlsIndex; but we have used another tls index (ApiOverrideSimpleHookCriticalCallTlsIndex) for storing array index 
    SIZE_T LastIndex = (SIZE_T)TlsGetValueApiSubstitution(ApiOverrideSimpleHookCriticalCallTlsIndex);
    return &SimpleHookCriticalCallInfo[LastIndex];
}

//-----------------------------------------------------------------------------
// Name: FreeLastSimpleHookCriticalCallInfo
// Object:  free last used hook infos and restore previous last hook infos
//          [ used only when tls data CApiOverrideTlsData object is not created in ApiOverrideTlsIndex
//           and we are in a critical section so we must only call original api ]
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void FreeLastSimpleHookCriticalCallInfo()
{
    // CApiOverrideTlsData object is not created in ApiOverrideTlsIndex; but we have used another tls index (ApiOverrideSimpleHookCriticalCallTlsIndex) for storing array index 
    SIZE_T LastIndex = (SIZE_T)TlsGetValueApiSubstitution(ApiOverrideSimpleHookCriticalCallTlsIndex);
    // restore previous last index for current thread
    TlsSetValueApiSubstitution(ApiOverrideSimpleHookCriticalCallTlsIndex,(LPVOID)SimpleHookCriticalCallInfo[LastIndex].PreviousIndex);
    // mark global array item as free
    SimpleHookCriticalCallInfo[LastIndex].bIsUsed = FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetFreeSimpleHookCriticalCallInfo
// Object:  get memory space for hook infos 
//          [ used only when tls data CApiOverrideTlsData object is not created in ApiOverrideTlsIndex
//           and we are in a critical section so we must only call original api ]
// Parameters :
//     in : 
// Return : SIMPLE_HOOK_CRITICAL_CALL_INFO* : pointer to struct where informations are stored
//-----------------------------------------------------------------------------
SIMPLE_HOOK_CRITICAL_CALL_INFO* GetFreeSimpleHookCriticalCallInfo()
{
    SIZE_T Cnt;
    DWORD LastError;

    // as we call an api we must save last error
    LastError = GetLastErrorApiSubstitution();

    // SimpleHookCriticalCallInfo is a global array shared among threads, 
    // so we must synchronize it
    WaitForSingleObject(hevtSimpleHookCriticalCallUnlockedEvent,500);// Dead lock can occur if WaitForSingleObject and SetEvent are monitored with first opcode replacment

    // 0 is for a not set index (Tls default value is 0) --> begin loop at 1
    for(Cnt = 1; Cnt<MAX_SIMPLE_HOOK_CRITICAL_CALL_INFO ; Cnt++)
    {
        // if array item is currently not used
        if (!SimpleHookCriticalCallInfo[Cnt].bIsUsed)
        {
            // mark it as used
            SimpleHookCriticalCallInfo[Cnt].bIsUsed = TRUE;
            // fill previous index with the current one
            SimpleHookCriticalCallInfo[Cnt].PreviousIndex = (DWORD)TlsGetValueApiSubstitution(ApiOverrideSimpleHookCriticalCallTlsIndex);
            // update current index for current thread
            TlsSetValueApiSubstitution(ApiOverrideSimpleHookCriticalCallTlsIndex,(LPVOID)Cnt);

            // release lock for array
            SetEvent(hevtSimpleHookCriticalCallUnlockedEvent);

            // restore last api error
            SetLastErrorApiSubstitution(LastError);

            // return pointer to newly used array item
            return &SimpleHookCriticalCallInfo[Cnt];
        }
    }
#ifdef _DEBUG
    if (IsDebuggerPresent())// avoid to crash application if no debugger
        DebugBreak();
    SetEvent(hevtSimpleHookCriticalCallUnlockedEvent);
    SetLastErrorApiSubstitution(LastError);
#endif
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: IsSt0Empty
// Object: check if floating stack st(0) member is empty
// Notice : as IsSt0Empty is called in critical parts IT MUST NOT CALL API
//          naked to avoid _RTC_CheckStackVars@8 and __RTC_CheckEsp which can call API
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) BOOL __cdecl IsSt0Empty()
{
    FPU_STATUS_REGISTER FPUStatusRegister;
    WORD wFPUStatusRegister;
    BOOL bResult;
    __asm
    {
        // standard frame saving
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE

        // get st(0) informations (informations are stored in status flag ConditionCode[0-4]
        fxam

        // get current floating status
        fstsw [wFPUStatusRegister]
    }

    FPUStatusRegister.wRegisters = wFPUStatusRegister;

    bResult = ( (FPUStatusRegister.Registers.ConditionCode0 == 1)
                && (FPUStatusRegister.Registers.ConditionCode3 == 1) );

    // return bResult;
    __asm
    {
        mov eax, [bResult]

        // restore frame
        mov esp, ebp
        pop ebp

        ret
    }
}


//-----------------------------------------------------------------------------
// Name: SaveFloatingRegistersAndMaskFloatingExceptions
// Object: save floating point unit registers and mask floating exceptions
// Notice : as SaveFloatingRegistersAndMaskFloatingExceptions is called in critical parts IT MUST NOT CALL API
//          naked to avoid _RTC_CheckStackVars@8 and __RTC_CheckEsp which can call API
// Parameters :
//     in : PBYTE FPUContent : buffer that will get FPU content 
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) void __cdecl SaveFloatingRegistersAndMaskFloatingExceptions(PBYTE FPUContent)
{
    WORD wFPUControlRegister;
    __asm
    {
        // standard frame saving
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE
        mov eax,[FPUContent] // [ebp+8] 

        fsave [eax]

        fwait

        // fsave change fpu registers so put them into their original state again for checking st0
        mov eax,[FPUContent] // [ebp+8] +4 for push ebp +4 for ret address

        frstor [eax]
        fwait

        // get current floating exception flags
        fstcw [wFPUControlRegister]
    }

    // add floating exception masks --> no floating exception will occur
    wFPUControlRegister |= 0x7F;

    __asm
    {
        // apply new floating exception masks (no floating exception will occur)
        fldcw [wFPUControlRegister]

        // restore frame
        mov esp, ebp
        pop ebp

        ret
    }
}

//-----------------------------------------------------------------------------
// Name: RestoreFloatingRegistersAndUnmaskFloatingExceptions
// Object: restore floating point unit registers and clear floating exceptions
// Notice : as RestoreFloatingRegistersAndUnmaskFloatingExceptions is called in critical parts IT MUST NOT CALL API
//          naked to avoid _RTC_CheckStackVars@8 and __RTC_CheckEsp which can call API
// Parameters :
//     in : PBYTE FPUContent : FPU content to restore
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) void __cdecl RestoreFloatingRegistersAndUnmaskFloatingExceptions(PBYTE FPUContent)
{
    __asm
    {
        // clear exception flags if any
        fclex

        // restore floating registers
        mov eax,[esp+4] // [FPUContent] naked function sucks sometimes for args if no ebp pushing like here with cdecl calling convention

        frstor [eax]
        fwait

        ret
    }
}

//-----------------------------------------------------------------------------
// Name: CheckParameterNumbers
// Object: check the parameter numbers
// Parameters :
//     in out : API_INFO* pAPIInfo : parameter number is changed in case of bad parameter number
//     in : DWORD StackChangedSize
// Return : 
//-----------------------------------------------------------------------------
void FORCEINLINE __fastcall CheckParameterNumbers(API_INFO* const pAPIInfo,DWORD const StackChangedSize)
{

    // according to calling convention
    switch(pAPIInfo->CallingConvention)
    {
    case CALLING_CONVENTION_CDECL:
        {
            TCHAR sz[3*MAX_PATH];
            _sntprintf(sz,3*MAX_PATH,
                _T("Stack corruption in cdecl function %s in module %s"),
                pAPIInfo->szAPIName,
                pAPIInfo->szModuleName);
            CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,sz);
        }
        break;
    case CALLING_CONVENTION_FASTCALL:
        if ( 
            (StackChangedSize>pAPIInfo->StackSize)
            || (StackChangedSize<pAPIInfo->StackSize-FASTCALL_NUMBER_OF_PARAM_POTENTIALY_PASSED_BY_REGISTER*sizeof(PBYTE))// in case params passed through registers where include during stack size computing
            )
        {
            DWORD NbParamPassedByRegister;
            DWORD Cnt;
            NbParamPassedByRegister=0;
            for (Cnt=0;
                (NbParamPassedByRegister<FASTCALL_NUMBER_OF_PARAM_POTENTIALY_PASSED_BY_REGISTER) && (Cnt<pAPIInfo->MonitoringParamCount);
                Cnt++)
            {
                if ((CSupportedParameters::IsParamPassedByRegisterWithFastcall(pAPIInfo->ParamList[Cnt].dwType & SIMPLE_TYPE_FLAG_MASK))
                    && (pAPIInfo->ParamList[Cnt].dwSizeOfData<=sizeof(PBYTE)))
                    NbParamPassedByRegister++;
            }

            // bogus config file
            // signal it
            BadParameterNumber(pAPIInfo,pAPIInfo->StackSize,StackChangedSize+NbParamPassedByRegister*sizeof(PBYTE));
            // update dwParamSize
            pAPIInfo->StackSize=StackChangedSize;
        }

        break;
    case CALLING_CONVENTION_THISCALL: // cdecl for vararg
        // check given param size
        if (   (StackChangedSize!=pAPIInfo->StackSize)
            && (StackChangedSize!=pAPIInfo->StackSize-sizeof(PBYTE)) // in case param passed through registers where include during stack size computing
            )
        {
            // bogus config file
            // signal it
            BadParameterNumber(pAPIInfo,pAPIInfo->StackSize,StackChangedSize+sizeof(PBYTE));
            // update dwParamSize
            pAPIInfo->StackSize=StackChangedSize;
        }
        break;
    case CALLING_CONVENTION_STDCALL:
    case CALLING_CONVENTION_STDCALL_OR_CDECL:
    default:
        ////////////////////////////////
        // get Calling convention
        ////////////////////////////////
        if (pAPIInfo->StackSize!=StackChangedSize)
        {
            // bogus config file
            // signal it
            BadParameterNumber(pAPIInfo,pAPIInfo->StackSize,StackChangedSize);
            // update dwParamSize
            pAPIInfo->StackSize=StackChangedSize;
        }
        break;
    }
}

// pTlsData and pAPIInfo are redundant informations, provided only to avoid to do cast and local allocation again
// pTlsData=pLinkListTlsData->Tail->ItemData
// pAPIInfo=pTlsData->pAPIInfo
// restore hook, free stack duplication and pop pLinkListTlsData informations (remove hook infos from list)
void FORCEINLINE __fastcall RestoreHook(CLinkListSingleThreaded* const pLinkListTlsData,API_HANDLER_TLS_DATA* const pTlsData,API_INFO* const pAPIInfo,DWORD const LastError)
{
    PVOID pSource;
    PVOID pDest;
    DWORD dwSize;
    //////////////////////////////////////////////////////////////////////
    // free potential stack duplication
    //////////////////////////////////////////////////////////////////////
    if (pTlsData->pParametersPointer)
    {
        HeapFree(ApiOverrideLogHeap, 0,pTlsData->pParametersPointer);
        pTlsData->pParametersPointer=0;
    }

    //////////////////////////////////////////////////////////////////////
    // Restore Hook
    //////////////////////////////////////////////////////////////////////
    // for blocking call, check if remote thread has restore original opcode
    // notice: if ((!pAPIInfo->FirstBytesCanExecuteAnyWhereSize) || (!pAPIInfo->bFunctionPointer))
    //              pTlsData->BlockingCallThread is null
    if (pAPIInfo->BlockingCall&&pTlsData->BlockingCallThread)
    {
        // query thread to stop
        SetEvent(pTlsData->BlockingCallArg.evtThreadStop);

        // wait for thread to finish before restoring hook 
        // it's avoid lots of thread creation
        // and troubles if WaitForSingleObject is hooked
        WaitForSingleObject(pTlsData->BlockingCallThread,INFINITE);

        // close thread handle
        CloseHandle(pTlsData->BlockingCallThread);
    }

    if ((!pAPIInfo->FirstBytesCanExecuteAnywhereSize)&&(!pAPIInfo->bFunctionPointer))
        // stop boosting thread before restoring protection to avoid SetThreadPriority hook troubles
        SetThreadPriority(GetCurrentThread(),pTlsData->OriginalThreadPriority);

    if (pAPIInfo->UseCount==1)
    {
        // set end of hook event.
        // after setting this event use only local vars. Not pAPIInfo ones
        SetEvent(pAPIInfo->evtEndOfHook);
    }

    // all functions called by this thread and used inside item removal to list musn't be hooked to avoid infinite loop
    DWORD OldValue = (DWORD)TlsGetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex);
    if (OldValue!=TRUE)
        TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)TRUE);

    // remove thread data
    pLinkListTlsData->RemoveItem(pLinkListTlsData->Tail);

    // functions can be hooked now
    if (OldValue!=TRUE)
        TlsSetValueApiSubstitution(ApiOverrideListCriticalPartTlsIndex,(LPVOID)FALSE);

    // RESTORE LAST ERROR (no func must be called after this)
    SetLastErrorApiSubstitution(LastError);

    // decrease only when pAPIInfo is no more useful
    // pAPIInfo->UseCount--;
    InterlockedDecrement(&pAPIInfo->UseCount);
    if (pAPIInfo->UseCount<0)// can occurred for uncatched exception with flow restored by debugger
        pAPIInfo->UseCount=0;

    // restore hook for the next API call
    // (restore only if not in use by another thread)
    if (pAPIInfo->UseCount==0)
    {
        // restore original bytes only if !bFirstBytesCanExecuteAnyWhere and !pAPIInfo->bFunctionPointer
        if (pAPIInfo->bOriginalOpcodes)
        {
            // restore only if we don't have query to Unhook the function
            if (!pAPIInfo->AskedToRemove)
            {
                // flag to know we are restoring hook (in case of dump)
                pAPIInfo->bOriginalOpcodes=FALSE;

                pDest=pAPIInfo->APIAddress;
                pSource=pAPIInfo->pbHookCodes;
                dwSize=pAPIInfo->OpcodeReplacementSize;
                asm_memcpy(pDest, pSource, dwSize);
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Name: BlockingCallThreadProc
// Object: Restore originals opcode before the end of function
// Parameters :
//     in  : LPVOID lpParameter : BLOCKING_CALL* struc
// Return : TRUE
//-----------------------------------------------------------------------------
DWORD WINAPI BlockingCallThreadProc(LPVOID lpParameter)
{
    PBLOCKING_CALL pBlockingCallArg=(PBLOCKING_CALL)lpParameter;

    // wait for hook end private event
    WaitForSingleObject(pBlockingCallArg->evtThreadStop,1);

    // restore hook even if in use by another thread
    // as Blocking call is dedicated for slow function call, being here
    // means other threads are in a waiting state and have executed first bytes

    // if remove hook has not been called
    if (!pBlockingCallArg->pApiInfo->AskedToRemove)
    {
        pBlockingCallArg->pApiInfo->bOriginalOpcodes=FALSE;
        // restore hook
        PVOID pDest=pBlockingCallArg->pApiInfo->APIAddress;
        PVOID pSource=pBlockingCallArg->pApiInfo->pbHookCodes;
        DWORD dwSize=pBlockingCallArg->pApiInfo->OpcodeReplacementSize;
        asm_memcpy(pDest, pSource, dwSize);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Name: RemoveProtection
// Object: Remove protection of pAPIInfo->APIAddress, and store old Protection flags in
//               pAPIInfo->dwOldProtectionFlags
// Parameters :
//     in out  : API_INFO *pAPIInfo
// Return : TRUE
//-----------------------------------------------------------------------------
BOOL RemoveProtection(API_INFO *pAPIInfo)
{
    // Get page protection of API
    // MEMORY_BASIC_INFORMATION mbi;
    // VirtualQuery(pAPIInfo->APIAddress, &mbi, sizeof(mbi));
    // DWORD dwProtectionFlags = mbi.Protect;
    // Remove page protection from API

    //dwProtectionFlags &= ~PAGE_EXECUTE;
    //dwProtectionFlags &= ~PAGE_EXECUTE_READ;
    //dwProtectionFlags &= ~PAGE_EXECUTE_WRITECOPY;
    //dwProtectionFlags &= ~PAGE_NOACCESS;
    //dwProtectionFlags &= ~PAGE_READONLY;
    //dwProtectionFlags &= ~PAGE_WRITECOPY;

    // only gives full rights
    // dwProtectionFlags = PAGE_EXECUTE_READWRITE;

    // as pAPIInfo->APIAddress is not system page rounded, do not use "if (!VirtualProtect(pAPIInfo->APIAddress, dwSystemPageSize,..."
    if (!VirtualProtect(pAPIInfo->APIAddress, pAPIInfo->OpcodeReplacementSize, PAGE_EXECUTE_READWRITE, &pAPIInfo->dwOldProtectionFlags))
    {
        TCHAR psz[3*MAX_PATH];
        _stprintf(psz,_T("Error removing memory protection at 0x%p for function %s in module %s. Hook won't be installed for this function"),
                pAPIInfo->APIAddress,pAPIInfo->szAPIName,pAPIInfo->szModuleName);
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,psz);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: QueryEmptyItemAPIInfo
// Object: Get an empty APIInfo Struct
// Parameters :
//     in  : 
//     out : 
// Return : NULL on error
//-----------------------------------------------------------------------------
CLinkListItem* __stdcall QueryEmptyItemAPIInfo()
{
    // we don't need to care about API_INFO struct contained in CLinkListItem
    // as AddItem returns a struct with content set to 0 (due to HEAP_ZERO_MEMORY flag of HeapAlloc)
    return pLinkListAPIInfos->AddItem();
}


//-----------------------------------------------------------------------------
// Name: GetAssociatedItemAPIInfo
// Object: Get List Item containing APIInfo Struct Associated to given API address
// Parameters :
//     in  : PBYTE pbAPI virtual address of func to hook 
//     out : BOOL* pbAlreadyHooked : true if func is already hooked
// Return : NULL on error
//          on success 
//               - if Hook was not existing Allocate and return a new List Item containing an empty APIInfo struct
//               - else return the old List Item containing APIInfo struct to allow update of some fields
//-----------------------------------------------------------------------------
CLinkListItem* __stdcall GetAssociatedItemAPIInfo(PBYTE pbAPI,BOOL* pbAlreadyHooked)
{
    API_INFO *pAPIInfo;
    CLinkListItem* pItemAPIInfo;

    if ((pbAPI==NULL)||(pbAlreadyHooked==NULL))
        return NULL;
    *pbAlreadyHooked=FALSE;
    // Is it already hooked ?
    
CheckIfHooked:
    pLinkListAPIInfos->Lock();
    for (pItemAPIInfo=pLinkListAPIInfos->Head; pItemAPIInfo; pItemAPIInfo=pItemAPIInfo->NextItem)
    {
        pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
        if (pAPIInfo->APIAddress == (FARPROC)pbAPI)
        {
            // In case of monitoring file reload, assume the pAPIInfo is not the one being unloading
            if (pAPIInfo->FreeingMemory||pAPIInfo->AskedToRemove)
            {
                pLinkListAPIInfos->Unlock();
                Sleep(UNHOOK_SECURITY_WAIT_TIME_BEFORE_MEMORY_FREEING*4);
                // check again in case item is still freeing
                goto CheckIfHooked;
            }
            else
            {
                // return old hook to allow update some API_INFO fields
                *pbAlreadyHooked=TRUE;
                pLinkListAPIInfos->Unlock();
                return pItemAPIInfo;
            }
        }
        
    }
    
    // No, so add a new item
    pItemAPIInfo =pLinkListAPIInfos->AddItem(TRUE);

    // store api address
    pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
    pAPIInfo->APIAddress=(FARPROC)pbAPI;

    pLinkListAPIInfos->Unlock();

    return pItemAPIInfo;
}

//-----------------------------------------------------------------------------
// Name: HookAPIFunction
// Object: Patch Opcode located at pAPIInfo->APIAddress to make a call to APIHandler
//         Save original bytes in pAPIInfo->Opcodes
//         Save modified bytes in pAPIInfo->pbHookCodes to avoid recomputing on each hook restoration
// Parameters :
//     in out  : API_INFO *pAPIInfo : pAPIInfo->APIAddress contains virtual address of API to patch
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL __stdcall HookAPIFunction(API_INFO *pAPIInfo)
{
    PBYTE* pAPI;
    BOOL Result;
    DWORD Index;
    DWORD dwOldProtectionFlags;
    BOOL bFirstBytesCanExecuteAnywhere=FALSE;
    CHookAvailabilityCheck::IsFunctionHookableResult CheckResult;
    CHookAvailabilityCheck::STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT FirstBytesCanExecuteAnywhereResult;

    if (pAPIInfo == NULL)
        return FALSE;

    // if pAPIInfo->Opcodes are defined, that means function can already be hooked
    // and in this case if hook is installed,
    // the instruction
    // memcpy(pAPIInfo->Opcodes, pAPIInfo->APIAddress, pAPIInfo->OpcodeReplacementSize);
    // will store hook opcodes in pAPIInfo->Opcodes and we get an infinite loop :
    // APIHandler is always reentering when trying to execute original opcodes
    if (*pAPIInfo->Opcodes)
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        return FALSE;
    }

    if (IsBadCodePtr((FARPROC)pAPIInfo->APIAddress))
    {
        TCHAR psz[3*MAX_PATH];
        _sntprintf(psz,3*MAX_PATH,_T("Bad code pointer 0x%p for function %s in module %s"),pAPIInfo->APIAddress,pAPIInfo->szAPIName,pAPIInfo->szModuleName);
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,psz);
        return FALSE;
    }

    // if we are patching function pointer
    if (pAPIInfo->bFunctionPointer)
    {
        // code replacement size is the size of a pointer
        pAPIInfo->OpcodeReplacementSize=sizeof(PBYTE);
        if (IsBadCodePtr((FARPROC)pAPIInfo->APIAddress))
        {
            TCHAR psz[3*MAX_PATH];
            _sntprintf(psz,3*MAX_PATH,_T("Bad code pointer 0x%p for function %s in module %s"),pAPIInfo->APIAddress,pAPIInfo->szAPIName,pAPIInfo->szModuleName);
            CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,psz);
            return FALSE;
        }
    }
    else
        // else, code replacement size is the size of our hook
        pAPIInfo->OpcodeReplacementSize=OPCODE_REPLACEMENT_SIZE;

    // allow write access to memory
    Result = RemoveProtection(pAPIInfo);
    if (Result == FALSE)
        return FALSE;

    pAPIInfo->UseCount=0;


    // Save first pAPIInfo->OpcodeReplacementSize bytes of API
    memcpy(pAPIInfo->Opcodes, pAPIInfo->APIAddress, pAPIInfo->OpcodeReplacementSize);

    if (!pAPIInfo->bFunctionPointer)
    {

        // check FirstBytesCanExecuteAnywhereSize value
        // should be -1,0 or OPCODE_REPLACEMENT_SIZE<=FirstBytesCanExecuteAnywhereSize
        if (pAPIInfo->FirstBytesCanExecuteAnywhereSize<OPCODE_REPLACEMENT_SIZE)
            pAPIInfo->FirstBytesCanExecuteAnywhereSize=0;

        // if first byte can be executed anywhere size is specified
        if ((pAPIInfo->FirstBytesCanExecuteAnywhereSize!=0)&&(pAPIInfo->FirstBytesCanExecuteAnywhereSize!=(DWORD)-1))
            // believe in provided information
            bFirstBytesCanExecuteAnywhere=TRUE;

        // if first byte can be executed anywhere must be computed
        if ((pAPIInfo->FirstBytesCanExecuteAnywhereSize==0)
            &&(FirstBytesAutoAnalysis!=FIRST_BYTES_AUTO_ANALYSIS_NONE)
            )
        {
            Result=FALSE;

            // we have to check if function is hookable and if first bytes can be executed anywhere
            
            // set default value
            bFirstBytesCanExecuteAnywhere=FALSE;

            // do first byte checking to verify that function can be hooked
            Result=CHookAvailabilityCheck::IsFunctionHookable((PBYTE)pAPIInfo->APIAddress,&CheckResult);

            if (Result)
            {
                if (CheckResult==CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE)
                {
                    // if function seems to be not hookable query the user the action to do
                    Result=FALSE;
                    TCHAR psz[3*MAX_PATH];
                    _stprintf(psz,
                            _T("Function %s seems to be not hookable.\r\n")
                            _T("If you try to hook it, your targeted application may will crash\r\n")
                            _T("Do you want to try to hook it anyway ?\r\n(check function first bytes if you're not sure)\r\n\r\n")
                            _T("Notice: To remove this warning, disable function in monitoring file, or use a %s/%s option"),
                            pAPIInfo->szAPIName,
                            OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE,
                            OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE);
                    if (DynamicMessageBoxInDefaultStation(NULL,psz,_T("Warning"),MB_ICONWARNING|MB_TOPMOST|MB_YESNO)==IDYES)
                        Result=TRUE;
                }
                else if ((CheckResult==CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE)
                        ||(Result==FALSE))
                {
                    // if function seems to be not hookable query the user the action to do
                    Result=FALSE;
                    TCHAR psz[3*MAX_PATH];
                    _stprintf(psz,
                        _T("Function %s could be not hookable.\r\n")
                        _T("Look at function first bytes to check if you can hook it.\r\n")
                        _T("Do you want to hook it ?\r\n\r\n")
                        _T("Notice: To remove this warning add a %s or %s option to your monitoring file"),
                        pAPIInfo->szAPIName,
                        OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE,
                        OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE);
                    if (DynamicMessageBoxInDefaultStation(NULL,psz,_T("Warning"),MB_ICONWARNING|MB_TOPMOST|MB_YESNO)==IDYES)
                        Result=TRUE;
                }
            }

            // if function is hookable
            if (Result)
            {
                // check if first bytes can be executed anywhere
                Result=CHookAvailabilityCheck::CanFirstBytesBeExecutedAnyWhere((PBYTE)pAPIInfo->APIAddress,&FirstBytesCanExecuteAnywhereResult);
                if (Result)
                {
                    // in case of insecure first bytes analysis
                    if (FirstBytesAutoAnalysis==FIRST_BYTES_AUTO_ANALYSIS_INSECURE)
                    {
                        if (FirstBytesCanExecuteAnywhereResult.FirstBytesCanBeExecutedAnyWhereResult==CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY)
                            FirstBytesCanExecuteAnywhereResult.FirstBytesCanBeExecutedAnyWhereResult=CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES;
                        else if (FirstBytesCanExecuteAnywhereResult.FirstBytesCanBeExecutedAnyWhereResult==CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY_NEED_RELATIVE_ADDRESS_CHANGES)
                            FirstBytesCanExecuteAnywhereResult.FirstBytesCanBeExecutedAnyWhereResult=CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES_NEED_RELATIVE_ADDRESS_CHANGES;
                    }

                    // update pAPIInfo with first bytes analysis result
                    if (FirstBytesCanExecuteAnywhereResult.FirstBytesCanBeExecutedAnyWhereResult==CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES)
                    {
                        bFirstBytesCanExecuteAnywhere=TRUE;
                        pAPIInfo->FirstBytesCanExecuteAnywhereSize=FirstBytesCanExecuteAnywhereResult.NbBytesToExecuteAtAnotherPlace;
                    }
                    else if(FirstBytesCanExecuteAnywhereResult.FirstBytesCanBeExecutedAnyWhereResult==CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES_NEED_RELATIVE_ADDRESS_CHANGES)
                    {
                        bFirstBytesCanExecuteAnywhere=TRUE;
                        pAPIInfo->FirstBytesCanExecuteAnywhereSize=FirstBytesCanExecuteAnywhereResult.NbBytesToExecuteAtAnotherPlace;
                        pAPIInfo->FirstBytesCanExecuteAnywhereNeedRelativeAddressChange=TRUE;
                    }
                }
            }
        }

        // if first bytes can be executed anywhere
        if (bFirstBytesCanExecuteAnywhere)
        {
            // algorithm is the following
            // jmp pAPIInfo->OpcodesExecutedAtAnotherPlace
            // [optional jmp pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended, if OpcodesExecutedAtAnotherPlace buffer size is not enough]
            // [optional jmp after API modified bytes to continue function execution, if no ret instruction executed]

            // by default pAPIInfo->OpcodesExecutedAtAnotherPlace contains function first bytes
            PBYTE pExecutedBuffer=pAPIInfo->OpcodesExecutedAtAnotherPlace;

            // if pAPIInfo->OpcodesExecutedAtAnotherPlace buffer is too small
            if (FIRST_OPCODES_MAX_SIZE<pAPIInfo->FirstBytesCanExecuteAnywhereSize)
            {
                // if buffer was already allocated 
                // (can occurs in case of 2 call of HookAPIFunction without freeing item. Ex : .NET rejitted function)
                if (pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended)
                {
                    HeapFree(ApiOverrideHeap, 0, pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended);
                    pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended=NULL;
                }

                // allocate a sufficient buffer
                pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended=
                    (PBYTE)HeapAlloc(ApiOverrideHeap,
                                    HEAP_ZERO_MEMORY,// for easiest debugging only (in DEBUG and RELEASE mode)
                                    pAPIInfo->FirstBytesCanExecuteAnywhereSize+REENTER_FUNCTION_FLOW_OPCODE_SIZE);
                // set OpcodesExecutedAtAnotherPlaceExtended as buffer containing function first bytes
                pExecutedBuffer=pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended;

                // for an easiest use in the rest of this software, 
                // we use pAPIInfo->OpcodesExecutedAtAnotherPlace to jump to pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended
                // so all our call made for OpcodesExecutedAtAnotherPlace will be translated to OpcodesExecutedAtAnotherPlaceExtended
                // (no need to add another case: / else if)

                // pAPIInfo->OpcodesExecutedAtAnotherPlace =jmp pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended
                Index=0;
                // jump to OpcodesExecutedAtAnotherPlaceExtended
                pAPIInfo->OpcodesExecutedAtAnotherPlace[Index++] = 0xE9;// relative jmp asm instruction
                // get pointer for storing jmp address
                pAPI = (PBYTE*)&pAPIInfo->OpcodesExecutedAtAnotherPlace[Index];
                // compute relative address from current address to OpcodesExecutedAtAnotherPlaceExtended
                pAPI[0] = (PBYTE)((UINT_PTR)pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended 
                    - (UINT_PTR)(&pAPIInfo->OpcodesExecutedAtAnotherPlace[Index])-sizeof(PBYTE));
            }

            // get first bytes of API
            memcpy(pExecutedBuffer, pAPIInfo->APIAddress, pAPIInfo->FirstBytesCanExecuteAnywhereSize);

            // begin of function will be executed at pAPIInfo->OpcodesExecutedAtAnotherPlace address
            // so pAPIInfo->OpcodesExecutedAtAnotherPlace must be executable memory, and opcode after OPCODE_REPLACEMENT_SIZE have
            // to do a jump to Original API Address + OPCODE_REPLACEMENT_SIZE

            // mark memory as executable
            // as pExecutedBuffer is not system page rounded, do not use "if (!VirtualProtect(pExecutedBuffer, dwSystemPageSize,..."
            VirtualProtect(pExecutedBuffer, pAPIInfo->FirstBytesCanExecuteAnywhereSize+REENTER_FUNCTION_FLOW_OPCODE_SIZE, PAGE_EXECUTE_READWRITE, &dwOldProtectionFlags);

            ///////////////////////////////////////////////
            // jump from OpcodesExecutedAtAnotherPlace/OpcodesExecutedAtAnotherPlaceExtended to next function original byte
            // update REENTER_FUNCTION_FLOW_OPCODE_SIZE if you change this part
            ///////////////////////////////////////////////
            Index=pAPIInfo->FirstBytesCanExecuteAnywhereSize;
            // compute jump between pAPIInfo->Opcodes and original address + OPCODE_REPLACEMENT_SIZE
            // jump to handler
            pExecutedBuffer[Index++] = 0xE9;// relative jmp asm instruction
            // get pointer for storing jmp address
            pAPI = (PBYTE*)&pExecutedBuffer[Index];
            // compute relative address from current address to begin of API + FirstBytesCanExecuteAnyWhereSize
            pAPI[0] = (PBYTE)((UINT_PTR)pAPIInfo->APIAddress+pAPIInfo->FirstBytesCanExecuteAnywhereSize 
                - (UINT_PTR)(&pExecutedBuffer[Index])-sizeof(PBYTE));

            ///////////////////////////////////////////////
            // if there's a relative 32bit address we have to compute new relative address from current position
            ///////////////////////////////////////////////
            if (pAPIInfo->FirstBytesCanExecuteAnywhereNeedRelativeAddressChange)
            {
                PBYTE RelativeAddress;
                // we currently support only one relative address change, only as last position
                // (see auto analysis)
                // --> relative address is at FirstBytesCanExecuteAnywhereSize-sizeof(PBYTE)
                pAPI=(PBYTE*)&pExecutedBuffer[pAPIInfo->FirstBytesCanExecuteAnywhereSize-sizeof(PBYTE)];
                // get original relative address
                RelativeAddress=*pAPI;
                // compute new relative address = oldRVA+oldRelative-newRVA
                pAPI[0]=(PBYTE)RelativeAddress+(UINT_PTR)pAPIInfo->APIAddress-(UINT_PTR)pExecutedBuffer;
            }
        }
        else // first bytes can't be executed anywhere
        {
            // adjust pAPIInfo->FirstBytesCanExecuteAnywhereSize
            pAPIInfo->FirstBytesCanExecuteAnywhereSize=0;
        }

    }
    /////////////////////////
    // first hook : only call second hook to overwrite the lesser byte as possible
    // this buffer is copied at the original address of API func, so code is never executed 
    // at this address --> no need to mark memory as PAGE_EXECUTE_READWRITE
    /////////////////////////
    Index=0;
    if (pAPIInfo->bFunctionPointer)
    {
        // we only have to change address pointed at pAPIInfo->APIAddress
        // by the address of pAPIInfo->pbSecondHook
        // address is absolute --> no need to compute relative address

        // get pointer for storing call address
        pAPI = (PBYTE*)&pAPIInfo->pbHookCodes[Index];
        // fill 
        pAPI[0] = pAPIInfo->pbSecondHook;
    }
    else
    {
        // we have to overwrite first function bytes
        pAPIInfo->pbHookCodes[Index++] = 0xE9;// JMP rel asm instruction
        // get pointer for storing call address
        pAPI = (PBYTE*)&pAPIInfo->pbHookCodes[Index];
        // compute relative address from API address to our second hooking function (pbSecondHook)
        pAPI[0] = (PBYTE)pAPIInfo->pbSecondHook - (UINT_PTR)pAPIInfo->APIAddress - OPCODE_REPLACEMENT_SIZE;
    }


    /////////////////////////
    // second hook : push pApiInfo and registers, and next jump to APIHandler
    // code is executed at pAPIInfo->pbSecondHook address --> we need to set memory as PAGE_EXECUTE_READWRITE
    /////////////////////////
    // VirtualProtect(pAPIInfo->pbSecondHook, dwSystemPageSize, PAGE_EXECUTE_READWRITE, &dwOldProtectionFlags);
    // as pAPIInfo->Opcodes some time needs to be executed, change the page protection for full pAPIInfo
    // as pAPIInfo is not system page rounded, do not use "if (!VirtualProtect(pAPIInfo, dwSystemPageSize,..."
    VirtualProtect(pAPIInfo, sizeof(API_INFO), PAGE_EXECUTE_READWRITE, &dwOldProtectionFlags);

    Index=0;

    // push pAPIInfo
    pAPIInfo->pbSecondHook[Index++] = 0x68; // PUSH asm instruction
    pAPI = (PBYTE*)&pAPIInfo->pbSecondHook[Index];
    pAPI[0]=(PBYTE)pAPIInfo;
    Index+=sizeof(PBYTE);

    // jump to handler
    pAPIInfo->pbSecondHook[Index++] = 0xE9;// relative jmp asm instruction
    // get pointer for storing call address
    pAPI = (PBYTE*)&pAPIInfo->pbSecondHook[Index];

    // compute relative address from pbSecondHook to our API hooking function (APIHandler)
    // pAPI[0] = APIHandler - pAPIInfo->pbSecondHook - SECOND_HOOK_SIZE;
    pAPI[0] = (PBYTE)APIHandler - (UINT_PTR)pAPIInfo->pbSecondHook - (Index+sizeof(PBYTE));

    //////////////////
    // change original code to CALL second hook
    // after this, hook is active
    //////////////////
    // flag to know we are installing hook (in case of dump)
    pAPIInfo->bOriginalOpcodes=FALSE;
    memcpy(pAPIInfo->APIAddress,pAPIInfo->pbHookCodes,pAPIInfo->OpcodeReplacementSize);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UnhookAPIFunction
// Object: try to remove hook (dll in use are not free to avoid crash)
//         should be not called directly use UnHookIfPossible instead
// Parameters :
//     in out  : CLinkListItem *pItemAPIInfo : free on success
// Return : FALSE on error or func is not unhook now, TRUE on success
//-----------------------------------------------------------------------------
BOOL UnhookAPIFunction(CLinkListItem *pItemAPIInfo)
{
    FREE_APIINFO sFreeApiInfo;
    API_INFO *pAPIInfo;
    int iMsgBoxRes;
    DWORD dwWaitRes;
    PBYTE pHookCodes[MAX_OPCODE_REPLACEMENT_SIZE];

    if (IsBadReadPtr(pItemAPIInfo,sizeof(CLinkListItem)))
        return FALSE;
    pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
    if (IsBadReadPtr(pAPIInfo,sizeof(API_INFO)))
        return FALSE;

    if (pAPIInfo->FreeingMemory)
        return TRUE;

    // inform we want to remove hook
    pAPIInfo->AskedToRemove=TRUE;

    // store hook codes for later checking
    memcpy(pHookCodes,pAPIInfo->pbHookCodes,pAPIInfo->OpcodeReplacementSize);

    // change pbHookCodes to Opcodes to avoid hook to be restore at the end of hooking
    memcpy(pAPIInfo->pbHookCodes, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize);

    // restore original opcode to avoid hook entering 

    // flag to know we are restoring original opcodes (in case of dump)
    pAPIInfo->bOriginalOpcodes=TRUE;

    // if dll has been unloaded without hook removal
    // or if pAPIInfo has never been hooked
    if (IsBadCodePtr((FARPROC)pAPIInfo->APIAddress))
    {
        // free API_INFO associated to hook
        ReleaseAndFreeApiInfo(pItemAPIInfo);
        return TRUE;
    }
    // restore bytes only if not already done
    if (memcmp(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize))
    {
        // assume opcodes is our one !
        // it can appear that for COM dll are unloaded and next reloaded at the same space,
        // if it's done too quickly or during COM unhooking, we can have original bytes
        // with original memory protection (due to reloading of dll), so pAPIInfo->APIAddress can be write protected
        if (memcmp(pAPIInfo->APIAddress,pHookCodes, pAPIInfo->OpcodeReplacementSize)==0)
        {
            // restore original opcodes
            if (!IsBadWritePtr(pAPIInfo->APIAddress,pAPIInfo->OpcodeReplacementSize))
                memcpy(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize);
        }
    }

    // Make sure we are not in hooking func
    iMsgBoxRes=IDYES;

    // wait end of hook
    // monitoring could be unhooked without waiting, but this can cause trouble because we can't know
    // if we can unload this dll or not (if we unload it and a monitoring hook is in use, the return
    // address will be in our unloaded dll --> beautiful crash)

    while(iMsgBoxRes==IDYES)
    {
        // set event to go out of our WaitForMultiple(xx,INFINITE) in ThreadFreeingHooksProc (it can avoid API freeing lock)
        SetEvent(hevtWaitForUnlocker);

        // assume API is not currently hooked, or if in use wait for the end of hook 
        dwWaitRes=WaitForSingleObject(pAPIInfo->evtEndOfHook,UNHOOK_MAX_WAIT_TIME);
        if (dwWaitRes==WAIT_OBJECT_0)
        {
            // wait a while that the hook ends for the last called functions
            // can be the case for func called between SetEvent (included) and pAPIInfo->UseCount--
            while (pAPIInfo->UseCount!=0)
                Sleep(50);
            break;
        }
        //else if (dwWaitRes==WAIT_TIMEOUT)
        //{
        //    // put the user aware and hope he will do actions that unlock fake api
        //    // a good sample of blocking call are MessageBox
        //    TCHAR pszMsg[2*MAX_PATH];
        //    _sntprintf(pszMsg,2*MAX_PATH,_T("Warning %s in module %s\r\n is in use.\r\nDo you want to wait more time ?"),pAPIInfo->szAPIName,pAPIInfo->szModuleName);
        //    iMsgBoxRes=DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST);
        //}
        //else
            break;
    }

    // put flag to indicate we are going to free hook
    pAPIInfo->FreeingMemory=TRUE;
    // add an item to the list of item to be free
    sFreeApiInfo.InitialTickCount=GetTickCount();
    sFreeApiInfo.pItemAPIInfo=pItemAPIInfo;
    // save current monitoring flag (pMonitoringFileInfos could have been empty before this call,
    // so use MonitoringParamCount to try guess if data should be logged)
    sFreeApiInfo.ForceLog=bMonitoring && (pAPIInfo->MonitoringParamCount>0);
    pLinkListAPIInfosToBeFree->AddItem(&sFreeApiInfo);
    // set an event to signal to the thread charged of hook freeing that there's new data to free
    SetEvent(hevtFreeAPIInfo);

    return TRUE;
}
DWORD WINAPI ThreadFreeingHooksProc(LPVOID lpParameter)
{
    HANDLE ph[3]={hevtAllAPIUnhookedDllFreeAll,hevtWaitForUnlocker,hevtFreeAPIInfo};
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    PFREE_APIINFO pFreeAPIInfo;
    API_INFO* pApiInfo;
    DWORD dwWaitRes;
    BOOL bContinue=TRUE;
    BOOL bApplicationUnload=(BOOL)lpParameter;
    
    while(bContinue)
    {
        // if application is being unload
        if (bApplicationUnload)
            // don't wait, ask only to free all hooks
            dwWaitRes=WAIT_OBJECT_0+2;
        else
            // wait for unlocking event, item added to list event, thread closing query event
            dwWaitRes=WaitForMultipleObjects(3,ph,FALSE,INFINITE);

        switch (dwWaitRes)
        {
        case WAIT_OBJECT_0: // hevtAllAPIUnhookedDllFreeAll
            // all api are unhooked and dll must detach --> just go out of current thread
            bContinue=FALSE;
            break;
        case WAIT_OBJECT_0+1: // hevtWaitForUnlocker
            // just event to wake up WaitForMultipleObjects and unlock it
            break;
        case WAIT_OBJECT_0+2: // hevtFreeAPIInfo
            // some data have to be free
            if (pLinkListAPIInfosToBeFree)
            {
                pLinkListAPIInfosToBeFree->Lock();
FreeAPIInfosToBeFreeLinkList:
                for (pItem=pLinkListAPIInfosToBeFree->Head;pItem;pItem=pNextItem)
                {
                    pFreeAPIInfo=(PFREE_APIINFO)pItem->ItemData;

                    // assume we have waited UNHOOK_SECURITY_WAIT_TIME_BEFORE_MEMORY_FREEING
                    if ((GetTickCount()-pFreeAPIInfo->InitialTickCount)<UNHOOK_SECURITY_WAIT_TIME_BEFORE_MEMORY_FREEING)
                        Sleep(UNHOOK_SECURITY_WAIT_TIME_BEFORE_MEMORY_FREEING);

                    // get next item now (before freeing pItem)
                    pNextItem=pItem->NextItem;

                    // Assume no more hook is in use
                    pApiInfo=(API_INFO*)pFreeAPIInfo->pItemAPIInfo->ItemData;
                    if (!IsBadReadPtr(pApiInfo,sizeof(API_INFO)))
                    {
                        // if hook still in use
                        if (pApiInfo->UseCount>0)
                        {
                            // for each thread, 
                            // if a hook match the current removed one,
                            // restore original return address and and original hook handler
                            // and log informations as InNoRet

                            CLinkListItem* pItemThreadInfos;
                            CApiOverrideTlsData* pApiOverrideTlsData;
                            CLinkListSingleThreaded* pLinkListTlsData;
                            CLinkListItem* pItemTlsData;
                            API_HANDLER_TLS_DATA* pTlsData;
                            pLinkListAllThreadTlsData->Lock();
                            HANDLE hThreadHandle;

                            // for each thread
                            for (pItemThreadInfos=pLinkListAllThreadTlsData->Head;pItemThreadInfos;pItemThreadInfos=pItemThreadInfos->NextItem)
                            {
                                pApiOverrideTlsData=(CApiOverrideTlsData*)pItemThreadInfos->ItemData;
                                if (!pApiOverrideTlsData->pLinkListTlsData->Head)
                                    continue;
                                if (pApiOverrideTlsData->ThreadId==GetCurrentThreadId())
                                    continue;
                                hThreadHandle = OpenThread(THREAD_SUSPEND_RESUME,FALSE,pApiOverrideTlsData->ThreadId);
RestoreOriginalAddress:
                                SuspendThread(hThreadHandle);
                                pLinkListTlsData=pApiOverrideTlsData->pLinkListTlsData;
                                // for each item
                                for (pItemTlsData=pLinkListTlsData->Head;pItemTlsData;pItemTlsData=pItemTlsData->NextItem)
                                {
                                    pTlsData=(API_HANDLER_TLS_DATA*)pItemTlsData->ItemData;
                                    if (pTlsData->pAPIInfo==pApiInfo)
                                    {
                                        // if function has been executed and we are in our after call handler
                                        if (pTlsData->ExecutionCompleted)
                                        {
                                            // resume thread
                                            ResumeThread(hThreadHandle);
                                            // let time to our after call handler to finish itself
                                            Sleep(500);
                                            // check again
                                            goto RestoreOriginalAddress;
                                        }

                                        // restore return address and original exception handler
                                        *((PBYTE*)pTlsData->AddressOfOriginalReturnAddress)=pTlsData->OriginalReturnAddress;
                                        pTlsData->UnhookedDuringCall=TRUE;
                                        UnhookExceptionHandler(pTlsData->OriginalExceptionHandler,pTlsData->OriginalExceptionHandlerAddress);
                                        SendLogInsideHookedFunctionCall(pLinkListTlsData,pTlsData,FALSE,pFreeAPIInfo->ForceLog || pTlsData->LogInfoIn.ParmeterParsed,pApiOverrideTlsData->ThreadId);
                                    }
                                }
                                ResumeThread(hThreadHandle);
                                CloseHandle(hThreadHandle);
                            }
                            pLinkListAllThreadTlsData->Unlock();
                        }
                    }
                    pLinkListAPIInfosToBeFree->Unlock();

                    // free API_INFO associated to hook
                    ReleaseAndFreeApiInfo(pFreeAPIInfo->pItemAPIInfo);

                    pLinkListAPIInfosToBeFree->Lock();

                    // remove item from list
                    pLinkListAPIInfosToBeFree->RemoveItem(pItem,TRUE);

                    if (!pLinkListAPIInfosToBeFree->IsItemStillInList(pNextItem,TRUE))
                    {
                        goto FreeAPIInfosToBeFreeLinkList;
                    }

                }
                pLinkListAPIInfosToBeFree->Unlock();
                
            }

            // if application is unloading, we are not in a thread but in func
            if (bApplicationUnload)
                // just go out of this func
                bContinue=FALSE;

            break;
        default: // error --> go out of thread (useless to loop on failing WaitForMultipleObjects)
            // don't close handles because they may will be use by other threads
            return 0xFFFFFFFF;
        }
    }
    CleanCloseHandle(&hevtFreeAPIInfo);
    CleanCloseHandle(&hevtAllAPIUnhookedDllFreeAll);
    FreeingThreadGracefullyClosed=TRUE;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: WaitForAllHookFreeing
// Object: assume there's no more item being freeing
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void WaitForAllHookFreeing()
{
    // if dll unload as begin, other threads are already destroyed
    // so hThreadFreeingHooks is already close and we'll get no chance to unhook
    // remaining func. So we can wait indefinitely
    // to avoid this just check the hevtUnload event state
    if (WaitForSingleObject(hevtUnload,0)==WAIT_OBJECT_0)
        return;// dll as begin to unload --> just return

    while (pLinkListAPIInfosToBeFree->GetItemsCount()!=0)
    {
        // wait for item unload
        Sleep(UNHOOK_SECURITY_WAIT_TIME_BEFORE_MEMORY_FREEING*4);
    }
}

//-----------------------------------------------------------------------------
// Name: UnhookAllAPIFunctions
// Object: try to remove all hooks (dll in use are not free to avoid crash)
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void UnhookAllAPIFunctions()
{
    BOOL bSuccess=TRUE;
    BOOL bRet;

    // remove all remaining hooks 
    if (pLinkListAPIInfos==NULL)
        return;
    
    // remove com hooks first
    if (pComManager)
    {
        pComManager->StopHookingCreatedCOMObjects();
        pComManager->UnHookAllComObjects();
    }
    if (pNetManager)
    {
        // if not called on dll detach (else deadlock can appear)
        // this is not a trouble as hookknet.dll manages it's DLL_PROCESS_DETACH
        // and internally calls UnHookAllNetMethods
        if (!ApiOverrideDllDetaching)
        {
            pNetManager->StopAutoHooking();
            pNetManager->UnHookAllNetMethods();
        }
    }
    // remove monitoring file hooks
    bRet=UnloadAllMonitoringFiles();
    bSuccess=bSuccess&&bRet;

    // unload all fake api dll hooks
    bRet=UnloadAllFakeApiDlls();
    bSuccess=bSuccess&&bRet;

    // wait until all item are freed (all handler successfully unhooked)
    // this should not wait as WaitForAllHookFreeing is called by
    // each UnloadFakeApiDll and each UnloadFakeApiDll
    WaitForAllHookFreeing();

    // in case there's still remain some func (can appear in case of deadlock)
    if ((!bSuccess) || pLinkListAPIInfos->Head)
    {
        if (WaitForSingleObject(hevtUnload,0)!=WAIT_OBJECT_0)// avoid 2 message 1 for warning and the over for unexpected unload
        {
            DynamicMessageBoxInDefaultStation(NULL,
                            _T("Warning all functions are not unhooked and injected dll is going to be unload.\r\n")
                            _T("This will probably crash your target application.\r\n")
                            _T("So save all your target application work, and when done click OK"),
                            _T("WARNING"),
                            MB_OK|MB_ICONWARNING|MB_TOPMOST);
        }
    }
}


//-----------------------------------------------------------------------------
// Name: GetFuncAddr
// Object: Load Library specified by pszModuleName only if not already loaded and
//          get function address of specified func name pszAPIName
// Parameters :
//      in: TCHAR* pszModuleName : dll name
//          TCHAR* pszAPIName : function name
//      out : HMODULE* phModule module handle 
// Return : func pointer on success, NULL on error
//-----------------------------------------------------------------------------
PBYTE GetFuncAddr(TCHAR* pszModuleName,TCHAR* pszAPIName,HMODULE* phModule)
{
    PBYTE pbAPI;
#if (defined(UNICODE)||defined(_UNICODE))
    CHAR pcFuncName[MAX_PATH];
#endif
    BOOL bLibraryLoaded=FALSE;
    if (phModule == NULL)
        return NULL;

    CDllStub* pDllStub = CHookAvailabilityCheck::GetDllStub();// quite dirty for object oriented soft, but allow to have only one object in memory 
    if (!pDllStub)
        return NULL;

    TCHAR RealModuleName[MAX_PATH];
    TCHAR* ModuleName;

    ModuleName = CStdFileOperations::GetFileName(pszModuleName);
    if (pDllStub->IsStubDll(ModuleName) )
    {
        if ( !pDllStub->GetRealModuleNameFromStubName( _T(""),ModuleName,RealModuleName ) )
            return NULL;
        ModuleName = RealModuleName;
    }
    else
        ModuleName = pszModuleName;

    // get module handle
    *phModule = GetModuleHandle(ModuleName);
    if (*phModule == NULL)
    {
        // if module handle not found load library
        *phModule=LoadLibrary(ModuleName);
        if (*phModule == NULL)
            return NULL;
        bLibraryLoaded=TRUE;
    }
#if (defined(UNICODE)||defined(_UNICODE))
    // convert into ansi until GetProcAddress don't support unicode
    CAnsiUnicodeConvert::UnicodeToAnsi(pszAPIName,pcFuncName,MAX_PATH);
    pbAPI = (PBYTE)GetProcAddress(*phModule, pcFuncName);
#else
    pbAPI = (PBYTE)GetProcAddress(*phModule, pszAPIName);
#endif
    if (pbAPI == NULL)
    {
        if (bLibraryLoaded)
            FreeLibrary(*phModule);
        return NULL;
    }
    return pbAPI;
}

//-----------------------------------------------------------------------------
// Name: GetForceLoadedDll
// Object: check if a dll has already be loaded by this module for monitoring or faking
// Parameters :
//      in: TCHAR* DllName : dll name
//      out : 
// Return : HMODULE if dll has already be loaded by this module, NULL else
//-----------------------------------------------------------------------------
HMODULE GetForceLoadedDll(TCHAR* DllName)
{
    CLinkListItem* pItem;
    LOADED_DLL* pLoadedDllInfos;

    // look in pLinkListLoadedDll to check if we have already loaded dll
    for (pItem = pLinkListLoadedDll->Head;pItem;pItem = pItem->NextItem)
    {
        pLoadedDllInfos=(LOADED_DLL*)pItem->ItemData;
        // if Dllname == already loaded dll name
        if (_tcsicmp(pLoadedDllInfos->Name, DllName) == 0)
        {
            // return already stored module handle
            return pLoadedDllInfos->hModule;
        }
    }
    // not found
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: AvoidDllUnload
// Object: Force LoadLibrary instead of GetModuleHandle to force dll refcount to be incremented,
//          so FreeLibrary will only decrease ref count
// Parameters :
//      in: TCHAR* DllName : dll name
//      out : 
// Return : dll HMODULE 
//-----------------------------------------------------------------------------
HMODULE AvoidDllUnload(TCHAR* DllName)
{
    HMODULE hModule;
    hModule = GetForceLoadedDll(DllName);
    if (hModule)
        return hModule;

    // if lib has already been loaded
    hModule=LoadLibrary(DllName);
    if (!hModule)
        return NULL;

    {
        LOADED_DLL LoadedDllInfos;
        _tcscpy(LoadedDllInfos.Name,DllName);
        LoadedDllInfos.hModule = hModule;
        pLinkListLoadedDll->AddItem(&LoadedDllInfos);
    }

    return hModule;
}
//-----------------------------------------------------------------------------
// Name: GetWinAPIOverrideFunctionDescriptionAddress
// Object: Load Library specified by pszModuleName only if not already loaded and
//          get function address of specified func name pszAPIName
// Parameters :
//      in: TCHAR* pszModuleName : dll name
//          TCHAR* pszAPIName : function name
//      out : BOOL* pbExeDllInternalHook 
//            BOOL* pbFunctionPointer 
// Return : func pointer on success, NULL on error
//-----------------------------------------------------------------------------
PBYTE __stdcall GetWinAPIOverrideFunctionDescriptionAddress(TCHAR* pszModuleName,TCHAR* pszAPIName,BOOL* pbExeDllInternalHook,BOOL* pbFunctionPointer)
{
    PBYTE pbAPI=NULL;
    *pbExeDllInternalHook=FALSE;
    *pbFunctionPointer=FALSE;



    // check if address is specified
    if (_tcsnicmp(pszModuleName,EXE_INTERNAL_RVA_POINTER_PREFIX,_tcslen(EXE_INTERNAL_RVA_POINTER_PREFIX))==0)
    {
        // remove prefix and get value
        pbAPI=0;
        _stscanf(&pszModuleName[_tcslen(EXE_INTERNAL_RVA_POINTER_PREFIX)],_T("%p"),&pbAPI);
        pbAPI+=(ULONG_PTR)::GetModuleHandle(0);
        *pbExeDllInternalHook=TRUE;// allow function to be callback (called by system dll)
        *pbFunctionPointer=TRUE;
    }
    else if (_tcsnicmp(pszModuleName,EXE_INTERNAL_RVA_PREFIX,_tcslen(EXE_INTERNAL_RVA_PREFIX))==0)
    {
        // remove prefix and get value
        pbAPI=0;
        _stscanf(&pszModuleName[_tcslen(EXE_INTERNAL_RVA_PREFIX)],_T("%p"),&pbAPI);
        pbAPI+=(ULONG_PTR)::GetModuleHandle(0);
        *pbExeDllInternalHook=TRUE;// allow function to be callback (called by system dll)
    }
    else if (_tcsnicmp(pszModuleName,EXE_INTERNAL_POINTER_PREFIX,_tcslen(EXE_INTERNAL_POINTER_PREFIX))==0)
    {
        // remove prefix and get value
        pbAPI=0;
        _stscanf(&pszModuleName[_tcslen(EXE_INTERNAL_POINTER_PREFIX)],_T("%p"),&pbAPI);
        *pbExeDllInternalHook=TRUE;// allow function to be callback (called by system dll)
        *pbFunctionPointer=TRUE;
    }
    else if (_tcsnicmp(pszModuleName,EXE_INTERNAL_PREFIX,_tcslen(EXE_INTERNAL_PREFIX))==0)
    {
        // remove prefix and get value
        pbAPI=0;
        _stscanf(&pszModuleName[_tcslen(EXE_INTERNAL_PREFIX)],_T("%p"),&pbAPI);
        *pbExeDllInternalHook=TRUE;// allow function to be callback (called by system dll)
    }
    else if (_tcsnicmp(pszModuleName,DLL_INTERNAL_PREFIX,_tcslen(DLL_INTERNAL_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        *pszDllName=0;
        PBYTE pbRvaFromDllBase=0;
        _stscanf(&pszModuleName[_tcslen(DLL_INTERNAL_PREFIX)],_T("%p@%s"),&pbRvaFromDllBase,(TCHAR*)pszDllName);
        AvoidDllUnload(pszDllName);
        pbAPI=GetExeRvaFromDllRva(pszDllName,pbRvaFromDllBase);
        *pbExeDllInternalHook=TRUE;// allow function to be callback (called by system dll)
    }
    else if (_tcsnicmp(pszModuleName,DLL_INTERNAL_POINTER_PREFIX,_tcslen(DLL_INTERNAL_POINTER_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        *pszDllName=0;
        PBYTE pbRvaFromDllBase=0;
        _stscanf(&pszModuleName[_tcslen(DLL_INTERNAL_POINTER_PREFIX)],_T("%p@%s"),&pbRvaFromDllBase,(TCHAR*)pszDllName);
        AvoidDllUnload(pszDllName);
        pbAPI=GetExeRvaFromDllRva(pszDllName,pbRvaFromDllBase);
        *pbExeDllInternalHook=TRUE;// allow function to be callback (called by system dll)
        *pbFunctionPointer=TRUE;
    }
    else if (_tcsnicmp(pszModuleName,DLL_ORDINAL_PREFIX,_tcslen(DLL_ORDINAL_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        *pszDllName=0;
        PBYTE pbOrdinalValue=0;
        _stscanf(&pszModuleName[_tcslen(DLL_ORDINAL_PREFIX)],_T("%p@%s"),&pbOrdinalValue,(TCHAR*)pszDllName);
        // get module handle
        HMODULE hModule = AvoidDllUnload(pszDllName);
        if (hModule == NULL)
            pbAPI= NULL;
        else
            pbAPI=(PBYTE)GetProcAddress(hModule,(LPCSTR)pbOrdinalValue);
    }
    else if (_tcsnicmp(pszModuleName,DLL_OR_EXE_NET_PREFIX,_tcslen(DLL_OR_EXE_NET_PREFIX))==0)
    {
        pbAPI=pNetManager->GetNetCompiledFunctionAddress(pszModuleName);
        *pbExeDllInternalHook=TRUE; // allow .Net function to be callback (called by system dll)
    }
    else
    {
        AvoidDllUnload(pszModuleName);
        // get address with loadlibrary + getprocaddress
        pbAPI=GetFuncAddr(pszModuleName,pszAPIName);
    }

    return pbAPI;
}

//-----------------------------------------------------------------------------
// Name: GetFuncAddr
// Object: Load Library specified by pszModuleName only if not already loaded and
//          get function address of specified func name pszAPIName
// Parameters :
//      in: TCHAR* pszModuleName : dll name
//          TCHAR* pszAPIName : function name
// Return : func pointer on success, NULL on error
//-----------------------------------------------------------------------------
PBYTE GetFuncAddr(TCHAR* pszModuleName,TCHAR* pszAPIName)
{
    HMODULE hModule;
    return GetFuncAddr(pszModuleName,pszAPIName,&hModule);
}

//-----------------------------------------------------------------------------
// Name: GetExeRvaFromDllRva
// Object: Convert a dll Rva address to current process Rva
//          add pbRvaFromDllBase to the real dll base address (the loaded one not prefered one)
// Parameters :
//      in: TCHAR* pszDllName : dll name
//          TCHAR* pbRvaFromDllBase : Rva from Dll bas address
// Return : process Rva on success, NULL on error
//-----------------------------------------------------------------------------
PBYTE GetExeRvaFromDllRva(TCHAR* pszDllName,PBYTE pbRvaFromDllBase)
{
    PBYTE pbHook;
    TCHAR* psz;
    MODULEENTRY32 me32 = {0}; 
    HANDLE hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwCurrentProcessID); // use snapshot to get module size, may speeder than retrieving size from pe

    if (hModuleSnap == INVALID_HANDLE_VALUE) 
        return NULL;

    // remove path from pszDllName if any to only keep module name
    psz=_tcsrchr(pszDllName,'\\');
    if (psz)
        pszDllName=psz+1;


    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        return NULL;
    }

    do 
    { 
        // if we have found the corresponding module name
        if (_tcsicmp(me32.szModule,pszDllName)==0)
        {
            // check if given relative address is valid
            if ((UINT_PTR)pbRvaFromDllBase>me32.modBaseSize)
            {
                // show user error message
                TCHAR pszMsg[MAX_PATH];
                _stprintf(  pszMsg,
                            _T("0x%p is an invalid relative address for module %s"),
                            pbRvaFromDllBase,
                            pszDllName);

                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

                // close snapshot handle
                CloseHandle(hModuleSnap);

                return NULL;
            }

            // compute exe RVA from dll RVA
            pbHook=(PBYTE)((UINT_PTR)pbRvaFromDllBase+(UINT_PTR)me32.modBaseAddr);

            // close snapshot handle
            CloseHandle(hModuleSnap);

            return pbHook;
        }
    } 
    while (Module32Next(hModuleSnap, &me32));

    // close handle
    CloseHandle(hModuleSnap);

    // not found
    return NULL;
}


//-----------------------------------------------------------------------------
// Name: Break
// Object: Make a break dialog avoiding new in APIHandler func
// Parameters :
//      in: PAPI_INFO pAPIInfo: associated pAPIInfo
//      out : 
// Return : 
//-----------------------------------------------------------------------------
void Break(PAPI_INFO pAPIInfo,LOG_INFOS* pLogInfo,PBYTE StackParamtersPointer,PREGISTERS pRegisters,double* pDoubleResult,PBYTE ReturnAddress,PBYTE CallerEbp,BOOL BeforeCall)
{
    CBreakUserInterface* pBreakUI;
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return;
    // show BreakUserInterface Dialog
    pBreakUI=new CBreakUserInterface(pAPIInfo,pLogInfo,StackParamtersPointer,pRegisters,pDoubleResult,ReturnAddress,CallerEbp,BeforeCall,pApiOverrideTlsData->pLinkListTlsData);
    pBreakUI->ShowDialog();
    delete pBreakUI;
}

//-----------------------------------------------------------------------------
// Name: BadParameterNumber
// Object: show message in case of bad parameter numbers in config file or faking dll
// Parameters :
//      in out : PAPI_INFO pAPIInfo: associated pAPIInfo (parameter numbers can be changed)
//      in: DWORD dwCurrentParamSize : param size currently store in pAPIInfo struct
//          DWORD StackChangedSize : real number of params
// Return : 
//-----------------------------------------------------------------------------
void BadParameterNumber(PAPI_INFO pAPIInfo,DWORD dwCurrentParamSize,DWORD StackChangedSize)
{
    TCHAR szMsg[3*MAX_PATH];
    int RealNbParam;
    int cnt;

    // update stack size
    pAPIInfo->StackSize=StackChangedSize;

    if (StackChangedSize>dwCurrentParamSize)
    {
        // compute real nb parameter generally dwParamSize/4 in 32 bit world
        // try to take into account parameters info in case of struct, double ... params (more than 4 bytes)
        RealNbParam=pAPIInfo->MonitoringParamCount+(StackChangedSize-dwCurrentParamSize)/REGISTER_BYTE_SIZE;
    }
    else
        // compute real nb parameter generally dwParamSize/4 in 32 bit world
        RealNbParam=StackChangedSize/REGISTER_BYTE_SIZE;

    _stprintf(szMsg,
              _T("Error in config file : %d parameters are required. ")
              _T("Stack size should be %d. (Current stack size was %d)"),
              RealNbParam,
              StackChangedSize,
              dwCurrentParamSize);

    _tcscat(szMsg,_T(" Api: "));
    _tcscat(szMsg,pAPIInfo->szAPIName);

    //////////////////////////
    // try to give configuration file responsible of the error
    // look first for monitoring file and next for faking api
    // (but it can be false if the fake dll was loaded before the monitoring file)
    //////////////////////////

    // if monitoring file
    if (pAPIInfo->pMonitoringFileInfos)
    {
        //////////////////////////////////
        // adjust optional parameter list
        //////////////////////////////////

        if (pAPIInfo->MonitoringParamCount<RealNbParam)
        {
            // allocate memory if not enough param
            for (cnt=pAPIInfo->MonitoringParamCount;(cnt<RealNbParam) && (cnt<MAX_PARAM);cnt++)
            {
                // set pAPIInfo->ParamList[cnt] fields to default
                pAPIInfo->ParamList[cnt].dwSizeOfPointedData=0;
                pAPIInfo->ParamList[cnt].dwType=PARAM_UNKNOWN;
            }
        }
        else // RealNbParam < MonitoringParamCount
        {   
            // we have to free too much allocated memory
            FreeOptionalParametersMemory(pAPIInfo,(BYTE)RealNbParam,pAPIInfo->MonitoringParamCount-1);
        }

        // apply changes to struct
        pAPIInfo->MonitoringParamCount=(BYTE)RealNbParam;

        //////////////////////////////////
        // get monitoring file name
        //////////////////////////////////

        TCHAR pszModuleName[MAX_PATH];
        if (GetMonitoringFileName(pAPIInfo->pMonitoringFileInfos,pszModuleName))
        {
            _tcscat(szMsg,_T(" Config File: "));
            _tcscat(szMsg,pszModuleName);
        }
    }
    else if (pAPIInfo->pFakeDllInfos)// fake API
    {
        // get faking dll file name
        TCHAR pszModuleName[MAX_PATH];
        if (GetFakeApiDllName(pAPIInfo->pFakeDllInfos,pszModuleName))
        {
            _tcscat(szMsg,_T(" Fake API Dll: "));
            _tcscat(szMsg,pszModuleName);
        }
    }
    else if (pAPIInfo->PreApiCallChain)
    {
        // get faking dll file name

        // the first item should be responsible of stack size (if no item removed else it will fail)
        if (pAPIInfo->PreApiCallChain->Head)
        {
            TCHAR pszModuleName[MAX_PATH];
            if (GetModuleFileName(((PRE_POST_API_CALL_CHAIN_DATA*)pAPIInfo->PreApiCallChain->Head->ItemData)->OwnerModule,pszModuleName,MAX_PATH))
            {
                _tcscat(szMsg,_T(" Fake API Dll: "));
                _tcscat(szMsg,pszModuleName);
            }
        }
    }
    else if (pAPIInfo->PostApiCallChain)
    {
        // get faking dll file name

        // the first item should be responsible of stack size (if no item removed else it will fail)
        if (pAPIInfo->PostApiCallChain->Head)
        {
            TCHAR pszModuleName[MAX_PATH];
            if (GetModuleFileName(((PRE_POST_API_CALL_CHAIN_DATA*)pAPIInfo->PostApiCallChain->Head->ItemData)->OwnerModule,pszModuleName,MAX_PATH))
            {
                _tcscat(szMsg,_T(" Fake API Dll: "));
                _tcscat(szMsg,pszModuleName);
            }
        }
    }

    _tcscat(szMsg,_T(" Module: "));
    _tcscat(szMsg,pAPIInfo->szModuleName);

    // // as we change parameter size in pAPIInfo struct, only 1 warning will be done, so we can use a messagebox
    // // DynamicMessageBoxInDefaultStation(NULL,szMsg,_T("API Override Warning"),MB_ICONWARNING|MB_OK|MB_TOPMOST);
    // report message instead of messagebox
    CReportMessage::ReportMessage(REPORT_MESSAGE_WARNING,szMsg);
}



//-----------------------------------------------------------------------------
// Name: RemoveAPIOverrideInternalModule
// Object: signal a module removed from APIOverride framework (APIOverride.dll + faking dll)
// Parameters :
//      in: HMODULE hModule : module handle
//      out : 
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL RemoveAPIOverrideInternalModule(HMODULE hModule)
{
    DWORD Cnt;
    for (Cnt=0;Cnt<APIOverrideInternalModulesLimitsIndex;Cnt++)
    {
        if (APIOverrideInternalModulesLimits[Cnt].hModule==hModule)
        {
            // replace current element by last one
            APIOverrideInternalModulesLimits[Cnt]=APIOverrideInternalModulesLimits[APIOverrideInternalModulesLimitsIndex-1];
            // decrease APIOverrideInternalModulesLimitsIndex
            APIOverrideInternalModulesLimitsIndex--;
            return TRUE;
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: AddAPIOverrideInternalModule
// Object: signal a module belonging to APIOverride framework (APIOverride.dll + faking dll)
// Parameters :
//      in: HMODULE hModule : new module handle
//      out : 
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL AddAPIOverrideInternalModule(HMODULE hModule)
{
    if (APIOverrideInternalModulesLimitsIndex>=MAX_APIOVERRIDE_MODULESLIMITS)
        return FALSE;

    BOOL bRet=FALSE;
    MODULEENTRY32 me32 = {0}; 
    HANDLE hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwCurrentProcessID);

    if (hModuleSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 

    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        return FALSE; 
    }
    do 
    { 
        // if we have found module
        if (me32.hModule==hModule)
        {
            // get it's address space limits
            APIOverrideInternalModulesLimits[APIOverrideInternalModulesLimitsIndex].hModule=hModule;
            APIOverrideInternalModulesLimits[APIOverrideInternalModulesLimitsIndex].Start=me32.modBaseAddr;
            APIOverrideInternalModulesLimits[APIOverrideInternalModulesLimitsIndex].End=me32.modBaseAddr+me32.modBaseSize;

            // increase array index
            APIOverrideInternalModulesLimitsIndex++;

            // signal success
            bRet=TRUE;

            // go out of while
            break;
        }
    } 
    while (Module32Next(hModuleSnap, &me32));
    CloseHandle(hModuleSnap);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: IsAPIOverrideInternalCall
// Object: check if call is originated from a module of APIOverride framework (APIOverride.dll + faking dll)
// Parameters :
//      in: 
//          PBYTE Address : address to check
//          PBYTE EbpAtAPIHandler
//      out : 
// Return : TRUE if address comes from APIOverride.dll or a faking dll
//-----------------------------------------------------------------------------
BOOL IsAPIOverrideInternalCall(PBYTE Address,PBYTE EbpAtAPIHandler)
{

    DWORD Cnt;
    // check return address
    for (Cnt=0;Cnt<APIOverrideInternalModulesLimitsIndex;Cnt++)
    {
        if ((APIOverrideInternalModulesLimits[Cnt].Start<=Address)
            &&(Address<=APIOverrideInternalModulesLimits[Cnt].End))
        {
            return TRUE;
        }
    }

    // for each address of stack, check it

    // Why we have to do it ?
    // Imagine you're hooking NtClearEvent.
    // for the first NtClearEvent hooked, we go inside APIHandler function which call ResetEvent.
    // And ResetEvent calls NtClearEvent, which is hooked (has we don't remove hook if first bytes can be executed anywhere)
    // So you see a call with a return address not coming from ApiOverride module but from kernel32...
    // so to avoid an infinite loop, you have to check the full stack, and that's why we add a pseudo return 
    // address with APIHandler address at the begin of the hook : to see a trace of it here !

    // parse call stack
    PBYTE PreviousEbp;
    PBYTE Ebp=0;
    PBYTE RetAddress=0;

    Ebp=EbpAtAPIHandler;
    // NOTICE: IsBadReadPtr can be called until it doesn't call hooked sub functions
    // currently IsBadReadPtr is pure asm code so we don't need to worry
    while (!(IsBadReadPtr(Ebp,REGISTER_BYTE_SIZE)))
    {
        // get previous ebp (call -1 ebp)
        PreviousEbp=*(PBYTE*)(Ebp);

        // if no previous ebp
        if (IsBadReadPtr(PreviousEbp,REGISTER_BYTE_SIZE))
            // stop
            break;

        // return address is at current ebp+REGISTER_BYTE_SIZE
        // so get it
        RetAddress=*(PBYTE*)(Ebp+REGISTER_BYTE_SIZE);

        // stack coherence checking : 
        // function having PreviousEbp has been called by function having EBP (we walk stack backward)
        // so PreviousEbp MUSTE BE greater or equal to EBP
        // and there if it's equal, we can't guess previous function ebp --> we have to stop
        if (Ebp>=PreviousEbp)
            break;

        // update ebp
        Ebp=PreviousEbp;

        for (Cnt=0;Cnt<APIOverrideInternalModulesLimitsIndex;Cnt++)
        {
            if ((APIOverrideInternalModulesLimits[Cnt].Start<=RetAddress)
                &&(RetAddress<=APIOverrideInternalModulesLimits[Cnt].End))
            {
                return TRUE;
            }
        }
    }

    // 5.2 changes : use thread tls informations stored since 5.0 for better check
    CApiOverrideTlsData* pApiOverrideTlsData = (CApiOverrideTlsData*)TlsGetValueApiSubstitution(ApiOverrideTlsIndex);
    if (!pApiOverrideTlsData)
        return FALSE;
    if (!pApiOverrideTlsData->pLinkListTlsData)
        return FALSE;
    CLinkListItem* pItem;
    API_HANDLER_TLS_DATA* pTlsData;
    for (pItem=pApiOverrideTlsData->pLinkListTlsData->Tail;pItem;pItem=pItem->PreviousItem)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)pItem->ItemData;
        
        //if (pTlsData->pAPIInfo==pAPIInfo) // activating this will hide recursive calls : not wanted
        //    return TRUE;

        for (Cnt=0;Cnt<APIOverrideInternalModulesLimitsIndex;Cnt++)
        {
            if ((APIOverrideInternalModulesLimits[Cnt].Start<=pTlsData->OriginalReturnAddress)
                &&(pTlsData->OriginalReturnAddress<=APIOverrideInternalModulesLimits[Cnt].End))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: ParseAPIParameters
// Object: parse parameters and put them to pszLogString
// Parameters :
//      in: API_INFO *pAPIInfo : API hook info
//          REGISTERS* pRegistersBeforeCall : register informations BEFORE function call
//                                            for thiscall and fastcall calling convention
//          PBYTE ParamStackAddress : parameters memory pointer
//      out : LOG_INFOS* pLogInfo : struct containing params values
// Return : 
//-----------------------------------------------------------------------------
void __fastcall ParseAPIParameters(API_INFO* const pAPIInfo, REGISTERS* const pRegistersBeforeCall, LOG_INFOS* const pLogInfo, PBYTE const ParamStackAddress)
{

    PBYTE ParamPointer;
    PBYTE ParamValue;
    BYTE cIndex;
    int iStringSize;
    DWORD dwPointedDataSize;
    DWORD dwDefaultSize;
    DWORD dwType;

    BYTE ParamNeedingSecondPass[MAX_PARAM];
    BYTE NbParamNeedingSecondPass=0;
    BYTE Index;
    BYTE ParameterIndex;
    BYTE IndexOfParamDefiningSize;
    BOOL BadPointer;
    BOOL ParamCanBePassedByRegister=FALSE;
    BOOL CurrentParamCanBePassedByRegister;
    BYTE NumberOfParamPotentialyPassedByRegister=0;
    BYTE NumberOfParamPassedByRegister=0;

#ifdef WIN32
    BYTE NetParamPassedByStack[MAX_PARAM];// LIFO like
    BYTE NetParamPassedByStackIndex=0;
#endif

    pLogInfo->ParmeterParsed=TRUE;
    if (pAPIInfo->MonitoringParamCount==0)
        return;

    switch (pAPIInfo->CallingConvention)
    {
    case CALLING_CONVENTION_FASTCALL:
        NumberOfParamPotentialyPassedByRegister=FASTCALL_NUMBER_OF_PARAM_POTENTIALY_PASSED_BY_REGISTER;
        ParamCanBePassedByRegister=TRUE;
        break;
    case CALLING_CONVENTION_THISCALL:
        NumberOfParamPotentialyPassedByRegister=1;// this
        ParamCanBePassedByRegister=TRUE;
        break;
    default:
        NumberOfParamPotentialyPassedByRegister=0;
        ParamCanBePassedByRegister=FALSE;
        break;
    }

    // argument retrieval
    ParamPointer=ParamStackAddress;

    // loop through parameters to set default value
    for (cIndex = 0; cIndex < pAPIInfo->MonitoringParamCount; cIndex++)
    {
        // free memory if a call to ParseAPIParameters has already done (see break re parsing after param modification)
        if (pLogInfo->ParamLogList[cIndex].pbValue)
        {
            HeapFree(ApiOverrideLogHeap, 0,pLogInfo->ParamLogList[cIndex].pbValue);
            pLogInfo->ParamLogList[cIndex].pbValue=NULL;
        }

        // retrieve type from pAPIInfo
        pLogInfo->ParamLogList[cIndex].dwType=pAPIInfo->ParamList[cIndex].dwType;

        // default param Log fields
        pLogInfo->ParamLogList[cIndex].dwSizeOfData=0;
        pLogInfo->ParamLogList[cIndex].dwSizeOfPointedValue=0;
        pLogInfo->ParamLogList[cIndex].pbValue=0;
        pLogInfo->ParamLogList[cIndex].Value=0;

#ifndef _WIN64
        if (pAPIInfo->HookType==HOOK_TYPE_NET)
        {
            // splitted for optimization (avoid to check calling convention if .Net hook)
            if (pAPIInfo->CallingConvention==CALLING_CONVENTION_FASTCALL)
            {
                // if param can be passed by register
                if ((CSupportedParameters::IsParamPassedByRegisterWithFastcall(pAPIInfo->ParamList[cIndex].dwType & SIMPLE_TYPE_FLAG_MASK))
                        && (pAPIInfo->ParamList[cIndex].dwSizeOfData<=sizeof(PBYTE))
                        && (NumberOfParamPassedByRegister<NumberOfParamPotentialyPassedByRegister)
                    )
                {
                    NumberOfParamPassedByRegister++;
                }
                else // param is passed by stack
                {
                    NetParamPassedByStack[NetParamPassedByStackIndex++]=cIndex;// LIFO like
                }
            }
        }
#endif
    }

#ifndef _WIN64
    // reset NumberOfParamPassedByRegister in case of .NET use
    NumberOfParamPassedByRegister=0;
    if (NetParamPassedByStackIndex)
        NetParamPassedByStackIndex--;// make NetParamPassedByStackIndex point to last existing index, instead of next item to fill
#endif

    // loop through parameters to get value.
    // LOOPS MUST BE INDEPENDENT DUE TO M$ .NET FASTCALL IMPLEMENTATION on x86 (else args>3 will be reset to 0)
    for (cIndex = 0; cIndex < pAPIInfo->MonitoringParamCount; cIndex++)
    {
        /////////////////////////////////////////
        // retrieve value or pointer value from pAPIInfo
        /////////////////////////////////////////

        // if pLogInfo->ParamLogList[cIndex].Value size is enough to contain data
        if (pAPIInfo->ParamList[cIndex].dwSizeOfData<=sizeof(PBYTE))
        {
            if (ParamCanBePassedByRegister)
            {
                if (NumberOfParamPassedByRegister<NumberOfParamPotentialyPassedByRegister)
                {
                    CurrentParamCanBePassedByRegister=CSupportedParameters::IsParamPassedByRegisterWithFastcall(pAPIInfo->ParamList[cIndex].dwType & SIMPLE_TYPE_FLAG_MASK)
                                                        && (pAPIInfo->ParamList[cIndex].dwSizeOfData<=sizeof(PBYTE));
                }
                else
                    CurrentParamCanBePassedByRegister=FALSE;
            }
            else
                CurrentParamCanBePassedByRegister=FALSE;

            if (CurrentParamCanBePassedByRegister)
            {
#ifdef _WIN64
                TO BE IMPLEMENTED
#else
                // the following works for CALLING_CONVENTION_FASTCALL and CALLING_CONVENTION_THISCALL
                switch(NumberOfParamPassedByRegister)
                {
                case 0:
                default:
                    ParamValue=(PBYTE)pRegistersBeforeCall->ecx;
                    break;
                case 1:
                    ParamValue=(PBYTE)pRegistersBeforeCall->edx;
                    break;
                }
                ParameterIndex=cIndex;
                NumberOfParamPassedByRegister++;
#endif
            }
            else
            {
                ParamValue = *((PBYTE*)ParamPointer);
                ParamPointer+=sizeof(PBYTE);
#ifdef _WIN64
                ParameterIndex=cIndex;
#else
                // check microsoft .NET fastcall bad implementation on x86 
                // "Microsoft's CLR implementation uses a calling convention similar to fastcall for managed calls, 
                // but not exactly. The main difference arises when you have to pass more parameters than will fit into 
                // the two registers dedicated for fastcall (ECX, EDX). Normal __fastcall has the remaining parameters pushed 
                // onto the stack right-to-left. But the CLR pushes those remaining parameters left-to-right. 
                // For more details, check out see http://msdn.microsoft.com/netframework/ecma/ ECMA Partition II,chapiter 15.5.6.1"
                // (thanks to "David Broman's CLR Profiling API Blog" http://blogs.msdn.com/davbr/default.aspx )
                if ((pAPIInfo->HookType==HOOK_TYPE_NET) 
                    && (pAPIInfo->CallingConvention==CALLING_CONVENTION_FASTCALL)
                    && (NetParamPassedByStackIndex>=0)
                    )
                    ParameterIndex=NetParamPassedByStack[NetParamPassedByStackIndex--];// LIFO like
                else
                    ParameterIndex=cIndex;
#endif
            }

            // retrieve value or pointer value from pAPIInfo
            pLogInfo->ParamLogList[ParameterIndex].Value=ParamValue;// WARNING : pLogInfo->ParamLogList[ParameterIndex].Value contains 4 bytes value even for byte or short : mask is required
            pLogInfo->ParamLogList[ParameterIndex].dwSizeOfData=pAPIInfo->ParamList[cIndex].dwSizeOfData;

            /////////////////////////////////////////
            // check if parameter is pointer parameter
            /////////////////////////////////////////

            // if parameter pointed value size depends of another parameter
            if (pAPIInfo->ParamList[cIndex].bSizeOfPointedDataDefinedByAnotherParameter)
            {
                // get index of argument containing size
                if (pAPIInfo->ParamList[cIndex].dwSizeOfPointedData>=pAPIInfo->MonitoringParamCount)
                    continue;

                ParamNeedingSecondPass[NbParamNeedingSecondPass]=cIndex;
                NbParamNeedingSecondPass++;
                continue;
            }
            // else

            // check for a standard pointer
            dwPointedDataSize=0;


            ///////////////////////////////////
            // Get pointed data size
            ///////////////////////////////////

            // if parameter type is PARAM_UNKNOWN or has the EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE flag
            // there's no default size of pointed data we can check
            if ( (pLogInfo->ParamLogList[ParameterIndex].dwType==PARAM_UNKNOWN)
                 || (pLogInfo->ParamLogList[ParameterIndex].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
                )
            {
                dwPointedDataSize=pAPIInfo->ParamList[cIndex].dwSizeOfPointedData;
                dwDefaultSize=dwPointedDataSize;
            }
            else
            {
                // retrieve default size of pointed data
                 dwDefaultSize=CSupportedParameters::GetParamPointedSize(pAPIInfo->ParamList[cIndex].dwType & SIMPLE_TYPE_FLAG_MASK);

                // special case for PARAM_PVOID
                if ( (pAPIInfo->ParamList[cIndex].dwType & SIMPLE_TYPE_FLAG_MASK) ==PARAM_PVOID)
                {
                    // if a size is given
                    if (pAPIInfo->ParamList[cIndex].dwSizeOfPointedData)
                    {
                        // don't check it
                        dwPointedDataSize=pAPIInfo->ParamList[cIndex].dwSizeOfPointedData;
                    }
                    else
                        dwPointedDataSize=dwDefaultSize;
                }
                else
                {
                    if (dwDefaultSize!=0)// if item is a pointer
                    {
                        // assume the specified size is enough to support at least one item
                        if (pAPIInfo->ParamList[cIndex].dwSizeOfPointedData<dwDefaultSize)
                            // get default pointed data size
                            dwPointedDataSize=dwDefaultSize;
                        else
                            dwPointedDataSize=pAPIInfo->ParamList[cIndex].dwSizeOfPointedData;
                    }// else data is not a pointer and it's default size is 0
                }
            }

            /////////////////////////////////////////
            // at this point if dwPointedDataSize is not null we have to get pointed data
            // else dwSizeOfData is not null
            // --> get data
            /////////////////////////////////////////

            // next algorithm is quite always the same :
            //  1) check if memory address is valid
            //  2) allocated memory to store parameters
            //  3) copy data in allocated buffer
            BadPointer=FALSE;

            dwType=pAPIInfo->ParamList[cIndex].dwType;
            if (dwType==PARAM_NET_STRING)
            {
                // translate net string into PWSTR

                // .Net string is like
                // Buffer (PBYTE)
                // max size (with \0)  (DWORD ? <- to check on 64bit)
                // length (without \0) (DWORD ? <- to check on 64bit)
                if (!IsBadReadPtr(ParamValue,sizeof(PBYTE)))
                {
                    ParamValue+=sizeof(NET_STRING_HEADER);
                }
                
                dwType=PARAM_PWSTR;
                // update log info struct
                pLogInfo->ParamLogList[ParameterIndex].dwType=PARAM_PWSTR;
            }
            else if (dwType & EXTENDED_TYPE_FLAG_MASK)
            {
                if (dwType & EXTENDED_TYPE_FLAG_NET_SINGLE_DIM_ARRAY )
                {
                    // translate .Net simple array into C single dimension array

                    // keep only simple type flag
                    pLogInfo->ParamLogList[ParameterIndex].dwType=pLogInfo->ParamLogList[ParameterIndex].dwType & SIMPLE_TYPE_FLAG_MASK;
                    
                    if (!IsBadReadPtr(ParamValue,sizeof(PBYTE)))
                    {
                        // net 1 dim array : 
                        // VTBL (PBYTE)
                        // nb items (DWORD ? <- to check on 64bit)
                        // item1,item2, ... itemN
                        ParamValue+=sizeof(PBYTE);
                        if (!IsBadReadPtr(ParamValue,sizeof(DWORD)))
                        {
                            // get pointed data size (default type size * nb param)
                            dwPointedDataSize=dwDefaultSize * ((DWORD)(*ParamValue));

                            // point on first item
                            ParamValue+=sizeof(DWORD);
                        }
                    }
                }
                else if (dwType & EXTENDED_TYPE_FLAG_NET_MULTIPLE_DIM_ARRAY)
                {
                    // net n dim array : 
                    // VTBL (PBYTE)
                    // nb items (DWORD ? <- to check on 64bit)
                    // UpperBound1, UpperBound2, ... (DWORD ? <- to check on 64bit)
                    // LowerBound1, LowerBound2, ... (DWORD ? <- to check on 64bit)
                    // item1,item2, ... itemN
                    if (!IsBadReadPtr(ParamValue,sizeof(DWORD)))
                    {
                        ParamValue+=sizeof(PBYTE);

                        // get pointed data size (default type size * nb param)
                        int NumberOfElements=(int)(*ParamValue);
                        dwPointedDataSize=dwDefaultSize * NumberOfElements;

                        // point on first UpperBound
                        ParamValue+=sizeof(int);

                        // there's no direct access on number of dimension of array,
                        // but we can find it looking for by doing 
                        // nb items=(UpperBound_1-LowerBound_1)*(UpperBound_2-LowerBound_2)*...(UpperBound_N-LowerBound_N)
                        // we have to found N. As we may fall in nb items=(UpperBound_1-UpperBound_3)*(UpperBound_2-UpperBound_4)
                        // we can increase probability by looking for an empty LowerBound too
                        // upper bounds and lower bounds MUST be > 0

                        int NbPotentialResults=0;
                        #define NET_MULTI_DIMENSION_ARRAY_MAX_POTENTIAL_RESULTS 4
                        #define NET_MULTI_DIMENSION_ARRAY_MAX_DIMENSION 10
                        int PossibleDimensions[NET_MULTI_DIMENSION_ARRAY_MAX_POTENTIAL_RESULTS];
                        int NbDim;
                        int Cnt;
                        int* pBounds=(int*)ParamValue;
                        int ComputedNumberOfElements;

                        for (NbPotentialResults=0,NbDim=2; // NbDim>=2  else it should have EXTENDED_TYPE_FLAG_NET_SINGLE_DIM_ARRAY flag
                                (NbPotentialResults<NET_MULTI_DIMENSION_ARRAY_MAX_POTENTIAL_RESULTS)
                             && (NbDim<=NET_MULTI_DIMENSION_ARRAY_MAX_DIMENSION);
                             NbDim++)
                        {
                            ComputedNumberOfElements=1;
                            for (Cnt=0;Cnt<NbDim;Cnt++)
                            {
                                // assume upper bound is >0
                                // and lower bound >=0
                                if (   (pBounds[Cnt]<=0) // upper bound
                                    || (pBounds[Cnt+NbDim]<0) // lower bound
                                    )
                                {
                                    // this is no more dimension, we have passed and of dim
                                    goto NetMultiDimension_StopCheckingNextDimension;
                                }

                                // assume that upper bound >= lower bound
                                if (pBounds[Cnt]<=pBounds[Cnt+NbDim])
                                {
                                    // the dimension value is not the good one
                                    // try another dimension value
                                    goto NetMultiDimension_CheckNextDimension;
                                }

                                // compute number of elements
                                ComputedNumberOfElements*=(pBounds[Cnt]-pBounds[Cnt+NbDim]);
                            }

                            if (ComputedNumberOfElements==NumberOfElements)
                            {
                                PossibleDimensions[NbPotentialResults++]=NbDim;
                                // if lower bound of dimension 0 is NULL, we can stop (else the next upper bound will be 0)
                                if (pBounds[NbDim]==0)
                                    break;
                            }
NetMultiDimension_CheckNextDimension:
                            continue;
NetMultiDimension_StopCheckingNextDimension:
                            break;
                        }

                        if (NbPotentialResults>0)
                        {
                            // the last result should have greatest probability
                            NbPotentialResults--;// translate next result index to last array index
                            NbDim=PossibleDimensions[NbPotentialResults];

                            // point on first element of the array
                            ParamValue+=NbDim*2*sizeof(int);

                            pLogInfo->ParamLogList[ParameterIndex].dwType=pLogInfo->ParamLogList[ParameterIndex].dwType & SIMPLE_TYPE_FLAG_MASK;

                            // from now array will look like a C array
                        }
                    }
                }

                // translate extended type to simple type for more processing
                dwType=dwType & SIMPLE_TYPE_FLAG_MASK;
            }

            switch (dwType)
            {
            case PARAM_PANSI_STRING:
                if (IsBadReadPtr((PVOID)ParamValue, sizeof(ANSI_STRING)))
                    break;
                if (((PANSI_STRING)ParamValue)->Length==0)
                {
                    pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,sizeof(ANSI_STRING)+sizeof(char));
                    if (!pLogInfo->ParamLogList[ParameterIndex].pbValue)
                        break;
                    pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=sizeof(ANSI_STRING)+sizeof(char);
                    memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,sizeof(UNICODE_STRING));
                    memset(&pLogInfo->ParamLogList[ParameterIndex].pbValue[sizeof(ANSI_STRING)],0,sizeof(char));
                    break;
                }
                iStringSize=CSupportedParameters::SecureStrlen(((PANSI_STRING)ParamValue)->Buffer);
                if (iStringSize<0)
                {
                    BadPointer=TRUE;
                    iStringSize=11+1;// strlen("Bad Pointer")+1;
                }

                dwPointedDataSize=sizeof(ANSI_STRING)+(iStringSize+1);
                pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,dwPointedDataSize);
                if (pLogInfo->ParamLogList[ParameterIndex].pbValue)
                {
                    pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=dwPointedDataSize;
                    memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,sizeof(ANSI_STRING));
                    if (BadPointer)
                        memcpy(&pLogInfo->ParamLogList[ParameterIndex].pbValue[sizeof(ANSI_STRING)],"Bad Pointer",(iStringSize+1));
                    else
                        memcpy(&pLogInfo->ParamLogList[ParameterIndex].pbValue[sizeof(ANSI_STRING)],((PANSI_STRING)ParamValue)->Buffer,(iStringSize+1));
                }

                break;
            case PARAM_PSTR:
                iStringSize=CSupportedParameters::SecureStrlen((LPSTR)ParamValue);
                if (iStringSize<0)
                    break;

                dwPointedDataSize=(iStringSize+1);
                // allocate and copy pointed data
                pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,dwPointedDataSize);
                if (pLogInfo->ParamLogList[ParameterIndex].pbValue)
                {
                    pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=dwPointedDataSize;
                    memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,dwPointedDataSize);
                }

                break;

            case PARAM_PUNICODE_STRING:
                if (IsBadReadPtr((PVOID)ParamValue, sizeof(UNICODE_STRING)))
                    break;

                if (((PUNICODE_STRING)ParamValue)->Length==0)
                {
                    pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,sizeof(UNICODE_STRING)+sizeof(wchar_t));
                    if (!pLogInfo->ParamLogList[ParameterIndex].pbValue)
                        break;
                    pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=sizeof(UNICODE_STRING)+sizeof(wchar_t);
                    memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,sizeof(UNICODE_STRING));
                    memset(&pLogInfo->ParamLogList[ParameterIndex].pbValue[sizeof(UNICODE_STRING)],0,sizeof(wchar_t));
                    break;
                }
                iStringSize=CSupportedParameters::SecureWstrlen(((PUNICODE_STRING)ParamValue)->Buffer);
                if (iStringSize<0)
                {
                    BadPointer=TRUE;
                    iStringSize=11+1;// wcslen(L"Bad Pointer")+1;
                }
                dwPointedDataSize=sizeof(UNICODE_STRING)+(iStringSize+1)*sizeof(wchar_t);

                // allocate and copy pointed data
                pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,dwPointedDataSize);
                if (pLogInfo->ParamLogList[ParameterIndex].pbValue)
                {
                    pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=dwPointedDataSize;
                    memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,sizeof(UNICODE_STRING));
                    if (BadPointer)
                        memcpy(&pLogInfo->ParamLogList[ParameterIndex].pbValue[sizeof(UNICODE_STRING)],L"Bad Pointer",(iStringSize+1)*sizeof(wchar_t));
                    else
                        memcpy(&pLogInfo->ParamLogList[ParameterIndex].pbValue[sizeof(UNICODE_STRING)],((PUNICODE_STRING)ParamValue)->Buffer,(iStringSize+1)*sizeof(wchar_t));
                }
                break;

            case PARAM_PWSTR:
            case PARAM_BSTR:
                iStringSize=CSupportedParameters::SecureWstrlen((LPWSTR)ParamValue);
                if (iStringSize<0)
                    break;

                dwPointedDataSize=(iStringSize+1)*sizeof(wchar_t);

                // allocate and copy pointed data
                pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,dwPointedDataSize);
                if (pLogInfo->ParamLogList[ParameterIndex].pbValue)
                {
                    pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=dwPointedDataSize;
                    memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,dwPointedDataSize);
                }

                break;
            case PARAM_PVARIANT:
                if (IsBadReadPtr((PVOID)ParamValue, sizeof(VARIANT)))
                    break;
                CSupportedParameters::GetVariantFromStack(ApiOverrideLogHeap,
                                                (VARIANT*)ParamValue,
                                                dwPointedDataSize,
                                                TRUE,
                                                &pLogInfo->ParamLogList[ParameterIndex]);
                break;
            case PARAM_PSAFEARRAY:
                if (IsBadReadPtr((PVOID)ParamValue, sizeof(SAFEARRAY)))
                    break;
                CSupportedParameters::GetSafeArrayFromStack(ApiOverrideLogHeap,
                                                (SAFEARRAY*)ParamValue,
                                                dwPointedDataSize,
                                                TRUE,
                                                &pLogInfo->ParamLogList[ParameterIndex]);
                break;
            case PARAM_PDISPPARAMS:
                if (IsBadReadPtr((PVOID)ParamValue, sizeof(DISPPARAMS)))
                    break;
                CSupportedParameters::GetDispparamsFromStack(ApiOverrideLogHeap,
                                                (DISPPARAMS*)ParamValue,
                                                dwPointedDataSize,
                                                TRUE,
                                                &pLogInfo->ParamLogList[ParameterIndex]);
                break;
            case PARAM_PEXCEPINFO:
                if (IsBadReadPtr((PVOID)ParamValue, sizeof(EXCEPINFO)))
                    break;
                CSupportedParameters::GetExcepinfoFromStack(ApiOverrideLogHeap,
                                                (EXCEPINFO*)ParamValue,
                                                dwPointedDataSize,
                                                TRUE,
                                                &pLogInfo->ParamLogList[ParameterIndex]);
                break;
            default:
                // allocate and copy pointed data
                if (dwPointedDataSize)
                {
                    if (!IsBadReadPtr((PVOID)ParamValue, dwPointedDataSize))
                    {
                        pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,dwPointedDataSize);
                        if (pLogInfo->ParamLogList[ParameterIndex].pbValue)
                        {
                            pLogInfo->ParamLogList[ParameterIndex].dwSizeOfPointedValue=dwPointedDataSize;
                            memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,dwPointedDataSize);
                        }
                    }
                }
                break;
            }

        }
        else // param value size (pAPIInfo->ParamList[cIndex].dwSizeOfData) 
             //   is more than 4 bytes (struct passed directly through stack)
        {
#ifdef _WIN64
            ParameterIndex=cIndex;
#else
            // check microsoft .NET fastcall bad implementation on x86 
            // "Microsoft's CLR implementation uses a calling convention similar to fastcall for managed calls, 
            // but not exactly. The main difference arises when you have to pass more parameters than will fit into 
            // the two registers dedicated for fastcall (ECX, EDX). Normal __fastcall has the remaining parameters pushed 
            // onto the stack right-to-left. But the CLR pushes those remaining parameters left-to-right. 
            // For more details, check out see http://msdn.microsoft.com/netframework/ecma/ ECMA Partition II,chapiter 15.5.6.1"
            // (thanks to "David Broman's CLR Profiling API Blog" http://blogs.msdn.com/davbr/default.aspx )
            if (pAPIInfo->HookType==HOOK_TYPE_NET)
                ParameterIndex=NetParamPassedByStack[NetParamPassedByStackIndex--];// LIFO like
            else
                ParameterIndex=cIndex;
#endif

            // in this case dwParamValue is a pointer to the stack position of the param
            ParamValue=ParamPointer;

            // adjust marker for next value
            ParamPointer+=pAPIInfo->ParamList[cIndex].dwSizeOfData;

            if (!IsBadReadPtr((PVOID)ParamValue, pAPIInfo->ParamList[cIndex].dwSizeOfData))
            {
                switch (pAPIInfo->ParamList[cIndex].dwType & SIMPLE_TYPE_FLAG_MASK)
                {
                case PARAM_VARIANT:
                    CSupportedParameters::GetVariantFromStack(ApiOverrideLogHeap,(VARIANT*)(ParamValue),pAPIInfo->ParamList[cIndex].dwSizeOfData,FALSE,&pLogInfo->ParamLogList[ParameterIndex]);
                    break;
                case PARAM_SAFEARRAY:
                    CSupportedParameters::GetSafeArrayFromStack(ApiOverrideLogHeap,(SAFEARRAY*)(ParamValue),pAPIInfo->ParamList[cIndex].dwSizeOfData,FALSE,&pLogInfo->ParamLogList[ParameterIndex]);
                    break;
                case PARAM_EXCEPINFO:
                    CSupportedParameters::GetExcepinfoFromStack(ApiOverrideLogHeap,(EXCEPINFO*)(ParamValue),pAPIInfo->ParamList[cIndex].dwSizeOfData,FALSE,&pLogInfo->ParamLogList[ParameterIndex]);
                    break;
                case PARAM_DISPPARAMS:
                    CSupportedParameters::GetDispparamsFromStack(ApiOverrideLogHeap,(DISPPARAMS*)ParamValue,pAPIInfo->ParamList[cIndex].dwSizeOfData,FALSE,&pLogInfo->ParamLogList[ParameterIndex]);
                    break;
                default:
                    pLogInfo->ParamLogList[ParameterIndex].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,pAPIInfo->ParamList[cIndex].dwSizeOfData);
                    if (pLogInfo->ParamLogList[ParameterIndex].pbValue)
                    {
                        pLogInfo->ParamLogList[ParameterIndex].dwSizeOfData=pAPIInfo->ParamList[cIndex].dwSizeOfData;
                        memcpy(pLogInfo->ParamLogList[ParameterIndex].pbValue,(PVOID)ParamValue,pAPIInfo->ParamList[cIndex].dwSizeOfData);
                    }
                    break;
                }
            }
        }
    }

    // second pass for pointed size defined by other args
    for(cIndex=0;cIndex<NbParamNeedingSecondPass;cIndex++)
    {
        // default dwPointedDataSize
        dwPointedDataSize=0;

        Index=ParamNeedingSecondPass[cIndex];
        // get index of argument containing size
        IndexOfParamDefiningSize=(BYTE)pAPIInfo->ParamList[Index].dwSizeOfPointedData;

        // if argument containing size is not a pointed one
        if (pLogInfo->ParamLogList[IndexOfParamDefiningSize].dwSizeOfData)
        {
            // if argument containing size has a size more than DWORD
            if (pLogInfo->ParamLogList[IndexOfParamDefiningSize].dwSizeOfData>sizeof(PBYTE))
                // as x86 are in little endian, we can cast ULONG64 pointer to DWORD pointer to get less significant DWORD
                dwPointedDataSize=(DWORD)*((PBYTE*)pLogInfo->ParamLogList[IndexOfParamDefiningSize].pbValue);
            else
            {
                dwPointedDataSize=(DWORD)pLogInfo->ParamLogList[IndexOfParamDefiningSize].Value;

                // WARNING : pLogInfo->ParamLogList[IndexOfParamDefiningSize].Value contains 4 bytes value even for byte or short
                // mask is required
                switch(CSupportedParameters::GetParamRealSize(pLogInfo->ParamLogList[IndexOfParamDefiningSize].dwType))
                {
                case 1: // byte
                    dwPointedDataSize &= 0xFF;
                    break;
                case 2: // short
                    dwPointedDataSize &= 0xFFFF;
                    break;
                case 3: // ??
                    dwPointedDataSize &= 0xFFFFFF;
                    break;
                }
            }
        }
        else
        {
            if (pLogInfo->ParamLogList[IndexOfParamDefiningSize].dwSizeOfPointedValue<sizeof(PBYTE))
            {
                
                // as we are in little endian, less significant bits come first in DWORD pointer
                // --> we can directly copy memory to get value regardless of its type
                memcpy(&dwPointedDataSize,
                    pLogInfo->ParamLogList[IndexOfParamDefiningSize].pbValue,
                    pLogInfo->ParamLogList[IndexOfParamDefiningSize].dwSizeOfPointedValue);

            }
            else
            {
                // as x86 are in little endian, we can cast ULONG64 pointers to DWORD pointers to get less significant DWORD
                // so even if type is ULONG64 we get its less significant part;
                // and in case of DWORD pointer,  all is ok
                dwPointedDataSize=(DWORD)*((PBYTE*)pLogInfo->ParamLogList[IndexOfParamDefiningSize].pbValue);
            }
        }

        // allocate and copy pointed data
        if (dwPointedDataSize)
        {
            // apply factor to arg defining size
            dwPointedDataSize*=pAPIInfo->ParamList[Index].SizeOfPointedDataDefinedByAnotherParameterFactor;

            // as dwValue has already been field with pointer value, we only need to copy data from it
            if (!IsBadReadPtr((PVOID)pLogInfo->ParamLogList[Index].Value, dwPointedDataSize))
            {
                pLogInfo->ParamLogList[Index].pbValue=(PBYTE)HeapAlloc(ApiOverrideLogHeap, 0,dwPointedDataSize);
                if (pLogInfo->ParamLogList[Index].pbValue)
                {
                    pLogInfo->ParamLogList[Index].dwSizeOfPointedValue=dwPointedDataSize;
                    memcpy(pLogInfo->ParamLogList[Index].pbValue,(PVOID)pLogInfo->ParamLogList[Index].Value,dwPointedDataSize);
                }
            }
        }
    }

}


//-----------------------------------------------------------------------------
// Name: CheckParamLogFilters
// Object: check parameters log filters
// Parameters :
//      in: API_INFO *pAPIInfo : API hook info
//          LOG_INFOS* pLogInfo : struct containing params values
// Return : TRUE if filters match, FALSE else
//-----------------------------------------------------------------------------
BOOL __fastcall CheckParamLogFilters(API_INFO* const pAPIInfo, LOG_INFOS * const pLogInfo)
{
    return CheckParamFilters(pAPIInfo,pLogInfo,TRUE);
}
//-----------------------------------------------------------------------------
// Name: CheckParamBreakFilters
// Object: check parameters break filters
// Parameters :
//      in: API_INFO *pAPIInfo : API hook info
//          LOG_INFOS* pLogInfo : struct containing params values
// Return : TRUE if filters match, FALSE else
//-----------------------------------------------------------------------------
BOOL __fastcall CheckParamBreakFilters(API_INFO* const pAPIInfo, LOG_INFOS* const pLogInfo)
{
    return CheckParamFilters(pAPIInfo,pLogInfo,FALSE);
}

//-----------------------------------------------------------------------------
// Name: CheckParamFilters
// Object: check parameters filters (Log filters or break filters depending bLogFilters)
//          done because code for break and log filtering is 99% the same
// Parameters :
//      in: API_INFO *pAPIInfo : API hook info
//          LOG_INFOS* pLogInfo : struct containing params values
//          BOOL bLogFilters : TRUE to check logging filters, FALSE to check Break filters
// Return : TRUE if filters match, FALSE else
//-----------------------------------------------------------------------------
BOOL __fastcall CheckParamFilters(API_INFO* const pAPIInfo, LOG_INFOS* const pLogInfo,BOOL const bLogFilters)
{
    BYTE cnt;
    CLinkListItem* pItem;
    CLinkList* pList;
    MONITORING_PARAMETER_OPTIONS* pParamOption;
    BOOL bMatch;
    for (cnt=0;cnt<pAPIInfo->MonitoringParamCount;cnt++)
    {
        pList=NULL;

        if (bLogFilters)
        {
            if (!pAPIInfo->ParamList[cnt].pConditionalLogContent)
                continue;
            pList=pAPIInfo->ParamList[cnt].pConditionalLogContent;
        }
        else
        {
            if (!pAPIInfo->ParamList[cnt].pConditionalBreakContent)
                continue;
            pList=pAPIInfo->ParamList[cnt].pConditionalBreakContent;
        }

        if (!pList)
        {
#ifdef _DEBUG
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
#endif
            continue;
        }

        pList->Lock();
        // if no filters defined for parameter
        if (!pList->Head)
        {
            pList->Unlock();
            // check next parameter
            continue;
        }

        // else : parameter should match at least one condition
        bMatch=FALSE;
        for(pItem=pList->Head;pItem;pItem=pItem->NextItem)
        {
            pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
            // if we have to check pointed data
            if (pParamOption->dwPointedValueSize)
            {
                // if log pointed data is smaller than data to check
                // we are sure condition doesn't match, so check next condition
                if (pLogInfo->ParamLogList[cnt].dwSizeOfPointedValue<pParamOption->dwPointedValueSize)
                    continue;
                // check if logged pointer can be read
                if(IsBadReadPtr(pLogInfo->ParamLogList[cnt].pbValue,pParamOption->dwPointedValueSize))
                    continue;

                // if pointed data match filters
                if (memcmp(pLogInfo->ParamLogList[cnt].pbValue,
                            pParamOption->pbPointedValue,
                            pParamOption->dwPointedValueSize)==0)
                // at soon a filter match, check next parameter
                {
                    bMatch=TRUE;
                    break;// go out of for
                }

            }
            // if we have to check value size
            else if (pParamOption->dwValueSize>REGISTER_BYTE_SIZE)
            {
                // if log pointed data is smaller than data to check
                // we are sure condition doesn't match, so check next condition
                if (pLogInfo->ParamLogList[cnt].dwSizeOfData<pParamOption->dwValueSize)
                    continue;
                // check if logged pointer can be read
                if(IsBadReadPtr(pLogInfo->ParamLogList[cnt].pbValue,pParamOption->dwValueSize))
                    continue;

                // if pointed data match filters
                if (memcmp(pLogInfo->ParamLogList[cnt].pbValue,
                            pParamOption->pbPointedValue,
                            pParamOption->dwValueSize)==0)
                // at soon a filter match, check next parameter
                {
                    bMatch=TRUE;
                    break;// go out of for
                }
            }
            else // we check value
            {
                if (pLogInfo->ParamLogList[cnt].Value==pParamOption->Value)
                // at soon a filter match, check next parameter
                {
                    bMatch=TRUE;
                    break;// go out of for
                }
            }
        }
        pList->Unlock();

        // if no one condition match
        if (!bMatch)
            return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: DoFunctionCallFail
// Object: check failure conditions depending the return of the function
// Parameters :
//      in: API_INFO *pAPIInfo : API hook info
//          PBYTE Return : return of the func
//          double FloatingReturn : float/double return of the func
//          DWORD dwLastErrorCode : LastErrorCode
// Return : TRUE if returned value match failure condition, FALSE if returned value don't match
//-----------------------------------------------------------------------------
BOOL __fastcall DoFunctionFail(API_INFO* const pAPIInfo,PBYTE const Return,double const FloatingReturn,DWORD const dwLastErrorCode)
{

    // check last error failure
    BOOL bLastErrorCodeFailure;
    BOOL bLastErrorCodeFailureSignificant;
    if (pAPIInfo->LogBreakWay.FailureIfLastErrorValue)
    {
        bLastErrorCodeFailure=(dwLastErrorCode==pAPIInfo->FailureLastErrorValue);
        bLastErrorCodeFailureSignificant=TRUE;
    }
    else if (pAPIInfo->LogBreakWay.FailureIfNotLastErrorValue)
    {
        bLastErrorCodeFailure=(dwLastErrorCode!=pAPIInfo->FailureLastErrorValue);
        bLastErrorCodeFailureSignificant=TRUE;
    }
    else if (pAPIInfo->LogBreakWay.FailureIfLastErrorValueLess)
    {
        bLastErrorCodeFailure=(dwLastErrorCode<pAPIInfo->FailureLastErrorValue);
        bLastErrorCodeFailureSignificant=TRUE;
    }
    else if (pAPIInfo->LogBreakWay.FailureIfLastErrorValueUpper)
    {
        bLastErrorCodeFailure=(dwLastErrorCode>pAPIInfo->FailureLastErrorValue);
        bLastErrorCodeFailureSignificant=TRUE;
    }
    else
    {
        bLastErrorCodeFailure=FALSE;
        bLastErrorCodeFailureSignificant=FALSE;
    }

    if (pAPIInfo->LogBreakWay.FailureIfNullRet)
    {
        if (Return==0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    // else if as only one failure param is allowed
    else if (pAPIInfo->LogBreakWay.FailureIfNotNullRet)
    {
        if (Return!=0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfRetValue)
    {
        if (Return==pAPIInfo->FailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfNotRetValue)
    {
        if (Return!=pAPIInfo->FailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfNegativeRetValue)
    {
        if (((int)Return)<0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfPositiveRetValue)
    {
        if (((int)Return)>0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }

    else if (pAPIInfo->LogBreakWay.FailureIfNullFloatingRet)
    {
        if (FloatingReturn==0.0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfNotNullFloatingRet)
    {
        if (FloatingReturn!=0.0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfFloatingRetValue)
    {
        if (FloatingReturn==pAPIInfo->FloatingFailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfNotFloatingRetValue)
    {
        if (FloatingReturn!=pAPIInfo->FloatingFailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfFloatingNegativeRetValue)
    {
        if (FloatingReturn<0.0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfFloatingPositiveRetValue)
    {
        if (FloatingReturn>0.0)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }

    else if (pAPIInfo->LogBreakWay.FailureIfSignedRetLess)
    {
        if (((int)Return)<((int)pAPIInfo->FailureValue))
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfSignedRetUpper)
    {
        if (((int)Return)>((int)pAPIInfo->FailureValue))
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfUnsignedRetLess)
    {
        if (Return<pAPIInfo->FailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfUnsignedRetUpper)
    {
        if (Return>pAPIInfo->FailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfFloatingRetLess)
    {
        if (FloatingReturn<pAPIInfo->FloatingFailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }
    else if (pAPIInfo->LogBreakWay.FailureIfFloatingRetUpper)
    {
        if (FloatingReturn>pAPIInfo->FloatingFailureValue)
        {
            if (bLastErrorCodeFailureSignificant)
                return bLastErrorCodeFailure;
            // else
            return TRUE;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: IsCOMHookDefinition
// Object: check is hook definition is for a com hook definition
// Parameters :
//      in: TCHAR* pszModuleDefinition : hook definition or module hook definition
// Return : TRUE hook definition is for a COM hook definition
//-----------------------------------------------------------------------------
BOOL IsCOMHookDefinition(TCHAR* pszModuleDefinition)
{
    // check if module definition begins with COM_DEFINITION_PREFIX
    return (_tcsnicmp(pszModuleDefinition,COM_DEFINITION_PREFIX,_tcslen(COM_DEFINITION_PREFIX))==0);
}

//-----------------------------------------------------------------------------
// Name: IsNetHookDefinition
// Object: check is hook definition is for a NET hook definition
// Parameters :
//      in: TCHAR* pszModuleDefinition : hook definition or module hook definition
// Return : TRUE hook definition is for a NET hook definition
//-----------------------------------------------------------------------------
BOOL IsNetHookDefinition(TCHAR* pszModuleDefinition)
{
    // check if module definition begins with DLL_OR_EXE_NET_PREFIX
    return (_tcsnicmp(pszModuleDefinition,DLL_OR_EXE_NET_PREFIX,_tcslen(DLL_OR_EXE_NET_PREFIX))==0);
}

//-----------------------------------------------------------------------------
// Name: ReportBadHookChainBadCallingConvention
// Object: check is hook definition is for a com hook definition
// Parameters :
//      in: API_INFO *pAPIInfo : pointer to api info
//          PBYTE PrePostHookCallBack : pointer to pre/post hook callback function
//          BOOL bPreHook : TRUE for Pre hook, FALSE for Post hook
// Return : TRUE hook definition is for a COM hook definition
//-----------------------------------------------------------------------------
void ReportBadHookChainBadCallingConvention(API_INFO *pAPIInfo,PBYTE PrePostHookCallBack,BOOL bPreHook)
{
    TCHAR pszMsg[MAX_PATH];
    TCHAR pszPrePosHook[20];
    if (bPreHook)
        _tcscpy(pszPrePosHook,_T("pre hook"));
    else
        _tcscpy(pszPrePosHook,_T("post hook"));

    _sntprintf(pszMsg,MAX_PATH,_T("Bad calling convention for %s %p of %s. Calling convention must be stdcall."),
                pszPrePosHook,PrePostHookCallBack,pAPIInfo->szAPIName);
    CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
}