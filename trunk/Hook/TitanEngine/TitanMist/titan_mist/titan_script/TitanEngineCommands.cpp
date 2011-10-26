#include "OllyLang.h"

#include "SDK.hpp"

using namespace TE;

/*
TE function return: $TE_RESULT
TE function callback arguments: $TE_ARG_X
*/

extern bool TSErrorExit;

//error
bool OllyLang::DoError(const string* args, size_t count)
{
	if(count == 0)
	{
		TSErrorExit = true;
		return true;
	}
	return false;
}

//dnf
bool OllyLang::DoDumpAndFix(const string* args, size_t count)
{
HANDLE hProcess;
void* LoadedBase;
ULONG_PTR ImageBase;
DWORD NtSizeOfImage;
DWORD SearchStart;
ULONG_PTR IATStart = NULL;
ULONG_PTR IATSize = NULL;

	if(count == 0)
	{
		hProcess = Debugger::GetProcessInformation()->hProcess;
		LoadedBase = (void*)Debugger::GetDebuggedFileBaseAddress();
		ImageBase = (ULONG_PTR)Dumper::GetPE32Data(variables["$INPUTFILE"].to_string().c_str(), NULL, UE_IMAGEBASE);
		NtSizeOfImage = (DWORD)Dumper::GetPE32Data(variables["$INPUTFILE"].to_string().c_str(), NULL, UE_SIZEOFIMAGE);
		SearchStart = (DWORD)Dumper::GetPE32Data(variables["$INPUTFILE"].to_string().c_str(), NULL, UE_SECTIONVIRTUALOFFSET) + ImageBase;
		if(Dumper::DumpProcess(hProcess, LoadedBase, variables["$OUTPUTFILE"].to_string().c_str(), Debugger::GetContextData(UE_CIP))){
			Importer::AutoSearchIAT(hProcess, variables["$OUTPUTFILE"].to_string().c_str(), ImageBase, SearchStart, NtSizeOfImage, &IATStart, &IATSize); 
			if(IATStart != NULL && IATSize != NULL){
				if(ImporterA::AutoFixIAT(hProcess, variables["$OUTPUTFILE"].to_string().c_str(), ImageBase, IATStart, IATSize, NULL) == 0x400){
					Realigner::RealignPEEx(variables["$OUTPUTFILE"].to_string().c_str(), NULL, NULL);
					Dumper::MakeAllSectionsRWE(variables["$OUTPUTFILE"].to_string().c_str());
					Debugger::StopDebug();
					variables["$TE_RESULT"] = true;
					return true;
				}
			}
		}
	}
	variables["$TE_RESULT"] = false;
	return false;
}

//bool StopDebug();
bool OllyLang::DoStopDebug(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Debugger::StopDebug();
		return true;
	}
	return false;
}

//bool DumpProcess(HANDLE hProcess, void* ImageBase, const char* szDumpFileName, ULONG_PTR EntryPoint);
bool OllyLang::DoDumpProcess(const string* args, size_t count)
{
rulong hProcess, ImageBase, EntryPoint;
string szDumpFileName;

	if(count == 4 && GetRulong(args[0], hProcess) && GetRulong(args[1], ImageBase) && GetString(args[2], szDumpFileName) && GetRulong(args[3], EntryPoint))
	{
		variables["$TE_RESULT"] = Dumper::DumpProcess((HANDLE)hProcess, (void*)ImageBase, szDumpFileName.c_str(), EntryPoint);
		return true;
	}
	return false;
}

//bool DumpRegions(HANDLE hProcess, char* szDumpFolder, bool DumpAboveImageBaseOnly);
bool OllyLang::DoDumpRegions(const string* args, size_t count)
{
rulong hProcess;
string szDumpFolder;
bool DumpAboveImageBaseOnly;

	if(count == 3 && GetRulong(args[0], hProcess) && GetString(args[1], szDumpFolder) && GetBool(args[2], DumpAboveImageBaseOnly))
	{
		variables["$TE_RESULT"] = Dumper::DumpRegions((HANDLE)hProcess, szDumpFolder.c_str(), DumpAboveImageBaseOnly);
		return true;
	}
	return false;
}

//bool DumpModule(HANDLE hProcess, LPVOID ModuleBase, char* szDumpFileName);
bool OllyLang::DoDumpModule(const string* args, size_t count)
{
rulong hProcess, ModuleBase;
string szDumpFileName;

	if(count == 3 && GetRulong(args[0], hProcess) && GetRulong(args[1], ModuleBase) && GetString(args[2], szDumpFileName))
	{
		variables["$TE_RESULT"] = Dumper::DumpModule((HANDLE)hProcess, (void*)ModuleBase, szDumpFileName.c_str());
		return true;
	}
	return false;
}

//bool PastePEHeader(HANDLE hProcess, LPVOID ImageBase, char* szDebuggedFileName);
bool OllyLang::DoPastePEHeader(const string* args, size_t count)
{
rulong hProcess, ImageBase;
string szDebuggedFileName;

	if(count == 3 && GetRulong(args[0], hProcess) && GetRulong(args[1], ImageBase) && GetString(args[2], szDebuggedFileName))
	{
		variables["$TE_RESULT"] = Dumper::PastePEHeader((HANDLE)hProcess, (void*)ImageBase, szDebuggedFileName.c_str());
		return true;
	}
	return false;
}

//bool ResortFileSections(char* szFileName);
bool OllyLang::DoResortFileSections(const string* args, size_t count)
{
string szFileName;

	if(count == 1 && GetString(args[0], szFileName))
	{
		variables["$TE_RESULT"] = Dumper::ResortFileSections(szFileName.c_str());
		return true;
	}
	return false;
}

