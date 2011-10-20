/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_WEHNTRUSTDIALOG_H
#define _WEHNTRUST_WEHNTRUST_UI_WEHNTRUSTDIALOG_H

#include "../Dialog.h"

#include "StatusDialog.h"
#include "ExemptionsDialog.h"
#include "AboutDialog.h"

class WehnTrustDialog : public Dialog
{
	public:
		WehnTrustDialog();
		~WehnTrustDialog();

		//
		// Notifications
		//

		BOOLEAN OnInit();
		BOOLEAN OnClose();

		VOID OnOK();
		VOID OnApply();
		VOID OnCancel();

		VOID EnableApply();
		VOID DisableApply();

		VOID SetExitOnClose(
				IN BOOLEAN TruthValue);
	protected:
		LRESULT OnNotifyControl(
				IN INT ControlId,
				IN HWND ControlWindow,
				IN UINT Code,
				IN PVOID Notify);
		VOID OnTimer(
				IN UINT TimerId);

		VOID RefreshStatus(
				IN BOOLEAN UpdateEnabledStatus = FALSE);
		VOID RefreshTitle();
		
		//
		// Attributes
		// 

		StatusDialog     Status;
		ExemptionsDialog Exemptions;
		AboutDialog      About;

		HANDLE           DriverHandle;
		ULONG            CurrentlyEnabledSubsystems;

		BOOLEAN          ExitOnClose;
};

#endif
