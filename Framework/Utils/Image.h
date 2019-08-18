#pragma once

namespace Utils {

class Image {
public:
    enum Format {
        Unknown,
        R8_G8_B8_A8,
        R32_G32_B32_FLOAT,
        R32_G32_B32_A32_FLOAT,
    };

    static Image * LoadFromBinary(const void *head, void *pixels);
    static Image * CreateFromFile(const char *filePath, bool hdr2ldr = true);

    ~Image(void);

    INLINE uint32_t GetWidth(void) const { return mWidth; }
    INLINE uint32_t GetHeight(void) const { return mHeight; }
    INLINE uint32_t GetPitch(void) const { return mPitch; }
    INLINE Format GetFormat(void) const { return mFormat; }
    INLINE const void * GetPixels(void) const { return mPixels; }
    INLINE uint32_t GetPixelsSize(void) { return mPitch * mHeight; }

    DXGI_FORMAT GetDXGIFormat(void);

private:
    static Image * CreateBitmapImage(const char *filePath);
    static Image * CreateHDRImage(const char *filePath);
    static Image * CreateCompressedImage(const char *filePath);

    Image(void);

    uint32_t    mWidth;
    uint32_t    mHeight;
    uint32_t    mPitch;
    Format      mFormat;
    void *      mPixels;
};

}
