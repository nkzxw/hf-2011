#pragma once

#include <string>
#include "types.h"

using std::string;

#ifndef _WIN64
const size_t MAX_INSTR_SIZE = 16; /*MAXCMDSIZE*/ // :>
#else
const size_t MAX_INSTR_SIZE = 32; // ?
#endif

size_t Assemble(const string& cmd, rulong ip, byte* op);
size_t AssembleEx(const string& cmd, rulong ip);
string Disassemble(const byte* op, rulong ip, size_t* size = NULL);
string DisassembleEx(rulong ip, size_t* size = NULL);
size_t LengthDisassemble(const byte* op);
size_t LengthDisassemble(const string& cmd);
size_t LengthDisassembleEx(rulong ip);
size_t LengthDisassembleBack(const byte* op);
size_t LengthDisassembleBackEx(rulong ip);
