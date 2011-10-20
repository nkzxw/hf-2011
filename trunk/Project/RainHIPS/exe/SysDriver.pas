unit SysDriver;

interface

uses windows, winsvc; // jwawinsvc;

Type
  TSysDriver = class(TObject)
  private
    HomeDir, DriverDir, DriverName, DEVICE_NAME_STRING,
    DriverPath      : string; // the whole thing
    hSCMan          : SC_HANDLE; // Service Control Manager
    hDevice         : SC_HANDLE; // Handle for device
  public
    HaveLoad        : Boolean;
    BaseControlCode : DWORD;
    constructor Create(DeviceName: string);

    //---------------------------------------
    // Interact with Service Control Manager
    //---------------------------------------
    function OpenSCM: DWORD;
    function CloseSCM: DWORD;

    //---------------------------------------
    // Install/Start/Stop/Remove driver
    //---------------------------------------
    function Install(newdriverpath: string): DWORD; { use '' for default }
    function Start:   DWORD;
    function Stop:    DWORD;
    function Remove:  DWORD;

    //--------------------------------
    // Device Open/Close
    //--------------------------------
    function DeviceOpen: DWORD;   // get a valid hDevice
    function DeviceClose: DWORD;

    //--------------------------------
    function IOControl(Cmd: DWORD; inBuf: Pointer; inSize: DWORD;
      outbuf: Pointer; var outSize: DWORD): Boolean;
    //--------------------------------
    function ErrorLookup(ErrorNum: DWORD): string;
  end;

//-------------------------------------------
implementation
//-------------------------------------------
uses sysutils;

Const // from ntddk
// Service Types (Bit Mask)
  SERVICE_KERNEL_DRIVER        =  $00000001;
  SERVICE_FILE_SYSTEM_DRIVER   =  $00000002;
  SERVICE_ADAPTER              =  $00000004;
  SERVICE_RECOGNIZER_DRIVER    =  $00000008;

  SERVICE_DRIVER               =  SERVICE_KERNEL_DRIVER OR
                                  SERVICE_FILE_SYSTEM_DRIVER OR
                                  SERVICE_RECOGNIZER_DRIVER;

  SERVICE_WIN32_OWN_PROCESS    =  $00000010;
  SERVICE_WIN32_SHARE_PROCESS  =  $00000020;
  SERVICE_WIN32                =  SERVICE_WIN32_OWN_PROCESS OR
                                  SERVICE_WIN32_SHARE_PROCESS;

  SERVICE_INTERACTIVE_PROCESS  =  $00000100;

  SERVICE_TYPE_ALL             =  SERVICE_WIN32   OR
                                  SERVICE_ADAPTER OR
                                  SERVICE_DRIVER  OR
                                  SERVICE_INTERACTIVE_PROCESS;
// Start Type
  SERVICE_BOOT_START           =  $00000000;
  SERVICE_SYSTEM_START         =  $00000001;
  SERVICE_AUTO_START           =  $00000002;
  SERVICE_DEMAND_START         =  $00000003;
  SERVICE_DISABLED             =  $00000004;

// Error control type
  SERVICE_ERROR_IGNORE         =  $00000000;
  SERVICE_ERROR_NORMAL         =  $00000001;
  SERVICE_ERROR_SEVERE         =  $00000002;
  SERVICE_ERROR_CRITICAL       =  $00000003;

Type
  TErrorMsg = record
    Num: integer;
    Msg: string;
  end;

