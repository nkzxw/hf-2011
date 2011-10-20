unit AnSafeThr;

interface

uses
  Classes,SysUtils,Windows,Forms,Messages,Dialogs;

type
  TAnSafeTread = class(TThread)
  private
    { Private declarations }
  protected
    procedure Execute; override;
    procedure MyMessageBox;
  end;

var
  DlgFjc,DlgMubiao:string;
  Leixing,
  messresult:Integer;
implementation
uses ShowAsk,Unit1,rulepub;



{ TAnSafeTread }
procedure TAnSafeTread.MyMessageBox;
var
StrTmp,StrTmp2: String;
I:Integer;
begin
 if ZhuIsHefu(DlgFjc,Leixing) or ZhIIsHefu(DlgMubiao,Leixing) then
 begin
  messresult:=IDYES;
 end else
 begin
  if RuleMoshi=0 then
  begin
    Form3.CheckBox1.Checked:=False;
    Form3.CheckBox2.Checked:=False;

    StrTmp:=DlgFjc;
    StrTmp2:='';
    for I := 1 to Length(StrTmp) do
    begin
     StrTmp2:=StrTmp2+copy(StrTmp,I,1);
     if (I mod 38)=0 then
     begin
      StrTmp2:=StrTmp2+#13#10
     end;
    end;
    Form3.Label4.Caption:=StrTmp2;


    StrTmp:=DlgMubiao;
    StrTmp2:='';
    for I := 1 to Length(StrTmp) do
    begin
     StrTmp2:=StrTmp2+copy(StrTmp,I,1);
     if (I mod 38)=0 then
     begin
      StrTmp2:=StrTmp2+#13#10
     end;
    end;
    Form3.Label5.Caption:=StrTmp2;

    if Leixing=0 then
    begin
     Form3.Label6.Caption:='创建进程';
    end else
    if Leixing=1 then
    begin
     Form3.Label6.Caption:='写注册表';
    end else
    if Leixing=2 then
    begin
     Form3.Label6.Caption:='加载驱动';
    end;

    //========================
    ResetEvent(WaitHandle);
    Form3.Show;
    WaitForSingleObject(WaitHandle,INFINITE);
    Sleep(50);
    messresult:=Form3.ModalResult;
    //========================

    if Form3.CheckBox1.Checked then
    begin
     AddRuleZhu(DlgFjc,Leixing);
    end;
    if Form3.CheckBox2.Checked then
    begin
     AddRuleZhI(DlgMubiao,Leixing);
    end;
  end else
  begin
    AddRuleZhI(DlgMubiao,Leixing);
    messresult:=IDYES;
  end;
 end;
end;


procedure TAnSafeTread.Execute;
var
 a:DWORD;
 Retname:PChar;
 x:Integer;
 pdest:PChar;
 Rets:Integer;
 TempBool:Boolean;
 fujinName,zjinname:string;
 czKind:Integer;
 baimingdan:Boolean;
label
 Skip;
begin
 while True do
 begin
   WaitForSingleObject(AppEvEnt,INFINITE);
   ResetEvent(AppEvEnt);
   move(outputbuff,a,4);
   TempBool:= not Boolean(a);
   if TempBool then Continue;
   Retname:=@outputbuff[8];
   pdest:=StrPos(Retname,'##');
   baimingdan:=False;
   if pdest<>nil then
   begin
     Rets:=pdest-Retname;
     fujinName:=copy(PChar(@outputbuff[8]),1,Rets);
     zjinname:=pchar(@outputbuff[Rets+10]);
     czKind:=0;
   end else
   begin
     pdest:=StrPos(Retname,'$$');
     if pdest<>nil then
     begin
      Rets:=pdest-Retname;
      fujinName:=copy(PChar(@outputbuff[8]),1,Rets);
      zjinname:=pchar(@outputbuff[Rets+10]);
      czKind:=1;
      zjinname:=GetRegDosName(zjinname);
     end else
     begin
      pdest:=StrPos(Retname,'&&');
      if pdest<>nil then
      begin
       Rets:=pdest-Retname;
       fujinName:=copy(PChar(@outputbuff[8]),1,Rets);
       zjinname:=pchar(@outputbuff[Rets+10]);
       czKind:=2;
      end;
     end;
   end;
   if not baimingdan then
   begin
     DlgFjc:=fujinName;
     DlgMubiao:=zjinname; 
     Leixing:=czKind;
     MyMessageBox;
     if messresult = IDYES then
     begin
       a:=1;
     end else
     begin
       a:=0;
     end;
   end else
   begin
       a:=1;
   end;
Skip:
   Move(a,outputbuff[4],4);
   a:=0;
   Move(a,outputbuff[0],4);
   SetEvent(SysEvent);
 end;
end;

end.
