// RemoteClientDlg.cpp: 瀹炵幇鏂囦欢
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

	int CStringToInt(const CString& text)
	{
		return _ttoi(text);
	}
}

// 鐢ㄤ簬搴旂敤绋嬪簭鈥滃叧浜庘€濊彍鍗曢」鐨?CAboutDlg 瀵硅瘽妗?

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 鏀寔
	
// 瀹炵幇
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

// CRemoteClientDlg 娑堟伅澶勭悊绋嬪簭

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 灏嗏€滃叧浜?..鈥濊彍鍗曢」娣诲姞鍒扮郴缁熻彍鍗曚腑銆?

	// IDM_ABOUTBOX 蹇呴』鍦ㄧ郴缁熷懡浠よ寖鍥村唴銆?
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

	// 
	

	InitUIData();
	m_isClosed = true;
	return TRUE;  // 闄ら潪灏嗙劍鐐硅缃埌鎺т欢锛屽惁鍒欒繑鍥?TRUE

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

// 濡傛灉鍚戝璇濇娣诲姞鏈€灏忓寲鎸夐挳锛屽垯闇€瑕佷笅闈㈢殑浠ｇ爜
//  鏉ョ粯鍒惰鍥炬爣銆? 瀵逛簬浣跨敤鏂囨。/瑙嗗浘妯″瀷鐨?MFC 搴旂敤绋嬪簭锛?
//  杩欏皢鐢辨鏋惰嚜鍔ㄥ畬鎴愩€?

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 鐢ㄤ簬缁樺埗鐨勮澶囦笂涓嬫枃

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 浣垮浘鏍囧湪宸ヤ綔鍖虹煩褰腑灞呬腑
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 缁樺埗鍥炬爣
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

