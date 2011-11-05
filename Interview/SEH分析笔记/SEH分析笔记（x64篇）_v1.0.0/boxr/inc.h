// code by Boxcounter

#include <assert.h>
#include <crtdbg.h>


#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))
#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
#define __WFUNCTION__ WIDEN(__FUNCTION__)
#define __WTIME__ WIDEN(__TIME__)

// ÄÚ´æ¹ÜÀí
#define MemAlloc(cbSize) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (cbSize))
#define MemFree(pMem) HeapFree(GetProcessHeap(), 0, pMem); pMem = NULL

// ¶ÏÑÔºê
#ifdef _DEBUG
#define ASSERT(x)                       assert((x));
#define ASSERT_RETURN(x)                assert((x));if (!(x)) {return;}
#define ASSERT_RETURN_VAL(x, RtnVal)    assert((x));if (!(x)) {return (RtnVal);}
#define ASSERT_BREAK(x)                 assert((x));if (!(x)) {break;}
#define ASSERT_CONTINUE(x)              assert((x));if (!(x)) {continue;}
#define ASSERT_GOTO(x, GOTOADDRESS)     assert((x));if (!(x)) {goto GOTOADDRESS;}
#else
#define ASSERT(x)
#define ASSERT_RETURN(x)                if (!(x)) {return;}
#define ASSERT_RETURN_VAL(x, RtnVal)    if (!(x)) {return (RtnVal);}
#define ASSERT_BREAK(x)                 if (!(x)) {break;}
#define ASSERT_CONTINUE(x)              if (!(x)) {continue;}
#define ASSERT_GOTO(x, GOTOADDRESS)     if (!(x)) {goto GOTOADDRESS;}
#endif


