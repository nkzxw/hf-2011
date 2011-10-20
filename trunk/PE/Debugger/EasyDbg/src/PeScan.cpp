// PeScan.cpp : implementation file
//

#include "stdafx.h"
#include "EasyDbg.h"
#include "PeScan.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPeScan dialog

//�������ģ��ʹ��
extern PIMAGE_SECTION_HEADER g_SecHeader=NULL;
extern DWORD g_SecNum=0;

extern DWORD g_dwImportRva=0;

//���ڴ������뵼����ģ��
extern char* g_pFile=NULL;

extern DWORD g_dwExportRva=0;


//�ӵ����̴߳��� �����Գ����ӳ���ַ
extern char* pFile;

CPeScan::CPeScan(CWnd* pParent /*=NULL*/)
	: CDialog(CPeScan::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPeScan)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    memset(m_FilePath,0,sizeof(m_FilePath));
    m_pFile=NULL;
    m_pDos=NULL;
    m_pFileHeader=NULL;
    m_pOption=NULL;
    m_Section=NULL;
}


void CPeScan::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPeScan)
	DDX_Control(pDX, IDC_LIST1, m_SecList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPeScan, CDialog)
	//{{AFX_MSG_MAP(CPeScan)
	ON_BN_CLICKED(BTN_SELECT, OnSelect)
	ON_BN_CLICKED(BTN_IMPORT, OnImport)
	ON_BN_CLICKED(BTN_EXPORT, OnExport)
    ON_WM_DROPFILES()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPeScan message handlers

void CPeScan::OnOK() 
{
	// TODO: Add extra validation here
	
	//CDialog::OnOK();
}

//ѡ���ļ�
void CPeScan::OnSelect() 
{
	// TODO: Add your control notification handler code here
    OPENFILENAME file = {0} ;
    file.lpstrFile = m_FilePath ;
    file.lStructSize = sizeof(OPENFILENAME) ;
    file.nMaxFile = 256 ;
    file.lpstrFilter = "Executables\0*.exe\0All Files\0*.*\0\0" ;
    file.nFilterIndex = 1 ;
    
    if(!::GetOpenFileName(&file))
    {
        //����ȡ����ť���˳�����
        return;
    }
    SetDlgItemText(IDC_FILEPATH,m_FilePath);
    if (!MapPEFile())
    {
        AfxMessageBox("�ļ�ӳ��ʧ��");
        return;
    }

    GetBasicPEInfo();


	
}

