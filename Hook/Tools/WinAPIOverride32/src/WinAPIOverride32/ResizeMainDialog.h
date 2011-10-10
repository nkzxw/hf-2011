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
// Object: manages the main dialog resize operations
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/Gui/Splitter/Splitter.h"

#define STATIC_GROUPBOX_BORDER_WIDTH 2
#define SPACE_BETWEEN_CONTROLS 5
#define MAIN_DIALOG_MIN_WIDTH 560
#define MAIN_DIALOG_MIN_HEIGHT 400

void GetClientWindowRect(HWND hItem,LPRECT lpRect);
void CheckSize(RECT* pWinRect);
void Resize(BOOL UserInterfaceInStartedMode);
void SplitterLoadMonitoringAndFakingMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam);
void SplitterDetailsMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam);