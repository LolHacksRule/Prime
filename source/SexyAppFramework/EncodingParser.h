#ifndef __ENCODINGPARSER_H__
#define __ENCODINGPARSER_H__

#include "Common.h"

struct PFILE;

namespace Sexy
{
	typedef std::vector<SexyChar> WcharBuffer; //moved from XMLParser

	class EncodingParser
	{
	protected:
		PFILE*					mFile;

	private:
		WcharBuffer				mBufferedText;
		bool					(EncodingParser::*mGetCharFunc)(SexyChar* theChar, bool* error);
		bool					mForcedEncodingType;
		bool					mFirstChar;
		bool					mByteSwap;

		bool					GetAsciiChar(SexyChar* theChar, bool* error);
		bool					GetUTF8Char(SexyChar* theChar, bool* error);
		bool					GetUTF16Char(SexyChar* theChar, bool* error);
		bool					GetUTF16LEChar(SexyChar* theChar, bool* error);
		bool					GetUTF16BEChar(SexyChar* theChar, bool* error);

	public:
		enum EncodingType //UTF32 is used but not defined here
		{
			ASCII,
			UTF_8,
			UTF_16,
			UTF_16_LE,
			UTF_16_BE
		};

		enum GetCharReturnType
		{
			SUCCESSFUL,
			INVALID_CHARACTER,
			END_OF_FILE,
			FAILURE				// general case failures
		};
	public:
		EncodingParser();
		virtual ~EncodingParser();

		virtual void					SetEncodingType(EncodingType theEncoding);
		virtual bool					OpenFile(const std::string& theFileName);
		virtual bool					CloseFile();
		virtual bool					EndOfFile();
		virtual void					SetStringSource(const SexyString& theString);
		void							SetStringSource(const std::string& theString);

		virtual GetCharReturnType		GetChar(SexyChar* theChar);
		virtual bool					PutChar(const SexyChar& theChar);
		virtual bool					PutString(const SexyString& theString);
	};
}
#endif