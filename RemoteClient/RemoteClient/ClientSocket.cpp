#include "pch.h"
#include "ClientSocket.h"
#include <Ws2tcpip.h>
#include <vector>
#include "ClientController.h"


//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;
CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        wsaErrCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
		0, NULL);
    ret = (char*)lpMsgBuf;
    LocalFree(lpMsgBuf);
    return ret;
}

void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());

}

CClientSocket::CClientSocket(const CClientSocket& ss) {
    m_hThread = INVALID_HANDLE_VALUE;
    m_bAutoClose = ss.m_bAutoClose;
    m_sock = ss.m_sock;
    m_nIP = ss.m_nIP;
    m_nPort = ss.m_nPort;
    std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
    for (; it != ss.m_mapFunc.end(); it++) {
        m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
    }
}

CClientSocket::CClientSocket() :
    m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
    m_hThread(INVALID_HANDLE_VALUE)
{
    if (InitSockEnv() == FALSE) {
        MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
        exit(0);
    }
    m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
    // 修复C6387警告，确保m_eventInvoke不为NULL再调用WaitForSingleObject
    if (m_eventInvoke != NULL && WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
        TRACE("网络消息处理线程启动失败了！\r\n");
    }
    CloseHandle(m_eventInvoke);
    m_buffer.resize(BUFFER_SIZE);
    memset(m_buffer.data(), 0, BUFFER_SIZE);
    struct {
        UINT message;
        MSGFUNC func;
    }funcs[] = {
        {WM_SEND_PACK,&CClientSocket::SendPack},
        {0,NULL}
    };
    for (int i = 0; funcs[i].message != 0; i++) {
        if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
            TRACE("插入失败，消息值：%d 函数值:%08X 序号:%d\r\n", funcs[i].message, funcs[i].func, i);
        }
    }
}

bool CClientSocket::InitSocket() {

    if (m_sock != INVALID_SOCKET)CloseSocket();
    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_sock == INVALID_SOCKET)return false;
    sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    TRACE("addr %08X nIP %08X\r\n", htonl(m_nIP), m_nIP);
    serv_adr.sin_addr.s_addr = htonl(m_nIP);
    serv_adr.sin_port = htons(m_nPort);
    if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
        AfxMessageBox("指定的IP地址，不存在！");
        return false;
    }
    int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
    if (ret == -1) {
        AfxMessageBox("连接失败!");
        TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
        return false;
    }
    TRACE("socket init done!\r\n");
    return true;
}




int CClientSocket::DealCommand() {
    if (m_sock == INVALID_SOCKET) return -1;
    //char buffer[1024];
    char* buffer = m_buffer.data();

    static size_t index = 0;
    while (true) {
        size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
        if (((int)len <= 0) && ((int)index <= 0)) {
            return -1;
        }
        TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
        index += len;
        len = index;
        m_packet = CPacket::CPacket((BYTE*)buffer, len);
        if (len > 0) {
            memmove(buffer, buffer + len, index - len);
            index -= len;
            return m_packet.sCmd;
        }
    }
    return -1;
}


bool CClientSocket::Send(const CPacket& pack) { 
    TRACE("m_sock = %d\r\n", m_sock);
    if (m_sock == INVALID_SOCKET)return false;
    std::string strOut;
    pack.Data(strOut);
    return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    PACKET_DATA data = *(PACKET_DATA*)wParam; 
    delete (PACKET_DATA*)wParam;
    HWND hWnd = (HWND)lParam;
    size_t nTemp = data.strData.size();
    CPacket current((BYTE*)data.strData.c_str(), nTemp);
    if (InitSocket() == true) {
        int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
        if (ret > 0) {
            size_t index = 0;
            std::string strBuffer;
            strBuffer.resize(BUFFER_SIZE);
            char* pBuffer = (char*)strBuffer.c_str();
            while (m_sock != INVALID_SOCKET) {
                int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
                if (length > 0 || (index > 0)) {
                    index += (size_t)length;
                    size_t nLen = index;
                    CPacket pack((BYTE*)pBuffer, nLen);
                    if (nLen > 0) {
                        TRACE("ack pack %d to hWnd %08X %d %d\r\n", pack.sCmd, hWnd, index, nLen);
                        TRACE("%04X\r\n", *(WORD*)(pBuffer + nLen));
                        ::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
                        if (data.nMode & CSM_AUTOCLOSE) {
                            CloseSocket();
                            return;
                        }
                        index -= nLen;
                        memmove(pBuffer, pBuffer + nLen, index);
                    }
                }
                else {//TODO：对方关闭了套接字，或者网络设备异常
                    TRACE("recv failed length %d index %d cmd %d\r\n", length, index, current.sCmd);
                    CloseSocket();
                    if (length < 0) {
                        ::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
                    }
                    return;
                }
            }
        }
        else {
            CloseSocket();
            //网络终止处理
            ::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
        }
    }
    else {
        ::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
    }
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
    UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
    std::string strOut;
    pack.Data(strOut);
    PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
    bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
    if (ret == false) {
        delete pData;
    }
    return ret;
}

