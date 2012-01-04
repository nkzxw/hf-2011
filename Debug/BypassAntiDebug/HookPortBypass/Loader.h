#ifndef __LOADER__
#define __LOADER__
#include <wdm.h>
#include <ntddk.h>

#include "pe.h"
////////////////////////////////////////////////////////////////////////////////
#define kmalloc(X)	ExAllocatePoolWithTag(NonPagedPool, (X), 'lidd')
#define kfree(x)		ExFreePool((x))



typedef struct _DRIVER_INFO 
{
	DWORD_PTR   Unknown1;
	PVOID	BaseAddress;
	ULONG	Size;
	ULONG   Unknown3;
	ULONG	Index;
	ULONG   Unknown4;
	CHAR	PathName[0x104];
}DRIVER_INFO,*PDRIVER_INFO;


typedef struct _SYSTEM_INFO_DRIVERS 
{
	ULONG	NumberOfDrivers;
	DWORD_PTR	Reserved;
	DRIVER_INFO Drivers[0x100];
}SYSTEM_INFO_DRIVERS,*PSYSTEM_INFO_DRIVERS;
////////////////////////////////////////////////////////////////////////////////
#pragma  pack( push, 1)

class CKl_Object 
{
public:
    long            m_nRefCount;
	
    CKl_Object();
    virtual ~CKl_Object();
    
    void* operator new(size_t size);
    void operator delete(void* p); 
    
    virtual unsigned long   Ref();
    virtual unsigned long   Deref();
};

#pragma pack ( pop )
////////////////////////////////////////////////////////////////////////////////
#define  CKl_HookModule CKl_HookModule1
class CKl_HookModule1 :public CKl_Object
{
public:
    char*       m_base;		
    ULONG       m_size;		
	
    PVOID       GetBase         ( char* ModuleName  );		
    ULONG       GetModuleSize   ( PVOID ModuleBase  );		
    PVOID       GetExportByName ( char* fName       );		
	
   
    bool        ContainPtr      ( void* ptr );
    bool        isPE();
    
    CKl_HookModule(PVOID    base = NULL);
    CKl_HookModule(PCHAR    ModuleName);
	
  //  virtual ~CKl_HookModule();
};
#define  CKl_ModLoader CKl_ModLoader1
class CKl_ModLoader1  :public CKl_Object
{
public:
    char*       m_base;
    char*       m_DriverBase;
	
    void        PrepareForExec(PVOID  *StartEntry); // подготавливает модуль для исполнения
	
    CKl_ModLoader(PVOID ModuleLoadBase = NULL);
  //  virtual ~CKl_ModLoader();
};

#endif