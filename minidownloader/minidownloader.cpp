// minidownloader.cpp : �������̨Ӧ�ó������ڵ㡣
//
#include <conio.h>
#include <windows.h>
#include "stdafx.h"
#include <string.h>
#include "stdlib.h"
#include "dlmanager.h"
#include "znet.h"

#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	int key;
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2),&wsaData);
#endif
	dlmanager dl_manager;
	dl_manager.init();
	while(1)
	{
		if(_kbhit())
		{
			if((key =_getch()) == 115/*s key*/)
				break;
		}
		Sleep(1000);
	}
	dl_manager.fini();
	return 0;
}

