#pragma once

#include <string>

namespace std
{
#if (defined(UNICODE)||defined(_UNICODE))
    typedef wstring _tstring;
    #define _ttoupper towupper
    #define _ttolower towlower
#else
    typedef string _tstring;
    #define _ttoupper toupper
    #define _ttolower tolower
#endif
    class tstring:public virtual std::_tstring
    {
    public:
        tstring():std::_tstring(){};
        tstring(std::_tstring str):std::_tstring(str){};
        tstring& operator=(const tstring& _Right)
        {	// assign _Right
            this->std::_tstring::assign((const std::_tstring)_Right);
            return (*this);           
        }

        tstring& operator=(const TCHAR*_Ptr)
        {	// assign [_Ptr, <null>)
            this->std::_tstring::assign(_Ptr);
            return (*this);
        }

        tstring& operator=(TCHAR _Ch)
        {	// assign 1 * _Ch
            this->std::_tstring::assign(1, _Ch);
            return (*this);
        }

        tstring ToUpper()
        {
            for(tstring::iterator i=this->begin(), iend = this->end(); i!=iend ; i++)
            {
                *i = _ttoupper(*i);
            }
            return (*this);
        }

        tstring ToUpper(size_t Begin)
        {
            for(tstring::iterator i=this->begin()+Begin, iend = this->end(); i!=iend ; i++)
            {
                *i = _ttoupper(*i);
            }
            return (*this);
        }

        tstring ToUpper(size_t Begin, size_t End)
        {
            for(tstring::iterator i=this->begin()+Begin, iend = this->begin()+End; i!=iend ; i++)
            {
                *i = _ttoupper(*i);
            }
            return (*this);
        }


        tstring ToLower()
        {
            for(tstring::iterator i=this->begin(), iend = this->end(); i!=iend ; i++)
            {
                *i = _ttolower(*i);
            }
            return (*this);
        }

        tstring ToLower(size_t Begin)
        {

            for(tstring::iterator i=this->begin()+Begin, iend = this->end(); i!=iend ; i++)
            {
                *i = _ttolower(*i);
            }
            return (*this);
        }

        tstring ToLower(size_t Begin, size_t End)
        {
            for(tstring::iterator i=this->begin()+Begin, iend = this->begin()+End; i!=iend ; i++)
            {
                *i = _ttolower(*i);
            }
            return (*this);
        }

        tstring Replace(TCHAR* ToBeReplaced, TCHAR* ReplacedBy)
        {
            tstring StrOut = _T("");
            TCHAR* OriginalPtr = _tcsdup(this->c_str());
            TCHAR* Ptr = OriginalPtr;
            TCHAR* NextPos;
            SIZE_T ToBeReplacedSize = _tcslen(ToBeReplaced);

            for (;;)
            {
                NextPos = _tcsstr(Ptr,ToBeReplaced);
                if (NextPos)
                {
                    // end previous occurrence
                    *NextPos = 0;

                    // add previous occurrence content to output string
                    StrOut+=Ptr;

                    // add replacement string to output string
                    StrOut+=ReplacedBy;

                    // increase input pointer to make it point after occurrence found
                    Ptr=NextPos+ToBeReplacedSize;
                }
                else
                    break;
            }
            // add least data to output string
            StrOut+=Ptr;

            *this = StrOut;
            free(OriginalPtr);
            return (*this);
        }
    };
}

// usage : like string and wstring
// std::tstring Foo(_T("foo"));
// Foo = Foo + _T("foo");
// Foo.append(_T("foo"));