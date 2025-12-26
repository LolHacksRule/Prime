#include "WindowsFileDriver.h"
#include "../../../SexyAppBase.h"
#include <shlobj.h>
#include <direct.h>
#include "../../../Debug.h"

using namespace Sexy;

static uint64 ConvertFILETIME(FILETIME& ft) //20-25
{
	LONGLONG ll = (__int64)ft.dwHighDateTime << 32;
	ll = ll + ft.dwLowDateTime - 116444736000000000;
	return (uint64)(ll / 10000000);
}

IFileDriver* IFileDriver::CreateFileDriver() //29-31
{
	return new WindowsFileDriver();
}

WindowsFile::WindowsFile(const std::string& filename, BYTE* buffer, ulong buffer_size, bool delete_buffer) //41-57
{
	mIsDirect = false;
	mReadSize = 0;
	mBuffer = buffer;
	mBufferSize = buffer_size;
	mDeleteBuffer = delete_buffer;
	mIsLoaded = false;
	mHasError = false;
	mReadPos = 0;

	if (!InitRead(filename))
	{
		mHasError = true;
	}
}

WindowsFile::WindowsFile(const std::string& filename) //60-74
{
	mIsDirect = true;
	mBuffer = NULL;
	mBufferSize = 0;
	mDeleteBuffer = false;
	mIsLoaded = false;
	mHasError = false;
	mReadSize = 0;
	mReadPos = 0;

	if (InitRead(filename) == NULL)
	{
		mHasError = true;
	}
}

bool WindowsFile::InitRead(const std::string& filename) //78-120
{
	ulong read;
	mHandle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL); //0x60000080?
	if (mHandle == INVALID_HANDLE_VALUE)
	{
		mHasError = true;
		return false;
	}
	else
	{
		mSize = GetFileSize(mHandle, 0);
		ZeroMemory(&mOverlapped, sizeof(mOverlapped));
		if (mIsDirect)
			return true;
		mReadPos = 0;
		mTotalReadSize = mSize + 2048 - mSize % 2048;
		DBG_ASSERTE(mTotalReadSize >= mBufferSize); //96
		int ret = mTotalReadSize >= 1048576 ? 1048576 : mTotalReadSize;
		mReadSize = ret; //?
		mReadSize = 0; //?
		if (ReadFile(mHandle, mBuffer, mReadSize, &read, &mOverlapped) != NULL)
			return true;
		ulong err = GetLastError();
		if (err == ERROR_IO_PENDING || err == ERROR_IO_INCOMPLETE)
			return true;
		else
		{
			mHasError = true;
			return false;
		}
	}
}

WindowsFile::~WindowsFile() //123-129
{
	if (mDeleteBuffer)
		//_aligned_free(mBuffer);
		delete mBuffer;
	Close();
}

bool WindowsFile::IsLoaded() //132-136
{
	if (mIsLoaded || mHasError) return mIsLoaded; //?
	AsyncLoad();
	return mIsLoaded;
}

bool WindowsFile::HasError() //139-143
{
	if (mIsLoaded || mHasError) return mHasError; //?
	AsyncLoad();
	return mHasError;
}

void WindowsFile::AsyncLoad() //? | 146-197
{
	ulong num_read;
	if (GetOverlappedResult(mHandle, &mOverlapped, &num_read, false) != NULL)
	{
		ulong err = GetLastError();
		if (err != ERROR_HANDLE_EOF)
		{
			if (err > ERROR_OPERATION_ABORTED && err <= ERROR_IO_PENDING)
				return;
			mHasError = true;
		}
	}

	if (mHasError)
		return;

	mReadPos += mReadSize; //?
	if (mReadPos < mTotalReadSize)
	{
		ulong res = mTotalReadSize - mReadPos > 1048576 ? 1048576 : mTotalReadSize - mReadPos;
		mReadSize = res; //?
		mReadSize = 0; //?
		ZeroMemory(&mOverlapped, 0);
		mOverlapped.Offset = mReadPos;
		ulong read;
		if (!ReadFile(mHandle, &mBuffer[mReadPos], mReadSize, &read, &mOverlapped))
		{
			ulong err = GetLastError();
			if (err != ERROR_IO_PENDING && err != ERROR_IO_INCOMPLETE)
				mHasError = true;
		}
	}
	else
		mIsLoaded = true;
}

