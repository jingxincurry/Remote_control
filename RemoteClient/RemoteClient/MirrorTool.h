#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>
#include <afx.h>

class CMirrorTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) { //调试输出十六进制数据
		std::string strOut; //调试输出十六进制数据
		for (size_t i = 0; i < nSize; i++) { //每16个字节换行
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str()); //输出调试信息

	}

	static int Bytes2Image(CImage& image, const std::string& strBuffer)
	{ //将字节数据转换为图像
		BYTE* pData = (BYTE*)strBuffer.c_str(); //图像数据指针
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); //能调整大小的 在内存中分配一个块
		if (hMem == NULL) {
			TRACE("ÄÚ´æ²»×ã\r\n");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream); //创建一个流对象，关联到内存块上
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length); //将字节数据写入流中
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL)
				image.Destroy();
			image.Load(pStream);
		}
		return hRet;
	}
};
