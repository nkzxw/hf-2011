/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif

#include "../Common_Files/RegistryCommonFunctions.h"
#include "../Common_Files/EmulatedRegistry.h"
#include "resource.h"

// ! REQUIRE FULL WINAPIOVERRIDE SOURCES TO COMPILE !
// sources are available at http://jacquelin.potier.free.fr/winapioverride32/
#include "../../../../../Tools/gui/Dialog/DialogBase.h"
#include "../../../../../Tools/gui/ListView/Listview.h"

class CHostInfos
{
public:
    CHostInfos()
    {
        this->bExtractRegistry = TRUE;
        this->pHostKey = NULL;
    }
    virtual ~CHostInfos()
    {
    
    }
    BOOL bExtractRegistry;
    EmulatedRegistry::CHostKey* pHostKey;
};

class CRegExtractorGUI:public CDialogBase
{
public:
    CRegExtractorGUI();
    ~CRegExtractorGUI();
protected:
    CListview* pListviewKeys;
    CListview* pListviewHosts;
    EmulatedRegistry::CEmulatedRegistry* pRegistry;
    CHostInfos* pCurrentlyEditedHostKey;

    BOOL Is64BitOs();
    BOOL b64BitOs;


    virtual void OnInit();
    virtual void OnClose();
    virtual void OnCommand(WPARAM wParam,LPARAM lParam);
    virtual void OnNotify(WPARAM wParam,LPARAM lParam);

    void OnAddHost();
    void OnRenameHost();
    void OnRemoveSelectedHosts();
    static void OnSelectHostChange(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void OnSelectHostChange(int ItemIndex,int SubItemIndex);
    void OnAddKey();
    void OnRemoveSelectedKeys();
    void OnCreateConfigurationFile();

    void AddHost(TCHAR* HostName);
    BOOL GetBaseKeyFromFullPath(TCHAR* const FullPath,OUT HKEY* phBaseKey,OUT TCHAR** pRelatvePathFromBaseKey);
    BOOL ExtractRealRegistryValues(BOOL bRegistryView32,EmulatedRegistry::CKeyReplace* pKey,HKEY hKeyToExtract);
    BOOL ExtractRealRegistryValues(BOOL bRegistryView32,EmulatedRegistry::CHostKey* pHostKey,OUT BOOL* pErrorDetectedDuringExtraction);
    CHostInfos* GetSelectedHost();
    BOOL SaveRegistry(EmulatedRegistry::CEmulatedRegistry* pEmulatedRegistry,TCHAR* const FileName);
    BOOL QuerySavingFileName(TCHAR* FileName);
    BOOL ExtractAndSaveRegistry(BOOL bRegistryView32);
    void SetGUIFromHostKeyName(EmulatedRegistry::CHostKey* pHostKey);
    void SetHostKeyInfosFromGUI(CHostInfos* pHostKeyInfos);
    void SetGUIFromHostKeyInfos(CHostInfos* pHostKeyInfos);
    BOOL GetRegistryView(BOOL bRegistryView32,IN OUT REGSAM* samDesired);
};