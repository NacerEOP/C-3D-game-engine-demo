#include "stb_image.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus")
using namespace Gdiplus;

extern "C" {

// Provide fallback macro definitions when using the minimal header wrapper
#ifndef STBIDEF
#define STBIDEF
#endif
#ifndef STBI_MALLOC
#include <stdlib.h>
#define STBI_MALLOC(sz) malloc((sz))
#define STBI_FREE(p) free((p))
#endif


static struct GdiplusStarter {
    ULONG_PTR token;
    GdiplusStarter() { GdiplusStartupInput input; GdiplusStartup(&token, &input, NULL); }
    ~GdiplusStarter() { GdiplusShutdown(token); }
} _gdiStarter;

STBIDEF unsigned char* stbi_load_from_memory(const unsigned char* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels)
{
    if (!buffer || len <= 0) return NULL;

    HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)len);
    if (!hMem) return NULL;
    void* p = ::GlobalLock(hMem);
    if (!p) { ::GlobalFree(hMem); return NULL; }
    memcpy(p, buffer, len);
    ::GlobalUnlock(hMem);

    IStream* stream = NULL;
    if (CreateStreamOnHGlobal(hMem, TRUE, &stream) != S_OK) {
        ::GlobalFree(hMem);
        return NULL;
    }

    Bitmap* bmp = Bitmap::FromStream(stream);
    if (!bmp) {
        stream->Release();
        return NULL;
    }
    Status st = bmp->GetLastStatus();
    if (st != Ok) {
        delete bmp;
        stream->Release();
        return NULL;
    }

    int width = bmp->GetWidth();
    int height = bmp->GetHeight();
    if (x) *x = width;
    if (y) *y = height;

    // We'll return 4 components (RGBA) to match common usage
    int out_comps = 4;
    if (channels_in_file) *channels_in_file = out_comps;

    BitmapData bd;
    Rect rect(0, 0, width, height);
    // Force 32bpp ARGB so we can read pixels consistently
    st = bmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bd);
    if (st != Ok) {
        delete bmp;
        stream->Release();
        return NULL;
    }

    int stride = bd.Stride;
    unsigned char* src = (unsigned char*)bd.Scan0;
    size_t outSize = (size_t)width * (size_t)height * (size_t)out_comps;
    unsigned char* out = (unsigned char*)STBI_MALLOC(outSize);
    if (!out) {
        bmp->UnlockBits(&bd);
        delete bmp;
        stream->Release();
        return NULL;
    }

    // GDI+ gives PixelFormat32bppARGB which is in memory as BGRA (little-endian)
    for (int row = 0; row < height; ++row) {
        unsigned char* srow = src + (size_t)row * stride;
        unsigned char* drow = out + (size_t)row * width * out_comps;
        for (int col = 0; col < width; ++col) {
            unsigned char b = srow[col * 4 + 0];
            unsigned char g = srow[col * 4 + 1];
            unsigned char r = srow[col * 4 + 2];
            unsigned char a = srow[col * 4 + 3];
            drow[col * 4 + 0] = r;
            drow[col * 4 + 1] = g;
            drow[col * 4 + 2] = b;
            drow[col * 4 + 3] = a;
        }
    }

    bmp->UnlockBits(&bd);
    delete bmp;
    stream->Release();

    // If a specific desired channel count was requested, convert
    if (desired_channels && desired_channels > 0 && desired_channels != out_comps) {
        // Only support dropping alpha or expanding to RGB/RGBA simply
        int req = desired_channels;
        size_t reqSize = (size_t)width * height * req;
        unsigned char* reqBuf = (unsigned char*)STBI_MALLOC(reqSize);
        if (!reqBuf) {
            STBI_FREE(out);
            return NULL;
        }
        for (int i = 0; i < width * height; ++i) {
            unsigned char r = out[i*4+0];
            unsigned char g = out[i*4+1];
            unsigned char b = out[i*4+2];
            unsigned char a = out[i*4+3];
            if (req == 1) {
                reqBuf[i] = (unsigned char)((299*r + 587*g + 114*b) / 1000);
            } else if (req == 2) {
                reqBuf[i*2+0] = (unsigned char)((299*r + 587*g + 114*b) / 1000);
                reqBuf[i*2+1] = a;
            } else if (req == 3) {
                reqBuf[i*3+0] = r;
                reqBuf[i*3+1] = g;
                reqBuf[i*3+2] = b;
            } else if (req == 4) {
                reqBuf[i*4+0] = r;
                reqBuf[i*4+1] = g;
                reqBuf[i*4+2] = b;
                reqBuf[i*4+3] = a;
            }
        }
        STBI_FREE(out);
        out = reqBuf;
        if (channels_in_file) *channels_in_file = req;
    }

    return out;
}

STBIDEF void stbi_image_free(void* retval_from_stbi_load)
{
    if (retval_from_stbi_load) STBI_FREE(retval_from_stbi_load);
}

} // extern "C"

#else
// Non-Windows stub: fall back to malloc failure so loadTextureFromMemory falls back to placeholder
extern "C" {
STBIDEF unsigned char* stbi_load_from_memory(const unsigned char* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels)
{
    (void)buffer; (void)len; (void)x; (void)y; (void)channels_in_file; (void)desired_channels;
    return NULL;
}
STBIDEF void stbi_image_free(void* retval_from_stbi_load) { (void)retval_from_stbi_load; }
}
#endif