//褰撶敤鎴锋嫋鍔ㄦ渶灏忓寲绐楀彛鏃剁郴缁熻皟鐢ㄦ鍑芥暟鍙栧緱鍏夋爣
//鏄剧ず銆?
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);

}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, CStringToInt(m_nPort));
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO:点击文件信息按钮，发送命令，获取文件信息
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
	// TODO: 获取选中的目录，发送命令，获取文件信息
	
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
{   //case 2:
	TRACE("hasnext %d isdirectory %d %s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);
	if (finfo.HasNext == FALSE)return;
	if (finfo.IsDirectory) {
		if (CString(finfo.szFileName) == "." || (CString(finfo.szFileName) == ".."))
			return;
		TRACE("hselected %08X %08X\r\n", hParent, m_Tree.GetSelectedItem());
		CString fileName(finfo.szFileName);
		HTREEITEM hTemp = m_Tree.InsertItem(fileName, hParent);
		m_Tree.InsertItem(_T(""), hTemp, TVI_LAST);
		m_Tree.Expand(hParent, TVE_EXPAND);  //TODO: 只展开当前目录，其他目录保持不变 
	}
	else {
		m_List.InsertItem(0, CString(finfo.szFileName));
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	TRACE("length %d index %d\r\n", length, index);
	if (length == 0) {
		
		length = *(long long*)strData.c_str();
		if (length == 0) {
			AfxMessageBox(_T("文件长度为零或者无法读取文件！！！"));
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
	CPoint ptMouse, ptList;  //鼠标位置
	GetCursorPos(&ptMouse); 
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList); //鼠标位置转换为列表控件坐标
	int ListSelected = m_List.HitTest(ptList);  //获取鼠标所在行
	if (ListSelected < 0) return;  //如果没有选中任何行则返回
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
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
//	int nListSelected = m_List.GetSelectionMark(); //鑾峰彇閫変腑琛岀殑绱㈠紩
//	CString strFile = m_List.GetItemText(nListSelected, 0); //鑾峰彇閫変腑琛岀殑鏂囦欢鍚?
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
//			AfxMessageBox(_T("鏈湴娌℃湁鏉冮檺淇濆瓨璇ユ枃浠讹紝鎴栬€呮枃浠舵棤娉曞垱寤猴紒锛侊紒"));
//			m_dlgStatus.ShowWindow(SW_HIDE);
//			EndWaitCursor();

//			return;
//		}
//		HTREEITEM hSelected = m_Tree.GetSelectedItem(); //鑾峰彇閫変腑鏍戣妭鐐?
//		strFile = GetPath(hSelected) + strFile; //鑾峰彇鏂囦欢鐨勫畬鏁磋矾寰?
//		TRACE("%s\r\n", LPCSTR(strFile));
//		CClientSocket* pClient = CClientSocket::getInstance();
//		do {
//			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			int ret = CClientController::getInstance()->SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			if (ret < 0) {
//				AfxMessageBox("鎵ц涓嬭浇鍛戒护澶辫触锛侊紒");
//				TRACE("鎵ц涓嬭浇澶辫触锛歳et = %d\r\n", ret);
//				break;
//			}
//			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
//			if (nLength == 0) {
//				AfxMessageBox("鏂囦欢闀垮害涓洪浂鎴栬€呮棤娉曡鍙栨枃浠讹紒锛侊紒");
//				break;
//			}
//			long long nCount = 0;
//			while (nCount < nLength) {
//				ret = pClient->DealCommand();
//				if (ret < 0) {
//					AfxMessageBox("浼犺緭澶辫触锛侊紒");
//					TRACE("浼犺緭澶辫触锛歳et = %d\r\n", ret);
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
//	MessageBox(_T("涓嬭浇瀹屾垚锛侊紒"), _T("瀹屾垚"));
//}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {
	case 1://鑾峰彇椹卞姩淇℃伅
		Str2Tree(strData, m_Tree);
		break;
	case 2://鑾峰彇鏂囦欢淇℃伅
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
        MessageBox(_T("打开文件完成！"), _T("操作完成"), MB_ICONINFORMATION);
		break;
	case 4:
		UpdateDownloadFile(strData, (FILE*)lParam);
		break;
	case 9:
        MessageBox(_T("删除文件完成！"), _T("操作完成"), MB_ICONINFORMATION);
		break;
	case 1981:
        MessageBox(_T("连接测试成功！"), _T("连接成功"), MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received! %d\r\n", nCmd);
		break;
	}
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		TRACE("socket is error %d\r\n", lParam);
	}
	else if (lParam == 1) {
		//TODO: 连接断了，通知界面
		TRACE("socket is closed!\r\n");
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(head.sCmd, head.strData, lParam);
		}
	}
	return 0;
}

void CRemoteClientDlg::InitUIData()
{
	//  初始化UI数据，例如设置默认的服务器地址和端口号，加载目录树等
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加初始化代码
	UpdateData();
	/*m_server_address = MAKEIPADDRESS(172, 20, 10, 3);*/
	/*m_server_address = MAKEIPADDRESS(192, 168, 43, 250);*/
	m_server_address = MAKEIPADDRESS(127, 0, 0, 1);
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, CStringToInt(m_nPort));

	UpdateData(FALSE);

	m_dlgStatus.Create(IDD_DLG_STATUS, this); // 创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE); // 默认隐藏状态对话框
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
			m_List.InsertItem(0, CString(pInfo->szFileName));
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
			HTREEITEM hTemp = tree.InsertItem(CString(dr.c_str()), TVI_ROOT, TVI_LAST);
			tree.InsertItem(_T(""), hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (dr.size() > 0) {
		dr += ":";
		HTREEITEM hTemp = tree.InsertItem(CString(dr.c_str()), TVI_ROOT, TVI_LAST);
		tree.InsertItem(_T(""), hTemp, TVI_LAST);
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	

	int nListSelected = m_List.GetSelectionMark(); //得到当前选中的行索引
	CString strFile = m_List.GetItemText(nListSelected, 0); //获取选中行的文件名

	HTREEITEM hSelected = m_Tree.GetSelectedItem(); //获取选中的树节点
	strFile = GetPath(hSelected) + strFile; //获取文件的完整路径
	int ret = CClientController::getInstance()->DownFile(strFile);
	if(ret != 0) {
        AfxMessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
	
}


void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加删除文件的处理代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	CStringA fileA = CStringToAnsi(strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true,
		(BYTE*)(LPCSTR)fileA, fileA.GetLength());
	if (ret < 0) {
        AfxMessageBox(_T("删除文件失败！"));
	}
	LoadFileCurrent();

}

void CRemoteClientDlg::OnOpenFile()
{
	// TODO: 在此添加打开文件的处理代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	CStringA fileA = CStringToAnsi(strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true,
		(BYTE*)(LPCSTR)fileA, fileA.GetLength());
	if (ret < 0) {
        AfxMessageBox(_T("打开文件失败！"));
	}
}



void CRemoteClientDlg::OnBnClickedBtnStartWatch() 
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	CDialogEx::OnTimer(nIDEvent);
}



void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO: 


	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, CStringToInt(m_nPort));
}
