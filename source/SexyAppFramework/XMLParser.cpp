#include "XMLParser.h"
#include "Debug.h"

using namespace Sexy;

XMLParser::XMLParser() //8-12
{
	mFile = NULL;
	mLineNum = 0;
	mAllowComments = false;
}

XMLParser::~XMLParser() //15-16
{
}

void XMLParser::Fail(const SexyString& theErrorText) //19-22
{
	mHasFailed = true;
	mErrorText = theErrorText;
}

void XMLParser::Init() //25-30
{
	mSection = _S("");
	mLineNum = 1;
	mHasFailed = false;
	mErrorText = _S("");
}

bool XMLParser::AddAttribute(XMLElement* theElement, const SexyString& theAttributeKey, const SexyString& theAttributeValue) //33-43
{
	std::pair<XMLParamMap::iterator,bool> aRet;

	aRet = theElement->mAttributes.insert(XMLParamMap::value_type(theAttributeKey, theAttributeValue));
	if (!aRet.second)
		aRet.first->second = theAttributeValue;
	if (theAttributeKey != _S("/"))
		theElement->mAttributeIteratorList.push_back(aRet.first);

	return aRet.second;
}

bool XMLParser::AddAttributeEncoded(XMLElement* theElement, const SexyString& theAttributeKey, const SexyString& theAttributeValue) //46-56
{
	std::pair<XMLParamMap::iterator, bool> aRet;

	aRet = theElement->mAttributesEncoded.insert(XMLParamMap::value_type(theAttributeKey, theAttributeValue));
	if (!aRet.second)
		aRet.first->second = theAttributeValue;
	if (theAttributeKey != _S("/"))
		theElement->mAttributeEncodedIteratorList.push_back(aRet.first);

	return aRet.second;
}

bool XMLParser::OpenFile(const std::string& theFileName) //59-70
{
	if (EncodingParser::OpenFile(theFileName) == NULL)
	{
		mLineNum = 0;
		Fail(StringToSexyString("Unable to open file " + theFileName));
		return false;
	}

	mFileName = theFileName.c_str();
	Init();
	return true;
}

