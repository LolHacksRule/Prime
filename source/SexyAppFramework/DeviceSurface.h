#ifndef __DEVICESURFACE_H__
#define __DEVICESURFACE_H__

#include "DeviceImage.h"
#include <ddraw.h>

namespace Sexy //This class and structs doesn't exist on EAMT or iPhoneOS.
{
    /*struct _DEVICEPIXELFORMAT //Ok now how does this work since this is multiplatform, not present in EAMT or Transmension
    {
    public:
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwFourCC;
        union {
            unsigned int dwRGBBitCount;
            unsigned int dwYUVBitCount;
            unsigned int dwZBufferBitDepth;
            unsigned int dwAlphaBitDepth;
            unsigned int dwLuminanceBitCount;
            unsigned int dwBumpBitCount;
            unsigned int dwPrivateFormatBitCount;
        };
        union {
            unsigned int dwRBitMask;
            unsigned int dwYBitMask;
            unsigned int dwStencilBitDepth;
            unsigned int dwLuminanceBitMask;
            unsigned int dwBumpDuBitMask;
            unsigned int dwOperations;
        };
        union {
            unsigned int dwGBitMask;
            unsigned int dwUBitMask;
            unsigned int dwZBitMask;
            unsigned int dwBumpDvBitMask;
            struct
            {
                uint16_t    wFlipMSTypes;       // Multisample methods supported via flip for this D3DFORMAT
                uint16_t    wBltMSTypes;        // Multisample methods supported via blt for this D3DFORMAT
            } MultiSampleCaps;
        };
        union {
            unsigned int dwBBitMask;
            unsigned int dwVBitMask;
            unsigned int dwStencilBitMask;
            unsigned int dwBumpLuminanceBitMask;
        };
        union {
            unsigned int dwRGBAlphaBitMask;
            unsigned int dwYUVAlphaBitMask;
            unsigned int dwLuminanceAlphaBitMask;
            unsigned int dwRGBZBitMask;
            unsigned int dwYUVZBitMask;
        };
    };
    struct _DEVICESURFACEDESC
    {
    public:
        unsigned int dwFlags;
        unsigned int dwHeight;
        unsigned int dwWidth;
        unsigned int lPitch;
        void* lpSurface;
        DevicePixelFormat ddpfPixelFormat;
    };*/
    //struct _DEVICEPIXELFORMAT : DDPIXELFORMAT {}; //Really stupid
    //struct _DEVICESURFACEDESC : DDSURFACEDESC {}; //Really stupid
    typedef DDPIXELFORMAT _DEVICEPIXELFORMAT;
#define NONAMELESSUNION
    typedef DDSURFACEDESC _DEVICESURFACEDESC;
#undef NONAMELESSUNION

    typedef _DEVICEPIXELFORMAT DevicePixelFormat; //Official typedefs
    typedef _DEVICESURFACEDESC DeviceSurfaceDesc; //Official typedefs

    class DeviceSurface
    {
    public:
        virtual bool Lock(DeviceSurfaceDesc* theDesc) = 0;
        virtual void Unlock(void* theParam) = 0;
        virtual int GetVersion() const = 0;
        virtual bool GenerateDeviceSurface(DeviceImage* theImage) = 0;
        virtual bool HasSurface() const = 0;
        virtual void* GetSurfacePtr() const = 0;
        virtual void AddRef() = 0;
        virtual void Release() = 0;
        virtual unsigned long* GetBits(DeviceImage* theImage) = 0;
        virtual void SetSurface(void* theSurface) = 0;
        virtual void GetDimensions(int* theWidth, int* theHeight) = 0;
        DWORD mImageFlags;
        //DeviceSurface();
        virtual ~DeviceSurface() {}; //80
    };
}

//typedef Sexy::_DEVICEPIXELFORMAT DevicePixelFormat;
//typedef Sexy::_DEVICESURFACEDESC DeviceSurfaceDesc;

#endif //__DEVICESURFACE_H__