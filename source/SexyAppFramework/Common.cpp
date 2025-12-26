#include "Common.h"
#include "MTRand.h"
#include "Debug.h"
//Do we need
#include <direct.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <aclapi.h>

#include "PerfTimer.h"

#include "AutoCrit.h"
#include "xprintf.h"
#include "IFileDriver.h"
#include "Point.h"

HINSTANCE Sexy::gHInstance;
bool Sexy::gDebug = false;
#ifndef _TRANSMENSION
bool gXprintfInitialized = false;
#endif

static Sexy::CritSect gXprintfInitCrit; //29
static Sexy::MTRand gCommonMTRand; //Changed from gMTRand | 44

using namespace xprintf; //?

const char* CharToCharFunc::Str(const char* theStr) //50-52
{
	return theStr;
}

char CharToCharFunc::Char(char theChar) //55-57
{
	return theChar;
}

const std::wstring CharToWCharFunc::Str(const char* theStr) //63-65
{
	return Sexy::StringToWString(theStr);
}

wchar_t CharToWCharFunc::Char(char theChar) //68-70
{
	return theChar;
}

int Sexy::Rand() //75-77
{
	return gCommonMTRand.Next();
}

int Sexy::Rand(int range) //80-82
{
	return gCommonMTRand.Next((unsigned long)range);
}

float Sexy::Rand(float range) //85-87
{
	return gCommonMTRand.Next(range);
}

void Sexy::SRand(ulong theSeed) //90-92
{
	gCommonMTRand.SRand(theSeed);
}

#if !defined(_EAMT) && !defined(_IPHONEOS) //Only on Windows/Transmension but present on Mac (ZR Multilingual and Legacy), why idk

