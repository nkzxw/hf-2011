//********************************************************************
//	created:	20:9:2008   5:29
//	file: 		cfgmenu.cpp
//	author:		tiamo
//	purpose:	config menu
//********************************************************************

#include "stdafx.h"

//
// last known good prompt start time
//
ULONG													LkgStartTime;

//
// show return to os choices
//
BOOLEAN													BlShowReturnToOSChoices = TRUE;

//
// count line
//
ULONG BlpCountLines(__in PCHAR Lines)
{
	ULONG NumLines										= 0;
	PCHAR p												= Lines;
	while(*p)
	{
		if(p[0] == '\r' && p[1] == '\n')
		{
			NumLines									+= 1;
			p											+= 1;
		}

		p												+= 1;
	}

	return NumLines;
}

//
// start config prompt
//
VOID BlStartConfigPrompt()
{
	if(!BlFindMessage(BL_LKG_MENU_PROMPT))
		return;

	LkgStartTime										= ArcGetRelativeTime();
}

//
// end config prompt
//
BOOLEAN BlEndConfigPrompt()
{
	ULONG EndTime										= BlTerminalConnected ? ArcGetRelativeTime() + 5 : 0;

	while(1)
	{
		BlShowReturnToOSChoices							= FALSE;
		ULONG Key										= BlGetKey();

		//
		// space key = show menu
		//
		if(Key == ' ')
			return TRUE;

		//
		// advanced boot
		//
		if(Key == F5_KEY || Key == F8_KEY)
		{
			BOOLEAN SavedShowReturnToOSChoices			= BlShowReturnToOSChoices;
			BlShowReturnToOSChoices						= FALSE;

			//
			// switch to text mode
			//
			if(DisplayLogoOnBoot)
				ExternalServicesTable->HardwareCursor(0x80000000,DbcsLangId ? 0x12 : 0x03);

			//
			// show advanced boot menu
			//
			ULONG SelectedAdvancedBootIndex				= BlDoAdvancedBoot(BL_ADVANCED_BOOT_MENU_PROMPT,0,0,0);

			//
			// check we should switch back to grahp mode
			//
			if(DisplayLogoOnBoot)
			{
				if(SelectedAdvancedBootIndex != 0xffffffff)
				{
					PCHAR LoadOptions					= BlGetAdvancedBootLoadOptions(SelectedAdvancedBootIndex);
					if(LoadOptions && strstr(LoadOptions,"SAFEBOOT"))
						DisplayLogoOnBoot				= FALSE;
				}

			#if _GRAPHICS_MODE_SUPPORT_
				if(DisplayLogoOnBoot)
				{
					//
					// enable graphics mode
					//
					ExternalServicesTable->HardwareCursor(0x80000000,0x12);
					if(DbcsLangId)
						TextClearDisplay();

					VgaEnableVideo();
					PaletteOn();
					DrawBitmap();
					BlUpdateBootStatus();
				}
			#endif
			}

			BlShowReturnToOSChoices						= SavedShowReturnToOSChoices;

			if(SelectedAdvancedBootIndex != 0xffffffff)
			{
				if(BlGetAdvancedBootID(SelectedAdvancedBootIndex) == BL_ADVANCED_BOOT_REBOOT_COMPUTER)
				{
					BlClearScreen();

					BdStopDebugger();

					ExternalServicesTable->RebootProcessor();
				}

				PCHAR LoadOptions						= BlGetAdvancedBootLoadOptions(SelectedAdvancedBootIndex);
				if(LoadOptions && !strstr(BlLoaderBlock->LoadOptions,LoadOptions))
				{
					//
					// why mul by 4 ?
					//
					ULONG Length							= (strlen(LoadOptions) + strlen(BlLoaderBlock->LoadOptions) + 2) * 4;
					PCHAR Buffer							= static_cast<PCHAR>(BlAllocateHeap(Length));
					if(Buffer)
					{
						strcpy(Buffer,BlLoaderBlock->LoadOptions);
						strcat(Buffer," ");
						strcat(Buffer,LoadOptions);
						BlLoaderBlock->LoadOptions			= Buffer;

						DbgPrint("Load Options = %s",Buffer);
					}
				}

				BlDoAdvancedBootLoadProcessing(SelectedAdvancedBootIndex);
			}
		}

		ULONG Current									= ArcGetRelativeTime();
		if(Current >= EndTime || Current < LkgStartTime)
			break;
	}

	BlRedrawProgressBar();
	return FALSE;
}

