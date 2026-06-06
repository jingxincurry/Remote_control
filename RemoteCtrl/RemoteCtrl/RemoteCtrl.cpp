#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "MirrorTool.h"
#include "MirrorServer.h"
#include "ServerSocket.h"
#include "Command.h"
#include "MNetwork.h"
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <string.h>
#include <conio.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define INVOKE_PATH _T("C:\\Users\\edoyun\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

#define DEBUG_RUN_IOCP_SERVER 1
#define DEBUG_RUN_OLD_SERVER 0
#define DEBUG_RUN_UDP_DEMO 0
#define DEBUG_RUN_UDP_SERVER 0

CWinApp theApp;

//业务和通用
bool ChooseAutoInvoke(const CString& strPath) {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    if (PathFileExists(strPath)) {
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        if (!CMirrorTool::WriteStartupDir(strPath))
        {
            MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

void udp_server();
void udp_client(bool ishost = true);
static int RunIocpServer();
static int RunOldServer();
static int RunUdpDemo(const char* exePath);
static void PrintRunModeHelp(const char* exePath);
static void WaitForDebugStop(const TCHAR* message);

bool initsock() {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0; //初始化Winsock库，指定使用2.2版本的Winsock
}

void clearsock() {
    WSACleanup();
}

int main(int argc, char* argv[])
{
	if (!CMirrorTool::Init()) {
		return 1;
	}
    int ret = 0;
    bool socketReady = false;

    if (argc == 1) {
#if DEBUG_RUN_IOCP_SERVER
        ret = RunIocpServer();
#elif DEBUG_RUN_OLD_SERVER
        ret = RunOldServer();
#elif DEBUG_RUN_UDP_DEMO
        socketReady = initsock();
        if (socketReady) {
            ret = RunUdpDemo(argv[0]);
        }
        else {
            ret = 1;
        }
#elif DEBUG_RUN_UDP_SERVER
        socketReady = initsock();
        if (socketReady) {
            udp_server();
        }
        else {
            ret = 1;
        }
#else
        PrintRunModeHelp(argv[0]);
#endif
    }
    else if (_stricmp(argv[1], "iocp") == 0) {
        ret = RunIocpServer();
    }
    else if (_stricmp(argv[1], "old") == 0) {
        ret = RunOldServer();
    }
    else if (_stricmp(argv[1], "udp-server") == 0) {
        socketReady = initsock();
        if (socketReady) {
            udp_server();
        }
        else {
            ret = 1;
        }
    }
    else if (_stricmp(argv[1], "udp-host") == 0 || strcmp(argv[1], "1") == 0) {
        socketReady = initsock();
        if (socketReady) {
            udp_client();
        }
        else {
            ret = 1;
        }
    }
    else if (_stricmp(argv[1], "udp-client") == 0 || strcmp(argv[1], "2") == 0) {
        socketReady = initsock();
        if (socketReady) {
            udp_client(false);
        }
        else {
            ret = 1;
        }
    }
    else if (_stricmp(argv[1], "udp-demo") == 0) {
        socketReady = initsock();
        if (socketReady) {
            ret = RunUdpDemo(argv[0]);
        }
        else {
            ret = 1;
        }
    }
    else if (_stricmp(argv[1], "help") == 0 || strcmp(argv[1], "?") == 0) {
        PrintRunModeHelp(argv[0]);
    }
    else {
        PrintRunModeHelp(argv[0]);
        ret = 1;
    }

    if (socketReady) {
        clearsock();
    }
    return ret;
}

static void PrintRunModeHelp(const char* exePath)
{
    printf("Usage:\r\n");
    printf("  %s iocp        start TCP IOCP server (default)\r\n", exePath);
    printf("  %s old         start old blocking TCP server\r\n", exePath);
    printf("  %s udp-server  start UDP NAT traversal coordinator\r\n", exePath);
    printf("  %s udp-host    register as host UDP test client\r\n", exePath);
    printf("  %s udp-client  register as peer UDP test client\r\n", exePath);
    printf("  %s udp-demo    start udp-host, udp-client, and udp-server locally\r\n", exePath);
}

static bool StartChildMode(const char* exePath, const char* mode)
{
    char workDir[MAX_PATH] = "";
    GetCurrentDirectoryA(MAX_PATH, workDir);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    std::string cmd = "\"";
    cmd += exePath;
    cmd += "\" ";
    cmd += mode;

    std::vector<char> commandLine(cmd.begin(), cmd.end());
    commandLine.push_back('\0');

    BOOL ret = CreateProcessA(NULL, commandLine.data(), NULL, NULL, FALSE, 0, NULL, workDir, &si, &pi);
    if (!ret) {
        printf("CreateProcessA failed for mode %s, error=%d\r\n", mode, GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    printf("started child mode %s, pid=%d\r\n", mode, pi.dwProcessId);
    return true;
}

static int RunUdpDemo(const char* exePath)
{
    StartChildMode(exePath, "udp-host");
    StartChildMode(exePath, "udp-client");
    udp_server();
    return 0;
}

static int RunIocpServer()
{
    int ret = 0;
    {
        CCommand cmd;
        MirrorServer server;
        if (!server.StartService(&CCommand::RunCommand, &cmd)) {
            MessageBox(NULL, _T("IOCP服务启动失败"), _T("RemoteCtrl"), MB_OK | MB_ICONERROR);
            ret = -1;
        }
        else {
            WaitForDebugStop(_T("IOCP服务器已启动，正在等待客户端连接。停止调试即可关闭服务器。\r\n"));
        }
    }

    WSACleanup();
    return ret;
}

static void WaitForDebugStop(const TCHAR* message)
{
    TRACE(_T("%s"), message);
    OutputDebugString(message);
    for (;;) {
        Sleep(1000);
    }
}

static int RunOldServer()
{
    CCommand cmd;
    int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd, 9527);
    switch (ret) {
    case -1:
        MessageBox(NULL, _T("Network initialization failed."), _T("RemoteCtrl"), MB_OK | MB_ICONERROR);
        break;
    case -2:
        MessageBox(NULL, _T("Client accept failed repeatedly."), _T("RemoteCtrl"), MB_OK | MB_ICONERROR);
        break;
    default:
        break;
    }
    return ret;
}

struct UdpRendezvousContext {
    UdpRendezvousContext() : server(NULL) {
        InitializeCriticalSection(&lock);
    }
    ~UdpRendezvousContext() {
        DeleteCriticalSection(&lock);
    }

    MServer* server;
    std::vector<MSockaddrIn> clients;
    CRITICAL_SECTION lock;
};

static bool SameEndpoint(const MSockaddrIn& left, const MSockaddrIn& right)
{
    return left.GetIP() == right.GetIP() && left.GetPort() == right.GetPort();
}

static void SendTextTo(MServer* server, MSockaddrIn& addr, const std::string& text)
{
    MBuffer buffer(text.c_str(), text.size());
    server->Sendto(addr, buffer);
}

int RecvFromCB(void* arg, const MBuffer& buffer, MSockaddrIn& addr) {
    UdpRendezvousContext* context = (UdpRendezvousContext*)arg;
    if (context == NULL || context->server == NULL) {
        return -1;
    }

    EnterCriticalSection(&context->lock);

    bool exists = false;
    for (size_t i = 0; i < context->clients.size(); i++) {
        if (SameEndpoint(context->clients[i], addr)) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        context->clients.push_back(addr);
    }

    char selfInfo[128] = "";
    snprintf(selfInfo, sizeof(selfInfo), "REGISTERED %s %d\n", addr.GetIP().c_str(), addr.GetPort());
    SendTextTo(context->server, addr, selfInfo);

    if (context->clients.size() >= 2) {
        for (size_t i = 0; i < context->clients.size(); i++) {
            if (SameEndpoint(context->clients[i], addr)) {
                continue;
            }

            char peerInfo[128] = "";
            snprintf(peerInfo, sizeof(peerInfo), "PEER %s %d\n",
                context->clients[i].GetIP().c_str(),
                context->clients[i].GetPort());
            SendTextTo(context->server, addr, peerInfo);

            char reverseInfo[128] = "";
            snprintf(reverseInfo, sizeof(reverseInfo), "PEER %s %d\n",
                addr.GetIP().c_str(),
                addr.GetPort());
            SendTextTo(context->server, context->clients[i], reverseInfo);
        }
    }

    LeaveCriticalSection(&context->lock);
    return (int)buffer.size();
}
int SendToCB(void* arg, const MSockaddrIn& addr, int ret) {
	printf("sendto %s:%d ret=%d\r\n", addr.GetIP().c_str(), addr.GetPort(), ret);
	return 0;
}

void udp_server()
{
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    MServerParameter param{
        "0.0.0.0",
        20000,
        MTYPE::MTypeUDP,NULL,NULL,NULL,RecvFromCB, SendToCB
    };
    UdpRendezvousContext context;
	MServer server(param);
    context.server = &server;
    if (server.Invoke(&context) != 0) {
        printf("UDP rendezvous server start failed.\r\n");
        return;
    }
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    WaitForDebugStop(_T("UDP穿透协调服务器已启动，正在等待客户端注册。停止调试即可关闭服务器。\r\n"));
    return;

}

void udp_client(bool ishost)
{
    Sleep(2000);
    sockaddr_in server, client;
    int len = sizeof(client);
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    InetPtonA(AF_INET, "127.0.0.1", &server.sin_addr);
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR!!!\r\n", __FILE__, __LINE__, __FUNCTION__);
        return;
    }

    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    std::string msg = ishost ? "REGISTER host\n" : "REGISTER client\n";
    int ret = sendto(sock, msg.c_str(), (int)msg.size(), 0, (sockaddr*)&server, sizeof(server));
    printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret > 0) {
        for (int i = 0; i < 3; i++) {
            len = sizeof(client);
            char recvbuf[256] = "";
            ret = recvfrom(sock, recvbuf, sizeof(recvbuf) - 1, 0, (sockaddr*)&client, &len);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            if (ret <= 0) {
                break;
            }

            recvbuf[ret] = '\0';
            printf("%s(%d):%s ip %08X port %d msg %s\r\n",
                __FILE__, __LINE__, __FUNCTION__,
                client.sin_addr.s_addr, ntohs(client.sin_port), recvbuf);
        }
    }

    closesocket(sock);
}
