#include "WinInetHTTPTransfer.h"
#include "AutoCrit.h"
#include <wininet.h>

using namespace Sexy;

WinInetHTTPTransfer::WinInetHTTPTransfer() //14-19
{
	mResult = RESULT_NOT_STARTED;
	mThreadRunning = false;
	mFP = NULL;
	mUsingFile = true;
}

WinInetHTTPTransfer::WinInetHTTPTransfer(const WinInetHTTPTransfer& rhs) : //45-46
	mSpecifiedBaseURL(rhs.mSpecifiedBaseURL),
	mSpecifiedRelURL(rhs.mSpecifiedRelURL),
	mURL(rhs.mURL),
	mProto(rhs.mProto),
	mUserName(rhs.mUserName),
	mUserPass(rhs.mUserPass),
	mHost(rhs.mHost),
	mPort(rhs.mPort),
	mPath(rhs.mPath),
	mAction(rhs.mAction),
	mUserAgent(rhs.mUserAgent),
	mContent(rhs.mContent),
	mContentLength(rhs.mContentLength),
	mCurContentLength(rhs.mCurContentLength),
	mTransferPending(rhs.mTransferPending),
	mExiting(rhs.mExiting),
	mAborted(rhs.mAborted),
	mResult(rhs.mResult)
{
}

WinInetHTTPTransfer& WinInetHTTPTransfer::operator=(const WinInetHTTPTransfer& rhs) //51-83
{
	if (!mThreadRunning && !rhs.mThreadRunning)
		return *this;

	mSpecifiedBaseURL = rhs.mSpecifiedBaseURL;
	mSpecifiedRelURL = rhs.mSpecifiedRelURL;
	mURL = rhs.mURL;
	mProto = rhs.mProto;
	mUserName = rhs.mUserName;
	mUserPass = rhs.mUserPass;
	mHost = rhs.mHost;
	mPort = rhs.mPort;
	mPath = rhs.mPath;
	mAction = rhs.mAction;
	mUserAgent = rhs.mUserAgent;

	if (mFP)
		fclose(mFP);

	mFP = NULL;
	mUsingFile = false;
	mContent = rhs.mContent;
	mContentLength = rhs.mContentLength;
	mCurContentLength = rhs.mCurContentLength;
	mTransferPending = rhs.mTransferPending;
	mThreadRunning = rhs.mThreadRunning;
	mExiting = rhs.mExiting;
	mAborted = rhs.mAborted;
	mResult = rhs.mResult;
}

WinInetHTTPTransfer::~WinInetHTTPTransfer() //88-94
{
	Abort();
	while (mThreadRunning)
	{		
		Sleep(1);
	}
}

bool WinInetHTTPTransfer::SetOutputFile(const std::string& theFilename) //99-108
{
	AutoCrit lock(mFileCritSection);

	if (mFP)
		fclose(mFP);
	mFP = fopen(theFilename.c_str(), "w+b");
	if (mFP)
		mUsingFile = true;
	return mFP != NULL;
}

bool WinInetHTTPTransfer::SetOutputFile(const std::wstring& theFilename) //113-122
{
	AutoCrit lock(mFileCritSection);

	if (mFP)
		fclose(mFP);
	mFP = _wfopen(theFilename.c_str(), L"w+b");
	if (mFP)
		mUsingFile = true;
	return mFP != NULL;
}

void WinInetHTTPTransfer::Get(const std::string& theURL) //127-131
{
	mSpecifiedBaseURL = "";
	mSpecifiedRelURL = theURL;
	GetHelper(theURL);
}

void WinInetHTTPTransfer::Get(const std::string& theBaseURL, const std::string& theRelURL) //136-141
{
	mSpecifiedBaseURL = theBaseURL;
	mSpecifiedRelURL = theRelURL;

	GetHelper(GetAbsURL(theBaseURL, theRelURL));
}

void WinInetHTTPTransfer::Post(const std::string& theURL, const std::string& theParams) //146-150
{
	mSpecifiedBaseURL = "";
	mSpecifiedRelURL = theURL;
	PostHelper(theURL, theParams);
}

void WinInetHTTPTransfer::Post(const std::string& theBaseURL, const std::string& theRelURL, const std::string& theParams) //155-160
{
	mSpecifiedBaseURL = theBaseURL;
	mSpecifiedRelURL = theRelURL;

	PostHelper(GetAbsURL(theBaseURL, theRelURL), theParams);
}

void WinInetHTTPTransfer::PostMultiPart(const std::string& theURL, const std::string& theParams, const std::string& theSeparator) //165-169
{
	mSpecifiedBaseURL = "";
	mSpecifiedRelURL = theURL;
	PostHelper(theURL, theParams, theSeparator.c_str());
}

void WinInetHTTPTransfer::PostMultiPart(const std::string& theBaseURL, const std::string& theRelURL, const std::string& theParams, const std::string& theSeparator) //174-179
{
	mSpecifiedBaseURL = theBaseURL;
	mSpecifiedRelURL = theRelURL;
	PostHelper(GetAbsURL(theBaseURL, theRelURL), theParams, theSeparator.c_str());
}