bool WindowsFile::ForceLoad() //200-206
{
	while (!mIsLoaded && !mHasError)
		AsyncLoad();
	return !mHasError;
}

uchar* WindowsFile::GetBuffer() //209-211
{
	return mBuffer;
}

ulong WindowsFile::GetSize() //214-216
{
	return mSize;
}

void WindowsFile::Close() //219-225
{
	if (mHandle != INVALID_HANDLE_VALUE)
		CloseHandle(mHandle);

	mHandle = INVALID_HANDLE_VALUE;
}

void WindowsFile::DirectSeek(uint64 theSeekPoint)
{
	DBG_ASSERTE(mIsDirect); //229
	if (DirectReadStatus() != READ_PENDING)
		DBG_ASSERTE((theSeekPoint % 2048)==0); //231
	mReadPos = theSeekPoint;
}

bool WindowsFile::DirectRead(uchar* theBuffer, uint64 theReadSize) //? | 236-272
{
	DBG_ASSERTE(mIsDirect); //237
	if (DirectReadStatus() != READ_PENDING)
		return false;
	mHasError = false;
	DBG_ASSERTE((theReadSize % 2048)==0); //241
	mReadSize = theReadSize;

	RtlZeroMemory(&mOverlapped, sizeof mOverlapped);
	ulong read;
	int ret;
	mOverlapped.Offset = mReadPos;
	if (ReadFile(mHandle, theBuffer, theReadSize, &read, &mOverlapped))
	{
		mReadPos += theReadSize;
		return true;
	}
	else
	{
		ulong err = GetLastError();
		if (err == ERROR_IO_PENDING || err == ERROR_IO_INCOMPLETE)
		{
			mReadPos += theReadSize;
			return true;
		}
		else
		{
			mHasError = true;
			return false;
		}
	}
}

IFile::Status WindowsFile::DirectReadStatus() //274-305
{
	if (!mReadSize)
		return READ_COMPLETE;
	if (mHasError)
		return READ_ERROR;
	ulong aNumRead;
	int res;
	if (GetOverlappedResult(mHandle, &mOverlapped, &aNumRead, 0))
		return (IFile::Status)(mHasError || aNumRead < mReadSize);
	DWORD err = GetLastError();
	if (err == ERROR_HANDLE_EOF)
		return (IFile::Status)(mHasError || aNumRead < mReadSize);
	if (err == ERROR_IO_PENDING || err == ERROR_IO_INCOMPLETE)
	{
		mHasError = true;
		return READ_ERROR;
	}
	else
		return READ_PENDING;
}

uint64 WindowsFile::DirectReadBlockSize() //308-312
{
	return 2048;
}

WindowsMMapFile::WindowsMMapFile(const std::string& theFilename, bool isDirect) //321-338
{
	mIsDirect = isDirect;
	mReadSize = 0;
	mBuffer = NULL;
	mBufferSize = 0;
	mIsLoaded = false;
	mHasError = false;
	mReadPos = 0;

	mHandle = INVALID_HANDLE_VALUE;
	mMapping = INVALID_HANDLE_VALUE;


	if (!InitRead(theFilename))
	{
		mHasError = true;
	}
}

WindowsMMapFile::~WindowsMMapFile() //341-355
{
	Close();
	if (mMapping != INVALID_HANDLE_VALUE)
	{
		CloseHandle(mMapping);
		mMapping = INVALID_HANDLE_VALUE;
	}

	if (mHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(mHandle);
		mHandle = INVALID_HANDLE_VALUE;
	}
}

bool WindowsMMapFile::IsLoaded() //358-360
{
	return mIsLoaded;
}

bool WindowsMMapFile::HasError() //363-365
{
	return mHasError;
}

void WindowsMMapFile::AsyncLoad() //368-369
{
}

