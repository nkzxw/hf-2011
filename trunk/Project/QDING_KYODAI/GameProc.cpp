// ChessPoint.cpp: implementation of the CChessPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GameProc.h"

//��Ϸ ���ܺ���
HWND gameh;
RECT r1;
POINT p;//x,y

CHESS_INFO g_ChessInfo;
byte chessdata[11][19];

CChessPoint::CChessPoint(POINT pxy) 
{    
	up = pxy;
	down = pxy;
	left = pxy;
	right = pxy;

	p = pxy;
	up.y = pxy.y - 1;
	down.y = pxy.y + 1;
	left.x = pxy.x - 1;
	right.x = pxy.x + 1;
}

CChessPoint::~CChessPoint()
{
	//TODO
}

void InitChessInfo()
{
	g_ChessInfo.caption = "QQ������";
	g_ChessInfo.lpQPBase = (LPVOID)0x0018BB58;
	g_ChessInfo.lpQZBase = (LPVOID)0x0018F51C;

	g_ChessInfo.width = 31;
	g_ChessInfo.height = 35;
	g_ChessInfo.x_offset = 30;
	g_ChessInfo.y_offset = 200;

	g_ChessInfo.start_xoffset = 663;
	g_ChessInfo.start_yoffset = 524;
}

void InitNetChessInfo(int x, int y)
{
	g_ChessInfo.caption = "QQ��Ϸ - ��������ɫ��";
	g_ChessInfo.lpQPBase = (LPVOID)0x0012A480;
	g_ChessInfo.lpQZBase = (LPVOID)0x0012E04C; //0x00115CDC

	g_ChessInfo.width = 31;
	g_ChessInfo.height = 35;
	g_ChessInfo.x_offset = 24;
	g_ChessInfo.y_offset = 192;

	g_ChessInfo.start_xoffset = 660;  //���԰�ť x = 740;  //��ʼ��ť x = 660;  
	g_ChessInfo.start_yoffset = 565;  //���԰�ť y = 565;  //��ʼ��ť y = 565;
}

BOOL EnablePrivilege ()
{
	HANDLE hToken;
	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)){
		return false;
	}

	TOKEN_PRIVILEGES tp;
	if (!LookupPrivilegeValue (NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)){
		return false;
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges (hToken, false, &tp, sizeof (tp), NULL, NULL);

	return true;
}

void MyMessageBox(LPCSTR lpText, int iErrorId)
{
	char buf[256];
	memset (buf, 0, 256);
	sprintf (buf, "%s GetLastError = %d.", lpText, iErrorId);
	MessageBoxA (NULL, buf, "KYODAI", 0);
}

// д�ַ������ļ�,bLog�����Ƿ�Ϊ��־�ļ�
void writeLog(const char *str)          
{
	static int i = 1;
    FILE *fp = fopen("c:\\monitor.txt", "a+");

	int ret = fprintf(fp, "[%d]: %s\n", ++i, str);

    if(ret >= 0)

    {
        fflush(fp);
    }
	fclose(fp);
}

void startGame()
{
	//��ȡ��Ϸ���ھ��
	gameh=::FindWindowA(NULL,g_ChessInfo.caption);
	::GetWindowRect(gameh,&r1); 
 
	//���浱ǰ���ָ��
	//ȡ�õ�ǰ���λ��
	GetCursorPos(&p);
	
	//�������ָ��λ��  ȡ������������:x=655;y=577 //lparam 0x0241028f
	SetCursorPos(g_ChessInfo.start_xoffset + r1.left, g_ChessInfo.start_yoffset + r1.top);
	
	//ģ������ ��������갴��/���̧��
	//MOUSEEVENTF_LEFTDOWN Specifies that the left button is down. 
    //MOUSEEVENTF_LEFTUP 
	//����ڵ�ǰλ�ð���
	mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
	
	//����ڵ�ǰλ��̧��
	mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	
	//��ԭ���λ��
	Sleep(200);//��һ��ʱ�� ��ִ�к�ߵĴ���
    SetCursorPos(p.x,p.y);
}

