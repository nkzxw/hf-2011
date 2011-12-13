{******************************************************************************}
{ Author:    Liwuyue                                                           }
{ Email:     smokingroom@sina.com                                              }
{ Home page: http://www.programmerlife.com                                     }
{ built:     2005-03-08                                                        }
{******************************************************************************}
unit uEncrypt;

interface

uses
  Windows, SysUtils, Messages;


procedure Encrypt(Handle:HWND; LFileName:string;LPassword:string;LBackup:Boolean);

function AttachStart:DWORD;stdcall;
procedure AttachProc;stdcall;
function AttachWindowProc(hwnd:HWND;uMsg:UINT;wParam:WPARAM;lParam:LPARAM):LRESULT;stdcall;
function CalcCrc32(lpSource:PChar;nLength:Integer):DWORD;stdcall;
procedure AttachEnd;stdcall;

implementation

const
  hWndAttachExStyle      = 0;
  hWndAttachStyle        = WS_MINIMIZEBOX or WS_SYSMENU or WS_CAPTION or WS_OVERLAPPED;    //WS_SIZEBOX
  dwWndAttachWidth       = 320;
  dwWndAttachHeight      = 120;
  IDC_EDIT_PASSWORD      = 100;
  IDC_BUTTON_OK	         = 101;
  IDC_BUTTON_CANCEL      = 102;
  IDM_ATTACH_MENU_ABOUT	 = 103;
  MAX_PASSWORD_LENGTH    = 16;

type
  TGetProcAddress        = function(hModule:HMODULE;lpProcName:LPCSTR):Pointer;stdcall;
  TLoadLibrary           = function(lpLibFileName:PChar):HMODULE;stdcall;
  TFreeLibrary           = function(hLibModule:HMODULE):BOOL;stdcall;
  TExitProcess           = procedure(uExitCode:UINT);stdcall;
  TGetModuleHandle       = function(lpModuleName:PChar):HMODULE;stdcall;
  TGetMessage            = function(var lpMsg:TMsg;hWnd:HWND;wMsgFilterMin,wMsgFilterMax:UINT):BOOL;stdcall;
  TTranslateMessage      = function(const lpMsg:TMsg):BOOL; stdcall;
  TDispatchMessage       = function(const lpMsg:TMsg):Longint; stdcall;
  TGetSystemMetrics      = function(nIndex:Integer): Integer; stdcall;
  TPostMessage           = function(hWnd:HWND;Msg:UINT;wParam:WPARAM;lParam:LPARAM):BOOL;stdcall;
  TSendMessage           = function(hWnd:HWND;Msg:UINT;wParam:WPARAM;lParam:LPARAM):LRESULT;stdcall;
  TShowWindow            = function(hWnd:HWND;nCmdShow:Integer):BOOL;stdcall;
  TUpdateWindow          = function(hWnd:HWND):BOOL;stdcall;
  TLoadCursor            = function(hInstance:HINST;lpCursorName:PAnsiChar):HCURSOR;stdcall;
  TLoadIcon              = function(hInstance:HINST;lpIconName:PAnsiChar):HICON; stdcall;
  TPostQuitMessage       = procedure(nExitCode:Integer);stdcall;
  TMessageBox            = function(hWnd:HWND;lpText,lpCaption:PChar;uType:UINT):Integer;stdcall;
  TRegisterClassEx       = function(const WndClass:TWndClassEx):ATOM;stdcall;
  TCreateWindowEx        = function(dwExStyle:DWORD;lpClassName:PChar;
                                    lpWindowName:PChar;dwStyle:DWORD;X,Y,nWidth,nHeight:Integer;
                                    hWndParent:HWND;hMenu:HMENU;hInstance:HINST;lpParam:Pointer):HWND;stdcall;
  TDefWindowProc         = function(hWnd:HWND;Msg:UINT;wParam:WPARAM;lParam:LPARAM):LRESULT;stdcall;
  TSetFocus              = function(hWnd:HWND):HWND;stdcall;
  TGetWindowLong         = function(hWnd:HWND;nIndex:Integer):Longint;stdcall;
  TSetWindowLong         = function(hWnd:HWND;nIndex:Integer;dwNewLong:Longint):Longint; stdcall;
  TGetDlgItemText        = function(hDlg:HWND;nIDDlgItem:Integer;lpString:PChar;nMaxCount:Integer):UINT;stdcall;
  TGetSystemMenu         = function(hWnd:HWND;bRevert:BOOL):HMENU;stdcall;
  TAppendMenu            = function(hMenu:HMENU;uFlags,uIDNewItem:UINT;lpNewItem:PChar):BOOL;stdcall;
  TCreateFontIndirect    = function(const p1:TLogFont):HFONT;stdcall;
  TDeleteObject          = function(p1:HGDIOBJ):BOOL;stdcall;
  TIsDialogMessage       = function(hDlg:HWND;var lpMsg:TMsg):BOOL;stdcall;
  TGetDlgItem            = function(hDlg:HWND;nIDDlgItem:Integer):HWND;stdcall;
  Twsprintf              = function(Output:PChar;Format:PChar;arglist:va_list):Integer;stdcall;
  TSetWindowText         = function(hWnd:HWND;lpString:PChar):BOOL;stdcall;
  Tlstrlen               = function(lpString:PChar):Integer;stdcall;


type
  PAttachData=^TAttachData;
  TAttachData=packed record
    hLibUser32:                HMODULE;
    hLibGDI32:                 HMODULE;
    _GetProcAddress:           TGetProcAddress;
    _LoadLibrary:              TLoadLibrary;
    _FreeLibrary:              TFreeLibrary;
    _ExitProcess:	       TExitProcess;
    _GetModuleHandle:	       TGetModuleHandle;
    _lstrlen:                  Tlstrlen;
    _GetMessage:	       TGetMessage;
    _TranslateMessage:	       TTranslateMessage;
    _DispatchMessage:	       TDispatchMessage;
    _GetSystemMetrics:	       TGetSystemMetrics;
    _PostMessage:	       TPostMessage;
    _SendMessage:	       TSendMessage;
    _ShowWindow:  	       TShowWindow;
    _UpdateWindow:	       TUpdateWindow;
    _LoadCursor:  	       TLoadCursor;
    _LoadIcon:                 TLoadIcon;
    _PostQuitMessage:	       TPostQuitMessage;
    _MessageBox: 	       TMessageBox;
    _RegisterClassEx:	       TRegisterClassEx;
    _CreateWindowEx:	       TCreateWindowEx;
    _DefWindowProc:	       TDefWindowProc;
    _SetFocus:		       TSetFocus;
    _GetWindowLong:	       TGetWindowLong;
    _SetWindowLong:	       TSetWindowLong;
    _GetDlgItemText:	       TGetDlgItemText;
    _GetSystemMenu:	       TGetSystemMenu;
    _AppendMenu: 	       TAppendMenu;
    _CreateFontIndirect:       TCreateFontIndirect;
    _DeleteObject:	       TDeleteObject;
    _IsDialogMessage:	       TIsDialogMessage;
    _GetDlgItem:	       TGetDlgItem;
    _wsprintf:		       Twsprintf;
    _SetWindowText:	       TSetWindowText;

    szLibUser32:	       array[0..6]  of Char; //	"user32"
    szLibGDI32:		       array[0..5]  of Char; //	"gdi32"

    szLoadLibrary:	       array[0..12] of Char; //	"LoadLibraryA"
    szFreeLibrary:	       array[0..11] of Char; //	"FreeLibrary"
    szExitProcess:	       array[0..11] of Char; //	"ExitProcess"
    szGetModuleHandle:         array[0..16] of Char; //	"GetModuleHandleA"
    szlstrlen:                 array[0..8]  of Char; // "lstrlenA"

    szGetMessage:	       array[0..11] of Char; //	"GetMessageA"
    szTranslateMessage:        array[0..16] of Char; //	"TranslateMessage"
    szDispatchMessage:         array[0..16] of Char; //	"DispatchMessageA"
    szGetSystemMetrics:        array[0..16] of Char; //	"GetSystemMetrics"
    szPostMessage:	       array[0..12] of Char; //	"PostMessageA"
    szSendMessage:	       array[0..12] of Char; //	"SendMessageA"
    szShowWindow:	       array[0..10] of Char; //	"ShowWindow"
    szUpdateWindow:	       array[0..12] of Char; //	"UpdateWindow"
    szLoadCursor:	       array[0..11] of Char; //	"LoadCursorA"
    szLoadIcon:                array[0..9]  of Char; // "LoadIconA"
    szPostQuitMessage:         array[0..15] of Char; //	"PostQuitMessage"
    szMessageBox:	       array[0..11] of Char; //	"MessageBoxA"
    szRegisterClassEx:         array[0..16] of Char; //	"RegisterClassExA"
    szCreateWindowEx:          array[0..15] of Char; //	"CreateWindowExA"
    szDefWindowProc:           array[0..14] of Char; //	"DefWindowProcA"
    szSetFocus:	               array[0..8]  of Char; //	"SetFocus"
    szGetWindowLong:           array[0..14] of Char; //	"GetWindowLongA"
    szSetWindowLong:           array[0..14] of Char; //	"SetWindowLongA"
    szGetDlgItemText:          array[0..15] of Char; //	"GetDlgItemTextA"
    szGetSystemMenu:           array[0..13] of Char; //	"GetSystemMenu"
    szAppendMenu:              array[0..11] of Char; //	"AppendMenuA"
    szIsDialogMessage:	       array[0..15] of Char; //	"IsDialogMessage"
    szGetDlgItem:	       array[0..10] of Char; //	"GetDlgItem"
    szwsprintf:	               array[0..10] of Char; //	"wvsprintfA"
    szSetWindowText:	       array[0..14] of Char; //	"SetWindowTextA"

    szCreateFontIndirect:      array[0..19] of Char; //	"CreateFontIndirectA"
    szDeleteObject:	       array[0..12] of Char; //	"DeleteObject"

    _szAppClass:	       array[0..10]  of Char; // "PE Encrypt"
    _szAppTitle:	       array[0..20]  of Char; // "PE Encrypt :: v1.0"
    _szMenuAbout:	       array[0..20]  of Char; // "&About PE Encrypt..."
    _szMsgAbout:	       array[0..150] of Char; //

    _szClassEdit:	       array[0..4]   of Char; // "Edit"
    _szClassStatic:	       array[0..6]   of Char; // "Static"
    _szClassButton:	       array[0..6]   of Char; // "Button"
    _szTitlePassword:	       array[0..11]  of Char; // "请输入密码:"
    _szOK:		       array[0..8]   of Char; // "确定(&O)"
    _szCancel:                 array[0..8]   of Char; // "取消(&C)"
    _szWrongPassword:	       array[0..24]  of Char; // "密码不正确,请重新输入!"
    _szTemplate:	       array[0..29]  of Char; // "--= 你还剩下 %d 次机会 =--"
    _dwPasswordCrc32:	       DWORD;                 // 密码的CRC32校检码
    _szChanceCount:	       array[0..255] of Char; // 256字节的缓冲区
    
    _hWndAttach:	       HWND;
    _fnt:                      LOGFONT;
    _hFont:		       THandle;
    _bCorrect:		       Byte;
    _hWndChanceCount:	       HWND;
    _wc:		       WNDCLASSEX;
    _nCount:		       DWORD;

    _ImageBase:                DWORD;
    _EntryPoint:               DWORD;
  end;

var
  AttachData:TAttachData;

procedure Init_AttachData;
begin
  //初始化数据
  FillChar(AttachData,SizeOf(Attachdata),0);
  with AttachData do
  begin
    szLibUser32 	       :='user32'#0;
    szLibGDI32		       :='gdi32'#0;

    szLoadLibrary	       :='LoadLibraryA'#0;
    szFreeLibrary	       :='FreeLibrary'#0;
    szExitProcess	       :='ExitProcess'#0;
    szGetModuleHandle          :='GetModuleHandleA'#0;
    szlstrlen                  :='lstrlenA'#0;

    szGetMessage	       :='GetMessageA'#0;
    szTranslateMessage         :='TranslateMessage'#0;
    szDispatchMessage          :='DispatchMessageA'#0;
    szGetSystemMetrics         :='GetSystemMetrics'#0;
    szPostMessage	       :='PostMessageA'#0;
    szSendMessage	       :='SendMessageA'#0;
    szShowWindow	       :='ShowWindow'#0;
    szUpdateWindow	       :='UpdateWindow'#0;
    szLoadCursor	       :='LoadCursorA'#0;
    szLoadIcon                 :='LoadIconA'#0; 
    szPostQuitMessage          :='PostQuitMessage'#0;
    szMessageBox	       :='MessageBoxA'#0;
    szRegisterClassEx          :='RegisterClassExA'#0;
    szCreateWindowEx           :='CreateWindowExA'#0;
    szDefWindowProc            :='DefWindowProcA'#0;
    szSetFocus	               :='SetFocus'#0;
    szGetWindowLong            :='GetWindowLongA'#0;
    szSetWindowLong            :='SetWindowLongA'#0;
    szGetDlgItemText           :='GetDlgItemTextA'#0;
    szGetSystemMenu            :='GetSystemMenu'#0;
    szAppendMenu               :='AppendMenuA'#0;
    szIsDialogMessage	       :='IsDialogMessage'#0;
    szGetDlgItem	       :='GetDlgItem'#0;
    szwsprintf	               :='wvsprintfA'#0;          //wsprintfA
    szSetWindowText	       :='SetWindowTextA'#0;

    szCreateFontIndirect       :='CreateFontIndirectA'#0;
    szDeleteObject	       :='DeleteObject'#0;

    _fnt.lfHeight:=12;
    _fnt.lfWidth:=0;
    _fnt.lfEscapement:=0;
    _fnt.lfOrientation:=0;
    _fnt.lfWeight:=FW_NORMAL;
    _fnt.lfItalic:=0;
    _fnt.lfUnderline:=0;
    _fnt.lfStrikeOut:=0;
    _fnt.lfCharSet:=DEFAULT_CHARSET;
    _fnt.lfOutPrecision:=OUT_DEFAULT_PRECIS;
    _fnt.lfClipPrecision:=CLIP_DEFAULT_PRECIS;
    _fnt.lfQuality:=PROOF_QUALITY;
    _fnt.lfPitchAndFamily:=DEFAULT_PITCH or FF_DONTCARE;
    _fnt.lfFaceName:='宋体';

    _szAppClass 	       :='PEEncrypt'#0;
    _szAppTitle 	       :='PE Encrypt :: v1.0'#0;
    _szMenuAbout               :='&About PE Encrypt...'#0;
    _szMsgAbout 	       :='[ PE Encrypt ]'#13#10
                                +'Version: 1.0'#13#10#13#10
                                +'作者: Liwuyue'#13#10
                                +'邮箱: smokingroom@sina.com'#13#10
                                +'主页: http://www.programmerlife.com'#0;

    _szClassEdit	       :='Edit'#0;
    _szClassStatic	       :='Static'#0;
    _szClassButton	       :='Button'#0;
    _szTitlePassword	       :='请输入密码:'#0;
    _szOK		       :='确定(&O)'#0;
    _szCancel                  :='取消(&C)'#0;
    _szWrongPassword	       :='密码不正确,请重新输入!'#0;
    _szTemplate 	       :='--= 你还剩下 %d 次机会 =--'#0;
    _nCount		       :=3;
  end;
end;

//*********************************附加优段开始**********************************************

function AttachStart:DWORD;stdcall;
asm
  CALL @@1
@@1:
  POP EAX
  SUB EAX, 5
end;

//附加段的处理模块
procedure AttachProc;stdcall;
var
  AttachData:PAttachData;
  dwKernel32:DWORD;
  dwNtHeaders:DWORD;
  dwExportEntry:DWORD;
  dwAddressOfNames:DWORD;
  dwAddressOfNameOrdinals:DWORD;
  dwAddressOfFunctions:DWORD;
  dwNumberOfNames:DWORD;
  RelativeID:DWORD;
  msg:TagMSG;
  I:DWORD;
  aLeft,aTop:Integer;
  EntryPoint:DWORD;
begin
   //******查找Kernel32.dll的基地址
  asm
	MOV	EAX,  [ESP+48]
	AND	EAX,  $FFFF0000
  @@chk:
       	CMP	DWORD PTR [EAX], $00905A4D
	JE	@@fnd
	SUB	EAX,  $1000
	JMP	@@chk
  @@fnd:
        MOV     dwKernel32, EAX
  end;
  AttachData:=Pointer(AttachStart-SizeOf(TAttachData));
  dwNtHeaders:=dwKernel32+DWORD(PImageDosHeader(dwKernel32)._lfanew);
  dwExportEntry:=dwKernel32+PImageNtHeaders(dwNtHeaders).OptionalHeader.DataDirectory[0].VirtualAddress;
  dwAddressOfNames:=dwKernel32+DWORD(PImageExportDirectory(dwExportEntry).AddressOfNames);
  dwAddressOfNameOrdinals:=dwKernel32+DWORD(PImageExportDirectory(dwExportEntry).AddressOfNameOrdinals);
  dwAddressOfFunctions:=dwKernel32+DWORD(PImageExportDirectory(dwExportEntry).AddressOfFunctions);
  dwNumberOfNames:=PImageExportDirectory(dwExportEntry).NumberOfNames;
  //*******在Kernel32.dll里面查找GetProcAddress函数的线性地址
  for I:=0 to dwNumberOfNames-1 do
  begin
    if (PDWORD(dwKernel32+PDWORD(dwAddressOfNames+I*4)^)^=$50746547)       //PteG --GetP
      and (PDWORD(dwKernel32+PDWORD(dwAddressOfNames+I*4)^+4)^=$41636F72)  //Acor --rocA
      and (PDWORD(dwKernel32+PDWORD(dwAddressOfNames+I*4)^+8)^=$65726464)  //erdd --ddre  
      and (PWORD(dwKernel32+PDWORD(dwAddressOfNames+I*4)^+12)^=$7373) then //ss   --ss
    begin
      RelativeID:=PWORD(dwAddressOfNameOrdinals+I*2)^;
      AttachData._GetProcAddress:=Pointer(dwKernel32+PDWORD(dwAddressOfFunctions+RelativeID*4)^);
      Break;
    end;
  end;
  with AttachData^ do
  begin
    _LoadLibrary               :=_GetProcAddress(dwKernel32,szLoadLibrary);
    _FreeLibrary               :=_GetProcAddress(dwKernel32,szFreeLibrary);
    _ExitProcess	       :=_GetProcAddress(dwKernel32,szExitProcess);
    _GetModuleHandle	       :=_GetProcAddress(dwKernel32,szGetModuleHandle);
    _lstrlen                   :=_GetProcAddress(dwKernel32,szlstrlen);

    hLibUser32                 :=_LoadLibrary(szLibUser32);   //载入user32.dll
    hLibGDI32                  :=_LoadLibrary(szLibGDI32);    //载入gdi32.dll

    //***获取user32.dll中的一些API
    _GetMessage	               :=_GetProcAddress(hLibUser32,szGetMessage);
    _TranslateMessage	       :=_GetProcAddress(hLibUser32,szTranslateMessage);
    _DispatchMessage	       :=_GetProcAddress(hLibUser32,szDispatchMessage);
    _GetSystemMetrics	       :=_GetProcAddress(hLibUser32,szGetSystemMetrics);
    _PostMessage	       :=_GetProcAddress(hLibUser32,szPostMessage);
    _SendMessage	       :=_GetProcAddress(hLibUser32,szSendMessage);
    _ShowWindow  	       :=_GetProcAddress(hLibUser32,szShowWindow);
    _UpdateWindow	       :=_GetProcAddress(hLibUser32,szUpdateWindow);
    _LoadCursor  	       :=_GetProcAddress(hLibUser32,szLoadCursor);
    _LoadIcon                  :=_GetProcAddress(hLibUser32,szLoadIcon);
    _PostQuitMessage	       :=_GetProcAddress(hLibUser32,szPostQuitMessage);
    _MessageBox 	       :=_GetProcAddress(hLibUser32,szMessageBox);
    _RegisterClassEx	       :=_GetProcAddress(hLibUser32,szRegisterClassEx);
    _CreateWindowEx	       :=_GetProcAddress(hLibUser32,szCreateWindowEx);
    _DefWindowProc	       :=_GetProcAddress(hLibUser32,szDefWindowProc);
    _SetFocus		       :=_GetProcAddress(hLibUser32,szSetFocus);
    _GetWindowLong	       :=_GetProcAddress(hLibUser32,szGetWindowLong);
    _SetWindowLong	       :=_GetProcAddress(hLibUser32,szSetWindowLong);
    _GetDlgItemText	       :=_GetProcAddress(hLibUser32,szGetDlgItemText);
    _GetSystemMenu	       :=_GetProcAddress(hLibUser32,szGetSystemMenu);
    _AppendMenu 	       :=_GetProcAddress(hLibUser32,szAppendMenu);
    _IsDialogMessage	       :=_GetProcAddress(hLibUser32,szIsDialogMessage);
    _GetDlgItem	               :=_GetProcAddress(hLibUser32,szGetDlgItem);
    _wsprintf		       :=_GetProcAddress(hLibUser32,szwsprintf);
    _SetWindowText	       :=_GetProcAddress(hLibUser32,szSetWindowText);

    //***获取gdi32.dll中的两个API
    _CreateFontIndirect        :=_GetProcAddress(hLibGDI32,szCreateFontIndirect);
    _DeleteObject	       :=_GetProcAddress(hLibGDI32,szDeleteObject);

    //******Win32 API操作:注册窗口类及创建窗口实例
    _wc.hInstance:=_GetModuleHandle(nil);
    _wc.cbSize:=SizeOf(WNDCLASSEX);
    _wc.style:=CS_HREDRAW or CS_VREDRAW;
    //窗口过程,非常重要!!!
    _wc.lpfnWndProc:=Pointer(DWORD(@AttachWindowProc)-DWORD(@AttachStart)+AttachStart);
    _wc.hbrBackground:=COLOR_WINDOW;
    _wc.lpszClassName:=_szAppClass;
    _wc.hCursor:=_LoadCursor(0,IDC_ARROW);
    _wc.hIcon:=_LoadIcon(0,IDI_WINLOGO);
    _wc.hIconSm:=_wc.hIcon; 
    _RegisterClassEx(_wc);
    aLeft:=(_GetSystemMetrics(SM_CXSCREEN)-dwWndAttachWidth) div 2;
    aTop:=(_GetSystemMetrics(SM_CYSCREEN)-dwWndAttachHeight) div 2;
    _hWndAttach:=_CreateWindowEx(hWndAttachExStyle,_szAppClass,_szAppTitle,hWndAttachStyle,aLeft,aTop,dwWndAttachWidth,dwWndAttachHeight,0,0,_wc.hInstance,nil);
    _ShowWindow(_hWndAttach,SW_SHOW);
    _UpdateWindow(_hWndAttach);

    //******处理消息循环
    while _GetMessage(msg,0,0,0) do
    begin
      if not _IsDialogMessage(_hWndAttach,msg) then
      begin
        _TranslateMessage(msg);
        _DispatchMessage(msg);
      end;
    end;
    _FreeLibrary(hLibGDI32);
    _FreeLibrary(hLibUser32);
    if _bCorrect=1 then
    begin
      EntryPoint:=_wc.hInstance+(_wc.hInstance-_ImageBase)+_EntryPoint;       //============================!!!
      asm
        MOV EAX, EntryPoint
        JMP EAX
      end;
    end;
    _ExitProcess(0);
  end;
end;


//附加段的消息循环
function AttachWindowProc(hwnd:HWND;uMsg:UINT;wParam:WPARAM;lParam:LPARAM):LRESULT;stdcall;
var
  AttachData:PAttachData;
  hSysMenu:HMENU;
  TmpHwnd:Windows.HWND;
  szPassword:array[0..MAX_PASSWORD_LENGTH-1] of Char;
begin
  Result:=0;
  AttachData:=Pointer(AttachStart-SizeOf(TAttachData));
  with AttachData^ do
    case uMsg of
      WM_CREATE:
        begin
          //在系统菜单中添加"关於"项目
          hSysMenu:=_GetSystemMenu(hwnd,False);
          _AppendMenu(hSysMenu,MF_SEPARATOR,0,nil);
          _AppendMenu(hSysMenu,MF_STRING,IDM_ATTACH_MENU_ABOUT,_szMenuAbout);
          _hFont:=_CreateFontIndirect(_fnt);
          //创建Static控件,显示还可以输入多少次密码
          _hWndChanceCount:=_CreateWindowEx(0,_szClassStatic,_szChanceCount,SS_CENTER or SS_CENTERIMAGE or WS_VISIBLE or WS_CHILD,
                          10,32,300,22,hwnd,0,_wc.hInstance,nil);
          _SendMessage(_hWndChanceCount,WM_SETFONT,_hFont,0);
          _wsprintf(_szChanceCount,_szTemplate,@_nCount);
          _SetWindowText(_hWndChanceCount,_szChanceCount);
          //创建Static控件,显示"请输入密码:'
          TmpHwnd:=_CreateWindowEx(0,_szClassStatic,_szTitlePassword,SS_RIGHT or SS_CENTERIMAGE or WS_VISIBLE or WS_CHILD,
                          5,10,80,22,hWnd,0,_wc.hInstance,nil);
          _SendMessage(TmpHwnd,WM_SETFONT,_hFont,0);
          //创建Edit控件,供用户输入密码
          TmpHwnd:=_CreateWindowEx(WS_EX_STATICEDGE,_szClassEdit,nil,ES_AUTOHSCROLL or ES_PASSWORD or WS_VISIBLE or WS_TABSTOP or WS_CHILD,
                          90,12,205,18,hWnd,0,_wc.hInstance,nil);
          _SendMessage(TmpHwnd,EM_SETLIMITTEXT,MAX_PASSWORD_LENGTH,0);
          _SetWindowLong(TmpHwnd,GWL_ID,IDC_EDIT_PASSWORD);
          _SetFocus(TmpHwnd);
          //创建Button控件,即"确定"按钮
          TmpHwnd:=_CreateWindowEx(0,_szClassButton,_szOK,BS_FLAT or BS_DEFPUSHBUTTON or WS_VISIBLE or WS_TABSTOP or WS_CHILD,
                        80,60,80,20,hWnd,0,_wc.hInstance,nil);
          _SendMessage(TmpHwnd,WM_SETFONT,_hFont,0);
          _SetWindowLong(TmpHwnd,GWL_ID,IDC_BUTTON_OK);
          //创建Button控件,即"取消"按钮
          TmpHwnd:=_CreateWindowEx(0,_szClassButton,_szCancel,BS_FLAT or WS_VISIBLE or WS_TABSTOP or WS_CHILD,
                        180,60,80,20,hWnd,0,_wc.hInstance,nil);
          _SendMessage(TmpHwnd,WM_SETFONT,_hFont,0);
          _SetWindowLong(TmpHwnd,GWL_ID,IDC_BUTTON_CANCEL);
        end;
      WM_COMMAND:
        begin
          if LOWORD(wParam)=IDC_BUTTON_OK then
          begin
            _GetDlgItemText(hwnd,IDC_EDIT_PASSWORD,szPassword,MAX_PASSWORD_LENGTH);
            if CalcCrc32(szPassword,_lstrlen(szPassword))=_dwPasswordCrc32 then
              _bCorrect:=1;
            if (_bCorrect=1) or (_nCount=1) then _PostMessage(hwnd,WM_CLOSE,0,0)
            else begin
              _MessageBox(hwnd,_szWrongPassword,_szAppTitle,MB_OK+MB_ICONERROR);
              TmpHwnd:=_GetDlgItem(hwnd,IDC_EDIT_PASSWORD);
              _SetFocus(TmpHwnd);
              _SendMessage(hwnd,EM_SETSEL,0,-1);
              _nCount:=_nCount-1;
              _wsprintf(_szChanceCount,_szTemplate,@_nCount);
              _SetWindowText(_hWndChanceCount,_szChanceCount);
            end;
          end else if LOWORD(wParam)=IDC_BUTTON_CANCEL then
            _PostMessage(hwnd,WM_CLOSE,0,0)
          else
            Result:=_DefWindowProc(hwnd,uMsg,wParam,lParam);
        end;
      WM_SYSCOMMAND:
        begin
          if wParam=IDM_ATTACH_MENU_ABOUT then
            _MessageBox(hwnd,_szMsgAbout,_szAppTitle,MB_OK+MB_ICONINFORMATION)
          else
            Result:=_DefWindowProc(hwnd,uMsg,wParam,lParam);
        end;
      WM_CLOSE:
        begin
          _DeleteObject(_hFont);
          Result:=_DefWindowProc(hwnd,uMsg,wParam,lParam);
        end;
      WM_DESTROY:
        begin
          _PostQuitMessage(0);
        end;
      else
        Result:=_DefWindowProc(hwnd,uMsg,wParam,lParam);
    end;
end;

function CalcCrc32(lpSource:PChar; nLength:Integer):DWORD;stdcall;
var
  Crc32Table:array[0..255] of DWORD;
  I,J:Integer;
  crc:DWORD;
begin
  //初始化CRC32表
  for I:=0 to 255 do
  begin
    crc:=I;
    for J:=0 to 7 do
    begin
      if crc and 1 > 0 then
        crc:=(crc shr 1) xor $EDB88320
      else
        crc:=crc shr 1;
    end;
    Crc32Table[I]:=crc;
  end;
  //计算CRC32值
  Result:=8;
  for I:=0 to nLength-1 do
    Result := Crc32Table[Byte(Result xor DWORD(Ord(lpSource[I])))] xor ((Result shr 8) and $00FFFFFF);
  Result:=not Result;
end;

procedure AttachEnd;stdcall;
begin
end;

//*********************************附加优段结束**********************************************

procedure Encrypt(Handle:HWND; LFileName:string;LPassword:string;LBackup:Boolean);
const
  DlgCaption='PE Ecrypt v1.0';
var
  hFile:THandle;
  ImgDosHeader:IMAGE_DOS_HEADER;
  ImgNtHeaders:IMAGE_NT_HEADERS;
  ImgSectionHeader:IMAGE_SECTION_HEADER;
  LPointerToRawData:DWORD;
  LVirtualAddress:DWORD;
  I:Integer;
  BytesRead,BytesWrite:Cardinal;
  AttachSize:DWORD;
begin
  hFile:=CreateFile(PChar(LFileName),GENERIC_READ or GENERIC_WRITE,FILE_SHARE_READ or FILE_SHARE_WRITE,nil,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if hFile=INVALID_HANDLE_VALUE then
  begin
    MessageBox(Handle,'打开文件失败!',DlgCaption,MB_ICONERROR+MB_OK);
    Exit;
  end;
  try
    ReadFile(hFile,ImgDosHeader,SizeOf(ImgDosHeader),BytesRead,nil);
    if ImgDosHeader.e_magic<>IMAGE_DOS_SIGNATURE then
    begin
      MessageBox(Handle,'这不是一个有效的PE文件!',DlgCaption,MB_ICONERROR+MB_OK);
      Exit;
    end;
    SetFilePointer(hFile,ImgDosHeader._lfanew,nil,FILE_BEGIN);
    ReadFile(hFile,ImgNtHeaders,SizeOf(ImgNtHeaders),BytesRead,nil);
    if ImgNtHeaders.Signature<>IMAGE_NT_SIGNATURE then
    begin
      MessageBox(Handle,'这不是一个有效的PE文件!',DlgCaption,MB_ICONERROR+MB_OK);
      Exit;
    end;
    LPointerToRawData:=0;
    LVirtualAddress:=0;
    for I:=0 to ImgNtHeaders.FileHeader.NumberOfSections-1 do
    begin
      ReadFile(hFile,ImgSectionHeader,SizeOf(ImgSectionHeader),BytesRead,nil);
      if PChar(@ImgSectionHeader.Name[0])='.LWY' then
      begin
        MessageBox(Handle,'呵呵, 您忘了吗? 文件已经加密了!',DlgCaption,MB_ICONINFORMATION+MB_OK);
        Exit;
      end;
      if LPointerToRawData<ImgSectionHeader.PointerToRawData+ImgSectionHeader.SizeOfRawData then
        LPointerToRawData:=ImgSectionHeader.PointerToRawData+ImgSectionHeader.SizeOfRawData;
      if LVirtualAddress<ImgSectionHeader.VirtualAddress+ImgSectionHeader.Misc.VirtualSize then
        LVirtualAddress:=ImgSectionHeader.VirtualAddress+ImgSectionHeader.Misc.VirtualSize;
    end;
    //******备份原文件
    if LBackup then
      CopyFile(PChar(LFileName),PChar(LFileName+'.bak'),False);
    //******计算新节的长度
    AttachSize:=Integer(@AttachEnd)-Integer(@AttachStart)+SizeOf(TAttachData);
    Move('.LWY'#0,ImgSectionHeader.Name[0],5);
    ImgSectionHeader.Misc.VirtualSize:=AttachSize;
    ImgSectionHeader.VirtualAddress:=LVirtualAddress;
    ImgSectionHeader.SizeOfRawData:=AttachSize;
    ImgSectionHeader.PointerToRawData:=LPointerToRawData;
    ImgSectionHeader.PointerToRelocations:=0;
    ImgSectionHeader.PointerToLinenumbers:=0;
    ImgSectionHeader.NumberOfRelocations:=0;
    //******计算新节的加载RVA (RVA必须为4096的整数倍)
    if ImgSectionHeader.VirtualAddress mod 4096 > 0 then
      ImgSectionHeader.VirtualAddress:=(ImgSectionHeader.VirtualAddress div 4096 + 1) * 4096;
    //******计算新节的PointerToRawData (为512的倍数<win2000/xp下正确加载>)
    if ImgSectionHeader.PointerToRawData mod 512 > 0 then
      ImgSectionHeader.PointerToRawData:=(ImgSectionHeader.PointerToRawData div 512 + 1) * 512;
    //******设置新节的Characteristics (code/data/execute/read/write/inited data/un-inited data)
    ImgSectionHeader.Characteristics:=$E00000E0;
    //******写入新节信息
    WriteFile(hFile,ImgSectionHeader,SizeOf(ImgSectionHeader),BytesWrite,nil);
    //******修改入口地址,记得要先记录原来的哦^_^
    AttachData._EntryPoint:=ImgNtHeaders.OptionalHeader.AddressOfEntryPoint;
    AttachData._ImageBase:=ImgNtHeaders.OptionalHeader.ImageBase;
    ImgNtHeaders.OptionalHeader.AddressOfEntryPoint:=ImgSectionHeader.VirtualAddress
                                                    +SizeOf(TAttachData)
                                                    +DWORD(@AttachProc)-DWORD(@AttachStart);
    //******修改镜像大小
    ImgNtHeaders.OptionalHeader.SizeOfImage:=ImgNtHeaders.OptionalHeader.SizeOfImage+AttachSize;
    //******保存密码
    AttachData._dwPasswordCrc32:=CalcCrc32(PChar(LPassword),Length(LPassword));
    //******修改节个数
    Inc(ImgNtHeaders.FileHeader.NumberOfSections);
    //******定位到IMAGE_NT_HEADERS
    SetFilePointer(hFile,ImgDosHeader._lfanew,nil,FILE_BEGIN);
    WriteFile(hFile,ImgNtHeaders,SizeOf(ImgNtHeaders),BytesWrite,nil);
    //******定位到新节的起始偏移地址
    SetFilePointer(hFile,ImgSectionHeader.PointerToRawData,nil,FILE_BEGIN);
    //******写入AttachData
    WriteFile(hFile,AttachData,SizeOf(AttachData),BytesWrite,nil);
    //******写入代码
    WriteFile(hFile,PByte(@AttachStart)^,Integer(@AttachEnd)-Integer(@AttachStart),BytesWrite,nil);
    MessageBox(Handle,'恭喜,加密成功!',DlgCaption,MB_OK+MB_ICONINFORMATION);
  finally
    CloseHandle(hFile);
  end;
end;

initialization
  Init_AttachData;;
finalization
end.
