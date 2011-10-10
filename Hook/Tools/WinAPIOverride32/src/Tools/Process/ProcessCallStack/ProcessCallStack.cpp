/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: class helper for getting process call stack
//-----------------------------------------------------------------------------

#include "ProcessCallStack.h"

//-----------------------------------------------------------------------------
// Name: GetCallStack
// Object: check if a process is still running 
// Parameters :
//     in : DWORD ProcessID : ID of process
//          PVOID EBP : current ebp of software at the moment we want to get call stack
//     in out : CLinkList* pCallStackItemList : linked list of CALLSTACK_ITEM (must be allocated and free by caller)
// Return : TRUE on success, else FALSE 
//-----------------------------------------------------------------------------
BOOL CProcessCallStack::GetCallStack(DWORD ProcessId,PVOID EBP,OUT CLinkList* pCallStackItemList)
{
    // first retrieve list of modules
    CLinkList ModulesEntryList(sizeof(MODULEENTRY32));
    CLinkListItem* pModuleEntryItem;
    MODULEENTRY32* pModuleEntry;
    CLinkListItem* pItem;
    CALLSTACK_ITEM* pCallStackItem;

    MODULEENTRY32 me32 = {0}; 
    HANDLE hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,ProcessId);

    if (hModuleSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        return FALSE; 
    }
    do 
    {
        ModulesEntryList.AddItem(&me32);
    } 
    while (Module32Next(hModuleSnap, &me32)); 

    // parse call stack
    PVOID PreviousEbp;
    PVOID RetAddress=0;
    BYTE ParametersPointer[CALLSTACK_EBP_RETRIEVAL_SIZE];
    SIZE_T NbBytesRead;
    PBYTE pBuffer[__max(REGISTER_BYTE_SIZE,CALLSTACK_EBP_RETRIEVAL_SIZE)];
    PVOID AddressToRead;
    BOOL  bReadSuccess;
    SIZE_T SizeToRead;
    BYTE LocalPseudoRetAddressBuffer[CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE+1];

    CProcessMemory ProcessMemory(ProcessId,TRUE,FALSE);

    bReadSuccess=TRUE;
    while (bReadSuccess && EBP) // while read success (valid remote process pointer) and ebp!=0
    {
        // get previous ebp (call -1 ebp) PreviousEbp=*(DWORD*)(EBP);
        AddressToRead=EBP;
        bReadSuccess=ProcessMemory.Read(AddressToRead,pBuffer,REGISTER_BYTE_SIZE,&NbBytesRead);
        memcpy(&PreviousEbp,pBuffer,REGISTER_BYTE_SIZE);
        
        // if no previous ebp
        if ((!bReadSuccess) || (PreviousEbp==0))
            // stop parsing call stack
            break;

        // return address is at current EBP+REGISTER_BYTE_SIZE RetAddress=*(DWORD*)(EBP+REGISTER_BYTE_SIZE);
        // so get it
        AddressToRead=(PVOID)((PBYTE)EBP+REGISTER_BYTE_SIZE);
        bReadSuccess=ProcessMemory.Read(AddressToRead,pBuffer,REGISTER_BYTE_SIZE,&NbBytesRead);
        memcpy(&RetAddress,pBuffer,REGISTER_BYTE_SIZE);

        // check if return address is a valid one
        bReadSuccess=ProcessMemory.Read(RetAddress,pBuffer,REGISTER_BYTE_SIZE,&NbBytesRead);
        if (!bReadSuccess)
            // if not the case break
            break;

        // get pointer to parameters (params are at ebp+2*REGISTER_BYTE_SIZE
        // AddressToRead=(PVOID)((PBYTE)EBP+2*REGISTER_BYTE_SIZE);
        // it's interesting to link retAddress with previous ebp parameters, because we get caller function with caller function parameters
        AddressToRead=(PVOID)((PBYTE)PreviousEbp+2*REGISTER_BYTE_SIZE);

        memset(ParametersPointer,0,CALLSTACK_EBP_RETRIEVAL_SIZE);
        // reducing size memory checker loop
        SizeToRead=CALLSTACK_EBP_RETRIEVAL_SIZE;
        do 
        {
            bReadSuccess=ProcessMemory.Read(AddressToRead,pBuffer,CALLSTACK_EBP_RETRIEVAL_SIZE,&NbBytesRead);
            if (SizeToRead>REGISTER_BYTE_SIZE)// avoid buffer underflow
                SizeToRead-=REGISTER_BYTE_SIZE;
            else
                SizeToRead=0;
        } while((!bReadSuccess)&&SizeToRead);
        if (SizeToRead)
            memcpy(ParametersPointer,pBuffer,SizeToRead);

        // stack coherence checking : 
        // function having PreviousEbp has been called by function having EBP (we walk stack backward)
        // so PreviousEbp MUSTE BE greater or equal to EBP
        // and there if it's equal, we can't guess previous function ebp --> we have to stop
        if (EBP>=PreviousEbp)
            break;

        // update ebp
        EBP=PreviousEbp;


        // put pCallStackItem to default
        pItem=pCallStackItemList->AddItem();
        pCallStackItem=(CALLSTACK_ITEM*)pItem->ItemData;
        *(pCallStackItem->CallingModuleName)=0;
        pCallStackItem->CallingRelativeAddressFromModuleBase=0;
        // fill pCallStackItem address (and adjust return address to call addr)
        pCallStackItem->CallingAddress=RetAddress;
        // we should have to do pCallStackItem->CallingAddress-=GetCallInstructionSizeFromReturnAddress(RetAddress)
        // but RetAddress is in remote process, and GetCallInstructionSizeFromReturnAddress needs
        // CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE before ret address
        // so read in remote process CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE before the ret address
        // and pass &LocalPseudoRetAddressBuffer[CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE]
        // as parameter to GetCallInstructionSizeFromReturnAddress
        if (ProcessMemory.Read((PBYTE)RetAddress-CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE,
                                &LocalPseudoRetAddressBuffer,
                                CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE,
                                &NbBytesRead)
            )
        {
            pCallStackItem->CallingAddress=(PVOID)((PBYTE)pCallStackItem->CallingAddress-
            CCallInstructionSizeFromReturnAddress::GetCallInstructionSizeFromReturnAddress(
                &LocalPseudoRetAddressBuffer[CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE]));
        }
        // fill ParametersPointer
        memcpy(pCallStackItem->ParametersPointer,ParametersPointer,CALLSTACK_EBP_RETRIEVAL_SIZE);

        // loop throw module list to know the calling module
        pModuleEntryItem=ModulesEntryList.Head;
        while(pModuleEntryItem)
        {
            pModuleEntry=(MODULEENTRY32*)pModuleEntryItem->ItemData;

            // check if current module is the calling one
            if ((pModuleEntry->modBaseAddr<=pCallStackItem->CallingAddress)
                &&(pCallStackItem->CallingAddress<=pModuleEntry->modBaseAddr+pModuleEntry->modBaseSize))
            {
                // we have found calling module
                _tcscpy(pCallStackItem->CallingModuleName,pModuleEntry->szExePath);
                pCallStackItem->CallingRelativeAddressFromModuleBase=(PVOID)((PBYTE)pCallStackItem->CallingAddress-pModuleEntry->modBaseAddr);

                // go out of module list loop
                break;
            }
            pModuleEntryItem=pModuleEntryItem->NextItem;
        }
    }
    CloseHandle(hModuleSnap);
    return TRUE;
}