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
int content_D[1];			//所查询注册表键值的内容
DWORD dwType=REG_SZ;		//定义读取数据类型
DWORD dwLength=256;
struct HKEY__*RootKey;		//注册表主键名称
TCHAR *SubKey;				//欲打开注册表项的地址
TCHAR *KeyName;				//欲设置项的名字
TCHAR *ValueName;			//欲设置值的名称
LPBYTE SetContent_S;		//字符串类型
int SetContent_D[256];		//DWORD类型
BYTE SetContent_B[256];		//二进制类型

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