#pragma once

#include "Shaders/types.pbr.h"

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
    virtual void OnChar(uint16_t cha);
    virtual void OnMouseLButtonDown(int64_t pos);
    virtual void OnMouseLButtonUp(int64_t pos);
    virtual void OnMouseRButtonDown(int64_t pos);
    virtual void OnMouseRButtonUp(int64_t pos);
    virtual void OnMouseMButtonDown(int64_t pos);
    virtual void OnMouseMButtonUp(int64_t pos);
    virtual void OnMouseMove(int64_t pos);
    virtual void OnMouseWheel(uint64_t param);

private:
    struct AppSettings {
        bool enableSkybox;
        bool showEnvironmentImage;
        bool showIrradianceImage;
        bool showPrefilteredImage;
        bool showBRDFLookupImage;
        bool enableTexture;
        bool enableIBL;
        uint32_t pbrPassState;
    };

    struct Environment {
        Environment(void): original(nullptr), irradiance(nullptr), prefiltered(nullptr) { }
        Render::PixelBuffer *original;
        Render::PixelBuffer *irradiance;
        Render::PixelBuffer *prefiltered;
    };

    static constexpr uint32_t LIGHT_COUNT = 5;
    static constexpr uint32_t ENV_COUNT = 3;
    static std::string WindowTitle;

    void UpdateGUI(float second);

    uint32_t                mWidth;
    uint32_t                mHeight;
    uint32_t                mCurrentFrame;
    uint64_t                mFenceValues[Render::FRAME_COUNT];

    float                   mSpeedX;
    float                   mSpeedZ;
    float                   mSpeedBoost;
    bool                    mIsRotating;
    int64_t                 mLastMousePos;
    int64_t                 mCurrentMousePos;
    uint32_t                mEnvIndex;
    bool                    mDirtyFlag;

    AppSettings             mAppSettings;
    SettingsCB              mSettings;
    LightCB                 mLights[LIGHT_COUNT];

    Render::GPUBuffer      *mLightsBuffer;
    Environment             mEnvTextures[ENV_COUNT];
    Render::PixelBuffer    *mBRDFLookupTexture;
    Render::DescriptorHeap *mTextureHeap;

    Utils::Timer            mTimer;
    Utils::Camera          *mCamera;
    Utils::GUILayer        *mGUI;

    std::vector<PbrDrawable *>  mDrawables;
    uint32_t                    mDrawIndex;
    PbrPass                    *mPbrPass;
    SkyboxPass                 *mSkyboxPass;
};
