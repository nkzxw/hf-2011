// ChessPoint.cpp: implementation of the CChessPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GameProc.h"

//游戏 功能函数
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
	g_ChessInfo.caption = "QQ连连看";
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
	g_ChessInfo.caption = "QQ游戏 - 连连看角色版";
	g_ChessInfo.lpQPBase = (LPVOID)0x0012A480;
	g_ChessInfo.lpQZBase = (LPVOID)0x0012E04C; //0x00115CDC

	g_ChessInfo.width = 31;
	g_ChessInfo.height = 35;
	g_ChessInfo.x_offset = 24;
	g_ChessInfo.y_offset = 192;

	g_ChessInfo.start_xoffset = 660;  //测试按钮 x = 740;  //开始按钮 x = 660;  
	g_ChessInfo.start_yoffset = 565;  //测试按钮 y = 565;  //开始按钮 y = 565;
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

// 写字符串到文件,bLog表明是否为日志文件
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
	//获取游戏窗口句柄
	gameh=::FindWindowA(NULL,g_ChessInfo.caption);
	::GetWindowRect(gameh,&r1); 
 
	//保存当前鼠标指针
	//取得当前鼠标位置
	GetCursorPos(&p);
	
	//设置鼠标指针位置  取开局所在坐标:x=655;y=577 //lparam 0x0241028f
	SetCursorPos(g_ChessInfo.start_xoffset + r1.left, g_ChessInfo.start_yoffset + r1.top);
	
	//模拟鼠标的 单击（鼠标按下/鼠标抬起）
	//MOUSEEVENTF_LEFTDOWN Specifies that the left button is down. 
    //MOUSEEVENTF_LEFTUP 
	//鼠标在当前位置按下
	mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
	
	//鼠标在当前位置抬起
	mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
	
	//还原鼠标位置
	Sleep(200);//过一段时间 再执行后边的代码
    SetCursorPos(p.x,p.y);
}

