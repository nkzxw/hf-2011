#include "var.h"

#include "HelperFunctions.h"

var::var(const var& rhs)
{
	*this = rhs;
}

var::var(const string& rhs) : type(STR), isbuf(false)
{
	size = rhs.length();
	str = rhs;
	if(is_bytestring(str))
	{
		str = toupper(str);
		size = (size/2)-1; // num of bytes
		isbuf = true;
	}
}

var::var(const char* rhs)
{
	*this = var(string(rhs));
}

var& var::operator+=(const var& rhs)
{
	switch(rhs.type)
	{
	case DW:  *this += rhs.dw;  break;
	case STR: *this += rhs.str; break;
	case FLT: *this += rhs.flt; break;
	}
	return *this;
}

var& var::operator+=(const string& rhs)
{
	var v = rhs;

	if(type == STR)
	{
		if(!isbuf) // str + buf/str -> str
		{
			*this = str + v.to_string();
		}
		else // buf + buf/str -> buf
		{
			*this = '#' + to_bytes() + v.to_bytes() + '#';
		}
	}
	else if(type == DW)
	{
		if(v.isbuf) // rulong + buf -> buf
		{
			*this = '#' + rul2hexstr(::reverse(dw), sizeof(rulong)*2) + v.to_bytes() + '#';
		}
		else // rulong + str -> str
		{
			*this = toupper(rul2hexstr(dw)) + v.str;
		}
	}
	return *this;
}

var& var::operator+=(const rulong& rhs)
{
	switch(type)
	{
	case DW:  dw  += rhs; break;
	case FLT: flt += rhs; break;
	case STR:
		if(isbuf) // buf + rulong -> buf
		{
			*this = '#' + to_bytes() + rul2hexstr(::reverse(rhs), sizeof(rulong)*2) + '#';
		}
		else // str + rulong -> str
		{
			*this = str + toupper(rul2hexstr(rhs));
		}
		break;
	}
	return *this;
}

var& var::operator+=(const double& rhs)
{
	if(type == FLT)
		flt += rhs;
	return *this;
}

/*
var::operator rulong()
{
	if(type == DW)
		return dw;
	else return 0;
}

var::operator string()
{
	if(type == STR)
		return str;
	else return string();
}

var::operator double()
{
	if(type == FLT)
		return flt;
	else return 0.0;
}
*/

int var::compare(const var& rhs) const
{
	// less than zero this < rhs
	// zero this == rhs 
	// greater than zero this > rhs 

	if(type != rhs.type || type == EMP)
		return -2;

	switch(type)
	{
	case DW:
		if(dw < rhs.dw) return -1;
		if(dw == rhs.dw) return 0;
		if(dw > rhs.dw) return 1;
		break;

	case FLT:
		if(flt < rhs.flt) return -1;
		if(flt == rhs.flt) return 0;
		if(flt > rhs.flt) return 1;
		break;

	case STR:
		if(isbuf == rhs.isbuf)
			return str.compare(rhs.str);
		else
			return to_bytes().compare(rhs.to_bytes());
		break;
	}
	return -2;
}

string var::to_bytes() const
{
	if(type != STR)
		return "";

	if(isbuf) // #001122# to "001122"
		return str.substr(1, str.length()-2);
	else      // "001122" to "303031313232"
		return toupper(bytes2hexstr((const byte*)str.data(), size));
}

string var::to_string() const
{
	if(type != STR)
		return "";

	if(isbuf) // #303132# to "012"
	{
		char* bytes = new char[size];
		hexstr2bytes(to_bytes(), (byte*)bytes, size);
		string tmp(bytes, size);
		delete[] bytes;
		return tmp;
	}
	else return str;
}

void var::resize(size_t newsize)
{
	switch(type)
	{
	case DW:
		dw = ::resize(dw, newsize);
		size = newsize;
		break;
	case STR:
		if(newsize < size)
		{
			if(isbuf)
				str = '#' + to_bytes().substr(0, newsize*2) + '#';
			else
				str.resize(newsize);
			size = newsize;
		}
		break;
	}
}

void var::reverse()
{
	switch(type)
	{
	case DW:
		dw = ::reverse(dw);
		break;
	case STR:
		if(isbuf)
		{
			for(size_t i = 0; i < size/2; i++)
			{
				size_t offs = (i*2)+1;

				char& c1 = str[offs], & c2 = str[offs+1];
				char& c3 = str[str.size()-offs-1-1], & c4 = str[str.size()-offs-1];

				char tmp1 = c1;
				char tmp2 = c2;
				c1 = c3;
				c2 = c4;
				c3 = tmp1;
				c4 = tmp2;
			}
		}
		else
			str = ::reverse(str);
		break;
	}
}
