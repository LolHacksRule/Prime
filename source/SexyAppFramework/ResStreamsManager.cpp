#include "ResStreamsManager.h"
#include "IResStreamsDriver.h"
#include "Debug.h"
#include "SexyTime.h"
//#include "DeviceImage.h"

using namespace Sexy;

static void Align(size_t& theSize, int theBytes) //35-38
{
    int mod = theSize % theBytes;
    theSize += mod ? theBytes - mod : 0;
}

///////////////////////////////////////////////////////////////////////////////
ResStreamsManager::ResStreamsManager(SexyAppBase* theApp) //C++ only | 42-67
{
    mApp = theApp;
    mIsIdle = true;
    mHasError = false;
    mCommonBuffer = NULL;
    mCommonSize = 0;
    memset(mReadBuffer[RESSTREAM_RESIDENT], 0, 0x100000u); //No clue what these are
    memset(mReadBuffer[RESSTREAM_GPU_TRANSFER], 0, 0x100000u); //No clue what these are
    mCommonBuffer = NULL;
    mGroups = NULL;
    mFile = NULL;
    mNumGroups = 0;
    mCurLoadDesc.group = NULL;
    mCurLoadDesc.pool = NULL;
    mCurLoadDesc.instance = -1; //No clue what this is
    *&mCurReadBuffer = *mReadBuffer; //?
    mWaitingOnRead = false;
    mWaitingOnDecode = false;
}

ResStreamsManager::~ResStreamsManager() //70-75
{
    delete mReadBuffer[RESSTREAM_RESIDENT];
    delete mReadBuffer[RESSTREAM_GPU_TRANSFER];
    delete mCommonBuffer;
    delete mGroups;
}

bool ResStreamsManager::IsInitialized() //78-80
{
    return mFile != NULL;
}

bool ResStreamsManager::InitializeWithRSB(const std::string& theFileName) //Accurate? Not finished on Win | 83-157
{
    if (mApp->mResStreamsDriver == NULL)
        return false;

    uint64 startTime = SexyTime();

    mFile = mApp->mFileDriver->CreateFileDirect(mApp->mFileDriver->GetLoadDataPath() + theFileName);
    if (mFile == NULL)
        return false;
    if (mFile->DirectRead(mReadBuffer[RESSTREAM_RESIDENT], 2048) == NULL)
        return false;

    while (mFile->DirectReadStatus() == IFile::READ_PENDING);

    if (mFile->DirectReadStatus() == IFile::READ_ERROR)
        return false;

    ResStreamHeader* aHeader = (ResStreamHeader*)mReadBuffer[RESSTREAM_RESIDENT]; //?
    mCommonSize = aHeader->v1.common_data_size;
    mCommonBuffer = new uchar[mCommonSize];
    memcpy(mCommonBuffer, mReadBuffer[RESSTREAM_RESIDENT], 2048); //?
    if (mCommonSize > 2048 && !mFile->DirectRead(mCommonBuffer + 2048, mCommonSize - 2048) == NULL)
        return false;

    while (mFile->DirectReadStatus() == IFile::READ_PENDING);

    if (mFile->DirectReadStatus() == IFile::READ_ERROR)
        return false;

    aHeader = (ResStreamHeader*)mCommonBuffer;//?
    if (aHeader->v1.file_index_location != -1)
        mFileToGroupIndex.Init(&mCommonBuffer[aHeader->v1.file_index_location], aHeader->v1.file_index_size);

    mGroupNameToIdIndex.Init(&mCommonBuffer[aHeader->v1.group_name_to_id_location], aHeader->v1.group_name_to_id_size);
    mNumPools = aHeader->v1.groups_count;
    ulong count = aHeader->v1.pools_count;
    //?
    //mPools = new ResStreamsPool(count);
    for (ulong i = 0; i < aHeader->v1.pools_count; ++i)
    {
        ResStreamPoolDescriptor* aPoolDesc = (ResStreamPoolDescriptor*)mCommonBuffer[aHeader->v1.pools_location + aHeader->v1.pool_header_size * i];
        mPools[i].InitDescriptor(aPoolDesc, &mCommonBuffer[aHeader->v1.texture_desc_location], aHeader->v1.texture_desc_size);
    }
    mNumGroups = aHeader->v1.groups_count;
    mGroups = new ResStreamsGroup();
    for (ulong i = 0; i < aHeader->v1.pools_count; ++i)
    {
        ResStreamGroupDescriptor* aGroupDesc = (ResStreamGroupDescriptor*)mCommonBuffer[aHeader->v1.groups_location + aHeader->v1.group_header_size * i];
        mGroups[i].InitDescriptor(aGroupDesc);
        mGroups[i].mPoolIndex = aGroupDesc->pool;
        mGroups[i].mPool = &mPools[aGroupDesc->pool];
    }

    uint64 endTime = SexyTime();
    OutputDebugStrF("LoadResStreamPackage: %dms\n", endTime - startTime);
    return true;
}

