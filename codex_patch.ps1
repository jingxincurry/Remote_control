$files = @(
  'd:\teach\c++\Remote_control\RemoteClient\RemoteClient\ClientController.cpp',
  'd:\teach\c++\Remote_control\RemoteClient\RemoteClient\RemoteClientDlg.cpp',
  'd:\teach\c++\Remote_control\RemoteCtrl\RemoteCtrl\Command.h'
)
$enc = [System.Text.Encoding]::GetEncoding(936)

function Read-Text($path) {
  [System.IO.File]::ReadAllText($path, $enc)
}
function Write-Text($path, $text) {
  [System.IO.File]::WriteAllText($path, $text, $enc)
}
function Replace-Exact($text, $old, $new) {
  if (-not $text.Contains($old)) { throw "Pattern not found: $old" }
  $text.Replace($old, $new)
}

$path = $files[0]
$text = Read-Text $path
if ($text -notmatch 'namespace \{\s*CStringA CStringToAnsi') {
  $text = Replace-Exact $text "#include <map>`r`n" "#include <map>`r`n`r`nnamespace {`r`n`tCStringA CStringToAnsi(const CString& text)`r`n`t{`r`n`t`treturn CStringA(text);`r`n`t}`r`n}`r`n"
}
$text = Replace-Exact $text "`tFILE* pFile = NULL;`r`n`tfopen_s(&pFile, m_strLocal, \"wb+\");`r`n" "`tFILE* pFile = NULL;`r`n#ifdef UNICODE`r`n`t_wfopen_s(&pFile, m_strLocal, L\"wb+\");`r`n#else`r`n`tfopen_s(&pFile, m_strLocal, \"wb+\");`r`n#endif`r`n"
$text = Replace-Exact $text "`tint ret = SendCommandPacket(m_remoteDlg.GetSafeHwnd(), 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);`r`n" "`tCStringA remotePath = CStringToAnsi(m_strRemote);`r`n`tint ret = SendCommandPacket(m_remoteDlg.GetSafeHwnd(), 4, false,`r`n`t`t(BYTE*)(LPCSTR)remotePath, remotePath.GetLength(), (WPARAM)pFile);`r`n"
Write-Text $path $text

$path = $files[1]
$text = Read-Text $path
if ($text -notmatch 'namespace \{\s*CStringA CStringToAnsi') {
  $text = Replace-Exact $text "#include <afxinet.h>`r`n" "#include <afxinet.h>`r`n`r`nnamespace {`r`n`tCStringA CStringToAnsi(const CString& text)`r`n`t{`r`n`t`treturn CStringA(text);`r`n`t}`r`n}`r`n"
}
$text = Replace-Exact $text "`tCClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelect);`r`n" "`tCStringA pathA = CStringToAnsi(strPath);`r`n`tCClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,`r`n`t`t(BYTE*)(LPCSTR)pathA, pathA.GetLength(), (WPARAM)hTreeSelect);`r`n"
$text = Replace-Exact $text "`tif (length == 0) {`r`n`t`tlength = *(long long*)strData.c_str();`r`n" "`tif (length == 0) {`r`n`t`tif (strData.size() < sizeof(long long)) {`r`n`t`t`tAfxMessageBox(\"下载数据包异常，文件长度信息不完整。\");`r`n`t`t`tCClientController::getInstance()->DownloadEnd();`r`n`t`t`treturn;`r`n`t`t}`r`n`t`tlength = *(long long*)strData.c_str();`r`n"
$text = Replace-Exact $text "`tint nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());`r`n" "`tCStringA pathA = CStringToAnsi(strPath);`r`n`tint nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,`r`n`t`t(BYTE*)(LPCSTR)pathA, pathA.GetLength());`r`n"
$text = Replace-Exact $text "`tint ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());`r`n" "`tCStringA fileA = CStringToAnsi(strFile);`r`n`tint ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true,`r`n`t`t(BYTE*)(LPCSTR)fileA, fileA.GetLength());`r`n"
$text = Replace-Exact $text "`tint ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());`r`n" "`tCStringA fileA = CStringToAnsi(strFile);`r`n`tint ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true,`r`n`t`t(BYTE*)(LPCSTR)fileA, fileA.GetLength());`r`n"
Write-Text $path $text

$path = $files[2]
$text = Read-Text $path
if ($text -notmatch 'namespace \{\s*std::wstring AnsiPathToWide') {
  $text = Replace-Exact $text "#include \"framework.h\"`r`n" "#include \"framework.h\"`r`n`r`nnamespace {`r`n`tstd::wstring AnsiPathToWide(const std::string& text)`r`n`t{`r`n`t`tif (text.empty()) {`r`n`t`t`treturn std::wstring();`r`n`t`t}`r`n`r`n`t`tint length = MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), NULL, 0);`r`n`t`tif (length <= 0) {`r`n`t`t`treturn std::wstring();`r`n`t`t}`r`n`r`n`t`tstd::wstring wide(length, L'\0');`r`n`t`tMultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), &wide[0], length);`r`n`t`treturn wide;`r`n`t}`r`n}`r`n"
}
$text = Replace-Exact $text "        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOW);`r`n" "        std::wstring widePath = AnsiPathToWide(strPath);`r`n        ShellExecuteW(NULL, NULL, widePath.c_str(), NULL, NULL, SW_SHOW);`r`n"
$text = Replace-Exact $text "        errno_t err = fopen_s(&fp, strPath.c_str(), \"rb\");`r`n" "        std::wstring widePath = AnsiPathToWide(strPath);`r`n        errno_t err = _wfopen_s(&fp, widePath.c_str(), L\"rb\");`r`n"
$text = Replace-Exact $text "        `r`n        TCHAR sPath[MAX_PATH] = _T(\"\");`r`n        //mbstowcs(sPath, strPath.c_str(), strPath.size()); //涓枃瀹规槗涔辩爜`r`n        MultiByteToWideChar(`r`n            CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,`r`n            sizeof(sPath) / sizeof(TCHAR));`r`n        DeleteFileA(strPath.c_str());`r`n" "`r`n        std::wstring widePath = AnsiPathToWide(strPath);`r`n        DeleteFileW(widePath.c_str());`r`n"
Write-Text $path $text