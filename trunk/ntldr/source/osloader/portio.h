//********************************************************************
//	created:	11:8:2008   19:56
//	file:		portio.h
//	author:		tiamo
//	purpose:	port io
//********************************************************************

#pragma once

//
// read port uchar
//
UCHAR READ_PORT_UCHAR(__in PUCHAR Port);

//
// read port ushort
//
USHORT READ_PORT_USHORT(__in PUSHORT Port);

//
// read port ulong
//
ULONG READ_PORT_ULONG(__in PULONG Port);

//
// read buffer uchar
//
VOID READ_PORT_BUFFER_UCHAR(__in PUCHAR Port,__out PUCHAR Buffer,__in ULONG Count);

//
// read buffer ushort
//
VOID READ_PORT_BUFFER_USHORT(__in PUSHORT Port,__out PUSHORT Buffer,__in ULONG Count);

//
// read buffer ulong
//
VOID READ_PORT_BUFFER_ULONG(__in PULONG Port,__out PULONG Buffer,__in ULONG Count);

//
// write port uchar
//
VOID WRITE_PORT_UCHAR(__in PUCHAR Port,__in UCHAR Value);

//
// write port ushort
//
VOID WRITE_PORT_USHORT(__in PUSHORT Port,__in USHORT Value);

//
// write port ulong
//
VOID WRITE_PORT_ULONG(__in PULONG Port,__in ULONG Value);

//
// write buffer uchar
//
VOID WRITE_PORT_BUFFER_UCHAR(__in PUCHAR Port,__in PUCHAR Buffer,__in ULONG Count);

//
// write buffer ushort
//
VOID WRITE_PORT_BUFFER_USHORT(__in PUSHORT Port,__in PUSHORT Buffer,__in ULONG Count);

//
// write buffer ulong
//
VOID WRITE_PORT_BUFFER_ULONG(__in PULONG Port,__in PULONG Buffer,__in ULONG Count);

//
// read register uchar
//
UCHAR READ_REGISTER_UCHAR(__in PUCHAR Port);

//
// read register ushort
//
USHORT READ_REGISTER_USHORT(__in PUSHORT Port);

//
// read register ulong
//
ULONG READ_REGISTER_ULONG(__in PULONG Port);

//
// read buffer uchar
//
VOID READ_REGISTER_BUFFER_UCHAR(__in PUCHAR Port,__out PUCHAR Buffer,__in ULONG Count);

//
// read buffer ushort
//
VOID READ_REGISTER_BUFFER_USHORT(__in PUSHORT Port,__out PUSHORT Buffer,__in ULONG Count);

//
// read buffer ulong
//
VOID READ_REGISTER_BUFFER_ULONG(__in PULONG Port,__out PULONG Buffer,__in ULONG Count);

//
// write register uchar
//
VOID WRITE_REGISTER_UCHAR(__in PUCHAR Port,__in UCHAR Value);

//
// write register ushort
//
VOID WRITE_REGISTER_USHORT(__in PUSHORT Port,__in USHORT Value);

//
// write register ulong
//
VOID WRITE_REGISTER_ULONG(__in PULONG Port,__in ULONG Value);

//
// write buffer uchar
//
VOID WRITE_REGISTER_BUFFER_UCHAR(__in PUCHAR Port,__in PUCHAR Buffer,__in ULONG Count);

//
// write buffer ushort
//
VOID WRITE_REGISTER_BUFFER_USHORT(__in PUSHORT Port,__in PUSHORT Buffer,__in ULONG Count);

//
// write buffer ulong
//
VOID WRITE_REGISTER_BUFFER_ULONG(__in PULONG Port,__in PULONG Buffer,__in ULONG Count);

//
// yield processor
//
VOID KeYieldProcessor();

//
// read msr
//
VOID __fastcall RDMSR(__in ULONG Index);

//
// write msr
//
VOID WRMSR(__in ULONG Index,__in ULONG _EAX,__in ULONG _EDX);

//
// hal reboot
//
VOID HalpReboot();

//
// stop floppy
//
VOID MdShutoffFloppy();