bool Sexy::CheckFor98Mill() //97-120
{
#ifdef _WIN32
	static bool needOsCheck = true;
	static bool is98Mill = false;

	if (needOsCheck)
	{
		bool invalid = false; //why
		OSVERSIONINFOEXA osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
		if( GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
		{
			osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
			if ( GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
				return false;
		}

		needOsCheck = false;
		is98Mill = osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS; // let's check Win95, 98, *AND* ME.
	}

	return is98Mill;
#else //Mac or Transmension
	return false;
#endif
}

bool Sexy::CheckForVista() //123-146
{
#ifdef _WIN32
	static bool needOsCheck = true;
	static bool isVista = false;

	if (needOsCheck)
	{
		bool invalid = false;
		OSVERSIONINFOEXA osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
		if( GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
		{
			osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
			if ( GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
				return false;
		}

		needOsCheck = false;
		isVista = osvi.dwMajorVersion >= 6;
	}

	return isVista;
#else //Mac or Transmension
	return false;
#endif
}
#endif

#if defined(_WIN32_) || defined(_MAC)

bool Sexy::CheckForWin7() //Only on Win and Mac, Not in Transmension, even in Bej3 fork | 149-172
{
#ifdef _WIN32
	static bool needOsCheck = true;
	static bool isWin7 = false;

	if (needOsCheck)
	{
		bool invalid = false;
		OSVERSIONINFOEXA osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
		if (GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
		{
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
			if (GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
				return false;
		}

		needOsCheck = false;
		isWin7 = osvi.dwMajorVersion >= 6 || osvi.dwMajorVersion == 6 && osvi.dwMinorVersion;
	}

	return isWin7;
#else //Probably
	return false;
#endif
}
#endif

#ifdef _WIN32

std::string Sexy::GetOSVersionString() //Correct? Present in MacOS ZR | 175-283
{
	static std::string sStr;
	std::string osNameStr;
	OSVERSIONINFOEXA osvi;
	if (sStr.empty())
		return sStr;

	memset(&osvi, 0, sizeof(OSVERSIONINFOEXA));
	if (GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		if (GetVersionExA((LPOSVERSIONINFOA)&osvi) == 0)
			return "";
		if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			if (osvi.dwMajorVersion == 4)
				osNameStr = osvi.dwMinorVersion == 10 ? "Windows 98" : osvi.dwMinorVersion == 90 ? "Windows ME" : "Windows 95";
		}
		if (osNameStr.empty())
			osNameStr = StrFormat("Unknown Windows %d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
		else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			switch (osvi.dwMajorVersion)
			{
			case 4:
				if (!osvi.dwMinorVersion)
					osNameStr == "Windows NT 4.0";
				break;
			case 5:
				osNameStr = osvi.dwMinorVersion == 1 ? "Windows 98" : osvi.dwMinorVersion == 2 ? "Windows Server 2003" : "Windows 2000";
				break;
			case 6:
#ifdef _SEXYDECOMP_USE_OPTIMIZED_CODE
				osNameStr = osvi.dwMinorVersion == 3 ? "Windows 8.1" : osvi.dwMinorVersion == 2 ? "Windows 8" : osvi.dwMinorVersion == 1 ? "Windows 7" : "Windows Vista"; //Detect Windows 8 and 8.1
				break;
			case 10:
				osNameStr = osvi.dwMinorVersion >= 22000 ? "Windows 11" : "Windows 10"; //Detect Windows 10 and 11
				break;
#else
				osNameStr = osvi.dwMinorVersion == 1 ? "Windows 7" : "Windows Vista";
#endif
				break;
			}
			if (osNameStr.empty())
				osNameStr = StrFormat("Unknown WinNT %d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
		}
	}
	else
		osNameStr = StrFormat("Unknown Platform%d %d.%d", osvi.dwPlatformId, osvi.dwMajorVersion, osvi.dwMinorVersion); //Calls osvi[20], not sure if this is correct;
	return osNameStr += StrFormat(" Build %d", osvi.dwBuildNumber);
}

void Sexy::GetProcessorInfoStrings(StringVector& outStrings) //Correct? Present in MacOS ZR | 286-330
{
	static StringVector sInfoStrings;
	HKEY aProcessorKey;
	char aBuf[512];
	int iProcessor = 0;
	if (sInfoStrings.empty())
	{
		while (iProcessor < 15)
		{
			std::string aKeyName = StrFormat("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d", iProcessor);
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, aKeyName.c_str(), 0, KEY_READ, &aProcessorKey) == ERROR_SUCCESS);
				break;
			std::string aIdentStr;
			ulong aType = 1;
			ulong aSize = 512;
			if (RegQueryValueExA(aProcessorKey, "Identifier", 0, &aType, (LPBYTE)aBuf, &aSize) == ERROR_SUCCESS)
			{
				aBuf[aSize] = 0;
				aIdentStr = aBuf;
			}
			std::string aNameStr;
			if (RegQueryValueExA(aProcessorKey, "ProcessorNameString", 0, &aType, (LPBYTE)aBuf, &aSize) == ERROR_SUCCESS)
			{
				aBuf[aSize] = 0;
				aNameStr = aBuf;
			}
			RegCloseKey(aProcessorKey);
			std::string aCpuString = StrFormat("CPU %d: \"%s\" \"%s\"", iProcessor, (const char*)aIdentStr.c_str(), (const char*)aNameStr.c_str());
			sInfoStrings.push_back(aCpuString);
			iProcessor++;
		}
	}
	outStrings = sInfoStrings;
}

#endif

std::string Sexy::GetAppDataFolder() //335-337
{
	return gFileDriver->GetSaveDataPath(); //MOVED TO *APPDRIVER
}

void Sexy::SetAppDataFolder(const std::string& thePath) //340-356
{
	DBG_ASSERT("DEPRECATED IN PRIME" == NULL); //341: The only remnant that mentions Prime. Possible the name of PopCap's new framework since Bejewled 3, maybe Pop/PopCap Rewritten Internal Multiplatform Engine
}

std::string Sexy::URLEncode(const std::string& theString) //Not in XNA | 360-387
{
	char* aHexChars = "0123456789ABCDEF";

	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
	{
		switch (theString[i])
		{
		case ' ':
			aString.insert(aString.end(), '+');
			break;
		case '?':
		case '&':
		case '%':
		case '+':
		case '\r':
		case '\n':
		case '\t':
			aString.insert(aString.end(), '%');
			aString.insert(aString.end(), aHexChars[(theString[i] >> 4) & 0xF]);
			aString.insert(aString.end(), aHexChars[(theString[i]     ) & 0xF]);
			break;
		default:
			aString.insert(aString.end(), theString[i]);
		}
	}

	return aString;
}

std::string Sexy::StringToUpper(const std::string& theString) //C++ only | 390-397
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += toupper(theString[i]);

	return aString;
}

std::wstring Sexy::StringToUpper(const std::wstring& theString) //C++ only | 400-407
{
	std::wstring aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += towupper(theString[i]);

	return aString;
}

std::string Sexy::StringToLower(const std::string& theString) //C++ only | 400-417
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += tolower(theString[i]);

	return aString;
}

std::wstring Sexy::StringToLower(const std::wstring& theString) //C++ only | 420-427
{
	std::wstring aString;

	for (unsigned i = 0; i < theString.length(); ++i)
		aString += tolower(theString[i]);

	return aString;
}

std::wstring Sexy::StringToWString(const std::string &theString) //C++ only | 430-436
{
	std::wstring aString;
	aString.reserve(theString.length());
	for(size_t i = 0; i < theString.length(); ++i)
		aString += (unsigned char)theString[i];
	return aString;
}

std::string Sexy::WStringToString(const std::wstring &theString, bool* isValid) //C++ only, changed | 439-450
{
	std::string aNewString;
	if (isValid) //Huh
		*isValid = 1;
	aNewString.reserve(theString.length());
	for (int i = 0; i < theString.length(); ++i)
	{
		const uint c = theString[i];
		if (isValid)
		{
			if (isValid)
				*isValid = c < 0x100;
		}
		aNewString.push_back(c);
	}
	return aNewString;
}

//----------------------------------------------------------------------------
// Stream UTF8 data in a const char* to keep receiving wchar_ts
//----------------------------------------------------------------------------
int Sexy::GetNextUTF8CharFromStream(const char** theBuffer, int theLen, wchar_t* theChar) //Changed from Buffer's GetUTF8Char, now in Sexy (then moved back in XNA) | 456-536
{
	static const unsigned short aMaskData[] = {
		0xC0,		// 1 extra byte
		0xE0,		// 2 extra bytes
		0xF0,		// 3 extra bytes
		0xF8,		// 4 extra bytes
		0xFC		// 5 extra bytes
	};

	if (theLen == 0) return 0;

	const char* aBuffer = *theBuffer;

	int aTempChar = int((unsigned char)*aBuffer++);
	if ((aTempChar & 0x80) != 0)
	{
		if ((aTempChar & 0xC0) != 0xC0) return 0; // sanity check: high bit should not be set without the next highest bit being set, too.

		int aBytesRead[6];
		int* aBytesReadPtr = &aBytesRead[0];

		*aBytesReadPtr++ = aTempChar;

		int aLen;
		for (aLen = 0; aLen < (int)(sizeof(aMaskData) / sizeof(*aMaskData)); ++aLen)
		{
			if ((aTempChar & aMaskData[aLen]) == ((aMaskData[aLen] << 1) & aMaskData[aLen])) break;
		}
		if (aLen >= (int)(sizeof(aMaskData) / sizeof(*aMaskData))) return 0;

		aTempChar &= ~aMaskData[aLen];
		int aTotalLen = aLen + 1;

		if (aTotalLen < 2 || aTotalLen > 6) return 0;

		int anExtraChar = 0;
		while (aLen > 0 && (aBuffer - *theBuffer) < theLen)
		{
			anExtraChar = int((unsigned char)*aBuffer++);
			if ((anExtraChar & 0xC0) != 0x80) return 0; // sanity check: high bit set, and next highest bit NOT set.

			*aBytesReadPtr++ = anExtraChar;

			aTempChar = (aTempChar << 6) | (anExtraChar & 0x3F);
			--aLen;
		}
		if (aLen > 0) return 0; // ran out of data before ending sequence

		// validate substrings
		bool valid = true;
		switch (aTotalLen)
		{
		case 2:
			valid = !((aBytesRead[0] & 0x3E) == 0);
			break;
		case 3:
			valid = !((aBytesRead[0] & 0x1F) == 0 && (aBytesRead[1] & 0x20) == 0);
			break;
		case 4:
			valid = !((aBytesRead[0] & 0x0F) == 0 && (aBytesRead[1] & 0x30) == 0);
			break;
		case 5:
			valid = !((aBytesRead[0] & 0x07) == 0 && (aBytesRead[1] & 0x38) == 0);
			break;
		case 6:
			valid = !((aBytesRead[0] & 0x03) == 0 && (aBytesRead[1] & 0x3C) == 0);
			break;
		}
		if (!valid) return 0;
	}

	int aConsumedCount = aBuffer - *theBuffer;

	if ((aTempChar >= 0xD800 && aTempChar <= 0xDFFF) || (aTempChar >= 0xFFFE && aTempChar <= 0xFFFF))
		return 0;

	*theChar = (wchar_t)aTempChar;

	*theBuffer = aBuffer;
	return aConsumedCount;
}

std::wstring Sexy::UTF8StringToWString(const std::string& theString) //Similar to Buffer::UTF8ToWideString, C++ only | 539-558
{
	std::wstring aString;
	int aDataSizeBytes = theString.length();
	const char* aData = (const char*)theString.c_str();
	const char* anEnd = &aData[aDataSizeBytes];
	while (aDataSizeBytes > 0)
	{
		if (aData >= anEnd)
			break;
		wchar_t aChar;
		int aConsumed = GetNextUTF8CharFromStream(&aData, aDataSizeBytes, &aChar);
		if (aConsumed == 0)
			break;
		aDataSizeBytes -= aConsumed;
		aString += aChar;
	}
	assert(aData == anEnd); //555
	return aString;
}

std::string Sexy::WStringToUTF8String(const std::wstring& theString) //Similar to Buffer::WriteUTF8String except returns a string, C++ only | 561-590
{
	std::string aTemp;
	for (int i = 0; i < (int)theString.length(); ++i)
	{
		const unsigned int c = (unsigned int)theString[i];
		if (c < 0x80)
		{
			aTemp.push_back((uchar)c);
		}
		else if (c < 0x800)
		{
			aTemp.push_back((uchar)(0xC0 | (c >> 6)));
			aTemp.push_back((uchar)(0x80 | (c & 0x3F)));
		}
		else if (c < 0x10000)
		{
			aTemp.push_back((uchar)(0xE0 | c >> 12));
			aTemp.push_back((uchar)(0x80 | ((c >> 6) & 0x3F)));
			aTemp.push_back((uchar)(0x80 | (c & 0x3F)));
		}
		else if (c < 0x110000)
		{
			aTemp.push_back((uchar)(0xF0 | (c >> 18)));
			aTemp.push_back((uchar)(0x80 | ((c >> 12) & 0x3F)));
			aTemp.push_back((uchar)(0x80 | ((c >> 6) & 0x3F)));
			aTemp.push_back((uchar)(0x80 | (c & 0x3F)));
		}
	}
	return aTemp;
}

SexyString Sexy::StringToSexyString(const std::string& theString) //593-599
{
#ifdef _USE_WIDE_STRING
	return StringToWString(theString);
#else
	return SexyString(theString);
#endif
}

SexyString Sexy::WStringToSexyString(const std::wstring &theString) //602-608
{
#ifdef _USE_WIDE_STRING
	return SexyString(theString);
#else
	return WStringToString(theString);
#endif
}

std::string Sexy::SexyStringToString(const SexyString& theString) //611-617
{
#ifdef _USE_WIDE_STRING
	return WStringToString(theString, false);
#else
	return std::string(theString);
#endif
}

std::wstring Sexy::SexyStringToWString(const SexyString& theString) //620-626
{
#ifdef _USE_WIDE_STRING
	return std::wstring(theString);
#else
	return StringToWString(theString);
#endif
}

std::string Sexy::ToString(const std::string& theString) //629-631
{
	return theString;
}

std::string Sexy::ToString(const std::wstring& theString) //634-636
{
	return WStringToString(theString, false);
}

std::wstring Sexy::ToWString(const std::string& theString) //639-641
{
	return StringToWString(theString);
}

std::wstring Sexy::ToWString(const std::wstring& theString) //644-646
{
	return theString;
}

SexyString Sexy::ToSexyString(const std::string& theString) //649-651
{
	return StringToSexyString(theString);
}

SexyString Sexy::ToSexyString(const std::wstring& theString) //654-656
{
	return WStringToSexyString(theString);
}

std::string Sexy::Trim(const std::string& theString) //659-669
{
	int aStartPos = 0;
	while ( aStartPos < (int) theString.length() && isspace(theString[aStartPos]) )
		aStartPos++;

	int anEndPos = theString.length() - 1;
	while ( anEndPos >= 0 && isspace(theString[anEndPos]) )
		anEndPos--;

	return theString.substr(aStartPos, anEndPos - aStartPos + 1);
}

std::wstring Sexy::Trim(const std::wstring& theString) //672-682
{
	int aStartPos = 0;
	while ( aStartPos < (int) theString.length() && iswspace(theString[aStartPos]) )
		aStartPos++;

	int anEndPos = theString.length() - 1;
	while ( anEndPos >= 0 && iswspace(theString[anEndPos]) )
		anEndPos--;

	return theString.substr(aStartPos, anEndPos - aStartPos + 1);
}

bool Sexy::StringToInt(std::string theString, int* theIntVal) //685-734
{
	*theIntVal = 0;

	if (theString.length() == 0)
		return false;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];
		
		if ((theRadix == 10) && (aChar >= '0') && (aChar <= '9'))
			*theIntVal = (*theIntVal * 10) + (aChar - '0');
		else if ((theRadix == 0x10) && 
			(((aChar >= '0') && (aChar <= '9')) || 
			 ((aChar >= 'A') && (aChar <= 'F')) || 
			 ((aChar >= 'a') && (aChar <= 'f'))))
		{			
			if (aChar <= '9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - '0');
			else if (aChar <= 'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'a') + 0x0A;
		}
		else if (((aChar == 'x') || (aChar == 'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}
	}

	if (isNeg)
		*theIntVal = -*theIntVal;

	return true;
}

bool Sexy::StringToInt(std::wstring theString, int* theIntVal) //737-786
{
	*theIntVal = 0;

	if (theString.length() == 0)
		return false;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];
		
		if ((theRadix == 10) && (aChar >= L'0') && (aChar <= L'9'))
			*theIntVal = (*theIntVal * 10) + (aChar - L'0');
		else if ((theRadix == 0x10) && 
			(((aChar >= L'0') && (aChar <= L'9')) || 
			 ((aChar >= L'A') && (aChar <= L'F')) || 
			 ((aChar >= L'a') && (aChar <= L'f'))))
		{			
			if (aChar <= L'9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'0');
			else if (aChar <= L'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'a') + 0x0A;
		}
		else if (((aChar == L'x') || (aChar == L'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}
	}

	if (isNeg)
		*theIntVal = -*theIntVal;

	return true;
}

bool Sexy::StringToDouble(std::string theString, double* theDoubleVal) //789-843
{
	*theDoubleVal = 0.0;

	if (theString.length() == 0)
		return false;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - '0');
		else if (aChar == '.')
		{
			i++;
			break;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	double aMult = 0.1;
	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
		{
			*theDoubleVal += (aChar - '0') * aMult;	
			aMult /= 10.0;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	if (isNeg)
		*theDoubleVal = -*theDoubleVal;

	return true;
}

bool Sexy::StringToDouble(std::wstring theString, double* theDoubleVal) //846-900
{
	*theDoubleVal = 0.0;

	if (theString.length() == 0)
		return false;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((aChar >= L'0') && (aChar <= L'9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - L'0');
		else if (aChar == L'.')
		{
			i++;
			break;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	double aMult = 0.1;
	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((aChar >= L'0') && (aChar <= L'9'))
		{
			*theDoubleVal += (aChar - L'0') * aMult;	
			aMult /= 10.0;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	if (isNeg)
		*theDoubleVal = -*theDoubleVal;

	return true;
}

//<locale>? Ehhh close enough
SexyString Sexy::CommaSeperate(int theValue) //907-909
{
	return Locale::CommaSeparate(theValue);
}

SexyString Sexy::UCommaSeparate(uint theValue) //912-914
{
	return Locale::UCommaSeparate(theValue);
}

std::string Sexy::GetCurDir() //917-921
{
	return gFileDriver->GetCurPath(); //MOVED TO *FILEDRIVER
}

std::string Sexy::GetFullPath(const std::string& theRelPath) //924-926
{
	return GetPathFrom(theRelPath, GetCurDir());
}
//TODO
template <class _T, class _F>
static _T GetPathFrom_(const _T& theRelPath, const _T& theDir) //930-1025 (appears twice)
{
	_T aDriveString;
    _T aNewPath = theDir;
 
    if ((theRelPath.length() >= 2) && (theRelPath[1] == _F::Char(':')))
        return theRelPath;
 
    _T::value_type aSlashChar = _F::Char('/');
 
    if ((theRelPath.find(_F::Char('\\')) != -1) || (theDir.find(_F::Char('\\')) != -1))
        aSlashChar = _F::Char('\\');
 
    if ((aNewPath.length() >= 2) && (aNewPath[1] == ':'))
    {
        aDriveString = aNewPath.substr(0, 2);
        aNewPath.erase(aNewPath.begin(), aNewPath.begin()+2);
    }
 
    // Append a trailing slash if necessary
    if ((aNewPath.length() > 0) && (aNewPath[aNewPath.length()-1] != _F::Char('\\')) && (aNewPath[aNewPath.length()-1] != _F::Char('/')))
        aNewPath += aSlashChar;
 
    _T aTempRelPath = theRelPath;
 
    for (;;)
    {
        if (aNewPath.length() == 0)
            break;
 
        int aFirstSlash = aTempRelPath.find('\\');
        int aFirstForwardSlash = aTempRelPath.find('/');
 
        if ((aFirstSlash == -1) || ((aFirstForwardSlash != -1) && (aFirstForwardSlash < aFirstSlash)))
            aFirstSlash = aFirstForwardSlash;
 
        if (aFirstSlash == -1)
            break;
 
        _T aChDir = aTempRelPath.substr(0, aFirstSlash);
 
        aTempRelPath.erase(aTempRelPath.begin(), aTempRelPath.begin() + aFirstSlash + 1);                       
 
        if (aChDir.compare(_F::Str("..")) == 0)
        {           
            int aLastDirStart = aNewPath.length() - 1;
            while ((aLastDirStart > 0) && (aNewPath[aLastDirStart-1] != '\\') && (aNewPath[aLastDirStart-1] != '/'))
                aLastDirStart--;
 
            _T aLastDir = aNewPath.substr(aLastDirStart, aNewPath.length() - aLastDirStart - 1);
            if (aLastDir.compare(_F::Str("..")) == 0)
            {
                aNewPath += _F::Str("..");
                aNewPath += aSlashChar;
            }
            else
            {
                aNewPath.erase(aNewPath.begin() + aLastDirStart, aNewPath.end());
            }
        }       
        else if (aChDir.compare(_F::Str("")) == 0)
        {
            aNewPath = aSlashChar;
            break;
        }
        else if (aChDir.compare(_F::Str(".")) != 0)
        {
            aNewPath += aChDir + aSlashChar;
            break;
        }
    }
 
    aNewPath = aDriveString + aNewPath + aTempRelPath;
 
    if (aSlashChar == _F::Char('/'))
    {
        for (;;)
        {
            int aSlashPos = aNewPath.find(_F::Char('\\'));
            if (aSlashPos == -1)
                break;
            aNewPath[aSlashPos] = _F::Char('/');
        }
    }
    else
    {
        for (;;)
        {
            int aSlashPos = aNewPath.find(_F::Char('/'));
            if (aSlashPos == -1)
                break;
            aNewPath[aSlashPos] = _F::Char('\\');
        }
    }
 
    return aNewPath;
}

std::string Sexy::GetPathFrom(const std::string& theRelPath, const std::string& theDir) //1028-1030
{
	return GetPathFrom_<std::string, CharToCharFunc>(theRelPath, theDir); //Unmatched?
}


#ifdef _WIN32

bool Sexy::AllowAllAccess(const std::string& theFileName) //Also appears on ZR MacOS | 1039-1123
{
	HMODULE aLib = LoadLibraryA("advapi32.dll");
	if (aLib == NULL)
		return false;

	BOOL (WINAPI *fnSetFileSecurity)(LPCTSTR lpFileName, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor);
	BOOL (WINAPI *fnSetSecurityDescriptorDacl)(PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bDaclPresent, PACL pDacl, BOOL bDaclDefaulted);
	BOOL (WINAPI *fnInitializeSecurityDescriptor)(PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision);
	BOOL (WINAPI *fnAllocateAndInitializeSid)(
	  PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	  BYTE nSubAuthorityCount,
	  DWORD dwSubAuthority0,
	  DWORD dwSubAuthority1,
	  DWORD dwSubAuthority2,
	  DWORD dwSubAuthority3,
	  DWORD dwSubAuthority4,
	  DWORD dwSubAuthority5,
	  DWORD dwSubAuthority6,
	  DWORD dwSubAuthority7,
	  PSID* pSid
	);
	DWORD (WINAPI *fnSetEntriesInAcl)(ULONG cCountOfExplicitEntries, PEXPLICIT_ACCESS pListOfExplicitEntries, PACL OldAcl, PACL* NewAcl);
	PVOID (WINAPI *fnFreeSid)(PSID pSid);

	*(void**)&fnSetFileSecurity = (void*)GetProcAddress(aLib, "SetFileSecurityA");
	*(void**)&fnSetSecurityDescriptorDacl = (void*)GetProcAddress(aLib, "SetSecurityDescriptorDacl");
	*(void**)&fnInitializeSecurityDescriptor = (void*)GetProcAddress(aLib, "InitializeSecurityDescriptor");
	*(void**)&fnAllocateAndInitializeSid = (void*)GetProcAddress(aLib, "AllocateAndInitializeSid");
	*(void**)&fnSetEntriesInAcl = (void*)GetProcAddress(aLib, "SetEntriesInAclA");
	*(void**)&fnFreeSid = (void*) GetProcAddress(aLib, "FreeSid");

	if (!(fnSetFileSecurity && fnSetSecurityDescriptorDacl && fnInitializeSecurityDescriptor && fnAllocateAndInitializeSid && fnSetEntriesInAcl && fnFreeSid))
	{
		FreeLibrary(aLib);
		return false;
	}


	PSID pEveryoneSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	bool result = false;

    // Create a well-known SID for the Everyone group.
    if (fnAllocateAndInitializeSid(&SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pEveryoneSID))
    {
		EXPLICIT_ACCESS ea;

		// Initialize an EXPLICIT_ACCESS structure for an ACE.
		// The ACE will allow Everyone read access to the key.
		ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
		ea.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName = (LPTSTR) pEveryoneSID;

		// Create a new ACL that contains the new ACEs.
		PACL pACL = NULL; 
		if (fnSetEntriesInAcl(1, &ea, NULL, &pACL) == ERROR_SUCCESS)
		{		
			// Initialize a security descriptor.  
			PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) new char[SECURITY_DESCRIPTOR_MIN_LENGTH]; 
						 
			if (fnInitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
			{  							 
				// Add the ACL to the security descriptor. 
				if (fnSetSecurityDescriptorDacl(pSD, 
						TRUE,     // bDaclPresent flag   
						pACL, 
						FALSE))   // not a default DACL 
				{
					if (fnSetFileSecurity(theFileName.c_str(), DACL_SECURITY_INFORMATION, pSD))
						result = true;
				}
			}
		}
	}

	FreeLibrary(aLib);
	return result;
}

#endif

bool Sexy::Deltree(const std::string& thePath) //1127-1129
{
	return gFileDriver->DeleteTree(thePath); //MOVED TO *FILEDRIVER
}

bool Sexy::FileExists(const std::string& theFileName, bool *isFolder) //1132-1134
{
	return gFileDriver->FileExists(theFileName, isFolder); //MOVED TO *FILEDRIVER
}

bool Sexy::FileExists(const std::wstring& theFileName, bool *isFolder) //1137-1139
{
	return gFileDriver->FileExists(theFileName, isFolder); //MOVED TO *FILEDRIVER
}

std::string Sexy::GetAppFullPath(const std::string& theAppRelPath) //1142-1144
{
	return GetPathFrom(theAppRelPath, gFileDriver->GetLoadDataPath());
}

void Sexy::MkDir(const std::string& theDir) //1147-1149
{
	gFileDriver->MakeFolders(theDir); //MOVED TO *FILEDRIVER
}

void Sexy::MkDir(const std::wstring& theDir) //1152-1154
{
	gFileDriver->MakeFolders(theDir); //MOVED TO *FILEDRIVER
}
//TODO
template <class _T, class _F>
static _T GetFileName_(const _T& thePath, bool noExtension) //1158-1172 (appears twice)
{
	int aLastSlash = max((int) thePath.rfind(_F::Char('\\')), (int) thePath.rfind(_F::Char('/')));
 
    if (noExtension)
    {
        int aLastDot = (int)thePath.rfind(_F::Char('.'));
        if (aLastDot > aLastSlash)
            return thePath.substr(aLastSlash + 1, aLastDot - aLastSlash - 1);
    }
 
    if (aLastSlash == -1)
        return thePath;
    else
        return thePath.substr(aLastSlash + 1);
}

std::string Sexy::GetFileName(const std::string& thePath, bool noExtension) //1175-1177
{
	return GetFileName_<std::string, CharToCharFunc>(thePath, noExtension); //Unmatched?
}

std::wstring Sexy::GetFileName(const std::wstring& thePath, bool noExtension) //1180-1182
{
	return GetFileName_<std::wstring, CharToWCharFunc>(thePath, noExtension); //Unmatched?
}
//TODO
template <class _T, class _F>
static _T GetFileDir_(const _T& thePath, bool withSlash) //1186-1198 (appears twice)
{
	int aLastSlash = max((int) thePath.rfind(_F::Char('\\')), (int) thePath.rfind(_F::Char('/')));
 
    if (aLastSlash == -1)
        return _F::Str("");
    else
    {
        if (withSlash)
            return thePath.substr(0, aLastSlash+1);
        else
            return thePath.substr(0, aLastSlash);
    }
}

std::string Sexy::GetFileDir(const std::string& thePath, bool withSlash) //1201-1203
{
	return GetFileDir_<std::string, CharToCharFunc>(thePath, withSlash); //Unmatched?
}

std::wstring Sexy::GetFileDir(const std::wstring& thePath, bool withSlash) //1206-1208
{
	return GetFileDir_<std::wstring, CharToWCharFunc>(thePath, withSlash); //Unmatched?
}
//Back to easy stuff
std::string Sexy::RemoveTrailingSlash(const std::string& theDirectory) //1211-1218
{
	int aLen = theDirectory.length();
	
	if ((aLen > 0) && ((theDirectory[aLen-1] == '\\') || (theDirectory[aLen-1] == '/')))
		return theDirectory.substr(0, aLen - 1);
	else
		return theDirectory;
}

std::wstring Sexy::RemoveTrailingSlash(const std::wstring& theDirectory) //C++ only | 1221-1228
{
	int aLen = theDirectory.length();

	if ((aLen > 0) && ((theDirectory[aLen - 1] == '\\') || (theDirectory[aLen - 1] == '/')))
		return theDirectory.substr(0, aLen - 1);
	else
		return theDirectory;
}

std::string Sexy::AddTrailingSlash(const std::string& theDirectory, bool backSlash) //C++ only | 1231-1242
{
	if (!theDirectory.empty())
	{
		char aChar = theDirectory[theDirectory.length()-1];
		if (aChar!='\\' && aChar!='/')
			return theDirectory + (backSlash?'\\':'/');
		else
			return theDirectory;
	}
	else
		return "";
}

std::wstring Sexy::AddTrailingSlash(const std::wstring& theDirectory, bool backSlash) //No refs lol, C++ only | 1245-1256
{
	if (!theDirectory.empty())
	{
		wchar_t aChar = theDirectory[theDirectory.length() - 1];
		if (aChar != '\\' && aChar != '/')
			return theDirectory + (backSlash? L'\\': L'/');
		else
			return theDirectory;
	}
	else
		return L"";
}

time_t Sexy::GetFileDate(const std::string& theFileName) //1260-1262
{
	return gFileDriver->GetFileTime(theFileName); //MOVED TO *FILEDRIVER
}

time_t Sexy::GetFileDate(const std::wstring& theFileName) //No refs lol, C++ only | 1265-1267
{
	return gFileDriver->GetFileTime(theFileName); //MOVED TO *FILEDRIVER
}

std::string Sexy::vformat(const char* fmt, va_list argPtr) //1270-1335
{
    // We draw the line at a 1MB string.
    const int maxSize = 1000000;

    // If the string is less than 161 characters,
    // allocate it on the stack because this saves
    // the malloc/free time.
    const int bufSize = 161;
	char stackBuffer[bufSize];

    int attemptedSize = bufSize - 1;
#ifndef _TRANSMENSION
	if (!gXprintfInitialized) //Nice, a custom library, fun, present on Mobile
		InitXprintf();
#endif

	int numChars = 0;
#ifndef _TRANSMENSION //All plats aside from Transmension uses xprintf
	numChars = xvsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#else
	numChars = _vsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#endif

	//cout << "NumChars: " << numChars << endl;

    if ((numChars >= 0) && (numChars <= attemptedSize)) 
	{
		// Needed for case of 160-character printf thing
		stackBuffer[numChars] = '\0';

        // Got it on the first try.
		return std::string(stackBuffer);
    }

    // Now use the heap.
    char* heapBuffer = NULL;

    while (((numChars == -1) || (numChars > attemptedSize)) && 
		(attemptedSize < maxSize)) 
	{
        // Try a bigger size
        attemptedSize *= 2;
		heapBuffer = (char*)realloc(heapBuffer, (attemptedSize + 1));
#ifdef WIN32
		numChars = _vsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#else
		numChars = xvsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#endif
    }

	heapBuffer[numChars] = 0;

	std::string result = std::string(heapBuffer);

    free(heapBuffer);
    return result;
}

//overloaded StrFormat: should only be used by the xml strings
std::string Sexy::StrFormat(const char* fmt ...) //1339-1346
{
    va_list argList;
    va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
    va_end(argList);

    return result;
}

std::wstring Sexy::vformat(const wchar_t* fmt, va_list argPtr) //1349-1415
{
    // We draw the line at a 1MB string.
    const int maxSize = 1000000;

    // If the string is less than 161 characters,
    // allocate it on the stack because this saves
    // the malloc/free time.
    const int bufSize = 161;
	wchar_t stackBuffer[bufSize];

    int attemptedSize = bufSize - 1;
#ifndef _TRANSMENSION
	if (gXprintfInitialized) //Nice, a custom library, fun
		InitXprintf();
#endif

	int numChars = 0;
#ifndef _TRANSMENSION
	numChars = wxvsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#else
	numChars = vsnwprintf(stackBuffer, attemptedSize, fmt, argPtr);
#endif

	//cout << "NumChars: " << numChars << endl;

    if ((numChars >= 0) && (numChars <= attemptedSize)) 
	{
		// Needed for case of 160-character printf thing
		stackBuffer[numChars] = '\0';

        // Got it on the first try.
		return std::wstring(stackBuffer);
    }

    // Now use the heap.
	wchar_t* heapBuffer = NULL;

    while (((numChars == -1) || (numChars > attemptedSize)) && 
		(attemptedSize < maxSize)) 
	{
        // Try a bigger size
        attemptedSize *= 2;
		heapBuffer = (wchar_t*)realloc(heapBuffer, (attemptedSize + 1));
#ifdef _WIN32 //On Win, do the builtin one, else XPRINTF
		numChars = _vsnwprintf(heapBuffer, attemptedSize, fmt, argPtr);
#else
		numChars = wxvsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#endif
    }

	heapBuffer[numChars] = 0;

	std::wstring result = std::wstring(heapBuffer);

    free(heapBuffer);

    return result;
}

//overloaded StrFormat: should only be used by the xml strings
std::wstring Sexy::StrFormat(const wchar_t* fmt ...) //1419-1426
{
    va_list argList;
    va_start(argList, fmt);
	std::wstring result = vformat(fmt, argList);
    va_end(argList);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Sexy::OutputDebugStrF(const char* fmt ...) //Changed to OutputDebugStrF | 1429-1448
{
	va_list argList;
	va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
	va_end(argList);

	OutputDebugStringA(result.c_str()); //Would be OutputDebugString for simplicity I'd rather look similar to the below
}

void Sexy::OutputDebugStrF(const wchar_t* fmt ...) //Changed to OutputDebugStrF | 1451-1464
{
	va_list argList;
	va_start(argList, fmt);
	std::wstring result = vformat(fmt, argList);
	va_end(argList);

	OutputDebugStringW(result.c_str());
}

std::string Sexy::Evaluate(const std::string& theString, const DefinesMap& theDefinesMap) //I think this is ok, in PopLoc in XNA? | 1467-1495
{
	std::string anEvaluatedString = theString;

	for (;;)
	{
		int aPercentPos = anEvaluatedString.find('%');

		if (aPercentPos == std::string::npos)
			break;
		
		int aSecondPercentPos = anEvaluatedString.find('%', aPercentPos + 1);
		if (aSecondPercentPos == std::string::npos)
			break;

		std::string aName = anEvaluatedString.substr(aPercentPos + 1, aSecondPercentPos - aPercentPos - 1);

		std::string aValue;
		DefinesMap::const_iterator anItr = theDefinesMap.find(aName);		
		if (anItr != theDefinesMap.end())
			aValue = anItr->second;
		else
			aValue = "";		

		anEvaluatedString.erase(anEvaluatedString.begin() + aPercentPos, anEvaluatedString.begin() + aSecondPercentPos + 1);
		anEvaluatedString.insert(anEvaluatedString.begin() + aPercentPos, aValue.begin(), aValue.begin() + aValue.length());
	}

	return anEvaluatedString;
}

std::string Sexy::XMLDecodeString(const std::string& theString) //1498-1544
{
	std::string aNewString;

	for (ulong i = 0; i < theString.length(); i++)
	{
		char c = theString[i];

		if (c == '&')
		{
			int aSemiPos = theString.find(';', i);

			if (aSemiPos != -1)
			{
				std::string anEntName = theString.substr(i+1, aSemiPos-i-1);
				i = aSemiPos;
											
				if (anEntName == "lt") //Maybe use a switch statement next time
					c = '<';
				else if (anEntName == "amp")
					c = '&';
				else if (anEntName == "gt")
					c = '>';
				else if (anEntName == "quot")
					c = '"';
				else if (anEntName == "apos")
					c = '\'';
				else if (anEntName == "nbsp")
					c = ' ';
				else if (anEntName == "cr")
					c = '\n';
				else if (anEntName[0] == '#' && anEntName.length() > 1)
				{
					int aCode = c;
					if (anEntName[1] == 'x')
						StringToInt("0x" + anEntName.substr(2), &aCode);
					else
						StringToInt(anEntName.substr(1), &aCode);
					c = aCode;
				}
			}
		}				
		
		aNewString += c;
	}

	return aNewString;
}

std::wstring Sexy::XMLDecodeString(const std::wstring& theString) //1547-1593
{
	std::wstring aNewString;

	int aUTF8Len = 0;
	int aUTF8CurVal = 0;

	for (ulong i = 0; i < theString.length(); i++)
	{
		wchar_t c = theString[i];

		if (c == L'&')
		{
			int aSemiPos = theString.find(L';', i);

			if (aSemiPos != -1)
			{
				std::wstring anEntName = theString.substr(i+1, aSemiPos-i-1);
				i = aSemiPos;
											
				if (anEntName == L"lt") //Maybe use a switch statement next time
					c = L'<';
				else if (anEntName == L"amp")
					c = L'&';
				else if (anEntName == L"gt")
					c = L'>';
				else if (anEntName == L"quot")
					c = L'"';
				else if (anEntName == L"apos")
					c = L'\'';
				else if (anEntName == L"nbsp")
					c = L' ';
				else if (anEntName == L"cr")
					c = L'\n';
				else if (anEntName[0] == '#' && anEntName.length() > 1)
				{
					int aCode = c;
					if (anEntName[1] == 'x')
						StringToInt(L"0x" + anEntName.substr(2), &aCode);
					else
						StringToInt(anEntName.substr(1), &aCode);
					c = aCode;
				}
			}
		}				
		
		aNewString += c;
	}

	return aNewString;
}

std::string Sexy::XMLEncodeString(const std::string& theString) //1597-1655
{
	std::string aNewString;

	bool hasSpace = false;

	for (ulong i = 0; i < theString.length(); i++)
	{
		char c = theString[i];

		if (c == ' ')
		{
			if (hasSpace)
			{
				aNewString += "&nbsp;";
				continue;
			}
			
			hasSpace = true;
		}
		else
			hasSpace = false;

		/*if ((uchar) c >= 0x80)
		{
			// Convert to UTF
			aNewString += (char) (0xC0 | ((c >> 6) & 0xFF));
			aNewString += (char) (0x80 | (c & 0x3F));
		}
		else*/
		{		
			switch (c)
			{
			case '<':
				aNewString += "&lt;";
				break;
			case '&':		
				aNewString += "&amp;";
				break;
			case '>':
				aNewString += "&gt;";
				break;
			case '"':
				aNewString += "&quot;";
				break;
			case '\'':
				aNewString += "&apos;";
				break;
			case '\n':
				aNewString += "&cr;";
				break;
			default:
				aNewString += c;
				break;
			}
		}
	}

	return aNewString;
}

std::wstring Sexy::XMLEncodeString(const std::wstring& theString) //1658-1716
{
	std::wstring aNewString;

	bool hasSpace = false;

	for (ulong i = 0; i < theString.length(); i++)
	{
		wchar_t c = theString[i];

		if (c == ' ')
		{
			if (hasSpace)
			{
				aNewString += L"&nbsp;";
				continue;
			}
			
			hasSpace = true;
		}
		else
			hasSpace = false;

		/*if ((uchar) c >= 0x80)
		{
			// Convert to UTF
			aNewString += (char) (0xC0 | ((c >> 6) & 0xFF));
			aNewString += (char) (0x80 | (c & 0x3F));
		}
		else*/
		{		
			switch (c)
			{
			case L'<':
				aNewString += L"&lt;";
				break;
			case L'&':		
				aNewString += L"&amp;";
				break;
			case L'>':
				aNewString += L"&gt;";
				break;
			case L'"':
				aNewString += L"&quot;";
				break;
			case L'\'':
				aNewString += L"&apos;";
				break;
			case L'\n':
				aNewString += L"&cr;";
				break;
			default:
				aNewString += c;
				break;
			}
		}
	}

	return aNewString;
}

#ifndef _TRANSMENSION
bool Sexy::WriteUTF8XMLFile(FILE* fp, const std::wstring& theData, bool includePreamble) //C++ only, Correct?, no clue where this is used, maybe it was used to generate resources, definitely not in Transmension or Bej3 iOS | 1719-1772
{	
	//How do we write this cleanly
	//if (fwrite((void*)'\xEF', 1, 1, fp) != 1)
	if (fwrite("ï", 1, 1, fp) != 1)
		return false;
	//if (fwrite((void*)'\xBB', 1, 1, fp) != 1)
	if (fwrite("»", 1, 1, fp) != 1)
		return false;
	//if (fwrite((void*)'\xBF', 1, 1, fp) != 1)
	if (fwrite("¿", 1, 1, fp) != 1)
		return false;
	if (includePreamble)
	{
		std::string aPreamble = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
		if (fwrite(aPreamble.c_str(), 1, aPreamble.length(), fp) != aPreamble.length())
			return false;
	}
	int i = 0;
	while (i <= theData.length())
	{
		const uint c = theData[i];
		if (c >= 0x80)
		{
			if (c >= 0x800)
			{
				uchar b1 = (c >> 12) | 0xE0;
				uchar b2 = (c >> 6) & 0x3F | 0x80;
				uchar b3 = c & 0x3F | 0x80;
				if (fwrite(&b1, 1, 1, fp) != 1)
					return false;
				if (fwrite(&b2, 1, 1, fp) != 1)
					return false;
				if (fwrite(&b3, 1, 1, fp) != 1)
					return false;
			}
			else
			{
				uchar b1 = (c >> 6) | 0xC0;
				uchar b2 = c & 0x3F | 0x80;
				if (fwrite(&b1, 1, 1, fp) != 1)
					return false;
				if (fwrite(&b2, 1, 1, fp) != 1)
					return false;
			}
		}
		else
		{
			uchar b = c;
			if (fwrite(&b, 1, 1, fp) != 1)
				return false;
		}
		i++;
	}
	fflush(fp);
	return true;
}
#endif

std::string Sexy::Upper(const std::string& _data) //1775-1779
{
	std::string s = _data;
	std::transform(s.begin(), s.end(), s.begin(), toupper);
	return s;
}

std::wstring Sexy::Upper(const std::wstring& _data) //1782-1786
{
	std::wstring s = _data;
	std::transform(s.begin(), s.end(), s.begin(), towupper);
	return s;
}

std::string Sexy::Lower(const std::string& _data) //1789-1793
{
	std::string s = _data;
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

std::wstring Sexy::Lower(const std::wstring& _data) //1796-1800
{
	std::wstring s = _data;
	std::transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int Sexy::StrFindNoCase(const char *theStr, const char *theFind) //1805-1828
{
	int p1,p2;
	int cp = 0;
	const int len1 = (int)strlen(theStr);
	const int len2 = (int)strlen(theFind);
	while(cp < len1)
	{
		p1 = cp;
		p2 = 0;
		while(p1<len1 && p2<len2)
		{
			if(tolower(theStr[p1])!=tolower(theFind[p2]))
				break;

			p1++; p2++;
		}
		if(p2==len2)
			return p1-len2;

		cp++;
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool Sexy::StrPrefixNoCase(const char *theStr, const char *thePrefix, int maxLength) //Not in iOS? | 1833-1849
{
	int i;
	char c1 = 0, c2 = 0;
	for (i=0; i<maxLength; i++)
	{
		c1 = tolower(*theStr++);
		c2 = tolower(*thePrefix++);

		if (c1==0 || c2==0)
			break;

		if (c1!=c2)
			return false;
	}

	return c2==0 || i==maxLength;
}


#ifndef _TRANSMENSION
void Sexy::InitXprintf() //1854-1944
{
	AutoCrit aCrit(gXprintfInitCrit);
	gXprintfInitialized = true;
	struct Local
	{
		static void Vec3Handler(TBufferWriter<char> *inWriter, const char* inType, char*& ioVarArgs, char* inParams) //1876-1900
		{
			char fmtBuf[256];
			strcpy(fmtBuf, "%");
			strcat(fmtBuf, inParams);
			strcat(fmtBuf, "f");
			char fmtBuf2[MAX_PATH];
			sprintf(fmtBuf2, "(%s, %s, %s)", fmtBuf, fmtBuf, fmtBuf);
			if (stricmp(inType, "vec3") == 0)
			{
				ioVarArgs += 8;
				double x = (*ioVarArgs - 1); //?
				ioVarArgs += 8;
				double y = (*ioVarArgs - 1); //?
				ioVarArgs += 8;
				double z = (*ioVarArgs - 1); //?
				char buf[256];
				sprintf(buf, fmtBuf2, x, y, z);
				inWriter->BufferWriteString(buf, 0);
			}
			else if (stricmp(inType, "pvec3") == 0)
			{
				float x = (*ioVarArgs - 1); //?
				char buf[256];
				sprintf(buf, fmtBuf2, x, x, x);
				inWriter->BufferWriteString(buf, 0);
			}
		}
		static void ChooseHandlerA(TBufferWriter<char>* inWriter, const char* inType, char*& ioVarArgs, char* inParams) //1902-1916
		{
			*ioVarArgs += 4;
			uint choice = *(ioVarArgs - 1); //?
			if (stricmp(inType, "if") == 0)
				choice = choice == 0;
			char* choiceStr = inParams;
			char* pipePtr;
			for (pipePtr = strchr(inParams, 124); ; pipePtr = strchr(pipePtr + 1, 124))
			{
				if (!choice-- || !pipePtr)
					break;
				choiceStr = pipePtr + 1;
			}
			if (pipePtr != NULL)
				pipePtr = 0;
			inWriter->BufferWriteString(choiceStr, 0);
		}
		static void ChooseHandlerW(TBufferWriter<wchar_t>* inWriter, const wchar_t* inType, char*& ioVarArgs, wchar_t* inParams) //1902-1916
		{
			*ioVarArgs += 4;
			uint choice = *(ioVarArgs - 1); //?
			if (wcsicmp(inType, L"if") == 0)
				choice = choice == 0;
			wchar_t* choiceStr = inParams;
			wchar_t* pipePtr;
			for (pipePtr = wcschr(inParams, 124); ; pipePtr = wcschr(pipePtr + 1, 124))
			{
				if (!choice-- || !pipePtr)
					break;
				choiceStr = pipePtr + 1;
			}
			if (pipePtr != NULL)
				pipePtr = 0;
			inWriter->BufferWriteString(choiceStr, 0);
		}
	};
#ifdef WIN32
	xprintf::xsnprintf(0, 0, "%{register:vec3}", Local::Vec3Handler, 12);
#endif
	xprintf::xsnprintf(0, 0, "%{register:pvec3}", Local::Vec3Handler, 4);
	xprintf::xsnprintf(0, 0, "%{register:switch}", Local::ChooseHandlerA, 4);
	xprintf::wxsnprintf(0, 0, _S("%{register:switch}"), Local::ChooseHandlerW, 4); //According to mobile, this is SexyString
	xprintf::xsnprintf(0, 0, "%{register:if}", Local::ChooseHandlerA, 4);
	xprintf::wxsnprintf(0, 0, _S("%{register:if}"), Local::ChooseHandlerW, 4);
}
#endif