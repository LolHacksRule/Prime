#ifndef __XPRINTF_H__
#define __XPRINTF_H__

#include "Common.h"

namespace xprintf //Not in Transmension, introduced in Blitz
{
	template <class _T> class TBufferWriter
	{
	public:
		virtual bool BufferWriteChar(_T inChar) = 0;
		virtual int BufferWriteString(const _T* inStr, int theLen) = 0;
		virtual void BufferTerminate() = 0;
		virtual const _T* BufferGetString() = 0;
		virtual int BufferGetPosition() const = 0;
		virtual bool BufferIsFull() const = 0;
	};
	template <class _T> class TMemBufferWriter : public TBufferWriter<_T>
	{
	protected:
		_T* mBuffer;
		int mBufSize;
		int mWritePos;
	public:
		TMemBufferWriter(_T* inBuffer, int inBufSize);
		virtual bool BufferWriteChar(_T inChar);
		virtual int BufferWriteString(const _T* inStr, int inLen);
		virtual void BufferTerminate();
		virtual const _T* BufferGetString();
		virtual int BufferGetPosition() const;
		virtual bool BufferIsFull() const;
	};
	template <class _T> class TNullBufferWriter : public TBufferWriter<_T>
	{
	public:
		virtual bool BufferWriteChar(_T inChar);
		virtual int BufferWriteString(const _T* inStr, int inLen);
		virtual void BufferTerminate();
		virtual const _T* BufferGetString();
		virtual int BufferGetPosition() const;
		virtual bool BufferIsFull() const;
		static TNullBufferWriter* GetInstance();
	};
	template <class _T> class THandlerEntry
	{
	public:
		_T mTypeName[56];
		void (THandlerEntry<_T>::*mHandlerFunc)(TBufferWriter<_T*>, const _T*, char*&, _T*);
		int mArgSize;
	};
	template <class _T> class TTypeTraits
	{
	public:
		typedef HANDLE DTypeHandlerFunc;
		typedef THandlerEntry<_T> DTypeHandlerEntry;
		static std::vector<DTypeHandlerEntry> sTypeHandlers;
		static const _T* GetEmptyString();
	};

	//All of this is in xprintf_aw.inc.
	/*template <class _T> static void* XTOSTRING32(ulong inValue, _T* outBuf, ulong inRadix, bool isNegValue);
	template <class _T> static void* XTOSTRING64(uint64 inValue, _T* outBuf, ulong inRadix, bool isNegValue);
	template <class _T> static _T* XITOA(int inValue, _T* outBuf, int inRadix);
	template <class _T> static void* XUITOA(ulong inValue, _T* outBuf, int inRadix);
	template <class _T> static _T* XI64TOA(int64 inValue, _T* outBuf, int inRadix);
	template <class _T> static _T* XUI64TOA(uint64 inValue, _T* outBuf, int inRadix);
	template <class _T> int xprintf_process(TBufferWriter<_T>* inWriter, const _T* inFmt, char* inArgs, std::vector<CVaList>* ioArgOffsets, bool inFindArgOffsets);
	template <class _T> int xviprintf(TBufferWriter<_T>* inWriter, const _T* inFmt, char* inArgs);
	template <class _T> int xvsnprintf(_T *inBuf, uint inMax, const _T* inFmt, char* inArgs);
	template <class _T> int xsnprintf(_T* inBuf, uint inMax, const char* inFmt...);
	//int wxprintf_process(TBufferWriter<wchar_t>* inWriter, const wchar_t* inFmt, char* inArgs, std::vector<CVaList>* ioArgOffsets, bool inFindArgOffsets);
	//int wxvsnprintf(wchar_t* inBuf, uint inMax, const wchar_t* inFmt, char* inArgs);
	//int wxsnprintf(wchar_t* inBuf, uint inMax, const wchar_t* inFmt...);

	//xprintf_process isnativetype
	//xprintf_process readflags
	//xprintf_process readwidth
	//xprintf_process ReadPrecision
	//xprintf_process ReadNativeFormatSpecsArgSize
	//xprintf_process ReadNativeFormatSpecs
	//xprintf_process HandleNativeType*/

	int sexyswprintf(wchar_t* dest, const wchar_t* fmt); //That's why they aren't releasing source oof
	class CVaList
	{
	public:
		char* mVaList;
		bool mVaListValid;
		CVaList(char*& inVaList);
		CVaList();
		~CVaList();
		CVaList& operator=(char*& inValList);
		CVaList& operator=(const CVaList*& inVaList);
		void Invalidate();
		bool IsValid();
		void Advance(int inSize);
		template <class _T> void Arg();
	};
	enum EFormatFlags
	{
		FORMATF_Minus = 1,
		FORMATF_Plus = 2,
		FORMATF_Zero = 4,
		FORMATF_Space = 8,
		FORMATF_Pound = 16,
		FORMATF_Caret = 32,
		FORMATF_Tilde = 64,
		FORMATF_Quote = 128,
		FORMATF_Comma = 256,
	};
}

#endif //__XPRINTF_H__