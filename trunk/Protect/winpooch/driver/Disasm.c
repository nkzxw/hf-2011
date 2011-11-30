/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


/******************************************************************/
/* Build configuration                                            */
/******************************************************************/

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Disasm.h"

#ifdef __NTDDK__

// ddk's headers
#include <ntddk.h>

#else

// windows's headers
#include <windows.h>

#endif

// project's headers
#include "Assert.h"
#include "Trace.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define MOD(X)		(((X)>>6)&3)
#define OPCODE(X)	(((X)>>3)&7)
#define RM(X)		((X)&7)



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

int _Dasm_ReadModRmAndSid (int nCur, BYTE * pInst)
{
  int nSize = 0 ;

  TRACE_INFO (TEXT("inst=0x%02X, mod=%d, opcode=%d, rm=%d\n"), 
	      pInst[0], OPCODE(pInst[nCur]), MOD(pInst[nCur]), RM(pInst[nCur])) ;
  
  switch( MOD(pInst[nCur]) )
    {
    case 0: // Mod==00 => No disp

      // /!\ not valid if R/M is 4
      switch( RM(pInst[nCur]) )
	{   
	case 4: // an SIB follows
	  nSize = nCur + 2 ;
	  break ;   
	case 5:  // has disp32
	  nSize = nCur + 5 ;
	  break ;
	default:
	  nSize = nCur + 1 ;
	}

      break ; 
      
    case 1: // Mod==01 => 8 bits disp

      switch( RM(pInst[nCur]) )
	{ 
	case 4: // an SIB follows
	  nSize = nCur + 3 ;
	  break ;

	default:
	  nSize = nCur + 2 ;
	}

      break ;
      
    case 2: // Mod==10 => 32 bits disp

      // /!\ not valid if R/M is 4
      nSize = nCur + 5 ;

      if( RM(pInst[nCur])==4 )
	TRACE_WARNING (TEXT("Not tested instruction decoding\n")) ;
      
      break ;
      
    case 3:

      nSize = nCur + 1 ;

      break ; 
    }

  return nSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Dasm_GetInstructionSize (BYTE * pInst, UINT * pnSize, UINT * pnSizeIfMoved) 
{
  int nSize = 0 ;  
  int nSizeIfMoved = 0 ;

  ASSERT (pInst!=NULL) ;

  switch( pInst[0] )
    {
    case 0x71:
    case 0x72:	// JB rel8
    case 0x73:	// JAE rel8
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:	// JA rel8
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F:	// JG rel8
    case 0xE3:	// JECXZ rel8      
      nSize = 2 ;
      nSizeIfMoved = 5 ;
      break ;

    case 0x0F:
      switch( pInst[1] )
	{
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8E:
	case 0x8F:
	  nSize = 6 ;
	  break ;
	}
      break ;

    case 0x48:	// DEC EAX
    case 0x49:  // DEC ECX
    case 0x4A:	// DEC EDX
    case 0x4B:	// DEC EBX
    case 0x4C:	// DEC ESP
    case 0x4D:  // DEC EBP
    case 0x4E:	// DEC ESI
    case 0x4F:	// DEC EDI
    case 0x50:	// PUSH EAX    
    case 0x51:	// PUSH ECX
    case 0x52:	// PUSH EDX
    case 0x53:  // PUSH EBX
    case 0x54:  // PUSH ESP
    case 0x55:	// PUSH EBP
    case 0x56:	// PUSH ESI
    case 0x57:	// PUSH EDI
    case 0x58:	// POP	EAX
    case 0x59:	// POP	ECX
    case 0x5A:	// POP	EDX
    case 0x5B:	// POP	EBX
    case 0x5C:	// POP	ESP
    case 0x5D:	// POP	EBP
    case 0x5E:	// POP	ESI
    case 0x5F:	// POP	EDI
    case 0x60:	// PUSHAD
    case 0x61:	// PUSHAD
    case 0x90:	// NOP
    case 0x91:  // XCHG EAX,ECX
    case 0x92:	// XCHG EAX,EDX
    case 0x93:	// XCHG EAX,EBX
    case 0x94:	// XCHG EAX,ESP
    case 0x95:  // XCHG EAX,EBP
    case 0x96:	// XCHG EAX,ESI
    case 0x97:	// XCHG EAX,EDI
    case 0xC3:	// RET (near)
    case 0xCB:	// RET (far)
    case 0xCC:	// INT 3
    case 0xCE:	// INTO
      nSize = 1 ;
      break ;

    case 0x0C:	// OR AL,imm8
    case 0x3F:	// CMP AL,imm8
    case 0x6A:	// PUSH imm8
    case 0xA0:	// MOV AL,moffs8
    case 0xA2:	// MOV moffs8,AL
    case 0xB0:  // MOV AL, imm8
    case 0xB1:  // MOV CL, imm8
    case 0xB2:  // MOV DL, imm8
    case 0xB3:  // MOV BL, imm8
    case 0xB4:  // MOV AH, imm8
    case 0xB5:  // MOV CH, imm8
    case 0xB6:  // MOV DH, imm8
    case 0xB7:  // MOV BH, imm32
    case 0xCD:	// INT imm8
      nSize = 2 ;
      break ;

    case 0xC2:	// RET imm16 (near)
    case 0xCA:	// RET imm16 (far)
      nSize = 3 ;
      break ;

    case 0x0D:	// OR EAX, imm32
    case 0x3D:	// CMP EAX,imm32
    case 0x68:	// PUSH imm32
    case 0xA1:	// MOV EAX,moffs32
    case 0xA3:	// MOV moffs32,EAX
    case 0xB8:  // MOV EAX, imm32
    case 0xB9:  // MOV ECX, imm32
    case 0xBA:  // MOV EDX, imm32
    case 0xBB:  // MOV EBX, imm32
    case 0xBC:  // MOV ESP, imm32
    case 0xBD:  // MOV EBP, imm32
    case 0xBE:  // MOV ESI, imm32
    case 0xBF:  // MOV EDI, imm32
    case 0xE8:	// CALL rel32
    case 0xE9:	// JMP rel32
      nSize = 5 ;
      break ;

    case 0x00:	// ADD r/m8,r8
    case 0x01:	// ADD r/m32,r32
    case 0x02:	// ADD r8,r/m8
    case 0x03:	// ADD r32,r/m32
    case 0x08:	// OR r/m8,r8
    case 0x09:	// OR r/m16,r16
    case 0x0A:	// OR r8,r/m8
    case 0x0B:	// OR r32,m/r32
    case 0x33:	// XOR r32,r/m32
    case 0x38:	// CMP r/m8,r8
    case 0x39:	// CMP r/m32,r32
    case 0x3A:	// CMP r8,r/m8
    case 0x3B:	// CMP r32,r/m32
    case 0x85:	// TEST r/m32,r32
    case 0x88:	// MOV r/m8,r8
    case 0x89:	// MOV r/m32,r32
    case 0x8A:	// MOV r8,r/m8
    case 0x8B:	// MOV r32,r/m32
    case 0x8C:	// MOV r/m16,Sreg
    case 0x8D:	// LEA r32,m
    case 0x8E:	// MOV Sreg,r/m16
      nSize = _Dasm_ReadModRmAndSid (1, pInst) ;
      break ;

    case 0x64:  // FS instruction prefix
      if( Dasm_GetInstructionSize(pInst+1,&nSize,&nSizeIfMoved) )
	{
	  nSize += 1 ;
	  nSizeIfMoved += 1 ;
	}
      break ;

    case 0xC6:
      switch( OPCODE(pInst[1]) )
	{
	case 0:		// MOV r/m8,imm8
	  nSize = _Dasm_ReadModRmAndSid (1, pInst) + 1 ;
	  break ;
	}
      break ;

    case 0xC7:
      switch( OPCODE(pInst[1]) )
	{
	case 0:		// MOV r/m32,imm32
	  nSize = _Dasm_ReadModRmAndSid (1, pInst) + 4 ;
	  break ;
	}
      break ;

    case 0x80:      
      switch( OPCODE(pInst[1]) )
	{
	case 1:		// OR r/m8,imm8
	case 7:		// CMP r/m8,imm8 
	  nSize = _Dasm_ReadModRmAndSid (1, pInst) + 1 ;
	  break ;
	}
      break ;

    case 0x81:
      switch( OPCODE(pInst[1]) )
	{
	case 1:		// OR r/m32,imm32
	case 5:		// SUB r/m32,imm32
	case 7:		// CMP r/m32,imm32 
	  nSize = _Dasm_ReadModRmAndSid (1, pInst) + 4 ;
	  break ;
	}
      break ;

    case 0x83:      
      switch( OPCODE(pInst[1]) )
	{
	case 1:		// OR r/m32,imm8
	case 5:		// SUB r/m32,imm8
	case 7:		// CMP r/m32,imm8
	  nSize = _Dasm_ReadModRmAndSid (1, pInst) + 1 ;
	  break ;	  
	}
      break ;

    case 0xFF:	
      switch( OPCODE(pInst[1]) )
	{
	case 6: // PUSH r/m32
	  nSize = _Dasm_ReadModRmAndSid (1, pInst) ;
	  break ;
	}
      break ;

    default:
      TRACE_ERROR (TEXT("Unknown instruction 0x%02X\n"), pInst[0]) ;
      break ; 
    }

  if( nSizeIfMoved==0 ) nSizeIfMoved = nSize ;
  
  if( pnSize ) *pnSize = nSize ;
  if( pnSizeIfMoved ) *pnSizeIfMoved = nSizeIfMoved ;

  return nSize != 0 ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL  Dasm_MoveProgram (BYTE * pSrc, UINT nSrcSize,
			BYTE * pDst, UINT * pnDstSize, UINT nDstMax)
{
  int		nReadBytes = 0 ;
  int		nWritBytes = 0 ;
  int		nReadInstSize ;
  int		nWritInstSize ;
  BYTE		*pReadInst, *pWritInst ;
  DWORD		dwAddress ;
  BOOL		bSuccess ;

  if( nDstMax < nSrcSize ) {
    TRACE_ERROR (TEXT("Destination buffer too small\n")) ;
    return FALSE ;
  }

  while( nReadBytes < nSrcSize )
    {
      pReadInst = pSrc + nReadBytes ;
      pWritInst = pDst + nWritBytes ;

      bSuccess = Dasm_GetInstructionSize (pReadInst, &nReadInstSize, &nWritInstSize) ;
      
      if( ! bSuccess ) {
	TRACE_ERROR (TEXT("GetInstructionSize failed\n")) ;
	return FALSE ;
      }
      
      if( nReadBytes+nReadInstSize > nSrcSize ) {
	TRACE_ERROR (TEXT("Invalid source size\n")) ;
	return FALSE ;
      }

      if( nWritBytes+nWritInstSize > nDstMax ) {
	TRACE_ERROR (TEXT("Insufficient destination size\n")) ;
	return FALSE ;
      }

      if( pReadInst[0]==0xE8 || pReadInst[0]==0xE9 )
	{
	  // CALL rel32
	  // JMP rel32

	  ASSERT (nReadInstSize==nWritInstSize) ;

	  dwAddress = *(DWORD*)(pReadInst+1) ;	// get source relative
	  dwAddress += (DWORD)pReadInst ;	// convert to absolute
	  dwAddress -= (DWORD)pWritInst ;	// convert to relative

	  pWritInst[0] = pReadInst[0] ;
	  memcpy (pWritInst+1, &dwAddress, sizeof(DWORD)) ;

	  TRACE_WARNING (TEXT("JMP rel32 or CALL rel32 instruction patched (0x%02X)\n"), pReadInst[0]) ;
	}
      else if( ( pReadInst[0]==0x0F && pReadInst[1]>=0x81 && pReadInst[1]<=0x8F ) || pReadInst[0]==0xE3 )
	{
	  // Jxx rel32

	  ASSERT (nReadInstSize==nWritInstSize) ;
	  
	  dwAddress = *(DWORD*)(pReadInst+2) ;	// get source relative
	  dwAddress += (DWORD)pReadInst ;	// convert to absolute
	  dwAddress -= (DWORD)pWritInst ;	// convert to relative

	  pWritInst[0] = pReadInst[0] ;
	  pWritInst[1] = pReadInst[1] ;
	  memcpy (pWritInst+2, &dwAddress, sizeof(DWORD)) ;

	  TRACE_WARNING (TEXT("Jxx rel32 instruction patched (0x%02X)\n"), pReadInst[0]) ;
	}
      else if( pReadInst[0]>=0x71 && pReadInst[0]<=0x7F )
	{
	  // Jxx rel8

	  ASSERT (nReadInstSize+3==nWritInstSize) ;
	  
	  dwAddress = *(BYTE*)(pReadInst+1) ;	// get source relative
	  dwAddress += (DWORD)pReadInst ;	// convert to absolute
	  dwAddress -= (DWORD)pWritInst ;	// convert to relative

	  pWritInst[0] = 0x0F ;
	  pWritInst[1] = pReadInst[0] + 0x10 ;
	  memcpy (pWritInst+2, &dwAddress, sizeof(DWORD)) ;

	  TRACE_WARNING (TEXT("Jxx rel8 instruction patched (0x%02X)\n"), pReadInst[0]) ;
	}
      else
	{
	  // Any other instruction

	  ASSERT (nReadInstSize==nWritInstSize) ;

	  memcpy (pWritInst, pReadInst, nReadInstSize) ;
	}
      
      nReadBytes += nReadInstSize ;
      nWritBytes += nWritInstSize ;
    }
  
  ASSERT (nReadBytes==nSrcSize) ;

  *pnDstSize = nWritBytes ;

  return TRUE ;
}
