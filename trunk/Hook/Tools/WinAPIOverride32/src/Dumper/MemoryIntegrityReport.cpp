#include "MemoryIntegrityReport.h"


extern HINSTANCE mhInstance;
extern HWND mhWndDialog;
extern TCHAR ApplicationName[MAX_PATH];
extern CListview* mpCListviewProcesses;
extern CListview* mpCListviewModules;

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

void __stdcall ModuleIntegrityCheckingHtmlElementEventsCallBack(DISPID dispidMember,WCHAR* ElementId,LPVOID UserParam)
{
    switch(dispidMember)
    {
    case DISPID_HTMLELEMENTEVENTS2_ONCLICK:
        {
            TCHAR* pszElementId;
            TCHAR* pszChangePointer;
            CHtmlViewerWindow* pHtmlViewerWindow=(CHtmlViewerWindow*)UserParam;

#if (defined(UNICODE)||defined(_UNICODE))
            pszElementId=ElementId;
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(ElementId,&pszElementId);
#endif


            if (_tcsncmp(pszElementId,HTML_REPORT_SECTION_RESTORE_PREFIX_ID,_tcslen(HTML_REPORT_SECTION_RESTORE_PREFIX_ID))==0)
            {
                CModuleIntegrityCheckingSectionChanges* pSectionChange;
                CModuleIntegrityChecking* pModuleIntegrityChecking;
                pszChangePointer=pszElementId+_tcslen(HTML_REPORT_SECTION_RESTORE_PREFIX_ID);
                if (_stscanf(pszChangePointer,_T("0x%p_0x%p"),&pModuleIntegrityChecking,&pSectionChange)!=2)
                {
#ifdef _DEBUG
                    DebugBreak();
#endif
                    return;
                }

                if (pModuleIntegrityChecking->RestoreSectionIntegrity(pSectionChange))
                {
                    // change restoration imgs and remove links
                    TCHAR StrImgIntegrityRestored[2*MAX_PATH];
                    TCHAR Str[2*MAX_PATH];
                    // forge img link
                    _stprintf(StrImgIntegrityRestored,
                        _T("<img src=\"res://%s/IMG/%d\" />"),
                        ApplicationName,IDR_IMG_INTEGRITY_RESTORED);

                    _stprintf(Str,_T("%s0x%p"),HTML_REPORT_CHANGE_CELL_PREFIX_ID,pSectionChange);
                    pHtmlViewerWindow->SetElementInnerHtml(Str,StrImgIntegrityRestored);

                    // for each change
                    CLinkListItem* pItemChange;
                    for (pItemChange=pSectionChange->pChangesLinkList->Head;pItemChange;pItemChange=pItemChange->NextItem)
                    {
                        _stprintf(Str,_T("%s0x%p"),HTML_REPORT_CHANGE_CELL_PREFIX_ID,pItemChange->ItemData);
                        pHtmlViewerWindow->SetElementInnerHtml(Str,StrImgIntegrityRestored);
                    }

                     MessageBox(pHtmlViewerWindow->GetControlHandle(),_T("Section successfully restored"),_T("Information"),MB_OK|MB_TOPMOST|MB_ICONINFORMATION);
                }


            }

            else if (_tcsncmp(pszElementId,HTML_REPORT_RESTORE_PREFIX_ID,_tcslen(HTML_REPORT_RESTORE_PREFIX_ID))==0)
            {
                CModuleIntegrityCheckingChanges* pChange;
                CModuleIntegrityChecking* pModuleIntegrityChecking;
                pszChangePointer=pszElementId+_tcslen(HTML_REPORT_RESTORE_PREFIX_ID);
                if (_stscanf(pszChangePointer,_T("0x%p_0x%p"),&pModuleIntegrityChecking,&pChange)!=2)
                {
#ifdef _DEBUG
                    DebugBreak();
#endif
                    return;
                }
                if (pModuleIntegrityChecking->RestoreIntegrity(pChange))
                {
                    TCHAR StrImgIntegrityRestored[2*MAX_PATH];
                    TCHAR Str[2*MAX_PATH];
                    _stprintf(StrImgIntegrityRestored,
                        _T("<img src=\"res://%s/IMG/%d\" />"),
                        ApplicationName,IDR_IMG_INTEGRITY_RESTORED);
                    _stprintf(Str,_T("%s0x%p"),HTML_REPORT_CHANGE_CELL_PREFIX_ID,pChange);
                    pHtmlViewerWindow->SetElementInnerHtml(Str,StrImgIntegrityRestored);
                    MessageBox(pHtmlViewerWindow->GetControlHandle(),_T("Change successfully restored"),_T("Information"),MB_OK|MB_TOPMOST|MB_ICONINFORMATION);
                }
            }

            else if ((_tcscmp(pszElementId,HTML_REPORT_SAVE_ID)==0)
                || (_tcscmp(pszElementId,HTML_REPORT_SAVE_ID_IMG)==0))
            {
                pHtmlViewerWindow->Save();
            }

            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // // to be implemented
            // else if (_tcsncmp(pszElementId,HTML_REPORT_VIEW_DISASM_PREFIX_ID,_tcslen(HTML_REPORT_VIEW_DISASM_PREFIX_ID))==0)
            // {
            //    // Show disassembly report
            // }
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
            free(pszElementId);
#endif
        }
        break;
    default:
        break;
    }
}

