#include "RenderStateManager.h"

using namespace Sexy;

void RenderStateManager::State::Init(const StateValue& inDefaultValue, const std::string& inName, const char* inValueEnumName) //50-58
{
	assert(!IsDirty()); //51 | 67 in BejLiveWin8
	mLastCommittedValue = inDefaultValue;
	mContextDefaultValue = mLastCommittedValue; //May be multiple
	mHardwareDefaultValue = mContextDefaultValue;
	mValue = mHardwareDefaultValue;
	mCommitFunc = mManager->GetCommitFunc(this);
	mName = inName;
	mValueEnumName = inValueEnumName;
}
void RenderStateManager::State::Init(const StateValue& inHardwareDefaultValue, const StateValue& inContextDefaultValue, const std::string& inName, const char* inValueEnumName) //60-74
{
	assert(IsDirty()); //61
	mLastCommittedValue = inHardwareDefaultValue;
	mHardwareDefaultValue = mLastCommittedValue;
	mValue = mHardwareDefaultValue;
	mContextDefaultValue = inContextDefaultValue;
	mCommitFunc = mManager->GetCommitFunc(this);
	mName = inName;
	mValueEnumName = inValueEnumName;
	mContextDefPrev = &mManager->mContextDefDummyHead;
	mContextDefNext = mManager->mContextDefDummyHead.mContextDefNext;
	mContextDefNext->mContextDefPrev = this;
	mContextDefPrev->mContextDefNext = this;
}

void RenderStateManager::State::Reset() //77-79
{
	mLastCommittedValue = mHardwareDefaultValue;
}

void RenderStateManager::State::SetDirty() //82-90
{
	if (IsDirty())
		return;

	mDirtyPrev = &mManager->mDirtyDummyHead;
	mDirtyNext = mManager->mDirtyDummyHead.mDirtyNext;
	mDirtyNext->mDirtyPrev = this;
	mDirtyPrev->mDirtyNext = this;
}
void RenderStateManager::State::ClearDirty(bool inActAsCommit) //92-105
{
	if (!IsDirty())
		return;

	if (inActAsCommit)
		mLastCommittedValue = mValue;
	mDirtyPrev->mDirtyNext = mDirtyNext;
	mDirtyNext->mDirtyPrev = mDirtyPrev;
	mDirtyNext = this;
	mDirtyPrev = this;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	//mManager.mWouldCommitStateDirty = true; //XNA
#endif
}

void RenderStateManager::State::SetValue(const StateValue& inValue) //108-125
{
	if (inValue == mValue)
		return;

	mManager->Flush();
	if (mManager->mCurrentContext != NULL)
	{
		mManager->mCurrentContext->SplitChildren();
		mManager->mCurrentContext->mJournal.push_back(Context::JournalEntry(this, mValue, inValue));
	}
	mValue = inValue;
	SetDirty();
}

RenderStateManager::Context::Context() : //133-134
	mJournalFloor(0),
	mParentContext(NULL)
{
}
RenderStateManager::Context::Context(const Context& inContext): //138-140
	mJournalFloor(0),
	mParentContext((Context*)&inContext) //?
{
	mParentContext->mChildContexts.push_back(this);
}
RenderStateManager::Context::~Context() //142-157
{
	SplitChildren();
	if (mParentContext == NULL)
		return;

	int aChildCount = mParentContext->mChildContexts.size();
	for (int iChild = 0; iChild < aChildCount; iChild++)
	{
		if (mParentContext->mChildContexts[iChild] == this)
		{
			mParentContext->mChildContexts.erase(mParentContext->mChildContexts.begin() + iChild);
			break;
		}
	}
}

void RenderStateManager::Context::RevertState() //Correct? Not in C# | 160-187
{
	SplitChildren();

	bool flushed = false;

	assert((ulong)mJournal.size() >= mJournalFloor); //165 | 189 in BejLiveWin8
	ulong popCount = mJournal.size() - mJournalFloor;
	while (popCount--)
	{
		const JournalEntry& entry = mJournal.back();
		assert(entry.mState->mValue == entry.mNewValue); //172 | 196 in BejLiveWin8
		if (!flushed)
		{
			entry.mState->mManager->Flush();
			flushed = true;
		}
		entry.mState->mValue.mType = entry.mOldValue.mType;
		entry.mState->mValue.mDword = entry.mOldValue.mDword;
		entry.mState->mValue.mY = entry.mOldValue.mY;
		entry.mState->mValue.mZ = entry.mOldValue.mZ;
		entry.mState->mValue.mW = entry.mOldValue.mW;
		entry.mState->SetDirty();
		mJournal.pop_back();
	}
}

void RenderStateManager::Context::PushState() //Not in C# | 190-199
{
	SplitChildren();

	mFloorStack.push_back(mJournalFloor);
	mJournalFloor = mJournal.size();
}
void RenderStateManager::Context::PopState() //Not in C# | 201-212
{
	SplitChildren();

	DBG_ASSERT(!mFloorStack.empty()); //207 | 231 in BejLiveWin8
	RevertState();
	mJournalFloor = mFloorStack.back();
	mFloorStack.pop_back();
}

