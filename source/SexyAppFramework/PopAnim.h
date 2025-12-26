#ifndef __POPANIM_H__
#define __POPANIM_H__

#include "PopAnimListener.h"

//Recovered vars from XNA
const int PAM_MAGIC = 0xBAF01954;
const int PAM_VERSION = 5; //PVZ2 is 6 iirc
const int PAM_STATE_VERSION = 1;

struct BlendSrcData //Not in H5
{
public:
    PAParticleEffectVector mParticleEffectVector;
    PATransform mTransform;
    Color mColor;
    //Prob no functions?
};

typedef std::multimap<std::string, BlendSrcData> StringToBlendSrcMap; //Recovered from PDB module
typedef std::list<std::pair<std::string, std::string>> StringPairList; //? | Recovered from PDB module

namespace Sexy //According to H5 PA is PopAnim, not that surprising
{
    class PACommand
    {
    public:
        std::string mCommand;
        std::string mParam;
        PACommand();
        ~PACommand();
    };
    class PAImage
    {
    public:
        SharedImageRefVector mImages;
        int mOrigWidth;
        int mOrigHeight;
        int mCols;
        int mRows;
        std::string mImageName;
        int mDrawMode;
        PATransform mTransform;
        PAImage();
        ~PAImage();
    };
    class PAObjectPos
    {
    public:
        const char* mName;
        int mObjectNum;
        bool mIsSprite;
        bool mIsAdditive;
        int mResNum;
        int mPreloadFrames;
        int mAnimFrameNum;
        float mTimeScale;
        PATransform mTransform;
        bool mHasSrcRect;
        Rect mSrcRect;
        Color mColor;
        PAObjectPos();
    };
    class PAFrame
    {
    public:
        PAFrame() //91-94
        {

            mHasStop = false;
        }
        ~PAFrame();
        bool mHasStop;
        PACommandVector mCommandVector;
        PAObjectPosVector mFrameObjectPosVector;
    };
    
