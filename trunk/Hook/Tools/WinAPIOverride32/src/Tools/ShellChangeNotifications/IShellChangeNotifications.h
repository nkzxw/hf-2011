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
// Object: ShellChangeNotification helper
//-----------------------------------------------------------------------------


/* Example of use
class CShellWatcher :public IShellItemChangeWatcher
{
    virtual void OnChangeNotify(long const lEvent, TCHAR* const sz1, TCHAR* const sz2)
    {
        TCHAR Msg[3*MAX_PATH];
        _stprintf(Msg,_T("Event : %8X, sz1 : %s, sz2 : %s\r\n"),lEvent,sz1,sz2);
        OutputDebugString(Msg);
    }
};

// global object definition
    CShellWatcher EventWatcher;
    #define WM_SHELL_CHANGE_NOTIFY (WM_APP+200)

// inside WM_INITDIALOG or WM_CREATE
    OleInitialize(NULL);
    TCHAR Path[256]= _T("c:\\MyPath");
    EventWatcher.StartWatching(Path, hwnd, WM_SHELL_CHANGE_NOTIFY, SHCNE_ALLEVENTS, TRUE);

// inside WM_CLOSE or WM_QUIT
    EventWatcher.StopWatching();

// inside DlgProc or WndProc
    case WM_SHELL_CHANGE_NOTIFY:
        // This is the message that will be sent when changes are detected in the shell namespace
        EventWatcher.OnChangeMessage(wParam, lParam);
        break;  
*/


#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include <shlobj.h>
#pragma comment (lib,"shell32.lib")


#ifndef ARRAYSIZE
    #define ARRAYSIZE(x) ( sizeof(x) / sizeof(x[0]))
#endif

// according to used SDK version these defines can not exist
#ifndef SHCNRF_InterruptLevel
    #define SHCNRF_InterruptLevel       0x0001
#endif
#ifndef SHCNRF_ShellLevel
    #define SHCNRF_ShellLevel           0x0002
#endif
#ifndef SHCNRF_RecursiveInterrupt
    #define SHCNRF_RecursiveInterrupt   0x1000
#endif
#ifndef SHCNRF_NewDelivery
    #define SHCNRF_NewDelivery          0x8000
#endif


class IShellItemChangeWatcher
{
protected:
    ULONG RegistrationID;

public:
    IShellItemChangeWatcher()
    {
        this->RegistrationID= 0;
    }

    ~IShellItemChangeWatcher()
    {
        this->StopWatching();
    }

    HRESULT StartWatching(TCHAR* const ItemToWatch, HWND const hwnd, UINT const uMsg, long const lEvents, BOOL const fRecursive)
    {
        HRESULT hResult= E_FAIL;
        WCHAR* pwscPath;
        IShellFolder* pDesktopFolder=NULL;
        ITEMIDLIST* pidlToWatch=NULL;

        // check parameter
        if (::IsBadReadPtr(ItemToWatch,sizeof(TCHAR)))
            return FALSE;

        // convert ansi to unicode if needed 
#if ( defined(UNICODE) || defined(_UNICODE))
        pwscPath=ItemToWatch;
#else
        size_t NbCharacters = strlen(ItemToWatch)+1;

        pwscPath = (WCHAR*) malloc(NbCharacters*sizeof(WCHAR));
        if (!pwscPath)
            goto CleanUp;

        // Convert to Unicode.
        if (::MultiByteToWideChar(CP_ACP, 0, ItemToWatch, (int)NbCharacters,pwscPath, (int)NbCharacters)==0)
            goto CleanUp;
#endif
        // get root folder
        hResult = ::SHGetDesktopFolder(&pDesktopFolder);
        if (FAILED(hResult)||(!pDesktopFolder))
            goto CleanUp;

        // get object full pidl from path name
        hResult = pDesktopFolder->ParseDisplayName(NULL,
                                                    NULL,
                                                    pwscPath,
                                                    NULL,
                                                    &pidlToWatch,
                                                    NULL);
        if ( FAILED(hResult) || (!pidlToWatch) )
            goto CleanUp;

        hResult = this->StartWatching(pidlToWatch,hwnd, uMsg, lEvents, fRecursive);
CleanUp:
#if ( (!defined(UNICODE)) && (!defined(_UNICODE)) )
        if (pwscPath)
            free(pwscPath);
#endif
        if (pidlToWatch)
            ::ILFree(pidlToWatch);
        if (pDesktopFolder)
            pDesktopFolder->Release();
        return hResult;
    }

    // lEvents is SHCNE_XXX values like SHCNE_ALLEVENTS
    // fRecursive means to listen for all events under this folder
    HRESULT StartWatching(ITEMIDLIST* const pidlToWatch, HWND const hwnd, UINT const uMsg, long const lEvents, BOOL const fRecursive)
    {
        this->StopWatching();

        SHChangeNotifyEntry entries[] = { pidlToWatch, fRecursive };
        int const nSources = SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery;

        this->RegistrationID = ::SHChangeNotifyRegister(hwnd, nSources, lEvents, uMsg, ARRAYSIZE(entries), entries);
        return  (this->RegistrationID != 0 ? S_OK : E_FAIL);
    }

    void StopWatching()
    {
        if (this->RegistrationID)
        {
            ::SHChangeNotifyDeregister(this->RegistrationID);
            this->RegistrationID = 0;
        }
    }

    // in your window procedure call this message to dispatch the events
    void OnChangeMessage(WPARAM wParam, LPARAM lParam)
    {
        long lEvent;
        ITEMIDLIST** rgpidl;
        HANDLE hNotifyLock = ::SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &rgpidl, &lEvent);
        if (hNotifyLock)
        {
            TCHAR sz1[MAX_PATH];
            TCHAR sz2[MAX_PATH];

            *sz1=0;
            *sz2=0;

            if (rgpidl[0])
            {
                ::SHGetPathFromIDList(rgpidl[0], sz1);
            }

            if (rgpidl[1])
            {
                ::SHGetPathFromIDList(rgpidl[1], sz2);
            }

            // derived class implements this method, that is where the events are delivered
            this->OnChangeNotify(lEvent, sz1, sz2);
            ::SHChangeNotification_Unlock(hNotifyLock);
        }
    }

    // derived class implements this event
    // see MSDN : SHChangeNotify for sz1/sz2 signification
    virtual void OnChangeNotify(long const lEvent, TCHAR* const sz1, TCHAR* const sz2) = 0;

};