ulong ResStreamsManager::LookupGroup(const std::string& theGroupName) //160-164
{
    ulong* foundId = (ulong*)mGroupNameToIdIndex.Find(theGroupName.c_str());
    if (foundId) return *foundId;
    return -1;
}

bool ResStreamsManager::IsGroupLoaded(const std::string& theGroupName) //167-169
{
    return IsGroupLoaded(LookupGroup(theGroupName));
}

bool ResStreamsManager::IsGroupLoaded(ulong theGroupId) //172-176
{
    if (theGroupId == -1)
        return false;

    return GetGroupStatus(theGroupId) == RESIDENT; //3 in mobile prime
}

ResStreamsManager::GroupStatus ResStreamsManager::GetGroupStatus(const std::string& theGroupName) //179-181
{
    return GetGroupStatus(LookupGroup(theGroupName));
}

ResStreamsManager::GroupStatus ResStreamsManager::GetGroupStatus(ulong theGroupId) //todo changed on mobile | 184-187
{
    if (theGroupId == -1) return NOT_RESIDENT;
#ifdef _SEXYDECOMP_USE_LATEST_CODE //TODO
    if (theGroupId && 0x10000000 == 0) return mGroups[theGroupId].GetStatus();
    int aCompositeNum GetComposite(theGroupId & -1);
    for (int i = 0; i < aCompositeNum; i++)
    {
        if (IsCompositeChildActive(aCompositeNum))
        {
            if (mGroups[theGroupId].GetStatus() > 3)
                return mGroups[theGroupId].GetStatus();
        }
    }
#else
    return mGroups[theGroupId].GetStatus();
#endif
}

bool ResStreamsManager::LoadGroup(const std::string& theGroupName) //190-192
{
    return LoadGroup(LookupGroup(theGroupName));
}