void PrintSection(CHtmlViewerWindow* pHtmlViewerWindow,CModuleIntegrityChecking* pModuleIntegrityChecking,CModuleIntegrityCheckingSectionChanges* pSectionChange,BOOL ContainsChanges)
{
    TCHAR Str[5*MAX_PATH];
    TCHAR RestoreSectionLink[MAX_PATH];
    TCHAR ChangesTable[MAX_PATH];
    TCHAR SectionAddress[MAX_PATH];
    TCHAR* SectionClassName;
    DWORD IdiJpg;

    // html content must be full (you can't add "<a>" and next "</a>")

    // apply style according to changes or not
    if (ContainsChanges)
    {
        SectionClassName=HTML_REPORT_INVALID_SECTION_CLASS_NAME;
        IdiJpg=IDR_IMG_INVALID_SECTION;
        _stprintf(RestoreSectionLink,
            HTMLVIEWER_DONT_SAVE_BEGIN
            _T("<a href=\"javascript:\"><img id=\"%s0x%p_0x%p\" src=\"res://%s/IMG/%d\" title=\"Restore Section\" /></a>")
            HTMLVIEWER_DONT_SAVE_END,
            HTML_REPORT_SECTION_RESTORE_PREFIX_ID,pModuleIntegrityChecking,pSectionChange,
            ApplicationName,IDR_IMG_RESTORE
            );
    }
    else
    {
        SectionClassName=HTML_REPORT_VALID_SECTION_CLASS_NAME;
        IdiJpg=IDR_IMG_VALID_SECTION;
        *RestoreSectionLink=0;
    }

    if (pSectionChange->SectionStartAddress)
    {
        _stprintf(SectionAddress,
                  _T("<td align=\"center\" nowrap=\"nowrap\">[0x%p-0x%p]</td>"),
                  pSectionChange->SectionStartAddress,
                  pSectionChange->SectionStartAddress+pSectionChange->SectionSize);
    }
    else
    {
        *SectionAddress=0;
    }
    


    // take care of % in sprintf --> %%
    _stprintf(Str,
        _T("<table align=\"center\" width=\"80%%\" cellspacing=\"0\" cellpadding=\"2\"><tr class=\"%s\"><td>")
        _T("<table width=\"100%%\"><tr><td><img src=\"res://%s/IMG/%d\" /></td>")
        _T("<td align=\"center\" width=\"100%%\">%s</td>")
        _T("%s")
        _T("<td align=\"right\" id=\"%s0x%p\">%s</td>")
        _T("</tr></table>")
        _T("</td></tr><tr><td>"),
        SectionClassName,
        ApplicationName,IdiJpg,
        pSectionChange->SectionName,
        SectionAddress,
        HTML_REPORT_CHANGE_CELL_PREFIX_ID,pSectionChange,RestoreSectionLink
        );

    // in case of changes, add table Memory / File
    if (ContainsChanges)
    {
        _stprintf(ChangesTable,
            _T("<table id=\"%s0x%p\" width=\"100%%\" cellspacing=\"0\" cellpadding=\"5\"><tr class=\"ReportFileMemoryHead\">")
            _T("<td colspan=\"2\" align=\"center\" width=\"50%%\"><b>Memory</b></td>")
            _T("<td colspan=\"2\" align=\"center\" width=\"50%%\"><b>File</b></td>")
            _T("<td></td></tr></table>"),
            pSectionChange->SectionName,pSectionChange
            );
        _tcscat(Str,ChangesTable);
    }

    _tcscat(Str,_T("</td></tr></table><br />"));

    pHtmlViewerWindow->AddHtmlContentToElement(HTML_REPORT_SECTION_ID,Str);

}