    class PAObjectDef
    {
    public:
        const char* mName;
        PASpriteDef* mSpriteDef;
        PAObjectDef() //111-115
        {

            mName = NULL;
            mSpriteDef = NULL;
        }
    };
    class PAParticleEffect
    {
    public:
        ResourceRef mResourceRef;
        PIEffect* mEffect;
        std::string mName;
        int mLastUpdated;
        bool mBehind;
        bool mAttachEmitter;
        bool mTransform;
        double mXOfs;
        double mYOfs;
        PAParticleEffect();
        ~PAParticleEffect();
    };
    class PASpriteDef
    {
    public:
        const char* mName;
        PAFrameVector mFrames;
        int mWorkAreaStart;
        int mWorkAreaDuration;
        StringIntMap mLabels;
        PAObjectDefVector mObjectDefVector;
        float mAnimRate;
        int GetLabelFrame(const std::string& theLabel);
        void GetLabelFrameRange(const std::string& theLabel, int& theStart, int& theEnd);
        PASpriteDef();
        ~PASpriteDef();
    };
    class PASpriteInst
    {
    public:
        PASpriteInst* mParent;
        int mDelayFrames;
        float mFrameNum;
        int mFrameRepeats;
        bool mOnNewFrame;
        int mLastUpdated;
        PATransform mCurTransform;
        Color mCurColor;
        PAObjectInstVector mChildren;
        PASpriteDef* mDef;
        PAParticleEffectVector mParticleEffectVector;
        PASpriteInst();
        virtual ~PASpriteInst();
        PAObjectInst* GetObjectInst(const std::string& theName);
    };
    class PATransform
    {
    public:
        SexyMatrix3 mMatrix;
        PATransform();
        PATransform TransformSrc(const PATransform& theSrcTransform);
        PATransform InterpolateTo(const PATransform& theNextTransform, float thePct);
    };
    class PAObjectInst
    {
    public:
        const char* mName;
        PASpriteInst* mSpriteInst;
        PATransform mBlendSrcTransform;
        Color mBlendSrcColor;
        bool mIsBlending;
        SexyTransform2D mTransform;
        Color mColorMult;
        bool mPredrawCallback;
        bool mImagePredrawCallback;
        bool mPostdrawCallback;
        PAObjectInst() //173-181
        {

            mName = "";
            mSpriteInst = NULL;
            mPredrawCallback = true;
            mPostdrawCallback = true;
            mImagePredrawCallback = true;
            mIsBlending = false;
        }
    };
    class PopAnimDef
    {
    public:
        PASpriteDef* mMainSpriteDef;
        PASpriteDefVector mSpriteDefVector;
        std::list<std::string> mObjectNamePool;
        int mRefCount;
        PopAnimDef() { mRefCount = 0; mMainSpriteDef = NULL; } //245
        ~PopAnimDef() { if (mMainSpriteDef != NULL) delete mMainSpriteDef; } //246
    };
    class PopAnim : public Widget
    {
    public:
        enum //Not actually in Bej3 JP but recovering from XNA
        {
            FRAMEFLAGS_HAS_REMOVES = 1,
            FRAMEFLAGS_HAS_ADDS,
            FRAMEFLAGS_HAS_MOVES = 4,
            FRAMEFLAGS_HAS_FRAME_NAME = 8,
            FRAMEFLAGS_HAS_STOP = 16,
            FRAMEFLAGS_HAS_COMMANDS = 32
        };
        enum
        {
            MOVEFLAGS_HAS_SRCRECT = 32768,
            MOVEFLAGS_HAS_ROTATE = 16384,
            MOVEFLAGS_HAS_COLOR = 8192,
            MOVEFLAGS_HAS_MATRIX = 4096,
            MOVEFLAGS_HAS_LONGCOORDS = 2048,
            MOVEFLAGS_HAS_ANIMFRAMENUM = 1024,
        };
        int mId;
        PopAnimListener* mListener;
        StringVector mImageSearchPathVector;
        int mVersion;
        Buffer mCRCBuffer;
        float mDrawScale;
        float mImgScale;
        bool mLoaded;
        int mMotionFilePos;
        std::string mModPamFile;
        std::string mLoadedPamFile;
        StringPairList mRemapList;
        int mAnimRate;
        Rect mAnimRect;
        std::string mError;
        std::string mLastPlayedFrameLabel;
        PAImageVector mImageVector;
        PASpriteInst* mMainSpriteInst;
        PopAnimDef* mMainAnimDef;
        float mBlendTicksTotal;
        float mBlendTicksCur;
        float mBlendDelay;
        MTRand mRand;
        bool mRandUsed;
        TPoint<double> mParticleAttachOffset;
        SexyTransform2D mTransform;
        Color mColor;
        bool mAdditive;
        bool mTransDirty;
        bool mAnimRunning;
        bool mMirror;
        bool mInterpolate;
        bool mLoadedImageIsNew;
        bool mPaused;
        bool mColorizeType;
        PopAnim();
        virtual ~PopAnim();
        PopAnim(const PopAnim& rhs);
        PopAnim(int theId, PopAnimListener* theListener);
        bool Fail(const std::string& theError);
        const std::string& Remap(const std::string& theString);
        bool LoadSpriteDef(Buffer* theBuffer, PASpriteDef* theSpriteDef);
        void InitSpriteInst(PASpriteInst* theSpriteInst, PASpriteDef* theSpriteDef);
        void GetToFirstFrame();
        void FrameHit(PASpriteInst* theSpriteInst, PAFrame* theFrame, PAObjectPos* theObjectPos);
        void DoFramesHit(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos);
        void CalcObjectPos(PASpriteInst* theSpriteInst, int theObjectPosIdx, bool frozen, PATransform* theTransform, Color* theColor);
        void UpdateTransforms(PASpriteInst* theSpriteInst, PATransform* theTransform, const Color& theColor, bool parentFrozen);
        void UpdateParticles(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos);
        void CleanParticles(PASpriteInst* theSpriteInst, bool force = false);
        bool HasParticles(PASpriteInst* theSpriteInst);
        void IncSpriteInstFrame(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos, float theFrac);
        void PrepSpriteInstFrame(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos);
        void AnimUpdate(float theFrac);
        void ResetAnimHelper(PASpriteInst* theSpriteInst);
        void SaveStateSpriteInst(Buffer& theBuffer, PASpriteInst* theSpriteInst);
        void LoadStateSpriteInst(Buffer& theBuffer, PASpriteInst* theSpriteInst);
        void DrawParticleEffects(Graphics* g, PASpriteInst* theSpriteInst, PATransform* theTransform, Color& theColor, bool front);
        virtual void DrawSprite(Graphics* g, PASpriteInst* theSpriteInst, PATransform* theTransform, Color& theColor, bool additive, bool parentFrozen);
        virtual void DrawSpriteMirrored(Graphics* g, PASpriteInst* theSpriteInst, PATransform* theTransform, Color& theColor, bool additive, bool parentFrozen);
        virtual void Load_Init();
        virtual void Load_SetModPamFile(const std::string& theFileName);
        virtual void Load_AddRemap(const std::string& theWildcard, const std::string& theReplacement);
        virtual bool Load_LoadPam(const std::string& theFileName);
        virtual bool Load_LoadMod(const std::string& theFileName);
        virtual SharedImageRef Load_GetImageHook(const std::string& theFileDir, const std::string& theOrigName, const std::string& theRemappedName);
        virtual bool Load_GetImage(PAImage* theImage, const std::string& theFileDir, const std::string& theOrigName, const std::string& theRemappedName);
        virtual void Load_PostImageLoadHook(PAImage* theImage);
        void Clear();
        PopAnim* Duplicate();
        bool LoadFile(const std::string& theFileName, bool doMirror = false);
        bool LoadState(Buffer& theBuffer);
        bool SaveState(Buffer& theBuffer);
        void ResetAnim();
        bool SetupSpriteInst(const std::string& theName = "");
        bool Play(int theFrameNum, bool resetAnim);
        bool Play(const std::string& theFrameLabel, bool resetAnim);
        bool BlendTo(const std::string& theFrameLabel, int theBlendTicks, int theAnimStartDelay);
        bool IsActive();
        void SetColor(const Color& theColor);
        PAObjectInst* GetObjectInst(const std::string& theName);
        virtual void Draw(Graphics* g);
        virtual void Update();
        virtual void UpdateF(float theFrac);
        int GetLabelFrame(const std::string& theFrameLabel);
        PASpriteDef* FindSpriteDef(const char* theAnimName);
    };
    class PopAnimModParser : public DescParser
    {
    public:
        int mPassNum;
        PopAnim* mPopAnim;
        std::string mErrorHeader;
        PopAnimModParser();
        virtual ~PopAnimModParser();
        virtual bool Error(const std::string& theError);
        void SetParamHelper(PASpriteDef* theSpriteDef, const std::string& theCmdName, int* theCmdNum, const std::string& theNewParam);
        virtual bool    HandleCommand(const ListDataElement& theParams);
    };

    typedef std::vector<PAFrame> PAFrameVector;
    typedef std::vector<PAImage> PAImageVector;
    typedef std::vector<PAObjectInst> PAObjectInstVector;
    typedef std::vector<PASpriteDef> PASpriteDefVector;
    typedef std::vector<PAParticleEffect> PAParticleEffectVector;
    typedef std::vector<PAObjectDef> PAObjectDefVector;
    typedef std::map<int, PAObjectPos> IntToPAObjectPosMap; //Recovered from PDB module
    typedef std::vector<PAObjectPos> PAObjectPosVector; //Recovered from PDB module
    typedef std::vector<PACommand> PACommandVector; //Recovered from PDB module
};

#endif