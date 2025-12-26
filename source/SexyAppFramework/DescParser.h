#ifndef __DESCPARSER_H__
#define __DESCPARSER_H__

#include "Common.h"
#include "EncodingParser.h"

namespace Sexy
{

class DataElement 
{
public:	
	bool					mIsList;

public:
	DataElement();
	virtual ~DataElement();

	virtual DataElement*	Duplicate() = 0;
};

class SingleDataElement : public DataElement
{
public:
	SexyString				mString;	
	DataElement*			mValue;

public:
	SingleDataElement();
	SingleDataElement(const SexyString theString);
	~SingleDataElement();

	DataElement*	Duplicate();
};

typedef std::vector<DataElement*> ElementVector;

class ListDataElement : public DataElement
{
public:
	ElementVector			mElementVector;

public:
	ListDataElement();
	ListDataElement(const ListDataElement& theListDataElement);
	~ListDataElement();
	
	ListDataElement&		operator=(const ListDataElement& theListDataElement);

	DataElement*	Duplicate();
};

typedef std::map<SexyString, DataElement*> DataElementMap;
typedef std::vector<SexyString> SexyStringVector;
typedef std::vector<int> IntVector;
typedef std::vector<double> DoubleVector;

class DescParser : public EncodingParser //Not in H5
{
public:
	enum //ECMDSEP in XNA, prob autogen
	{
		CMDSEP_SEMICOLON = 1,
		CMDSEP_NO_INDENT = 2
	};

public:
	int						mCmdSep;

	SexyString				mError;
	int						mCurrentLineNum;
	SexyString				mCurrentLine;
	DataElementMap			mDefineMap;

public:
	virtual bool                    Error(const SexyString& theError);
	virtual DataElement*	                Dereference(const SexyString& theString);
	bool					IsImmediate(const SexyString& theString);
	SexyString					Unquote(const SexyString& theQuotedString);
	bool					GetValues(ListDataElement* theSource, ListDataElement* theValues);
	SexyString					DataElementToString(DataElement* theDataElement, bool enclose = true);
	bool					DataToString(DataElement* theSource, SexyString* theString);
	bool					DataToKeyAndValue(DataElement* theSource, SexyString* theKey,
								DataElement** theValue);
	bool					DataToInt(DataElement* theSource, int* theInt);
	bool					DataToDouble(DataElement* theSource, double* theDouble);
	bool					DataToBoolean(DataElement* theSource, bool* theBool); //No symbol on iOS, prob still there
	bool					DataToStringVector(DataElement* theSource, SexyStringVector* theStringVector);
	bool					DataToList(DataElement* theSource, ListDataElement* theValues);
	bool					DataToIntVector(DataElement* theSource, IntVector* theIntVector);
	bool					DataToDoubleVector(DataElement* theSource, DoubleVector* theDoubleVector);
	bool					ParseToList(const SexyString& theString, ListDataElement* theList,
								bool expectListEnd, int* theStringPos);
	bool					ParseDescriptorLine(const SexyString& theDescriptorLine);

	// You must implement this one
	virtual bool			        HandleCommand(const ListDataElement& theParams) = 0;
	
public:
	DescParser();
	virtual ~DescParser();	

	virtual bool			        LoadDescriptor(const std::string& theFileName);
};

}

#endif //__DESCPARSER_H__