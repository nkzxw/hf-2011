#include "Opcodes.h"

#include "TE_Interface.h"
#include "Debug.h"
#include "HelperFunctions.h"
#ifndef _WIN64
#include "disasm.h"
#pragma comment(lib, "disasm.lib")
#else
#include "SDK.hpp"

using namespace TE;
#endif

size_t Assemble(const string& cmd, rulong ip, byte* op)
{
size_t size = 0;

#ifndef _WIN64
	t_asmmodel info;
	char error[TEXTLEN];

	if(0 < Assemble((char*)cmd.c_str(), ip, &info, 0, 3, error))
	{
		size = info.length;
		memcpy(op, info.code, info.length);
	}
#endif
	return size;
}

size_t AssembleEx(const string& cmd, rulong ip)
{
byte op[MAX_INSTR_SIZE];
size_t size;

	if((size = Assemble(cmd, ip, op)) && TE_WriteMemory(ip, size, op))
	{
		return size;
	}
	return 0;
}

string Disassemble(const byte* op, rulong ip, size_t* size)
{
size_t cmdsize = 0;
string cmd = "";

#ifndef _WIN64
	t_disasm info;
	cmdsize = Disasm((char*)op, MAXCMDSIZE, ip, &info, DISASM_FILE);
	cmd = info.result;
#else
	const char* instr = Debugger::StaticDisassembleEx(ip, op);
	if(instr)
	{
		cmd = instr;
		if(size)
			cmdsize = Debugger::StaticLengthDisassemble(op);
	}
#endif
	if(size) *size = cmdsize;
	return cmd;
}

string DisassembleEx(rulong ip, size_t* size)
{
byte op[MAX_INSTR_SIZE];

	if(TE_ReadMemory(ip, sizeof(op), &op))
	{
		return Disassemble(op, ip, size);
	}
	else
	{
		if(size) *size = 0;
		return "";
	}
}

size_t LengthDisassemble(const byte* op)
{
#ifndef _WIN64
	t_disasm info;
	return Disasm((char*)op, MAXCMDSIZE, 0, &info, DISASM_SIZE);
#else
	return Debugger::StaticLengthDisassemble(op);
#endif
}

size_t LengthDisassemble(const string& cmd)
{
byte op[MAX_INSTR_SIZE];

	return Assemble(cmd, 0, op);
}

size_t LengthDisassembleEx(rulong ip)
{
	byte op[MAX_INSTR_SIZE];
	if(TE_ReadMemory(ip, sizeof(op), &op))
	{
		return LengthDisassemble(op);
	}
	return 0;
}

size_t LengthDisassembleBack(const byte* op)
{
byte tmp[2*MAX_INSTR_SIZE];
size_t offs = 0;

	memcpy(tmp, op, MAX_INSTR_SIZE);

	while(offs < MAX_INSTR_SIZE)
	{
		size_t cmdsize = LengthDisassemble(tmp + offs);
		offs += cmdsize;
		if(offs == MAX_INSTR_SIZE)
		{
			return cmdsize;
		}
	}
	return 0;
}

size_t LengthDisassembleBackEx(rulong ip)
{
byte op[MAX_INSTR_SIZE];

	if(TE_ReadMemory(ip - sizeof(op), sizeof(op), &op))
	{
		return LengthDisassembleBack(op);
	}
	return 0;
}
