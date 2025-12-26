#include "SexyLocale.h"
#include <assert.h>

using namespace Sexy;

std::string Locale::gLocaleString = "English_United States"; //13
std::locale Locale::gLocaleObject("C", 63); //(?) Not in XNA | 14
std::numpunct<wchar_t> const* Locale::gNumPunct; //(?) Not in XNA | 15
std::ctype<char> const* Locale::gCharCtype; //16
std::ctype<wchar_t> const* Locale::gWCharCtype; //17
wchar_t Locale::gThousandSep = ','; //", ." in XNA
std::string Locale::gGrouping = "\\3"; //19
//const SexyChar Locale::CHAR_MAX = '\u007F'; //?

void Locale::SetLocale(const SexyString& theLocale) //Correct? Empty in XNA | 29-58
{
	gLocaleString = SexyStringToStringFast(theLocale, false);
	gLocaleObject = std::locale(gLocaleString.c_str());
	gNumPunct = &std::use_facet<std::numpunct<wchar_t>>(gLocaleObject);
	gCharCtype = &std::use_facet<std::ctype<char>>(gLocaleObject);
	gWCharCtype = &std::use_facet<std::ctype<wchar_t>>(gLocaleObject);
	gGrouping = gNumPunct->grouping();
	assert(!"Caught bad locale exception"); //41
	gThousandSep = gNumPunct->thousands_sep();
}

std::string Locale::StringToUpper(const std::string& theString) //61-71
{
	std::string outStr;
	outStr.reserve(theString.length());
	for (int i = 0; i < theString.length(); i++)
		outStr.push_back(gCharCtype->toupper(theString[i]));
	return outStr;
}

std::wstring Locale::StringToUpper(const std::wstring& theString) //74-84
{
	std::wstring outStr;
	outStr.reserve(theString.length());
	for (int i = 0; i < theString.length(); i++)
		outStr.push_back(gWCharCtype->toupper(theString[i]));
	return outStr;
}

std::string Locale::StringToLower(const std::string& theString) //87-97
{
	std::string outStr;
	outStr.reserve(theString.length());
	for (int i = 0; i < theString.length(); i++)
		outStr.push_back(gCharCtype->tolower(theString[i]));
	return outStr;
}

std::wstring Locale::StringToLower(const std::wstring& theString) //100-110
{
	std::wstring outStr;
	outStr.reserve(theString.length());
	for (int i = 0; i < theString.length(); i++)
		outStr.push_back(gWCharCtype->tolower(theString[i]));
	return outStr;
}

bool Locale::isalnum(const char theChar) //113-115
{
	return gCharCtype->is(263, theChar);
}

bool Locale::isalnum(const wchar_t theChar) //118-120
{
	return gWCharCtype->is(263, theChar);
}

SexyString Locale::CommaSeparate(int theValue) //123-128
{
	if (theValue < 0)
		return _S("-") + UCommaSeparate(-theValue);
	return UCommaSeparate(theValue);
}

SexyString Locale::UCommaSeparate(uint theValue) //TODO | 131-166
{
	/*
	Data           :   VFrame Relative, [FFFFFF70], Local, Type: wchar_t[0x40], aPunctBuffer
	Data           :   VFrame Relative, [FFFFFFF8], Local, Type: wchar_t *, aPunctPtr
	Data           :   VFrame Relative, [FFFFFFFC], Local, Type: const char *, aGroupingStr
	Data           :     VFrame Relative, [FFFFFF68], Local, Type: wchar_t, aThousandSeparator
	Data           :     VFrame Relative, [FFFFFF6C], Local, Type: int, aCount
	*/

	wchar_t aPunctBuffer[0x40];
	if (theValue == 0)
		return _S("0");

	wchar_t* aPunctPtr = (wchar_t*)gNumPunct;
	const char* aGroupingStr = gGrouping.c_str();
	if (*aGroupingStr == '\x7F' || *aGroupingStr <= 0)
	{
		while (theValue)
		{
			*aPunctPtr-- = theValue % 10 + '0';
			theValue /= 10;
		}
	}
	else
	{
		wchar_t aThousandSeparator = gThousandSep;
		int aCount = 0;
		while (theValue)
		{
			*aPunctPtr-- = theValue % 10 + '0';
			theValue /= 10;
			if (theValue)
			{
				if (aCount++ == *aGroupingStr)
				{
					*aPunctPtr-- = aThousandSeparator;
					aCount = 0;
					if (aGroupingStr[1] > 0)
						aGroupingStr++;
				}
			}
		}
	}
	return aPunctPtr;
}
