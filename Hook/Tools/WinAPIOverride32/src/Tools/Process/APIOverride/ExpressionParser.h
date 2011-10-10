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
// Object: Parse and resolve an expression like
//                  "FOO1 +1 -5*(FOO1-FOO2) +( (1+ FOO2/FOO1) *2)"
//                  "FOO1 | FOO3" 
//                  "~FOO3" 
//                  "FOO2^FOO1" 
//                  "(FOO2<FOO1) ? FOO3 : 7" 
//-----------------------------------------------------------------------------
#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CExpressionParser
{
public:
    typedef BOOL (*pfStringToValueFunction)(IN TCHAR* const Expression,OUT ULONG_PTR* pValue, IN PBYTE UserParam);
    typedef void (*pfReportError)(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam);
    static BOOL ParseExpression(IN TCHAR* const Expression,OUT ULONG_PTR* pValue,IN pfStringToValueFunction const StringToValueFunction,IN PBYTE const StringToValueFunctionUserParam,IN pfReportError const ReportErrorFunction,IN PBYTE const ReportErrorFunctionUserParam);
protected:

    typedef enum tagOperator
    {
        OPERATOR_END_OF_EXPRESSION,// no more operator (for last item)
        OPERATOR_NONE,// no operator 
        OPERATOR_PLUS,
        OPERATOR_MINUS,
        OPERATOR_MULT,
        OPERATOR_DIV,
        OPERATOR_BOOLEAN_OR,
        OPERATOR_BOOLEAN_AND,
        OPERATOR_BOOLEAN_NOT,
        OPERATOR_BINARY_OR,
        OPERATOR_BINARY_AND,
        OPERATOR_BINARY_NOT,
        OPERATOR_BINARY_XOR,
        OPERATOR_LEFT_SHIT,
        OPERATOR_LESS_OR_EQUAL,
        OPERATOR_LESS,
        OPERATOR_RIGHT_SHIT,
        OPERATOR_GREATER_OR_EQUAL,
        OPERATOR_GREATER,
        OPERATOR_BOOLEAN_EQUAL,
        OPERATOR_EQUAL,
        OPERATOR_TERNARY_FIRST_PART,
        OPERATOR_TERNARY_SECOND_PART
    }OPERATOR;

    typedef struct tagExpression
    {
        OPERATOR  NextOperator;
        TCHAR*    OperandString;
        ULONG_PTR OperandValue;
        BOOL      OperandValueSolved;
    }EXPRESSION;


    typedef struct tagTwoOperandOperatorPriority
    {
        OPERATOR  Operator;
        SIZE_T    Priority;
    }TWO_OPERAND_OPERATOR_PRIORITY;
    static CExpressionParser::TWO_OPERAND_OPERATOR_PRIORITY g_OperatorsPriority[];

    static BOOL GetValue(IN TCHAR* const Expression,OUT ULONG_PTR* pValue,IN pfStringToValueFunction const StringToValueFunction,IN PBYTE const StringToValueFunctionUserParam);
    static SIZE_T CExpressionParser::GetOperatorPriority(OPERATOR Operator);
};
