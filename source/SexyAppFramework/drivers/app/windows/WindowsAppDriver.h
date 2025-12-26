#ifndef __WINDOWSAPPDRIVER_H__
#define __WINDOWSAPPDRIVER_H__

#include "../../../IAppDriver.h"

class HistoryGraph
{
public:
	struct Frame
	{
		struct FrameItem
		{
			double mTimeMS;
			ulong mColor;
		};
		std::vector<FrameItem> mItems;
	};
	std::vector<Frame> mFrames;
	HistoryGraph();
	void NextFrame();
	void AddItem(double inTimeMS, ulong inColor);
};

static HistoryGraph gHistoryGraph;

namespace Sexy
{
	class WindowsGraphicsDriver;

	void SexyAppRun(SexyAppBase* appBase);
	class WindowsAppDriver : public IAppDriver
	{
	private:
		SexyAppBase*			mApp;
		WindowsGraphicsDriver*	mWindowsGraphicsDriver;
	public:
		WindowsAppDriver(SexyAppBase* appBase);
		~WindowsAppDriver();
		bool						InitAppDriver();

		bool						ProcessMessages();//WIP
		bool						ProcessDeferredMessages(bool singleMessage);//WIP
		void						UpdateFTimeAcc();
		virtual bool				Process(bool allowSleep = true);
		virtual bool				DoUpdateFrames();
		virtual void				DoUpdateFramesF(float theFrac);
		virtual void				MakeWindow(); //WIP
		virtual void				EnforceCursor();
		void						Remove3DData(MemoryImage* theMemoryImage);

		static void					LoadingThreadProcStub(void* theArg); //WIP

		// Cursor thread methods
		void						CursorThreadProc(); //TODO
		static void					CursorThreadProcStub(void* theArg); //FIX
		void						StartCursorThread();
		
		void						WaitForLoadingThread();

		void						RestoreScreenResolution();
		void						DoExit(int theCode);

		void						TakeScreenshot();
		void						DumpProgramInfo();
		void						ShowMemoryUsage();

		// Registry helpers
		bool						ConfigRead(const std::string& theValueName, ulong* theType, uchar* theValue, ulong* theLength);
		bool						RegistryReadKey(const std::string& theValueName, ulong* theType, uchar* theValue, ulong* theLength, HKEY theMainKey = HKEY_CURRENT_USER);
		bool						ConfigWrite(const std::string& theValueName, ulong theType, const uchar* theValue, ulong theLength);

		//Public methods
		void						BeginPopup();
		void						EndPopup();
		int							MsgBox(const std::string& theText, const std::string& theTitle = "Message", int theFlags = MB_OK);
		int							MsgBox(const std::wstring& theText, const std::wstring& theTitle = L"Message", int theFlags = MB_OK);
		void						Popup(const std::string& theString);
		void						Popup(const std::wstring& theString);
		virtual void				LogScreenSaverError(const std::string& theError);

		bool						OpenURL(const std::string& theURL, bool shutdownOnOpen = false);
		virtual std::string			GetProductVersion(const std::string& thePath);		

		void						SEHOccured();
		std::string					GetGameSEHInfo();
		void						GetSEHWebParams(DefinesMap* theDefinesMap);
		void						Shutdown();

		void						DoParseCmdLine();
		void						ParseCmdLine(const std::string& theCmdLine);
		void						HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
		virtual void				HandleNotifyGameMessage(int theType, int theParam); // for HWND_BROADCAST of mNotifyGameMessage (0-1000 are reserved for SexyAppBase for theType)
		virtual void				HandleGameAlreadyRunning();

		void						Start();
		void						Init();

		void						StartLoadingThread();
		double						GetLoadingThreadProgress();

		void						CopyToClipboard(const std::string& theString);
		std::string					GetClipboard();

		void						SetCursor(int theCursorNum);
		int							GetCursor();
		void						EnableCustomCursors(bool enabled);
		void						SetCursorImage(int theCursorNum, Image* theImage);

		void						SwitchScreenMode();
		void						SwitchScreenMode(bool wantWindowed);
		void						SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false);

		virtual bool				IsAltKeyUsed(WPARAM wParam);
		bool						KeyDown(int theKey);
		bool						DebugKeyDown(int theKey); //Help
		bool						DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown);
		virtual void				CloseRequestAsync();
		bool						Is3DAccelerated();
		bool						Is3DAccelerationSupported();
		bool						Is3DAccelerationRecommended();
		void						Set3DAcclerated(bool is3D, bool reinit = true); //Check?
		virtual void				Done3dTesting();
		virtual std::string			NotifyCrashHook(); // return file name that you want to upload

		bool						CheckSignature(const Buffer& theBuffer, const std::string& theFileName);
		virtual bool				DrawDirtyStuff(); //TODO
		void						Redraw(Rect* theClipRect); //TODO

		virtual void				ReloadAllResources_DrawStateUpdate(const std::string& theFileName, const std::string& theSubText, float thePct); //TODO
		virtual void				ReloadAllResourcesProc(); //TODO
		static void					ReloadAllResourcesProcStub(void* theArg);
		bool						ReloadAllResources(); //TODO

		// Registry helpers
		bool						ConfigGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys);
		bool						ConfigReadString(const std::string& theValueName, std::string* theString);
		bool						ConfigReadString(const std::string& theValueName, std::wstring* theString);
		bool						ConfigReadInteger(const std::string& theValueName, int* theValue);
		bool						ConfigReadBoolean(const std::string& theValueName, bool* theValue);
		bool						ConfigReadData(const std::string& theValueName, uchar* theValue, ulong* theLength);
		bool						ConfigWriteString(const std::string& theValueName, const std::string& theString);
		bool						ConfigWriteString(const std::string& theValueName, const std::wstring& theString);
		bool						ConfigWriteInteger(const std::string& theValueName, int theValue);
		bool						ConfigWriteBoolean(const std::string& theValueName, bool theValue);
		bool						ConfigWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength);
		bool						ConfigEraseKey(const SexyString& theKeyName);
		void						ConfigEraseValue(const SexyString& theValueName);

		// File access methods
		bool						WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer);
		bool						ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo = false);//TODO
		bool						WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen); //TODO

		// Misc methods
		virtual void				DoMainLoop();
		bool						UpdateAppStep(bool* updated);
		int							InitGraphicsInterface(); //TODO
		void						ClearUpdateBacklog(bool relaxForASecond = false);
		bool						IsScreenSaver();
		virtual bool				AppCanRestore();
		static LRESULT CALLBACK		WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//TODO
		virtual bool				OverrideWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& theResult);

		void						RehupFocus();
		void						ClearKeysDown();

		DeviceImage*				GetOptimizedImage(const std::string& theFileName, bool commitBits = true, bool allowTriReps = true);
		bool						ShouldPauseUpdates() { return mApp->mMinimized; } //167
		Reflection::CRefSymbolDb*	GetReflection(); //TODO? Probably.
		static bool					ScreenSaverWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& theResult);//TODO
	};
	//typedef std::list<MSG> WindowsMessageList; //This is in SAB since SAB calls the variables
}
#endif //__WINDOWSAPPDRIVER_H__