void updatdChess() //�������������� chessdata
{
	if (!EnablePrivilege ())
	{
		MyMessageBox ("EnablePrivilege", GetLastError ());
	}

	//��ȡ���ھ��
	HWND gameh=::FindWindowA(NULL,g_ChessInfo.caption);
	
	//��ȡ���ڽ���ID
	DWORD processid;
	::GetWindowThreadProcessId(gameh,&processid);
	
	//��ָ������
	HANDLE processH=::OpenProcess(PROCESS_ALL_ACCESS,false,processid);
	
	//��ָ������ �ڴ�����
    DWORD byread;
	//LPCVOID pbase=(LPCVOID)0x0012A508;  //�������ݻ�ַ
	LPVOID  nbuffer=(LPVOID)&chessdata; //�����������

	DWORD dwOldProtect;
	if(!VirtualProtectEx (processH , g_ChessInfo.lpQPBase, sizeof (chessdata), PAGE_READWRITE, &dwOldProtect)){
		MyMessageBox ("VirtualProtectEx", GetLastError ());
	}

	::ReadProcessMemory(processH,g_ChessInfo.lpQPBase,nbuffer,11*19,&byread);

	DWORD dwNewProtect;
	if(!VirtualProtectEx (processH, g_ChessInfo.lpQPBase, sizeof (chessdata), dwOldProtect, &dwNewProtect)){
		MyMessageBox ("VirtualProtectEx2", GetLastError ());
	}
}

int ReadChessNum() //�������������� chessdata
{	
	if (!EnablePrivilege ())
	{
		MyMessageBox ("EnablePrivilege", GetLastError ());
	}

	//��ȡ���ھ��
	HWND gameh=::FindWindowA(NULL,g_ChessInfo.caption);
	
	//��ȡ���ڽ���ID
	DWORD processid;
	::GetWindowThreadProcessId(gameh,&processid);
	
	//��ָ������
	HANDLE processH=::OpenProcess(PROCESS_ALL_ACCESS,false,processid);
	
	//��ָ������ �ڴ�����
    DWORD byread;
	//LPCVOID pbase=(LPCVOID)0x001166E0 ; //�������ݻ�ַ
	int ChessNum;
	LPVOID  nbuffer=(LPVOID)&ChessNum;  //�����������
	
	DWORD dwOldProtect;
	if(!VirtualProtectEx (processH , g_ChessInfo.lpQPBase, sizeof (int), PAGE_READWRITE, &dwOldProtect)){
		MyMessageBox ("VirtualProtectEx", GetLastError ());
	}

	::ReadProcessMemory(processH,g_ChessInfo.lpQZBase,nbuffer,4,&byread);

	DWORD dwNewProtect;
	if(!VirtualProtectEx (processH, g_ChessInfo.lpQZBase, sizeof (int), dwOldProtect, &dwNewProtect)){
		MyMessageBox ("VirtualProtectEx2", GetLastError ());
	}
	
	return ChessNum;
}
bool CheckLine(	POINT p1,POINT p2)
{
	//ͬһ���ϵ������ ȫΪ0 �򷵻��� 

	int x,y,t;    

	//��� p1==p2 and Ϊ0 Ҳ������
	if ((p1.x == p2.x) &&
		(p1.y == p2.y) && 
		(chessdata[p1.y][p1.x] == 0) && 
		(chessdata[p2.y][p2.x] == 0))  
	{
		return  true;   
	}
	else if ((p1.x < 0) || (p1.x > 18) || 
			 (p1.y < 0) || (p1.y > 10) ||
			 (p2.x < 0) || (p2.x > 18) || 
			 (p2.y < 0) || (p2.y > 10) )  
	{
		return  false; 
	}

	//���X������� �Ƚ�
	if (p1.x == p2.x) 
	{
		if (p1.y > p2.y) 
		{
			t = p1.y;
			p1.y = p2.y;
			p2.y = t;
		}
		
		for (y = p1.y; y<= p2.y; y++)
		{
			if (chessdata[y][p1.x] != 0) 
			{
				return false;
			}
		}
	} 

	//���Y����� ��Ƚ�
	if (p1.y == p2.y) 
	{    
		if (p1.x > p2.x)  
		{
			t = p1.x;
			p1.x = p2.x ;
			p2.x = t;
		}

		for(x = p1.x; x <= p2.x; x++)
		{
			if (chessdata[p1.y][x] != 0) 
			{
				return  false;
			}
		};
	};

	return  true;
};

