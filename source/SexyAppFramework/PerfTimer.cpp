#include "PerfTimer.h"
#include <map>
#include "Debug.h"
#include "SexyAppBase.h"

using namespace Sexy;

static __int64 gCPUSpeed = 0;
static bool sFrequencyValid;
static LARGE_INTEGER sFrequency;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
inline int QueryCounters(__int64 *lpPerformanceCount) //19-29 (Matched)
{
	/* returns TSC only */
	_asm
	{
		mov ebx, dword ptr [lpPerformanceCount]
		rdtsc
			mov dword ptr [ebx], eax
			mov dword ptr [ebx+4], edx
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
inline int DeltaCounters(__int64 *lpPerformanceCount) //34-45 (Matched)
{
	_asm
	{
		mov ebx, dword ptr [lpPerformanceCount]
		rdtsc
			sub eax, dword ptr [ebx]
			sbb edx, dword ptr [ebx+4]
			mov dword ptr [ebx],   eax
				mov dword ptr [ebx+4], edx
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static __int64 CalcCPUSpeed() //50-70 (Matched)
{
	int aPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
	LARGE_INTEGER	goal, current, period;
	__int64 Ticks;

	if( !QueryPerformanceFrequency( &period ) ) return 0;

	QueryPerformanceCounter(&goal);
	goal.QuadPart+=period.QuadPart/100;
	QueryCounters( &Ticks );
	do
	{
		QueryPerformanceCounter(&current);
	} while(current.QuadPart<goal.QuadPart);
	DeltaCounters( &Ticks );

	SetThreadPriority(GetCurrentThread(),aPriority);
	return( Ticks * 100 );		// Hz

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
union _LARGE_INTEGER PerfTimer::GetPerformanceFrequency() //75-86
{
	if (sFrequencyValid)
	{
		if (!QueryPerformanceFrequency(&sFrequency))
			sFrequency.QuadPart = 1;
	}
	sFrequencyValid = true;
	return sFrequency;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PerfTimer::PerfTimer() //90-95 (Matched)
{
	mDuration = 0;
	mStart.QuadPart = 0;
	mRunning = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PerfTimer::CalcDuration() //100-105 (Matched)
{
	LARGE_INTEGER anEnd, aFreq;
	QueryPerformanceCounter(&anEnd);
	QueryPerformanceFrequency(&aFreq);
	mDuration = ((anEnd.QuadPart-mStart.QuadPart)*1000)/(double)aFreq.QuadPart;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PerfTimer::Start() //110-113 (Matched)
{
	mRunning = true;
	QueryPerformanceCounter(&mStart);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PerfTimer::Stop() //118-124 (Matched)
{
	if(mRunning)
	{
		CalcDuration();
		mRunning = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
double PerfTimer::GetDuration() //129-134 (Matched)
{
	if(mRunning)
		CalcDuration();

	return mDuration;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
__int64 PerfTimer::GetCPUSpeed() //139-148 (Matched)
{
	if(gCPUSpeed<=0)
	{
		gCPUSpeed = CalcCPUSpeed();
		if (gCPUSpeed<=0)
			gCPUSpeed = 1;
	}

	return gCPUSpeed;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int PerfTimer::GetCPUSpeedMHz() //153-155 (Matched)
{
	return (int)(gCPUSpeed/1000000);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct PerfInfo
{
	const char *mPerfName;
	mutable __int64 mStartTime;
	mutable __int64 mDuration;
	mutable double mMillisecondDuration;
	mutable double mLongestCall;
	mutable __int64 mFrameDuration;
	mutable int mStartCount;
	mutable int mCallCount;

	PerfInfo(const char *theName) : mPerfName(theName), mStartTime(0), mDuration(0), mStartCount(0), mCallCount(0), mLongestCall(0), mFrameDuration(0) { } //235 (Matched)

	bool operator<(const PerfInfo &theInfo) const { return stricmp(mPerfName,theInfo.mPerfName)<0; } //237 (Matched)
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef std::set<PerfInfo> PerfInfoSet; //243
static PerfInfoSet gPerfInfoSet;
static bool gPerfOn = false;
static __int64 gStartTime;
static __int64 gCollateTime;
double gDuration = 0;
int gStartCount = 0;
int gPerfRecordTop = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct PerfRecord
{
	const char *mName;
	__int64 mTime;
	bool mStart;

	PerfRecord() { }
	PerfRecord(const char *theName, bool start) : mName(theName), mStart(start) { QueryCounters(&mTime); } //260 (Matched)
};
typedef std::vector<PerfRecord> PerfRecordVector;
PerfRecordVector gPerfRecordVector; //263

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class PerfJournal
{
	struct DiskFrame
	{
		double mAppSeconds;
		float mDurationMS;
		ulong mNodeFirst;
		ushort mNodeCount;
		ushort mReserved;
	};
	struct DiskNode
	{
		float mDurationMS;
		ulong mScopeIndex;
		ushort mParentNodeRel;
		ulong mCallCount;
	};
	struct MemItem
	{
		const char* mName;
		int64 mTime;
		ulong mStart;
		MemItem(const char* inName, int64 inTime, bool inStart) : mName(inName), mTime(inTime), mStart(inStart) {} //294
	};
	struct DiskStackItem
	{
		int mNodeIndex;
		double mTime;
		DiskStackItem(int inNodeIndex, double inTime) : mNodeIndex(inNodeIndex), mTime(inTime) {} //313
	};
	struct Scope
	{
		const char* mName;
		double mTotalMS;
		float mLongestCallMS;
		ulong mTotalCallCount;
	};
protected:
	std::vector<MemItem> mMemItems;
	std::vector<DiskNode> mDiskNodes;
	std::vector<DiskFrame> mDiskFrames;
	std::vector<DiskStackItem> mDiskNodeStack;
	std::vector<Scope> mScopes;
	typedef std::map<const char*, int> DScopeNameToIndexMap;
	DScopeNameToIndexMap mScopeNameToIndexMap;
	typedef std::pair<int, int> DNodeScopePair;
	typedef std::map<DNodeScopePair, int> DNodeScopeToChildNodeMap;
	DNodeScopeToChildNodeMap mNodeScopeToChildNodeMap;
public:
	int64 mBaseTime;
	int64 mBaseTime;
	int mStackDepth;
	bool mChangeFrame;

protected:
	int PerfJournal::GetScopeIndex(const char* inName) //TODO | 331-352
	{
		DScopeNameToIndexMap::iterator it = mScopeNameToIndexMap.find(inName);
		if (it != mScopeNameToIndexMap.end())
		{
			DBG_ASSERTE(mScopes[it->second].mName == inName); //335 | 338 BejLiveWin8
			return it->second;
		}
		else
		{
			Scope* scope;
			ZeroMemory(scope, sizeof scope); //?
			mScopes.push_back(*scope);
			scope = &mScopes.back();
			scope->mName = inName;
			scope->mTotalMS = 0.0;
			scope->mLongestCallMS = 0.0;
			scope->mTotalCallCount = 0;
			std::pair<const char*, int> p;
			p.first = inName;
			p.second = mScopes.size() - 1;
			mScopeNameToIndexMap.insert(p);
			return p.second;
		}
	}
	void PerfJournal::Flush() //Correct? | 354-455
	{
		int64 aFreq = PerfTimer::GetCPUSpeed();
		int memItemCount = mMemItems.size();
		int dnIndex;
		for (int iMemItem = 0; iMemItem < memItemCount; ++iMemItem)
		{
			MemItem* mi = &mMemItems[iMemItem];
			double dblTime = mi->mTime * 1000.0 / aFreq;
			if (mi->mName)
			{
				if (mi->mStart)
				{
					int scopeIndex = GetScopeIndex(mi->mName);
					Scope* scope = &mScopes[scopeIndex];
					DiskNode* dn = NULL;
					DBG_ASSERTE(!mDiskNodeStack.empty()); //385
					DiskStackItem* dsi = &mDiskNodeStack.back();
					DNodeScopeToChildNodeMap::iterator it = mNodeScopeToChildNodeMap.find(DNodeScopePair(dsi->mNodeIndex, scopeIndex)); //?
					if (it != mNodeScopeToChildNodeMap.end())
					{
						dnIndex = it->second;
						dn = &mDiskNodes[dnIndex];
					}
					else
					{
						memset(&dn, 0, sizeof(dn));
						mDiskNodes.push_back(*dn); //Memset 0
						dn = &mDiskNodes.back();
						dnIndex = mDiskNodes.size() - 1;
						DiskFrame* df = &mDiskFrames.back();
						if (df->mNodeCount == 0)
							df->mNodeFirst = dnIndex;
						df->mNodeCount++;
						dn->mDurationMS = 0.0;
						dn->mScopeIndex = scopeIndex;
						dn->mParentNodeRel = -1;
						dn->mCallCount = 0;
						if (dsi->mNodeIndex >= 0)
						{
							DBG_ASSERTE((dsi->mNodeIndex - df->mNodeFirst) < 0xffff); //412
							dn->mParentNodeRel = dsi->mNodeIndex >> 8 - df->mNodeFirst >> 8;
						}
						std::pair<DNodeScopePair, int> p;
						p.first.first = dsi->mNodeIndex; //?
						p.first.second = scopeIndex;
						p.second = dnIndex;
						mNodeScopeToChildNodeMap.insert(p);
					}
					scope->mTotalCallCount++;
					dn->mCallCount++;
					mDiskNodeStack.push_back(DiskStackItem(dnIndex, dblTime));
				}
				else
				{
					int scopeIndex = GetScopeIndex(mi->mName);
					Scope* scope = &mScopes[scopeIndex];
					DiskNode* dn = &mDiskNodes[mDiskNodeStack.back().mNodeIndex];
					DiskStackItem* dsi = &mDiskNodeStack.back();
					DBG_ASSERTE(dn->mScopeIndex == scopeIndex); //434
					float deltaTime = dblTime - dsi->mTime; //dsi?
					dn->mDurationMS += + deltaTime;
					mScopes[scopeIndex].mTotalMS += deltaTime;
					scope->mTotalMS += deltaTime;
					if (deltaTime > mScopes[scopeIndex].mLongestCallMS)
						scope->mLongestCallMS = deltaTime;
					mDiskNodeStack.pop_back();
					DBG_ASSERTE(!mDiskNodeStack.empty()); //444
					if (mDiskNodeStack.size() == 1)
					{
						DiskFrame* df = &mDiskFrames.back();
						df->mDurationMS += deltaTime;
					}
				}
			}
			else
			{
				//memset line?
				DiskFrame* df = &mDiskFrames.back();
				df->mAppSeconds = dblTime / 1000.0;
				df->mDurationMS = 0.0;
				df->mNodeCount = 0;
				df->mNodeFirst = 0;
				df->mReserved = 0;
				mNodeScopeToChildNodeMap.clear();
			}
		}
		mMemItems.clear();
	}
public:
	PerfJournal::PerfJournal() //462-472
	{
		mMemItems.reserve(65536);

		QueryCounters(&mBaseTime);
		mDiskNodeStack.push_back(DiskStackItem(-1, 0.0));

		mStackDepth = 0;

		mChangeFrame = false;
		AddItem(NULL);
	}
	void PerfJournal::AddItem(const PerfRecord* inRecord) //475-507
	{
		if (!gSexyAppBase->IsMainThread())
			return;

		if (inRecord)
		{
			mMemItems.push_back(MemItem(inRecord->mName, inRecord->mTime - mBaseTime, inRecord->mStart));
			if (inRecord->mStart)
				mStackDepth++;
			else if (!mStackDepth-- && mChangeFrame)
			{
				mChangeFrame = 0;
				Flush();
				AddItem(NULL);
			}
			else
			{
				__int64 curTime;
				QueryCounters(&curTime);
				mMemItems.push_back(MemItem("", curTime - mBaseTime, 0));
			}
		}
	}
	void PerfJournal::NextFrame() //509-517
	{
		mChangeFrame = true;
	}
	void PerfJournal::Save(const char* theFileName) //519-601
	{
		Flush();
		Buffer scopeBuf;
		for (int iScope = 0; iScope < mScopes.size(); ++iScope)
		{
			Scope* scope = &mScopes[iScope];
			int nameLen = strlen(scope->mName);
			if (nameLen >= 128)
			{
				scopeBuf.WriteByte(nameLen & 0x7F | 80);
				scopeBuf.WriteByte(nameLen >> 7);
			}
			else
				scopeBuf.WriteByte(nameLen);
			for (int i = 0; i < nameLen; ++i)
				scopeBuf.WriteByte(scope->mName[i]);
			scopeBuf.WriteBytes((const uchar*)&scope->mTotalMS, 8);
			scopeBuf.WriteBytes((const uchar*)&scope->mLongestCallMS, 4);
			scopeBuf.WriteLong(scope->mTotalCallCount);
			struct Header
			{
				ulong mMagic;
				ulong mVersion;
				ulong mScopeCount;
				ulong mScopeOffset;
				ulong mFrameCount;
				ulong mFrameOffset;
				ulong mNodeCount;
				ulong mNodeOffset;
			};
			Header hdr;
			hdr.mMagic = 1146309971; //SMSD
			hdr.mVersion = 1;
			hdr.mScopeCount = mScopes.size();
			hdr.mScopeOffset = 32;
			hdr.mFrameCount = mDiskFrames.size();
			hdr.mFrameOffset = scopeBuf.GetDataLen() + 32;
			hdr.mNodeCount = mDiskNodes.size();
			hdr.mNodeOffset = hdr.mFrameOffset + 20 * hdr.mFrameCount;
			Buffer buf;
			buf.WriteBytes((const uchar*)&hdr, sizeof(Header));
			buf.WriteBytes(scopeBuf.GetDataPtr(), scopeBuf.GetDataLen());
			if (!mDiskFrames.empty())
				buf.WriteBytes((const uchar*)&mDiskFrames[0], 20 * mDiskFrames.size());
			if (!mDiskNodes.empty())
				buf.WriteBytes((const uchar*)&mDiskNodes[0], 12 * mDiskNodes.size());
			gSexyAppBase->WriteBufferToFile(theFileName, &buf);
		}
	}
	~PerfJournal();
};

static PerfJournal* gPerfJournal;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void InsertPerfRecord(PerfRecord &theRecord) //608-633 (Matched?)
{
	if(theRecord.mStart)
	{
		PerfInfoSet::iterator anItr = gPerfInfoSet.insert(PerfInfo(theRecord.mName)).first;
		anItr->mCallCount++;

		if ( ++anItr->mStartCount == 1)
			anItr->mStartTime = theRecord.mTime;
	}
	else
	{
		PerfInfoSet::iterator anItr = gPerfInfoSet.find(theRecord.mName);
		if(anItr != gPerfInfoSet.end())
		{
			if( --anItr->mStartCount == 0)
			{
				__int64 aDuration = theRecord.mTime - anItr->mStartTime;
				anItr->mDuration += aDuration;

				if (aDuration > anItr->mLongestCall)
					anItr->mLongestCall = (double)aDuration;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void CollatePerfRecords() //639-650 (Matched)
{
	__int64 aTime1,aTime2;
	QueryCounters(&aTime1);

	for(int i=0; i<gPerfRecordTop; i++)
		InsertPerfRecord(gPerfRecordVector[i]);

	gPerfRecordTop = 0;
	QueryCounters(&aTime2);

	gCollateTime += aTime2-aTime1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void PushPerfRecord(PerfRecord &theRecord) //655-665
{
	if(gPerfRecordTop >= (int)gPerfRecordVector.size())
		gPerfRecordVector.push_back(theRecord);
	else
		gPerfRecordVector[gPerfRecordTop] = theRecord;

	++gPerfRecordTop;

	if (gPerfJournal)
		gPerfJournal->AddItem(&theRecord);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool SexyPerf::IsPerfOn() //670-672
{
	return gPerfOn;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool SexyPerf::IsPerfRecording() //677-679
{
	return gPerfJournal != NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::BeginPerf(bool measurePerfOverhead, bool recordSession) //684-697
{
	gPerfInfoSet.clear();
	gPerfRecordTop = 0;
	gStartCount = 0;
	gCollateTime = 0;

	if(!measurePerfOverhead)
		gPerfOn = true;
	
	QueryCounters(&gStartTime);
	if (recordSession && !measurePerfOverhead)
		gPerfJournal = new PerfJournal();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::EndPerf() //702-745
{
	__int64 anEndTime;
	QueryCounters(&anEndTime);

	CollatePerfRecords();

	gPerfOn = false;

	__int64 aFreq = PerfTimer::GetCPUSpeed();

	gDuration = ((double)(anEndTime - gStartTime - gCollateTime))*1000/aFreq;

	for (PerfInfoSet::iterator anItr = gPerfInfoSet.begin(); anItr != gPerfInfoSet.end(); ++anItr)
	{
		const PerfInfo &anInfo = *anItr;
		anInfo.mMillisecondDuration = (double)anInfo.mDuration*1000/aFreq;
		anInfo.mLongestCall = anInfo.mLongestCall*1000/aFreq;
	}
	if (gPerfJournal)
	{
		std::string aDataPath = RemoveTrailingSlash(GetAppDataFolder());
		char* tempFileName = tempnam(aDataPath.c_str(), "smstemp");
		if (tempFileName)
		{
			gPerfJournal->Save(tempFileName);
			std::string cmdArgs = StrFormat("%s -t", tempFileName);
			char curDir[260];
			memset(curDir, 0, sizeof(curDir));
			GetCurrentDirectory(260, curDir);
			ShellExecuteA(NULL, "open", "SpikeMonkey.exe", cmdArgs.c_str(), curDir, true);
			free(tempFileName);
		}
	}
	delete gPerfJournal;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::StartTiming(const char *theName) //750-756
{
	if(gPerfOn)
	{
		++gStartCount;
		PushPerfRecord(PerfRecord(theName,true));
	}
}

	
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::StopTiming(const char *theName) //762-769
{
	if(gPerfOn)
	{
		PushPerfRecord(PerfRecord(theName,false));
		if(--gStartCount==0)
			CollatePerfRecords();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
std::string SexyPerf::GetResults() //774-798
{
	std::string aResult;
	char aBuf[512];

	sprintf(aBuf, "Total Time: %.2f\n", gDuration);
	aResult += aBuf;
	for (PerfInfoSet::iterator anItr = gPerfInfoSet.begin(); anItr != gPerfInfoSet.end(); ++anItr)
	{
		const PerfInfo& anInfo = *anItr;
		sprintf(aBuf, "%s (%d calls, %.2f%% time): %.*f (%.*f avg, %.*flongest)\n",anInfo.mPerfName,anInfo.mCallCount,anInfo.mMillisecondDuration/gDuration*100,(anInfo.mMillisecondDuration>=0.009999999776482582?3:6),anInfo.mMillisecondDuration,(anInfo.mMillisecondDuration/anInfo.mCallCount>=0.009999999776482582?3:6),anInfo.mMillisecondDuration/anInfo.mCallCount,(anInfo.mLongestCall>=0.009999999776482582?3:6),anInfo.mLongestCall);
		aResult += aBuf;
	}


	return aResult;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::ClearFrameInfo() //803-812
{
	for (PerfInfoSet::iterator anItr = gPerfInfoSet.begin(); anItr != gPerfInfoSet.end(); ++anItr)
	{
		const PerfInfo& anInfo = *anItr;
		anInfo.mFrameDuration = 0;
	}
	if (gPerfJournal)
		gPerfJournal->NextFrame();
}