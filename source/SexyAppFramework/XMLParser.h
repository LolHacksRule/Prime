#ifndef __XMLPARSER_H__
#define __XMLPARSER_H__

#include "Common.h"

#include "PerfTimer.h"

#include "EncodingParser.h"

namespace Sexy
{

typedef std::map<SexyString, SexyString>	XMLParamMap;
typedef std::list<XMLParamMap::iterator>	XMLParamMapIteratorList;

class XMLElement //Added mValueEncoded, mAttributesEncoded and mAttributeEncodedIteratorList
{
public:
	enum
	{
		TYPE_NONE,
		TYPE_START,
		TYPE_END,
		TYPE_ELEMENT,
		TYPE_INSTRUCTION,
		TYPE_COMMENT
	};
public:
	
	int						mType;
	SexyString				mSection;
	SexyString				mValue;
	SexyString				mInstruction;
	SexyString				mValueEncoded;
	XMLParamMap				mAttributes;
	XMLParamMap				mAttributesEncoded;
	XMLParamMapIteratorList	mAttributeIteratorList; // stores attribute iterators in their original order
	XMLParamMapIteratorList	mAttributeEncodedIteratorList;
};

class XMLParser : public EncodingParser
{
protected:
	std::string				mFileName;
	SexyString				mErrorText;
	int						mLineNum;
	bool					mHasFailed;
	bool					mAllowComments;
	SexyString				mSection;
	//Moved to EncodingParser

protected:
	void					Fail(const SexyString& theErrorText);
	void					Init();

	bool					AddAttribute(XMLElement* theElement, const SexyString& aAttributeKey, const SexyString& aAttributeValue);
	bool					AddAttributeEncoded(XMLElement* theElement, const SexyString& aAttributeKey, const SexyString& aAttributeValue);

public:
	XMLParser();
	~XMLParser();

	bool			OpenFile(const std::string& theFilename);
	virtual bool			NextElement(XMLElement* theElement);
	SexyString				GetErrorText();
	int						GetCurrentLineNum();
	std::string				GetFileName();

	inline void				AllowComments(bool doAllow) { mAllowComments = doAllow; }

	void			SetStringSource(const SexyString& theString);

	bool					HasFailed();
};

};

#endif //__XMLPARSER_H__
