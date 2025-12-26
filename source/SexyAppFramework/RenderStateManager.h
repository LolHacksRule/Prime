#ifndef __RENDERSTATEMANAGER_H__
#define __RENDERSTATEMANAGER_H__

#include "Debug.h"

namespace Sexy
{
	class RenderStateManager //Not in H5
	{
	public:
		class StateValue
		{
		public:
			enum EStateValueType
			{
				SV_Dword,
				SV_Float,
				SV_Ptr,
				SV_Vector,
			};
			EStateValueType mType;
			union
			{
			public:
				DWORD mDword;
				float mFloat;
				void* mPtr;
				float mX;
			};
			float mY;
			float mZ;
			float mW;
			StateValue(); //Idk
			StateValue(DWORD inDword) { mType = SV_Dword; mDword = inDword; } //70
			StateValue(float inFloat) { mType = SV_Float; mFloat = inFloat; } //71
			StateValue(void* inPtr) { mType = SV_Ptr; mPtr = inPtr; } //72
			StateValue(float inX, float inY, float inZ, float inW) { mType = SV_Vector; mFloat = inX; mY = inY; mZ = inZ; mW = inW; } //73
			StateValue(const StateValue& inValue) //77-83 (UNMATCHED)
			{
				*this = inValue;
			}
			DWORD GetDword() const { assert(mType == SV_Dword); return mDword; } //85
			float GetFloat() const { assert(mType == SV_Float); return mFloat; } //86
			void* GetPtr() const { assert(mType == SV_Ptr); return mPtr; } //87
			void GetVector(float& outX, float& outY, float& outZ, float& outW) const { assert(mType == SV_Vector); outX = mFloat; outY = mY; outZ = mZ; outW = mW; } //88
			bool operator==(const StateValue& inValue) const //91-103
			{
				assert(mType == inValue.mType); //92

				switch (mType)
				{
				case SV_Dword: return mDword == inValue.mDword;
				case SV_Float: return mFloat == inValue.mFloat;
				case SV_Ptr: return mPtr == inValue.mPtr;
				case SV_Vector: return mX == inValue.mX && mY == inValue.mY && mZ == inValue.mZ && mW == inValue.mW;
				default: assert(false && "Invalid StateValue type"); return false; //100
				}
			}
		};
		class State
		{
		public:
			typedef bool (*FCommitFunc)(State*); //?
			RenderStateManager* mManager;
			ulong mContext[4];
			State* mDirtyPrev;
			State* mDirtyNext;
			StateValue mValue;
			StateValue mHardwareDefaultValue;
			StateValue mContextDefaultValue;
			StateValue mLastCommittedValue;
			State* mContextDefPrev;
			State* mContextDefNext;
			FCommitFunc mCommitFunc;
			std::string mName;
			const char* mValueEnumName;
			State(); //Idk
			State(RenderStateManager* inManager, ulong inContext0, ulong inContext1, ulong inContext2, ulong inContext3) //144-152 (UNMATCHED)
			{
				mManager = inManager;
				mCommitFunc = NULL;
				mValueEnumName = "";
				mDirtyPrev = this;
				mDirtyNext = this;
				mContextDefPrev = this;
				mContextDefNext = this;
				mContext[StateValue::SV_Dword] = inContext0;
				mContext[StateValue::SV_Float] = inContext1;
				mContext[StateValue::SV_Ptr] = inContext2;
				mContext[StateValue::SV_Vector] = inContext3;
			}
			State(const State& inState) //163-169
			{
				mManager = inState.mManager;
				mCommitFunc = inState.mCommitFunc;
				mDirtyPrev = (mDirtyNext = this);
				mContextDefPrev = (mContextDefNext = this);
				for (int i = 0; i < 4; i++)
					mContext[i] = inState.mContext[i];
			}
			~State();
			void Init(const StateValue& inDefaultValue, const std::string& inName, const char* inValueEnumName = "");
			void Init(const StateValue& inHardwareDefaultValue, const StateValue& inContextDefaultValue, const std::string& inName, const char* inValueEnumName);
			void Reset();
			void SetDirty();
			void ClearDirty(bool inActAsCommit = false);
			void SetValue(const StateValue& inValue);
			bool HasContextDefault() const;
			bool IsDirty() const { return mDirtyPrev != this; } //187
			void SetValue(float inX, float inY, float inZ, float inW);
			void SetValue(DWORD inDword) { SetValue(StateValue(inDword)); } //193
			void SetValue(float inFloat) { SetValue(StateValue(inFloat)); } //194
			void SetValue(void* inPtr) { SetValue(StateValue(inPtr)); } //195
			DWORD GetDword() const { return mValue.GetDword(); } //199
			float GetFloat() const { return mValue.GetFloat(); } //200
			void* GetPtr() const { return mValue.GetPtr(); } //201
			void GetVector(float& outX, float& outY, float& outZ, float& outW) const { return mValue.GetVector(outX, outY, outZ, outW); } //202
		};
		class Context
		{
		public:
			class JournalEntry
			{
			public:
				State* mState;
				StateValue mOldValue;
				StateValue mNewValue;
				JournalEntry() { mState = NULL; } //233
				JournalEntry(State* inState, const StateValue& inOldValue, const StateValue& inNewValue) { mState = inState; } //238
			};
			std::vector<JournalEntry> mJournal;
			ulong mJournalFloor;
			std::vector<ulong> mFloorStack;
			Context* mParentContext;
			std::vector<Context*> mChildContexts;
			Context();
			Context(const Context& inContext);
			~Context();
			void RevertState();
			void PushState();
			void PopState();
			void Unacquire(bool inIgnoreParent = false);
			void Reacquire(bool inIgnoreParent = false);
			void SplitChildren();
		};
		protected:
			State mDirtyDummyHead;
			State mContextDefDummyHead;
			Context* mCurrentContext;
			Context mDefaultContext;
			virtual State::FCommitFunc GetCommitFunc(State* inState) = 0; //?
		public:
			RenderStateManager() { mCurrentContext = &mDefaultContext; } //281
			virtual ~RenderStateManager()  //285-286
			{
			}
			void ApplyContextDefaults();
			bool CommitState();
			virtual void Flush();
			void SetContext(Context* inContext);
			virtual void Init() = 0;
			virtual void Reset() = 0;
			virtual void Cleanup() //295-298
			{
				assert(mCurrentContext != NULL); //296
				mCurrentContext->Unacquire();
			}
			bool IsDirty() const;
			Context* GetContext() const { return mCurrentContext; } //307

			//
			void RevertState() { mCurrentContext->RevertState(); } //310
			void PushState() { mCurrentContext->PushState(); }; //311
			void PopState() { mCurrentContext->PopState(); }; //312
	};
}
#endif