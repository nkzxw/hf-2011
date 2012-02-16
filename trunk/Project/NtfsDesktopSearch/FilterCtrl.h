// FilterCtrl.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//������˵����пؼ�
#define FILTER_HWND_COUNT 19
class CFilterCtrl
{
public:
    CFilterCtrl();
    virtual ~CFilterCtrl();
public:
    void Go();
    void Show(int width,int hight);
    void Hide();
    void CreateContext(HWND hSizeCheck,HWND hSizeLeftComb,HWND hSizeEdit,HWND hSizeRightComb
        ,HWND hDateCheck,HWND hDateTypeComb,HWND hPreRadio,HWND hPreTime,HWND hPosRadio,HWND hPosTime,HWND hMidRadio,HWND hMidTime1,HWND hMidTime2
        ,HWND hAttrCheck,HWND hAttrNormalCheck,HWND hAttrHiddenCheck,HWND hAttrSysCheck
        ,HWND hBtnFilter,HWND hParentWnd);
    void Initialize();

    void SetSizeUse(BOOL);
    void SetDateUse(BOOL);
    void SetAttrUse(BOOL);

    BOOL RetrieveFilter(void);
protected:
    BOOL IsSat(DWORDLONG *pOutTimeCreate
        ,DWORDLONG *pOutTimeAccess
        ,DWORDLONG *pOutTimeChange
        ,DWORDLONG *pOutSize
        ,DWORD *pOutAttr);
private:
    //SIZE�ؼ�
    HWND m_hSizeCheck,m_hSizeLeftComb,m_hSizeEdit,m_hSizeRightComb;
    //SIZE����
    BOOL m_bUseSize;//��ѡ�ļ���С����
    int m_typeCompare,m_typeUnit;
    DWORDLONG m_llSize;

    //Date�ؼ�
    HWND m_hDateCheck,m_hDateTypeComb
        ,m_hPreRadio,m_hPreTime
        ,m_hPosRadio,m_hPosTime
        ,m_hMidRadio,m_hMidTime1,m_hMidTime2;
    //Date����
    BOOL m_bUseDate;
    int  m_iDateType;//�����ַ�ʽ����
    int  m_typeDate;    
    DWORDLONG m_llDate;  //MFT�洢ʱ��
    DWORDLONG m_llDate2;
   
    //�ؼ�
    HWND m_hAttrCheck
        ,m_hAttrNormalCheck,m_hAttrHiddenCheck,m_hAttrSysCheck;
    //����
    BOOL m_bUseAttr;
    BOOL m_bNormal;
    DWORD m_dwAttr;

    HWND m_hBtnFilter;//��ť
    HWND m_hParentWnd;//�� 

    //��ʱ���
    HWND m_vhWnd[FILTER_HWND_COUNT];//19�����
};