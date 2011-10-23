//********************************************************************
//	created:	18:8:2008   19:42
//	file:		advboot.cpp
//	author:		tiamo
//	purpose:	advanced boot
//********************************************************************

#include "stdafx.h"

//
// load processing
//
typedef VOID (*PADVANCED_BOOT_LOAD_PROCESSING)();

//
// is visible
//
typedef BOOLEAN (*PADVANCED_BOOT_IS_VISIBLE)();

//
// advanced boot options
//
typedef struct _ADVANCED_BOOT_OPTION
{
	//
	// item type,1 = normal item,2 = separator
	//
	ULONG												Type;

	//
	// message id
	//
	ULONG												MenuTextMessageId;

	//
	// display string
	//
	PCHAR												DisplayString;

	//
	// loader options string
	//
	PCHAR												LoaderOptions;

	//
	// show or hide this item is depend on another item
	//
	ULONG												DependOnIndex;

	//
	// show this item or not
	//
	ULONG												IsVisible;

	//
	// offset 18
	//
	ULONG												Offset18;

	//
	// load process
	//
	PADVANCED_BOOT_LOAD_PROCESSING						LoadProcessing;

	//
	// check hide
	//
	PADVANCED_BOOT_IS_VISIBLE							CheckIsVisible;

	//
	// default menu item
	//
	ULONG												IsDefaultMenuItem;
}ADVANCED_BOOT_OPTION,*PADVANCED_BOOT_OPTION;

//
// advanced boot options
//
extern ADVANCED_BOOT_OPTION								AdvancedBootOptions[14];

//
// selected index
//
ULONG													AdvancedBoot = 0xffffffff;

//
// do load processing
//
VOID BlDoAdvancedBootLoadProcessing(__in ULONG BootIndex)
{
	if(BootIndex >= ARRAYSIZE(AdvancedBootOptions))
		return;

	if(AdvancedBootOptions[BootIndex].LoadProcessing)
		AdvancedBootOptions[BootIndex].LoadProcessing();
}

//
// get boot load options
//
PCHAR BlGetAdvancedBootLoadOptions(__in ULONG BootIndex)
{
	if(BootIndex >= ARRAYSIZE(AdvancedBootOptions))
		return "";

	return AdvancedBootOptions[BootIndex].LoaderOptions;
}

//
// get boot id
//
ULONG BlGetAdvancedBootID(__in ULONG BootIndex)
{
	if(BootIndex >= ARRAYSIZE(AdvancedBootOptions))
		return 0xffffffff;

	return AdvancedBootOptions[BootIndex].MenuTextMessageId;
}

//
// get display string
//
PCHAR BlGetAdvancedBootDisplayString(__in ULONG BootIndex)
{
	if(BootIndex >= ARRAYSIZE(AdvancedBootOptions))
		return "";

	return AdvancedBootOptions[BootIndex].DisplayString;
}

//
// process last known good options
//
VOID BlProcessLastKnownGoodOption()
{
	extern BOOLEAN ForceLastKnownGood;
	ForceLastKnownGood									= TRUE;
}

//
// disable autoreboot on crash
//
VOID BlProcessDisableAutoRebootOnCrash()
{
	extern BOOLEAN BlDisableCrashAutoReboot;
	BlDisableCrashAutoReboot							= TRUE;
}

//
// show return to os select
//
BOOLEAN BlIsReturnToOSChoicesValid()
{
	extern BOOLEAN BlShowReturnToOSChoices;
	return BlShowReturnToOSChoices;
}

//
// get advanced boot option
//
ULONG BlGetAdvancedBootOption()
{
	return AdvancedBoot;
}

