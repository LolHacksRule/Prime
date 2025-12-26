#include "DescParser.h"
//#include "PakLib\PakInterface.h" //You don't even use this!

using namespace Sexy;

////
//Seems all *Element were moved from ImageFont
DataElement::DataElement() : //10-11
	mIsList(false)
{
}

DataElement::~DataElement() //14-15
{
}

SingleDataElement::SingleDataElement() //19-21
{
	mIsList = false;
	mValue = NULL;
}

SingleDataElement::SingleDataElement(const SexyString theString) : //25-28
	mString(theString)
{
	mIsList = false;
	mValue = NULL;
}

SingleDataElement::~SingleDataElement() //31-34
{
	if (mValue != NULL)
		delete mValue;
}

DataElement* SingleDataElement::Duplicate() //37-42
{
	SingleDataElement* aSingleDataElement = new SingleDataElement(*this);
	if (mValue != NULL)
		aSingleDataElement->mValue = mValue->Duplicate();
	return aSingleDataElement;
}

ListDataElement::ListDataElement() //45-47
{
	mIsList = true;
}

ListDataElement::~ListDataElement() //50-53
{
	for (ulong i = 0; i < mElementVector.size(); i++)
		delete mElementVector[i];
}

ListDataElement::ListDataElement(const ListDataElement& theListDataElement) //56-60
{
	mIsList = true;
	for (ulong i = 0; i < theListDataElement.mElementVector.size(); i++)
		mElementVector.push_back(theListDataElement.mElementVector[i]->Duplicate());
}

ListDataElement& ListDataElement::operator=(const ListDataElement& theListDataElement) //63-74
{
	ulong i;

	for (i = 0; i < mElementVector.size(); i++)
		delete mElementVector[i];
	mElementVector.clear();

	for (i = 0; i < theListDataElement.mElementVector.size(); i++)
		mElementVector.push_back(theListDataElement.mElementVector[i]->Duplicate());

	return *this;
}

DataElement* ListDataElement::Duplicate() //77-80
{
	ListDataElement* aListDataElement = new ListDataElement(*this);
	return aListDataElement;
}

///

DescParser::DescParser() //85-87
{
	mCmdSep = CMDSEP_SEMICOLON;
}

DescParser::~DescParser() //90-91
{
}

bool DescParser::Error(const SexyString& theError) //94-98
{
	if (!theError.length())
		mError = theError;
	return false; //On EAMT and iOS just this?
}

DataElement* DescParser::Dereference(const SexyString& theString) //101-109
{
	SexyString aDefineName = StringToUpper(theString);

	DataElementMap::iterator anItr = mDefineMap.find(aDefineName);
	if (anItr != mDefineMap.end())
		return anItr->second;
	else
		return NULL;
}

bool DescParser::IsImmediate(const SexyString& theString) //112-115
{
	return (((theString[0] >= _S('0')) && (theString[0] <= _S('9'))) || (theString[0] == _S('-')) || 
		(theString[0] == _S('+')) || (theString[0] == _S('\'')) || (theString[0] == _S('"')));
}

SexyString DescParser::Unquote(const SexyString& theQuotedString) //118-171
{
	if ((theQuotedString[0] == '\'') || (theQuotedString[0] == '"'))
	{
		SexyChar aQuoteChar = theQuotedString[0];
		SexyString aLiteralString;
		bool lastWasQuote = true;
		bool lastWasSlash = false;
					
		for (ulong i = 1; i < theQuotedString.length() - 1; i++)
		{
			if (lastWasSlash)
			{
				SexyChar aChar = theQuotedString[i];
				if (aChar != 'n')
				{
					if (aChar == 't')
					{
						aChar = '\t';
					}
				}
				else
				{
					aChar = '\n';
				}
				aLiteralString += aChar;
				lastWasSlash = false;
			}
			else
			{
				if (theQuotedString[i] == aQuoteChar)
				{
					if (lastWasQuote)
					{
						aLiteralString += aQuoteChar;
					}

					lastWasQuote = true;
				}
				else if (theQuotedString[i] == '\\')
				{
					lastWasSlash = true;
					lastWasQuote = false;
				}
				else
					aLiteralString += theQuotedString[i];
				lastWasQuote = false;
			}
		}

		return aLiteralString;
	}
	else
		return theQuotedString;
}