//bool ExtractOverlay(char* szFileName, char* szExtractedFileName);
bool OllyLang::DoExtractOverlay(const string* args, size_t count)
{
string szFileName, szExtractedFileName;

	if(count == 2 && GetString(args[0], szFileName) && GetString(args[1], szExtractedFileName))
	{
		variables["$TE_RESULT"] = Dumper::ExtractOverlay(szFileName.c_str(), szExtractedFileName.c_str());
		return true;
	}
	return false;
}

//bool AddOverlay(char* szFileName, char* szOverlayFileName);
bool OllyLang::DoAddOverlay(const string* args, size_t count)
{
string szFileName, szOverlayFileName;

	if(count == 2 && GetString(args[0], szFileName) && GetString(args[1], szOverlayFileName))
	{
		variables["$TE_RESULT"] = Dumper::AddOverlay(szFileName.c_str(), szOverlayFileName.c_str());
		return true;
	}
	return false;
}

//bool CopyOverlay(char* szInFileName, char* szOutFileName);
bool OllyLang::DoCopyOverlay(const string* args, size_t count)
{
string szInFileName, szOutFileName;

	if(count == 2 && GetString(args[0], szInFileName) && GetString(args[1], szOutFileName))
	{
		variables["$TE_RESULT"] = Dumper::CopyOverlay(szInFileName.c_str(), szOutFileName.c_str());
		return true;
	}
	return false;
}

//bool RemoveOverlay(char* szFileName);
bool OllyLang::DoRemoveOverlay(const string* args, size_t count)
{
string szFileName;

	if(count == 1 && GetString(args[0], szFileName))
	{
		variables["$TE_RESULT"] = Dumper::RemoveOverlay(szFileName.c_str());
		return true;
	}
	return false;
}

//bool MakeAllSectionsRWE(char* szFileName);
bool OllyLang::DoMakeAllSectionsRWE(const string* args, size_t count)
{
string szFileName;

	if(count == 1 && GetString(args[0], szFileName))
	{
		variables["$TE_RESULT"] = Dumper::MakeAllSectionsRWE(szFileName.c_str());
		return true;
	}
	return false;
}

//long AddNewSection(char* szFileName, char* szSectionName, DWORD SectionSize);
bool OllyLang::DoAddNewSection(const string* args, size_t count)
{
string szFileName, szSectionName;
dword SectionSize;

	if(count == 3 && GetString(args[0], szFileName) && GetString(args[1], szSectionName) && GetNum(args[2], SectionSize))
	{
		variables["$TE_RESULT"] = Dumper::AddNewSection(szFileName.c_str(), szSectionName.c_str(), SectionSize);
		return true;
	}
	return false;
}

//bool ResizeLastSection(char* szFileName, DWORD NumberOfExpandBytes, bool AlignResizeData);
bool OllyLang::DoResizeLastSection(const string* args, size_t count)
{
string szFileName;
dword NumberOfExpandBytes;
bool AlignResizeData;

	if(count == 3 && GetString(args[0], szFileName) && GetNum(args[1], NumberOfExpandBytes) && GetBool(args[2], AlignResizeData))
	{
		variables["$TE_RESULT"] = Dumper::ResizeLastSection(szFileName.c_str(), NumberOfExpandBytes, AlignResizeData);
		return true;
	}
	return false;
}

//long long GetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData);
bool OllyLang::DoGetPE32Data(const string* args, size_t count)
{
string szFileName;
dword WhichSection;
rulong WhichData;

	if(count == 3 && GetString(args[0], szFileName) && GetNum(args[1], WhichSection) && GetRulong(args[2], WhichData))
	{
		variables["$TE_RESULT"] = Dumper::GetPE32Data(szFileName.c_str(), WhichSection, (ePE32Data)WhichData);
		return true;
	}
	return false;
}

//bool SetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue);
bool OllyLang::DoSetPE32Data(const string* args, size_t count)
{
string szFileName;
dword WhichSection;
rulong WhichData;
rulong NewDataValue;

	if(count == 4 && GetString(args[0], szFileName) && GetNum(args[1], WhichSection) && GetRulong(args[2], WhichData) && GetRulong(args[3], NewDataValue))
	{
		variables["$TE_RESULT"] = Dumper::SetPE32Data(szFileName.c_str(), WhichSection, (ePE32Data)WhichData, NewDataValue);
		return true;
	}
	return false;
}

//long GetPE32SectionNumberFromVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert);
bool OllyLang::DoGetPE32SectionNumberFromVA(const string* args, size_t count)
{
rulong FileMapVA, AddressToConvert;

	if(count == 2 && GetRulong(args[0], FileMapVA) && GetRulong(args[1], AddressToConvert))
	{
		variables["$TE_RESULT"] = Dumper::GetPE32SectionNumberFromVA(FileMapVA, AddressToConvert);
		return true;
	}
	return false;
}

//long long ConvertVAtoFileOffset(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType);
bool OllyLang::DoConvertVAtoFileOffset(const string* args, size_t count)
{
rulong FileMapVA, AddressToConvert;
bool ReturnType;

	if(count == 3 && GetRulong(args[0], FileMapVA) && GetRulong(args[1], AddressToConvert) && GetBool(args[2], ReturnType))
	{
		variables["$TE_RESULT"] = Dumper::ConvertVAtoFileOffset(FileMapVA, AddressToConvert, ReturnType);
		return true;
	}
	return false;
}

