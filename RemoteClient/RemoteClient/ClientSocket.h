#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <list>
#define WM_SEND_PACK (WM_USER+1) //发送包数据
#define WM_SEND_PACK_ACK (WM_USER+2) //发送包数据应答

#pragma pack(push)
#pragma pack(1) 

#define BUFFER_SIZE 4096000


class CPacket
{
public:
	CPacket()
		:sHead(0), nLength(0), sCmd(0), sSum(0)
	{};
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
		: sHead(0xFEFF), nLength(static_cast<DWORD>(nSize + 4)), sCmd(nCmd), sSum(0) {
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack)
		: sHead(pack.sHead), nLength(pack.nLength), sCmd(pack.sCmd), strData(pack.strData), sSum(pack.sSum), strOut(pack.strOut) {
	}
	CPacket(const BYTE* pData, size_t& nSize)
		: sHead(0), nLength(0), sCmd(0), sSum(0) {
		size_t i = 0;
		if (pData == NULL || nSize < 2) {
			nSize = 0;
			return;
		}
		//查找包头
		for (; i + sizeof(WORD) <= nSize; i++) {
			WORD head = 0;
			memcpy(&head, pData + i, sizeof(head));
			if (head == 0xFEFF) {
				sHead = head;
				i += 2;
				break;
			}
		}
		if (sHead != 0xFEFF) {
			nSize = 0;
			return;
		}
		// 2. 至少还要有 nLength + sCmd + sSum
		if (i + 4 + 2 + 2 > nSize) {  //包数据可能不全，或者包头未能完全收到
			nSize = 0;
			return;
		}
		// 3. 读取长度
		memcpy(&nLength, pData + i, sizeof(nLength)); i += 4;
		if (nLength < 4 || nLength + i > nSize) {  //包未完全收到，就返回，解析失败
			nSize = 0;
			return;
		}
		// 5. 读取命令字
		memcpy(&sCmd, pData + i, sizeof(sCmd)); i += 2;
		// 6. 读取数据区
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		//7.读取校验和
		memcpy(&sSum, pData + i, sizeof(sSum)); i += 2;

		// 8. 计算校验和
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		// 9. 校验
		if (sum == sSum) {
			nSize = i;  //head length data...
			return;
		}
		nSize = 0;

	}
	~CPacket() {};
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;

	}
	int Size() {
		return nLength + 2 + 4; //head length data sum
	}
	const char* Data(std::string& strOut) const {
		strOut.resize(nLength + 2 + 4);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	//[包头 sHead] [长度 nLength] [命令 sCmd] [数据 strData] [校验 sSum]
	WORD sHead; //固定位 FE FF                     2
	DWORD nLength; //包长度（从控制命令开始，到和校验结束）     4
	// nLength = 2 + 数据长度 + 2         所以 数据长度 = nLength - 4
	WORD sCmd;  //控制命令				2
	std::string strData; //包数据    不确定
	WORD sSum; //和校验            2
	std::string strOut;

	//HANDLE hEvent; //等待应答的事件
};
#pragma pack(pop)

typedef struct MouseEvent {
	//nButton：0表示左键，1表示右键，2表示中键，4没有按键
	//nAction: 0表示单击，1表示双击，2表示按下，3表示放开，4不作处理
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	//nButton：0表示左键，1表示右键，2表示中键，4没有按键
	//nAction: 0表示单击，1表示双击，2表示按下，3表示放开，4不作处理

	WORD nAction; // 点击，移动，双击 
	WORD nButton; // 左键，右键，中键
	POINT ptXY; //坐标

}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; //是否有效
	BOOL IsDirectory; // 是否为目录 0 否 1 是
	BOOL HasNext; //是否有后续  0 没有 1 有
	char szFileName[256];  //文件名
}FILEINFO, * PFILEINFO;

void Dump(BYTE* pData, size_t nSize);

enum {
	CSM_AUTOCLOSE = 1,//CSM = Client Socket Mode 自动关闭模式
};

typedef struct PacketData {
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}
}PACKET_DATA;

std::string GetErrInfo(int wsaErrCode);
void Dump(BYTE* pData, size_t nSize);
class CClientSocket
{
public:
	static CClientSocket* getInstance() { //局部静态变量单例模式	
		if (m_instance == NULL) {
			m_instance = new CClientSocket();
		}
		return m_instance;

	};
	

	bool InitSocket();

	int DealCommand();

	

	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);


	bool GetFilePath(std::string& strPath);

	bool GetMouseEvent(MOUSEEV& mouse);

	CPacket& GetPacket()
	{
		return m_packet;
	}
	void CloseSocket() {
		if (m_sock != INVALID_SOCKET) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
		}
	}

	void UpdateAddress(int nIP, int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

private:

	HANDLE m_eventInvoke;//启动事件
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClosed;

	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}; //禁止赋值
	CClientSocket(const CClientSocket& ss); //禁止复制构造
	CClientSocket();  //构造函数私有化，禁止外部创建对象

	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	};
	static unsigned __stdcall threadEntry(void* arg);
	//void Shutdown();
	//void threadFunc();
	void threadFunc2();

	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return FALSE;
		};
		return TRUE;

	};
	static void releaseInstance() {
		TRACE("CClientSocket has been called!\r\n");
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	bool Send(const char* pData, int nSize) {
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);

	static CClientSocket* m_instance;
	class CHelper
	{
	public:
		CHelper() {
		}
		~CHelper() {
			CClientSocket::releaseInstance(); //程序结束时自动调用，释放单例对象
		}

	};
	static CHelper m_helper;
};

//extern CServerSocket server;
