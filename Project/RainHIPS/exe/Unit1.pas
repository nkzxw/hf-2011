unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, SkinData, DynamicSkinForm, SkinCtrls,
  ExtCtrls, ImgList, sppngimagelist, se_controls,
  spTrayIcon, AppEvnts, Menus, SkinMenus, SysPub, dsaadapter,ShellAPI,
  FlatUtils, Buttons, RXCtrls,IniFiles,
  FlatBtns, OleCtrls, pngimage, ToolWin, ComCtrls, SkinTabs, FlatCtrls, se_image;
type
  TForm1 = class(TForm)
    spDynamicSkinForm1: TspDynamicSkinForm;
    spSkinData1: TspSkinData;
    spSkinPopupMenu1: TspSkinPopupMenu;
    N1: TMenuItem;
    N2: TMenuItem;
    spTrayIcon1: TspTrayIcon;
    dsaSkinAdapter1: TdsaSkinAdapter;
    N3: TMenuItem;
    N4: TMenuItem;
    N5: TMenuItem;
    N6: TMenuItem;
    N7: TMenuItem;
    spSkinMainMenu1: TspSkinMainMenu;
    spSkinMainMenuBar1: TspSkinMainMenuBar;
    N333331: TMenuItem;
    N11: TMenuItem;
    N12: TMenuItem;
    N13: TMenuItem;
    ImageList1: TImageList;
    seXLabel1: TseXLabel;
    N8: TMenuItem;
    N9: TMenuItem;
    N10: TMenuItem;
    N14: TMenuItem;
    spSkinPopupMenu2: TspSkinPopupMenu;
    N16: TMenuItem;
    N17: TMenuItem;
    N18: TMenuItem;
    N19: TMenuItem;
    N20: TMenuItem;
    N21: TMenuItem;
    Image1: TImage;
    N22: TMenuItem;
    N26: TMenuItem;
    N15: TMenuItem;
    procedure N1Click(Sender: TObject);
    procedure N2Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure N5Click(Sender: TObject);
    procedure N4Click(Sender: TObject);
    procedure N6Click(Sender: TObject);
    procedure N7Click(Sender: TObject);
    procedure N333331Click(Sender: TObject);
    procedure N12Click(Sender: TObject);
    procedure N13Click(Sender: TObject);
    procedure N10Click(Sender: TObject);
    procedure N9Click(Sender: TObject);
    procedure N26Click(Sender: TObject);
    procedure N22Click(Sender: TObject);
    procedure N15Click(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
  private
    { Private declarations }
    //procedure CreateParams(var   Params:   TCreateParams);override;
    procedure ShowBallHit(Title,Caption:string);
  public
    SystemDir,WinDir: String;
    { Public declarations }
  end;

var
  Form1: TForm1;
  SkinPatch:string='skin\BlackBrilliant2\';
  AppEvEnt,SysEvent:THandle;
  
  FullSkinPatch:string;
  outputbuff:array[0..511] of Char;
  controlbuff:array[0..63] of DWORD;
  CurrentSid:string;

implementation
uses Unit2,AnSafeThr,rulepub,uWindowsVersion,GetSid;

var
 SafThr:TAnSafeTread;
{$R *.dfm}

{procedure TForm1.CreateParams(var Params:TCreateParams);
begin
      inherited;   
      Params.WndParent:=0;
end;  }

procedure TForm1.ShowBallHit(Title,Caption:string);
begin
spTrayIcon1.ShowBalloonHint(Title,Caption,spbitInfo);
end;

function XiTongHeFa:Boolean;
var
 TempVer:TWinVer;
begin
  Result:=False;
  TempVer:=GetWindowsVersion;
  if (TempVer.dwVersion=Win2000) or  (TempVer.dwVersion=WinXP) or  (TempVer.dwVersion=Win2003) or  (TempVer.dwVersion=WinVista) then
  begin
    Result:=True;
  end;
end;

procedure mmLoadInstall;
var
 Logoini:TIniFile;
 MainLogoPatch:string;
begin

 SetRuleProcessZHUData;
 SetRuleProcessZHIData;
 SetRuleRegeditZHUData;
 SetRuleRegeditZHIData;
 SetRuleSysloadZHUData;
 SetRuleSysloadZHIData;


 FullSkinPatch:=ExtractFilePath(ParamStr(0))+SkinPatch;
 Logoini:=TIniFile.Create(FullSkinPatch+'skin.ini');
 MainLogoPatch:=FullSkinPatch+Logoini.ReadString('mainlogo','logo','');
 Logoini.Free;
 Form1.Image1.Picture.LoadFromFile(MainLogoPatch);
 Form1.spSkinData1.LoadFromFile(SkinPatch+'skin.ini');
end;

procedure TForm1.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  CanClose:=False;
  Application.Minimize;
end;

procedure TForm1.FormCreate(Sender: TObject);
var
 InBuff:array[0..259]of Char;
 InEventBuff:array[0..1]of DWORD;
 TempSafePatch:string;
 SafePatch:array[0..255]of Char;
 SafePatchLen:Integer;
 outBuff:DWORD;
 OutSize,insize:DWORD;
begin
if XiTongHeFa then
begin
 mmLoadInstall;
 IO_install;
 if sysdrv.HaveLoad then
 begin
 ShowBallHit('提示','恭喜你！驱动加载成功！');
 AppEvEnt:=CreateEvent(nil,True,False,'AppWait');
 SysEvent:=CreateEvent(nil,True,False,'SysWait');

 SafThr:=TAnSafeTread.Create(False);
 ZeroMemory(@outputbuff,512);
 controlbuff[0]:=1;
 controlbuff[1]:=DWORD(@outputbuff);

 TempSafePatch:=ExtractFilePath(ParamStr(0));
 SafePatchLen:=Length(TempSafePatch);
 CopyMemory(@SafePatch,PChar(TempSafePatch),SafePatchLen);

 CopyMemory(@InBuff,@SafePatchLen,4);       //传递应用程序目录 让驱动保护
 CopyMemory(@InBuff[4],@SafePatch,256);
 insize:=260;
 OutSize:=0;
 sysdrv.IOControl(2000,@InBuff,insize,@outBuff,OutSize);

 insize:=256;                               //传递共享内存地址
 OutSize:=0;
 sysdrv.IOControl(2001,@controlbuff,insize,@controlbuff,OutSize);

 InEventBuff[0]:=AppEvEnt;                 //传递信号变量
 InEventBuff[1]:=SysEvEnt;
 insize:=8;
 OutSize:=0;
 sysdrv.IOControl(2002,@InEventBuff,insize,@outBuff,OutSize);

 insize:=0;
 OutSize:=0;
 sysdrv.IOControl(2003,nil,insize,nil,OutSize);

 CurrentSid:=GetCurrentUserSid;
 end else
 begin
   ShowBallHit('提示','出错!驱动加载失败');
 end;
end else
begin
  MessageBox(0, '非常抱歉,目前测试版本只支持2000,XP,2003,vista系统', '提示', MB_OK + 
    MB_ICONINFORMATION + MB_TOPMOST);
  ExitProcess(0);  
end;

end;

procedure TForm1.N10Click(Sender: TObject);
begin
 N10.Checked:=True;
 RuleMoshi:=0;
end;

procedure TForm1.N12Click(Sender: TObject);
begin
ShellExecute(0,'open','http://www.800.la',nil,nil,SW_NORMAL);
end;

procedure TForm1.N13Click(Sender: TObject);
begin
if Form2=nil then
begin
  Form2:=TForm2.Create(nil);
end;
Form2.ShowModal;
end;

procedure TForm1.N15Click(Sender: TObject);
var
 BuffSize:Cardinal;
 TempBuf:Pointer;
begin
BuffSize:=0;
TempBuf:=nil;
if not N15.Checked then
begin
sysdrv.IOControl(1008,TempBuf,BuffSize,TempBuf,BuffSize);
N15.Checked:=True;
ShowBallHit('提示','自我保护成功开启');
end else
begin
sysdrv.IOControl(1009,TempBuf,BuffSize,TempBuf,BuffSize);
N15.Checked:=False;
ShowBallHit('提示','自我保护关闭');
end;

end;

procedure TForm1.N1Click(Sender: TObject);
begin
Form1.Show;
end;

procedure TForm1.N22Click(Sender: TObject);
begin
ShellExecute(0,'open',PChar(ExtractFilePath(ParamStr(0))+'规则\'),nil,nil,SW_NORMAL);
end;

procedure TForm1.N26Click(Sender: TObject);
begin
 SetRuleProcessZHUData;
 SetRuleProcessZHIData;
 SetRuleRegeditZHUData;
 SetRuleRegeditZHIData;
 SetRuleSysloadZHUData;
 SetRuleSysloadZHIData;
end;

procedure TForm1.N2Click(Sender: TObject);
var
 BuffSize:Cardinal;
begin
BuffSize:=0;
sysdrv.IOControl(2004,nil,BuffSize,nil,BuffSize);
MessageBox(0, '禹盾hips已经成功安全退出！', '提示', MB_OK +MB_ICONINFORMATION);
IO_Uninstall;
spTrayIcon1.IconVisible:=False;
ExitProcess(0);
end;

procedure TForm1.N333331Click(Sender: TObject);
begin
spSkinPopupMenu1.Popup(Mouse.CursorPos.X-15,Mouse.CursorPos.Y+12);
end;

procedure TForm1.N4Click(Sender: TObject);
var
 BuffSize:Cardinal;
begin
 if not N4.Checked then
 begin
  BuffSize:=256;
  sysdrv.IOControl(1002,@controlbuff,BuffSize,@controlbuff,BuffSize);
  N4.Checked:=True;
  ShowBallHit('提示','注册表监控成功开启');
 end else
 begin
  BuffSize:=256;
  sysdrv.IOControl(1003,@controlbuff,BuffSize,@controlbuff,BuffSize);
  N4.Checked:=False;
  ShowBallHit('提示','注册表监控关闭');
 end;
end;

procedure TForm1.N5Click(Sender: TObject);
var
 BuffSize:Cardinal;
begin
 if not N5.Checked then
 begin
  BuffSize:=256;
  sysdrv.IOControl(1000,@controlbuff,BuffSize,@controlbuff,BuffSize);
  N5.Checked:=True;
  ShowBallHit('提示','进程监控成功开启');
 end else
 begin
  BuffSize:=256;
  sysdrv.IOControl(1001,@controlbuff,BuffSize,@controlbuff,BuffSize);
  N5.Checked:=False;
  ShowBallHit('提示','进程监控关闭');
 end;
end;

procedure TForm1.N6Click(Sender: TObject);
var
 BuffSize:Cardinal;
begin
 if not N6.Checked then
 begin
  BuffSize:=256;
  sysdrv.IOControl(1004,@controlbuff,BuffSize,@controlbuff,BuffSize);
  N6.Checked:=True;
  ShowBallHit('提示','驱动监控成功开启');
 end else
 begin
  BuffSize:=256;
  sysdrv.IOControl(1005,@controlbuff,BuffSize,@controlbuff,BuffSize);
  N6.Checked:=False;
  ShowBallHit('提示','进驱动监控关闭');
 end;
end;

procedure TForm1.N7Click(Sender: TObject);
var
 BuffSize:Cardinal;
 TempBuf:Pointer;
begin
BuffSize:=0;
TempBuf:=nil;
if not N7.Checked then
begin
sysdrv.IOControl(1006,TempBuf,BuffSize,TempBuf,BuffSize);
N7.Checked:=True;
ShowBallHit('提示','时间保护成功开启');
end else
begin
sysdrv.IOControl(1007,TempBuf,BuffSize,TempBuf,BuffSize);
N7.Checked:=False;
ShowBallHit('提示','时间保护关闭');
end;

end;

procedure TForm1.N9Click(Sender: TObject);
begin
 N9.Checked:=True;
 RuleMoshi:=1;
end;

end.