bool CheckChess(
	POINT a,
	POINT b
	)
{
	CChessPoint p1(a), p2(b);
	POINT  pa, pb;	//ת�ǵ�
	int x, y;

	// ���2��Ϊͬһ�� �򷵻ؼ�
	if ((a.x == b.x) && (a.y == b.y ))  { 
		return false;
	} 
	else if ((chessdata[a.y][a.x]==0) || 
			(chessdata[b.y][b.x]==0)){ 
		return false;
	} 
	else if (chessdata[a.y][a.x] != 
			 chessdata[b.y][b.x]){ 
		return false;
	}
  
	pa = a; 
	pb = b;
	
	// �ں���һ������ y���� ��ͬ
	if (a.y == b.y)  
	{     
		// 2����y������
		if ((p1.right.x == p2.p.x) || 
			(p1.left.x == p2.p.x) ){ 
			return true;   
		}

		//��� �������Ƿ���һ��·����ͨ
        if (CheckLine(p1.right, p2.left)) {
			return true; 
		}

		//��� ���� 
		
		//y ��
		pa = a;
		pb = b;
		if ((p1.up.y >= 0) && 
			(p1.up.y <= 10)) {
			for ( y=0 ;y<=p1.up.y;y++)
			{
				pa.y=y;pb.y=y;
				if (CheckLine(pa,p1.up) && 
					CheckLine(pb,p2.up ) && 
					CheckLine(pa,pb))  { 
						return true; 
				}
			}
		}

		// y��
		pa = a; 
		pb = b;
		if ((p1.down.y >= 0) && 
			(p1.down.y <= 10) ){ 
			for ( y=p1.down.y;y<=10;y++)
			{
				pa.y=y;pb.y=y;
				if (CheckLine(pa,p1.down ) && 
					CheckLine(pb,p2.down ) && 
					CheckLine(pa,pb))  { 
						return true; 
				}                                                
			}
		}

		//��� ���� ��Ϊ y����ȣ����Բ���������·��
	} 
	else if (a.x==b.x){
		//x���� ���ڲ�
		if ((p1.down.y==p2.p.y ) || 
			(p1.up.y==p2.p.y))   { 
			return true;   
		}
		
		//��� �������Ƿ���һ��·����ͨ
		if (CheckLine(p1.down,p2.up) )  { 
			return true;    
		}
		
		//��� ����   ��Ϊx ����� ���Բ�����·��
		//��� ����

		//x��
		pa=a;
		pb=b;
		for (x=0 ;x<=p1.left.x ;x++)
		{
			pa.x=x;
			pb.x=x;
			if (CheckLine(pa,p1.left) && 
				CheckLine(pb,p2.left ) && 
				CheckLine(pa,pb))  { 
				return true;  
			}
		}

		//x��
		pa = a;
		pb = b;
		for(x = p1.right.x; x <= 18; x++)
		{
			pa.x =x;
			pb.x = x;
			if (CheckLine(pa,p1.right ) && 
				CheckLine(pb,p2.right ) && 
				CheckLine(pa,pb))  { 
				return true;  
			}
		}
	} 
	else {
		pa = a;
		pb = b;
		if (a.x > b.x)  {   
			// p2�� �� �� left
			// ��x��·��
			for (x=0;x<=p2.left.x;x++)
			{
				pa.x=x;
				pb.x=x;
				if (CheckLine(pa,p1.left) && 
					CheckLine(pa,pb) && 
					CheckLine(pb,p2.left)){
					return true; 
				}                               
			} // end for

			for (x=p2.right.x ;x<= p1.left.x;x++)
			{
				pa.x=x;
				pb.x=x;
				if (CheckLine(p2.right,pb) && 
					CheckLine(pa,pb)&& 
					CheckLine(pa,p1.left))  {
					return true; 
				}                                                
			}

			for (x=p2.right.x;x<=18;x++)
			{
				pa.x=x;
				pb.x=x;
				if (CheckLine(p1.right ,pa)&& 
					CheckLine(p2.right ,pb) && 
					CheckLine(pa,pb))  { 
					return true; 
				}
			}

			// ��y��·�� �����Ǵ������� ���� ����p1.y>p2.y
			// ��ʼ������ y����
			pa.x=a.x;   
			pb.x=b.x; 
			for ( y=0 ;y<=p1.up.y; y++)    //1��
			{
				pa.y=y;
				pb.y=y;
				if (CheckLine(pb,pa) && 
					CheckLine(pa,p1.up) && 
					CheckLine(pb,p2.up)){ 
					return true;
				}                                                            
			}

			for (y=p1.down.y ;y<=p2.up.y;y++)//2��
			{
				pa.y=y;
				pb.y=y;
				if (CheckLine(pb,pa)&& 
					CheckLine(p1.down,pa) && 
					CheckLine(pb,p2.up))   {
					return true;
				}
			}

			for (y=p2.down.y ;y<=10 ;y++) //3��
			{
				pa.y=y;
				pb.y=y;
				if (CheckLine(pb,pa) && 
					CheckLine(p1.down,pa) && 
					CheckLine(p2.down,pb))   { 
					return true;   
				}
			}
		} 
		else{
			// p2��  �� �� right a.x>b.x
			// ��ʼ������
			pa.y=a.y;   
			pb.y=b.y; 
			for (x=0 ;x<= p1.left.x ;x++);
			{
				pa.x=x;
				pb.x=x;
				if (CheckLine(pa,pb)&& 
					CheckLine(pa,p1.left)&& 
					CheckLine(pb,p2.left))  {
					return true;
				}                                                       
			}

			for (x=p1.right.x ;x<=p2.left.x;x++)
			{
				pa.x=x;
				pb.x=x;
				if (CheckLine(pa,pb)&& 
					CheckLine(p1.right,pa)&& 
					CheckLine(pb,p2.left))  { 
					return true;
				}
			}

			for (x=p2.right.x ;x<=18;x++)
			{
				pa.x=0;pb.x=x;
				if (CheckLine(pa,pb) && 
					CheckLine(p1.right,pa)&& 
					CheckLine(p2.right,pb)) {
						return true; 
				}                                                               
			}

			// y�ὥ��
			// ��ʼ������
			pa.x =a.x;   
			pb.x =b.x ; 
			if ((p1.up.y>=0) && 
				(p1.up.y<=10))
			{
				for (y=0 ;y<=p1.up.y ;y++)    //1��
				{
					pa.y=y;
					pb.y=y;
					if (CheckLine(pa,pb)&& 
						CheckLine(pa,p1.up) && 
						CheckLine(pb,p2.up))  { 
						return true; 
					}                                                                           
				}
			}

			//��ʼ������
			pa.x =a.x;   
			pb.x =b.x ; 
			if ((p1.down.y<=10) 
				&& (p2.up.y>=0)) 
			{
				for (y=p1.down.y ;y<=p2.up.y;y++)  //2��
				{
					pa.y=y;
					pb.y=y;
					if (CheckLine(pa,pb)&& 
						CheckLine(p1.down,pa) && 
						CheckLine(pb,p2.up))  { 
						return true;
					}                                                                          
				}
			}
	    
			pa.x =a.x;   
			pb.x =b.x ; //��ʼ������
			if (p2.down.y <=10) 
			{
				for ( y=p2.down.y;y<=10;y++)           //3��
				{
					pa.y=y;
					pb.y=y;
					if (CheckLine(pa,pb) && 
						CheckLine(p1.down,pa)&& 
						CheckLine(p2.down ,pb))  { 
						return true; 
					}
				}                                                                          
			}
		}
	}
	return false;
}

