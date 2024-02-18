#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string> 
#include <locale>
#include <codecvt>
#include "../json.hpp"

std::string ws2s(const std::wstring& ws) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(ws);
}
using json = nlohmann::json;

const json defaultConfig = {
    {"color", {{"red", 255}, {"green", 10}, {"blue", 17}}},
    {"crosshair", {{"length", 2}, {"width", 2}}}
};

json readConfigFile(const std::string& filename) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        std::ofstream defaultConfigFile(filename);
        defaultConfigFile << std::setw(4) << defaultConfig << std::endl;
        defaultConfigFile.close();

        configFile.open(filename);
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open default config file: " + filename);
        }
    }

    json config;
    configFile >> config;
    return config;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WCHAR exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring wstr(exePath);

    std::wstring exeDirectory(wstr.begin(), wstr.end());

    size_t lastSlashIndex = exeDirectory.find_last_of(L"\\/");
    std::wstring exeDirectorySubstring = exeDirectory.substr(0, lastSlashIndex);
    std::wstring configFilePath = exeDirectorySubstring + L"\\config.json";

    json config = readConfigFile(ws2s(configFilePath));

    int red = config["color"]["red"];
    int green = config["color"]["green"];
    int blue = config["color"]["blue"];

    int crosshairLength = config["crosshair"]["length"];
    int crosshairWidth = config["crosshair"]["width"];

    int windowSize = crosshairLength * 10;

    const wchar_t CLASS_NAME[] = L"CrossWindowClass";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - windowSize) / 2;
    int windowY = (screenHeight - windowSize) / 2;

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST, // Extended window style for layered and topmost
        CLASS_NAME,                     // Window class
        L"BetterCrosshairs",           // Window text
        WS_POPUP,                       // No title bar, no borders
        windowX, windowY, windowSize, windowSize,
        NULL,                           // No parent window
        NULL,                           // No menu
        GetModuleHandle(NULL),          // Instance handle
        NULL                            // No additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);

    ShowWindow(hwnd, SW_SHOWNORMAL);

    HDC hdc = GetDC(hwnd);

    HPEN hPen = CreatePen(PS_SOLID, crosshairWidth, RGB(red, green, blue));
    SelectObject(hdc, hPen);

    MSG msg = {};
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT || (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE))
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        int middleX = width / 2;
        int middleY = height / 2;

        MoveToEx(hdc, middleX - crosshairLength * 5, middleY, NULL);
        LineTo(hdc, middleX + crosshairLength * 5, middleY);

        MoveToEx(hdc, middleX, middleY - crosshairLength * 5, NULL);
        LineTo(hdc, middleX, middleY + crosshairLength * 5);

        SwapBuffers(hdc);

        Sleep(10);
    }

    DeleteObject(hPen);
    ReleaseDC(hwnd, hdc);

    return 0;
}