bool WindowsMMapFile::ForceLoad() //372-374
{
	return !mHasError;
}

uchar* WindowsMMapFile::GetBuffer() //377-379
{
	return mBuffer;
}

ulong WindowsMMapFile::GetSize() //382-384
{
	return mBufferSize;
}

void WindowsMMapFile::Close() //387-389
{

}

void WindowsMMapFile::DirectSeek(uint64 theSeekPoint) //392-397
{
	DBG_ASSERTE(mIsDirect); //393
	if (DirectReadStatus() != READ_PENDING)
		DBG_ASSERTE((theSeekPoint % 2048)==0); //395
	mReadPos = theSeekPoint;
}

bool WindowsMMapFile::DirectRead(uchar* theBuffer, uint64 theReadSize) //400-414
{
	DBG_ASSERTE(mIsDirect); //401
	if (DirectReadStatus() == READ_PENDING)
		return NULL;
	mHasError = false;
	DBG_ASSERTE((theReadSize % 2048)==0); //407
	if (mBufferSize >= theReadSize + mReadPos)
		mBufferSize = theReadSize + mReadPos;
	else
		mReadSize = mBufferSize - mReadPos;


	memcpy(theBuffer, &mBuffer[mReadPos], mReadSize);
	return true;
}

IFile::Status WindowsMMapFile::DirectReadStatus() //417-419
{
	return READ_COMPLETE;
}

uint64 WindowsMMapFile::DirectReadBlockSize() //422-424
{
	return 2048;
}

