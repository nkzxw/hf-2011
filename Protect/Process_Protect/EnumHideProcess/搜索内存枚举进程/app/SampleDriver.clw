; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSampleDriverDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "SampleDriver.h"

ClassCount=4
Class1=CSampleDriverApp
Class2=CSampleDriverDlg
Class3=CAboutDlg

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_SAMPLEDRIVER_DIALOG

[CLS:CSampleDriverApp]
Type=0
HeaderFile=SampleDriver.h
ImplementationFile=SampleDriver.cpp
Filter=N

[CLS:CSampleDriverDlg]
Type=0
HeaderFile=SampleDriverDlg.h
ImplementationFile=SampleDriverDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC

[CLS:CAboutDlg]
Type=0
HeaderFile=SampleDriverDlg.h
ImplementationFile=SampleDriverDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_SAMPLEDRIVER_DIALOG]
Type=1
Class=CSampleDriverDlg
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_LIST,SysListView32,1342275597
Control4=IDC_STATUS,static,1342308352

