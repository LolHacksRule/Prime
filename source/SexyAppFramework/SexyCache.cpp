#include "SexyCache.h"
#include "AutoCrit.h"

using namespace Sexy;

SexyCache Sexy::gSexyCache;

#ifndef ANDROID
SexyCache::SexyCache() //15-29
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing
	CritSect(mCritSect);
	mSexyCacheMessage = 0;
	mSexyCacheData.mHWnd = NULL;
	mSexyCacheData.mVersion = 0;
	HANDLE aMappedFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, "SexyCacheData");
	if (aMappedFile != NULL)
	{
		SexyCacheData* aSexyCacheDataP = (SexyCacheData*)MapViewOfFile(aMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, 16); //?
		memcpy(&mSexyCacheData, aSexyCacheDataP, sizeof mSexyCacheData);
		UnmapViewOfFile(aSexyCacheDataP);
		CloseHandle(aMappedFile);
		mSexyCacheMessage = RegisterWindowMessageA("SexyCacheMessage");
	}
#endif
}

bool SexyCache::Connected() //32-34
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing
	return mSexyCacheMessage != 0;
#else
	return false;
#endif
}

void SexyCache::Disconnect() //37-38
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing
	mSexyCacheMessage = 0;
#endif
}

bool SexyCache::HadCachedObjects() //42-44
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing
	return mSexyCacheData.mCachedItemCount > 0;
#else
	return false;
#endif
}

void SexyCache::CheckFileChanges() //47-52
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing
	if (mSexyCacheMessage)
	{
		SendMessage(mSexyCacheData.mHWnd, mSexyCacheMessage, WM_MOVE, NULL);
	}
#endif
}

bool SexyCache::CheckData(const std::string& theSrcFile, const std::string& theDataType) //55-75
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	char aReqName[256];
	SexyDataHeader* aHeader;
	uint aResult;

	if (mSexyCacheMessage == NULL)
		return 0;

	sprintf(aReqName, "SexyCacheReq#%d", GetCurrentThreadId());
	HANDLE aHandle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 772, aReqName);
	aHeader = (SexyDataHeader*)MapViewOfFile(aHandle, FILE_MAP_ALL_ACCESS, 0, 0, 772); //?
	strcpy(aHeader->mSrcFile, theSrcFile.c_str());
	strcpy(aHeader->mDataType, theDataType.c_str());

	aHeader->mSize = 0;

	aResult = SendMessage(mSexyCacheData.mHWnd, mSexyCacheMessage, WM_NULL, GetCurrentThreadId());
	bool success = aResult != 0;
	UnmapViewOfFile(aHeader);
	CloseHandle(aHandle);
	return success;
#else
	return false;
#endif
}

bool SexyCache::SetUpToDate(const std::string& theSrcFile, const std::string& theDataType) //78-98
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	char aReqName[256];
	SexyDataHeader* aHeader;
	uint aResult;

	if (mSexyCacheMessage == NULL)
		return 0;

	sprintf(aReqName, "SexyCacheReq#%d", GetCurrentThreadId());
	HANDLE aHandle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 772, aReqName);
	aHeader = (SexyDataHeader*)MapViewOfFile(aHandle, FILE_MAP_ALL_ACCESS, 0, 0, 772); //?
	strcpy(aHeader->mSrcFile, theSrcFile.c_str());
	strcpy(aHeader->mDataType, theDataType.c_str());

	aResult = SendMessage(mSexyCacheData.mHWnd, mSexyCacheMessage, 4, GetCurrentThreadId());
	bool success = aResult != 0;
	UnmapViewOfFile(aHeader);
	CloseHandle(aHandle);
	return success;
#else
	return false;
#endif
}

bool SexyCache::GetData(const std::string& theSrcFile, const std::string& theDataType, void** thePtr, int* theSize) //101-139 | Correct?
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	char aReqName[256];
	char aName[256];
	uint aResult;
	void* aData;

	if (mSexyCacheMessage == NULL)
		return false;

	sprintf(aReqName, "SexyCacheReq#%d", GetCurrentThreadId());
	HANDLE aHandle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 772u, aReqName);
	SexyDataHeader* aHeader = (SexyDataHeader*)MapViewOfFile(aHandle, FILE_MAP_ALL_ACCESS, 0, 0, 772); //?

	strcpy(aHeader->mSrcFile, theSrcFile.c_str());
	strcpy(aHeader->mDataType, theDataType.c_str());
	aHeader->mSize = 0;
	bool success = false;
	aResult = SendMessage(mSexyCacheData.mHWnd, mSexyCacheMessage, WM_NULL, GetCurrentThreadId());
	if (aResult)
	{
		sprintf(aName, "SexyCacheData#%d", aResult);
		HANDLE aMappedFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, aName);
		if (aMappedFile)
		{
			aData = MapViewOfFile(aMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, aHeader->mSize);
			*thePtr = aData;
			AutoCrit anAutoCrit(mCritSect);
			mAllocDataMap.insert(PtrToHandleMap::value_type(aData, aMappedFile)); //?
			success = true;
		}
		*theSize = aHeader->mSize;
	}

	UnmapViewOfFile(aHeader);
	CloseHandle(aHandle);
	return success;