//
// auto advanced boot
//
VOID BlAutoAdvancedBoot(__inout PCHAR* LoaderOptions,__in ULONG LastBootStatus,__in ULONG DefaultAdvancedBootIndex)
{
	CHAR LastStatusBuffer[32];
	sprintf(LastStatusBuffer,"LastBootStatus=%d",LastBootStatus);

	PCHAR LoaderOptionsFromAdvancedBoot					= DefaultAdvancedBootIndex != 0xffffffff ? BlGetAdvancedBootLoadOptions(DefaultAdvancedBootIndex) : 0;
	PCHAR OldLoaderOptions								= *LoaderOptions;

	ULONG TotalLength									= OldLoaderOptions ? strlen(OldLoaderOptions) + 1 : 0;
	TotalLength											+= LoaderOptionsFromAdvancedBoot ? strlen(LoaderOptionsFromAdvancedBoot) + 1 : 0;
	TotalLength											+= strlen(LastStatusBuffer) + 1;

	PCHAR NewOptions									= static_cast<PCHAR>(BlAllocateHeap(TotalLength));
	if(!NewOptions)
		return;

	sprintf(NewOptions,"%s %s %s",OldLoaderOptions ? OldLoaderOptions : "",LoaderOptionsFromAdvancedBoot ? LoaderOptionsFromAdvancedBoot : "",LastStatusBuffer);
	*LoaderOptions										= NewOptions;
}

