#pragma once

#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	};

	bool InitSocket();

	bool AcceptClient();

	int DealCommand();

	bool Send(const char* pData, int nSize);

private:
	SOCKET m_client;
	SOCKET m_sock;
	CServerSocket& operator=(const CServerSocket& ss) {};
	CServerSocket(const CServerSocket& ss) {
		m_client = ss.m_client;
		m_sock = ss.m_sock;
	};
	CServerSocket() {
		
		m_client = INVALID_SOCKET; // -1
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境, 请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);;
	};

	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return FALSE;
		};
		return TRUE;

	};
	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;

	class CHelper
	{
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}

	};
	static CHelper m_helper;
};

//extern CServerSocket server;