unit ShowAsk;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, DynamicSkinForm, dsaadapter, ExtCtrls;

type
  TForm3 = class(TForm)
    Label1: TLabel;
    Label3: TLabel;
    Button1: TButton;
    Button2: TButton;
    CheckBox1: TCheckBox;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    spDynamicSkinForm1: TspDynamicSkinForm;
    dsaSkinAdapter1: TdsaSkinAdapter;
    Label2: TLabel;
    CheckBox2: TCheckBox;
    Timer1: TTimer;
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
  private
    { Private declarations }
    procedure CreateParams(var   Params:   TCreateParams);override;
  public
    { Public declarations }
  end;

var
  Form3:TForm3;
  WaitHandle:THandle;
implementation
uses Unit1;

{$R *.dfm}

procedure TForm3.CreateParams(var Params:TCreateParams);
begin
      inherited;   
      Params.WndParent:=0;
end;

procedure TForm3.Button1Click(Sender: TObject);
begin
ModalResult:=IDYES;
Close;
end;

procedure TForm3.Button2Click(Sender: TObject);
begin
ModalResult:=IDCANCEL;
Close;
end;

procedure TForm3.FormClose(Sender: TObject; var Action: TCloseAction);
begin
SetEvent(WaitHandle);
end;

procedure TForm3.FormCreate(Sender: TObject);
begin
WaitHandle:=CreateEvent(nil,True,False,'okoksys');
end;

procedure TForm3.Timer1Timer(Sender: TObject);
begin
 if Visible then
 begin
  SetForegroundWindow(handle);
  SetActiveWindow(handle);
  ShowWindow(Application.Handle,SW_HIDE)
 end;
end;

end.
