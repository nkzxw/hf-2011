#ifndef _GAME_PROC_H_
#define _GAME_PROC_H_

#include "stdafx.h" 

typedef struct _CHESS_INFO
{	
	PCHAR caption;
	LPVOID lpQPBase;  //棋盘数据基址
	LPVOID lpQZBase;  //棋子数据基址
	
	int width;	//棋子的宽
	int height; //棋子的高
	int x_offset; //第一个棋子距离窗口的x偏移
	int y_offset; //第一个棋子距离窗口的y偏移

	int start_xoffset; //开始按钮距离窗口的x偏移
	int start_yoffset; //开始按钮距离窗口的y偏移
}CHESS_INFO, *PCHESS_INFO;

class CChessPoint  
{
public:
	POINT p;
	POINT up;
	POINT down;
	POINT left;
	POINT right;
	CChessPoint(POINT pxy);  //构造函数
	virtual ~CChessPoint();
};

void MyMessageBox(LPCSTR lpText, int iErrorId);

void writeLog(const char *str);

BOOL EnablePrivilege ();

void InitChessInfo();

void InitNetChessInfo (int x, int y);

int ReadChessNum();

void updatdChess();

void startGame();

bool CheckLine(	POINT p1,POINT p2);

bool CheckChess(
	POINT a,
	POINT b);

bool ClickChess(POINT p1,POINT p2);

bool ClearPiar();

#endif //_GAME_PROC_H_