void WinInetHTTPTransfer::Reset() //184-205
{
	if (mThreadRunning)
	{
		Abort();
		WaitFor();
	}

	mResult = RESULT_NOT_STARTED;
	mContent.erase();
	mExiting = false;
	mAborted = false;
	mURL.erase();
	mProto.erase();
	mUserName.erase();
	mUserPass.erase();
	mHost.erase();
	mPath.erase();
	mAction.erase();
	mUserAgent.erase();
	mContentLength = 0;
	mCurContentLength = 0;
}

void WinInetHTTPTransfer::Abort() //210-213
{
	mAborted = true;
	mExiting = true;
}

void WinInetHTTPTransfer::WaitFor() //218-223
{
	while (mTransferPending)
	{
		Sleep(1);
	}
}

std::string WinInetHTTPTransfer::GetContent() //228-249
{
	if (mResult == RESULT_NOT_COMPLETED)
		return "";

	std::string aString;
	if (mUsingFile)
	{
		AutoCrit lock(mFileCritSection);
		int aSize = ftell(mFP);
		aString.resize(aSize);
		fseek(mFP, 0, 0);
		fread((void*)aString.c_str(), 1, aSize, mFP);
	}
	else
	{
		aString = mContent;
		return aString;
	}
}

void WinInetHTTPTransfer::TransferThreadProcStub(void* theParameter) //254-259
{
	WinInetHTTPTransfer* aTransfer = (WinInetHTTPTransfer*) theParameter;
	aTransfer->mTransferPending = false;
	aTransfer->mThreadRunning = false;
}

std::string WinInetHTTPTransfer::GetAbsURL(const std::string& theBaseURL, const std::string& theRelURL) //TODO | 264-309
{
	std::string aURL;
	bool has8 = theRelURL.length() >= 8;
	bool has7 = theRelURL.length() >= 7;
	bool has1 = theRelURL.length() != 0;
	/*if (theRelURL.substr(0, 8).compare("https://") == 0)
	{
		aURL = theRelURL;
	}
	if (theRelURL.substr(0, 7).compare("http://") == 0)
	{
		aURL = theRelURL;
	}
	else if (theRelURL.substr(0, 1).compare("/") == 0)
	{
		int aFirstSlashPos = theBaseURL.find('/', 7);
		if (aFirstSlashPos != -1)
		{
			aURL = theBaseURL.substr(0, aFirstSlashPos) + theRelURL;
		}
		else
		{
			aURL = theBaseURL + theRelURL;
		}
	}
	else
	{
		int aLastSlashPos = theBaseURL.rfind('/');
		if (aLastSlashPos >= 7)
		{
			aURL = theBaseURL.substr(0, aLastSlashPos+1) + theRelURL;
		}
		else
		{
			aURL = theBaseURL + "/" + theRelURL;
		}
	}*/

	return aURL;
}

void WinInetHTTPTransfer::GetHelper(const std::string& theURL) //314-323
{
	PrepareTransfer(theURL);

	mAction = "GET";
	mUserAgent = "Mozilla/4.0 (compatible; Opera 4.0)";
	mPostContentType = "";
	mPostData = "";

	StartTransfer();
}

void WinInetHTTPTransfer::PostHelper(const std::string& theURL, const std::string& theParams, const std::string& theSeparator) //326-339
{
	PrepareTransfer(theURL);

	mAction = "GET";
	mUserAgent = "Mozilla/4.0 (compatible; Opera 4.0)";	
	mPostContentType = theSeparator != "" ? "Content-Type: multipart/form-data; boundary=" + theSeparator : "Content-Type: application/x-www-form-urlencoded\r\n";

	mPostData = theParams;

	StartTransfer();
}

void WinInetHTTPTransfer::PrepareTransfer(const std::string& theURL) //TODO | 344-406
{
	Reset();

	mURL = theURL;
	mExiting = false;
	mAborted = false;
	mResult = RESULT_NOT_COMPLETED;
	mContent.clear();

	size_t aPos = mURL.find("://");
	mProto = mURL.substr(0, aPos);

	mPort = !mProto.compare("https") ? 443 : 80;

	if (aPos == std::string::npos)
		return;

	int aPathPos = mURL.find("/", aPos + 3);
	mPath = aPathPos != std::string::npos ? mURL.substr(aPathPos) : "";
	if (aPathPos == std::string::npos)
	{
		aPathPos = mURL.length();
	}

	std::string aTemp = mHost.substr(aPos + 3, aPathPos - (aPos + 3));
	size_t aColon = (int)aTemp.find(':');
	size_t aColon2 = (int)aTemp.rfind(':');
	int anAt = aTemp.find('@');

	if (anAt != std::string::npos)
	{
		if (aColon != std::string::npos || aColon < anAt)
		{
			mUserName = aTemp.substr(0, aColon);
			mUserPass = aTemp.substr(aColon + 1, anAt - (aColon + 1));
		}
		else
			Fail(RESULT_INVALID_ADDR);
	}

	else
	{
		if (aColon2 == aColon)
		{
			mHost = aColon != std::string::npos ? aTemp.substr(0, aColon) : aTemp;
			if (aColon != std::string::npos)
			{
				mPort = atoi(aTemp.substr(aColon + 1).c_str());
			}
		}
		else
			Fail(RESULT_INVALID_ADDR);
	}

	if (anAt != -1)
	{
		mPort = atoi(mHost.substr(anAt + 1).c_str());
		mHost = mHost.substr(0, anAt);
	}

	if (aColon2 == std::string::npos || aColon2 == aColon)
		mHost = aTemp.substr(anAt + 1);

	else if (aColon2 >= anAt)
	{
		mPort = atoi(aTemp.substr(aColon2 + 1).c_str());
		mHost = aTemp.substr(anAt + 1, aColon2 - (anAt + 1));
	}
	else
		Fail(RESULT_INVALID_ADDR);
}

