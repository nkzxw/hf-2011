//********************************************************************
//	created:	20:8:2008   22:04
//	file:		selkrnl.cpp
//	author:		tiamo
//	purpose:	select kernel
//********************************************************************

#include "stdafx.h"

//
// menu options
//
typedef struct _MENU_OPTION
{
	//
	// title
	//
	PCHAR												Title;

	//
	// path
	//
	PCHAR												Path;

	//
	// enable debug
	//
	BOOLEAN												EnableDebug;

	//
	// max memory
	//
	LONG												MaxMemory;

	//
	// load options
	//
	PCHAR												LoadOptions;

	//
	// scsi ordinal
	//
	LONG												ForcedScsiOrdinal;

	//
	// win95 or dos
	//
	LONG												Win95;

	//
	// redirect
	//
	BOOLEAN												Redirect;
}MENU_OPTION,*PMENU_OPTION;

//
// default switches
//
PCHAR													pDefSwitches;

//
// load options
//
CHAR													DebugLoadOptions[0x100];

//
// enable ems
//
BOOLEAN													RedirectEnabled;

//
// truncate descriptors
//
VOID BlpTruncateDescriptors(__in ULONG MaxPage);

//
// translate dos drive name to arc name
//
VOID BlpTranslateDosToArc(__in PCHAR DosName,__out PCHAR ArcName);

//
// parse os boot options
//
VOID BlParseOsOptions(__inout PMENU_OPTION MenuOption,__in PCHAR OptionsString)
{
	//
	// setup default values
	//
	MenuOption->EnableDebug								= FALSE;
	MenuOption->ForcedScsiOrdinal						= -1;
	MenuOption->LoadOptions								= 0;
	MenuOption->MaxMemory								= 0;
	MenuOption->Redirect								= 0;
	MenuOption->Win95									= 0;

	//
	// we can use a default switchs if we can not find a / in the options string
	//
	PCHAR Temp											= strchr(OptionsString,'/');
	if(!Temp && pDefSwitches)
		OptionsString									= pDefSwitches;

	//
	// upcase
	//
	_strupr(OptionsString);

	//
	// search scsi ordinal
	//
	Temp												= strstr(OptionsString,"/SCSIORDINAL:");
	if(Temp)
		MenuOption->ForcedScsiOrdinal					= atol(Temp + sizeof("/SCSIORDINAL:") - sizeof(CHAR));

	//
	// redirect
	//
	Temp												= strstr(OptionsString,"/REDIRECT");
	if(Temp)
		MenuOption->Redirect							= TRUE;

	//
	// load options
	//
	Temp												= strchr(OptionsString,'/');
	if(!Temp)
		return;

	Temp												= strchr(OptionsString + 1,'/');
	if(!Temp)
		return;

	//
	// found it
	//
	OptionsString										= Temp;
	MenuOption->LoadOptions								= Temp;

	//
	// max memory,why just MAXMEM instead of MAXMEM=
	//
	Temp												= strstr(OptionsString,"/MAXMEM");
	if(Temp)
		MenuOption->MaxMemory							= atol(Temp + sizeof("/MAXMEM"));

	//
	// win95
	//
	if(strstr(OptionsString,"/WIN95DOS"))
		MenuOption->Win95								= 1;
	else if(strstr(OptionsString,"/WIN95"))
		MenuOption->Win95								= 2;

	//
	// debug
	//
	if(strstr(OptionsString,"NODEBUG") || strstr(OptionsString,"CRASHDEBUG"))
		return;

	if(!strstr(OptionsString,"DEBUG") && !strstr(OptionsString,"BAUDRATE"))
		return;

	//
	// skip the option who will boot to another dos sector
	//
	if(!_stricmp(MenuOption->Path,"C:\\"))
		return;

	MenuOption->EnableDebug								= TRUE;
}

//
// convert file to lines array
//
PCHAR* BlpFileToLines(__in PCHAR File,__out PULONG LineCount)
{
	PCHAR p												= File;
	*LineCount											= 1;

	//
	// count the number of lines in the file so we know how large an array to allocate.
	//
	while(*p)
	{
		p												= strchr(p,'\n');
		if(!p)
			break;

		//
		// skip \n
		//
		p												+= 1;

		//
		// see if there's any text following the CR/LF.
		//
		if(*p == 0)
			break;

		*LineCount										+= 1;
	}

	//
	// allocate lines array
	//
	PCHAR* LineArray									= static_cast<PCHAR*>(BlAllocateHeap(*LineCount * sizeof(PCHAR)));

	//
	// step through the file again, replacing CR/LF with \0\0 and filling in the array of pointers.
	//
	p													= File;
	for(ULONG Line = 0; Line < *LineCount; Line ++)
	{
		LineArray[Line]									= p;

		p												= strchr(p,'\r');
		if(p)
		{
			//
			// replace \r to zero
			//
			*p											= 0;
			p											+= 1;

			//
			// the next char after \r is \n,also set it to zero,and advance p
			//
			if(*p == '\n')
			{
				*p										= 0;
				p										+= 1;
			}
		}
		else
		{
			//
			// unable to find \r,try \n again
			//
			p											= strchr(LineArray[Line],'\n');
			if(p)
			{
				*p										= 0;
				p										+= 1;
			}
		}

		//
		// remove trailing white space
		//
		PCHAR Space										= LineArray[Line] + strlen(LineArray[Line]) - 1;
		while(*Space == ' ' || *Space == '\t')
		{
			*Space										= 0;
			Space										-= 1;
		}
	}

	return LineArray;
}

