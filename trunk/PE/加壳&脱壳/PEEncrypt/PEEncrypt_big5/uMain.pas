{******************************************************************************}
{ Author:    Liwuyue                                                           }
{ Email:     smokingroom@sina.com                                              }
{ Home page: http://www.programmerlife.com                                     }
{ built:     2005-03-08                                                        }
{******************************************************************************}
unit uMain;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Mask, Buttons;

type
  TMainFrm = class(TForm)
    GroupBox1: TGroupBox;
    EdtFileName: TEdit;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    EdtPwd: TMaskEdit;
    EdtRptPwd: TMaskEdit;
    ChxBackup: TCheckBox;
    BtnEncrypt: TButton;
    BtnAbout: TButton;
    BtnClose: TButton;
    BtnBrowse: TButton;
    procedure FormCreate(Sender: TObject);
    procedure BtnCloseClick(Sender: TObject);
    procedure BtnBrowseClick(Sender: TObject);
    procedure BtnEncryptClick(Sender: TObject);
    procedure BtnAboutClick(Sender: TObject);
  private
    function CheckInput(var FileName,Password:string; var Backup:Boolean):Boolean;
    procedure WMSysCommand(var Message:TMessage);message WM_SYSCOMMAND;
    { Private declarations }
  public
    { Public declarations }
  end;

var
  MainFrm: TMainFrm;

implementation

{$R *.dfm}

uses
  uEncrypt, uAbout;

const
  IDM_ABOUT=100;

procedure TMainFrm.WMSysCommand(var Message: TMessage);
begin
  if Message.WParam=IDM_ABOUT then
    BtnAbout.Click
  else
    inherited;
end;

procedure TMainFrm.FormCreate(Sender: TObject);
var
  hSysMenu:HMENU;
begin
  hSysMenu:=GetSystemMenu(Handle,False);
  AppendMenu(hSysMenu,MF_SEPARATOR,0,nil);
  AppendMenu(hSysMenu,MF_STRING,IDM_ABOUT,'&About PE Encrypt v1.0');
  SetWindowLong(BtnBrowse.Handle,GWL_STYLE,GetWindowLong(BtnBrowse.handle,GWL_STYLE) or BS_FLAT);
  SetWindowLong(BtnEncrypt.Handle,GWL_STYLE,GetWindowLong(BtnEncrypt.handle,GWL_STYLE) or BS_FLAT);
  SetWindowLong(BtnAbout.Handle,GWL_STYLE,GetWindowLong(BtnAbout.handle,GWL_STYLE) or BS_FLAT);
  SetWindowLong(BtnClose.Handle,GWL_STYLE,GetWindowLong(BtnClose.handle,GWL_STYLE) or BS_FLAT);
end;

procedure TMainFrm.BtnCloseClick(Sender: TObject);
begin
  Close;
end;

procedure TMainFrm.BtnBrowseClick(Sender: TObject);
begin
  with TOpenDialog.Create(Self) do
  try
    Filter:='可執行文件 (*.exe)|*.exe';
    if Execute then
    begin
      EdtFileName.Text:=FileName;
    end;
  finally
    Free;
  end;
end;

function TMainFrm.CheckInput(var FileName,Password:string; var Backup:Boolean): Boolean;
begin
  Result:=False;
  FileName:=Trim(EdtFileName.Text);
  Password:=EdtPwd.Text;
  Backup:=ChxBackup.Checked;
  if not FileExists(FileName) then
    MessageBox(Handle,'請輸入有效的文件名!',PChar(Caption),MB_ICONERROR+MB_OK)
  else if Length(Password)=0 then
    MessageBox(Handle,'請輸入密碼!',PChar(Caption),MB_ICONERROR+MB_OK)
  else if Length(EdtRptPwd.Text)=0 then
    MessageBox(Handle,'請輸入確認密碼!',PChar(Caption),MB_ICONERROR+MB_OK)
  else if EdtPwd.Text<>EdtRptPwd.Text then
    MessageBox(Handle,'確認密碼與密碼不一致!',PChar(Caption),MB_ICONERROR+MB_OK)
  else
    Result:=True;
end;

procedure TMainFrm.BtnEncryptClick(Sender: TObject);
var
  LFileName,LPassword:string;
  LBackup:Boolean;
begin
  if not CheckInput(LFileName,LPassword,LBackup) then Exit;
  Encrypt(Handle,LFileName,LPassword,LBackup);
end;

procedure TMainFrm.BtnAboutClick(Sender: TObject);
begin
  with TAboutFrm.Create(Self) do
    ShowModal; 
end;


end.
