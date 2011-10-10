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

#include "ExpressionParser.h"

#include <malloc.h>
#include "../../String/TrimString.h"
#include "../../String/StringConverter.h"
#include "../../LinkList/SingleThreaded/LinkListSingleThreaded.h"
#include "../../String/CodeParserHelper.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

// define operations priority order
#define CExpressionParser_MAX_OPERATOR_PRIORITY 5
CExpressionParser::TWO_OPERAND_OPERATOR_PRIORITY CExpressionParser::g_OperatorsPriority[]=
{
    {CExpressionParser::OPERATOR_MULT,         5 },
    {CExpressionParser::OPERATOR_DIV,          5 },

    {CExpressionParser::OPERATOR_PLUS,         4 },
    {CExpressionParser::OPERATOR_MINUS,        4 },

    {CExpressionParser::OPERATOR_BINARY_XOR,   3 },
    {CExpressionParser::OPERATOR_BINARY_OR,    3 },
    {CExpressionParser::OPERATOR_BINARY_AND,   3 },
    {CExpressionParser::OPERATOR_LEFT_SHIT,    3 },
    {CExpressionParser::OPERATOR_RIGHT_SHIT,   3 },

    {CExpressionParser::OPERATOR_BOOLEAN_OR,        2 },
    {CExpressionParser::OPERATOR_BOOLEAN_AND,       2 },
    {CExpressionParser::OPERATOR_LESS_OR_EQUAL,     2 },
    {CExpressionParser::OPERATOR_LESS,              2 },
    {CExpressionParser::OPERATOR_GREATER_OR_EQUAL,  2 },
    {CExpressionParser::OPERATOR_GREATER,           2 },
    {CExpressionParser::OPERATOR_BOOLEAN_EQUAL,     2 },

    {CExpressionParser::OPERATOR_TERNARY_FIRST_PART,   1 },
    {CExpressionParser::OPERATOR_TERNARY_SECOND_PART,  1 },

    {CExpressionParser::OPERATOR_EQUAL,  0 }
};

