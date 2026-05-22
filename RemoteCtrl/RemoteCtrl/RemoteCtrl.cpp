#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "MirrorTool.h"
#include "Command.h"
#include "MirrorServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

int main()
{
	if (!CMirrorTool::Init()) {
		return 1;
	}

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
	return 0;
}