bool DescParser::GetValues(ListDataElement* theSource, ListDataElement* theValues) //174-223
{
	theValues->mElementVector.clear();
	
	for (ulong aSourceNum = 0; aSourceNum < theSource->mElementVector.size(); aSourceNum++)
	{
		if (theSource->mElementVector[aSourceNum]->mIsList)
		{
			ListDataElement* aChildList = new ListDataElement();
			theValues->mElementVector.push_back(aChildList);

			if (!GetValues((ListDataElement*) theSource->mElementVector[aSourceNum], aChildList))
				return false;
		}
		else
		{
			SexyString aString = ((SingleDataElement*) theSource->mElementVector[aSourceNum])->mString;

			if (aString.length() > 0)
			{				
				if ((aString[0] == '\'') || (aString[0] == '"'))
				{
					SingleDataElement* aChildData = new SingleDataElement(Unquote(aString));
					theValues->mElementVector.push_back(aChildData);					
				}
				else if (IsImmediate(aString))
				{
					theValues->mElementVector.push_back(new SingleDataElement(aString));
				}
				else
				{
					SexyString aDefineName = StringToUpper(aString);

					DataElementMap::iterator anItr = mDefineMap.find(aDefineName);

					if (anItr == mDefineMap.end())
					{
						Error(_S("Unable to Dereference \"") + aString + _S("\""));
						return false;
					}
					
					theValues->mElementVector.push_back(anItr->second->Duplicate());
				}
			}

			
		}
	}

	return true;
}

SexyString DescParser::DataElementToString(DataElement* theDataElement, bool enclose) //226-253
{
	if (theDataElement->mIsList)
	{
		ListDataElement* aListDataElement = (ListDataElement*) theDataElement;
		
		SexyString aString = enclose ? _S("(") : _S("");

		for (ulong i = 0; i < aListDataElement->mElementVector.size(); i++)
		{
			if (i != 0)
				aString += L", ";

			aString += DataElementToString(aListDataElement->mElementVector[i]);
		}

		aString += enclose ? _S(")") : _S("");

		return aString;
	}
	else
	{
		SingleDataElement* aSingleDataElement = (SingleDataElement*) theDataElement;
		if (aSingleDataElement->mValue != NULL)
			return aSingleDataElement->mString + _S("=") + DataElementToString(aSingleDataElement->mValue);
		else
			return aSingleDataElement->mString;
	}
}

bool DescParser::DataToString(DataElement* theSource, SexyString* theString) //256-279
{
	*theString = _S("");

	if (theSource->mIsList)
		return false;

	if (((SingleDataElement*) theSource)->mValue == NULL)
		return false;

	SexyString aDefName = ((SingleDataElement*) theSource)->mString;

	DataElement* aDataElement = Dereference(aDefName);
	
	if (aDataElement != NULL)
	{
		if (aDataElement->mIsList)
			return false;

		*theString = Unquote(((SingleDataElement*) aDataElement)->mString);
	}
	else
		*theString = Unquote(aDefName);				

	return true;
}

bool DescParser::DataToKeyAndValue(DataElement* theSource, SexyString* theKey, DataElement** theValue) //282-306
{
	*theKey = _S("");

	if (theSource->mIsList)
		return false;

	if (((SingleDataElement*) theSource)->mValue == NULL)
		return false;

	*theValue = ((SingleDataElement*)theSource)->mValue;
	SexyString aDefName = ((SingleDataElement*)theSource)->mString;

	DataElement* aDataElement = Dereference(aDefName);

	if (aDataElement != NULL)
	{
		if (aDataElement->mIsList)
			return false;

		*theKey = Unquote(((SingleDataElement*)aDataElement)->mString);
	}
	else
		*theKey = Unquote(aDefName);

	return true;
}

