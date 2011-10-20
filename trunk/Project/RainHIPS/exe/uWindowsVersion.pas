unit uWindowsVersion;

interface

uses
  Windows, SysUtils;

const
  TStrWindowsVersion : array [0..8] of String = ('Windows 3.x',
          'Windows 95',
          'Windows 98/Me',
          'Windows Nt 4.0',
          'Windows 2000',
          'Windows XP',
          'Windows Server 2003',
          'Windows Vista',
          'Windows Server 2008');


  Win3x    = $03000000;
  Win95    = $04000000;
  Win98ME  = $04010000;
  WinNT40  = $04020000;
  Win2000  = $05000000;
  WinXP    = $05010000;
  Win2003  = $05020000;
  WinVista = $06000000;
  Win2008  = $06000100;

TYPE
  TWinVer = Packed Record
     dwVersion : DWORD;
     sVerSion : String;
  end;
  

function GetWindowsVersion: TWinVer;


implementation


function GetWindowsVersion: TWinVer;
begin
  case  Win32Platform of
    VER_PLATFORM_WIN32s:      // Windows 3.Xƽ̨
      begin
        Result.dwVersion := Win3x;
        Result.sVerSion  := TStrWindowsVersion[0];
      end;

    VER_PLATFORM_WIN32_WINDOWS:      // Windows 9x/ME ƽ̨
        if ((Win32MajorVersion > 4 ) or
          ((Win32MajorVersion = 4 ) and (Win32MajorVersion > 0 ))) then
          begin
          Result.dwVersion := win98ME;
          Result.sVerSion  := TStrWindowsVersion[2];
          end
        else
          begin
          Result.dwVersion := Win95;
          Result.sVerSion  := TStrWindowsVersion[1];
          end;

    VER_PLATFORM_WIN32_NT:          // Windows NT ƽ̨
       case Win32MajorVersion of    //
         4: begin
          Result.dwVersion := WinNT40;
          Result.sVerSion  := TStrWindowsVersion[3];
          end;
         5: begin
          case Win32MinorVersion of    //
          0: begin
          Result.dwVersion := Win2000;
          Result.sVerSion  := TStrWindowsVersion[4];
          end;
          1: begin
          Result.dwVersion := WinXP;
          Result.sVerSion  := TStrWindowsVersion[5];
          end;
          2: begin
          Result.dwVersion := Win2003;
          Result.sVerSion  := TStrWindowsVersion[6];
          end;
          end;    // case
          end;
         6: begin
          case Win32MinorVersion  of    //
          0: begin
          Result.dwVersion := WinVista;
          Result.sVerSion  := TStrWindowsVersion[7];
          end;
          1: begin
          Result.dwVersion := Win2008;
          Result.sVerSion  := TStrWindowsVersion[8];
          end;
          end;    // case
          end;
       end;    // case
  end;
end;

end.
