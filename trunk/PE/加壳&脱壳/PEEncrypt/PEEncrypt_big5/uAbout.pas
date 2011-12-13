{******************************************************************************}
{ Author:    Liwuyue                                                           }
{ Email:     smokingroom@sina.com                                              }
{ Home page: http://www.programmerlife.com                                     }
{ built:     2005-03-08                                                        }
{******************************************************************************}
unit uAbout;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls;

type
  TAboutFrm = class(TForm)
    Label1: TLabel;
    Label2: TLabel;
    BtnOk: TButton;
    Label3: TLabel;
    Label4: TLabel;
    Bevel1: TBevel;
    Label5: TLabel;
    Image1: TImage;
    Bevel2: TBevel;
    procedure FormCreate(Sender: TObject);
    procedure Label3MouseEnter(Sender: TObject);
    procedure Label4MouseLeave(Sender: TObject);
    procedure Label3Click(Sender: TObject);
    procedure Label4Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  AboutFrm: TAboutFrm;

implementation

{$R *.dfm}

uses
  ShellApi;

procedure URLink(URL:PChar);
begin
  ShellExecute(0, nil, URL, nil, nil, SW_NORMAL);
end;

procedure TAboutFrm.FormCreate(Sender: TObject);
begin
  SetWindowLong(BtnOk.Handle,GWL_STYLE,GetWindowLong(BtnOk.handle,GWL_STYLE) or BS_FLAT);
end;

procedure TAboutFrm.Label3MouseEnter(Sender: TObject);
begin
  (Sender as TLabel).Font.Color:=clRed;
end;

procedure TAboutFrm.Label4MouseLeave(Sender: TObject);
begin
  (Sender as TLabel).Font.Color:=clBlue;
end;

procedure TAboutFrm.Label3Click(Sender: TObject);
begin
  URLink(PChar('http://www.programmerlife.com'));
end;

procedure TAboutFrm.Label4Click(Sender: TObject);
begin
  URLink(PChar('mailto:smokingroom@sina.com'));
end;

end.
