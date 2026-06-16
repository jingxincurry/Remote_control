#include "pch.h"
#include "MNetwork.h"

MServer::MServer(const MServerParameter& param) : m_stop(false), m_args(NULL)
{
	m_params = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)& MServer::threadFunc));
}

MServer::~MServer() {
	Stop();
}

int MServer::Invoke(void* arg)
{
    m_args = arg;
	m_sock.reset(new MSocket(m_params.m_type));
    //MSOCKET sock(new MSocket(MTYPE::MTypeUDP));
    if (*m_sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return -1;
    }
	if (m_params.m_type == MTYPE::MTypeTCP)
        if (m_sock->listen() == -1) {
            return -2;
        }
    MSockaddrIn client;
    if (-1 == m_sock->bind(m_params.m_ip, m_params.m_port)) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return -3;
    }
	if (m_thread.Start() == false) return -4;
	return 0;
}

int MServer::Send(MSOCKET& client, const MBuffer& buffer)
{
    int ret = m_sock->send(buffer); //TODO:待优化
    if (m_params.m_send) m_params.m_send(m_args, client, ret);
    return ret;
}

int MServer::Sendto(MSockaddrIn& addr, const MBuffer& buffer)
{
    int ret = m_sock->sendto(buffer, addr); //TODO:待优化
    if (m_params.m_sendto) m_params.m_sendto(m_args, addr, ret);
    return ret;
}

int MServer::Stop()
{
    if (m_stop == false) {
        if (m_sock) {
            m_sock->close();
        }
        m_stop = true;
        m_thread.Stop();
    }
    return 0;
}

int MServer::threadFunc()
{
    if (m_params.m_type == MTYPE::MTypeTCP) {
        return threadTCPFunc();
    }
    else if (m_params.m_type == MTYPE::MTypeUDP) {
        return threadUDPFunc();
    }
    return -1;
}

int MServer::threadUDPFunc()
{
    MBuffer buf(1024 * 256);
    MSockaddrIn client;
    int ret = 0;
    while (!m_stop) {
        buf.resize(1024 * 256);
        ret = m_sock->recvfrom(buf, client);
        if (ret > 0) {
            client.update();
			if (m_params.m_recvfrom != NULL) {
				m_params.m_recvfrom(m_args, buf, client);
			}
        }
        else {
            printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
            break;
        }
    }
	if (m_stop == false) m_stop = true;
    m_sock->close();
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    return 0;
}

int MServer::threadTCPFunc()
{
    return 0;
}

MServerParameter::MServerParameter(
    const std::string& ip, 
    short port, MTYPE type, 
    AcceptFunc acceptf, 
    RecvFunc recvf, 
    SendFunc sendf, 
    RecvFromFunc recvfromf, 
    SendToFunc sendtof)
{
	m_ip = ip;
	m_port = port;
	m_type = type;
	m_accept = acceptf;
	m_recv = recvf;
	m_send = sendf;
	m_recvfrom = recvfromf;
	m_sendto = sendtof;
}

MServerParameter& MServerParameter::operator<<(AcceptFunc func)
{
	m_accept = func;
	return *this;

}

MServerParameter& MServerParameter::operator<<(RecvFunc func)
{
    m_recv = func;
	return *this;
   
}

MServerParameter& MServerParameter::operator<<(SendFunc func)
{
	m_send = func;
    return *this;
}

MServerParameter& MServerParameter::operator<<(RecvFromFunc func)
{
	m_recvfrom = func;
    return *this;
}

MServerParameter& MServerParameter::operator<<(SendToFunc func)
{
	m_sendto = func;
    return *this;
}

MServerParameter& MServerParameter::operator<<(std::string& ip)
{
	m_ip = ip;
    return *this;
}

MServerParameter& MServerParameter::operator<<(short port)
{
	m_port = port;
    return *this;
}

MServerParameter& MServerParameter::operator<<(MTYPE type)
{
	m_type = type;
    return *this;
}


MServerParameter& MServerParameter::operator>>(AcceptFunc& func)
{
	func = m_accept;
    return *this;
}

MServerParameter& MServerParameter::operator>>(RecvFunc& func)
{
	func = m_recv;
    return *this;
}

MServerParameter& MServerParameter::operator>>(SendFunc& func)
{
	func = m_send;
    return *this;
}

MServerParameter& MServerParameter::operator>>(RecvFromFunc& func)
{
	func = m_recvfrom;
    return *this;
}

MServerParameter& MServerParameter::operator>>(SendToFunc& func)
{
	func = m_sendto;
    return *this;
}

MServerParameter& MServerParameter::operator>>(std::string& ip)
{
	ip = m_ip;
    return *this;
}

MServerParameter& MServerParameter::operator>>(short& port)
{
	port = m_port;
    return *this;
}

MServerParameter& MServerParameter::operator>>(MTYPE& type)
{
	type = m_type;
    return *this;
}

MServerParameter::MServerParameter(const MServerParameter& param)
{
	m_ip = param.m_ip;
	m_port = param.m_port;
	m_type = param.m_type;
	m_accept = param.m_accept;
	m_recv = param.m_recv;
	m_send = param.m_send;
	m_recvfrom = param.m_recvfrom;
	m_sendto = param.m_sendto;
}

MServerParameter& MServerParameter::operator=(const MServerParameter& param)
{
    if (this != &param) {
        m_ip = param.m_ip;
        m_port = param.m_port;
        m_type = param.m_type;
        m_accept = param.m_accept;
        m_recv = param.m_recv;
        m_send = param.m_send;
        m_recvfrom = param.m_recvfrom;
        m_sendto = param.m_sendto;
    }
    return *this;
}
