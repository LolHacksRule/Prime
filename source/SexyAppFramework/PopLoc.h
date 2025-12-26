#ifndef __POPLOC_H
#define __POPLOC_H

#include "Common.h"

namespace Sexy
{
	class PopLoc //Not in BejC
	{
        typedef std::map<int, std::wstring> IdToStringMap;
        typedef std::map<std::wstring, std::wstring> NameToStringMap;
	private:
        IdToStringMap mIdStrings;
        NameToStringMap mNameStrings;
	public:
        PopLoc();
        ~PopLoc();
        std::wstring GetString(const int theId, const std::wstring& theDefaultString);
        std::wstring GetString(const std::wstring& theName, const std::wstring& theDefaultString);
        bool SetString(const int theId, const std::wstring& theString, bool reset);
        bool SetString(const std::wstring& theName, const std::wstring& theString, bool reset);
        bool RemoveString(const int theId);
        bool RemoveString(const std::wstring& theName);
        std::wstring Evaluate(const std::wstring& theInput); //Is this SexyString?
	};
}
#endif //__POPLOC_H