//
// show advanced boot menu
//
ULONG BlDoAdvancedBoot(__in ULONG TitleMessageId,__in ULONG DefaultSelectedMenuIndex,__in ULONG Arg8,__in ULONG Timeout)
{
	//
	// get some messages ....
	//
	PCHAR TitleMessage									= BlFindMessage(TitleMessageId);
	PCHAR TimeoutMessage								= BlFindMessage(BL_ADVANCED_BOOT_TIMEOUT);
	PCHAR TrailerMessage								= BlFindMessage(BL_MOVE_HIGHLIGHT);
	if(!TimeoutMessage|| !TitleMessage || !TrailerMessage)
		return 0xffffffff;

	//
	// trim line feed,because we will append a number to it
	//
	PCHAR EndOfTimeoutMessage							= strchr(TimeoutMessage,'\r');
	if(EndOfTimeoutMessage)
		*EndOfTimeoutMessage							= 0;

	//
	// clear screen
	//
	BlClearScreen();

	//
	// switch to normal mode
	//
	BlSetInverseMode(FALSE);

	//
	// goto (1,2)
	//
	BlPositionCursor(1,2);

	//
	// show title
	//
	BlPrint(TitleMessage);

	//
	// read current position
	//
	ULONG x												= 0;
	ULONG MenuStartY									= 0;
	TextGetCursorPosition(&x,&MenuStartY);

	//
	// for each menu item,check its visibilities,and set the default selected item index
	//
	for(ULONG i = 0; i < ARRAYSIZE(AdvancedBootOptions); i ++)
	{
		if(Arg8 && !AdvancedBootOptions[i].Offset18)
			AdvancedBootOptions[i].IsVisible			= FALSE;
		else if(AdvancedBootOptions[i].CheckIsVisible)
			AdvancedBootOptions[i].IsVisible			= AdvancedBootOptions[i].CheckIsVisible();
		else
			AdvancedBootOptions[i].IsVisible			= TRUE;

		if(DefaultSelectedMenuIndex == 0xffffffff && AdvancedBootOptions[i].IsDefaultMenuItem)
			DefaultSelectedMenuIndex					= i;
	}

	//
	// also check dependency
	// hide those items whose dependency item is visible
	//
	for(ULONG i = 0; i < ARRAYSIZE(AdvancedBootOptions); i ++)
	{
		if(AdvancedBootOptions[i].DependOnIndex != 0xffffffff && AdvancedBootOptions[AdvancedBootOptions[i].DependOnIndex].IsVisible)
			AdvancedBootOptions[i].IsVisible			= FALSE;
	}

	//
	// then count total items
	//
	ULONG TotalVisibleItems								= 0;
	for(ULONG i = 0; i < ARRAYSIZE(AdvancedBootOptions); i ++)
	{
		if(AdvancedBootOptions[i].IsVisible)
			TotalVisibleItems							+= 1;
	}

	//
	// and setup those display string
	//
	for(ULONG i	= 0; i < ARRAYSIZE(AdvancedBootOptions); i ++)
	{
		if(AdvancedBootOptions[i].Type == 1 && !AdvancedBootOptions[i].DisplayString)
			AdvancedBootOptions[i].DisplayString		= BlFindMessage(AdvancedBootOptions[i].MenuTextMessageId);
	}

	//
	// show foot message
	//
	BlPositionCursor(1,MenuStartY + TotalVisibleItems + 1);
	BlPrint(TrailerMessage);

	//
	// set current select item index
	//
	ULONG CurrentSelectedItemIndex						= DefaultSelectedMenuIndex;

	//
	// make sure it is visible
	// BUGBUGBUG deadloop if all the items is not visible
	//
	while(!AdvancedBootOptions[CurrentSelectedItemIndex].IsVisible || AdvancedBootOptions[CurrentSelectedItemIndex].Type == 2)
	{
		CurrentSelectedItemIndex						= (CurrentSelectedItemIndex + 1) % ARRAYSIZE(AdvancedBootOptions);
	}

	//
	// setup some vars
	//
	ULONG CurrentTimeInSecond							= 0;
	ULONG CurrentTime									= 0;
	BOOLEAN RedrawMenu									= TRUE;
	ULONG LastCounter									= 0;

	//
	// calc timeout values
	//
	if(Timeout)
	{
		CurrentTimeInSecond								= Timeout * 182 / 10 * 10 / 10;
		CurrentTime										= Timeout * 182 / 10;
		LastCounter										= ExternalServicesTable->GetCounter();
	}

	//
	// start menu loop
	//
	while(1)
	{
		//
		// check timeout
		//
		if(Timeout)
		{
			ULONG CurrentCounter						= ExternalServicesTable->GetCounter();
			ULONG Elapse								= CurrentCounter >= LastCounter ? CurrentCounter - LastCounter : 1;

			if(CurrentTime < Elapse)
				Elapse									= CurrentTime;

			CurrentTime									-= Elapse;
			LastCounter									= CurrentCounter;

			if(CurrentTimeInSecond > CurrentTime * 10 / 182)
			{
				CurrentTimeInSecond						= CurrentTime * 10 / 182;
				RedrawMenu								= TRUE;
			}

			if(!CurrentTime)
				return 0xffffffff;
		}

		//
		// redraw menu
		//
		if(RedrawMenu)
		{
			RedrawMenu									= FALSE;

			//
			// draw each items
			//
			for(ULONG i = 0,DrawIndex = 1; i < ARRAYSIZE(AdvancedBootOptions); i ++)
			{
				if(!AdvancedBootOptions[i].IsVisible)
					continue;

				//
				// set cursor position
				//
				BlPositionCursor(1,MenuStartY + DrawIndex + 1);

				//
				// highlight current item
				//
				BlSetInverseMode(i == CurrentSelectedItemIndex ? TRUE : FALSE);

				//
				// display text
				//
				if(AdvancedBootOptions[i].Type == 1)
					BlPrint("    %s",AdvancedBootOptions[i].DisplayString);

				//
				// back to normal mode
				//
				BlSetInverseMode(FALSE);

				DrawIndex								+= 1;
			}

			//
			// show timeout message
			//
			BlPositionCursor(1,MenuStartY + TotalVisibleItems + 4);
			if(Timeout)
				BlPrint("%s %d \n",TimeoutMessage,CurrentTimeInSecond);
			else
				BlClearToEndOfLine();
		}

		//
		// read key
		//
		ULONG Key										= BlGetKey();

		//
		// any key press will stop timeout
		//
		if(Key)
			Timeout										= 0;

		//
		// escape return
		//
		if(!Arg8 && Key == ESC_KEY)
			return 0xffffffff;

		//
		// return key will break menu loop
		//
		if(Key == '\r' || Key == '\n')
		{
			if(CurrentSelectedItemIndex != 0xffffffff && AdvancedBootOptions[CurrentSelectedItemIndex].MenuTextMessageId != BL_ADVANCED_BOOT_RETURN_TO_OS_SELECT)
				return CurrentSelectedItemIndex;

			return 0xffffffff;
		}

		//
		// up,down,home,end will move current selected item index,and redraw menu
		//
		ULONG Inc										= 1;
		if(Key == UP_ARROW)
		{
			CurrentSelectedItemIndex					= (CurrentSelectedItemIndex + ARRAYSIZE(AdvancedBootOptions) - 1) % ARRAYSIZE(AdvancedBootOptions);
			Inc											= ARRAYSIZE(AdvancedBootOptions) - 1;
		}
		else if(Key == DOWN_ARROW)
		{
			CurrentSelectedItemIndex					= (CurrentSelectedItemIndex + 1) % ARRAYSIZE(AdvancedBootOptions);
			Inc											= 1;
		}
		else if(Key == HOME_KEY)
		{
			CurrentSelectedItemIndex					= 0;
			Inc											= 1;
		}
		else if(Key == END_KEY)
		{
			CurrentSelectedItemIndex					= ARRAYSIZE(AdvancedBootOptions) - 1;
			Inc											= ARRAYSIZE(AdvancedBootOptions) - 1;
		}
		else
		{
			//
			// ignore all other keys
			//
			continue;
		}

		//
		// make sure the next selected item is a visible item
		//
		while(!AdvancedBootOptions[CurrentSelectedItemIndex].IsVisible || AdvancedBootOptions[CurrentSelectedItemIndex].Type == 2)
		{
			CurrentSelectedItemIndex					= (CurrentSelectedItemIndex + Inc) % ARRAYSIZE(AdvancedBootOptions);
		}

		RedrawMenu										= TRUE;
	}

	return 0xffffffff;
}

