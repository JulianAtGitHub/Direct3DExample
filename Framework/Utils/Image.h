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
    INLINE uint32_t GetPitch(uint32_t level = 0) { if (level < mMipLevels) { return mHead.mBPP * mSlices[level].mWidth; } return 0; }
    INLINE const void * GetPixels(uint32_t level = 0) const { if (level < mMipLevels) { return mSlices[level].mPixels; } return nullptr; }
    INLINE uint32_t GetPixelsSize(uint32_t level = 0) { if (level < mMipLevels) { return mHead.mBPP * mSlices[level].GetPixelCount(); } return 0; }

    DXGI_FORMAT GetDXGIFormat(void);
    void GetSubResources(std::vector<D3D12_SUBRESOURCE_DATA> &resources);

    Image * BumpMapToNormalMap(float strength = 2.0f, float level = 9.0f);

private:
    enum FilterType {
        Sobel,
        Scharr
    };

    struct Slice {
        Slice(void): mWidth(0), mHeight(0), mPixels(nullptr) { }
        Slice(uint32_t width, uint32_t height, void *pixels): mWidth(width), mHeight(height), mPixels(pixels) { }
        INLINE uint32_t GetPixelCount(void) { return mWidth * mHeight; }
        uint32_t    mWidth;
        uint32_t    mHeight;
        void *      mPixels;
    };

    static Image * CreateBitmapImage(const char *filePath);
    static Image * CreateHDRImage(const char *filePath);
    static Image * CreateCompressedImage(const char *filePath);

    Image(void);

    void GenerateMipmaps(void);
    bool NextMipLevel(const Slice &src, Slice &dst);
    void MergePixels_R8(const Slice &src, Slice &dst, uint32_t dstU, uint32_t dstV, uint32_t widthFactor, uint32_t heightFactor);
    void MergePixels_R8G8(const Slice &src, Slice &dst, uint32_t dstU, uint32_t dstV, uint32_t widthFactor, uint32_t heightFactor);
    void MergePixels_R8G8B8A8(const Slice &src, Slice &dst, uint32_t dstU, uint32_t dstV, uint32_t widthFactor, uint32_t heightFactor);

    void * GetPixel(const Slice &slice, uint32_t u, uint32_t v);
    void BumpMapToNormalMap(const Slice &bump, Slice &normal, float dz, FilterType filter);

    Head                mHead;
    uint32_t            mMipLevels;
    std::vector<Slice>  mSlices;
};

}
