unit SysPub;

interface

uses
  Windows,SysUtils,SysDriver;

var
  sysdrv: TSysDriver=nil;

  procedure IO_install;
  procedure IO_Uninstall;
implementation

procedure IO_install;
begin
  if sysdrv = nil then
  begin
  sysdrv := TSysDriver.Create('rainsafe');
  sysdrv.OpenSCM;
  sysdrv.Install('');
  sysdrv.Start;
  sysdrv.DeviceOpen;
  end;
end;

procedure IO_Uninstall;
begin
  if sysdrv <> nil then
  begin
  sysdrv.DeviceClose;
  sysdrv.Stop;
  sysdrv.Remove;
  sysdrv.CloseSCM;
  sysdrv.Free;
  sysdrv := nil;
  end;
end;

end.
