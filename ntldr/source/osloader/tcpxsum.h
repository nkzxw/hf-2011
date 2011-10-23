//********************************************************************
//	created:	27:9:2008   23:21
//	file:		tcpxsum.h
//	author:		tiamo
//	purpose:	tcp checksum
//********************************************************************

#pragma once

//
// tcpx checksum
//
ULONG tcpxsum(__in ULONG IntialValue,__in PVOID Buffer,__in ULONG Length);