//long long ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType);
bool OllyLang::DoConvertFileOffsetToVA(const string* args, size_t count)
{
rulong FileMapVA, AddressToConvert;
bool ReturnType;

	if(count == 3 && GetRulong(args[0], FileMapVA) && GetRulong(args[1], AddressToConvert) && GetBool(args[2], ReturnType))
	{
		variables["$TE_RESULT"] = Dumper::ConvertFileOffsetToVA(FileMapVA, AddressToConvert, ReturnType);
		return true;
	}
	return false;
}

//bool IsFileDLL(char* szFileName, ULONG_PTR FileMapVA);
bool OllyLang::DoIsFileDLL(const string* args, size_t count)
{
const char* szFileName;
rulong FileMapVA;

	if(count == 2 && GetRulong(args[1], FileMapVA))
	{
		string sFileName;
		rulong vFileName;

		if(GetString(args[0], sFileName))
			szFileName = sFileName.c_str();
		else if(GetRulong(args[0], vFileName) && vFileName == 0)
			szFileName = NULL;
		else
			return false;

		variables["$TE_RESULT"] = Realigner::IsFileDLL(szFileName, FileMapVA);
		return true;
	}
	return false;
}

//DWORD RealignPE(ULONG_PTR FileMapVA, DWORD FileSize, DWORD RealingMode);
bool OllyLang::DoRealignPE(const string* args, size_t count)
{
rulong FileMapVA;
rulong FileSize;

	if(count == 3 && GetRulong(args[0], FileMapVA) && GetRulong(args[1], FileSize))
	{
		variables["$TE_RESULT"] = Realigner::RealignPE(FileMapVA, FileSize, NULL);
		return true;
	}
	return false;
}

//void RelocaterCleanup();
bool OllyLang::DoRelocaterCleanup(const string* args, size_t count)
{
	if(count == 0)
	{
		Relocater::Cleanup();
		return true;
	}
	return false;
}

//void RelocaterInit(DWORD MemorySize, ULONG_PTR OldImageBase, ULONG_PTR NewImageBase);
bool OllyLang::DoRelocaterInit(const string* args, size_t count)
{
dword MemorySize;
rulong OldImageBase, NewImageBase;

	if(count == 3 && GetNum(args[0], MemorySize) && GetRulong(args[1], OldImageBase) && GetRulong(args[2], NewImageBase))
	{
		Relocater::Init(MemorySize, OldImageBase, NewImageBase);
		return true;
	}
	return false;
}

//void RelocaterAddNewRelocation(HANDLE hProcess, ULONG_PTR RelocateAddress, DWORD RelocateState);
bool OllyLang::DoRelocaterAddNewRelocation(const string* args, size_t count)
{
rulong hProcess;
rulong RelocateAddress;
dword RelocateState;

	if(count == 3 && GetRulong(args[0], hProcess) && GetRulong(args[1], RelocateAddress) && GetNum(args[2], RelocateState))
	{
		Relocater::AddNewRelocation((HANDLE)hProcess, RelocateAddress, RelocateState);
		return true;
	}
	return false;
}

//long RelocaterEstimatedSize();
bool OllyLang::DoRelocaterEstimatedSize(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Relocater::EstimatedSize();
		return true;
	}
	return false;
}

//bool RelocaterExportRelocation(ULONG_PTR StorePlace, DWORD StorePlaceRVA, ULONG_PTR FileMapVA);
bool OllyLang::DoRelocaterExportRelocation(const string* args, size_t count)
{
rulong StorePlace;
dword StorePlaceRVA;
rulong FileMapVA;

	if(count == 3 && GetRulong(args[0], StorePlace) && GetNum(args[1], StorePlaceRVA) && GetRulong(args[2], FileMapVA))
	{
		variables["$TE_RESULT"] = Relocater::ExportRelocation(StorePlace, StorePlaceRVA, FileMapVA);
		return true;
	}
	return false;
}

//bool RelocaterExportRelocationEx(char* szFileName, char* szSectionName);
bool OllyLang::DoRelocaterExportRelocationEx(const string* args, size_t count)
{
string szFileName, szSectionName;

	if(count == 2 && GetString(args[0], szFileName) && GetString(args[1], szSectionName))
	{
		variables["$TE_RESULT"] = Relocater::ExportRelocationEx(szFileName.c_str(), szSectionName.c_str());
		return true;
	}
	return false;
}

//bool RelocaterMakeSnapshot(HANDLE hProcess, char* szSaveFileName, LPVOID MemoryStart, ULONG_PTR MemorySize);
bool OllyLang::DoRelocaterMakeSnapshot(const string* args, size_t count)
{
rulong hProcess;
string szSaveFileName;
rulong MemoryStart, MemorySize;

	if(count == 4 && GetRulong(args[0], hProcess) && GetString(args[1], szSaveFileName) && GetRulong(args[2], MemoryStart) && GetRulong(args[3], MemorySize))
	{
		variables["$TE_RESULT"] = Relocater::MakeSnapshot((HANDLE)hProcess, szSaveFileName.c_str(), (void*)MemoryStart, MemorySize);
		return true;
	}
	return false;
}