void updatdChess() //更新棋盘数据至 chessdata
{
	if (!EnablePrivilege ())
	{
		MyMessageBox ("EnablePrivilege", GetLastError ());
	}

	//获取窗口句柄
	HWND gameh=::FindWindowA(NULL,g_ChessInfo.caption);
	
	//获取窗口进程ID
	DWORD processid;
	::GetWindowThreadProcessId(gameh,&processid);
	
	//打开指定进程
	HANDLE processH=::OpenProcess(PROCESS_ALL_ACCESS,false,processid);
	
	//读指定进程 内存数据
    DWORD byread;
	//LPCVOID pbase=(LPCVOID)0x0012A508;  //棋盘数据基址
	LPVOID  nbuffer=(LPVOID)&chessdata; //存放棋盘数据

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

int ReadChessNum() //更新棋盘数据至 chessdata
{	
	if (!EnablePrivilege ())
	{
		MyMessageBox ("EnablePrivilege", GetLastError ());
	}

	//获取窗口句柄
	HWND gameh=::FindWindowA(NULL,g_ChessInfo.caption);
	
	//获取窗口进程ID
	DWORD processid;
	::GetWindowThreadProcessId(gameh,&processid);
	
	//打开指定进程
	HANDLE processH=::OpenProcess(PROCESS_ALL_ACCESS,false,processid);
	
	//读指定进程 内存数据
    DWORD byread;
	//LPCVOID pbase=(LPCVOID)0x001166E0 ; //棋子数据基址
	int ChessNum;
	LPVOID  nbuffer=(LPVOID)&ChessNum;  //存放棋子数据
	
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
	//同一线上的两点间 全为0 则返回真 

	int x,y,t;    

	//如果 p1==p2 and 为0 也返回真
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

	//如果X轴相等则 比较
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

	//如果Y轴相等 则比较
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
	POINT  pa, pb;	//转角点
	int x, y;

	// 如果2点为同一点 则返回假
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
	
	// 在横向一条线上 y坐标 相同
	if (a.y == b.y)  
	{     
		// 2点在y轴相邻
		if ((p1.right.x == p2.p.x) || 
			(p1.left.x == p2.p.x) ){ 
			return true;   
		}

		//检测 这条线是否有一条路径相通
        if (CheckLine(p1.right, p2.left)) {
			return true; 
		}

		//检测 上下 
		
		//y 上
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

		// y下
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

		//检测 左右 因为 y轴相等，所以不存在左右路径
	} 
	else if (a.x==b.x){
		//x下上 相邻不
		if ((p1.down.y==p2.p.y ) || 
			(p1.up.y==p2.p.y))   { 
			return true;   
		}
		
		//检测 这条线是否有一条路径相通
		if (CheckLine(p1.down,p2.up) )  { 
			return true;    
		}
		
		//检测 上下   国为x 轴相等 所以不存在路径
		//检测 左右

		//x左
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

		//x右
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
			// p2点 在 左 left
			// 找x轴路径
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

			// 找y轴路径 由于是从上向下 搜索 所以p1.y>p2.y
			// 初始化坐标 y渐变
			pa.x=a.x;   
			pb.x=b.x; 
			for ( y=0 ;y<=p1.up.y; y++)    //1段
			{
				pa.y=y;
				pb.y=y;
				if (CheckLine(pb,pa) && 
					CheckLine(pa,p1.up) && 
					CheckLine(pb,p2.up)){ 
					return true;
				}                                                            
			}

			for (y=p1.down.y ;y<=p2.up.y;y++)//2段
			{
				pa.y=y;
				pb.y=y;
				if (CheckLine(pb,pa)&& 
					CheckLine(p1.down,pa) && 
					CheckLine(pb,p2.up))   {
					return true;
				}
			}

			for (y=p2.down.y ;y<=10 ;y++) //3段
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
			// p2点  在 右 right a.x>b.x
			// 初始化坐标
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

			// y轴渐变
			// 初始化坐标
			pa.x =a.x;   
			pb.x =b.x ; 
			if ((p1.up.y>=0) && 
				(p1.up.y<=10))
			{
				for (y=0 ;y<=p1.up.y ;y++)    //1段
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

			//初始化坐标
			pa.x =a.x;   
			pb.x =b.x ; 
			if ((p1.down.y<=10) 
				&& (p2.up.y>=0)) 
			{
				for (y=p1.down.y ;y<=p2.up.y;y++)  //2段
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
			pb.x =b.x ; //初始化坐标
			if (p2.down.y <=10) 
			{
				for ( y=p2.down.y;y<=10;y++)           //3段
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
	//点击p1
	HWND hwnd=FindWindowA(NULL,g_ChessInfo.caption);

	int lparam;
	lparam = ((p1.y*g_ChessInfo.height + g_ChessInfo.y_offset) << 16) + (p1.x*g_ChessInfo.width + g_ChessInfo.x_offset);

	SendMessage(hwnd,WM_LBUTTONDOWN,0,lparam);//
	SendMessage(hwnd,WM_LBUTTONUP,0,lparam);//

	//点击p2
	lparam = ((p2.y*g_ChessInfo.height + g_ChessInfo.y_offset) << 16) + (p2.x*g_ChessInfo.width + g_ChessInfo.x_offset);

	SendMessage(hwnd,WM_LBUTTONDOWN,0,lparam);//
	SendMessage(hwnd,WM_LBUTTONUP,0,lparam);//

	chessdata[p1.y][p1.x] = 0;
	chessdata[p2.y][p2.x] = 0;

	return true; 
}

bool ClearPiar() //消除一对棋子
{    
	//读出棋盘数据至chessdata 11,19
    updatdChess();
	
	//遍历整个棋盘 找出相同类型 一对棋子
	POINT p1,p2;
	int x1,y1,x2,y2;
	for (y1=0;y1<11;y1++){
		for (x1=0;x1<19;x1++)
		{   
			for (y2=y1;y2<11;y2++){
				for (x2=0;x2<19;x2++){
					if ((chessdata[y1][x1] == chessdata[y2][x2]) // 棋子1与棋子2 类型是否相同
						&&(!((x1 == x2)&&(y1 == y2)))  //要求点1与点2 相等则假
						)
					{  
						p1.x=x1;
						p1.y=y1;
						
						p2.x=x2;
						p2.y=y2;

						//检测 相同的2个棋子是否可消掉
						if ( CheckChess(p1,p2))//如果可消除 则返回真
						{
							//click2p 鼠标模拟 点击 p1，p2
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