#else
	return false;
#endif
}

void SexyCache::FreeGetData(HANDLE theGetDataPtr) //Correct? | Line 142-151
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	AutoCrit anAutoCrit(mCritSect);
	PtrToHandleMap::iterator anItr = mAllocDataMap.find(theGetDataPtr);
	if (anItr != mAllocDataMap.end())
	{
		UnmapViewOfFile(anItr->first);
		CloseHandle(anItr->second);
		mAllocDataMap.erase(anItr);
	}
#endif
}

void SexyCache::SetFileDeps(const std::string& theSrcFile, const std::string& theDataType, const std::string& theFileDeps) //154-161
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	int aResult = theFileDeps.length();

	void *aData = AllocSetData(theSrcFile, theDataType, aResult + 1);
	if (aData)
	{
		memcpy(aData, theFileDeps.c_str(), aResult + 1);
		SendMessage(mSexyCacheData.mHWnd, mSexyCacheMessage, WM_DESTROY, GetCurrentThreadId());
		FreeSetData(aData);
	}
#endif
}

void* SexyCache::AllocSetData(const std::string& theSrcFile, const std::string& theDataType, int theSize) //Correct? | Lines 164-190
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	char aReqName[256];
	void* aPtr;

	if (mSexyCacheMessage == NULL)
		return NULL;

	sprintf(aReqName, "SexyCacheReq#%d", GetCurrentThreadId());
	HANDLE aHandle = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 772u, aReqName);
	if (aHandle == INVALID_HANDLE_VALUE)
		return NULL;
	void* aData = MapViewOfFile(aHandle, FILE_MAP_ALL_ACCESS, 0, 0, theSize + 772);
	if (aData)
	{
		SexyDataHeader* aHeader = (SexyDataHeader*)aData; //?
		strcpy(aHeader->mSrcFile, theSrcFile.c_str());
		strcpy(aHeader->mDataType, theDataType.c_str());
		aHeader->mSize = theSize;
		AutoCrit anAutoCrit(mCritSect);
		aPtr = &aData + 772;
		//std::pair(aPtr, aHandle);
		//mAllocDataMap.insert();

		aPtr = &mAllocDataMap.insert(PtrToHandleMap::value_type(aPtr, aHandle)); //?

		return aPtr;
	}
	else
	{
		CloseHandle(aHandle);
		return NULL;
	}
#endif
}

void SexyCache::FreeSetData(void* theSetDataPtr) //193-202
{
#if !defined(_EAMT) && !defined(__APPLE__) //On mobile, nothing //On EAMT, disabled
	AutoCrit anAutoCrit(mCritSect);

	PtrToHandleMap::iterator anItr = mAllocDataMap.find(theSetDataPtr);
	if (anItr == mAllocDataMap.end())
		return;
	UnmapViewOfFile(&anItr->first - 772);
	CloseHandle(&anItr->second);
	mAllocDataMap.erase(&anItr);
#endif
}

bool SexyCache::SetData(void* theSetDataPtr) //205-222
{
#ifndef _EAMT //On EAMT, disabled
	int aResult;
	AutoCrit anAutoCrit(mCritSect);
	PtrToHandleMap::iterator anItr = mAllocDataMap.find(theSetDataPtr);

	if (anItr == mAllocDataMap.end())
	{
		return false;
	}
	else
	{
		int aResult = SendMessageA(mSexyCacheData.mHWnd, mSexyCacheMessage, WM_CREATE, GetCurrentThreadId());
		if (aResult == -1)
		{
			mSexyCacheMessage = NULL;
			return false;
		}
		else
			return true;
	}
#else
	return false;
#endif
}

#endif

void Sexy::SMemW(void** theDest, const void* theSrc, int theSize) //227-230
{
	memcpy(theDest, theSrc, theSize); theDest = theDest + theSize;
}

void Sexy::SMemWStr(void** theDest, const std::string& theStr) //233-237
{
	int aLen = theStr.length(); SMemW(theDest, &aLen, 4);
	SMemW(theDest, theStr.c_str(), aLen);
}

void Sexy::SMemR(void** theSrc, void* theDest, int theSize) //240-243
{
	memcpy(theDest, theSrc, theSize);
	theSrc += theSize;
}

void Sexy::SMemRStr(void** theDest, std::string* theStr) //246-251
{
	int aLen = 0;
	SMemR(theDest, &aLen, 4);
	theStr->resize(aLen);
	SMemR(theDest, (void*)theStr->c_str(), aLen);
}