bool WindowsMMapFile::InitRead(const std::string& theFileName) //427-482
{
	mHandle = CreateFile(theFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (mHandle != INVALID_HANDLE_VALUE)
	{
		HMODULE aModule = GetModuleHandle("");
		HRSRC aFindRes = FindResource(aModule, theFileName.c_str(), "PAK");
		if (aFindRes == NULL)
			return false;
		HGLOBAL aResGlob = LoadResource(aModule, aFindRes);
		if (aResGlob == NULL)
			return false;
		mMapping = INVALID_HANDLE_VALUE;
		mBuffer = (uchar*)LockResource(aResGlob);
		if (mBuffer == NULL)
		{
			CloseHandle(mHandle);
			mHandle = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	else
	{
		mBufferSize = GetFileSize(mHandle, 0);
		mMapping = CreateFileMapping(mHandle, 0, PAGE_READONLY, 0, mBufferSize, 0);
		if (!mMapping)
		{
			CloseHandle(mHandle);
			return false;
		}
		mBuffer = (uchar*)MapViewOfFile(mMapping, PAGE_READWRITE, 0, 0, mBufferSize);
		if (!mBuffer)
		{
			CloseHandle(mMapping);
			CloseHandle(mHandle);
			mHandle = INVALID_HANDLE_VALUE;
			mMapping = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	mIsLoaded = true;
	return true;
}

WindowsFileDriver::WindowsFileDriver() //492-496
{
	mApp = NULL;
	mSaveDataFolder = "";
	mLoadDataFolder = "";
}

WindowsFileDriver::~WindowsFileDriver() //499-500
{
}

typedef HRESULT(WINAPI* SHGetFolderPathFunc)(HWND, int, HANDLE, DWORD, LPTSTR);
void* GetSHGetFolderPath(const char* theDLL, HMODULE* theMod) //504-520
{
	HMODULE aMod = LoadLibrary(theDLL);
	SHGetFolderPathFunc aFunc = NULL;

	if (aMod != NULL)
	{
		*((void**)&aFunc) = (void*)GetProcAddress(aMod, "SHGetFolderPathA");
		if (aFunc == NULL)
		{
			FreeLibrary(aMod);
			aMod = NULL;
		}
	}

	*theMod = aMod;
	return aFunc;
}

bool WindowsFileDriver::InitFileDriver(SexyAppBase* app) //523-533
{
	mApp = app;
	char aModuleFileName[255];
	RtlZeroMemory(aModuleFileName, sizeof aModuleFileName);
	GetModuleFileName(GetModuleHandle(NULL), aModuleFileName, sizeof aModuleFileName);
	mLoadDataFolder = GetFileDir(aModuleFileName, true);

	InitSaveDataFolder();

	return true;
}

void WindowsFileDriver::InitSaveDataFolder() //Correct? | 536-559
{
	mSaveDataFolder = "";
	if (CheckForVista())
	{
		HMODULE aMod;
		SHGetFolderPathFunc aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath("shell32.dll", &aMod);
		if (aFunc == NULL || aMod == NULL)
			aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath("shfolder.dll", &aMod);

		if (aMod != NULL)
		{
			char aPath[MAX_PATH];
			aFunc(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, aPath);

			std::string aDataPath = RemoveTrailingSlash(aPath) + "\\" + mApp->mFullCompanyName + "\\" + mApp->mProdName;
			mSaveDataFolder = (aDataPath + "\\");

			FreeLibrary(aMod);
		}
	}
}

std::string WindowsFileDriver::GetSaveDataPath() //562-564
{
	return mSaveDataFolder;
}

std::string WindowsFileDriver::FixPath(const std::string& inFileName) //567-579
{
	char out[MAX_PATH];

	strcpy(out, inFileName.c_str());
	for (char* chscan = out; *chscan; chscan++)
	{
		if (chscan == "/")
			chscan = "\\";
	}

	return out;
}

std::string WindowsFileDriver::GetCurPath() //583-587
{
	//
	char temp[MAX_PATH];
	return getcwd(temp, sizeof(temp));
}

void WindowsFileDriver::SetLoadDataPath(const std::string& path) //590-592
{
	mLoadDataFolder = path;
}

std::string WindowsFileDriver::GetLoadDataPath() //595-597
{
	return mLoadDataFolder;
}

IFile* WindowsFileDriver::CreateFile(const std::string& path) //600-611
{
	LPWIN32_FILE_ATTRIBUTE_DATA data;
	int file_size; //?
	if (!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, data))
		return NULL;
	int buffer_size = file_size + 2048 - file_size % 2048;
	uchar* buffer = (uchar*)_aligned_malloc(buffer_size, 4096u);


	return CreateFileWithBufferAndAttrData(path, buffer, buffer_size, data, true);
}

IFile* WindowsFileDriver::CreateFileDirect(const std::string& thePath) //614-623
{
	std::string fixPath = FixPath(thePath);
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesA(fixPath.c_str()), GetFileExInfoStandard, &data)
		return new WindowsFile(fixPath);
	else
		return false;
}

IFile* WindowsFileDriver::CreateFileWithBuffer(const std::string& path, BYTE* buffer, ulong buffer_size) //626-639
{
	LPWIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, data))
		return NULL;
	int file_size = data->nFileSizeLow + 2048 - (int)data->nFileSizeLow % 2048; //?
	ulong buffer_size_should_be = file_size;
	DBG_ASSERTE(buffer_size >= buffer_size_should_be); //633

	if (buffer_size < buffer_size_should_be)
		return CreateFileWithBufferAndAttrData(path, buffer, buffer_size, data, false);
	else
		return NULL;
}

IFile* WindowsFileDriver::CreateFileWithBufferAndAttrData(const std::string& path, BYTE* buffer, ulong buffer_size, LPWIN32_FILE_ATTRIBUTE_DATA attrData, bool delete_buffer) //643-645
{
	return new WindowsFile(path, buffer, buffer_size, delete_buffer);
}

IFile* WindowsFileDriver::CreateFileMemoryMapped(const std::string& thePath) //648-650
{
	return new WindowsMMapFile(thePath, false);
}

uint64 WindowsFileDriver::GetFileSize(const std::string& path) //653-663
{
	LPWIN32_FILE_ATTRIBUTE_DATA data;

	if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, data))
		return data->nFileSizeLow; //?
	else
		return 0; //?
	return 0; //?
}

uint64 WindowsFileDriver::GetFileTime(const std::string& path) //? | 666-676
{
	LPWIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, data))
		return ConvertFILETIME(data->ftLastWriteTime);
	else
		return 0;
}

