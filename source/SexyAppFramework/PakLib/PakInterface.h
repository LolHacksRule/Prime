#ifndef __PAKINTERFACE_H__
#define __PAKINTERFACE_H__

#include <map>
#include <list>
#include <string>

#include "../Debug.h"
#include "../IFileDriver.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <stdint.h>
#endif

class PakCollection;

/*namespace Sexy
{
	class IFile;
	class IFileDriver;
	class FileSearchInfo;
};*/

//using namespace Sexy; //Not doing this so imagelib.cpp won't flip out, now what to do about this file for imglib

class PakRecord
{
public:
	PakCollection*			mCollection;
	std::string				mFileName;
	uint64					mFileTime; //Changed to u64
	int						mStartPos;
	int						mSize;	
};

typedef std::map<std::string, PakRecord> PakRecordMap;

class PakCollection //Changed to IFile
{
public:
	IFile*					mFile;
	void*					mDataPtr;
};

struct PakFileTime
{
	ulong LowDateTime; //?
	ulong HighDateTime;
};

typedef std::list<PakCollection> PakCollectionList;

struct PFILE //Changed
{
	PakRecord*				mRecord;
	int						mPos;
	IFile*					mFile;
	uchar*					mBuffer;
	ulong					mSize;
	ulong					mPointer;
	bool					mBufferOwner;
};

class PFindData : public IFileSearch
{
public:
	IFileSearch*		mDriverSearch;
	std::string				mLastFind;
	std::string				mFindCriteria;
};

class PakFileDesc
{
public:
	std::string filename;
	uchar* buffer;
	ulong size;
	ulong actual_size;
	IFile* f;
};

enum
{
	PSEARCH_PAK_THEN_FILES,
	PSEARCH_FILES_THEN_PAK,
	PSEARCH_JUST_PAK,
	PSEARCH_JUST_FILES
};

class PakInterfaceBase //Add cleanup
{
public:
	virtual void			Cleanup() = 0;
	virtual PFILE*			FOpen(const char* theFileName, const char* theAccess, int theSearchOrder = -1) = 0;
	virtual PFILE*			FOpen(const wchar_t* theFileName, const wchar_t* theAccess, int theSearchOrder = -1) = 0;
	virtual int				FClose(PFILE* theFile) = 0;
	virtual int				FSeek(PFILE* theFile, long theOffset, int theOrigin) = 0;
	virtual int				FTell(PFILE* theFile) = 0;
	virtual size_t			FRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile) = 0;
	virtual size_t			DecryptFRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile) = 0;
	virtual int				FGetC(PFILE* theFile) = 0;
	virtual int				UnGetC(int theChar, PFILE* theFile) = 0;
	virtual char*			FGetS(char* thePtr, int theSize, PFILE* theFile) = 0;
	virtual wchar_t*		FGetS(wchar_t* thePtr, int theSize, PFILE* theFile) { return thePtr; } //93
	virtual int				FEof(PFILE* theFile) = 0;

	virtual IFileSearch*	FindFirstFile(const char* theFileName, FileSearchInfo* theFindFileData) = 0;
	virtual bool			FindNextFile(IFileSearch* theFileSearch, FileSearchInfo* theFindFileData) = 0;
	virtual bool			FindClose(IFileSearch* theFileSearch) = 0;

	virtual int				SetFileSearchOrder(int theOrder) = 0;
	virtual int				GetFileSearchOrder() = 0;
	virtual void			SetPassCode(uchar thePassCode) = 0;
};

class PakInterface : public PakInterfaceBase
{
public:
	PakCollectionList		mPakCollectionList;	
	PakRecordMap			mPakRecordMap;
	int						mSearchOrder;
	uchar					mPassCode;
	bool					mIsMMapped;

protected:
	bool					PFindNext(IFileSearch* theSearch, FileSearchInfo* theFindFileData);
public:
	PFILE*					FOpen_Pak(const char* theFileName, const char* theAccess);
	static PFILE*			FOpen_File(const char* theFileName, const char* anAccess);
	static PFILE*			FOpen_File(const wchar_t* theFileName, const wchar_t* anAccess);
	bool					FGetBuffer_Pak(const char* theFileName, uchar** theOutBuffer, ulong* theOutSize, PFILE** theOutFile);
	static bool				FGetBuffer_File(const char* theFileName, uchar** theOutBuffer, ulong* theOutSize, PFILE** theOutFile);

public:
	PakInterface();
	~PakInterface();

	void					Cleanup();
	bool					PreparePakFile(const std::string& theFileName, PakFileDesc& out);
	bool					IsPakFileLoaded(PakFileDesc& desc);
	bool					HasPakFileFailed(PakFileDesc& desc);
	bool					ClosePakFile(PakFileDesc& desc);
	bool					AddPakFile(PakFileDesc& desc);
	bool					AddPakFile(const std::string& theFileName);
	bool					RemovePakFile(PakFileDesc& desc); //Not a function but present
	
