#pragma once

namespace Utils {

class AnExample {
public:
    AnExample(void): mHwnd(NULL) { }
    virtual ~AnExample(void) { }

    virtual void Init(HWND hwnd) { mHwnd = hwnd; };
    virtual void Update(void) = 0;
    virtual void Render(void) = 0;
    virtual void Destroy(void) = 0;

    virtual void OnKeyDown(uint8_t key) {}
    virtual void OnKeyUp(uint8_t key) {}
    virtual void OnMouseLButtonDown(int64_t pos) {}
    virtual void OnMouseLButtonUp(int64_t pos) {}
    virtual void OnMouseMove(int64_t pos) {}

protected:
    HWND mHwnd;
};

}
