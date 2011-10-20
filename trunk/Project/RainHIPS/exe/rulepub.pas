unit rulepub;

interface

uses
  Classes,SysUtils,Windows,Forms,Messages,Dialogs,IniFiles,StrUtils;

var
 RuleProcessZHU:array of string;
 RuleProcessZHI:array of string;
 RuleRegeditZHU:array of string;
 RuleRegeditZHI:array of string;
 RuleSysloadZHU:array of string;
 RuleSysloadZHI:array of string;

 RuleMoshi:Integer=0;//0Ϊ����ģʽ 1Ϊѧϰģʽ

function GetRegDosName(RegNtName:string):string;//ע���ת��

procedure SetRuleProcessZHUData;
procedure SetRuleProcessZHIData;
procedure SetRuleRegeditZHUData;
procedure SetRuleRegeditZHIData;
procedure SetRuleSysloadZHUData;
procedure SetRuleSysloadZHIData;

procedure AddRuleProcessZHUData(rulevalue:string);
procedure AddRuleProcessZHIData(rulevalue:string);
procedure AddRuleRegeditZHUData(rulevalue:string);
procedure AddRuleRegeditZHIData(rulevalue:string);
procedure AddRuleSysloadZHUData(rulevalue:string);
procedure AddRuleSysloadZHIData(rulevalue:string);

function ProcessZHUIsHF(rulevalue:string):Boolean;
function ProcessZHIIsHF(rulevalue:string):Boolean;
function RegeditZHUIsHF(rulevalue:string):Boolean;
function RegeditZHIIsHF(rulevalue:string):Boolean;
function SysLoadZHUIsHF(rulevalue:string):Boolean;
function SysLoadZHIIsHF(rulevalue:string):Boolean;

function ZhuIsHefu(rulevalue:string;typeid:Integer):Boolean;
function ZhIIsHefu(rulevalue:string;typeid:Integer):Boolean;

procedure AddRuleZhu(rulevalue:string;typeid:Integer);
procedure AddRuleZhI(rulevalue:string;typeid:Integer);

implementation
uses Unit1;