//-----------------------------------------------------------------------------
// Name: GetOperatorPriority
// Object: extract priority for operator from g_OperatorsPriority
// Parameters :
//      OPERATOR Operator
// Return : operator priority
//-----------------------------------------------------------------------------
SIZE_T CExpressionParser::GetOperatorPriority(CExpressionParser::OPERATOR Operator)
{
    // loop throw g_OperatorsPriority
    for (SIZE_T Cnt = 0; Cnt<_countof(g_OperatorsPriority); Cnt++)
    {
        // if operator matchs
        if (g_OperatorsPriority[Cnt].Operator == Operator)
            // return operator priority
            return g_OperatorsPriority[Cnt].Priority;
    }
    // by default
    return 0;
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: resolve a single operand value
// Parameters :
//              IN TCHAR* const Expression : operand name
//              OUT ULONG_PTR* pValue : operand value
//              IN pfStringToValueFunction const StringToValueFunction : function containing operand name to operand value definition map
//              IN PBYTE const StringToValueFunctionUserParam : StringToValueFunction user param
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CExpressionParser::GetValue(IN TCHAR* const Expression,
                                 OUT ULONG_PTR* pValue,
                                 IN pfStringToValueFunction const StringToValueFunction,
                                 IN PBYTE const StringToValueFunctionUserParam
                                 )
{
    // try to convert operand to value (by the way expression is "0x12345")
    if ( CStringConverter::StringToSIZE_T(Expression,pValue) )
        return TRUE;

    // if basic conversion from string to size_t fails, call StringToValueFunction if possible
    if (!StringToValueFunction)
        return FALSE;

    return StringToValueFunction(Expression,pValue,StringToValueFunctionUserParam);
}


//-----------------------------------------------------------------------------
// Name: ParseExpression
// Object: solves an expression
//                  LIMITATIONS :- parse expression without comments
//                               - integer value support only (no float/double, nor strings)
//
// Parameters :
//              IN TCHAR* const Expression : expression to parse
//              OUT ULONG_PTR* pValue : computed value
//              IN pfStringToValueFunction const StringToValueFunction : function containing operand name to operand value definition map
//              IN PBYTE const StringToValueFunctionUserParam : StringToValueFunction user param
//              IN pfReportError const ReportErrorFunction : function called to report errors
//              IN PBYTE const ReportErrorFunctionUserParam : ReportErrorFunction user param
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CExpressionParser::ParseExpression(IN TCHAR* const Expression,
                                        OUT ULONG_PTR* pValue,
                                        IN pfStringToValueFunction const StringToValueFunction,
                                        IN PBYTE const StringToValueFunctionUserParam,
                                        IN pfReportError const ReportErrorFunction,
                                        IN PBYTE const ReportErrorFunctionUserParam
                                        )
{
    BOOL bSuccess;
    TCHAR* pCurrentPosition;
    TCHAR* pNextSplitter;
    SIZE_T Depth;
    TCHAR* LocalExpression;
    TCHAR* SubExpressionBegin;
    EXPRESSION CurrentExpression;

    // default return value
    *pValue = 0;

    // make a local copy of expression to modify string
    LocalExpression = _tcsdup(Expression);

    CLinkListSingleThreaded ExpressionsList(sizeof(EXPRESSION));
    Depth = 0;
    bSuccess = TRUE;

    SubExpressionBegin = LocalExpression;
    pCurrentPosition = LocalExpression;

    // first loop : split operand 
    // break on 2 operands operators + - * / & | && || ^ < > << >>
    // and solve subexpressions ()
    for (pNextSplitter = pCurrentPosition;pNextSplitter;)
    {
        // break on supported operators and ()
        pNextSplitter = _tcspbrk(pCurrentPosition,_T("()+-*/^&|<>=?:") );
        
        // if no more operator
        if (pNextSplitter == NULL)
        {
            // check if ( and ) match
            if (Depth==0)
            {
                // no more operator --> OPERATOR_END_OF_EXPRESSION
                CurrentExpression.NextOperator = OPERATOR_END_OF_EXPRESSION;
                // get last operand content
                CurrentExpression.OperandString = SubExpressionBegin;
                // add to list of {operand,operator couples}
                ExpressionsList.AddItem(&CurrentExpression);
            }
            break;
        }

        // position after operator
        pCurrentPosition = pNextSplitter+1;

        // if operator is (
        if (*pNextSplitter == '(')
        {
            // increase depth
            Depth++;
            continue;
        }
        // if operator is )
        else if (*pNextSplitter == ')')
        {
            // decrease depth
            Depth--;
            continue;
        }
        if (Depth>0)
        {
            // sub expression will be parsed later
            continue;
        }

        // get operand
        CurrentExpression.OperandString = SubExpressionBegin;

        // according to operator
        switch (*pNextSplitter)
        {
        case '+':
            CurrentExpression.NextOperator = OPERATOR_PLUS;
            break;
        case '-':
            CurrentExpression.NextOperator = OPERATOR_MINUS;
            break;
        case '*':
            CurrentExpression.NextOperator = OPERATOR_MULT;
            break;
        case '/':
            CurrentExpression.NextOperator = OPERATOR_DIV;
            break;
        case '^':
            CurrentExpression.NextOperator = OPERATOR_BINARY_XOR;
            break;
        case '&':
            if (*(pNextSplitter+1)=='&')
            {
                CurrentExpression.NextOperator = OPERATOR_BOOLEAN_AND;
                pCurrentPosition++;
            }
            else
            {
                CurrentExpression.NextOperator = OPERATOR_BINARY_AND;
            }
            break;
        case '|':
            if (*(pNextSplitter+1)=='|')
            {
                CurrentExpression.NextOperator = OPERATOR_BOOLEAN_OR;
                pCurrentPosition++;
            }
            else
            {
                CurrentExpression.NextOperator = OPERATOR_BINARY_OR;
            }
            break;
        case '<':
            if (*(pNextSplitter+1)=='<')
            {
                CurrentExpression.NextOperator = OPERATOR_LEFT_SHIT;
                pCurrentPosition++;
            }
            if (*(pNextSplitter+1)=='=')
            {
                CurrentExpression.NextOperator = OPERATOR_LESS_OR_EQUAL;
                pCurrentPosition++;
            }
            else
            {
                CurrentExpression.NextOperator = OPERATOR_LESS;
            }
            break;
        case '>':
            if (*(pNextSplitter+1)=='>')
            {
                CurrentExpression.NextOperator = OPERATOR_RIGHT_SHIT;
                pCurrentPosition++;
            }
            if (*(pNextSplitter+1)=='=')
            {
                CurrentExpression.NextOperator = OPERATOR_GREATER_OR_EQUAL;
                pCurrentPosition++;
            }
            else
            {
                CurrentExpression.NextOperator = OPERATOR_GREATER;
            }
            break;
        case '=':
            if (*(pNextSplitter+1)=='=')
            {
                CurrentExpression.NextOperator = OPERATOR_BOOLEAN_EQUAL;
                pCurrentPosition++;
            }
            else
            {
                CurrentExpression.NextOperator = OPERATOR_EQUAL;
            }
            break;
        case '?':
            CurrentExpression.NextOperator = OPERATOR_TERNARY_FIRST_PART;
            break;
        case ':':
            CurrentExpression.NextOperator = OPERATOR_TERNARY_SECOND_PART;
            break;
        }
        // add current item to expression list
        ExpressionsList.AddItem(&CurrentExpression);

        // ends current subexpression
        *pNextSplitter = 0;

        // prepare next subexpression
        SubExpressionBegin = pCurrentPosition;
    }

    // in case of bad ( and ) match
    if (Depth!=0)
    {
        // report error
        if (ReportErrorFunction)
            ReportErrorFunction(_T("Syntax error bad ( ) matching"),ReportErrorFunctionUserParam);
        // free memory
        free(LocalExpression);
        // return
        return FALSE;
    }


    // look for single operand operators ! ~ and get string values
    CLinkListItem* pItem;
    EXPRESSION* pExpression;
    OPERATOR SingleOperandOperator;
    for (pItem = ExpressionsList.Head;pItem && bSuccess;pItem = pItem->NextItem)
    {
        pExpression = (EXPRESSION*)pItem->ItemData;
        pExpression->OperandString = CTrimString::TrimString(pExpression->OperandString);

        // check if there is single operator
        SingleOperandOperator = OPERATOR_NONE;
        switch(*pExpression->OperandString)
        {
            case '!':
                SingleOperandOperator = OPERATOR_BOOLEAN_NOT;
                pExpression->OperandString++;
                pExpression->OperandString = CTrimString::TrimString(pExpression->OperandString);
                break;
            case '~':
                SingleOperandOperator = OPERATOR_BINARY_NOT;
                pExpression->OperandString++;
                pExpression->OperandString = CTrimString::TrimString(pExpression->OperandString);
                break;
            default:
                SingleOperandOperator = OPERATOR_NONE;
                break;
        }

        // check for subexpression
        pNextSplitter = _tcspbrk(pExpression->OperandString,_T("()") );
        if (pNextSplitter)
        {
            // item can be like "(1+2*(1+4))"
            for (pCurrentPosition = pExpression->OperandString;pNextSplitter;pCurrentPosition = pNextSplitter+1)
            {
                // check for sub expression
                pNextSplitter = _tcspbrk(pCurrentPosition,_T("()") );

                // if no subexpression
                if (!pNextSplitter)
                    break;

                // if subexpression begin
                if (*pNextSplitter == '(')
                {
                    if (Depth == 0)
                        pExpression->OperandString = pNextSplitter+1;

                    // increase depth
                    Depth++;
                }
                // if subexpression end
                else if (*pNextSplitter == ')')
                {
                    // decrease depth
                    Depth--;
                    // if depth is null --> we have found the end of subexpression
                    if (Depth == 0)
                    {
                        // ends current operand
                        *pNextSplitter = 0;
                        break;
                    }
                }
            }
            // remove splitters from beginning of operand if any
            pExpression->OperandString = CCodeParserHelper::FindNextNotSplitterChar(pExpression->OperandString);

            // do recursive call to solve subexpression
            if (!CExpressionParser::ParseExpression(pExpression->OperandString,&pExpression->OperandValue, StringToValueFunction,StringToValueFunctionUserParam,ReportErrorFunction,ReportErrorFunctionUserParam))
                bSuccess = FALSE;// error message has been display by sub call
        }
        else // no subexpression : single operand
        {
            // remove splitter at the beginning of operand if any
            pExpression->OperandString = CCodeParserHelper::FindNextNotSplitterChar(pExpression->OperandString);
            // if no operand string
            if (*pExpression->OperandString==0)
            {
                // check the case of expressions like "+12" or "-45"
                if (pItem == ExpressionsList.Head)
                {
                    switch (pExpression->NextOperator)
                    {
                        case OPERATOR_PLUS:
                        case OPERATOR_MINUS:
                            // default left operand to 0
                            pExpression->OperandString = _T("0");
                            break;
                    }
                }
            }
            // ends current operand
            pNextSplitter = CCodeParserHelper::FindNextNotAlphaNumOrUnderscore(pExpression->OperandString);
            *pNextSplitter = 0;
            // resolve operand value (operand can be a value or a name)
            if (!CExpressionParser::GetValue(pExpression->OperandString, &pExpression->OperandValue, StringToValueFunction,StringToValueFunctionUserParam))
            {
                // on error, report error
                bSuccess = FALSE;
                if (ReportErrorFunction)
                {
                    TCHAR* Msg;
                    // avoid alloca in loop
                    Msg =new TCHAR[_tcslen(_T("Can't solve expression ")) + _tcslen(pExpression->OperandString) +1 ];
                    if (Msg)
                    {
                        _tcscpy(Msg,_T("Can't solve expression "));
                        _tcscat(Msg,pExpression->OperandString);
                        ReportErrorFunction(Msg,ReportErrorFunctionUserParam);
                        delete[] Msg;
                    }
                }
            }
        }

        // check success
        if (bSuccess)
        {
            // on success

            // resolve single operand operator
            switch (SingleOperandOperator)
            {
            case OPERATOR_BOOLEAN_NOT:
                pExpression->OperandValue = !pExpression->OperandValue;
                break;
            case OPERATOR_BINARY_NOT:
                pExpression->OperandValue = ~pExpression->OperandValue;
                break;
            }
        }
    }
    // check success
    if (!bSuccess)
    {
        // on error
        free(LocalExpression);
        return FALSE;
    }

    // on success
    // here ExpressionsList contains a list of left operand value and operator (all literals has been previously converted to value)

    SIZE_T Cnt;
    SIZE_T Priority;
    EXPRESSION* pNextExpression;
    CLinkListItem* pNextItem;

    // if list is not empty
    if (ExpressionsList.Head)
        // fill value with first operand value
        *pValue = ((EXPRESSION*)(ExpressionsList.Head->ItemData))->OperandValue;

    // for each operator priority
    for ( Cnt = 0; (Cnt<CExpressionParser_MAX_OPERATOR_PRIORITY) && bSuccess ; Cnt++)
    {
        Priority = CExpressionParser_MAX_OPERATOR_PRIORITY - Cnt;

        // if there's only a single operand
        if (ExpressionsList.GetItemsCount() == 1)
            // stop looping we have value
            break;

        // for each item, resolve operator matching wanted priority
        for (pItem = ExpressionsList.Head;(pItem != ExpressionsList.Tail) && bSuccess;pItem = pNextItem)
        {
            // store Next item as item can be removed from list during loop
            pNextItem = pItem->NextItem;
            pExpression = (EXPRESSION*)pItem->ItemData; // left operand + operator
            pNextExpression = (EXPRESSION*)pNextItem->ItemData; // right operand

            // if operator priority match
            if (GetOperatorPriority(pExpression->NextOperator) == Priority)
            {
                // store expression result in right operand and remove left operand (and computed operation) from list
                switch (pExpression->NextOperator)
                {
                case OPERATOR_PLUS:
                    pNextExpression->OperandValue = pExpression->OperandValue + pNextExpression->OperandValue;
                    break;
                case OPERATOR_MINUS:
                    pNextExpression->OperandValue = pExpression->OperandValue - pNextExpression->OperandValue;
                    break;
                case OPERATOR_MULT:
                    pNextExpression->OperandValue = pExpression->OperandValue * pNextExpression->OperandValue;
                    break;
                case OPERATOR_DIV:
                    if (pNextExpression->OperandValue == 0)
                    {
                        bSuccess = FALSE;
                        if (ReportErrorFunction)
                        {
                            ReportErrorFunction(_T("Error division by 0"),ReportErrorFunctionUserParam);
                        }
                    }
                    else
                    {
                        pNextExpression->OperandValue = pExpression->OperandValue / pNextExpression->OperandValue;
                    }
                    break;
                case OPERATOR_BOOLEAN_OR:
                    pNextExpression->OperandValue = pExpression->OperandValue || pNextExpression->OperandValue;
                    break;
                case OPERATOR_BOOLEAN_AND:
                    pNextExpression->OperandValue = pExpression->OperandValue && pNextExpression->OperandValue;
                    break;
                case OPERATOR_BINARY_OR:
                    pNextExpression->OperandValue = pExpression->OperandValue | pNextExpression->OperandValue;
                    break;
                case OPERATOR_BINARY_AND:
                    pNextExpression->OperandValue = pExpression->OperandValue & pNextExpression->OperandValue;
                    break;
                case OPERATOR_BINARY_XOR:
                    pNextExpression->OperandValue = pExpression->OperandValue ^ pNextExpression->OperandValue;
                    break;
                case OPERATOR_LEFT_SHIT:
                    pNextExpression->OperandValue = pExpression->OperandValue << pNextExpression->OperandValue;
                    break;
                case OPERATOR_LESS_OR_EQUAL:
                    pNextExpression->OperandValue = pExpression->OperandValue <= pNextExpression->OperandValue;
                    break;
                case OPERATOR_LESS:
                    pNextExpression->OperandValue = pExpression->OperandValue < pNextExpression->OperandValue;
                    break;
                case OPERATOR_RIGHT_SHIT:
                    pNextExpression->OperandValue = pExpression->OperandValue >> pNextExpression->OperandValue;
                    break;
                case OPERATOR_GREATER_OR_EQUAL:
                    pNextExpression->OperandValue = pExpression->OperandValue >= pNextExpression->OperandValue;
                    break;
                case OPERATOR_GREATER:
                    pNextExpression->OperandValue = pExpression->OperandValue > pNextExpression->OperandValue;
                    break;
                case OPERATOR_BOOLEAN_EQUAL:
                    pNextExpression->OperandValue = (pExpression->OperandValue == pNextExpression->OperandValue);
                    break;
                case OPERATOR_EQUAL:
                    // what can we do ???
                    // supposed to be a syntax error
                    if (ReportErrorFunction)
                    {
                        ReportErrorFunction(_T("Syntax error : single '=' found"),ReportErrorFunctionUserParam);
                        bSuccess = FALSE;
                    }
                    break;
                case OPERATOR_TERNARY_FIRST_PART:
                    {
                        CLinkListItem* pTernaryLastPartItem;
                        EXPRESSION* pTernaryLastPartExpression;

                        pTernaryLastPartItem = pNextItem->NextItem;
                        pTernaryLastPartExpression = (EXPRESSION*)pTernaryLastPartItem->ItemData;
                        if (pNextExpression->NextOperator != OPERATOR_TERNARY_SECOND_PART)
                        {
                            if (ReportErrorFunction)
                            {
                                ReportErrorFunction(_T("Syntax error : '?' found without matching ':'"),ReportErrorFunctionUserParam);
                                bSuccess = FALSE;
                            }
                        }
                        else
                        {
                            pTernaryLastPartExpression->OperandValue = pExpression->OperandValue ? pNextExpression->OperandValue : pTernaryLastPartExpression->OperandValue;

                            // remove the OPERATOR_TERNARY_SECOND_PART item
                            // and make pNextItem point to the operation after OPERATOR_TERNARY_SECOND_PART
                            ExpressionsList.RemoveItem(pNextItem);
                            pNextItem = pTernaryLastPartItem;
                        }

                    }
                    break;
                case OPERATOR_TERNARY_SECOND_PART:
                    // syntax error
                    if (ReportErrorFunction)
                    {
                        ReportErrorFunction(_T("Syntax error : ':' found without matching '?'"),ReportErrorFunctionUserParam);
                        bSuccess = FALSE;
                    }
                    break;
                }

                // remove left operand
                ExpressionsList.RemoveItem(pItem);
            }
        }
    }

    // check success
    if (bSuccess)
    {
        // ExpressionsList should only contains the solution
        if (ExpressionsList.Tail)
            *pValue = ((EXPRESSION*)(ExpressionsList.Tail->ItemData))->OperandValue;
    }

    // free memory
    free(LocalExpression);

    // return success state
    return bSuccess;
}