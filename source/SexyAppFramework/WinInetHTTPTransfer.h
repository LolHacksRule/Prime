#ifndef __WININETHTTPTRANSFER_H__
#define __WININETHTTPTRANSFER_H__

#include "CritSect.h"

namespace Sexy
{

class WinInetHTTPTransfer //C++ only.
{
public:
	enum EResult
	{
		RESULT_DONE,
		RESULT_NOT_STARTED,
		RESULT_NOT_COMPLETED,
		RESULT_NOT_FOUND,
		RESULT_HTTP_ERROR,
		RESULT_HTTP_REDIRECT,
		RESULT_ABORTED,
		RESULT_SOCKET_ERROR,
		RESULT_INVALID_ADDR,
		RESULT_CONNECT_FAIL,
		RESULT_DISCONNECTED,
		RESULT_INTERNAL_ERROR,
	};

public:
	WinInetHTTPTransfer();
	WinInetHTTPTransfer(const WinInetHTTPTransfer& rhs);
	WinInetHTTPTransfer& operator=(const WinInetHTTPTransfer& rhs);
	virtual ~WinInetHTTPTransfer();

	bool					SetOutputFile(const std::string& theFilename);
	bool					SetOutputFile(const std::wstring& theFilename);

	void					Get(const std::string& theURL);
	void					Get(const std::string& theBaseURL, const std::string& theRelURL);

	void					Post(const std::string& theURL, const std::string& theParams);
	void					Post(const std::string& theBaseURL, const std::string& theRelURL, const std::string& theParams);
	void					PostMultiPart(const std::string& theURL, const std::string& theParams, const std::string& theSeperator);
	void					PostMultiPart(const std::string& theBaseURL, const std::string& theRelURL, const std::string& theParams, const std::string& theSeparator);

	void					Reset();
	void					Abort();
	void					WaitFor();
	EResult					GetResultCode() const { return mResult; } //56
	std::string				GetContent();
	static void				TransferThreadProcStub(void* theParameter);

protected:
	std::string				GetAbsURL(const std::string& theBaseURL, const std::string& theRelURL);
	void					GetHelper(const std::string& theURL);
	void					PostHelper(const std::string& theURL, const std::string& theParams, const std::string& theSeparator = "");
	void					PrepareTransfer(const std::string& theURL);
	void					StartTransfer();

	void					TransferThreadProc();
	void					Fail(EResult theResult);

private:
	std::string				mSpecifiedBaseURL;
	std::string				mSpecifiedRelURL;
	std::string				mURL;
	std::string				mProto;
	std::string				mUserName;
	std::string				mUserPass;
	std::string				mHost;
	int						mPort;
	std::string				mPath;
	std::string				mAction;
	std::string				mUserAgent;
	std::string				mPostContentType;
	std::string				mPostData;
	FILE* mFP;
	bool					mUsingFile;
	int						mContentLength;
	int						mCurContentLength;
	std::string				mContent;
	bool					mTransferPending;
	bool					mThreadRunning;
	bool					mExiting;
	bool					mAborted;
	EResult					mResult;
	CritSect				mFileCritSection;
};

};

#endif //__WININETHTTPTRANSFER_H__