bool CClientSocket::GetFilePath(std::string& strPath) {
    if (m_packet.sCmd >= 2 && m_packet.sCmd <= 4) {
        strPath = m_packet.strData;
        return true;
    }
    return false;
}

bool CClientSocket::GetMouseEvent(MOUSEEV& mouse) {
    if (m_packet.sCmd == 5) {
        memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
        return true;
    }
    return false;
}

unsigned CClientSocket::threadEntry(void* arg)
{
    CClientSocket* thiz = (CClientSocket*)arg;
    thiz->threadFunc2();
    _endthreadex(0);
    return 0;
}


//void CClientSocket::threadFunc()
//{
//    std::string strBuffer;
//    strBuffer.resize(BUFFER_SIZE);
//    char* pBuffer = (char*)strBuffer.c_str();
//    int index = 0;
//    InitSocket();
//    while (m_sock != INVALID_SOCKET) {
//        if (m_lstSend.size() > 0) {
//            TRACE("lstSend size: %d\r\n", m_lstSend.size());
//            m_lock.lock();
//            CPacket& head = m_lstSend.front();
//            m_lock.unlock();
//            if (Send(head) == false) {
//                TRACE("发送失败！\r\n");
//                continue;
//            }
//            std::map<HANDLE, std::list<CPacket>&>::iterator it;
//            it = m_mapAck.find(head.hEvent);
//            if (it != m_mapAck.end()) {
//                std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
//                do {
//                    int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
//                    TRACE("recv %d %d\r\n", length, index);
//                    if (length > 0 || (index > 0)) {
//                        index += length;
//                        size_t size = (size_t)index;
//                        CPacket pack((BYTE*)pBuffer, size);
//                        if (size > 0) {//TODO:对于文件夹信息获取，文件信息获取可能产生问题
//                            pack.hEvent = head.hEvent;
//                            it->second.push_back(pack);
//                            memmove(pBuffer, pBuffer + size, index - size);
//                            index -= size;
//                            TRACE("SetEvent %d %d\r\n", pack.sCmd, it0->second);
//                            if (it0->second) {
//                                SetEvent(head.hEvent);
//                                break;
//                            }
//                        }
//                    }
//                    else if (length <= 0 && index <= 0) {
//                        CloseSocket();
//                        SetEvent(head.hEvent);//等到服务器关闭命令之后，再通知事情完成
//                        if (it0 != m_mapAutoClosed.end()) {
//                            TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
//                        }
//                        else {
//                            TRACE("异常的情况，没有对应的pair\r\n");
//                        }
//                        break;
//                    }
//                } while (it0->second == false);
//            }
//            m_lock.lock();
//            m_lstSend.pop_front();
//            m_mapAutoClosed.erase(head.hEvent);
//            m_lock.unlock();
//            if (InitSocket() == false) {
//                InitSocket();
//            }
//        }
//        Sleep(1);
//    }
//    CloseSocket();
//}

void CClientSocket::threadFunc2() 
{
    SetEvent(m_eventInvoke);
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        TRACE("Get Message :%08X \r\n", msg.message);
        if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
            (this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
        }
    }
}


//void CClientSocket::Shutdown()
//{
//    CloseSocket();
//    if (m_nThreadID != 0) {
//        PostThreadMessage(m_nThreadID, WM_QUIT, 0, 0);
//        m_nThreadID = 0;
//    }
//    if (m_hThread != INVALID_HANDLE_VALUE && m_hThread != NULL) {
//        WaitForSingleObject(m_hThread, 1000);
//        CloseHandle(m_hThread);
//        m_hThread = INVALID_HANDLE_VALUE;
//    }
//    WSACleanup();
//}