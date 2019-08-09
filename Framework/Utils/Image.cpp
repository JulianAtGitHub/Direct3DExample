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
    const Image *other = (const Image *)head;
    image->mWidth = other->mWidth;
    image->mHeight = other->mHeight;
    image->mPitch = other->mPitch;
    image->mFormat = other->mFormat;
    image->mPixels = malloc(image->mPitch * image->mHeight);
    memcpy(image->mPixels, pixels, image->mPitch * image->mHeight);

    return image;
}

Image * Image::CreateFromFile(const char *filePath) {
    if (!filePath) {
        return nullptr;
    }

    if (stbi_is_hdr(filePath)) {
        return CreateHDRImage(filePath);
    } else {
        return CreateBitmapImage(filePath);
    }
}

Image * Image::CreateBitmapImage(const char *filePath) {
    stbi_set_flip_vertically_on_load(1);
    int width, height, channels;
    stbi_uc *pixels = stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        return nullptr;
    }

    constexpr uint32_t bpp = 4;

    Image *image = new Image();
    image->mWidth = width;
    image->mHeight = height;
    image->mPitch = width * bpp;
    image->mPixels = malloc(image->mPitch * image->mHeight);
    image->mFormat = R8_G8_B8_A8;
    memcpy(image->mPixels, pixels, image->mPitch * image->mHeight);

    stbi_image_free(pixels);

    return image;
}

Image * Image::CreateHDRImage(const char *filePath) {
    stbi_set_flip_vertically_on_load(0);
    int width, height, channels;
    float *pixels = stbi_loadf(filePath, &width, &height, &channels, STBI_default);
    if (!pixels) {
        return nullptr;
    }

    ASSERT_PRINT((channels == 3 || channels == 4));
    uint32_t bpp = static_cast<uint32_t>(sizeof(float)) * channels;

    Image *image = new Image();
    image->mWidth = width;
    image->mHeight = height;
    image->mPitch = width * bpp;
    image->mPixels = malloc(image->mPitch * image->mHeight);
    if (channels == 3) {
        image->mFormat = R32_G32_B32_FLOAT;
    } else {
        image->mFormat = R32_G32_B32_A32_FLOAT;
    }
    
    memcpy(image->mPixels, pixels, image->mPitch * image->mHeight);

    stbi_image_free(pixels);

    return image;
}


Image * Image::CreateCompressedImage(const char *filePath) {
    return nullptr;
}

Image::Image(void)
: mWidth(0)
, mHeight(0)
, mPitch(0)
, mFormat(Unknown)
, mPixels(nullptr)
{

}

Image::~Image(void) {
    if (mPixels) {
        free(mPixels);
    }
}

DXGI_FORMAT Image::GetDXGIFormat(void) {
    switch (mFormat) {
        case R8_G8_B8_A8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case R32_G32_B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case R32_G32_B32_A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN; 
    }
}

}
