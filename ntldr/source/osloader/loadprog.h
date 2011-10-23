//********************************************************************
//	created:	20:8:2008   0:43
//	file:		loadprog.h
//	author:		tiamo
//	purpose:	load progress
//********************************************************************

#pragma once

//
// loaded file count
//
extern ULONG											BlNumFilesLoaded;

//
// progressbar loaded file count
//
extern ULONG											BlNumProgressBarFilesLoaded;

//
// total files count
//
extern ULONG											BlProgressBarFilesToLoad;

//
// max files to load
//
extern ULONG											BlMaxFilesToLoad;

//
// show timeout
//
extern ULONG											BlProgressBarShowTimeOut;

//
// display boot logo
//
extern BOOLEAN											DisplayLogoOnBoot;

//
// graph mode
//
extern BOOLEAN											GraphicsMode;

//
// show progress bar
//
extern BOOLEAN											BlShowProgressBar;

//
// start time
//
extern ULONG											BlStartTime;

//
// display dot instead of file name
//
extern BOOLEAN											BlOutputDots;

//
// fatal error
//
VOID BlFatalError(__in ULONG ClassMessage,__in ULONG DetailMessage,__in ULONG ActionMessage);

//
// bad file message
//
VOID BlBadFileMessage(__in PCHAR BadFileName);

//
// redraw progressbar
//
VOID BlRedrawProgressBar();

//
// update progress bar
//
VOID BlUpdateProgressBar(__in ULONG Percent);

//
// update boot status
//
VOID BlUpdateBootStatus();

//
// output startup message
//
VOID BlOutputStartupMsg(__in ULONG MessageId);

//
// output trailer message
//
VOID BlOutputTrailerMsg(__in ULONG MessageId);

//
// output load message
//
VOID BlOutputLoadMessage(__in PCHAR LoadPartition,__in PCHAR LoadingFile,__in ULONG Unused);

//
// set progressbar characteristics
//
VOID BlSetProgBarCharacteristics(__in ULONG FrontMessageId,__in ULONG BackMessageId);