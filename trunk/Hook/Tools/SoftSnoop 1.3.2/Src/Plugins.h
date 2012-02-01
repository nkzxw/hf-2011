
#ifndef __Plugins
#   define __Plugins

#   include <windows.h>
#   include "SoftSnoop.h"
#   include "SSPlugin.h"

void  InitPluginStruct(LPSSAPI pSSApi);
BOOL  LoadPlugins();
BOOL  ProcessPluginMenu(WORD wID);
void  PostPluginWindowMessage(UINT Msg, WPARAM wParam, LPARAM lParam);

extern SSAPI   SSApi;
extern DWORD   dwPluginWndNum;
extern BOOL    bPluginHandler;

#   endif