bool ResStreamsManager::LoadGroup(ulong theGroupId) //195-212
{
    ResStreamsLoadDesc aLoadDesc;

    if (theGroupId == -1) return false;
    if (!CanLoadGroup(theGroupId)) return false;
    mIsIdle = false;
    aLoadDesc.group = &mGroups[theGroupId];
    aLoadDesc.pool = &mPools[aLoadDesc.group->mPoolIndex];
    aLoadDesc.instance = aLoadDesc.pool->LockInstanceForGroup(aLoadDesc.group);
    mLoadingQueue.push_back(aLoadDesc);
    mGroups[theGroupId].mStatus = PREPARING;
    return true;
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
int ResStreamsManager::LoadGenericResFileFromManifest(*) //Not on PC
{
    //todo
    GenericResFileRes* aFile = new GenericResFileRes();
    if (LoadCommonResourceFromManifest())
}
#endif

bool ResStreamsManager::ForceLoadGroup(const std::string& theGroupName) //215-217
{
    return ForceLoadGroup(LookupGroup(theGroupName));
}

bool ResStreamsManager::ForceLoadGroup(ulong theGroupId) //220-245
{
    if (theGroupId == -1)
        return false;
    GroupStatus theStatus = GetGroupStatus(theGroupId);
    if (theStatus == RESIDENT)
        return true;
    OutputDebugStrF("WARNING: (performance) Force loading group %s\n", mGroups[theGroupId]);
    if (theStatus == NOT_RESIDENT && !LoadGroup(theGroupId))
        return false;
    while (!IsGroupLoaded(theGroupId))
    {
        SexySleep(0);
        if (HasError())
            return false;
        Update();
    }
    return IsGroupLoaded(theGroupId);
}

bool ResStreamsManager::CanLoadGroup(std::string theGroupName) //248-250
{
    return CanLoadGroup(LookupGroup(theGroupName));
}

bool ResStreamsManager::CanLoadGroup(ulong theGroupId) //253-257
{
    if (theGroupId == -1)
        return false;

    return mGroups[theGroupId].CanLoad();
}

bool ResStreamsManager::DeleteGroup(const std::string& theGroupName) //260-262
{
    return DeleteGroup(LookupGroup(theGroupName));
}

bool ResStreamsManager::DeleteGroup(ulong theGroupId) //265-269
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE //Present in non-Windows/RT, not implemented in Bej3 JP
    if (theGroupId == -1)
        return false;

    else if (theGroupId & 0x10000000)
#else
    return false;
#endif
}

bool ResStreamsManager::HasError() //272-274
{
    return mHasError;
}

bool ResStreamsManager::HasGlobalFileIndex() //277-279
{
    return mFileToGroupIndex.Initialized();
}

ulong ResStreamsManager::GetGroupForFile(const std::string& theFileName) //282-288
{
    if (HasGlobalFileIndex())
        return -1;

    ulong* theGroup = (ulong*)mFileToGroupIndex.Find(theFileName.c_str());
    return theGroup ? *theGroup : -1;
}

ulong ResStreamsManager::GetLoadedGroupForFile(const std::string& theFileName) //291-298
{
    ulong aGroup;

    if (GetLoadedFileData(theFileName, aGroup))
        return aGroup;

    return -1;
}

uchar* ResStreamsManager::GetLoadedFileData(const std::string& theFileName, ulong& theGroup) //301-318
{
    for (ulong i = 0; i < mNumGroups; i++)
    {
        if (mGroups[i].GetStatus() == RESIDENT)
        {
            uchar* theData = mGroups[i].GetFileIndexData(theFileName);
            if (theData)
            {
                theGroup = i;
                return theData;
            }
        }
    }
    return NULL;
}

bool ResStreamsManager::GetResidentFileBuffer(ulong theGroupId, const std::string& theFileName, uchar** theBuffer, ulong* theSize) //321-342
{
    uchar* theData;
    if (theGroupId == -1)
    {
        theData = GetLoadedFileData(theFileName, theGroupId);
    }
    else
    {
        DBG_ASSERT(IsGroupLoaded(theGroupId)); //329
        theData = mGroups[theGroupId].GetFileIndexData(theFileName);
    }
    if (theData == NULL)
        return 0;
    if (*theData)
        return 0;
    uchar* aBaseAddress = mGroups[theGroupId].GetResidentDataBaseAddress();
    *theBuffer = &aBaseAddress[*(theData + 1)];
    *theSize = *(theData + 2);
    return true;
}

bool ResStreamsManager::GetImage(ulong theGroupId, const std::string& theFileName, Image** theOutImage) //345-387
{
    uchar* theData = NULL;
    if (theGroupId == -1)
    {
        theData = GetLoadedFileData(theFileName, theGroupId);
    }
    else
    {
        DBG_ASSERTE(IsGroupLoaded(theGroupId)); //353
        theData = mGroups[theGroupId].GetFileIndexData(theFileName);
    }
    if (!theData)
        return 0;

    ResStreamFileGPULocationInfo* aLocationInfo = (ResStreamFileGPULocationInfo*)theData; //?
    if (*theData != 1 )
        return false;
    DeviceImage* anImage = mApp->mResStreamsDriver->GetImageFromResStream(theFileName, mGroups[theGroupId].GetTextureReference(aLocationInfo->texture_index), aLocationInfo);
    *theOutImage = anImage;
    return anImage != 0;
}


