//********************************************************************
//	created:	10:8:2008   23:21
//	file:		terminal.cpp
//	author:		tiamo
//	purpose:	serial console
//********************************************************************

#include "stdafx.h"

//
// first entry
//
BOOLEAN FirstEntry										= TRUE;

//
// screen width
//
ULONG ScreenWidth										= 80;

//
// screen height
//
ULONG ScreenHeight										= 25;

//
// line position
//
ULONG													LinePosition;

//
// redirection info
//
LOADER_REDIRECTION_INFORMATION							LoaderRedirectionInfo;

//
// terminal device id
//
ULONG													BlTerminalDeviceId;

//
// terminal delay
//
ULONG													BlTerminalDelay;

//
// line buffer
//
CHAR													TerminalLine[96];

//
// connected
//
BOOLEAN													BlTerminalConnected;

//
// console initialized
//
BOOLEAN													BlConsoleInitialized;

//
// clear screen
//
VOID BlClearScreen()
{
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,"\x1B[2J",4,&Count);
}

//
// clear to end of screen
//
VOID BlClearToEndOfScreen()
{
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,"\x1B[J",3,&Count);
}

//
// clear to end of line
//
VOID BlClearToEndOfLine()
{
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,"\x1B[K",3,&Count);
}

//
// set inverse mode
//
VOID BlSetInverseMode(__in ULONG Mode)
{
	if(Mode)
		ArcWrite(BlConsoleOutDeviceId,"\x1B[7m",4,&Mode);
	else
		ArcWrite(BlConsoleOutDeviceId,"\x1B[0m",4,&Mode);
}

//
// set cursor position
//
VOID BlPositionCursor(__in ULONG x,__in ULONG y)
{
	CHAR Buffer[20];

	sprintf(Buffer,"\x1B[%d;%dH",y,x);

	ULONG Length										= strlen(Buffer);

	ArcWrite(BlConsoleOutDeviceId,Buffer,Length,&Length);
}

//
// initialize terminal
//
VOID BlInitializeHeadlessPort()
{
	//
	// get bios redirection info
	//
	if(!LoaderRedirectionInfo.PortNum || !LoaderRedirectionInfo.PortAddress)
		BlRetrieveBIOSRedirectionInformation(&LoaderRedirectionInfo);

	if(!LoaderRedirectionInfo.PortNum)
	{
		BlTerminalConnected								= FALSE;
		return;
	}

	//
	// guest port address from port number
	//
	if(!LoaderRedirectionInfo.PortAddress)
	{
		switch(LoaderRedirectionInfo.PortNum)
		{
		case 2:
			LoaderRedirectionInfo.PortAddress			= 0x2f8;
			break;

		case 3:
			LoaderRedirectionInfo.PortAddress			= 0x3e8;
			break;

		case 4:
			LoaderRedirectionInfo.PortAddress			= 0x2e8;
			break;

		default:
			LoaderRedirectionInfo.PortAddress			= 0x3f8;
			break;
		}
	}

	//
	// set default baudrate
	//
	if(!LoaderRedirectionInfo.Baudrate)
		LoaderRedirectionInfo.Baudrate					= 9600;

	//
	// open port
	//
	BlTerminalConnected									= BlPortInitialize(LoaderRedirectionInfo.Baudrate,LoaderRedirectionInfo.PortNum,
																		   LoaderRedirectionInfo.PortAddress,BlTerminalConnected,&BlTerminalDeviceId);

	if(!BlTerminalConnected)
	{
		//
		// port open failed
		//
		RtlZeroMemory(&LoaderRedirectionInfo,sizeof(LoaderRedirectionInfo));
		return;
	}

	//
	// enable fifo
	//
	BlEnableFifo(BlTerminalDeviceId,1);

	//
	// load guid
	//
	BlLoadGUID(&LoaderRedirectionInfo.Guid);

	//
	// calc delay count
	//
	BlTerminalDelay										= 1000000 / (LoaderRedirectionInfo.Baudrate / 10) * 10 / 6;

	for(ULONG i = 0; i < sizeof("\x1B[m") - 1; i ++)
	{
		BlPortPutByte(BlTerminalDeviceId,"\x1B[m"[i]);
		FwStallExecution(BlTerminalDelay);
	}
}

