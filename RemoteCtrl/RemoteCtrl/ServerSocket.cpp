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

int CServerSocket::DealCommand() {
    if (m_client == -1) return false;
    char buffer[1024];
    while (true) {
        int len = recv(m_client, buffer, sizeof(buffer), 0);
        if (len <= 0) {
            return -1;
        }
        //TODO:
    }
    
}

bool CServerSocket::Send(const char* pData, int nSize) {
    if (m_client == -1) return false;
    return send(m_client, pData, nSize, 0) > 0;
}