// page1.cpp : implementation file
//

#include "stdafx.h"
#include "RegSafe.h"
#include "page1.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// page1 dialog


page1::page1(CWnd* pParent /*=NULL*/)
	: CDialog(page1::IDD, pParent)
{

}


void page1::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(page1)
	DDX_Control(pDX, IDC_P1CHECK2, m_p1check2);
	DDX_Control(pDX, IDC_P1CHECK1, m_p1check1);
DDX_Control(pDX, IDC_P1CHECK3, m_p1check3);
DDX_Control(pDX, IDC_P1CHECK4, m_p1check4);
DDX_Control(pDX, IDC_P1CHECK5, m_p1check5);
DDX_Control(pDX, IDC_P1CHECK6, m_p1check6);
DDX_Control(pDX, IDC_P1CHECK7, m_p1check7);
DDX_Control(pDX, IDC_P1CHECK8, m_p1check8);
DDX_Control(pDX, IDC_P1CHECK9, m_p1check9);
DDX_Control(pDX, IDC_P1CHECK10, m_p1check10);
DDX_Control(pDX, IDC_P1CHECK11, m_p1check11);
DDX_Control(pDX, IDC_P1CHECK12, m_p1check12);
DDX_Control(pDX, IDC_P1CHECK13, m_p1check13);
DDX_Control(pDX, IDC_P1CHECK14, m_p1check14);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(page1, CDialog)
	//{{AFX_MSG_MAP(page1)
	ON_BN_CLICKED(IDC_P1CHECK1, OnP1Check1)
	ON_BN_CLICKED(IDC_P1CHECK2, OnP1check2)
ON_BN_CLICKED(IDC_P1CHECK3, OnP1check3)
ON_BN_CLICKED(IDC_P1CHECK4, OnP1check4)
ON_BN_CLICKED(IDC_P1CHECK5, OnP1check5)
ON_BN_CLICKED(IDC_P1CHECK6, OnP1check6)
ON_BN_CLICKED(IDC_P1CHECK7, OnP1check7)
ON_BN_CLICKED(IDC_P1CHECK8, OnP1check8)
ON_BN_CLICKED(IDC_P1CHECK9, OnP1check9)
ON_BN_CLICKED(IDC_P1CHECK10, OnP1check10)
ON_BN_CLICKED(IDC_P1CHECK11, OnP1check11)
ON_BN_CLICKED(IDC_P1CHECK12, OnP1check12)
ON_BN_CLICKED(IDC_P1CHECK13, OnP1check13)
ON_BN_CLICKED(IDC_P1CHECK14, OnP1check14)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// page1 message handlers

BOOL page1::OnInitDialog() 
{
	CDialog::OnInitDialog();
	

   setcheck();
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void page1::setcheck()
{
i_p1check1=m_page1.DWORD_Getdate(IDC_P1CHECK1);
	m_p1check1.SetCheck(i_p1check1);
i_p1check2=m_page1.DWORD_Getdate(IDC_P1CHECK2);
    m_p1check2.SetCheck(i_p1check2);
i_p1check3=m_page1.DWORD_Getdate(IDC_P1CHECK3);
    m_p1check3.SetCheck(i_p1check3);
i_p1check4=m_page1.DWORD_Getdate(IDC_P1CHECK4);
    m_p1check4.SetCheck(i_p1check4);
i_p1check5=m_page1.DWORD_Getdate(IDC_P1CHECK5);
    m_p1check5.SetCheck(i_p1check5);
i_p1check6=m_page1.DWORD_Getdate(IDC_P1CHECK6);
    m_p1check6.SetCheck(i_p1check6);
i_p1check7=m_page1.DWORD_Getdate(IDC_P1CHECK7);
    m_p1check7.SetCheck(i_p1check7);
i_p1check8=m_page1.DWORD_Getdate(IDC_P1CHECK8);
    m_p1check8.SetCheck(i_p1check8);
i_p1check9=m_page1.DWORD_Getdate(IDC_P1CHECK9);
    m_p1check9.SetCheck(i_p1check9);
i_p1check10=m_page1.DWORD_Getdate(IDC_P1CHECK10);
    m_p1check10.SetCheck(i_p1check10);
i_p1check11=m_page1.DWORD_Getdate(IDC_P1CHECK11);
    m_p1check11.SetCheck(i_p1check11);
i_p1check12=m_page1.DWORD_Getdate(IDC_P1CHECK12);
    m_p1check12.SetCheck(i_p1check12);
i_p1check13=m_page1.DWORD_Getdate(IDC_P1CHECK13);
    m_p1check13.SetCheck(i_p1check13);
i_p1check14=m_page1.DWORD_Getdate(IDC_P1CHECK14);
    m_p1check14.SetCheck(i_p1check14);
}
void page1::OnP1Check1() 
{
    
	if(i_p1check1==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK1,1);
	    i_p1check1=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK1,0);
	   i_p1check1=0;
   }
}

void page1::OnP1check2() 
{
if(i_p1check2==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK2,1);
	    i_p1check2=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK2,0);
	   i_p1check2=0;
   }	
}
void page1::OnP1check3() 
{
if(i_p1check3==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK3,1);
	    i_p1check3=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK3,0);
	   i_p1check3=0;
   }	
}
void page1::OnP1check4() 
{
if(i_p1check4==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK4,1);
	    i_p1check4=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK4,0);
	   i_p1check4=0;
   }	
}
void page1::OnP1check5() 
{
if(i_p1check5==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK5,1);
	    i_p1check5=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK5,0);
	   i_p1check5=0;
   }	
}
void page1::OnP1check6() 
{
if(i_p1check6==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK6,1);
	    i_p1check6=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK6,0);
	   i_p1check6=0;
   }	
}
void page1::OnP1check7() 
{
if(i_p1check7==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK7,1);
	    i_p1check7=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK7,0);
	   i_p1check7=0;
   }	
}
void page1::OnP1check8() 
{
if(i_p1check8==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK8,1);
	    i_p1check8=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK8,0);
	   i_p1check8=0;
   }	
}
void page1::OnP1check9() 
{
if(i_p1check9==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK9,1);
	    i_p1check9=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK9,0);
	   i_p1check9=0;
   }	
}
void page1::OnP1check10() 
{
if(i_p1check10==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK10,1);
	    i_p1check10=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK10,0);
	   i_p1check10=0;
   }	
}
void page1::OnP1check11() 
{
if(i_p1check11==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK11,1);
	    i_p1check11=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK11,0);
	   i_p1check11=0;
   }	
}
void page1::OnP1check12() 
{
if(i_p1check12==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK12,1);
	    i_p1check12=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK12,0);
	   i_p1check12=0;
   }	
}
void page1::OnP1check13() 
{
if(i_p1check13==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK13,1);
	    i_p1check13=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK13,0);
	   i_p1check13=0;
   }	
}
void page1::OnP1check14() 
{
if(i_p1check14==0)
	{
		m_page1.DWORD_SetKey(IDC_P1CHECK14,1);
	    i_p1check14=1;
	}
   else
   {
	  m_page1.DWORD_SetKey(IDC_P1CHECK14,0);
	   i_p1check14=0;
   }	
}