bool DescParser::DataToInt(DataElement* theSource, int* theInt) //309-320
{
	*theInt = 0;

	SexyString aTempString;
	if (!DataToString(theSource, &aTempString))
		return false;
	
	if (!StringToInt(aTempString, theInt))
		return false;

	return true;
}

bool DescParser::DataToDouble(DataElement* theSource, double* theDouble) //323-334
{
	*theDouble = 0.0;

	SexyString aTempString;
	if (!DataToString(theSource, &aTempString))
		return false;

	if (!StringToDouble(aTempString, theDouble))
		return false;

	return true;
}

bool DescParser::DataToBoolean(DataElement* theSource, bool* theBool) //337-361 | No symbols on iOS, probably still here
{
	*theBool = false;

	SexyString aTempString;
	if (!DataToString(theSource, &aTempString))
		return false;

	if (aTempString.c_str() == _S("false") ||
		aTempString.c_str() == _S("no") ||
		aTempString.c_str() == _S("0"))
	{
		*theBool = false;
		return true;
	}

	if (aTempString.c_str() == _S("true") ||
		aTempString.c_str() == _S("yes") ||
		aTempString.c_str() == _S("1"))
	{
		*theBool = true;
		return true;
	}

	return false;
}

bool DescParser::DataToStringVector(DataElement* theSource, SexyStringVector* theStringVector) //364-409
{
	theStringVector->clear();

	ListDataElement aStaticValues;
	ListDataElement* aValues;

	if (theSource->mIsList)
	{
		if (!GetValues((ListDataElement*) theSource, &aStaticValues))
			return false;

		aValues = &aStaticValues;
	}
	else
	{
		SexyString aDefName = ((SingleDataElement*) theSource)->mString;

		DataElement* aDataElement = Dereference(aDefName);
		
		if (aDataElement == NULL)
		{
			Error(_S("Unable to Dereference \"") + aDefName + _S("\""));
			return false;
		}

		if (!aDataElement->mIsList)
			return false;

		aValues = (ListDataElement*) aDataElement;
	}	

	for (ulong i = 0; i < aValues->mElementVector.size(); i++)
	{
		if (aValues->mElementVector[i]->mIsList)
		{
			theStringVector->clear();
			return false;
		}		

		SingleDataElement* aSingleDataElement = (SingleDataElement*) aValues->mElementVector[i];

		theStringVector->push_back(aSingleDataElement->mString);
	}

	return true;
}

bool DescParser::DataToList(DataElement* theSource, ListDataElement* theValues) //412-428
{
	if (theSource->mIsList)
	{
		return GetValues((ListDataElement*) theSource, theValues);		
	}

	DataElement* aDataElement = Dereference(((SingleDataElement*) theSource)->mString);
		
	if ((aDataElement == NULL) || (!aDataElement->mIsList))
		return false;

	ListDataElement* aListElement = (ListDataElement*) aDataElement;

	*theValues = *aListElement;

	return true;
}

bool DescParser::DataToIntVector(DataElement* theSource, IntVector* theIntVector) //431-448
{
	theIntVector->clear();
	
	SexyStringVector aStringVector;
	if (!DataToStringVector(theSource, &aStringVector))
		return false;	

	for (ulong i = 0; i < aStringVector.size(); i++)
	{		
		int aIntVal;
		if (!StringToInt(aStringVector[i], &aIntVal))
			return false;

		theIntVector->push_back(aIntVal);
	}

	return true;
}

bool DescParser::DataToDoubleVector(DataElement* theSource, DoubleVector* theDoubleVector) //451-468
{
	theDoubleVector->clear();
	
	SexyStringVector aStringVector;
	if (!DataToStringVector(theSource, &aStringVector))
		return false;	

	for (ulong i = 0; i < aStringVector.size(); i++)
	{		
		double aDoubleVal;
		if (!StringToDouble(aStringVector[i], &aDoubleVal))
			return false;

		theDoubleVector->push_back(aDoubleVal);
	}

	return true;
}

