#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <string>
#include "Common.h"

namespace Sexy
{

typedef std::vector<uchar> ByteVector;

class Buffer //This file 1:1 matches PopCap's from Windows/Bejeweled 3 (pl_d.dll). You can easily replace it with the original file if you want, note it is missing stuff seen in newer engine revisions.
{
public:
	ByteVector				mData;
	int						mDataBitSize;
	mutable int				mReadBitPos;
	mutable int				mWriteBitPos;	

public:
	Buffer();
	virtual ~Buffer();
			
	void					SeekFront() const;
	void					Clear();

	void					FromWebString(const std::string& theString); //Unimplemented on XNA
	void					WriteByte(uchar theByte);
	void					WriteNumBits(int theNum, int theBits);
	static int				GetBitsRequired(int theNum, bool isSigned);
	void					WriteBoolean(bool theBool);
	void					WriteShort(short theShort);
	void					WriteLong(long theLong);
	void					WriteString(const std::string& theString);
	void					WriteUTF8String(const std::wstring& theString);
	void					WriteLine(const std::string& theString);	
	void					WriteBuffer(const ByteVector& theBuffer);
	void					WriteBuffer(const Buffer& theBuffer);
	void					WriteBytes(const uchar* theByte, int theCount);
	void					SetData(const ByteVector& theBuffer);
	void					SetData(uchar* thePtr, int theCount);

	std::string				ToWebString() const; //Unimplemented on XNA
	std::wstring			UTF8ToWideString() const; //Unimplemented on XNA
	uchar					ReadByte() const;
	int						ReadNumBits(int theBits, bool isSigned) const;
	bool					ReadBoolean() const;
	short					ReadShort() const;
	long					ReadLong() const;
	std::string				ReadString() const;	
	std::wstring			ReadUTF8String() const;
	std::string				ReadLine() const;
	void					ReadBytes(uchar* theData, int theLen) const;
	void					ReadBuffer(ByteVector* theByteVector) const;
	void					ReadBuffer(Buffer* theBuffer) const;

	const uchar*			GetDataPtr() const;
	int						GetDataLen() const;	
	int						GetDataLenBits() const;
	ulong					GetCRC32(ulong theSeed = 0) const; //Unimplemented on XNA

	bool					AtEnd() const;
	bool					PastEnd() const;

#ifdef _SEXYDECOMP_USE_LATEST_CODE
	FPoint					ReadFPoint(); const
	SexyVector2				ReadVector2(); const //XNA only?
	int						GetBitsAvailable();
	int						GetBytesAvailable();
	int64					ReadInt64(); const
	int						ReadInt32(); const
	short					ReadInt16(); const //So why do you have this when readshort exists?
	byte					ReadInt8(); const
	double					ReadDouble(); const
	float					ReadFloat(); const
	SexyString				ReadSexyString() const;
	int						ReadFPoint();
	int						WriteInt64(long long);
	int						WriteInt32(long);
	int						WriteInt16(short);
	int						WriteInt8(signed char);
	int						WriteDouble(double theDouble);
	int						WriteSexyString(SexyString& theString); //Unimplemented on XNA
	SexyTransform2D			ReadTransform2D();
	//static int			GetNextUTF8CharFromStream(const char** theBuffer, int theLen, wchar_t* theChar); //Did they move this from Common?
	//static int			GetNextUTF8CharFromStream(const char** theBuffer, int start = 0, int theLen, wchar_t* theChar); //Did they move this from Common?
	int						GetCurrReadBytePos(); const
	int						GetCurrWriteBytePos(); const
	void					SeekReadBit(int pos);
	void					SeekReadByte(int pos);
	void					SeekWriteBit(int pos);
	void					SeekWriteByte(int pos); const
	void					Seek(SeekMode mode = SeekMode_eReadStart);
	void					Seek(SeekMode mode, uint bytePos); const
	void					Resize(uint bytes);
#endif
};

}

#endif //__BUFFER_H__