void ResStreamsManager::Update() //Correct? | 391-602
{
    if (!mApp->IsMainThread())
        SexySleep(1);

    if (!mCurLoadDesc.group && !mLoadingQueue.empty())
    {
        mCurLoadDesc = *mLoadingQueue.begin();
        mLoadingQueue.pop_front();
        mCurLoadDesc.pool->Allocate();
        mCurLoadDesc.group->StartLoad();
        mCurLoadDesc.group->mLoadStartTime = SexyTime();
        mCurLoadDesc.group->mDiskIOStartTime = SexyTime();
        mFile->DirectSeek(mCurLoadDesc.group->mFileLocation);
        if (mCurLoadDesc.group->mDesc->data_common_size + mCurLoadDesc.group->mDesc->data_resident_file_size)
        {
            mFile->DirectRead((uchar*)mCurLoadDesc.pool->GetResidentDataMemory(mCurLoadDesc.instance), mCurLoadDesc.group->mDesc->data_common_size + mCurLoadDesc.group->mDesc->data_resident_file_size);
            mCurReadState = RESSTREAM_RESIDENT;
        }
        else
            StartGPUDataTransfer();
        mCurReadBytes = 0;
    }
    if (mCurLoadDesc.group)
    {
        if (mFile->DirectReadStatus() == IFile::READ_ERROR)
            FlagError();
        else
        {
            if (mCurReadState == RESSTREAM_GPU_TRANSFER && mWaitingOnRead)
            {
                mCurLoadDesc.group->mDiskIOTime += SexyTime() - mCurLoadDesc.group->mDiskIOStartTime;
                mCurReadBytes += mCurReadSize;
                mWaitingOnRead = false;
            }
            else
            {
                mCurReadBytes += mCurLoadDesc.group->mDesc->data_common_size + mCurLoadDesc.group->mDesc->data_resident_file_size;
                mCurReadState = RESSTREAM_GPU_TRANSFER;
                mCurLoadDesc.group->mDiskIOTime += SexyTime() - mCurLoadDesc.group->mDiskIOStartTime;
                uchar* theBuffer = (uchar*)mCurLoadDesc.pool->GetResidentDataMemory(mCurLoadDesc.instance);
                mCurLoadDesc.group->InitGroupStream((ResStreamGroupHeader*)theBuffer, mCurLoadDesc.instance);
                StartGPUDataTransfer();
            }
        }
        if (!mWaitingOnRead && !mWaitingOnDecode && mCurReadState == RESSTREAM_GPU_TRANSFER)
        {
            if (mCurReadBytes >= mCurLoadDesc.group->mDesc->data_gpu_file_size)
                mCurReadState = RESSTREAM_TRANSIENT;
            else
            {
                mCurLoadDesc.group->mDiskIOStartTime = SexyTime();
                mCurDecodeBuffer = mCurReadBuffer == mReadBuffer[RESSTREAM_RESIDENT] ? mReadBuffer[RESSTREAM_RESIDENT] : mReadBuffer[RESSTREAM_GPU_TRANSFER];
                ulong dataToDecode = mCurReadSize;
                ulong decodeScan = 0;
                mCurReadSize = mCurLoadDesc.group->mDesc->data_gpu_file_size - mCurReadBytes <= 1048576 ? mCurLoadDesc.group->mDesc->data_gpu_file_size - mCurReadBytes : 1048576;
                mFile->DirectRead(mCurReadBuffer, mCurReadSize);
                mWaitingOnRead = true;
                mWaitingOnDecode = false;
                while (decodeScan < dataToDecode)
                {
                    if (!mCopyingTexture)
                    {
                        mCurTextureIndex++;
                        if (mCurTextureIndex >= mCurLoadDesc.group->GetNumTextures())
                            DBG_ASSERTE(false); //531
                        void* theTextureRef = mCurLoadDesc.pool->GetTextureReference(mCurLoadDesc.instance, mCurTextureIndex);
                        ResStreamTextureDescriptor* theTextureDesc = mCurLoadDesc.pool->GetTextureDescriptor(mCurTextureIndex);
                        mSizeOfCurTexture = mApp->mResStreamsDriver->GetGPUDataSize(theTextureDesc);
                        Align(mSizeOfCurTexture, 4096);
                        mApp->mResStreamsDriver->BeginGPUDataCopy(theTextureDesc);
                        mCopyingTexture = true;
                        if (mCopyingTexture)
                        {
                            theTextureRef = mCurLoadDesc.pool->GetTextureReference(mCurLoadDesc.instance, mCurTextureIndex);
                            ulong amtToCopy = mSizeOfCurTexture >= dataToDecode - decodeScan ? dataToDecode - decodeScan : mSizeOfCurTexture;
                            mApp->mResStreamsDriver->CopyDataToTexture(theTextureRef, mCurDecodeBuffer[decodeScan], amtToCopy);
                            mSizeOfCurTexture -= amtToCopy;
                            decodeScan += amtToCopy;
                            if (!mSizeOfCurTexture)
                            {
                                mApp->mResStreamsDriver->EndGPUDataCopy();
                                mCopyingTexture = false;
                            }
                        }
                    }
                }
            }
        }
        if (mCurReadState > RESSTREAM_GPU_TRANSFER)
        {
            mCurLoadDesc.group->mStatus = RESIDENT;
            mCurLoadDesc.group->mTotalLoadTime = SexyTime() - mCurLoadDesc.group->mLoadStartTime;
            OutputDebugStrF("Load time for group %s is %dms\n", mCurLoadDesc.group->mTotalLoadTime);
            mCurLoadDesc.group = NULL;
            mCurLoadDesc.pool = NULL;
            mCurLoadDesc.instance = -1;
        }
    }
    mIsIdle = mLoadingQueue.empty() && !mCurLoadDesc.group;
}

