#include "stdafx.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Utils {

Image * Image::CreateFromFile(const char *filePath, bool hdr2ldr) {
    if (!filePath) {
        return nullptr;
    }

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
    image->mPixels = data;

    stbi_image_free(pixels);

    image->CalculateMipLevles();

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
    image->mPixels = data;

    stbi_image_free(pixels);

    return image;
}

Image * Image::CreateCompressedImage(const char *filePath) {
    return nullptr;
}

Image::Image(void)
: mMipLevels(0)
, mPixels(nullptr)
{

}

Image::~Image(void) {
    if (mPixels) {
        free(mPixels);
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

void Image::CalculateMipLevles(void) {
    uint32_t width = mHead.mWidth;
    uint32_t height = mHead.mHeight;
    while (width > 1 || height > 1) {
        width >>= 1;
        height >>= 1;
        if (width == 0) { width = 1; }
        if (height == 0) { height = 1; }
        ++ mMipLevels;
    }
}

void * Image::GetPixel(uint32_t u, uint32_t v) {
    if (u >= mHead.mWidth || v >= mHead.mHeight) {
        return nullptr;
    }

    uint8_t *pixels = (uint8_t *)mPixels;
    pixels += mHead.mWidth * mHead.mBPP * v + mHead.mBPP * u;

    return pixels;
}

Image * Image::BumpMapToNormalMap(float strength, float level) {
    if (mHead.mFormat != R8 && mHead.mFormat != R8G8 && mHead.mFormat != R8G8B8A8) {
        return nullptr;
    }

    Image *normal = new Image();
    normal->mHead = mHead;
    normal->mHead.mBPP = 4;
    normal->mMipLevels = mMipLevels;

    float dz = 1.0f / strength * (1.0f + std::powf(2.0f, level));
    BumpMapToNormalMap(*normal, dz, Sobel);

    return normal;
}

// ref from https://github.com/cpetry/NormalMap-Online
void Image::BumpMapToNormalMap(Image &normal, float dz, FilterType filter) {
    if (!mPixels) {
        return;
    }

    normal.mHead.mWidth = mHead.mWidth;
    normal.mHead.mHeight = mHead.mHeight;
    normal.mPixels = malloc(normal.mHead.mWidth * normal.mHead.mHeight * 4);

    XMINT2 dimensions = { (int)mHead.mWidth, (int)mHead.mHeight };
    for (int u = 0; u < dimensions.x; ++u) {
        for (int v = 0; v < dimensions.y; ++v) {
            XMINT2 tlv = { u - 1, v - 1 };
            XMINT2 lv  = { u - 1, v     };
            XMINT2 blv = { u - 1, v + 1 };
            XMINT2 tv  = { u,     v - 1 };
            XMINT2 bv  = { u,     v + 1 };
            XMINT2 trv = { u + 1, v - 1 };
            XMINT2 rv  = { u + 1, v     };
            XMINT2 brv = { u + 1, v + 1 };
            tlv = { tlv.x >= 0 ? tlv.x : dimensions.x + tlv.x, tlv.y >= 0 ? tlv.y : dimensions.y + tlv.y };
            tlv = { tlv.x < dimensions.x ? tlv.x : tlv.x - dimensions.x, tlv.y < dimensions.y ? tlv.y : tlv.y - dimensions.y };
            lv  = { lv.x >= 0 ? lv.x : dimensions.x + lv.x, lv.y >= 0 ? lv.y : dimensions.y + lv.y };
            lv  = { lv.x < dimensions.x ? lv.x : lv.x - dimensions.x, lv.y < dimensions.y ? lv.y : lv.y - dimensions.y };
            blv = { blv.x >= 0 ? blv.x : dimensions.x + blv.x, blv.y >= 0 ? blv.y : dimensions.y + blv.y };
            blv = { blv.x < dimensions.x ? blv.x : blv.x - dimensions.x, blv.y < dimensions.y ? blv.y : blv.y - dimensions.y };
            tv  = { tv.x >= 0 ? tv.x : dimensions.x + tv.x, tv.y >= 0 ? tv.y : dimensions.y + tv.y };
            tv  = { tv.x < dimensions.x ? tv.x : tv.x - dimensions.x, tv.y < dimensions.y ? tv.y : tv.y - dimensions.y };
            bv  = { bv.x >= 0 ? bv.x : dimensions.x + bv.x, bv.y >= 0 ? bv.y : dimensions.y + bv.y };
            bv  = { bv.x < dimensions.x ? bv.x : bv.x - dimensions.x, bv.y < dimensions.y ? bv.y : bv.y - dimensions.y };
            trv = { trv.x >= 0 ? trv.x : dimensions.x + trv.x, trv.y >= 0 ? trv.y : dimensions.y + trv.y };
            trv = { trv.x < dimensions.x ? trv.x : trv.x - dimensions.x, trv.y < dimensions.y ? trv.y : trv.y - dimensions.y };
            rv  = { rv.x >= 0 ? rv.x : dimensions.x + rv.x, rv.y >= 0 ? rv.y : dimensions.y + rv.y };
            rv  = { rv.x < dimensions.x ? rv.x : rv.x - dimensions.x, rv.y < dimensions.y ? rv.y : rv.y - dimensions.y };
            brv = { brv.x >= 0 ? brv.x : dimensions.x + brv.x, brv.y >= 0 ? brv.y : dimensions.y + brv.y };
            brv = { brv.x < dimensions.x ? brv.x : brv.x - dimensions.x, brv.y < dimensions.y ? brv.y : brv.y - dimensions.y };
            float tl = (*(uint8_t *)GetPixel(tlv.x, tlv.y)) / 255.0f;
            float l  = (*(uint8_t *)GetPixel( lv.x,  lv.y)) / 255.0f;
            float bl = (*(uint8_t *)GetPixel(blv.x, blv.y)) / 255.0f;
            float t  = (*(uint8_t *)GetPixel( tv.x,  tv.y)) / 255.0f;
            float b  = (*(uint8_t *)GetPixel( bv.x,  bv.y)) / 255.0f;
            float tr = (*(uint8_t *)GetPixel(trv.x, trv.y)) / 255.0f;
            float r  = (*(uint8_t *)GetPixel( rv.x,  rv.y)) / 255.0f;
            float br = (*(uint8_t *)GetPixel(brv.x, brv.y)) / 255.0f;
            float dx = 0.0f;
            float dy = 0.0f;
            if (filter == Sobel) {
                dx = tl + l*2.0f + bl - tr - r*2.0f - br;
                dy = tl + t*2.0f + tr - bl - b*2.0f - br;
            } else {
                dx = tl*3.0f + l*10.0f + bl*3.0f - tr*3.0f - r*10.0f - br*3.0f;
                dy = tl*3.0f + t*10.0f + tr*3.0f - bl*3.0f - b*10.0f - br*3.0f;
            }

            XMFLOAT4 n;
            XMStoreFloat4(&n, XMVector3Normalize({dx * 255.0f, dy * 255.0f, dz}));
            n.x *= 0.5f; n.x += 0.5f;
            n.y *= 0.5f; n.y += 0.5f;

            uint8_t *pixels = (uint8_t *)normal.mPixels;
            pixels += normal.mHead.mWidth * 4 * v + 4 * u;
            pixels[0] = uint8_t(n.x * 255.0f);
            pixels[1] = uint8_t(n.y * 255.0f);
            pixels[2] = uint8_t(n.z * 255.0f);
            pixels[3] = 255;
        }
    }
}

}
