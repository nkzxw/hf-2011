
#ifndef __NETWALLAPI_H__
#define __NETWALLAPI_H__

#include <objbase.h>
#include <initguid.h>

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NETWALLDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NETWALLDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef NETWALLDLL_EXPORTS
#define NETWALLDLL_API __declspec(dllexport)
#else
#define NETWALLDLL_API __declspec(dllimport)
#endif

//ERROR CODES
#define DRV_SUCCESS						 (DWORD)0		//ALL OK

#define DRV_ERROR_SCM					 (DWORD)-1		//ERROR at Open de Service Manager
#define DRV_ERROR_SERVICE				 (DWORD)-2		//ERROR at create service
#define DRV_ERROR_MEMORY				 (DWORD)-3		//ERROR	at reserving memory
#define DRV_ERROR_INVALID_PATH_OR_FILE	 (DWORD)-4		//ERROR, the path gived is not valid
#define DRV_ERROR_INVALID_HANDLE		 (DWORD)-5		//ERROR, driver handle is not valid
#define DRV_ERROR_STARTING				 (DWORD)-6		//ERROR at starting the driver
#define DRV_ERROR_STOPPING				 (DWORD)-7		//ERROR at stopping the driver
#define DRV_ERROR_REMOVING				 (DWORD)-8		//ERROR at removing the driver "service"
#define DRV_ERROR_IO					 (DWORD)-9		//ERROR at io operation
#define DRV_ERROR_NO_INITIALIZED		 (DWORD)-10		//ERROR, class not initialized
#define DRV_ERROR_ALREADY_INITIALIZED	 (DWORD)-11		//ERROR, class already initialized
#define DRV_ERROR_NULL_POINTER			 (DWORD)-12		//ERROR, pointer introduced is NULL
#define DRV_ERROR_UNKNOWN				 (DWORD)-13		//UNKNOWN ERROR



class NETWALLDLL_API CDriver
{
public:
    CDriver(void);		    //constructor
    virtual ~CDriver(void);	//destructor

private:
    //functions to initialized the driver variables
    DWORD InitDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName = NULL);
    DWORD InitDriver(LPCTSTR path);

    //functions to load and unload drivers. If start = TRUE, the driver will be started.
    DWORD LoadDriver(BOOL start = TRUE);

public:    
    DWORD LoadDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName = NULL, BOOL start = TRUE);
    DWORD LoadDriver(LPCTSTR path, BOOL start = TRUE);
    
    //if forceClearData == TRUE, will remove variables although we cant remove driver "service"
    DWORD UnloadDriver(BOOL forceClearData = FALSE);
    
    //functions to start and stop, driver "service"
    DWORD StartDriver(void);
    DWORD StopDriver(void);
    
    //if true, the driver havent been removed at finish
    void SetRemovable(BOOL value);
        
    //funtion to return class status
    BOOL IsInitialized();
    BOOL IsStarted();
    BOOL IsLoaded();
        
    //function to get driver handle
    HANDLE GetDriverHandle(void);
    
    //funtions to make IO operation with driver
    DWORD WriteIo(DWORD code, PVOID buffer, DWORD count);
    DWORD ReadIo(DWORD code, PVOID buffer, DWORD count);
    DWORD RawIo(DWORD code, PVOID inBuffer, DWORD inCount, PVOID outBuffer, DWORD outCount);
    
    //
public:
    BOOL LoadDeviceDriver(LPCTSTR lpszDriverName, LPCTSTR lpszDriverFromPath, LPTSTR lpszError);
    BOOL OpenDevice(DWORD dwInst, HANDLE * lphDevice, LPTSTR lpszError, IN GUID * pGuid = NULL);
    BOOL UnloadDeviceDriver(LPCTSTR lpszDriverName, LPTSTR lpszError);
    // BOOL TestLoadDriver();

private:
    BOOL CreateDriver(LPCTSTR lpszDriverName, LPCTSTR lpszFullDriver, LPTSTR lpszError);
    BOOL StartDriver(LPCTSTR lpszDriverName, LPTSTR lpszError);    
    int  FindInMultiSz(LPTSTR MultiSz, int MultiSzLen, LPCTSTR Match);

    
private:
    HANDLE driverHandle;	//driver handle
    
    LPTSTR driverName;		//driver name
    LPTSTR driverPath;		//driver disk path
    LPTSTR driverDosName;	//driver's dos name, to link with it
    
    BOOL initialized;	//variables to store the status of this class
    BOOL started;
    BOOL loaded;
    BOOL removable;
    
    //get a handle to the driver
    DWORD OpenDevice(void);	
};

#endif