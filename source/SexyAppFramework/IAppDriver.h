#ifndef __IAPPDRIVER_H__
#define __IAPPDRIVER_H__

#include "SexyAppBase.h"

namespace Sexy
{
    class IAppDriver
    {
    public:
        static IAppDriver* CreateAppDriver(SexyAppBase* appBase);
        //IAppDriver();
        virtual ~IAppDriver() {} //53
        virtual bool            InitAppDriver() = 0;
        virtual void            Start() = 0;
        virtual void            Init() = 0;
        virtual bool            UpdateAppStep(bool* updated) = 0; //?
        virtual void            ClearUpdateBacklog(bool relaxForASecond) = 0;
        virtual void            Shutdown() = 0;
        virtual void            DoExit(int theCode) = 0;
        virtual void            Remove3DData(MemoryImage* theImage) = 0;
        virtual void			BeginPopup() = 0;
        virtual void			EndPopup() = 0;
        virtual int				MsgBox(const std::string& theText, const std::string& theTitle = "Message", int theFlags = MB_OK) = 0;
        virtual int				MsgBox(const std::wstring& theText, const std::wstring& theTitle = L"Message", int theFlags = MB_OK) = 0;
        virtual void			Popup(const std::string& theString) = 0;
        virtual void			Popup(const std::wstring& theString) = 0;
        virtual bool			OpenURL(const std::string& theURL, bool shutdownOnOpen = false) = 0;
        virtual std::string     GetGameSEHInfo() = 0;

        virtual void            SEHOccured() {} //Lol | 82
        virtual void			GetSEHWebParams(DefinesMap* theDefinesMap) {} //83

#ifdef USE_LATEST_CODE //iOS
        virtual bool			IsSystemUIShowing() = 0;
        virtual void			ReadFromConfig() = 0;
        virtual void			WriteToConfig() = 0;
        virtual bool			IsUIOrientationAllowed() = 0;
        virtual UI_ORIENTATION	GetUIOrientation() = 0;
#endif

        virtual void            DoParseCmdLine() = 0;
        virtual void			ParseCmdLine(const std::string& theCmdLine) = 0;
        virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) = 0;
        virtual void            StartLoadingThread() = 0;
        virtual double          GetLoadingThreadProgress() = 0;
        virtual void			CopyToClipboard(const std::string& theString) = 0;
        virtual std::string     GetClipboard() = 0;
        virtual void            SetCursor(int theCursorNum) = 0;
        virtual int             GetCursor() = 0;
        virtual void            EnableCustomCursors(bool enabled) = 0;
        virtual void            SetCursorImage(int theCursorNum, Image* theImage) = 0;
        virtual void            SwitchScreenMode() = 0;
        virtual void            SwitchScreenMode(bool wantWindowed) = 0;
        virtual void            SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) = 0;
        virtual bool            KeyDown(int theKey) = 0;
        virtual bool            DebugKeyDown(int theKey) = 0;
        virtual bool            DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown) = 0;
        virtual bool            Is3DAccelerated() = 0;
        virtual bool            Is3DAccelerationSupported() = 0;
        virtual bool            Is3DAccelerationRecommended() = 0;
        virtual void            Set3DAcclerated(bool is3D, bool reinit = true) = 0; //Lol
        virtual bool			CheckSignature(const Buffer& theBuffer, const std::string& theFileName) = 0;
        virtual bool            ReloadAllResources() = 0;
        virtual bool            ConfigGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys) = 0;
        virtual bool            ConfigReadString(const std::string& theValueName, std::wstring* theString) = 0;
        virtual bool            ConfigReadString(const std::string& theValueName, std::string* theString) = 0;
        virtual bool            ConfigReadInteger(const std::string& theValueName, int* theValue) = 0;
        virtual bool            ConfigReadBoolean(const std::string& theValueName, bool* theValue) = 0;
        virtual bool			ConfigReadData(const std::string& theValueName, uchar* theValue, ulong* theLength) = 0;
        virtual bool            ConfigWriteString(const std::string& theValueName, const std::wstring& theString) = 0;
        virtual bool			ConfigWriteString(const std::string& theValueName, const std::string& theString) = 0;
        virtual bool            ConfigWriteInteger(const std::string& theValueName, int theValue) = 0;
        virtual bool            ConfigWriteBoolean(const std::string& theValueName, bool theValue) = 0;
        virtual bool            ConfigWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength) = 0;
        virtual bool            ConfigEraseKey(const std::wstring& theKeyName) = 0;
        virtual void            ConfigEraseValue(const std::wstring& theValueName) = 0;
        virtual bool            WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer) = 0;
        virtual bool            ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo = false) = 0; //UNICODE
        virtual bool            WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen) = 0;
        virtual DeviceImage*    GetOptimizedImage(const std::string& theFileName, bool commitBits, bool allowTriReps) = 0;
        virtual bool            ShouldPauseUpdates() = 0;
        virtual Reflection::CRefSymbolDb*   GetReflection() = 0;
    };
}
#endif //__IAPPDRIVER_H__