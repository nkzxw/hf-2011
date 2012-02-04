#ifndef _GAME_PROC_H_
#define _GAME_PROC_H_

#include "stdafx.h" 

typedef struct _CHESS_INFO
{	
	PCHAR caption;
	LPVOID lpQPBase;  //�������ݻ�ַ
	LPVOID lpQZBase;  //�������ݻ�ַ
	
	int width;	//���ӵĿ�
	int height; //���ӵĸ�
	int x_offset; //��һ�����Ӿ��봰�ڵ�xƫ��
	int y_offset; //��һ�����Ӿ��봰�ڵ�yƫ��

	int start_xoffset; //��ʼ��ť���봰�ڵ�xƫ��
	int start_yoffset; //��ʼ��ť���봰�ڵ�yƫ��
}CHESS_INFO, *PCHESS_INFO;

class CChessPoint  
{
public:
	POINT p;
	POINT up;
	POINT down;
	POINT left;
	POINT right;
	CChessPoint(POINT pxy);  //���캯��
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