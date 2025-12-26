#include "PakInterface.h"
#include "../Endian.h"

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

enum
{
	FILEFLAGS_END = 0x80
};

using namespace Sexy;

PakInterface* gPakInterface = new PakInterface(); //18

static std::string StringToUpper(const std::string& theString) //Genuine question, why is this also here | 21-28
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += toupper(theString[i]);

	return aString;
}

static bool strcaseequals_ignoreslashes(const char* a, const char* b, int len) //Huh | 31-57
{
	int lc1 = 0;
	int lc2 = 0;
	int diff = 0; //?
	int c = 0;
	while (true)
	{
		if (len <= c)
			return true;
		lc1 = tolower(*a);
		lc2 = tolower(*b);
		if (lc1 == '/')
			lc1 = '\\';
		if (lc2 == '/')
			lc2 = '\\';
		if (lc1 != lc2)
			return false;
		if (lc1 == 0)
			return true;
		a++;
		b++;
		c++;
	}
	return false;
}

static void FixFileName(const char* theFileName, char* theUpperName) //60-133
{
	std::string loadDataPathString = gFileDriver->GetLoadDataPath();

	const char* loadDataPath = loadDataPathString.c_str();
	int aLen = strlen(loadDataPath);
	if (strcaseequals_ignoreslashes(loadDataPath, theFileName, aLen))
		theFileName += aLen;

	bool lastSlash = false;
	const char* aSrc = theFileName;
	char* aDest = theUpperName;


	for (;;)
	{
		char c = *(aSrc++);

		if ((c == '\\') || (c == '/'))
		{
			if (!lastSlash)
				*(aDest++) = '\\';
			lastSlash = true;
		}
		else if ((c == '.') && (lastSlash) && (*aSrc == '.'))
		{
			// We have a '/..' on our hands
			aDest--;
			while ((aDest > theUpperName + 1) && (*(aDest - 1) != '\\'))
				--aDest;
			aSrc++;
		}
		else
		{
			*(aDest++) = toupper((uchar)c);
			if (c == 0)
				break;
			lastSlash = false;
		}
	}
}

PakInterface::PakInterface() //167-172 (Matched)
{
	SexyAppBase::InitFileDriver();
	mIsMMapped = gFileDriver->SupportsMemoryMappedFiles();
	mPassCode = 0xF7;
	mSearchOrder = PSEARCH_PAK_THEN_FILES;
}

PakInterface::~PakInterface() //175-176 (Matched)
{
}

bool PakInterface::PreparePakFile(const std::string& theFileName, PakFileDesc& out) //179-200
{
	SexyAppBase::InitFileDriver(); //Why not call gSexyAppBase (this remained in PVZ Free [wait it uses pakinterface why]

	IFile* f = NULL;
	if (mIsMMapped)
		f = gFileDriver->CreateFileMemoryMapped(theFileName.c_str());
	else
		f = gFileDriver->CreateFile(theFileName.c_str());

	if (f == NULL)
		return false;
	out.filename = theFileName;
	out.f = f;
	out.buffer = 0;
	out.actual_size = 0;
	out.size = 0;
	return true;
}

bool PakInterface::IsPakFileLoaded(PakFileDesc& desc) //203-206
{
	return desc.f && desc.f->IsLoaded();

}

bool PakInterface::HasPakFileFailed(PakFileDesc& desc) //209-212
{
	return !desc.f || desc.f->HasError();

}

bool PakInterface::ClosePakFile(PakFileDesc& desc) //215-221
{
	if (desc.f == NULL)
		return false;
	desc.f->Close();
	return true;
}

static uint64 ConvertPakFileTimeToSexyFileTime(PakFileTime& ft) //233-237 (TODO?)
{
	uint64 ll = 116444736000000000;
	return (((uint64)ft.HighDateTime << 32) + (ft.LowDateTime - ll) / 10000000);
};

