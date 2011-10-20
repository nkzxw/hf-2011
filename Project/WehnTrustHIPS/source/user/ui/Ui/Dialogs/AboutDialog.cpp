/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

AboutDialog::AboutDialog()
: Dialog(IDD_ABOUT)
{
}

AboutDialog::~AboutDialog()
{
}

////
//
// Notifications
//
////

//
// Update the text area for the about text
//
BOOLEAN AboutDialog::OnInit()
{
	UpdateControlText(
			ABOUT_LABEL_ABOUT,
			IDS_ABOUT_BODY);

	return TRUE;
}