typedef struct tagThreadedPrintChangeStruct
{
    CHtmlViewerWindow* pHtmlViewerWindow;
    CModuleIntegrityCheckingSectionChanges* pSectionChange;
    CModuleIntegrityCheckingChanges* pChange;
    CModuleIntegrityChecking* pModuleIntegrityChecking;
    DWORD ChangeIndex;
}THREADED_PRINT_CHANGE,*PTHREADED_PRINT_CHANGE;
DWORD ThreadedPrintChange(PVOID UserParam);
void PrintChange(CHtmlViewerWindow* pHtmlViewerWindow,CModuleIntegrityChecking* pModuleIntegrityChecking,CModuleIntegrityCheckingSectionChanges* pSectionChange,CModuleIntegrityCheckingChanges* pChange,DWORD ChangeIndex)
{
    // we have to add row to table --> we can't use pHtmlViewerWindow->AddHtmlContentToElement
    THREADED_PRINT_CHANGE tpc;
    tpc.pHtmlViewerWindow=pHtmlViewerWindow;
    tpc.pSectionChange=pSectionChange;
    tpc.pChange=pChange;
    tpc.pModuleIntegrityChecking=pModuleIntegrityChecking;
    tpc.ChangeIndex=ChangeIndex;
    DWORD dwRes;
    pHtmlViewerWindow->ExecuteFunctionInHTMLViewerThread(ThreadedPrintChange,&tpc,&dwRes);
}
DWORD ThreadedPrintChange(PVOID UserParam)
{
    CHtmlViewerWindow* pHtmlViewerWindow;
    CModuleIntegrityCheckingSectionChanges* pSectionChange;
    CModuleIntegrityCheckingChanges* pChange;
    CModuleIntegrityChecking* pModuleIntegrityChecking;
    THREADED_PRINT_CHANGE* ptpc=(THREADED_PRINT_CHANGE*)UserParam;
    pHtmlViewerWindow=ptpc->pHtmlViewerWindow;
    pSectionChange=ptpc->pSectionChange;
    pChange=ptpc->pChange;
    pModuleIntegrityChecking=ptpc->pModuleIntegrityChecking;
    DWORD RowNumber=ptpc->ChangeIndex;

    SIZE_T Cnt;
    SIZE_T Size;
    BSTR bStr;
    WCHAR* pStr;
    WCHAR* pStr2;
    WCHAR Str[MAX_PATH];
    TCHAR SectionId[MAX_PATH];

    // we can use AddHtmlContentToElement for table object
    // so we have to take the table element object
    IHTMLElement* pElement=NULL;
    IHTMLTable* pTable=NULL;
    IHTMLTableRow* pRow=NULL;
    IHTMLElement* pRowElement=NULL;
    IHTMLTableCell* pCell=NULL;
    IHTMLElement* pCellElement=NULL;
    HRESULT hr;
    BOOL bSetColoredStyleForLine;
    BSTR bsColoredStyle=SysAllocString(L"ReportColoredLine");
    BSTR bsInsertAdjacentHTMLBeforeEnd=SysAllocString(L"beforeEnd");
    

    _stprintf(SectionId,_T("%s0x%p"),pSectionChange->SectionName,pSectionChange);
    pElement=pHtmlViewerWindow->GetHTMLElement(SectionId);
    if (!pElement)
        goto CleanUp;
    hr=pElement->QueryInterface(IID_IHTMLTable,(void**)&pTable);
    if (FAILED(hr) || (pTable==NULL))
        goto CleanUp;

    // make line color alternate
    bSetColoredStyleForLine=(RowNumber%2==1);


    // if function name informations are provided
    if (*pChange->AssociatedFunctionName)
    {
        hr=pTable->insertRow(-1,(IDispatch**)&pRow);
        if (FAILED(hr) || (pRow==NULL))
            goto CleanUp;

        if (bSetColoredStyleForLine)
        {
            hr=pRow->QueryInterface(__uuidof(IHTMLElement),(void**)&pRowElement);
            if (FAILED(hr) || (pRowElement==NULL))
                goto CleanUp;

            pRowElement->put_className(bsColoredStyle);
            pRowElement->Release();
        }

        hr=pRow->insertCell(-1,(IDispatch**)&pCell);
        if (FAILED(hr) || (pCell==NULL))
            goto CleanUp;

        pCell->put_colSpan(HTML_REPORT_NB_COLOMNS_FOR_A_CHANGE);

        hr=pCell->QueryInterface(IID_IHTMLElement,(void**)&pCellElement);
        if (FAILED(hr) || (pCellElement==NULL))
            goto CleanUp;

        

#if (defined(UNICODE)||defined(_UNICODE))
        pStr=pChange->AssociatedFunctionName;
#else
        CAnsiUnicodeConvert::AnsiToUnicode(pChange->AssociatedFunctionName,&pStr);
#endif
        swprintf(Str,MAX_PATH,L"%s&nbsp;+&nbsp;0x%p",pStr,pChange->RelativeAddressFromFunctionStart);
#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
        free(pStr);
#endif
        bStr=SysAllocString(Str);
        pCellElement->put_innerHTML(bStr);
        SysFreeString(bStr);

        pCellElement->Release();
        pCellElement=NULL;
        pCell->Release();
        pCell=NULL;
        pRow->Release();
        pRow=NULL;
    }


    // add changes
    hr=pTable->insertRow(-1,(IDispatch**)&pRow);
    if (FAILED(hr) || (pRow==NULL))
        goto CleanUp;

    if (bSetColoredStyleForLine)
    {
        hr=pRow->QueryInterface(__uuidof(IHTMLElement),(void**)&pRowElement);
        if (FAILED(hr) || (pRowElement==NULL))
            goto CleanUp;

        pRowElement->put_className(bsColoredStyle);
        pRowElement->Release();
    }

    for (int ColumnIndex=0;ColumnIndex<HTML_REPORT_NB_COLOMNS_FOR_A_CHANGE;ColumnIndex++)
    {
        hr=pRow->insertCell(-1,(IDispatch**)&pCell);
        if (FAILED(hr) || (pCell==NULL))
            goto CleanUp;

        hr=pCell->QueryInterface(IID_IHTMLElement,(void**)&pCellElement);
        if (FAILED(hr) || (pCellElement==NULL))
            goto CleanUp;

        switch(ColumnIndex)
        {
        case 0: // memory offset
            switch(pChange->ChangeType)
            {
            case CModuleIntegrityChecking::ChangesType_SectionNotInMemory:
            case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:
                break;
            case CModuleIntegrityChecking::ChangesType_Header:
                // add class to cell
                bStr=SysAllocString(L"ReportMemoryOffset");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

                // put content
                swprintf(Str,L"0x%p <br/>(PE)",pChange->MemoryOffset+(SIZE_T)pModuleIntegrityChecking->GetModuleBaseAddress());
                bStr=SysAllocString(Str);
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionNotInBinary:
            case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
                // add class to cell
                bStr=SysAllocString(L"ReportMemoryOffset");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

                // put content
                swprintf(Str,L"0x%p ",pChange->MemoryOffset+(SIZE_T)pModuleIntegrityChecking->GetModuleBaseAddress());
                bStr=SysAllocString(Str);
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_Undefined:
            case CModuleIntegrityChecking::ChangesType_None:
            default:
                goto CleanUp;
            }
            break;
        case 1:// memory data
            switch(pChange->ChangeType)
            {
            case CModuleIntegrityChecking::ChangesType_SectionNotInMemory:
                bStr=SysAllocString(L"Section not in memory");
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:
                bStr=SysAllocString(L"Memory section size less than file section size");
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionNotInBinary:
            case CModuleIntegrityChecking::ChangesType_Header:
            case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
                bStr=SysAllocString(L"ReportMemoryHexData");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

                Size=wcslen(L"&nbsp;");
                pStr=new WCHAR[(2+Size)*pChange->BufferLen+1];
                *pStr = 0; // required in case pChange->BufferLen == 0
#ifdef _DEBUG
                if (pChange->BufferLen == 0)// either a change is present but buffer len is not filled or there's no change and item shouldn't be in list
                    if (::IsDebuggerPresent())
                        ::DebugBreak();
#endif
                pStr2=pStr;
                for (Cnt=0;Cnt<pChange->BufferLen;Cnt++)
                {
                    // use unbreakable space to avoid to break DWORD 
                    if (Cnt%4==3)
                    {
                        swprintf(pStr2,L"%.2X ",pChange->MemoryBuffer[Cnt]);
                        pStr2+=3;
                    }
                    else
                    {
                        swprintf(pStr2,L"%.2X&nbsp;",pChange->MemoryBuffer[Cnt]);
                        pStr2+=2+Size;
                    }
                }
                bStr=SysAllocString(pStr);
                pCellElement->insertAdjacentHTML(bsInsertAdjacentHTMLBeforeEnd,bStr);
                SysFreeString(bStr);
                delete[] pStr;
                break;
            case CModuleIntegrityChecking::ChangesType_Undefined:
            case CModuleIntegrityChecking::ChangesType_None:
            default:
                goto CleanUp;
            }
            break;
        case 2:// file offset
            switch(pChange->ChangeType)
            {
            case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionNotInBinary:
                break;
            case CModuleIntegrityChecking::ChangesType_Header:
                bStr=SysAllocString(L"ReportFileOffset");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

                swprintf(Str,L"0x%p <br/>(PE)",pChange->FileOffset);
                bStr=SysAllocString(Str);
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionNotInMemory:
            case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
                bStr=SysAllocString(L"ReportFileOffset");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

                swprintf(Str,L"0x%p ",pChange->FileOffset);
                bStr=SysAllocString(Str);
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_Undefined:
            case CModuleIntegrityChecking::ChangesType_None:
            default:
                goto CleanUp;
            }
            break;
        case 3:// file data
            switch(pChange->ChangeType)
            {
            case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:
                bStr=SysAllocString(L"File section size less than memory section size");
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_SectionNotInBinary:
                bStr=SysAllocString(L"Section not in file");
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);
                break;
            case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionNotInMemory:
            case CModuleIntegrityChecking::ChangesType_Header:
            case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
                bStr=SysAllocString(L"ReportFileHexData");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

                Size=wcslen(L"&nbsp;");
                pStr=new WCHAR[(2+Size)*pChange->BufferLen+1];
                pStr2=pStr;
                for (Cnt=0;Cnt<pChange->BufferLen;Cnt++)
                {
                    // use unbreakable space to avoid to break DWORD 
                    if (Cnt%4==3)
                    {
                        swprintf(pStr2,L"%.2X ",pChange->FileBuffer[Cnt]);
                        pStr2+=3;
                    }
                    else
                    {
                        swprintf(pStr2,L"%.2X&nbsp;",pChange->FileBuffer[Cnt]);
                        pStr2+=2+Size;
                    }
                }
                bStr=SysAllocString(pStr);
                pCellElement->insertAdjacentHTML(bsInsertAdjacentHTMLBeforeEnd,bStr);
                SysFreeString(bStr);
                delete[] pStr;

                break;
            case CModuleIntegrityChecking::ChangesType_Undefined:
            case CModuleIntegrityChecking::ChangesType_None:
            default:
                goto CleanUp;
            }

            break;
        case 4:// restore
            switch(pChange->ChangeType)
            {
            case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
                {
                    // set content of cell
                    TCHAR StrTmp[2*MAX_PATH];
                    WCHAR* pwsc;

                    _stprintf(StrTmp,
                        _T("&nbsp;") // put_innerHTML bug : first comments are removed, so we have to add a stupid stuff before
                        // this works: pCellElement->put_innerHTML(L"&nbsp;<!-- comment1 --><!-- comment1b -->other html data<!-- comment2 -->");
                        // but not this : pCellElement->put_innerHTML(L"<!-- comment1 --><!-- comment1b -->other html data<!-- comment2 -->");
                        // be happy be M$ !!!
                        HTMLVIEWER_DONT_SAVE_BEGIN
                        _T("<a href=\"javascript:\"><img id=\"%s0x%p_0x%p\" src=\"res://%s/IMG/%d\" title=\"Restore this change\" /></a>")
                        HTMLVIEWER_DONT_SAVE_END,
                        HTML_REPORT_RESTORE_PREFIX_ID,pModuleIntegrityChecking,pChange,
                        ApplicationName,IDR_IMG_RESTORE
                        );
#if (defined(UNICODE)||defined(_UNICODE))
                    pwsc=StrTmp;
#else
                    CAnsiUnicodeConvert::AnsiToUnicode(StrTmp,&pwsc);
#endif
                    bStr=SysAllocString(pwsc);
#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
                    free(pwsc);
#endif

                    pCellElement->put_innerHTML(bStr);
                    SysFreeString(bStr);

                    // set id to cell
                    swprintf(Str,L"%s0x%p",HTML_REPORT_CHANGE_CELL_PREFIX_IDW,pChange);
                    bStr=SysAllocString(Str);
                    pCellElement->put_id(bStr);
                    SysFreeString(bStr);
                }

                break;
            case CModuleIntegrityChecking::ChangesType_Header:
            case CModuleIntegrityChecking::ChangesType_SectionNotInBinary:
            case CModuleIntegrityChecking::ChangesType_SectionNotInMemory:
            case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_Undefined:
            case CModuleIntegrityChecking::ChangesType_None:
            default:
                goto CleanUp;
            }
            break;
            /*
            to be implemented
            case 5:// show disassembly
            switch(pChange->ChangeType)
            {
            case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
            case CModuleIntegrityChecking::ChangesType_SectionNotInBinary:
            case CModuleIntegrityChecking::ChangesType_SectionNotInMemory:
            case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:
            case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:
            swprintf(Str,L"Disasm");
            bStr=SysAllocString(Str);
            pCellElement->put_innerHTML(bStr);
            SysFreeString(bStr);
            break;
            case CModuleIntegrityChecking::ChangesType_Header:
            case CModuleIntegrityChecking::ChangesType_Undefined:
            case CModuleIntegrityChecking::ChangesType_None:
            default:
            goto CleanUp;
            }    
            break;
            */
        }

        pCellElement->Release();
        pCellElement=NULL;
        pCell->Release();
        pCell=NULL;
    }

