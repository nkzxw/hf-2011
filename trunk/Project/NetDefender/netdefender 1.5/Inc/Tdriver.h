

#if !defined(TDRIVER_CLASS)
#define TDRIVER_CLASS

#pragma once
#include "winsvc.h"

//ERROR CODES
#define DRV_SUCCESS						(DWORD) 0		//ALL OK
#define DRV_ERROR_SCM					(DWORD) - 1		//ERROR at Open de Service Manager
#define DRV_ERROR_SERVICE				(DWORD) - 2		//ERROR at create service
#define DRV_ERROR_MEMORY				(DWORD) - 3		//ERROR	at reserving memory
#define DRV_ERROR_INVALID_PATH_OR_FILE	(DWORD) - 4		//ERROR, the path gived is not valid
#define DRV_ERROR_INVALID_HANDLE		(DWORD) - 5		//ERROR, driver handle is not valid
#define DRV_ERROR_STARTING				(DWORD) - 6		//ERROR at starting the driver
#define DRV_ERROR_STOPPING				(DWORD) - 7		//ERROR at stopping the driver
#define DRV_ERROR_REMOVING				(DWORD) - 8		//ERROR at removing the driver "service"
#define DRV_ERROR_IO					(DWORD) - 9		//ERROR at io operation
#define DRV_ERROR_NO_INITIALIZED		(DWORD) - 10	//ERROR, class not initialized
#define DRV_ERROR_ALREADY_INITIALIZED	(DWORD) - 11	//ERROR, class already initialized
#define DRV_ERROR_NULL_POINTER			(DWORD) - 12	//ERROR, pointer introduced is NULL
#define DRV_ERROR_UNKNOWN				(DWORD) - 13	//UNKNOWN ERROR
class TDriver
{
public:
	/*********************************************
	 * constructor
	 *
	  ********************************************/
	TDriver(void);			//constructor

	/*********************************************
	 *destructor
	 ********************************************/
	~		TDriver(void);	//destructor

	/*********************************************
	 * functions to initialized the driver variables
	 *
	 * @param name of the driver 
	 * @param path of the driver 
	 * @param dosName 
	 * @return DWORD 
	 ********************************************/
	DWORD	InitDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName = NULL);

	/*********************************************
	 * functions to initialized the driver variables
	 *
	 * @param name of the driver 
	 ********************************************/
	DWORD	InitDriver(LPCTSTR path);

	/*********************************************
	 * functions to load and unload drivers. If start = TRUE, the driver will be started.
	 *
	 * @param If start = TRUE, the driver will be started. 
	 * @return DWORD 
	 ********************************************/
	DWORD	LoadDriver(BOOL start = TRUE);

	/*********************************************
	 * functions to load and unload drivers. If start = TRUE, the driver will be started.
	 *
	 * @param name of the driver  
	 * @param path of the driver 
	 * @param dosName 
	 * @param If start = TRUE, the driver will be started. 
	 * @return DWORD 
	 ********************************************/
	DWORD	LoadDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName = NULL, BOOL start = TRUE);

	/*********************************************
	 * LoadDriver:
	 *
	 * @param path of the driver 
	 * @param If start = TRUE, the driver will be started. 
	 * @return DWORD 
	 ********************************************/
	DWORD	LoadDriver(LPCTSTR path, BOOL start = TRUE);

	//if forceClearData == TRUE, will remove variables although we cant remove driver "service"
	DWORD	UnloadDriver(BOOL forceClearData = FALSE);

	//functions to start and stop, driver "service"
	/*********************************************
	 *functions to start  driver "service"
	 *
	 * @param  
	 * @return DWORD 
	 ********************************************/
	DWORD	StartDriver(void);

	/*********************************************
	 * functions to stop driver "service"
	 *
	 * @param  
	 * @return DWORD 
	 ********************************************/
	DWORD	StopDriver(void);

	//if true, the driver havent been removed at finish
	/*********************************************
	 * if true, the driver havent been removed at finish
	 *
	 * @param value 
	 * @return void 
	 ********************************************/
	void	SetRemovable(BOOL value);

	//funtion to return class status
	/*********************************************
	 * funtion to return class status <br>
	 *If the driver is successfully initialized
	 *
	 * @return BOOL 
	 ********************************************/
	BOOL	IsInitialized();

	/*********************************************
	 * funtion to return class status <br>
	 *If the driver is successfully started
	 *
	 * @return BOOL 
	 ********************************************/
	BOOL	IsStarted();

	/*********************************************
	 * funtion to return class status <br>
	 *If the driver is successfully loaded
	 *
	 * @return BOOL 
	 ********************************************/
	BOOL	IsLoaded();

	/*********************************************
	 * GetDriverHandle: function to get driver handle
	 *
	 * @param  
	 * @return HANDLE 
	 ********************************************/
	HANDLE	GetDriverHandle(void);

	//funtions to make IO operation with driver
	/*********************************************
	 * WriteIo: funtions to make IO operation with driver
	 * It will add rule to Driver
	 *
	 * @param code 
	 * @param buffer 
	 * @param count 
	 * @return DWORD 
	 ********************************************/
	DWORD	WriteIo(DWORD code, PVOID buffer, DWORD count);

	/*********************************************
	 * ReadIo: funtions to make IO operation with driver
	 * It will read rule from Driver
	 *
	 * @param code 
	 * @param buffer 
	 * @param count 
	 * @return DWORD 
	 ********************************************/
	DWORD	ReadIo(DWORD code, PVOID buffer, DWORD count);
	DWORD	RawIo(DWORD code, PVOID inBuffer, DWORD inCount, PVOID outBuffer, DWORD outCount);
private:
	HANDLE	driverHandle;	//driver handle
	CString	m_strDriverName;		//driver name
	CString	m_strDriverPath;		//driver disk path
	CString	m_strDriverDosName;	//driver's dos name, to link with it
	BOOL	m_binitialized;	//variables to store the status of this class
	BOOL	m_bStarted;
	BOOL	m_bLoaded;
	BOOL	m_bRemovable;

	//get a handle to the driver
	DWORD	OpenDevice(void);
};
#endif
