/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_UI_H
#define _WEHNTRUST_WEHNTRUST_UI_UI_H

#include "Locale.h"

#include "SystemTray.h"
#include "Dialogs/WehnTrustDialog.h"

#define UI_RESULT_OK      0
#define UI_RESULT_RESTART 1

//
// This class is responsible for managing the user interface for the user
//
class Ui
{
	public:
		Ui();
		~Ui();
		
		static Ui *GetInstance();
		static DWORD Display(
				IN BOOLEAN ShowDialog = TRUE,
				IN BOOLEAN ShowTray = TRUE,
				IN BOOLEAN FromStartup = FALSE);
		static DWORD Close(
				IN DWORD Result = UI_RESULT_OK);

		static BOOLEAN IsTrayDisabled();
		static VOID EnableTray();
		static VOID DisableTray();
	protected:
		DWORD HandleWindowMessages();

		DWORD DisplayDialog();
		DWORD DisplayTrayIcon(
				IN BOOLEAN Show = TRUE);

		//
		// Attributes
		// 

		SystemTray      Tray;
		WehnTrustDialog Status;

		DWORD           Result;
		BOOLEAN         TrayDisabled;
		BOOLEAN         MonitoringMessages;
		BOOLEAN         Closing;
};

#endif
