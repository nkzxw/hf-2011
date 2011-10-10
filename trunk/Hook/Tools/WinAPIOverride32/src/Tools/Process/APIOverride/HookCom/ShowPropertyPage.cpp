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
// Object: manages the Com property page
//-----------------------------------------------------------------------------

#include "showpropertypage.h"

CShowPropertyPage::CShowPropertyPage(void)
{
}

CShowPropertyPage::~CShowPropertyPage(void)
{
}
//-----------------------------------------------------------------------------
// Name: HasPropertyPage
// Object: check is an object has a property page
// Parameters :
//     in  : IUnknown* pObject : object to test
//     out :
//     return : TRUE if object has a property page
//-----------------------------------------------------------------------------
BOOL CShowPropertyPage::HasPropertyPage(IUnknown* pObject)
{
    ISpecifyPropertyPages* pProp;
    pProp=NULL;
    // query IID_ISpecifyPropertyPages interface
    if (FAILED(CSecureIUnknown::QueryInterface(pObject,IID_ISpecifyPropertyPages, (void **)&pProp)))
        return FALSE;

    if (IsBadReadPtr(pProp,sizeof(ISpecifyPropertyPages)))
        return FALSE;

    // release increment done by QueryInterface
    CSecureIUnknown::Release(pProp);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowPropertyPage
// Object: display object property page
// Parameters :
//     in  : IUnknown* pObject : object for which we want to display propertypage
//           HWND ParentWindow : parent window handle
//           UINT x : pos x
//           UINT y : pos y
//           TCHAR* Caption : display window caption
//     out :
//     return : TRUE if object has a property page
//-----------------------------------------------------------------------------
BOOL CShowPropertyPage::ShowPropertyPage(IUnknown* pObject,HWND ParentWindow, UINT x, UINT y, TCHAR* Caption)
{
    HRESULT hRes;
    CAUUID caGUID;
    ISpecifyPropertyPages* pProp;
    WCHAR* pwcCaption;
#if (defined(UNICODE)||defined(_UNICODE))
    pwcCaption=Caption;
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Caption,&pwcCaption);
#endif

    // get ISpecifyPropertyPages interface
    pProp=NULL;
    if (FAILED(CSecureIUnknown::QueryInterface(pObject,IID_ISpecifyPropertyPages, (void **)&pProp)))
        return FALSE;

    if (IsBadReadPtr(pProp,sizeof(ISpecifyPropertyPages)))
        return FALSE;

    // get pages
    pProp->GetPages(&caGUID);
    CSecureIUnknown::Release(pProp);

    BSTR bstrCaption=SysAllocString(pwcCaption);

    // Show the page
    OCPFIPARAMS Params;
    Params.cbStructSize=sizeof(OCPFIPARAMS);
    Params.hWndOwner=NULL;//ParentWindow;FIXME doesn't work if handle is specified
    Params.x=x;
    Params.y=y;
    Params.lpszCaption=bstrCaption;
    Params.cObjects=1;
    Params.lplpUnk=(LPUNKNOWN*) &pObject;
    Params.cPages=caGUID.cElems;
    Params.lpPages=caGUID.pElems;
    Params.lcid=GetUserDefaultLCID();
    Params.dispidInitialProperty=0;

    hRes=OleCreatePropertyFrameIndirect(&Params);
    //hRes=OleCreatePropertyFrame(
    //    ParentWindow,           // Parent window
    //    0,0, //x, y,            // x,y reserved for OleCreatePropertyFrame
    //    bstrCaption,            // Caption for the dialog box
    //    1,                      // Number of objects
    //    (LPUNKNOWN*) &pObject,  // Array of object pointers. 
    //    caGUID.cElems,          // Number of property pages
    //    caGUID.pElems,          // Array of property page CLSIDs
    //    GetUserDefaultLCID(),   // Locale identifier
    //    0, NULL                 // Reserved
    //    );

    SysFreeString(bstrCaption);

    // free allocated memory
#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
    free(pwcCaption);
#endif
    CoTaskMemFree(caGUID.pElems);

    return SUCCEEDED(hRes);
}