//bool RelocaterCompareTwoSnapshots(HANDLE hProcess, ULONG_PTR LoadedImageBase, ULONG_PTR NtSizeOfImage, char* szDumpFile1, char* szDumpFile2, ULONG_PTR MemStart);
bool OllyLang::DoRelocaterCompareTwoSnapshots(const string* args, size_t count)
{
rulong hProcess, LoadedImageBase, NtSizeOfImage;
string szDumpFile1, szDumpFile2;
rulong MemStart;

	if(count == 6 && GetRulong(args[0], hProcess) && GetRulong(args[1], LoadedImageBase) && GetRulong(args[2], NtSizeOfImage) && GetString(args[3], szDumpFile1) && GetString(args[4], szDumpFile2) && GetRulong(args[5], MemStart))
	{
		variables["$TE_RESULT"] = Relocater::CompareTwoSnapshots((HANDLE)hProcess, LoadedImageBase, NtSizeOfImage, szDumpFile1.c_str(), szDumpFile2.c_str(), MemStart);
		return true;
	}
	return false;
}

//bool RelocaterChangeFileBase(char* szFileName, ULONG_PTR NewImageBase);
bool OllyLang::DoRelocaterChangeFileBase(const string* args, size_t count)
{
string szFileName;
rulong NewImageBase;

	if(count == 2 && GetString(args[0], szFileName) && GetRulong(args[1], NewImageBase))
	{
		variables["$TE_RESULT"] = Relocater::ChangeFileBase(szFileName.c_str(), NewImageBase);
		return true;
	}
	return false;
}

//bool ThreaderPauseThread(HANDLE hThread);
bool OllyLang::DoThreaderPauseThread(const string* args, size_t count)
{
rulong hThread;

	if(count == 1 && GetRulong(args[0], hThread))
	{
		variables["$TE_RESULT"] = Threader::PauseThread((HANDLE)hThread);
		return true;
	}
	return false;
}

//bool ThreaderResumeThread(HANDLE hThread);
bool OllyLang::DoThreaderResumeThread(const string* args, size_t count)
{
rulong hThread;

	if(count == 1 && GetRulong(args[0], hThread))
	{
		variables["$TE_RESULT"] = Threader::ResumeThread((HANDLE)hThread);
		return true;
	}
	return false;
}

//bool ThreaderTerminateThread(HANDLE hThread, DWORD ThreadExitCode);
bool OllyLang::DoThreaderTerminateThread(const string* args, size_t count)
{
rulong hThread;
dword ThreadExitCode;

	if(count == 2 && GetRulong(args[0], hThread) && GetNum(args[1], ThreadExitCode))
	{
		variables["$TE_RESULT"] = Threader::TerminateThread((HANDLE)hThread, ThreadExitCode);
		return true;
	}
	return false;
}

//bool ThreaderPauseAllThreads(bool LeaveMainRunning);
bool OllyLang::DoThreaderPauseAllThreads(const string* args, size_t count)
{
bool LeaveMainRunning;

	if(count == 1 && GetBool(args[0], LeaveMainRunning))
	{
		variables["$TE_RESULT"] = Threader::PauseAllThreads(LeaveMainRunning);
		return true;
	}
	return false;
}

//bool ThreaderResumeAllThreads(bool LeaveMainPaused);
bool OllyLang::DoThreaderResumeAllThreads(const string* args, size_t count)
{
bool LeaveMainRunning;

	if(count == 1 && GetBool(args[0], LeaveMainRunning))
	{
		variables["$TE_RESULT"] = Threader::ResumeAllThreads(LeaveMainRunning);
		return true;
	}
	return false;
}

//long long GetDebuggedDLLBaseAddress();
bool OllyLang::DoGetDebuggedDLLBaseAddress(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Debugger::GetDebuggedDLLBaseAddress();
		return true;
	}
	return false;
}

//long long GetDebuggedFileBaseAddress();
bool OllyLang::DoGetDebuggedFileBaseAddress(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Debugger::GetDebuggedFileBaseAddress();
		return true;
	}
	return false;
}

//long long GetJumpDestination(HANDLE hProcess, ULONG_PTR InstructionAddress);
bool OllyLang::DoGetJumpDestination(const string* args, size_t count)
{
rulong hProcess;
rulong InstructionAddress;

	if(count == 2 && GetRulong(args[0], hProcess) && GetRulong(args[1], InstructionAddress))
	{
		variables["$TE_RESULT"] = Debugger::GetJumpDestination((HANDLE)hProcess, InstructionAddress);
		return true;
	}
	return false;
}

//bool IsJumpGoingToExecute();
bool OllyLang::DoIsJumpGoingToExecute(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Debugger::IsJumpGoingToExecute();
		return true;
	}
	return false;
}

//bool DetachDebuggerEx(DWORD ProcessId);
bool OllyLang::DoDetachDebuggerEx(const string* args, size_t count)
{
dword ProcessId;

	if(count == 1 && GetNum(args[0], ProcessId))
	{
		variables["$TE_RESULT"] = Debugger::DetachDebuggerEx(ProcessId);
		return true;
	}
	return false;
}

//void SetCustomHandler(DWORD ExceptionId, LPVOID CallBack);
bool OllyLang::DoSetCustomHandler(const string* args, size_t count)
{
rulong ExceptionId;
Debugger::fCustomHandlerCallback Callback;

	if(count == 2 && GetRulong(args[0], ExceptionId))
	{
		rulong vCallback;

		eCustomException ExId = (eCustomException)ExceptionId;

		if(script.is_label(args[1]))
		{
			CustomHandlerLabels[ExId] = script.labels[args[1]];
			Callback = CustomHandlerCallbacks[ExId];
		}
		else if(GetRulong(args[1], vCallback) && vCallback == 0)
		{
			Callback = NULL;
		}
		else return false;

		Debugger::SetCustomHandler(ExId, Callback);
		return true;
	}
	return false;
}