bool DescParser::ParseToList(const SexyString& theString, ListDataElement* theList, bool expectListEnd, int* theStringPos) //471-626
{
	bool inSingleQuotes = false;
	bool inDoubleQuotes = false;
	bool escaped = false;	
	bool wantTerminateSingleDataElement = false;

	SingleDataElement* aKeySingleDataElement = NULL;
	SingleDataElement* aCurSingleDataElement = NULL;	

	int aStringPos = 0;
	
	if (theStringPos == NULL)
		theStringPos = &aStringPos;

	while (*theStringPos < (int) theString.length())
	{
		bool addSingleChar = false;
		SexyChar aChar = theString[(*theStringPos)++];

		bool isSeperator = (aChar == ' ') || (aChar == '\t') || (aChar == '\n') || (aChar == ',');
		
		if (escaped)
		{
			addSingleChar = true;
			//escaped = false;
		}
		else
		{
			if ((aChar == '\'') && (!inDoubleQuotes))
				inSingleQuotes = !inSingleQuotes;
			else if ((aChar == '"') && (!inSingleQuotes))
				inDoubleQuotes = !inDoubleQuotes;

			if (aChar == '\\')
			{
				escaped = true;
			}
			else if ((!inSingleQuotes) && (!inDoubleQuotes))
			{
				if (aChar == ')')
				{
					if (expectListEnd)
						return true;
					else
					{
						Error(_S("Unexpected List End"));
						return false;
					}
				}
				else if (aChar == '(') 
				{
					if (wantTerminateSingleDataElement)
					{
						aCurSingleDataElement = NULL;
						wantTerminateSingleDataElement = false;
					}
					if (aCurSingleDataElement != NULL)
					{
						Error(_S("Unexpected List Start"));
						return false;
					}

					ListDataElement* aChildList = new ListDataElement();

					if (!ParseToList(theString, aChildList, true, theStringPos))
						return false;

					if (aKeySingleDataElement != NULL)
					{
						aKeySingleDataElement->mValue = aChildList;
						aKeySingleDataElement = NULL;
					}
					else
						theList->mElementVector.push_back(aChildList);
				}	
				else if (aChar == '=')
				{
					aKeySingleDataElement = aCurSingleDataElement;
					wantTerminateSingleDataElement = true;
				}
				else if (isSeperator)
				{
					if ((aCurSingleDataElement != NULL) && (aCurSingleDataElement->mString.length() > 0))
						wantTerminateSingleDataElement = true;
				}
				else
				{
					if (wantTerminateSingleDataElement)
					{
						aCurSingleDataElement = NULL;
						wantTerminateSingleDataElement = false;
					}
					addSingleChar = true;
				}
			}
			else
			{
				if (wantTerminateSingleDataElement)
				{
					aCurSingleDataElement = NULL;
					wantTerminateSingleDataElement = false;
				}
				addSingleChar = true;
			}
		}

		if (addSingleChar)
		{
			if (aCurSingleDataElement == NULL)
			{
				aCurSingleDataElement = new SingleDataElement();
				if (aKeySingleDataElement != NULL)
				{
					aKeySingleDataElement->mValue = aCurSingleDataElement;
					aKeySingleDataElement = NULL;
				}
				else
					theList->mElementVector.push_back(aCurSingleDataElement);
			}

			if (escaped)
			{
				aCurSingleDataElement->mString += _S("\\");
				escaped = false;
			}

			aCurSingleDataElement->mString += aChar;
		}
	}

	if (inSingleQuotes)
	{
		Error(_S("Unterminated Single Quotes"));
		return false;
	}

	if (inDoubleQuotes)
	{
		Error(_S("Unterminated Double Quotes"));
		return false;
	}

	if (expectListEnd)
	{
		Error(_S("Unterminated List"));
		return false;
	}

	return true;
}