ADVANCED_BOOT_OPTION									AdvancedBootOptions[14] =
{
	{
		1,
		BL_ADVANCED_BOOT_SAFE_MODE,
		0,
		"SAFEBOOT:MINIMAL SOS BOOTLOG NOGUIBOOT",
		0xffffffff,
		0,
		TRUE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_SAFE_MODE_WITH_NETWORK,
		0,
		"SAFEBOOT:NETWORK SOS BOOTLOG NOGUIBOOT",
		0xffffffff,
		0,
		TRUE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_SAFE_MODE_WITH_CMD,
		0,
		"SAFEBOOT:MINIMAL(ALTERNATESHELL) SOS BOOTLOG NOGUIBOOT",
		0xffffffff,
		0,
		TRUE,
		0,
		0,
		FALSE
	},

	{
		2,
		0,
		0,
		0,
		0xffffffff,
		0,
		TRUE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_ENABLE_BOOT_LOGGING,
		0,
		"BOOTLOG",
		0xffffffff,
		0,
		FALSE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_BASE_VIDEO,
		0,
		"BASEVIDEO",
		0xffffffff,
		0,
		FALSE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_USE_LAST_KNOWN_GOOD,
		0,
		0,
		0xffffffff,
		0,
		TRUE,
		&BlProcessLastKnownGoodOption,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_DIRECTORY_SERVICES_RESTORE_MODE,
		0,
		"SAFEBOOT:DSREPAIR SOS",
		0xffffffff,
		0,
		FALSE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_DEBUG_MODE,
		0,
		"DEBUG",
		0xffffffff,
		0,
		FALSE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_DISABLE_CRASH_AUTO_REBOOT,
		0,
		0,
		0xffffffff,
		0,
		FALSE,
		&BlProcessDisableAutoRebootOnCrash,
		0,
		FALSE
	},

	{
		2,
		0,
		0,
		0,
		0xffffffff,
		0,
		TRUE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_START_WINDOWS_NORMALLY,
		0,
		0,
		0xffffffff,
		0,
		TRUE,
		0,
		0,
		TRUE
	},

	{
		1,
		BL_ADVANCED_BOOT_REBOOT_COMPUTER,
		0,
		0,
		0xffffffff,
		0,
		FALSE,
		0,
		0,
		FALSE
	},

	{
		1,
		BL_ADVANCED_BOOT_RETURN_TO_OS_SELECT,
		0,
		0,
		0xffffffff,
		0,
		FALSE,
		0,
		&BlIsReturnToOSChoicesValid,
		FALSE
	},
};