#ifndef __OBJECTPOOL_H__
#define __OBJECTPOOL_H__

#include "Common.h"

namespace Sexy
{
    template <class _T> class ObjectPool
    {
    public:
        int mNumPools;
        int mNumAvailObjects;
        uchar** mDataPools;
        int** mFreeIndicesPools;
        int mChunkSize;

        ObjectPool() //23-30
        {
            mNumPools = 0;
            mNumAvailObjects = 0;
            mDataPools = 0;
            mFreeIndicesPools = 0;
            mChunkSize = sizeof ObjectPool;
        }

        ~ObjectPool() //33-44
        {
            for (int aPoolNum = 0; aPoolNum < mNumPools; aPoolNum++)
            {
                delete mDataPools[aPoolNum];
                delete mFreeIndicesPools[aPoolNum];
            }
            if (mDataPools)
                free(mDataPools);
            if (mFreeIndicesPools)
                free(mFreeIndicesPools);
        }

        GrowPool() //47-67
        {
            int aCurPoolNum = mNumPools;
            int aNumChunksPerPool = 32768 / mChunkSize;

            mNumPools++;
            mNumAvailObjects += aNumChunksPerPool;

            mDataPools = realloc(mDataPools, 4 * mNumPools);
            mDataPools[aCurPoolNum] = new uchar* [32768]; //?

            mFreeIndicesPools = realloc(mFreeIndicesPools, 4 * mNumPools);
            mFreeIndicesPools[aCurPoolNum] = new int*[aNumChunksPerPool]; //?

            int aCurIdx = aNumChunksPerPool * aCurPoolNum;
            int anIndexPtr = *mFreeIndicesPools;
            int* anEnd = &anIndexPtr[aNumChunksPerPool];
            while (anIndexPtr != anEnd)
                *anIndexPtr++ = aCurIdx++;
        }

        _T* Alloc() //? | 70-82
        {
            if (!mNumAvailObjects)
                GrowPool();
            int aNumChunksPerPool = 32768 / mChunkSize;

            mNumAvailObjects--;

            int anIndex = mFreeIndicesPools[mNumAvailObjects / aNumChunksPerPool][mNumAvailObjects % aNumChunksPerPool];
            return new _T*(mDataPools[anIndex / aNumChunksPerPool][mChunkSize * (anIndex % aNumChunksPerPool)]);
        }

        void Free(_T* thePtr) //? | 85-103
        {
            int aNumChunksPerPool;
            int anOffset;
            int anIndex;
            for (int aPoolNum = mNumPools - 1; aPoolNum >= 0; aPoolNum--)
            {
                int anOffset = thePtr - mDataPools[aPoolNum];
                if (anOffset <= 0x7FFF)
                {
                    mFreeIndicesPools[mNumAvailObjects / (0x8000 / mChunkSize)][mNumAvailObjects % (0x8000 / mChunkSize)] = anOffset / mChunkSize + 0x8000 / mChunkSize * aPoolNum;
                    ++mNumAvailObjects;
                    break;
                }
            }
            delete thePtr(0); //?
        }
    };
}

#endif //__OBJECTPOOL_H__