void ResStreamsManager::StartGPUDataTransfer() //605-636
{
    if (mCurLoadDesc.group->mDesc->data_gpu_file_size)
    {
        mCurLoadDesc.group->mDiskIOStartTime = SexyTime();
        mCurReadBuffer = mReadBuffer[RESSTREAM_RESIDENT];
        mCurDecodeBuffer = mReadBuffer[RESSTREAM_GPU_TRANSFER];
        if (mCurLoadDesc.group->mDesc->data_gpu_file_size <= 1048576)
            mCurReadSize = mCurLoadDesc.group->mDesc->data_gpu_file_size;
        else
            mCurReadSize = 1048576;
        mFile->DirectRead(mCurReadBuffer, mCurReadSize);
        mWaitingOnRead = true;
        mWaitingOnDecode = false;
        mCurReadState = PREPARING;
        mCurTextureIndex = -1;
        mCurGPUDataPtr = 0;
        mCopyingTexture = false;
    }
    else
        mCurReadState = RESIDENT;
}

#ifdef _WIN32
void ResStreamsManager::DebugDraw(Graphics* g) //640-641
{
}

//Present in non-Windows (Like Bej3 Mac), check PVZ2C debug
#else
void ResStreamsManager::DebugDraw(Graphics* g, Rect aRegion)
{
    Font* gsSysFont;
    g->PushState();
    g->SetColor(0x8F000000);
    g->FillRect(g->mClipRect); //0x20
    g->Translate(aRegion.mX, aRegion.mY);
    if (!gsSysFont)
    {
        gsSysFont = new SysFont("Arial", 10, 0, 0);
    }
    g->DrawString(_S("ResStreamsManager Debug View"));

    //?
    g->PopState();
}

#endif

void ResStreamsManager::FlagError() //644-652
{
    DBG_ASSERTE("false"); //645
}


