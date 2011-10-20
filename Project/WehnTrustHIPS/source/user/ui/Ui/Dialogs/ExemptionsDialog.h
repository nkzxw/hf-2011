/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_DIALOGS_EXEMPTIONSDIALOG_H
#define _WEHNTRUST_WEHNTRUST_UI_DIALOGS_EXEMPTIONSDIALOG_H

class WehnTrustDialog;

//
// Provides the user with an interface to exempting certain processes
// and images from being randomized.
//
class ExemptionsDialog : public Dialog
{
	public:
		ExemptionsDialog();
		~ExemptionsDialog();

		//
		// Getters/Setters
		// 
		VOID SetParent(
				IN WehnTrustDialog *Parent);

		//
		// Notifications
		//
		
		BOOLEAN OnInit();
		BOOLEAN OnClose();
	protected:

		LRESULT OnNotifyControl(
				IN INT ControlId,
				IN HWND ControlWindow,
				IN UINT Code,
				IN PVOID Notify);

		//
		// Displaying certain exemptions
		//
		VOID DisplayApplicationExemptions();
		VOID DisplayImageFileExemptions();
		VOID DisplayDirectoryExemptions();

		VOID DisplayExemptions(
				IN EXEMPTION_TYPE type);

		VOID FlushExemptionList();

		//
		// Driver communication wrappers
		//
		DWORD RemoveExemptionFromCurrentType(
				IN LPTSTR FilePath);
		DWORD FlushExemptionsFromCurrentType();

		//
		// Get the current selection's NT file path
		//
		LPTSTR GetCurrentSelectionNtFilePath();

		//
		// Attributes
		//
		WehnTrustDialog *Parent;
		EXEMPTION_TYPE  CurrentExemptionType;
};

#include "AddEditExemptionDialog.h"

#endif
