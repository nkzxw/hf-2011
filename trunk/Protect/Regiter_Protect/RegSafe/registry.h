#if !defined(REG_H)
#define REG_H

//reg.h head file
class CRegistry
{
public:
	CRegistry();
	CRegistry( HKEY, const char * );
	~CRegistry();

	BOOL Open( HKEY, const char * );
	BOOL Close( void );
	BOOL IsOpen( void );
/*HKEY hKey;
char content_S[256];
int content_D[1];			//����ѯע����ֵ������
DWORD dwType=REG_SZ;		//�����ȡ��������
DWORD dwLength=256;
struct HKEY__*RootKey;		//ע�����������
TCHAR *SubKey;				//����ע�����ĵ�ַ
TCHAR *KeyName;				//�������������
TCHAR *ValueName;			//������ֵ������
LPBYTE SetContent_S;		//�ַ�������
int SetContent_D[256];		//DWORD����
BYTE SetContent_B[256];		//����������

int ShowContent_S(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReValueName);
int ShowContent_D(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReValueName);
int SetValue_S(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReValueName,LPBYTE ReSetContent_S);
int SetValue_D(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReValueName,int ReSetContent_D[256]);
int SetValue_B(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReValueName,BYTE ReSetContent_B[256]);
int DeleteKey(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReKeyName);
int DeleteValue(struct HKEY__*ReRootKey,TCHAR *ReSubKey,TCHAR *ReValueName);
*/
	BOOL ReadDWORD( const char *, DWORD *, DWORD *pdwLastError = NULL );
	BOOL ReadString( const char *, LPVOID, int, DWORD *pdwLastError = NULL );
	char **ReadMultipleStrings( const char *, DWORD *pdwLastError = NULL );
	static void DeleteList( char ** );

	BOOL WriteDWORD( const char *, DWORD, DWORD *pdwLastError = NULL );
	BOOL WriteString( const char *, LPVOID, DWORD *pdwLastError = NULL );
	BOOL Write( const char *, LPVOID, DWORD, int );

	static BOOL CreateKey( HKEY, const char *, const char * );
	static BOOL DeleteKey( HKEY, const char * );

protected:
	HKEY m_hKey;
	DWORD m_dwLastError;
	int m_nSize;

	BOOL Read( const char *, LPVOID, int );

};
#endif