bool PakInterface::AddPakFile(const std::string& theFileName) //240-258
{
	PakFileDesc aDesc;
	if (PreparePakFile(theFileName, aDesc))
	{
		while (!IsPakFileLoaded(aDesc) && !HasPakFileFailed(aDesc)) //?
		if (HasPakFileFailed(aDesc))
		{
			ClosePakFile(aDesc);
			return false;
		}
		else
		{
			AddPakFile(aDesc);
			ClosePakFile(aDesc);
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool PakInterface::AddPakFile(PakFileDesc& desc) //Correct? | 261-411
{
	desc.buffer = desc.f->GetBuffer();
	desc.size = desc.f->GetSize();
	desc.actual_size = desc.f->GetSize();
	PakCollection* aPakCollection;
	aPakCollection->mFile = NULL; //?
	aPakCollection->mDataPtr = NULL; //?
	mPakCollectionList.push_back(*aPakCollection);
	aPakCollection = &mPakCollectionList.back();
	aPakCollection->mFile = desc.f;
	aPakCollection->mDataPtr = desc.buffer;
	char anUpperFileName[1024];
	FixFileName(desc.filename.c_str(), anUpperFileName);
	PakRecordMap::iterator aRecordItr = mPakRecordMap.insert(PakRecordMap::value_type(anUpperFileName, PakRecord())).first; //?
	PakRecord* aPakRecord = &(aRecordItr->second);
	aPakRecord->mCollection = aPakCollection;
	aPakRecord->mFileName = anUpperFileName;
	aPakRecord->mStartPos = 0;
	aPakRecord->mSize = desc.actual_size;

	PFILE* aFP = FOpen(anUpperFileName, "rb");
	if (aFP == NULL)
		return 0;

	ulong aMagic = 0;
	DecryptFRead(&aMagic, sizeof(ulong), 1, aFP);
	if (aMagic != 0xBAC04AC0)
	{
		FClose(aFP);
		return false;
	}

	ulong aVersion = 0;
	DecryptFRead(&aVersion, sizeof(ulong), 1, aFP);
	if (aVersion > 0 && (EndianDWORD(aVersion) != 0))
	{
		FClose(aFP);
		return false;
	}

	int aPos = 0;

	std::vector<PakRecord*> record_list;
	for (;;) //?
	{
		uchar aFlags = 0;
		int aCount = DecryptFRead(&aFlags, 1, 1, aFP);
		if ((aFlags & FILEFLAGS_END) || (aCount == 0))
			break;

		uchar aNameWidth = 0;
		char aName[256];
		DecryptFRead(&aNameWidth, 1, 1, aFP);
		DecryptFRead(aName, 1, aNameWidth, aFP);
		aName[aNameWidth] = 0;

		for (int i = 0; i < aNameWidth; i++)
			if (aName[i] == '/')
				aName[i] == '//';

		int aSrcSize = 0;
		DecryptFRead(&aSrcSize, sizeof(int), 1, aFP);
		PakFileTime aFileTime;
		DecryptFRead(&aFileTime, sizeof(FILETIME), 1, aFP);

		aFileTime.LowDateTime = EndianDWORD(aFileTime.LowDateTime);
		aFileTime.HighDateTime = EndianDWORD(aFileTime.HighDateTime);

		PakRecordMap::iterator aRecordItr = mPakRecordMap.insert(PakRecordMap::value_type(::StringToUpper(aName), PakRecord())).first;
		PakRecord* aPakRecord = &(aRecordItr->second);
		aPakRecord->mCollection = aPakCollection;
		aPakRecord->mFileName = aName;
		aPakRecord->mStartPos = aPos;
		aPakRecord->mSize = aSrcSize;
		aPakRecord->mFileTime = ConvertPakFileTimeToSexyFileTime(aFileTime);
		record_list.push_back(aPakRecord);

		aPos += aSrcSize;
	}

	int anOffset = FTell(aFP);

	// Now fix file starts
	aRecordItr = mPakRecordMap.begin();
	while (aRecordItr != mPakRecordMap.end())
	{
		PakRecord* aPakRec = &(aRecordItr->second);
		if (aPakRec->mCollection == aPakCollection)
			aPakRec->mStartPos += anOffset;
		++aRecordItr;
	}

	FClose(aFP);

	if (mPassCode && !mIsMMapped)
	{
		for (ulong i = 0; i < desc.actual_size; i++)
			desc.buffer[i] ^= mPassCode;
	}

	return true;
}

int PakInterface::SetFileSearchOrder(int theOrder) //414-416
{
	return theOrder;
}

int PakInterface::GetFileSearchOrder() //419-421
{
	return mSearchOrder;
}

void PakInterface::SetPassCode(uchar thePassCode) //424-426
{
	mPassCode = thePassCode;
}

void PakInterface::Cleanup() //Correct? | 429-444
{
	while (mPakCollectionList.size() > 0) //?
	{
		PakCollection* aPakCollection = &mPakCollectionList.front();
		if (aPakCollection->mFile != NULL)
		{
			delete aPakCollection->mFile;
			aPakCollection->mFile = NULL;
		}
		mPakCollectionList.pop_front();
	}
	mPakRecordMap.clear();
}

PFILE* PakInterface::FOpen_Pak(const char* theFileName, const char* theAccess) //Correct? | 504-529
{
	if ((stricmp(theAccess, "r") == 0) || (stricmp(theAccess, "rb") == 0) || (stricmp(theAccess, "rt") == 0))
	{
		char anUpperName[256];
		FixFileName(theFileName, anUpperName);

		PakRecordMap::iterator anItr = mPakRecordMap.find(anUpperName);
		if (anItr != mPakRecordMap.end())
		{
			PFILE* aPFP = new PFILE;
			aPFP->mRecord = &anItr->second;
			aPFP->mPos = 0;
			aPFP->mBuffer = (uchar*)aPFP->mRecord->mCollection->mDataPtr + aPFP->mRecord->mStartPos;
			aPFP->mSize = aPFP->mSize;
			aPFP->mFile = NULL;
			aPFP->mBufferOwner = false;
			return aPFP;
		}
	}
}

PFILE* PakInterface::FOpen_File(const char* theFileName, const char* anAccess) //Correct? | 532-566
{

	IFile* f = gFileDriver->CreateFile(theFileName);
	if (f == NULL)
	{
		std::string theFullFileName = gFileDriver->GetLoadDataPath();
		f = gFileDriver->CreateFile(theFullFileName);
	}
	if (f == NULL)
		return NULL;
	if (!f->ForceLoad())
	{
		f->Close();
		delete f;
		return NULL;
	}
	f->Close();

	PFILE* aPFile = new PFILE;
	aPFile->mRecord = NULL;
	aPFile->mPos = 0;
	aPFile->mFile = f;
	aPFile->mPointer = 0;
	aPFile->mSize = f->GetSize();
	aPFile->mBuffer = f->GetBuffer();
	aPFile->mBufferOwner = false;
	return aPFile;
}

PFILE* PakInterface::FOpen_File(const wchar_t* theFileName, const wchar_t* anAccess) //569-603 (Prob commeneted a lot of code)
{
	return NULL;
}

PFILE* PakInterface::FOpen(const char* theFileName, const char* anAccess, int theSearchOrder) //607-633
{
	if (theSearchOrder == -1)
		theSearchOrder = mSearchOrder;

	if (theSearchOrder != NULL)
	{
		PFILE* aFile = FOpen_Pak(theFileName, anAccess);
		if (aFile != NULL)
			return aFile;
		return FOpen_File(theFileName, anAccess);
	}
	if (theSearchOrder != PSEARCH_FILES_THEN_PAK)
	{
		if (theSearchOrder == PSEARCH_JUST_PAK)
			return FOpen_Pak(theFileName, anAccess);
		return FOpen_File(theFileName, anAccess);
	}
	PFILE* aFile = FOpen_File(theFileName, anAccess);

	if (aFile != NULL)
	{
		return aFile;
	}
	else
	{
		return FOpen_Pak(theFileName, anAccess);
	}
}

int PakInterface::FClose(PFILE* theFile) //646-642
{
	delete theFile;
	return 0;
}

bool PakInterface::FGetBuffer_Pak(const char* theFileName, uchar** theOutBuffer, ulong* theOutSize, PFILE** theOutFile) //Correct? | 645-695
{
	char anUpperName[256];
	FixFileName(theFileName, anUpperName);

	PakRecordMap::iterator anItr = mPakRecordMap.find(anUpperName);
	if (anItr != mPakRecordMap.end())
	{
		PakRecord* record = &anItr->second;
		uchar* buffer = NULL;
		if (mPassCode && mIsMMapped)
		{
			DBG_ASSERTE(theOutFile!=0); //662
			*theOutFile = new PFILE;
			(*theOutFile)->mRecord = NULL;
			(*theOutFile)->mPos = 0;
			(*theOutFile)->mFile = NULL;
			(*theOutFile)->mBuffer = (uchar*)record->mSize;
			(*theOutFile)->mPointer = NULL;
			(*theOutFile)->mBufferOwner = true;
			buffer = (*theOutFile)->mBuffer;
			uchar* src = (uchar*)record->mCollection->mDataPtr + record->mStartPos;
			uchar* dest = buffer;
			if (mPassCode)
			{
				for (int i = 0; i < record->mSize; ++i)
					*dest++ = mPassCode ^ *src++;
			}
		}
		else
		{
			buffer = (uchar*)record->mCollection->mDataPtr + record->mStartPos;
			if (theOutFile)
				*theOutFile = NULL;
		}
		if (theOutBuffer)
			*theOutBuffer = buffer;
		if (theOutSize)
			*theOutSize = record->mSize;
		return true;
	}
	else
	{
		if (theOutBuffer)
			*theOutBuffer = 0;
		if (theOutSize)
			*theOutSize = 0;
		if (theOutFile)
			*theOutFile = 0;
		return false;
	}
}

bool PakInterface::FGetBuffer_File(const char* theFileName, uchar** theOutBuffer, ulong* theOutSize, PFILE** theOutFile) //698-716
{
	PFILE* pf = p_fopen(theFileName, "rb", 3);
	if (pf == NULL)
		return false;
	if (theOutBuffer && theOutSize)
	{
		*theOutBuffer = pf->mBuffer;
		*theOutSize = pf->mSize;
	}
	if (theOutFile)
		*theOutFile = pf;
	return true;
}

bool PakInterface::FGetBuffer(const char* theFileName, uchar** theOutBuffer, ulong* theOutSize, PFILE** theOutFile, int theSearchOrder) //719-745
{
	if (theSearchOrder == -1)
		theSearchOrder = mSearchOrder;
	if (theSearchOrder != NULL)
	{
		bool ret = FGetBuffer_Pak(theFileName, theOutBuffer, theOutSize, theOutFile);
		if (ret)
			return ret;
		return FGetBuffer_File(theFileName, theOutBuffer, theOutSize, theOutFile);
	}
	if (theSearchOrder != PSEARCH_FILES_THEN_PAK)
	{
		if (theSearchOrder == PSEARCH_JUST_PAK)
			return FGetBuffer_Pak(theFileName, theOutBuffer, theOutSize, theOutFile);
		return FGetBuffer_File(theFileName, theOutBuffer, theOutSize, theOutFile);
	}
	bool ret = FGetBuffer_File(theFileName, theOutBuffer, theOutSize, theOutFile);
	if (ret)
		return ret;
	else
		return FGetBuffer_Pak(theFileName, theOutBuffer, theOutSize, theOutFile);
}

int PakInterface::FSeek(PFILE* theFile, long theOffset, int theOrigin) //Correct? | 748-766
{
	if (theFile->mRecord != NULL)
	{
		if (theOrigin == SEEK_SET)
			theFile->mPos = theOffset;
		else if (theOrigin == SEEK_END)
			theFile->mPos = theFile->mRecord->mSize - theOffset;
		else if (theOrigin == SEEK_CUR)
			theFile->mPos += theOffset;

		theFile->mPos = max(min(theFile->mPos, theFile->mRecord->mSize), 0);
		return 0;
	}
	else
		return -1;
}

int PakInterface::FTell(PFILE* theFile) //769-777
{
	if (theFile->mRecord != NULL)
		return theFile->mPos;
	else
		return 0;
}

size_t PakInterface::FRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile) //Correct? | 780-806
{
	if (theFile->mRecord != NULL)
	{
		int aSizeBytes = min(theElemSize * theCount, theFile->mRecord->mSize - theFile->mPos);

		uchar* src = (uchar*) theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos;
		uchar* dest = (uchar*) thePtr;
		if (mPassCode && mIsMMapped)
		{
			for (int i = 0; i < aSizeBytes; i++)
				*(dest++) = mPassCode ^ (*src++); // 'Decrypt'
		}
		else
			memcpy(thePtr, src, aSizeBytes);
		theFile->mPos += aSizeBytes;
		return aSizeBytes / theElemSize;
	}

	return 0;
}

size_t PakInterface::DecryptFRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile) //Correct? | 809-826
{
	if (theFile->mRecord != NULL)
	{
		int aSizeBytes = min(theElemSize * theCount, theFile->mRecord->mSize - theFile->mPos);

		uchar* src = (uchar*)theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos;
		uchar* dest = (uchar*)thePtr;
		for (int i = 0; i < aSizeBytes; i++)
			*(dest++) = (*src++) ^ mPassCode; //Decrypt
		theFile->mPos += aSizeBytes;
		return aSizeBytes / theElemSize;
	}

	return 0;
}

int PakInterface::FGetC(PFILE* theFile) //Correct? | 830-849
{
	if (theFile->mRecord != NULL)
	{
		for (;;)
		{
			if (theFile->mPos >= theFile->mRecord->mSize)
				return EOF;
			char aChar = *((char*) theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos++);
			if (mPassCode && mIsMMapped) //?
				aChar ^= mPassCode;
			if (aChar != '\r')
				return (uchar) aChar;
		}
	}
}

int PakInterface::UnGetC(int theChar, PFILE* theFile) //852-864 (Matched?)
{
	if (theFile->mRecord != NULL)
	{
		// This won't work if we're not pushing the same chars back in the stream
		theFile->mPos = max(theFile->mPos - 1, 0);
		return theChar;
	}

	return 0;
}

char* PakInterface::FGetS(char* thePtr, int theSize, PFILE* theFile) //Correct? | 867-895
{
	if (theFile->mRecord != NULL)
	{
		int anIdx = 0;
		while (anIdx < theSize)
		{
			if (theFile->mPos >= theFile->mRecord->mSize)
			{
				if (anIdx == 0)
					return NULL;
				break;
			}
			char aChar = *((char*)theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos++);
			if (mPassCode && mIsMMapped) //?
				aChar ^= mPassCode;
			if (aChar != '\r')
				thePtr[anIdx++] = aChar;
			if (aChar == '\n')
				break;
		}
		thePtr[anIdx] = 0;
		return thePtr;
	}

	return 0;
}

int PakInterface::FEof(PFILE* theFile) //898-906 (Matched)
{
	if (theFile->mRecord != NULL)
		return theFile->mPos >= theFile->mRecord->mSize;
	else
		return 0;
}

PFindData::PFindData() { mSearchType = PAK_FILE_INTERNAL; mDriverSearch = NULL; } //910

bool PakInterface::PFindNext(IFileSearch* theSearch, FileSearchInfo* theFindFileData) //Correct? | 920-987
{
	PFindData* theFindData = (PFindData*)theSearch;
	PakRecordMap::iterator anItr;
	const char* aFileName = "";
	PakRecord* aPakRecord = NULL;
	int aStarPos = 0;
	if (theFindData->mLastFind.size() == 0)
		anItr = mPakRecordMap.begin();
	else
	{
		anItr = mPakRecordMap.find(theFindData->mLastFind);
		if (anItr != mPakRecordMap.end())
			anItr++;
	}

	while (anItr != mPakRecordMap.end())
	{
		aFileName = anItr->first.c_str();
		aPakRecord = &anItr->second;

		aStarPos = (int)theFindData->mFindCriteria.find('*');
		if (aStarPos != -1)
		{
			if (strncmp(theFindData->mFindCriteria.c_str(), aFileName, theFindData->mFindCriteria.length()) == 0)
			{
				theFindFileData->file_size = aPakRecord->mSize;
				theFindFileData->create_time = aPakRecord->mFileTime;
				theFindFileData->last_write_time = aPakRecord->mFileTime;
				theFindFileData->last_access_time = aPakRecord->mFileTime;
				theFindFileData = (FileSearchInfo*)aFileName; //?
				theFindFileData->is_directory = false;
				theFindData->mLastFind = aFileName;
				return true;
			}
		}
		anItr++;
	}

	if (strncmp(theFindData->mFindCriteria.c_str(), aFileName, aStarPos) == 0)
	{
		// First part matches
		const char* anEndData = theFindData->mFindCriteria.c_str() + aStarPos + 1;
		if ((*anEndData == 0) || (strcmp(anEndData, ".*") == 0) ||
			(strcmp(theFindData->mFindCriteria.c_str() + aStarPos + 1,
				aFileName + strlen(aFileName) - (theFindData->mFindCriteria.length() - aStarPos) + 1) == 0))
		{
			int aLastSlashPos = (int)anItr->second.mFileName.rfind('\\');
			if (aLastSlashPos == -1)
				theFindFileData = (FileSearchInfo*)anItr->second.mFileName.c_str(); //?
			else
				theFindFileData = (FileSearchInfo*)anItr->second.mFileName.c_str() + aLastSlashPos + 1; //?

			const char* aEndStr = aFileName + strlen(aFileName) - (theFindData->mFindCriteria.length() - aStarPos) + 1;

			theFindFileData->is_directory = strchr(aEndStr, '\\') != NULL;
			theFindFileData->file_size = aPakRecord->mSize;
			theFindFileData->create_time = aPakRecord->mFileTime;
			theFindFileData->last_write_time = aPakRecord->mFileTime;
			theFindFileData->last_access_time = aPakRecord->mFileTime;
			theFindData->mLastFind = aFileName;

			return true;
		}
	}
	anItr++;
}

IFileSearch* PakInterface::FindFirstFile(const char* theFileName, FileSearchInfo* theFindFileData) //Correct? | 990-1003
{
	PFindData* aFindData = new PFindData;

	char anUpperName[2048];
	FixFileName(theFileName, anUpperName);
	aFindData->mFindCriteria = anUpperName;
	aFindData->mDriverSearch = NULL;

	if (PFindNext(aFindData, theFindFileData))	
		return aFindData;

	delete aFindData;
	return NULL;
}

bool PakInterface::FindNextFile(IFileSearch* theFileSearch, FileSearchInfo* theFindFileData) //Correct? | 1006-1018
{
	PFindData* aFindData = new PFindData;

	if (aFindData->mDriverSearch)
	{
		if (PFindNext(aFindData, theFindFileData))
			return TRUE;

		aFindData->mDriverSearch = FindFirstFile(aFindData->mFindCriteria.c_str(), theFindFileData);
		return (aFindData->mDriverSearch != NULL);
	}

	return FindNextFile(aFindData->mDriverSearch, theFindFileData);
}

bool PakInterface::FindClose(IFileSearch* theFileSearch) //1021-1026
{
	if (theFileSearch)

		delete theFileSearch;
	return true;
}