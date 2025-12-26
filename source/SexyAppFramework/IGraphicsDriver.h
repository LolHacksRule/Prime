#ifndef __IGRAPHICSDRIVER_H__
#define __IGRAPHICSDRIVER_H__

#include "NativeDisplay.h"
#include "GraphicsMetrics.h"

namespace Sexy
{
    class IGraphicsDriver
    {
    public:
        enum EResult
        {
            RESULT_OK,
            RESULT_FAIL,
            RESULT_DD_CREATE_FAIL,
            RESULT_SURFACE_FAIL,
            RESULT_EXCLUSIVE_FAIL,
            RESULT_DISPCHANGE_FAIL,
            RESULT_INVALID_COLORDEPTH,
            RESULT_3D_FAIL,
            RESULT_3D_NOTREADY
        };

        enum ERenderMode
        {
            RENDERMODE_Default,
            RENDERMODE_Overdraw,
            RENDERMODE_PseudoOverdraw,
            RENDERMODE_BatchSize,
            RENDERMODE_Wireframe,
            RENDERMODE_WastedOverdraw,
            RENDERMODE_TextureHash,
            RENDERMODE_OverdrawExact,
            RENDERMODE_COUNT,
            RENDERMODE_CYCLE_END = 7
        };

        enum ERenderModeFlags
        {
            RENDERMODEF_NoBatching = 1,
            RENDERMODEF_HalfTris,
            RENDERMODEF_NoDynVB = 4,
            RENDERMODEF_PreventLag = 8,
            RENDERMODEF_NoTriRep = 16,
            RENDERMODEF_NoStretchRectFromTextures = 32,
            RENDERMODEF_HalfPresent = 64,
            RENDERMODEF_USEDBITS = 7
        };

        virtual                     ~IGraphicsDriver() {} //72
        virtual bool                Is3D() = 0;
        virtual int                 GetVersion() = 0;
        virtual ulong               GetRenderModeFlags() = 0;
        virtual void                SetRenderModeFlags(ulong flags) = 0; //Is this ERenderModeFlags
        virtual ERenderMode         GetRenderMode() = 0;
        virtual void                SetRenderMode(ERenderMode inRenderMode) = 0;
        virtual std::string         GetRenderModeString(ERenderMode inRenderMode, ulong inRenderModeFlags, bool inIgnoreMode, bool inIgnoreFlags) = 0;
        virtual void                AddDeviceImage(DeviceImage* theDDImage) = 0;
        virtual void                RemoveDeviceImage(DeviceImage* theDDImage) = 0;
        virtual void                Remove3DData(MemoryImage* theImage) = 0;
        virtual DeviceImage*        GetScreenImage() = 0;
        virtual void                WindowResize(int theWidth, int theHeight) = 0;
        virtual bool                Redraw(Rect* theClipRect) = 0;
        virtual void                RemapMouse(int& theX, int& theY) = 0;
        virtual bool                SetCursorImage(Image* theImage) = 0;
        virtual void                SetCursorPos(int theCursorX, int theCursorY) = 0;
        virtual void                RemoveShader(const void*) = 0;
        virtual DeviceSurface*      CreateDeviceSurface() = 0;
        virtual NativeDisplay*      GetNativeDisplayInfo() = 0;
        virtual RenderDevice*       GetRenderDevice() = 0;
        virtual RenderDevice3D*     GetRenderDevice3D() = 0;
        virtual Ratio               GetAspectRatio() = 0;
        virtual int                 GetDisplayWidth() = 0;
        virtual int                 GetDisplayHeight() = 0;
        virtual CritSect&   		GetCritSect() = 0;
        virtual Mesh*               LoadMesh(const std::string& thePath, MeshListener* theListener) = 0;
        virtual void                AddMesh(Mesh* theMesh) = 0;
        virtual void                RemoveMesh(Mesh* theMesh) = 0;
        virtual GraphicsMetrics&    GetMetrics() = 0;
        static std::string	ResultToString(int theResult) //161-181
        {
            switch (theResult)
            {
            case RESULT_OK:
                return "RESULT_OK";
            case RESULT_FAIL:
                return "RESULT_FAIL";
            case RESULT_DD_CREATE_FAIL:
                return "RESULT_DD_CREATE_FAIL";
            case RESULT_SURFACE_FAIL:
                return "RESULT_SURFACE_FAIL";
            case RESULT_EXCLUSIVE_FAIL:
                return "RESULT_EXCLUSIVE_FAIL";
            case RESULT_DISPCHANGE_FAIL:
                return "RESULT_DISPCHANGE_FAIL";
            case RESULT_INVALID_COLORDEPTH:
                return "RESULT_INVALID_COLORDEPTH";
            default:
                return "RESULT_UNKNOWN";
            }
        };
    };
}

#endif //__IGRAPHICSDRIVER_H__