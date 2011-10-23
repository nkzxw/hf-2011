//********************************************************************
//	created:	20:8:2008   0:41
//	file:		loadprog.cpp
//	author:		tiamo
//	purpose:	load progress
//********************************************************************

#include "stdafx.h"

//
// loaded file count
//
ULONG													BlNumFilesLoaded;

//
// progressbar loaded file count
//
ULONG													BlNumProgressBarFilesLoaded;

//
// total files count
//
ULONG													BlProgressBarFilesToLoad;

//
// max files to load
//
ULONG													BlMaxFilesToLoad = 0x50;

//
// show timeout
//
ULONG													BlProgressBarShowTimeOut = 3;

//
// display boot logo
//
BOOLEAN													DisplayLogoOnBoot;

//
// graph mode
//
BOOLEAN													GraphicsMode;

//
// show progress bar
//
BOOLEAN													BlShowProgressBar;

//
// start time
//
ULONG													BlStartTime;

//
// display dot instead of file name
//
BOOLEAN													BlOutputDots;

//
// front message id
//
ULONG													BlProgBarFrontMsgID = BL_PROGRESS_BAR_FRONT_CHAR;

//
// back message id
//
ULONG													BlProgBarBackMsgID = BL_PROGRESS_BAR_BACK_CHAR;

//
// front char
//
ULONG													BlFrontChar = 0xdb;

//
// back char
//
ULONG													BlBackChar	= 0x20;

//
// x position
//
ULONG													ProgressBarXPosition;

//
// y position
//
ULONG													ProgressBarYPosition;

//
// width
//
ULONG													ProgressBarWidth = 80;

//
// fatal error
//
VOID BlFatalError(__in ULONG ClassMessage,__in ULONG DetailMessage,__in ULONG ActionMessage)
{
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,"\r\n",2,&Count);

	//
	// remove any remains from the last known good message
	//
	BlClearToEndOfScreen();

	CHAR Buffer[40];
	PCHAR Text											= BlFindMessage(ClassMessage);
	if(!Text)
	{
		sprintf(Buffer,"%08lx\r\n",ClassMessage);
		Text											= Buffer;
	}

	ArcWrite(BlConsoleOutDeviceId,Text,strlen(Text),&Count);

	Text												= BlFindMessage(DetailMessage);
	if(!Text)
	{
		sprintf(Buffer,"%08lx\r\n",DetailMessage);
		Text											= Buffer;
	}

	ArcWrite(BlConsoleOutDeviceId,Text,strlen(Text),&Count);

	Text												= BlFindMessage(ActionMessage);

	if(!Text)
	{
		sprintf(Buffer,"%08lx\r\n",ActionMessage);
		Text											= Buffer;
	}

	ArcWrite(BlConsoleOutDeviceId,Text,strlen(Text),&Count);
}

//
// bad file message
//
VOID BlBadFileMessage(__in PCHAR BadFileName)
{
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,"\r\n",2,&Count);

	BlClearToEndOfScreen();

	PCHAR Temp											= BlFindMessage(LOAD_SW_MIS_FILE_CLASS);
	if(Temp)
		ArcWrite(BlConsoleOutDeviceId,Temp,strlen(Temp),&Count);

	ArcWrite(BlConsoleOutDeviceId,BadFileName,strlen(BadFileName),&Count);

	ArcWrite(BlConsoleOutDeviceId,"\r\n\r\n",4,&Count);

	Temp												= BlFindMessage(LOAD_SW_FILE_REST_ACT);
	if(Temp)
		ArcWrite(BlConsoleOutDeviceId,Temp,strlen(Temp),&Count);
}

//
// update graph progress bar
//
VOID BlUpdateGfxProgressBar(__in ULONG Percent)
{
}

