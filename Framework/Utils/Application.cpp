#include "stdafx.h"
#include "Application.h"
#include "AnExample.h"

namespace Utils {

Application::Application(uint32_t width, uint32_t height, AnExample *example)
: mHwnd(NULL)
, mWidth(width)
, mHeight(height)
, mTitle("")
, mExample(example)
{
    ParseCmds();
}

Application::~Application(void) {

}

void Application::ParseCmds(void) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    for (int i = 1; i < argc; ++i) {
        // handle command here
    }

    LocalFree(argv);
}

int Application::Run(HINSTANCE hInstance, int nCmdShow) {
    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = "Application";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    mHwnd = CreateWindow(
        windowClass.lpszClassName,
        mTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,        // We have no parent window.
        nullptr,        // We aren't using menus.
        hInstance,
        this);

    mExample->Init(mHwnd);

    ShowWindow(mHwnd, nCmdShow);

    // Main sample loop.
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
       }
    }

    mExample->Destroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    AnExample *example = nullptr;
    Application *app = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (app) {
        example = app->mExample;
    }

    switch (message) {

    case WM_CREATE: {
        // Save the Application* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        return 0;
    }

    case WM_KEYDOWN:        if (example) { example->OnKeyDown(static_cast<uint8_t>(wParam)); } return 0;
    case WM_KEYUP:          if (example) { example->OnKeyUp(static_cast<uint8_t>(wParam)); } return 0;
    case WM_CHAR:           if (example) { example->OnChar(static_cast<uint16_t>(wParam)); } return 0;
    case WM_LBUTTONDOWN:    if (example) { example->OnMouseLButtonDown(lParam); } return 0;
    case WM_LBUTTONUP:      if (example) { example->OnMouseLButtonUp(lParam); } return 0;
    case WM_RBUTTONDOWN:    if (example) { example->OnMouseRButtonDown(lParam); }  return 0;
    case WM_RBUTTONUP:      if (example) { example->OnMouseRButtonUp(lParam); } return 0;
    case WM_MBUTTONDOWN:    if (example) { example->OnMouseMButtonDown(lParam); } return 0;
    case WM_MBUTTONUP:      if (example) { example->OnMouseMButtonUp(lParam); } return 0;
    case WM_MOUSEMOVE:      if (example) { example->OnMouseMove(lParam); } return 0;
    case WM_MOUSEWHEEL:     if (example) { example->OnMouseWheel(wParam); } return 0;

    case WM_PAINT:
        if (example) {
            example->Update();
            example->Render();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

}