//void* GetPEBLocation(HANDLE hProcess);
bool OllyLang::DoGetPEBLocation(const string* args, size_t count)
{
rulong hProcess;

	if(count == 1 && GetRulong(args[0], hProcess))
	{
		variables["$TE_RESULT"] = (rulong)Hider::GetPEBLocation((HANDLE)hProcess);
		return true;
	}
	return false;
}

//void ImporterInit(DWORD MemorySize, ULONG_PTR ImageBase);
bool OllyLang::DoImporterInit(const string* args, size_t count)
{
dword MemorySize;
rulong ImageBase;

	if(count == 2 && GetNum(args[0], MemorySize) && GetRulong(args[1], ImageBase))
	{
		Importer::Init(MemorySize, ImageBase);
		return true;
	}
	return false;
}

//void ImporterCleanup();
bool OllyLang::DoImporterCleanup(const string* args, size_t count)
{
	if(count == 0)
	{
		Importer::Cleanup();
		return true;
	}
	return false;
}

//void ImporterSetImageBase(ULONG_PTR ImageBase);
bool OllyLang::DoImporterSetImageBase(const string* args, size_t count)
{
rulong ImageBase;

	if(count == 1 && GetRulong(args[0], ImageBase))
	{
		Importer::SetImageBase(ImageBase);
		return true;
	}
	return false;
}

//void ImporterAddNewDll(char* szDLLName, ULONG_PTR FirstThunk);
bool OllyLang::DoImporterAddNewDll(const string* args, size_t count)
{
string szDLLName;
rulong FirstThunk;

	if(count == 2 && GetString(args[0], szDLLName) && GetRulong(args[1], FirstThunk))
	{
		Importer::AddNewDll(szDLLName.c_str(), FirstThunk);
		return true;
	}
	return false;
}

//void ImporterAddNewAPI(char* szAPIName, ULONG_PTR ThunkValue);
bool OllyLang::DoImporterAddNewAPI(const string* args, size_t count)
{
string szAPIName;
rulong FirstThunk;

	if(count == 2 && GetString(args[0], szAPIName) && GetRulong(args[1], FirstThunk))
	{
		Importer::AddNewAPI(szAPIName.c_str(), FirstThunk);
		return true;
	}
	return false;
}

//void ImporterAddNewOrdinalAPI(ULONG_PTR OrdinalNumber, ULONG_PTR ThunkValue);
bool OllyLang::DoImporterAddNewOrdinalAPI(const string* args, size_t count)
{
rulong OrdinalNumber, ThunkValue;

	if(count == 2 && GetRulong(args[0], OrdinalNumber) && GetRulong(args[1], ThunkValue))
	{
		Importer::AddNewOrdinalAPI(OrdinalNumber, ThunkValue);
		return true;
	}
	return false;
}

//long ImporterGetAddedDllCount();
bool OllyLang::DoImporterGetAddedDllCount(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Importer::GetAddedDllCount();
		return true;
	}
	return false;
}

//long ImporterGetAddedAPICount();
bool OllyLang::DoImporterGetAddedAPICount(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Importer::GetAddedAPICount();
		return true;
	}
	return false;
}

//void ImporterMoveIAT();
bool OllyLang::DoImporterMoveIAT(const string* args, size_t count)
{
	if(count == 0)
	{
		Importer::MoveIAT();
		return true;
	}
	return false;
}
//bool ImporterRelocateWriteLocation(ULONG_PTR AddValue);
bool OllyLang::DoImporterRelocateWriteLocation(const string* args, size_t count)
{
rulong AddValue;

	if(count == 1 && GetRulong(args[0], AddValue))
	{
		variables["$TE_RESULT"] = Importer::RelocateWriteLocation(AddValue);
		return true;
	}
	return false;
}

//bool ImporterExportIAT(ULONG_PTR StorePlace, ULONG_PTR FileMapVA);
bool OllyLang::DoImporterExportIAT(const string* args, size_t count)
{
rulong StorePlace, FileMapVA;

	if(count == 2 && GetRulong(args[0], StorePlace) && GetRulong(args[1], FileMapVA))
	{
		variables["$TE_RESULT"] = Importer::ExportIAT(StorePlace, FileMapVA);
		return true;
	}
	return false;
}

//long ImporterEstimatedSize();
bool OllyLang::DoImporterEstimatedSize(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Importer::EstimatedSize();
		return true;
	}
	return false;
}

//bool ImporterExportIATEx(char* szExportFileName, char* szSectionName);
bool OllyLang::DoImporterExportIATEx(const string* args, size_t count)
{
string szExportFileName, szSectionName;

	if(count == 2 && GetString(args[0], szExportFileName) && GetString(args[1], szSectionName))
	{
		variables["$TE_RESULT"] = Importer::ExportIATEx(szExportFileName.c_str(), szSectionName.c_str());
		return true;
	}
	return false;
}

//long long ImporterGetNearestAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
bool OllyLang::DoImporterGetNearestAPIAddress(const string* args, size_t count)
{
rulong hProcess, APIAddress;

	if(count == 2 && GetRulong(args[0], hProcess) && GetRulong(args[1], APIAddress))
	{
		variables["$TE_RESULT"] = Importer::GetNearestAPIAddress((HANDLE)hProcess, APIAddress);
		return true;
	}
	return false;
}

