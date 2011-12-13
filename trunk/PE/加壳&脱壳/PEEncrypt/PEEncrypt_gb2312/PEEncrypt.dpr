{******************************************************************************}
{ Author:    Liwuyue                                                           }
{ Email:     smokingroom@sina.com                                              }
{ Home page: http://www.programmerlife.com                                     }
{ built:     2005-03-08                                                        }
{******************************************************************************}
program PEEncrypt;

uses
  Forms,
  uMain in 'uMain.pas' {MainFrm},
  uEncrypt in 'uEncrypt.pas',
  uAbout in 'uAbout.pas' {AboutFrm};

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TMainFrm, MainFrm);
  Application.Run;
end.
