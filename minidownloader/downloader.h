#ifndef DOWNLOADER_IF_H
#define DOWNLOADER_IF_H

#if !defined(WIN32)
#define DL_DECLARE(type)            type
#define DL_DECLARE_NONSTD(type)     type
#define DL_DECLARE_DATA
#else
#define DL_DECLARE(type)            __declspec(dllexport) type __stdcall
#define DL_DECLARE_NONSTD(type)     __declspec(dllexport) type
#define DL_DECLARE_DATA             __declspec(dllexport)
#endif

enum state {
	DL_STATE_LEECH = 0,   //��������
	DL_STATE_COMPLETE = 1    //���
};

struct dlstat{
	//����״̬
	enum state st;
	/*
	* peers:������
	*/
	int peers;
	/*
	* files_got:�Ѿ����ص��ļ���
	* files_total: ���ļ���
	* rate_down: ��������
	*/
	int files_got,files_total,rate_down;
};

class DL_DECLARE_DATA downloader
{
public:
	static int start(void);
	static int stop(void);
	static int state(struct dlstat *dl_state);
};

#endif