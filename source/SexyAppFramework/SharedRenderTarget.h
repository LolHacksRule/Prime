#ifndef __SHAREDRENDERTARGET_H__
#define __SHAREDRENDERTARGET_H__

#include "Common.h"
#include "DeviceImage.h"

namespace Sexy
{
    class SharedRenderTarget
    {
    public:
        class Pool
        {
            struct Entry
            {
                DeviceImage* mImage;
                RenderSurface* mScreenSurface;
                SharedRenderTarget* mLockOwner;
                std::string mLockDebugTag;
            };
        protected:
            std::vector<Entry> mEntries;
        public:
            Pool();
            ~Pool();
            void Acquire(SharedRenderTarget& outTarget, int theWidth, int theHeight, ulong theD3DFlags, const char* debugTag);
            void UpdateEntry(SharedRenderTarget& inTarget);
            void Unacquire(SharedRenderTarget& ioTarget);
            void InvalidateSurfaces();
            void InvalidateDevice();
            std::string GetInfoString();
        };
    protected:
        DeviceImage* mImage;
        RenderSurface* mScreenSurface;
        HANDLE_PTR mLockHandle;
    public:
        SharedRenderTarget();
        ~SharedRenderTarget();
        DeviceImage* Lock(int theWidth, int theHeight, ulong additionalD3DFlags, const char* debugTag);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
        DeviceImage* LockScreenImage(const char* debugTag = "", ulong flags = 0);
#else
        DeviceImage* LockScreenImage(const char* debugTag = "");
#endif
        bool Unlock(); //Correct?
        DeviceImage* GetCurrentLockImage();
    };
}

#endif