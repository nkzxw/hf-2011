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

 RuleMoshi:Integer=0;//0为正常模式 1为学习模式

function GetRegDosName(RegNtName:string):string;//注册表转换

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
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许主启动进程列表.ini');
ListCount:=tempini.ReadInteger('主动进程规则','规则数',0);
SetLength(RuleProcessZHU,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleProcessZHU[I]:=tempini.ReadString('主动进程规则','规则_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleProcessZHIData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许被启动进程列表.ini');
ListCount:=tempini.ReadInteger('被动进程规则','规则数',0);
SetLength(RuleProcessZHI,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleProcessZHI[I]:=tempini.ReadString('被动进程规则','规则_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleRegeditZHUData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许操作注册表进程列表.ini');
ListCount:=tempini.ReadInteger('注册表进程规则','规则数',0);
SetLength(RuleRegeditZHU,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleRegeditZHU[I]:=tempini.ReadString('注册表进程规则','规则_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleRegeditZHIData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许被操作注册表路径.ini');
ListCount:=tempini.ReadInteger('注册表路径规则','规则数',0);
SetLength(RuleRegeditZHI,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleRegeditZHI[I]:=tempini.ReadString('注册表路径规则','规则_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleSysloadZHUData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许加载驱动进程列表.ini');
ListCount:=tempini.ReadInteger('驱动进程规则','规则数',0);
SetLength(RuleSysloadZHU,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleSysloadZHU[I]:=tempini.ReadString('驱动进程规则','规则_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure SetRuleSysloadZHIData;
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许被加载驱动列表.ini');
ListCount:=tempini.ReadInteger('加载驱动规则','规则数',0);
SetLength(RuleSysloadZHI,ListCount);

for I := 0 to ListCount-1  do
begin
 RuleSysloadZHI[I]:=tempini.ReadString('加载驱动规则','规则_'+inttostr(I),'');
end;

tempini.Free;
end;

procedure AddRuleProcessZHUData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许主启动进程列表.ini');
ListCount:=tempini.ReadInteger('主动进程规则','规则数',0);
ListCount:=ListCount+1;
SetLength(RuleProcessZHU,ListCount);
tempini.WriteInteger('主动进程规则','规则数',ListCount);
tempini.WriteString('主动进程规则','规则_'+inttostr(ListCount-1),rulevalue);
RuleProcessZHU[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleProcessZHIData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许被启动进程列表.ini');
ListCount:=tempini.ReadInteger('被动进程规则','规则数',0);
ListCount:=ListCount+1;
SetLength(RuleProcessZHI,ListCount);
tempini.WriteInteger('被动进程规则','规则数',ListCount);
tempini.WriteString('被动进程规则','规则_'+inttostr(ListCount-1),rulevalue);
RuleProcessZHI[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleRegeditZHUData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许操作注册表进程列表.ini');
ListCount:=tempini.ReadInteger('注册表进程规则','规则数',0);
ListCount:=ListCount+1;
SetLength(RuleRegeditZHU,ListCount);
tempini.WriteInteger('注册表进程规则','规则数',ListCount);
tempini.WriteString('注册表进程规则','规则_'+inttostr(ListCount-1),rulevalue);
RuleRegeditZHU[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleRegeditZHIData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许被操作注册表路径.ini');
ListCount:=tempini.ReadInteger('注册表路径规则','规则数',0);
ListCount:=ListCount+1;
SetLength(RuleRegeditZHI,ListCount);
tempini.WriteInteger('注册表路径规则','规则数',ListCount);
tempini.WriteString('注册表路径规则','规则_'+inttostr(ListCount-1),rulevalue);
RuleRegeditZHI[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleSysloadZHUData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许加载驱动进程列表.ini');
ListCount:=tempini.ReadInteger('驱动进程规则','规则数',0);
ListCount:=ListCount+1;
SetLength(RuleSysloadZHU,ListCount);
tempini.WriteInteger('驱动进程规则','规则数',ListCount);
tempini.WriteString('驱动进程规则','规则_'+inttostr(ListCount-1),rulevalue);
RuleSysloadZHU[ListCount-1]:=rulevalue;
tempini.Free;
end;

procedure AddRuleSysloadZHIData(rulevalue:string);
var
 tempini:TIniFile;
 ListCount:Integer;
 I:Integer;
begin
tempini:=TIniFile.Create(ExtractFilePath(ParamStr(0))+'规则\'+'允许被加载驱动列表.ini');
ListCount:=tempini.ReadInteger('加载驱动规则','规则数',0);
ListCount:=ListCount+1;
SetLength(RuleSysloadZHI,ListCount);
tempini.WriteInteger('加载驱动规则','规则数',ListCount);
tempini.WriteString('加载驱动规则','规则_'+inttostr(ListCount-1),rulevalue);
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
