// shell.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//�������봦�Թ�����Դ
#pragma region ��ʾ�Ҽ��˵����

extern IContextMenu2* g_pIContext2;
extern IContextMenu3* g_pIContext3;
extern WNDPROC oldWndProc;
static inline LRESULT CALLBACK HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_MENUCHAR: // only supported by IContextMenu3
        if (g_pIContext3) {
            LRESULT lResult = 0;
            g_pIContext3->HandleMenuMsg2(message, wParam, lParam, &lResult);
            return(lResult);
        }
        break;
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        if (wParam) {
            break; // if wParam != 0 then the message is not menu-related
        }
    case WM_INITMENUPOPUP:
        if (g_pIContext2) {
            g_pIContext2->HandleMenuMsg(message, wParam, lParam);
        }
        else {
            g_pIContext3->HandleMenuMsg(message, wParam, lParam);
        }
        return(message == WM_INITMENUPOPUP ? 0 : TRUE);
        break;
    default:
        break;
    }
    return ::CallWindowProc(oldWndProc,hWnd,message,wParam,lParam);//??oldWndProc(hWnd, message, wParam, lParam);
}

template<typename __PIUNKNOW>
inline void SafeRelease(__PIUNKNOW &pInterface){
    if(pInterface!=NULL) {
        pInterface->Release();
        pInterface=NULL;
    }
}

inline BOOL ShowContextMenu(HWND hWnd,LPSHELLFOLDER lpsfParent, LPITEMIDLIST lpi)
{
    BOOL bRet=TRUE;

    LPCONTEXTMENU spContextMenu=NULL;
    LPCONTEXTMENU pCtxMenuTemp = NULL;
    int menuType = 0;
    g_pIContext2 = NULL;
    g_pIContext3 = NULL;

    HRESULT hr = lpsfParent->GetUIObjectOf(hWnd,1, (const struct _ITEMIDLIST**)&lpi, IID_IContextMenu, 0, (LPVOID*)&pCtxMenuTemp);
    if(FAILED(hr)) {
        bRet=FALSE;
        goto exit;
    }

    if(pCtxMenuTemp->QueryInterface(IID_IContextMenu3, (void**) &spContextMenu)==NO_ERROR)
    {
        menuType=3;
    }
    else if(pCtxMenuTemp->QueryInterface(IID_IContextMenu2, (void**) &spContextMenu)==NO_ERROR)
    {
        menuType=2;
    }
    else if(pCtxMenuTemp->QueryInterface(IID_IContextMenu, (void**) &spContextMenu)==NO_ERROR)
    {
        menuType=1;
    }

    HMENU hMenu = ::CreatePopupMenu();
    if(hMenu == NULL){
        bRet=FALSE;
        goto exit;
    }

    // Get the context menu for the item.
    hr = spContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, 
        CMF_EXPLORE     //��Դ������
        /*|CMF_CANRENAME*/  //֧��������
        |CMF_NORMAL
        );
    if(FAILED(hr)){
        bRet=FALSE;
        goto exit;
    }

    oldWndProc = NULL;
    if (menuType > 1) 
    {
        oldWndProc = (WNDPROC)::SetWindowLongPtr(hWnd,GWL_WNDPROC, (LONG) HookWndProc);
        if (menuType == 2) {
            g_pIContext2 = (LPCONTEXTMENU2) spContextMenu;
        }
        else {
            g_pIContext3 = (LPCONTEXTMENU3) spContextMenu;
        }
        spContextMenu->AddRef();
    }

    POINT pt;
    ::GetCursorPos(&pt);  
    int idCmd = ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);

    if (oldWndProc) 
    {
        SetWindowLongPtr(hWnd,GWL_WNDPROC, (LONG) oldWndProc);
    }

    if (idCmd != 0)
    {
        // ִ��ѡ�������
        CMINVOKECOMMANDINFO cmi = { 0 };
        cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
        cmi.fMask = 0;
        cmi.hwnd = hWnd;
        cmi.lpVerb = LPCSTR(MAKEINTRESOURCE(idCmd - 1));
        cmi.lpParameters = NULL;
        cmi.lpDirectory = NULL;
        cmi.nShow = SW_SHOWNORMAL;
        cmi.dwHotKey = 0;
        cmi.hIcon = NULL;
        hr = spContextMenu->InvokeCommand(&cmi);
    }

