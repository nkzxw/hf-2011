/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

StatusDialog::StatusDialog()
: Dialog(IDD_STATUS),
  Parent(NULL)
{
}

StatusDialog::~StatusDialog()
{
}

//
// Set the pointer to the parent dialog
//
VOID StatusDialog::SetParent(
		IN WehnTrustDialog *InParent)
{
	Parent = InParent;
}

//
// Update the text area for the about text
//
BOOLEAN StatusDialog::OnInit()
{
	UpdateControlText(
			STATUS_LABEL_STATISTICS,
			IDS_STATISTICS);
	UpdateControlText(
			STATUS_LABEL_CONTROL,
			IDS_CONTROL);
	UpdateControlText(
			STATUS_CHECK_RANDALLOC,
			IDS_RAND_ALLOC_TEXT);
	UpdateControlText(
			STATUS_CHECK_RANDDLL,
			IDS_RAND_DLL_TEXT);
	UpdateControlText(
			STATUS_CHECK_TRAYICON,
			IDS_SHOW_SYSTEM_TRAY_ICON);

	return TRUE;
}


////
//
// Protected Methods
//
////

//
// Handle control notifications
//
LRESULT StatusDialog::OnNotifyControl(
		IN INT ControlId,
		IN HWND ControlWindow,
		IN UINT Code,
		IN PVOID Notify)
{
	LRESULT Result = 0;

	switch (ControlId)
	{
		case STATUS_CHECK_RANDDLL:
		case STATUS_CHECK_RANDALLOC:
		case STATUS_CHECK_TRAYICON:
			switch (Code)
			{
				case BN_CLICKED:
					Parent->EnableApply();
					break;
			}
			break;
	}

	return Result;
}
