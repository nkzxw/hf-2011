//********************************************************************
//	created:	11:8:2008   19:31
//	file:		comport.h
//	author:		tiamo
//	purpose:	comport
//********************************************************************

#pragma once

//
// initialize port
//
BOOLEAN BlPortInitialize(__in ULONG Baudrate,__in ULONG PortNum,__in ULONG PortAddr,__in BOOLEAN ReInitialize,__out PULONG DeviceId);

//
// enable fifo
//
VOID BlEnableFifo(__in ULONG DeviceId,__in UCHAR Fifo);

//
// receive byte
//
ULONG BlPortGetByte(__in ULONG DeviceId,__out PUCHAR Value);

//
// send byte
//
VOID BlPortPutByte(__in ULONG DeviceId,__in UCHAR Value);

//
// poll read byte
//
ULONG BlPortPollByte(__in ULONG DeviceId,__out PUCHAR Value);

//
// poll only
//
ULONG BlPortPollOnly(__in ULONG DeviceId);