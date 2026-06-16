#pragma once
#include "MSocket.h"
#include "MirrorThread.h"
/*
//1.核心功能到底是什么？（核心功能是网络通信，提供TCP和UDP两种协议的支持）
//2.业务逻辑是什么？（业务逻辑是实现一个远程控制系统，允许客户端连接并发送命令，服务器接收并处理这些命令）
*/
class MNetwork
{

};

typedef int (*AcceptFunc)(void* arg, MSOCKET& client); //接受连接回调函数
typedef int (*RecvFunc)(void* arg, MBuffer& buffer);
typedef int (*SendFunc)(void* arg, MSOCKET& client, int ret);
typedef int (*RecvFromFunc)(void* arg, const MBuffer& buffer, MSockaddrIn& addr);
typedef int (*SendToFunc)(void* arg, const MSockaddrIn& addr, int ret);
class MServerParameter {
public:
	MServerParameter(
		const std::string& ip = "0.0.0.0",
		short port = 9527,
		MTYPE type = MTYPE::MTypeTCP, 
		AcceptFunc acceptf = NULL, 
		RecvFunc recvf = NULL,
		SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	);
	//输入
	MServerParameter& operator<<(AcceptFunc func);
	MServerParameter& operator<<(RecvFunc func);
	MServerParameter& operator<<(SendFunc func);
	MServerParameter& operator<<(RecvFromFunc func);
	MServerParameter& operator<<(SendToFunc func);
	MServerParameter& operator<<(std::string& ip);
	MServerParameter& operator<<(short port);
	MServerParameter& operator<<(MTYPE type);
	//输出
	MServerParameter& operator>>(AcceptFunc& func);
	MServerParameter& operator>>(RecvFunc& func);
	MServerParameter& operator>>(SendFunc& func);
	MServerParameter& operator>>(RecvFromFunc& func);
	MServerParameter& operator>>(SendToFunc& func);
	MServerParameter& operator>>(std::string& ip);
	MServerParameter& operator>>(short& port);
	MServerParameter& operator>>(MTYPE& type);
	//赋值构造函数，等于号重载，用于同类型赋值
	MServerParameter(const MServerParameter& param);
	MServerParameter& operator=(const MServerParameter& param);

	std::string m_ip;
	short m_port;
	MTYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;

};

class MServer:public ThreadFuncBase {
public:
	MServer(const MServerParameter& param);
	~MServer();
	int Invoke(void* arg);
	int Send(MSOCKET& client, const MBuffer& buffer);

	int Sendto(MSockaddrIn& addr, const MBuffer& buffer);
	int Stop();
private:
	int threadFunc();
	int threadUDPFunc();
	int threadTCPFunc();
private:
	//AcceptFunc m_accept;
	//RecvFunc m_recv;
	//SendFunc m_send;
	MServerParameter m_params;
	void* m_args;
	MirrorThread m_thread;
	MSOCKET m_sock;
	std::atomic<bool> m_stop;
};
