#pragma once

class Example {
public:
    Example(HWND hwnd): mHwnd(hwnd) { }
    virtual ~Example(void) { }

    virtual void Init(void) = 0;
    virtual void Update(void) = 0;
    virtual void Render(void) = 0;
    virtual void Destroy(void) = 0;

    virtual void OnKeyDown(uint8_t /*key*/) {}
    virtual void OnKeyUp(uint8_t /*key*/) {}

protected:
    HWND mHwnd;
};