//
// initialize stdio
//
ARC_STATUS BlInitStdio(__in ULONG Argc,__in PCHAR Argv[])
{
	//
	// already initialized
	//
	if(BlConsoleInitialized)
		return ESUCCESS;

	//
	// if we are booting with serial redirect(SAC),draw a progess bar instead of windows logo
	//
	if(BlTerminalConnected)
	{
		extern BOOLEAN BlShowProgressBar;
		extern BOOLEAN DisplayLogoOnBoot;

		BlShowProgressBar								= TRUE;
		DisplayLogoOnBoot								= FALSE;
	}

	//
	// invalid device name
	//
	PCHAR ConsoleOutDeviceName							= BlGetArgumentValue(Argc,Argv,"consoleout");
	if(!ConsoleOutDeviceName && !BlTerminalConnected)
		return ENODEV;

	//
	// open console out
	//
	ARC_STATUS Status									= ArcOpen(ConsoleOutDeviceName,ArcOpenWriteOnly,&BlConsoleOutDeviceId);
	if(Status != ESUCCESS && !BlTerminalConnected)
		return Status;

	//
	// invalid console in device name?
	//
	PCHAR ConsoleInDeviceName							= BlGetArgumentValue(Argc,Argv,"consolein");
	if(!ConsoleInDeviceName && !BlTerminalConnected)
		return ENODEV;

	//
	// open console in
	//
	Status												= ArcOpen(ConsoleInDeviceName,ArcOpenReadOnly,&BlConsoleInDeviceId);
	if(Status != ESUCCESS && !BlTerminalConnected)
		return Status;

	BlConsoleInitialized								= TRUE;
	return Status;
}

//
// restart
//
BOOLEAN BlpRestartCmd(__in PCHAR CmdLine)
{
	return TRUE;
}

//
// process command
//
BOOLEAN BlpDoCommand(__in PCHAR CmdLine)
{
	typedef BOOLEAN (*PPROCESS_COMMAND_ROUTINE)(__in PCHAR CmdLine);

	typedef struct _COMMAND_INFO
	{
		//
		// process
		//
		PPROCESS_COMMAND_ROUTINE						mProcess;

		//
		// name
		//
		PCCHAR											mCmd;

		//
		// help message
		//
		PCCHAR											mHelpMessage;
	}COMMAND_INFO;

	static COMMAND_INFO info[] =
	{
		&BlpRestartCmd,				"restart",		"Restart the system immediately.",
	};

	for(ULONG i = 0; i < ARRAYSIZE(info); i ++)
	{
		if(!_stricmp(info[i].mCmd,CmdLine))
			return info[i].mProcess(CmdLine);
	}

	ULONG Count											= 0;
	if(!_stricmp(CmdLine,"help") || !_stricmp(CmdLine,"?"))
	{
		for(ULONG i = 0; i < ARRAYSIZE(info); i ++)
		{
			ArcWrite(BlConsoleOutDeviceId,info[i].mCmd,strlen(info[i].mCmd),&Count);
			ArcWrite(BlConsoleOutDeviceId,"    ",4,&Count);
			ArcWrite(BlConsoleOutDeviceId,info[i].mHelpMessage,strlen(info[i].mHelpMessage),&Count);
			ArcWrite(BlConsoleOutDeviceId,"\r\n",2,&Count);
		}
	}
	else
	{
		CHAR InvalidCmd[]								= "Invalid Command, use \'?\' for help.\r\n";
		ArcWrite(BlConsoleOutDeviceId,InvalidCmd,sizeof(InvalidCmd) - sizeof(CHAR),&Count);
	}

	return FALSE;
}

