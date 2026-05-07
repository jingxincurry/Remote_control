// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"
#include <afxinet.h>

namespace {
	CStringA CStringToAnsi(const CString& text)
	{
		return CStringA(text);
	}
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	
// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()





CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//m_serv_address = MAKEIPADDRESS(127, 0, 0, 1);
	//m_nPort = _T("9527");
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_Serv, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_Serv, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_OPEN_FILE, &CRemoteClientDlg::OnOpenFile)
	
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
END_MESSAGE_MAP()

// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = MAKEIPADDRESS(172, 20, 10, 3);
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));

	UpdateData(FALSE);

	m_dlgStatus.Create(IDD_DLG_STATUS, this); // 创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE); // 显示状态对话框

    
	m_isClosed = true;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE

}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CRemoteClientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	if (m_dlgStatus.GetSafeHwnd())
	{
		m_dlgStatus.DestroyWindow();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);

}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	std::list<CPacket> lstPackets;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == 0) {
		AfxMessageBox(_T("命令处理失败!!!"));
		return;
	}
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

void CRemoteClientDlg::LoadFileInfo()
{
	// TODO: 在此添加控件通知处理程序代码
	
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelect = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelect == NULL) {
		return;
	}
	if (m_Tree.GetChildItem(hTreeSelect) == NULL) {
		return;
	}
	DeleteTreeChildrenItem(hTreeSelect);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelect);
	TRACE("hTreeSelected %08X\r\n", hTreeSelect);
	CStringA pathA = CStringToAnsi(strPath);
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCSTR)pathA, pathA.GetLength(), (WPARAM)hTreeSelect);
	
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	TRACE("hasnext %d isdirectory %d %s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);
	if (finfo.HasNext == FALSE)return;
	if (finfo.IsDirectory) {
		if (CString(finfo.szFileName) == "." || (CString(finfo.szFileName) == ".."))
			return;
		TRACE("hselected %08X %08X\r\n", hParent, m_Tree.GetSelectedItem());
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, hParent);
		m_Tree.InsertItem("", hTemp, TVI_LAST);
		m_Tree.Expand(hParent, TVE_EXPAND);
	}
	else {
		m_List.InsertItem(0, finfo.szFileName);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	TRACE("length %d index %d\r\n", length, index);
	if (length == 0) {
		if (strData.size() < sizeof(long long)) {
			AfxMessageBox("下载数据包异常，文件长度信息不完整。");
			CClientController::getInstance()->DownloadEnd();
			return;
		}
		length = *(long long*)strData.c_str();
		if (length == 0) {
			AfxMessageBox("文件长度为零或者无法读取文件！！！");
			CClientController::getInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && (index >= length)) {
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else {
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		TRACE("index = %d\r\n", index);
		if (index >= length) {
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;  //获取鼠标位置
	GetCursorPos(&ptMouse); 
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList); //屏幕坐标转换为客户区坐标
	int ListSelected = m_List.HitTest(ptList);  //获取鼠标所在的行
	if (ListSelected < 0) return;  //如果没有选中任何行则返回
	CMenu menu;
	menu.LoadMenuA(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0); //获取第一个子菜单
	if(pPupup != NULL)
        pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
}



//void CRemoteClientDlg::threadEntryForDownFile(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
//	thiz->threadDownFile();
//	_endthread();
//}
//
//void CRemoteClientDlg::threadDownFile()
//{
//
//	int nListSelected = m_List.GetSelectionMark(); //获取选中行的索引
//	CString strFile = m_List.GetItemText(nListSelected, 0); //获取选中行的文件名
//	CFileDialog dlg(FALSE, NULL,
//		strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
//		NULL, this);
//	if (dlg.DoModal() == IDOK) {
//      FILE* pFile = NULL;
//     errno_t openErr = 0;
//		#ifdef _UNICODE
//		openErr = _wfopen_s(&pFile, (LPCWSTR)dlg.GetPathName(), L"wb+");
//		#else
//		openErr = fopen_s(&pFile, (LPCSTR)dlg.GetPathName(), "wb+");
//		#endif
//		if (openErr != 0 || pFile == NULL) {
//			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
//			m_dlgStatus.ShowWindow(SW_HIDE);
//			EndWaitCursor();

//			return;
//		}
//		HTREEITEM hSelected = m_Tree.GetSelectedItem(); //获取选中树节点
//		strFile = GetPath(hSelected) + strFile; //获取文件的完整路径
//		TRACE("%s\r\n", LPCSTR(strFile));
//		CClientSocket* pClient = CClientSocket::getInstance();
//		do {
//			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			int ret = CClientController::getInstance()->SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			if (ret < 0) {
//				AfxMessageBox("执行下载命令失败！！");
//				TRACE("执行下载失败：ret = %d\r\n", ret);
//				break;
//			}
//			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
//			if (nLength == 0) {
//				AfxMessageBox("文件长度为零或者无法读取文件！！！");
//				break;
//			}
//			long long nCount = 0;
//			while (nCount < nLength) {
//				ret = pClient->DealCommand();
//				if (ret < 0) {
//					AfxMessageBox("传输失败！！");
//					TRACE("传输失败：ret = %d\r\n", ret);
//					break;
//				}
//				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
//				nCount += pClient->GetPacket().strData.size();
//			}
//		} while (false);
//		fclose(pFile);
//		pClient->CloseSocket();
//	}
//	m_dlgStatus.ShowWindow(SW_HIDE);
//	EndWaitCursor();
//	MessageBox(_T("下载完成！！"), _T("完成"));
//}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {
	case 1://获取驱动信息
		Str2Tree(strData, m_Tree);
		break;
	case 2://获取文件信息
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
		MessageBox("打开文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 4:
		UpdateDownloadFile(strData, (FILE*)lParam);
		break;
	case 9:
		MessageBox("删除文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 1981:
		MessageBox("连接测试成功！", "连接成功", MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received! %d\r\n", nCmd);
		break;
	}
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (wParam != NULL) {
		CPacket* pPacket = (CPacket*)wParam;
		DealCommand(pPacket->sCmd, pPacket->strData, lParam);
		delete pPacket;
	}
	return 0;
}

void CRemoteClientDlg::InitUIData()
{
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	UpdateData();
	m_server_address = 0x7F000001;//0xC0A80167;//192.168.1.103
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	CStringA pathA = CStringToAnsi(strPath);
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCSTR)pathA, pathA.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (dr.size() > 0) {
		dr += ":";
		HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		tree.InsertItem("", hTemp, TVI_LAST);
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	// TODO: 在此添加命令处理程序代码

	int nListSelected = m_List.GetSelectionMark(); //获取选中行的索引
	CString strFile = m_List.GetItemText(nListSelected, 0); //获取选中行的文件名

	HTREEITEM hSelected = m_Tree.GetSelectedItem(); //获取选中树节点
	strFile = GetPath(hSelected) + strFile; //获取文件的完整路径
	int ret = CClientController::getInstance()->DownFile(strFile);
	if(ret != 0) {
		AfxMessageBox("下载文件失败！！！");
	}
	
}


void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	CStringA fileA = CStringToAnsi(strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true,
		(BYTE*)(LPCSTR)fileA, fileA.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！！");
	}
	LoadFileCurrent();

}

void CRemoteClientDlg::OnOpenFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	CStringA fileA = CStringToAnsi(strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true,
		(BYTE*)(LPCSTR)fileA, fileA.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！！");
	}
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch() 
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	CDialogEx::OnTimer(nIDEvent);
}



void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}
