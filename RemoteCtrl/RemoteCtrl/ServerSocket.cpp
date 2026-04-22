#include "pch.h"
#include "ServerSocket.h"
#include "MirrorTool.h"


//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();

bool CServerSocket::InitSocket(short port) {
    
    if (m_sock == -1) return false;
     //2.绑定地址
    sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_port = htons(port);
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
int CServerSocket::Run(SOCK_CALLBACK callback, void* arg, short port = 9527) {
	// socket、bind、listen、accept、recv、send、close
	bool ret = InitSocket(port);
    if(ret == false) return -1;
	std::list<CPacket> lstPacket;
    m_callback = callback;
    m_arg = arg;
	int count = 0;
    while (true) {
        if (AcceptClient() == false) {
            if(count >= 3) {
                //MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入失败！"), MB_OK | MB_ICONERROR);
                return -2;
			}
			count++;
        }
		int ret = DealCommand();
        if (ret > 0) {
			m_callback(m_arg, ret, lstPacket, m_packet);
            while(lstPacket.size() > 0) {
                Send(lstPacket.front());
                lstPacket.pop_front();
			}
			
        }
        CloseClient();
    }
    
}

bool CServerSocket::AcceptClient() {
    sockaddr_in client_adr;
    int client_len = sizeof(client_adr);
    m_client = accept(m_sock, (sockaddr*)&client_adr, &client_len);
	TRACE("AcceptClient client %d\r\n", m_client);
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
			delete[]buffer;
            return -1;
        }
        index += len;
        len = index;
        m_packet = CPacket::CPacket((BYTE*)buffer, len);
        if (len > 0) {
            memmove(buffer, buffer + len, BUFFER_SIZE - len);
            index -= len;
            delete[]buffer;
            return m_packet.sCmd;
        }
    }
    delete[]buffer;
    return -1;
}

bool CServerSocket::Send(const char* pData, int nSize) {
    if (m_client == -1) return false;
    return send(m_client, pData, nSize, 0) > 0;
}

bool CServerSocket::Send(CPacket& pack) {
    if (m_client == -1) return false;
  CMirrorTool::Dump((BYTE*)pack.Data(), pack.Size());
    return send(m_client, pack.Data(), pack.Size(), 0) > 0;
}



