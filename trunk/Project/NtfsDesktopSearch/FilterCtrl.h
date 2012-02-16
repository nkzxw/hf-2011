// FilterCtrl.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//管理过滤的所有控件
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
    //SIZE控件
    HWND m_hSizeCheck,m_hSizeLeftComb,m_hSizeEdit,m_hSizeRightComb;
    //SIZE数据
    BOOL m_bUseSize;//勾选文件大小设置
    int m_typeCompare,m_typeUnit;
    DWORDLONG m_llSize;

    //Date控件
    HWND m_hDateCheck,m_hDateTypeComb
        ,m_hPreRadio,m_hPreTime
        ,m_hPosRadio,m_hPosTime
        ,m_hMidRadio,m_hMidTime1,m_hMidTime2;
    //Date数据
    BOOL m_bUseDate;
    int  m_iDateType;//按哪种方式搜索
    int  m_typeDate;    
    DWORDLONG m_llDate;  //MFT存储时间
    DWORDLONG m_llDate2;
   
    //控件
    HWND m_hAttrCheck
        ,m_hAttrNormalCheck,m_hAttrHiddenCheck,m_hAttrSysCheck;
    //数据
    BOOL m_bUseAttr;
    BOOL m_bNormal;
    DWORD m_dwAttr;

    HWND m_hBtnFilter;//按钮
    HWND m_hParentWnd;//父 

    //临时句柄
    HWND m_vhWnd[FILTER_HWND_COUNT];//19个句柄
};