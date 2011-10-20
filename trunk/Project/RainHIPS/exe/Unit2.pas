unit Unit2;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, DynamicSkinForm, dsaadapter, StdCtrls, SkinCtrls, pngimage, ExtCtrls;

type
  TForm2 = class(TForm)
    spDynamicSkinForm1: TspDynamicSkinForm;
    dsaSkinAdapter1: TdsaSkinAdapter;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    spSkinGroupBox1: TspSkinGroupBox;
    Label5: TLabel;
    Image1: TImage;
    procedure Label5Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form2: TForm2;

implementation
uses Unit1;

{$R *.dfm}

procedure TForm2.Label5Click(Sender: TObject);
begin
Close;
end;

end.
