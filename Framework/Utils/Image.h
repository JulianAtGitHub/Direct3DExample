#pragma once

namespace Utils {

class Image {
public:
    enum Format {
        Unknown,
        R8,
        R8G8,
        R8G8B8A8,
        R32G32B32_FLOAT,
        R32G32B32A32_FLOAT,
    };

    struct Head {
        Head(void): mWidth(0), mHeight(0), mBPP(0), mFormat(Unknown) { }
        uint32_t            mWidth;
        uint32_t            mHeight;
        uint32_t            mBPP;   // byte per pxiel
        Format              mFormat;
    };

    static Image * CreateFromFile(const char *filePath, bool hdr2ldr = true);

    ~Image(void);

    INLINE const Head & GetHead(void) const { return mHead; }
    INLINE uint32_t GetWidth(void) const { return mHead.mWidth; }
    INLINE uint32_t GetHeight(void) const { return mHead.mHeight; }
    INLINE Format GetFormat(void) const { return mHead.mFormat; }
    INLINE uint32_t GetMipLevels(void) const { return mMipLevels; }
    INLINE uint32_t GetPitch(void) { return mHead.mBPP * mHead.mWidth; }
    INLINE const void * GetPixels(void) const { return mPixels; }

    DXGI_FORMAT GetDXGIFormat(void);

    Image * BumpMapToNormalMap(float strength = 2.0f, float level = 9.0f);

private:
    enum FilterType {
        Sobel,
        Scharr
    };

    static Image * CreateBitmapImage(const char *filePath);
    static Image * CreateHDRImage(const char *filePath);
    static Image * CreateCompressedImage(const char *filePath);

    Image(void);

    void CalculateMipLevles(void);

    void * GetPixel(uint32_t u, uint32_t v);
    void BumpMapToNormalMap(Image &normal, float dz, FilterType filter);

    Head                mHead;
    uint32_t            mMipLevels;
    void               *mPixels;
};

}