bool DescParser::ParseDescriptorLine(const SexyString& theDescriptorLine) //629-647
{
	ListDataElement aParams;
	if (!ParseToList(theDescriptorLine, &aParams, false, NULL))
		return false;
	
	if (aParams.mElementVector.size() > 0)
	{
		if (aParams.mElementVector[0]->mIsList)
		{
			Error(_S("Missing Command"));
			return false;
		}

		if (!HandleCommand(aParams))
			return false;
	}

	return true;
}

bool DescParser::LoadDescriptor(const std::string& theFileName) //650-772
{
	mCurrentLineNum = 0;
	int aLineCount = 0;
	bool hasErrors = false;

	//Apparently VC6 doesn't have a clear() function for basic_strings
	//mError.clear();
	mError.erase();
	mCurrentLine.clear();

	if (!EncodingParser::OpenFile(theFileName))
		return Error(_S("Unable to open file: ") + SexyString(theFileName.begin(), theFileName.end()));

	while (!EndOfFile())
	{		
		SexyChar aChar;
						
		bool skipLine = false;
		bool atLineStart = true;
		bool inSingleQuotes = false;
		bool inDoubleQuotes = false;
		bool escaped = false; 
		bool isIndented = false;

		for (;;)
		{
			EncodingParser::GetCharReturnType aResult = GetChar(&aChar);
			if (aResult == END_OF_FILE)
				break;

			if (aResult == INVALID_CHARACTER)
				return Error(_S("Invalid Character"));
			if (aResult != SUCCESSFUL)
				return Error(_S("Internal Error"));
			
			if (aChar != '\r')
			{
				if (aChar == '\n')
					aLineCount++;

				if (((aChar == ' ') || (aChar == '\t')) && (atLineStart))
					isIndented = true;

				if ((!atLineStart) || ((aChar != ' ') && (aChar != '\t') && (aChar != '\n')))
				{
					if (atLineStart)
					{
						if ((mCmdSep & CMDSEP_NO_INDENT) && (!isIndented) && (mCurrentLine.size() > 0))
						{
							// Start a new non-indented line
							PutChar(aChar);
							break;
						}

						if (aChar == '#')
							skipLine = true;

						atLineStart = false;
					}					

					if (aChar == '\n')		
					{
						isIndented = false;
						atLineStart = true;				
					}

					if ((aChar == '\n') && (skipLine))
					{
						skipLine = false;						
					}
					else if (!skipLine)
					{
						if (aChar == '\\' && (inSingleQuotes || inDoubleQuotes) && !escaped)
							escaped = true;
						else
						{
							if ((aChar == '\'') && (!inDoubleQuotes) && (!escaped))
								inSingleQuotes = !inSingleQuotes;

							if ((aChar == '"') && (!inSingleQuotes) && (!escaped))
								inDoubleQuotes = !inDoubleQuotes;
							
							if ((aChar == ';') && (mCmdSep & CMDSEP_SEMICOLON) && (!inSingleQuotes) && (!inDoubleQuotes))
								break;
							
							if(escaped) // stay escaped for when this is actually parsed
							{
								mCurrentLine += '\\';
								escaped = false;
							}

							if (mCurrentLine.size() == 0)
								mCurrentLineNum = aLineCount + 1;

							mCurrentLine += aChar;
						}
					}
				}
			}
		}

		if (mCurrentLine.length() > 0)
		{
			if (!ParseDescriptorLine(mCurrentLine))
			{
				hasErrors = true;
				break;
			}

			//Apparently VC6 doesn't have a clear() function for basic_strings
			//mCurrentLine.clear();
			mCurrentLine.erase();
		}
	}

	//Apparently VC6 doesn't have a clear() function for basic_strings
	//mCurrentLine.clear();
	mCurrentLine.erase();
	mCurrentLineNum = 0;

	CloseFile();
	return !hasErrors;
}
