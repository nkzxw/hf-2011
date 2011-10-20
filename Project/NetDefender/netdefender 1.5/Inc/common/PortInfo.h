#pragma once
class CPortInfo
{
public:
	CPortInfo();
	~CPortInfo();
	CMapStringToString   mapPortInfo;
	BOOL ReturnPortInfo(CString strPort,CString &strReturn);
	void init();
};