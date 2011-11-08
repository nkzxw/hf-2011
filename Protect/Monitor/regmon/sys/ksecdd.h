//======================================================================
// 
// Ksecdd.h
//
// Copyright (C) 2000 Mark Russinovich 
//
// Definitions pulled from the Win2K IFS Kit ntifs.h.
//
//======================================================================

#define SEC_ENTRY   __stdcall

typedef LONG SECURITY_STATUS;
typedef LONG HRESULT;
typedef UNICODE_STRING SECURITY_STRING, *PSECURITY_STRING;
typedef struct _SECURITY_USER_DATA {
    SECURITY_STRING UserName;           // User name
    SECURITY_STRING LogonDomainName;    // Domain the user logged on to
    SECURITY_STRING LogonServer;        // Server that logged the user on
    PSID            pSid;               // SID of user
} SECURITY_USER_DATA, *PSECURITY_USER_DATA;

typedef SECURITY_USER_DATA SecurityUserData, * PSecurityUserData;

#define UNDERSTANDS_LONG_NAMES  1
#define NO_LONG_NAMES           2


HRESULT SEC_ENTRY
GetSecurityUserInfo(
    IN PLUID LogonId,
    IN ULONG Flags,
    OUT PSecurityUserData * UserInformation
    );


SECURITY_STATUS SEC_ENTRY
MapSecurityError( SECURITY_STATUS hrValue );


NTSTATUS
NTAPI
LsaFreeReturnBuffer (
    IN PVOID Buffer
    );





