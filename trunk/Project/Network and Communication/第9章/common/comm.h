//////////////////////////////////////////////////
// comm.h�ļ�

// ����һЩ��������



#ifndef __COMM_H__
#define __COMM_H__


// У��͵ļ���
// ��16λ����Ϊ��λ����������������ӣ��������������Ϊ������
// ���ټ���һ���ֽڡ����ǵĺʹ���һ��32λ��˫����
USHORT checksum(USHORT* buff, int size);

BOOL SetTTL(SOCKET s, int nValue);
BOOL SetTimeout(SOCKET s, int nTime, BOOL bRecv = TRUE);


#endif // __COMM_H__