bool ClickChess(POINT p1,POINT p2)
{
	//���p1
	HWND hwnd=FindWindowA(NULL,g_ChessInfo.caption);

	int lparam;
	lparam = ((p1.y*g_ChessInfo.height + g_ChessInfo.y_offset) << 16) + (p1.x*g_ChessInfo.width + g_ChessInfo.x_offset);

	SendMessage(hwnd,WM_LBUTTONDOWN,0,lparam);//
	SendMessage(hwnd,WM_LBUTTONUP,0,lparam);//

	//���p2
	lparam = ((p2.y*g_ChessInfo.height + g_ChessInfo.y_offset) << 16) + (p2.x*g_ChessInfo.width + g_ChessInfo.x_offset);

	SendMessage(hwnd,WM_LBUTTONDOWN,0,lparam);//
	SendMessage(hwnd,WM_LBUTTONUP,0,lparam);//

	chessdata[p1.y][p1.x] = 0;
	chessdata[p2.y][p2.x] = 0;

	return true; 
}

bool ClearPiar() //����һ������
{    
	//��������������chessdata 11,19
    updatdChess();
	
	//������������ �ҳ���ͬ���� һ������
	POINT p1,p2;
	int x1,y1,x2,y2;
	for (y1=0;y1<11;y1++){
		for (x1=0;x1<19;x1++)
		{   
			for (y2=y1;y2<11;y2++){
				for (x2=0;x2<19;x2++){
					if ((chessdata[y1][x1] == chessdata[y2][x2]) // ����1������2 �����Ƿ���ͬ
						&&(!((x1 == x2)&&(y1 == y2)))  //Ҫ���1���2 ������
						)
					{  
						p1.x=x1;
						p1.y=y1;
						
						p2.x=x2;
						p2.y=y2;

						//��� ��ͬ��2�������Ƿ������
						if ( CheckChess(p1,p2))//��������� �򷵻���
						{
							//click2p ���ģ�� ��� p1��p2
							BYTE bp1 = chessdata[p1.y][p1.x];
							BYTE bp2 = chessdata[p2.y][p2.x];
							ClickChess(p1,p2);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}