//
// update text progress bar
//
VOID BlUpdateTxtProgressBar(__in ULONG Percent)
{
	//
	// disable progress bar
	//
	if(!BlShowProgressBar || !BlOutputDots)
		return;

	CHAR LineBuffer[81 * 4];
	CHAR FrontBuffer[82 * 2];
	CHAR BackBuffer[82 * 2];
	RtlZeroMemory(FrontBuffer,sizeof(FrontBuffer));
	RtlZeroMemory(BackBuffer,sizeof(BackBuffer));
	RtlZeroMemory(LineBuffer,sizeof(LineBuffer));

	//
	// setup position
	//
	if(!ProgressBarXPosition)
	{
		if(ScreenWidth <= 80)
		{
			ProgressBarXPosition						= 1;
			ProgressBarWidth							= ScreenWidth;
		}
		else
		{
			ProgressBarXPosition						= (ScreenWidth - 80) / 2;
			ProgressBarWidth							= 80;
		}
	}

	if(!ProgressBarYPosition)
		ProgressBarYPosition							= ScreenHeight - 2;

	//
	// fix buggy percent
	//
	if(Percent > 100)
		Percent											= 100;

	//
	// calc chars count
	//
	ULONG FrontCount									= Percent * ProgressBarWidth / 100;
	ULONG BackCount										= ProgressBarWidth - FrontCount;

	//
	// fill front buffer with front char
	//
	if(FrontCount)
	{
		if(FrontCount >= sizeof(FrontBuffer))
			FrontCount									= sizeof(FrontBuffer);

		if(BlFrontChar & 0xff00)
		{
			ULONG Count									= (FrontCount + 1) / 2;
			for(ULONG i = 0; i < Count; i ++)
				*Add2Ptr(FrontBuffer,i * 2,PUSHORT)		= static_cast<USHORT>(BlFrontChar);
		}
		else
		{
			for(ULONG i = 0; i < FrontCount; i ++)
				FrontBuffer[i]							= static_cast<UCHAR>(BlFrontChar);
		}
	}

	//
	// and the same to the back buffer
	//
	if(BackCount)
	{
		if(BackCount >= sizeof(BackBuffer))
			BackCount									= sizeof(BackBuffer);

		if(BlBackChar & 0xff00)
		{
			ULONG Count									= (BackCount + 1) / 2;
			for(ULONG i = 0; i < Count; i ++)
				*Add2Ptr(BackBuffer,i * 2,PUSHORT)		= static_cast<USHORT>(BlBackChar);
		}
		else
		{
			for(ULONG i = 0; i < BackCount; i ++)
				BackBuffer[i]							= static_cast<UCHAR>(BlBackChar);
		}
	}

	//
	// make the line buffer
	//
	strcpy(LineBuffer,FrontBuffer);
	strcat(LineBuffer,BackBuffer);

	//
	// output it
	//
	BlPositionCursor(ProgressBarXPosition,ProgressBarYPosition);
	ArcWrite(BlConsoleOutDeviceId,LineBuffer,strlen(LineBuffer),&FrontCount);
}

//
// update progress bar
//
VOID BlUpdateProgressBar(__in ULONG Percent)
{
	if(DisplayLogoOnBoot)
		BlUpdateGfxProgressBar(Percent);
	else
		BlUpdateTxtProgressBar(Percent);
}

//
// redraw progress bar
//
VOID BlRedrawProgressBar()
{
	if(!BlShowProgressBar)
	{
		if(!BlProgressBarShowTimeOut || ArcGetRelativeTime() - BlStartTime > BlProgressBarShowTimeOut)
		{
			BlProgressBarFilesToLoad					= BlMaxFilesToLoad - BlNumFilesLoaded;
			BlShowProgressBar							= TRUE;
			BlNumProgressBarFilesLoaded					= 1;
		}
	}

	if(BlShowProgressBar && BlProgressBarFilesToLoad)
		BlUpdateProgressBar(BlNumProgressBarFilesLoaded * 100 / BlProgressBarFilesToLoad);
}

//
// update boot status
//
VOID BlUpdateBootStatus()
{
	BlNumFilesLoaded									+= 1;

	if(BlShowProgressBar)
		BlNumProgressBarFilesLoaded						+= 1;

	BlRedrawProgressBar();
}