bool WindowsFileDriver::FileExists(const std::string& path, bool* isFolder) //679-693
{
	LPWIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, data))
		return false;
	if (isFolder)
		*isFolder = (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	return true;
}

bool WindowsFileDriver::MakeFolders(const std::string& theDir) //Carbon copy of 130 Sexy::MkDir | 696-715
{
	std::string aPath = theDir;

	int aCurPos = 0;
	for (;;)
	{
		int aSlashPos = aPath.find_first_of("\\/", aCurPos);
		if (aSlashPos == -1)
		{
			_mkdir(aPath.c_str());
			break;
		}

		aCurPos = aSlashPos + 1;

		std::string aCurPath = aPath.substr(0, aSlashPos);
		_mkdir(aCurPath.c_str());
	}
	return true;
}

bool WindowsFileDriver::DeleteTree(const std::string& thePath) //Carbon copy of Sexy::Deltree | 718-761
{
	bool success = true;

	std::string aSourceDir = thePath;

	if (aSourceDir.length() < 2)
		return false;

	if ((aSourceDir[aSourceDir.length() - 1] != '\\') ||
		(aSourceDir[aSourceDir.length() - 1] != '/'))
		aSourceDir += "\\";

	WIN32_FIND_DATAA aFindData;

	HANDLE aFindHandle = FindFirstFile((aSourceDir + "*.*").c_str(), &aFindData);
	if (aFindHandle == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		if ((aFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			if ((strcmp(aFindData.cFileName, ".") != 0) &&
				(strcmp(aFindData.cFileName, "..") != 0))
			{
				// Follow the directory
				if (!Deltree(aSourceDir + aFindData.cFileName))
					success = false;
			}
		}
		else
		{
			std::string aFullName = aSourceDir + aFindData.cFileName;
			if (!DeleteFile(aFullName.c_str()))
				success = false;
		}
	} while (FindNextFile(aFindHandle, &aFindData));
	FindClose(aFindHandle);

	if (rmdir(thePath.c_str()) == 0)
		success = false;

	return success;
}

bool WindowsFileDriver::DeleteFile(const std::string& path) //764-766
{
	return DeleteFile(path.c_str());
}

static void ConvertFindDataToSearchInfo(FileSearchInfo* out, LPWIN32_FIND_DATAA in) //770-777
{
	out = (FileSearchInfo*)in->cFileName;
	out->is_directory = (in->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	out->create_time = ConvertFILETIME(in->ftCreationTime);
	out->last_access_time = ConvertFILETIME(in->ftLastAccessTime);
	out->last_write_time = ConvertFILETIME(in->ftLastWriteTime);
	out->file_size = in->nFileSizeLow;
}

IFileSearch* WindowsFileDriver::FileSearchStart(const std::string& criteria, FileSearchInfo* findInfo) //780-792
{
	WIN32_FIND_DATAA findData;
	HANDLE hSearch = FindFirstFile(criteria.c_str(), &findData);;
	WindowsFileSearch* search = new WindowsFileSearch(hSearch);

	if (hSearch == INVALID_HANDLE_VALUE)
		return NULL;
	ConvertFindDataToSearchInfo(findInfo, &findData);
	return search;
}

bool WindowsFileDriver::FileSearchNext(IFileSearch* isearch, FileSearchInfo* findInfo) //795-808
{
	LPWIN32_FIND_DATAA findData;
	if (!isearch)
		return false;
	WindowsFileSearch* search = (WindowsFileSearch*)isearch; //?
	if (search->mSearchHandle == INVALID_HANDLE_VALUE)
		return false;
	if (FindNextFile(&search->mSearchHandle, findData))
		return false;
	ConvertFindDataToSearchInfo(findInfo, findData);
	return true;
}

bool WindowsFileDriver::FileSearchEnd(IFileSearch* isearch) //811-824
{
	WindowsFileSearch* search;
	if (!isearch)
		return false;
	if (search->mSearchHandle == INVALID_HANDLE_VALUE)
	{
		delete isearch;
		return false;
	}
	else
	{
		FindClose(search->mSearchHandle);
		delete isearch;
		return true;
	}
}