bool XMLParser::NextElement(XMLElement* theElement) //Correct? | 73-440
{
	for (;;)
	{
		theElement->mType = XMLElement::TYPE_NONE;
		theElement->mSection = mSection;
		theElement->mValue = _S("");
		theElement->mValueEncoded = _S("");
		theElement->mAttributes.clear();
		theElement->mAttributesEncoded.clear();
		theElement->mInstruction.erase();
		theElement->mAttributeIteratorList.clear();
		theElement->mAttributeEncodedIteratorList.clear();

		bool hasSpace = false;
		bool inQuote = false;
		bool gotEndQuote = false;

		bool doingAttribute = false;
		bool AttributeVal = false;
		std::wstring aAttributeKey;
		std::wstring aAttributeValue;

		std::wstring aLastAttributeKey;
		std::wstring aLastAttributeKeyEncoded;

		for (;;)
		{
			// Process character by character

			wchar_t c;
			int aVal;

			switch (GetChar(&c))
			{
			case SUCCESSFUL:
				aVal = 1;
				break;
			case INVALID_CHARACTER:
				Fail(_S("Illegal Character"));
				return false;
			case FAILURE:
				Fail(_S("Internal Error"));
				return false;
			case END_OF_FILE:
			default:
				aVal = 0;
				break;
			}

			if (aVal == 1)
			{
				bool processChar = false;

				if (c == L'\n')
				{
					mLineNum++;
				}

				if (theElement->mType == XMLElement::TYPE_COMMENT)
				{
					// Just add text to theElement->mInstruction until we find -->

					SexyString* aStrPtr = &theElement->mInstruction;

					*aStrPtr += (SexyChar)c;

					int aLen = aStrPtr->length();

					if ((c == L'>') && (aLen >= 3) && ((*aStrPtr)[aLen - 2] == L'-') && ((*aStrPtr)[aLen - 3] == L'-'))
					{
						*aStrPtr = aStrPtr->substr(0, aLen - 3);
						break;
					}
				}
				else if (theElement->mType == XMLElement::TYPE_INSTRUCTION)
				{
					// Just add text to theElement->mInstruction until we find ?>

					SexyString* aStrPtr = &theElement->mValue;

					if ((theElement->mInstruction.length() != 0) || (::iswspace(c)))
						aStrPtr = &theElement->mInstruction;

					*aStrPtr += (SexyChar)c;

					int aLen = aStrPtr->length();

					if ((c == L'>') && (aLen >= 2) && ((*aStrPtr)[aLen - 2] == L'?'))
					{
						*aStrPtr = aStrPtr->substr(0, aLen - 2);
						break;
					}
				}
				else
				{
					if (c == L'"')
					{
						inQuote = !inQuote;
						if (theElement->mType == XMLElement::TYPE_NONE || theElement->mType == XMLElement::TYPE_ELEMENT)
							processChar = true;

						if (!inQuote)
							gotEndQuote = true;
					}
					else if (!inQuote)
					{
						if (c == L'<')
						{
							if (theElement->mType == XMLElement::TYPE_ELEMENT)
							{
								//TODO: Fix buffered text.  Not sure what I meant by that.

								//OLD: mBufferedText = c + mBufferedText;

								PutChar(c);
								break;
							}

							if (theElement->mType == XMLElement::TYPE_NONE)
							{
								theElement->mType = XMLElement::TYPE_START;
							}
							else
							{
								Fail(_S("Unexpected '<'"));
								return false;
							}
						}
						else if (c == L'>')
						{
							if (theElement->mType == XMLElement::TYPE_START)
							{
								bool insertEnd = false;

								if (aAttributeKey == L"/")
								{
									// We will get this if we have a space before the />, so we can ignore it
									//  and go about our business now
									insertEnd = true;
								}
								else
								{
									// Probably isn't committed yet
									if (aAttributeKey.length() > 0)
									{
										//										theElement->mAttributes[aLastAttributeKey] = aAttributeValue;

										aLastAttributeKey = XMLDecodeString(aAttributeKey);
										aLastAttributeKeyEncoded = aAttributeKey;

										AddAttribute(theElement, WStringToSexyString(aLastAttributeKey), WStringToSexyString(aAttributeValue));

										AddAttributeEncoded(theElement, WStringToSexyString(aLastAttributeKey), WStringToSexyString(aAttributeValue));

										aAttributeKey = L"";
										aAttributeValue = L"";
									}

									if (aLastAttributeKey.length() > 0)
									{
										SexyString aStrVal = theElement->mAttributesEncoded[WStringToSexyString(aLastAttributeKey)]; //aVal in Transmension

										int aLen = aStrVal.length();

										if ((aLen > 0) && (aStrVal[aLen - 1] == '/'))
										{
											// Its an empty element, fake start and end segments
//											theElement->mAttributes[aLastAttributeKey] = aVal.substr(0, aLen - 1);

											AddAttribute(theElement, WStringToSexyString(aLastAttributeKey), XMLDecodeString(aStrVal.substr(0, aLen - 1)));

											insertEnd = true;
										}
										aStrVal = theElement->mAttributesEncoded[WStringToSexyString(aLastAttributeKeyEncoded)];
										aLen = aStrVal.length(); //Same var don't define
										if ((aLen > 0) && (aStrVal[aLen - 1] == '/'))
										{
											AddAttributeEncoded(theElement, WStringToSexyString(aLastAttributeKeyEncoded), XMLDecodeString(aStrVal.substr(0, aLen - 1)));

											insertEnd = true;
										}
									}
									else
									{
										int aLen = theElement->mValue.length();

										if ((aLen > 0) && (theElement->mValue[aLen - 1] == '/'))
										{
											// Its an empty element, fake start and end segments
											theElement->mValue = theElement->mValue.substr(0, aLen - 1);
											insertEnd = true;
										}
									}
								}

								// Do we want to fake an ending section?
								if (insertEnd)
								{
									SexyString anAddString = _S("</") + theElement->mValue + _S(">");


									//mBufferedText.resize(anOldSize + anAddLength);
									PutString(anAddString);

									// clear out aAttributeKey, since it contains "/" as its value and will insert
									// it into the element's attribute map.
									aAttributeKey = L"";

									//OLD: mBufferedText = "</" + theElement->mValue + ">" + mBufferedText;
								}

								if (mSection.length() != 0)
									mSection += _S("/");

								mSection += theElement->mValue;

								break;
							}
							else if (theElement->mType == XMLElement::TYPE_END)
							{
								int aLastSlash = mSection.rfind(_S('/'));
								if ((aLastSlash == -1) && (mSection.length() == 0))
								{
									Fail(_S("Unexpected End"));
									return false;
								}

								SexyString aLastSectionName = mSection.substr(aLastSlash + 1);

								if (aLastSectionName != theElement->mValue)
								{
									Fail(_S("End '") + theElement->mValue + _S("' Doesn't Match Start '") + aLastSectionName + _S("'"));
									return false;
								}

								if (aLastSlash == -1)
									mSection.erase(mSection.begin(), mSection.end());
								else
									mSection.erase(mSection.begin() + aLastSlash, mSection.end());

								break;
							}
							else
							{
								Fail(_S("Unexpected '>'"));
								return false;
							}
						}
						else if ((c == L'/') && (theElement->mType == XMLElement::TYPE_START) && (theElement->mValue == _S("")))
						{
							theElement->mType = XMLElement::TYPE_END;
						}
						else if ((c == L'?') && (theElement->mType == XMLElement::TYPE_START) && (theElement->mValue == _S("")))
						{
							theElement->mType = XMLElement::TYPE_INSTRUCTION;
						}
						else if (::isspace((uchar)c))
						{
							if (theElement->mValue != _S(""))
								hasSpace = true;

							// It's a comment!
							if ((theElement->mType == XMLElement::TYPE_START) && (theElement->mValue == _S("!--")))
								theElement->mType = XMLElement::TYPE_COMMENT;
						}
						else if (c > 32)
						{
							processChar = true;
						}
						else
						{
							Fail(_S("Illegal Character"));
							return false;
						}
					}
					else
					{
						processChar = true;
					}

					if (processChar)
					{
						if (theElement->mType == XMLElement::TYPE_NONE)
							theElement->mType = XMLElement::TYPE_ELEMENT;

						if (theElement->mType == XMLElement::TYPE_START)
						{
							if (hasSpace)
							{
								if ((!doingAttribute) || ((!AttributeVal) && (c != _S('='))) ||
									((AttributeVal) && ((aAttributeValue.length() > 0) || gotEndQuote)))
								{
									if (doingAttribute)
									{
										aAttributeKey = XMLDecodeString(aAttributeKey);
										aAttributeValue = XMLDecodeString(aAttributeValue);

										//										theElement->mAttributes[aAttributeKey] = aAttributeValue;

										AddAttribute(theElement, WStringToSexyString(aAttributeKey), WStringToSexyString(aAttributeValue));

										AddAttributeEncoded(theElement, WStringToSexyString(aAttributeKey), WStringToSexyString(aAttributeValue));

										aAttributeKey = L"";
										aAttributeValue = L"";

										aLastAttributeKey = L""; //= ""?
										aLastAttributeKeyEncoded = L""; //= ""?
									}
									else
									{
										doingAttribute = true;
									}

									AttributeVal = false;
								}

								hasSpace = false;
							}

							std::wstring* aStrPtr = NULL;

							if (!doingAttribute)
							{
								theElement->mValue += (SexyChar)c;
							}
							else
							{
								if (c == L'=')
								{
									AttributeVal = true;
									gotEndQuote = false;
								}
								else
								{
									if (!AttributeVal)
										aStrPtr = &aAttributeKey;
									else
										aStrPtr = &aAttributeValue;
								}
							}

							if (aStrPtr != NULL)
							{
								*aStrPtr += c;
							}
						}
						else
						{
							if (hasSpace)
							{
								theElement->mValue += _S(" ");
								hasSpace = false;
							}

							theElement->mValue += (SexyChar)c;
						}
					}
				}
			}
			else
			{
				if (theElement->mType != XMLElement::TYPE_NONE)
					Fail(_S("Unexpected End of File"));

				return false;
			}
		}

		if (aAttributeKey.length() > 0)
		{
			aAttributeKey = XMLDecodeString(aAttributeKey);
			aAttributeValue = XMLDecodeString(aAttributeValue);
			//			theElement->mAttributes[aAttributeKey] = aAttributeValue;

			AddAttribute(theElement, WStringToSexyString(aAttributeKey), WStringToSexyString(aAttributeValue));
		}

		theElement->mValueEncoded = theElement->mValue;
		theElement->mValue = XMLDecodeString(theElement->mValue);

		// Ignore comments
		if ((theElement->mType != XMLElement::TYPE_COMMENT) || mAllowComments)
			return true;
	}
}

bool XMLParser::HasFailed() //443-445
{
	return mHasFailed;
}

SexyString XMLParser::GetErrorText() //448-450
{
	return mErrorText;
}

int XMLParser::GetCurrentLineNum() //453-455
{
	return mLineNum;
}

std::string XMLParser::GetFileName() //458-460
{
	return mFileName;
}

void XMLParser::SetStringSource(const SexyString& theString) //463-466
{
	Init();
	EncodingParser::SetStringSource(theString);
}
