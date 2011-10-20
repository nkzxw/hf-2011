unit   GetSid;

interface 

uses 
    Windows,   SysUtils; 

function  GetCurrentUserSid:string; 
procedure getCurrentUserAndDomain(var User, Domain: String);
implementation 

const 
    HEAP_ZERO_MEMORY   =   $00000008; 
    SID_REVISION           =   1;   //   Current   revision   level 

type 
    PTokenUser=^TTokenUser;
    TTokenUser=packed record
    User:TSidAndAttributes;
    end; 

function ConvertSid(Sid:PSID;pszSidText:PChar;var dwBufferLen:DWORD):BOOL;
var 
    psia:   PSIDIdentifierAuthority; 
    dwSubAuthorities:   DWORD; 
    dwSidRev:   DWORD; 
    dwCounter:   DWORD; 
    dwSidSize:   DWORD; 
begin 
 Result:=False;
 dwSidRev:=SID_REVISION;
 if not IsValidSid(Sid) then Exit;
 psia:=GetSidIdentifierAuthority(Sid);
 dwSubAuthorities:=GetSidSubAuthorityCount(Sid)^;
 dwSidSize:=(15+12+(12*dwSubAuthorities)+1)*SizeOf(Char);
 if (dwBufferLen<dwSidSize)then
 begin
  dwBufferLen:=dwSidSize;
  SetLastError(ERROR_INSUFFICIENT_BUFFER);
  Exit;
 end;

 StrFmt(pszSidText,'S-%u-',[dwSidRev]);

 if (psia.Value[0]<>0)or(psia.Value[1]<>0)   then
 begin
  StrFmt(pszSidText+StrLen(pszSidText),'0x%.2x%.2x%.2x%.2x%.2x%.2x',[psia.Value[0],psia.Value[1],psia.Value[2],psia.Value[3],psia.Value[4],psia.Value[5]]);
 end else
 begin
  StrFmt(pszSidText+StrLen(pszSidText),'%u',[DWORD(psia.Value[5])+DWORD(psia.Value[4]shl 8)+DWORD(psia.Value[3] shl 16) +DWORD(psia.Value[2]shl 24)]);
 end;
  dwSidSize:=StrLen(pszSidText);

  for dwCounter:=0 to dwSubAuthorities-1 do
  begin
   StrFmt(pszSidText   +   dwSidSize,'-%u',[GetSidSubAuthority(Sid,dwCounter)^]);
   dwSidSize:=StrLen(pszSidText);
  end;

 Result:=True; 
end; 

function ObtainTextSid(hToken:THandle;pszSid:PChar;var dwBufferLen:DWORD):BOOL;
var 
 dwReturnLength:DWORD;
 dwTokenUserLength:DWORD;
 tic:TTokenInformationClass;
 ptu:Pointer;
begin 
 Result:=False;
 dwReturnLength:=0;
 dwTokenUserLength:=0;
 tic:=TokenUser;
 ptu:=nil;

 if not GetTokenInformation(hToken,tic,ptu,dwTokenUserLength,dwReturnLength)then
 begin
  if GetLastError=ERROR_INSUFFICIENT_BUFFER then
  begin
    ptu:=HeapAlloc(GetProcessHeap,   HEAP_ZERO_MEMORY,   dwReturnLength);
    if ptu=nil  then   Exit;
    dwTokenUserLength:=dwReturnLength;
    dwReturnLength:=0;

    if not GetTokenInformation(hToken,tic,ptu,dwTokenUserLength,dwReturnLength) then Exit;
  end else
  begin
    Exit;
  end;
 end;

  if not ConvertSid((PTokenUser(ptu).User).Sid,pszSid,dwBufferLen) then Exit;

  if not HeapFree(GetProcessHeap,0,ptu) then Exit;

  Result:=True;
end; 

function GetCurrentUserSid:string;
var 
 hAccessToken:THandle;
 bSuccess:BOOL;
 dwBufferLen:DWORD;
 szSid:array[0..260]of Char;
begin 
 Result:='';
 bSuccess:=False;
 bSuccess:=OpenThreadToken(GetCurrentThread,TOKEN_READ,True,hAccessToken);
 if not bSuccess then
 begin
  if GetLastError= ERROR_NO_TOKEN   then bSuccess:=OpenProcessToken(GetCurrentProcess,TOKEN_READ,hAccessToken);
 end;
 if  bSuccess then
 begin
   ZeroMemory(@szSid,SizeOf(szSid));
   dwBufferLen:=SizeOf(szSid);
   if ObtainTextSid(hAccessToken,szSid,dwBufferLen) then Result:=szSid;
   CloseHandle(hAccessToken);
 end;
end;

procedure getCurrentUserAndDomain(var User, Domain: String);
var hProcess, hAccessToken: THandle; 
    InfoBuffer: array[0..1000] of Char; 
    szAccountName, szDomainName: array [0..200] of Char; 
    dwInfoBufferSize, dwAccountSize, dwDomainSize: DWORD; 
    pUser: PTokenUser; 
    snu: SID_NAME_USE; 
begin 
    dwAccountSize:=200;
    dwDomainSize:=200; 
    hProcess:=GetCurrentProcess; 
    OpenProcessToken(hProcess,TOKEN_QUERY,hAccessToken);
    GetTokenInformation(hAccessToken,TokenUser,@InfoBuffer[0],1000,
    dwInfoBufferSize); 
    pUser:=PTokenUser(@InfoBuffer[0]); 
    LookupAccountSid(nil, pUser.User.Sid, szAccountName, dwAccountSize,    szDomainName, dwDomainSize, snu);
    User:=WideCharToString(pUser.User.Sid);
    Domain:=szDomainName; 
    CloseHandle(hAccessToken); 
end; 


end.   