//void ImporterAutoSearchIAT(HANDLE hProcess, char* szFileName, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, LPVOID pIATStart, LPVOID pIATSize);
bool OllyLang::DoImporterAutoSearchIAT(const string* args, size_t count)
{
rulong hProcess;
string szFileName;
rulong ImageBase, SearchStart;
dword SearchSize;
rulong temp_pIATStart, temp_pIATSize;

	if(count == 7 && GetRulong(args[0], hProcess) && GetString(args[1], szFileName) && GetRulong(args[2], ImageBase) && GetRulong(args[3], SearchStart) && GetNum(args[4], SearchSize))
	{
		Importer::AutoSearchIAT((HANDLE)hProcess, szFileName.c_str(), ImageBase, SearchStart, SearchSize, &temp_pIATStart, &temp_pIATSize);
		return (SetRulong(args[5], temp_pIATStart) && SetRulong(args[6], temp_pIATSize));
	}
	return false;
}

//void ImporterAutoSearchIATEx(HANDLE hProcess, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, LPVOID pIATStart, LPVOID pIATSize);
bool OllyLang::DoImporterAutoSearchIATEx(const string* args, size_t count)
{
rulong hProcess;
rulong ImageBase, SearchStart;
dword SearchSize;
rulong temp_pIATStart = 0, temp_pIATSize = 0;

	if(count == 6 && GetRulong(args[0], hProcess) && GetRulong(args[1], ImageBase) && GetRulong(args[2], SearchStart) && GetNum(args[3], SearchSize))
	{
		Importer::AutoSearchIATEx((HANDLE)hProcess, ImageBase, SearchStart, SearchSize, &temp_pIATStart, &temp_pIATSize);
		return (SetRulong(args[4], temp_pIATStart) && SetRulong(args[5], temp_pIATSize));
	}
	return false;
}

//long ImporterAutoFixIATEx(HANDLE hProcess, char* szDumpedFile, char* szSectionName, bool DumpRunningProcess, bool RealignFile, ULONG_PTR EntryPointAddress, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, DWORD SearchStep, bool TryAutoFix, bool FixEliminations, LPVOID UnknownPointerFixCallback);
bool OllyLang::DoImporterAutoFixIATEx(const string* args, size_t count)
{
rulong hProcess;
string szDumpedFile, szSectionName;
bool DumpRunningProcess, RealignFile;
rulong EntryPointAddress, ImageBase, SearchStart;
dword SearchSize, SearchStep;
bool TryAutoFix, FixEliminations;

	if(count == 13 && GetRulong(args[0], hProcess) && GetString(args[1], szDumpedFile) && GetString(args[2], szSectionName) && GetBool(args[3], DumpRunningProcess) && GetBool(args[4], RealignFile) && GetRulong(args[5], EntryPointAddress) && GetRulong(args[6], ImageBase) && GetRulong(args[7], SearchStart) && GetNum(args[8], SearchSize) && GetNum(args[9], SearchStep) && GetBool(args[10], TryAutoFix) && GetBool(args[11], FixEliminations))
	{
		if(script.is_label(args[12]))
		{
			Label_AutoFixIATEx =args[12];
			variables["$TE_RESULT"] = Importer::AutoFixIATEx((HANDLE)hProcess, szDumpedFile.c_str(), szSectionName.c_str(), DumpRunningProcess, RealignFile, EntryPointAddress, ImageBase, SearchStart, SearchSize, SearchStep, TryAutoFix, FixEliminations, &Callback_AutoFixIATEx);
			return true;
		}
	}
	return false;
}

//long ImporterAutoFixIAT(HANDLE hProcess, char* szDumpedFile, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, DWORD SearchStep);
bool OllyLang::DoImporterAutoFixIAT(const string* args, size_t count)
{
rulong hProcess;
string szDumpedFile;
rulong ImageBase, SearchStart;
dword SearchSize, SearchStep;

	if(count == 6 && GetRulong(args[0], hProcess) && GetString(args[1], szDumpedFile) && GetRulong(args[2], ImageBase) && GetRulong(args[3], SearchStart) && GetNum(args[4], SearchSize) && GetNum(args[5], SearchStep))
	{
		variables["$TE_RESULT"] = Importer::AutoFixIAT((HANDLE)hProcess, szDumpedFile.c_str(), ImageBase, SearchStart, SearchSize, SearchStep);
		return true;
	}
	return false;
}

//long long TracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace);
bool OllyLang::DoTracerLevel1(const string* args, size_t count)
{
rulong hProcess;
rulong AddressToTrace;

	if(count == 2 && GetRulong(args[0], hProcess) && GetRulong(args[1], AddressToTrace))
	{
		variables["$TE_RESULT"] = Tracer::Level1((HANDLE)hProcess, AddressToTrace);
		return true;
	}
	return false;
}

//long long HashTracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD InputNumberOfInstructions);
bool OllyLang::DoHashTracerLevel1(const string* args, size_t count)
{
rulong hProcess;
rulong AddressToTrace;
dword InputNumberOfInstructions;

	if(count == 3 && GetRulong(args[0], hProcess) && GetRulong(args[1], AddressToTrace) && GetNum(args[2], InputNumberOfInstructions))
	{
		variables["$TE_RESULT"] = Tracer::HashTracerLevel1((HANDLE)hProcess, AddressToTrace, InputNumberOfInstructions);
		return true;
	}
	return false;
}

//long TracerDetectRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace);
bool OllyLang::DoTracerDetectRedirection(const string* args, size_t count)
{
rulong hProcess;
rulong AddressToTrace;

	if(count == 2 && GetRulong(args[0], hProcess) && GetRulong(args[1], AddressToTrace))
	{
		variables["$TE_RESULT"] = Tracer::DetectRedirection((HANDLE)hProcess, AddressToTrace);
		return true;
	}
	return false;
}

