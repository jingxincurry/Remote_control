// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {
    string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str()); 

}

int MakeDriverInfo()
{
    string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0)   // 1==>A  2==>B 3==>C
        {
            if (result.size() > 0) {
                result += ',';
            }
            result += 'A' + i - 1;

        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <io.h>
#include <list>


int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> listFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {   //无效目录
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限，访问目录"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件！"));
        return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, sizeof(finfo.szFileName));
        //listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));
    //发送信息到控制端
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOW);
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* fp = NULL;
    errno_t err = fopen_s(&fp, strPath.c_str(), "rb");

    if (err != 0) {
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }

    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        data = _ftelli64(fp);
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(fp, 0, SEEK_SET);
        char buffer[1024];
        size_t nSize = 0;
        do {
            nSize = fread(buffer, 1, sizeof(buffer), fp);
            if (nSize > 0) {
                CPacket pack(4, (BYTE*)buffer, nSize);
                CServerSocket::getInstance()->Send(pack);
            }
        } while (nSize >= 1024); //1024字节为一个包,读
        fclose(fp);
    }

    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);

    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {

        DWORD nFlags = 0;
        switch (mouse.nButton)
        {
        case 0: //左键
            nFlags = 1;
            break;
        case 1: //右键
            nFlags = 2;
            break;
        case 2: //中键
            nFlags = 4;
            break;
        case 4: //没有按键
            nFlags = 8;
            break;
        }
        if (nFlags != 8) {
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        }
        switch (mouse.nAction)
        {
        case 0: //单击
            nFlags |= 0x10;
            break;
        case 1: //双击
            nFlags |= 0x20;
            break;
        case 2: //按下
            nFlags |= 0x40;
            break;
        case 3: //放开
            nFlags |= 0x80;
            break;
        default: //不作处理 
            break;
        }
        switch (nFlags) {
        case 0x21: //左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11: //左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x41: //左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81: //左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22: //右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12: //右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42: //右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82: //右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24: //中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14: //中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44: //中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84: //中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08: //单词鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(5, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败！！"));
        return -1;
    }

    return 0;

}

int SendScreen() {
    CImage screen; //GDI
    HDC hScreen = GetDC(NULL);
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  //屏幕位深
    int nWidth = GetDeviceCaps(hScreen, HORZRES);  //屏幕宽高
    int nHeight = GetDeviceCaps(hScreen, VERTRES); //屏幕宽高
    screen.Create(nWidth, nHeight, nBitPerPixel); //创建一个与屏幕等大的图像

    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY); //将屏幕内容复制到图像中
    ReleaseDC(NULL, hScreen);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  //能调整大小的 在内存中分配一个块
    if (hMem == NULL) {
        OutputDebugString(_T("内存分配失败！"));
        return -1;
    }
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream); //创建一个流对象，关联到内存块上
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG); //将图像以PNG格式保存到流中
        //获取流中的数据

        LARGE_INTEGER liZero = {};
        pStream->Seek(liZero, STREAM_SEEK_SET, NULL); //将流的指针移到开头
        PBYTE pData = (PBYTE)GlobalLock(hMem); //锁定内存块，获取指向数据的指针
        SIZE_T nSize = GlobalSize(hMem); //获取内存块的大小
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack); //发送图像数据到控制端
        GlobalUnlock(hMem); //解锁内存块

    }

    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}


#include "LockDialog.h"
#include <process.h>
CLockDialog dlg;
unsigned threadid = 0;
unsigned __stdcall threadLockDlg(void* arg)
{
    TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CXFULLSCREEN);
    TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
    //printf("screen width:%d\r\n", rect.right);
    dlg.MoveWindow(rect); //调整对话框大小和位置，使其覆盖整个屏幕
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    if (pText) {
        CRect rtText;
        pText->GetWindowRect(rtText);
        int nWidth = rtText.Width();//w0
        int x = (rect.right - nWidth) / 2;
        int nHeight = rtText.Height();
        int y = (rect.bottom - nHeight) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
    }

    //窗口置顶
    dlg.SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); //最顶层显示
    //限制鼠标功能
    ShowCursor(FALSE); //隐藏鼠标
    //隐藏任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
	//限制鼠标在对话框内移动
    dlg.GetWindowRect(rect);
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    ClipCursor(rect); //限制鼠标在对话框内移动

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN) { //按下任意键，解锁
            TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x1B) { //按下ESC键，解锁
                break;
            }

        }
    }
    ClipCursor(NULL);
    ShowCursor(TRUE); //显示鼠标
    //显示任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine() {
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
        //_beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\r\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnLockMachine() {
    //dlg.SendMessage(WM_KEYDOWN, 0x1B, 0x01E0001);
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0 x1B, 0x01E0001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
    CPacket pack(8, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConnect() {
    CPacket pack(1981, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\r\n", ret);
    return 0;
}

int DeleteLocalFile()
{
    std::string strPath;
    if (!CServerSocket::getInstance()->GetFilePath(strPath) || strPath.empty()) {
        CPacket pack(9, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size()); //中文容易乱码
    MultiByteToWideChar(
        CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
        sizeof(sPath) / sizeof(TCHAR));
    DeleteFileA(strPath.c_str());
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\r\n", ret);
    return 0;
}

int ExcuteCommand(int nCmd) {
    int ret = 0;
    switch (nCmd) {
    case 1: //查看磁盘分区
        ret = MakeDriverInfo();
        break;
    case 2: //查看指定路径下的文件和文件夹
        ret = MakeDirectoryInfo();
        break;
    case 3: //打开文件
        ret = RunFile();
        break;
    case 4: // 下载文件
        ret = DownloadFile();
        break;
    case 5: //鼠标事件
        ret = MouseEvent();
        break;
    case 6: //发送屏幕内容=》发送屏幕截图
        ret = SendScreen();
        break;
    case 7: //锁机
        ret = LockMachine();
        break;
    case 8: //解锁
        ret = UnLockMachine();
        break;
    case 9: //删除文件
		ret = DeleteLocalFile();
		break;
    case 1981:
        ret = TestConnect();
        break;
    }
    return ret;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。

            CServerSocket* pserver = CServerSocket::getInstance(); //创建单例
            int count = 0;
            if (pserver->InitSocket() == false) {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化网络，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance() != NULL) {
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入失败！"), MB_OK | MB_ICONERROR);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
                TRACE("AcceptClient return true\r\n");
                int ret = pserver->DealCommand();
                TRACE("DealCommand ret %d\r\n", ret);
                if (ret > 0) {
                    ret = ExcuteCommand(ret);
                    if (ret != 0) {
                        TRACE("执行命令失败：%d ret=%d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("Command has done!\r\n");
                }
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
