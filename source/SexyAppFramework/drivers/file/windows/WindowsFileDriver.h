#ifndef __WINDOWSFILEDRIVER_H__
#define __WINDOWSFILEDRIVER_H__

//#include "../../../SexyAppBase.h"
#include "../../../IFileDriver.h"

namespace Sexy
{
    class WindowsFile : public IFile
    {
    public:
        WindowsFile(const std::string& filename);
        WindowsFile(const std::string& filename, BYTE* buffer, ulong buffer_size, bool delete_buffer);
        ~WindowsFile();
        bool IsLoaded();
        bool HasError();
        void AsyncLoad();
        bool ForceLoad();
        uchar* GetBuffer();
        ulong GetSize();
        void Close();
        void DirectSeek(uint64 theSeekPoint);
        bool DirectRead(uchar* theBuffer, uint64 theReadSize);
        Status DirectReadStatus();
        uint64 DirectReadBlockSize();
    protected:
        bool InitRead(const std::string& filename);
    private:
        bool mDeleteBuffer;
        UCHAR* mBuffer;
        ulong mBufferSize;
        ulong mTotalReadSize;
        ulong mSize;
        bool mIsLoaded;
        bool mHasError;
        bool mIsDirect;
        uint64 mReadSize;
        HANDLE mHandle; //volatile?
        OVERLAPPED mOverlapped;
        ulong mReadPos;
    };
    class WindowsMMapFile : public IFile
    {
    private:
        uchar* mBuffer;
        ulong mBufferSize;
        bool mIsLoaded;
        bool mHasError;
        bool mIsDirect;
        uint64 mReadSize;
        HANDLE mHandle;
        HANDLE mMapping;
        ulong mReadPos;
    public:
        WindowsMMapFile(const std::string& theFileName, bool isDirect);
        ~WindowsMMapFile();
        bool IsLoaded();
        bool HasError();
        void AsyncLoad();
        bool ForceLoad();
        uchar* GetBuffer();
        ulong GetSize();
        void Close();
        void DirectSeek(uint64 theSeekPoint);
        bool DirectRead(uchar* theBuffer, uint64 theReadSize);
        Status DirectReadStatus();
        uint64_t DirectReadBlockSize();
    protected:
        bool InitRead(const std::string& theFilename);
    };
    class WindowsFileSearch : public IFileSearch
    {
    public:
        HANDLE mSearchHandle;
        WindowsFileSearch(HANDLE searchHandle) { mSearchHandle = searchHandle; } //109
        virtual ~WindowsFileSearch() {} //110
    };
    class WindowsFileDriver : public IFileDriver
    {
    public:
        WindowsFileDriver();
        ~WindowsFileDriver();
        bool InitFileDriver(SexyAppBase* app);
        void InitSaveDataFolder();
        std::string FixPath(const std::string& inFileName);
        std::string GetSaveDataPath();
        std::string GetCurPath();
        void SetLoadDataPath(const std::string& path);
        std::string GetLoadDataPath();
        IFile* CreateFile(const std::string& path); //Not CreateFile* EXCEPT on Windows
        IFile* CreateFileDirect(const std::string& thePath);
        IFile* CreateFileWithBuffer(const std::string& path, BYTE* buffer, ulong buffer_size = 0);
        virtual IFile* CreateFileWithBufferAndAttrData(const std::string& path, BYTE* buffer, ulong buffer_size, LPWIN32_FILE_ATTRIBUTE_DATA attrData, bool delete_buffer);
        bool SupportsMemoryMappedFiles() { return true; } //143
        IFile* CreateFileMemoryMapped(const std::string& thePath);
        uint64 GetFileSize(const std::string& path);
        uint64 GetFileTime(const std::string& path); //help
        bool FileExists(const std::string& path, bool* isFolder); //TODO
        bool MakeFolders(const std::string& theDir);
        bool DeleteTree(const std::string&);
        bool DeleteFile(const std::string& path); //Not DeleteFile* EXCEPT on Windows
        IFileSearch* FileSearchStart(const std::string& criteria, FileSearchInfo* findInfo);
        bool FileSearchNext(IFileSearch* isearch, FileSearchInfo* findInfo);
        bool FileSearchEnd(IFileSearch* isearch); //TODO
    private:
        SexyAppBase* mApp;
        std::string mSaveDataFolder;
        std::string mLoadDataFolder;
    };
}
#endif //__WINDOWSFILEDRIVER_H__