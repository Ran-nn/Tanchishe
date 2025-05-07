#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <string>
#include <nlohmann/json.hpp>

#pragma comment(lib, "wininet.lib")

using json = nlohmann::json;

// 函数用于发送 POST 请求
std::string sendPostRequest(const std::string& url, const std::string& data) {
    HINTERNET hInternet = InternetOpen(L"APIRequest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
        return "";
    }

    std::wstring wideUrl(url.begin(), url.end());
    HINTERNET hConnect = InternetOpenUrl(hInternet, wideUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_POST, 0);
    if (!hConnect) {
        std::cerr << "InternetOpenUrl failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return "";
    }

    std::wstring contentType = L"application/json";
    DWORD dwBytesWritten;
    if (!HttpSendRequest(hConnect, contentType.c_str(), -1, const_cast<char*>(data.c_str()), static_cast<DWORD>(data.length()))) {
        std::cerr << "HttpSendRequest failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[1024];
    std::string response;
    DWORD bytesRead;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return response;
}

// 注册函数
bool registerUser(const std::string& username, const std::string& password) {
    std::string url = "http://127.0.0.1:5000/register";
    json requestData = {
        {"username", username},
        {"password", password}
    };
    std::string response = sendPostRequest(url, requestData.dump());

    try {
        json responseJson = json::parse(response);
        if (responseJson.contains("message") && responseJson["message"] == "User registered successfully") {
            return true;
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return false;
}

// 登录函数，验证账号密码
bool loginUser(const std::string& username, const std::string& password) {
    std::string url = "http://127.0.0.1:5000/verify";
    json requestData = {
        {"username", username},
        {"password", password}
    };
    std::string response = sendPostRequest(url, requestData.dump());

    try {
        json responseJson = json::parse(response);
        if (responseJson.contains("message") && responseJson["message"] == "Password verified successfully") {
            return true;
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return false;
}
    