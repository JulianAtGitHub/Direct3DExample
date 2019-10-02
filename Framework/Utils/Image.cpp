#include "stdafx.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Utils {

Image * Image::LoadFromBinary(const void *head, void *pixels) {
    if (!head || !pixels) {
        return nullptr;
    }

    Image *image = new Image();
    const Head *other = (const Head *)head;
    image->mHead = *other;
    image->mMipLevels = 1;
    // copy pixels
    uint32_t dataSize = image->mHead.mBPP * image->mHead.mWidth * image->mHead.mHeight;
    void *data = malloc(dataSize);
    memcpy(data, pixels, dataSize);
    image->mSlices.push_back(Slice(image->mHead.mWidth, image->mHead.mHeight, data));

    image->GenerateMipmaps();

    return image;
}

Image * Image::CreateFromFile(const char *filePath, bool hdr2ldr) {
    if (!filePath) {
        return nullptr;
    }

    stbi_set_flip_vertically_on_load(1);

    if (!hdr2ldr && stbi_is_hdr(filePath)) {
        return CreateHDRImage(filePath);
    } else {
        return CreateBitmapImage(filePath);
    }
}

Image * Image::CreateBitmapImage(const char *filePath) {
    int width, height, channels;
    stbi_uc *pixels = stbi_load(filePath, &width, &height, &channels, STBI_default);
    if (!pixels) {
        return nullptr;
    }

    if (channels == 3) {
        stbi_image_free(pixels);
        pixels = stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha);
        channels = 4;
    }

    Image *image = new Image();
    image->mHead.mWidth = width;
    image->mHead.mHeight = height;
    image->mHead.mBPP = channels;
    image->mMipLevels = 1;
    switch (channels) {
        case 1: image->mHead.mFormat = R8; break;
        case 2: image->mHead.mFormat = R8G8; break;
        default: image->mHead.mFormat = R8G8B8A8; break;
    }

    uint32_t dataSize = image->mHead.mBPP * image->mHead.mWidth * image->mHead.mHeight;
    void *data = malloc(dataSize);
    memcpy(data, pixels, dataSize);
    image->mSlices.push_back(Slice(width, height, data));

    stbi_image_free(pixels);

    image->GenerateMipmaps();

    return image;
}

Image * Image::CreateHDRImage(const char *filePath) {
    int width, height, channels;
    float *pixels = stbi_loadf(filePath, &width, &height, &channels, STBI_default);
    if (!pixels) {
        return nullptr;
    }

    ASSERT_PRINT((channels == 3 || channels == 4));

    Image *image = new Image();
    image->mHead.mWidth = width;
    image->mHead.mHeight = height;
    image->mHead.mBPP = static_cast<uint32_t>(sizeof(float)) * channels;
    image->mMipLevels = 1;
    if (channels == 3) {
        image->mHead.mFormat = R32G32B32_FLOAT;
    } else {
        image->mHead.mFormat = R32G32B32A32_FLOAT;
    }

    uint32_t dataSize = image->mHead.mBPP * image->mHead.mWidth * image->mHead.mHeight;
    void *data = malloc(dataSize);
    memcpy(data, pixels, dataSize);
    image->mSlices.push_back(Slice(width, height, data));

    stbi_image_free(pixels);

    return image;
}

Image * Image::CreateCompressedImage(const char *filePath) {
    return nullptr;
}

Image::Image(void)
: mMipLevels(0)
{

}

Image::~Image(void) {
    for (auto slice : mSlices) {
        free(slice.mPixels);
    }
}

DXGI_FORMAT Image::GetDXGIFormat(void) {
    switch (mHead.mFormat) {
        case R8:
            return DXGI_FORMAT_R8_UNORM;
        case R8G8:
            return DXGI_FORMAT_R8G8_UNORM;
        case R8G8B8A8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN; 
    }
}

void Image::GetSubResources(std::vector<D3D12_SUBRESOURCE_DATA> &resources) {
    resources.clear();
    resources.reserve(mMipLevels);
    for (auto slice : mSlices) {
        resources.push_back({ slice.mPixels, slice.mWidth * mHead.mBPP, slice.mWidth * mHead.mBPP * slice.mHeight });
    }
}

void Image::GenerateMipmaps(void) {
    Slice next;
    while (NextMipLevel(mSlices[mMipLevels - 1], next)) {
        ++ mMipLevels;
        mSlices.push_back(next);
    }
}

