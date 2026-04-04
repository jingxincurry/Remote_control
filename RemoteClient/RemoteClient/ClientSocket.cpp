#include "pch.h"
#include "ClientSocket.h"
#include <Ws2tcpip.h>
#include <vector>


//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();


std::string CClientSocket::GetErrInfo(int wsaErrCode) {
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

bool CClientSocket::InitSocket(int nIP, int nPort) {

    if (m_sock != INVALID_SOCKET)CloseSocket();
    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_sock == -1)return false;
    sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    TRACE("addr %08X nIP %08X\r\n", htonl(nIP), nIP);
    serv_adr.sin_addr.s_addr = htonl(nIP);
    serv_adr.sin_port = htons(nPort);
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
    return true;
}




int CClientSocket::DealCommand() {
    if (m_sock == -1) return -1;
    //char buffer[1024];
    char* buffer = m_buffer.data();

    memset(buffer, 0, BUFFER_SIZE);
    size_t index = 0;
    while (true) {
        size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
        if (len <= 0) {
            return -1;
        }
        index += len;
        len = index;
        m_packet = CPacket::CPacket((BYTE*)buffer, len);
        if (len > 0) {
            memmove(buffer, buffer + len, BUFFER_SIZE - len);
            index -= len;
            return m_packet.sCmd;
        }
    }
    return -1;
}

bool CClientSocket::Send(const char* pData, int nSize) {
    if (m_sock == -1) return false;
    return send(m_sock, pData, nSize, 0) > 0;
}

bool CClientSocket::Send(CPacket& pack) {
    if (m_sock == -1) return false;
    return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
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