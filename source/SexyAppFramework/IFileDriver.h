#ifndef __IFILEDRIVER_H__
#define __IFILEDRIVER_H__

#include "SexyAppBase.h"

namespace Sexy
{
    //class SexyAppBase;
    struct FileSearchInfo
    {
        std::string file_name;
        bool is_directory;
        uint64 create_time;
        uint64 last_write_time;
        uint64 last_access_time;
        uint64 file_size;
    };
    class IFile
    {
    public:
        enum Status
        {
            READ_COMPLETE,
            READ_PENDING,
            READ_ERROR
        };
    public:
        virtual ~IFile() {} //36
        virtual bool IsLoaded() = 0;
        virtual bool HasError() = 0;
        virtual void AsyncLoad() = 0;
        virtual bool ForceLoad() = 0;
        virtual uchar* GetBuffer() = 0;
        virtual ulong GetSize() = 0;
        virtual void Close() = 0;
        virtual void DirectSeek(uint64 theSeekPoint) = 0;
        virtual bool DirectRead(uchar* theBuffer, uint64 theReadSize) = 0;
        virtual Status DirectReadStatus() = 0;
        virtual uint64 DirectReadBlockSize() = 0;
    };
    class IFileSearch
    {
    public:
        enum SearchType
        {
            UNKNOWN,
            PAK_FILE_INTERNAL,
            DRIVER_INTERNAL
        };
    public:
        virtual ~IFileSearch() {} //108
        SearchType GetSearchType();
    protected:
        SearchType mSearchType;
        IFileSearch() { mSearchType = UNKNOWN; } //124
    };
    class IFileDriver //Everything is named 1:1 as Windows calls Windows APIs
    {
    public:
        static IFileDriver* CreateFileDriver();
        virtual ~IFileDriver() {} //134
        IFileDriver();
        virtual bool InitFileDriver(SexyAppBase* app);
        virtual void InitSaveDataFolder() = 0;
        virtual std::wstring FixPath(const std::wstring& theFileName)
        {
            return StringToWString(FixPath(WStringToString(theFileName, false))); //What | 147-149
        }
//#ifdef _WIN32
        virtual std::string FixPath(const std::string& theFileName) = 0;
//#endif
        virtual std::string GetSaveDataPath() = 0;
        virtual std::string GetCurPath() = 0;
        virtual void SetLoadDataPath(const std::string& path) {} //165
        virtual std::string GetLoadDataPath() = 0;
        virtual IFile* CreateFile(const std::wstring& thePath) //171-173
        {
            return CreateFile(WStringToString(thePath, false)); //Do we use CreateFile or use CreateFileA, it seems to be the first in XNA so assuming and probably accurate
        }
//#ifdef _WIN32
        virtual IFile* CreateFile(const std::string& thePath) = 0;
//#endif
        virtual IFile* CreateFileWithBuffer(const std::wstring& thePath, uchar* theBuffer, ulong theBufferSize) //184-186
        {
            return CreateFileWithBuffer(WStringToString(thePath, false), theBuffer, theBufferSize);
        }
//#ifdef _WIN32
        virtual IFile* CreateFileWithBuffer(const std::string& thePath, uchar* theBuffer, ulong theBufferSize) = 0;
//#endif
        virtual IFile* CreateFileDirect(const std::wstring& thePath) //196-198
        {
            return CreateFileDirect(WStringToString(thePath, false));
        }
//#ifdef _WIN32
        virtual IFile* CreateFileDirect(const std::string& thePath) = 0;
//#endif
        virtual bool SupportsMemoryMappedFiles() { return false; } //205

        virtual IFile* CreateFileMemoryMapped(const std::string& thePath) = 0; //207


        virtual IFile* CreateFileMemoryMapped(const std::wstring& thePath) //210-212
        {
            return CreateFileMemoryMapped(WStringToString(thePath, false));
        }

        virtual uint64 GetFileSize(const std::wstring& thePath) //219-222
        {
            return GetFileSize(WStringToString(thePath, false));
        }
//#ifdef _WIN32
        virtual uint64 GetFileSize(const std::string& thePath) = 0;
//#endif
        virtual uint64 GetFileTime(const std::wstring& thePath) //227-229
        {
            return GetFileTime(WStringToString(thePath, false));
        }
//#ifdef _WIN32
        virtual uint64 GetFileTime(const std::string& thePath) = 0;
//#endif
        virtual bool FileExists(const std::wstring& thePath, bool* isFolder) //234-236
        {
            return FileExists(WStringToString(thePath, false), isFolder);
        }
//#ifdef _WIN32
        virtual bool FileExists(const std::string&, bool* isFolder) = 0;
//#endif
        virtual bool MakeFolders(const std::wstring& theFolder) //246-248
        {
            return MakeFolders(WStringToString(theFolder, false));
        }
//#ifdef _WIN32
        virtual bool MakeFolders(const std::string& theFolder) = 0;
//#endif
        virtual bool DeleteTree(const std::wstring& thePath) //253-255
        {
            return DeleteTree(WStringToString(thePath, false));
        }
//#ifdef _WIN32
        virtual bool DeleteTree(const std::string& thePath) = 0;
//#endif
        virtual bool DeleteFile(const std::wstring& thePath) //260-262
        {
            return DeleteFile(WStringToString(thePath, false));
        }
//#ifdef _WIN32
        virtual bool DeleteFile(const std::string& thePath) = 0;
//#endif

#ifndef _WIN32
        virtual bool MoveFile(const std::wstring& thePathSrc, const std::wstring& thePathDest) = 0; //Present on EAMT
#endif

        virtual IFileSearch* FileSearchStart(const std::wstring& theCriteria, FileSearchInfo* outInfo) //268-270
        {
            return FileSearchStart(WStringToString(theCriteria, false), outInfo);
        }
//#ifdef _WIN32
        virtual IFileSearch* FileSearchStart(const std::string& theCriteria, FileSearchInfo* outInfo) = 0;
//#endif
        virtual bool FileSearchNext(IFileSearch* theSearch, FileSearchInfo* theInfo) = 0;
        virtual bool FileSearchEnd(IFileSearch* theSearch) = 0;
#ifndef _WIN32
        virtual bool MoveFile(const std::string& thePathSrc, const std::string thePathDest) { return false; } //Present and false on EAMT
#endif
    };
};

#endif //__IFILEDRIVER_H__