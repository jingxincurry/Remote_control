#include "pch.h"
#include "MirrorServer.h"
#include "MirrorTool.h"
#pragma warning(disable:4407)

template<MirrorOperator op>
AcceptOverlapped<op>::AcceptOverlapped(){
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	m_operator = EAccept; //操作类型
	memset(&m_overlapped, 0, sizeof(m_overlapped)); //初始化OVERLAPPED结构体
	m_buffer.resize(1024); //分配缓冲区，大小为1024字节
	m_server = NULL;
}

template<MirrorOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	TRACE("AcceptWorker this %08X\r\n", this);
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize() > 0) {
		LPSOCKADDR pLocalAddr, pRemoteAddr;
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&pLocalAddr, &lLength,//本地地址
			(sockaddr**)&pRemoteAddr, &rLength//远程地址
		);
		memcpy(m_client->GetLocalAddr(), pLocalAddr, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), pRemoteAddr, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client, (ULONG_PTR)m_client);
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:报错
			TRACE("WSARecv failed %d\r\n", ret);
		}
		if (!m_server->NewAccept())
		{
			return -2;
		}
	}
	return -1;//必须返回-1，否则循环不会终止
}

template<MirrorOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<MirrorOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}


MirrorClient::MirrorClient()
	:m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()), //重叠结构体，包含了OVERLAPPED结构体和其他相关信息，用于异步操作
	m_recv(new RECVOVERLAPPED()),  //接收重叠结构体，包含了OVERLAPPED结构体和其他相关信息，用于异步接收操作
	m_send(new SENDOVERLAPPED()),  //发送重叠结构体
	m_vecSend(this, (SENDCALLBACK)&MirrorClient::SendData) //发送数据队列
{
	TRACE("m_overlapped %08X\r\n", &m_overlapped);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

MirrorClient::~MirrorClient()
{
	m_buffer.clear();
	closesocket(m_sock);
}

void MirrorClient::SetOverlapped(MirrorClient* ptr) { //设置重叠结构体的客户端指针，方便在回调函数中访问客户端对象
	m_overlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
}

MirrorClient::operator LPOVERLAPPED() 
{
	return &m_overlapped->m_overlapped;
}

LPWSABUF MirrorClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPOVERLAPPED MirrorClient::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}

LPWSABUF MirrorClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

LPOVERLAPPED MirrorClient::SendOverlapped()
{
	return &m_send->m_overlapped;
}

int MirrorClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0)return -1;
	m_used += (size_t)ret;
	//TODO:解析数据
	CMirrorTool::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}

int MirrorClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

int MirrorClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {
		m_send->m_buffer = data;
		m_send->m_wsabuffer.buf = m_send->m_buffer.data();
		m_send->m_wsabuffer.len = (ULONG)m_send->m_buffer.size();
		m_received = 0;
		m_flags = 0;
		memset(&m_send->m_overlapped, 0, sizeof(m_send->m_overlapped));
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && WSAGetLastError() != WSA_IO_PENDING) {
			CMirrorTool::ShowError();
			return ret;
		}
	}
	return 0;
}

MirrorServer::~MirrorServer()
{
	std::map<SOCKET, MirrorClient*>::iterator it = m_client.begin();
	for (; it != m_client.end(); it++) {
		delete it->second;
		it->second = NULL;
	}
	m_client.clear();
}

bool MirrorServer::StartService()
{

	CreateSocket();
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		TRACE("bind failed %d\r\n", WSAGetLastError());
		return false;
	}
	if(listen(m_sock, 3) == -1) {
		TRACE("listen failed %d\r\n", WSAGetLastError());
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); //创建一个新的IOCP对象，返回一个句柄
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	//将监听套接字m_sock绑定到IOCP句柄m_hIOCP上，0作为完成端口的键值，0表示使用默认的线程数
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, 0, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&MirrorServer::threadIocp));
	if (!NewAccept())return false;
	//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&MirrorServer::threadIocp));
	//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&MirrorServer::threadIocp));
	return true;

}

void MirrorServer::BindNewSocket(SOCKET s, ULONG_PTR nKey)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, nKey, 0); //将套接字s绑定到IOCP句柄m_hIOCP上，nKey作为完成端口的键值，0表示使用默认的线程数
}

int MirrorServer::threadIocp()
{
	DWORD tranferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
		if (CompletionKey != 0) {
			MirrorOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MirrorOverlapped, m_overlapped);
			pOverlapped->m_server = this;
			TRACE("Operator is %d\r\n", pOverlapped->m_operator);
			switch (pOverlapped->m_operator) {
			case EAccept: //如果操作类型是EAccept，表示这是一个接受连接的操作
			{
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
				TRACE("pOver %08X\r\n", pOver);
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case ERecv: //如果操作类型是ERecv，表示这是一个接收数据的操作
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case ESend: //发送数据
			{
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case EError: //如果操作类型是EError，表示这是一个错误的操作
			{
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}