ResStreamsGroup::ResStreamsGroup() //656-667
{
#ifdef _WIN32
    mStatus = ResStreamsManager::NOT_RESIDENT;
    mPool = NULL;
    mTotalLoadTime = 0;
    mDecodeTime = 0;
    mDiskIOTime = 0;
    mLoadStartTime = 0;
    mDecodeStartTime = 0;
    mDiskIOTime = 0;
    mLoadedAtInstance = -1;
    mLoadedHeader = NULL;
#else
    //?
    ResStreamsGroup::Reset();
#endif
}

void ResStreamsGroup::InitDescriptor(ResStreamGroupDescriptor* theDesc) //More on mobile? | 670-676
{
    //this = theDesc; //?
    mFileLocation = theDesc->file_location;
    mFileSize = theDesc->file_size;
    mPoolIndex = theDesc->pool;
    mDesc = theDesc;
}

void ResStreamsGroup::InitGroupStream(ResStreamGroupHeader* theHeader, ulong theInstanceId) //679-686
{
    uchar* aBuffer = (uchar*)theHeader + theHeader->file_index_location;
    mLoadedHeader = theHeader;
    mFileIndex.Init(aBuffer, theHeader->file_index_size);
    mLoadedAtInstance = theInstanceId;
}

uchar* ResStreamsGroup::GetFileIndexData(const std::string& theFileName) //689-691
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
    FixFilename(theFileName.c_str);
#endif
    return mFileIndex.Find(theFileName.c_str());
}

void ResStreamsGroup::StartLoad() //694-701
{
    mTotalLoadTime = 0;
    mDecodeTime = 0;
    mDiskIOTime = 0;
    mLoadStartTime = 0;
    mDecodeStartTime = 0;
    mDiskIOTime = 0;
}

bool ResStreamsGroup::CanLoad() //704-707
{
    if (mStatus != ResStreamsManager::NOT_RESIDENT) return false;
    return mPool->IsInstanceAvailable();
}

uchar* ResStreamsGroup::GetResidentDataBaseAddress() //710-712
{
    return (uchar*)mPool->GetResidentDataMemory(mLoadedAtInstance + mDesc->data_resident_location);
}

uchar* ResStreamsGroup::GetGPUDataBaseAddress() //715-717
{
    return (uchar*)mPool->GetGPUDataMemory(mLoadedAtInstance);
}

void* ResStreamsGroup::GetTextureReference(ulong theTextureId) //720-722
{
    return mPool->GetTextureReference(mLoadedAtInstance, theTextureId); //return mInstances[theInstanceId].mTextures[theTextureId]; in new?
}

ulong ResStreamsGroup::GetNumTextures() //725-730
{
    return mPool->GetNumTextures();
}

ResStreamsPool::ResStreamsPool() //733-736
{
    mOccupant = NULL;
    mAllocated = false;
    //mobile added a var?
}

ResStreamsPool::~ResStreamsPool() //739-744
{
    if (mOccupant)
    {
        delete mOccupant;
    }
}

void ResStreamsPool::InitDescriptor(ResStreamPoolDescriptor* theDesc, uchar* theTexDescsPtr, ulong theTexDescSize) //747-767
{
    //theDesc = "";
    mResidentDataSize = theDesc->resident_data_size;
    mGPUDataSize = theDesc->gpu_data_size;
    mNumInstances = theDesc->instance_count;
    mFlags = theDesc->flags;
    mAllocated = false;
    mOccupant = (ResStreamsGroup**)new ulong[mNumInstances];
    mInstances = (PoolInstance*) new void*[mNumInstances];
    for (ulong i = 0; i < mNumInstances; ++i)
    {
        mOccupant[i] = NULL;
        mInstances[i].mResidentData = NULL;
        mInstances[i].mGPUData = NULL;
    }
    mTexDescSize = theTexDescSize;
    mTexDescsPtr = &theTexDescsPtr[theTexDescSize * theDesc->texture_offset];
    mNumTexDescs = theDesc->texture_count;
}

void ResStreamsPool::Allocate() //770-889
{
    if (!mAllocated)
#ifdef _WIN32
    {
        DBG_ASSERT("NOT IMPLEMENTED"==0); //884, not on Win
        mAllocated = 1;
    }
#else
        //?
#endif
}