//
// display config menu
//
BOOLEAN BlConfigMenuPrompt(__in ULONG Timeout,__inout PBOOLEAN UseLastKnownGood,__inout PHCELL_INDEX ControlSetKeyCell,
						   __inout PCM_HARDWARE_PROFILE_LIST* ProfileList,__inout PCM_HARDWARE_DOCK_INFO_LIST* DockInfoList)
{
	ULONG CurrentTime									= 0;
	ULONG EndTime										= 0;
	PCHAR TimeoutPrompt									= 0;
	ULONG Count											= 0;
	if(Timeout != -1 && Timeout)
	{
		CurrentTime										= ArcGetRelativeTime();
		EndTime											= CurrentTime + Timeout;
		TimeoutPrompt									= BlFindMessage(BL_LKG_TIMEOUT);
		PCHAR p											= strchr(TimeoutPrompt,'\n');
		if(p)
			*p											= 0;

		p												= strchr(TimeoutPrompt,'\r');
		if(p)
			*p											= 0;
	}

	PCHAR MenuHeader									= BlFindMessage(BL_LKG_MENU_HEADER);
	PCHAR Temp											= BlFindMessage(BL_LKG_SELECT_MNEMONIC);
	if(!Temp)
		return TRUE;
	ULONG LkgMnemonic									= toupper(Temp[0]);

	Temp												= BlFindMessage(BL_DEFAULT_SELECT_MNEMONIC);
	if(!Temp)
		return TRUE;
	ULONG DefaultMnemonic								= toupper(Temp[0]);

	BOOLEAN Finished									= FALSE;
	ULONG CurrentSelection								= 0;
	ULONG ProfileCount									= 0;
	ULONG LastTime										= 0;

	while(!Finished)
	{
		ProfileCount									= *ProfileList ? (*ProfileList)->CurrentProfileCount : 0;
		PCHAR MenuTrailer1								= BlFindMessage(ProfileCount ? BL_LKG_MENU_TRAILER : BL_LKG_MENU_TRAILER_NO_PROFILES);
		PCHAR MenuTrailer2								= BlFindMessage(*UseLastKnownGood ? BL_SWITCH_DEFAULT_TRAILER : BL_SWITCH_LKG_TRAILER);

		if(!MenuHeader || !MenuTrailer1 || !MenuTrailer2)
			return TRUE;

		BlClearScreen();

		BlSetInverseMode(0);

		ULONG HeaderLines								= BlpCountLines(MenuHeader);
		ArcWrite(BlConsoleOutDeviceId,MenuHeader,strlen(MenuHeader),&Count);

		ULONG TrailerLines								= BlpCountLines(MenuTrailer1) + BlpCountLines(MenuTrailer2) + (TimeoutPrompt ? 1 : 0);
		BlPositionCursor(1, ScreenHeight - TrailerLines);
		ArcWrite(BlConsoleOutDeviceId,MenuTrailer1,strlen(MenuTrailer1),&Count);
		ArcWrite(BlConsoleOutDeviceId,MenuTrailer2,strlen(MenuTrailer2),&Count);

		ULONG DisplayLines								= ScreenHeight - HeaderLines - TrailerLines - 3;
		if(ProfileCount < DisplayLines)
			DisplayLines								= ProfileCount;

		ULONG TopProfileLine							= 0;
		CHAR MenuOption[80]								= {0};
		ULONG OptionLength								= 0;
		BOOLEAN Restart									= FALSE;
		CurrentSelection								= 0;

		while(!Finished && !Restart)
		{
			if(ProfileCount > 0)
			{
				for(ULONG i = 0; i < DisplayLines; i ++)
				{
					ULONG CurrentProfile				= i + TopProfileLine;
					PCM_HARDWARE_PROFILE Profile		= &(*ProfileList)->Profile[CurrentProfile];

					RtlUnicodeToMultiByteN(MenuOption,sizeof(MenuOption),&OptionLength,Profile->FriendlyName,Profile->NameLength);

					BlPositionCursor(5,HeaderLines + i + 2);
					BlSetInverseMode(CurrentProfile == CurrentSelection ? TRUE : FALSE);
					ArcWrite(BlConsoleOutDeviceId,MenuOption,OptionLength,&Count);
					BlSetInverseMode(FALSE);
					BlClearToEndOfLine();
				}
			}
			else
			{
				Temp									= BlFindMessage(BL_BOOT_DEFAULT_PROMPT);
				if(Temp)
				{
					BlPositionCursor(5, HeaderLines + 3);
					BlSetInverseMode(TRUE);
					ArcWrite(BlConsoleOutDeviceId,Temp,strlen(Temp),&Count);
					BlSetInverseMode(FALSE);
				}
			}

			ULONG Key									= 0;
			while(!Key && !Finished && !Restart)
			{
				Key										= BlGetKey();

				if(TimeoutPrompt)
				{
					CurrentTime							= ArcGetRelativeTime();
					if(CurrentTime != LastTime)
					{
						LastTime						= CurrentTime;
						sprintf(MenuOption,TimeoutPrompt,EndTime - CurrentTime);
						BlPositionCursor(1,ScreenHeight);
						ArcWrite(BlConsoleOutDeviceId,MenuOption,strlen(MenuOption),&Count);
						BlClearToEndOfLine();
					}

					if(CurrentTime >= EndTime)
						Finished						= TRUE;
				}

				switch(Key)
				{
				case F3_KEY:
					*ControlSetKeyCell					= HCELL_NIL;
					return FALSE;

				case UP_ARROW:
					if(ProfileCount > 0)
					{
						if(!CurrentSelection)
						{
							CurrentSelection			= ProfileCount - 1;
							if(TopProfileLine + DisplayLines <= CurrentSelection)
								TopProfileLine			= CurrentSelection - DisplayLines + 1;
						}
						else
						{
							CurrentSelection			-= 1;
							if(CurrentSelection < TopProfileLine)
								TopProfileLine			= CurrentSelection;
						}
					}
					break;

				case DOWN_ARROW:
					if(ProfileCount > 0)
					{
						CurrentSelection				= (CurrentSelection + 1) % ProfileCount;
						if(!CurrentSelection)
							TopProfileLine				= 0;
						else if(TopProfileLine + DisplayLines <= CurrentSelection)
							TopProfileLine				= CurrentSelection - DisplayLines + 1;
					}
					break;

				case '\r':
				case '\n':
					Finished							= TRUE;
					break;

				default:
					VOID BlpSwitchControlSet(__out PCM_HARDWARE_PROFILE_LIST* ProfileList,__out PCM_HARDWARE_DOCK_INFO_LIST* DockInfoList,
											 __in BOOLEAN UseLastKnownGood,__out PHCELL_INDEX ControlSetKeyCell);

					if(toupper(Key) == LkgMnemonic && *UseLastKnownGood == FALSE)
					{
						*UseLastKnownGood				= TRUE;
						BlpSwitchControlSet(ProfileList,DockInfoList,TRUE,ControlSetKeyCell);
						Restart							= TRUE;
					}
					else if(toupper(Key) == DefaultMnemonic && *UseLastKnownGood)
					{
						*UseLastKnownGood				= FALSE;
						BlpSwitchControlSet(ProfileList,DockInfoList,FALSE,ControlSetKeyCell);
						Restart							= TRUE;
					}
					break;
				}
			}
		}
	}

	if(ProfileCount > 0 && CurrentSelection < ProfileCount)
		BlUpdateProfileOption(*ProfileList,*ControlSetKeyCell,CurrentSelection);

	return TRUE;
}