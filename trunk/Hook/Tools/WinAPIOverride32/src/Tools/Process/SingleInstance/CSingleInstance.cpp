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
// Object: allow only one instance of an application
//-----------------------------------------------------------------------------

#include "csingleinstance.h"

CSingleInstance::CSingleInstance(TCHAR *strMutexName)
{
    //be sure to use a name that is unique for this application otherwise
    //two apps may think they are the same if they are using same name for
    //3rd parm to CreateMutex
    m_hMutex = CreateMutex(NULL, FALSE, strMutexName); //do early
    m_dwLastError = GetLastError(); //save for use later...
}
   
CSingleInstance::~CSingleInstance() 
{
    if (m_hMutex)  //don't forget to close handles...
    {
        CloseHandle(m_hMutex); //do as late as possible
        m_hMutex = NULL; //good habit to be in
    }
}

BOOL CSingleInstance::IsAnotherInstanceRunning() 
{
    return (ERROR_ALREADY_EXISTS == m_dwLastError);
}