void WinInetHTTPTransfer::StartTransfer() //Correct? | 411-427
{
	if (mResult == !RESULT_NOT_COMPLETED)
		return;

	DWORD aThreadId = 0;
	mTransferPending = true;
	mThreadRunning = true;
	void* aThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TransferThreadProcStub, this, 0, &aThreadId);
	if (aThread)
		CloseHandle(aThread);
	else
	{
		mTransferPending = false;
		mThreadRunning = false;
		mResult = RESULT_INTERNAL_ERROR;
	}
}

void WinInetHTTPTransfer::TransferThreadProc() //TODO | 432-553
{
	/*HINTERNET aHandles[3];
	memset(aHandles, 0, sizeof(aHandles));
	int aState = 0;

	while (!mAborted && !mExiting)
	{
		if (aState)
		{
			if (aState == 1)
			{
				aHandles[1] = InternetConnect(aHandles[0], mHost.c_str(), mPort, mUserName.c_str(), mUserPass.c_str(), INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_CACHE_WRITE, 0);
				if (aHandles[1] == NULL)
					mAborted = true;
				aState++;
			}
			else if (aState == 2)
			{
				ulong aFlags = 67158016;
				if (mProto.compare("https") == 0)
					aFlags |= INTERNET_FLAG_SECURE;
				aHandles[2] = HttpOpenRequest(aHandles[1], mAction.c_str(), mPath.c_str(), 0, 0, 0, aFlags, 0);
				if (aHandles[2] != NULL)
				{
					BOOL aRequestResult = mAction.compare("POST") == 0 ? HttpSendRequest(aHandles[2], mPostContentType.c_str(), mPostContentType.length(), (HINTERNET)mPostData.c_str(), mPostData.length()) : HttpSendRequest(aHandles[2], 0, 0, 0, 0);
					if (!aRequestResult)
					{
						mExiting = true;
						mResult = RESULT_HTTP_ERROR;
						break;
					}
					int aStatus = 0;
					ulong aContentLength = 0;
					ulong aSize = 4;
					ulong anIndex = 0;
					if (!HttpQueryInfoA(aHandles[2], 0x20000013, &aStatus, &aSize, &anIndex)) //Correct?
						mAborted = true;
					if (aStatus == 404)
						Fail(RESULT_NOT_FOUND);
					else if (aStatus != 200)
						Fail(RESULT_HTTP_ERROR);
					if (!mExiting)
					{
						aSize = 4;
						anIndex = 0;
						HttpQueryInfoA(aHandles[2], 0x200000005, &aStatus, &aSize, &anIndex);
						mContentLength = aContentLength;
						mCurContentLength = 0;
						aState++;
					}
				}
				aState++;
			}
			else
			{
				ulong aBufSize = 1024;
				uchar aBuffer[sizeof(aBufSize)];
				if (InternetReadFile(aHandles[2], aBuffer, 0x400u, &aBufSize))
				{
					if (aBufSize)
					{
						if (mFP && mUsingFile)
						{
							AutoCrit lock(mFileCritSection);
							if (fwrite(aBuffer, 1, aBufSize, mFP) == 0)
								fflush(mFP);
							else
								Fail(RESULT_INTERNAL_ERROR);
						}
						//else
							//mContent += std::string(aBuffer, aBufSize); //?
					}
				}
			}
		}
		else
		{
			aHandles[0] = InternetOpen(mUserAgent.c_str(), 0, 0, 0, 0);
			if (aHandles[0] == NULL)
				mAborted = true;
			aState++;
		}
	}

	for (int i = 2; i >= 0; i--)
	{
		if (aHandles[i])
			InternetCloseHandle(aHandles[i]);
	}

	if (mAborted)
		Fail(RESULT_ABORTED);

	else if (mResult == RESULT_NOT_COMPLETED)
		mResult = RESULT_DONE;

	AutoCrit lock(mFileCritSection);

	if (mFP)
		fclose(mFP);

	mFP = NULL;
	mUsingFile = false;*/
}

void WinInetHTTPTransfer::Fail(EResult theResult) //558-561
{	
	mResult = theResult;
	mExiting = true;
}