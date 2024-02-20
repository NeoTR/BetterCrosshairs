#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <ShellAPI.h>
#include <cmath>
#include "../json.hpp"
#include "../resource.h"

using json = nlohmann::json;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

NOTIFYICONDATA nid = {};
HMENU hMenu;

void CreateNotificationIcon(HWND hwnd) {
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
    nid.uCallbackMessage = WM_USER + 1;

    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    if (hIcon == NULL) {
        return;
    }
    nid.hIcon = hIcon;

    wcscpy_s(nid.szTip, L"BetterCrosshairs");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveNotificationIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

std::string ws2s(const std::wstring& ws) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(ws);
}

const json defaultConfig = {
    {"color", {{"red", 255}, {"green", 10}, {"blue", 17}}},
    {"crosshair", {{"type", 1},{"length", 2}, {"width", 2}, {"rotation", 45}, {"offset", 3}}}
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
        RemoveNotificationIcon();
        PostQuitMessage(0);
        break;
    case WM_USER + 1:
        break;
    case WM_CONTEXTMENU:
        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case WM_APP + 1:
            DestroyWindow(hwnd);
            break;
        }
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

    int crosshairType = config["crosshair"]["type"];
    int crosshairLength = config["crosshair"]["length"];
    int crosshairWidth = config["crosshair"]["width"];
    int crosshairRotation = config["crosshair"]["rotation"];
    int crosshairOffset = config["crosshair"]["offset"];

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
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
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

    CreateNotificationIcon(hwnd);

    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, windowSize, windowSize);
    SelectObject(memDC, memBM);

    HPEN hPen = CreatePen(PS_SOLID, crosshairWidth, RGB(red, green, blue));
    SelectObject(memDC, hPen);

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

        FillRect(memDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        HPEN hPen = CreatePen(PS_SOLID, crosshairWidth, RGB(red, green, blue));
        SelectObject(memDC, hPen);

        int middleX = width / 2;
        int middleY = height / 2;

        double radianRotation = crosshairRotation * M_PI / 180.0;

        if (crosshairType == 1) {
            int points[4][2] = {
                {middleX - crosshairLength * 5, middleY},
                {middleX + crosshairLength * 5, middleY},
                {middleX, middleY - crosshairLength * 5},
                {middleX, middleY + crosshairLength * 5}
            };

            for (int i = 0; i < 4; i++) {
                int x = points[i][0] - middleX;
                int y = points[i][1] - middleY;

                double rotatedX = x * cos(radianRotation) - y * sin(radianRotation);
                double rotatedY = x * sin(radianRotation) + y * cos(radianRotation);

                points[i][0] = static_cast<int>(rotatedX + middleX);
                points[i][1] = static_cast<int>(rotatedY + middleY);
            }

            MoveToEx(memDC, points[0][0], points[0][1], NULL);
            LineTo(memDC, points[1][0], points[1][1]);

            MoveToEx(memDC, points[2][0], points[2][1], NULL);
            LineTo(memDC, points[3][0], points[3][1]);
        }
        else if (crosshairType == 2) {
            int points[8][2] = {
                {middleX, middleY - crosshairLength * 4},
                {middleX, middleY - crosshairOffset},
                {middleX, middleY + crosshairLength * 4},
                {middleX, middleY + crosshairOffset},
                {middleX - crosshairLength * 4, middleY},
                {middleX - crosshairOffset, middleY},
                {middleX + crosshairLength * 4, middleY},
                {middleX + crosshairOffset, middleY}
            };

            for (int i = 0; i < 8; i++) {
                int x = points[i][0] - middleX;
                int y = points[i][1] - middleY;

                double rotatedX = x * cos(radianRotation) - y * sin(radianRotation);
                double rotatedY = x * sin(radianRotation) + y * cos(radianRotation);

                points[i][0] = static_cast<int>(rotatedX + middleX);
                points[i][1] = static_cast<int>(rotatedY + middleY);
            }

            MoveToEx(memDC, points[0][0], points[0][1], NULL);
            LineTo(memDC, points[1][0], points[1][1]);

            MoveToEx(memDC, points[2][0], points[2][1], NULL);
            LineTo(memDC, points[3][0], points[3][1]);

            MoveToEx(memDC, points[4][0], points[4][1], NULL);
            LineTo(memDC, points[5][0], points[5][1]);

            MoveToEx(memDC, points[6][0], points[6][1], NULL);
            LineTo(memDC, points[7][0], points[7][1]);
        }

        BitBlt(hdc, 0, 0, windowSize, windowSize, memDC, 0, 0, SRCCOPY);

        DeleteObject(hPen);
        ReleaseDC(hwnd, hdc);

        Sleep(10);
    }

    DeleteObject(memBM);
    DeleteDC(memDC);

    return 0;
}
