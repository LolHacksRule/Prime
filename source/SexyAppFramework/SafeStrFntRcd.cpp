#include "SafeStrFntRcd.h"

using namespace Sexy;

SafeStrFntRcd::SafeStrFntRcd() //6-9
{
	InitializeCriticalSection(&mCSStrFntRcd);
	mStrFntRcd.clear();
}
SafeStrFntRcd::~SafeStrFntRcd() //11-13
{
	DeleteCriticalSection(&mCSStrFntRcd);
}

void SafeStrFntRcd::AddRecord(const std::wstring& str, const std::string& fntFile) //16-43
{
	EnterCriticalSection(&mCSStrFntRcd);
	std::string fontFileName = fntFile;
	if (mStrFntRcd.find(str) == mStrFntRcd.end() || strcmp(mStrFntRcd[str].c_str(), fontFileName.c_str()))
		mStrFntRcd[str] = fontFileName;
	LeaveCriticalSection(&mCSStrFntRcd);
}
void SafeStrFntRcd::WriteRecordToFile() //45-74
{
	FILE* fileHandle = _wfopen(L"StringRecord.txt", L"w+,css=UTF-8");;
	if (fileHandle == NULL)
		return;

	WStringStringMap::const_iterator it = mStrFntRcd.begin();
	while (it != mStrFntRcd.end())
	{
		SexyString theStringDrawn = it->first;
		SexyString theFontFile = StringToWString(it->second);
		fwprintf(fileHandle, L"String %s <==> Font file: %s\n", theStringDrawn.c_str(), theFontFile.c_str());
		it++;
	}
	fclose(fileHandle);
}