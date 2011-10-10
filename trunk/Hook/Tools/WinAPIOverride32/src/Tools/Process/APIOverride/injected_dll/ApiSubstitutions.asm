;Goal of this file : assume that the following critical function wont be hooked by winapioverride
;It should be the same (or quite the same) of original api asm code


;1) Go to Project properties, select All Configurations in the Settings For list. 
;2) In "Build Event" / "Pre-Build Event"
;     put "Command Line" to MakeApiSubstitution.bat
;3)In "Linker" / "Input" add ApiSubstitution.obj to "Additional Dependencies" 

; use 486P instead of 386 for xadd instruction
.486P
.MODEL flat
ASSUME fs:NOTHING

.code

; LONG __stdcall InterlockedIncrement(volatile LONG *lpAddend)
public _InterlockedIncrementApiSubstitution@4
_InterlockedIncrementApiSubstitution@4 proc
    mov         ecx,dword ptr [esp+4] ;[lpAddend]
    mov         eax,1 
    lock xadd   dword ptr [ecx],eax 
    inc         eax  
    ret         4 
_InterlockedIncrementApiSubstitution@4 endp


; LONG __stdcall InterlockedDecrement(volatile LONG *lpAddend)
public _InterlockedDecrementApiSubstitution@4
_InterlockedDecrementApiSubstitution@4 proc
    mov         ecx,dword ptr [esp+4] ;[lpAddend]
    mov         eax,0FFFFFFFFh 
    lock xadd   dword ptr [ecx],eax 
    dec         eax  
    ret         4 
_InterlockedDecrementApiSubstitution@4 endp



; LPVOID __stdcall TlsGetValue(DWORD dwTlsIndex)
public _TlsGetValueApiSubstitution@4
_TlsGetValueApiSubstitution@4 proc
                mov     eax, dword ptr fs:[00000018h]
                mov     ecx, [esp+4] ;[dwTlsIndex]
                cmp     ecx, 40h
                jnb     LocalClearRet
                and     dword ptr [eax+34h], 0
                mov     eax, [eax+ecx*4+0E10h]
                jmp     LocalEnd
LocalClearRet:
                xor     eax, eax
LocalEnd:
                retn    4
_TlsGetValueApiSubstitution@4     endp


; BOOL __stdcall TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
public _TlsSetValueApiSubstitution@8
_TlsSetValueApiSubstitution@8 proc
                push    esi
                push    edi
                mov     eax, dword ptr fs:[00000018h]
                mov     edi, [esp+12]; [dwTlsIndex]
                cmp     edi, 40h
                mov     esi, eax
                jnb     LocalClearRet
                mov     eax, [esp+16];[lpTlsValue]
                mov     [esi+edi*4+0E10h], eax
                xor     eax, eax
                inc     eax
                jmp     LocalEnd

                LocalClearRet:
                xor     eax, eax

                LocalEnd:
                pop     edi
                pop     esi
                retn    8
_TlsSetValueApiSubstitution@8     endp


public _GetLastErrorApiSubstitution@0
_GetLastErrorApiSubstitution@0 proc
mov     eax, dword ptr fs:[00000018h]
mov     eax, [eax+34h]
retn
_GetLastErrorApiSubstitution@0 endp


public _SetLastErrorApiSubstitution@4
_SetLastErrorApiSubstitution@4 proc
mov     eax, dword ptr fs:[00000018h]
mov     ecx, [esp+4]
mov     [eax+34h], ecx
retn    4
_SetLastErrorApiSubstitution@4 endp


end