//
// handle loader failure
//
BOOLEAN BlTerminalHandleLoaderFailure()
{
	if(!BlTerminalConnected)
		return TRUE;

	ULONG Count											= 0;
	static CHAR Prompt[]								= "!SAC> ";
	static ULONG PromptLength							= sizeof(Prompt) - 1;
	static CHAR NewLine[]								= "\r\n";
	static ULONG NewLineLength							= sizeof(NewLine) - 1;

	//
	// enter this routine first time
	//
	if(FirstEntry)
	{
		FirstEntry										= FALSE;

		//
		// set cursor position
		//
		BlPositionCursor(1,ScreenHeight);

		//
		// send a new line
		//
		ArcWrite(BlConsoleOutDeviceId,NewLine,NewLineLength,&Count);

		//
		// send prompt
		//
		ArcWrite(BlConsoleOutDeviceId,Prompt,PromptLength,&Count);
	}

	//
	// get input status
	//
	if(ArcGetReadStatus(BlConsoleInDeviceId) != ESUCCESS)
		return FALSE;

	//
	// get key
	//
	ULONG Key												= BlGetKey();
	if(Key == ESC_KEY)
	{
		//
		// set cursor position
		//
		BlPositionCursor(1,ScreenHeight);

		//
		// send a new line
		//
		ArcWrite(BlConsoleOutDeviceId,NewLine,NewLineLength,&Count);

		//
		// send prompt
		//
		ArcWrite(BlConsoleOutDeviceId,Prompt,PromptLength,&Count);

		//
		// reset line buffer
		//
		LinePosition									= 0;
		RtlZeroMemory(TerminalLine,sizeof(TerminalLine));

		return FALSE;
	}
	else if(Key == BACKSPACE_KEY)
	{
		if(!LinePosition)
			return FALSE;

		BlPositionCursor(LinePosition + PromptLength,ScreenHeight);

		ArcWrite(BlConsoleOutDeviceId," ",1,&Count);

		BlPositionCursor(LinePosition + PromptLength,ScreenHeight);

		LinePosition									-= 1;
		TerminalLine[LinePosition]						= 0;

		return FALSE;
	}
	else if(Key == TAB_KEY || Key == ASCI_HT)
	{
		if(LinePosition < ARRAYSIZE(TerminalLine) - 1 - 4)
		{
			ArcWrite(BlConsoleOutDeviceId,"    ",4,&Count);
			TerminalLine[LinePosition ++]				= ' ';
			TerminalLine[LinePosition ++]				= ' ';
			TerminalLine[LinePosition ++]				= ' ';
			TerminalLine[LinePosition ++]				= ' ';
			TerminalLine[LinePosition]					= 0;
		}

		return FALSE;
	}
	else if(Key == '\r')
	{
		TerminalLine[LinePosition]						= 0;

		ArcWrite(BlConsoleOutDeviceId,NewLine,NewLineLength,&Count);

		if(LinePosition && BlpDoCommand(TerminalLine))
			return TRUE;

		//
		// set cursor position
		//
		BlPositionCursor(1,ScreenHeight);

		//
		// send a new line
		//
		ArcWrite(BlConsoleOutDeviceId,NewLine,NewLineLength,&Count);

		//
		// send prompt
		//
		ArcWrite(BlConsoleOutDeviceId,Prompt,PromptLength,&Count);

		LinePosition									= 0;
		RtlZeroMemory(TerminalLine,sizeof(TerminalLine));

		return FALSE;
	}

	if(Key != (Key & 0x7f))
		return FALSE;

	if(LinePosition < ARRAYSIZE(TerminalLine) - 1)
	{
		TerminalLine[LinePosition]						= static_cast<CHAR>(Key & 0x7f);

		if(LinePosition >= ScreenWidth - PromptLength * 2)
			BlPositionCursor(LinePosition + PromptLength,ScreenHeight);
		else
			LinePosition								+= 1;

		ArcWrite(BlConsoleOutDeviceId,&Key,1,&Count);
	}

	return FALSE;;
}