//long long TracerFixKnownRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD RedirectionId);
bool OllyLang::DoTracerFixKnownRedirection(const string* args, size_t count)
{
rulong hProcess;
rulong AddressToTrace;
dword RedirectionId;

	if(count == 3 && GetRulong(args[0], hProcess) && GetRulong(args[1], AddressToTrace) && GetNum(args[2], RedirectionId))
	{
		variables["$TE_RESULT"] = Tracer::FixKnownRedirection((HANDLE)hProcess, AddressToTrace, RedirectionId);
		return true;
	}
	return false;
}

//long TracerFixRedirectionViaImpRecPlugin(HANDLE hProcess, char* szPluginName, ULONG_PTR AddressToTrace);
bool OllyLang::DoTracerFixRedirectionViaImpRecPlugin(const string* args, size_t count)
{
rulong hProcess;
string szPluginName;
rulong AddressToTrace;

	if(count == 3 && GetRulong(args[0], hProcess) && GetString(args[1], szPluginName) && GetRulong(args[2], AddressToTrace))
	{
		variables["$TE_RESULT"] = Tracer::FixRedirectionViaImpRecPlugin((HANDLE)hProcess, szPluginName.c_str(), AddressToTrace);
		return true;
	}
	return false;
}

//void ExporterCleanup();
bool OllyLang::DoExporterCleanup(const string* args, size_t count)
{
	if(count == 0)
	{
		Exporter::Cleanup();
		return true;
	}
	return false;
}

//void ExporterSetImageBase(ULONG_PTR ImageBase);
bool OllyLang::DoExporterSetImageBase(const string* args, size_t count)
{
rulong ImageBase;

	if(count == 1 && GetRulong(args[0], ImageBase))
	{
		Exporter::SetImageBase(ImageBase);
		return true;
	}
	return false;
}

//void ExporterInit(DWORD MemorySize, ULONG_PTR ImageBase, DWORD ExportOrdinalBase, char* szExportModuleName);
bool OllyLang::DoExporterInit(const string* args, size_t count)
{
dword MemorySize;
rulong ImageBase;
dword ExportOrdinalBase;
string szExportModuleName;

	if(count == 4 && GetNum(args[0], MemorySize) && GetRulong(args[1], ImageBase) && GetNum(args[2], ExportOrdinalBase) && GetString(args[3], szExportModuleName))
	{
		Exporter::Init(MemorySize, ImageBase, ExportOrdinalBase, szExportModuleName.c_str());
		return true;
	}
	return false;
}

//bool ExporterAddNewExport(char* szExportName, DWORD ExportRelativeAddress);
bool OllyLang::DoExporterAddNewExport(const string* args, size_t count)
{
string szExportName;
dword ExportRelativeAddress;

	if(count == 2 && GetString(args[0], szExportName) && GetNum(args[1], ExportRelativeAddress))
	{
		variables["$TE_RESULT"] = Exporter::AddNewExport(szExportName.c_str(), ExportRelativeAddress);
		return true;
	}
	return false;
}

//bool ExporterAddNewOrdinalExport(DWORD OrdinalNumber, DWORD ExportRelativeAddress);
bool OllyLang::DoExporterAddNewOrdinalExport(const string* args, size_t count)
{
dword OrdinalNumber, ExportRelativeAddress;

	if(count == 2 && GetNum(args[0], OrdinalNumber) && GetNum(args[1], ExportRelativeAddress))
	{
		variables["$TE_RESULT"] = Exporter::AddNewOrdinalExport(OrdinalNumber, ExportRelativeAddress);
		return true;
	}
	return false;
}

//long ExporterGetAddedExportCount();
bool OllyLang::DoExporterGetAddedExportCount(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Exporter::GetAddedExportCount();
		return true;
	}
	return false;
}

//long ExporterEstimatedSize();
bool OllyLang::DoExporterEstimatedSize(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = Exporter::EstimatedSize();
		return true;
	}
	return false;
}

//bool ExporterBuildExportTable(ULONG_PTR StorePlace, ULONG_PTR FileMapVA);
bool OllyLang::DoExporterBuildExportTable(const string* args, size_t count)
{
rulong StorePlace, FileMapVA;

	if(count == 2 && GetRulong(args[0], StorePlace) && GetRulong(args[1], FileMapVA))
	{
		variables["$TE_RESULT"] = Exporter::BuildExportTable(StorePlace, FileMapVA);
		return true;
	}
	return false;
}

//bool ExporterBuildExportTableEx(char* szExportFileName, char* szSectionName);
bool OllyLang::DoExporterBuildExportTableEx(const string* args, size_t count)
{
string szExportFileName, szSectionName;

	if(count == 2 && GetString(args[0], szExportFileName) && GetString(args[1], szSectionName))
	{
		variables["$TE_RESULT"] = Exporter::BuildExportTableEx(szExportFileName.c_str(), szSectionName.c_str());
		return true;
	}
	return false;
}

//bool LibrarianSetBreakPoint(char* szLibraryName, DWORD bpxType, bool SingleShoot, LPVOID bpxCallBack);
bool OllyLang::DoLibrarianSetBreakPoint(const string* args, size_t count)
{
string szLibraryName;
rulong bpxType;
eLibraryEvent bpxT;
bool SingleShoot;

	if(count == 4 && GetString(args[0], szLibraryName) && GetRulong(args[1], bpxType) && GetBool(args[2], SingleShoot))
	{
		if(script.is_label(args[3]))
		{
			variables["$TE_RESULT"] = 0;

			const Librarian::LIBRARY_ITEM_DATA* Lib = Librarian::GetLibraryInfo(szLibraryName.c_str());
			if(Lib)
			{
				bpxT = (eLibraryEvent)bpxType;
				LibraryBreakpointLabels[bpxT][Lib->szLibraryPath] =args[3];

				variables["$TE_RESULT"] = Librarian::SetBreakPoint(Lib->szLibraryPath, bpxT, SingleShoot, LibraryBreakpointCallbacks[bpxT]);
			}
			return true;
		}
	}
	return false;
}

