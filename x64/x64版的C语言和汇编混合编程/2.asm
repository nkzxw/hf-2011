include listing.inc

INCLUDELIB LIBCMT
INCLUDELIB OLDNAMES

PUBLIC  GetPid
EXTRN  PrintGs:PROC

_TEXT  SEGMENT
GetPid  PROC
  mov  rcx,gs
  sub  rsp,8
  call   PrintGs
  add  rsp,8
  mov  eax,gs:[64]
  ret  0
GetPid  ENDP
_TEXT  ENDS
END