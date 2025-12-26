#include "xprintf.h"
//#include "xprintf_aw.inc"

using namespace xprintf;

const char* TTypeTraits<char>::GetEmptyString() { return ""; } //92

TTypeTraits<char> sTypeHandlers; //96

const wchar_t* TTypeTraits<wchar_t>::GetEmptyString() { return L""; } //104

TTypeTraits<wchar_t> sTypeHandlers; //108

template <class _T> TMemBufferWriter<_T>::TMemBufferWriter(_T* inBuffer, int inBufSize) { mBuffer = inBuffer; mBufSize = inBufSize; mWritePos = 0; } //131

template <class _T> bool TMemBufferWriter<_T>::BufferWriteChar(_T inChar) //134-140
{
	if (BufferIsFull())
		return false;

	mBuffer[mWritePos++] = inChar;
	return !BufferIsFull();
} 
template <class _T> int TMemBufferWriter<_T>::BufferWriteString(const _T* inStr, int inLen) //142-152
{
	int len = 0;
	while (inStr)
	{
		if (!BufferIsFull(inStr))
			break;
		inStr++;
		len++;
		if (inLen && len >= inLen)
			break;
	}
	return len;
}
template <class _T> void TMemBufferWriter<_T>::BufferTerminate() //154-166
{
	if (mBufSize != 0 || mWritePos < mBufSize)
		mBuffer[mWritePos] = NULL;
}
template <class _T> const _T* TMemBufferWriter<_T>::BufferGetString() //168-171
{
	BufferTerminate();
	return mBuffer;
}
template <class _T> int TMemBufferWriter<_T>::BufferGetPosition() const //173-175
{	
	return mWritePos;
}
template <class _T> bool TMemBufferWriter<_T>::BufferIsFull() const //177-182
{
	if (mWritePos < mBufSize)
		return false;

	return mBufSize;
}

template <class _T> bool TNullBufferWriter<_T>::BufferWriteChar(_T inChar) { return false; } //190
template <class _T> int TNullBufferWriter<_T>::BufferWriteString(const _T* inStr, int inLen) { return false; } //191
template <class _T> void TNullBufferWriter<_T>::BufferTerminate() {} //192
template <class _T> const _T* TNullBufferWriter<_T>::BufferGetString() { return TTypeTraits<_T>::GetEmptyString(); } //193
template <class _T> int TNullBufferWriter<_T>::BufferGetPosition() const { return 0; } //194
template <class _T> bool TNullBufferWriter<_T>::BufferIsFull() const { return false; } //195

TNullBufferWriter<char>* TNullBufferWriter<char>::GetInstance() { static TNullBufferWriter<char> sInstance; return &sInstance; } //198
TNullBufferWriter<wchar_t>* TNullBufferWriter<wchar_t>::GetInstance() { static TNullBufferWriter<wchar_t> sInstance; return &sInstance; } //199

CVaList::CVaList() { mVaListValid = false; } //213

CVaList::CVaList(char*& inVaList) //216-218
{
	mVaListValid = true;
	mVaList = inVaList;
}

CVaList::~CVaList() //220-222
{
	Invalidate();
}

CVaList& CVaList::operator=(const CVaList*& inVaList) //225-232
{
	if (mVaListValid)
		mVaList = NULL;
	mVaListValid = inVaList->mVaListValid;
	if (mVaListValid)
		mVaList = inVaList->mVaList;
	return *this;
}

void CVaList::Invalidate() //241-245
{
	if (mVaListValid)
		mVaList = NULL;
	mVaListValid = false;
}

void CVaList::Advance(int inSize) //252-267
{
	if (mVaListValid)
		mVaList += inSize;
}

template<class _T> void CVaList::Arg() //269-271
{
	mVaList += sizeof _T; return mVaList - 1;
}

int xprintf::sexyswprintf(wchar_t* dest, const wchar_t* fmt) //284-298
{
	va_list args;
	va_start(args, fmt);
	int numChars = 1073741823; //?
	return _vsnwprintf(dest, numChars, fmt, args);
}
