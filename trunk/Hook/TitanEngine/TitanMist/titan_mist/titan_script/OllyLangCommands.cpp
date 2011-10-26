#include "OllyLang.h"

#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include "Debug.h"
#include "HelperFunctions.h"
#include "Dialogs.h"
#include "SDK.hpp"
#include "TE_Interface.h"
#include "Opcodes.h"

using namespace TE;
using std::search;
using std::min;
using std::max;

bool OllyLang::DoADD(const string* args, size_t count)
{
rulong dw1, dw2;
double flt1, flt2;
string str1, str2;

	if(count == 2)
	{
		if(GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
		{
			return SetRulong(args[0], dw1+dw2);
		}
		else if(GetFloat(args[0], flt1) && GetFloat(args[1], flt2))
		{
			return SetFloat(args[0], flt1+flt2);
		}
		else if(GetFloat(args[0], flt1) && GetRulong(args[1], dw2))
		{
			return SetFloat(args[0], flt1+dw2);
		}
		else if(GetString(args[0], str1) && GetRulong(args[1], dw2))
		{
			var v1 = str1, v2 = dw2;
			var v3 = v1 + v2;
			return SetString(args[0], v3.str);
		}
		else if((GetString(args[0], str1) && GetAnyValue(args[1], str2)) || (GetAnyValue(args[0], str1) && GetAnyValue(args[1], str2)))
		{
			var v1 = str1, v2 = str2;
			var v3 = v1 + v2;
			return SetString(args[0], v3.str);
		}
	}
	return false;
}

bool OllyLang::DoAI(const string* args, size_t count)
{
	if(count == 0)
	{
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoALLOC(const string* args, size_t count)
{
rulong size;

	if(count == 1 && GetRulong(args[0], size))
	{
		rulong addr;
		variables["$RESULT"] = addr = TE_AllocMemory(size);
		if(addr)
			regBlockToFree(addr, size, false);
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoAN(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		return true;
	}
	return false;
}

bool OllyLang::DoAND(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], dw1 & dw2);
	}
	return false;
}

bool OllyLang::DoAO(const string* args, size_t count)
{
	if(count == 0)
	{
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoASK(const string* args, size_t count)
{
string title;
string returned;

	if(count == 1 && GetString(args[0], title))
	{
		variables["$RESULT"] = variables["$RESULT_1"] = 0;

		if(DialogASK(title, returned))
		{
			if(is_hex(returned)) 
			{
				variables["$RESULT"] = hexstr2rul(returned);
				variables["$RESULT_1"] = ((returned.length()+1)/2); // size in bytes rounded to 2
			}
			else
			{
				returned = UnquoteString(returned, '"'); // To Accept input like "FFF" (forces string)
				variables["$RESULT"] = returned;
				variables["$RESULT_1"] = returned.length();
			}
		}
		else Pause();
		return true;
	}
	return false;
}

bool OllyLang::DoASM(const string* args, size_t count)
{
string cmd;
rulong addr, attempt;

	if(count >= 2 && count <= 3 && GetRulong(args[0], addr) && GetString(args[1], cmd))
	{
		if(count == 3 && !GetRulong(args[2], attempt))
			return false;

		size_t len = AssembleEx(FormatAsmDwords(cmd), addr);
		if(!len)
		{
			errorstr = "Invalid command: " + cmd;
			return false;
		}
		variables["$RESULT"] = (rulong)len;
		return true;
	}
	return false;
}

bool OllyLang::DoASMTXT(const string* args, size_t count)
{
rulong addr;
string asmfile;

	if(count == 2 && GetRulong(args[0], addr) && GetString(args[1], asmfile))
	{
		bool Success = false;
		size_t totallen = 0;

		vector<string> lines = getlines_file(ascii2unicode(asmfile).c_str());

		for(size_t i = 0; i < lines.size(); i++)
		{
			string line = lines[i];
			if(line.size())
			{
				size_t len = AssembleEx(FormatAsmDwords(line), addr + totallen);
				if(!len)
				{
					errorstr = "Invalid command: " + line;
					return false;
				}
				totallen += len;
			}
		}

		variables["$RESULT"] = totallen;
		variables["$RESULT_1"] = lines.size();
		return true;
	}
	return false;
}

bool OllyLang::DoATOI(const string* args, size_t count)
{
string str;
rulong base = 16;

	if(count >= 1 && count <= 2 && GetString(args[0], str))
	{
		if(count == 2 && !GetRulong(args[1], base))
			return false;

		variables["$RESULT"] = str2rul(str, base);
		return true;
	}
	return false;
}

bool OllyLang::DoBC(const string* args, size_t count)
{
rulong addr;

	if(count >= 0 && count <= 1)
	{
		if(count == 0)
		{
			return DoBCA(NULL, 0);
		}
		else if(GetRulong(args[0], addr))
		{
			Debugger::DeleteBPX(addr);
			return true;
		}
	}
	return false;
}

bool OllyLang::DoBCA(const string* args, size_t count)
{
	if(count == 0)
	{
		Debugger::RemoveAllBreakPoints(UE_OPTION_REMOVEALLDISABLED);
		Debugger::RemoveAllBreakPoints(UE_OPTION_REMOVEALLENABLED);
		return true;
	}
	return false;
}

bool OllyLang::DoBD(const string* args, size_t count)
{
rulong addr;

	if(count >= 0 && count <= 1)
	{
		if(count == 0)
		{
			return DoBDA(NULL, 0);
		}
		else if(GetRulong(args[0], addr))
		{
			Debugger::DisableBPX(addr);
			return true;
		}
	}
	return false;
}

bool OllyLang::DoBDA(const string* args, size_t count)
{
	if(count == 0)
	{
		Debugger::RemoveAllBreakPoints(UE_OPTION_DISABLEALL); // int3 only?
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoBEGINSEARCH(const string* args, size_t count)
{
rulong start = 0;

	if(count >= 0 && count <= 1)
	{
		if(count == 1 && !GetRulong(args[0], start))
			return false;

		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoENDSEARCH(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

bool OllyLang::DoBP(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		Debugger::SetBPX(addr, UE_BREAKPOINT, &SoftwareCallback);
		return true;
	}
	return false;
}

// TE?
bool OllyLang::DoBPCND(const string* args, size_t count)
{
rulong addr = 0;
string condition;

	if(count == 2 && GetRulong(args[0], addr) && GetString(args[1], condition))
	{
		errorstr = "Unsupported command!";
		return false;

		/*
		Setbreakpoint(addr, TY_ACTIVE, 0);
		Insertname(addr, NM_BREAK, (char *)condition.c_str());
		Deletenamerange(addr, addr + 1, NM_BREAKEXPL);
		Deletenamerange(addr, addr + 1, NM_BREAKEXPR);
		return true;
		*/
	}
	return false;
}

bool OllyLang::DoBPD(const string* args, size_t count)
{
	if(count == 1)
	{
		return callCommand(&OllyLang::DoBPX, 2, args[0].c_str(), "1");
	}
	return false;
}

bool OllyLang::DoBPGOTO(const string* args, size_t count)
{
rulong addr;

	if(count == 2 && GetRulong(args[0], addr) && script.is_label(args[1]))
	{
		bpjumps[addr] = script.labels[args[1]];
		return true;
	}
	return false;
}

bool OllyLang::DoBPHWCA(const string* args, size_t count)
{
	if(count == 0)
	{
		for(int i = 0; i < 4; i++)
		{
			Debugger::DeleteHardwareBreakPoint(i);
		}
		return true;
	}
	return false;
}

bool OllyLang::DoBPHWC(const string* args, size_t count)
{
rulong addr;

	if(count >= 0 && count <= 1)
	{
		if(count == 0)
		{
			return DoBPHWCA(NULL, 0);
		}
		else if(GetRulong(args[0], addr))
		{
			rulong DRX[4];

			DRX[0] = Debugger::GetContextData(UE_DR0);
			DRX[1] = Debugger::GetContextData(UE_DR1);
			DRX[2] = Debugger::GetContextData(UE_DR2);
			DRX[3] = Debugger::GetContextData(UE_DR3);
			for(int i = 0; i < _countof(DRX); i++)
			{
				if(DRX[i] == addr)
				{
					Debugger::DeleteHardwareBreakPoint(i);
				}
			}
			return true;
		}
	}
	return false;
}

bool OllyLang::DoBPHWS(const string* args, size_t count)
{
rulong addr;
string typestr = "x";

	if(count >= 1 && count <= 2 && GetRulong(args[0], addr))
	{
		if(count == 2 && (!GetString(args[1], typestr) || typestr.length() != 1))
			return false;

		eHWBPType type;

		switch(typestr[0])
		{
		case 'r': type = UE_HARDWARE_READWRITE; break;
		case 'w': type = UE_HARDWARE_WRITE;     break;
		case 'x': type = UE_HARDWARE_EXECUTE;   break;
		default: return false;
		}

		Debugger::SetHardwareBreakPoint(addr, NULL, type, UE_HARDWARE_SIZE_1, &HardwareCallback);
		return true;
	}
	return false;
}

// TE?
bool OllyLang::DoBPL(const string* args, size_t count)
{
rulong addr;
string expression;

	if(count == 2 && GetRulong(args[0], addr) && GetString(args[1], expression))
	{
		errorstr = "Unsupported command!";
		return false;

		/*
		expression = 'E' + expression; // 0x45//COND_NOBREAK | COND_LOGALWAYS | COND_ARGALWAYS | COND_FILLING
		
		Setbreakpoint(addr, TY_ACTIVE, 0);
		Deletenamerange(addr, addr + 1, NM_BREAK);
		Deletenamerange(addr, addr + 1, NM_BREAKEXPL);
		Insertname(addr, NM_BREAKEXPR, expression.c_str());
		return true;
		*/
	}
	return false;
}

// TE?
bool OllyLang::DoBPLCND(const string* args, size_t count)
{
rulong addr;
string expression, condition;

	if(count == 3 && GetRulong(args[0], addr) && GetString(args[1], expression) && GetString(args[2], condition))
	{
		errorstr = "Unsupported command!";
		return false;

		/*
		Setbreakpoint(addr, TY_ACTIVE, 0);
		Deletenamerange(addr, addr + 1, NM_BREAKEXPL);
		Insertname(addr, NM_BREAK, condition.c_str());
		expression = 'C' + expression; // 0x43
		Insertname(addr, NM_BREAKEXPR, expression.c_str());
		return true;
		*/
	}
	return false;
}

bool OllyLang::DoBPMC(const string* args, size_t count)
{
	if(count == 0)
	{
		if(membpaddr && membpsize) 
		{
			Debugger::RemoveMemoryBPX(membpaddr, membpsize);
			membpaddr = membpsize = 0;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoBPRM(const string* args, size_t count)
{
rulong addr, size;

	if(count == 2 && GetRulong(args[0], addr) && GetRulong(args[1], size))
	{
		if(membpaddr && membpsize)
			DoBPMC(NULL, 0);

		if(Debugger::SetMemoryBPXEx(addr, size, UE_MEMORY_READ, true, &MemoryCallback))
		{
			membpaddr = addr;
			membpsize = size;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoBPWM(const string* args, size_t count)
{
rulong addr, size;

	if(count == 2 && GetRulong(args[0], addr) && GetRulong(args[1], size))
	{
		if(membpaddr && membpsize)
			DoBPMC(NULL, 0);

		if(Debugger::SetMemoryBPXEx(addr, size, UE_MEMORY_WRITE, true, &MemoryCallback))
		{
			membpaddr = addr;
			membpsize = size;
		}
		return true;
	}
	return false;
}

// TE?
bool OllyLang::DoBPX(const string* args, size_t count)
{
string callname;
rulong del = 0;
 
	if(count >= 1 && count <= 2 && GetString(args[0], callname))
	{
		if(count == 2 && !GetRulong(args[1], del))
			return false;

		errorstr = "Unsupported command!";
		return false;

		/*
		int bpnmb = 0;
		//size_t count;
		//int i;

		//t_table *reftable;
		//t_ref *pref;

		char findname[256] = {0};

		if(callname == "") 
		{
			errorstr = "Function name missed";
			return false;
		}

		
		char name[256];
		strcpy(name, callname.c_str());

		Findalldllcalls((t_dump *)Plugingetvalue(VAL_CPUDASM),0,"Intermodular calls");
		reftable=(t_table *)Plugingetvalue(VAL_REFERENCES);

		if(reftable==NULL || reftable->data.n==0)
		{
			errorstr = "No references";
			return false;
		}

		 if(reftable->data.itemsize<sizeof(t_ref))
		{
			errorstr = "Old version of OllyDbg";
			return false;
		}

		for(i = 0; i < reftable->data.n; i++) 
		{
			// The safest way: size of t_ref may change in the future!
			pref=(t_ref *)((char *)reftable->data.data+reftable->data.itemsize*i);

			if(Findlabel(pref->dest, findname) == 0) 
			{// Unnamed destination
				continue;
			}
   
			if(!_stricmp(name, findname)) 
			{
				if(!del) 
				{   // Set breakpoint
					SetBPX(pref->addr, UE_BREAKPOINT, &SoftwareCallback);
					bpnmb++;
				}
				else 
				{
					DeleteBPX(pref->addr);
					bpnmb++;
				}
			}
		}
        variables["$RESULT"] = bpnmb;
        return true;
		*/
	}
	return false;
}

bool OllyLang::DoBUF(const string* args, size_t count)
{
	if(count == 1 && is_variable(args[0]))
	{
		var& v = variables[args[0]];

		switch(v.type)
		{
		case var::STR: // empty buf + str -> buf
			if(!v.isbuf)
				v = var("##") + v.str;
			break;
		case var::DW: // empty buf + dw -> buf
			v = var("##") + v.dw;
			break;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoCALL(const string* args, size_t count)
{
	if(count == 1 && script.is_label(args[0]))
	{
		calls.push_back(script_pos+1);
		return DoJMP(args, count);
	}
	return false;
}

bool OllyLang::DoCLOSE(const string* args, size_t count)
{
rulong hwnd;

	if(count == 1)
	{
		const char valid_commands[][12] = {"SCRIPT", "SCRIPTLOG", "MODULES", "MEMORY", "THREADS", "BREAKPOINTS", "REFERENCES", "SOURCELIST", "WATCHES", "PATCHES", "CPU", "RUNTRACE", "WINDOWS", "CALLSTACK", "LOG", "TEXT", "FILE", "HANDLES", "SEH", "SOURCE"};

		if(valid_commands+_countof(valid_commands) != find(valid_commands, valid_commands+_countof(valid_commands), toupper(args[0])))
		{
			return true;
		}
		else if(GetRulong(args[0], hwnd) && hwnd)
		{
			DestroyWindow((HWND)hwnd);
			return true;
		}
		errorstr = "Bad operand";
	}
	return false;
}

bool OllyLang::DoCMP(const string* args, size_t count)
{
rulong dw1, dw2;
string s1, s2;
double flt1, flt2;
rulong size = 0;

	if(count >= 2 && count <= 3)
	{
		if(count == 3 && !GetRulong(args[2], size))
			return false;

		var v1, v2;

		if(GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
		{
			v1 = dw1;
			v2 = dw2;
		}
		else if(GetFloat(args[0], flt1) && GetFloat(args[1], flt2))
		{
			v1 = flt1;
			v2 = flt2;
		}
		else if(GetAnyValue(args[0], s1, true) && GetAnyValue(args[1], s2, true))
		{ // see also SCMP command, code is not finished here...
			v1 = s1;
			v2 = s2;
		}
		else return false;

		if(size > 0)
		{
			v1.resize(size);
			v2.resize(size);
		}

		int res = v1.compare(v2); //Error if -2 (type mismatch)
		if(res != -2)
		{
			SetCMPFlags(res);
			return true;
		}
	}
	return false;
}

// Olly only
bool OllyLang::DoCMT(const string* args, size_t count)
{
rulong addr;
string cmt;

	if(count == 2 && GetRulong(args[0], addr) && GetString(args[1], cmt))
	{
		return true;
	}
	return false;
}

bool OllyLang::DoCOB(const string* args, size_t count)
{
	if(count == 0)
	{
		EOB_row = -1;
		return true;
	}
	return false;
}

bool OllyLang::DoCOE(const string* args, size_t count)
{
	if(count == 0)
	{
		EOE_row = -1;
		return true;
	}
	return false;
}

bool OllyLang::DoDBH(const string* args, size_t count)
{
	if(count == 0)
	{
		Hider::HideDebugger(TE_GetProcessHandle(), UE_HIDE_PEBONLY);
		return true;
	}
	return false;
}

bool OllyLang::DoDBS(const string* args, size_t count)
{
	if(count == 0)
	{
		Hider::UnHideDebugger(TE_GetProcessHandle(), UE_HIDE_PEBONLY);
		return true;
	}
	return false;
}

bool OllyLang::DoDEC(const string* args, size_t count)
{
rulong dw;
double flt;

	if(count == 1)
	{
		if(GetRulong(args[0], dw))
		{
			dw--;
			return SetRulong(args[0], dw);
		}
		else if(GetFloat(args[0], flt))
		{
			flt--;
			return SetFloat(args[0], flt);
		}
	}
	return false;
}

bool OllyLang::DoDIV(const string* args, size_t count)
{
rulong dw1, dw2;
double flt1, flt2;

	if(count == 2)
	{
		if(GetRulong(args[0], dw1) && GetRulong(args[1], dw2) && dw2 != 0)
		{
			return SetRulong(args[0], dw1 / dw2);
		}
		else if(GetFloat(args[0], flt1) && GetFloat(args[1], flt2) && flt2 != 0)
		{
			return SetFloat(args[0], flt1 / flt2);
		}
		else if(GetFloat(args[0], flt1) && GetRulong(args[1], dw2) && dw2 != 0)
		{
			return SetFloat(args[0], flt1 / dw2);
		}
		else if(GetRulong(args[0], dw1) && GetFloat(args[1], flt2) && flt2 != 0)
		{
			return SetFloat(args[0], dw1 / flt2);
		}
		else return false;

		errorstr = "Division by 0";
	}
	return false;
}

bool OllyLang::DoDM(const string* args, size_t count)
{
rulong addr, size;
string filename;

	if(count == 3 && GetRulong(args[0], addr) && GetRulong(args[1], size) && GetString(args[2], filename))
	{
        if(!isfullpath(filename))
			filename = TE_GetTargetDirectory() + filename;	

		// Truncate existing file
		HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			SetEndOfFile(hFile);
			CloseHandle(hFile);
		}

		return DoDMA(args, count);
	}
	return false;
}

bool OllyLang::DoDMA(const string* args, size_t count)
{
rulong addr, size;
string filename;

	if(count == 3 && GetRulong(args[0], addr) && GetRulong(args[1], size) && GetString(args[2], filename))
	{
		bool success = true;

        if(!isfullpath(filename))
			filename = TE_GetTargetDirectory() + filename;

		variables["$RESULT"] = size;

		HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			byte membuf[PAGE_SIZE];
			rulong sum = 0;
			DWORD bytes;

			SetFilePointer(hFile, 0, NULL, FILE_END);

			while(size >= sizeof(membuf) && success)
			{
				success = false;
				if(!TE_ReadMemory(addr, sizeof(membuf), membuf))
				{
					memset(membuf, 0, sizeof(membuf));
				}

				success = (WriteFile(hFile, membuf, sizeof(membuf), &bytes, NULL) && (bytes == sizeof(membuf)));

				addr += sizeof(membuf);
				size -= sizeof(membuf);
			}

			if(success && size)
			{
				success = false;
				if(!TE_ReadMemory(addr, size, membuf))
				{
					memset(membuf, 0, size);
				}
				success = (WriteFile(hFile, membuf, size, &bytes, NULL) && (bytes == size));
			}

			CloseHandle(hFile);
			return success;
		}
		else errorstr = "Couldn't create file";
	}
	return false;
}

bool OllyLang::DoDPE(const string* args, size_t count)
{
string filename;
rulong ep;

	if(count == 2 && GetString(args[0], filename) && GetRulong(args[1], ep))
	{
        if(!isfullpath(filename))
			filename = TE_GetTargetDirectory() + filename;	

		return Dumper::DumpProcess(TE_GetProcessHandle(), (void*)Debugger::GetDebuggedFileBaseAddress(), filename.c_str(), ep);
	}
	return false;
}

bool OllyLang::DoENDE(const string* args, size_t count)
{
	if(count == 0)
	{
		if(pmemforexec)
			TE_FreeMemory(pmemforexec);
		return true;
	}
	return false;
}

bool OllyLang::DoERUN(const string* args, size_t count)
{
	if(count == 0)
	{
		ignore_exceptions = true;
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoESTI(const string* args, size_t count)
{
	if(count == 0)
	{
		ignore_exceptions = true;
		stepcount = 1;
		StepIntoCallback();
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoESTEP(const string* args, size_t count)
{
	if(count == 0)
	{
		ignore_exceptions = true;
		stepcount = 1;
		StepOverCallback();
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoEOB(const string* args, size_t count)
{
	if(count >= 0 && count <= 1)
	{
		if(count == 0) // Go interactive
		{
			EOB_row = -1;
			return true;
		}
		else if(script.is_label(args[0])) // Set label to go to
		{
			EOB_row = script.labels[args[0]];
			return true;
		}
	}
	return false;
}

bool OllyLang::DoEOE(const string* args, size_t count)
{
	if(count >= 0 && count <= 1)
	{
		if(count == 0) // Go interactive
		{
			EOE_row = -1;
			return true;
		}
		else if(script.is_label(args[0])) // Set label to go to
		{
			EOE_row = script.labels[args[0]];
			return true;
		}
	}
	return false;
}

bool OllyLang::DoEVAL(const string* args, size_t count)
{
string to_eval;

	if(count == 1 && GetString(args[0], to_eval))
	{
		variables["$RESULT"] = ResolveVarsForExec(to_eval, false);
		return true;
	}
	return false;
}

bool OllyLang::DoEXEC(const string* args, size_t count)
{
	if(count == 0)
	{
		uint first = script_pos + 1;
		uint ende = script.next_command(first);

		if(ende > script.lines.size())
		{
			errorstr = "EXEC needs ENDE command!";
			return false;
		}

		// max size for all commands + jmp eip
		size_t memsize = (ende-first+1) * MAX_INSTR_SIZE;

		pmemforexec = TE_AllocMemory(memsize);
		if(pmemforexec)
		{
			size_t len, totallen = 0;

			for(uint i = first; i < ende; i++)
			{
				string line = ResolveVarsForExec(script.lines[i].line, true);
				if(!(len = AssembleEx(line, pmemforexec + totallen)))
				{
					TE_FreeMemory(pmemforexec);
					errorstr = "Invalid command: " + line;
					return false;
				}
				totallen += len;
			}

			//return at ENDE
			script_pos_next = ende;

			rulong eip = Debugger::GetContextData(UE_CIP);

			// Add jump to original EIP
			string jmpstr = "jmp " + rul2hexstr(eip);
			rulong jmpaddr = pmemforexec + totallen;

			len = AssembleEx(jmpstr, pmemforexec + totallen);
			totallen += len;

			// Set new eip and run to the original one
			Debugger::SetContextData(UE_CIP, pmemforexec);

			// ignore next breakpoint
			bInternalBP = true;

			debuggee_running = false; // :s

			//Debugger::SetBPX(jmpaddr, UE_BREAKPOINT, &EXECJMPCallback);
			Debugger::SetBPX(eip, UE_BREAKPOINT, &SoftwareCallback);

			t_dbgmemblock block = { 0 };

			block.address = pmemforexec;
			block.size = memsize;
			block.script_pos = script_pos_next;
			block.free_at_ip = eip;
			block.autoclean = true;

			regBlockToFree(block);

			require_addonaction = true;
			back_to_debugloop = true;

			return true;
		}
	}
	return false;
}

bool OllyLang::DoFILL(const string* args, size_t count)
{
rulong addr, len;
byte val;

	if(count == 3 && GetRulong(args[0], addr) && GetRulong(args[1], len) && GetNum(args[2], val))
	{
		byte membuf[PAGE_SIZE];
		memset(membuf, val, min((size_t)len, sizeof(membuf)));

		while(len >= sizeof(membuf))
		{
			if(!TE_WriteMemory(addr, sizeof(membuf), membuf))
				return false;

			addr += sizeof(membuf);
			len -= sizeof(membuf);
		}

		if(len && !TE_WriteMemory(addr, len, membuf))
				return false;

		return true;
	}
	return false;
}

bool OllyLang::DoFIND(const string* args, size_t count)
{
rulong addr;
string finddata;
rulong maxsize = 0;

	if(count >= 2 && count <= 3 && GetRulong(args[0], addr))
	{
		if(count == 3 && !GetRulong(args[2], maxsize))
			return false;

		rulong dw;
		if(GetRulong(args[1], dw))
		{
			finddata = rul2hexstr(reverse(dw));
			// Remove trailing zeroes, keep even character count
			size_t end;
			for(end = finddata.length()-1; end; end--)
			{
				if(finddata[end] != '0')
					break;
			}
			end++;
			finddata.resize(end + (end % 2), '0');
		}
		else if(GetString(args[1], finddata))
		{
			var v = finddata;
			finddata = v.to_bytes();
			if(!v.isbuf)
				ReplaceString(finddata, "3f", "??"); // 0x3F = '?' -> wildcard like "mov ?ax, ?bx"
		}
		else return false;

		if(is_hexwild(finddata))
		{
			variables["$RESULT"] = 0;

			// search in current mem block
			MEMORY_BASIC_INFORMATION MemInfo;
			if(TE_GetMemoryInfo(addr, &MemInfo))
			{
				rulong memlen = (rulong)MemInfo.BaseAddress + MemInfo.RegionSize - addr;
				if(maxsize && maxsize < memlen)
						memlen = maxsize;

				byte* membuf = 0, * mask = 0, * bytes;

				try
				{
					membuf = new byte[memlen];
					if(TE_ReadMemory(addr, memlen, membuf))
					{
						size_t bytecount = finddata.length()/2;

						mask  = new byte[bytecount];
						bytes = new byte[bytecount];

						hexstr2bytemask(finddata, mask, bytecount);
						hexstr2bytes(finddata, bytes, bytecount);

						for(size_t i = 0; (i+bytecount) <= memlen; i++)
						{
							if(memcmp_mask(membuf+i, bytes, mask, bytecount))
							{
								variables["$RESULT"] = addr + i;
								break;
							}
						}

						delete[] mask;
						delete[] bytes;
					}
					delete[] membuf;
				}
				catch(std::bad_alloc)
				{
					delete[] membuf;
					delete[] mask;
					errorstr = "Out of memory!";
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

// TE?
bool OllyLang::DoFINDCALLS(const string* args, size_t count)
{
bool bRefVisible = false, bResetDisam = false;
rulong addr, base, size, disamsel = 0;

	if(count >= 1 && count <= 2 && GetRulong(args[0], addr))
	{
		errorstr = "Unsupported command!";
		return false;

		return true;
	}
	return false;

	/*
	Getdisassemblerrange(&base, &size);

	//Get initial Ref Window State
	t_table* tt;
	tt=(t_table*) Plugingetvalue(VAL_REFERENCES);
	if (tt!=NULL)
		bRefVisible=(tt->hw!=0);

	t_dump* td;
	td=(t_dump*) Plugingetvalue(VAL_CPUDASM);
	if (td==NULL)
		return false;

	if(GetRulong(ops[0], addr))
	{
		if(addr < base || addr >= (base+size))
		{
			//outside debugger window range
			disamsel = td->sel0;
			Setdisasm(addr, 0, 0);
			bResetDisam = true;
		}

		variables["$RESULT"] = 0;
		if(Findalldllcalls(td, addr, NULL) > 0)
		{
			
			if(tt==NULL)
				tt=(t_table*) Plugingetvalue(VAL_REFERENCES);

			if(tt!=NULL) 
			{
				t_ref* tr;

				if(tt->data.n > 1) 
				{ 
					//Filter results
					string filter;
					if(GetString(ops[1], filter) && filter != "")
					{
						//filter=ToLower(filter);
						(char*) buffer[TEXTLEN+1];
						for(int nref = tt->data.n-1; nref > 0; nref--)
						{
							tr=(t_ref*) Getsortedbyselection(&tt->data, nref);
							if(tr != NULL && tr->dest != 0)
							{
								//ZeroMemory(buffer,TEXTLEN+1);
								//Decodename(tr->dest,NM_LABEL,buffer);
								Findlabel(tr->dest, buffer);
								if(_stricmp(buffer, filter.c_str()))
									Deletesorteddata(&tt->data, tr->addr);
							}
						}
					}

					tr=(t_ref*) Getsortedbyselection(&tt->data, 1); //0 is CPU initial
					if (tr!=NULL)
						variables["$RESULT"] = tr->addr;
				}

				if(tt->hw && !bRefVisible)
				{
					DestroyWindow(tt->hw);
					tt->hw = 0;
				}
			}
		}
		if(bResetDisam)
			Setdisasm(disamsel, 0, 0);
		return true;
	}
	return false;
	*/
}

// TE?
bool OllyLang::DoFINDCMD(const string* args, size_t count)
{
bool bRefVisible = false, bResetDisam = false;
string cmd, cmds;
int len;
size_t pos;
rulong addr, base,size, attempt, opsize = 3, disamsel = 0;
int startadr = 0, endadr = 0, lps = 0, length, ncmd = 0, cmdpos = 0;
char error[256] = {0};

	if(count == 2)
	{
		errorstr = "Unsupported command!";
		return false;
		//return true;
	}
	return false;

	/*
	Getdisassemblerrange(&base,&size);

	//Get initial Ref Window State
	t_table* tt;
	tt=(t_table*) Plugingetvalue(VAL_REFERENCES);
	if (tt!=NULL)
		bRefVisible=(tt->hw!=0);

	t_dump* td;
	td=(t_dump*) Plugingetvalue(VAL_CPUDASM);
	if (td==NULL)
		return false;

	ulong tmpaddr = TE_AllocMemory(0x100);

	if (GetRulong(ops[0], addr) 
		&& GetString(ops[1], cmds))
	{
		if (addr<base || addr>=(base+size)) {
			//outside debugger window range
			disamsel=td->sel0;
			Setdisasm(addr,0,0);
			bResetDisam=true;
		}

		t_asmmodel model={0};
		t_extmodel models[NSEQ][NMODELS]={0};

		length = cmds.length();
		while (cmdpos<length && ncmd<NSEQ)
		{

			endadr= cmds.find(";",startadr);
			if (endadr==-1)
			{
				endadr=length;
			}
			lps=endadr-startadr;

			cmd=cmds.substr(startadr,lps);
			
			attempt=0;
			strcpy(buffer, cmd.c_str());

			do {

				if((len = Assemble(buffer, tmpaddr, &model, attempt, opsize, error)) <= 0)
				{
					if (attempt!=0) {
						break;
					}

					pos=(cmd.length()+len);
					if (pos>=0 && pos<cmd.length())
						errorstr = "\nFINDCMD error at \""+cmd.substr(pos,cmd.length()-pos)+"\"!\n\n";
					else
						errorstr = "\nFINDCMD error !\n\n";
					errorstr.append(error);
					goto return_false;
				}
				memcpy(&models[ncmd][attempt],&model,sizeof(model));
				attempt++;

			} while (len>0 && attempt<NMODELS);

			startadr=endadr+1;
			cmdpos+=lps+1;

			ncmd++;
		}

		variables["$RESULT"]=0;
		if (Findallsequences(td,models,addr,NULL)>0) {
			
			if (tt==NULL)
				tt=(t_table*) Plugingetvalue(VAL_REFERENCES);

			if (tt!=NULL) 
			{
				t_ref* tr;
				if (tt->data.n > 1)
				{
					tr=(t_ref*) Getsortedbyselection(&tt->data, 1); //0 is CPU initial
					if (tr!=NULL)
						variables["$RESULT"]=tr->addr;
				}

				if (tt->hw && !bRefVisible) {
					DestroyWindow(tt->hw);
					tt->hw=0;
				}
			}
		}
		TE_FreeMemory(tmpaddr);
		if(bResetDisam)
			Setdisasm(disamsel,0,0);
		return true;

	}
return_false:
	if(bResetDisam)
		Setdisasm(disamsel,0,0);
	TE_FreeMemory(tmpaddr);
	return false;
	*/
	return true;
}
/*
//Buggy, could assemble different command code bytes, (from chinese code)
bool OllyLang::DoFINDCMDS(const string* args, size_t count)
{

	string ops[2];
	t_asmmodel model;
	ulong addr;
	string cmds,args1,cmd;
	char opcode[256]={0},buff[32]={0},tmp[64]={0},error[64]={0};
	int i,pos,len=0,length=0,startadr=0,endadr=0,lps=0,codelen=0;
	int attempt=0,opsize=3;

	if(!CreateOp(args, ops, 2))
		return false;

	if (GetRulong(ops[0], addr) 
		&& GetString(ops[1], cmds))
	{

	  if (cmds.find(";")==-1)
	  {
		nIgnoreNextValuesHist=1;
		return DoFINDoneCMD(args);
	  }

	  length = cmds.length();
 
	  ulong tmpaddr = TE_AllocMemory(0x100);

	  while (len<length)
	  {
		endadr= cmds.find(";",startadr);
		if (endadr==-1)
		{
			endadr=length;
		}
		lps=endadr-startadr;
		cmd=cmds.substr(startadr,lps);
       
		strcpy(buffer, cmd.c_str());
		if((codelen = Assemble(buffer, tmpaddr, &model, attempt, opsize, error)) <= 0)
		{
			pos=(cmd.length()+codelen);
			if (pos>=0 && pos<cmd.length())
				errorstr = "\nFINDCMDS error on \""+cmd.substr(pos,cmd.length()-pos)+"\"!\n\n";
			else
				errorstr = "\nFINDCMDS error !\n\n";
			errorstr.append(error);
			TE_FreeMemory(tmpaddr);
			return false;
		}
		else
		{
			strcpy(buff, model.code);
		}

		i=0;
		while(i<codelen)
		{
			_itoa(buff[i],tmp,16);
			i++;
			strcat(opcode,tmp);
		}

		startadr=endadr+1;
		len=len+lps+1;
	  }
	  TE_FreeMemory(tmpaddr);

	  return DoFIND(ops[0] + ", " + '#' + opcode + '#');
	}
	return false;
}
*/

// TE?
bool OllyLang::DoFINDOP(const string* args, size_t count)
{
rulong addr;
string finddata;
rulong maxsize = 0;

	if(count >= 2 && count <= 3 && GetRulong(args[0], addr))
	{
		if(count == 3 && !GetRulong(args[2], maxsize))
			return false;

		rulong dw;
		if(GetRulong(args[1], dw))
		{
			finddata = rul2hexstr(reverse(dw));
			// Remove trailing zeroes, keep even character count
			size_t end;
			for(end = finddata.length()-1; end; end--)
			{
				if(finddata[end] != '0')
					break;
			}
			end++;
			finddata.resize(end + (end % 2), '0');
		}
		else if(GetString(args[1], finddata))
		{
			var v = finddata;
			finddata = v.to_bytes();
			if(!v.isbuf)
				ReplaceString(finddata, "3f", "??"); // 0x3F = '?' -> wildcard like "mov ?ax, ?bx"
		}
		else return false;

		if(is_hexwild(finddata))
		{
			variables["$RESULT"] = 0;

			// search in current mem block
			MEMORY_BASIC_INFORMATION MemInfo;
			if(TE_GetMemoryInfo(addr, &MemInfo))
			{
				rulong memlen = (rulong)MemInfo.BaseAddress + MemInfo.RegionSize - addr;
				if(maxsize && maxsize < memlen)
						memlen = maxsize;

				byte* membuf = 0, * mask = 0, * bytes;

				try
				{
					membuf = new byte[memlen];
					if(TE_ReadMemory(addr, memlen, membuf))
					{
						size_t bytecount = finddata.length()/2;

						mask  = new byte[bytecount];
						bytes = new byte[bytecount+MAX_INSTR_SIZE];

						hexstr2bytemask(finddata, mask, bytecount);
						hexstr2bytes(finddata, bytes, bytecount);

						for(size_t i = 0; (i+bytecount) <= memlen;)
						{
							size_t len = LengthDisassemble(membuf+i);
							if(!len)
								break;

							if(len >= bytecount && memcmp_mask(membuf+i, bytes, mask, bytecount))
							{
								variables["$RESULT"] = addr + i;
								variables["$RESULT_1"] = len;
								break;
							}
							i += len;
						}

						delete[] mask;
						delete[] bytes;
					}
					delete[] membuf;
				}
				catch(std::bad_alloc)
				{
					delete[] membuf;
					delete[] mask;
					errorstr = "Out of memory!";
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool OllyLang::DoFINDMEM(const string* args, size_t count)
{
rulong addr = 0; 

	if(count >= 1 && count <= 2)
	{
		if(count == 2 && !GetRulong(args[1], addr))
			return false;

		variables["$RESULT"] = 0;

		MEMORY_BASIC_INFORMATION MemInfo;
		while(TE_GetMemoryInfo(addr, &MemInfo) && !variables["$RESULT"].dw)
		{
			if(!callCommand(&OllyLang::DoFIND, 2, rul2hexstr(addr).c_str(), args[0].c_str()))
				return false;
			addr = (rulong)MemInfo.BaseAddress + MemInfo.RegionSize;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoFREE(const string* args, size_t count)
{
rulong addr, size = 0;

	if(count >= 1 && count <= 2 && GetRulong(args[0], addr))
	{
		if(count == 2 && !GetRulong(args[1], size))
			return false;

		variables["$RESULT"] = 0;
	
		if((!size && TE_FreeMemory(addr)) || (size && TE_FreeMemory(addr, size)))
		{
			variables["$RESULT"] = true;
			unregMemBlock(addr);
		}
		return true;
	}
	return false;
}

bool OllyLang::DoGAPI(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		errorstr = "Unsupported command!";
		return false;

		//variables["$RESULT_4"] = Debugger::GetJumpDestination(TE_GetProcessHandle(), Debugger::GetContextData(UE_CIP)); 
		
		/*
		rulogn size, test, addr2
		BYTE buffer[MAXCMDSIZE];
		//size=Readmemory(buffer, addr, MAXCMDSIZE, MM_SILENT);
		size = Readcommand(addr, (char *)buffer);
		
		if(size > 0)
		{
			t_disasm disasm;
			size = Disasm(buffer, size, addr, NULL, &disasm, DISASM_CODE, NULL);
			test = disasm.jmpaddr;

			if(size > 0)
			{
//				variables["$RESULT"] = disasm.result; //asm command text
//				variables["$RESULT_1"] = disasm.dump;     //command bytes
				variables["$RESULT_3"] = disasm.addrdata;
				variables["$RESULT_4"] = disasm.jmpaddr; 

			}
		}
		if(test)
		{
			t_disasm disasm;
			size = Disasm(buffer, size, addr, NULL, &disasm, DISASM_CODE, NULL);
		 	char sym[4096] = {0};
		    char buf[TEXTLEN] = {0};
			addr2 = disasm.addrdata;
            int res = Decodeaddress(addr2, 0, ADC_JUMP | ADC_STRING | ADC_ENTRY | ADC_OFFSET | ADC_SYMBOL, sym, 4096, buf);
		    if(res)
			{
				variables["$RESULT"] = sym;
				char *tmp = strstr(sym, ".");
				if(tmp)
				{
					strtok(sym, ">");                          //buxfix
					*tmp = '\0';
					variables["$RESULT_1"] = sym + 2;          //bugfix
					variables["$RESULT_2"] = tmp + 1;
				}
			}
		    return true;
		}
		*/
		variables["$RESULT"] = 0;
		return true;
	}
    return false;
}

bool OllyLang::DoGBPM(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$RESULT"] = break_memaddr;
		return true;
	}
	return false;
}

bool OllyLang::DoGBPR(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$RESULT"] = break_reason;
		return true;
	}
	return false;
}

bool OllyLang::DoGCI(const string* args, size_t count)
{
string param;
rulong addr;

	if(count == 2 && GetRulong(args[0], addr))
	{
		param = toupper(args[1]);

		if(param == "COMMAND")
		{
			size_t size = 0;
			string instr = DisassembleEx(addr, &size);
			if(size)
				variables["$RESULT"] = instr; 
			else
				variables["$RESULT"] = 0;
			return true;
		}
		else if(param == "CONDITION") // Olly only
		{
			variables["$RESULT"] = 0;
			return true;
		}
		else if(param == "DESTINATION") 
		{
			/*
			if(is_RETNX)
				variables["$RESULT"] = [ESP+X];
			*/
			variables["$RESULT"] = Debugger::GetJumpDestination(TE_GetProcessHandle(), addr);
			return true;
		}
		else if(param == "SIZE") 
		{
			variables["$RESULT"] = LengthDisassembleEx(addr);
			return true;
		}
		else if(param == "TYPE") // Olly only
		{
			variables["$RESULT"] = 0;
			return true;
		}
	}
	return false;
}

// Olly only
bool OllyLang::DoGCMT(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		variables["$RESULT"] = "";
		return true;
	}
	return false;
}

bool OllyLang::DoGFO(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		variables["$RESULT"] = 0;

		vector<MODULEENTRY32> Modules;
		if(TE_GetModules(Modules))
		{
			for(size_t i = 0; i < Modules.size(); i++)
			{
				if(addr >= (rulong)Modules[i].modBaseAddr && addr < ((rulong)Modules[i].modBaseAddr + Modules[i].modBaseSize))
				{
					const Librarian::LIBRARY_ITEM_DATA* lib = Librarian::GetLibraryInfoEx(Modules[i].modBaseAddr);
					if(lib && lib->hFileMappingView)
					{
						ULONG_PTR filebase = Dumper::GetPE32DataFromMappedFile((ULONG_PTR)lib->hFileMappingView, NULL, UE_IMAGEBASE);
						variables["$RESULT"] = Dumper::ConvertVAtoFileOffset((ULONG_PTR)lib->hFileMappingView, addr-(ULONG_PTR)Modules[i].modBaseAddr+filebase, false);
					}
					break;
				}
			}
		}
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoGLBL(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		variables["$RESULT"] = 0;
		return true;
	}
	return false;
}

bool OllyLang::DoGMA(const string* args, size_t count)
{
string mod;

	if(count == 2 && GetString(args[0], mod))
	{
		if(mod.length() > 8)
			mod.resize(8);
		mod = tolower(mod);

		vector<MODULEENTRY32> Modules;
		if(TE_GetModules(Modules))
		{
			for(size_t i = 0; i < Modules.size(); i++)
			{
				string cur = Modules[i].szModule;
				if(cur.length() > 8)
					cur.resize(8);
				if(tolower(cur) == mod)
				{
					return callCommand(&OllyLang::DoGMI, 2, rul2hexstr((rulong)Modules[i].modBaseAddr).c_str(), args[1].c_str());
				}
			}
		}
		variables["$RESULT"] = 0;
		return true;
	}
	return false;
}

bool OllyLang::DoGMEMI(const string* args, size_t count)
{
rulong addr;
string val;

	if(count == 2 && GetRulong(args[0], addr))
	{
		variables["$RESULT"] = 0;

		MEMORY_BASIC_INFORMATION MemInfo;
		if(TE_GetMemoryInfo(addr, &MemInfo))
		{
			val = toupper(args[1]);

			     if(val == "MEMORYBASE")  variables["$RESULT"] = (rulong)MemInfo.BaseAddress;
			else if(val == "MEMORYSIZE")  variables["$RESULT"] = MemInfo.RegionSize;
			else if(val == "MEMORYOWNER") variables["$RESULT"] = (rulong)MemInfo.AllocationBase;
			else
			{
				errorstr = "Second operand bad";
				return false;
			}
		}
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoNAMES(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		return true;
	}
	return false;
}

bool OllyLang::DoGMEXP(const string* args, size_t count)
{
rulong addr, num = 0;
string str;

	if(count >= 2 && count <= 3 && GetRulong(args[0], addr))
	{
		if(count == 3 && !GetRulong(args[2], num))
			return false;

		errorstr = "Unsupported command!";
		return false;

		variables["$RESULT"] = 0;

		str = toupper(args[1]);

		/*
		ulong count = 0;
		bool cache = false, cached = false;

		t_module * mod = Findmodule(addr);
		if(!mod)
		{
			return true;
		}

		t_export exp = {0};

		if(str == "COUNT")
		{
			cache = true;
			tExportsCache.clear();
			exportsCacheAddr = addr;
		}
		else
		{
			if(exportsCacheAddr == addr && num < tExportsCache.size())
			{
				exp = tExportsCache[num];
				count = tExportsCache.size();
				cached = true;
			}
		}

		if(!cached)
		{
			for(ulong i = 0; i < mod->codesize ; i++)
			{
				if(Findname(mod->codebase + i, NM_EXPORT, exp.label))
				{
					count++;
					exp.addr = mod->codebase + i;
					if(count == num && !cache) break;
					if(cache)
						tExportsCache.push_back(exp);
				}
			}
		}

		if(num > count) //no more
		{
			return true;
		}

		     if(str == "COUNT")   variables["$RESULT"] = count;
		else if(str == "ADDRESS") variables["$RESULT"] = exp.addr;
		else if(str == "LABEL")   variables["$RESULT"] = exp.label;
		else
		{
			errorstr = "Second operand bad";
			return false;
		}
		*/
		return true;
	}
	return false;
}

bool OllyLang::DoGMI(const string* args, size_t count)
{
rulong addr;
string str;

	if(count == 2 && GetRulong(args[0], addr))
	{
		vector<MODULEENTRY32> Modules;
		MODULEENTRY32* Module;
		rulong ModBase = 0;

		variables["$RESULT"] = 0;

		if(TE_GetModules(Modules))
		{
			for(size_t i = 0; i < Modules.size(); i++)
			{
				Module = &Modules[i];
				if(addr >= (rulong)Module->modBaseAddr && addr < ((rulong)Module->modBaseAddr + Module->modBaseSize))
				{
					ModBase = (rulong)Module->modBaseAddr;
					break;
				}
			}
		}

		Dumper::PEStruct PEInfo;

		if(!ModBase || !GetPE32DataEx(Module->szExePath, &PEInfo))
		{
			return true;
		}

		str = toupper(args[1]);

		if(str == "MODULEBASE")
		{ 
			variables["$RESULT"] = ModBase;
		}
		else if(str == "MODULESIZE")
		{
			variables["$RESULT"] = round_up(Module->modBaseSize, PAGE_SIZE);
		}
		else if(str == "CODEBASE") // workaround: section of EP
		{
			HANDLE hFile, hMap;
			DWORD Size;
			ULONG_PTR VA;
			if(Static::FileLoad(Module->szExePath, UE_ACCESS_READ, false, &hFile, &Size, &hMap, &VA))
			{
				DWORD Section = Dumper::GetPE32SectionNumberFromVA(VA, PEInfo.ImageBase + PEInfo.OriginalEntryPoint);
				if(Section != UE_VANOTFOUND)
				{
					variables["$RESULT"] = ModBase + Dumper::GetPE32DataFromMappedFile(VA, Section, UE_SECTIONVIRTUALOFFSET);
				}
				Static::FileUnload(Module->szExePath, false, hFile, Size, hMap, VA);
			}
		}
		else if(str == "CODESIZE")
		{
			HANDLE hFile, hMap;
			DWORD Size;
			ULONG_PTR VA;
			if(Static::FileLoad(Module->szExePath, UE_ACCESS_READ, false, &hFile, &Size, &hMap, &VA))
			{
				DWORD Section = Dumper::GetPE32SectionNumberFromVA(VA, PEInfo.ImageBase + PEInfo.OriginalEntryPoint);
				if(Section != UE_VANOTFOUND)
				{
					variables["$RESULT"] = round_up(Dumper::GetPE32DataFromMappedFile(VA, Section, UE_SECTIONVIRTUALSIZE), PAGE_SIZE);
				}
				Static::FileUnload(Module->szExePath, false, hFile, Size, hMap, VA);
			}
		}
		else if(str == "ENTRY")
		{
			variables["$RESULT"] = ModBase + PEInfo.OriginalEntryPoint;
		}
		else if(str == "NSECT")
		{
			variables["$RESULT"] = PEInfo.SectionNumber;
		}
		else if(str == "DATABASE")
		{
			//variables["$RESULT"] = 0;
		}
		else if(str == "EDATATABLE")
		{
			if(PEInfo.ExportTableSize)
				variables["$RESULT"] = ModBase + PEInfo.ExportTableAddress;
		}
		else if(str == "EDATASIZE")
		{
			variables["$RESULT"] = PEInfo.ExportTableSize;
		}
		else if(str == "IDATABASE")
		{
			if(PEInfo.ImportTableSize)
			{
				HANDLE hFile, hMap;
				DWORD Size;
				ULONG_PTR VA;
				if(Static::FileLoad(Module->szExePath, UE_ACCESS_READ, false, &hFile, &Size, &hMap, &VA))
				{
					DWORD Section = Dumper::GetPE32SectionNumberFromVA(VA, PEInfo.ImageBase + PEInfo.ImportTableAddress);
					if(Section != UE_VANOTFOUND)
					{
						variables["$RESULT"] = ModBase + Dumper::GetPE32DataFromMappedFile(VA, Section, UE_SECTIONVIRTUALOFFSET);
					}
					Static::FileUnload(Module->szExePath, false, hFile, Size, hMap, VA);
				}
			}
		}
		else if(str == "IDATATABLE")
		{
			if(PEInfo.ImportTableSize)
				variables["$RESULT"] = ModBase + PEInfo.ImportTableAddress;
		}
		else if(str == "IDATASIZE")
		{
			variables["$RESULT"] = PEInfo.ImportTableSize;
		}
		else if(str == "RESBASE")
		{
			if(PEInfo.ResourceTableSize)
			{
				HANDLE hFile, hMap;
				DWORD Size;
				ULONG_PTR VA;
				if(Static::FileLoad(Module->szExePath, UE_ACCESS_READ, false, &hFile, &Size, &hMap, &VA))
				{
					DWORD Section = Dumper::GetPE32SectionNumberFromVA(VA, PEInfo.ImageBase + PEInfo.ResourceTableAddress);
					if(Section != UE_VANOTFOUND)
					{
						variables["$RESULT"] = ModBase + Dumper::GetPE32DataFromMappedFile(VA, Section, UE_SECTIONVIRTUALOFFSET);
					}
					Static::FileUnload(Module->szExePath, false, hFile, Size, hMap, VA);
				}
			}
			//if(PEInfo.ResourceTableSize)
			//	variables["$RESULT"] = ModBase + PEInfo.ResourceTableAddress;
		}
		else if(str == "RESSIZE")
		{
			variables["$RESULT"] = PEInfo.ResourceTableSize;
		}
		else if(str == "RELOCTABLE")
		{
			if(PEInfo.RelocationTableSize)
				variables["$RESULT"] = ModBase + PEInfo.RelocationTableAddress;
		}
		else if(str == "RELOCSIZE")
		{
			variables["$RESULT"] = PEInfo.RelocationTableSize;
		}
		else if(str == "NAME") // max 8 chars, no extension
		{
			string name = Module->szModule;

			size_t offset;
			if((offset = name.rfind('.')) != string::npos)
				name.resize(offset);
			if(name.length() > 8)
				name.resize(8);
			variables["$RESULT"] = name;
		}
		else if(str == "PATH")
		{
			variables["$RESULT"] = Module->szExePath;
		}
		else if(str == "VERSION")
		{
			string version;
			if(GetAppVersionString(Module->szExePath, "FileVersion", version))
				variables["$RESULT"] = version;
			else
				variables["$RESULT"] = "";
		}
		else
		{
			errorstr = "Second operand bad";
			return false;
		}
		return true;
	}
	errorstr = "Bad operand";
	return false;
}

bool OllyLang::DoGMIMP(const string* args, size_t count)
{
rulong addr, num = 0;
string str;

	if(count >= 2 && count <= 3 && GetRulong(args[0], addr))
	{
		if(count == 3 && !GetRulong(args[2], num))
			return false;

		errorstr = "Unsupported command!";
		return false;

		variables["$RESULT"] = 0;

		str = toupper(args[1]);

		/*
		rulong i, count=0;
		string str;
		bool cache = false, cached=false;


		t_module * mod = Findmodule(addr);
		if(!mod)
		{
			return true;
		}

		t_export exp={0};

		if(str == "COUNT")
		{
			cache = true;
			tImportsCache.clear();
			importsCacheAddr = addr;
		}
		else
		{
			if(importsCacheAddr == addr && num < tImportsCache.size())
			{
				exp = tImportsCache[num];
				count = tImportsCache.size();
				cached = true;
			}
		}

		if(!cached)
		{
			for(i = 0; i < mod->codesize ; i++)
			{
				if (Findname(mod->codebase + i, NM_IMPORT, exp.label))
				{
					count++;
					exp.addr=mod->codebase + i;
					if(count == num && !cache) break;
					if(cache)
						tImportsCache.push_back(exp);
				}
			}
		}

		if(num > count) //no more
		{
			return true;
		}

		if(str == "COUNT")
		{
			variables["$RESULT"] = count;
		}
		else if(str == "ADDRESS")
		{
			variables["$RESULT"] = exp.addr;
		}
		else if(str == "LABEL")
		{
			variables["$RESULT"] = exp.label;
		}
		else if(str == "NAME")
		{
			string s = exp.label;
			if(s.find('.') != string::npos)
				variables["$RESULT"] = s.substr(s.find('.')+1);
			else 
				variables["$RESULT"] = exp.label;
		}
		else if(str == "MODULE")
		{
			string s = exp.label;
			if(s.find('.') != string::npos)
			{
				variables["$RESULT"] = s.substr(0, s.find('.'));
			}
			else 
				variables["$RESULT"] = "";
		}
		else
		{
			errorstr = "Second operand bad";
			return false;
		}
		*/
		return true;
	}
	return false;
}

bool OllyLang::DoGN(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		const char * pAPI = Importer::GetAPIName(addr);
		const char * pDLL = Importer::GetDLLName(addr);
		if(pAPI && pDLL && pAPI[0] && pDLL[0])
		{
			string API = pAPI, DLL = pDLL;

			size_t offset;
			if((offset = DLL.rfind('.')) != string::npos) // remove extension
				DLL.resize(offset);

			variables["$RESULT"] = API;
			variables["$RESULT_1"] = DLL;
			variables["$RESULT_2"] = API;
		}
		else
		{
			variables["$RESULT"] = variables["$RESULT_1"] = variables["$RESULT_2"] = 0;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoGO(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		Debugger::SetBPX(addr, UE_SINGLESHOOT, &SoftwareCallback);
		bInternalBP = true;
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoGOPI(const string* args, size_t count)
{
rulong addr, index;
string param;

	if(count == 3 && GetRulong(args[0], addr) && GetRulong(args[1], index))
	{
		if(index < 1 || index > 3)
		{
			errorstr = "Bad operand index (1-3)";
			return false;
		}

		index--;

		param = toupper(args[2]);

		errorstr = "Unsupported command!";
		return false;
		
		return true;
		/*
		
		rulong size;

		BYTE buffer[MAXCMDSIZE]={0};
//		size=Readmemory(buffer, addr, MAXCMDSIZE, MM_SILENT);
		size=Readcommand(addr,(char *) buffer);

		if (size>0) 
		{
			t_disasm disasm;
			size = Disasm(buffer, size, addr, NULL, &disasm, DISASM_ALL, TE_GetCurrentThreadId());

			if(size <= 0)
				return false;
			else if (param == "TYPE")
			{
				variables["$RESULT"] = disasm.optype[index]; // Type of operand (extended set DEC_xxx)
				return true;
			}
			else if (param == "SIZE") 
			{
				variables["$RESULT"] = disasm.opsize[index]; // Size of operand, bytes
				return true;
			}
			else if (param == "GOOD") 
			{
				variables["$RESULT"] = disasm.opgood[index]; // Whether address and data valid
				return true;
			}
			else if (param == "ADDR") 
			{
				variables["$RESULT"] = disasm.opaddr[index]; // Address if memory, index if register
				return true;
			}
			else if (param == "DATA") 
			{
				variables["$RESULT"] = disasm.opdata[index]; // Actual value (only integer operands)
				return true;
			}
		}
		*/
	}
	return false;
}

bool OllyLang::DoGPA(const string* args, size_t count)
{
string proc, lib;
bool dofree;

	if(count >= 2 && count <= 3 && GetString(args[0], proc) && GetString(args[1], lib))
	{
		if(count == 3 && !GetBool(args[2], dofree))
			return false;

		rulong addr = Importer::GetRemoteAPIAddressEx(lib.c_str(), proc.c_str());
		if(addr)
		{
			variables["$RESULT"] = addr;
			variables["$RESULT_1"] = lib;
			variables["$RESULT_2"] = proc;
		}
		else
		{
			variables["$RESULT"] = variables["$RESULT_1"] = variables["$RESULT_2"] = 0;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoGPI(const string* args, size_t count)
{
string str;

	if(count == 1)
	{
		str = toupper(args[0]);

		if(str == "HPROCESS") // Handle of debugged process 
		{
			variables["$RESULT"] = (rulong)TE_GetProcessHandle();
		}
		else if(str == "PROCESSID") // Process ID of debugged process 
		{
			variables["$RESULT"] = TE_GetProcessId();
		}
		else if(str == "HMAINTHREAD") // Handle of main thread of debugged process 
		{
			variables["$RESULT"] = (rulong)TE_GetMainThreadHandle();
		}
		else if(str == "MAINTHREADID") // Thread ID of main thread of debugged process 
		{
			variables["$RESULT"] = TE_GetMainThreadId();
		}
		else if(str == "MAINBASE") // Base of main module in the debugged process (NOT DLL -> loader base)
		{
			variables["$RESULT"] = (rulong)Debugger::GetDebuggedFileBaseAddress();
		}
		else if(str == "PROCESSNAME") // File name of the debugged process/dll (no extension)
		{
			string name = filefrompath(TE_GetTargetPath());
			size_t offs;
			if((offs = name.rfind('.')) != string::npos)
				name.resize(offs);
			variables["$RESULT"] = name;
		}
		else if(str == "EXEFILENAME") // Full path of the debugged file/dll
		{
			variables["$RESULT"] = TE_GetTargetPath();
		}
		else if(str == "CURRENTDIR") // Current directory for debugged process (with trailing '\')
		{
			variables["$RESULT"] = TE_GetTargetDirectory();
		}
		else if(str == "SYSTEMDIR") // Windows system directory (with trailing '\')
		{
			char SysDir[MAX_PATH];
			GetSystemDirectoryA(SysDir, _countof(SysDir));
			variables["$RESULT"] = pathfixup(SysDir, true);
		}
		else
		{
			errorstr = "Bad operand";
			return false;
		}
		return true;
	}
	return false;
}

//in dev... i try to find API parameters number and types
bool OllyLang::DoGPP(const string* args, size_t count)
{
	/*
	if (!DoGPA(args))
		return false;

	ulong addr = variables["$RESULT"].dw;
	if (addr==0)
		return false;

	string sAddr = _itoa(addr,buffer,16);
	if (!DoREF(sAddr))
		return false;

	int t=Plugingetvalue(VAL_REFERENCES);
	if (t<=0)
		return false;

	int size;
	t_table* tref=(t_table*) t;
	for (int n=0;n<tref->data.n;n++) {
	
		t_ref* ref= (t_ref*) Getsortedbyselection(&tref->data,n);
			
		if (ref->addr == addr)
			continue; 

			//Disasm origin to get comments
		BYTE buffer[MAXCMDSIZE];
		size=Readmemory(buffer, ref->addr, MAXCMDSIZE, MM_SILENT);					
		if (size>0) {

			t_disasm disasm;
			t_module* mod = Findmodule(ref->addr);
			Analysecode(mod);

			size=Disasm(buffer,size,ref->addr,NULL,&disasm,DISASM_ALL,NULL);
            DbgMsg(disasm.nregstack,disasm.comment);

			if (size>0) {
				variables["$RESULT"] = ref->addr;
				variables["$RESULT_1"] = disasm.result; //command text
				variables["$RESULT_2"] = disasm.comment;
				return true; 
			}
		}
	}
	*/
	return true;
}

// Olly only
bool OllyLang::DoGREF(const string* args, size_t count)
{
rulong line;

	if(count >= 0 && count <= 1)
	{
		if(count == 1 && !GetRulong(args[0], line))
			return false;

		variables["$RESULT"] = 0;
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoGRO(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		variables["$RESULT"] = 0;
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoGSL(const string* args, size_t count)
{
string str = "";

	if(count >= 0 && count <= 1)
	{
		if(count == 1)
			str = args[0];

		const char valid_commands[][9] = {"", "CPUDASM", "CPUDUMP", "CPUSTACK"};

		variables["$RESULT"] = 0;
		variables["$RESULT_1"] = 0;
		variables["$RESULT_2"] = 0;

		return (valid_commands+_countof(valid_commands) != find(valid_commands, valid_commands+_countof(valid_commands), toupper(str)));
	}
	return false;
}

bool OllyLang::DoGSTR(const string* args, size_t count)
{
rulong addr, size = 2;

	if(count >= 1 && count <= 2 && GetRulong(args[0], addr))
	{
		if(count == 2 && !GetRulong(args[1], size))
			return false;

		variables["$RESULT"] = variables["$RESULT_1"] = 0;

		MEMORY_BASIC_INFORMATION MemInfo;
		if(TE_GetMemoryInfo(addr, &MemInfo))
		{
			rulong memsize = (rulong)MemInfo.BaseAddress + MemInfo.RegionSize - addr;

			char* buffer;

			try
			{
				buffer = new char[memsize+1];
			}
			catch(std::bad_alloc)
			{
				errorstr = "Out of memory!";
				return false;
			}

			buffer[0] = buffer[memsize] = '\0';

			if(TE_ReadMemory(addr, memsize, buffer))
			{
				rulong strsize = strlen(buffer);
				if(strsize && strsize >= size && strsize < memsize)
				{
					variables["$RESULT"] = buffer;
					variables["$RESULT_1"] = strsize;
				}
			}

			delete[] buffer;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoHANDLE(const string* args, size_t count)
{
rulong x, y;
string sClassName;

	if(count == 3 && GetRulong(args[0], x) && GetRulong(args[1], y) && GetString(args[2], sClassName)) 
	{
		variables["$RESULT"] = (rulong)FindHandle(TE_GetMainThreadId(), sClassName, x, y);
		return true;
	}
	return false;
}

bool OllyLang::DoINC(const string* args, size_t count)
{
rulong dw;
double flt;

	if(count == 1)
	{
		if(GetRulong(args[0], dw))
		{
			dw++;
			return SetRulong(args[0], dw);
		}
		else if(GetFloat(args[0], flt))
		{
			flt++;
			return SetFloat(args[0], flt);
		}
	}
	return false;
}

// Olly only
bool OllyLang::DoHISTORY(const string* args, size_t count)
{
rulong dw;

	if(count == 1 && GetRulong(args[0], dw))
	{
		return true;
	}
	return false;
}

bool OllyLang::DoITOA(const string* args, size_t count)
{
rulong dw;
rulong base = 16;

	if(count >= 1 && count <= 2 && GetRulong(args[0], dw))
	{
		if(count == 2 && !GetRulong(args[1], base))
			return false;

		if(base >= 2 && base <= 32)
		{
			variables["$RESULT"] = rul2str(dw, base);
			return true;
		}
		errorstr = "Invalid base";
	}
	return false;
}

bool OllyLang::DoJA(const string* args, size_t count)
{
	if(count == 1)
	{
		if(!zf && !cf)
			return DoJMP(args, count);
		else
			return true;
	}
	return false;
}

bool OllyLang::DoJAE(const string* args, size_t count)
{
	if(count == 1)
	{
		if(!cf)
			return DoJMP(args, count);
		else
			return true;
	}
	return false;
}

bool OllyLang::DoJB(const string* args, size_t count)
{
	if(count == 1)
	{
		if(cf)
			return DoJMP(args, count);
		else
			return true;
	}
	return false;
}

bool OllyLang::DoJBE(const string* args, size_t count)
{
	if(count == 1)
	{
		if(zf || cf)
			return DoJMP(args, count);
		else
			return true;
	}
	return false;
}

bool OllyLang::DoJE(const string* args, size_t count)
{
	if(count == 1)
	{
		if(zf)
			return DoJMP(args, count);
		else
			return true;
	}
	return false;
}

bool OllyLang::DoJMP(const string* args, size_t count)
{
	if(count == 1 && script.is_label(args[0]))
	{
		script_pos_next = script.labels[args[0]];
		return true;
	}
	return false;
}

bool OllyLang::DoJNE(const string* args, size_t count)
{
	if(count == 1)
	{
		if(!zf)
			return DoJMP(args, count);
		else
			return true;
	}
	return false;
}

// TE?
bool OllyLang::DoKEY(const string* args, size_t count)
{
rulong key;
bool shift = false, ctrl = false;

	if(count >= 1 && count <= 3 && GetRulong(args[0], key))
	{
		switch(count)
		{
		case 3:
			if(!GetBool(args[2], ctrl)) return false;
		case 2:
			if(!GetBool(args[1], shift)) return false;
		}

		//Sendshortcut(PM_MAIN, 0, WM_KEYDOWN, ctrl, shift, key); 
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoLBL(const string* args, size_t count)
{
rulong addr;
string lbl;

	if(count == 2 && GetRulong(args[0], addr) && GetString(args[1], lbl))
	{
		return true;
	}
	return false;
}

bool OllyLang::DoLM(const string* args, size_t count)
{
rulong addr, size;
string filename;

    if(count == 3 && GetRulong(args[0], addr) && GetRulong(args[1], size) && GetString(args[2], filename))
    {
		if(!isfullpath(filename))
			filename = TE_GetTargetDirectory() + filename;	

		HANDLE hFile;
		hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			char membuf[PAGE_SIZE];
			DWORD bytes;

			bool success = true;

			if(!size)
				size = GetFileSize(hFile, NULL);

			variables["$RESULT"] = size;

			while(size >= sizeof(membuf) && success)
			{
				success = false;

				if(ReadFile(hFile, membuf, sizeof(membuf), &bytes, NULL) && (bytes == sizeof(membuf)))
				{
					if(TE_WriteMemory(addr, sizeof(membuf), membuf))
					{
						success = true;
					}
				}

				addr += sizeof(membuf);
				size -= sizeof(membuf);
			}

			if(success && size)
			{
				success = false;
				if(ReadFile(hFile, membuf, size, &bytes, NULL) && (bytes == size))
				{
					if(TE_WriteMemory(addr, size, membuf))
					{
						success = true;
					}
				}
			}

			CloseHandle(hFile);
			return success;
		}
        else errorstr = "Couldn't open file!";
    }
    return false;
}

// Olly only
bool OllyLang::DoLC(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoLCLR(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

bool OllyLang::DoLEN(const string* args, size_t count)
{
string str;

	if(count == 1 && GetString(args[0], str))
	{
		variables["$RESULT"] = str.length();
		return true;
	}
	return false;
}

bool OllyLang::DoLOADLIB(const string* args, size_t count)
{
string str;

	if(count == 1 && GetString(args[0], str))
	{
		variables["$RESULT"] = 0;

		errorstr = "Unsupported command!";
		return false;

		if(Remote::LoadLibrary(TE_GetProcessHandle(), str.c_str(), false))
		{
			// $RESULT EAX!!!
			back_to_debugloop = true;
			return true;
		}

		/*
		ulong fnload;

		SaveRegisters(true);
		rulong ip = GetContextData(UE_CIP);
		variables["$RESULT"] = 0;

		DoGPA("\"LoadLibraryA\",\"kernel32.dll\"");
		fnload = variables["$RESULT"].dw;

		//alloc memory bloc to store DLL name
		ULONG_PTR hMem = TE_AllocMemory(0x1000); //VirtualAllocEx(TE_GetProcessHandle(), NULL, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

		char bfdlladdr[10]={0};
		sprintf(bfdlladdr, "%09lX", hMem);

		TE_WriteMemory(hMem, str.length(), str.c_str());
		//Writememory((void*)str.c_str(), (ulong) hMem, str.length(), MM_DELANAL|MM_SILENT);

		if(DoPUSH(bfdlladdr))
		{
			char bffnloadlib[10] = {0};
			sprintf(bffnloadlib, "%09X", fnload);
			string libPtrToLoad = bffnloadlib;

			//ExecuteASM("call " + libPtrToLoad);	

			variables["$RESULT"] = 0;

			// result returned after process
			// variables["$RESULT"] = pt->reg.r[REG_EAX];
			t_dbgmemblock block={0};
			block.hmem = (void*)hMem;
			block.size = 0x1000;
			block.script_pos = script_pos;
			block.free_at_ip = ip;
			block.result_register = true;
			block.reg_to_return = UE_EAX; // !!!
			block.restore_registers = true;
			block.listmemory = true;
			block.autoclean = true;

			// Free memory block after next ollyloop
			regBlockToFree(block);
			require_addonaction = 1;
			back_to_debugloop = 1;
			
			return true;
		}
		*/
	
	}
	return false;
}

bool OllyLang::DoLOG(const string* args, size_t count)
{
string prefix;

	if(count >= 1 && count <= 2)
	{
		if(count == 2 && !GetString(args[1], prefix))
			return false;

		bool noprefix = (count == 1);

		rulong dw;
		string str;
		double flt;
		
		if(noprefix && is_writable(args[0]))
			prefix = args[0] + ": ";

		string out;

		if(GetRulong(args[0], dw))
		{
			out = toupper(rul2hexstr(dw, sizeof(rulong)*2));
		}
		else if(GetFloat(args[0], flt))
		{
			out = dbl2str(flt);
		}
		else if(GetString(args[0], str))
		{
			out = is_bytestring(str) ? str : CleanString(str);
		}
		else return false;

		TE_Log((prefix + out).c_str());
		return true;
	}
	return false;
}

bool OllyLang::DoLOGBUF(const string* args, size_t count)
{
string sep = " ";
rulong dw = 0;

	if(count >= 1 && count <= 3 && is_variable(args[0]))
	{
		switch(count)
		{
		case 3:
			if(!GetString(args[2], sep)) return false;
		case 2:
			if(!GetRulong(args[1], dw)) return false;
		}

		if(!dw)
			dw = 16;

		const var& v = variables[args[0]];

		string line;
		string data = v.to_bytes();

		for(size_t i = 0; i < v.size; i++)
		{
			line += data.substr(i*2, 2) + sep;
			if(i > 0 && !((i+1) % dw))
			{ 
				TE_Log(line.c_str());
				line = "";
			}
		}

		if(!line.empty())
			TE_Log(line.c_str());

		return true;
	}
	return false;
}

bool OllyLang::DoMEMCPY(const string* args, size_t count)
{
	if(count == 3)
	{
		return callCommand(&OllyLang::DoMOV, 3, ('[' + args[0] + ']').c_str(), ('[' + args[1] + ']').c_str(), args[2].c_str());
	}
	return false;	
}

bool OllyLang::DoMOV(const string* args, size_t count)
{
rulong maxsize = 0;

	if(count >= 2 && count <= 3)
	{
		if(count == 3 && !GetRulong(args[2], maxsize))
			return false;

		rulong dw;
		string str;
		double flt;
		bool bl;

		if(!is_writable(args[0]) && !DoVAR(&args[0], 1))
		{
			return false;
		}

		const register_t* reg;

		// Check destination
		if(is_variable(args[0]))
		{
			var& v = variables[args[0]];

			if(maxsize > sizeof(rulong) && is_memory(args[1])) // byte string
			{
				string tmp = UnquoteString(args[1], '[', ']');

				rulong src;
				if(GetRulong(tmp, src))
				{
					ASSERT(src != 0);

					byte* bytes;

					try
					{
						bytes = new byte[maxsize];
					}
					catch(std::bad_alloc)
					{
						errorstr = "Out of memory!";
						return false;
					}

					if(!TE_ReadMemory(src, maxsize, bytes))
					{
						delete[] bytes;
						return false;
					}
					v = '#' + bytes2hexstr(bytes, maxsize) + '#';
					delete[] bytes;
				}
			}
			else if(maxsize <= sizeof(rulong) && GetRulong(args[1], dw)) // rulong
			{
				// DW to DW/FLT var
				if(!maxsize) maxsize = sizeof(rulong);
				dw = resize(dw, maxsize);
				v = dw;
				v.size = maxsize;
			}
			else if(GetString(args[1], str, maxsize)) // string
			{
				v = str;
			}
			else if(GetFloat(args[1], flt)) // float
			{
				v = flt;
			}
			else return false;

			return true;
		}
		else if(reg = find_register(args[0]))
		{
			// Dest is register
			if(GetRulong(args[1], dw))
			{
				if(!maxsize) maxsize = reg->size;
				dw = resize(dw, min(maxsize, (rulong)reg->size));
				if(reg->size < sizeof(rulong))
				{
					rulong oldval, newval;
					oldval = Debugger::GetContextData(reg->id);
					oldval &= ~(((1 << (reg->size * 8)) - 1) << (reg->offset * 8));
					newval = resize(dw, reg->size) << (reg->offset * 8);
					dw = oldval | newval;
				}
				return Debugger::SetContextData(reg->id, dw);
			}
		}
		else if(is_flag(args[0]))
		{
			bool flagval;

			if(GetBool(args[1], flagval))
			{
				eflags_t flags;
				flags.dw = Debugger::GetContextData(UE_EFLAGS);

				switch(args[0][1])
				{
				case 'a': flags.bits.AF = flagval; break;
				case 'c': flags.bits.CF = flagval; break;
				case 'd': flags.bits.DF = flagval; break;
				case 'o': flags.bits.OF = flagval; break;
				case 'p': flags.bits.PF = flagval; break;
				case 's': flags.bits.SF = flagval; break;
				case 'z': flags.bits.ZF = flagval; break;
				}

				return Debugger::SetContextData(UE_EFLAGS, flags.dw);
			}
		}
		else if(is_floatreg(args[0]))
		{
			if(GetFloat(args[1], flt))
			{
				int index = args[0][3] - '0';
				double* preg;
				#ifdef _WIN64
					XMM_SAVE_AREA32 fltctx;
					preg = (double*)&fltctx.FloatRegisters + index;
				#else
					FLOATING_SAVE_AREA fltctx;
					preg = (double*)&fltctx.RegisterArea[0] + index;
				#endif
				if(Debugger::GetContextFPUDataEx(TE_GetCurrentThreadHandle(), &fltctx))
				{
					*preg = flt;
					return Debugger::SetContextFPUDataEx(TE_GetCurrentThreadHandle(), &fltctx);
				}
			}
		}
		else if(is_memory(args[0]))
		{
			string tmp = UnquoteString(args[0], '[', ']');

			rulong target;
			if(GetRulong(tmp, target))
			{
				ASSERT(target != 0);

				if(maxsize > sizeof(rulong) && is_memory(args[1]))
				{
					tmp = UnquoteString(args[1], '[', ']');

					rulong src;
					if(GetRulong(tmp, src))
					{
						ASSERT(src != 0);

						char* copybuffer;

						try
						{
							copybuffer = new char[maxsize];
						}
						catch(std::bad_alloc)
						{
							errorstr = "Out of memory!";
							return false;
						}

						if(!TE_ReadMemory(src, maxsize, copybuffer) || !TE_WriteMemory(target, maxsize, copybuffer))
						{
							delete[] copybuffer;
							return false;
						}
						delete[] copybuffer;
						return true;
					}
				}
				else if(maxsize <= sizeof(rulong) && GetRulong(args[1], dw))
				{
					if(!maxsize) maxsize = sizeof(rulong);
					return TE_WriteMemory(target, maxsize, &dw);
				}
				else if(GetString(args[1], str, maxsize))
				{
					var v = str;
					if(!maxsize) maxsize = v.size;
					maxsize = min(maxsize, (rulong)v.size);
					return TE_WriteMemory(target, maxsize, v.to_string().data());		
				}
				else if(GetFloat(args[1], flt))
				{
					return TE_WriteMemory(target, sizeof(flt), &flt);
				}
			}
		}
	}
	return false;
}

bool OllyLang::DoMSG(const string* args, size_t count)
{
string msg;

	if(count == 1 && GetAnyValue(args[0], msg))
	{
		int input;
		if(DialogMSG(msg, input))
		{
			if(input == IDCANCEL)
				return Pause();
			return true;
		}
	}
	return false;
}

bool OllyLang::DoMSGYN(const string* args, size_t count)
{
string msg;

	if(count == 1 && GetString(args[0], msg))
	{
		int input;
		if(DialogMSGYN(msg, input))
		{
			switch(input)
			{
			case IDCANCEL:
				variables["$RESULT"] = 2;
				return Pause();
			case IDYES:
				variables["$RESULT"] = 1;
				break;
			default:
				variables["$RESULT"] = 0;
				break;
			}
			return true;
		}
	}
	return false;
}

bool OllyLang::DoOLLY(const string* args, size_t count)
{
string param;

	if(count == 1)
	{
		param = toupper(args[0]);

		if(param == "PID")
		{
			variables["$RESULT"] = GetCurrentProcessId();
			return true;
		}
		else if(param == "HWND")
		{
			variables["$RESULT"] = NULL;
			return true;
		}
	}
	return false;
}

bool OllyLang::DoOR(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], dw1 | dw2);
	}
	return false;
}

bool OllyLang::DoMUL(const string* args, size_t count)
{
rulong dw1, dw2;
double flt1, flt2;

	if(count == 2)
	{
		if(GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
		{
			return SetRulong(args[0], dw1 * dw2);
		}
		else if(GetFloat(args[0], flt1) && GetFloat(args[1], flt2))
		{
			return SetFloat(args[0], flt1 * flt2);
		}
		else if(GetFloat(args[0], flt1) && GetRulong(args[1], dw2))
		{
			return SetFloat(args[0], flt1 * dw2);
		}
		else if(GetRulong(args[0], dw1) && GetFloat(args[1], flt2))
		{
			return SetFloat(args[0], dw1 * flt2);
		}
	}
	return false;
}

bool OllyLang::DoNEG(const string* args, size_t count)
{
rulong dw;
double flt;

	if(count == 1)
	{
		if(GetRulong(args[0], dw))
		{
			return SetRulong(args[0], -dw);
		}
		else if(GetFloat(args[0], flt))
		{
			return SetFloat(args[0], -flt);
		}
	}
	return false;
}

bool OllyLang::DoNOT(const string* args, size_t count)
{
rulong dw;

	if(count == 1 && GetRulong(args[0], dw))
	{
		return SetRulong(args[0], ~dw);
	}
	return false;
}

//see also GCI
bool OllyLang::DoOPCODE(const string* args, size_t count)
{
rulong addr;

	if(count == 1 && GetRulong(args[0], addr))
	{
		byte buffer[MAX_INSTR_SIZE];

		variables["$RESULT"] = variables["$RESULT_1"] = variables["$RESULT_2"] = 0;

		if(TE_ReadMemory(addr, sizeof(buffer), buffer))
		{
			size_t opsize;
			string opstring = Disassemble(buffer, addr, &opsize);
			if(opsize)
			{
				variables["$RESULT"]   = bytes2hexstr(buffer, opsize);
				variables["$RESULT_1"] = opstring;
				variables["$RESULT_2"] = opsize;
			}
		}
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoOPENDUMP(const string* args, size_t count)
{
rulong addr, base, size;

	if(count >= 1 && count <= 3 && GetRulong(args[0], addr))
	{
		switch(count)
		{
		case 3:
			if(!GetRulong(args[2], size)) return false;
		case 2:
			if(!GetRulong(args[1], base)) return false;
		}

		variables["$RESULT"] = 0;
		return true;
	}
	return false;
}

// Olly only
bool OllyLang::DoOPENTRACE(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

bool OllyLang::DoPAUSE(const string* args, size_t count)
{
	if(count == 0)
	{
		return Pause();
	}
	return false;
}

bool OllyLang::DoPOP(const string* args, size_t count)
{
rulong dw;

	if(count >= 0 && count <= 1)
	{
		if(count == 1 && !GetRulong(args[0], dw))
			return false;

		rulong CSP = Debugger::GetContextData(UE_CSP);
		Debugger::SetContextData(UE_CSP, CSP + sizeof(rulong));
		if(count == 1)
		{
			return (TE_ReadMemory(CSP, sizeof(dw), &dw) && SetRulong(args[0], dw));
		}
		return true;
	}
	return false;
}

bool OllyLang::DoPOPA(const string* args, size_t count)
{
	if(count == 0)
	{
		return RestoreRegisters(true);
	}
	return false;
}

bool OllyLang::DoPREOP(const string* args, size_t count)
{
rulong addr;

	if(count >= 0 && count <= 1)
	{
		if(count == 1 && !GetRulong(args[0], addr))
			return false;
		else if(count == 0)
			addr = Debugger::GetContextData(UE_CIP);

		variables["$RESULT"] = 0;

		size_t prevsize = LengthDisassembleBackEx(addr);
		if(prevsize)
		{
			variables["$RESULT"] = addr - prevsize;
		}
		return true;
	}
	return false;
}

bool OllyLang::DoPUSH(const string* args, size_t count)
{
rulong dw;

	if(count == 1 && GetRulong(args[0], dw))
	{
		rulong CSP = Debugger::GetContextData(UE_CSP) - sizeof(rulong);
		Debugger::SetContextData(UE_CSP, CSP);
		return TE_WriteMemory(CSP, sizeof(dw), &dw);;
	}
	return false;
}

bool OllyLang::DoPUSHA(const string* args, size_t count)
{
	if(count == 0)
	{
		return SaveRegisters(true);
	}
	return false;
}

bool OllyLang::DoREADSTR(const string* args, size_t count)
{
rulong maxsize;
string str;

	if(count == 2 && GetRulong(args[1], maxsize) && GetString(args[0], str, maxsize)) 
	{
		variables["$RESULT"] = str;
		return true;
	}
    return false;
}

// Restore Break Points
// restores all hardware and software breakpoints
// (if arg1 == 'STRICT', all soft bp set by script will be deleted and only those have been set before it runs
// will be restored
// if no argument set, previous soft bp will be appended to those set by script)

// rbp [arg1]
// arg1 = may be STRICT or nothing

// return in:
// - $RESULT number of restored swbp
// - $RESULT_1 number of restored hwbp

// ex     : rbp
//        : rbp STRICT
bool OllyLang::DoRBP(const string* args, size_t count)
{
bool strict = false;

	if(count >= 0 && count <= 1)
	{
		if(count == 1)
		{
			if(toupper(args[0]) != "STRICT")
				return false;
			strict = true;
		}

		return true;
	}
	return false;
	/*
	t_table* bpt = 0;
	t_bpoint* bpoint = 0;
	uint n,i=0;
	string ops[1];

	CreateOperands ( args, ops, 1 );
	
	variables["$RESULT"] = 0;

	if ( saved_bp )
	{
		bpt = ( t_table * ) Plugingetvalue ( VAL_BREAKPOINTS );
		if ( bpt != NULL )
		{
			bpoint = ( t_bpoint * ) bpt->data.data;

			if ( ops[0] == "STRICT" )
			{
				int dummy;
				dummy = bpt->data.n;
				for ( n = 0; n < dummy; n++ )
				{
					Deletebreakpoints ( bpoint->addr, ( bpoint->addr ) + 1, 1 );
				}
			}

			for ( n=0; n < sortedsoftbp_t.n; n++ )
				Setbreakpoint ( softbp_t[n].addr, ( softbp_t[n].type | TY_KEEPCODE ) ^ TY_KEEPCODE, 0 );

			variables["$RESULT"] = ( DWORD ) sortedsoftbp_t.n;

			Broadcast ( WM_USER_CHALL, 0, 0 );
		}

	}

	//Hardware Bps
	for ( n=0; n < 4; n++ ) {
		if (hwbp_t[n].addr) {
			Sethardwarebreakpoint ( hwbp_t[n].addr, hwbp_t[n].size, hwbp_t[n].type );
			i++;
		}
	}
	variables["$RESULT_1"] = ( DWORD ) i;
	*/

	return true;
}


// Olly only
bool OllyLang::DoREF(const string* args, size_t count)
{
rulong addr;
string str = "MEMORY";

	if(count >= 1 && count <= 2 && GetRulong(args[0], addr))
	{
		if(count == 2)
			str = args[1];

		const char valid_commands[][7] = {"MEMORY", "CODE", "MODULE"};

		variables["$RESULT"] = 0;
		variables["$RESULT_1"] = 0;
		variables["$RESULT_2"] = 0;

		return (valid_commands+_countof(valid_commands) != find(valid_commands, valid_commands+_countof(valid_commands), toupper(str)));

	}
	return false;
}

// Olly only
bool OllyLang::DoREFRESH(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

bool OllyLang::DoREPL(const string* args, size_t count)
{
rulong addr;
string v1, v2;
rulong len;

	if(count >= 3 && count <= 4 && GetRulong(args[0], addr) && is_bytestring(args[1]) && is_bytestring(args[2]))
	{
		if(count == 4 && !GetRulong(args[3], len))
			return false;
		else if(count == 3)
		{
			MEMORY_BASIC_INFORMATION MemInfo;
			if(!TE_GetMemoryInfo(addr, &MemInfo))
				return true;

			len = (rulong)MemInfo.BaseAddress + MemInfo.RegionSize - addr;
		}

		v1 = UnquoteString(args[1], '#');
		v2 = UnquoteString(args[2], '#');

		size_t oplen = v1.length();

		if(oplen != v2.length())
		{
			errorstr = "Hex strings must have the same size!";
			return false;
		}

		byte* membuf = 0, * mask1 = 0, * mask2 = 0, * bytes1 = 0, * bytes2;

		try
		{
			membuf = new byte[len];
			if(TE_ReadMemory(addr, len, membuf))
			{
				bool replaced = false;

				size_t bytecount = oplen/2;
				
				mask1  = new byte[bytecount];
				mask2  = new byte[bytecount];
				bytes1 = new byte[bytecount];
				bytes2 = new byte[bytecount];

				hexstr2bytemask(v1, mask1, bytecount);
				hexstr2bytemask(v2, mask2, bytecount);
				hexstr2bytes(v1, bytes1, bytecount);
				hexstr2bytes(v2, bytes2, bytecount);

				for(size_t i = 0; (i+bytecount) <= len;)
				{
					if(memcmp_mask(membuf+i, bytes1, mask1, bytecount))
					{
						memcpy_mask(membuf+i, bytes2, mask2, bytecount);
						i += bytecount;
						replaced = true;
					}
					else i++;
				}

				delete[] mask1;
				delete[] mask2;
				delete[] bytes1;
				delete[] bytes2;

				if(replaced)
					TE_WriteMemory(addr, len, membuf);
			}
			delete[] membuf;
		}
		catch(std::bad_alloc)
		{
			delete[] membuf;
			delete[] mask1;
			delete[] mask2;
			delete[] bytes1;
			errorstr = "Out of memory!";
			return false;
		}
		return true;
	}
	return false;
}

// TE?
bool OllyLang::DoRESET(const string* args, size_t count)
{
	if(count == 0)
	{
		//Sendshortcut(PM_MAIN, 0, WM_KEYDOWN, 1, 0, VK_F2); 
		return true;
	}
	return false;
}

bool OllyLang::DoRET(const string* args, size_t count)
{
	if(count >= 0 && count <= 1)
	{
		if(count == 0)
		{
			if(calls.size())
			{
				script_pos_next = calls.back();
				calls.pop_back(); 

				if(callbacks.size() && callbacks.back().call == calls.size())
				{
					if(callbacks.back().returns_value)
					{
						errorstr = "Callback needs to return a value!";
						return false;
					}
					callbacks.pop_back();
				}
			}
			else
			{
#ifdef TITANSCRIPT_STANDALONE_BUILD
				MsgInfo("Script finished");
#endif
				Reset();
			}
			return true;
		}
		else if(count == 1)
		{
			if(calls.size())
			{
				script_pos_next = calls.back();
				calls.pop_back();

				if(callbacks.size() && callbacks.back().call == calls.size())
				{
					if(callbacks.back().returns_value)
					{
						variables["$TEMP"] = 0;
						if(callCommand(&OllyLang::DoMOV, 2, "$TEMP", args[0].c_str()))
						{
							callback_return = variables["$TEMP"];
							variables.erase("$TEMP");

							if(callback_return.type == var::EMP || callback_return.type == callbacks.back().return_type)
							{
								callbacks.pop_back();
								return true;
							}
						}
						else errorstr = "Invalid callback return type";
					}
					else errorstr = "Callback shouldn't return a value!";
				}
				else errorstr = "Callback shouldn't return a value!";
			}
			else errorstr = "Returning value outside of a callback!";
		}
	}
	return false;
}

bool OllyLang::DoREV(const string* args, size_t count)
{
rulong dw;
string str;

	if(count == 1)
	{
		if(GetRulong(args[0], dw))
		{
			variables["$RESULT"] = reverse(dw);
			return true;
		}
		else if(GetString(args[0], str))
		{
			var tmp = str;
			tmp.reverse();
			variables["$RESULT"] = tmp;
			return true;
		}
	}
	return false;
}

bool OllyLang::DoROL(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], rol(dw1, dw2));
	}
	return false;
}

bool OllyLang::DoROR(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], ror(dw1, dw2));
	}
	return false;
}

bool OllyLang::DoRTR(const string* args, size_t count)
{
	if(count == 0)
	{
		run_till_return = true;
		stepcount = -1;
		StepOverCallback();
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoRTU(const string* args, size_t count)
{
	if(count == 0)
	{
		return_to_usercode = true;
		stepcount = -1;
		StepOverCallback();
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoRUN(const string* args, size_t count)
{
	if(count == 0)
	{
		back_to_debugloop = true;
		return true;
	}
	return false;
}

// Store Break Points
// stores all hardware and software breakpoints

// return in:
// - $RESULT number of restored swbp
// - $RESULT_1 number of restored hwbp

// ex 	: sbp
// 		: no argument

bool OllyLang::DoSBP(const string* args, size_t count)
{
	if(count == 0)
	{
		/*
		uint n = 0, i;
		bool success;
		t_table *bpt;
		t_bpoint *bpoint;

		variables["$RESULT"] = 0;
		variables["$RESULT_1"] = 0;

		bpt = ( t_table * ) Plugingetvalue ( VAL_BREAKPOINTS );
		if ( bpt != NULL )
		{
			bpoint = ( t_bpoint * ) ( bpt->data.data );
			if ( bpoint != NULL )
			{
				n = bpt->data.n;

				if ( n > alloc_bp )
				{
					//FreeBpMem();
					success = AllocSwbpMem ( n );
				}

				if ( n > alloc_bp && !success ) {
					errorstr = "Can't allocate enough memory to copy all breakpoints";
					return false;
				}
				else if (n > 0)
				{
					memcpy ( ( void* ) softbp_t, bpt->data.data, n*sizeof ( t_bpoint ) );
					memcpy ( ( void* ) &sortedsoftbp_t, ( void* ) &bpt->data, sizeof ( t_sorted ) );
					
				} 

				saved_bp = n;
				variables["$RESULT"] =  ( DWORD ) n;
			}
		}

		memcpy ( ( void* ) &hwbp_t, ( void* ) ( Plugingetvalue ( VAL_HINST ) +0xD8D70 ), 4 * sizeof ( t_hardbpoint ) );

		n = i = 0;
		while ( n < 4 )
		{
			if ( hwbp_t[n].addr )
				i++;
			n++;
		}
		variables["$RESULT_1"] =  ( DWORD ) i;
		*/
		return true;
	}
	return false;
}

bool OllyLang::DoSCMP(const string* args, size_t count)
{
string s1, s2;
rulong size = 0;

	if(count >= 2 && count <= 3)
	{
		if(count == 3 && !GetRulong(args[2], size))
			return false;

		if(GetString(args[0], s1, size) && GetString(args[1], s2, size))
		{
			SetCMPFlags(s1.compare(s2));
			return true;
		}
	}
	return false;
}

bool OllyLang::DoSCMPI(const string* args, size_t count)
{
	if(count >= 2 && count <= 3)
	{
		switch(count)
		{
		case 2:
			return callCommand(&OllyLang::DoSCMP, 2, tolower(args[0]).c_str(), tolower(args[1]).c_str());
		case 3:
			return callCommand(&OllyLang::DoSCMP, 3, tolower(args[0]).c_str(), tolower(args[1]).c_str(), args[2].c_str());
		}
	}
	return false;
}

// Olly only
bool OllyLang::DoSETOPTION(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

bool OllyLang::DoSHL(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], dw1 << dw2);
	}
	return false;
}

bool OllyLang::DoSHR(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], dw1 >> dw2);
	}
	return false;
}

bool OllyLang::DoSTI(const string* args, size_t count)
{
	if(count == 0)
	{
		stepcount = 1;
		StepIntoCallback();
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoSTO(const string* args, size_t count)
{
	if(count == 0)
	{
		stepcount = 1;
		StepOverCallback();
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoSTR(const string* args, size_t count)
{
	if(count == 1 && is_variable(args[0]))
	{
		var& v = variables[args[0]];
		switch(v.type)
		{
		case var::DW: // empty buf + dw -> buf
			v = var("##") + v.dw;
		case var::STR:
			if(v.isbuf)
				v = v.to_string();
			return true;
		}
	}
	return false;
}

bool OllyLang::DoSUB(const string* args, size_t count)
{
rulong dw1, dw2;
double flt1, flt2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		if(GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
		{
			return SetRulong(args[0], dw1 - dw2);
		}
		else if(GetFloat(args[0], flt1) && GetFloat(args[1], flt2))
		{
			return SetFloat(args[0], flt1 - flt2);
		}
	}
	return false;
}

// Olly only
bool OllyLang::DoTC(const string* args, size_t count)
{
	if(count == 0)
	{
		return true;
	}
	return false;
}

bool OllyLang::DoTEST(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		zf = ((dw1 & dw2) == 0);
		return true;
	}
	return false;
}

bool OllyLang::DoTI(const string* args, size_t count)
{
	if(count == 0)
	{
		back_to_debugloop = true;
		return true;
	}
	return false;
}

bool OllyLang::DoTICK(const string* args, size_t count)
{
rulong timeref;

	if(count >= 0 && count <= 2)
	{
		if(count == 2 && !GetRulong(args[1], timeref))
			return false;

		rulong tickcount = MyTickCount() - tickcount_startup;

		if(count == 0)
		{
			variables["$RESULT"] = rul2decstr(tickcount/1000) + " ms";
			return true;
		}
		else if(is_variable(args[0]) || DoVAR(&args[0], 1))
		{
			variables[args[0]] = tickcount;
			if(count == 2)
				variables["$RESULT"] = tickcount - timeref;
			return true;
		}
	}
	return false;
}

// TE?
bool OllyLang::DoTICND(const string* args, size_t count)
{
string condition;

	if(count == 1 && GetString(args[0], condition))
	{
		errorstr = "Unsupported command!";
		return false;

		/*
		char* buffer = new char[condition.length() + 1];
		strcpy(buffer, condition.c_str());
		if(Runtracesize() == 0) 
		{
			ulong threadid = Getcputhreadid();
			if(threadid == 0)
				threadid = Plugingetvalue(VAL_MAINTHREADID);
			t_thread* pthr = Findthread(threadid);
			if(pthr != NULL)
				Startruntrace(&(pthr->reg)); 
		}
		Settracecondition(buffer, 0, 0, 0, 0, 0);
		Sendshortcut(PM_MAIN, 0, WM_KEYDOWN, 1, 0, VK_F11); 
		back_to_debugloop = true;
		return true;
		*/
		
	}
	return false;
}

bool OllyLang::DoTO(const string* args, size_t count)
{
	if(count == 0)
	{
		back_to_debugloop = true;
		return true;
	}
	return false;
}

// TE?
bool OllyLang::DoTOCND(const string* args, size_t count)
{
string condition;

	if(count == 1 && GetString(args[0], condition))
	{
		errorstr = "Unsupported command!";
		return false;

		/*
		char* buffer = new char[condition.length() + 1];
		strcpy(buffer, condition.c_str());
		if(Runtracesize() == 0) 
		{
			ulong threadid = Getcputhreadid();
			if(threadid == 0)
				threadid = Plugingetvalue(VAL_MAINTHREADID);
			t_thread* pthr = Findthread(threadid);
			if(pthr != NULL)
				Startruntrace(&(pthr->reg));
		}
		Settracecondition(buffer, 0, 0, 0, 0, 0);
		Sendshortcut(PM_MAIN, 0, WM_KEYDOWN, 1, 0, VK_F12); 
		back_to_debugloop = true;
		return true;
		*/
	}
	return false;
}

// Unused
bool OllyLang::DoUNICODE(const string* args, size_t count)
{
bool enable;

	if(count == 1 && GetBool(args[0], enable))
	{
		return true;
	}
	return false;
}

bool OllyLang::DoVAR(const string* args, size_t count)
{
	if(count == 1)
	{
		if(is_valid_variable_name(args[0]))
		{
			variables[args[0]] = 0;
			return true;
		}
		errorstr = "Bad variable name: " + args[0];
	}
	return false;
}

bool OllyLang::DoXCHG(const string* args, size_t count)
{
rulong dw1, dw2;
double flt1, flt2;

	if(count == 2)
	{
		if(GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
		{
			return SetRulong(args[0], dw2) && SetRulong(args[1], dw1);
		}
		else if(GetFloat(args[0], flt1) && GetFloat(args[1], flt2))
		{
			return SetFloat(args[0], flt2) && SetFloat(args[1], flt1);
		}
		else if(is_variable(args[0]) && is_variable(args[1]))
		{
			var tmp = variables[args[0]];
			variables[args[0]] = variables[args[1]];
			variables[args[1]] = tmp;
			return true;
		}
	}
	return false;
}

bool OllyLang::DoXOR(const string* args, size_t count)
{
rulong dw1, dw2;

	if(count == 2 && GetRulong(args[0], dw1) && GetRulong(args[1], dw2))
	{
		return SetRulong(args[0], dw1 ^ dw2);
	}
	return false;
}

bool OllyLang::DoWRT(const string* args, size_t count)
{
string filename, data;

	if(count == 2 && GetString(args[0], filename) && GetAnyValue(args[1], data))
	{
		if(!isfullpath(filename))
			filename = TE_GetTargetDirectory() + filename;

		HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			DWORD written;
			WriteFile(hFile, data.c_str(), data.length(), &written, NULL);
			CloseHandle(hFile);
		}
		return true;
	}
	return false;
}

bool OllyLang::DoWRTA(const string* args, size_t count)
{
string filename, data, out = "\r\n";

	if(count >= 2 && count <= 3 && GetString(args[0], filename) && GetAnyValue(args[1], data))
	{
		if(count == 3 && !GetString(args[2], out))
			return false;

		if(!isfullpath(filename))
			filename = TE_GetTargetDirectory() + filename;

		HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			DWORD written;
			out += data;
			SetFilePointer(hFile, 0, NULL, FILE_END);
			WriteFile(hFile, out.c_str(), out.length(), &written, NULL);
			CloseHandle(hFile);
		}
		return true;
	}
	return false;
}
