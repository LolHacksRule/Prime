#ifndef __PIEFFECT_H__
#define __PIEFFECT_H__

#include "Bezier.h"
#include "DeviceImage.h"
#include "MTRand.h"
//#include "Buffer.h"
//#include "Color.h"
//#include "Point.h"
//#include "Graphics.h"
//#include "SexyMatrix.h"
#include "ObjectPool.h"

//struct PFILE;

//Recovered global vars from XNA/H5
static int PI_QUANT_SIZE = 256;
static int PI_BUFSIZE = 1024; //XNA
static float M_PI = 3.14159; //XNA | idk why this is here maybe this was pre sexymath
static int PPF_MIN_VERSION = 0; //H5
static int PPF_VERSION = 1; //H5
static int PPF_STATE_VERSION = 1; //H5

namespace Sexy
{
    class PIEffect
    {
    public:
        PFILE* mReadFP; //C++ only
        FILE* mWriteFP; //C++ only
        int mFileChecksum;
        bool mIsPPF;
        bool mAutoPadImages;
        int mVersion;
        std::string mSrcFileName;
        std::string mDestFileName;
        MTRand mRand;
        Buffer mStartupState;
        uchar mBuf[1024];
        int mBufTemp;
        int mBufPos; //Not used in H5
        std::string mNotes;
        int mFileIdx;
        std::vector<std::string> mStringVector;
        int mWidth;
        int mHeight;
        Color mBkgColor;
        int mFramerate;
        int mFirstFrameNum;
        int mLastFrameNum;
        DeviceImage* mThumbnail;
        DefinesMap mNotesParams;
        PIEffectDef* mDef;
        PILayerVector mLayerVector;
        std::string mError;
        bool mLoaded;
        int mUpdateCnt;
        float mFrameNum;
        bool mIsNewFrame;
        ObjectPool<PIParticleInstance> mParticlePool;
        ObjectPool<PIFreeEmitterInstance> mFreeEmitterPool;
        bool mHasEmitterTransform;
        bool mHasDrawTransform;
        bool mDrawTransformSimple;
        int mCurNumParticles;
        int mCurNumEmitters;
        int mLastDrawnPixelCount;
        float mAnimSpeed;
        Color mColor;
        bool mDebug;
        bool mDrawBlockers;
        bool mEmitAfterTimeline;
        IntVector mRandSeeds;
        bool mWantsSRand;
        SexyTransform2D mDrawTransform;
        SexyTransform2D mEmitterTransform;
        bool Fail(const std::string& theError);
        void Deref();
        float GetRandFloat(); //mRand
        float GetRandFloatU();
        float GetRandSign();
        float GetVariationScalar();
        float GetVariationScalarU();
        std::string ReadString();
        std::string ReadStringS();
        bool ExpectCmd(const std::string& theCmdExpected);
        void ReadValue2D(PIValue2D* theValue2D);
        void ReadEPoint(PIValue2D* theValue2D);
        void ReadValue(PIValue* theValue);
        void ReadEmitterType(PIEmitter* theEmitter);
        //C++ exclusive
        void WriteByte(char theByte);
        void WriteInt(int theInt);
        void WriteShort(short theShort);
        void WriteFloat(float theFloat);
        void WriteBool(bool theBool);
        void WriteString(const std::string& theString);
        void WriteValue2D(PIValue2D* theValue2D);
        void WriteEPoint(PIValue2D* theValue2D);
        void WriteValue(PIValue* theValue);
        void WriteEmitterType(PIEmitter* theEmitter);
        //Not in H5
        void SaveParticleDefInstance(Buffer& theBuffer, PIParticleDefInstance* theParticleDefInstance);
        void SaveParticle(Buffer& theBuffer, PILayer* theLayer, PIParticleInstance* theParticle);
        void LoadParticleDefInstance(const Buffer& theBuffer, PIParticleDefInstance* theParticleDefInstance);
        //Back to normal
        void LoadParticle(const Buffer& theBuffer, PILayer* theLayer, PIParticleInstance* theParticle);
        FPoint GetGeomPos(PIEmitterInstance* theEmitterInstance, PIParticleInstance* theParticleInstance, float* theTravelAngle, bool* isMaskedOut);
        bool LoadEffect(const std::string& theFileName);
        bool SaveAsPPF(const std::string& theFileName, bool saveInitialState); //C++ only
        bool LoadState(Buffer& theBuffer, bool shortened = false);
        bool SaveState(Buffer& theBuffer, bool shortened = false);
        void ResetAnim();
        void Clear();
        PILayer* GetLayer(int theIdx);
        PILayer* GetLayer(const std::string& theName);
        FPoint GetEmitterPos(PIEmitterInstance* theEmitterInstance, bool doTransform);
        int CountParticles(PIParticleInstance* theStart);
        void CalcParticleTransform(PILayer* theLayer, PIEmitterInstance* theEmitterInstance, PIEmitter* theEmitter, PIParticleDef* theParticleDef, PIParticleGroup* theParticleGroup, PIParticleInstance* theParticleInstance);
        void UpdateParticleDef(PILayer* theLayer, PIEmitter* theEmitter, PIEmitterInstance* theEmitterInstance, PIParticleDef* theParticleDef, PIParticleDefInstance* theParticleDefInstance, PIParticleGroup* theParticleGroup, PIFreeEmitterInstance* theFreeEmitter);
        void UpdateParticleGroup(PILayer* theLayer, PIEmitterInstance* theEmitterInstance, PIParticleGroup* theParticleGroup);
        void DrawParticleGroup(Graphics* g, PILayer* theLayer, PIEmitterInstance* theEmitterInstance, PIParticleGroup* theParticleGroup, bool isDarkeningPass);
        PIEffect(const PIEffect& rhs);
        PIEffect();
        ~PIEffect();
        PIEffect* Duplicate();
        virtual SharedImageRef GetImage(const std::string& theName, const std::string& theFilename);
        virtual void SetImageOpts(DeviceImage* theImage);
        virtual std::string WriteImage(const std::string& theName, int theIdx, DeviceImage* theImage, bool* hasPadding);
        bool IsActive();
        void Update();
        void DrawLayer(Graphics* g, PILayer* theLayer);
        void Draw(Graphics* g);
        bool HasTimelineExpired();
        std::string GetNotesParam(const std::string& theName, const std::string& theDefault);
        bool CheckCache();
        bool SetCacheUpToDate();
        void WriteToCache();
    };
    class PIValuePoint2D
    {
    public:
        float mTime;
        FPoint mValue;
        PIValuePoint2D();
    };
    class PIParticleDef
    {
    public:
        enum
        {
            VALUE_LIFE,
            VALUE_NUMBER,
            VALUE_SIZE_X,
            VALUE_VELOCITY,
            VALUE_WEIGHT,
            VALUE_SPIN,
            VALUE_MOTION_RAND,
            VALUE_BOUNCE,
            VALUE_LIFE_VARIATION,
            VALUE_NUMBER_VARIATION,
            VALUE_SIZE_X_VARIATION,
            VALUE_VELOCITY_VARIATION,
            VALUE_WEIGHT_VARIATION,
            VALUE_SPIN_VARIATION,
            VALUE_MOTION_RAND_VARIATION,
            VALUE_BOUNCE_VARIATION,
            VALUE_SIZE_X_OVER_LIFE,
            VALUE_VELOCITY_OVER_LIFE,
            VALUE_WEIGHT_OVER_LIFE,
            VALUE_SPIN_OVER_LIFE,
            VALUE_MOTION_RAND_OVER_LIFE,
            VALUE_BOUNCE_OVER_LIFE,
            VALUE_VISIBILITY,
            VALUE_EMISSION_ANGLE,
            VALUE_EMISSION_RANGE,
            VALUE_SIZE_Y,
            VALUE_SIZE_Y_VARIATION,
            VALUE_SIZE_Y_OVER_LIFE,
            NUM_VALUES
        };
    public:
        PIEmitter* mParent;
        std::string mName;
        int mTextureIdx;
        PIValue mValues[NUM_VALUES];
        Point mRefPointOfs;
        bool mLockAspect;
        bool mIntense;
        bool mSingleParticle;
        bool mPreserveColor;
        bool mAttachToEmitter;
        int mAnimSpeed;
        bool mAnimStartOnRandomFrame;
        float mAttachVal;
        bool mFlipHorz;
        bool mFlipVert;
        int mRepeatColor;
        int mRepeatAlpha;
        bool mRandomGradientColor;
        bool mUseNextColorKey;
        bool mGetColorFromLayer;
        bool mUpdateColorFromLayer;
        bool mGetTransparencyFromLayer;
        bool mUpdateTransparencyFromLayer;
        int mNumberOfEachColor;
        bool mLinkTransparencyToColor;
        bool mUseKeyColorsOnly;
        bool mUseEmitterAngleAndRange;
        bool mAngleAlignToMotion;
        bool mAngleKeepAlignedToMotion;
        bool mAngleRandomAlign;
        int mAngleAlignOffset;
        int mAngleValue;
        int mAngleRange;
        int mAngleOffset;
        PIInterpolator mColor;
        PIInterpolator mAlpha;
        float GetValueAt(int theTime, float theDefault);
        PIParticleDef();
        ~PIParticleDef();
    };
    class PIValue
    {
    public:
        std::vector<float> mQuantTable;
        PIValuePointVector mValuePointVector;
        Bezier mBezier;
        float mLastTime;
        float mLastValue;
        float mLastCurveT;
        float mLastCurveTDelta;
        PIValue() //51-57
        {
            mLastTime = -1.0;
            mLastCurveT = 0.0;
            mLastCurveTDelta = 0.01;
        }
        void QuantizeCurve();
        float GetValueAt(float theTime, float theDefault = 0.0);
        float GetLastKeyframe(float theTime);
        float GetLastKeyframeTime(float theTime);
        float GetNextKeyframeTime(float theTime);
        int GetNextKeyframeIdx(float theTime);
        ~PIValue();
    };
    class PIInterpolatorPoint
    {
    public:
        int mValue;
        float mTime;
    };
    class PIValue2D
    {
    public:
        PIValuePoint2DVector mValuePoint2DVector;
        Bezier mBezier;
        float mLastTime;
        FPoint mLastPoint;
        float mLastVelocityTime;
        FPoint mLastVelocity;
        PIValue2D() //88-91
        {
            mLastTime = -1.0;
        }
        FPoint GetValueAt(float theTime);
        FPoint GetVelocityAt(float theTime);
        ~PIValue2D();
    };
    class PIDeflector
    {
    public:
        std::string mName;
        float mBounce;
        float mHits;
        float mThickness;
        bool mVisible;
        PIValue2D mPos;
        PIValue mActive;
        PIValue mAngle;
        PIValue2DVector mPoints;
        std::vector<FPoint> mCurPoints;
        PIDeflector();
        ~PIDeflector();
    };
    class PIBlocker
    {
    public:
        std::string mName;
        bool mUseLayersBelowForBg;
        PIValue2D mPos;
        PIValue mActive;
        PIValue mAngle;
        PIValue2DVector mPoints;
        PIBlocker();
        ~PIBlocker();
    };
    class PIForce
    {
    public:
        std::string mName;
        bool mVisible;
        PIValue2D mPos;
        PIValue mStrength;
        PIValue mDirection;
        PIValue mActive;
        PIValue mAngle;
        PIValue mWidth;
        PIValue mHeight;
        FPoint mCurPoints[5];
        PIForce();
        ~PIForce();
    };
    class PILayerDef
    {
    public:
        std::string mName;
        PIEmitterInstanceDefVector mEmitterInstanceDefVector;
        PIDeflectorVector mDeflectorVector;
        PIBlockerVector mBlockerVector;
        PIForceVector mForceVector;
        PIValue2D mOffset;
        FPoint mOrigOffset;
        PIValue mAngle;
        PILayerDef();
        ~PILayerDef();
    };
    class PIEmitterInstanceDef
    {
    public:
        enum
        {
            VALUE_LIFE,
            VALUE_NUMBER,
            VALUE_SIZE_X,
            VALUE_VELOCITY,
            VALUE_WEIGHT,
            VALUE_SPIN,
            VALUE_MOTION_RAND,
            VALUE_BOUNCE,
            VALUE_ZOOM,
            VALUE_VISIBILITY,
            VALUE_TINT_STRENGTH,
            VALUE_EMISSION_ANGLE,
            VALUE_EMISSION_RANGE,
            VALUE_ACTIVE,
            VALUE_ANGLE,
            VALUE_XRADIUS,
            VALUE_YRADIUS,
            VALUE_SIZE_Y,
            VALUE_UNKNOWN4,
            NUM_VALUES
        };
        enum
        {
            GEOM_POINT = 0,
            GEOM_LINE = 1,
            GEOM_ECLIPSE = 2,
            GEOM_AREA = 3,
            GEOM_CIRCLE = 4
        };
    public:
        std::string mName;
        int mFramesToPreload;
        int mEmitterDefIdx;
        int mEmitterGeom;
        bool mEmitIn;
        bool mEmitOut;
        int mEmitAtPointsNum;
        int mEmitAtPointsNum2;
        bool mIsSuperEmitter;
        IntVector mFreeEmitterIndices;
        bool mInvertMask;
        PIValue2D mPosition;
        PIValue mValues[NUM_VALUES];
        PIValue2DVector mPoints;
        PIEmitterInstanceDef();
        ~PIEmitterInstanceDef();
    };
    class PIValuePoint
    {
    public:
        float mTime;
        float mValue;
    };
    class PIEmitterBase
    {
    public:
        PIPaticleDefInstanceVector mParticleDefInstanceVector;
        PIParticleGroup mParticleGroup;
        PIEmitterBase();
        ~PIEmitterBase();
    };
    class PIInterpolator
    {
    public:
        PIInterpolatorPointVector mInterpolatorPointVector;
        int GetValueAt(float theTime);
        int GetKeyframeNum(int theIdx);
        float GetKeyframeTime(int theIdx);
        PIInterpolator();
        ~PIInterpolator();
    };
    class PIEmitter
    {
    public:
        enum //PIEmitterValue
        {
            VALUE_F_LIFE,
            VALUE_F_NUMBER,
            VALUE_F_VELOCITY,
            VALUE_F_WEIGHT,
            VALUE_F_SPIN,
            VALUE_F_MOTION_RAND,
            VALUE_F_BOUNCE,
            VALUE_F_ZOOM,
            VALUE_LIFE,
            VALUE_NUMBER,
            VALUE_SIZE_X,
            VALUE_SIZE_Y,
            VALUE_VELOCITY,
            VALUE_WEIGHT,
            VALUE_SPIN,
            VALUE_MOTION_RAND,
            VALUE_BOUNCE,
            VALUE_ZOOM,
            VALUE_VISIBILITY,
            VALUE_UNKNOWN3,
            VALUE_TINT_STRENGTH,
            VALUE_EMISSION_ANGLE,
            VALUE_EMISSION_RANGE,
            VALUE_F_LIFE_VARIATION,
            VALUE_F_NUMBER_VARIATION,
            VALUE_F_SIZE_X_VARIATION,
            VALUE_F_SIZE_Y_VARIATION,
            VALUE_F_VELOCITY_VARIATION,
            VALUE_F_WEIGHT_VARIATION,
            VALUE_F_SPIN_VARIATION,
            VALUE_F_MOTION_RAND_VARIATION,
            VALUE_F_BOUNCE_VARIATION,
            VALUE_F_ZOOM_VARIATION,
            VALUE_F_NUMBER_OVER_LIFE,
            VALUE_F_SIZE_X_OVER_LIFE,
            VALUE_F_SIZE_Y_OVER_LIFE,
            VALUE_F_VELOCITY_OVER_LIFE,
            VALUE_F_WEIGHT_OVER_LIFE,
            VALUE_F_SPIN_OVER_LIFE,
            VALUE_F_MOTION_RAND_OVER_LIFE,
            VALUE_F_BOUNCE_OVER_LIFE,
            VALUE_F_ZOOM_OVER_LIFE,
            NUM_VALUES
        };
    public:
        std::string mName;
        PIValue mValues[NUM_VALUES];
        PIPaticleDefVector mParticleDefVector;
        bool mKeepInOrder;
        bool mOldestInFront;
        bool mIsSuperEmitter;
        PIEmitter();
        ~PIEmitter();
    };
    class PIParticleInstance
    {
    public:
        enum
        {
            VARIATION_LIFE,
            VARIATION_SIZE_X,
            VARIATION_SIZE_Y,
            VARIATION_VELOCITY,
            VARIATION_WEIGHT,
            VARIATION_SPIN,
            VARIATION_MOTION_RAND,
            VARIATION_BOUNCE,
            VARIATION_ZOOM,
            NUM_VARIATIONS
        };
        PIParticleInstance* mPrev;
        PIParticleInstance* mNext;
        PIParticleDef* mParticleDef;
        PIEmitter* mEmitterSrc;
        int mNum;
        PIFreeEmitterInstance* mParentFreeEmitter;
        FPoint mPos;
        FPoint mOrigPos;
        FPoint mEmittedPos;
        FPoint mLastEmitterPos;
        FPoint mVel;
        float mImgAngle;
        float mVariationValues[NUM_VARIATIONS];
        float mZoom;
        float mSrcSizeXMult;
        float mSrcSizeYMult;
        float mGradientRand;
        float mOrigEmitterAng;
        int mAnimFrameRand;
        Transform mTransform;
        float mTransformScaleFactor;
        int mImgIdx;
        float mThicknessHitVariation;
        float mTicks;
        float mLife;
        float mLifePct;
        bool mHasDrawn;
        ulong mBkgColor;
        static int mCount;
        PIParticleInstance() //345-359
        {
            mPrev = 0;
            mNext = 0;
            mTransformScaleFactor = 1.0;
            mImgIdx = 0;
            mBkgColor = -1;
            mSrcSizeXMult = 1.0;
            mSrcSizeYMult = 1.0;
            mParentFreeEmitter = 0;
            mHasDrawn = 0;
            mCount++;
        }

