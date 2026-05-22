#include "pch.h"
#include "MirrorServer.h"
#include "MirrorTool.h"
#pragma warning(disable:4407)

template<MirrorOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	m_operator = EAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
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
			(sockaddr**)&pLocalAddr, &lLength,
			(sockaddr**)&pRemoteAddr, &rLength
		);
		memcpy(m_client->GetLocalAddr(), pLocalAddr, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), pRemoteAddr, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client, (ULONG_PTR)m_client);
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			TRACE("WSARecv failed %d\r\n", ret);
		}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
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
	: m_sock(INVALID_SOCKET), m_received(0), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_buffer(1024),
	m_used(0),
	m_isbusy(false),
	m_vecSend(this, (SENDCALLBACK)&MirrorClient::SendData)
{
	TRACE("m_overlapped %08X\r\n", &m_overlapped);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
	m_recv->m_wsabuffer.buf = m_buffer.data();
	m_recv->m_wsabuffer.len = (ULONG)m_buffer.size();
	m_send->m_wsabuffer.buf = NULL;
	m_send->m_wsabuffer.len = 0;
}

MirrorClient::~MirrorClient()
{
	m_buffer.clear();
	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
	}
}

void MirrorClient::SetOverlapped(MirrorClient* ptr) {
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
	int ret = recv(m_sock, m_buffer.data() + m_used, (int)(m_buffer.size() - m_used), 0);
	if (ret <= 0) {
		return -1;
	}
	m_used += (size_t)ret;
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
	if (listen(m_sock, 3) == -1) {
		TRACE("listen failed %d\r\n", WSAGetLastError());
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, ListenerCompletionKey(), 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&MirrorServer::threadIocp));
	if (!NewAccept()) {
		return false;
	}
	return true;
}

void MirrorServer::BindNewSocket(SOCKET s, ULONG_PTR nKey)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, nKey, 0);
}

int MirrorServer::threadIocp()
{
	while (m_hIOCP != INVALID_HANDLE_VALUE) {
		DWORD tranferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (!GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
			TRACE("GetQueuedCompletionStatus failed %d\r\n", WSAGetLastError());
			continue;
		}
		if (CompletionKey == 0 || lpOverlapped == NULL) {
			continue;
		}

		MirrorOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MirrorOverlapped, m_overlapped);
		pOverlapped->m_server = this;
		TRACE("Operator is %d\r\n", pOverlapped->m_operator);
		switch (pOverlapped->m_operator) {
		case EAccept:
		{
			ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
			TRACE("pOver %08X\r\n", pOver);
			m_pool.DispatchWorker(pOver->m_worker);
		}
		break;
		case ERecv:
		{
			RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
			m_pool.DispatchWorker(pOver->m_worker);
		}
		break;
		case ESend:
		{
			SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
			m_pool.DispatchWorker(pOver->m_worker);
		}
		break;
		case EError:
		{
			ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
			m_pool.DispatchWorker(pOver->m_worker);
		}
		break;
		default:
			break;
		}
	}
	return 0;
}