//
// find section in ini file using case-sensitive search
//
PCHAR* BlpFindSection(__in PCHAR SectionName,__in PCHAR* BootFile,__in ULONG BootFileLines,__out PULONG NumberLines)
{
	ULONG i;
	for(i = 0; i < BootFileLines; i ++)
	{
		//
		// check to see if this is the line we are looking for
		//
		if(!_stricmp(BootFile[i],SectionName))
			break;
	}

	if(i == BootFileLines)
	{
		//
		// we ran out of lines, never found the right section.
		//
		*NumberLines									= 0;
		return 0;
	}

	ULONG StartLine										= i + 1;

	//
	// find end of section
	//
	for(i = StartLine; i < BootFileLines; i ++)
	{
		if(BootFile[i][0] == '[')
			break;
	}

	*NumberLines										= i - StartLine;

	return BootFile + StartLine;
}

//
// load bootsect.dos to 0x7c00 and jump to it
//
VOID BlpRebootDOS(__in_opt PCHAR BootSectorImage,__in_opt PCHAR RebootParam)
{
	//
	// HACKHACK John Vert (jvert)
	//	some SCSI drives get really confused and return zeroes when you use the BIOS to query their size after the AHA driver has initialized.
	//	this can completely tube OS/2 or DOS.
	//	so here we try and open both BIOS-accessible hard drives.
	//	our open code is smart enough to retry if it gets back zeros, so hopefully this will give the SCSI drives a chance to get their act together.
	//
	ULONG DriveId										= 0;
	ARC_STATUS Status									= ArcOpen("multi(0)disk(0)rdisk(0)partition(0)",ArcOpenReadOnly,&DriveId);
	if(Status == ESUCCESS)
		ArcCacheClose(DriveId);

	Status												= ArcOpen("multi(0)disk(0)rdisk(1)partition(0)",ArcOpenReadOnly,&DriveId);
	if(Status == ESUCCESS)
		ArcCacheClose(DriveId);

	//
	// load the boot sector at address 0x7C00 (expected by Reboot callback)
	//
	extern CHAR BootPartitionName[];
	Status												= ArcOpen(BootPartitionName,ArcOpenReadOnly,&DriveId);
	if(Status != ESUCCESS)
	{
		BlPrint(BlFindMessage(BL_REBOOT_IO_ERROR),BootPartitionName);
		while (1)
			BlGetKey();
	}

	ULONG SectorId										= 0;
	Status												= BlOpen(DriveId,BootSectorImage ? BootSectorImage : "\\bootsect.dos",ArcOpenReadOnly,&SectorId);
	if(Status != ESUCCESS)
	{
		if(BootSectorImage)
		{
			//
			// the boot sector image might actually be a directory.return to the caller to attempt standard boot.
			//
			BlClose(DriveId);
			return;
		}

		BlPrint(BlFindMessage(BL_REBOOT_IO_ERROR),BootPartitionName);
		while(1);
	}

	//
	// read it into 0x7c00
	//
	ULONG Read											= 0;
	Status												= BlRead(SectorId,reinterpret_cast<PVOID>(0x7c00),SECTOR_SIZE,&Read);
	if(Status != ESUCCESS)
	{
		BlPrint(BlFindMessage(BL_REBOOT_IO_ERROR),BootPartitionName);
		while(1);
	}

	//
	// the FAT boot code is only one sector long so we just want to load it up and jump to it.
	//
	// for HPFS and NTFS, we can't do this because the first sector loads the rest of the boot sectors
	// but we want to use the boot code in the boot sector image file we loaded.
	//
	// for HPFS, we load the first 20 sectors (boot code + super and space blocks) into d00:200.
	// fortunately this works for both NT and OS/2.
	//
	// for NTFS, we load the first 16 sectors and jump to d00:256.
	// if the OEM field of the boot sector starts with NTFS, we assume it's NTFS boot code.
	//

	//
	// try to read 8K from the boot code image.if this succeeds, we have either HPFS or NTFS.
	//
	LARGE_INTEGER SeekPosition;
	SeekPosition.QuadPart								= 0;
	BlSeek(SectorId,&SeekPosition,SeekAbsolute);
	BlRead(SectorId,reinterpret_cast<PVOID>(0xd000),SECTOR_SIZE * 16,&Read);

	//
	// default boot type is fat
	//
	ULONG BootType										= 0;
	if(Read == SECTOR_SIZE * 16)
	{
		if(memcmp(reinterpret_cast<PVOID>(0x7c03),"NTFS",4))
		{
			//
			// HPFS -- we need to load the super block.
			//
			BootType									= 1;

			SeekPosition.QuadPart						= 16 * SECTOR_SIZE;

			ArcSeek(DriveId,&SeekPosition,SeekAbsolute);
			ArcRead(DriveId,reinterpret_cast<PVOID>(0xf000),SECTOR_SIZE * 4,&Read);
		}
		else
		{
			//
			// NTFS -- we've loaded everything we need to load.
			//
			BootType									= 2;
		}
	}

	//
	// check cmdcons and ROLLBACK
	//
	if(RebootParam)
	{
		if(!_stricmp(RebootParam,"cmdcons"))
			RtlCopyMemory(reinterpret_cast<PVOID>(0x7c03),"cmdcons",sizeof("cmdcons"));
		else if(!strcmp(RebootParam,"ROLLBACK"))
			RtlCopyMemory(reinterpret_cast<PVOID>(0x7c03),"undo",sizeof("undo"));
	}

	//
	// dX must be the drive to boot from
	//	HACKHACKHACK...does this inline asm work?
	//
	_asm
	{
		mov dx,0x80
	}

	ExternalServicesTable->Reboot(BootType);
}