exit:
    ::DestroyMenu(hMenu);
    SafeRelease(spContextMenu);
    SafeRelease(pCtxMenuTemp);
    SafeRelease(g_pIContext2);
    SafeRelease(g_pIContext3);
    return bRet;
}

#pragma endregion





//�򿪿�ݷ�ʽ
//wszShortCutName ��ݷ�ʽȫ·����������
//L"C:\\Documents and Settings\\Administrator\\����\\��ѶQQ.lnk"
//������ L"��ѶQQ.lnk" ���� L"��ѶQQ"
inline BOOL Help_OpenShortCut(PWCHAR wszShortCutName)
{
    HRESULT hr=CoInitialize(NULL);
    if(SUCCEEDED(hr))
    {
        IShellLink  * pShLink; 
        hr=CoCreateInstance(CLSID_ShellLink,NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (PVOID*)&pShLink );
        if(SUCCEEDED(hr))
        {
            IPersistFile * ppf;
            hr=pShLink->QueryInterface(IID_IPersistFile,(PVOID*)&ppf);
            if(SUCCEEDED(hr))
            {
                hr=ppf->Load(wszShortCutName,STGM_READ);
                if(SUCCEEDED(hr))
                {
                    hr=pShLink->Resolve(NULL,SLR_ANY_MATCH|SLR_NO_UI);
                    if(SUCCEEDED(hr))
                    {
                        WIN32_FIND_DATAW wfd ;
                        WCHAR szPath[MAX_PATH];
                        hr=pShLink->GetPath(szPath,MAX_PATH,&wfd,SLGP_SHORTPATH);
                        WCHAR szParam[MAX_PATH];
                        hr=pShLink->GetArguments(szParam,MAX_PATH);
                        ShellExecuteW(NULL,L"open",szPath,szParam,NULL,SW_SHOWNORMAL);
                    }
                }
                ppf->Release();
            }
            pShLink->Release();
        }
        CoUninitialize();
    }
    return SUCCEEDED(hr);
}

//wszFileName������ȫ·���ļ�����ȫ·���ļ�����������
//L"c:\\windows\\system32\\cmd.exe"
//L"c:\\windows"
//L"c:\\windows\\system32\\"
inline BOOL Help_OpenFile(PWCHAR wszFileName)
{
    ShellExecuteW(NULL,L"open",wszFileName,NULL,NULL,SW_SHOWNORMAL);
    return TRUE;
}



inline int Helper_GetDirectoryIconIndex(PWCHAR pszPath,int pathLen) 
{      
    if(0==pathLen){//��Ŀ¼
        SHFILEINFOW sfi;
        WCHAR path[MAX_PATH];      
        GetSystemDirectoryW(path,MAX_PATH);
        path[3]=L'\0';
        SHGetFileInfoW(path,  
            FILE_ATTRIBUTE_DIRECTORY,  
            &sfi,  
            sizeof(sfi),  
            SHGFI_SMALLICON 
            | SHGFI_SYSICONINDEX 
            | SHGFI_USEFILEATTRIBUTES
            );
        return sfi.iIcon;
    }else{
        static int dirIconIndex=-1;
        if(-1==dirIconIndex){
            SHFILEINFOW sfi;
            WCHAR path[MAX_PATH];
            GetSystemDirectoryW(path,MAX_PATH);
            SHGetFileInfoW(path,  
                FILE_ATTRIBUTE_DIRECTORY,  
                &sfi,  
                sizeof(sfi),  
                SHGFI_SMALLICON 
                | SHGFI_SYSICONINDEX 
                | SHGFI_USEFILEATTRIBUTES
                );
            dirIconIndex=sfi.iIcon;              
        }
        return dirIconIndex;      
    }
}

inline int Helper_GetFileIconIndex(PWCHAR pszPath,int pathLen,PWCHAR pFileName,int fileLen)
{
    int j=pathLen;
    for(int i=0;i<fileLen;++i)
    {
        pszPath[j++]=pFileName[i];
    }
    pszPath[j]=L'\0';

    static SHFILEINFOW sfi;
    SHGetFileInfoW(pszPath,  
        FILE_ATTRIBUTE_NORMAL,  
        &sfi,  
        sizeof(sfi),  
        SHGFI_SMALLICON //SHGFI_LARGEICON
        | SHGFI_SYSICONINDEX 
        | SHGFI_USEFILEATTRIBUTES
        ) ;
    pszPath[pathLen]=L'\0';
    return sfi.iIcon;
}
