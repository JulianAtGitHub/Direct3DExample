#pragma once

class PbrDrawable;
class PbrPass;
class SkyboxPass;

class PbrExample : public Utils::AnExample {
public:
    PbrExample(void);
    virtual ~PbrExample(void);

    virtual void Init(HWND hwnd);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

    virtual void OnKeyDown(uint8_t key);
    virtual void OnKeyUp(uint8_t key);
    virtual void OnMouseLButtonDown(int64_t pos);
    virtual void OnMouseLButtonUp(int64_t pos);
    virtual void OnMouseMove(int64_t pos);

private:
    static std::string WindowTitle;

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mCurrentFrame;
    uint64_t mFenceValues[Render::FRAME_COUNT];

    float   mSpeedX;
    float   mSpeedZ;
    bool    mIsRotating;
    int64_t mLastMousePos;
    int64_t mCurrentMousePos;

    Render::PixelBuffer    *mEnvTexture;
    Render::DescriptorHeap *mEnvTextureHeap;

    Utils::Timer    mTimer;
    Utils::Camera  *mCamera;
    PbrDrawable    *mSphere;
    PbrPass        *mPbrPass;
    SkyboxPass     *mSkyboxPass;
};
