#include "ImageFont.h"
#include "Graphics.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "ResStreamsManager.h"
#include "MemoryImage.h"
#include "DeviceImage.h"
#include "..\SexyAppFramework\AutoCrit.h" //?
#include "Debug.h"
#include "SexyCache.h"
#include "Crypt/md5.h" //This is here just because compiled widefonts use MD5 hashes.

#ifdef _SEXYDECOMP_USE_LATEST_CODE
#include "Endian.h"
#endif

using namespace Sexy;

////

SortedKern::SortedKern(SexyChar inKey, SexyChar inValue, int inOffset) : //44
	mKey(inKey),
	mValue(inValue),
	mOffset(inOffset)
{}

int SortedKern::Compare(const void* a, const void* b) //47-59
{
	SortedKern* left = (SortedKern*)a; //?
	SortedKern* right = (SortedKern*)b; //?
	if (left->mKey < right->mKey)
		return -1;
	if (left->mKey > right->mKey)
		return 1;
	if (left->mValue < right->mValue)
		return -1;
	if (left->mValue > right->mValue)
		return 1;
	return -1;
}

int CharDataHashTable::GetBucketIndex(SexyChar inChar) //? | 63-76
{
	if (mOrderedHash)
		return inChar & HASH_BUCKET_MASK;


	const uint m = 0x5BD1E995;
	uint h = m * ((m * (inChar ^ 0xBEEFCAFE)) ^ ((m * (inChar ^ 0xBEEFCAFE)) >> 13));
	return (h ^ (h >> 15)) && HASH_BUCKET_MASK;
}

CharData* CharDataHashTable::GetCharData(SexyChar inChar, bool inAllowAdd) //79-132
{
	ulong aEntryIndex = GetBucketIndex(inChar);
	CharDataHashEntry* aEntry = &mHashEntries[aEntryIndex];
	if (aEntry->mChar == inChar && aEntry->mDataIndex != 0xFFFF)
		return &mCharData[aEntry->mDataIndex];
	if (aEntry->mChar == NULL)
	{
		if (!inAllowAdd)
			return NULL;
		aEntry->mChar = inChar;
		aEntry->mDataIndex = mCharData.size();
		mCharData.push_back(CharData()); //?
		CharData* aCharData = &mCharData[aEntry->mDataIndex];
		aCharData->mHashEntryIndex = aEntryIndex;
		return aCharData;
	}
	while (aEntry->mChar != inChar)
	{
		if (aEntry->mNext == -1)
		{
			if (!inAllowAdd)
				return NULL;
			aEntry->mNext = mHashEntries.size();
			mHashEntries.push_back(CharDataHashEntry());
			aEntry = &mHashEntries[aEntryIndex];
			CharDataHashEntry* aNewEntry = &mHashEntries[aEntry->mNext];
			aNewEntry->mChar = inChar;
			aNewEntry->mDataIndex = mCharData.size();
			mCharData.push_back(CharData()); //?
			CharData* aCharData = &mCharData[aNewEntry->mDataIndex]; //?
			aCharData->mHashEntryIndex = aEntry->mNext;
			return aCharData;
		}
		aEntryIndex = aEntry->mNext;
		aEntry = &mHashEntries[aEntryIndex];
	}
	DBG_ASSERTE(aEntry->mDataIndex!=0xffff); //129 | 134 BejLiveWin8
	return &mCharData[aEntry->mDataIndex];
}

CharData::CharData() //137-140
{
	mKerningFirst = mKerningCount = 0;
	mWidth = mOrder = 0;
}

FontLayer::FontLayer() //143-161
{
	mFontData = NULL;
	mDrawMode = -1;
	mSpacing = 0;
	mPointSize = 0;
	mAscent = 0;
	mAscentPadding = 0;
	mMinPointSize = -1;
	mMaxPointSize = -1;
	mHeight = 0;
	mDefaultHeight = 0;
	mColorMult = Color::White;
	mColorAdd = Color(0, 0, 0, 0);
	mLineSpacingOffset = 0;
	mBaseOrder = 0;
	mImageIsWhite = false;
	mUseAlphaCorrection = true;
	mCharDataHashTable.mOrderedHash = ImageFont::mOrderedHash;
}

FontLayer::FontLayer(FontData* theFontData) //164-182
{
	mFontData = theFontData;
	mDrawMode = -1;
	mSpacing = 0;
	mPointSize = 0;
	mAscent = 0;
	mAscentPadding = 0;
	mMinPointSize = -1;
	mMaxPointSize = -1;
	mHeight = 0;
	mDefaultHeight = 0;
	mColorMult = Color::White;
	mColorAdd = Color(0, 0, 0, 0);
	mLineSpacingOffset = 0;
	mBaseOrder = 0;
	mImageIsWhite = false;
	mUseAlphaCorrection = true;
	mCharDataHashTable.mOrderedHash = ImageFont::mOrderedHash;
}

FontLayer::FontLayer(const FontLayer& theFontLayer) : //209-210
	mFontData(theFontLayer.mFontData),
	mRequiredTags(theFontLayer.mRequiredTags),
	mExcludedTags(theFontLayer.mExcludedTags),
	mImage(theFontLayer.mImage),
	mDrawMode(theFontLayer.mDrawMode),
	mOffset(theFontLayer.mOffset),
	mSpacing(theFontLayer.mSpacing),
	mMinPointSize(theFontLayer.mMinPointSize),
	mMaxPointSize(theFontLayer.mMaxPointSize),
	mPointSize(theFontLayer.mPointSize),
	mAscent(theFontLayer.mAscent),
	mAscentPadding(theFontLayer.mAscentPadding),
	mHeight(theFontLayer.mHeight),
	mDefaultHeight(theFontLayer.mDefaultHeight),
	mColorMult(theFontLayer.mColorMult),
	mColorAdd(theFontLayer.mColorAdd),
	mLineSpacingOffset(theFontLayer.mLineSpacingOffset),
	mBaseOrder(theFontLayer.mBaseOrder),
	mExtendedInfo(theFontLayer.mExtendedInfo),
	mKerningData(theFontLayer.mKerningData),
	mCharDataHashTable(theFontLayer.mCharDataHashTable),
	mUseAlphaCorrection(theFontLayer.mUseAlphaCorrection),
	mLayerName(theFontLayer.mLayerName)
{
}

CharData* FontLayer::GetCharData(SexyChar theChar) //213-215
{
	return mCharDataHashTable.GetCharData(theChar, true);
}

FontData::FontData() //218-224
{
	mInitialized = false;

	mApp = NULL;
	mRefCount = 0;
	mDefaultPointSize = 0;
}

FontData::~FontData() //227-237
{
	DataElementMap::iterator anItr = mDefineMap.begin();
	while (anItr != mDefineMap.end())
	{
		std::wstring aDefineName = anItr->first;
		DataElement* aDataElement = anItr->second;

		delete aDataElement;
		++anItr;
	}
}

void FontData::Ref() //240-242
{
	mRefCount++;
}

void FontData::DeRef() //245-250
{
	if (--mRefCount == 0)
	{
		delete this;
	}
}

bool FontData::Error(const std::wstring& theError) //Return false on XNA | 253-264
{
	if (mApp != NULL)
	{
		std::wstring anErrorString = mFontErrorHeader + theError;

		if (mCurrentLine.length() > 0)
		{
			anErrorString += L" on Line " + StrFormat(L"%d:\r\n\r\n", mCurrentLineNum) + mCurrentLine;
		}

		mApp->Popup(anErrorString);
	}

	return false;
}

bool FontData::DataToLayer(DataElement* theSource, FontLayer** theFontLayer) //267-285
{
	*theFontLayer = NULL;

	if (theSource->mIsList)
		return false;

	std::string aLayerName = WStringToString(StringToUpper(((SingleDataElement*)theSource)->mString), false); //?

	FontLayerMap::iterator anItr = mFontLayerMap.find(aLayerName);
	if (anItr == mFontLayerMap.end())
	{
		DescParser::Error(L"Undefined Layer");
		return false;
	}

	*theFontLayer = anItr->second;

	return true;
}

bool FontData::GetColorFromDataElement(DataElement* theElement, Color& theColor) //288-310
{
	if (theElement->mIsList)
	{
		DoubleVector aFactorVector;
		if (!DataToDoubleVector(theElement, &aFactorVector) && (aFactorVector.size() == 4))
			return false;

		theColor = Color(
			(int)(aFactorVector[0] * 255),
			(int)(aFactorVector[1] * 255),
			(int)(aFactorVector[2] * 255),
			(int)(aFactorVector[3] * 255));

		return true;
	}

	int aColor = 0;
	if (!StringToInt(((SingleDataElement*)theElement)->mString, &aColor))
		return false;

	theColor = aColor;
	return true;
}


