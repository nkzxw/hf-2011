#pragma once

#include <string>
#include "types.h"

using std::string;

class var
{
public:

	enum etype { EMP, DW, STR, FLT };

	rulong dw;
	string str;
	double flt;

	etype type;
	size_t size;
	bool isbuf;

	var() : type(EMP) {}
	var(const var&    rhs);
	var(const string& rhs); 
	var(const char* rhs);
	var(const rulong& rhs) : type(DW),  dw(rhs),  size(sizeof(rhs)) {}
	var(const int&    rhs) : type(DW),  dw(rhs),  size(sizeof(rhs)) {} // needed for var = 0
	var(const size_t& rhs) : type(DW),  dw(rhs),  size(sizeof(rhs)) {}
#ifdef _WIN64
	var(const ulong&  rhs) : type(DW),  dw(rhs),  size(sizeof(rhs)) {}
#endif
	var(const double& rhs) : type(FLT), flt(rhs), size(sizeof(rhs)) {}

	int compare(const var& rhs) const; 

	string to_bytes() const;
	string to_string() const;

	var operator+(const var& rhs) { return var(*this).operator+=(rhs); }

	var& operator+=(const var& rhs);
	var& operator+=(const string& rhs);
	var& operator+=(const rulong& rhs);
	var& operator+=(const double& rhs);
	//var& operator+=(const int& rhs) { return operator+=((const rulong))rhs; }

	//operator rulong();
	//operator string();
	//operator double();

	void resize(size_t newsize);
	void reverse();
};
