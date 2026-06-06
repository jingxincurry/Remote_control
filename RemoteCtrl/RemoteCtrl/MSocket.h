#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Ws2ipdef.h>
#include <Iphlpapi.h>
#include <memory>
#include <string>

enum class MTYPE
{
	MTypeTCP = 1,
	MTypeUDP
};

class MSockaddrIn {
private:
	static std::string ConvertAddrToString(const in_addr& addr) {
		char buffer[INET_ADDRSTRLEN];
		InetNtopA(AF_INET, (void*)&addr, buffer, sizeof(buffer));
		return std::string(buffer);
	}
public:
	MSockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}
	MSockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = ConvertAddrToString(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	MSockaddrIn(UINT nIP, USHORT nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = nIP;
		m_ip = ConvertAddrToString(m_addr.sin_addr);
		m_port = nPort;
	};
	MSockaddrIn(const std::string& strIP, short nPort) {
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		InetPtonA(AF_INET, strIP.c_str(), &m_addr.sin_addr);
	}
	MSockaddrIn(const MSockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));
		m_ip = ConvertAddrToString(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	MSockaddrIn& operator=(const MSockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));
			m_ip = ConvertAddrToString(m_addr.sin_addr);
			m_port = ntohs(m_addr.sin_port);
		}
		return *this;
	}

	operator sockaddr* () const {
		return (sockaddr*)&m_addr;
	};
	operator void* () const {
		return (void*)&m_addr;
	};
	void update() {
		m_ip = ConvertAddrToString(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	void update(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = ConvertAddrToString(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	std::string GetIP() const { return m_ip; }
	short GetPort() const { return m_port; }
	inline int size() const { return sizeof(sockaddr_in); }
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;

};

class MBuffer : public std::string
{
public:
	MBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	MBuffer(size_t size = 0) : std::string()
	{
		if (size > 0) {
			resize(size);
			memset((void*)c_str(), 0, this->size());
		};
	}
	MBuffer(const char* buffer, size_t size) : std::string(buffer, size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

	~MBuffer() {
	}

	operator char* () const { return (char*)c_str();}
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
};

class MSocket
{
public:
	MSocket(MTYPE nType = MTYPE::MTypeTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_nProtocol = nProtocol;
	}
	MSocket(const MSocket& sock) {
		m_socket = socket(PF_INET, (int)sock.m_type, sock.m_nProtocol);
		m_type = sock.m_type;
		m_nProtocol = sock.m_nProtocol;
		m_addr = sock.m_addr;
	}
	MSocket& operator=(const MSocket& sock) {
		if (this != &sock) {
			closesocket(m_socket);
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_nProtocol);
			m_type = sock.m_type;
			m_nProtocol = sock.m_nProtocol;
		}
		return *this;
	}

	~MSocket() {
		close();
	}
	operator SOCKET() const { return m_socket; }
	operator SOCKET(){ return m_socket;}
	bool operator ==(SOCKET sock) const {
		return m_socket == sock;
	}
	int listen(int backlog = 5) {
		if (m_type != MTYPE::MTypeTCP) {
			return -1;
		}
		return ::listen(m_socket, backlog);
	}
	int bind(const std::string& ip, short port) {
		m_addr = MSockaddrIn(ip, port);
		return ::bind(m_socket, m_addr, m_addr.size());
	}
	int accept() {
		return 0;
	}
	int connect(const std::string& ip, short port) {
		return 0;
	}
	int send(const MBuffer& buffer) {
		return ::send(m_socket, buffer.c_str(), (int)buffer.size(), 0);
	}

	int recv(MBuffer& buffer) {
		int ret = ::recv(m_socket, (char*)buffer.c_str(), (int)buffer.size(), 0);
		if (ret > 0) {
			buffer.resize(ret);
		}
		return ret;
	}

	int sendto(const MBuffer& buffer, const MSockaddrIn& to) {
		return ::sendto(m_socket, buffer.c_str(), (int)buffer.size(), 0, to, to.size());
	}

	int recvfrom(MBuffer& buffer, MSockaddrIn& from) {
		int len = from.size();
		int ret = ::recvfrom(m_socket, (char*)buffer.c_str(), (int)buffer.size(), 0, from, &len);
		if (ret > 0) {
			buffer.resize(ret);
			from.update();
		}
		return ret;
	}
	void close() {
		if (m_socket != INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
private:
	SOCKET m_socket;
	MTYPE m_type;
	int m_nProtocol;
	MSockaddrIn m_addr;
};

typedef std::shared_ptr<MSocket> MSOCKET;
