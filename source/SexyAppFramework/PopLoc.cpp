#include "PopLoc.h"

using namespace Sexy;

PopLoc::PopLoc() //8-9
{
}

PopLoc::~PopLoc() //14-15
{
}

std::wstring PopLoc::GetString(const int theId, const std::wstring& theDefaultString) //id and strDefault in XNA | 20-26
{
	IdToStringMap::const_iterator itr = mIdStrings.find(theId); //?
	if (itr == mIdStrings.end())
		return theDefaultString;
	else
		return itr->second;
}

std::wstring PopLoc::GetString(const std::wstring& theName, const std::wstring& theDefaultString) //name and strDefault in XNA | 31-37
{
	NameToStringMap::const_iterator itr = mNameStrings.find(StringToUpper(theName)); //?
	if (itr == mNameStrings.end())
		return theDefaultString;
	else
		return itr->second;
}

bool PopLoc::SetString(const int theId, const std::wstring& theString, bool reset) //id and str in XNA, TODO | 42-50
{
	if (reset)
	{
		mIdStrings[theId] = theString;
		return true;
	}
	else
	{
		return &mIdStrings.insert(IdToStringMap::value_type(theId, theString)); //?
	}
}

bool PopLoc::SetString(const std::wstring& theName, const std::wstring& theString, bool reset) //id and str in XNA, TODO | 55-63
{
	if (reset)
	{
		mNameStrings[StringToUpper(theName)] = theString;
		return true;
	}
	else
		return &mNameStrings.insert(NameToStringMap::value_type(theName, theString));
}

bool PopLoc::RemoveString(const int theId) //? | 68-76
{
	IdToStringMap::iterator itr = mIdStrings.find(theId); //?
	if (itr != mIdStrings.end())
	{
		return false;
	}
	mIdStrings.erase(itr); //?
	return true;
}

bool PopLoc::RemoveString(const std::wstring& theName) //81-89
{
	NameToStringMap::iterator itr = mNameStrings.find(StringToUpper(theName));
	if (itr != mNameStrings.end())
	{
		return false;
	}
	mNameStrings.erase(itr); //?
	return true;
}

std::wstring PopLoc::Evaluate(const std::wstring& theInput) //Correct? | 94-138
{
	std::wstring anEvalText = theInput;
	uint anIndex;
	do
	{
		uint aPercentPos = theInput.find('%');
		if (aPercentPos == std::wstring::npos)
			break;
		uint aPercent2Pos = theInput.find('%', aPercentPos + 1);
		if (aPercent2Pos == std::wstring::npos)
			break;
		if (aPercent2Pos == aPercentPos + 1)
		{
			anEvalText.erase(aPercent2Pos);
			anIndex = aPercent2Pos;
		}
		else
		{
			std::wstring aVariableText = anEvalText.substr(aPercentPos + 1, aPercent2Pos - (aPercentPos + 1));
			int anIntValue = 0;
			if (!StringToInt(aVariableText, &anIntValue))
			{
				std::wstring aReplaceString = GetString(anIntValue, L"");
				anEvalText.replace(aPercentPos, aPercent2Pos - aPercentPos + 1, aReplaceString);
				anIndex = aPercentPos;
			}
			else
			{
				std::wstring aReplaceString = GetString(anIntValue, GetString(aVariableText, L""));
				anEvalText.replace(aPercentPos, aPercent2Pos - aPercentPos + 1, aReplaceString);
				anIndex = aPercentPos;
			}
		}
	}
	while (anIndex < anEvalText.length());
	return anEvalText;
}