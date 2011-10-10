/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: Code Parser Helper
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CCodeParserHelper
{
public:
    static FORCEINLINE BOOL __fastcall CCodeParserHelper::IsSpaceOrSplitter(TCHAR const Char)
    {
        return ( (Char==' ') || (Char=='\t') || (Char=='\r') || (Char=='\n') );
    }

    // starts at Position
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindNextNotSplitterChar(TCHAR* const BeginPosition)
    {
        TCHAR* Position = BeginPosition;
        while ( CCodeParserHelper::IsSpaceOrSplitter(*Position)  && (*Position))
        {
            Position++;
        }
        return Position;
    }

    // starts at Position
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindNextSplitterChar(TCHAR* const BeginPosition)
    {
        TCHAR* Position = BeginPosition;
        while ( (!CCodeParserHelper::IsSpaceOrSplitter(*Position)) && (*Position) )
        {
            Position++;
        }
        return Position;
    }

    // starts at (Position-1)
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindPreviousNotSplitterChar(TCHAR* const String,TCHAR* const Position)
    {
        if (Position<=String)
            return String;

        TCHAR* pChar = Position;
        pChar--;
        while ( CCodeParserHelper::IsSpaceOrSplitter(*pChar)
            && (pChar>String) 
            )
        {
            pChar--;
        }
        return pChar;
    }

    // starts at (Position-1)
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindPreviousSplitterChar(TCHAR* const String,TCHAR* const Position)
    {
        if (Position<=String)
            return String;

        TCHAR* pChar = Position;
        pChar--;
        while ( !CCodeParserHelper::IsSpaceOrSplitter(*pChar)
            && (pChar>String) 
            )
        {
            pChar--;
        }
        return pChar;
    }

    static FORCEINLINE BOOL __fastcall CCodeParserHelper::IsAlphaNumOrUnderscore(TCHAR const Char)
    {
        return (    ( ('a' <= Char) && ( Char <= 'z') )
                 || ( ('A' <= Char) && ( Char <= 'Z') )
                 || ( ('0' <= Char) && ( Char <= '9') )
                 || (Char == '_')
                );
    }

    // starts at Position
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindNextNotAlphaNumOrUnderscore(TCHAR* const BeginPosition)
    {
        TCHAR* Position = BeginPosition;
        while (CCodeParserHelper::IsAlphaNumOrUnderscore(*Position) && (*Position))
        {
            Position++;
        }
        return Position;
    }

    // starts at Position
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindNextAlphaNumOrUnderscore(TCHAR* const BeginPosition)
    {
        TCHAR* Position = BeginPosition;
        while ((!CCodeParserHelper::IsAlphaNumOrUnderscore(*Position)) && (*Position))
        {
            Position++;
        }
        return Position;
    }

    // starts at (Position-1)
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindPreviousNotAlphaNumOrUnderscore(TCHAR* const String,TCHAR* const Position)
    {
        if (Position<=String)
            return String;
        TCHAR* pChar = Position;
        pChar--;
        while ( (CCodeParserHelper::IsAlphaNumOrUnderscore(*pChar) ) && (pChar>String) )
        {
            pChar--;
        }
        return pChar;
    }

    static FORCEINLINE BOOL __fastcall CCodeParserHelper::IsDecoratedFunctionNameChar(TCHAR const Char)
    {
        return ( CCodeParserHelper::IsAlphaNumOrUnderscore(Char)
                 || (Char == '$') || (Char == '@') || (Char == '?') // for name mangling (decorated function name)
            );
    }

    static FORCEINLINE BOOL __fastcall CCodeParserHelper::IsTypeDescriptorChar(TCHAR const Char)
    {
        return (    ( ('a' <= Char) && ( Char <= 'z') )
            || ( ('A' <= Char) && ( Char <= 'Z') )
            || ( ('0' <= Char) && ( Char <= '9') )
            || (Char == '_') || (Char == ':') || (Char == '<') || (Char == '>') // add : for class::type and <> for templates
            );
    }

    // starts at Position
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindNextNotTypeDescriptorChar(TCHAR* const BeginPosition)
    {
        TCHAR* Position = BeginPosition;
        while (CCodeParserHelper::IsTypeDescriptorChar(*Position) && (*Position))
        {
            Position++;
        }
        return Position;
    }

    // starts at Position
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindNextTypeDescriptorChar(TCHAR* const BeginPosition)
    {
        TCHAR* Position = BeginPosition;
        while ((!CCodeParserHelper::IsTypeDescriptorChar(*Position)) && (*Position))
        {
            Position++;
        }
        return Position;
    }

    // starts at (Position-1)
    static FORCEINLINE TCHAR* __fastcall CCodeParserHelper::FindPreviousNotTypeDescriptorChar(TCHAR* const String,TCHAR* const Position)
    {
        if (Position<=String)
            return String;
        TCHAR* pChar = Position;
        pChar--;
        while ( (CCodeParserHelper::IsTypeDescriptorChar(*pChar) ) && (pChar>String) )
        {
            pChar--;
        }
        return pChar;
    }


};
