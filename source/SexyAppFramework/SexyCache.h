#ifndef __SEXYCACHE_H__
#define __SEXYCACHE_H__

#include "CritSect.h"

typedef std::map<void*, HANDLE> PtrToHandleMap;

namespace Sexy
{
#if !defined(EAMT) && !defined(ANDROID) && !defined(APPLE)
    struct SexyDataHeader
    {
        char mSrcFile[256];
        char mDataType[512];
        int mSize;
    };
    struct SexyCacheData
    {
        int mVersion;
        HWND mHWnd;
        int mCachedItemCount;
        int mCacheSize;
    };
#endif

    class SexyCache //C++ only
    {
#ifndef _ANDROID
    public:
        CritSect mCritSect;
        SexyCacheData mSexyCacheData;
        uint mSexyCacheMessage;
        PtrToHandleMap mAllocDataMap;
        SexyCache();
        ~SexyCache();
        bool Connected();
        void Disconnect();
        bool HadCachedObjects();
        void CheckFileChanges(); //? is WM_MOVE correct
        bool  CheckData(const std::string& theSrcFile, const std::string& theDataType); //todo
        bool  SetUpToDate(const std::string& theSrcFile, const std::string& theDataType); //todo
        bool  GetData(const std::string& theSrcFile, const std::string& theDataType, void** thePtr, int* theSize); //todo
        void  FreeGetData(HANDLE theGetDataPtr);
        void  SetFileDeps(const std::string& theSrcFile, const std::string& theDataType, const std::string& theFileDeps); 
        void* AllocSetData(const std::string& theSrcFile, const std::string& theDataType, int theSize); //todo
        void  FreeSetData(void* theSetDataPtr);
        bool  SetData(void* theSetDataPtr);
#endif
    };
    extern SexyCache gSexyCache;
    void SMemW(void** theDest, const void* theSrc, int theSize);
    void SMemWStr(void** theDest, const std::string& theStr);
    void SMemR(void** theSrc, void* theDest, int theSize);
    void SMemRStr(void** theDest, std::string* theStr);
}
#endif