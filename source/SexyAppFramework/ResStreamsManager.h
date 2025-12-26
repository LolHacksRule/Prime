#ifndef __RESSTREAMSMANAGER_H__
#define __RESSTREAMSMANAGER_H__

#include "CompiledMap.h"
#include "ResStreamsFormat.h"
#include "IFileDriver.h"

namespace Sexy
{
    class ResStreamsManager
    {
    public:
        enum GroupStatus
        {
            NOT_RESIDENT,
            PREPARING,
            RESIDENT, //3 in mobile
            DELETING
        };
    private:
        SexyAppBase* mApp;
        bool mHasError;
        bool mIsIdle;
        IFile* mFile;
        uchar* mCommonBuffer;
        size_t mCommonSize;
        CompiledMap mFileToGroupIndex;
        CompiledMap mIdIndex;
        CompiledMap mGroupNameToIdIndex;
        uchar* mReadBuffer[2]; //No clue what these two are
        uchar* mCurReadBuffer;
        uchar* mCurDecodeBuffer;
        bool mWaitingOnRead;
        bool mWaitingOnDecode;
        size_t mCurReadSize;
        ulong mNumGroups;
        ResStreamsGroup* mGroups;
        ulong mNumPools;
        ResStreamsPool* mPools;
        ResStreamsLoadDesc mCurLoadDesc;
        ResStreamsLoadList mLoadingQueue;
        ulong mCurReadBytes;
        ulong mCurReadState;
        bool mCopyingTexture;
        ulong mCurTextureIndex;
        ulong mCurGPUDataPtr;
        size_t mSizeOfCurTexture;
    public:
        ResStreamsManager(SexyAppBase* theApp);
        virtual ~ResStreamsManager();
        bool InitializeWithRSB(const std::string& theFileName);
        bool IsInitialized();
        ulong LookupGroup(const std::string& theGroupName);
        bool IsGroupLoaded(ulong theGroupId);
        bool IsGroupLoaded(const std::string& theGroupName);
        GroupStatus GetGroupStatus(ulong theGroupId);
        GroupStatus GetGroupStatus(const std::string& theGroupName);
        bool LoadGroup(ulong theGroupId);
        bool LoadGroup(const std::string& theGroupName);
        bool ForceLoadGroup(ulong theGroupId);
        bool ForceLoadGroup(const std::string& theGroupName);
        bool CanLoadGroup(ulong theGroupId);
        bool CanLoadGroup(std::string theGroupName);
        bool DeleteGroup(ulong theGroupId);
        bool DeleteGroup(const std::string& theGroupName);
        bool HasError();
        bool HasGlobalFileIndex();
        ulong GetGroupForFile(const std::string& theFileName);
        ulong GetLoadedGroupForFile(const std::string& theFileName);
        bool GetResidentFileBuffer(ulong theGroupId, const std::string& theFileName, uchar** theBuffer, ulong* theSize);
        bool GetImage(ulong theGroupId, const std::string& theFileName, Image** theOutImage);
        void Update();
        void DebugDraw(Graphics* g);
    protected:
        void FlagError();
        uchar* GetLoadedFileData(const std::string& theFileName, ulong& theGroup);
        void StartGPUDataTransfer();
    };
    class ResStreamsGroup
    {
    public:
        ResStreamsManager::GroupStatus mStatus;
        CompiledMap mFileIndex;
        uint mPoolIndex;
        ResStreamsPool* mPool;
        ulong mFileLocation;
        ulong mFileSize;
        ResStreamGroupDescriptor* mDesc;
        uint64 mTotalLoadTime;
        uint64 mDecodeTime;
        uint64 mDiskIOTime;
        uint64 mLoadStartTime;
        uint64 mDecodeStartTime;
        uint64 mDiskIOStartTime;
        ulong mLoadedAtInstance;
        ResStreamGroupHeader* mLoadedHeader;
        ResStreamsGroup();
        ~ResStreamsGroup();
        void InitDescriptor(ResStreamGroupDescriptor* theDesc);
        void InitGroupStream(ResStreamGroupHeader* theHeader, ulong theTextureId);
        ResStreamsManager::GroupStatus GetStatus() { return mStatus; } //278
        uchar* GetFileIndexData(const std::string& theFileName);
        void StartLoad();
        bool CanLoad();
        uchar* GetResidentDataBaseAddress();
        uchar* GetGPUDataBaseAddress();
        void* GetTextureReference(ulong theTextureId);
        ulong GetNumTextures();
        const std::string mName;
    };
}
#endif