/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_DIALOGS_ABOUTDIALOG_H
#define _WEHNTRUST_WEHNTRUST_UI_DIALOGS_ABOUTDIALOG_H

//
// Displays contact information for Wehnus and other such information
//
class AboutDialog : public Dialog
{
	public:
		AboutDialog();
		~AboutDialog();

		//
		// Notifications
		//
		
		BOOLEAN OnInit();
	protected:
};


#endif
