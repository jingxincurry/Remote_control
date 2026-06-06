#pragma once
#include <MSWSock.h>
#include "MirrorThread.h"
#include "CMirrorQueue.h"
#include "Packet.h"
#include <map>
#include <list>
#include "MirrorTool.h"
#include <WS2tcpip.h>
enum MirrorOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class MirrorServer;
class MirrorClient;
typedef std::shared_ptr<MirrorClient> PCLIENT;
typedef void (*MIRROR_SOCK_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

class MirrorOverlapped :public ThreadFuncBase
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char> m_buffer;//缓冲区
	ThreadWorker m_worker;//工作线程
	MirrorServer* m_server;//服务器对象
	MirrorClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;
	virtual ~MirrorOverlapped() {
		m_client = NULL;
	}
};

template<MirrorOperator>class AcceptOverlapped;  // 接受连接的重叠结构体
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;  // 接受连接的重叠结构体
template<MirrorOperator>class RecvOverlapped;  // 接收数据的重叠结构体
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;  // 接收数据的重叠结构体
template<MirrorOperator>class SendOverlapped;  // 发送数据的重叠结构体
typedef SendOverlapped<ESend> SENDOVERLAPPED;  // 发送数据的重叠结构体

class MirrorClient :public ThreadFuncBase {
public:
	MirrorClient();

	~MirrorClient();

	void SetOverlapped(MirrorClient* ptr);
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return (PVOID)m_buffer.data();
	}
	operator LPOVERLAPPED();

	operator LPDWORD() {
		return &m_received;
	}
	LPWSABUF RecvWSABuffer();
	LPOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_buffer.size(); }
	void SetReceived(DWORD received) { m_received = received; }
	void ResetBuffer();
	int PostRecv();
	int Recv(MirrorServer* server);
	int Send(void* buffer, size_t nSize);
	int SendPacket(CPacket& pack);
	int SendData(std::vector<char>& data);
	void Close();
private:
	SOCKET m_sock;  // 套接字
	DWORD m_received; // 接收的数据大小
	DWORD m_flags;  // WSARecv的标志位
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped; //重叠结构体，包含OVERLAPPED结构体和其他相关信息，用于异步操作
	std::shared_ptr<RECVOVERLAPPED> m_recv;   //接收重叠结构体，包含OVERLAPPED结构体和其他相关信息，用于异步接收数据
	std::shared_ptr<SENDOVERLAPPED> m_send;   //发送重叠结构体，包含OVERLAPPED结构体和其他相关信息，用于异步发送数据
	std::vector<char> m_buffer;
	size_t m_used;//已使用缓冲区大小
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	MirrorSendQueue<std::vector<char>> m_vecSend;//发送数据队列
};

template<MirrorOperator>
class AcceptOverlapped :public MirrorOverlapped
{
public:
	AcceptOverlapped();
	virtual ~AcceptOverlapped() {};
	int AcceptWorker();
};


template<MirrorOperator>
class RecvOverlapped :public MirrorOverlapped
{
public:
	RecvOverlapped();
	virtual ~RecvOverlapped() {};
	int RecvWorker() {
		int ret = m_client->Recv(m_server);
		return ret;
	}
};

template<MirrorOperator>
class SendOverlapped :public MirrorOverlapped
{
public:
	SendOverlapped();
	virtual ~SendOverlapped() {};
	int SendWorker() {
		return -1;
	}
};

template<MirrorOperator>
class ErrorOverlapped :public MirrorOverlapped
{
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	virtual ~ErrorOverlapped() {}
	int ErrorWorker() {
		//TODO:
		return -1;
	}
};

typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class MirrorServer :public ThreadFuncBase
{
public:
	MirrorServer(const std::string& ip = "0.0.0.0", short port = 9527) : m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE; // IOCP缓冲区
		m_sock = INVALID_SOCKET;
		m_callback = NULL;
		m_arg = NULL;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		//m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
		InetPtonA(AF_INET, ip.c_str(), &m_addr.sin_addr);
	}
	~MirrorServer();
	bool StartService(MIRROR_SOCK_CALLBACK callback = NULL, void* arg = NULL);
	void StopService();
	int DealCommand(MirrorClient* client, CPacket& inPacket);
	bool NewAccept() {
		MirrorClient* pClient = new MirrorClient(); 
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, MirrorClient*>(*pClient, pClient));
		if (!AcceptEx(m_sock,
			*pClient,
			*pClient,
			0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClient, *pClient))
		{
			if (WSAGetLastError() != ERROR_SUCCESS && (WSAGetLastError() != WSA_IO_PENDING)) {
				TRACE("连接失败：%d %s\r\n", WSAGetLastError(), CMirrorTool::GetErrInfo(WSAGetLastError()).c_str());
				closesocket(m_sock);
				m_sock = INVALID_SOCKET;
				m_hIOCP = INVALID_HANDLE_VALUE;
				return false;
			}
		}
		return true;
	}
	void BindNewSocket(SOCKET s, ULONG_PTR nKey);
	ULONG_PTR ListenerCompletionKey() const { return (ULONG_PTR)this; }

private:
	void CreateSocket() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return;
		}
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);  //支持重叠I/O的socket
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
	}
	int threadIocp();
private:
	MirrorThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, MirrorClient*> m_client;
	MIRROR_SOCK_CALLBACK m_callback;
	void* m_arg;
};