        ~PIParticleInstance() //362-364
        {
            --mCount;
        }
    };
    class PITexture
    {
    public:
        std::string mName;
        SharedImageRefVector mImageVector;
        SharedImageRef mImageStrip;
        int mNumCels;
        bool mPadded;
        PITexture();
        ~PITexture();
    };
    class PIParticleDefInstance
    {
    public:
        float mNumberAcc;
        float mCurNumberVariation;
        int mParticlesEmitted;
        int mTicks;
        PIParticleDefInstance() //378-381
        {
            Reset();
        }

        void Reset() //384-389
        {
            mNumberAcc = 0.0;
            mCurNumberVariation = 0.0;
            mParticlesEmitted = 0;
            mTicks = 0;
        }
    };
    class PIParticleGroup
    {
    public:
        PIParticleInstance* mHead;
        PIParticleInstance* mTail;
        int mCount;
        bool mIsSuperEmitter;
        bool mWasEmitted;
        PIParticleGroup() //405-412
        {
            mIsSuperEmitter = false;
            mWasEmitted = false;
            mHead = NULL;
            mTail = NULL;
            mCount = 0;
        }
    };
    class PIFreeEmitterInstance : public PIParticleInstance
    {
    public:
        PIEmitterBase mEmitter;
        PIFreeEmitterInstance() //430-433
        {
            mEmitter.mParticleGroup.mWasEmitted = true;
        }
        ~PIFreeEmitterInstance();
    };
    class PIEmitterInstance : public PIEmitterBase
    {
    public:
        PIEmitterInstanceDef* mEmitterInstanceDef;
        bool mWasActive;
        bool mWithinLifeFrame;
        PIPaticleDefInstanceVector mSuperEmitterParticleDefInstanceVector;
        PIParticleGroup mSuperEmitterGroup;
        PIPaticleDefInstanceVector mParticleDefInstanceVector;
        PIParticleGroup mParticleGroup;
        Color mTintColor;
        SharedImageRef mMaskImage;
        SexyTransform2D mTransform;
        FPoint mOffset;
        float mNumberScale;
        bool mVisible;
        PIEmitterInstance() //519-529
        {
            mWasActive = false;
            mWithinLifeFrame = true;
            mSuperEmitterGroup.mIsSuperEmitter = true;
            mTransform.LoadIdentity();
            mNumberScale = 1.0;
            mVisible = true;
        }
        void SetVisible(bool isVisible);
        ~PIEmitterInstance();
    };
    class PILayer
    {
    public:
        PILayerDef* mLayerDef;
        PIEmitterInstanceVector mEmitterInstanceVector;
        bool mVisible;
        Color mColor;
        DeviceImage* mBkgImage;
        Point mBkgImgDrawOfs;
        SexyMatrix3 mBkgTransform;
        PILayer() //621-627
        {
            mVisible = true;
            mColor = Color::White;
            mBkgImage = NULL;
            mBkgTransform.LoadIdentity();
        }
        void SetVisible(bool isVisible);
        PIEmitterInstance* GetEmitter(const std::string& theName);
        PIEmitterInstance* GetEmitter(int theIdx);
        ~PILayer();
    };
    class PIEffectDef
    {
    public:
        int mRefCount;
        PIEmitterPtrVector mEmitterVector;
        PITextureVector mTextureVector;
        PILayerDefVector mLayerDefVector;
        IntToIntMap mEmitterRefMap;
        PIEffectDef() //650-653
        {
            mRefCount = 1;
        }