function GetRegDosName(RegNtName:string):string;
begin
 Result:=RegNtName;
 //Exit;
 Result:=StringReplace(Result,'\REGISTRY\USER\'+currentSid,'HKEY_CURRENT_USER',[rfReplaceAll, rfIgnoreCase]);
 Result:=StringReplace(Result,'\REGISTRY\USER','HKEY_USERS',[rfReplaceAll, rfIgnoreCase]);
 Result:=StringReplace(Result,'\REGISTRY\MACHINE\SOFTWARE\Classes','HKEY_CLASSES_ROOT',[rfReplaceAll, rfIgnoreCase]);
 Result:=StringReplace(Result,'\REGISTRY\MACHINE\SYSTEM\ControlSet001\Hardware Profiles\0001','HKEY_CURRENT_CONFIG',[rfReplaceAll, rfIgnoreCase]);
 Result:=StringReplace(Result,'\REGISTRY\MACHINE','HKEY_LOCAL_MACHINE',[rfReplaceAll, rfIgnoreCase]);
end;

procedure SetRuleProcessZHUData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'���������������б�.ini');
ListCount:=tempini.ReadInteger('�������̹���','������',0);
SetLength(RuleProcessZHU,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleProcessZHU[I]:=tempini.ReadString('�������̹���','����_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleProcessZHIData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'�������������б�.ini');
ListCount:=tempini.ReadInteger('�������̹���','������',0);
SetLength(RuleProcessZHI,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleProcessZHI[I]:=tempini.ReadString('�������̹���','����_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleRegeditZHUData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'�������ע�������б�.ini');
ListCount:=tempini.ReadInteger('ע�����̹���','������',0);
SetLength(RuleRegeditZHU,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleRegeditZHU[I]:=tempini.ReadString('ע�����̹���','����_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleRegeditZHIData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'��������ע���·��.ini');
ListCount:=tempini.ReadInteger('ע���·������','������',0);
SetLength(RuleRegeditZHI,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleRegeditZHI[I]:=tempini.ReadString('ע���·������','����_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleSysloadZHUData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'����������������б�.ini');
ListCount:=tempini.ReadInteger('�������̹���','������',0);
SetLength(RuleSysloadZHU,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleSysloadZHU[I]:=tempini.ReadString('�������̹���','����_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleSysloadZHIData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'�������������б�.ini');
ListCount:=tempini.ReadInteger('������������','������',0);
SetLength(RuleSysloadZHI,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleSysloadZHI[I]:=tempini.ReadString('������������','����_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure AddRuleProcessZHUData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'���������������б�.ini');
ListCount:=tempini.ReadInteger('�������̹���','������',0);
ListCount:=ListCount+1;
SetLength(RuleProcessZHU,ListCount);
tempini.WriteInteger('�������̹���','������',ListCount);
tempini.WriteString('�������̹���','����_'+inttostr(ListCount-1),rulevalue);
RuleProcessZHU[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleProcessZHIData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'�������������б�.ini');
ListCount:=tempini.ReadInteger('�������̹���','������',0);
ListCount:=ListCount+1;
SetLength(RuleProcessZHI,ListCount);
tempini.WriteInteger('�������̹���','������',ListCount);
tempini.WriteString('�������̹���','����_'+inttostr(ListCount-1),rulevalue);
RuleProcessZHI[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleRegeditZHUData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'�������ע�������б�.ini');
ListCount:=tempini.ReadInteger('ע�����̹���','������',0);
ListCount:=ListCount+1;
SetLength(RuleRegeditZHU,ListCount);
tempini.WriteInteger('ע�����̹���','������',ListCount);
tempini.WriteString('ע�����̹���','����_'+inttostr(ListCount-1),rulevalue);
RuleRegeditZHU[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleRegeditZHIData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'��������ע���·��.ini');
ListCount:=tempini.ReadInteger('ע���·������','������',0);
ListCount:=ListCount+1;
SetLength(RuleRegeditZHI,ListCount);
tempini.WriteInteger('ע���·������','������',ListCount);
tempini.WriteString('ע���·������','����_'+inttostr(ListCount-1),rulevalue);
RuleRegeditZHI[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleSysloadZHUData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'����������������б�.ini');
ListCount:=tempini.ReadInteger('�������̹���','������',0);
ListCount:=ListCount+1;
SetLength(RuleSysloadZHU,ListCount);
tempini.WriteInteger('�������̹���','������',ListCount);
tempini.WriteString('�������̹���','����_'+inttostr(ListCount-1),rulevalue);
RuleSysloadZHU[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleSysloadZHIData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'����\'+'�������������б�.ini');
ListCount:=tempini.ReadInteger('������������','������',0);
ListCount:=ListCount+1;
SetLength(RuleSysloadZHI,ListCount);
tempini.WriteInteger('������������','������',ListCount);
tempini.WriteString('������������','����_'+inttostr(ListCount-1),rulevalue);
RuleSysloadZHI[ListCount-1]:=rulevalue;
tempini.Free;
end;

function ProcessZHUIsHF(rulevalue:string):Boolean;
var
 I:Integer;
begin
Result:=False;
 for I := 0 to High(RuleProcessZHU) do
 begin
   if UpperCase(rulevalue)=UpperCase(RuleProcessZHU[I]) then
   begin
     Result:=True;
     Break;
   end;
 end;
end;

function ProcessZHIIsHF(rulevalue:string):Boolean;
var
 I:Integer;
begin
Result:=False;
 for I := 0 to High(RuleProcessZHI) do
 begin
   if UpperCase(rulevalue)=UpperCase(RuleProcessZHI[I]) then
   begin
     Result:=True;
     Break;
   end;
 end;
end;

function RegeditZHUIsHF(rulevalue:string):Boolean;
var
 I:Integer;
begin
Result:=False;
 for I := 0 to High(RuleRegeditZHU) do
 begin
   if UpperCase(rulevalue)=UpperCase(RuleRegeditZHU[I]) then
   begin
     Result:=True;
     Break;
   end;
 end;
end;

function RegeditZHIIsHF(rulevalue:string):Boolean;
var
 I:Integer;
begin
Result:=False;
 for I := 0 to High(RuleRegeditZHi) do
 begin
   if Pos(UpperCase(RuleRegeditZHI[I]),UpperCase(rulevalue))>0 then
   begin
     Result:=True;
     Break;
   end;
 end;
end;


function SysLoadZHUIsHF(rulevalue:string):Boolean;
var
 I:Integer;
begin
Result:=False;
 for I := 0 to High(RuleSysLoadZHU) do
 begin
   if UpperCase(rulevalue)=UpperCase(RuleSysLoadZHU[I]) then
   begin
     Result:=True;
     Break;
   end;
 end;
end;

function SysLoadZHIIsHF(rulevalue:string):Boolean;
var
 I:Integer;
begin
Result:=False;
 for I := 0 to High(RuleSysLoadZHI) do
 begin
   if UpperCase(rulevalue)=UpperCase(RuleSysLoadZHI[I]) then
   begin
     Result:=True;
     Break;
   end;
 end;
end;

function ZhuIsHefu(rulevalue:string;typeid:Integer):Boolean;
begin
  Result:=False;
  if typeid=0 then
  begin
    Result:=ProcessZHUIsHF(rulevalue);
  end else
  if typeid=1 then
  begin
    Result:=RegeditZHUIsHF(rulevalue);
  end else
  if typeid=2 then
  begin
    Result:=SysLoadZHUIsHF(rulevalue);
  end;
end;

function ZhIIsHefu(rulevalue:string;typeid:Integer):Boolean;
begin
  Result:=False;
  if typeid=0 then
  begin
    Result:=ProcessZHIIsHF(rulevalue);
  end else
  if typeid=1 then
  begin
    Result:=RegeditZHIIsHF(rulevalue);
  end else
  if typeid=2 then
  begin
    Result:=SysLoadZHIIsHF(rulevalue);
  end;
end;

procedure AddRuleZhu(rulevalue:string;typeid:Integer);
begin
  if typeid=0 then
  begin
    AddRuleProcessZHUData(rulevalue);
  end else
  if typeid=1 then
  begin
    AddRuleRegeditZHUData(rulevalue);
  end else
  if typeid=2 then
  begin
    AddRuleSysloadZHUData(rulevalue);
  end;
end;

procedure AddRuleZhI(rulevalue:string;typeid:Integer);
begin
  if typeid=0 then
  begin
    AddRuleProcessZHIData(rulevalue);
  end else
  if typeid=1 then
  begin
    AddRuleRegeditZHIData(rulevalue);
  end else
  if typeid=2 then
  begin
    AddRuleSysloadZHIData(rulevalue);
  end;
end;


end.
