// miniloader_gui.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CMiniloaderApp:
// �йش����ʵ�֣������ miniloader_gui.cpp
//

class CMiniloaderApp : public CWinApp
{
public:
	CMiniloaderApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMiniloaderApp theApp;