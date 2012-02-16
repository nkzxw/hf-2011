// ExtArray.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once


//��չ�����ݿ⣬������չ���������ٹʸÿ������ɾ
//������չ�����ֵ���������

#define ICON_INDEX_UNINITIALIZED    -3
#define ICON_INDEX_REALTIMECALL     -2
#define ICON_INDEX_NOICON           -1

class CExtArray
{

    static const DWORD DELT=64;
public:
    static DWORD s_dwOmitExt;  //����ʵʱ���õ�ͼ������ 
    CExtArray();
    ~CExtArray();
    DWORD size(){return m_size;}

    void InitRealTimeCallExt();
    
    //���������е�˳��
    //����-1��ʾû���ҵ�
    int  find(PWCHAR pszExtName)const;

    //���������е�˳��
    //����-1��ʾû���ҵ�
    int find(PWCHAR pszExtName,int ExtNameLen)const; 

    //�����ļ���ʱ��Ҫ������չ��
    //�����ظ���չ������Ӧ��ID
    int insert(PWCHAR pszExtName,int ExtNameLen); 

    int GetExtIndexSequence(int idExtName)const;

    int Compare(int idExt1,int idExt2)const;//�Ƚ�������չ����С

    PWCHAR GetExtName(int idExtName)const;
    //��ü�����չ����ICON����
    //������>-2 ��ֱ��ʹ��
    //==-2 ��Ҫ�û�����SHGetFileInfo
    //==-3 ������û�г�ʼ����id
    //�û���Ҫ����SetIconIndex����ʼ����չ������
    int GetIconIndex(int idExtName)const;

    //������չ������
    int SetIconIndex(int idExtName,PWCHAR pFilePath,int filePathLen,PWCHAR pFileName,int fileLen);

    //��չ�����ݿ���������ȼ���
    //������FALSE ���ļ������ݿ��е���չ��������Ч
    //��ʱ��Ҫ���������ļ������ݿ�
    BOOL LoadFromFile(PWCHAR pExtFileName);

    //��д�ļ������ݿ�֮��д����չ�����ݿ�
    BOOL WriteToFile(PWCHAR pExtFileName);
    
    
private:
    BOOL  m_bInitReal;
    DWORD m_dwMax;
    DWORD m_size;

    int *m_piIcon;  //��չ����Ӧ��ϵͳͼ��� �����չ��
                    //-3��ʾδ��ʼ�� -2��ʾ��Ҫʵʱ���� -1��ʾ��ͼ�� >=0��ֱ�ӷ���
                    //һ��Ϊ-2�� exe ico cur 

    PWCHAR *m_vpExtName;//��չ��ָ������

    typedef struct IndexNode
    {
        PWCHAR  pExtName;       //��չ���洢������ָ��,�˷��˿ռ䣬�Ա���������
        int     idExtName;      //��չ����id
        void exchange(IndexNode& node)//�����������
        {
            DWORDLONG *p=(DWORDLONG *)&node,
                *pThis=(DWORDLONG *)this;
            DWORDLONG temp=*p; 
            *p=*pThis; 
            *pThis=temp;
        }
    }INDEXNODE,*PINDEXNODE;
    INDEXNODE *m_vpIndexExtName;//��չ������ָ������
};