BOOL CPeScan::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
 

    m_SecList.InsertColumn(0,TEXT("������"),LVCFMT_LEFT,70);
    m_SecList.InsertColumn(1,TEXT("VirtualAddress"),LVCFMT_LEFT,100);
    m_SecList.InsertColumn(2,TEXT("VirtualSize"),LVCFMT_LEFT,80);
    m_SecList.InsertColumn(3,TEXT("PointerToRawData"),LVCFMT_LEFT,100);
    m_SecList.InsertColumn(4,TEXT("SizeOfRawData"),LVCFMT_LEFT,100);
  
    
    m_SecList.SetExtendedStyle(m_SecList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    //Ĭ����ʾ�����Գ������Ϣ
    if (pFile!=NULL)
    {
     
      m_pFile=pFile;
      g_pFile=pFile;
      GetBasicPEInfo();
      
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//ӳ��PE�ļ�
BOOL CPeScan::MapPEFile()
{
    HANDLE hFile=NULL;
    //���ļ�����ļ����
    hFile=CreateFile(m_FilePath,GENERIC_ALL,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        DWORD dwErrorCode=GetLastError();
        AfxMessageBox("���ļ�ʧ��");
        return FALSE;
    }
    HANDLE hFileMap=NULL;
    //�����ļ�ӳ��
    hFileMap=CreateFileMapping(hFile,NULL,PAGE_READWRITE,0,0,NULL);
    if (hFileMap==NULL)
    {
        AfxMessageBox("�����ڴ�ӳ��ʧ��");
        CloseHandle(hFile);
        return FALSE;
    }
    //ӳ���ļ�
    m_pFile=(char*)MapViewOfFile(hFileMap,FILE_MAP_ALL_ACCESS,0,0,0);
    if (m_pFile==NULL)
    {
        AfxMessageBox("ӳ��ʧ��");
        CloseHandle(hFile);
        CloseHandle(hFileMap);
        return FALSE;
    }
    CloseHandle(hFile);
    CloseHandle(hFileMap);
    g_pFile=m_pFile;

   return TRUE;

}


//���PE���������Ϣ ���ж��Ƿ�����ЧPE
BOOL CPeScan::GetBasicPEInfo()
{

   //��ȡ���ṹ��ָ��
    m_pDos=(PIMAGE_DOS_HEADER)m_pFile;
    PIMAGE_NT_HEADERS pNt=(PIMAGE_NT_HEADERS)(m_pFile+m_pDos->e_lfanew);
    m_pFileHeader=(PIMAGE_FILE_HEADER)(m_pFile+m_pDos->e_lfanew+4);
    m_pOption=(PIMAGE_OPTIONAL_HEADER)((char*)m_pFileHeader+sizeof(IMAGE_FILE_HEADER));
    m_Section=(PIMAGE_SECTION_HEADER)((char*)m_pOption+m_pFileHeader->SizeOfOptionalHeader);

    g_SecHeader=m_Section;

    //���MZ PE ������־
    if (m_pDos->e_magic!=IMAGE_DOS_SIGNATURE || pNt->Signature!=IMAGE_NT_SIGNATURE)
    {
        AfxMessageBox("������Ч��PE�ļ�");
        return FALSE;
    }
    CString szText;
    //����Ŀ
    szText.Format("%04X",m_pFileHeader->NumberOfSections);
    SetDlgItemText(IDC_SECTIONNUM,szText);

    g_SecNum=m_pFileHeader->NumberOfSections;

    //ѡ��ͷ��С
    szText.Format("%04X",m_pFileHeader->SizeOfOptionalHeader);
    SetDlgItemText(IDC_OPTION,szText);
    //OEP
    szText.Format("%08X",m_pOption->AddressOfEntryPoint);
    SetDlgItemText(IDC_ENTRY,szText);
    //ImageBase
    szText.Format("%08X",m_pOption->ImageBase);
    SetDlgItemText(IDC_IMAGE,szText);
    //SizeofImage
    szText.Format("%08X",m_pOption->SizeOfImage);
    SetDlgItemText(IDC_IMAGESIZE,szText);
    //BaseOfCode
    szText.Format("%08X",m_pOption->BaseOfCode);
    SetDlgItemText(IDC_CODEBASE,szText);
    //BaseOfData
    szText.Format("%08X",m_pOption->BaseOfData);
    SetDlgItemText(IDC_DATABASE,szText);
    //SectionAlignment
    szText.Format("%08X",m_pOption->SectionAlignment);
    SetDlgItemText(IDC_SECTIONALIGNMENT,szText);
    //FileAlignment
    szText.Format("%08X",m_pOption->FileAlignment);
    SetDlgItemText(IDC_FILEALIGNMENT,szText);
    //Subsystem
    szText.Format("%04X",m_pOption->Subsystem);
    SetDlgItemText(IDC_SUBSYSTEM,szText);
    //CheckSum

    szText.Format("%08X",m_pOption->CheckSum);
    SetDlgItemText(IDC_CHECKSUM,szText);
    //SizeOfHeaders
    szText.Format("%08X",m_pOption->SizeOfHeaders);
    SetDlgItemText(IDC_SIZEOFHEADER,szText);
    //EXPORT
    szText.Format("%08X",m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    SetDlgItemText(IDC_EXPORTRVA,szText);
    szText.Format("%08X",m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
    SetDlgItemText(IDC_EXPORTSIZE,szText);
    //import
    szText.Format("%08X",m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    SetDlgItemText(IDC_IMPORTRVA,szText);
    szText.Format("%08X",m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
    SetDlgItemText(IDC_IMPORTSIZE,szText);
    
    //��ֵ
    g_dwImportRva=m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    g_dwExportRva=m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    if (m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress==0)
    {
        //���û�е��������䰴ť���
        GetDlgItem(BTN_EXPORT)->EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(BTN_EXPORT)->EnableWindow(TRUE);

    }

    if (m_pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress==0)
    {
        //���û�е�������䰴ť���
        GetDlgItem(BTN_IMPORT)->EnableWindow(FALSE);
    }

    else
    {
        GetDlgItem(BTN_IMPORT)->EnableWindow(TRUE);

    }


    


    m_SecList.DeleteAllItems();
    //������Ϣ
    for (int i=0;i<m_pFileHeader->NumberOfSections;i++)
    {
        

        //������
        szText.Format("%s",m_Section[i].Name);
        m_SecList.InsertItem(i,szText);
        //RVA
        szText.Format("%08X",m_Section[i].VirtualAddress);
        m_SecList.SetItemText(i,1,szText);
        //V��С
        szText.Format("%08X",m_Section[i].Misc.VirtualSize);
        m_SecList.SetItemText(i,2,szText);
        //�ļ�ƫ��
        szText.Format("%08X",m_Section[i].PointerToRawData);
        m_SecList.SetItemText(i,3,szText);
        //R��С
        szText.Format("%08X",m_Section[i].SizeOfRawData);
        m_SecList.SetItemText(i,4,szText);



    }



    return TRUE;
}


//�鿴������
void CPeScan::OnImport() 
{
	// TODO: Add your control notification handler code here
    CImportTable dlg;
    dlg.DoModal();
	
}

//�鿴������
void CPeScan::OnExport() 
{
	// TODO: Add your control notification handler code here

    CExportTable dlg;
    dlg.DoModal();
	
}

void CPeScan::OnDropFiles( HDROP hDropInfo )
{
  
    
    ::DragQueryFile 
        (hDropInfo,0,m_FilePath,sizeof(m_FilePath)); //0��ʾȡ��һ������ק���ļ���
    SetDlgItemText(IDC_FILEPATH,m_FilePath); 
    
    ::DragFinish (hDropInfo); //�ͷ��ڴ�

    if (!MapPEFile())
    {
        AfxMessageBox("�ļ�ӳ��ʧ��");
        return;
    }


    GetBasicPEInfo();

   
      
}


//������ ��ӳ�䳷��
void CPeScan::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
    //�����Խ��̵�ӳ��Ͳ�������
//     if (pFile!=NULL)
//     {
//         UnmapViewOfFile(pFile);
//         pFile=NULL;
//     }
    if (m_pFile!=NULL)
    {
        //����Ǳ����Խ��̵�ӳ���ַ�Ļ��Ͳ�����
        if (m_pFile!=pFile)
        {
            UnmapViewOfFile(m_pFile);
           m_pFile=NULL;
        }

    }
	
	CDialog::OnClose();
}