void ResStreamsPool::Destroy() //892-893
{
#ifndef _WIN32
    //todo
#endif
}

const std::string& ResStreamsPool::GetName() //896-898
{
    return;
}

bool ResStreamsPool::IsInstanceAvailable() //901-910
{
    for (ulong i = 0; i < mNumInstances; ++i)
    {
        if (!mOccupant[i])
            return true;
    }
    return false;
}

ulong ResStreamsPool::LockInstanceForGroup(ResStreamsGroup* theGroup) //913-923
{
    for (ulong i = 0; i < mNumInstances; i++)
    {
        if (mOccupant[i])
        {
            mOccupant[i] = theGroup;
            return i;
        }
    }
    return -1;
}

void* ResStreamsPool::GetResidentDataMemory(ulong theInstanceId) //926-928
{
    return mInstances[theInstanceId].mResidentData;
}

void* ResStreamsPool::GetGPUDataMemory(ulong theInstanceId) //931-933
{
    return mInstances[theInstanceId].mGPUData;
}

void* ResStreamsPool::GetTextureReference(ulong theInstanceId, ulong theTextureId) //Not on mobile? | 936-938
{
    return mInstances[theInstanceId].mTextures[theTextureId];
}

ResStreamTextureDescriptor* ResStreamsPool::GetTextureDescriptor(ulong theTextureId) //Not on mobile? | 941-943
{
    return (ResStreamTextureDescriptor*)&mTexDescsPtr[mTexDescSize * theTextureId];
}

ulong ResStreamsPool::GetNumTextures() //Not on mobile | 946-948
{
    return mNumTexDescs;
}

void ResStreamsPool::UnlockInstanceForGroup(ResStreamsGroup* theGroup) //951-952
{
#ifndef _WIN32 //Not on Windows.
    //TODO
    //for (int i = 0; i < mNumInstances; i++)
#endif
}

//Todo mobile: GetBytesLoadedForGroup, GetAttribString, GetComposite, FindNextGroupToLoad, GetPakFileFromResidentBuffer, GetTotalBytesForGroup, IsCompositeChildActive, KickOffDecodeTask, OnAdvanceTiles, StartGPUDataLoad, OnResidentDataFinished, OnTaskFinished, StartGPUDataLoad, StartLoadNextGroup, StartResidentDataLoad, RSG::GetOccupantCount
//Copy these from ResMan2? "LoadCommonResourceFromManifest, LoadFontResourceFromManifest, LoadGenericResFileFromManifest, LoadImageResourceFromManifest, LoadPIEffectResourceFromManifest, LoadPopAnimResourceFromManifest, LoadRenderEffectResourceFromManifest, LoadResourceFromManifest, LoadResourcesManifest, LoadSoundResourceFromManifest"
#ifndef _WIN32 //Not on Windows
void ResStreamsGroup::Reset()
{
	mStatus = NOT_RESIDENT;
    mPool = 0;
    mTotalLoadTime = 0;
    mDecodeTime = 0;
    mDiskIOTime = 0;
    mLoadStartTime = 0;
    mDecodeStartTime = 0;
    mDiskIOTime = 0;
    mLoadedAtInstance = -1;
    mLoadedHeader = 0;
}

int ResStreamsPool::GetOccupantCount() 
{
    //todo, not on Win
}


ulong ResStreamsManager::GetAttribString(const ulong *)
{
    //? Not on Win
    //return 5 + 0x26 + *;
}

void ResStreamsManager::ExecuteTasks() //Not on PVZ iPhone CH
{
    
}

//DBG_ASSERT(mGroups[theGroupId].mPool!=0); | 394 BejLiveWin8

void ResStreamsManager::LoadCommonResourceFromManifest(ManifestLoadContext*, ResStream_BaseResDesc, BaseRes*, ResMap)
{
    DBG_ASSERT("Resource already defined"); //1483 BejLiveWin8
}

#endif