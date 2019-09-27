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
    image->mWidth = width;
    image->mHeight = height;
    image->mPitch = width * channels;
    image->mPixels = malloc(image->mPitch * image->mHeight);
    switch (channels) {
        case 1: image->mFormat = R8; break;
        case 2: image->mFormat = R8_G8; break;
        default: image->mFormat = R8_G8_B8_A8; break;
    }
    memcpy(image->mPixels, pixels, image->mPitch * image->mHeight);

    stbi_image_free(pixels);

    return image;
}

Image * Image::CreateHDRImage(const char *filePath) {
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

Image * Image::CombineImages(const char *aoFile, const char *roughnessFile, const char *metalicFile) {
    if (!aoFile || !roughnessFile || !metalicFile) {
        return nullptr;
    }

    int channels;
    int aoW, aoH;
    int roughnessW, roughnessH;
    int metalicW, metalicH;

    stbi_uc *aoPixels = stbi_load(aoFile, &aoW, &aoH, &channels, STBI_rgb_alpha);
    stbi_uc *roughnessPixels = stbi_load(roughnessFile, &roughnessW, &roughnessH, &channels, STBI_rgb_alpha);
    stbi_uc *metalicPixels = stbi_load(metalicFile, &metalicW, &metalicH, &channels, STBI_rgb_alpha);

    ASSERT_PRINT((aoPixels && roughnessPixels && metalicPixels));
    ASSERT_PRINT((aoW == roughnessW && roughnessW == metalicW && aoH == roughnessH && roughnessH == metalicH));

    constexpr uint32_t bpp = 4;
    uint32_t width = aoW;
    uint32_t height = aoH;

    Image *image = new Image();
    image->mWidth = width;
    image->mHeight = height;
    image->mPitch = width * bpp;
    image->mPixels = malloc(image->mPitch * image->mHeight);
    image->mFormat = R8_G8_B8_A8;

    stbi_uc *pixels = (stbi_uc *)(image->mPixels);
    for (uint32_t i = 0; i < width; ++i) {
        for (uint32_t j = 0; j < height; ++j) {
            uint32_t offset = width * bpp * i + bpp * j;
            pixels[offset + 0] = aoPixels[offset];
            pixels[offset + 1] = roughnessPixels[offset];
            pixels[offset + 2] = metalicPixels[offset];
            pixels[offset + 3] = 0;
        }
    }

    stbi_image_free(aoPixels);
    stbi_image_free(roughnessPixels);
    stbi_image_free(metalicPixels);

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
        case R8:
            return DXGI_FORMAT_R8_UNORM;
        case R8_G8:
            return DXGI_FORMAT_R8G8_UNORM;
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
