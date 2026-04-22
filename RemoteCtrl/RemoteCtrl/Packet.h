#pragma once

#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1) 
class CPacket
{
public:
	CPacket()
		:sHead(0), nLength(0), sCmd(0), sSum(0)
	{
	};
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
		//查找包头
		for (; i < nSize; i++) {  // i + 1 ?
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		// 2. 至少还要有 nLength + sCmd + sSum
		if (i + 4 + 2 + 2 > nSize) {  //包数据可能不全，或者包头未能完全收到
			nSize = 0;
			return;
		}
		// 3. 读取长度
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {  //包未完全收到，就返回，解析失败
			nSize = 0;
			return;
		}
		// 5. 读取命令字
		sCmd = *(WORD*)(pData + i); i += 2;
		// 6. 读取数据区
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		//7.读取校验和
		sSum = *(WORD*)(pData + i); i += 2;

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
	const char* Data() {
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
};

#pragma pack(pop)


typedef struct MouseEvent {
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