bool FontData::HandleCommand(const ListDataElement& theParams) //Changed, XNA doesn't have in commands | 314-1213
{
	std::wstring aCmd = ((SingleDataElement*)theParams.mElementVector[0])->mString;

	bool invalidNumParams = false;
	bool invalidParamFormat = false;
	bool literalError = false;
	bool sizeMismatch = false;

	if (sexystricmp(aCmd.c_str(), _S("Define")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			if (!theParams.mElementVector[1]->mIsList)
			{
				std::wstring aDefineName = StringToUpper(((SingleDataElement*)theParams.mElementVector[1])->mString);

				if (!IsImmediate(aDefineName))
				{
					DataElementMap::iterator anItr = mDefineMap.find(aDefineName);
					if (anItr != mDefineMap.end())
					{
						delete anItr->second;
						mDefineMap.erase(anItr);
					}

					if (theParams.mElementVector[2]->mIsList)
					{
						ListDataElement* aValues = new ListDataElement();
						if (!GetValues(((ListDataElement*)theParams.mElementVector[2]), aValues))
						{
							delete aValues;
							return false;
						}

						mDefineMap.insert(DataElementMap::value_type(aDefineName, aValues));
					}
					else
					{
						SingleDataElement* aDefParam = (SingleDataElement*)theParams.mElementVector[2];

						DataElement* aDerefVal = Dereference(aDefParam->mString);

						if (aDerefVal)
							mDefineMap.insert(DataElementMap::value_type(aDefineName, aDerefVal->Duplicate()));
						else
							mDefineMap.insert(DataElementMap::value_type(aDefineName, aDefParam->Duplicate()));
					}
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("CreateHorzSpanRectList")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			IntVector aRectIntVector;
			IntVector aWidthsVector;

			if ((!theParams.mElementVector[1]->mIsList) &&
				(DataToIntVector(theParams.mElementVector[2], &aRectIntVector)) &&
				(aRectIntVector.size() == 4) &&
				(DataToIntVector(theParams.mElementVector[3], &aWidthsVector)))
			{
				std::wstring aDefineName = StringToUpper(((SingleDataElement*)theParams.mElementVector[1])->mString);

				int aXPos = 0;

				ListDataElement* aRectList = new ListDataElement();

				for (ulong aWidthNum = 0; aWidthNum < aWidthsVector.size(); aWidthNum++)
				{
					ListDataElement* aRectElement = new ListDataElement();
					aRectList->mElementVector.push_back(aRectElement);

					SexyString aStr;

					aStr = StrFormat(_S("%d"), aRectIntVector[0] + aXPos);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					aStr = StrFormat(_S("%d"), aRectIntVector[1]);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					aStr = StrFormat(_S("%d"), aWidthsVector[aWidthNum]);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					aStr = StrFormat(_S("%d"), aRectIntVector[3]);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					aXPos += aWidthsVector[aWidthNum];
				}

				DataElementMap::iterator anItr = mDefineMap.find(aDefineName);
				if (anItr != mDefineMap.end())
				{
					delete anItr->second;
					mDefineMap.erase(anItr);
				}

				mDefineMap.insert(DataElementMap::value_type(aDefineName, aRectList));
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("SetDefaultPointSize")) == 0)
	{
		if (theParams.mElementVector.size() == 2)
		{
			int aPointSize;

			if ((!theParams.mElementVector[1]->mIsList) &&
				(StringToInt(((SingleDataElement*)theParams.mElementVector[1])->mString, &aPointSize)))
			{
				mDefaultPointSize = aPointSize;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("SetCharMap")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			SexyStringVector aFromVector;
			SexyStringVector aToVector;

			if ((DataToStringVector(theParams.mElementVector[1], &aFromVector)) &&
				(DataToStringVector(theParams.mElementVector[2], &aToVector)))
			{
				if (aFromVector.size() == aToVector.size())
				{
					for (ulong aMapIdx = 0; aMapIdx < aFromVector.size(); aMapIdx++)
					{
						if ((aFromVector[aMapIdx].length() == 1) && (aToVector[aMapIdx].length() == 1))
						{
							mCharMap[(uchar)aFromVector[aMapIdx][0]] = (uchar)aToVector[aMapIdx][0];
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("CreateLayer")) == 0)
	{
		if (theParams.mElementVector.size() == 2)
		{
			if (!theParams.mElementVector[1]->mIsList)
			{
				std::string aLayerName = WStringToString(StringToUpper(((SingleDataElement*)theParams.mElementVector[1])->mString), false);

				mFontLayerList.push_back(FontLayer(this));
				FontLayer* aFontLayer = &mFontLayerList.back();

				if (!mFontLayerMap.insert(FontLayerMap::value_type(aLayerName, aFontLayer)).second)
				{
					Error(L"Layer Already Exists");
				}
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("CreateLayerFrom")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aSourceLayer;

			if ((!theParams.mElementVector[1]->mIsList) && (DataToLayer(theParams.mElementVector[2], &aSourceLayer)))
			{
				std::string aLayerName = WStringToString(StringToUpper(((SingleDataElement*)theParams.mElementVector[1])->mString), false);

				mFontLayerList.push_back(FontLayer(*aSourceLayer));
				FontLayer* aFontLayer = &mFontLayerList.back();

				if (!mFontLayerMap.insert(FontLayerMap::value_type(aLayerName, aFontLayer)).second)
				{
					Error(L"Layer Already Exists");
				}
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerRequireTags")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			SexyStringVector aStringVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aStringVector)))
			{
				for (ulong i = 0; i < aStringVector.size(); i++)
					aLayer->mRequiredTags.push_back(WStringToString(StringToUpper(aStringVector[i]), false));
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerExcludeTags")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			SexyStringVector aStringVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aStringVector)))
			{
				for (ulong i = 0; i < aStringVector.size(); i++)
					aLayer->mExcludedTags.push_back(WStringToString(StringToUpper(aStringVector[i]), false));
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerPointRange")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList) &&
				(!theParams.mElementVector[3]->mIsList))
			{
				int aMinPointSize;
				int aMaxPointSize;

				if ((StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &aMinPointSize)) &&
					(StringToInt(((SingleDataElement*)theParams.mElementVector[3])->mString, &aMaxPointSize)))
				{
					aLayer->mMinPointSize = aMinPointSize;
					aLayer->mMaxPointSize = aMaxPointSize;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetPointSize")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int aPointSize;
				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &aPointSize))
				{
					aLayer->mPointSize = aPointSize;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetHeight")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int aHeight;
				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &aHeight))
				{
					aLayer->mHeight = aHeight;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetImage")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			std::wstring aFileNameString;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToString(theParams.mElementVector[2], &aFileNameString)))
			{
				std::string aFileName = GetPathFrom(ToString(aFileNameString), GetFileDir(mSourceFile));

				bool isNew;
				bool old = gSexyAppBase->mWriteToSexyCache; //?
				gSexyAppBase->mWriteToSexyCache = false; //?

				SharedImageRef anImage = mApp->GetSharedImage(aFileName, "", &isNew); //?

				aLayer->mImageFileName = aFileName;
				gSexyAppBase->mWriteToSexyCache = old; //?

				if ((Image*)anImage != NULL)
				{
					if (!isNew && anImage->mColorTable != NULL)
					{
						aLayer->mImageIsWhite = true;
						for (int i = 0; i < 256; i++)
						{
							if ((anImage->mColorTable[i] & 0xFFFFFF) != 0xFFFFFF)
								aLayer->mImageIsWhite = false;
						}
					}
					aLayer->mImage = anImage;
				}
				else
				{
					Error(L"Failed to Load Image");
					return false;
				}
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetDrawMode")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && (!theParams.mElementVector[2]->mIsList))
			{
				int anDrawMode;
				if ((StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &anDrawMode)) &&
					(anDrawMode >= 0) && (anDrawMode <= 1))
				{
					aLayer->mDrawMode = anDrawMode;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetColorMult")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if (DataToLayer(theParams.mElementVector[1], &aLayer))
			{
				if (!GetColorFromDataElement(theParams.mElementVector[2], aLayer->mColorMult))
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetColorAdd")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if (DataToLayer(theParams.mElementVector[1], &aLayer))
			{
				if (!GetColorFromDataElement(theParams.mElementVector[2], aLayer->mColorAdd))
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetAscent")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int anAscent;
				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &anAscent))
				{
					aLayer->mAscent = anAscent;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetAscentPadding")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int anAscent;
				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &anAscent))
				{
					aLayer->mAscentPadding = anAscent;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetLineSpacingOffset")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int anAscent;
				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &anAscent))
				{
					aLayer->mLineSpacingOffset = anAscent;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetOffset")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			IntVector anOffset;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && (DataToIntVector(theParams.mElementVector[2], &anOffset)) && (anOffset.size() == 2))
			{
				aLayer->mOffset.mX = anOffset[0];
				aLayer->mOffset.mY = anOffset[1];
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetCharWidths")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			SexyStringVector aCharsVector;
			IntVector aCharWidthsVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) &&
				(DataToIntVector(theParams.mElementVector[3], &aCharWidthsVector)))
			{
				if (aCharsVector.size() == aCharWidthsVector.size())
				{
					for (ulong i = 0; i < aCharsVector.size(); i++)
					{
						if (aCharsVector[i].length() == 1)
						{
							aLayer->GetCharData(aCharsVector[i][0])->mWidth =
								aCharWidthsVector[i];
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetSpacing")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			IntVector anOffset;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int aSpacing;

				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &aSpacing))
				{
					aLayer->mSpacing = aSpacing;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetImageMap")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			SexyStringVector aCharsVector;
			ListDataElement aRectList;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) &&
				(DataToList(theParams.mElementVector[3], &aRectList)))
			{
				if (aCharsVector.size() == aRectList.mElementVector.size())
				{
					if ((Image*)aLayer->mImage != NULL)
					{
						int anImageWidth = aLayer->mImage->GetWidth();
						int anImageHeight = aLayer->mImage->GetHeight();

						for (ulong i = 0; i < aCharsVector.size(); i++)
						{
							IntVector aRectElement;

							if ((aCharsVector[i].length() == 1) &&
								(DataToIntVector(aRectList.mElementVector[i], &aRectElement)) &&
								(aRectElement.size() == 4))

							{
								Rect aRect = Rect(aRectElement[0], aRectElement[1], aRectElement[2], aRectElement[3]);

								if ((aRect.mX < 0) || (aRect.mY < 0) ||
									(aRect.mX + aRect.mWidth > anImageWidth) || (aRect.mY + aRect.mHeight > anImageHeight))
								{
									Error(L"Image rectangle out of bounds");
									return false;
								}

								aLayer->GetCharData(aCharsVector[i][0])->mImageRect = aRect;;
							}
							else
								invalidParamFormat = true;
						}

						aLayer->mDefaultHeight = 0;
						int aCharDataCount = aLayer->mCharDataHashTable.mCharData.size();
						CharData* aCharData = &aLayer->mCharDataHashTable.mCharData.front();
						for (int iCharData = 0; iCharData < aCharDataCount; iCharData++)
							if (aCharData[iCharData].mImageRect.mHeight + aCharData[iCharData].mOffset.mY > aLayer->mDefaultHeight)
								aLayer->mDefaultHeight = aCharData[iCharData].mImageRect.mHeight + aCharData[iCharData].mOffset.mY;
					}
					else
					{
						Error(L"Layer image not set");
						return false;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetCharOffsets")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			SexyStringVector aCharsVector;
			ListDataElement aRectList;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) &&
				(DataToList(theParams.mElementVector[3], &aRectList)))
			{
				if (aCharsVector.size() == aRectList.mElementVector.size())
				{
					for (ulong i = 0; i < aCharsVector.size(); i++)
					{
						IntVector aRectElement;

						if ((aCharsVector[i].length() == 1) &&
							(DataToIntVector(aRectList.mElementVector[i], &aRectElement)) &&
							(aRectElement.size() == 2))
						{
							aLayer->GetCharData(aCharsVector[i][0])->mOffset =
								Point(aRectElement[0], aRectElement[1]);
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetKerningPairs")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			SexyStringVector aPairsVector;
			IntVector anOffsetsVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aPairsVector)) &&
				(DataToIntVector(theParams.mElementVector[3], &anOffsetsVector)))
			{
				if (aPairsVector.size() == anOffsetsVector.size())
				{
					std::vector<SortedKern> aSortedKern;
					for (ulong i = 0; i < aPairsVector.size(); i++)
					{
						if (aPairsVector[i].length() == 2)
						{
							aSortedKern.push_back(SortedKern(aPairsVector[i][0],
								aPairsVector[i][1], anOffsetsVector[i]));
						}
						else
							invalidParamFormat = true;
					}
					if (!aSortedKern.empty())
						qsort(&aSortedKern[0], aSortedKern.size(), 8, SortedKern::Compare);
					DBG_ASSERTE(aLayer->mKerningData.empty()); //1062 | 1084 BejLiveWin8
					aLayer->mKerningData.resize(aSortedKern.size());
					for (int i = 0; i < aSortedKern.size(); i++)
					{
						SortedKern* sk = &aSortedKern[i];
						FontLayer::KerningValue* kv = (FontLayer::KerningValue*)&aLayer->mKerningData[i];
						kv->mChar = sk->mValue;
						kv->mOffset = sk->mOffset;
						CharData* aCharData = aLayer->GetCharData(sk->mKey);
						if (!aCharData->mKerningCount)
							aCharData->mKerningFirst = i;
						aCharData->mKerningCount++;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetBaseOrder")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(!theParams.mElementVector[2]->mIsList))
			{
				int aBaseOrder;
				if (StringToInt(((SingleDataElement*)theParams.mElementVector[2])->mString, &aBaseOrder))
				{
					aLayer->mBaseOrder = aBaseOrder;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetCharOrders")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			SexyStringVector aCharsVector;
			IntVector aCharOrdersVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) &&
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) &&
				(DataToIntVector(theParams.mElementVector[3], &aCharOrdersVector)))
			{
				if (aCharsVector.size() == aCharOrdersVector.size())
				{
					for (ulong i = 0; i < aCharsVector.size(); i++)
					{
						if (aCharsVector[i].length() == 1)
						{
							aLayer->GetCharData(aCharsVector[i][0])->mOrder =
								aCharOrdersVector[i];
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetExInfo")) == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer; //?
			SexyStringVector aKeys;
			SexyStringVector aValues;
			if (DataToLayer(theParams.mElementVector[1], &aLayer) && DataToStringVector(theParams.mElementVector[2], &aKeys) && DataToStringVector(theParams.mElementVector[3], &aValues))
			{
				if (aKeys.size() == aValues.size())
				{
					for (int i = 0; i > aKeys.size(); i++)
						aLayer->mExtendedInfo.insert(aKeys[i], aValues[i]); //?
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (sexystricmp(aCmd.c_str(), _S("LayerSetAlphaCorrection")) == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer; //?
			int aUseAlphaCorrection = 0;
			if (DataToLayer(theParams.mElementVector[1], &aLayer) && DataToInt(theParams.mElementVector[2], &aUseAlphaCorrection))
				aLayer->mUseAlphaCorrection = aUseAlphaCorrection != 0;
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else
	{
		Error(L"Unknown Command");
		return false;
	}

	if (invalidNumParams)
	{
		Error(L"Invalid Number of Parameters");
		return false;
	}

	if (invalidParamFormat)
	{
		Error(L"Invalid Paramater Type"); //Paramater lol
		return false;
	}

	if (literalError)
	{
		Error(L"Undefined Value");
		return false;
	}

	if (sizeMismatch)
	{
		Error(L"List Size Mismatch");
		return false;
	}

	return true;
}

bool FontData::Load(SexyAppBase* theSexyApp, const std::string& theFontDescFileName) //1216-1232
{
	if (mInitialized)
		return false;

	bool hasErrors = false;

	mApp = theSexyApp;
	mCurrentLine = L"";

	mFontErrorHeader = L"Font Descriptor Error in " + StringToSexyStringFast(theFontDescFileName) + L"\r\n";

	mSourceFile = theFontDescFileName;

	mInitialized = LoadDescriptor(theFontDescFileName); ;

	return !hasErrors;
}

bool FontData::LoadLegacy(Image* theFontImage, const std::string& theFontDescFileName) //TODO | 1235-1300
{
	if (mInitialized)
		return false;

	mFontLayerList.push_back(FontLayer(this));
	FontLayer* aFontLayer = &mFontLayerList.back();

	FontLayerMap::iterator anItr = mFontLayerMap.insert(FontLayerMap::value_type("MAIN", aFontLayer)).first; //?
	if (anItr == mFontLayerMap.end())
		return false;

	aFontLayer->mImage = (MemoryImage*)theFontImage;
	aFontLayer->mDefaultHeight = aFontLayer->mImage->GetHeight();
	aFontLayer->mAscent = aFontLayer->mImage->GetHeight();

	int aCharPos = 0;
	FILE* aStream = fopen(theFontDescFileName.c_str(), "r");

	if (aStream == NULL)
		return false;

	mSourceFile = theFontDescFileName;

	
	fscanf(aStream, "%d%d", &aFontLayer->GetCharData(' ')->mWidth, &aFontLayer->mAscent);

	while (!feof(aStream))
	{
		char aBuf[2] = { 0, 0 }; // needed because fscanf will null terminate the string it reads
		char aChar = 0;
		int aWidth = 0;

		fscanf(aStream, "%1s%d", aBuf, &aWidth);
		aChar = aBuf[0];


		if (aChar == 0)
			break;

		aFontLayer->GetCharData(aChar)->mImageRect = Rect(aCharPos, 0, aWidth, aFontLayer->mImage->GetHeight());
		aFontLayer->GetCharData(aChar)->mWidth = aWidth;

		aCharPos += aWidth;
	}

	char c;

	for (c = 'A'; c <= 'Z'; c++)
		if ((aFontLayer->GetCharData(c)->mWidth == 0) && (aFontLayer->GetCharData(c - 'A' + 'a')->mWidth != 0))
			mCharMap[c] = c - 'A' + 'a';

	for (c = 'a'; c <= 'z'; c++)
		if ((aFontLayer->GetCharData(c)->mWidth == 0) && (aFontLayer->GetCharData(c - 'a' + 'A')->mWidth != 0))
			mCharMap[c] = c - 'a' + 'A';

	mInitialized = true;
	fclose(aStream);

	return true;
}

////

ActiveFontLayer::ActiveFontLayer() //1305-1306
{
}

ActiveFontLayer::ActiveFontLayer(const ActiveFontLayer& theActiveFontLayer) : //1313-1316
	mBaseFontLayer(theActiveFontLayer.mBaseFontLayer),
	mUseAlphaCorrection(theActiveFontLayer.mUseAlphaCorrection),
	mScaledCharImageRects(theActiveFontLayer.mScaledCharImageRects),
	mColorStack(theActiveFontLayer.mColorStack)
{
	for (int i = 0; i < 8; i++)
		mScaledImages[i] = theActiveFontLayer.mScaledImages[i];
}

ActiveFontLayer::~ActiveFontLayer() //1319-1320
{
}

////

ImageFont::ImageFont() //1328-1338
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mImageFontSet.insert(this);
	mFontImage = NULL;
	mScale = 1.0;
	mWantAlphaCorrection = false;
	mFontData = new FontData();
	mFontData->Ref();
}

ImageFont::ImageFont(SexyAppBase* theSexyApp, const std::string& theFontDescFileName) //Correct? | 1341-1441
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mImageFontSet.insert(this);
	mFontImage = NULL;
	mScale = 1.0;
	mWantAlphaCorrection = false;
	mFontData = new FontData();
	mFontData->Ref();
	std::string aLocalCachedFontFileName = "cached\\" + theFontDescFileName + ".cfw2";
	std::string aCachedFontFileName = GetAppDataFolder() + aLocalCachedFontFileName;
	Buffer aBuf;
	bool aGotBuf;
	if (theSexyApp->ReadBufferFromFile(aLocalCachedFontFileName, &aBuf) && aBuf.GetDataLen() >= 16)
		aGotBuf = true;
	else if (theSexyApp->ReadBufferFromFile(aCachedFontFileName, &aBuf) && aBuf.GetDataLen() >= 16) //Move it to the other one?
		aGotBuf = true;
	bool readCacheSuccess;
	if (aGotBuf)
	{
		if (theSexyApp->mResStreamsManager != NULL && theSexyApp->mResStreamsManager->IsInitialized())
		{
			SerializeRead((void*)(aBuf.GetDataPtr() + 16), aBuf.GetDataLen() - 16);
			readCacheSuccess = true;
		}
		else
		{
			Buffer aDescBuf;
			if (theSexyApp->ReadBufferFromFile(theFontDescFileName, &aDescBuf)) //C++ only
			{
				MD5Context aMD5Context;
				MD5Init(&aMD5Context);
				MD5Update(&aMD5Context, aDescBuf.GetDataPtr(), aDescBuf.GetDataLen());
				uchar digest[10];
				MD5Final(digest, &aMD5Context);
				if (memcmp(aBuf.GetDataPtr(), digest, 0x10))
				{
					SerializeRead((void*)(aBuf.GetDataPtr() + 16), aBuf.GetDataLen() - 16);
					readCacheSuccess = true;
				}
			}
		}
	}

	if (!readCacheSuccess)
	{
		mFontData->Load(theSexyApp, theFontDescFileName);
		mPointSize = mFontData->mDefaultPointSize;
		mActivateAllLayers = true;
		GenerateActiveFontLayers();
		mActiveListValid = true;
		mForceScaledImagesWhite = false;
		if (theSexyApp->mWriteFontCacheDir) //C++ only
		{
			Buffer aDescBuf;
			if (theSexyApp->ReadBufferFromFile(theFontDescFileName, &aDescBuf))
			{
				MD5Context aMD5Context;
				MD5Init(&aMD5Context);
				MD5Update(&aMD5Context, aDescBuf.GetDataPtr(), aDescBuf.GetDataLen());
				uchar digest[10];
				MD5Final(digest, &aMD5Context);
				MkDir(GetFileDir(aCachedFontFileName));
				int aSize = SerializeWrite(0, 0);
				char* aBytes = new char[aSize + 16];
				memcpy(aBytes, digest, 16);
				SerializeWrite(aBytes + 16, aSize);
				theSexyApp->WriteBytesToFile(aCachedFontFileName, aBytes, aSize + 16);
				delete aBytes;
			}
		}
	}
}

ImageFont::ImageFont(Image* theFontImage) //1444-1467
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mImageFontSet.insert(this);
	mScale = 1.0;
	mWantAlphaCorrection = true;
	mFontData = new FontData();
	mFontData->Ref();
	mFontData->mInitialized = true;
	mPointSize = mFontData->mDefaultPointSize;
	mActiveListValid = false;
	mForceScaledImagesWhite = false;
	mActivateAllLayers = false;

	mFontData->mFontLayerList.push_back(FontLayer(mFontData));
	FontLayer* aFontLayer = &mFontData->mFontLayerList.back();

	mFontData->mFontLayerMap.insert(FontLayerMap::value_type("", aFontLayer)).first;
	aFontLayer->mImage = (MemoryImage*)theFontImage;
	aFontLayer->mDefaultHeight = aFontLayer->mImage->GetHeight();
	aFontLayer->mAscent = aFontLayer->mImage->GetHeight();
}

ImageFont::ImageFont(const ImageFont& theImageFont) : //? | 1480-1489
	Font(theImageFont),
	mScale(theImageFont.mScale),
	mFontData(theImageFont.mFontData),
	mPointSize(theImageFont.mPointSize),
	mTagVector(theImageFont.mTagVector),
	mActiveListValid(theImageFont.mActiveListValid),
	mForceScaledImagesWhite(theImageFont.mForceScaledImagesWhite),
	mWantAlphaCorrection(theImageFont.mWantAlphaCorrection),
	mActivateAllLayers(theImageFont.mActivateAllLayers),
	mFontImage(theImageFont.mFontImage)
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mImageFontSet.insert(this);
	mFontData->Ref();

	if (mActiveListValid)
		mActiveLayerList = theImageFont.mActiveLayerList;
}

ImageFont::ImageFont(Image* theFontImage, const std::string& theFontDescFileName) //1492-1506
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mImageFontSet.insert(this);
	mScale = 1.0;
	mFontImage = NULL;
	mFontData = new FontData();
	mFontData->Ref();
	mFontData->LoadLegacy(theFontImage, theFontDescFileName);
	mPointSize = mFontData->mDefaultPointSize;
	mActivateAllLayers = false;
	GenerateActiveFontLayers();
	mActiveListValid = true;
}

ImageFont::~ImageFont() //1509-1515
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	//gSexyAppBase->mImageFontSet.find(this); //?
	gSexyAppBase->mImageFontSet.erase(this); //?
	mFontData->DeRef();
}

void ImageFont::GenerateActiveFontLayers() //Correct? | 1529-1761
{
	if (!mFontData->mInitialized)	
		return;

	mActiveLayerList.clear();


	ulong i;

	mAscent = 0;
	mAscentPadding = 0;
	mHeight = 0;
	mLineSpacingOffset = 0;

	FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();

	bool firstLayer = true;

	while (anItr != mFontData->mFontLayerList.end())
	{
		FontLayer* aFontLayer = &*anItr;

		if ((mPointSize >= aFontLayer->mMinPointSize) &&
			((mPointSize <= aFontLayer->mMaxPointSize) || (aFontLayer->mMaxPointSize == -1)))
		{
			bool active = true;

			// Make sure all required tags are included
			for (i = 0; i < aFontLayer->mRequiredTags.size(); i++)
				if (std::find(mTagVector.begin(), mTagVector.end(), aFontLayer->mRequiredTags[i]) == mTagVector.end())
					active = false;

			// Make sure no excluded tags are included
			for (i = 0; i < mTagVector.size(); i++)
				if (std::find(aFontLayer->mExcludedTags.begin(), aFontLayer->mExcludedTags.end(),
					mTagVector[i]) != aFontLayer->mExcludedTags.end())
					active = false;

			active |= mActivateAllLayers;

			if (active)
			{
				mActiveLayerList.push_back(ActiveFontLayer());

				ActiveFontLayer* anActiveFontLayer = &mActiveLayerList.back();

				anActiveFontLayer->mBaseFontLayer = aFontLayer;
				anActiveFontLayer->mUseAlphaCorrection = mWantAlphaCorrection && aFontLayer->mImageIsWhite;

				double aLayerPointSize = 1;
				double aPointSize = mScale;

				if ((mScale == 1.0) && ((aFontLayer->mPointSize == 0) || (mPointSize == aFontLayer->mPointSize)))
				{
					anActiveFontLayer->mScaledImages[7] = aFontLayer->mImage;
					if (mFontImage)
						anActiveFontLayer->mScaledImages[7].mUnsharedImage = mFontImage;

					int aCharDataCount = aFontLayer->mCharDataHashTable.mCharData.size();
					CharData* aCharData = &aFontLayer->mCharDataHashTable.mCharData.front();
					for (int iCharData = 0; iCharData < aCharDataCount; iCharData++)
					{
						CharDataHashEntry aHashEntry = aFontLayer->mCharDataHashTable.mHashEntries[aCharData[iCharData].mHashEntryIndex];
						anActiveFontLayer->mScaledCharImageRects.insert(SexyCharToRectMap::value_type(aHashEntry.mChar, aCharData[iCharData].mImageRect));
					}
				}
				else
				{
					if (aFontLayer->mPointSize != 0)
					{
						aLayerPointSize = aFontLayer->mPointSize;
						aPointSize = mPointSize * mScale;
					}

					// Resize font elements

					MemoryImage* aMemoryImage = new MemoryImage(mFontData->mApp);

					int aCurX = 0;
					bool uniformHeights = true;
					int aMinY = 0;
					int aMaxY = 0;

					int aCharDataCount = aFontLayer->mCharDataHashTable.mCharData.size();
					CharData* aCharData = &aFontLayer->mCharDataHashTable.mCharData.front();

					for (int i = 0; i < aCharDataCount; i++)
					{
						Rect* anOrigRect = &aCharData[aCharDataCount].mImageRect;

						int aStartY = anOrigRect->mY;
						int aEndY = anOrigRect->mHeight + aStartY;

						aMinY = min(aStartY, aMinY);
						aMaxY = max(aEndY, aMaxY);

						if (aMinY != aStartY || aMaxY != aEndY)
							uniformHeights = false;

						aCurX += anOrigRect->mWidth + 2;
					}

					// Create the image now

					if (!uniformHeights)
					{
						MemoryImage* aNewImage = new MemoryImage(mFontData->mApp);
						aNewImage->Create(aCurX, aMaxY - aMinY);
						aNewImage->SetImageMode(true, true); //C++ only
						Graphics g(aNewImage);
						aCurX = 0;
						aCharDataCount = aFontLayer->mCharDataHashTable.mCharData.size();
						aCharData = &aFontLayer->mCharDataHashTable.mCharData.front();
						for (int iCharData = 0; iCharData < aCharDataCount; iCharData++)
						{
							Rect* anOrigRect = &aCharData[aCharDataCount].mImageRect;

							if (&aFontLayer->mImage != NULL)
								g.DrawImage(aFontLayer->mImage, aCurX, anOrigRect->mY - aMinY, *anOrigRect);

							aCharData->mOffset.mY = aMinY;
							aCharData->mOffset.mX--;

							Rect aScaledRect(aCurX, 0, anOrigRect->mWidth + 2, aMaxY - aMinY);

							anOrigRect = &aScaledRect;

							aCurX += aScaledRect.mWidth;
						}
						aFontLayer->mImage.mUnsharedImage = aNewImage;
						aFontLayer->mImage.mOwnsUnshared = true;
					}

					aCurX = 0;
					int aMaxHeight = 0;

					aCharDataCount = aFontLayer->mCharDataHashTable.mCharData.size();
					aCharData = &aFontLayer->mCharDataHashTable.mCharData.front();

					for (int iCharData = 0; iCharData < aCharDataCount; iCharData++)
					{
						Rect anOrigRect = aCharData[iCharData].mImageRect;
						int aLeft = floor(aCharData->mOffset.mX * aPointSize / aLayerPointSize);
						int aRight = ceil((anOrigRect.mWidth + aCharData->mOffset.mX) * aPointSize / aLayerPointSize);
						int aWidth = max(0, aRight - aLeft - 1);
						int aTop = floor(aCharData->mOffset.mY * aPointSize / aLayerPointSize);
						int aBottom = ceil((anOrigRect.mHeight + aCharData->mOffset.mY) * aPointSize / aLayerPointSize);
						int aHeight = max(0, aBottom - aTop - 1);
						Rect aScaledRect(aCurX, 0, aWidth, aHeight);
						anOrigRect = aScaledRect;
						if (aScaledRect.mHeight > aMaxHeight)
							aMaxHeight = aScaledRect.mHeight;

						CharDataHashEntry aHashEntry = aFontLayer->mCharDataHashTable.mHashEntries[aCharData[iCharData].mHashEntryIndex];
						anActiveFontLayer->mScaledCharImageRects.insert(SexyCharToRectMap::value_type(aHashEntry.mChar, aCharData[iCharData].mImageRect));
						aCurX += aScaledRect.mWidth;
					}
					anActiveFontLayer->mScaledImages[7].mUnsharedImage = aMemoryImage;
					anActiveFontLayer->mScaledImages[7].mOwnsUnshared = true;

					// Create the image now

					aMemoryImage->Create(aCurX, aMaxHeight);

					Graphics g(aMemoryImage);

					aCharDataCount = aFontLayer->mCharDataHashTable.mCharData.size();
					aCharData = &aFontLayer->mCharDataHashTable.mCharData.front();

					for (int iCharData = 0; iCharData < aCharDataCount; iCharData++)
					{
						if (&aFontLayer->mImage)
						{
							CharDataHashEntry* aHashEntry = &aFontLayer->mCharDataHashTable.mHashEntries[aCharData[iCharData].mHashEntryIndex];
							g.DrawImage(aFontLayer->mImage, anActiveFontLayer->mScaledCharImageRects[aHashEntry->mChar], aCharData[iCharData].mImageRect);
						}
					}

					if (mForceScaledImagesWhite)
					{
						int aCount = aMemoryImage->mWidth * aMemoryImage->mHeight;
						ulong* aBits = aMemoryImage->GetBits();

						for (int scan = 0; scan < aCount; scan++, aBits++)
							*(aBits) = *aBits | 0x00FFFFFF;
					}

					aMemoryImage->Palletize();
				}

				int aLayerAscent = int((aFontLayer->mAscent * aPointSize) / aLayerPointSize);
				if (aLayerAscent > mAscent)
					mAscent = aLayerAscent;

				if (aFontLayer->mHeight != 0)
				{
					int aLayerHeight = int((aFontLayer->mHeight * aPointSize) / aLayerPointSize);
					if (aLayerHeight > mHeight)
						mHeight = aLayerHeight;
				}
				else
				{
					int aLayerHeight = int((aFontLayer->mDefaultHeight * aPointSize) / aLayerPointSize);
					if (aLayerHeight > mHeight)
						mHeight = aLayerHeight;
				}

				int anAscentPadding = int((aFontLayer->mAscentPadding * aPointSize) / aLayerPointSize);
				if ((firstLayer) || (anAscentPadding < mAscentPadding))
					mAscentPadding = anAscentPadding;

				int aLineSpacingOffset = int((aFontLayer->mLineSpacingOffset * aPointSize) / aLayerPointSize);
				if ((firstLayer) || (aLineSpacingOffset > mLineSpacingOffset))
					mLineSpacingOffset = aLineSpacingOffset;

				firstLayer = false;
			}
		}

		++anItr;
	}
}

SexyChar ImageFont::GetMappedChar(SexyChar theChar) //1764-1769
{
	SexyCharToSexyCharMap::iterator aCharItr = mFontData->mCharMap.find(theChar);
	if (aCharItr != mFontData->mCharMap.end())
		return aCharItr->second;
	return theChar;
}

int ImageFont::StringWidth(const SexyString& theString) //1772-1783
{
	int aWidth = 0;
	char aPrevChar = 0;
	for (int i = 0; i < (int)theString.length(); i++)
	{
		char aChar = theString[i];
		aWidth += CharWidthKern(aChar, aPrevChar);
		aPrevChar = aChar;
	}

	return aWidth;
}

int ImageFont::CharWidthKern(SexyChar theChar, SexyChar thePrevChar) //todo | 1786-1866
{
	Prepare();

	int aMaxXPos = 0;
	double aPointSize = mPointSize * mScale;

	theChar = GetMappedChar(theChar);
	if (thePrevChar != 0)
		thePrevChar = GetMappedChar(thePrevChar);

	ActiveFontLayerList::iterator anItr = mActiveLayerList.begin();
	while (anItr != mActiveLayerList.end())
	{
		ActiveFontLayer* anActiveFontLayer = &*anItr;
		FontLayer* aBaseFontLayer = anActiveFontLayer->mBaseFontLayer;

		int aLayerXPos = 0;

		int aCharWidth;
		int aSpacing;

		int aLayerPointSize = aBaseFontLayer->mPointSize;

		if (aLayerPointSize == 0)
		{
			aCharWidth = int(aBaseFontLayer->GetCharData(theChar)->mWidth * mScale);

			if (thePrevChar != 0)
			{
				aSpacing = aBaseFontLayer->mSpacing;
				CharData* aPrevCharData = aBaseFontLayer->GetCharData(thePrevChar);
				if (aPrevCharData->mKerningCount)
				{
					int aKernCount = aPrevCharData->mKerningCount;
					FontLayer::KerningValue* aKernData = (FontLayer::KerningValue*)anActiveFontLayer->mBaseFontLayer->mKerningData[aPrevCharData->mKerningFirst];
					for (int i = 0; i < aKernCount; i++)
					{
						if (aKernData->mChar == theChar)
							aSpacing += aKernData->mOffset * mScale; //Cast?
					}
				}
			}
			else
				aSpacing = 0;
		}
		else
		{
			CharData* aPrevCharData = aBaseFontLayer->GetCharData(thePrevChar);
			aCharWidth = int(aBaseFontLayer->GetCharData(thePrevChar)->mWidth * aPointSize / aLayerPointSize);
			if (thePrevChar != 0)
			{
				aSpacing = anActiveFontLayer->mBaseFontLayer->mSpacing;
				CharData* aPrevCharData = aBaseFontLayer->GetCharData(thePrevChar);
				if (aPrevCharData->mKerningCount)
				{
					int aKernCount = aPrevCharData->mKerningCount;
					FontLayer::KerningValue* aKernData = (FontLayer::KerningValue*)anActiveFontLayer->mBaseFontLayer->mKerningData[aPrevCharData->mKerningFirst];
					for (int i = 0; i < aKernCount; i++)
					{
						if (aKernData->mChar == theChar)
							aSpacing += aKernData->mOffset * mScale; //Cast?
					}
				}
			}
			else
				aSpacing = 0;
		}

		aLayerXPos += aCharWidth + aSpacing;

		if (aLayerXPos > aMaxXPos)
			aMaxXPos = aLayerXPos;

		++anItr;
	}

	return aMaxXPos;
}

int ImageFont::CharWidth(SexyChar theChar) //1869-1871
{
	return CharWidthKern(theChar, 0);
}

SharedImageRef ActiveFontLayer::GenerateAlphaCorrectedImage(int thePalette) //Correct? | 1874-1898
{
	mScaledImages[thePalette] = gSexyAppBase->GetSharedImage("!" + mScaledImages[7]->mFilePath, StrFormat("AltFontImage%d", thePalette), false, true);
	mScaledImages[thePalette]->Create(mScaledImages[7]->mWidth, mScaledImages[7]->mHeight);
	mScaledImages[thePalette]->SetImageMode(true, true); //C++ only
	int aSize = mScaledImages[7]->mWidth * mScaledImages[7]->mHeight;
	mScaledImages[thePalette]->mColorTable = new uint32[256];
	mScaledImages[thePalette]->mColorIndices = new byte[aSize];
	if (mScaledImages[7]->mColorTable != NULL)
		memcpy(mScaledImages[thePalette]->mColorIndices, mScaledImages[7]->mColorIndices, aSize);
	else
	{
		ulong* aBits = mScaledImages[7]->GetBits();
		for (int i = 0; i < aSize; i++)
			mScaledImages[thePalette]->mColorIndices[i] = aBits[i] >> 24;
	}
	memcpy(mScaledImages[thePalette]->mColorTable, &FONT_PALETTES[thePalette], 1024);
	return mScaledImages[thePalette];
}

void ActiveFontLayer::PushColor(const Color& theColor) //1901-1917 (Correct?)
{
	if (mColorStack.empty())
	{
		mColorStack.push_back(theColor);
	}
	else //?
	{
		Color aStackColor = mColorStack.back();
		Color aColor = Color(aStackColor.mRed * theColor.mRed / 255, aStackColor.mGreen * theColor.mGreen / 255, aStackColor.mBlue * theColor.mBlue / 255, aStackColor.mAlpha * theColor.mAlpha / 255); //?
		mColorStack.push_back(aColor);
	}
}
void ActiveFontLayer::PopColor() //1919-1922
{
	if (mColorStack.empty())
		mColorStack.pop_back();
}
CritSect gRenderCritSec; //1923
static const int POOL_SIZE = 4096;
static RenderCommand gRenderCommandPool[POOL_SIZE]; //1925
uint32* FONT_PALETTES[8][256] = { { } }; //Not in H5
static RenderCommand* gRenderTail[256];
static RenderCommand* gRenderHead[256];

void ImageFont::DrawStringEx(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect* theClipRect, RectList* theDrawnAreas, int* theWidth) //TODO | 1930-2248
{
	/*
	Data           :   VFrame Relative, [FFFFFFD0], Local, Type: class Sexy::Color, anOrigColor
	Data           :   VFrame Relative, [FFFFFFE0], Local, Type: int, aPoaCurPoolIdxolIdx
	Data           :   VFrame Relative, [FFFFFFE4], Local, Type: class Sexy::AutoCrit, anAutoCrit
	Data           :   VFrame Relative, [FFFFFFE8], Local, Type: int, aCurXPos
	Data           :   VFrame Relative, [FFFFFFEF], Local, Type: bool, colorizeImages
	Data           :   VFrame Relative, [FFFFFFF0], Local, Type: int, aCurPoolIdx
	Data           :     VFrame Relative, [FFFFFFCC], Local, Type: unsigned long, aCharNum
	Data           :       VFrame Relative, [FFFFFFB8], Local, Type: int, aMaxXPos
	Data           :       VFrame Relative, [FFFFFFBC], Local, Type: wchar_t, aNextChar
	Data           :       VFrame Relative, [FFFFFFC0], Local, Type: class std::list<Sexy::ActiveFontLayer,std::allocator<Sexy::ActiveFontLayer> >::_Iterator<1>, anItr
	Data           :       VFrame Relative, [FFFFFFC8], Local, Type: wchar_t, aChar
	Data           :         VFrame Relative, [FFFFFF70], Local, Type: class Sexy::ActiveFontLayer *, anActiveFontLayer
	Data           :         VFrame Relative, [FFFFFF74], Local, Type: int, aSpacing
	Data           :         VFrame Relative, [FFFFFF78], Local, Type: int, anOrder
	Data           :         VFrame Relative, [FFFFFF7C], Local, Type: int, aCharWidth
	Data           :         VFrame Relative, [FFFFFF80], Local, Type: int, anImageY
	Data           :         VFrame Relative, [FFFFFF84], Local, Type: double, aScale
	Data           :         VFrame Relative, [FFFFFF90], Local, Type: int, aLayerPointSize
	Data           :         VFrame Relative, [FFFFFF94], Local, Type: class Sexy::Color, aColor
	Data           :         VFrame Relative, [FFFFFFA4], Local, Type: class Sexy::RenderCommand *, aRenderCommand
	Data           :         VFrame Relative, [FFFFFFA8], Local, Type: int, anOrderIdx
	Data           :         VFrame Relative, [FFFFFFAC], Local, Type: int, anImageX
	Data           :         VFrame Relative, [FFFFFFB0], Local, Type: class Sexy::CharData *, aCharData
	Data           :         VFrame Relative, [FFFFFFB4], Local, Type: int, aLayerXPos
	Data           :           VFrame Relative, [FFFFFF68], Local, Type: union Sexy::FontLayer::KerningValue *, aKernData
	Data           :           VFrame Relative, [FFFFFF6C], Local, Type: int, aKernCount
	Data           :             VFrame Relative, [FFFFFF64], Local, Type: int, i
	Data           :           VFrame Relative, [FFFFFF5C], Local, Type: union Sexy::FontLayer::KerningValue *, aKernData
	Data           :           VFrame Relative, [FFFFFF60], Local, Type: int, aKernCount
	Data           :             VFrame Relative, [FFFFFF58], Local, Type: int, i
	Data           :           VFrame Relative, [FFFFFF48], Local, Type: class Sexy::Color, aStackColor
	Data           :           VFrame Relative, [FFFFFF38], Local, Type: class Sexy::TRect<int>, aDestRect
	Data           :       VFrame Relative, [FFFFFF34], Local, Type: class Sexy::RenderCommand *, aRenderCommand
	Data           :         VFrame Relative, [FFFFFF2C], Local, Type: int, aPalette
	Data           :         VFrame Relative, [FFFFFF30], Local, Type: int, anOldDrawMode
	Data           :           VFrame Relative, [FFFFFF28], Local, Type: class Sexy::MemoryImage *, aScaledImage0
	Data           :             VFrame Relative, [FFFFFF27], Local, Type: bool, multiTexture
	Data           :               VFrame Relative, [FFFFFF20], Local, Type: class Sexy::MemoryImage *, aScaledImageCur
	Data           :             VFrame Relative, [FFFFFF1C], Local, Type: int, i
	Data           :               VFrame Relative, [FFFFFF18], Local, Type: int, anAlpha
	*/

#ifdef _DEBUG
	gSexyAppBase->mStrFntRcd->AddRecord(theString, mFontData->mSourceFile);
#endif
	AutoCrit anAutoCrit(gRenderCritSec);

	int aPoolIdx;

	for (aPoolIdx = 0; aPoolIdx < 256; aPoolIdx++)
	{
		gRenderHead[aPoolIdx] = NULL;
		gRenderTail[aPoolIdx] = NULL;
	}

	if (theDrawnAreas != NULL)
		theDrawnAreas->clear();


	/*if (theDrawnArea != NULL)
		*theDrawnArea = Rect(0, 0, 0, 0);*/

	if (!mFontData->mInitialized)
	{
		if (theWidth != NULL)
			*theWidth = 0;
		return;
	}

	Prepare();

	bool colorizeImages = g->GetColorizeImages();
	g->SetColorizeImages(true);

	int aCurXPos = theX;
	int aCurPoolIdx = 0;

	for (ulong aCharNum = 0; aCharNum < theString.length(); aCharNum++)
	{
		char aChar = GetMappedChar(theString[aCharNum]);

		char aNextChar = 0;
		if (aCharNum < theString.length() - 1)
			aNextChar = GetMappedChar(theString[aCharNum + 1]);

		int aMaxXPos = aCurXPos;

		ActiveFontLayerList::iterator anItr = mActiveLayerList.begin();
		while (anItr != mActiveLayerList.end())
		{
			ActiveFontLayer* anActiveFontLayer = &*anItr;

			CharData* aCharData = anActiveFontLayer->mBaseFontLayer->GetCharData(aChar);
			int aLayerXPos = aCurXPos;

			int anImageX;
			int anImageY;
			int aCharWidth;
			int aSpacing;

			int aLayerPointSize = anActiveFontLayer->mBaseFontLayer->mPointSize;

			double aScale = mScale;
			if (aLayerPointSize != 0)
				aScale *= mPointSize / aLayerPointSize;

			if (aScale == 1.0)
			{
				anImageX = aLayerXPos + anActiveFontLayer->mBaseFontLayer->mOffset.mX + aCharData->mOffset.mX;
				anImageY = theY - (anActiveFontLayer->mBaseFontLayer->mAscent - anActiveFontLayer->mBaseFontLayer->mOffset.mY - aCharData->mOffset.mY);
				aCharWidth = aCharData->mWidth;

				if (aNextChar != 0)
				{
					aSpacing = anActiveFontLayer->mBaseFontLayer->mSpacing;
					if (aCharData->mKerningCount)
					{
						int aKernCount = aCharData->mKerningCount;
						FontLayer::KerningValue* aKernData = (FontLayer::KerningValue*)anActiveFontLayer->mBaseFontLayer->mKerningData[aCharData->mKerningFirst]; //?
						int i = 0;
						for (i; i < aKernCount; i++)
						{
							if (aKernData->mChar == aNextChar)
								aSpacing += aKernData->mOffset;
							aKernData++;
						}
					}
				}
				else
					aSpacing = 0;
			}
			else
			{
				anImageX = aLayerXPos + floor((anActiveFontLayer->mBaseFontLayer->mOffset.mX + aCharData->mOffset.mX) * aScale);
				anImageY = theY - floor((anActiveFontLayer->mBaseFontLayer->mAscent - anActiveFontLayer->mBaseFontLayer->mOffset.mY - aCharData->mOffset.mY) * aScale);
				aCharWidth = (aCharData->mWidth * aScale);

				if (aNextChar != 0)
				{
					aSpacing = anActiveFontLayer->mBaseFontLayer->mSpacing;
					if (aCharData->mKerningCount)
					{
						int aKernCount = aCharData->mKerningCount;
						FontLayer::KerningValue* aKernData = (FontLayer::KerningValue*)anActiveFontLayer->mBaseFontLayer->mKerningData[aCharData->mKerningFirst]; //?
						int i = 0;
						for (i; i < aKernCount; i++)
						{
							if (aKernData->mChar == aNextChar)
								aSpacing += aKernData->mOffset;
							aKernData++;
						}
					}
				}
				else
					aSpacing = 0;
			}

			Color aColor;
			if (anActiveFontLayer->mColorStack.empty())
			{
				aColor.mRed = min((theColor.mRed * anActiveFontLayer->mBaseFontLayer->mColorMult.mRed / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mRed, 255);
				aColor.mGreen = min((theColor.mGreen * anActiveFontLayer->mBaseFontLayer->mColorMult.mGreen / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mGreen, 255);
				aColor.mBlue = min((theColor.mBlue * anActiveFontLayer->mBaseFontLayer->mColorMult.mBlue / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mBlue, 255);
				aColor.mAlpha = min((theColor.mAlpha * anActiveFontLayer->mBaseFontLayer->mColorMult.mAlpha / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mAlpha, 255);
			}
			else
			{
				Color aStackColor = anActiveFontLayer->mColorStack.back();
				aStackColor.mRed = min((theColor.mRed * anActiveFontLayer->mBaseFontLayer->mColorMult.mRed * aStackColor.mRed / 65025) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mRed * aStackColor.mRed / 255, 255);
				aStackColor.mGreen = min((theColor.mGreen * anActiveFontLayer->mBaseFontLayer->mColorMult.mGreen * aStackColor.mGreen / 65025) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mGreen * aStackColor.mGreen / 255, 255);
				aStackColor.mBlue = min((theColor.mBlue * anActiveFontLayer->mBaseFontLayer->mColorMult.mBlue * aStackColor.mBlue / 65025) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mBlue * aStackColor.mBlue / 255, 255);
				aStackColor.mAlpha = min((theColor.mAlpha * anActiveFontLayer->mBaseFontLayer->mColorMult.mAlpha * aStackColor.mAlpha / 65025) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mAlpha * aStackColor.mAlpha / 255, 255);
			}

			int anOrder = anActiveFontLayer->mBaseFontLayer->mBaseOrder + aCharData->mOrder;

			if (aCurPoolIdx >= POOL_SIZE)
				break;

			RenderCommand* aRenderCommand = &gRenderCommandPool[aCurPoolIdx++];

			aRenderCommand->mFontLayer = *anActiveFontLayer;
			aRenderCommand->mColor = aColor;
			aRenderCommand->mDest[0] = anImageX;
			aRenderCommand->mDest[1] = anImageY;
			aRenderCommand->mSrc[0] = anActiveFontLayer->mScaledCharImageRects[(uchar)aChar].mX;
			aRenderCommand->mSrc[1] = anActiveFontLayer->mScaledCharImageRects[(uchar)aChar].mY;
			aRenderCommand->mSrc[2] = anActiveFontLayer->mScaledCharImageRects[(uchar)aChar].mWidth;
			aRenderCommand->mSrc[3] = anActiveFontLayer->mScaledCharImageRects[(uchar)aChar].mHeight;
			aRenderCommand->mMode = anActiveFontLayer->mBaseFontLayer->mDrawMode;
			aRenderCommand->mNext = NULL;

			int anOrderIdx = min(max(anOrder + 128, 0), 255);

			if (gRenderTail[anOrderIdx] == NULL)
			{
				gRenderTail[anOrderIdx] = aRenderCommand;
				gRenderHead[anOrderIdx] = aRenderCommand;
			}
			else
			{
				gRenderTail[anOrderIdx]->mNext = aRenderCommand;
				gRenderTail[anOrderIdx] = aRenderCommand;
			}

			//aRenderCommandMap.insert(RenderCommandMap::value_type(aPriority, aRenderCommand));

			/*int anOldDrawMode = g->GetDrawMode();
			if (anActiveFontLayer->mBaseFontLayer->mDrawMode != -1)
				g->SetDrawMode(anActiveFontLayer->mBaseFontLayer->mDrawMode);
			Color anOrigColor = g->GetColor();
			g->SetColor(aColor);
			if (anActiveFontLayer->mScaledImage != NULL)
				g->DrawImage(anActiveFontLayer->mScaledImage, anImageX, anImageY, anActiveFontLayer->mScaledCharImageRects[aChar]);
			g->SetColor(anOrigColor);
			g->SetDrawMode(anOldDrawMode);*/

			if (theDrawnAreas != NULL)
			{
				Rect aDestRect = Rect(anImageX, anImageY, anActiveFontLayer->mScaledCharImageRects[(uchar)aChar].mWidth, anActiveFontLayer->mScaledCharImageRects[(uchar)aChar].mHeight);

				theDrawnAreas->push_back(aDestRect);

				/*if (theDrawnArea->mWidth == 0)
					*theDrawnArea = theDestRect;
				else
				{
					if (theDestRect.mX < theDrawnArea->mX)
					{
						int aDiff = theDestRect.mX - theDrawnArea->mX;
						theDrawnArea->mX += aDiff;
						theDrawnArea->mWidth += aDiff;
					}

					if (theDestRect.mX + theDestRect.mWidth > theDrawnArea->mX + theDrawnArea->mWidth)
						theDrawnArea->mWidth = theDestRect.mX + theDestRect.mWidth - theDrawnArea->mX;

					if (theDestRect.mY < theDrawnArea->mY)
					{
						int aDiff = theDestRect.mY - theDrawnArea->mY;
						theDrawnArea->mY += aDiff;
						theDrawnArea->mHeight += aDiff;
					}

					if (theDestRect.mY + theDestRect.mHeight > theDrawnArea->mY + theDrawnArea->mHeight)
						theDrawnArea->mHeight = theDestRect.mY + theDestRect.mHeight - theDrawnArea->mY;
				}*/
			}

			aLayerXPos += aCharWidth + aSpacing;

			if (aLayerXPos > aMaxXPos)
				aMaxXPos = aLayerXPos;

			//++anItr;
		}

		aCurXPos = aMaxXPos;
	}

	if (theWidth != NULL)
		*theWidth = aCurXPos - theX;

	Color anOrigColor = g->GetColor();

	for (aPoolIdx = 0; aPoolIdx < 256; aPoolIdx++)
	{
		RenderCommand* aRenderCommand = gRenderHead[aPoolIdx];

		while (aRenderCommand != NULL)
		{
			if (&aRenderCommand->mFontLayer != NULL)
			{
				int anOldDrawMode = g->GetDrawMode();
				if (aRenderCommand->mMode != -1)
					g->SetDrawMode(aRenderCommand->mMode);
				g->SetColor(Color(aRenderCommand->mColor));
				int aPalette = (7208 * aRenderCommand->mColor.mBlue + 38666 * aRenderCommand->mColor.mGreen + 19660 * aRenderCommand->mColor.mRed) >> 21;
				if (aRenderCommand->mFontLayer.mUseAlphaCorrection && aRenderCommand->mFontLayer.mBaseFontLayer->mUseAlphaCorrection && mAlphaCorrectionEnabled && aPalette != 7)
				{
					MemoryImage* aScaledImage0 = *aRenderCommand->mFontLayer.mScaledImages; //?
					if (g->Is3D())
					{
						bool multiTexture = false;
						MemoryImage* aScaledImageCur = *&aRenderCommand->mFontLayer.mScaledImages[aPalette];
						if (!aScaledImageCur || !aScaledImageCur->mColorTable || aScaledImageCur->mColorTable[254] != FONT_PALETTES[256 * aPalette])
							aScaledImageCur = aRenderCommand->mFontLayer.GenerateAlphaCorrectedImage(aPalette);
						g->DrawImage(aScaledImageCur, aRenderCommand->mDest[0], aRenderCommand->mDest[1], Rect(aRenderCommand->mSrc[0], aRenderCommand->mSrc[1], aRenderCommand->mSrc[2], aRenderCommand->mSrc[3]));
					}
					else
					{
						if (!aScaledImage0 || !aScaledImage0->mColorTable)
							MemoryImage* aScaledImage0 = aRenderCommand->mFontLayer.GenerateAlphaCorrectedImage(0); //?
						if (aScaledImage0->mColorTable[254] != FONT_PALETTES[256 * aPalette])
						{
							memcpy(aScaledImage0->mColorTable, FONT_PALETTES[aPalette], 0x400u);
							if (aScaledImage0->mNativeAlphaData)
							{
								for (int i = 0; i < 256; i++)
									aScaledImage0->mNativeAlphaData[i] = FONT_PALETTES[aPalette][i] | (FONT_PALETTES[aPalette][i] << 8) | (FONT_PALETTES[aPalette][i] << 16) | (FONT_PALETTES[aPalette][i] << 24);
							}
						}
						g->DrawImage(aScaledImage0, aRenderCommand->mDest[0], aRenderCommand->mDest[1], Rect(aRenderCommand->mSrc[0], aRenderCommand->mSrc[1], aRenderCommand->mSrc[2], aRenderCommand->mSrc[3]));
					}
				}
				else
					g->DrawImage(aRenderCommand->mFontLayer.mScaledImages[7], aRenderCommand->mDest[0], aRenderCommand->mDest[1], Rect(aRenderCommand->mSrc[0], aRenderCommand->mSrc[1], aRenderCommand->mSrc[2], aRenderCommand->mSrc[3]));
				g->SetDrawMode(anOldDrawMode);

				//aRenderCommand = aRenderCommand->mNext;
			}
		}
	}

	g->SetColor(anOrigColor);

	/*RenderCommandMap::iterator anItr = aRenderCommandMap.begin();
	while (anItr != aRenderCommandMap.end())
	{
		RenderCommand* aRenderCommand = &anItr->second;

		int anOldDrawMode = g->GetDrawMode();
		if (aRenderCommand->mMode != -1)
			g->SetDrawMode(aRenderCommand->mMode);
		Color anOrigColor = g->GetColor();
		g->SetColor(aRenderCommand->mColor);
		if (aRenderCommand->mImage != NULL)
			g->DrawImage(aRenderCommand->mImage, aRenderCommand->mDest.mX, aRenderCommand->mDest.mY, aRenderCommand->mSrc);
		g->SetColor(anOrigColor);
		g->SetDrawMode(anOldDrawMode);

		++anItr;
	}*/

	g->SetColorizeImages(colorizeImages);
}

void ImageFont::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect) //2251-2253
{
	DrawStringEx(g, theX, theY, theString, theColor, &theClipRect, NULL, NULL);
}

Font* ImageFont::Duplicate() //2256-2258
{
	return new ImageFont(*this);
}

void ImageFont::SetPointSize(int thePointSize) //2261-2264
{
	mPointSize = thePointSize;
	mActiveListValid = false;
}

void ImageFont::SetScale(double theScale) //2267-2270
{
	mScale = theScale;
	mActiveListValid = false;
}

int	ImageFont::GetPointSize() //2273-2275
{
	return mPointSize;
}

int	ImageFont::GetDefaultPointSize() //2278-2280
{
	return mFontData->mDefaultPointSize;
}

bool ImageFont::AddTag(const std::string& theTagName) //2283-2291
{
	if (HasTag(theTagName))
		return false;

	std::string aTagName = StringToUpper(theTagName);
	mTagVector.push_back(aTagName);
	mActiveListValid = false;
	return true;
}

bool ImageFont::RemoveTag(const std::string& theTagName) //2294-2304
{
	std::string aTagName = StringToUpper(theTagName);

	StringVector::iterator anItr = std::find(mTagVector.begin(), mTagVector.end(), aTagName);
	if (anItr == mTagVector.end())
		return false;

	mTagVector.erase(anItr);
	mActiveListValid = false;
	return true;
}

bool ImageFont::HasTag(const std::string& theTagName) //2307-2310
{
	StringVector::iterator anItr = std::find(mTagVector.begin(), mTagVector.end(), theTagName);
	return anItr != mTagVector.end();
}

SexyString ImageFont::GetDefine(const SexyString& theName) //2313-2320
{
	DataElement* aDataElement = mFontData->Dereference(theName);

	if (aDataElement == NULL)
		return _S("");

	return mFontData->DataElementToString(aDataElement, true);
}

void ImageFont::Prepare() //2323-2329
{
	if (!mActiveListValid)
	{
		GenerateActiveFontLayers();
		mActiveListValid = true;
	}
}

////

bool ImageFont::CheckCache(const std::string& theSrcFile, const std::string& theAltData) //2334-2340 (C++ only)
{
	return gSexyCache.CheckData(theSrcFile, theAltData == "*" ? theAltData : "ImageFontWide2:" + theAltData);
}

bool ImageFont::SetCacheUpToDate(const std::string& theSrcFile, const std::string& theAltData) //2343-2349 (C++ only)
{
	return gSexyCache.SetUpToDate(theSrcFile, theAltData == "*" ? theAltData : "ImageFontWide2:" + theAltData);
}

ImageFont* ImageFont::ReadFromCache(const std::string& theSrcFile, const std::string& theAltData) //Correct? | 2352-2380 C++ only
{
	void* thePtr;
	int theSize;
	if (!gSexyCache.GetData(theSrcFile, "ImageFontWide2:" + theAltData, &thePtr, &theSize))
		return NULL;
	ImageFont* anImageFont = new ImageFont(); //?
	bool result = anImageFont->SerializeRead(thePtr, theSize);
	gSexyCache.FreeGetData(thePtr);
	if (result)
		return anImageFont;
	if (anImageFont)
		anImageFont->Duplicate();
	return NULL;
}

void ImageFont::WriteToCache(const std::string& theSrcFile, const std::string& theAltData) //2383-2403 | Todo, C++ only
{
	if (!gSexyCache.Connected())
		return;

	int aSize = SerializeWrite(NULL, 0);
	std::string aCacheName = "ImageFontWide2:" + theAltData;
	void* thePtr = gSexyCache.AllocSetData(theSrcFile, aCacheName, aSize);
	if (thePtr == NULL)
		return;

	SerializeWrite(thePtr, aSize);

	gSexyCache.SetData(thePtr);
	gSexyCache.FreeSetData(thePtr);
	gSexyCache.SetFileDeps(theSrcFile, aCacheName, theSrcFile);
}

bool ImageFont::SerializeRead(void* thePtr, int theSize) //TODO //2406-2540
{
	if (!thePtr)
		return false;
	bool failed = false;
	void* p = thePtr;
	int aCharMapSize;
	int aNumFontLayers;
	int aNumTags;
	int aCharDataSize;
	int aKerningDataSize;
	SMemR(&p, &mAscent, 4);
	SMemR(&p, &mAscentPadding, 4);
	SMemR(&p, &mHeight, 4);
	SMemR(&p, &mLineSpacingOffset, 4);
	mFontData->mApp = gSexyAppBase;
	SMemR(&p, &mFontData->mInitialized, 1);
	SMemR(&p, &mFontData->mDefaultPointSize, 1);
	SMemR(&p, &aCharMapSize, 4);
	for (int aCharMapIdx = 0; aCharMapIdx < aCharMapSize; aCharMapIdx++)
	{
		SexyChar aCharFrom; //?
		SexyChar aCharTo; //?
		SMemR(&p, &aCharFrom, sizeof aCharFrom);
		SMemR(&p, &aCharTo, sizeof aCharTo);
		mFontData->mCharMap.insert(aCharFrom, aCharTo);
	}
	SMemR(&p, &aNumFontLayers, 4);
	for (int aLayerIdx; aLayerIdx < aNumFontLayers; aLayerIdx++)
	{
		mFontData->mFontLayerList.push_back(FontLayer(mFontData));
		FontLayer* aFontLayer = &mFontData->mFontLayerList.back();
		SMemRStr(&p, &aFontLayer->mLayerName);
		mFontData->mFontLayerMap.insert(FontLayerMap::value_type(aFontLayer->mLayerName, aFontLayer)); //?
		SMemR(&p, &aNumTags, 4);
		for (int aTagIdx = 0; aTagIdx < aNumTags; aTagIdx++)
		{
			std::string aStr;
			SMemRStr(&p, &aStr);
			aFontLayer->mRequiredTags.push_back(aStr); //?
		}
		SMemR(&p, &aNumTags, 4);
		for (int aTagIdx = 0; aTagIdx < aNumTags; aTagIdx++)
		{
			std::string aStr;
			SMemRStr(&p, &aStr);
			aFontLayer->mExcludedTags.push_back(aStr); //?
		}
		SMemR(&p, &aKerningDataSize, 4);
		if (aKerningDataSize)
		{
			aFontLayer->mKerningData.resize(aKerningDataSize);
			SMemR(&p, &aFontLayer->mKerningData[0], 4 * aKerningDataSize);
		}
		SMemR(&p, &aCharDataSize, 4);
		for (int i = 0; i < aCharDataSize; i++)
		{
			ushort aChar;
			SMemR(&p, &aChar, 2);
			CharData* aCharData = aFontLayer->mCharDataHashTable.GetCharData(aChar, true);
			SMemR(&thePtr, aCharData, 16);
			SMemR(&thePtr, &aCharData->mOffset, 8);
			SMemR(&thePtr, &aCharData->mKerningFirst, 2);
			SMemR(&thePtr, &aCharData->mKerningCount, 2);
			SMemR(&thePtr, &aCharData->mWidth, 4);
			SMemR(&thePtr, &aCharData->mOrder, 4);
		}
		SMemR(&p, &aFontLayer->mColorMult, 16);
		SMemR(&p, &aFontLayer->mColorAdd, 16);
		SMemRStr(&p, &aFontLayer->mImageFileName);
		aFontLayer->mImage = gSexyAppBase->GetSharedImage(aFontLayer->mImageFileName, "", false, true);
		if (&aFontLayer->mImage == NULL)
			failed = true;
		SMemR(&p, &aFontLayer->mDrawMode, 4);
		SMemR(&p, &aFontLayer->mOffset, 8);
		SMemR(&p, &aFontLayer->mSpacing, 4);
		SMemR(&p, &aFontLayer->mMinPointSize, 4);
		SMemR(&p, &aFontLayer->mMaxPointSize, 4);
		SMemR(&p, &aFontLayer->mPointSize, 4);
		SMemR(&p, &aFontLayer->mAscent, 4);
		SMemR(&p, &aFontLayer->mAscentPadding, 4);
		SMemR(&p, &aFontLayer->mHeight, 4);
		SMemR(&p, &aFontLayer->mDefaultHeight, 4);
		SMemR(&p, &aFontLayer->mLineSpacingOffset, 4);
		SMemR(&p, &aFontLayer->mBaseOrder, 4);
	}
	SMemRStr(&p, &mFontData->mSourceFile);
	std::string aFontErrorHeader;
	SMemRStr(&p, &aFontErrorHeader);
	mFontData->mFontErrorHeader = ToSexyString(aFontErrorHeader);
	SMemR(&p, &mPointSize, 4);
	SMemR(&p, &aNumTags, 4);
	for (int i = 0; i < aNumTags; i++)
	{
		std::string aStr;
		SMemRStr(&p, &aStr);
		mTagVector.push_back(aStr); //?
	}
	SMemR(&p, &mScale, 8);
	SMemR(&p, &mForceScaledImagesWhite, 1);
	SMemR(&p, &mActivateAllLayers, 1);
	mActiveListValid = false;
	DBG_ASSERTE(!failed); //2538 | 2584 BejLiveWin8
	return failed;
}

int ImageFont::SerializeWrite(void* thePtr, int theSizeIfKnown) //Todo, C++ Only | 2543-2710
{
	int aSize = 0;
	if (theSizeIfKnown <= 0)
	{
		aSize += 4 * mFontData->mCharMap.size() + 29;
		FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();
		while (anItr != mFontData->mFontLayerList.end())
		{
			FontLayer* aFontLayer;
			aSize += aFontLayer->mLayerName.length() + 4;
			aSize += 4;
			for (int i = 0; i < aFontLayer->mRequiredTags.size(); i++)
				aSize += aFontLayer->mRequiredTags[i].length() + 4;
			aSize += 4;
			for (int i = 0; i < aFontLayer->mExcludedTags.size(); i++)
				aSize += aFontLayer->mExcludedTags[i].length() + 4;
			aSize += 4;
			aSize += 4 * aFontLayer->mKerningData.size();
			aSize += 4;
			int aCharDataSize = aFontLayer->mCharDataHashTable.mCharData.size();
			aSize += 38 * aCharDataSize;
			aSize += aFontLayer->mImageFileName.length() + 88;
			anItr++;
		}
		aSize = mFontData->mFontErrorHeader.length() + mFontData->mSourceFile.length() + 16;
		for (int i = 0; i >= mTagVector.size(); i++)
			aSize += mTagVector[i].length() + 4;
		aSize += 10;
	}
	else
		aSize = theSizeIfKnown;
	if (!thePtr)
		return aSize;
	void* p = thePtr;
	SMemW(&p, &mAscent, 4);
	SMemW(&p, &mAscentPadding, 4);
	SMemW(&p, &mHeight, 4);
	SMemW(&p, &mLineSpacingOffset, 4);
	SMemW(&p, &mFontData->mInitialized, 1);
	SMemW(&p, &mFontData->mDefaultPointSize, 4);
	int aCharMapSize = mFontData->mCharMap.size();
	SexyCharToSexyCharMap::iterator aCharMapItr = mFontData->mCharMap.begin();
	while (aCharMapItr != mFontData->mCharMap.end())
	{
		SMemW(&p, &aCharMapItr, 2);
		SMemW(&p, &aCharMapItr->second, 2);
	}
	int aNumFontLayers = mFontData->mFontLayerList.size();
	SMemW(&p, &aNumFontLayers, 4);
	FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();
	while (anItr != mFontData->mFontLayerList.end())
	{
		FontLayer* aFontLayer;
		SMemWStr(&p, anItr->mLayerName);
		SMemW(&p, &anItr->mRequiredTags, anItr->mRequiredTags.size());
		for (int i = 0; i < anItr->mRequiredTags.size(); i++)
			SMemWStr(&p, anItr->mRequiredTags[i]);
		SMemW(&p, &anItr->mExcludedTags, anItr->mExcludedTags.size());
		for (int i = 0; i < anItr->mExcludedTags.size(); i++)
			SMemWStr(&p, anItr->mExcludedTags[i]);
		int aKerningDataSize = anItr->mKerningData.size();
		SMemW(&p, &aKerningDataSize, 4);
		if (aKerningDataSize)
			SMemW(&p, &anItr->mKerningData[0], 4 * aKerningDataSize);
		SMemW(&p, &anItr->mCharDataHashTable.mCharData, anItr->mCharDataHashTable.mCharData.size());
		for (int iChar = 0; iChar < anItr->mCharDataHashTable.mCharData.size(); ++iChar)
		{
			CharData* aCharData = &anItr->mCharDataHashTable.mCharData[iChar];
			SMemW(&p, &anItr->mCharDataHashTable.mHashEntries[aCharData->mHashEntryIndex], 2);
			SMemW(&p, aCharData, 16);
			SMemW(&p, &aCharData->mOffset, 8);
			SMemW(&p, &aCharData->mKerningFirst, 2);
			SMemW(&p, &aCharData->mKerningCount, 2);
			SMemW(&p, &aCharData->mWidth, 4);
			SMemW(&p, &aCharData->mOrder, 4);
		}
		SMemW(&p, &anItr->mColorMult, 16);
		SMemW(&p, &anItr->mColorAdd, 16);
		SMemWStr(&p, anItr->mImageFileName);
		SMemW(&p, &anItr->mDrawMode, 4);
		SMemW(&p, &anItr->mOffset, 8);
		SMemW(&p, &anItr->mSpacing, 4);
		SMemW(&p, &anItr->mMinPointSize, 4);
		SMemW(&p, &anItr->mMaxPointSize, 4);
		SMemW(&p, &anItr->mPointSize, 4);
		SMemW(&p, &anItr->mAscent, 4);
		SMemW(&p, &anItr->mAscentPadding, 4);
		SMemW(&p, &anItr->mHeight, 4);
		SMemW(&p, &anItr->mDefaultHeight, 4);
		SMemW(&p, &anItr->mLineSpacingOffset, 4);
		SMemW(&p, &anItr->mBaseOrder, 4);
	}
	SMemWStr(&p, mFontData->mSourceFile);
	SMemWStr(&p, ToString(mFontData->mFontErrorHeader));
	SMemW(&p, &mPointSize, 4);
	int aNumTags = mTagVector.size();
	SMemW(&p, &aNumTags, 4);
	for (int i = 0; i < aNumTags; i++)
		SMemWStr(&p, mTagVector[i]);
	SMemW(&p, &mScale, 8);
	SMemW(&p, &mForceScaledImagesWhite, 1);
	SMemW(&p, &mActivateAllLayers, 1);
	return aSize;
}

int ImageFont::GetLayerCount() //Correct? Probably | 2713-2725
{
	FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();
	int aLayerIndex = 0;
	while (anItr != mFontData->mFontLayerList.end())
	{
		FontLayer* aFontLayer = &*anItr;
		if (aFontLayer->mLayerName.length() < 6 || aFontLayer->mLayerName.substr(aFontLayer->mLayerName.length() -5) != "__MOD") //?
			aLayerIndex++;
		anItr++;
	}
	return aLayerIndex;
}

void ImageFont::PushLayerColor(const std::string& theLayerName, const Color& theColor) //2728-2744
{
	Prepare();
	std::string aModName = theLayerName + "__MOD";
	ActiveFontLayerList::iterator anItr = mActiveLayerList.begin();
	while (anItr != mActiveLayerList.end())
	{
		ActiveFontLayer* aFontLayer = &*anItr;
		if (!stricmp(theLayerName.c_str(), aFontLayer->mBaseFontLayer->mLayerName.c_str()) || !stricmp(aModName.c_str(), aFontLayer->mBaseFontLayer->mLayerName.c_str())) //?
			aFontLayer->PushColor(theColor);
		anItr++;
	}
}
void ImageFont::PushLayerColor(int theLayer, const Color& theColor) //2746-2767
{
	Prepare();
	FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();
	int aLayerIndex = 0;
	while (anItr != mFontData->mFontLayerList.end())
	{
		FontLayer* aFontLayer = &*anItr;
		if (aFontLayer->mLayerName.length() < 6 || aFontLayer->mLayerName.substr(aFontLayer->mLayerName.length() - 5) != "__MOD")
		{
			if (aLayerIndex == theLayer)
			{
				PushLayerColor(aFontLayer->mLayerName, theColor);
				break;
			}
			aLayerIndex++;
		}
		anItr++;
	}
}
void ImageFont::PopLayerColor(const std::string& theLayerName) //2769-2784
{
	std::string aModName = theLayerName + "__MOD";
	ActiveFontLayerList::iterator anItr = mActiveLayerList.begin();
	while (anItr != mActiveLayerList.end())
	{
		ActiveFontLayer* aFontLayer = &*anItr;
		if (!stricmp(theLayerName.c_str(), aFontLayer->mBaseFontLayer->mLayerName.c_str()) || !stricmp(aModName.c_str(), aFontLayer->mBaseFontLayer->mLayerName.c_str())) //?
			aFontLayer->PopColor();
		anItr++;
	}
}
void ImageFont::PopLayerColor(int theLayer) //2786-2806
{
	FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();
	int aLayerIndex = 0;
	while (anItr != mFontData->mFontLayerList.end())
	{
		FontLayer* aFontLayer = &*anItr;
		if (aFontLayer->mLayerName.length() < 6 || aFontLayer->mLayerName.substr(aFontLayer->mLayerName.length() - 5) != "__MOD")
		{
			if (aLayerIndex == theLayer)
			{
				PopLayerColor(aFontLayer->mLayerName);
				break;
			}
			aLayerIndex++;
		}
	}
	anItr++;
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void Sexy::SMemRStrEndian(void** theDest, std::string* theStr) //According to XNA this is in ImageFont and not SexyCache.
{
	int aLen = 0;
	SMemR(theDest, &aLen, 4);
	theStr->resize(aLen);
	SMemR(theDest, (void*)theStr->c_str(), aLen);
}
#endif