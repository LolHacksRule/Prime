#ifndef __SEHCATHER_H__ //lol
#define __SEHCATHER_H__

#include "Common.h"
#include "HTTPTransfer.h"
#include <imagehlp.h>

namespace Sexy //PC/Mac only?
{

class SexyAppBase;

typedef BOOL (__stdcall * SYMINITIALIZEPROC)(HANDLE, LPSTR, BOOL);

typedef DWORD (__stdcall *SYMSETOPTIONSPROC)(DWORD);

typedef BOOL (__stdcall *SYMCLEANUPPROC)(HANDLE);

typedef LPCSTR (__stdcall *UNDECORATESYMBOLNAMEPROC)(LPCSTR, LPSTR, DWORD, DWORD);

typedef BOOL (__stdcall * STACKWALKPROC)
           ( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
            PREAD_PROCESS_MEMORY_ROUTINE,PFUNCTION_TABLE_ACCESS_ROUTINE,
            PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)(HANDLE, DWORD);

typedef DWORD (__stdcall *SYMGETMODULEBASEPROC)(HANDLE, DWORD);

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL (__stdcall *SYMGETLINEFROMADDR)(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE);

typedef VOID (__stdcall *RTLCAPTURECONTEXTPROC)(PCONTEXT);
typedef WORD (__stdcall *RTLCAPTURESTACKBACKTRACE)(DWORD, DWORD, PVOID*, PDWORD);

typedef BOOL (__stdcall *MINIDUMPWRITEPROC)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

class SEHCatcher 
{
public:	
	static SexyAppBase*		mApp;
	static HFONT			mDialogFont;
	static HFONT			mBoldFont;
	static HWND				mYesButtonWindow;
	static HWND				mNoButtonWindow;
	static HWND				mDebugButtonWindow;
	static HWND				mEditWindow;
	static bool				mHasDemoFile;
	static bool				mDebugError;
	static std::string		mErrorTitle;
	static std::string		mErrorText;
	static std::string		mUserText;
	static std::string		mUploadFileName;
	static std::wstring		mCrashMessage;
	static std::string		mSubmitHost;
	static std::wstring		mSubmitErrorMessage;
	static std::wstring		mSubmitMessage;
	static HMODULE			mImageHelpLib;
	static HMODULE			mKernelLib;	
	static SYMINITIALIZEPROC mSymInitialize;
	static SYMSETOPTIONSPROC mSymSetOptions;
	static UNDECORATESYMBOLNAMEPROC mUnDecorateSymbolName;
	static SYMCLEANUPPROC	mSymCleanup;
	static STACKWALKPROC	mStackWalk;
	static SYMFUNCTIONTABLEACCESSPROC mSymFunctionTableAccess;
	static SYMGETMODULEBASEPROC mSymGetModuleBase;
	static SYMGETSYMFROMADDRPROC mSymGetSymFromAddr;
	static SYMGETLINEFROMADDR mSymGetLineFromAddr;
	static RTLCAPTURECONTEXTPROC mRtlCaptureContext;
	static RTLCAPTURESTACKBACKTRACE mRtlCaptureStackBackTrace;
	static HTTPTransfer		mSubmitReportTransfer;
	static bool				mExiting;
	static bool				mShowUI;
	static bool				mAllowSubmit;
	static bool				mAllowMinidumps;
	static bool				mAllowUltradumps;
	static bool				mDisableCrashHandler;

protected:
	static LPTOP_LEVEL_EXCEPTION_FILTER mPreviousFilter;

public:
	static void				SubmitReportThread(void *theArg);

	static LRESULT CALLBACK SEHWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK SubmitInfoWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static long __stdcall	UnhandledExceptionFilter(LPEXCEPTION_POINTERS lpExceptPtr);	
	static void				DoHandleDebugEvent(LPEXCEPTION_POINTERS lpEP);
	static bool				GetLogicalAddress(void* addr, char* szModule, DWORD len, DWORD& section, DWORD& offset);
	static std::string		GetFilename(const std::string& thePath);
	static void				WriteToFile(const std::string& theErrorText);
	static void				ShowSubmitInfoDialog();
	static void				ShowErrorDialog(const std::string& theErrorTitle, const std::string& theErrorText);	
	static bool				LoadImageHelp();
	static void				UnloadImageHelp();
	static std::string		IntelWalk(PCONTEXT theContext, int theSkipCount);
	static std::string		ImageHelpWalk(PCONTEXT theContext, int theSkipCount);
	static std::string		GetSysInfo();
	static void				GetSymbolsFromMapFile(std::string &theDebugDump);

public:
	SEHCatcher();
	~SEHCatcher();
	void Init();
	bool HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
	void DisableCrashHandler();
};

extern SEHCatcher gSEHCatcher;

}

#endif 