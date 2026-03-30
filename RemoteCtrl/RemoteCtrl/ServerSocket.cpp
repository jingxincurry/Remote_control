#include "pch.h"
#include "ServerSocket.h"


//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();

bool CServerSocket::InitSocket() {
    
    if (m_sock == -1) return false;
     //2.∞Û∂®µÿ÷∑
    sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_port = htons(9527);
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(m_sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr)) == -1)
    {
        return false;
    }
      
    if (listen(m_sock, 1) == -1) {
        return false;
    }
    return true;
}

bool CServerSocket::AcceptClient() {
    sockaddr_in client_adr;
    int client_len = sizeof(client_adr);
    m_client = accept(m_sock, (sockaddr*)&client_adr, &client_len);
    if (m_client == -1) return false;
    return true;
    
}


#define BUFFER_SIZE 4096
int CServerSocket::DealCommand() {
    if (m_client == -1) return -1;
    //char buffer[1024];
    char* buffer = new char[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    size_t index = 0;
    while (true) {
        size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
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

bool CServerSocket::Send(const char* pData, int nSize) {
    if (m_client == -1) return false;
    return send(m_client, pData, nSize, 0) > 0;
}

bool CServerSocket::Send(CPacket& pack) {
    if (m_client == -1) return false;
    return send(m_client, pack.Data(), pack.Size(), 0) > 0;
}

bool CServerSocket::GetFilePath(std::string& strPath) {
    if (m_packet.sCmd >= 2 &&  m_packet.sCmd <= 4 ) {
        strPath = m_packet.strData;
		return true;
    }
	return false;
}

bool CServerSocket::GetMouseEvent(MOUSEEV& mouse) {
    if (m_packet.sCmd == 5) {
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
        return true;
    }
	return false;
}