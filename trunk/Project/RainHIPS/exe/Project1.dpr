program Project1;

uses
  Forms,
  Windows,
  Unit1 in 'Unit1.pas' {Form1},
  SysPub in 'SysPub.pas',
  Unit2 in 'Unit2.pas' {Form2},
  ShowAsk in 'ShowAsk.pas' {Form3},
  AnSafeThr in 'AnSafeThr.pas',
  rulepub in 'rulepub.pas',
  uWindowsVersion in 'uWindowsVersion.pas',
  GetSid in 'GetSid.pas';

{$R *.res}
var   
    hMutex:HWND;
    Ret:Integer;

begin
  hMutex:=CreateMutex(nil,   False,   'CreateMore');
  Ret:=GetLastError;
  if Ret<>ERROR_ALREADY_EXISTS then
  begin
  Application.Initialize;
  //Application.MainFormOnTaskbar := True;
  Application.Title := '”Ì∂‹HIPS';
  Application.CreateForm(TForm1, Form1);
  Application.CreateForm(TForm3, Form3);
  Application.Run;
  end;
end.
