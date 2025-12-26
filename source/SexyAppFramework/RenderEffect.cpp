#include "RenderEffect.h"
#include "SexyAppBase.h"

using namespace Sexy;

bool RenderEffectDefinition::LoadFromMem(ulong inDataLen, const void* inData, const char* inSrcFileName, const char* inDataFormat) //Unlimplemented in XNA | 46-55
{
	mData.reserve(inDataLen);
	mData.resize(inDataLen);
	if (inDataLen)
		memcpy(&mData[0], inData, inDataLen);
	mSrcFileName = inSrcFileName;
	mDataFormat = inDataFormat;

	return true;
}

bool RenderEffectDefinition::LoadFromFile(const char* inFileName, const char* inSrcFileName) //Unlimplemented in XNA | 58-89
{
	std::string aFileName = GetFileDir(inFileName, true) + GetFileName(inFileName, true);
#ifdef _WIN32
	aFileName += ".d3dfx"; //Load Windows Direct3D shaders
#else
	aFileName += ".popfx"; //Load multiplatform shader format
#endif
	Buffer buf;
	if (gSexyAppBase->ReadBufferFromFile(aFileName, &buf, true))
	{
		std::string anExt;
		int aDotPos = aFileName.rfind('.');
		if (aDotPos != -1)
			anExt = Lower(aFileName.substr((aDotPos)));
		if (anExt.length() > 1)
			anExt = anExt.substr((1));
		LoadFromMem(buf.GetDataLen(), buf.GetDataPtr(), inSrcFileName, anExt.c_str());
	}
	else
		return false;
}