//bool LibrarianRemoveBreakPoint(char* szLibraryName, DWORD bpxType);
bool OllyLang::DoLibrarianRemoveBreakPoint(const string* args, size_t count)
{
string szLibraryName;
rulong bpxType;
eLibraryEvent bpxT;

	if(count == 2 && GetString(args[0], szLibraryName) && GetRulong(args[1], bpxType))
	{
		variables["$TE_RESULT"] = 0;

		const Librarian::LIBRARY_ITEM_DATA* Lib = Librarian::GetLibraryInfo(szLibraryName.c_str());
		if(Lib)
		{
			bpxT = (eLibraryEvent)bpxType;
			LibraryBreakpointLabels[bpxT].erase(Lib->szLibraryPath);

			variables["$TE_RESULT"] = Librarian::RemoveBreakPoint(Lib->szLibraryPath, bpxT);
		}
		return true;
	}
	return false;
}

//bool TLSRemoveCallback(char* szFileName);
bool OllyLang::DoTLSRemoveCallback(const string* args, size_t count)
{
string szFileName;

	if(count == 1 && GetString(args[0], szFileName))
	{
		variables["$TE_RESULT"] = TLS::RemoveCallback(szFileName.c_str());
		return true;
	}
	return false;
}

//bool TLSRemoveTable(char* szFileName);
bool OllyLang::DoTLSRemoveTable(const string* args, size_t count)
{
string szFileName;

	if(count == 1 && GetString(args[0], szFileName))
	{
		variables["$TE_RESULT"] = TLS::RemoveTable(szFileName.c_str());
		return true;
	}
	return false;
}

//bool TLSBackupData(char* szFileName);
bool OllyLang::DoTLSBackupData(const string* args, size_t count)
{
string szFileName;

	if(count == 1 && GetString(args[0], szFileName))
	{
		variables["$TE_RESULT"] = TLS::BackupData(szFileName.c_str());
		return true;
	}
	return false;
}

//bool TLSRestoreData();
bool OllyLang::DoTLSRestoreData(const string* args, size_t count)
{
	if(count == 0)
	{
		variables["$TE_RESULT"] = TLS::RestoreData();
		return true;
	}
	return false;
}

//bool HandlerIsHandleOpen(DWORD ProcessId, HANDLE hHandle);
bool OllyLang::DoHandlerIsHandleOpen(const string* args, size_t count)
{
dword ProcessId;
rulong hHandle;

	if(count == 2 && GetNum(args[0], ProcessId) && GetRulong(args[1], hHandle))
	{
		variables["$TE_RESULT"] = Handler::IsHandleOpen(ProcessId, (HANDLE)hHandle);
		return true;
	}
	return false;
}

//bool HandlerCloseRemoteHandle(HANDLE hProcess, HANDLE hHandle);
bool OllyLang::DoHandlerCloseRemoteHandle(const string* args, size_t count)
{
rulong hProcess, hHandle;

	if(count == 2 && GetRulong(args[0], hProcess) && GetRulong(args[1], hHandle))
	{
		variables["$TE_RESULT"] = Handler::CloseRemoteHandle((HANDLE)hProcess, (HANDLE)hHandle);
		return true;
	}
	return false;
}

//bool StaticFileLoad(char* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA);
bool OllyLang::DoStaticFileLoad(const string* args, size_t count)
{
string szFileName;
rulong DesiredAccess;
bool SimulateLoad;
HANDLE temp_FileHandle;
dword temp_LoadedSize;
HANDLE temp_FileMap;
rulong temp_FileMapVA;

	if(count == 7 && GetString(args[0], szFileName) && GetRulong(args[1], DesiredAccess) && GetBool(args[2], SimulateLoad))
	{
		variables["$TE_RESULT"] = Static::FileLoad(szFileName.c_str(), (eAccess)DesiredAccess, SimulateLoad, &temp_FileHandle, (DWORD*)&temp_LoadedSize, &temp_FileMap, &temp_FileMapVA);
		return (SetRulong(args[3], (rulong)temp_FileHandle) && SetRulong(args[4], (rulong)temp_LoadedSize) && SetRulong(args[5], (rulong)temp_FileMap) && SetRulong(args[6], temp_FileMapVA));
	}
	return false;
}

//bool StaticFileUnload(char* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA);
bool OllyLang::DoStaticFileUnload(const string* args, size_t count)
{
string szFileName;
bool CommitChanges;
rulong FileHandle;
dword LoadedSize;
rulong FileMap, FileMapVA;

	if(count == 6 && GetString(args[0], szFileName) && GetBool(args[1], CommitChanges) && GetRulong(args[2], FileHandle) && GetNum(args[3], LoadedSize) && GetRulong(args[4], FileMap) && GetRulong(args[5], FileMapVA))
	{
		variables["$TE_RESULT"] = Static::FileUnload(szFileName.c_str(), CommitChanges, (HANDLE)FileHandle, LoadedSize, (HANDLE)FileMap, FileMapVA);
		return true;
	}
	return false;
}
