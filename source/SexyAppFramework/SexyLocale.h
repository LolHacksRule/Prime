#ifndef __SEXYLOCALE_H__
#define __SEXYLOCALE_H__

#include "Common.h"
#include <locale>

namespace Sexy
{
	class Locale
	{
	public:
		void SetLocale(const SexyString& theLocale); //WIP
		std::string StringToUpper(const std::string& theString); //WIP
		std::wstring StringToUpper(const std::wstring& theString); //WIP
		std::string  StringToLower(const std::string& theString); //WIP
		std::wstring StringToLower(const std::wstring& theString); //WIP
		bool isalnum(const char theChar); //WIP
		bool isalnum(const wchar_t theChar); //WIP
		SexyString CommaSeparate(int theValue); //Check?
		SexyString UCommaSeparate(uint theValue); //WIP
		static std::string gLocaleString;
		static std::locale gLocaleObject;
		static std::numpunct<wchar_t> const* gNumPunct;
		static std::ctype<char> const* gCharCtype;
		static std::ctype<wchar_t> const* gWCharCtype;
		static wchar_t gThousandSep;
		static std::string gGrouping;
	};
}

#endif