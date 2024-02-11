#include <iostream>
#include <Windows.h>

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
    // make a variable that will be the length of the crosshair
    int crosshairLength = 1;
    int crosshairWidth = 2;



    // determine the size of the window, it should be exactly as long as the crosshair
    int windowSize = crosshairLength * 10;

    // Register the window class
    const wchar_t CLASS_NAME[] = L"CrossWindowClass";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Calculate window position to center it
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - windowSize) / 2;
    int windowY = (screenHeight - windowSize) / 2;

    // Create the window without decorations
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

    // Make the window always on top
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);

    // Show the window
    ShowWindow(hwnd, SW_SHOWNORMAL);

    // Retrieve a handle to the device context for the window
    HDC hdc = GetDC(hwnd);

    // Set the pen color and make the line thicker (red in this case, width 4 pixels)
    HPEN hPen = CreatePen(PS_SOLID, crosshairWidth, RGB(255, 17, 10));
    SelectObject(hdc, hPen);

    // Run the game loop until the 'Esc' key is pressed
    MSG msg = {};
    while (true) {
        // Process messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT || (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE))
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Clear the window with a white background
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        // Calculate the middle of the window
        int middleX = width / 2;
        int middleY = height / 2;

        // Draw a horizontal line (cross) in the middle of the window
        MoveToEx(hdc, middleX - crosshairLength * 5, middleY, NULL);
        LineTo(hdc, middleX + crosshairLength * 5, middleY);

        // Draw a vertical line (cross) in the middle of the window
        MoveToEx(hdc, middleX, middleY - crosshairLength * 5, NULL);
        LineTo(hdc, middleX, middleY + crosshairLength * 5);

        // Swap buffers to update the window
        SwapBuffers(hdc);

        // Sleep for a short duration to control the frame rate
        Sleep(10);
    }

    // Release resources
    DeleteObject(hPen);
    ReleaseDC(hwnd, hdc);

    return 0;
}
