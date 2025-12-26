#ifndef __WINDOWSGRAPHICSDRIVER_H__
#define __WINDOWSGRAPHICSDRIVER_H__

#include "../../../IGraphicsDriver.h"
#include <d3d.h>

namespace Sexy
{
    class SexyAppBase;
    class DeviceImage;
    class Image;
    class MemoryImage;
    class D3DInterface;
    class D3DTester;
    //class DeviceSurfaceDesc; //Do we need?
    //class D3D8Interface; //Do we need?
    //class D3D9Interface; //Do we need?
    //class DXSurface7; //Do we need?
    //class DXSurface8; //Do we need?
    //class DXSurface9; //Do we need?
    //class D3DStateManager; //Do we need?
    class IDXSurface;
    class GraphicsMetrics;

    typedef std::set<DeviceImage*> DDImageSet; //They aren't even trying to hide it

    class WindowsGraphicsDriver : public IGraphicsDriver, protected NativeDisplay //DDInterface on steroids
    {
    public:
        WindowsGraphicsDriver(SexyAppBase* theApp);
        ~WindowsGraphicsDriver();
        bool                    InitGraphicsDriver();
        bool                    Is3D() { return mIs3D; } //52
        int                     GetVersion();
        ulong                   GetRenderModeFlags() { return mRenderModeFlags | 1; } //56
        void                    SetRenderModeFlags(ulong flags) { mRenderModeFlags = flags; } //58
        ERenderMode             GetRenderMode() { return mRenderMode; } //60
        void                    SetRenderMode(ERenderMode inRenderMode);
        std::string             GetRenderModeString(ERenderMode inRenderMode, ulong inRenderModeFlags, bool inIgnoreMode, bool inIgnoreFlags);
        void                    AddDeviceImage(DeviceImage* theDDImage);
        void                    RemoveDeviceImage(DeviceImage* theDDImage);
        void                    Remove3DData(MemoryImage* theImage);
        DeviceImage* GetScreenImage();
        virtual int				Init(HWND theWindow, bool IsWindowed);
        void                    WindowResize(int theWidth, int theHeight);
        bool			        Redraw(Rect* theClipRect = NULL);
        void			        RemapMouse(int& theX, int& theY);

        bool			        SetCursorImage(Image* theImage);
        void			        SetCursorPos(int theCursorX, int theCursorY);
        void                    RemoveShader(const void* theShader) {} //89
        DeviceSurface* CreateDeviceSurface();
        NativeDisplay* GetNativeDisplayInfo();
        CritSect& GetCritSect();
        RenderDevice* GetRenderDevice();
        RenderDevice3D* GetRenderDevice3D();
        GraphicsMetrics& GetMetrics() { return mGraphicsMetrics; } //102

        Ratio                   GetAspectRatio() { return Ratio(mAspect.mDenominator, mAspect.mNumerator); } //105

        int                     GetDisplayWidth() { return mDisplayWidth; } //107

        int                     GetDisplayHeight() { return mDisplayHeight; } //109
        bool                    IsD3D9() const;
        bool                    IsD3D8() const;
        bool                    IsD3D8Or9() const;

        CritSect                mCritSect;

    protected:
        SexyAppBase*            mApp;
        ERenderMode             mRenderMode;
        ulong                   mRenderModeFlags;
        GraphicsMetrics         mGraphicsMetrics;
        RenderDevice3D*         mRenderDevice3D;
        D3DTester*              mD3DTester;
        bool					mIs3D;
        bool                    mWantD3D9;
        bool                    mIsD3D9;
        bool                    mIsD3D8;
        bool                    mIsD3D8Or9;
        bool                    mInRedraw;
        bool                    mInRedrawCursor;
        LPDIRECTDRAW			mDD;
        LPDIRECTDRAW7			mDD7;
        LPDIRECTDRAWSURFACE	    mPrimarySurface;
        LPDIRECTDRAWSURFACE	    mSecondarySurface;
        IUnknown*               mDrawSurface;
        IUnknown*               mWindowScaleBuffers[4];
        int						mWidth;
        int						mHeight;
        Ratio					mAspect;
        int						mDesktopWidth;
        int						mDesktopHeight;
        Ratio					mDesktopAspect;
        bool					mIsWidescreen;
        int						mDisplayWidth;
        int						mDisplayHeight;
        Ratio					mDisplayAspect;
        float                   mFov;
        float                   mNearPlane;
        float                   mFarPlane;
        Rect                    mPresentationRect;
        int                     mFullscreenBits;
        DWORD                   mRefreshRate;
        DWORD                   mMillisecondsPerFrame;
        int                     mScanLineFailCount;
        bool                    mInitialized;
        HWND                    mHWnd;
        bool                    mIsWindowed;
        DeviceImage*            mScreenImage;
        DeviceImage*            mSecondarySurfaceImage;
        DDImageSet              mDDImageSet;
        ulong                   mInitCount;
        int                     mCursorWidth;
        int                     mCursorHeight;
        int                     mNextCursorX;
        int                     mNextCursorY;
        int                     mCursorX;
        int                     mCursorY;
        Image*                  mCursorImage;
        bool                    mHasOldCursorArea;
        LPDIRECTDRAWSURFACE     mOldCursorArea;
        LPDIRECTDRAWSURFACE     mNewCursorArea;
        DeviceImage*            mOldCursorAreaImage;
        DeviceImage*            mNewCursorAreaImage;
        IDXSurface*             mPrimaryDXSurface;
        std::string             mErrorString;
        MeshSet                 mMeshSet;

        bool					CopyBitmap(LPDIRECTDRAWSURFACE theSurface, HBITMAP TheBitmap, int theX, int theY, int theWidth, int theHeight);
        ulong					GetColorRef(ulong theRGB);
        void                    Cleanup();
        void                    CleanupMeshes();
        bool					GotDXError(HRESULT theResult, const char* theContext = "");

        void					RestoreOldCursorAreaFrom(LPDIRECTDRAWSURFACE theSurface, bool adjust);
        void					DrawCursorTo(LPDIRECTDRAWSURFACE theSurface, bool adjust);
        void					MoveCursorTo(LPDIRECTDRAWSURFACE theSurface, bool adjust, int theNewCursorX, int theNewCursorY);

        HRESULT					CreateSurface(LPDDSURFACEDESC2 theDesc, IUnknown** theSurface, void*);
        void					ClearSurface(LPDIRECTDRAWSURFACE theSurface);
        bool					Do3DTest(HWND theHWND);
        Mesh* LoadMesh(const std::string& thePath, MeshListener* theListener);
        void                    AddMesh(Mesh* theMesh);
        void                    RemoveMesh(Mesh* theMesh);

        D3DInterface*           mD3DInterface;
    };
}
#endif //__WINDOWSGRAPHICSDRIVER_H__