Const
  ErrorMsgCt = 30;
  ERROR_SCM_CANT_CONNECT = 9998;
  ERROR_NO_DEVICE_HANDLE = 9997;
  ERROR_GW_BUFFER_TOO_SMALL = 9997;
  ERROR_UNEXPECTED = 9999;

  ErrorMsgs: array[1..ErrorMsgCt] of TErrorMsg = (
    (Num: ERROR_SUCCESS                   ; Msg: 'Operation was successful'),
    (Num: ERROR_INVALID_FUNCTION          ; Msg: 'Invalid Function'),
    (Num: ERROR_ACCESS_DENIED             ; Msg: 'Access denied'),
    (Num: ERROR_CIRCULAR_DEPENDENCY       ; Msg: 'Circular dependency'),
    (Num: ERROR_DATABASE_DOES_NOT_EXIST   ; Msg: 'Database doesn''t exist'),
    (Num: ERROR_DEPENDENT_SERVICES_RUNNING; Msg: 'Dependent services running'),
    (Num: ERROR_DUP_NAME                  ; Msg: 'Display name already exists'),
    (Num: ERROR_INVALID_HANDLE            ; Msg: 'Invalid handle'),
    (Num: ERROR_INVALID_NAME              ; Msg: 'Invalid service name'),
    (Num: ERROR_INVALID_PARAMETER         ; Msg: 'Invalid Parameter'),
    (Num: ERROR_INVALID_SERVICE_ACCOUNT   ; Msg: 'User account doesn''t exist'),
    (Num: ERROR_INVALID_SERVICE_CONTROL   ; Msg: 'Invalid service control code'),
    (Num: ERROR_PATH_NOT_FOUND            ; Msg: 'Path not found'),
    (Num: ERROR_SERVICE_ALREADY_RUNNING   ; Msg: 'Service already running'),
    (Num: ERROR_SERVICE_CANNOT_ACCEPT_CTRL; Msg: 'Service can''t accept control'),
    (Num: ERROR_SERVICE_DATABASE_LOCKED   ; Msg: 'The database is locked'),
    (Num: ERROR_SERVICE_DEPENDENCY_DELETED; Msg: 'Depends on nonexistant service'),
    (Num: ERROR_SERVICE_DEPENDENCY_FAIL   ; Msg: 'Depends on service that failed'),
    (Num: ERROR_SERVICE_DISABLED          ; Msg: 'Service has been disabled'),
    (Num: ERROR_SERVICE_DOES_NOT_EXIST    ; Msg: 'Service doesn''t exist'),
    (Num: ERROR_SERVICE_EXISTS            ; Msg: 'Service already exists'),
    (Num: ERROR_SERVICE_LOGON_FAILED      ; Msg: 'Service couldn''t be logged on'),
    (Num: ERROR_SERVICE_MARKED_FOR_DELETE ; Msg: 'Service marked for deletion'),
    (Num: ERROR_SERVICE_NO_THREAD         ; Msg: 'Couldn''t create thread'),
    (Num: ERROR_SERVICE_NOT_ACTIVE        ; Msg: 'Service hasn''t been started'),
    (Num: ERROR_SERVICE_REQUEST_TIMEOUT   ; Msg: 'Service timed out'),
    (Num: ERROR_GW_BUFFER_TOO_SMALL       ; Msg: 'Buffer too small'),
    (Num: ERROR_NO_DEVICE_HANDLE          ; Msg: 'No device handle'),
    (Num: ERROR_SCM_CANT_CONNECT          ; Msg: 'Can''t connect to Service Control Manager'),
    (Num: ERROR_UNEXPECTED                ; Msg: 'An unexpected error occured')
  );

//-----------------------------------------
function TSysDriver.ErrorLookup(ErrorNum: DWORD): string;
//-----------------------------------------
Var
  S: string;
  N: integer;
label foundit;
Begin
  If Error <> ERROR_SUCCESS then
    S := 'Error: ' + IntToStr(ErrorNum) + ': ';
    
  For N := 1 to ErrorMsgCt do
  Begin
    if ErrorNum = ErrorMsgs[N].Num then
    Begin
      goto foundit;
    end;
  end;
foundit:
  If N > ErrorMsgCt then N := ErrorMsgCt;
  S := S + ErrorMsgs[N].Msg;
  result := S;
end;

//----------------------------------------------------------
// IOCTL codes
//----------------------------------------------------------
function CTL_CODE(DeviceType: integer; func: integer; meth: integer; access: integer): DWORD;
Begin
  result := (DeviceType shl 16) or (Access shl 14) or (func shl 2) or (meth);
end;

Const
  // Buffering method for user-mode app talking to drive
  METHOD_BUFFERED    = 0;
  METHOD_IN_DIRECT   = 1;
  METHOD_OUT_DIRECT  = 2;
  METHOD_NEITHER     = 3;

  // Define the access allowed
  FILE_ANY_ACCESS    = 0;
  FILE_READ_ACCESS   = 1;     // file & pipe
  FILE_WRITE_ACCESS  = 2;     // file & pipe


//-----------------------------------------
constructor TSysDriver.Create(DeviceName: string);
//-----------------------------------------
Begin
  hSCMan  := 0;
  hDevice := INVALID_HANDLE_VALUE;
  HomeDir := ExtractFilePath(GetModuleName(HInstance));
  DEVICE_NAME_STRING := DeviceName;
  DriverName  := DEVICE_NAME_STRING;
  HaveLoad    :=False;
    // default driver name needed by stop/remove if install wasn't executed
    // this run (ie: driver already installed
end;

//-------------------------------------------
function TSysDriver.OpenSCM:  DWORD;
//-------------------------------------------
Begin
  result := ERROR_SUCCESS;
  hSCMan := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if hSCMan = 0 then result := ERROR_SCM_CANT_CONNECT;
end;

//-------------------------------------------
function TSysDriver.CloseSCM:  DWORD;
//-------------------------------------------
Begin
  result := ERROR_SUCCESS;
  CloseServiceHandle(hSCMan);
  hSCMan := 0;
end;

//-----------------------------------------
function TSysDriver.Install(newdriverpath: string): DWORD; { use '' for default }
//-----------------------------------------
Var
  hService: SC_HANDLE;
  dwStatus: DWORD;
