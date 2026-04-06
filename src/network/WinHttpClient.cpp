#include "network/WinHttpClient.hpp"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace kvm::network {

bool WinHttpClient::Post(const std::string& url, const std::string& body, std::string& outResponse) {
#ifdef _WIN32
    std::string cleanUrl = url;
    std::string token;
    size_t tokenPos = cleanUrl.find("token=");
    if (tokenPos != std::string::npos) {
        token = cleanUrl.substr(tokenPos + 6);
        size_t ampPos = token.find('&');
        if (ampPos != std::string::npos) token = token.substr(0, ampPos);
        
        // Remove token from URL for API calls as per README_API.md
        size_t startRemove = tokenPos > 0 && (cleanUrl[tokenPos-1] == '?' || cleanUrl[tokenPos-1] == '&') ? tokenPos - 1 : tokenPos;
        size_t endRemove = (ampPos != std::string::npos) ? tokenPos + 6 + ampPos : cleanUrl.length();
        cleanUrl.erase(startRemove, endRemove - startRemove);
        if (!cleanUrl.empty() && cleanUrl.back() == '?') cleanUrl.pop_back();
    }

    std::wstring wUrl(cleanUrl.begin(), cleanUrl.end());
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(hostName[0]);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(urlPath[0]);

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) return false;

    HINTERNET hSession = WinHttpOpen(L"KVM-Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    if (flags & WINHTTP_FLAG_SECURE) {
        DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    std::wstring headers = L"Content-Type: application/json\r\nAccept: application/json\r\n";
    if (!token.empty()) headers += L"Authorization: Bearer " + std::wstring(token.begin(), token.end()) + L"\r\n";

    if (!WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)body.c_str(), (DWORD)body.length(), (DWORD)body.length(), 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD dwStatusCode = 0;
    DWORD dwStatusSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwStatusSize, WINHTTP_NO_HEADER_INDEX);
    std::cout << "[WinHTTP] POST " << cleanUrl << " -> " << dwStatusCode << std::endl;

    DWORD dwSize = 0;
    do {
        DWORD dwDownloaded = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<char> buffer(dwSize);
        if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) outResponse.append(buffer.data(), dwDownloaded);
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    return (dwStatusCode >= 200 && dwStatusCode < 300);
#else
    return false;
#endif
}

} // namespace kvm::network
