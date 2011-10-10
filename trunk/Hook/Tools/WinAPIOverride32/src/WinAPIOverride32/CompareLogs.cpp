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
// Object: manages the compare dialog
//-----------------------------------------------------------------------------

#include "CompareLogs.h"

CCompareLogs::CCompareLogs(void)
{
    this->pListview=NULL;
    this->pLog1=NULL;
    this->pLog2=NULL;
    this->pPropertyDontMatch=NULL;
    this->pPropertyDontMatchSize=0;
}

CCompareLogs::~CCompareLogs(void)
{
    if (this->pPropertyDontMatch)
    {
        delete this->pPropertyDontMatch;
        this->pPropertyDontMatch=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: create a modeless compare dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//           LOG_ENTRY* pLog1 : first log entry to compare
//           LOG_ENTRY* pLog2 : second entry to compare
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::ShowDialog(HINSTANCE hInstance,HWND hWndDialog,LOG_ENTRY* pLog1,LOG_ENTRY* pLog2)
{
    CCompareLogs* pCompareLogs=new CCompareLogs();
    pCompareLogs->pLog1=pLog1;
    pCompareLogs->pLog2=pLog2;

    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(hWndDialog);
    if (CreateDialogParam (hInstance,(LPCTSTR)IDD_DIALOG_COMPARE_LOGS,NULL,(DLGPROC)CCompareLogs::WndProc,(LPARAM)pCompareLogs))
        ShowWindow(pCompareLogs->hWndDialog,SW_SHOW);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::Init()
{
    // initialize list view
    this->pListview=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_COMPARE));
    this->pListview->AddColumn(_T("Property"),120,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("First Log Value"),220,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Second Log Value"),220,LVCFMT_LEFT);
    this->pListview->SetStyle(TRUE,TRUE,FALSE,FALSE);
    this->pListview->EnableColumnSorting(FALSE);
    this->pListview->EnableDefaultCustomDraw(FALSE);

    this->Compare();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::Close()
{
    if (this->pListview)
    {
        delete this->pListview;
        this->pListview=NULL;
    }
    DestroyWindow(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: CompareHexValue()
// Object: Make comparison between 2 hex values and add to listview
// Parameters :
//     in  : int Index : index
//           TCHAR* PropertyName : property name
//           PBYTE Value1 : first value to compare
//           PBYTE Value2 : second value to compare
//           CLinkListSimple* pPropertyDontMatchList : pointer to the current used PropertyDontMatchList object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::CompareHexValue(int Index,TCHAR* PropertyName,PBYTE Value1,PBYTE Value2,CLinkListSimple* pPropertyDontMatchList)
{
    this->CompareHexValue(Index,PropertyName,Value1,Value2,TRUE,TRUE,pPropertyDontMatchList);
}
void CCompareLogs::CompareHexValue(int Index,TCHAR* PropertyName,DWORD Value1,DWORD Value2,CLinkListSimple* pPropertyDontMatchList)
{
    this->CompareHexValue(Index,PropertyName,(PBYTE)Value1,(PBYTE)Value2,pPropertyDontMatchList);
}

//-----------------------------------------------------------------------------
// Name: CompareHexValue()
// Object: Make comparison between 2 hex values and add to listview
// Parameters :
//     in  : int Index : index
//           TCHAR* PropertyName : property name
//           PBYTE Value1 : first value to compare
//           PBYTE Value2 : second value to compare
//           BOOL bValue1Significant : TRUE if value as a meaning, FALSE else
//           BOOL bValue2Significant : TRUE if value as a meaning, FALSE else
//           CLinkListSimple* pPropertyDontMatchList : pointer to the current used PropertyDontMatchList object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::CompareHexValue(int Index,TCHAR* PropertyName,PBYTE Value1,PBYTE Value2,BOOL bValue1Significant,BOOL bValue2Significant,CLinkListSimple* pPropertyDontMatchList)
{
    TCHAR psz[MAX_PATH];
    this->pListview->SetItemText(Index,0,PropertyName);
    if (bValue1Significant)
    {
        _stprintf(psz,_T("0x%p"),Value1);
        this->pListview->SetItemText(Index,1,psz);
    }
    if (bValue2Significant)
    {
        _stprintf(psz,_T("0x%p"),Value2);
        this->pListview->SetItemText(Index,2,psz);
    }

    if (bValue1Significant&&bValue2Significant)
        pPropertyDontMatchList->AddItem((PVOID)(Value1!=Value2));
    else
        pPropertyDontMatchList->AddItem((PVOID)TRUE);
}
void CCompareLogs::CompareHexValue(int Index,TCHAR* PropertyName,DWORD Value1,DWORD Value2,BOOL bValue1Significant,BOOL bValue2Significant,CLinkListSimple* pPropertyDontMatchList)
{
    this->CompareHexValue(Index,PropertyName,(PBYTE)Value1,(PBYTE)Value2,bValue1Significant,bValue2Significant,pPropertyDontMatchList);
}

//-----------------------------------------------------------------------------
// Name: CompareParameterField()
// Object: Make comparison between 2 string and add to listview
// Parameters :
//     in  : int Index : index
//           TCHAR* PropertyName : property name
//           TCHAR* str1 : first string to compare
//           TCHAR* str2 : second string to compare
//           CLinkListSimple* pPropertyDontMatchList : pointer to the current used PropertyDontMatchList object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::CompareParameterField(int Index,TCHAR* str1,TCHAR* str2,CLinkListSimple* pPropertyDontMatchList)
{
    // make a simple comparison
    // if a function has no more parameters
    if ((str1==NULL)
        ||(str2==NULL))
    {
        // we're sure parameters don't match
        pPropertyDontMatchList->AddItem((PVOID)TRUE);
    }
    else // we have to compare them
    {
        // the easiest way is to compare results string as they are made the same way
        // if parameters are fully identical, resulting string are identical
        pPropertyDontMatchList->AddItem((PVOID)_tcsicmp(str1,str2));
    }

    // add to listview
    this->pListview->SetItemText(Index,1,str1);
    this->pListview->SetItemText(Index,2,str2);
}

//-----------------------------------------------------------------------------
// Name: Compare()
// Object: Make comparison and add to listview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::Compare()
{
    TCHAR psz[MAX_PATH];
    TCHAR* Log1ParamString;
    TCHAR* Log2ParamString;
    BYTE Cnt;
    int Index;
    BYTE MaxParam;
    BOOL AtLeastOneDoesntMatch;
    BOOL bValue1Significant;
    BOOL bValue2Significant;
    BOOL DoesntMatch;
    CLinkListSimple PropertyDontMatchList;
    CLinkListSimple* pParam1Fields;
    CLinkListSimple* pParam2Fields;
    CLinkListItem* pParam1Field;
    CLinkListItem* pParam2Field;
    TCHAR* pszParam1Field;
    TCHAR* pszParam2Field;

    // compute the number of parameters
    MaxParam=this->pLog1->pHookInfos->bNumberOfParameters;
    if (MaxParam<this->pLog2->pHookInfos->bNumberOfParameters)
        MaxParam=this->pLog2->pHookInfos->bNumberOfParameters;

    // initialize this->pPropertyDontMatch array
    Index=0;

    this->pListview->SetItemText(Index,0,_T("Api Name"));
    this->pListview->SetItemText(Index,1,this->pLog1->pszApiName);
    this->pListview->SetItemText(Index,2,this->pLog2->pszApiName);
    PropertyDontMatchList.AddItem((PVOID)(_tcsicmp(this->pLog1->pszApiName,this->pLog2->pszApiName)));
    Index++;

    this->pListview->SetItemText(Index,0,_T("Module Name"));
    this->pListview->SetItemText(Index,1,this->pLog1->pszModuleName);
    this->pListview->SetItemText(Index,2,this->pLog2->pszModuleName);
    PropertyDontMatchList.AddItem((PVOID)(_tcsicmp(this->pLog1->pszModuleName,this->pLog2->pszModuleName)));
    Index++;

    this->pListview->SetItemText(Index,0,_T("Calling Module"));
    this->pListview->SetItemText(Index,1,this->pLog1->pszCallingModuleName);
    this->pListview->SetItemText(Index,2,this->pLog2->pszCallingModuleName);
    PropertyDontMatchList.AddItem((PVOID)(_tcsicmp(this->pLog1->pszCallingModuleName,this->pLog2->pszCallingModuleName)));
    Index++;

    // empty line
    this->pListview->SetItemText(Index,0,_T(""));
    PropertyDontMatchList.AddItem((PVOID)FALSE);
    Index++;

    // Call time
    this->pListview->SetItemText(Index,0,_T("Start Time"));
    ULONGLONG ul;
    ul = (((ULONGLONG) this->pLog1->pHookInfos->CallTime.dwHighDateTime) << 32) + this->pLog1->pHookInfos->CallTime.dwLowDateTime;
    int Nano100s=(int)(ul%10);
    int MicroSeconds=(int)((ul/10)%1000);
    int MilliSeconds=(int)((ul/10000)%1000);
    int Seconds=(int)((ul/_SECOND)%60);
    int Minutes=(int)((ul/_MINUTE)%60);
    int Hours=(int)((ul/_HOUR)%24);
    _sntprintf(psz,MAX_PATH,_T("%.2u:%.2u:%.2u:%.3u:%.3u,%.1u"),
                        Hours,
                        Minutes,
                        Seconds,
                        MilliSeconds,
                        MicroSeconds,
                        Nano100s
                        );

    this->pListview->SetItemText(Index,1,psz);

    ul = (((ULONGLONG) this->pLog2->pHookInfos->CallTime.dwHighDateTime) << 32) + this->pLog2->pHookInfos->CallTime.dwLowDateTime;
    Nano100s=(int)(ul%10);
    MicroSeconds=(int)((ul/10)%1000);
    MilliSeconds=(int)((ul/10000)%1000);
    Seconds=(int)((ul/_SECOND)%60);
    Minutes=(int)((ul/_MINUTE)%60);
    Hours=(int)((ul/_HOUR)%24);
    _sntprintf(psz,MAX_PATH,_T("%.2u:%.2u:%.2u:%.3u:%.3u,%.1u"),
        Hours,
        Minutes,
        Seconds,
        MilliSeconds,
        MicroSeconds,
        Nano100s
        );

    this->pListview->SetItemText(Index,2,psz);
    DoesntMatch=FALSE;
    if ((this->pLog1->pHookInfos->CallTime.dwHighDateTime!=this->pLog2->pHookInfos->CallTime.dwHighDateTime)
        ||(this->pLog1->pHookInfos->CallTime.dwLowDateTime!=this->pLog2->pHookInfos->CallTime.dwLowDateTime)
       )
        DoesntMatch=TRUE;

    PropertyDontMatchList.AddItem((PVOID)DoesntMatch);
    Index++;

    // Call duration
    this->pListview->SetItemText(Index,0,_T("Duration (us)"));
    _sntprintf(psz,MAX_PATH,_T("%u"),this->pLog1->pHookInfos->dwCallDuration);
    this->pListview->SetItemText(Index,1,psz);
    _sntprintf(psz,MAX_PATH,_T("%u"),this->pLog2->pHookInfos->dwCallDuration);
    this->pListview->SetItemText(Index,2,psz);
    PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->dwCallDuration!=this->pLog2->pHookInfos->dwCallDuration));
    Index++;

    // process
    this->pListview->SetItemText(Index,0,_T("Process Id"));
    _sntprintf(psz,MAX_PATH,_T("0x") _T("%X") ,this->pLog1->pHookInfos->dwProcessId);
    this->pListview->SetItemText(Index,1,psz);
    _sntprintf(psz,MAX_PATH,_T("0x") _T("%X") ,this->pLog2->pHookInfos->dwProcessId);
    this->pListview->SetItemText(Index,2,psz);
    PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->dwProcessId!=this->pLog2->pHookInfos->dwProcessId));
    Index++;

    // thread
    this->pListview->SetItemText(Index,0,_T("Thread Id"));
    _sntprintf(psz,MAX_PATH,_T("0x") _T("%X") ,this->pLog1->pHookInfos->dwThreadId);
    this->pListview->SetItemText(Index,1,psz);
    _sntprintf(psz,MAX_PATH,_T("0x") _T("%X") ,this->pLog2->pHookInfos->dwThreadId);
    this->pListview->SetItemText(Index,2,psz);
    PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->dwThreadId!=this->pLog2->pHookInfos->dwThreadId));
    Index++;

    // origin address
    this->CompareHexValue(Index,
                          _T("Caller Address"),
                          this->pLog1->pHookInfos->pOriginAddress,
                          this->pLog2->pHookInfos->pOriginAddress,
                          &PropertyDontMatchList);
    Index++;

    // caller relative address
    if (this->pLog1->pHookInfos->RelativeAddressFromCallingModuleName
        ||this->pLog2->pHookInfos->RelativeAddressFromCallingModuleName)
        this->pListview->SetItemText(Index,0,_T("Caller Relative Address"));

    if (this->pLog1->pHookInfos->RelativeAddressFromCallingModuleName)
    {
        _sntprintf(psz,MAX_PATH,_T("0x%p"),this->pLog1->pHookInfos->RelativeAddressFromCallingModuleName);
        this->pListview->SetItemText(Index,1,psz);
    }
    if (this->pLog2->pHookInfos->RelativeAddressFromCallingModuleName)
    {
        _sntprintf(psz,MAX_PATH,_T("0x%p"),this->pLog2->pHookInfos->RelativeAddressFromCallingModuleName);
        this->pListview->SetItemText(Index,2,psz);
    }
    PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->RelativeAddressFromCallingModuleName
                                     !=this->pLog2->pHookInfos->RelativeAddressFromCallingModuleName));
    Index++;

    // empty line
    this->pListview->SetItemText(Index,0,_T(""));
    PropertyDontMatchList.AddItem((PVOID)FALSE);
    Index++;

    // param direction
    this->pListview->SetItemText(Index,0,_T("Logging Type"));
    switch (this->pLog1->pHookInfos->bParamDirectionType)
    {
    case PARAM_DIR_TYPE_IN:
        _tcscpy(psz,_T("In"));
        break;
    case PARAM_DIR_TYPE_OUT:
        _tcscpy(psz,_T("Out"));
        break;
    case PARAM_DIR_TYPE_IN_NO_RETURN:
        _tcscpy(psz,_T("In No Return"));
        break;
    }
    this->pListview->SetItemText(Index,1,psz);
    switch (this->pLog2->pHookInfos->bParamDirectionType)
    {
    case PARAM_DIR_TYPE_IN:
        _tcscpy(psz,_T("In"));
        break;
    case PARAM_DIR_TYPE_OUT:
        _tcscpy(psz,_T("Out"));
        break;
    case PARAM_DIR_TYPE_IN_NO_RETURN:
        _tcscpy(psz,_T("In No Return"));
        break;
    }
    this->pListview->SetItemText(Index,2,psz);
    PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->bParamDirectionType
                                     !=this->pLog2->pHookInfos->bParamDirectionType));
    Index++;

    // params
    for (Cnt=0;Cnt<MaxParam;Cnt++)
    {

        _stprintf(psz,_T("Parameter %u"),Cnt+1);
        this->pListview->SetItemText(Index,0,psz);

        Log1ParamString=NULL;
        Log2ParamString=NULL;
        pParam1Fields=NULL;
        pParam2Fields=NULL;
        if (Cnt<this->pLog1->pHookInfos->bNumberOfParameters)
        {
            // translate param to string
            CSupportedParameters::ParameterToString(this->pLog1->pszModuleName,&this->pLog1->ParametersInfoArray[Cnt],&Log1ParamString,TRUE);
            // get fields
            CSupportedParameters::SplitParameterFields(Log1ParamString,this->pLog1->ParametersInfoArray[Cnt].dwType,&pParam1Fields);
        }

        if (Cnt<this->pLog2->pHookInfos->bNumberOfParameters)
        {
            // translate param to string
            CSupportedParameters::ParameterToString(this->pLog2->pszModuleName,&this->pLog2->ParametersInfoArray[Cnt],&Log2ParamString,TRUE);
            // get fields
            CSupportedParameters::SplitParameterFields(Log2ParamString,this->pLog2->ParametersInfoArray[Cnt].dwType,&pParam2Fields);

        }

        // if both params don't contain fields
        if ((!pParam1Fields) && (!pParam2Fields))
        {
            this->CompareParameterField(Index,Log1ParamString,Log2ParamString,&PropertyDontMatchList);
            Index++;
        }
        else
        {
            // particular case for first item, cause it can compare head of list with a single value
            if (pParam1Fields)
            {
                // take first item of field list
                pParam1Field=pParam1Fields->Head;
                pszParam1Field=(TCHAR*)pParam1Field->ItemData;
                // get next field item
                pParam1Field=pParam1Field->NextItem;
            }
            else
            {
                // directly get the parameter string
                pszParam1Field=Log1ParamString;
                pParam1Field=NULL;
            }

            if (pParam2Fields)
            {
                // take first item of field list
                pParam2Field=pParam2Fields->Head;
                pszParam2Field=(TCHAR*)pParam2Field->ItemData;
                // get next field item
                pParam2Field=pParam2Field->NextItem;
            }
            else
            {
                // directly get the parameter string
                pszParam2Field=Log2ParamString;
                pParam2Field=NULL;
            }

            // compare first values
            this->CompareParameterField(Index,pszParam1Field,pszParam2Field,&PropertyDontMatchList);
            Index++;

            // while there's field in one list
            while(pParam1Field||pParam2Field)
            {
                // try to take next field in each list
                if (pParam1Field)
                    pszParam1Field=(TCHAR*)pParam1Field->ItemData;
                else
                    pszParam1Field=NULL;
                
                if (pParam2Field)
                    pszParam2Field=(TCHAR*)pParam2Field->ItemData;
                else
                    pszParam2Field=NULL;

                // compare the 2 fields
                this->CompareParameterField(Index,pszParam1Field,pszParam2Field,&PropertyDontMatchList);
                Index++;

                // get next fields
                if (pParam1Field)
                    pParam1Field=pParam1Field->NextItem;
                if (pParam2Field)
                    pParam2Field=pParam2Field->NextItem;
            }

            // free parameters lists
            if (pParam1Fields)
                CSupportedParameters::FreeSplittedParameterFields(&pParam1Fields);
            if (pParam2Fields)
                CSupportedParameters::FreeSplittedParameterFields(&pParam2Fields);
        }

        if (Log1ParamString)
            // free string allocated by ParameterToString
            delete Log1ParamString;
        if (Log2ParamString)
            // free string allocated by ParameterToString
            delete Log2ParamString;
    }

    // ret value

    // if at least one func call has a return value
    if ((this->pLog1->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN)
        ||(this->pLog2->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN))
    {
        // empty line
        this->pListview->SetItemText(Index,0,_T(""));
        PropertyDontMatchList.AddItem((PVOID)FALSE);
        Index++;

        this->pListview->SetItemText(Index,0,_T("Returned Value"));

        // if log1 has a return value
        if (this->pLog1->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN)
        {
            _sntprintf(psz,MAX_PATH,_T("0x%p"),this->pLog1->pHookInfos->ReturnValue);
            this->pListview->SetItemText(Index,1,psz);

            // log1 log floating ret value
            _stprintf(psz,_T("%.19g"), this->pLog1->pHookInfos->DoubleResult);
            this->pListview->SetItemText(Index+1,0,_T("Floating Return Value"));
            this->pListview->SetItemText(Index+1,1,psz);

            // log1 log last error
            _sntprintf(psz,MAX_PATH,_T("0x%.8X"),this->pLog1->pHookInfos->dwLastError);
            this->pListview->SetItemText(Index+2,0,_T("Last Error Value"));
            this->pListview->SetItemText(Index+2,1,psz);
        }
        // if log2 has a return value
        if (this->pLog2->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN)
        {
            _sntprintf(psz,MAX_PATH,_T("0x%p"),this->pLog2->pHookInfos->ReturnValue);
            this->pListview->SetItemText(Index,2,psz);

            // log2 log floating ret value
            _stprintf(psz,_T("%.19g"), this->pLog2->pHookInfos->DoubleResult);
            this->pListview->SetItemText(Index+1,0,_T("Floating Return Value"));
            this->pListview->SetItemText(Index+1,2,psz);

            // if log2 log last error
            _sntprintf(psz,MAX_PATH,_T("0x%.8X"),this->pLog2->pHookInfos->dwLastError);
            this->pListview->SetItemText(Index+2,0,_T("Last Error Value"));
            this->pListview->SetItemText(Index+2,2,psz);

        }
        // if one log dosen't have a valid return value
        if ((this->pLog1->pHookInfos->bParamDirectionType==PARAM_DIR_TYPE_IN_NO_RETURN)
            ||(this->pLog2->pHookInfos->bParamDirectionType==PARAM_DIR_TYPE_IN_NO_RETURN))
        {
            // we're sure property doesn't match
            PropertyDontMatchList.AddItem((PVOID)TRUE);
            PropertyDontMatchList.AddItem((PVOID)TRUE);
            PropertyDontMatchList.AddItem((PVOID)TRUE);
        }
        else// we have to compare return value
        {
            // compare ret value
            PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->ReturnValue!=this->pLog2->pHookInfos->ReturnValue));

            // compare floating ret value
            // don't work if fields are not defined
            // this->pPropertyDontMatch[Index+1]=(this->pLog1->pHookInfos->DoubleResult!=this->pLog2->pHookInfos->DoubleResult);
            // so make direct memory compare
            PropertyDontMatchList.AddItem((PVOID)(memcmp(&this->pLog1->pHookInfos->DoubleResult,&this->pLog2->pHookInfos->DoubleResult,sizeof(double))));

            // compare last error value
            PropertyDontMatchList.AddItem((PVOID)(this->pLog1->pHookInfos->dwLastError!=this->pLog2->pHookInfos->dwLastError));
        }
        Index++;// increment index for returned value
        Index++;// increment index for floating return value
        Index++;// increment index for last error
    }

    // empty line
    this->pListview->SetItemText(Index,0,_T(""));
    PropertyDontMatchList.AddItem((PVOID)FALSE);
    Index++;

    // registers before call
    this->pListview->SetItemText(Index,0,_T("Registers Before Call"));
    PropertyDontMatchList.AddItem((PVOID)FALSE);
    Index++;


    this->CompareHexValue(Index,
                          _T("EAX"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.eax,
                          this->pLog2->pHookInfos->RegistersBeforeCall.eax,
                          &PropertyDontMatchList);
    Index++;

    this->CompareHexValue(Index,
                          _T("EBX"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.ebx,
                          this->pLog2->pHookInfos->RegistersBeforeCall.ebx,
                          &PropertyDontMatchList);
    Index++;

    this->CompareHexValue(Index,
                          _T("ECX"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.ecx,
                          this->pLog2->pHookInfos->RegistersBeforeCall.ecx,
                          &PropertyDontMatchList);
    Index++;

    this->CompareHexValue(Index,
                          _T("EDX"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.edx,
                          this->pLog2->pHookInfos->RegistersBeforeCall.edx,
                          &PropertyDontMatchList);
    Index++;

    this->CompareHexValue(Index,
                          _T("ESI"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.esi,
                          this->pLog2->pHookInfos->RegistersBeforeCall.esi,
                          &PropertyDontMatchList);
    Index++;

    this->CompareHexValue(Index,
                          _T("EDI"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.edi,
                          this->pLog2->pHookInfos->RegistersBeforeCall.edi,
                          &PropertyDontMatchList);
    Index++;

    this->CompareHexValue(Index,
                          _T("EFL"),
                          this->pLog1->pHookInfos->RegistersBeforeCall.efl,
                          this->pLog2->pHookInfos->RegistersBeforeCall.efl,
                          &PropertyDontMatchList);
    Index++;

    // empty line
    this->pListview->SetItemText(Index,0,_T(""));
    PropertyDontMatchList.AddItem((PVOID)FALSE);
    Index++;

    // registers after call
    // if one log have a valid return value
    if ((this->pLog1->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN)
     ||(this->pLog2->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN))
    {

        bValue1Significant=(this->pLog1->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN);
        bValue2Significant=(this->pLog2->pHookInfos->bParamDirectionType!=PARAM_DIR_TYPE_IN_NO_RETURN);
        this->pListview->SetItemText(Index,0,_T("Registers After Call"));
        PropertyDontMatchList.AddItem((PVOID)FALSE);
        Index++;


        this->CompareHexValue(Index,
                            _T("EAX"),
                            this->pLog1->pHookInfos->RegistersAfterCall.eax,
                            this->pLog2->pHookInfos->RegistersAfterCall.eax,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;

        this->CompareHexValue(Index,
                            _T("EBX"),
                            this->pLog1->pHookInfos->RegistersAfterCall.ebx,
                            this->pLog2->pHookInfos->RegistersAfterCall.ebx,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;

        this->CompareHexValue(Index,
                            _T("ECX"),
                            this->pLog1->pHookInfos->RegistersAfterCall.ecx,
                            this->pLog2->pHookInfos->RegistersAfterCall.ecx,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;

        this->CompareHexValue(Index,
                            _T("EDX"),
                            this->pLog1->pHookInfos->RegistersAfterCall.edx,
                            this->pLog2->pHookInfos->RegistersAfterCall.edx,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;

        this->CompareHexValue(Index,
                            _T("ESI"),
                            this->pLog1->pHookInfos->RegistersAfterCall.esi,
                            this->pLog2->pHookInfos->RegistersAfterCall.esi,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;

        this->CompareHexValue(Index,
                            _T("EDI"),
                            this->pLog1->pHookInfos->RegistersAfterCall.edi,
                            this->pLog2->pHookInfos->RegistersAfterCall.edi,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;

        this->CompareHexValue(Index,
                            _T("EFL"),
                            this->pLog1->pHookInfos->RegistersAfterCall.efl,
                            this->pLog2->pHookInfos->RegistersAfterCall.efl,
                            bValue1Significant,
                            bValue2Significant,
                            &PropertyDontMatchList);
        Index++;
    }

    this->pPropertyDontMatch=(BOOL*)PropertyDontMatchList.ToArray(&this->pPropertyDontMatchSize);

    AtLeastOneDoesntMatch=FALSE;
    for (Cnt=1;Cnt<this->pPropertyDontMatchSize;Cnt++)
    {
        AtLeastOneDoesntMatch=AtLeastOneDoesntMatch||this->pPropertyDontMatch[Cnt];
    }
    // if all parameter match
    if(!AtLeastOneDoesntMatch)
    {
        MessageBox(this->hWndDialog,_T("Logs are identicals"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    }
}
//-----------------------------------------------------------------------------
// Name: Resize
// Object: Resize dialog box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCompareLogs::Resize()
{
    RECT Rect;
    RECT RectWindow;

    HWND hItem=GetDlgItem(this->hWndDialog,IDC_LIST_COMPARE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndDialog,&RectWindow);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,0,
        RectWindow.right-RectWindow.left-2*(Rect.left)-5,
        RectWindow.bottom-RectWindow.top-2*(Rect.top)-30,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);
}
//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CCompareLogs::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CCompareLogs* pCompareLogs;
            pCompareLogs=(CCompareLogs*)lParam;
            pCompareLogs->hWndDialog=hWnd;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pCompareLogs);

            pCompareLogs->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_COMPARE);
           
            pCompareLogs->Resize();
        }
        break;
    case WM_CLOSE:
        ((CCompareLogs*)GetWindowLongPtr(hWnd,GWLP_USERDATA))->Close();
        break;
    case WM_NOTIFY:
        {
            CCompareLogs* pCompareLogs=((CCompareLogs*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pCompareLogs)
                break;
            if (pCompareLogs->pListview)
            {
                LPNMLISTVIEW pnm;
                pnm = (LPNMLISTVIEW)lParam;

                // send notification for copy & save
                if (pCompareLogs->pListview->OnNotify(wParam,lParam))
                    break;

                // color notifications
                if (pnm->hdr.hwndFrom == pCompareLogs->pListview->GetControlHandle() &&
                    pnm->hdr.code == NM_CUSTOMDRAW)
                {
                    //must use SetWindowLongPtr to return value from dialog proc
                    LRESULT lResult=pCompareLogs->ProcessListViewCustomDraw(lParam);
                    SetWindowLongPtr(hWnd, DWLP_MSGRESULT, lResult);
                    return TRUE;
                }
            }
        }
        break;
    case WM_DESTROY:
        {
        CCompareLogs* pCompareLogs=((CCompareLogs*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
        if (!pCompareLogs)
            break;
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)0);
        delete pCompareLogs;
        return FALSE;
        }
    case WM_SIZE:
        {
            CCompareLogs* pCompareLogs=((CCompareLogs*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pCompareLogs)
                break;
            pCompareLogs->Resize();
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: ProcessCustomDrawListViewDetails
// Object: Handle NM_CUSTOMDRAW message (set listview details colors)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CCompareLogs::ProcessListViewCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
    switch(lplvcd->nmcd.dwDrawStage) 
    {
        case CDDS_PREPAINT : //Before the paint cycle begins
            //request notifications for individual listview items
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT: //Before an item is drawn
            // lplvcd->nmcd.dwItemSpec // item index
            // to request notification for subitems
            return CDRF_NOTIFYSUBITEMDRAW;
        // for subitem customization
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
        // lplvcd->nmcd.dwItemSpec // item number
        // lplvcd->iSubItem // subitem number

        // customize subitem appearance for column 0
        if (lplvcd->iSubItem==0)
        {
            // lplvcd->clrText   = RGB(255,255,255);
            lplvcd->clrTextBk = CCOMPARELOGS_PROPERTIES_BACKGROUND_COLOR;
        }
        else
        {
            // default back ground color to white
            lplvcd->clrTextBk =RGB(255,255,255);

            if (this->pPropertyDontMatch)
            {
                if (lplvcd->nmcd.dwItemSpec<this->pPropertyDontMatchSize)
                {
                    // if propertie is not the same
                    if (this->pPropertyDontMatch[lplvcd->nmcd.dwItemSpec])
                        lplvcd->clrTextBk =CCOMPARELOGS_DIFFERENT_VALUES_COLOR;
                }
            }
        }

        //else
        return CDRF_DODEFAULT;
    }
    return CDRF_DODEFAULT;
}