        ~PIEffectDef() //656-661
        {
            for (int i = 0; i < mEmitterVector.size(); i++)
                delete mEmitterVector[i];
            for (int i = 0; i < mTextureVector.size(); i++)
                delete mTextureVector[i];
            mEmitterRefMap.clear();
            mEmitterVector.clear();
            mLayerDefVector.clear();
            mTextureVector.clear();
        }
    };
    typedef std::vector<PIBlocker> PIBlockerVector;
    typedef std::vector<PIDeflector> PIDeflectorVector;
    typedef std::vector<PIEmitterInstanceDef> PIEmitterInstanceDefVector;
    typedef std::vector<PIEmitterInstance> PIEmitterInstanceVector;
    typedef std::vector<PIEmitter*> PIEmitterPtrVector;
    typedef std::vector<PIForce> PIForceVector;
    typedef std::vector<PIInterpolatorPoint> PIInterpolatorPointVector;
    typedef std::vector<PILayerDef> PILayerDefVector;
    typedef std::vector<PILayer> PILayerVector;
    typedef std::vector<PIParticleDefInstance> PIPaticleDefInstanceVector; //Lol
    typedef std::vector<PIParticleDef> PIPaticleDefVector; //Lol
    typedef std::vector<PITexture*> PITextureVector;
    typedef std::vector<PIValue2D> PIValue2DVector;
    typedef std::vector<PIValuePoint2D> PIValuePoint2DVector;
    typedef std::vector<PIValuePoint> PIValuePointVector;
    typedef std::map<PIEmitter*, int> PIEmitterToIdMap;
}
#endif