//
// rename win95 system file
//
ARC_STATUS BlpRenameWin95SystemFile(__in ULONG DriveId,__in ULONG Type,__in PCHAR FileTitle,__in PCHAR Ext,__in_opt PCHAR NewName)
{
	CHAR FileName[16];
	if(Type == 1)
	{
		sprintf(FileName,"%s.dos",FileTitle);
	}
	else
	{
		if(NewName)
			strcpy(FileName,NewName);
		else
			sprintf(FileName,"%s.w40",FileTitle);
	}

	ULONG FileId										= 0;
	ARC_STATUS Status									= BlOpen(DriveId,FileName,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return Status;

	CHAR CurrentFileName[16];
	sprintf(CurrentFileName,"%s.%s",FileTitle,Ext);
	ULONG FileIdCur										= 0;
	Status												= BlOpen(DriveId,CurrentFileName,ArcOpenReadOnly,&FileIdCur);
	if(Status != ESUCCESS)
	{
		BlClose(FileId);
		return Status;
	}

	CHAR NewFileName[16];
	if(Type == 1)
	{
		if(NewName)
			strcpy(NewFileName,NewName);
		else
			sprintf(NewFileName,"%s.w40",FileTitle);
	}
	else
	{
		sprintf(NewFileName,"%s.dos",FileTitle);
	}

	Status												= BlRename(FileIdCur,NewFileName);
	BlClose(FileIdCur);

	if(Status != ESUCCESS)
	{
		BlClose(FileId);
		return Status;
	}

	Status												= BlRename(FileId,CurrentFileName);

	BlClose(FileId);

	return Status;
}

//
// rename win95 system files
//
VOID BlpRenameWin95Files(__in ULONG DriveId,__in ULONG Type)
{
	BlpRenameWin95SystemFile(DriveId,Type,"command", "com",0);
	BlpRenameWin95SystemFile(DriveId,Type,"msdos",   "sys",0);
	BlpRenameWin95SystemFile(DriveId,Type,"io",      "sys","winboot.sys" );
	BlpRenameWin95SystemFile(DriveId,Type,"autoexec","bat",0);
	BlpRenameWin95SystemFile(DriveId,Type,"config",  "sys",0);
}

//
// input string
//
VOID BlInputString(__in ULONG PromptMessageId,__in ULONG StartX,__in ULONG StartY,__inout PCHAR Buffer,__in ULONG BufferSize)
{
	#pragma PRAGMA_MSG("Unimplemented!!!!")
}

//
// display menu
//
ULONG BlpPresentMenu(__in PMENU_OPTION MenuOption,__in ULONG Count,__in ULONG DefaultSelection,__in LONG TimeOut)
{
	//
	// get some strings....
	//
	PCHAR SelectOSMessage								= BlFindMessage(BL_SELECT_OS);
	PCHAR MoveHighLightMessage							= BlFindMessage(BL_MOVE_HIGHLIGHT);
	PCHAR TimeOutMessage								= BlFindMessage(BL_TIMEOUT_COUNTDOWN);
	PCHAR AdvancedBootPressF8Message					= BlFindMessage(BL_GOTO_ADVANCED_BOOT_F8);
	PCHAR DebuggerEnabledMessage						= BlFindMessage(BL_ENABLED_KD_TITLE);
	PCHAR EMSEnabledMessage								= BlFindMessage(BL_ENABLED_EMS_TITLE);

	//
	// file corrupted
	//
	if(!SelectOSMessage || !MoveHighLightMessage || !TimeOutMessage || !AdvancedBootPressF8Message || !DebuggerEnabledMessage || !EMSEnabledMessage)
		return DefaultSelection;

	//
	// remove trialer \r\n
	//
	PCHAR Temp											= strchr(TimeOutMessage,'\r');
	if(Temp)
		*Temp											= 0;

	Temp												= strchr(DebuggerEnabledMessage,'\r');
	if(Temp)
		*Temp											= 0;

	Temp												= strchr(EMSEnabledMessage,'\r');
	if(Temp)
		*Temp											= 0;

	//
	// if just only one option,there's only one choice
	//
	if(Count <= 1)
		TimeOut											= 0;

	//
	// even the timeout value is zero,always give user a choice to stop at here
	//
	if(TimeOut == 0)
	{
		ULONG Key										= BlGetKey();
		if(Key != F5_KEY && Key != F8_KEY)
			return DefaultSelection;

		TimeOut											= -1;
	}

	//
	// fit every menu item into just one line
	//
	for(ULONG i = 0; i < Count; i ++)
	{
		ULONG Length									= strlen(MenuOption[i].Title);
		ULONG EMSLength									= strlen(EMSEnabledMessage);
		ULONG KdLength									= strlen(DebuggerEnabledMessage);

		if(MenuOption[i].Redirect)
			Length										+= EMSLength;

		if(Length > 71)
		{
			if(MenuOption[i].Redirect)
				MenuOption[i].Title[71 - EMSLength]		= 0;
			else
				MenuOption[i].Title[71]					= 0;

			Length										= 71;
		}

		if(MenuOption[i].EnableDebug)
			Length										+= KdLength;

		if(Length > 71)
		{
			if(MenuOption[i].EnableDebug)
				MenuOption[i].Title[71 - KdLength]		= 0;
			else
				MenuOption[i].Title[71]					= 0;

			Length										= 71;
		}
	}

	//
	// HACK HACK HACK,
	//
	extern ULONG AdvancedBoot;

	//
	// setup selection index
	//
	ULONG CurrentSelection								= DefaultSelection;

	//
	// magic debug info switch,if you input with this string,we will display a debug screen instead of the normal screen
	//
	PCHAR MagicDebugInputSeq							= "unsupporteddebug";

	//
	// debug trace message
	//
	PCHAR DebugTraceMessage								= 0;

	//
	// we should redraw background info,if we are go into or get out from advanced boot screen,this value will be set
	//
	BOOLEAN RedrawBackground							= TRUE;

	//
	// need redraw menu
	//
	BOOLEAN RedrawMenu									= TRUE;

	//
	// start time
	//
	ULONG StartTime										= ExternalServicesTable->GetCounter();

	//
	// current time
	//
	ULONG CurrentTime									= StartTime;

	//
	// end time
	//
	ULONG EndTime										= StartTime + TimeOut * 182 / 10;

	//
	// last time
	//
	ULONG LastTime										= CurrentTime;

	//
	// bias time,see below
	//
	ULONG BiasTime										= 0;

	//
	// clear timeout
	//
	BOOLEAN ClearTimeout								= TRUE;

	//
	// last seconds
	//
	ULONG OldSecondsLeft								= 0;

	//
	// memu loop
	//
	while(CurrentTime < EndTime || TimeOut == -1)
	{
		//
		// need redraw background
		//
		if(RedrawBackground)
		{
			RedrawBackground							= FALSE;

			//
			// switch to normal mode
			//
			BlSetInverseMode(FALSE);

			//
			// clear screen
			//
			BlClearScreen();

			//
			// show advanced boot info
			//
			BlPositionCursor(1,24);

			if(AdvancedBoot != 0xffffffff)
			{
				//
				// use highlight blue to indicate we are using advanced boot
				//
				ULONG Dummy;
				ArcWrite(BlConsoleOutDeviceId,"\x1B[1;34m",7,&Dummy);

				//
				// show advanced boot disply string,such as "safe mode"
				//
				BlPrint(BlGetAdvancedBootDisplayString(AdvancedBoot));

				//
				// restore to normal mode
				//
				BlSetInverseMode(FALSE);
			}
			else
			{
				BlClearToEndOfLine();
			}

			//
			// press F8 to goto advanced boot screen
			//
			BlPositionCursor(1,22);
			BlPrint(AdvancedBootPressF8Message);

			//
			// select os to start
			//
			BlPositionCursor(1,3);
			BlPrint(SelectOSMessage);

			//
			// use up and down to move selection
			//
			BlPositionCursor(1,5 + Count);
			BlPrint(MoveHighLightMessage);
		}

		//
		// need redraw menu
		//
		if(RedrawMenu)
		{
			RedrawMenu									= FALSE;

			for(ULONG i = 0; i < Count; i ++)
			{
				PMENU_OPTION Current					= MenuOption + i;

				BlPositionCursor(1,6 + i);

				//
				// highlight current selection
				//
				BlSetInverseMode(i == CurrentSelection ? TRUE : FALSE);

				BlPrint("    %s",Current->Title);

				if(Current->Redirect)
					BlPrint(EMSEnabledMessage);

				if(Current->EnableDebug)
					BlPrint(DebuggerEnabledMessage);

				//
				// restore to normal mode
				//
				BlSetInverseMode(FALSE);
			}

			//
			// show debug message
			//
			if(DebugTraceMessage)
			{
				BlPositionCursor(1,Count + 7);

				BlClearToEndOfScreen();

				//
				// copy load options
				//
				if(MenuOption[CurrentSelection].LoadOptions)
					_snprintf(DebugLoadOptions,ARRAYSIZE(DebugLoadOptions) - 1,"%s",MenuOption[CurrentSelection].LoadOptions);
				else
					DebugLoadOptions[0]					= 0;

				BlPrint(DebugTraceMessage,MenuOption[CurrentSelection].Title,MenuOption[CurrentSelection].Path,DebugLoadOptions);
			}
		}

		//
		// if debug trace is enabled,we skip timeout check,because you are debugging me
		//
		if(!DebugTraceMessage)
		{
			//
			// timeout is enabled
			//
			if(TimeOut != -1)
			{
				LastTime								= CurrentTime;
				CurrentTime								= ExternalServicesTable->GetCounter();

				//
				// deal with wraparound at midnight
				// we can't do it the easy way because there are not exactly 18.2 * 60 * 60 * 24 tics/day.  (just approximately)
				//
				if(CurrentTime < StartTime)
				{
					if(BiasTime == 0)
						BiasTime						= LastTime + 1;

					CurrentTime							+= BiasTime;
				}

				LONG SecondsLeft						= static_cast<LONG>(EndTime - CurrentTime) * 10 / 182;

				//
				// note that if the user hits the PAUSE key, the counter stops and, as a result, SecondsLeft can become < 0.
				//
				if (SecondsLeft < 0)
					SecondsLeft							= 0;

				if(OldSecondsLeft != SecondsLeft)
				{
					OldSecondsLeft						= SecondsLeft;

					BlPositionCursor(1,5 + Count);
					BlPrint(MoveHighLightMessage);
					BlPrint(TimeOutMessage);
					BlPrint(" %d \n",SecondsLeft);
				}
			}
			else if(ClearTimeout)
			{
				ClearTimeout							= FALSE;
				BlPositionCursor(1,5 + Count);
				BlPrint(MoveHighLightMessage);
				BlPrint("                                                                      \n");
			}
		}

		//
		// read input
		//
		ULONG Key										= BlGetKey();
		if(!Key)
			continue;

		//
		// any key will stop timeout
		//
		TimeOut											= -1;

		//
		// check magic input seq
		//
		if(Key == *MagicDebugInputSeq)
		{
			//
			// goto the next char
			//
			MagicDebugInputSeq							+= 1;

			if(*MagicDebugInputSeq == 0)
			{
				//
				// you have inputed the whole magic string,give you the debug screen
				//
				DebugTraceMessage						= BlFindMessage(BL_TRACE_OS_SELECTION_INFO);
				RedrawMenu								= TRUE;
			}

			continue;
		}

		//
		// if the key is not the one in the magic string
		// reset the magic string,you need input it from the beginning
		//
		MagicDebugInputSeq							= "unsupporteddebug";

		//
		// up,down,home,end will move current selection
		//
		ULONG OldSelection							= CurrentSelection;
		if(Key == UP_ARROW)
			CurrentSelection						= (CurrentSelection + Count - 1) % Count;
		else if(Key == DOWN_ARROW)
			CurrentSelection						= (CurrentSelection + 1) % Count;
		else if(Key == HOME_KEY)
			CurrentSelection						= 0;
		else if(Key == END_KEY)
			CurrentSelection						= Count - 1;

		RedrawMenu									= OldSelection != CurrentSelection ? TRUE : FALSE;

		//
		// like printscreen will debug break into debugger,we use F10 to break into debugger
		//
		if(BdDebuggerEnabled && Key == F10_KEY)
			DbgBreakPoint();

		//
		// F5 and F8 will display the advanced boot screen
		//
		if(Key == F5_KEY || Key == F8_KEY)
		{
			//
			// show advanced boot screen and wait user to select one
			//
			AdvancedBoot							= BlDoAdvancedBoot(BL_ADVANCED_BOOT_MENU_PROMPT,AdvancedBoot,0,0);

			//
			// user selecto to reboot machine,do it now
			//
			if(AdvancedBoot != 0xffffffff && BlGetAdvancedBootID(AdvancedBoot) == BL_ADVANCED_BOOT_REBOOT_COMPUTER)
			{
				BlClearScreen();

				BdStopDebugger();

				ExternalServicesTable->RebootProcessor();
			}

			//
			// redraw everything
			//
			RedrawMenu								= TRUE;
			RedrawBackground						= TRUE;
		}

		//
		// enter will select current selection
		//
		if(Key == '\r')
		{
			//
			// if you are using debug screen,you have a chance to edit the load options before actually boot windows
			//
			if(DebugTraceMessage)
			{
				BlClearScreen();
				BlPositionCursor(1,3);
				BlClearToEndOfScreen();

				//
				// show current selected os info,see msg.h for more info
				//
				PMENU_OPTION Current				= MenuOption + CurrentSelection;
				BlPrint(DebugTraceMessage,Current->Title,Current->Path,DebugLoadOptions);

				//
				// input load option string
				//
				BlInputString(BL_PRMOPT_ENTER_NEW_LOAD_OPTIONS,0,7,DebugLoadOptions,ARRAYSIZE(DebugLoadOptions) - 1);

				//
				// and parse it
				//
				BlParseOsOptions(Current,DebugLoadOptions);
			}

			break;
		}
	}

	return CurrentSelection;
}

//
// select kernel
//
PCHAR BlSelectKernel(__in ULONG DriveId,__in PCHAR IniFileContents,__inout PCHAR* LoadOptions,__in BOOLEAN UseTimeout)
{
	CHAR DefaultPathBuffer[0x100];
	PCHAR DefaultTitle									= BlFindMessage(BL_DEFAULT_TITLE);
	PCHAR DefaultPath									= DefaultPathBuffer;

	//
	// try to determinate default path is winnt or windows
	//
	ULONG DirectoryId									= 0;
	ARC_STATUS Status									= BlOpen(DriveId,"winnt",ArcOpenDirectory,&DirectoryId);
	if(Status == ESUCCESS)
	{
		//
		// winnt directory exsit,default path is c:\winnt
		//
		strcpy(DefaultPath,"c:\\winnt\\");

		BlClose(DirectoryId);
	}
	else
	{
		//
		// unable to open winnt directory,assume default path is windows
		//
		strcpy(DefaultPath,"c:\\windows\\");
	}

	//
	// we will read boot.ini and build this menu options
	//
	MENU_OPTION MenuOption[11];
	ULONG NumberSystems									= 0;
	ULONG FileLineCount									= 0;
	ULONG MbLineCount									= 0;
	ULONG OsLineCount									= 0;
	ULONG DebugLineCount								= 0;
	PCHAR* FileLines									= 0;
	PCHAR* MbLines										= 0;
	PCHAR* OsLines										= 0;
	PCHAR* DebugLines									= 0;

	if(!IniFileContents || *IniFileContents == 0)
	{
		//
		// the boot.ini is an empty file,use a default menu
		//
		BlPrint(BlFindMessage(BL_INVALID_BOOT_INI),DefaultPath);

		MenuOption[0].EnableDebug						= FALSE;
		MenuOption[0].ForcedScsiOrdinal					= -1;
		MenuOption[0].LoadOptions						= 0;
		MenuOption[0].MaxMemory							= 0;
		MenuOption[0].Path								= DefaultPath;
		MenuOption[0].Redirect							= FALSE;
		MenuOption[0].Title								= DefaultTitle;
		MenuOption[0].Win95								= 0;
		NumberSystems									= 1;
	}
	else
	{
		//
		// convert boot.ini to lines array
		//
		FileLines										= BlpFileToLines(IniFileContents,&FileLineCount);

		//
		// search [boot loader] section
		//
		MbLines											= BlpFindSection("[boot loader]",FileLines,FileLineCount,&MbLineCount);

		//
		// if failed,try [flexboot] section
		//
		if(!MbLines)
			MbLines										= BlpFindSection("[flexboot]",FileLines,FileLineCount,&MbLineCount);

		//
		// then try [multiboot]
		//
		if(!MbLines)
			MbLines										= BlpFindSection("[multiboot]",FileLines,FileLineCount,&MbLineCount);

		//
		// search [operating systems]
		//
		OsLines											= BlpFindSection("[operating systems]",FileLines,FileLineCount,&OsLineCount);
		if(!OsLines)
		{
			//
			// unable to find [operation systems] sector
			// return if we are booting from net
			//
			if(BlBootingFromNet)
				return 0;

			//
			// make a default one
			//
			BlPrint(BlFindMessage(BL_INVALID_BOOT_INI),DefaultPath);

			MenuOption[0].EnableDebug					= FALSE;
			MenuOption[0].ForcedScsiOrdinal				= -1;
			MenuOption[0].LoadOptions					= 0;
			MenuOption[0].MaxMemory						= 0;
			MenuOption[0].Path							= DefaultPath;
			MenuOption[0].Redirect						= FALSE;
			MenuOption[0].Title							= DefaultTitle;
			MenuOption[0].Win95							= 0;
			NumberSystems								= 1;
		}

		//
		// search [debug]
		//
		DebugLines										= BlpFindSection("[debug]",FileLines,FileLineCount,&DebugLineCount);
	}

	//
	// timeout count
	//
	LONG TimeOut										= UseTimeout ? -1 : 0;

	//
	// parse [boot loader] sector
	//
	for(ULONG i = 0; i < MbLineCount; i ++)
	{
		PCHAR CurrentLine								= MbLines[i];

		//
		// throw away any leading whitespace
		//
		CurrentLine										+= strspn(CurrentLine," \t");

		//
		// result an empty line,ignore it
		//
		if(*CurrentLine == 0)
			continue;

		if(!_strnicmp(CurrentLine,"DefSwitches",11))
		{
			//
			// this line setup a default switches
			//
			PCHAR Value									= strchr(CurrentLine,'=');
			if(Value)
				pDefSwitches							= Value + 1;
		}
		else if(!_strnicmp(CurrentLine,"timeout",7))
		{
			//
			// timeout
			//
			PCHAR Value									= strchr(CurrentLine,'=');
			if(Value && UseTimeout)
				TimeOut									= atol(Value + 1);
		}
		else if(!_strnicmp(CurrentLine,"default",7))
		{
			//
			// default operation system
			//
			PCHAR Value									= strchr(CurrentLine,'=');
			if(Value)
			{
				DefaultPath								= Value + 1;
				DefaultPath								+= strspn(DefaultPath," \t");
			}
		}
		else if(!_strnicmp(CurrentLine,"redirectbaudrate",16))
		{
			//
			// redirect baudrate
			//
			PCHAR Value									= strchr(CurrentLine,'=');
			if(Value)
			{
				Value									+= 1;
				Value									+= strspn(Value," \t");
			}

			if(Value && *Value)
			{
				LoaderRedirectionInfo.Baudrate			= 9600;
				if(!_strnicmp(Value,"115200",6))
					LoaderRedirectionInfo.Baudrate		= 115200;
				else if(!_strnicmp(Value,"57600",5))
					LoaderRedirectionInfo.Baudrate		= 57600;
				else if(!_strnicmp(Value,"19200",5))
					LoaderRedirectionInfo.Baudrate		= 19200;
			}
		}
		else if(!_strnicmp(CurrentLine,"redirect",8))
		{
			//
			// redirect port
			//
			PCHAR Value									= strchr(CurrentLine,'=');
			if(Value)
			{
				Value									+= 1;
				Value									+= strspn(Value," \t");
			}

			if(Value && *Value)
			{
				if(!_strnicmp(Value,"com",3))
				{
					//
					// com port number
					//
					LoaderRedirectionInfo.PortNum		= static_cast<ULONG>(atol(Value + 3));
				}
				else if(!_strnicmp(Value,"usebiossettings",15))
				{
					//
					// apci table
					//
					BlRetrieveBIOSRedirectionInformation(&LoaderRedirectionInfo);
				}
				else
				{
					//
					// port address
					//
					LoaderRedirectionInfo.PortAddress	= strtoul(Value,0,0x10);
					if(LoaderRedirectionInfo.PortAddress)
						LoaderRedirectionInfo.PortNum	= 3;
				}
			}
		}
	}

	//
	// initialize serial console
	//
	BlInitializeHeadlessPort();

	//
	// parse [operation systems] section
	//
	for(ULONG i = 0; i < OsLineCount; i ++)
	{
		if(NumberSystems >= ARRAYSIZE(MenuOption) - 1)
			break;

		PCHAR CurrentLine								= OsLines[i];

		//
		// throw away any leading whitespace
		//
		CurrentLine										+= strspn(CurrentLine," \t");

		//
		// result an empty line,ignore it
		//
		if(*CurrentLine == 0)
			continue;

		//
		// first part is arc path
		//
		MenuOption[NumberSystems].Path					= CurrentLine;

		//
		// the first space or '=' character indicates the end of the path specifier, so we need to replace it with a '\0'
		//
		while(*CurrentLine != ' ' && *CurrentLine != '='&& *CurrentLine != 0)
			CurrentLine									+= 1;

		*CurrentLine									= 0;

		//
		// the next character that is not space, equals, or double-quote is the start of the title.
		//
		CurrentLine										+= 1;

		while(*CurrentLine == ' ' || *CurrentLine == '=' || *CurrentLine == '"')
			CurrentLine									+= 1;

		//
		// no title was found, so just use the path as the title.
		//
		if(*CurrentLine == 0)
			MenuOption[NumberSystems].Title				= DefaultTitle;
		else
			MenuOption[NumberSystems].Title				= CurrentLine;

		//
		// the next character that is either a double-quote or a \0 indicates the end of the title.
		//
		while(*CurrentLine != 0 && *CurrentLine != '"')
			CurrentLine									+= 1;

		//
		// the left part is load options,parse it
		//
		BlParseOsOptions(MenuOption + NumberSystems,CurrentLine);

		//
		// trim load options from title
		//
		*CurrentLine									= 0;

		//
		// inc count
		//
		NumberSystems									+= 1;
	}

	//
	// parse [debug] section
	//
	for(ULONG i = 0; i < DebugLineCount; i ++)
	{
		PCHAR CurrentLine								= DebugLines[i];

		//
		// throw away any leading whitespace
		//
		CurrentLine										+= strspn(CurrentLine," \t");

		//
		// result an empty line,ignore it
		//
		if(*CurrentLine == 0)
			continue;

	#if _SCSI_SUPPORT_
		//
		// scsi debug flags
		//
		if(!_strnicmp(CurrentLine,"scsidebug",9))
		{
			PCHAR Value									= strchr(CurrentLine,'=');
			if(Value)
				ScsiDebug								= static_cast<ULONG>(atol(Value + 1));
		}
	#endif
		//
		// initialize debugger
		//
		if(!_strnicmp(CurrentLine,"/debug ",7))
			BdInitDebugger("osloader.exe",OsLoaderBase,CurrentLine);
	}

	//
	// look for a Title entry from the [operating systems] section that matches the default entry from the [multiboot] section. this will give us a title.
	// if no entry matches, we will add an entry at the end of the list and provide a default Title.
	//
	ULONG Selection;
	for(Selection = 0; Selection < NumberSystems; Selection ++)
	{
		if(!_stricmp(MenuOption[Selection].Path,DefaultPath))
			break;
	}

	if(Selection == NumberSystems)
	{
		//
		// create a default entry in the title and path arrays
		//
		MenuOption[NumberSystems].Path					= DefaultPath;
		MenuOption[NumberSystems].Title					= DefaultTitle;
		MenuOption[NumberSystems].EnableDebug			= FALSE;
		MenuOption[NumberSystems].MaxMemory				= 0;
		MenuOption[NumberSystems].LoadOptions			= 0;
		MenuOption[NumberSystems].Win95					= 0;
		MenuOption[NumberSystems].ForcedScsiOrdinal		= -1;
		MenuOption[NumberSystems].EnableDebug			= FALSE;
		MenuOption[NumberSystems].Redirect				= FALSE;
		NumberSystems									+= 1;
	}

	//
	// show menu
	//
	Selection											= BlpPresentMenu(MenuOption,NumberSystems,Selection,TimeOut);

	//
	// setup output params
	//
	if(MenuOption[Selection].LoadOptions)
	{
		*LoadOptions									= MenuOption[Selection].LoadOptions + 1;

		//
		// replace / with space
		//
		PCHAR Options									= *LoadOptions;
		for(ULONG i = 0; Options[i]; i ++)
		{
			if(Options[i] == '/')
				Options[i]								= ' ';
		}
	}
	else
	{
		*LoadOptions									= 0;
	}

	//
	// set redirect flags
	//
	RedirectEnabled											= MenuOption[Selection].Redirect;

	//
	// rename win95 system files
	//
	if(MenuOption[Selection].Win95)
		BlpRenameWin95Files(DriveId,MenuOption[Selection].Win95);

	//
	// boot bootsect.dos
	//
	if(!_strnicmp(MenuOption[Selection].Path,"C:\\",3))
		BlpRebootDOS(MenuOption[Selection].Path[3] ? MenuOption[Selection].Path + 2 : 0,*LoadOptions);

	//
	// translate the DOS name into an ARC name
	//
	static CHAR Kernel[128];
	if(MenuOption[Selection].Path[1] == ':')
	{
		CHAR DosName[3]									= {MenuOption[Selection].Path[0],MenuOption[Selection].Path[1],0};

		BlpTranslateDosToArc(DosName,Kernel);

		strcat(Kernel,MenuOption[Selection].Path + 2);
	}
	else
	{
		strcpy(Kernel,MenuOption[Selection].Path);
	}

	//
	// append advanced boot options
	//
	if(BlGetAdvancedBootOption() != 0xffffffff)
	{
		PCHAR AdvancedBootLoadOptions					= BlGetAdvancedBootLoadOptions(BlGetAdvancedBootOption());
		if(AdvancedBootLoadOptions && strlen(AdvancedBootLoadOptions))
		{
			//
			// why multi by 4?
			//
			ULONG Length								= (strlen(AdvancedBootLoadOptions) + (*LoadOptions ? strlen(*LoadOptions) : 0) + 2) * 4;
			PCHAR Buffer								= static_cast<PCHAR>(BlAllocateHeap(Length));
			if(Buffer)
			{
				if(*LoadOptions)
				{
					strcpy(Buffer,*LoadOptions);
					strcat(Buffer," ");
				}
				else
				{
					Buffer[0]							= 0;
				}

				strcat(Buffer,AdvancedBootLoadOptions);

				*LoadOptions							= Buffer;
			}
		}

		BlDoAdvancedBootLoadProcessing(BlGetAdvancedBootOption());
	}

	//
	// make sure there is no trailing slash
	//
	if(Kernel[strlen(Kernel) - 1] == '\\')
		Kernel[strlen(Kernel) -1 ]						= 0;

	//
	// truncate memory,
	//	MAXMEM is in unit of MB,convert it to 4K PAGE size by left shift 8bits
	//
	if(MenuOption[Selection].MaxMemory)
		BlpTruncateDescriptors((MenuOption[Selection].MaxMemory << 8) - 1);

#if _SCSI_SUPPORT_
	//
	// set scsi ordinal
	//
	ForcedScsiOrdinal									= MenuOption[Selection].ForcedScsiOrdinal;
#endif

	return Kernel;
}