//
// output startup message
//
VOID BlOutputStartupMsgStr(__in PCHAR MessageString)
{
	//
	// should not output any text
	//
	if(DisplayLogoOnBoot || !BlOutputDots)
		return;

	if(!MessageString)
		return;

	//
	// clear screen
	//
	BlClearScreen();

	//
	// normal mode
	//
	BlSetInverseMode(FALSE);

	//
	// calc x position
	//
	LONG PositionX										= ScreenWidth / 2 - strlen(MessageString) / 2;
	if(PositionX > static_cast<LONG>(ScreenWidth))
		PositionX										= 1;

	//
	// set cursor position
	//
	BlPositionCursor(PositionX,ScreenHeight - 3);

	//
	// write string
	//
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,MessageString,strlen(MessageString),&Count);

	//
	// update progress bar
	//
	BlRedrawProgressBar();
}

//
// output startup message
//
VOID BlOutputStartupMsg(__in ULONG MessageId)
{
	if(DisplayLogoOnBoot || !BlOutputDots)
		return;

	BlOutputStartupMsgStr(BlFindMessage(MessageId));
}

//
// output trailer message string
//
VOID BlOutputTrailerMsgStr(__in PCHAR MessageString)
{
	if(DisplayLogoOnBoot || !BlOutputDots)
		return;

	if(!MessageString)
		return;

	//
	// set cursor position
	//
	BlPositionCursor(1,ScreenHeight);

	//
	// copy to local buffer
	//
	CHAR Buffer[0x100];
	strcpy(Buffer,MessageString);

	//
	// remove \r\n
	//
	ULONG Length										= strlen(Buffer);
	if(Length >= 2 && Buffer[Length - 2] == '\r' && Buffer[Length - 1] == '\n')
	{
		Buffer[Length - 2]								= 0;
		Length											-= 2;
	}

	//
	// output it
	//
	ArcWrite(BlConsoleOutDeviceId,Buffer,Length,&Length);
}

//
// output trailer message
//
VOID BlOutputTrailerMsg(__in ULONG MessageId)
{
	BlOutputTrailerMsgStr(BlFindMessage(MessageId));
}

//
// output load message
//
VOID BlOutputLoadMessage(__in PCHAR LoadPartition,__in PCHAR LoadingFile,__in ULONG Unused)
{
	if(DisplayLogoOnBoot || BlOutputDots)
		return;

	CHAR Buffer[256];
	strcpy(Buffer,"  ");
	strcat(Buffer,LoadPartition);

	if(LoadingFile)
		strcat(Buffer,LoadingFile);

	strcat(Buffer,"\r\n");

	BlLogPrint(LOG_LOGFILE,Buffer);

	ArcWrite(BlConsoleOutDeviceId,Buffer,strlen(Buffer),&Unused);
}

//
// set progressbar characteristics
//
VOID BlSetProgBarCharacteristics(__in ULONG FrontMessageId,__in ULONG BackMessageId)
{
	BlProgBarFrontMsgID									= FrontMessageId;
	BlProgBarBackMsgID									= BackMessageId;

	PCHAR Temp											= BlFindMessage(FrontMessageId);
	ULONG Length										= Temp ? strlen(Temp) : 0;
	if(Length == 1 || (Length > 2 && Temp[1] == '\r' && Temp[2] == '\n'))
		BlFrontChar										= static_cast<UCHAR>(Temp[0]);
	else if(Length && (Length == 2 || Temp[1] != '\r'))
		BlFrontChar										= *reinterpret_cast<PUSHORT>(Temp);

	Temp												= BlFindMessage(BackMessageId);
	Length												= Temp ? strlen(Temp) : 0;
	if(Length == 1 || (Length > 2 && Temp[1] == '\r' && Temp[2] == '\n'))
		BlBackChar										= static_cast<UCHAR>(Temp[0]);
	else if(Length && (Length == 2 || Temp[1] != '\r'))
		BlBackChar										= *reinterpret_cast<PUSHORT>(Temp);

	if(BlFrontChar & 0xff00)
	{
		if(BlBackChar & 0xff00)
			return;

		BlBackChar										|= ((BlBackChar & 0xff) << 8);
	}
	else if(BlBackChar & 0xff00)
	{
		BlFrontChar										|= ((BlFrontChar & 0xff) << 8);
	}
}