void RenderStateManager::Context::Unacquire(bool inIgnoreParent) //Empty in C# | 215-240
{
	bool flushed = false;
	int count = mJournal.size(); //Prob correct
	for (int i = 0; i < count; i++)
	{
		const JournalEntry& entry = mJournal[i];
		assert(entry.mState->mValue == entry.mNewValue); //224 | 248 in BejLiveWin8
		if (!flushed)
		{
			entry.mState->mManager->Flush();
			flushed = true;
		}
		entry.mState->mValue.mType = entry.mOldValue.mType;
		entry.mState->mValue.mDword = entry.mOldValue.mDword;
		entry.mState->mValue.mY = entry.mOldValue.mY;
		entry.mState->mValue.mZ = entry.mOldValue.mZ;
		entry.mState->mValue.mW = entry.mOldValue.mW;
		entry.mState->SetDirty();
		if (mParentContext && !inIgnoreParent)
			mParentContext->Reacquire();
	}
}
void RenderStateManager::Context::Reacquire(bool inIgnoreParent) //Not in C# | 242-267
{
	if (mParentContext && !inIgnoreParent)
		mParentContext->Reacquire();
	bool flushed = false;
	int count = mJournal.size();
	for (int i = 0; i < count; i++)
	{
		const JournalEntry& entry = mJournal[i];
		assert(entry.mState->mValue == entry.mOldValue); //254 | 278 in BejLiveWin8
		if (!flushed)
		{
			entry.mState->mManager->Flush();
			flushed = true;
		}
		entry.mState->mValue.mType = entry.mOldValue.mType;
		entry.mState->mValue.mDword = entry.mOldValue.mDword;
		entry.mState->mValue.mY = entry.mOldValue.mY;
		entry.mState->mValue.mZ = entry.mOldValue.mZ;
		entry.mState->mValue.mW = entry.mOldValue.mW;
		entry.mState->SetDirty();
	}
}

void RenderStateManager::Context::SplitChildren() //Not in C# | 269-307
{
	if (mChildContexts.empty())
		return;

	ulong aJournalSize = mJournal.size();
	int aChildCount = mChildContexts.size();
	for (int iChild = 0; iChild < aChildCount; iChild++)
	{
		Context* aChild = mChildContexts[iChild];
		if (aJournalSize > 0)
		{
			ulong aTempJournalSize = aChild->mJournal.size();
			std::vector<JournalEntry> aTempJournal;
			if (aTempJournalSize > 0)
			{
				aTempJournal.resize(aTempJournalSize);
				memcpy(&aTempJournal[0], &aChild->mJournal[0], 44 * aTempJournalSize);
			}
			aChild->mJournal.resize(aTempJournalSize + aJournalSize);
			memcpy(&aChild->mJournal[0], &mJournal[0], 44 * aJournalSize);
			if (aTempJournalSize > 0)
				memcpy(&aChild->mJournal[aJournalSize], &aTempJournal[0], 44 * aTempJournalSize);
			aChild->mJournalFloor += aJournalSize;
			for (ulong iStack = 0; iStack < aChild->mFloorStack.size(); iStack++)
				aChild->mFloorStack[iStack] += aJournalSize;
		}
		aChild->mParentContext = mParentContext;
		if (mParentContext)
			mParentContext->mChildContexts.push_back(aChild);
	}
	mChildContexts.clear();
}

void RenderStateManager::ApplyContextDefaults() //313-319
{
	for (State* s = mContextDefDummyHead.mContextDefNext; s != &mContextDefDummyHead; s = s->mContextDefNext)
	{
		s->mValue = s->mContextDefaultValue; //May be multiple, C# is latter
		s->SetDirty();
	}
}

bool RenderStateManager::CommitState() //322-351
{
	bool result = true;
	while (mDirtyDummyHead.mDirtyNext != &mDirtyDummyHead)
	{
		State* s = mDirtyDummyHead.mDirtyNext;
		if (s->mValue == s->mLastCommittedValue)
			s->ClearDirty();
		if (s->mCommitFunc != NULL)
		{
			result &= s->mCommitFunc(s);
			assert(!s->IsDirty()); //338 | 390 in BejLiveWin8
		}
		else
			s->ClearDirty();
		s->mLastCommittedValue = s->mValue;
	}
	return false;
}

void RenderStateManager::Flush() //354-361 (There's probably a long message here)
{
}

void RenderStateManager::SetContext(Context* inContext) //364-397
{
	if (inContext == NULL)
		inContext = &mDefaultContext;
	if (inContext != mCurrentContext)
	{
		assert(inContext != NULL); //375 | 427 in BejLiveWin8
		assert(mCurrentContext != NULL); //376 | 428 in BejLiveWin8
		if (mCurrentContext->mParentContext == inContext)
		{
			mCurrentContext->Unacquire(true);
			mCurrentContext = inContext;
		}
		else if (inContext->mParentContext == mCurrentContext)
		{
			mCurrentContext = inContext;
			mCurrentContext->Reacquire(true);
		}
		else
		{
			mCurrentContext->Unacquire();
			mCurrentContext = inContext;
			mCurrentContext->Reacquire();
		}
	}
}