	PFILE*					FOpen(const wchar_t* theFileName, const wchar_t* theAccess, int theSearchOrder = -1) { return NULL; } //151
	PFILE*					FOpen(const char* theFileName, const char* theAccess, int theSearchOrder = -1);
	int						FClose(PFILE* theFile);
	int						FSeek(PFILE* theFile, long theOffset, int theOrigin);
	int						FTell(PFILE* theFile);
	size_t					FRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile);
	size_t					DecryptFRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile);
	int						FGetC(PFILE* theFile);
	int						UnGetC(int theChar, PFILE* theFile);
	char*					FGetS(char* thePtr, int theSize, PFILE* theFile);
	int						FEof(PFILE* theFile);

	bool					FGetBuffer(const char* theFileName, uchar** theOutBuffer, ulong* theOutSize, PFILE **theOutFile, int theSearchOrder); 

	IFileSearch*			FindFirstFile(const char* theFileName, FileSearchInfo* theFindFileData);
	bool					FindNextFile(IFileSearch* theFileSearch, FileSearchInfo* theFindFileData);
	bool					FindClose(IFileSearch* theFileSearch);
	
	int						SetFileSearchOrder(int theOrder);
	int						GetFileSearchOrder();
	void					SetPassCode(uchar thePassCode);
};

static PakInterface* gPakInterface = NULL;

static PakInterfaceBase* GetPakPtr() //179-181
{
	return gPakInterface;
}

static PFILE* p_fopen(const char* theFileName, const char* theAccess, int theSearchOrder = -1) //184-196
{
	DBG_ASSERTE(theAccess!=0 && theAccess[0]=='r'); //185
	if (GetPakPtr() != NULL)
		return gPakInterface->FOpen(theFileName, theAccess, theSearchOrder);
	else
		return PakInterface::FOpen_File(theFileName, theAccess); //?
}

static int p_fclose(PFILE* theFile) //234-271 (UNMATCHED)
{
	if (!theFile)
		return 0;
	if (!theFile->mRecord)
	{
		if (theFile->mFile)
			delete theFile->mFile; //?
	}
	if (theFile->mBufferOwner)
		operator delete(theFile->mBuffer);
	operator delete(theFile);
	return 0;
}

static int p_fseek(PFILE* theFile, long theOffset, int theOrigin) //274-300 (UNMATCHED)
{
	if (GetPakPtr() != NULL && theFile->mRecord)
		return (gPakInterface)->FSeek(theFile, theOffset, theOrigin);
	if (theOrigin) //prob a way to simplify
	{
		if (theOrigin == 1)
			theFile->mPointer += theOffset;
		else
		{
			if (theOrigin != 2)
				DBG_ASSERTE(false); //295
			theFile->mPointer = theOffset + theFile->mSize;
		}
	}
	else
		theFile->mPointer = theOffset;
	return theFile->mPointer;
}

static int p_ftell(PFILE* theFile) //303-315
{
	if (GetPakPtr() != NULL && theFile->mRecord)
		return (gPakInterface)->FTell(theFile);
	return theFile->mPointer;
}

static size_t p_fread(void* thePtr, int theSize, int theCount, PFILE* theFile) //318-339 (UNMATCHED)
{
	if (GetPakPtr() && theFile->mRecord)
		return (gPakInterface)->FRead(thePtr, theSize, theCount, theFile);
	ulong size = theCount * theSize;
	ulong num_read = theCount;
	if (theCount * theSize + theFile->mPointer > theFile->mSize)

	{
		num_read = (theFile->mSize - theFile->mPointer) / theSize;

		size = theSize * num_read;
	}
	memcpy(thePtr, &theFile->mBuffer[theFile->mPointer], size);
	theFile->mPointer += size;
	return num_read;
}

static int p_fgetc(PFILE* theFile) //342-356
{
	if (GetPakPtr() != NULL && theFile->mRecord)
		return (gPakInterface)->FGetC(theFile);

	if (theFile->mPointer >= theFile->mSize)
	{
		return -1;
	}
	return theFile->mBuffer[theFile->mPointer++];
}

static int p_ungetc(int theChar, PFILE* theFile) //329-372 (Matched?)
{
	if (GetPakPtr() != NULL && theFile->mRecord)
		return (gPakInterface)->UnGetC(theChar, theFile);
	theFile->mPointer--;
	return theChar;
}

static int p_feof(PFILE* theFile) //418-429
{
	if (GetPakPtr() != NULL && theFile->mRecord)
		return gPakInterface->FEof(theFile);
	return theFile->mPointer >= theFile->mSize;
}

static int p_GetFileSearchOrder() //488-493
{
	if (GetPakPtr() != NULL)
		return gPakInterface->GetFileSearchOrder();
	else
		return PSEARCH_JUST_FILES;
}

#endif //__PAKINTERFACE_H__
