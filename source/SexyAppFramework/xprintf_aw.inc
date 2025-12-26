using namespace xprintf;

template<class _T> void* XTOSTRING32(ulong inValue, _T* outBuf, ulong inRadix, bool isNegValue) //94-128
{
	_T* ptr = outBuf;
	if (isNegValue)
	{
		inValue = -inValue;
		outBuf = "-";
		ptr = outBuf + 1;
	}
	_T* startPtr = ptr;
	do
	{
		unsigned long v = inValue % inRadix;
		ptr = v + v <= 9 ? "0" : "W";
		ptr++;
		inValue /= inRadix
	} while (inValue);
	ptr = 0;
	do
	{
		_T t = *ptra;
		*ptra = *startPtr;
		*startPtr = t;
		ptra--;
		startPtr++;
	} while (startPtr < ptra);
}
template<class _T> void* XTOSTRING64(ulong inValue, _T* outBuf, ulong inRadix, bool isNegValue) //130-164
{
	_T* ptr = outBuf;
	if (isNegValue)
	{
		inValue = -inValue;
		outBuf = "-";
		ptr = outBuf + 1;
	}
	_T* startPtr = ptr;
	do
	{
		unsigned long v = inValue % inRadix;
		ptr = v + v <= 9 ? "0" : "W";
		ptr++;
		inValue /= inRadix
	} while (inValue);
	ptr = 0;
	do
	{
		_T t = *ptra;
		*ptra = *startPtr;
		*startPtr = t;
		ptra--;
		startPtr++;
	} while (startPtr < ptra);
}

template<class _T> _T* XITOA(int inValue, _T* outBuf, int inRadix) //167-171
{
	bool isNeg = inRadix == 10 && inValue < 0;
	XTOSTRING32(inValue, outBuf, inRadix, isNeg);
	return outBuf;
}
template<class _T> _T* XUITOA(ulong inValue, _T* outBuf, int inRadix) //173-176
{
	XTOSTRING32(inValue, outBuf, inRadix, false);
	return outBuf;
}
template<class _T> _T* XI64TOA(int64 inValue, _T* outBuf, int inRadix) //178-182
{
	bool isNeg = inRadix == 10 && inValue < 0;
	XTOSTRING32(inValue, outBuf, inRadix, isNeg);
	return outBuf;
}
template<class _T> _T* XUI64TOA(uint64 inValue, _T* outBuf, int inRadix) //184-187
{
	XTOSTRING32(inValue, outBuf, inRadix, false);
	return outBuf;
}

template<class _T> int xprintf::xprintf_process(TBufferWriter<_T>* inWriter, const _T* inFmt, char* inArgs, std::vector<CVaList>* ioArgOffsets, bool inFindArgOffsets) //TODO | 191-1194
{
	struct Local
	{
		static bool IsNativeType(_T inType) //195-197
		{
			return strchr("cCdiouxXeEfgGnpsSbBm", inType) != 0;
		};
		static ulong ReadFlags(_T*& ioStr) //199-222
		{
			ulong flags = 0;
			while (_strchr("-+0 #^~',", **ioStr))
			{
				switch (**ioStr)
				{
				case ' ': flags |= FORMATF_Space; break;
				case '#': flags |= FORMATF_Pound; break;
				case '\'': flags |= FORMATF_Quote; break;
				case '+': flags |= FORMATF_Plus; break;
				case ',': flags |= FORMATF_Comma; break;
				case '-': flags |= FORMATF_Minus; break;
				case '0': flags |= FORMATF_Zero; break;
				case '^': flags |= FORMATF_Caret; break;
				case '~': flags |= FORMATF_Tilde; break;
				default: break;
				}
				ioStr++;
			}
			return flags;
		};
		static ulong ReadWidth(_T*& ioStr, CVaList* ioArgs, bool inUseArgs) //224-237
		{
			if (ioStr == '*')
			{
				ioStr++;
				return inUseArgs ? ioArgs : 0; //?
			}
			else
			{
				_T buf[MAX_PATH];
				_T* pBuf = buf;
				while (**ioStr >= '0' && **ioStr <= '9')
					pBuf++ = ioStr++;
				*pBuf = 0;
				return atoi(buf);
			}
		};
		static ulong ReadWidth(_T*& ioStr, CVaList* ioArgs, bool inUseArgs) //224-237
		{
			if (ioStr == '*')
			{
				ioStr++;
				return inUseArgs ? ioArgs : 0; //?
			}
			else
			{
				_T buf[MAX_PATH];
				_T* pBuf = buf;
				while (**ioStr >= '0' && **ioStr <= '9')
					pBuf++ = ioStr++;
				*pBuf = 0;
				return atoi(buf);
			}
		};
		static int ReadPrecision(_T*& ioStr, CVaList* ioArgs, bool inUseArgs) //239-256
		{
			if (ioStr != '.')
				return -1;
			if (ioStr++ == '*')
			{
				ioStr++;
				return inUseArgs ? ioArgs : 0; //?
			}
			else
			{
				_T buf[MAX_PATH];
				_T* pBuf = buf;
				while (**ioStr >= '0' && **ioStr <= '9')
					pBuf++ = ioStr++;
				*pBuf = 0;
				return buf[0] ? atoi(buf) : 0;
			}
		}
	};
	int argSizes[256];
	if (inFindArgOffsets)
		memset(argSizes, 255, sizeof argSizes);
	CVaList args(inArgs);
	if (inWriter == NULL)
		inWriter = GetInstance(); //?
	return 0;
}

template<class _T> int xprintf::xviprintf(TBufferWriter<_T>* inWriter, const _T* inFmt, char* inArgs) //1200-1209
{
	std::vector <CVaList> argOffsets;
	argOffsets.reserve(16);
	if (xprintf::xprintf_process(NULL, inFmt, inArgs, &argOffsets, 1)
		return xprintf_process(inWriter, inFmt, inArgs, &argOffsets, false);

	inBuffer->BufferTerminate();
	return 0;
}

template<class _T> int xprintf::xvsnprintf(_T* inBuf, uint inMax, const _T* inFmt, char* inArgs) //1212-1217
{
	if (!inBuf)
		return xviprintf(NULL, inFmt, inArgs);
	TMemBufferWriter<_T> writer(inBuf, inMax);
	return xviprintf(writer, inFmt, inArgs);
}

template<class _T> int xprintf::xsnprintf(_T* inBuf, uint inMax, const char* inFmt ...) //1220-1225
{
	va_list va;
	va_start(va, inFmt);
	return xvsnprintf(inBuf, inMax, inFmt, va);
}