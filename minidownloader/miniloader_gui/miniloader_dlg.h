// miniloader_dlg.h : ͷ�ļ�
//

#pragma once
#include "downloader.h"

// CMiniloaderDlg �Ի���
class CMiniloaderDlg : public CDialog
{
// ����
public:
	CMiniloaderDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MINILOADER_GUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	downloader downloader_;
public:
	afx_msg void OnBnClickedButton1();
};
