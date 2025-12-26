#ifndef __INTERNETMANAGER_H__
#define __INTERNETMANAGER_H__

#include "HTTPTransfer.h"
#include "Image.h"

namespace Sexy //This is useless but since it's just for universal adverts it's fine
{
    class MoreGamesAd
    {
    public:
        Image*                      mImage;
        Image*                      mOverImage;
        std::string                 mClickURL;
        MoreGamesAd() : mImage(NULL), mOverImage(NULL) {} //43
    };
	typedef std::vector<MoreGamesAd> MoreGamesAdVector;
    class MoreGamesAdContainer
    {
    public:
		MoreGamesAdVector           mAds;
    };

    enum AdType
    {
        AdType_None, //No ad
        AdType_Popup, //Popup ad
        AdType_MoreGames //More Games ad
    };

    class AdInstance
    {
    public:
        AdType                      mType;
        int                         mWidth;
        int                         mHeight;
        StringList                  mResourceList; //?
        bool                        mHasAllResources;
        bool                        mIsNew;
        bool                        mRemove;
        bool                        mJustDownloaded;
        std::string                 mClickURL;
        std::string                 mInstanceName;
        AdInstance()
        {
            mType = AdType_None;
            mRemove, mJustDownloaded = false; //Prob not original but looks more cleaner
        }
        //~AdInstance();
    };

    typedef std::list<AdInstance> AdInstanceList;
	typedef std::list<std::string> StringList;
    enum
    {
        VCRESULT_NONE,
        VCRESULT_CHECKING,
        VCRESULT_DONE,
        VCRESULT_FAILED
    };

    class InternetManager
    {
    public:
        int                         mNumMoreGamesAds;
        HTTPTransfer                mVersionCheckTransfer;
        HTTPTransfer                mFileTransfers[4];
        bool                        mParsed;
        bool                        mIsUpToDate;
        bool                        mDoingTransfers;
        std::string                 mUpdateURL;
        AdInstanceList              mAdInstanceList;
        AdInstanceList              mOldAdInstanceList;
        StringList                  mPendingResourceList;
    protected:
        bool                        ConfirmParsed();
        virtual void                ParseAdLine(const std::string& theString, bool justDownloaded = true); //Assuming true
        AdInstance*                 FindAdInstanceInList(AdInstanceList& theList, const AdInstance& theInstance);
        void                        GenAdLine(AdInstance& theInstance, std::string& theLine);
        bool                        LoadAdList();
        bool                        SaveAdList();
        void                        CheckAdResources();
        void                        CheckAdResources(AdInstance& theInstance);
        void                        EnsureSufficientMoreGamesAds();
        void                        TryLoadMoreGamesAd(AdInstance* theInstance, MoreGamesAd* theAds, int& theIndex);
        void					    GetThreadProc();
        static void				    GetThreadProcStub(void *theArg);
    public:
        InternetManager();
        ~InternetManager();
        void					    Init();
        void                        StartVersionCheck(const std::string& theUrl);
        int                         GetVersionCheckResult();
        const bool                  IsUpToDate();
        const std::string           GetUpdateURL();
        void                        StartTransfers();
        void                        StopTransfers();
        bool                        HasNewAds(AdType theType, bool includeJustDownloaded);
        void                        TryShowAd();
        bool                        GetMoreGamesAds(MoreGamesAdContainer* theContainer, bool includeJustDownloaded);
        void                        Update();
    };
}

#endif