CleanUp:
    SysFreeString(bsInsertAdjacentHTMLBeforeEnd);
    SysFreeString(bsColoredStyle);
    if (pCellElement)
        pCellElement->Release();
    if (pCell)
        pCell->Release();
    if (pRow)
        pRow->Release();
    if (pTable)
        pTable->Release();
    if (pElement)
        pElement->Release();

    return 0;
}

void DisplayModuleIntegrityChecking(CLinkListSimple* pList)
{
    CloseHandle(CreateThread(0,0,DisplayModuleIntegrity,(LPVOID)pList,0,0));
}
DWORD WINAPI DisplayModuleIntegrity(PVOID lParam)
{

    TCHAR Path[2*MAX_PATH];
    CLinkListItem* pItemSection;
    CLinkListSimple* pList=(CLinkListSimple*)lParam;
    CModuleIntegrityChecking* pModuleIntegrityChecking;
    CModuleIntegrityCheckingSectionChanges* pSectionChange;
    CModuleIntegrityCheckingChanges* pChange;
    CLinkListItem* pItem;
    CLinkListItem* pItemModule;
    DWORD ChangeIndex;
    TCHAR Str[2*MAX_PATH];
    CHtmlViewerWindow* pHtmlViewerWindow;

    pHtmlViewerWindow = NULL;
    //////////////////////////
    // check if there is changes before to display report
    //////////////////////////
    BOOL Changes=FALSE;
    // for each module
    for (pItemModule=pList->Head;pItemModule;pItemModule=pItemModule->NextItem)
    {
        pModuleIntegrityChecking=(CModuleIntegrityChecking*)pItemModule->ItemData;
        // for each section
        for (pItemSection=pModuleIntegrityChecking->pSectionsChangesLinkList->Head;pItemSection;pItemSection=pItemSection->NextItem)
        {
            pSectionChange=(CModuleIntegrityCheckingSectionChanges*)pItemSection->ItemData;
            if (pSectionChange->pChangesLinkList->GetItemsCount()!=0)
            {
                Changes=TRUE;
                break;
            }
        }
        if (Changes)
            break;
    }
    if (!Changes)
    {
        // as XP Style components may not be initialized for just created thread, 
        // send a message to main dialog to show dialog box
        // doing this, we are sure themes are correctly applied
        // MessageBox(NULL,/*mhWndDialog,*/_T("Integrity has been successfully checked"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
        SendMessage(mhWndDialog,WM_SHOW_INTEGRITY_SUCCESSFULLY_CHECKED_MSG,0,0);

        goto CleanUp;
    }

    //////////////////////////
    // There is changes --> display report
    //////////////////////////

    // forge window title
    _sntprintf(Str,_countof(Str),_T("Report for process 0x%p"),(PBYTE)GetSelectedProcessId());
    // if only one module selected
    if (pList->GetItemsCount()==1)
    {
        // get module name
        mpCListviewModules->GetItemText(mpCListviewModules->GetSelectedIndex(),Path,MAX_PATH);
        // add module name to window title
        _tcscat(Str,_T(", module "));
        _tcscat(Str,CStdFileOperations::GetFileName(Path));
    }

    pHtmlViewerWindow = new CHtmlViewerWindow(HTML_REPORT_WINDOW_WITH,HTML_REPORT_WINDOW_HEIGHT);
    if (!pHtmlViewerWindow->Show(mhInstance,mhWndDialog,Str,mhInstance,IDI_ICON_APP))
        goto CleanUp;

    pHtmlViewerWindow->EnableSelection(TRUE);

    RegisterSubDialog(pHtmlViewerWindow->GetControlHandle());

    // forge resource link
    GetModuleFileName(NULL, Str, MAX_PATH);
    _sntprintf(Path,_countof(Path),_T("res://%s/%d/%d"), Str,RT_HTML, IDR_HTML_REPORT);

    // load template
    pHtmlViewerWindow->Navigate(Path);
    // wait end of template loading
    pHtmlViewerWindow->WaitForPageCompleted(NULL);
    // add events callback
    pHtmlViewerWindow->SetElementsEventsCallBack(ModuleIntegrityCheckingHtmlElementEventsCallBack,pHtmlViewerWindow);

    // forge name of precess with id
    mpCListviewProcesses->GetItemText(mpCListviewProcesses->GetSelectedIndex(),Str,MAX_PATH);
    _sntprintf(Path,_countof(Path),_T("Report for process %s (0x%p)"),Str,GetSelectedProcessId());
    pHtmlViewerWindow->SetElementInnerHtml(HTML_REPORT_PROCESS_ID,Path);

    // for each module
    for (pItemModule=pList->Head;pItemModule;pItemModule=pItemModule->NextItem)
    {
        pModuleIntegrityChecking=(CModuleIntegrityChecking*)pItemModule->ItemData;
        // add module name
        _stprintf(Str,
            _T("<div class=\"ReportFilePath\">%s :</div><br />"),
            pModuleIntegrityChecking->GetModulePath());
        pHtmlViewerWindow->AddHtmlContentToElement(HTML_REPORT_SECTION_ID,Str);

        // for each section
        for (pItemSection=pModuleIntegrityChecking->pSectionsChangesLinkList->Head;pItemSection;pItemSection=pItemSection->NextItem)
        {
            pSectionChange=(CModuleIntegrityCheckingSectionChanges*)pItemSection->ItemData;

            if (pSectionChange->pChangesLinkList->GetItemsCount()==0)
            {
                // add section to report
                PrintSection(pHtmlViewerWindow,pModuleIntegrityChecking,pSectionChange,FALSE);
            }
            else
            {
                // add section to report
                PrintSection(pHtmlViewerWindow,pModuleIntegrityChecking,pSectionChange,TRUE);

                ChangeIndex=0;
                // for each change
                for (pItem=pSectionChange->pChangesLinkList->Head;pItem;pItem=pItem->NextItem,ChangeIndex++)
                {
                    pChange=(CModuleIntegrityCheckingChanges*)pItem->ItemData;
                    // add change to report
                    PrintChange(pHtmlViewerWindow,pModuleIntegrityChecking,pSectionChange,pChange,ChangeIndex);
                }
            }
        }
    }

    pHtmlViewerWindow->WaitWindowClose();
    UnregisterSubDialog(pHtmlViewerWindow->GetControlHandle());

    //////////////////
    // free memory
    //////////////////
CleanUp:
    if (pHtmlViewerWindow)
        delete pHtmlViewerWindow;

    for (pItemModule=pList->Head;pItemModule;pItemModule=pItemModule->NextItem)
    {
        pModuleIntegrityChecking=(CModuleIntegrityChecking*)pItemModule->ItemData;
        delete pModuleIntegrityChecking;
    }
    delete pList;

    return 0;
}