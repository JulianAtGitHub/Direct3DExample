#pragma once

class Example;

class Application {
public:
    Application(uint32_t width, uint32_t height, Example *example);
    ~Application(void);

    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    void ParseCmds(void);

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND mHwnd;
    uint32_t mWidth;
    uint32_t mHeight;
    std::string mTitle;
    Example *mExample;
};