bool Image::NextMipLevel(const Slice &src, Slice &dst) {
    if (((src.mWidth >> 1) == 0 && (src.mHeight >> 1) == 0) || src.mPixels == nullptr) {
        return false;
    }

    bool isHalfWidth = (src.mWidth > 1);
    bool isHalfHeight = (src.mHeight > 1);

    dst.mWidth = isHalfWidth ? src.mWidth >> 1 : src.mWidth;
    dst.mHeight = isHalfHeight ? src.mHeight >> 1 : src.mHeight;
    dst.mPixels = malloc(mHead.mBPP * dst.mWidth * dst.mHeight);

    for (uint32_t i = 0; i < dst.mWidth; ++i) {
        for (uint32_t j = 0; j < dst.mHeight; ++j) {
            switch (mHead.mFormat) {
                case R8: MergePixels_R8(src, dst, i, j, isHalfWidth ? 2 : 1, isHalfHeight ? 2 : 1); break;
                case R8G8: MergePixels_R8G8(src, dst, i, j, isHalfWidth ? 2 : 1, isHalfHeight ? 2 : 1); break;
                case R8G8B8A8: MergePixels_R8G8B8A8(src, dst, i, j, isHalfWidth ? 2 : 1, isHalfHeight ? 2 : 1); break;
                default: break;
            }
        }
    }

    return true;
}

void Image::MergePixels_R8(const Slice &src, Slice &dst, uint32_t dstU, uint32_t dstV, uint32_t widthFactor, uint32_t heightFactor) {
    const uint8_t *srcPixel = (const uint8_t *)src.mPixels + (dstV * heightFactor * src.mWidth + dstU * widthFactor) * mHead.mBPP;
    uint8_t *dstPixel = (uint8_t *)dst.mPixels + (dstV * dst.mWidth + dstU) * mHead.mBPP;
    
    uint32_t r = 0;
    for (uint32_t i = 0; i < widthFactor; ++i) {
        for (uint32_t j = 0; j < heightFactor; ++j) {
            r += *(srcPixel + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
        }
    }

    r /= (widthFactor * heightFactor);
    *dstPixel = (uint8_t)r;
}

void Image::MergePixels_R8G8(const Slice &src, Slice &dst, uint32_t dstU, uint32_t dstV, uint32_t widthFactor, uint32_t heightFactor) {
    const uint8_t *srcPixel = (const uint8_t *)src.mPixels + (dstV * heightFactor * src.mWidth + dstU * widthFactor) * mHead.mBPP;
    uint8_t *dstPixel = (uint8_t *)dst.mPixels + (dstV * dst.mWidth + dstU) * mHead.mBPP;

    uint32_t r = 0;
    uint32_t g = 0;
    for (uint32_t i = 0; i < widthFactor; ++i) {
        for (uint32_t j = 0; j < heightFactor; ++j) {
            r += *(srcPixel + 0 + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
            g += *(srcPixel + 1 + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
        }
    }

    r /= (widthFactor * heightFactor);
    g /= (widthFactor * heightFactor);
    *(dstPixel + 0) = (uint8_t)r;
    *(dstPixel + 1) = (uint8_t)g;
}

void Image::MergePixels_R8G8B8A8(const Slice &src, Slice &dst, uint32_t dstU, uint32_t dstV, uint32_t widthFactor, uint32_t heightFactor) {
    const uint8_t *srcPixel = (const uint8_t *)src.mPixels + (dstV * heightFactor * src.mWidth + dstU * widthFactor) * mHead.mBPP;
    uint8_t *dstPixel = (uint8_t *)dst.mPixels + (dstV * dst.mWidth + dstU) * mHead.mBPP;

    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t a = 0;
    for (uint32_t i = 0; i < widthFactor; ++i) {
        for (uint32_t j = 0; j < heightFactor; ++j) {
            r += *(srcPixel + 0 + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
            g += *(srcPixel + 1 + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
            b += *(srcPixel + 2 + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
            a += *(srcPixel + 3 + j * src.mWidth * mHead.mBPP + i * mHead.mBPP);
        }
    }

    r /= (widthFactor * heightFactor);
    g /= (widthFactor * heightFactor);
    b /= (widthFactor * heightFactor);
    a /= (widthFactor * heightFactor);
    *(dstPixel + 0) = (uint8_t)r;
    *(dstPixel + 1) = (uint8_t)g;
    *(dstPixel + 2) = (uint8_t)b;
    *(dstPixel + 3) = (uint8_t)a;
}

}
