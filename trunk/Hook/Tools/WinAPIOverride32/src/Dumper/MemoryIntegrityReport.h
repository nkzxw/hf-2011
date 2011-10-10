#pragma once
#include "dumper.h"

#define HTML_REPORT_WINDOW_WITH    600
#define HTML_REPORT_WINDOW_HEIGHT  400
#define HTML_REPORT_PROCESS_ID                  _T("ReportProcessId")
#define HTML_REPORT_SECTION_ID                  _T("ReportSectionId")
#define HTML_REPORT_SAVE_ID                     _T("SaveId")
#define HTML_REPORT_SAVE_ID_IMG                 _T("SaveIdImg")
#define HTML_REPORT_SECTION_RESTORE_PREFIX_ID   _T("SectionRestore_")
#define HTML_REPORT_RESTORE_PREFIX_ID           _T("Restore_")
#define HTML_REPORT_VIEW_DISASM_PREFIX_ID       _T("Disasm_")
#define HTML_REPORT_VALID_SECTION_CLASS_NAME    _T("ReportValidSection")
#define HTML_REPORT_INVALID_SECTION_CLASS_NAME  _T("ReportInvalidSection")
#define HTML_REPORT_CHANGE_CELL_PREFIX_ID       _T("ChangeCell_")
#define HTML_REPORT_CHANGE_CELL_PREFIX_IDW      L"ChangeCell_"
// empty links : do not use <a href="#"></a> because you're page goes on top
//                      use <a href="javascript:"></a>
#define HTML_REPORT_NB_COLOMNS_FOR_A_CHANGE 5


//////////////////////// structs //////////////////////// 

void DisplayModuleIntegrityChecking(CLinkListSimple* pList);
DWORD WINAPI DisplayModuleIntegrity(PVOID lParam);