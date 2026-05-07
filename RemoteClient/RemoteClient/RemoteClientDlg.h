// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1) //自定义消息，发送数据包到服务器

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	

	
private:
	CImage m_image; //缓存屏幕图像
	
	bool m_isClosed;//监视是否关闭
private:
	

	/*static void threadEntryForDownFile(void* arg);
	void threadDownFile();*/
	void DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam);
	void InitUIData();


	void LoadFileCurrent();

	void Str2Tree(const std::string& drivers, CTreeCtrl& tree);

	void LoadFileInfo();
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);
	void UpdateDownloadFile(const std::string& strData, FILE* pFile);

	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	



// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;


	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();


	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnOpenFile();

	
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnEnChangeEditPort();
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);
};