Begin
  dwStatus := 0;

  If newdriverpath = '' then
  Begin
    DriverDir   := HomeDir;
    DriverName  := DEVICE_NAME_STRING;
  end else
  Begin
    DriverDir  := ExtractFilePath(driverpath);
    DriverName := ExtractFileName(driverpath);
  end;
  DriverPath  := DriverDir + DriverName + '.sys';

   // add to service control manager's database
   hService := CreateService(hSCMan, PChar(DriverName),PChar(DriverName),
              SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
              SERVICE_ERROR_NORMAL, PChar(DriverPath),
              nil, nil, nil, nil, nil);
   if (hService = 0) then
   Begin
     dwStatus := GetLastError();
   end else
   Begin
     CloseServiceHandle(hService);
   end;

   result := dwStatus;
end;

//-------------------------------------------
function TSysDriver.Start:   DWORD;
//-------------------------------------------
Var
  hService: SC_HANDLE;
  dwStatus: DWORD;
  lpServiceArgVectors: PChar;
  temp: LongBool;
Begin
  dwStatus := 0;
  lpServiceArgVectors := nil;

   // get a handle to the service
   hService := OpenService(hSCMan, PChar(DriverName), SERVICE_ALL_ACCESS);
   if hService <> 0 then
   Begin
      // start the driver
      temp := StartService(hService, 0, PChar(lpServiceArgVectors));
      if not temp then dwStatus := GetLastError();
   end else dwStatus := GetLastError();

   if (hService <> 0) then CloseServiceHandle(hService);
   result := dwStatus;
end;

//-------------------------------------------
function TSysDriver.Stop:    DWORD;
//-------------------------------------------
Var
  hService: SC_HANDLE;
  dwStatus: DWORD;
  serviceStatus: TServiceStatus;
  temp: LongBool;
Begin
  dwStatus := 0;

  // get a handle to the service
  hService := OpenService(hSCMan, PChar(DriverName), SERVICE_ALL_ACCESS);
  if hService <> 0 then
  Begin
     // stop the driver
     temp := ControlService(hService, SERVICE_CONTROL_STOP, serviceStatus);
     if not temp then dwStatus := GetLastError();
  end else dwStatus := GetLastError();

  if (hService <> 0) then CloseServiceHandle(hService);
  result := dwStatus;
end;

//-------------------------------------------
function TSysDriver.Remove:  DWORD;
//-------------------------------------------
Var
  hService: SC_HANDLE;
  dwStatus: DWORD;
  temp: LongBool;
Begin
  dwStatus := Stop;  // ignore result

  // get a handle to the service
  hService := OpenService(hSCMan, PChar(DriverName), SERVICE_ALL_ACCESS);
  if hService <> 0 then
  Begin
     temp := DeleteService(hService);
     if not temp then dwStatus := GetLastError();
  end else dwStatus := GetLastError();

  if (hService <> 0) then CloseServiceHandle(hService);
  result := dwStatus;
end;

//=============================================================
// Device Open/Close functions
//=============================================================

//-------------------------------------------
function TSysDriver.DeviceOpen:  DWORD;
//-------------------------------------------
Var
  dwStatus: DWORD;
Begin
  dwStatus := 0;

  if hDevice <> INVALID_HANDLE_VALUE then DeviceClose;

  // get a handle to the device
  hDevice := CreateFile(
             { lpFileName: PChar            } PChar('\\.\'+ DEVICE_NAME_STRING),
             { dwDesiredAccess: integer     } GENERIC_READ or GENERIC_WRITE,
             { dwShareMode: Integer         } 0,
             { lpSecurityAttributes         } nil,
             { dwCreationDisposition: DWORD } OPEN_EXISTING,
             { dwFlagsAndAttributes: DWORD  } FILE_ATTRIBUTE_NORMAL,
             { hTemplateFile: THandle       } 0);

  if hDevice = INVALID_HANDLE_VALUE then
  Begin
    dwStatus := GetLastError();
  end else
  begin
    HaveLoad:=True;
  end;  

  result := dwStatus;
end;

//-------------------------------------------
function TSysDriver.DeviceClose:  DWORD;
//-------------------------------------------
Var
  dwStatus: DWORD;
Begin
  dwStatus := 0;
  if (hDevice <> INVALID_HANDLE_VALUE) then CloseHandle(hDevice);
  hDevice := INVALID_HANDLE_VALUE;
  result := dwStatus; { assume that it went OK? }
end;

//-------------------------------------------
function TSysDriver.IOControl(Cmd: DWORD; inBuf: Pointer; inSize: DWORD;
      outbuf: Pointer; var outSize: DWORD): Boolean;
//-------------------------------------------
Var
  BytesReturned: DWORD;
  MyControlCode: DWORD;

Begin
  Result := False;

  if hDevice = INVALID_HANDLE_VALUE then Exit;

  MyControlCode := Cmd;//CTL_CODE(BaseControlCode, Cmd , METHOD_BUFFERED, FILE_ANY_ACCESS);

  BytesReturned := 0;
  Result := DeviceIoControl(hDevice, MyControlCode ,
          { in buffer  (to driver)   }  InBuf,  inSize,
          { out buffer (from driver) }  OutBuf, outSize,

          BytesReturned, nil);
end;

end.


